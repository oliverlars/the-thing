#include "friday2.h"

#include <stdio.h>

f_str8 BackingFileMemory[32];
f_str8_array TemporaryGlobalFilePaths = {0};

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

	F_WaveFor(Wave, I, SomeArrayCount)
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

	f_watch Watch = F_WatchFilesFromArray(Wave, TemporaryGlobalFilePaths);


	while(!F_AnyFilesChangedInWatch(Wave, &Watch));


	F_WaveBarrier(Wave);
}


int main(int ArgCount, char **Arguments)
{
	TemporaryGlobalFilePaths.Count = 2;
	TemporaryGlobalFilePaths.Items = BackingFileMemory;
	TemporaryGlobalFilePaths.Items[0] = F_Str8("friday2.h");
	TemporaryGlobalFilePaths.Items[1] = F_Str8("friday2.c");
	F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
}