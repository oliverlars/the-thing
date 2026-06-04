
#include "draw.h"
#include "os.h"

#define UI_WIDGET_TABLE_SIZE 4096

typedef struct ui_widgget
{
	u64 Key;
	f32 ActiveAnimation;
	f32 HoverAnimation;
	s32 FrameLastTouched;
} ui_widget;

typedef struct ui_context
{
	draw_context *Draw;
	os_window *Window;
	d_font Font;

	s32 Frame;

	ui_widget Widgets[UI_WIDGET_TABLE_SIZE];
} ui_context;


b32 UI_Button(ui_context *UI, str8 Name);