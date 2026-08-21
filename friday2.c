#define FRIDAY_IMPLEMENTATION
#include "friday2.h"

#include <stdio.h>

#include <stdlib.h>

F_LaneEntry(Wave)
{


	if(F_WaveIsFirstLane(Wave))
	{
		F_OS_MakeDirectoryIfNotAlreadyMade(Wave->GlobalArena, F_Str8("build"));
	}
}

int main(int ArgCount, char **Arguments)
{
	F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
}