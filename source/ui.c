#include "misc.h"
#include "draw.h"
#include "os.h"

#include "ui.h"


b32 UI_Button(ui_context *UI, char *Name)
{
	b32 Clicked = False;
	v2f MousePosition = OS_MousePosition(UI->Window);
	if (MousePosition.X >= 500 && MousePosition.X < 750 &&
		MousePosition.Y >= 500 && MousePosition.Y <= 600 && OS_MouseDown(OS_MOUSE_LEFT)) {
		Clicked = True;
	}
	f32 Extrusion = 1.0;
	if (Clicked) Extrusion = 0.0;
	D_Capsule(UI->Draw, v2f(500, 500),
		250, 100,
		.Colour = v4f(1.0f, 0.5f, 0.0f, 1),
		.OutlineColour = v4f(1.0f, 0.5f, 0.0f, 1),
		.OutlineThickness = 4.0f,
		.Extrusion = Extrusion);
	D_Str8InBox(UI->Draw, Str8(Name),
		v2f(500, 500), v2f(250, 100),
		D_ALIGN_CENTRE, D_ALIGN_CENTRE,
		.Font = UI->Font,
		.HeightInPixels = 48.0f,
		.Colour = v4f(0.45f, 0.05f, 0.0f, 1));

	return Clicked;
}