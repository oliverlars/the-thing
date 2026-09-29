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

	AppCompileOptions.ExtraFlags = F_Str8ArrayFromCStrs(Lane.PermanentArena, "-fobjc-arc");

	f_str8_array IncludePaths = F_Str8ArrayFromCStrs(Lane.PermanentArena,
	"include/",
	"external/",
	"build/",
	);

	AppCompileOptions.IncludePaths = IncludePaths;

	f_str8_list SourceFilesList = {0};


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

	AppLinkOptions.OutputKind = F_OUTPUT_DYNAMIC;

	F_BroadcastFirstLane(AppLinkOptions);

	f_str8_array SourceFiles = F_Str8ArrayFromList(Lane.PermanentArena, SourceFilesList);
	f_range Range = F_SplitRange(SourceFiles.Count);
	SourceFiles.Items += Range.Start;
	SourceFiles.Count = Range.End - Range.Start;
	f_build_watch BuildWatch = F_BuildWatchFromSourceFiles(Lane.PermanentArena, SourceFiles, AppCompileOptions.IncludePaths);
	
	f_compilation *Compilation;
	F_FirstLane()
	{
		Compilation = F_ArenaPushType(Lane.PermanentArena, f_compilation);
	}
	F_BroadcastFirstLane(Compilation);

	while(F_True)
	{
		
		F_ArenaReset(Lane.TemporaryArena);
	}
}