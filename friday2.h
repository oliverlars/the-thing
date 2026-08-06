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

#define Null ((void*)0)

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

typedef struct f_file_info
{
	f_b32 Exists;
	f_u64 LastModified;
	f_u64 SizeInBytes;
}
f_file_info;

typedef struct f_thread f_thread;

typedef struct f_dir_iter f_dir_iter;

typedef struct f_process_state
{
	f_b32 Exited;
	f_s32 ExitCode;
}
f_process_state;

f_memory_arena      F_ArenaReserve    (f_s64 Bytes);
f_memory_arena      F_ArenaFromMemory (void *Memory, f_s64 Size);
f_u8               *F_ArenaPush       (f_memory_arena *Arena, f_s64 Bytes);

f_str8              F_Str8PushF         (f_memory_arena *A, char *Format, ...);
void                F_Str8ListAppend    (f_memory_arena *A, f_str8_list *L, f_str8 S);
f_str8_array        F_Str8ArrayFromList (f_memory_arena *A, f_str8_list L);
f_str8              F_Str8Join          (f_memory_arena *A, f_str8_array Array, f_str8 Separator);
f_b32               F_Str8Equal         (f_str8 A, f_str8 B);
f_b32               F_Str8EndsWith      (f_str8 S, f_str8 Suffix);


#define F_Entry(WaveInfo) void F_Entry(f_wave *WaveInfo)
F_Entry(Data);
void F_Dispatch(f_wave_context *Ctx, f_s64 LaneCount);

f_s64           F_WaveLaneIndex   (f_wave *Wave);
f_s64           F_WaveLaneCount   (f_wave *Wave);
f_b32           F_WaveIsFirstLane (f_wave *Wave);
void            F_WaveBarrier     (f_wave *Wave);
f_b32           F_WaveAnyTrue     (f_wave *Wave, f_b32 Value);
f_b32           F_WaveAllTrue     (f_wave *Wave, f_b32 Value);
f_u64           F_WavePrefixSum   (f_wave *Wave, f_u64 Value);
void            F_WaveBroadcast   (f_wave *Wave, void *Bytes, f_s64 Count);
f_memory_arena *F_WaveArena       (f_wave *Wave);
f_memory_arena *F_WaveSharedArena (f_wave *Wave);


#define F_WaveOnce(W)
#define F_WaveFor(W, Index, Count)
#define F_WaveUniform(W, Var, Expr)


typedef void f_output_sink(void *User, f_str8 Text);

f_b32 F_ProcessSpawn(f_memory_arena *Arena, f_str8_array Argv, f_process *Out);
f_b32 F_ProcessPoll (f_process *Process, f_output_sink *Sink, void *User);
void  F_ProcessWait (f_process *Process, f_output_sink *Sink, void *User);

f_watch *F_WatchCreate (f_wave *Wave, f_str8_list Paths);
f_b32    F_WatchChanged(f_wave *Wave, f_watch *Watch);

f_build      *F_BuildCreate(f_wave *Wave, f_build_options Options);
f_b32         F_BuildStart (f_wave *Wave, f_build *Build);
f_build_state F_BuildTick  (f_wave *Wave, f_build *Build);

f_u8 *F_OS_Reserve   (f_s64 NumberOfBytes);
void  F_OS_Commit    (void *Address, f_s64 BytesToCommit);
f_s64 F_OS_PageSize  (void);

f_b32 F_OS_FileInfo       (f_str8 Path, f_file_info *Out);
f_s64 F_OS_FileRead      (f_str8 Path, f_u8 *Dest, f_s64 Max);
f_b32 F_OS_FileWriteStr8 (f_str8 Path, f_str8 Data);
f_b32 F_OS_FileDelete    (f_str8 Path);
f_b32 F_OS_FileRename    (f_str8 From, f_str8 To);
f_b32 F_OS_DirCreate     (f_str8 Path);
f_b32 F_OS_DirNext       (f_dir_iter *Iter, f_str8 *OutName);

f_b32 F_OS_ProcessSpawn  (char **Argv, f_process *Out);
f_b32 F_OS_ProcessState  (f_process *P, f_s32 *ExitCode);

void  F_OS_ThreadStart(f_thread *T, void *(*Fn)(void *), void *Arg);
void  F_OS_ThreadJoin (f_thread T);
void  F_OS_FutexWait  (void *Address, f_u64 Expected);
void  F_OS_FutexWakeAll(void *Address);


f_s64 F_OS_CPUCount(void);

#if defined(_WIN32) 
#define OS_WINDOWS
#endif
#if(__APPLE__)
#define OS_DARWIN

#include <sys/mman.h> // mmap, mprotect
#include <unistd.h> // getpagesize()

#endif

#endif
#define FRIDAY_IMPLEMENTATION
#ifdef FRIDAY_IMPLEMENTATION

f_u8 *F_OS_Reserve(f_s64 NumberOfBytes)
{
	f_u8 *Result;
#if defined(OS_WINDOWS)
	Result = (f_u8*)VirtualAlloc((void*)0, NumberOfBytes, MEM_RESERVE, PAGE_READWRITE);
#elif defined(OS_DARWIN)
	Result = (f_u8 *)mmap(Null, NumberOfBytes, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
	return Result;
}

void  F_OS_Commit(void *Address, f_s64 BytesToCommit)
{
#if defined(OS_WINDOWS)
	VirtualAlloc((void*)(Address), NumberOfBytes, MEM_COMMIT, PAGE_READWRITE);
#elif defined(OS_DARWIN)
	mprotect(Address, BytesToCommit, PROT_READ|PROT_WRITE);
#else
#error "Unsupported OS"
#endif
}

f_s64 F_OS_PageSize(void)
{
#if defined(OS_WINDOWS)
	SYSTEM_INFO SystemInfo = {0};
	GetSystemInfo(&SystemInfo);

	return SystemInfo.dwPageSize;
#elif defined(OS_DARWIN)
	return getpagesize();
#else
#error "Unsupported OS"
#endif
}

#endif