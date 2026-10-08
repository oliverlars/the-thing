#include "misc.h"
#include "memory.h"
#include "strings.h"
#include "log.h"
#include "maths.h"
#include "os.h"

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

	App.Platform = (os_platform){
    .LoadEntireFileIntoMemory = OS_LoadEntireFileIntoMemory,
    .LastModified            = OS_LastModified,

    .MousePosition = OS_MousePosition,
    .MouseDown     = OS_MouseDown,
    .MousePressed  = OS_MousePressed,
    .MouseReleased = OS_MouseReleased,
    .KeyDown       = OS_KeyDown,
    .KeyPressed    = OS_KeyPressed,
    .KeyReleased   = OS_KeyReleased,
    .NextEvent     = OS_NextEvent,

    .TimeStampNow              = OS_TimeStampNow,
    .MilliSecondsFromTimeStamp = OS_MilliSecondsFromTimeStamp,

    .NextDrawable = OS_NextDrawable,
    .WindowWidth  = OS_WindowWidth,
    .WindowHeight = OS_WindowHeight,

    .MemoryReserve = MemoryReserve,
    .MemoryCommit  = MemoryCommit,
    .GetPageSize   = GetPageSize,
	};

	f_dll AppDLL = F_OS_DLLFromPath(F_Str8("build/app.dynlib0"));
	void (*AppUpdate)(void *) = F_OS_DLLSymbolFromName(AppDLL, F_Str8("AppUpdate"));

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

	/*
	app.c and app.h need moving to folders to make this new assumption work better
	*/
	BuildOptions.SourceDirectories = F_Str8ArrayFromCStrs(F_PArena(), "source", "source/app", "source/darwin", "external");
	BuildOptions.Kind = F_OUTPUT_DYNAMIC;

	BuildOptions.Toolchain = F_TOOLCHAIN_CLANG;
	BuildOptions.Debug = F_True;

	assert(F_DefaultGroup().Count > 0);

	f_build *Build = 0;
	if(F_DefaultGroup().ID == 0)
	{
		Build = F_BuildFromBuildOptions(F_PArena(), BuildOptions);
		atomic_store_explicit(&Build->TUsToProcess, 3, memory_order_release);
	}
	Build = F_ReadFromGroupID(F_DefaultGroup(), 0, Build);

	f_group UpdateGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID == 0);
	f_group BuildGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID > 0);
	assert(UpdateGroup.Count == 1);
	_Atomic(f_b32) *ShouldReload = 0;
	_Atomic(f_u64) *NumberOfReloads = 0;
	if(F_DefaultGroup().ID == 0)
	{
		ShouldReload = F_ArenaPushType(F_PArena(), _Atomic(f_b32));
		NumberOfReloads = F_ArenaPushType(F_PArena(), _Atomic(f_u64));
	}
	ShouldReload = F_ReadFromGroupID(F_DefaultGroup(), 0, ShouldReload);
	NumberOfReloads = F_ReadFromGroupID(F_DefaultGroup(), 0, NumberOfReloads);
	for(;;)
	{
		if(F_IsInGroup(UpdateGroup))
		{
			for(;!atomic_load(ShouldReload);)
			{
				OS_BeginFrame();
				if(AppUpdate) AppUpdate(&App);
				OS_EndFrame();
			}
			//printf("Finished Updating\n");
			if(!AppUpdate || atomic_exchange(ShouldReload, F_False))
			{
				AppDLL = F_OS_DLLFromPath(F_Str8PushF(F_TArena(), "build/app.dynlib%llu", atomic_load(NumberOfReloads) - 1));
				AppUpdate = F_OS_DLLSymbolFromName(AppDLL, F_Str8("AppUpdate"));
			}
		}
		if(F_IsInGroup(BuildGroup))
		{
			
			f_build_cache BuildCache = F_BuildCacheFromOutputPath(BuildGroup, F_TArena(), F_Str8("build/"));
			
			f_str8_array  SourceFiles = {0};
			f_str8_array  ObjectFiles = {0};
			f_b32 ShouldLinkTUs = F_False;
			if(BuildGroup.ID == 0) 
			{
				SourceFiles = F_OS_ArrayOfEntriesInDirectories(
					F_TArena(), 
				 .Directories[0] = BuildOptions.SourceDirectories.Items[0],
					.Directories[1] = BuildOptions.SourceDirectories.Items[1],
					.Directories[2] = BuildOptions.SourceDirectories.Items[2],
					.Directories[3] = BuildOptions.SourceDirectories.Items[3],
				 .PrependPath = F_True,
				 .FilterByExtension = F_Str8ArrayFromCStrs(F_TArena(), ".m", ".c")
				);
				ObjectFiles.Items = F_ArenaPushArrayType(F_TArena(), SourceFiles.Count, f_str8);
				ObjectFiles.Count = SourceFiles.Count;
			}
			SourceFiles = F_ReadFromGroupID(BuildGroup, 0, SourceFiles);
			ObjectFiles = F_ReadFromGroupID(BuildGroup, 0, ObjectFiles);
			f_range SourceFileRange = F_SplitRange(BuildGroup, SourceFiles.Count);
			f_process *Processes = F_ArenaPushArrayType(F_TArena(), SourceFileRange.End - SourceFileRange.Start, f_process);
			for(f_s32 I = SourceFileRange.Start; I < SourceFileRange.End; I++)
			{
				f_u64 Hash = F_HashSourceAndIncludes(F_TArena(), SourceFiles.Items[I], BuildOptions.IncludeDirectories);
				f_str8 ObjectPath = {0};
				if(BuildOptions.OutputPath.Size > 0)
				{
					ObjectPath = F_Str8PushF(F_TArena(), "%.*s/%llu.o", F_Expand(BuildOptions.OutputPath), Hash);
				}
				else
				{
					ObjectPath = F_Str8PushF(F_TArena(), "%llu.o", Hash);
				}
				ObjectFiles.Items[I] = ObjectPath;
				if(!F_HashIsInBuildCache(BuildCache, Hash))
				{
					ShouldLinkTUs = F_True;
					f_str8_list Args = {0};

					F_Str8ListAppend(F_TArena(), &Args, BuildOptions.Toolchain.CompilerPath);
					F_Str8ListAppend(F_TArena(), &Args, BuildOptions.Toolchain.CompileOnlyFlag);
					F_Str8ListAppend(F_TArena(), &Args, SourceFiles.Items[I]);

					if(BuildOptions.Debug)
					{
						F_Str8ListAppendF(F_TArena(), &Args, "%.*s", F_Expand(BuildOptions.Toolchain.DebugFlag));
					}

					for(int I = 0; I < BuildOptions.CompileFlags.Count; I++)
					{
						F_Str8ListAppend(F_TArena(), &Args, BuildOptions.CompileFlags.Items[I]);
					}

					F_Str8ListAppendF(F_TArena(), &Args, "%.*s%.*s", F_Expand(BuildOptions.Toolchain.OutputFlag), F_Expand(ObjectPath));

					for(int I = 0; I < BuildOptions.IncludeDirectories.Count; I++)
					{
						F_Str8ListAppendF(F_TArena(), &Args, "%.*s%.*s", F_Expand(BuildOptions.Toolchain.IncludeFlag), F_Expand(BuildOptions.IncludeDirectories.Items[I]));
					}
					F_Print(F_Str8JoinList(F_TArena(), Args, F_Str8(" ")));
					F_Print(F_Str8("\n"));

					F_StartProcess(F_TArena(), Processes + I - SourceFileRange.Start, Args);
				}
			}
			F_WaitForMultipleProcesses(Processes, SourceFileRange.End - SourceFileRange.Start);
			ShouldLinkTUs = F_GroupAnyTrue(BuildGroup, ShouldLinkTUs);
			F_GroupBarrier(BuildGroup);

			if((ShouldLinkTUs || !AppUpdate) && BuildGroup.ID == 0)
			{
				f_str8 OutputPath = {0};
				if(BuildOptions.OutputPath.Size > 0)
				{
					OutputPath = F_Str8PushF(F_TArena(), "%.*s/%.*s%llu", F_Expand(BuildOptions.OutputPath), F_Expand(BuildOptions.OutputName), atomic_fetch_add(NumberOfReloads, 1));
				}
				else
				{
					OutputPath = F_Str8PushF(F_TArena(), "%.*s", F_Expand(BuildOptions.OutputName));
				}

				f_str8_list Args = {0};

				F_Str8ListAppend(F_TArena(), &Args, BuildOptions.Toolchain.CompilerPath);

				for(int I = 0; I < BuildOptions.LinkFlags.Count; I++)
				{
					F_Str8ListAppend(F_TArena(), &Args, BuildOptions.LinkFlags.Items[I]);
				}

				for(int I = 0; I < BuildOptions.Frameworks.Count; I++)
				{
					F_Str8ListAppend(F_TArena(), &Args, F_Str8("-framework"));
					F_Str8ListAppend(F_TArena(), &Args, BuildOptions.Frameworks.Items[I]);
				}

				if(BuildOptions.Kind == F_OUTPUT_DYNAMIC)
				{
					F_Str8ListAppend(F_TArena(), &Args, F_Str8PushF(F_TArena(), "%.*s", F_Expand(BuildOptions.Toolchain.DynamicFlag)));
				}
				else if(BuildOptions.Kind == F_OUTPUT_STATIC)
				{
					F_Str8ListAppend(F_TArena(), &Args, F_Str8PushF(F_TArena(), "%.*s", F_Expand(BuildOptions.Toolchain.StaticFlag)));
				}

				F_Str8ListAppend(F_TArena(), &Args, F_Str8PushF(F_TArena(), "%.*s%.*s", F_Expand(BuildOptions.Toolchain.OutputFlag), OutputPath));
				
				for(int I = 0; I < ObjectFiles.Count; I++)
				{
					F_Str8ListAppend(F_TArena(), &Args, ObjectFiles.Items[I]);
				}
				f_process LinkProcess = {0};
				F_StartProcess(F_TArena(), &LinkProcess, Args);
				F_WaitForProcess(&LinkProcess);

				f_process DSYMProcess = {0};
				f_str8_list DSymArgs = {0};
				F_Str8ListAppend(F_TArena(), &DSymArgs, F_Str8("dsymutil"));
				F_Str8ListAppend(F_TArena(), &DSymArgs, OutputPath);
				F_Str8ListAppend(F_TArena(), &DSymArgs, F_Str8("-o"));
				F_Str8ListAppend(F_TArena(), &DSymArgs, F_Str8PushF(F_TArena(), "%.*s.dSYM", F_Expand(OutputPath)));

				F_StartProcess(F_TArena(), &DSYMProcess, DSymArgs);
				
				F_WaitForProcess(&DSYMProcess);

				atomic_store(ShouldReload, F_True);

				F_Print(F_Str8("LINKING!!! "));
				F_Print(F_Str8JoinList(F_TArena(), Args, F_Str8(" ")));
			}
		}
		F_ResetTArena();
		F_GroupBarrier(F_DefaultGroup());
	}
}
