#include "friday2.h"

#include <stdio.h>

F_LaneEntry(Wave)
{
	f_s32 LaneTimesTwo = Wave->Lane * 2;

	printf("Lane 4 has value: %d\n", F_WaveValueAtLane(Wave, LaneTimesTwo, 4));

	F_WaveBarrier(Wave);
}


int main(int ArgCount, char **Arguments)
{
	F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
}