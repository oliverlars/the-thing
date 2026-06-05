#include "maths.h"
#include "misc.h"
#include "draw.h"
#include "os.h"

#include "ui.h"


ui_widget *GetWidget(ui_context *UI, str8 Name)
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
	Widget->FrameLastTouched = UI->Frame;
	return Widget;
}


b32 UI_Button(ui_context *UI, str8 Name)
{
	ui_widget *Widget = GetWidget(UI, Name);
	b32 Clicked = False;
	v2f MousePosition = OS_MousePosition(UI->Window);
	if (MousePosition.X >= 500 && MousePosition.X < 750 &&
		MousePosition.Y >= 500 && MousePosition.Y <= 600)
	{
		Widget->HoverAnimation = LerpF32(Widget->HoverAnimation, 1.0, 0.1);
		if(OS_MouseDown(OS_MOUSE_LEFT))
		{
			Clicked = True;
			Widget->ActiveAnimation = 1.0;
		}
	}
	else
	{
		Widget->HoverAnimation = LerpF32(Widget->HoverAnimation, 0.0, 0.1);
	}
	D_Capsule(UI->Draw, v2f(500 - Widget->HoverAnimation*5, 500 - Widget->HoverAnimation*6),
		250 + Widget->HoverAnimation*10, 100 + Widget->HoverAnimation*10,
		.Colour = v4f(1.0f, 0.5f, 0.0f, 1),
		.OutlineColour = v4f(1.0f, 0.5f, 0.0f, 0),
		.OutlineThickness = 0.0f,
		.Extrusion = 1.0 - Widget->ActiveAnimation);
	f32 Padding = 10.0f;
	D_Str8InBox(UI->Draw, Name,
		v2f(500 + Padding, 500 - Padding), v2f(250 - Padding*2, 100 + Padding*2),
		D_ALIGN_CENTRE, D_ALIGN_CENTRE,
		.Font = UI->Font,
		.HeightInPixels = 50.0f,
		.Colour = v4f(0.45f, 0.05f, 0.0f, 1));
	Widget->ActiveAnimation = LerpF32(Widget->ActiveAnimation, 0.0, 0.1);
	return Clicked;
}