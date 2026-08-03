#include "maths.h"
#include "memory.h"
#include "misc.h"
#include "draw.h"
#include "os.h"
#include "render_common.h"

#include "ui.h"

void UI_Begin(ui_context *UI)
{
	UI->Hover = UI->HoverNext;
	UI->HoverNext = 0;
	UI->HoverNextLayer = 0;
	UI->FloatCount = 0;
	UI->NextGap = UI_DEFAULT_GAP;
	UI->CurrentLayout = 0;
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

ui_widget *UI_GetWidget(ui_context *UI, str8 Name)
{
	u64 Key = M_FNVHashString(M_FNV_HASH_START, Name);
	ui_widget *Widget = UI->Widgets + (Key % UI_WIDGET_TABLE_SIZE);
	if(Widget->Key == 0)
	{
		MemoryZero(Widget, sizeof(ui_widget));
		Widget->Key = Key;
	}
	else if(Widget->Key != Key)
	{
		if(Widget->FrameLastTouched < UI->Frame)
		{
			MemoryZero(Widget, sizeof(ui_widget));
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
	// Apply the current flag scope (UI_FILL / UI_FLAG) every frame, not just at creation, so scope
	// flags behave intuitively for widgets that persist across frames. Widget functions OR in their
	// own flags (e.g. UI_Button's draw flags) after this, so nothing they set is lost.
	Widget->Flags = UI->Flags;
	Widget->FrameLastTouched = UI->Frame;
	Widget->First = 0;
	Widget->Last = 0;
	Widget->Next = 0;
	Widget->Parent = 0;
	UI->CurrentWidget = Widget;
	return Widget;
}

ui_widget *UI_GetWidgetByKey(ui_context *UI, u64 Key)
{
	ui_widget *Widget = UI->Widgets + (Key % UI_WIDGET_TABLE_SIZE);
	if(Widget->Key == 0)
	{
		MemoryZero(Widget, sizeof(ui_widget));
		Widget->Key = Key;
	}
	else if(Widget->Key != Key)
	{
		if(Widget->FrameLastTouched < UI->Frame)
		{
			MemoryZero(Widget, sizeof(ui_widget));
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
	return Widget;
}

 b32 UI_PointInRect(v2f P, v2f Position, v2f Size)
{
	return (P.X >= Position.X && P.X < (Position.X + Size.X) &&
	        P.Y >= Position.Y && P.Y <= (Position.Y + Size.Y));
}

ui_signal UI_Interact(ui_context *UI, ui_widget *Widget)
{
	ui_signal Signal = {0};

	v2f MousePosition = Platform.MousePosition(UI->Window);

	MousePosition.Y = UI->Draw->ViewportSize.Y - MousePosition.Y;

	if(UI->Hover == Widget->Key)
	{
		Signal.Hovered = True;
		Widget->HoverAnimation.Target = 1.0;
		if(Platform.MouseDown(OS_MOUSE_LEFT))
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
		Signal.Pressed = True;
		if(Platform.MouseReleased(OS_MOUSE_LEFT))
		{
			Signal.Clicked = True;
			UI->Active = 0;
			Widget->ActiveAnimation.Target = 0.0;
		}
	}

	if(UI_PointInRect(MousePosition, Widget->Position, Widget->Size) &&
	   Widget->Layer >= UI->HoverNextLayer)
	{
		UI->HoverNext = Widget->Key;
		UI->HoverNextLayer = Widget->Layer;
	}

	return Signal;
}

v2f UI_RenderPos(ui_context *UI, ui_widget *Widget);

void UI_Texture(ui_context *UI, str8 Name, r_texture Texture)
{
	ui_widget *Widget = UI_GetWidget(UI, Name);
	Widget->Kind = UI_WIDGET_BUTTON;
	Widget->String = Name;
	Widget->DisplayString = Name;

	ui_widget *Layout = UI->CurrentLayout;
	UI_AppendAsChild(Layout, Widget);

	v2f RenderPosition = UI_RenderPos(UI, Widget);
	R_UniformM44F(UI->Draw->Render, Str8("PVMMatrix"), R_CurrentTransform(UI->Draw->Render));
	R_UniformV2F(UI->Draw->Render, Str8("Position"), RenderPosition);
	R_UniformV2F(UI->Draw->Render, Str8("Size"), Widget->Size);
	R_BindTexture(UI->Draw->Render, Str8("Texture"), Texture);
	R_Draw(UI->Draw->Render, Str8("textured_quad.metal"), 6, R_TRIANGLES);

	Widget->Aspect = (f32)Texture.Size.Y / (f32)Texture.Size.X;
	Widget->Size   = v2f(Texture.Size.X, Texture.Size.Y);
}

void UI_CollapsingTexture(ui_context *UI, str8 Name, str8 TextureName, r_texture Texture)
{
	b32 Clicked = False;
	ui_widget *Header;

	UI_ROW(UI, 0) UI_FILL(UI)
	{
		Header = UI_GetWidget(UI, Name);

		Header->Kind = UI_WIDGET_BUTTON;
		Header->String = Name;
		Header->DisplayString = Name;

		Header->Flags |= UI_DRAW_BACKGROUND | UI_DRAW_TEXT | UI_DRAW_HOT;
		Header->Colour = v4f(1.0f, 0.44f, 0.14f, 1.0f);
		Header->TextColour = v4f(0.9f, 0.87f, 0.83f, 1.0f);
		Header->CornerRadius = 30.0f;

		ui_widget *Layout = UI->CurrentLayout;
		UI_AppendAsChild(Layout, Header);

		ui_signal Signal = UI_Interact(UI, Header);

		Header->Size.X = D_Str8WidthFromHeight(UI->Font, Name, 50) + 30;
		Header->Size.Y = D_Str8HeightFromHeight(UI->Font, 50) + 30;
		if(Signal.Clicked)
		{
			Header->Toggled = !Header->Toggled;
		}
	}

	if(Header->Toggled)
	{
		UI_BACKDROP(UI, v4f(0.08f, 0.09f, 0.12f, 1.0f))
		{
			UI_Texture(UI, TextureName, Texture);
			Header->Size.Y += UI->CurrentWidget->Size.Y;
		}
	}
}

b32 UI_Button(ui_context *UI, str8 Name)
{
	ui_widget *Widget = UI_GetWidget(UI, Name);
	Widget->Kind = UI_WIDGET_BUTTON;
	Widget->String = Name;
	Widget->DisplayString = Name;

	Widget->Flags |= UI_DRAW_BACKGROUND | UI_DRAW_TEXT | UI_DRAW_HOT;
	Widget->Colour = v4f(1.0f, 0.44f, 0.14f, 1.0f);
	Widget->TextColour = v4f(0.9f, 0.87f, 0.83f, 1.0f);
	Widget->CornerRadius = 30.0f;

	ui_widget *Layout = UI->CurrentLayout;
	UI_AppendAsChild(Layout, Widget);

	ui_signal Signal = UI_Interact(UI, Widget);

	UI_DrawWidget(UI, Widget);

	Widget->Size.X = D_Str8WidthFromHeight(UI->Font, Name, 50) + 30;
	Widget->Size.Y = D_Str8HeightFromHeight(UI->Font, 50) + 30;

	return Signal.Clicked;
}

 v2f UI_RenderPos(ui_context *UI, ui_widget *Widget)
{
	return v2f(Widget->Position.X,
	           UI->Draw->ViewportSize.Y - Widget->Position.Y - Widget->Size.Y);
}

 v2f UI_ContentInset(f32 CornerRadius, v2f Padding)
{
	return v2f(Padding.X + CornerRadius, Padding.Y);
}

 void UI_ContentRect(v2f BoxPos, v2f BoxSize, f32 CornerRadius, v2f Padding,
                           v2f *OutPos, v2f *OutSize)
{
	v2f Inset = UI_ContentInset(CornerRadius, Padding);
	*OutPos  = AddV2F(BoxPos, Inset);
	*OutSize = v2f(Max(BoxSize.X - Inset.X * 2.0f, 0.0f),
	               Max(BoxSize.Y - Inset.Y * 2.0f, 0.0f));
}

 v2f UI_HugBox(v2f ContentSize, f32 CornerRadius, v2f Padding)
{
	v2f Inset = UI_ContentInset(CornerRadius, Padding);
	return v2f(ContentSize.X + Inset.X * 2.0f, ContentSize.Y + Inset.Y * 2.0f);
}

 void UI_ComboHeaderDraw(ui_context *UI, ui_widget *Widget)
{
	v2f P = UI_RenderPos(UI, Widget);
	f32 Hot = Widget->HoverAnimation.Value;
	D_RoundedQuad(UI->Draw, P, Widget->Size.X, Widget->Size.Y, Widget->CornerRadius,
		.Colour = LerpV4F(v4f(0.22f, 0.22f, 0.26f, 1.0f), v4f(0.30f, 0.30f, 0.36f, 1.0f), Hot),
		.OutlineColour = v4f(0.45f, 0.45f, 0.50f, 1.0f),
		.OutlineThickness = 2.0f,
		.Extrusion = 0.2);

	v2f CPos, CSize;
	UI_ContentRect(P, Widget->Size, Widget->CornerRadius, Widget->Padding, &CPos, &CSize);
	D_Str8InBox(UI->Draw, Widget->DisplayString, CPos, CSize, D_ALIGN_MIN, D_ALIGN_CENTRE,
		.Font = UI->Font, .HeightInPixels = 50.0f, .Colour = v4f(1, 1, 1, 1));
	D_Str8InBox(UI->Draw, Widget->Icon, CPos, CSize, D_ALIGN_MAX, D_ALIGN_CENTRE,
		.Font = UI->IconFont, .HeightInPixels = 50.0f, .Colour = v4f(1, 1, 1, 1));
}

 void UI_ComboItemDraw(ui_context *UI, ui_widget *Widget)
{
	v2f P = UI_RenderPos(UI, Widget);
	f32 Hot = Widget->HoverAnimation.Value;
	v4f Base = Widget->Selected ? v4f(0.32f, 0.26f, 0.10f, 1.0f)
	                            : v4f(0.18f, 0.18f, 0.22f, 1.0f);
	D_RoundedQuad(UI->Draw, P, Widget->Size.X, Widget->Size.Y, Widget->CornerRadius,
		.Colour = LerpV4F(Base, v4f(0.40f, 0.40f, 0.46f, 1.0f), Hot),
		.OutlineColour = v4f(0.35f, 0.35f, 0.40f, 1.0f),
		.OutlineThickness = 1.0f);
	D_Str8InBox(UI->Draw, Widget->DisplayString, P, Widget->Size,
		D_ALIGN_CENTRE, D_ALIGN_CENTRE,
		.Font = UI->Font, .HeightInPixels = 50.0f, .Colour = v4f(1, 1, 1, 1));
}

int UI_ComboBox(ui_context *UI, str8 Name, str8 *Elements, int ElementCount, int ActiveElement)
{
	int ClickedElement = ActiveElement;

	ui_widget *Header = UI_GetWidget(UI, Name);
	Header->Kind = UI_WIDGET_COMBO_BOX;
	Header->String = Name;
	Header->DisplayString = Elements[ActiveElement];
	Header->CustomDraw = UI_ComboHeaderDraw;
	Header->Icon = Str8("\uE211");
	Header->Padding = v2f(10.0f, 12.0f);

	ui_widget *Layout = UI->CurrentLayout;
	UI_AppendAsChild(Layout, Header);

	f32 LabelWidth = 0;
	for(int E = 0; E < ElementCount; E++)
	{
		LabelWidth = Max(LabelWidth, D_Str8WidthFromHeight(UI->Font, Elements[E], 50));
	}
	f32 LineHeight = D_Str8HeightFromHeight(UI->Font, 50);
	f32 IconWidth  = D_Str8WidthFromHeight(UI->IconFont, Header->Icon, 50);
	f32 IconGap    = 24.0f;
	v2f Content    = v2f(LabelWidth + IconGap + IconWidth, LineHeight);

	f32 BoxHeight        = Content.Y + Header->Padding.Y * 2.0f;
	Header->CornerRadius = BoxHeight * 0.5f;
	Header->Size         = UI_HugBox(Content, Header->CornerRadius, Header->Padding);

	v2f  ItemPadding      = v2f(20.0f, 12.0f);
	f32  ItemCornerRadius = 15.0f;
	f32  ItemHeight       = LineHeight + ItemPadding.Y * 2.0f;
	f32  ItemWidth        = Header->Size.X;

	ui_signal HeaderSignal = UI_Interact(UI, Header);
	if(HeaderSignal.Clicked) Header->Toggled = !Header->Toggled;

	UI_DrawWidget(UI, Header);

	if(Header->Toggled)
	{
		ui_widget *List = UI_GetWidget(UI, Str8PushF(TemporaryArena(), "?S_list", Name));
		List->Flags = UI_LAYOUT_COLUMN | UI_LAYOUT_FLOATING;
		List->Gap = UI_DEFAULT_GAP;
		List->Padding = v2f(0, 0);
		List->Position = v2f(Header->Position.X,
		                     Header->Position.Y + Header->Size.Y + UI_DEFAULT_GAP);
		UI->FloatRoots[UI->FloatCount++] = List;

		for(int E = 0; E < ElementCount; E++)
		{
			ui_widget *Item = UI_GetWidget(UI, Str8PushF(TemporaryArena(), "?S_item?s32", Name, E));
			Item->Kind = UI_WIDGET_COMBO_BOX;
			Item->String = Elements[E];
			Item->DisplayString = Elements[E];
			Item->Selected = (E == ActiveElement);
			Item->Layer = 1;
			Item->Padding = ItemPadding;
			Item->CornerRadius = ItemCornerRadius;
			Item->Size.X = ItemWidth;
			Item->Size.Y = ItemHeight;
			Item->CustomDraw = UI_ComboItemDraw;
			UI_AppendAsChild(List, Item);

			ui_signal ItemSignal = UI_Interact(UI, Item);
			if(ItemSignal.Clicked)
			{
				ClickedElement = E;
				Header->Toggled = False;
			}
		}
	}

	return ClickedElement;
}

void UI_LayoutPop(ui_context *UI)
{
	assert(UI->CurrentLayout);
	UI->CurrentLayout = UI->CurrentLayout->Parent;
}

void UI_LayoutPush(ui_context *UI, ui_widget Layout, u64 Seed)
{
	u64 ParentKey = UI->CurrentLayout ? UI->CurrentLayout->Key : 0;
	u64 Key = M_FNVHashBytes(M_FNV_HASH_START, (u8*)&Seed, sizeof(Seed));
	Key = M_FNVHashBytes(Key, (u8*)&ParentKey, sizeof(ParentKey));
	if(Key == 0) Key = 1;

	ui_widget *Pushed = UI_GetWidgetByKey(UI, Key);

	v2f          KeepPos    = Pushed->Position;
	v2f          KeepSize   = Pushed->Size;
	ui_animation KeepActive = Pushed->ActiveAnimation;
	ui_animation KeepHover  = Pushed->HoverAnimation;

	*Pushed = Layout;
	Pushed->Key              = Key;
	Pushed->FrameLastTouched = UI->Frame;
	Pushed->ActiveAnimation  = KeepActive;
	Pushed->HoverAnimation   = KeepHover;
	if(!(Layout.Flags & UI_LAYOUT_FIXED))
	{
		Pushed->Position = KeepPos;
		Pushed->Size     = KeepSize;
	}

	if(UI->CurrentLayout)
	{
		UI_AppendAsChild(UI->CurrentLayout, Pushed);
	}
	else
	{
		UI->Root = Pushed;
	}
	UI->CurrentLayout = Pushed;

	if(Pushed->Flags & UI_DRAW_BACKGROUND)
	{
		UI_DrawWidget(UI, Pushed);
	}
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
	v2f ChildSize = UI->Root->Size;
	if(UI->Root->Flags & UI_LAYOUT_ROW)    UI_LayoutRow(UI, UI->Root, ChildSize);
	if(UI->Root->Flags & UI_LAYOUT_COLUMN) UI_LayoutColumn(UI, UI->Root, ChildSize);
}

v2f UI_LayoutRow(ui_context *UI, ui_widget *Row, v2f MaxSize)
{
	v2f TotalSize = v2f(0.0, 0.0);

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
		else if(Sibling->Flags & UI_LAYOUT_CENTRE)
		{
			ChildSize = UI_LayoutCentre(UI, Sibling, MaxSize);
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
		else if(Sibling->Flags & UI_LAYOUT_CENTRE)
		{
			ChildSize = UI_LayoutCentre(UI, Sibling, MaxSize);
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

	v2f SpaceLeft = SubV2F(MaxSize, TotalSize);
	Assert(SpaceLeft.Y >= 0.0f);
	int NumberOfFillChildren = 0;
	for(ui_widget *Sibling = Column->First; Sibling; Sibling = Sibling->Next)
	{
		if(Sibling->Flags & UI_LAYOUT_FILL) NumberOfFillChildren++;
	}
	if(NumberOfFillChildren > 0)
	{
		f32 ExtraSpacePerFillChild = SpaceLeft.Y / (f32)NumberOfFillChildren;
		for(ui_widget *Sibling = Column->First; Sibling; Sibling = Sibling->Next)
		{
			if(Sibling->Flags & UI_LAYOUT_FILL)
			{
				Sibling->Size.Y += ExtraSpacePerFillChild;
				Sibling->Size.X  = MaxSize.X;
			}
		}
		TotalSize.Y += SpaceLeft.Y;
		TotalSize.X  = Max(TotalSize.X, MaxSize.X);
	}
	Column->Size = AddV2F(TotalSize, Paddingx2);
	return TotalSize;
}

v2f UI_LayoutCentre(ui_context *UI, ui_widget *Centre, v2f MaxSize)
{
	v2f Content = v2f(0.0, 0.0);
	for(ui_widget *Child = Centre->First; Child; Child = Child->Next)
	{
		v2f ChildSize = v2f(0.0, 0.0);
		if(Child->Flags & UI_LAYOUT_ROW)         ChildSize = UI_LayoutRow(UI, Child, MaxSize);
		else if(Child->Flags & UI_LAYOUT_COLUMN) ChildSize = UI_LayoutColumn(UI, Child, MaxSize);
		else if(Child->Flags & UI_LAYOUT_CENTRE) ChildSize = UI_LayoutCentre(UI, Child, MaxSize);
		else                                     ChildSize = Child->Size;
		Content.X = Max(Content.X, ChildSize.X);
		Content.Y = Max(Content.Y, ChildSize.Y);
	}
	Centre->Size = Content;
	return Content;
}

void UI_LayoutPositions(ui_context *UI, ui_widget *Widget, v2f Position)
{
	if(!Widget) return;
	Widget->Position = Position;
	v2f Inner    = AddV2F(Position, Widget->Padding);
	v2f InnerBox = SubV2F(Widget->Size, MulV2F(Widget->Padding, v2f(2, 2)));
	for(ui_widget *Child = Widget->First; Child; Child = Child->Next)
	{
		if(Widget->Flags & UI_LAYOUT_CENTRE)
		{
			if(Child->Aspect > 0.0f)
			{
				f32 H = InnerBox.Y;
				f32 W = H * Child->Aspect;
				if(W > InnerBox.X) { W = InnerBox.X; H = W / Child->Aspect; }
				Child->Size = v2f(W, H);
			}
			v2f ChildPos = v2f(Inner.X + (InnerBox.X - Child->Size.X) * 0.5f,
			                   Inner.Y + (InnerBox.Y - Child->Size.Y) * 0.5f);
			UI_LayoutPositions(UI, Child, ChildPos);
		}
		else
		{
			UI_LayoutPositions(UI, Child, Inner);
			if(Widget->Flags & UI_LAYOUT_ROW)         Inner.X += Child->Size.X + Widget->Gap;
			else if(Widget->Flags & UI_LAYOUT_COLUMN) Inner.Y += Child->Size.Y + Widget->Gap;
		}
	}
}

void UI_DrawWidget(ui_context *UI, ui_widget *Widget)
{
	v2f RenderPosition = v2f(Widget->Position.X,
		UI->Draw->ViewportSize.Y - Widget->Position.Y - Widget->Size.Y);

	f32 Grow = (Widget->Flags & UI_DRAW_HOT) ? Widget->HoverAnimation.Value * 5.0f : 0.0f;
	v2f DrawPos  = v2f(RenderPosition.X - Grow, RenderPosition.Y - Grow);
	v2f DrawSize = v2f(Widget->Size.X + Grow * 2.0f, Widget->Size.Y + Grow * 2.0f);

	if(Widget->Flags & UI_DRAW_BACKGROUND)
	{
		f32 Extrusion = 0.5;
		if(Widget->Flags & UI_DRAW_EXTRUDED)
		{
			Extrusion =  (Widget->Flags & UI_DRAW_HOT) ? (1.0f - Widget->ActiveAnimation.Value) : 0.0f;
		}
		D_RoundedQuad(UI->Draw, DrawPos, DrawSize.X, DrawSize.Y, Widget->CornerRadius,
			.Colour = Widget->Colour,
			.OutlineColour = Widget->OutlineColour,
			.OutlineThickness = Widget->OutlineThickness,
			.Extrusion = Extrusion);
	}

	if(Widget->Flags & UI_DRAW_TEXT)
	{
		D_Str8InBox(UI->Draw, Widget->DisplayString, DrawPos, DrawSize,
			D_ALIGN_CENTRE, D_ALIGN_CENTRE,
			.Font = UI->Font, .HeightInPixels = 50.0f,
			.Colour = Widget->TextColour);
	}

	if(Widget->CustomDraw) Widget->CustomDraw(UI, Widget);
}

void UI_RenderWidgets(ui_context *UI, ui_widget *Widget)
{
	UI_DrawWidget(UI, Widget);
	for(ui_widget *Child = Widget->First; Child; Child = Child->Next)
	{
		UI_RenderWidgets(UI, Child);
	}
}