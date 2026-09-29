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

	f_str8_list SourceFilesList = {0};
	f_build_options BuildOptions = {0};

	BuildOptions.LinkFlags = F_Str8ArrayFromCStrs(Lane.PermanentArena, "-fobjc-arc");

	BuildOptions.OutputPathWithFilename = F_Str8("build/app.dynlib");

	BuildOptions.IncludeDirectories = F_Str8ArrayFromCStrs(Lane.PermanentArena,
	"include/",
	"external/",
	"build/",
	);

	F_FirstLane()
	{
		F_Str8ListAppendMany(
			Lane.PermanentArena, &SourceFilesList,
			F_OS_ListOfFilesInDirectory(Lane.PermanentArena, "source/",
			.PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromCStrs(Lane.PermanentArena, ".c"))
		);

		F_Str8ListAppendMany(
			Lane.PermanentArena, &SourceFilesList,
			F_OS_ListOfFilesInDirectory(Lane.PermanentArena, "source/darwin/",
			.PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromCStrs(Lane.PermanentArena, ".c", ".m"))
		);

		F_Str8ListAppend(Lane.PermanentArena, &SourceFilesList, F_Str8("app.c"));
	}

	F_BroadcastFirstLane(SourceFilesList);

	BuildOptions.Kind = F_OUTPUT_DYNAMIC;

	BuildOptions.SourceFiles = F_Str8ArrayFromList(Lane.PermanentArena, SourceFilesList);

	f_build Build = {0};
	F_FirstLane() Build = F_BuildFromBuildOptions(BuildOptions);
	F_BroadcastFirstLane(Build);
	while(F_True)
	{

		F_ArenaReset(Lane.TemporaryArena);
	}
}