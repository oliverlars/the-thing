#include "app.h"

#define FRIDAY_IMPLEMENTATION
#define FRIDAY_MAIN
#include "friday2.h"

F_LaneEntry
{


	app_memory App = {0};
	App.PermanentArena = ArenaInitialise(F_GigaBytes(8));
	App.FrameArenas[0] = ArenaInitialise(F_GigaBytes(8));
	App.FrameArenas[1] = ArenaInitialise(F_GigaBytes(8));
	
	per_thread_context *Ctx = ArenaPushArrayZero(App.PermanentArena, per_thread_context, 1);

	Ctx->ScratchArenas[0] = ArenaInitialise(MegaBytes(64));
	Ctx->ScratchArenas[1] = ArenaInitialise(MegaBytes(64));
	Ctx->TemporaryArena = App.FrameArenas[0];
	Ctx->LogArena = ArenaInitialise(MegaBytes(8));
	Ctx->Logger = LoggerInitialise(Ctx->LogArena);


	App.ThreadContext = Ctx;
	SetThreadLocalContextPointer(Ctx);

	f_build_options BuildOptions = {0};
	BuildOptions.CompileFlags = F_Str8ArrayFromCStrs(F_PArena(), "-fobjc-arc");
	//BuildOptions.LinkFlags = F_Str8ArrayFromCStrs(F_PArena(), "-fobjc-arc", "build/platform.dynlib");
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
	for(;;)
	{
		if(F_IsInGroup(UpdateGroup))
		{
			Build->Update(&App);
		}
		F_UpdateBuild(BuildGroup, Build, &BuildOptions);
		F_ResetTArena();
	}
}
