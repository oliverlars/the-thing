#include "misc.h"
#include "memory.h"
#include "render_common.h"
#include "strings.h"
#include "log.h"
#include "maths.h"
#include "os.h"
#include "render.h"
#include "draw.h"
#include "ui.h"

#include "embedded_fonts.h"

#include <math.h>


#include "app.h"

#include <dlfcn.h>

typedef struct platform_context
{
	app_memory *Memory;
	void (*AppUpdate)(void *UserData, v2i ViewportSize);
}
platform_context;

static void PlatformFrame(void *UserData, v2i ViewportSize)
{
	platform_context *Context = (platform_context*)UserData;
	Context->AppUpdate(Context->Memory, ViewportSize);
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

	void *AppLib = dlopen("build/app.dynlib", RTLD_NOW | RTLD_LOCAL);
	if(AppLib)
	{
		PlatformContext.AppUpdate = dlsym(AppLib, "AppUpdate");
	}


	OS_Run(&Window, PlatformFrame, &PlatformContext);
	return 0;
}
