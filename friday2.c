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

	SomeArrayCount = F_WaveValueAtFirstLane(Wave, SomeArrayCount);
	SomeArray = F_WaveValueAtFirstLane(Wave, SomeArray);
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

	F_WaveBarrier(Wave);
}


int main(int ArgCount, char **Arguments)
{
	F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
}