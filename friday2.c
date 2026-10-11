#include "include/app.h"

#define FRIDAY_IMPLEMENTATION
#define FRIDAY_MAIN
#include "friday2.h"

F_LaneEntry
{
	f_build_options BuildOptions = {0};
	BuildOptions.CompileFlags = F_Str8ArrayFromCStrs(F_PArena(), "-fobjc-arc");
	BuildOptions.LinkFlags = F_Str8ArrayFromCStrs(F_PArena(), "-fobjc-arc");
	BuildOptions.Frameworks = F_Str8ArrayFromCStrs(F_PArena(), 
		"Cocoa", "Metal", "QuartzCore"
	);

	BuildOptions.OutputPath = F_Str8("build");
	BuildOptions.OutputName = F_Str8("app.dynlib");

	BuildOptions.IncludeDirectories = F_Str8ArrayFromCStrs(F_PArena(),
	"include/",
	"external/",
	"build/",
	);

	BuildOptions.SourceDirectories = F_Str8ArrayFromCStrs(F_PArena(), "source", "source/app", "source/darwin", "external");
	BuildOptions.Kind = F_OUTPUT_DYNAMIC;

	BuildOptions.Toolchain = F_TOOLCHAIN_CLANG;
	BuildOptions.Debug = F_True;

	f_group UpdateGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID == 0);
	f_group BuildGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID > 0);
	f_build *Build = F_BuildFromGroup(F_PArena(), F_DefaultGroup());
	

	assert(Build->Update);

	app_interface App = {0};

	for(;;)
	{
		if(F_IsInGroup(UpdateGroup))
		{
			Build->Update(&App);
			if(App.WantsToQuit) F_Exit();
		}
		F_UpdateBuild(BuildGroup, Build, &BuildOptions);
		F_ResetTArena();
	}
}
