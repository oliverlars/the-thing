#include "maths.h"
#include "memory.h"
#include "misc.h"
#include "draw.h"
#include "os.h"

#include "ui.h"

void UI_Begin(ui_context *UI)
{
	UI->Hover = UI->HoverNext;
	UI->HoverNext = 0;
	UI->NextGap = UI_DEFAULT_GAP;
	for(int WidgetIndex = 0; WidgetIndex < UI_WIDGET_TABLE_SIZE; WidgetIndex++)
	{
		ui_widget *Widget = UI->Widgets + WidgetIndex;
		Widget->ActiveAnimation.Value = LerpF32(Widget->ActiveAnimation.Value, Widget->ActiveAnimation.Target, 0.1);
		Widget->HoverAnimation.Value = LerpF32(Widget->HoverAnimation.Value, Widget->HoverAnimation.Target, 0.1);
	}
}

void UI_End(ui_context *UI)
{
	UI->Frame += 1;
	UI->WidgetIndex = 0;
}

ui_widget *GetWidget(ui_context *UI, str8 Name)
{
	u64 Key = M_FNVHashString(M_FNV_HASH_START, Name);
	ui_widget *Widget = UI->Widgets + (Key % UI_WIDGET_TABLE_SIZE);
	if(Widget->Key == 0)
	{
		MemoryZero(Widget, sizeof(ui_widget));
		Widget->Flags = UI->Flags;
		Widget->Key = Key;
	}
	else if(Widget->Key != Key)
	{
		if(Widget->FrameLastTouched < UI->Frame)
		{
			MemoryZero(Widget, sizeof(ui_widget));
			Widget->Flags = UI->Flags;
			Widget->Key = Key;
		}
		else
		{
			for(int I = 1; I < UI_WIDGET_TABLE_SIZE; I++)
			{
				Widget = UI->Widgets + ((Key % UI_WIDGET_TABLE_SIZE) + I) % UI_WIDGET_TABLE_SIZE;
				if(Widget->Key == Key) break;
			}
		}
	}
	Widget->FrameLastTouched = UI->Frame;
	Widget->First = 0;
	Widget->Last = 0;
	Widget->Next = 0;
	Widget->Parent = 0;
	return Widget;
}

b32 UI_Button(ui_context *UI, str8 Name)
{
	ui_widget *Widget = GetWidget(UI, Name);
	Widget->String = Name;
	ui_widget *Layout = UI->LayoutStack + UI->LayoutIndex - 1;
	UI_AppendAsChild(Layout, Widget);
	b32 Clicked = False;
	v2f MousePosition = OS_MousePosition(UI->Window);
	// OS_MousePosition is in the renderer's bottom-left origin (Y-up) space;
	// flip Y into the UI's top-left origin (Y-down) space for hit testing.
	MousePosition.Y = UI->Draw->ViewportSize.Y - MousePosition.Y;

	
	if(UI->Hover == Widget->Key)
	{
		Widget->HoverAnimation.Target = 1.0;
		if(OS_MouseDown(OS_MOUSE_LEFT))
		{
			UI->Active = Widget->Key;
			Widget->ActiveAnimation.Target = 1.0;
		}
	}
	else
	{
		Widget->HoverAnimation.Target = 0.0;
	}

	if(UI->Active == Widget->Key)
	{
		if(OS_MouseReleased(OS_MOUSE_LEFT))
		{
			Clicked = True;
			UI->Active = 0;
			Widget->ActiveAnimation.Target = 0.0;
		}
	}

	if (MousePosition.X >= Widget->Position.X && MousePosition.X < (Widget->Position.X + Widget->Size.X) &&
		MousePosition.Y >= Widget->Position.Y && MousePosition.Y <= (Widget->Position.Y + Widget->Size.Y))
	{
		UI->HoverNext = Widget->Key;
	}
	Widget->Size.X = D_Str8WidthFromHeight(UI->Font, Name, 50) + 30;
	Widget->Size.Y = D_Str8HeightFromHeight(UI->Font, 50) + 30;
	return Clicked;
}

