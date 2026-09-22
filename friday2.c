#define FRIDAY_IMPLEMENTATION
#define FRIDAY_SINGLE_THREAD
#define FRIDAY_MAIN
#include "friday2.h"

#include <stdio.h>

#include <stdlib.h>

F_LaneEntry
{
	f_compile_options CompileOptions = {0};
	f_link_options    LinkOptions    = {0};
	f_build_cache     BuildCache     = {0};
	F_FirstLane()
	{
		F_OS_MakeDirectoryIfNotAlreadyMade(Lane.Arena, F_Str8("build"));
	}

	LinkOptions.ExtraFlags = F_Str8ArrayFromCStrs(Lane.Arena, 
		"-framework",  "Cocoa", "-framework", "Metal", "-framework", "QuartzCore"
	);
	CompileOptions.Debug = F_True;

	CompileOptions.ExtraFlags = F_Str8ArrayFromCStrs(Lane.Arena, "-fobjc-arc");

	f_str8_array IncludePaths = F_Str8ArrayFromCStrs(Lane.Arena,
	"include/",
	"external/",
	"build/",
	);

	CompileOptions.IncludePaths = IncludePaths;
	CompileOptions.OutputPath = F_Str8("build/");

	LinkOptions.OutputPath = F_Str8("build/");
	LinkOptions.OutputName = F_Str8("the-thing2");

	f_str8_array SourceFiles = {0};
	F_FirstLane()
	{
		f_str8_list SourceFileList = {0};
		F_Str8ListAppendMany(Lane.Arena, &SourceFileList,
			F_OS_ListOfFilesInDirectories(
				Lane.Arena,
				.Directories = {"source", "source/darwin", "external"},
				.PrependPath = F_True,
				.FilterByExtension = F_Str8ArrayFromCStrs(Lane.Arena, ".c", ".m"),
			)
		);

		F_Str8ListAppend(Lane.Arena, &SourceFileList, F_Str8("main.c"));
		F_Str8ListAppend(Lane.Arena, &SourceFileList, F_Str8("build/embedded_fonts.c"));

		SourceFiles = F_Str8ArrayFromList(Lane.Arena, SourceFileList);
	}

	F_BroadcastFirstLane(SourceFiles);


	F_FirstLane()
	{
		BuildCache = F_BuildCacheFromOutputPath(Lane.Arena, CompileOptions.OutputPath);
	}

	F_BroadcastFirstLane(BuildCache);


	f_u64 CompileStartTime = F_OS_TimeStamp();

	f_link Link = {0};
	f_compilation Compilation = F_CompileNewAndChangedTUs(Lane.Arena, BuildCache, CompileOptions, SourceFiles, F_TOOLCHAIN_CLANG);
	F_WaitForCompilation(&Compilation);

	f_u64 CompileTotalTime = F_OS_TimeStamp() - CompileStartTime;
	F_FirstLane()
	{
		f_u64 LinkStartTime = F_OS_TimeStamp();
		if(Compilation.Status == F_COMPILE_SUCCEEDED)
		{
			Link = F_LinkTUs(Lane.Arena, LinkOptions, (f_str8_array){Compilation.ObjectPaths, Compilation.Count}, F_TOOLCHAIN_CLANG);

			F_WaitForLinking(&Link);
		}
		f_str8_array StatList = F_Str8ArrayFromStr8s(
			Lane.Arena,
			F_Str8PushF(Lane.Arena, "compiled in %.2fms", F_MilliSecondsFromTimeStamp(CompileTotalTime)),
			F_Str8PushF(Lane.Arena, "linked in %.2fms",  F_MilliSecondsFromTimeStamp(F_OS_TimeStamp() - LinkStartTime))
		);
		if(Link.Status == F_LINK_SUCCEEDED)
		{
			F_PassedBanner(StatList);
		}
		else
		{
			F_FailedBanner(StatList);
		}
	}
}

