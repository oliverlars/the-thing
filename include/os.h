#pragma once
#include "strings.h"
#include "maths.h"

typedef void* os_handle;

typedef struct os_window
{
	int Width;
	int Height;

	os_handle Handle;   // NSWindow*
	os_handle MetalView; // MTKView*
	os_handle Layer;    // CAMetalLayer*
	os_handle Device;   // id<MTLDevice>

	str8 Title;
}
os_window;

typedef enum os_event_type
{
	OS_EVENT_NONE,
	OS_EVENT_RESIZE,
	OS_EVENT_KEY_DOWN,
	OS_EVENT_KEY_UP,
	OS_EVENT_MOUSE_DOWN,
	OS_EVENT_MOUSE_UP,
	OS_EVENT_MOUSE_MOVE,
	OS_EVENT_CLOSE,
} os_event_type;

typedef struct os_event
{
	os_event_type Type;
	int KeyCode;
	v2f MousePosition;
} os_event;

typedef void (*os_frame_fn)(void *UserData, v2i ViewportSize);

os_window OS_MakeWindow(str8 Name, int Width, int Height);
void OS_Run(os_window *Window, os_frame_fn FrameFn, void *UserData);

void *OS_NextDrawable(os_window *Window); // CAMetalDrawable* (autoreleased)

int OS_WindowWidth(os_window *Window);
int OS_WindowHeight(os_window *Window);

u64 OS_TimeStampNow(void);
f32 OS_MilliSecondsFromTimeStamp(u64 TimeStamp);

str8 OS_LoadEntireFileIntoMemory(m_arena *Arena, str8 Filename);