int UI_ComboBox(ui_context *UI, str8 Name, str8 *Elements, int ElementCount, int ActiveElement)
{
	ui_widget *Widget = GetWidget(UI, Name);
	Widget->String = Name;
	ui_widget *Layout = UI->LayoutStack + UI->LayoutIndex - 1;
	UI_AppendAsChild(Layout, Widget);
	int ClickedElement = ActiveElement;
	v2f MousePosition = OS_MousePosition(UI->Window);
	// OS_MousePosition is in the renderer's bottom-left origin (Y-up) space;
	// flip Y into the UI's top-left origin (Y-down) space for hit testing.
	MousePosition.Y = UI->Draw->ViewportSize.Y - MousePosition.Y;

	f32 MaxWidth = 0;
	for(int E = 0; E < ElementCount; E++)
	{
		MaxWidth = Max(MaxWidth, D_Str8WidthFromHeight(UI->Font, Elements[E], 50) + 30);
	}
	Widget->Size.X = MaxWidth;
	Widget->Size.Y = D_Str8HeightFromHeight(UI->Font, 50) + 30;
	f32 ElementHeight = Widget->Size.Y;

	UI_COLUMN(UI, UI_LAYOUT_FIXED)
	{
		if(UI->Active == Widget->Key)
		{
			if(OS_MouseReleased(OS_MOUSE_LEFT))
			{
				if(UI->Hover == Widget->Key)
				{
					Widget->Toggled = !Widget->Toggled;
					Widget->ActiveAnimation.Target = 0.0;
					UI->Active = 0;
				}
			}
		}
		else if(UI->Hover == Widget->Key)
		{
			if(OS_MouseDown(OS_MOUSE_LEFT))
			{
				UI->Active = Widget->Key;
				Widget->HoverAnimation.Target = 1.0;
			}
		}

		if(Widget->Toggled)
		{
			for(int E = 0; E < ElementCount; E++)
			{
				UI_Button(UI, Str8PushF(TemporaryArena(), "?S?s32", Elements[E], E));
			}

			if(OS_MouseReleased(OS_MOUSE_LEFT))
			{
				UI->Active = 0;
				Widget->ActiveAnimation.Target = 0.0;
			}
		}
	}

	if (MousePosition.X >= Widget->Position.X && MousePosition.X < (Widget->Position.X + Widget->Size.X) &&
		MousePosition.Y >= Widget->Position.Y && MousePosition.Y <= (Widget->Position.Y + Widget->Size.Y))
	{
		UI->HoverNext = Widget->Key;
	}

	return ClickedElement;
}

void UI_LayoutPop(ui_context *UI)
{
	assert(UI->LayoutIndex > 0);
	UI->LayoutIndex--;
}

void UI_LayoutPush(ui_context *UI, ui_widget Layout)
{
	UI->LayoutStack[UI->LayoutIndex] = Layout;
	if(UI->LayoutIndex > 0)
	{
		UI_AppendAsChild(UI->LayoutStack + UI->LayoutIndex - 1, UI->LayoutStack + UI->LayoutIndex);
	}
	else
	{
		UI->Root = UI->LayoutStack + 0;
	}
	UI->LayoutIndex++;
}

void UI_AppendAsChild(ui_widget *Parent, ui_widget *Child)
{
	if(!Parent->First)
	{
		Parent->First = Child;
		Parent->Last = Child;
	}
	else
	{
		Parent->Last->Next = Child;
		Parent->Last = Child;
	}
	Child->Parent = Parent;
}

void UI_AppendAsSibling(ui_widget *OlderSibling, ui_widget *YoungerSibling)
{
	if(!OlderSibling->Next)
	{
		OlderSibling->Next = YoungerSibling;
	}
	else
	{
		if(OlderSibling->Parent)
		{
			UI_AppendAsChild(OlderSibling->Parent, YoungerSibling);
		}
		else
		{
			ui_widget *Sibling = OlderSibling->Next;
			while(Sibling->Next) Sibling = Sibling->Next;
			Sibling->Next = YoungerSibling;
		}
	}
}

void UI_Layout(ui_context *UI)
{
	//for(ui_widget *Child = UI->Root->First; Child; Child = Child->Next)
	v2f ChildSize = UI->Root->Size;
	ChildSize = SubV2F(ChildSize, UI->Root->Padding);
	{
		if(UI->Root->Flags & UI_LAYOUT_ROW)    UI_LayoutRow(UI, UI->Root, ChildSize);
		if(UI->Root->Flags & UI_LAYOUT_COLUMN) UI_LayoutColumn(UI, UI->Root, ChildSize);
	}
}

v2f UI_LayoutRow(ui_context *UI, ui_widget *Row, v2f MaxSize)
{
	v2f TotalSize = v2f(0.0, 0.0);
	// Offset by Padding, decrease MaxSize by Padding * 2
	v2f Paddingx2 = MulV2F(Row->Padding, v2f(2, 2));
	MaxSize = SubV2F(MaxSize, Paddingx2);
	int NumberOfChildren = 0;
	for(ui_widget *Sibling = Row->First; Sibling; Sibling = Sibling->Next)
	{
		v2f ChildSize = v2f(0.0, 0.0);
		if(Sibling->Flags & UI_LAYOUT_ROW)
		{
			ChildSize = UI_LayoutRow(UI, Sibling, MaxSize);
		}
		else if(Sibling->Flags & UI_LAYOUT_COLUMN)
		{
			ChildSize = UI_LayoutColumn(UI, Sibling, MaxSize);
		}
		else
		{
			ChildSize = Sibling->Size;
		}
		TotalSize.X += ChildSize.X;
		TotalSize.Y = Max(TotalSize.Y, ChildSize.Y);
		NumberOfChildren++;
	}
	if(NumberOfChildren > 1) TotalSize.X += Row->Gap * (f32)(NumberOfChildren - 1);
	v2f SpaceLeft = SubV2F(MaxSize, TotalSize);
	Assert(SpaceLeft.X >= 0.0f);
	Assert(SpaceLeft.Y >= 0.0f);
	int NumberOfFillChildren = 0;
	for(ui_widget *Sibling = Row->First; Sibling; Sibling = Sibling->Next)
	{
		if(Sibling->Flags & UI_LAYOUT_FILL)
		{
			NumberOfFillChildren++;
		}
	}
	f32 ExtraSpacePerFillChild = SpaceLeft.X/(f32)NumberOfFillChildren;
	for(ui_widget *Sibling = Row->First; Sibling; Sibling = Sibling->Next)
	{
		if(Sibling->Flags & UI_LAYOUT_FILL)
		{
			Sibling->Size.X += ExtraSpacePerFillChild;
		}
	}
	if(NumberOfFillChildren == 0) SpaceLeft = v2f(0, 0);
	TotalSize.X += SpaceLeft.X;
	Row->Size = AddV2F(TotalSize, Paddingx2);
	return TotalSize;
}

