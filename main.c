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

	f_build Build = {0};
	F_FirstLane() Build = F_BuildFromBuildOptions(BuildOptions);
	F_BroadcastFirstLane(Build);
	while(F_True)
	{
		F_ArenaReset(Lane.TemporaryArena);
	}
}