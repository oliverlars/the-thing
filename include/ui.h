
#include "draw.h"
#include "os.h"

#define UI_WIDGET_TABLE_SIZE 4096


typedef enum ui_widget_flags
{
	UI_LAYOUT_FIXED  = 1 << 0,
	UI_LAYOUT_FILL   = 1 << 1,
	UI_LAYOUT_HUG    = 1 << 2,
	UI_LAYOUT_ROW    = 1 << 3,
	UI_LAYOUT_COLUMN = 1 << 4,

	UI_RENDER_BACKGROUND = 1 << 5
}
ui_widget_flags;

typedef struct ui_animation
{
	f32 Target;
	f32 Value;
}
ui_animation;

typedef struct ui_widget
{
	ui_widget_flags Flags;
	str8 String;
	u64 Key;
	ui_animation ActiveAnimation;
	ui_animation HoverAnimation;
	s32 FrameLastTouched;

	v2f Padding;
	v2f Position;
	v2f Size;

	struct ui_widget *Parent;
	struct ui_widget *First;
	struct ui_widget *Last;
	struct ui_widget *Next;
}
ui_widget;

typedef struct ui_context
{
	draw_context *Draw;
	os_window *Window;
	d_font Font;

	s32 Frame;

	ui_widget_flags Flags;
	v2f       PaddingStack[4096];
	int       PaddingIndex;
	ui_widget LayoutStack[4096];
	int       LayoutIndex;
	ui_widget Widgets[UI_WIDGET_TABLE_SIZE];
	int WidgetIndex;

	u64 Active;
	u64 HoverNext;
	u64 Hover;

	ui_widget *Root;
}
ui_context;

void UI_Begin(ui_context *UI);
void UI_End(ui_context *UI);

b32 UI_Button(ui_context *UI, str8 Name);

v2f UI_CurrentPadding(ui_context *UI);

void UI_LayoutPop(ui_context *UI);
void UI_LayoutPush(ui_context *UI, ui_widget Layout);

void UI_AppendAsChild(ui_widget *Parent, ui_widget *Child);
void UI_AppendAsSibling(ui_widget *OlderSibling, ui_widget *YoungerSibling);

void UI_Layout(ui_context *UI);
v2f UI_LayoutRow(ui_context *UI, ui_widget *Row, v2f MaxSize);
v2f UI_LayoutColumn(ui_context *UI, ui_widget *Column, v2f MaxSize);

void UI_LayoutPositions(ui_context *UI, ui_widget *Widget, v2f Position);
void UI_LayoutRootPosition(ui_context *UI, ui_widget *Widget, v2f Position);
void UI_RenderWidgets(ui_context *UI, ui_widget *Widget);

#define DeferLoop(Start, End) for(int _I_ = ((Start), 0); _I_ < 1; ((End), _I_++))

#define UI_FLAG(UI, Flag) DeferLoop((UI)->Flags |= (Flag), (UI)->Flags &= ~(Flag))

#define UI_PADDING(UI, Pad) DeferLoop((UI)->PaddingStack[(UI)->PaddingIndex++] = v2f(Pad, Pad), (UI)->PaddingStack[--(UI)->PaddingIndex] = v2f(0.0, 0.0))

#define UI_PANEL(UI, P, S) UI_PADDING(UI, 5) DeferLoop(UI_LayoutPush(UI, (ui_widget){ .Flags = UI_LAYOUT_COLUMN | UI_LAYOUT_FIXED | UI_RENDER_BACKGROUND, .Position = P, .Size = S}), UI_LayoutPop(UI))

#define UI_ROW(UI, LayoutFlag) DeferLoop(UI_LayoutPush(UI, (ui_widget){ .Flags = UI_LAYOUT_ROW | LayoutFlag | (UI)->Flags}), UI_LayoutPop(UI))

#define UI_COLUMN(UI, LayoutFlag) DeferLoop(UI_LayoutPush(UI, (ui_widget){ .Flags = UI_LAYOUT_COLUMN | LayoutFlag | (UI)->Flags}), UI_LayoutPop(UI))

#define UI_FILL(UI) DeferLoop((UI)->Flags |= UI_LAYOUT_FILL, (UI)->Flags &= ~UI_LAYOUT_FILL)