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

	BuildOptions.LinkFlags = F_Str8ArrayFromCStrs(F_PArena(), "-fobjc-arc");

	BuildOptions.OutputPathWithFilename = F_Str8("build/app.dynlib");

	BuildOptions.IncludeDirectories = F_Str8ArrayFromCStrs(F_PArena(),
	"include/",
	"external/",
	"build/",
	);

	/*
	app.c and app.h need moving to folders to make this new assumption work better
	*/
	BuildOptions.SourceDirectories = F_Str8ArrayFromCStrs(F_PArena(), "source/", "source/darwin/", "external/");
	BuildOptions.Kind = F_OUTPUT_DYNAMIC;

	assert(F_DefaultGroup().Count > 0);

	f_build *Build = 0;
	if(F_DefaultGroup().ID == 0)
	{
		Build = F_BuildFromBuildOptions(F_PArena(), BuildOptions);
		atomic_store_explicit(&Build->TUsToProcess, 3, memory_order_release);
	}
	Build = F_ReadFromGroupID(F_DefaultGroup(), 0, Build);
	f_s32 FutureWork = 3;

	f_group UpdateGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID == 0);
	f_group BuildGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID > 0);
	assert(UpdateGroup.Count == 1);
	for(;;)
	{
		//f_build_cache BuildCache = {0};
		//if(F_DefaultGroup().ID == 0) BuildCache = F_BuildCacheFromOutputPath(F_DefaultGroup(), F_TArena(), F_Str8("build/"));
		//BuildCache = F_ReadFromGroupID(F_DefaultGroup(), 0, BuildCache);

		if(F_IsCallerInGroup(UpdateGroup))
		{
			printf("I'm in the update group!\n");
		}
		if(F_IsCallerInGroup(BuildGroup))
		{
			printf("I'm the build group!\n");
			F_GroupBarrier(BuildGroup);
			if(BuildGroup.ID == 0) printf("This will always be called after every build group lanes first message!\n");
		// 	atomic_store(&Build->TUsToProcess, 0);
		// 	atomic_fetch_add_explicit(&Build->TUsToProcess, FutureWork, memory_order_release);
		// 	FutureWork = 0;
		// }
		// F_WaveBarrier();
		// F_ArenaReset(Lane.TemporaryArena);
		}
		F_GroupBarrier(F_DefaultGroup());
		printf("This will be printed once both build and update lanes have printed their messages!\n");
		break;
	}
}
