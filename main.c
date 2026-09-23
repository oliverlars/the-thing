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
			F_OS_ListOfFilesInDirectory(Lane.Arena, "source/",
			.PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromCStrs(Lane.Arena, ".c"))
		);

		F_Str8ListAppendMany(
			Lane.Arena, &SourceFiles,
			F_OS_ListOfFilesInDirectory(Lane.Arena, "source/darwin/",
			.PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromCStrs(Lane.Arena, ".c", ".m"))
		);
	}
	
	F_BroadcastFirstLane(SourceFiles);

	AppLinkOptions.OutputKind = F_OUTPUT_DYNAMIC;

	F_BroadcastFirstLane(AppLinkOptions);


	f_build_watch BuildWatch = F_BuildWatchFromSourceFiles(Lane.Arena, F_Str8ArrayFromList(Lane.Arena, SourceFiles), AppCompileOptions.IncludePaths);


	while(F_True) F_Temporary(Lane.Arena)
	{
		f_b32 ChangeDetected = F_False;
		f_b32 *Mask = F_MaskOfTUsChanged(Lane.Arena, BuildWatch, 100);
		for F_EachInLaneSpan(I, BuildWatch.TUCount)
		{
			if(Mask[I])
			{
				printf("File change detected\n");
			}
		}
	} 

	printf("build watch %llu\n", BuildWatch.TUCount);

}