#include "os.h"
#include "log.h"
#include "memory.h"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <mach/mach_time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

u64 OS_TimeStampNow(void)
{
	return mach_absolute_time();
}

u64 OS_LastModified(str8 Path)
{
	struct stat Stat;
	m_scratch_arena Scratch = ScratchBegin(0, 0);
	if (stat(CStrFromStr8(Scratch.Arena, Path), &Stat) == 0)
	{
		return Stat.st_mtime; 
	}
	return 0;
}

f32 OS_MilliSecondsFromTimeStamp(u64 TimeStamp)
{
	static mach_timebase_info_data_t Info = {0};
	if (Info.denom == 0) mach_timebase_info(&Info);
	return (f32)(TimeStamp * Info.numer / Info.denom) / 1000000.0f;
}

str8 OS_LoadEntireFileIntoMemory(m_arena *Arena, str8 Filename)
{
	m_scratch_arena Scratch = ScratchBegin(&Arena, 1);
	char *Path = CStrFromStr8(Scratch.Arena, Filename);
	int Fd = open(Path, O_RDONLY);
	if (Fd < 0)
	{
		LogError("Could not open file: ?cstr", Path);
		ScratchEnd(Scratch);
		return (str8){0};
	}

	struct stat St;
	if (fstat(Fd, &St) < 0 || !S_ISREG(St.st_mode))
	{
		// open() happily opens directories read-only, but read() on them fails.
		LogError("Not a regular file: ?cstr", Path);
		close(Fd);
		ScratchEnd(Scratch);
		return (str8){0};
	}

	u8 *Buffer = ArenaPush(Arena, St.st_size + 1);
	ssize_t BytesRead = read(Fd, Buffer, St.st_size);
	close(Fd);
	if (BytesRead != St.st_size)
	{
		LogError("Could not read file: ?cstr", Path);
		ScratchEnd(Scratch);
		return (str8){0};
	}
	Buffer[St.st_size] = 0;
	ScratchEnd(Scratch);

	return (str8){ .Size = St.st_size, .Bytes = Buffer };
}

// Ring buffer for OS_NextEvent. The run loop classifies each Cocoa NSEvent
// it pulls off the system queue and pushes a corresponding os_event here;
// the host's frame callback drains it with OS_NextEvent.
#define OS_EVENT_QUEUE_CAP 1024
static os_event GlobalEventQueue[OS_EVENT_QUEUE_CAP];
static int      GlobalEventHead = 0;  // next to pop
static int      GlobalEventTail = 0;  // next to push

static void OS_PushEvent(os_event E)
{
	int Next = (GlobalEventTail + 1) % OS_EVENT_QUEUE_CAP;
	if (Next == GlobalEventHead) return;  // full — drop oldest by dropping the new one
	GlobalEventQueue[GlobalEventTail] = E;
	GlobalEventTail = Next;
}

b32 OS_NextEvent(os_window *Window, os_event *Out)
{
	(void)Window;
	if (GlobalEventHead == GlobalEventTail) return False;
	*Out = GlobalEventQueue[GlobalEventHead];
	GlobalEventHead = (GlobalEventHead + 1) % OS_EVENT_QUEUE_CAP;
	return True;
}

#define OS_KEY_BIT_COUNT 256
static u64 GlobalKeyHeld     [OS_KEY_BIT_COUNT / 64];
static u64 GlobalKeyPressed  [OS_KEY_BIT_COUNT / 64];
static u64 GlobalKeyReleased [OS_KEY_BIT_COUNT / 64];
static u32 GlobalMouseHeld;
static u32 GlobalMousePressed;
static u32 GlobalMouseReleased;

static void OS_BitSet  (u64 *Bits, int I) { Bits[I >> 6] |=  (1ULL << (I & 63)); }
static void OS_BitClear(u64 *Bits, int I) { Bits[I >> 6] &= ~(1ULL << (I & 63)); }
static b32  OS_BitTest (u64 *Bits, int I) { return (b32)((Bits[I >> 6] >> (I & 63)) & 1ULL); }

static void OS_UpdateInputState(os_event E)
{
	switch (E.Type)
	{
		case OS_EVENT_KEY_DOWN:
			if (E.KeyCode >= 0 && E.KeyCode < OS_KEY_BIT_COUNT) {
				OS_BitSet(GlobalKeyHeld,    E.KeyCode);
				OS_BitSet(GlobalKeyPressed, E.KeyCode);
			}
			break;
		case OS_EVENT_KEY_UP:
			if (E.KeyCode >= 0 && E.KeyCode < OS_KEY_BIT_COUNT) {
				OS_BitClear(GlobalKeyHeld,    E.KeyCode);
				OS_BitSet  (GlobalKeyReleased, E.KeyCode);
			}
			break;
		case OS_EVENT_MOUSE_DOWN:
			if (E.KeyCode >= 0 && E.KeyCode < 32) {
				GlobalMouseHeld    |=  (1U << E.KeyCode);
				GlobalMousePressed |=  (1U << E.KeyCode);
			}
			break;
		case OS_EVENT_MOUSE_UP:
			if (E.KeyCode >= 0 && E.KeyCode < 32) {
				GlobalMouseHeld     &= ~(1U << E.KeyCode);
				GlobalMouseReleased |=  (1U << E.KeyCode);
			}
			break;
		default: break;
	}
}

