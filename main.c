#include "misc.h"
#include "memory.h"
#include "strings.h"
#include "log.h"
#include "maths.h"
#include "os.h"

#include "app.h"

#include "friday.h"

#include <dlfcn.h>

typedef struct platform_context
{
	app_memory *Memory;
	void (*AppUpdate)(void *UserData, v2i ViewportSize);


	f_build_options AppBuildOptions;
	f_memory_arena *FridayArena;

	u64 AppLibModified;
	u64 LoadCounter;
	u64 FrameCounter;
}
platform_context;


// HACK!!!
u64 *SourceFileTimeStamps;
u64 SourceFileTimeStampsCount;

F_Entry(InputWaveInfo)
{
	f_wave_info *WaveInfo = (f_wave_info*)InputWaveInfo;
	f_shared_wave_info *Shared = WaveInfo->Shared;

	f_memory_arena Arena = {0};
	Arena.Start = F_MemoryReserve(F_GigaBytes(1));

	b32 ShouldRebuildApp = False;
	f_str8_array RelativeSourceFilePaths = {0};
	f_str8_array ObjectFileNames = {0};
	if(F_WaveIsFirstLane(WaveInfo))
	{
		RelativeSourceFilePaths = F_Str8ArrayFromList(&WaveInfo->Shared->Arena, Shared->BuildOptions.SourceFiles);
		ObjectFileNames.Count = RelativeSourceFilePaths.Count;
		ObjectFileNames.Items = (f_str8*)F_ArenaPush(&WaveInfo->Shared->Arena, sizeof(f_str8)*RelativeSourceFilePaths.Count);
	}
	
	F_BroadcastToAllLanes(WaveInfo, RelativeSourceFilePaths);

	f_s64 Count = F_LaneValueFromTotal(WaveInfo, RelativeSourceFilePaths.Count);
	f_s64 Index = F_ExclusiveWavePrefixSum(WaveInfo, Count);

	f_b32 AnyFilesChanged = F_False;
	for(int I = Index; I < Index + Count; I++)
	{
		f_str8 Path = RelativeSourceFilePaths.Items[I];

		struct stat FileStat = {0};
		u64 LastModified = 0;
		if(stat(F_CStrFromStr8(&Arena, Path), &FileStat) == 0)
		{
			LastModified = (u64)FileStat.st_mtime;
		}

		if(LastModified != SourceFileTimeStamps[I])
		{
			SourceFileTimeStamps[I] = LastModified;
			AnyFilesChanged = F_True;
		}
	}

	ShouldRebuildApp = F_WaveAnyTrue(WaveInfo, AnyFilesChanged);
	if(ShouldRebuildApp)
	{
		F_Build(WaveInfo, &Arena, Shared->BuildOptions);
	}

	return 0;
}


static void AppCodeReload(platform_context *Context)
{
	u64 Modified = OS_LastModified(Str8("build/app.dynlib"));
	if(Modified == 0 || Modified == Context->AppLibModified) return;

	m_scratch_arena Scratch = ScratchBegin(0, 0);



	// dlopen caches by path, so each load needs a filename nothing has used yet.
	str8 CopyPath = Str8PushF(Scratch.Arena, "build/app-?u64.dynlib", Context->LoadCounter);
	str8 CopyCommand = Str8PushF(Scratch.Arena, "cp build/app.dynlib ?S", CopyPath);
	system(CStrFromStr8(Scratch.Arena, CopyCommand));

	void *Library = dlopen(CStrFromStr8(Scratch.Arena, CopyPath), RTLD_NOW | RTLD_LOCAL);
	if(Library)
	{
		void (*Update)(void*, v2i) = dlsym(Library, "AppUpdate");
		if(Update)
		{
			// The old image is deliberately left loaded: string literals and
			// Metal completion blocks handed out by it are still referenced
			// from arena memory that outlives the swap.
			Context->AppUpdate = Update;
			Context->AppLibModified = Modified;
			Context->LoadCounter++;
			LogInfo("Reloaded app.dynlib (load ?u64)", Context->LoadCounter);
		}
		else
		{
			LogError("app.dynlib has no AppUpdate, keeping previous code");
		}
	}
	else
	{
		LogError("dlopen failed: ?cstr", dlerror());
	}

	ScratchEnd(Scratch);
}

static void PlatformFrame(void *UserData, v2i ViewportSize)
{
	platform_context *Context = (platform_context*)UserData;

	AppCodeReload(Context);

	Context->AppUpdate(Context->Memory, ViewportSize);
	Context->FrameCounter++;
}

