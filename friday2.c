#include "friday2.h"

#include <stdio.h>

#include <stdlib.h>

F_LaneEntry(Wave)
{
	f_s32 RandomLaneValue = Wave->Lane;

	f_s32 TotalWorkToDo = F_WaveSum(Wave, RandomLaneValue);
	printf("Lane %llu value is: %d | PrefixSum: %llu | TotalWorkToDo: %d\n", Wave->Lane, RandomLaneValue, F_WaveExclusivePrefixSum(Wave,  RandomLaneValue), TotalWorkToDo);


	f_u64 *SomeArray;
	if(F_WaveIsFirstLane(Wave))
	{
		SomeArray = F_ArenaPushArrayType(Wave->GlobalArena, TotalWorkToDo, f_u64);
	}

	F_BroadcastFirstLane(Wave, SomeArray);

	f_s32 Start = F_WaveExclusivePrefixSum(Wave, RandomLaneValue);
	for(int I = Start; I < Start + RandomLaneValue; I++)
	{
		SomeArray[I] = Wave->Lane;
	}

	if(F_WaveIsFirstLane(Wave))
	{
		for(int I = 0; I < TotalWorkToDo; I++)
		{
			printf("SomeArray[%d] = %llu\n", I, SomeArray[I]);
		}
	}

	f_str8_array FilesToWatch;
	if(F_WaveIsFirstLane(Wave))
	{
		f_str8_list FileList = F_OS_ListOfEntriesInDirectory(Wave->GlobalArena, F_Str8("source"), .ExcludeDirectories = F_True, .PrependPath = F_True);
		FilesToWatch = F_Str8ArrayFromList(Wave->GlobalArena, FileList);
		for(int I = 0; I < FilesToWatch.Count; I++)
		{
			printf("FileToWatch: %.*s\n", F_Expand(FilesToWatch.Items[I]));
		}
	}

	F_BroadcastFirstLane(Wave, FilesToWatch);

	f_watch Watch = F_WatchFilesFromArray(Wave, FilesToWatch);


	// f_u64 NumberOfChecks = 0;
	// int FilesChangedCount = 0;
	// for(;FilesChangedCount < 3;)
	// {

	// 	f_str8_array FilesChanged = F_FilesChangedInWatch(Wave->Arena, Wave, &Watch, F_DEFAULT_TIMEOUT_MS);

	// 	if(FilesChanged.Count > 0)
	// 	{
	// 		if(F_WaveIsFirstLane(Wave))
	// 		{
	// 			for(int I = 0; I < FilesChanged.Count; I++)
	// 			{
	// 				FilesChangedCount++;
	// 			}
	// 		}
	// 	}
	// 	F_BroadcastFirstLane(Wave, FilesChangedCount);
	// }

	F_WaveBarrier(Wave);


	if(F_WaveIsFirstLane(Wave))
	{
		f_compile_options CompileOptions = {0};
		CompileOptions.OutputName = F_Str8("friday2isawesome");
		f_compilation Compilation = F_CompileTU(Wave->GlobalArena, F_Str8("friday2.c"), CompileOptions, F_TOOLCHAIN_CLANG);

		F_WaitForProcess(&Compilation.Process);

		f_str8_array ObjectFiles = {0};
		ObjectFiles.Items = &Compilation.ObjectPath;
		ObjectFiles.Count = 1;
		f_link_options LinkOptions = {0};
		LinkOptions.OutputName = F_Str8("friday2isawesomeexe");
		f_link Link = F_LinkTUs(Wave->GlobalArena, ObjectFiles, LinkOptions, F_TOOLCHAIN_CLANG);

	}
	F_WaveBarrier(Wave);
}

int main(int ArgCount, char **Arguments)
{
	F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
}