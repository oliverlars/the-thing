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

	LinkOptions.ExtraFlags = F_Str8ArrayFromMany(Wave->Arena, 
		F_Str8("-framework"),  F_Str8("Cocoa"), F_Str8("-framework"), F_Str8("Metal"), F_Str8("-framework"), F_Str8("QuartzCore")
	);
	CompileOptions.Debug = F_True;

	f_str8_array IncludePaths = F_Str8ArrayFromMany(Wave->Arena,
	F_Str8("include/"),
	F_Str8("external/"),
	F_Str8("build/"),
	);

	CompileOptions.IncludePaths = IncludePaths;
	CompileOptions.OutputPath = F_Str8("build");

	LinkOptions.OutputPath = F_Str8("build/");
	LinkOptions.OutputName = F_Str8("the-thing2");

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
				.FilterByExtension = F_Str8ArrayFromMany(Wave->GlobalArena, F_Str8(".c")),
			)
		);

		F_Str8ListAppend(Wave->GlobalArena, &SourceFileList, F_Str8("main.c"));
		F_Str8ListAppend(Wave->GlobalArena, &SourceFileList, F_Str8("build/embedded_fonts.c"));

		SourceFiles = F_Str8ArrayFromList(Wave->GlobalArena, SourceFileList);
	}

	F_BroadcastFirstLane(Wave, SourceFiles);



	f_b32 ShouldLink = F_True;
	f_str8_array ObjectFiles = {0};
	ObjectFiles.Count = SourceFiles.Count;
	if(F_WaveIsFirstLane(Wave))
	{
		ObjectFiles.Items = F_ArenaPushArrayType(Wave->GlobalArena, SourceFiles.Count, f_str8);
	}
	F_BroadcastFirstLane(Wave, ObjectFiles);

	for F_EachWaveSplit(Wave, Index, SourceFiles.Count)
	{
		printf("filename %.*s\n", F_Expand(SourceFiles.Items[Index]));
		f_file File = F_OS_FileFromFilename(Wave->Arena, SourceFiles.Items[Index]);
		f_str8 Source = F_OS_ReadEntireFile(Wave->Arena, &File);
		F_OS_ReleaseFile(&File);
		f_u64 Hash = F_HashSourceContentsAndIncludes(Wave->Arena, Source, IncludePaths);
		f_str8 ObjectFile = F_Str8PushF(Wave->Arena, "%llu.o", Hash);
		ObjectFiles.Items[Index] = F_Str8PushF(Wave->Arena, "%.*s/%.*s", F_Expand(CompileOptions.OutputPath), F_Expand(ObjectFile));
		printf("FILE:HASH | %.*s:%llu\n", F_Expand(SourceFiles.Items[Index]), Hash);
		//if(!F_OS_FileInfoFromFilename(Wave->Arena, ObjectFiles.Items[Index]).Exists)
		{
			CompileOptions.OutputName = ObjectFile;
			f_compilation Compilation = F_CompileTU(Wave->Arena, SourceFiles.Items[Index], CompileOptions, F_TOOLCHAIN_CLANG);
			F_WaitForProcess(&Compilation.Process);
			ShouldLink &= F_ProcessExitedCleanly(&Compilation.Process);
		}
	}

	ShouldLink = F_WaveAllTrue(Wave, ShouldLink);

	if(F_WaveIsFirstLane(Wave))
	{
		if(ShouldLink)
		{
			f_link Link = F_LinkTUs(Wave->GlobalArena, ObjectFiles, LinkOptions, F_TOOLCHAIN_CLANG);
			F_WaitForProcess(&Link.Process);
		}
	}
}

int main(int ArgCount, char **Arguments)
{
	//F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
	F_DispatchAcrossNLanes(1);
}

/*

clang  -framework Cocoa -framework Metal -framework QuartzCore  -o build/the-thing build/9540935867446464531.o build/43993904911722607.o build/5609599489510449241.o build/5212239408225621290.o build/7136770315221062523.o build/6370771780646960387.o build/1119850315201764335.o build/14831241361877324663.o build/15492044181874365471.o build/1344928503081225400.o build/8005342778149996573.o build/2487373655189650430.o 

clang  -framework Cocoa -framework Metal -framework QuartzCore  -o build/the-thing2 build/11992984539254367672.o build/17559955129005566269.o build/11138700977964269690.o build/4556935579390488136.o build/16909743166938913828.o build/9007981636814547503.o build/15765771869898447467.o build/12550837958979098442.o build/18281941688440889440.o build/1728623339491047175.o build/5834215734801951731.o build/10680307360561197557.o build/3204253810457534359.o
*/