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
	
	f_compile_options AppCompileOptions = {0};
	f_link_options    AppLinkOptions  = {0};
	AppCompileOptions.OutputPath = F_Str8("build/app.dynlib");

	f_str8_list SourceFiles = {0};


	F_FirstLane()
	{
		F_Str8ListAppendMany(
			Lane.Arena, &SourceFiles,
			F_OS_ListOfFilesInDirectory(Lane.Arena, "source",
			.PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromCStrs(Lane.Arena, ".c"))
		);

		F_Str8ListAppendMany(
			Lane.Arena, &SourceFiles,
			F_OS_ListOfFilesInDirectory(Lane.Arena, "source/darwin",
			.PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromCStrs(Lane.Arena, ".c", ".m"))
		);
	}

	AppLinkOptions.OutputKind = F_OUTPUT_DYNAMIC;

	f_build_watch BuildWatch = F_BuildWatchFromSourceFiles(Lane.Arena, F_Str8ArrayFromList(Lane.Arena, SourceFiles), AppCompileOptions.IncludePaths);

	printf("build watch %llu\n", BuildWatch.TUCount);
}