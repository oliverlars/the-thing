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
	fstat(Fd, &St);

	u8 *Buffer = ArenaPush(Arena, St.st_size + 1);
	read(Fd, Buffer, St.st_size);
	Buffer[St.st_size] = 0;
	close(Fd);
	ScratchEnd(Scratch);

	return (str8){ .Size = St.st_size, .Bytes = Buffer };
}

// Plain NSView backed by a CAMetalLayer. No MTKView, no display-link, no
// delegate — the host owns the run loop and pulls drawables explicitly.
@interface MetalView : NSView
@end

@implementation MetalView
+ (Class)layerClass { return [CAMetalLayer class]; }
- (CALayer *)makeBackingLayer { return [CAMetalLayer layer]; }
- (BOOL)wantsUpdateLayer { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)resizeDrawable
{
	CAMetalLayer *Layer = (CAMetalLayer *)self.layer;
	CGFloat Scale = self.window ? self.window.backingScaleFactor : 1.0;
	NSSize Size = self.bounds.size;
	Layer.contentsScale = Scale;
	Layer.drawableSize  = CGSizeMake(Size.width * Scale, Size.height * Scale);
}

- (void)viewDidChangeBackingProperties
{
	[super viewDidChangeBackingProperties];
	[self resizeDrawable];
}

- (void)setFrameSize:(NSSize)NewSize
{
	[super setFrameSize:NewSize];
	[self resizeDrawable];
}

- (void)setBoundsSize:(NSSize)NewSize
{
	[super setBoundsSize:NewSize];
	[self resizeDrawable];
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, assign) BOOL ShouldQuit;
@end

@implementation AppDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender { return NSTerminateNow; }
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender { return YES; }
@end

@interface WindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) BOOL ShouldQuit;
@end

@implementation WindowDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender
{
	self.ShouldQuit = YES;
	[NSApp terminate:nil];
	return YES;
}
@end

// NSWindow.delegate and NSApp.delegate are weak references — hold these in
// statics so ARC keeps them alive for the lifetime of the process.
static AppDelegate    *GlobalAppDelegate    = nil;
static WindowDelegate *GlobalWindowDelegate = nil;
static MetalView      *GlobalView           = nil;

os_window OS_MakeWindow(str8 Name, int Width, int Height)
{
	@autoreleasepool {
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

		GlobalAppDelegate = [[AppDelegate alloc] init];
		[NSApp setDelegate:GlobalAppDelegate];

		NSRect Frame = NSMakeRect(0, 0, Width, Height);
		NSUInteger Style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
		                   NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

		NSWindow *Window = [[NSWindow alloc] initWithContentRect:Frame
		                                               styleMask:Style
		                                                 backing:NSBackingStoreBuffered
		                                                   defer:NO];

		m_scratch_arena Scratch = ScratchBegin(0, 0);
		[Window setTitle:[NSString stringWithUTF8String:CStrFromStr8(Scratch.Arena, Name)]];
		ScratchEnd(Scratch);

		GlobalWindowDelegate = [[WindowDelegate alloc] init];
		[Window setDelegate:GlobalWindowDelegate];

		id<MTLDevice> Device = MTLCreateSystemDefaultDevice();

		GlobalView = [[MetalView alloc] initWithFrame:Frame];
		GlobalView.wantsLayer = YES;
		CAMetalLayer *Layer = (CAMetalLayer *)GlobalView.layer;
		Layer.device          = Device;
		Layer.pixelFormat     = MTLPixelFormatBGRA8Unorm;
		Layer.framebufferOnly = YES;
		// Backs the layer's drawable to the layer's bounds and rescales for
		// retina automatically — set initial size before the window orders in.
		CGFloat Scale = [NSScreen mainScreen].backingScaleFactor;
		Layer.contentsScale = Scale;
		Layer.drawableSize  = CGSizeMake(Width * Scale, Height * Scale);

		[Window setContentView:GlobalView];
		[Window center];
		[Window makeKeyAndOrderFront:nil];
		[NSApp activateIgnoringOtherApps:YES];

		os_window Result = {0};
		Result.Width     = Width;
		Result.Height    = Height;
		Result.Title     = Name;
		Result.Handle    = (__bridge_retained void*)Window;
		Result.MetalView = (__bridge void*)GlobalView;
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

void *OS_NextDrawable(os_window *Window)
{
	CAMetalLayer *Layer = (__bridge CAMetalLayer*)Window->Layer;
	id<CAMetalDrawable> Drawable = [Layer nextDrawable];
	return (__bridge void*)Drawable;
}

void OS_Run(os_window *Window, os_frame_fn FrameFn, void *UserData)
{
	@autoreleasepool {
		[NSApp finishLaunching];

		while (True)
		{
			@autoreleasepool {
				NSEvent *Event;
				while ((Event = [NSApp nextEventMatchingMask:NSEventMaskAny
				                                  untilDate:[NSDate distantPast]
				                                     inMode:NSDefaultRunLoopMode
				                                    dequeue:YES]))
				{
					[NSApp sendEvent:Event];
				}

				if (GlobalAppDelegate.ShouldQuit) break;

				v2i Size = v2i(OS_WindowWidth(Window), OS_WindowHeight(Window));
				FrameFn(UserData, Size);
			}
		}
	}
}
