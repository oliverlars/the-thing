#define FRIDAY_IMPLEMENTATION
#define FRIDAY_MAIN
#include "friday2.h"

#include <stdio.h>

#include <stdlib.h>


#if 1
F_LaneEntry
{

	f_compile_options CompileOptions = {0};
	f_link_options    LinkOptions    = {0};
	if(F_IsFirstLane())
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
	CompileOptions.OutputPath = F_Str8("build");

	LinkOptions.OutputPath = F_Str8("build/");
	LinkOptions.OutputName = F_Str8("the-thing2");

	f_str8_array SourceFiles = {0};
	if(F_IsFirstLane())
	{
		f_str8_list SourceFileList = {0};
		F_Str8ListAppendMany(Lane.Arena, &SourceFileList,
			F_OS_ListOfEntriesInDirectory(
				Lane.Arena,
				F_Str8("source"),
				.ExcludeDirectories = F_True,
				.PrependPath = F_True,
			)
		);
		F_Str8ListAppendMany(Lane.Arena, &SourceFileList,
			F_OS_ListOfEntriesInDirectory(
				Lane.Arena,
				F_Str8("source/darwin"),
				.ExcludeDirectories = F_True,
				.PrependPath = F_True,
			)
		);
		F_Str8ListAppendMany(Lane.Arena, &SourceFileList,
			F_OS_ListOfEntriesInDirectory(
				Lane.Arena,
				F_Str8("external"),
				.ExcludeDirectories = F_True,
				.PrependPath = F_True,
				.FilterByExtension = F_Str8ArrayFromCStrs(Lane.Arena, ".c"),
			)
		);

		F_Str8ListAppend(Lane.Arena, &SourceFileList, F_Str8("main.c"));
		F_Str8ListAppend(Lane.Arena, &SourceFileList, F_Str8("build/embedded_fonts.c"));

		SourceFiles = F_Str8ArrayFromList(Lane.Arena, SourceFileList);
	}

	F_BroadcastFirstLane(SourceFiles);



	f_b32 CompiledSuccessfully = F_True;
	f_str8_array ObjectFiles = {0};
	ObjectFiles.Count = SourceFiles.Count;
	if(F_IsFirstLane())
	{
		ObjectFiles.Items = F_ArenaPushArrayType(Lane.Arena, SourceFiles.Count, f_str8);
	}
	F_BroadcastFirstLane(ObjectFiles);


	f_u64 CompileStartTime = F_OS_TimeStamp();
	f_u64 NumberOfTUsRecompiled = 0;
	for F_EachInLaneSpan(Index, SourceFiles.Count)
	{
		f_u64 Hash = F_HashSourceAndIncludes(Lane.Arena, SourceFiles.Items[Index], IncludePaths);
		Hash ^= F_HashCompileOptions(CompileOptions);
		f_str8 ObjectFile = F_Str8PushF(Lane.Arena, "%llu.o", Hash);
		ObjectFiles.Items[Index] = F_Str8PushF(Lane.Arena, "%.*s/%.*s", F_Expand(CompileOptions.OutputPath), F_Expand(ObjectFile));
		if(!F_OS_FileInfoFromFilename(Lane.Arena, ObjectFiles.Items[Index]).Exists)
		{
			CompileOptions.OutputName = ObjectFile;
			f_compilation Compilation = F_CompileTU(Lane.Arena, SourceFiles.Items[Index], CompileOptions, F_TOOLCHAIN_CLANG);
			F_WaitForProcess(&Compilation.Process);
			CompiledSuccessfully &= F_ProcessExitedCleanly(&Compilation.Process);
			NumberOfTUsRecompiled++;
		}
	}

	NumberOfTUsRecompiled = F_WaveSum(NumberOfTUsRecompiled);

	CompiledSuccessfully = F_WaveAllTrue(CompiledSuccessfully);
	f_u64 CompileTotalTime = F_OS_TimeStamp() - CompileStartTime;

	f_b32 FinishedSuccessfully = CompiledSuccessfully;
	if(F_IsFirstLane())
	{
		f_u64 LinkStartTime = F_OS_TimeStamp();
		if(CompiledSuccessfully)
		{
			f_link Link = F_LinkTUs(Lane.Arena, ObjectFiles, LinkOptions, F_TOOLCHAIN_CLANG);
			F_WaitForProcess(&Link.Process);
			FinishedSuccessfully = F_ProcessExitedCleanly(&Link.Process);
		}
		f_str8_array StatList = F_Str8ArrayFromStr8s(
			Lane.Arena,
			F_Str8PushF(Lane.Arena, "compiled %d TUs in %.2fms",  NumberOfTUsRecompiled, F_MilliSecondsFromTimeStamp(CompileTotalTime)),
			F_Str8PushF(Lane.Arena, "linked in %.2fms",  F_MilliSecondsFromTimeStamp(F_OS_TimeStamp() - LinkStartTime))
		);
		if(FinishedSuccessfully)
		{
			F_PassedBanner(StatList);
		}
		else
		{
			F_FailedBanner(StatList);
		}
	}
}
#else

F_LaneEntry(Wave)
{
	if(F_WaveIsFirstLane(Wave))
	{
		f_str8_list Entries = F_OS_ListOfEntriesInDirectory(Lane.Arena, F_Str8("source"), .PrependPath = F_True, .FilterByExtension = F_Str8ArrayFromMany(Lane.Arena, F_Str8(".c")));
		for(f_str8_list_node *Node = Entries.First; Node; Node = Node->Next)
		{
			printf("%.*s\n", F_Expand(Node->String));
		}
		f_process Process = {0};
		f_str8_list Args = {0};
		F_Str8ListAppend(Lane.Arena, &Args, F_Str8("test.exe"));
		F_StartProcess(Lane.Arena, &Process, Args);
		int SomeValue = 0;
		for(;F_PollProcess(&Process);)
		{
			printf("%.*s", F_Expand(Process.StdOutSinceLastPoll));
			SomeValue = 10;
		}
		if(F_ProcessExitedCleanly(&Process))
		{
			printf("EXITED CLEANLY");
		}
	}
}

#endif