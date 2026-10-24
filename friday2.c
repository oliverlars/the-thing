#include "include/app.h"

#define FRIDAY_IMPLEMENTATION
#define FRIDAY_MAIN
#include "friday2.h"

F_LaneEntry
{
	f_build_options BuildOptions = {
		.CompileFlags = { "-fobjc-arc" },
		.LinkFlags = { "-fobjc-arc" },
		.Frameworks = { "Cocoa", "Metal", "QuartzCore" },
		.OutputPath = "build",
		.OutputName = "app.dynlib",
		.IncludeDirectories = { "include/", "external/", "build/" },
		.SourceDirectories = { "source", "source/app", "source/darwin", "external" },
		.Kind = F_OUTPUT_DYNAMIC,
		.Toolchain = F_TOOLCHAIN_CLANG,
		.Debug = F_True,
		.EmbedDirectories = { "fonts" },
		.UpdateName = "AppUpdate"
	};

	for(f_s32 I = 0; I < F_CommandLineArgs.Count; I++)
	{
		if(F_StringEqual(F_CommandLineArgs.Items[I], "release"))	
		{
			BuildOptions.Debug = F_False;
			BuildOptions.Optimise = F_True;
		}
	}

	f_group UpdateGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID == 0);
	f_group BuildGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID > 0);
	f_build *Build = F_BuildFromGroup(F_PArena(), F_DefaultGroup());

	app_interface App = {0};

	for(;;)
	{
		if(App.WantsToQuit) F_Exit();
		F_Update(UpdateGroup, Build, &App);
		F_Build(BuildGroup, Build, &BuildOptions);
		F_OS_WaitForNMilliseconds(BuildGroup, 64);
		F_ResetTArena();
	}
}
