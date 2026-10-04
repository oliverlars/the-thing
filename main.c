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

f_group GROUP_HACK = { 0 };


// HACK: Fix this 
f_b32 F_IsBuildLane()
{
	return F_True;
}
f_b32 F_IsUpdateLane()
{
	return F_True;
}

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
		f_build_cache BuildCache = {0};
		F_FirstLane() BuildCache = F_BuildCacheFromOutputPath(GROUP_HACK, Lane.TemporaryArena, F_Str8("build/"));
		F_BroadcastFirstLane(BuildCache);
		if(F_IsUpdateLane())
		{
			for(f_s32 I = 0; I < 256; I++)
			{
				
			}
		}
		if(F_IsBuildLane())
		{
			printf("BuildLane: %d\n", BuildCache.Count);
			f_range Range = F_SplitRange(GROUP_HACK, BuildCache.Count);
			printf("Range: [%d, %d]\n", Range.Start, Range.End);
		// 	atomic_store(&Build->TUsToProcess, 0);
		// 	atomic_fetch_add_explicit(&Build->TUsToProcess, FutureWork, memory_order_release);
		// 	FutureWork = 0;
		// }
		// F_WaveBarrier();
		// F_ArenaReset(Lane.TemporaryArena);
		}
		F_WaveBarrier();
	}
}
