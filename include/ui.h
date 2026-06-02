
#include "draw.h"
#include "os.h"
typedef struct ui_context {
	draw_context *Draw;
	os_window *Window;
	d_font Font;
} ui_context;


b32 UI_Button(ui_context *UI, char *Name);