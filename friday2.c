#include "friday2.h"

#include <stdio.h>

F_LaneEntry(Wave)
{
	printf("Lane %llu\n", Wave->Lane);

	F_WaveBarrier(Wave);
}


int main(int ArgCount, char **Arguments)
{
	f_wave_context WaveContext = {0};
	F_Dispatch(&WaveContext, F_OS_NumberOFHighPerformanceCPUs());
}