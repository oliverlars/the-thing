#define FRIDAY_IMPLEMENTATION
#include "friday2.h"

#include <stdio.h>

#include <stdlib.h>


#if 1
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



	f_b32 CompiledSuccessfully = F_True;
	f_str8_array ObjectFiles = {0};
	ObjectFiles.Count = SourceFiles.Count;
	if(F_WaveIsFirstLane(Wave))
	{
		ObjectFiles.Items = F_ArenaPushArrayType(Wave->GlobalArena, SourceFiles.Count, f_str8);
	}
	F_BroadcastFirstLane(Wave, ObjectFiles);


	f_u64 CompileStartTime = F_OS_TimeStamp();
	f_u64 NumberOfTUsRecompiled = 0;
	for F_EachWaveSplit(Wave, Index, SourceFiles.Count)
	{
		f_u64 Hash = F_HashSourceAndIncludes(Wave->Arena, SourceFiles.Items[Index], IncludePaths);
		Hash ^= F_HashCompileOptions(CompileOptions);
		f_str8 ObjectFile = F_Str8PushF(Wave->Arena, "%llu.o", Hash);
		ObjectFiles.Items[Index] = F_Str8PushF(Wave->Arena, "%.*s/%.*s", F_Expand(CompileOptions.OutputPath), F_Expand(ObjectFile));
		if(!F_OS_FileInfoFromFilename(Wave->Arena, ObjectFiles.Items[Index]).Exists)
		{
			CompileOptions.OutputName = ObjectFile;
			f_compilation Compilation = F_CompileTU(Wave->Arena, SourceFiles.Items[Index], CompileOptions, F_TOOLCHAIN_CLANG);
			F_WaitForProcess(&Compilation.Process);
			CompiledSuccessfully &= F_ProcessExitedCleanly(&Compilation.Process);
			NumberOfTUsRecompiled++;
		}
	}

	NumberOfTUsRecompiled = F_WaveSum(Wave, NumberOfTUsRecompiled);

	CompiledSuccessfully = F_WaveAllTrue(Wave, CompiledSuccessfully);
	f_u64 CompileTotalTime = F_OS_TimeStamp() - CompileStartTime;

	f_b32 FinishedSuccessfully = CompiledSuccessfully;
	if(F_WaveIsFirstLane(Wave))
	{
		f_u64 LinkStartTime = F_OS_TimeStamp();
		if(CompiledSuccessfully)
		{
			f_link Link = F_LinkTUs(Wave->GlobalArena, ObjectFiles, LinkOptions, F_TOOLCHAIN_CLANG);
			F_WaitForProcess(&Link.Process);
			FinishedSuccessfully = F_ProcessExitedCleanly(&Link.Process);
		}
		f_str8_array StatList = F_Str8ArrayFromMany(
			Wave->GlobalArena,
			F_Str8PushF(Wave->GlobalArena, "compiled %d TUs in %.2fms",  NumberOfTUsRecompiled, F_MilliSecondsFromTimeStamp(CompileTotalTime)),
			F_Str8PushF(Wave->GlobalArena, "linked in %.2fms",  F_MilliSecondsFromTimeStamp(F_OS_TimeStamp() - LinkStartTime))
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
		f_str8_list Entries = F_OS_ListOfEntriesInDirectory(Wave->Arena, F_Str8("source"), .PrependPath = F_True, .FilterByExtension = F_Str8ArrayFromMany(Wave->Arena, F_Str8(".c")));
		for(f_str8_list_node *Node = Entries.First; Node; Node = Node->Next)
		{
			printf("%.*s\n", F_Expand(Node->String));
		}
		f_process Process = {0};
		f_str8_list Args = {0};
		F_Str8ListAppend(Wave->Arena, &Args, F_Str8("test.exe"));
		F_StartProcess(Wave->Arena, &Process, Args);
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

int main(int ArgCount, char **Arguments)
{
	F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
}