static void OS_ResetTransitions(void)
{
	for (int I = 0; I < (int)(sizeof(GlobalKeyPressed) / sizeof(GlobalKeyPressed[0])); I++) {
		GlobalKeyPressed [I] = 0;
		GlobalKeyReleased[I] = 0;
	}
	GlobalMousePressed  = 0;
	GlobalMouseReleased = 0;
}

b32 OS_KeyDown    (int Key)    { return (Key    >= 0 && Key    < OS_KEY_BIT_COUNT) ? OS_BitTest(GlobalKeyHeld,     Key)    : False; }
b32 OS_KeyPressed (int Key)    { return (Key    >= 0 && Key    < OS_KEY_BIT_COUNT) ? OS_BitTest(GlobalKeyPressed,  Key)    : False; }
b32 OS_KeyReleased(int Key)    { return (Key    >= 0 && Key    < OS_KEY_BIT_COUNT) ? OS_BitTest(GlobalKeyReleased, Key)    : False; }
b32 OS_MouseDown    (int Button) { return (Button >= 0 && Button < 32) ? (b32)((GlobalMouseHeld     >> Button) & 1U) : False; }
b32 OS_MousePressed (int Button) { return (Button >= 0 && Button < 32) ? (b32)((GlobalMousePressed  >> Button) & 1U) : False; }
b32 OS_MouseReleased(int Button) { return (Button >= 0 && Button < 32) ? (b32)((GlobalMouseReleased >> Button) & 1U) : False; }

static v2f WindowPointToRender(NSPoint WindowPt, NSWindow *NSWin, NSView *View)
{
	NSPoint LocView = [View convertPoint:WindowPt fromView:nil];
	CGFloat Scale   = NSWin.backingScaleFactor;
	return v2f((f32)(LocView.x * Scale), (f32)(LocView.y * Scale));
}

static os_event OS_EventFromNSEvent(NSEvent *E, NSWindow *NSWin, NSView *View)
{
	os_event Out = {0};
	switch (E.type)
	{
		case NSEventTypeLeftMouseDown:
		case NSEventTypeRightMouseDown:
		case NSEventTypeOtherMouseDown:
			Out.Type          = OS_EVENT_MOUSE_DOWN;
			Out.KeyCode       = (int)E.buttonNumber;
			Out.MousePosition = WindowPointToRender(E.locationInWindow, NSWin, View);
			break;

		case NSEventTypeLeftMouseUp:
		case NSEventTypeRightMouseUp:
		case NSEventTypeOtherMouseUp:
			Out.Type          = OS_EVENT_MOUSE_UP;
			Out.KeyCode       = (int)E.buttonNumber;
			Out.MousePosition = WindowPointToRender(E.locationInWindow, NSWin, View);
			break;

		case NSEventTypeMouseMoved:
		case NSEventTypeLeftMouseDragged:
		case NSEventTypeRightMouseDragged:
		case NSEventTypeOtherMouseDragged:
			Out.Type          = OS_EVENT_MOUSE_MOVE;
			Out.MousePosition = WindowPointToRender(E.locationInWindow, NSWin, View);
			break;

		case NSEventTypeScrollWheel:
			Out.Type          = OS_EVENT_SCROLL;
			Out.MousePosition = WindowPointToRender(E.locationInWindow, NSWin, View);
			Out.Scroll        = v2f((f32)E.scrollingDeltaX, (f32)E.scrollingDeltaY);
			break;

		case NSEventTypeKeyDown:
			if (E.isARepeat) break;
			Out.Type    = OS_EVENT_KEY_DOWN;
			Out.KeyCode = (int)E.keyCode;
			break;

		case NSEventTypeKeyUp:
			Out.Type    = OS_EVENT_KEY_UP;
			Out.KeyCode = (int)E.keyCode;
			break;

		default:
			break;
	}
	return Out;
}

