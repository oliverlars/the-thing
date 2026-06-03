
#include "draw.h"
#include "os.h"

typedef struct ui_widgget {
	u64 Key;
	f32 ActiveAnimation;
	f32 HoverAnimation;
	s32 FramesSinceLastTouched;
} ui_widget;

typedef struct ui_context {
	draw_context *Draw;
	os_window *Window;
	d_font Font;


	ui_widget Widgets[4096];
} ui_context;


b32 UI_Button(ui_context *UI, str8 Name);