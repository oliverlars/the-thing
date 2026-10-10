
#include "misc.h"
#include "memory.h"
#include "maths.h"
#include "os.h"
#include "ui.h"
#include "embed.h"


typedef struct app_memory
{
	m_arena *PermanentArena;
	m_arena *FrameArenas[2];
	per_thread_context *ThreadContext;
	os_window   Window;
	embed_data *Berkeley;
	embed_data *Lucide;
	b32 GPULogging;

	void *AppState;
	u64  StateSize;
	u32  StateVersion;
	b32  Reloaded;
} app_memory;