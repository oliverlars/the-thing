
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

	// Generic, flag-driven render stages. A widget opts into the stages it
	// wants; the renderer is a flat sequence of these, not a per-kind switch.
	// A widget with none of these (and no CustomDraw) is pure layout/input
	// structure and draws nothing.
	UI_DRAW_BACKGROUND = 1 << 5, // rounded quad filled with Colour + outline
	UI_DRAW_TEXT       = 1 << 6, // DisplayString centred in the box
	UI_DRAW_HOT        = 1 << 7, // hover grow + active extrusion on the bg

	// A floating widget is its own layout root: parented to nothing, anchored
	// manually, and laid out + rendered in a pass after the main tree so it
	// sits on top. Combo dropdowns, tooltips and menus are all "floating root
	// + later pass".
	UI_LAYOUT_FLOATING = 1 << 8,
}
ui_widget_flags;

// A box is a layout + input handle, not a visual. UI_Interact runs the generic
// hover/active machinery against a box's (retained) rect and reports what the
// pointer did this frame. Bespoke widgets consume the signal however they like.
typedef struct ui_signal
{
	b32 Hovered; // pointer is over the box
	b32 Pressed; // box is the active (held) widget
	b32 Clicked; // mouse released this frame while the box was active
}
ui_signal;

typedef struct ui_animation
{
	f32 Target;
	f32 Value;
}
ui_animation;

// Forward declaration so ui_widget's CustomDraw pointer refers to the
// file-scope ui_context tag (defined below), not a member-local one.
struct ui_context;

typedef enum ui_widget_kind
{
	UI_WIDGET_BUTTON,
	UI_WIDGET_COMBO_BOX,	
}
ui_widget_kind;

typedef struct ui_widget
{
	ui_widget_kind  Kind;
	d_align Align;
	ui_widget_flags Flags;
	str8 String;
	str8 DisplayString;
	str8 Icon;
	u64 Key;
	ui_animation ActiveAnimation;
	ui_animation HoverAnimation;
	s32 FrameLastTouched;

	b32 Toggled;

	f32 Gap;
	v2f Padding;
	v2f Position;
	v2f Size;

	// Generic render-stage style, consumed by the UI_DRAW_* flags.
	v4f Colour;
	v4f TextColour;
	v4f OutlineColour;
	f32 OutlineThickness;
	f32 CornerRadius;

	b32 Selected; // e.g. a combo item that is the active element
	s32 Layer;    // hover priority; floating overlays sit on a higher layer

	// Bespoke render escape hatch: called after the generic stages, in this
	// widget's own render pass (so floating items land on top).
	void (*CustomDraw)(struct ui_context *UI, struct ui_widget *Widget);

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
	d_font IconFont;

	s32 Frame;

	ui_widget_flags Flags;
	f32 NextGap;
	ui_widget LayoutStack[4096];
	int       LayoutIndex;
	ui_widget Widgets[UI_WIDGET_TABLE_SIZE];
	int WidgetIndex;

	u64 Active;
	u64 HoverNext;
	s32 HoverNextLayer; // layer of the current HoverNext candidate
	u64 Hover;

	// Floating layout roots (combo dropdowns, etc.) registered this frame, laid
	// out and rendered after the main tree so they sit on top.
	ui_widget *FloatRoots[64];
	int FloatCount;

	ui_widget *CurrentWidget;

	ui_widget *Root;
}
ui_context;

void UI_Begin(ui_context *UI);
void UI_End(ui_context *UI);

ui_widget *UI_GetWidget(ui_context *UI, str8 Name);

ui_signal UI_Interact(ui_context *UI, ui_widget *Widget);

b32 UI_Button(ui_context *UI, str8 Name);
int UI_ComboBox(ui_context *UI, str8 Name, str8 *Elements, int ElementCount, int ActiveElement);

void UI_LayoutPop(ui_context *UI);
void UI_LayoutPush(ui_context *UI, ui_widget Layout);

void UI_AppendAsChild(ui_widget *Parent, ui_widget *Child);
void UI_AppendAsSibling(ui_widget *OlderSibling, ui_widget *YoungerSibling);

void UI_Layout(ui_context *UI);
v2f UI_LayoutRow(ui_context *UI, ui_widget *Row, v2f MaxSize);
v2f UI_LayoutColumn(ui_context *UI, ui_widget *Column, v2f MaxSize);

void UI_LayoutPositions(ui_context *UI, ui_widget *Widget, v2f Position);
void UI_LayoutRootPosition(ui_context *UI, ui_widget *Widget, v2f Position);
void UI_DrawWidget(ui_context *UI, ui_widget *Widget);
void UI_RenderWidgets(ui_context *UI, ui_widget *Widget);

#define UI_DEFAULT_GAP 10.0f
#define UI_DEFAULT_PADDING 25.0f
#define UI_DEFAULT_PADDING2 v2f(UI_DEFAULT_PADDING, UI_DEFAULT_PADDING)

#define DeferLoop(Start, End) for(int _I_ = ((Start), 0); _I_ < 1; ((End), _I_++))

#define UI_FLAG(UI, Flag) DeferLoop((UI)->Flags |= (Flag), (UI)->Flags &= ~(Flag))

#define UI_PADDING(UI, Pad) DeferLoop((UI)->PaddingStack[(UI)->PaddingIndex++] = v2f(Pad, Pad), (UI)->PaddingStack[--(UI)->PaddingIndex] = v2f(0.0, 0.0))

#define UI_PANEL(UI, P, S) DeferLoop(UI_LayoutPush(UI, (ui_widget){ .Gap = UI_DEFAULT_GAP, .Padding = UI_DEFAULT_PADDING2, .Flags = UI_LAYOUT_COLUMN | UI_LAYOUT_FIXED | UI_DRAW_BACKGROUND, .Colour = v4f(0.125, 0.129, 0.14, 1.0), .OutlineColour = v4f(0.3, 0.3, 0.3, 1.0), .OutlineThickness = 2.0f, .CornerRadius = 15.0f, .Position = P, .Size = S}), UI_LayoutPop(UI))

#define UI_GAP(UI, G) DeferLoop((UI)->NextGap = (G), (UI)->NextGap = UI_DEFAULT_GAP)

#define UI_ROW(UI, LayoutFlag) DeferLoop((UI_LayoutPush(UI, (ui_widget){ .Gap = (UI)->NextGap, .Flags = UI_LAYOUT_ROW | LayoutFlag | (UI)->Flags}), (UI)->NextGap = UI_DEFAULT_GAP), UI_LayoutPop(UI))

#define UI_COLUMN(UI, LayoutFlag) DeferLoop((UI_LayoutPush(UI, (ui_widget){ .Gap = (UI)->NextGap, .Flags = UI_LAYOUT_COLUMN | LayoutFlag | (UI)->Flags}), (UI)->NextGap = UI_DEFAULT_GAP), UI_LayoutPop(UI))

#define UI_FILL(UI) DeferLoop((UI)->Flags |= UI_LAYOUT_FILL, (UI)->Flags &= ~UI_LAYOUT_FILL)