#include "friday.h"
#include <sys/stat.h>

/*
Build system for the-thing — a minimal Metal renderer.

Usage:
	./friday          build (debug)
	./friday run      build then launch
	./friday release  optimised build (-O2)
	./friday rebuild  force full rebuild
	./friday help     print this

Bootstrap (one-time): see build.sh, or run:
	clang friday.c -O2 -o friday
*/

F_Entry(InputWaveInfo)
{
	f_wave_info *WaveInfo = (f_wave_info*)InputWaveInfo;
	f_shared_wave_info *Shared = WaveInfo->Shared;

	f_memory_arena Arena = {0};
	Arena.Start = F_MemoryReserve(F_GigaBytes(64));

	f_build_options BuildOptions = {0};
	f_b32 PrintHelpMenu = F_False;
	f_b32 ShouldRun = F_False;
	f_b32 GpuLogging = F_False;
	f_b32 AppOnly = F_False;

	if(F_WaveIsFirstLane(WaveInfo))
	{
		f_b32 OptimisedBuild = F_False;
		f_b32 Rebuild = F_False;

		f_b32 Carnage = F_False;

		for(f_s64 ArgIndex = 1; ArgIndex < Shared->ArgCount; ArgIndex++)
		{
			f_str8 Arg = F_Str8PushF(&Arena, "%s", Shared->Args[ArgIndex]);
			if(0) {}
			else if(F_Str8Equal(Arg, F_Str8("release"))) { OptimisedBuild = F_True; }
			else if(F_Str8Equal(Arg, F_Str8("rebuild"))) { Rebuild = F_True; }
			else if(F_Str8Equal(Arg, F_Str8("run")))     { ShouldRun = F_True; }
			else if(F_Str8Equal(Arg, F_Str8("gpu-log"))) { GpuLogging = F_True; }
			else if(F_Str8Equal(Arg, F_Str8("carnage"))) { Carnage = F_True; }
			else if(F_Str8Equal(Arg, F_Str8("app")))     { AppOnly = F_True; }
			else if(F_Str8Equal(Arg, F_Str8("help")))
			{
				printf("\nfriday — build system for the-thing\n\n");
				printf("Flags:\n");
				printf("  run       build, then launch the binary\n");
				printf("  app       build only app.dynlib (the hot reload inner loop)\n");
				printf("  gpu-log   with run: enable GPU shader logging at runtime\n");
				printf("  release   optimised build (-O2)\n");
				printf("  rebuild   force full rebuild (ignore object cache)\n");
				printf("  help      print this menu\n\n");
				PrintHelpMenu = F_True;
			}
		}

		f_str8_list IncludePaths = {0};
		f_str8_list SourceFiles = {0};
		f_str8_list ExtraLinkFlags = {0};
		f_str8_list ExtraCompileFlags = {0};

		F_Str8ListAppend(&Arena, &IncludePaths, F_Str8("include/"));
		F_Str8ListAppend(&Arena, &IncludePaths, F_Str8("external/"));
		F_Str8ListAppend(&Arena, &IncludePaths, F_Str8("build/"));

		// main.c lives at the project root; the rest live in source/.
		F_Str8ListAppend(&Shared->Arena, &SourceFiles, F_Str8("main.c"));
		F_Str8ListAppend(&Shared->Arena, &SourceFiles, F_Str8("build/embedded_fonts.c"));
		F_AppendSourceFilesInFolder(&Shared->Arena, &SourceFiles, F_Str8("source"));
		F_AppendSourceFilesInFolder(&Shared->Arena, &SourceFiles, F_Str8("source/darwin"));
		F_AppendSourceFilesInFolder(&Shared->Arena, &SourceFiles, F_Str8("external"));

		F_Str8ListAppend(&Shared->Arena, &ExtraLinkFlags,
			F_Str8("-framework Cocoa -framework Metal -framework QuartzCore "));

		BuildOptions = (f_build_options){
			.Compiler          = Carnage ? F_COMPILER_CARNAGE : F_COMPILER_PLATFORM_DEFAULT,
			.OutputName        = F_Str8("the-thing"),
			.OutputType        = F_OUTPUT_EXECUTABLE,
			.SourceFiles       = SourceFiles,
			.IncludePaths      = IncludePaths,
			.ExtraLinkFlags    = ExtraLinkFlags,
			.ExtraCompileFlags = ExtraCompileFlags,
			.WorkingDirectory  = F_GetExecutableDirectory(&Arena),
			.OutputPath        = F_Str8("build/"),
			.Optimise          = OptimisedBuild,
			.FullRebuild       = Rebuild,
		};

		mkdir("build", 0755);

		{
			f_str8_list FontPaths = {0};
			F_Str8ListAppend(&Arena, &FontPaths, F_Str8("fonts/BerkeleyMono-Regular.ttf"));
			F_Str8ListAppend(&Arena, &FontPaths, F_Str8("fonts/lucide.ttf"));

			f_str8_list FontNames = {0};
			F_Str8ListAppend(&Arena, &FontNames, F_Str8("G_BerkeleyMono"));
			F_Str8ListAppend(&Arena, &FontNames, F_Str8("G_Lucide"));

			F_GenerateEmbedHeader(&Arena, F_Str8("build"));
			F_EmbedFiles(&Arena, FontPaths, F_Str8("build/embedded_fonts.c"), FontNames);
		}

		F_EmitCompileCommandsJson(&Arena, BuildOptions);
	}

	F_BroadcastToAllLanes(WaveInfo, BuildOptions);
	F_BroadcastToAllLanes(WaveInfo, PrintHelpMenu);
	F_BroadcastToAllLanes(WaveInfo, ShouldRun);
	F_BroadcastToAllLanes(WaveInfo, AppOnly);

	f_b32 FailedToBuild = F_False;
	if(!PrintHelpMenu)
	{
		FailedToBuild = F_Build(WaveInfo, &Arena, BuildOptions);
	}

	if(F_WaveIsFirstLane(WaveInfo) && ShouldRun && !FailedToBuild)
	{
		f_str8 GPULoggingArgument = F_Str8("");
		if(GpuLogging) GPULoggingArgument = F_Str8("gpu-log");
		f_str8 PathToExecutable = F_Str8PushF(&Arena, "%.*s%.*s %.*s",
			F_Str8ToPrintfArgs(BuildOptions.OutputPath),
			F_Str8ToPrintfArgs(BuildOptions.OutputName),
			F_Str8ToPrintfArgs(GPULoggingArgument)
		);
		printf("\nStarting the-thing...\n\n");
		f_b32 Outcome = F_False;
		F_ExecuteCommand(&Arena, PathToExecutable, &Outcome);
	}

	return 0;
}

int main(f_s32 ArgCount, char **Args)
{
	f_memory_arena SharedArena = {0};
	SharedArena.Start = F_MemoryReserve(F_GigaBytes(64));

	f_shared_wave_info *Shared = (f_shared_wave_info*)F_ArenaPush(&SharedArena, sizeof(f_shared_wave_info));
	Shared->ArgCount = ArgCount;
	Shared->Args = Args;
	Shared->Arena = SharedArena;
	F_Dispatch(Shared, F_GetNumberOfCPUs());
}
