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

	f_str8_array SourceFiles = {0};


	F_FirstLane()
	{
		F_Str8ListAppendMany(
			Lane.Arena, &SourceFiles,
			F_OS_ListOfFilesInDirectory(Lane.Arena, F_Str8("source"),
			.PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromCStrs(".c"))
		);

		F_Str8ListAppendMany(
			Lane.Arena, &SourceFiles,
			F_OS_ListOfFilesInDirectory(Lane.Arena, F_Str8("source/darwin"),
			.PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromCStrs(".c", ".m"))
		);
	}
}