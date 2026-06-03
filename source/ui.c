#include "maths.h"
#include "misc.h"
#include "draw.h"
#include "os.h"

#include "ui.h"


ui_widget *GetWidget(ui_context *UI, str8 Name)
{
	u64 Key = M_FNVHashString(M_FNV_HASH_START, Name);
	ui_widget *Widget = UI->Widgets + (Key % 4096);
	if(Widget->Key == 0 && Widget->Key == Key)
	{
		MemorySet(Widget, 0, sizeof(ui_widget));
	}
	else if(Widget->Key != 0 && Widget->Key != Key && Widget->FramesSinceLastTouched > 2)
	{
		MemorySet(Widget, 0, sizeof(ui_widget));
		Widget->Key = Key;
	}
	else if(Widget->Key != 0 && Widget->Key != Key && Widget->FramesSinceLastTouched < 2)
	{
		for(int I = 1; I < 4096; I++)
		{
			Widget = UI->Widgets + ((Key % 4096) + I) % 4096;
			if(Widget->Key == Key) break;
		}
	}

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
		if(OS_MouseDown(OS_MOUSE_LEFT)) Clicked = True;
	}
	else
	{
		Widget->HoverAnimation = LerpF32(Widget->HoverAnimation, 0.0, 0.1);
	}
	f32 Extrusion = 1.0;
	if (Clicked) Extrusion = 0.0;
	D_Capsule(UI->Draw, v2f(500 - Widget->HoverAnimation*10, 500 - Widget->HoverAnimation*10),
		250 + Widget->HoverAnimation*20, 100 + Widget->HoverAnimation*20,
		.Colour = v4f(1.0f, 0.5f, 0.0f, 1),
		.OutlineColour = v4f(1.0f, 0.5f, 0.0f, 1),
		.OutlineThickness = 4.0f,
		.Extrusion = Extrusion);
	D_Str8InBox(UI->Draw, Name,
		v2f(500, 500), v2f(250, 100),
		D_ALIGN_CENTRE, D_ALIGN_CENTRE,
		.Font = UI->Font,
		.HeightInPixels = 48.0f,
		.Colour = v4f(0.45f, 0.05f, 0.0f, 1));
	
	return Clicked;
}