int main(int ArgCount, char **Args)
{

	m_arena *Arena = ArenaInitialise(GigaBytes(4));

	per_thread_context MainThreadContext = {0};
	SetThreadLocalContextPointer(&MainThreadContext);

	per_thread_context *Ctx = GetThreadLocalContext();
	Ctx->ScratchArenas[0] = ArenaInitialise(GigaBytes(1));
	Ctx->ScratchArenas[1] = ArenaInitialise(GigaBytes(1));
	Ctx->LogArena = ArenaInitialise(MegaBytes(64));
	Ctx->Logger = LoggerInitialise(Ctx->LogArena);
	Ctx->Logger->Arena = Ctx->LogArena;



	app_memory AppMemory = {0};
	AppMemory.FrameArenas[0] = ArenaInitialise(MegaBytes(256));
	AppMemory.FrameArenas[1] = ArenaInitialise(MegaBytes(256));
	AppMemory.PermanentArena = Arena;
	AppMemory.ThreadContext = &MainThreadContext;
	SetTemporaryArena(AppMemory.FrameArenas[0]);


	Platform.LoadEntireFileIntoMemory = OS_LoadEntireFileIntoMemory;
	Platform.NextDrawable = OS_NextDrawable;
	Platform.WindowHeight = OS_WindowHeight;
	Platform.WindowWidth = OS_WindowWidth;
	Platform.MousePosition = OS_MousePosition;
	Platform.NextEvent = OS_NextEvent;
	Platform.KeyDown = OS_KeyDown;
	Platform.MouseDown = OS_MouseDown;
	Platform.KeyPressed = OS_KeyPressed;
	Platform.KeyReleased = OS_KeyReleased;
	Platform.MousePressed = OS_MousePressed;
	Platform.MouseReleased = OS_MouseReleased;
	Platform.TimeStampNow = OS_TimeStampNow;
	Platform.MilliSecondsFromTimeStamp = OS_MilliSecondsFromTimeStamp;
	Platform.LastModified = OS_LastModified;

	AppMemory.Platform = Platform;

	os_window Window = OS_MakeWindow(Str8("the-thing"), 1280, 800);
	AppMemory.Window = &Window;

	b32 GPULogging = False;
	for(int I = 0; I < ArgCount; I++)
	{
		if(strcmp(Args[I], "gpu-log") == 0)
		{
			GPULogging = True;
		}
	}

	platform_context PlatformContext = {0};
	PlatformContext.Memory = &AppMemory;

	AppCodeReload(&PlatformContext);
	if(!PlatformContext.AppUpdate)
	{
		LogError("no app code to run — build build/app.dynlib first");
		return 1;
	}


	f_build_options AppBuildOptions = {0};
	AppBuildOptions.OutputType = F_OUTPUT_DYNAMIC_LIBRARY;
	AppBuildOptions.OutputName = F_Str8("app.dynlib");
	AppBuildOptions.OutputPath = F_Str8("build/");
	AppBuildOptions.SourceFiles = (f_str8_list){0};
	f_memory_arena FridayArena = {0};
	FridayArena.Start = F_MemoryReserve(F_GigaBytes(1));

	F_Str8ListAppend(&FridayArena, &AppBuildOptions.IncludePaths, F_Str8("include/"));
	F_Str8ListAppend(&FridayArena, &AppBuildOptions.IncludePaths, F_Str8("external/"));
	F_Str8ListAppend(&FridayArena, &AppBuildOptions.IncludePaths, F_Str8("build/"));
	F_Str8ListAppend(&FridayArena, &AppBuildOptions.ExtraLinkFlags,
		F_Str8("-framework Cocoa -framework Metal -framework QuartzCore ")
	);

	F_Str8ListAppend(&FridayArena, &AppBuildOptions.SourceFiles, F_Str8("app.c"));
	F_AppendSourceFilesInFolder(&FridayArena, &AppBuildOptions.SourceFiles, F_Str8("source"));
	F_AppendSourceFilesInFolder(&FridayArena, &AppBuildOptions.SourceFiles, F_Str8("source/darwin"));
	F_AppendSourceFilesInFolder(&FridayArena, &AppBuildOptions.SourceFiles, F_Str8("external"));
	F_Str8ListAppend(&FridayArena, &AppBuildOptions.SourceFiles, F_Str8("build/embedded_fonts.c"));

	SourceFileTimeStampsCount = 0;
	for(f_str8_list_node *Node = AppBuildOptions.SourceFiles.First; Node; Node = Node->Next)
	{
		SourceFileTimeStampsCount += 1;
	}

	SourceFileTimeStamps = (u64*)(F_ArenaPush(&FridayArena, sizeof(u64)*SourceFileTimeStampsCount));

	f_shared_wave_info *Shared = (f_shared_wave_info*)F_ArenaPush(&FridayArena, sizeof(f_shared_wave_info));
	Shared->ArgCount = ArgCount;
	Shared->Args = Args;
	Shared->Arena = FridayArena;
	Shared->BuildOptions = AppBuildOptions;
	F_Dispatch(Shared, F_GetNumberOfCPUs());

	OS_Run(&Window, PlatformFrame, &PlatformContext);
	return 0;
}