v2f UI_LayoutColumn(ui_context *UI, ui_widget *Column, v2f MaxSize)
{
	v2f TotalSize = v2f(0.0, 0.0);
	// Offset by Padding, decrease MaxSize by Padding * 2
	v2f Paddingx2 = MulV2F(Column->Padding, v2f(2, 2));
	MaxSize = SubV2F(MaxSize, Paddingx2);
	int NumberOfChildren = 0;
	for(ui_widget *Sibling = Column->First; Sibling; Sibling = Sibling->Next)
	{
		v2f ChildSize = v2f(0.0, 0.0);
		if(Sibling->Flags & UI_LAYOUT_ROW)
		{
			ChildSize = UI_LayoutRow(UI, Sibling, MaxSize);
		}
		else if(Sibling->Flags & UI_LAYOUT_COLUMN)
		{
			ChildSize = UI_LayoutColumn(UI, Sibling, MaxSize);
		}
		else
		{
			ChildSize = Sibling->Size;
		}
		TotalSize.Y += ChildSize.Y;
		TotalSize.X = Max(TotalSize.X, ChildSize.X);
		NumberOfChildren++;
	}
	if(NumberOfChildren > 1) TotalSize.Y += Column->Gap * (f32)(NumberOfChildren - 1);
	Column->Size = AddV2F(TotalSize, Paddingx2);
	return TotalSize;
}

void UI_LayoutPositions(ui_context *UI, ui_widget *Widget, v2f Position)
{
	if(!Widget) return;
	Widget->Position = Position;
	Position = AddV2F(Position, Widget->Padding);
	for(ui_widget *Child = Widget->First; Child; Child = Child->Next)
	{
		UI_LayoutPositions(UI, Child, Position);
		if(Widget->Flags & UI_LAYOUT_ROW)         Position.X += Child->Size.X + Widget->Gap;
		else if(Widget->Flags & UI_LAYOUT_COLUMN) Position.Y += Child->Size.Y + Widget->Gap;
	}
}

void UI_RenderWidgets(ui_context *UI, ui_widget *Widget)
{
	// The UI works in a top-left origin (Y-down) space, but the renderer is
	// bottom-left origin (Y-up). Flip the widget's top-left position into a
	// render-space corner here; X is unchanged. Size is orientation-agnostic.
	v2f RenderPosition = v2f(Widget->Position.X,
		UI->Draw->ViewportSize.Y - Widget->Position.Y - Widget->Size.Y);

	if(Widget->Flags & UI_RENDER_BACKGROUND)
	{
		D_RoundedQuad(UI->Draw, RenderPosition, Widget->Size.X, Widget->Size.Y,
			15.0f,
			.Colour = v4f(0.2, 0.2, 0.2, 1.0),
			.OutlineColour = v4f(0.3, 0.3, 0.3, 1.0),
			.Extrusion = 0.0,
			.OutlineThickness = 2.0f
		);
	}

	if(!(Widget->Flags & UI_LAYOUT_COLUMN) && !(Widget->Flags & UI_LAYOUT_ROW))
	{
		D_RoundedQuad(UI->Draw, v2f(RenderPosition.X - Widget->HoverAnimation.Value*5, RenderPosition.Y - Widget->HoverAnimation.Value*5),
		Widget->Size.X + Widget->HoverAnimation.Value*10, Widget->Size.Y + Widget->HoverAnimation.Value*10, 15.0f,
		.Colour = v4f(1.0f, 0.5f, 0.0f, 1),
		.OutlineColour = v4f(1.0f, 0.5f, 0.0f, 0),
		.OutlineThickness = 0.0f,
		.Extrusion = 1.0 - Widget->ActiveAnimation.Value);
		D_Str8InBox(UI->Draw, Widget->String,
			RenderPosition, Widget->Size,
			D_ALIGN_CENTRE, D_ALIGN_CENTRE,
			.Font = UI->Font,
			.HeightInPixels = 50.0f,
			.Colour = v4f(0.45f, 0.05f, 0.0f, 1)
		);
	}
	for(ui_widget *Child = Widget->First; Child; Child = Child->Next)
	{
		UI_RenderWidgets(UI, Child);
	}
}