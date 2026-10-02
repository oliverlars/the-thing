#include "misc.h"
#include "memory.h"
#include "strings.h"
#include "log.h"
#include "maths.h"
#include "os.h"

#include "app.h"

#define FRIDAY_IMPLEMENTATION
#define FRIDAY_MAIN
#include "friday2.h"

F_LaneEntry
{
	f_build_options BuildOptions = {0};

	BuildOptions.LinkFlags = F_Str8ArrayFromCStrs(Lane.PermanentArena, "-fobjc-arc");

	BuildOptions.OutputPathWithFilename = F_Str8("build/app.dynlib");

	BuildOptions.IncludeDirectories = F_Str8ArrayFromCStrs(Lane.PermanentArena,
	"include/",
	"external/",
	"build/",
	);

	/*
	app.c and app.h need moving to folders to make this new assumption work better
	*/
	BuildOptions.SourceDirectories = F_Str8ArrayFromCStrs(Lane.PermanentArena, "source/", "source/darwin/", "external/");
	BuildOptions.Kind = F_OUTPUT_DYNAMIC;

	f_build *Build = 0;
	F_FirstLane()
	{
		Build = F_BuildFromBuildOptions(Lane.PermanentArena, BuildOptions);
		atomic_store_explicit(&Build->TUsToProcess, 3, memory_order_release);
	}
	F_BroadcastFirstLane(Build);
	f_s32 FutureWork = 3;
	for(;;)
	{
		if(F_IsUpdateLane())
		{
			for(f_s32 I = 0; I < 256; I++)
			{
				for(;;)
				{
					f_s64 NextTU = atomic_fetch_sub_explicit(&Build->TUsToProcess, 1, memory_order_release);

					if(NextTU <= 0) break;
					printf("Lane %d processing TU %d\n", Lane.ID, NextTU - 1);
				}
			}
		}
		if(F_IsIOLane())
		{
			atomic_store(&Build->TUsToProcess, 0);
			atomic_fetch_add_explicit(&Build->TUsToProcess, FutureWork, memory_order_release);
			FutureWork = 0;
		}
		F_WaveBarrier();
		F_ArenaReset(Lane.TemporaryArena);
	}
}