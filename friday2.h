#ifndef FRIDAY_H
#define FRIDAY_H

#define FRIDAY_VERSION "V"

typedef signed char f_s8;
typedef signed short int f_s16;
typedef signed int f_s32;
typedef signed long long f_s64;
typedef unsigned char f_u8;
typedef unsigned short int f_u16;
typedef unsigned int f_u32;
typedef unsigned long long f_u64;
typedef float f_f32;
typedef double f_f64;
typedef unsigned char f_b8;
typedef unsigned short int f_b16;
typedef unsigned int f_b32;
typedef unsigned long long f_b64;

#define F_KiloBytes(X) (1024ull * X)
#define F_MegaBytes(X) (1024ull * F_KiloBytes(X))
#define F_GigaBytes(X) (1024ull * F_MegaBytes(X))

#define F_Expand(String) (int)String.Size, (char*)String.Bytes
#define F_Str8(String) (f_str8) { strlen(String), (f_s8*)String }

#define F_RED     "\x1B[31m"
#define F_GREEN   "\x1B[32m"
#define F_YELLOW  "\x1B[33m"
#define F_BLUE    "\x1B[34m"
#define F_MAGENTA "\x1B[35m"
#define F_CYAN    "\x1B[36m"
#define F_WHITE   "\x1B[37m"
#define F_RESET   "\x1B[0m"

typedef enum f_operating_system
{
	F_OS_WINDOWS,
	F_OS_DARWIN,
	F_OS_LINUX,
}
f_operating_system;

typedef struct f_memory_arena
{
	f_u8 *Start;
	f_s64 Committed;
	f_s64 At;
}
f_memory_arena;

typedef struct f_str8
{
	f_s64 Size;
	f_s8 *Bytes;
}
f_str8;

typedef struct f_str8_list_node
{
	struct f_str8_list_node *Next;
	struct f_str8_list_node *Previous;
	f_str8 String;
}
f_str8_list_node;

typedef struct f_str8_list
{
	f_str8_list_node *First;
	f_str8_list_node *Last;
}
f_str8_list;

typedef struct f_str8_array
{
	f_str8 *Items;
	f_s64   Count;
}
f_str8_array;

typedef struct f_wave_context f_wave_context;
typedef struct f_wave f_wave;

typedef struct f_process f_process;

typedef struct f_watch f_watch;

typedef struct f_build f_build;

typedef struct f_build_options f_build_options;

typedef enum f_build_state f_build_state;

f_memory_arena      F_ArenaReserve(f_s64 Bytes);
f_memory_arena      F_ArenaFromMemory(void *Memory, f_s64 Size);
f_u8               *F_ArenaPush(f_memory_arena *Arena, f_s64 Bytes);

f_str8              F_Str8PushF(f_memory_arena *A, char *Format, ...);
void                F_Str8ListAppend(f_memory_arena *A, f_str8_list *L, f_str8 S);
f_str8_array        F_Str8ArrayFromList(f_memory_arena *A, f_str8_list L);
f_str8              F_Str8Join(f_memory_arena *A, f_str8_array Array, f_str8 Separator);
f_b32               F_Str8Equal(f_str8 A, f_str8 B);
f_b32               F_Str8EndsWith(f_str8 S, f_str8 Suffix);


//F_Entry(Data);
void F_Dispatch(f_wave_context *Ctx, f_s64 LaneCount);

f_s64           F_WaveLaneIndex(f_wave *Wave);
f_s64           F_WaveLaneCount(f_wave *Wave);
f_b32           F_WaveIsFirstLane(f_wave *Wave);
void            F_WaveBarrier(f_wave *Wave);
f_b32           F_WaveAnyTrue(f_wave *Wave, f_b32 Value);
f_b32           F_WaveAllTrue(f_wave *Wave, f_b32 Value);
f_u64           F_WavePrefixSum(f_wave *Wave, f_u64 Value);
void            F_WaveBroadcast(f_wave *Wave, void *Bytes, f_s64 Count);
f_memory_arena *F_WaveArena(f_wave *Wave);
f_memory_arena *F_WaveSharedArena(f_wave *Wave);


#define F_WaveOnce(W)
#define F_WaveFor(W, Index, Count)
#define F_WaveUniform(W, Var, Expr)


typedef void f_output_sink(void *User, f_str8 Text);
f_b32 F_ProcessSpawn(f_memory_arena *Arena, f_str8_array Argv, f_process *Out);
f_b32 F_ProcessPoll(f_process *Process, f_output_sink *Sink, void *User);
void  F_ProcessWait(f_process *Process, f_output_sink *Sink, void *User);

f_watch *F_WatchCreate(f_wave *Wave, f_str8_list Paths);
f_b32    F_WatchChanged(f_wave *Wave, f_watch *Watch);

f_build      *F_BuildCreate(f_wave *Wave, f_build_options Options);
f_b32         F_BuildStart(f_wave *Wave, f_build *Build);
f_build_state F_BuildTick(f_wave *Wave, f_build *Buil);

#endif
#ifdef FRIDAY_IMPLEMENTATION
#endif