os_window OS_MakeWindow(str8 Name, int Width, int Height)
{
	@autoreleasepool {
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		[NSApp finishLaunching];

		NSRect Frame = NSMakeRect(0, 0, Width, Height);
		NSUInteger Style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
		                   NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

		NSWindow *Window = [[NSWindow alloc] initWithContentRect:Frame
		                                               styleMask:Style
		                                                 backing:NSBackingStoreBuffered
		                                                   defer:NO];
		Window.releasedWhenClosed = NO;

		m_scratch_arena Scratch = ScratchBegin(0, 0);
		[Window setTitle:[NSString stringWithUTF8String:CStrFromStr8(Scratch.Arena, Name)]];
		ScratchEnd(Scratch);

		[Window setAcceptsMouseMovedEvents:YES];

		id<MTLDevice> Device = MTLCreateSystemDefaultDevice();

		NSView *View = [[NSView alloc] initWithFrame:Frame];
		CAMetalLayer *Layer = [CAMetalLayer layer];
		View.layer = Layer;
		View.wantsLayer = YES;
		Layer.device          = Device;
		Layer.pixelFormat     = MTLPixelFormatBGRA10_XR_sRGB;
		Layer.framebufferOnly = YES;
		CGColorSpaceRef ColourSpace = CGColorSpaceCreateWithName(kCGColorSpaceExtendedSRGB);
		Layer.colorspace = ColourSpace;
		CGColorSpaceRelease(ColourSpace);
		CGFloat Scale = [NSScreen mainScreen].backingScaleFactor;
		Layer.contentsScale = Scale;
		Layer.drawableSize  = CGSizeMake(Width * Scale, Height * Scale);

		[Window setContentView:View];
		[Window center];
		[Window makeKeyAndOrderFront:nil];
		[NSApp activateIgnoringOtherApps:YES];

		os_window Result = {0};
		Result.Width  = (int)Layer.drawableSize.width;
		Result.Height = (int)Layer.drawableSize.height;
		Result.Title     = Name;
		Result.Handle    = (__bridge_retained void*)Window;
		Result.MetalView = (__bridge void*)View;
		Result.Layer     = (__bridge void*)Layer;
		Result.Device    = (__bridge_retained void*)Device;
		return Result;
	}
}

int OS_WindowWidth(os_window *Window)
{
	CAMetalLayer *Layer = (__bridge CAMetalLayer*)Window->Layer;
	return (int)Layer.drawableSize.width;
}

int OS_WindowHeight(os_window *Window)
{
	CAMetalLayer *Layer = (__bridge CAMetalLayer*)Window->Layer;
	return (int)Layer.drawableSize.height;
}

v2f OS_MousePosition(os_window *Window)
{
	NSWindow *NSWin = (__bridge NSWindow*)Window->Handle;
	NSView   *View  = (__bridge NSView*)Window->MetalView;
	if (!NSWin || !View) return v2f(0, 0);
	NSPoint MouseInWindow = [NSWin mouseLocationOutsideOfEventStream];
	NSPoint MouseInView   = [View convertPoint:MouseInWindow fromView:nil];
	CGFloat Scale         = NSWin.backingScaleFactor;
	f32 X = (f32)(MouseInView.x * Scale);
	f32 Y = (f32)(MouseInView.y * Scale);
	return v2f(X, Y);
}

void *objc_autoreleasePoolPush(void);
void  objc_autoreleasePoolPop(void *Token);

void *Pool;

void OS_BeginFrame()
{
	Pool = objc_autoreleasePoolPush();
}

void OS_EndFrame()
{
	objc_autoreleasePoolPop(Pool);
}

void *OS_NextDrawable(os_window *Window)
{
	CAMetalLayer *Layer = (__bridge CAMetalLayer*)Window->Layer;
	id<CAMetalDrawable> Drawable = [Layer nextDrawable];
	return (__bridge void*)Drawable;
}

b32 OS_UpdateWindow(os_window *Window)
{
	@autoreleasepool {

		NSWindow *NSWin = (__bridge NSWindow*)Window->Handle;
		@autoreleasepool {
			OS_ResetTransitions();

			NSEvent *Event;
			while ((Event = [NSApp nextEventMatchingMask:NSEventMaskAny
																																					untilDate:[NSDate distantPast]
																																								inMode:NSDefaultRunLoopMode
																																							dequeue:YES]))
			{
				os_event Mapped = OS_EventFromNSEvent(Event, NSWin, (__bridge NSView*)Window->MetalView);
				if (Mapped.Type != OS_EVENT_NONE)
				{
					OS_PushEvent(Mapped);
					OS_UpdateInputState(Mapped);
				}
				b32 IsKey = Event.type == NSEventTypeKeyDown || Event.type == NSEventTypeKeyUp;
				if (!IsKey || (Event.modifierFlags & NSEventModifierFlagCommand))
					[NSApp sendEvent:Event];
			}

			if (!NSWin.isVisible && !NSWin.isMiniaturized) return False;

			NSView *View = (__bridge NSView*)Window->MetalView;
			CAMetalLayer *Layer = (__bridge CAMetalLayer*)Window->Layer;
			CGFloat Scale = NSWin.backingScaleFactor;
			CGSize Want = CGSizeMake(View.bounds.size.width * Scale, View.bounds.size.height * Scale);
			if (!CGSizeEqualToSize(Layer.drawableSize, Want))
			{
				Layer.contentsScale = Scale;
				Layer.drawableSize  = Want;
			}

			Window->Width  = OS_WindowWidth(Window);
			Window->Height = OS_WindowHeight(Window);
		}
	}
	return True;
}
