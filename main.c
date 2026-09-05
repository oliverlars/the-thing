#include "misc.h"
#include "memory.h"
#include "strings.h"
#include "log.h"
#include "maths.h"
#include "os.h"

#include "app.h"

#define FRIDAY_IMPLEMENTATION
#include "friday2.h"

#include <dlfcn.h>

typedef struct platform_context
{
	app_memory *Memory;
	void (*AppUpdate)(void *UserData, v2i ViewportSize);

	f_memory_arena *FridayArena;

	f_str8_list SourceFiles;
	f_str8_list IncludePaths;
	f_str8_array ExtraFlags;

	u64 AppLibModified;
	u64 LoadCounter;
	u64 FrameCounter;
}
platform_context;

platform_context PlatformContext;

void AppCodeReload(f_memory_arena *Arena)
{
	f_u64 Mark = Arena->At;
	u64 Modified = OS_LastModified(Str8("build/app.dynlib"));
	if(Modified == PlatformContext.AppLibModified) return;

	// dlopen caches by path, so each load needs a filename nothing has used yet.
	f_str8 CopyPath = F_Str8PushF(Arena, "build/app-%llu.dynlib", PlatformContext.LoadCounter);
	f_str8 CopyCommand = F_Str8PushF(Arena, "cp build/app.dynlib %.*s", F_Expand(CopyPath));
	system(F_CStrFromStr8(Arena, CopyCommand));
	void *Library = dlopen(F_CStrFromStr8(Arena, CopyPath), RTLD_NOW | RTLD_LOCAL);
	if(Library)
	{
		void (*Update)(void*, v2i) = dlsym(Library, "AppUpdate");
		if(Update)
		{
			// The old image is deliberately left loaded: string literals and
			// Metal completion blocks handed out by it are still referenced
			// from arena memory that outlives the swap.
			PlatformContext.AppUpdate = Update;
			PlatformContext.AppLibModified = Modified;
			PlatformContext.LoadCounter++;
			LogInfo("Reloaded app.dynlib (load ?u64)", PlatformContext.LoadCounter);
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
	Arena->At = Mark;
}

F_LaneEntry(Wave)
{

	f_watch Watch = F_WatchFilesFromArray(Wave, F_Str8ArrayFromList(PlatformContext.FridayArena, PlatformContext.SourceFiles));


	f_compile_options CompileOptions = {0};
	if(F_WaveIsFirstLane(Wave))
	{
		CompileOptions.IncludePaths = F_Str8ArrayFromList(PlatformContext.FridayArena, PlatformContext.IncludePaths);
		CompileOptions.Debug = F_True;
		CompileOptions.ExtraFlags = F_Str8ArrayFromCStrs(PlatformContext.FridayArena, "-fobjc-arc");
	}
	F_BroadcastFirstLane(Wave, CompileOptions);


	f_link_options LinkOptions = {0};
	if(F_WaveIsFirstLane(Wave))
	{
		LinkOptions.OutputKind = F_OUTPUT_DYNAMIC;
		LinkOptions.ExtraFlags = PlatformContext.ExtraFlags;
	}
	F_BroadcastFirstLane(Wave, LinkOptions);

	f_str8_array ObjectFiles = {0};
	ObjectFiles.Count = Watch.FileCount;
	if(F_WaveIsFirstLane(Wave))
	{
		ObjectFiles.Items = F_ArenaPushArrayType(Wave->GlobalArena, Watch.FileCount, f_str8);
	}
	F_BroadcastFirstLane(Wave, ObjectFiles);

	f_str8_array SourceFiles = F_Str8ArrayFromList(PlatformContext.FridayArena, PlatformContext.SourceFiles);
	f_str8_array IncludePaths = F_Str8ArrayFromList(PlatformContext.FridayArena, PlatformContext.IncludePaths);

	{
		f_b32 CompileSucceeded = F_True;
		for F_EachWaveSplit(Wave, I, SourceFiles.Count)
		{
			f_u64 Hash = F_HashSourceAndIncludes(Wave->Arena, SourceFiles.Items[I], IncludePaths);
			Hash ^= F_HashCompileOptions(CompileOptions);
			f_str8 ObjectFile = F_Str8PushF(Wave->Arena, "%llu.o", Hash);
			ObjectFiles.Items[I] = F_Str8PushF(Wave->Arena, "build/%.*s", F_Expand(ObjectFile));
			CompileOptions.OutputName = ObjectFiles.Items[I];
			f_compilation Compilation = F_CompileTU(Arena, SourceFiles.Items[I], CompileOptions, F_TOOLCHAIN_CLANG);
			F_WaitForProcess(&Compilation.Process);
			CompileSucceeded &= F_ProcessExitedCleanly(&Compilation.Process);
		}

		if(F_WaveAnyTrue(Wave, CompileSucceeded))
		{
			if(F_WaveIsFirstLane(Wave))
			{
				LinkOptions.OutputName = F_Str8PushF(Wave->Arena, "build/app.dynlib");
				f_link Link = F_LinkTUs(Wave->Arena, ObjectFiles, LinkOptions, F_TOOLCHAIN_CLANG);
				F_WaitForProcess(&Link.Process);
			}
		}
	}

	F_WaveBarrier(Wave);
	for(;;)
	{
		if(F_WaveIsFirstLane(Wave)) OS_BeginFrame();
		
		if(F_AnyFilesChangedInWatch(Wave, &Watch, F_DEFAULT_TIMEOUT_MS))
		{
			f_b32 CompileSucceeded = F_True;
			for F_EachWaveSplit(Wave, I, Watch.FileCount)
			{
				f_file_info FileInfo = F_OS_FileInfoFromFilename(Wave->Arena, Watch.Files[I]);
				if(FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch.TimeStamps[I] > 0) printf("Change time in ms: %llu\n", FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch.TimeStamps[I]);
				if(FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch.TimeStamps[I] > 0)
				{
					Watch.TimeStamps[I] = FileInfo.LastModifiedInMillisecondsSinceEpoch;
					f_u64 Hash = F_HashSourceAndIncludes(Wave->Arena, Watch.Files[I], IncludePaths);
					CompileOptions.OutputName = F_Str8PushF(Wave->Arena, "build/%llu.o", Hash);
					ObjectFiles.Items[I] = CompileOptions.OutputName;
					Hash ^= F_HashCompileOptions(CompileOptions);
					f_compilation Compilation = F_CompileTU(Arena, Watch.Files[I], CompileOptions, F_TOOLCHAIN_CLANG);
					F_WaitForProcess(&Compilation.Process);
					CompileSucceeded &= F_ProcessExitedCleanly(&Compilation.Process);
				}
			}

				if(F_WaveAnyTrue(Wave, CompileSucceeded))
				{
					if(F_WaveIsFirstLane(Wave))
					{
						LinkOptions.OutputName = F_Str8PushF(Arena, "build/app.dynlib");
						f_link Link = F_LinkTUs(Arena, ObjectFiles, LinkOptions, F_TOOLCHAIN_CLANG);
						F_WaitForProcess(&Link.Process);
					}
				}
			}

		F_WaveBarrier(Wave);

		if(F_WaveIsFirstLane(Wave))
		{
			AppCodeReload(PlatformContext.FridayArena);

			if(!OS_UpdateWindow(PlatformContext.Memory->Window)) break;


			PlatformContext.AppUpdate(PlatformContext.Memory, v2i(1280*2, 720*2));
			PlatformContext.FrameCounter++;
		}
		if(F_WaveIsFirstLane(Wave)) OS_EndFrame();
	}
}

int main(int ArgCount, char **Args)
{
	m_arena *Arena = ArenaInitialise(GigaBytes(4));

	per_thread_context MainThreadContext = {0};
	SetThreadLocalContextPointer(&MainThreadContext);

	per_thread_context *Ctx = GetThreadLocalContext();
	Ctx->ScratchArenas[0] = ArenaInitialise(GigaBytes(4));
	Ctx->ScratchArenas[1] = ArenaInitialise(GigaBytes(4));
	Ctx->LogArena = ArenaInitialise(MegaBytes(1024));
	Ctx->Logger = LoggerInitialise(Ctx->LogArena);
	Ctx->Logger->Arena = Ctx->LogArena;


	app_memory AppMemory = {0};
	AppMemory.FrameArenas[0] = ArenaInitialise(GigaBytes(4));
	AppMemory.FrameArenas[1] = ArenaInitialise(GigaBytes(4));
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

	PlatformContext.FridayArena = F_ArenaReserveButCommitFirstPage(F_GigaBytes(1));

	PlatformContext.Memory = &AppMemory;
	


	f_compile_options AppCompileOptions = {0};
	AppCompileOptions.OutputName = F_Str8("app.dynlib");
	AppCompileOptions.OutputPath = F_Str8("build/");

	f_link_options AppLinkOptions  = {0};

	F_Str8ListAppendMany(PlatformContext.FridayArena, &PlatformContext.SourceFiles,
		F_OS_ListOfEntriesInDirectory(PlatformContext.FridayArena, 
			F_Str8("source"), .ExcludeDirectories = F_True, .PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromMany(PlatformContext.FridayArena, F_Str8(".c"), F_Str8(".m")))
	);

	F_Str8ListAppendMany(PlatformContext.FridayArena, &PlatformContext.SourceFiles,
		F_OS_ListOfEntriesInDirectory(PlatformContext.FridayArena, 
			F_Str8("source/darwin"), .ExcludeDirectories = F_True, .PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromMany(PlatformContext.FridayArena, F_Str8(".c"), F_Str8(".m")))
	);

	F_Str8ListAppendMany(PlatformContext.FridayArena, &PlatformContext.SourceFiles,
		F_OS_ListOfEntriesInDirectory(PlatformContext.FridayArena, 
			F_Str8("external/"), .ExcludeDirectories = F_True, .PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromMany(PlatformContext.FridayArena, F_Str8(".c"), F_Str8(".m")))
	);

	F_Str8ListAppend(PlatformContext.FridayArena, &PlatformContext.IncludePaths, F_Str8("include/"));
	F_Str8ListAppend(PlatformContext.FridayArena, &PlatformContext.IncludePaths, F_Str8("external/"));
	F_Str8ListAppend(PlatformContext.FridayArena, &PlatformContext.IncludePaths, F_Str8("build/"));
	PlatformContext.ExtraFlags = F_Str8ArrayFromMany(PlatformContext.FridayArena,
		F_Str8("-framework"), F_Str8("Cocoa"), F_Str8("-framework"), F_Str8("Metal"), F_Str8("-framework"), F_Str8("QuartzCore")
	);

	F_Str8ListAppend(PlatformContext.FridayArena, &PlatformContext.SourceFiles, F_Str8("app.c"));
	F_Str8ListAppend(PlatformContext.FridayArena, &PlatformContext.SourceFiles, F_Str8("build/embedded_fonts.c"));

	//F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs());
	F_DispatchAcrossNLanes(1);
	return 0;
}
