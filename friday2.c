#include "friday2.h"

#include <stdio.h>

F_LaneEntry(Wave)
{
	f_u64 *SomeArray;
	f_u64  SomeArrayCount;
	if(F_WaveIsFirstLane(Wave))
	{
		SomeArrayCount = Wave->LaneCount * 3;
		SomeArray = F_ArenaPushArrayType(Wave->GlobalArena, SomeArrayCount, f_u64);
	}

	F_WaveBarrier(Wave);

	F_BroadcastFirstLane(Wave, SomeArrayCount);
	F_BroadcastFirstLane(Wave, SomeArray);

	printf("Lane %llu has value: %llu\n", Wave->Lane, SomeArrayCount);

	F_WaveBarrier(Wave);

	for F_EachWaveSplit(Wave, I, SomeArrayCount)
	{
		printf("I: %llu\n", I);
		SomeArray[I] = Wave->Lane;
	}

	if(F_WaveIsFirstLane(Wave))
	{
		for(int I = 0; I < SomeArrayCount; I++)
		{
			printf("SomeArray[%d] = %llu\n", I, SomeArray[I]);
		}
	}

	f_str8_array FilesToWatch;
	if(F_WaveIsFirstLane(Wave))
	{
		f_str8_list FileList;
		F_Str8ListAppend(Wave->GlobalArena, &FileList, F_Str8("friday2.c"));
		F_Str8ListAppend(Wave->GlobalArena, &FileList, F_Str8("friday2.h"));
		FilesToWatch = F_Str8ArrayFromList(Wave->GlobalArena, FileList);
	}

	F_BroadcastFirstLane(Wave, FilesToWatch);

	f_watch Watch = F_WatchFilesFromArray(Wave, FilesToWatch);


	// f_u64 NumberOfChecks = 0;
	// while(!F_AnyFilesChangedInWatch(Wave, &Watch))
	// {
	// 	if(F_WaveIsFirstLane(Wave))
	// 	{
	// 		printf("Nothing changed: %llu\n", NumberOfChecks);
	// 		NumberOfChecks++;
	// 	}
	// }

	f_process Process = {0};
	f_str8_list Args = {0};
	F_Str8ListAppend(Wave->Arena, &Args, F_Str8("clang"));
	F_ProcessSpawn(Wave->Arena, &Process, Args);
	while(F_ProcessPoll(&Process))
	{
		printf("%.*s", F_Expand(Process.StdOutSinceLastPoll));
	}


	F_WaveBarrier(Wave);
	if(!F_ProcessExitedCleanly(&Process))
	{
		printf("Clang exited with errors\n");
	}

	F_WaveBarrier(Wave);
}


int main(int ArgCount, char **Arguments)
{
	F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
}