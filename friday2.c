#define FRIDAY_IMPLEMENTATION
#include "friday2.h"

#include <stdio.h>

#include <stdlib.h>

F_LaneEntry(Wave)
{

	f_compile_options CompileOptions = {0};
	f_link_options    LinkOptions    = {0};
	if(F_WaveIsFirstLane(Wave))
	{
		F_OS_MakeDirectoryIfNotAlreadyMade(Wave->GlobalArena, F_Str8("build"));
	}

	CompileOptions.ExtraFlags = F_Str8("-framework Cocoa -framework Metal -framework QuartzCore ");
	CompileOptions.Debug = F_True;

	f_str8_array IncludePaths = F_Str8ArrayFromMany(Wave->Arena,
	F_Str8("include/"),
	F_Str8("external/"),
	F_Str8("build/"),
	);

	CompileOptions.IncludePaths = IncludePaths;

	f_str8_array SourceFiles = {0};
	if(F_WaveIsFirstLane(Wave))
	{
		f_str8_list SourceFileList = {0};
		F_Str8ListAppendMany(Wave->GlobalArena, &SourceFileList,
			F_OS_ListOfEntriesInDirectory(
				Wave->GlobalArena,
				F_Str8("source"),
				.ExcludeDirectories = F_True,
				.PrependPath = F_True,
			)
		);
		F_Str8ListAppendMany(Wave->GlobalArena, &SourceFileList,
			F_OS_ListOfEntriesInDirectory(
				Wave->GlobalArena,
				F_Str8("source/darwin"),
				.ExcludeDirectories = F_True,
				.PrependPath = F_True,
			)
		);
		F_Str8ListAppendMany(Wave->GlobalArena, &SourceFileList,
			F_OS_ListOfEntriesInDirectory(
				Wave->GlobalArena,
				F_Str8("external"),
				.ExcludeDirectories = F_True,
				.PrependPath = F_True,
			)
		);

		SourceFiles = F_Str8ArrayFromList(Wave->GlobalArena, SourceFileList);
	}

	F_BroadcastFirstLane(Wave, SourceFiles);


	for F_EachWaveSplit(Wave, Index, SourceFiles.Count)
	{
		printf("filename %.*s\n", F_Expand(SourceFiles.Items[Index]));
		f_file File = F_OS_FileFromFilename(Wave->Arena, SourceFiles.Items[Index]);
		f_str8 Source = F_OS_ReadEntireFile(Wave->Arena, &File);
		F_OS_ReleaseFile(&File);
		f_u64 Hash = F_HashSourceContentsAndIncludes(Wave->Arena, Source, IncludePaths);
		printf("%.*s hash: %llu\n", F_Expand(SourceFiles.Items[Index]), Hash);
	}

	F_WaveBarrier(Wave);

}

int main(int ArgCount, char **Arguments)
{
	//F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
	F_DispatchAcrossNLanes(1);
}