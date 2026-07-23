#include "render_common.h"
#include "misc.h"
#include "memory.h"

#include <assert.h>

void R_PushTransform(render_context *R, m44f Transform)
{
	assert(R->TransformCount < MAX_TRANSFORMS);
	R->TransformStack[R->TransformCount++] = Transform;
}

void R_PopTransform(render_context *R)
{
	assert(R->TransformCount >= 1);
	--R->TransformCount;
}

void R_PushScissor(render_context *R, v4i Scissor)
{
	assert(R->ScissorCount < MAX_SCISSORS);
	R->ScissorStack[R->ScissorCount++] = Scissor;
}

void R_PopScissor(render_context *R)
{
	assert(R->ScissorCount >= 1);
	--R->ScissorCount;
}

v4i R_CurrentScissor(render_context *R)
{
	assert(R->ScissorCount >= 1);
	return R->ScissorStack[R->ScissorCount - 1];
}

m44f R_CurrentTransform(render_context *R)
{
	assert(R->TransformCount >= 1);
	return R->TransformStack[R->TransformCount - 1];
}

void R_SetLight(render_context *R, v3f Position, v3f Colour, f32 Intensity, f32 Radius)
{
	R->LightPosition  = Position;
	R->LightColour    = Colour;
	R->LightRadius    = Radius;
	R->LightIntensity = Intensity;
}
