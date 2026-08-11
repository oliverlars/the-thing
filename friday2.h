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

#define F_Null ((void*)0)

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

typedef struct f_barrier
{
	_Atomic(f_u64) Generation;
	_Atomic(f_u64) LanesWaiting;
	f_s64          LaneCount;
}
f_barrier;

typedef struct f_wave
{
	f_s64      Lane;
	f_s64      LaneCount;
	f_barrier *Barrier;
	void      **BroadcastPointers;
	_Atomic(f_u64) *Vote;
	// TODO: Make this work for > 64 lanes
	_Atomic(f_u64) *Ballot;

	// Only defined for the first lane
	f_memory_arena *GlobalArena;
	// Each lane has one of these.
	// Maybe a better idea for this
	f_memory_arena *Arena;
}
f_wave;

typedef enum f_process_state
{
	F_PROCESS_NONE,
	F_PROCESS_FAILED_TO_LAUNCH,
	F_PROCESS_RUNNING,
	F_PROCESS_EXITED,
}
f_process_state;

typedef struct f_process
{
	f_s32 ID;
	f_process_state State; 
	f_u64 ExitCode;
	f_str8_list StdOutSinceLastPoll;
}
f_process;

typedef struct f_watch
{
	f_str8 *Files;
	f_u64  *TimeStamps;
	f_s32   FileCount;
}
f_watch;

typedef struct f_build f_build;

typedef struct f_toolchain
{
	f_str8 CompilerPath;

	f_str8 IncludeFlag;
	f_str8 ObjectFlag;
	f_str8 DebugFlag;
	f_str8 OptimiseFlag;
	f_str8 SharedFlag;
}
f_toolchain;

f_toolchain F_TOOLCHAIN_CLANG;
f_toolchain F_TOOLCHAIN_MSVC;
f_toolchain F_TOOLCHAIN_GCC;
f_toolchain F_TOOLCHAIN_CARNAGE;

typedef struct f_build_options
{
}
f_build_options;

typedef enum f_build_state f_build_state;

typedef struct f_file_info
{
	f_b32 Exists;
	f_u64 LastModified;
	f_u64 SizeInBytes;
}
f_file_info;

typedef struct f_thread
{
	f_u64 Handle;
}
f_thread;

typedef struct f_thread_entry_params
{
	void (*LaneEntry)(f_wave *WaveArgument);
	f_wave *WaveArgument;
}
f_thread_entry_params;

typedef struct f_dir_iter f_dir_iter;

f_memory_arena     *F_ArenaReserve   (f_s64 Bytes);

#define F_ArenaPushType(Arena, Type) (Type*)F_Raw_ArenaPush(Arena, sizeof(Type))
#define F_ArenaPushArrayType(Arena, Count, Type) (Type*)F_Raw_ArenaPush(Arena, Count * (sizeof(Type)))
#define F_ArenaPush(Arena, BytesToPush) F_Raw_ArenaPush(Arena, BytesToPush)

f_u8 *F_Raw_ArenaPush(f_memory_arena *Arena, f_s64 BytesToPush);
void  F_ArenaReset(f_memory_arena *Arena);

void  F_ArenaResetTo(f_memory_arena *Arena, f_s64 Mark);

f_str8       F_Str8PushF         (f_memory_arena *A, char *Format, ...);
void         F_Str8ListAppend    (f_memory_arena *A, f_str8_list *L, f_str8 S);
f_str8_array F_Str8ArrayFromList (f_memory_arena *A, f_str8_list L);
f_str8       F_Str8JoinList      (f_memory_arena *Arena, f_str8_list List,   f_str8 Separator);
f_str8       F_Str8JoinArray     (f_memory_arena *Arena, f_str8_array Array, f_str8 Separator);
f_b32        F_Str8Equal         (f_str8 A, f_str8 B);
f_b32        F_Str8EndsWith      (f_str8 S, f_str8 Suffix);
f_s64        F_Str8ListCount     (f_str8_list List);
f_s64        F_Str8ListLength    (f_str8_list List);


void F_DispatchAcrossNLanes(f_s64 LaneCount);

f_b32           F_WaveIsFirstLane        (f_wave *Wave);
f_u64           F_WaveInclusivePrefixSum (f_wave *Wave, f_u64 Value);
f_u64           F_WaveExclusivePrefixSum (f_wave *Wave, f_u64 Value);

#define         F_WaveAnyTrue(Wave, Value) F_Internal_WaveAnyTrue(Wave, Value != 0)
#define         F_WaveAllTrue(Wave, Value) F_Internal_WaveAllTrue(Wave, Value != 0)
f_b32           F_Internal_WaveAnyTrue   (f_wave *Wave, f_b32 Condition);
f_b32           F_Internal_WaveAllTrue   (f_wave *Wave, f_b32 Condition);
#define         F_WaveCountTrue(Wave, Value) F_Internal_WaveCountTrue(Wave, Value != 0)
f_u64           F_Internal_WaveCountTrue (f_wave *Wave, f_b32 Condition);

#define F_BroadcastLane(Wave, Variable, SourceLane) \
      do {\
              f_wave *F_W_ = (Wave);\
              if(F_W_->Lane == (SourceLane)) F_W_->BroadcastPointers[0] = &(Variable);\
              F_WaveBarrier(F_W_);\
              F_MemoryCopy(&(Variable), F_W_->BroadcastPointers[0], sizeof(Variable));\
              F_WaveBarrier(F_W_);\
      } while(0)
 
#define F_BroadcastFirstLane(Wave, Variable) F_BroadcastLane(Wave, Variable, 0)

void            F_WaveBarrier (f_wave *Wave);

#define __F_Concat_(A, B) A##B
#define __F_Concat(A, B) __F_Concat_(A, B)

#define F_WaveFor(Wave, Index, Count) \
    for(f_s64 Index                          = (Wave)->Lane, \
              __F_Concat(F_ForEnd__,  __LINE__) = (f_s64)(Count), \
              __F_Concat(F_ForStep__, __LINE__) = (Wave)->LaneCount; \
        Index < __F_Concat(F_ForEnd__, __LINE__); \
        Index += __F_Concat(F_ForStep__, __LINE__))

void  F_ProcessSpawn (f_memory_arena *Arena, f_process *Process, f_str8_list Args);
f_b32 F_ProcessPoll  (f_process *Process);
void  F_ProcessWait  (f_process *Process);

f_watch      F_WatchFilesFromArray (f_wave *Wave, f_str8_array FilePaths);
f_str8_array F_FilesChangedInWatch (f_wave *Wave, f_watch *Watch);
f_b32        F_AnyFilesChangedInWatch(f_wave *Wave, f_watch *Watch);

f_build      *F_BuildCreate (f_wave *Wave, f_build_options Options);
f_b32         F_BuildStart  (f_wave *Wave, f_build *Build);
f_build_state F_BuildTick   (f_wave *Wave, f_build *Build);

f_u8 *F_OS_Reserve   (f_s64 NumberOfBytes);
void  F_OS_Commit    (void *Address, f_s64 BytesToCommit);
f_s64 F_OS_PageSize  (void);


f_u64 F_OS_TimeStamp ();

f_file_info F_OS_FileInfo(f_memory_arena *Arena, f_str8 Filename);
f_s64 F_OS_FileRead       (f_str8 Path, f_u8 *Dest, f_s64 Max);
f_b32 F_OS_FileWriteStr8  (f_str8 Path, f_str8 Data);
f_b32 F_OS_FileDelete     (f_str8 Path);
f_b32 F_OS_FileRename     (f_str8 From, f_str8 To);
f_b32 F_OS_DirCreate      (f_str8 Path);
f_b32 F_OS_DirNext        (f_dir_iter *Iter, f_str8 *OutName);

f_b32 F_OS_ProcessSpawn  (char **Argv, f_process *Out);
f_b32 F_OS_ProcessState  (f_process *P, f_s32 *ExitCode);

void  F_OS_ThreadStart         (f_thread *Thread, void *(*Entry)(void *), void *EntryArgument);
void  F_OS_WaitForSingleThread (f_thread Thread);
void  F_OS_WaitForThreads      (f_thread *Thread, f_s32 NumberOfThreadsToWaitFor);
void  F_OS_FutexWait    (void *Address, f_u64 Expected);
void  F_OS_FutexWakeAll (void *Address);

f_u64 F_OS_TimeStamp (void);
f_s64 F_OS_NumberOFHighPerformanceCPUs (void);

#if defined(_MSC_VER)
#define OS_WINDOWS
#endif
#if(__APPLE__)
#define OS_DARWIN

#if defined(OS_WINDOWS)
typedef DWORD (WINAPI *f_thread_function)(void *);
#define F_Internal_ThreadEntry(Argument) DWORD WINAPI F_Internal_ThreadEntry(f_thread_entry_params *Argument)
#elif defined(OS_DARWIN)
typedef void *(*f_thread_function)(void *);
#define F_Internal_ThreadEntry(Argument) void *F_Internal_ThreadEntry(f_thread_entry_params *Argument)
#endif

#define F_LaneEntry(Wave) void F_LaneEntry(f_wave *Wave)

F_LaneEntry(Wave);

#include <sys/mman.h> // mmap, mprotect
#include <unistd.h> // getpagesize()
#include <os/os_sync_wait_on_address.h> // os_sync_wake_on_address(), os_sync_wait_on_address()
#include <pthread.h> // pthread_create(), pthread_join()
#include <sys/sysctl.h> // sysctlbyname()
#include <mach/mach_time.h> // mach_absolute_time()
#include <fcntl.h> // open()
#include <sys/stat.h> // stat
#include <spawn.h> // posix_spawn()
#include <util.h> // openpty()

extern char **environ;

#endif

#endif
#define FRIDAY_IMPLEMENTATION
#ifdef FRIDAY_IMPLEMENTATION

#include <stdatomic.h> // atomic_fetch_add, atomic_store, atomic_load, atomic_memory_fence ...
#include <stdio.h> // printf()
#include <string.h> // memcpy
#include <assert.h> // assert()
#include <stdarg.h> // va_list, va_args etc


f_toolchain F_TOOLCHAIN_CLANG = 
{
	.CompilerPath = F_Str8("clang"),
	.ObjectFlag = F_Str8("-o"),
	.DebugFlag = F_Str8("-g"),
	.OptimiseFlag = F_Str8("-O2"),
	.IncludeFlag = F_Str8("-I"),
	.SharedFlag = F_Str8("-dynamiclib"),

};

f_toolchain F_TOOLCHAIN_MSVC =
{
	// TODO
};

f_toolchain F_TOOLCHAIN_GCC = 
{
	// TODO
};

f_toolchain F_TOOLCHAIN_CARNAGE =
{
	// TODO
};


#if defined(_MSC_VER)
  #include <intrin.h>
#define F_Yield() YieldProcessor()
#elif defined(__i386__) || defined(__x86_64__)
#define F_YIELD() __builtin_ia32_pause()
#elif defined(__aarch64__)
#define F_Yield() __asm__ __volatile__("isb sy" ::: "memory")
#else
#error "Unsupported OS or CPU architecture"
#endif

#define F_Assert(Condition) assert(Condition)
#define F_MemoryCopy(Destination, Source, BytesToCopy) memcpy(Destination, Source, BytesToCopy)

#define F_True  ((f_b32)1)
#define F_False ((f_b32)0)

f_u8 *F_OS_Reserve(f_s64 NumberOfBytes)
{
	f_u8 *Result;
#if defined(OS_WINDOWS)
	Result = (f_u8*)VirtualAlloc((void*)0, NumberOfBytes, MEM_RESERVE, PAGE_READWRITE);
#elif defined(OS_DARWIN)
	Result = (f_u8 *)mmap(F_Null, NumberOfBytes, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
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

f_u64 F_OS_TimeStamp(void)
{
	f_u64 Result;
#if defined(OS_WINDOWS)
	Result = 
#elif defined(OS_DARWIN)
	Result = mach_absolute_time();
#endif
	return Result;
}

f_s64 F_OS_NumberOFHighPerformanceCPUs(void)
{
	f_s64 Result = 0;
	// Windows assumes only high performance for now
#if defined(OS_WINDOWS)
	SYSTEM_INFO SystemInfo = {0};
	GetSystemInfo(&SystemInfo);
	Result = SystemInfo.dwNumberOfProcessors;
#elif defined(OS_DARWIN)
	f_s32 Count = 0;
	size_t Size = sizeof Count;
	if(sysctlbyname("hw.perflevel0.physicalcpu", &Count, &Size, F_Null, 0) != 0)
	{
		Size = sizeof Count;
		if(sysctlbyname("hw.physicalcpu", &Count, &Size, F_Null, 0) != 0) Count = 1;
	}
	Result = Count;
#else
#error "Unsupported OS"
#endif
	return Result;
}

f_s64 F_OS_PageSize(void)
{
	f_s64 PageSize;
#if defined(OS_WINDOWS)
	SYSTEM_INFO SystemInfo = {0};
	GetSystemInfo(&SystemInfo);

	PageSize = SystemInfo.dwPageSize;
#elif defined(OS_DARWIN)
	PageSize = getpagesize();
#else
#error "Unsupported OS"
#endif
	return PageSize;
}

void F_OS_ThreadStart(f_thread *Thread, f_thread_function ThreadEntry, void *EntryArgument)
{
#if defined(OS_WINDOWS)
	CreateThread(0, 0, Entry, EntryArgument, 0, &Thread.Handle);
#elif defined(OS_DARWIN)
	pthread_create((pthread_t*)&Thread->Handle, F_Null, ThreadEntry, EntryArgument);
#else
#error "Unsupported OS"
#endif
}

void  F_OS_WaitForSingleThread(f_thread Thread)
{
#if defined(OS_WINDOWS)
	WaitForSingleObject((HANDLE)Thread.Handle, INFINITE);
#elif defined(OS_DARWIN)
	pthread_join((pthread_t)Thread.Handle, F_Null);
#else
#error "Unsupported OS"
#endif
}

void  F_OS_WaitForThreads(f_thread *Threads, f_s32 NumberOfThreadsToWaitFor)
{
#if defined(OS_WINDOWS)
	WaitForMultipleObjects(NumberOfThreadsToWaitFor, (const HANDLE*)Threads, TRUE, INFINITE);
#elif defined(OS_DARWIN)
	for(f_s32 ThreadIndex = 0; ThreadIndex < NumberOfThreadsToWaitFor; ThreadIndex++)
	{
		pthread_join((pthread_t)Threads[ThreadIndex].Handle, F_Null);
	}
#else
#error "Unsupported OS"
#endif
}

void F_OS_FutexWait(void *Address, f_u64 Expected)
{
#if defined(OS_WINDOWS)
	WaitOnAddress(Address, &Expected, sizeof Expected, INFINITE);
#elif defined(OS_DARWIN)
	os_sync_wait_on_address(Address, Expected, sizeof Expected, OS_SYNC_WAIT_ON_ADDRESS_NONE);
#else
#error "Unsupported OS"
#endif
}

void F_OS_FutexWakeAll(void *Address)
{
#if defined(OS_WINDOWS)
	WakeByAddressAll(Address);
#elif defined(OS_DARWIN)
	os_sync_wake_by_address_all(Address, sizeof Address, OS_SYNC_WAKE_BY_ADDRESS_NONE);
#else
#error "Unsupported OS"
#endif
}

f_memory_arena *F_ArenaReserveButCommitFirstPage(f_s64 NumberOfBytesToReserve)
{
	f_u8 *Start = F_OS_Reserve(NumberOfBytesToReserve + F_OS_PageSize());
	F_OS_Commit(Start, F_OS_PageSize());
	f_memory_arena *Arena = (f_memory_arena*)Start;
	Arena->Start = Start + F_OS_PageSize();
	Arena->At = 0;
	Arena->Committed = 0;
	return Arena;
}

void  F_ArenaReset(f_memory_arena *Arena)
{
	Arena->At = 0;
}

void  F_ArenaResetTo(f_memory_arena *Arena, f_s64 Mark)
{
	Arena->At = Mark;
}

f_u8 *F_Raw_ArenaPush(f_memory_arena *Arena, f_s64 BytesToPush)
{
	if(BytesToPush > Arena->Committed - Arena->At)
	{
		f_s64 PagesToCommitInBytes = (BytesToPush + F_OS_PageSize() - 1) & ~(F_OS_PageSize() - 1);
		F_OS_Commit(Arena->Start + Arena->Committed, PagesToCommitInBytes);
		Arena->Committed += PagesToCommitInBytes;
	}
	f_u8 *Result = Arena->Start + Arena->At;
	Arena->At += (BytesToPush + 15) & ~15;
	return Result;
}

f_b32 F_WaveIsFirstLane(f_wave *Wave)
{
	F_WaveBarrier(Wave);
	return Wave->Lane == 0;
}

f_u64 F_Internal_WaveCountTrue(f_wave *Wave, f_b32 Condition)
{
	if(F_WaveIsFirstLane(Wave)) atomic_store_explicit(Wave->Vote, 0, memory_order_relaxed);
	F_WaveBarrier(Wave);
	atomic_fetch_add_explicit(Wave->Vote, (Condition != 0), memory_order_relaxed);
	F_WaveBarrier(Wave);
	f_u64 Result = atomic_load_explicit(Wave->Vote, memory_order_relaxed);
	F_WaveBarrier(Wave);
	return Result;
}

f_b32 F_Internal_WaveAnyTrue(f_wave *Wave, f_b32 Condition)
{
	return F_Internal_WaveCountTrue(Wave, Condition) > 0;
}

f_b32 F_Internal_WaveAllTrue(f_wave *Wave, f_b32 Condition)
{
	return F_Internal_WaveCountTrue(Wave, Condition) == (f_u64)Wave->LaneCount;
}

f_u64 F_WaveExclusivePrefixSum (f_wave *Wave, f_u64 Value)
{
	Wave->BroadcastPointers[Wave->Lane] = &Value;
	F_WaveBarrier(Wave);

	f_u64 ValueForThisLane = 0;
	for(f_u64 I = 0; I < Wave->Lane; I++)
	{
		ValueForThisLane += *(f_u64 *)Wave->BroadcastPointers[I];
	}
	F_WaveBarrier(Wave);
	return ValueForThisLane;
}

f_u64 F_WaveInclusivePrefixSum (f_wave *Wave, f_u64 Value)
{
	return F_WaveExclusivePrefixSum(Wave, Value) + Value;
}

void F_WaveBarrier(f_wave *Wave)
{
	f_s64 Generation = atomic_load_explicit(&Wave->Barrier->Generation, memory_order_relaxed);
	f_s64 LanesArrived = atomic_fetch_add_explicit(&Wave->Barrier->LanesWaiting, 1, memory_order_release);

	if(LanesArrived == Wave->Barrier->LaneCount - 1)
	{
		atomic_thread_fence(memory_order_acquire);
		atomic_store_explicit(&Wave->Barrier->LanesWaiting, 0, memory_order_relaxed);
		atomic_store_explicit(&Wave->Barrier->Generation, Generation + 1, memory_order_release);
		F_OS_FutexWakeAll(&Wave->Barrier->Generation);
	}
	else
	{
		f_s64 Spins = 0;
		for(;;)
		{
			if(atomic_load_explicit(&Wave->Barrier->Generation, memory_order_acquire) != Generation) break;

			if(Spins < 4096)
			{
				F_Yield();
			}
			else
			{
				F_OS_FutexWait(&Wave->Barrier->Generation, Generation);
			}
			Spins++;
		}
	}
}

#if defined(OS_WINDOWS)
F_Internal_ThreadEntry(ThreadParams)
{
	ThreadParams->LaneEntry(ThreadParams->WaveArgument);
	return 0;
}
#elif defined(OS_DARWIN)
F_Internal_ThreadEntry(ThreadParams)
{
	ThreadParams->LaneEntry(ThreadParams->WaveArgument);
	return F_Null;
}
#else
#error "Unsupported OS"
#endif

void F_DispatchAcrossNLanes(f_s64 LaneCount)
{
	f_memory_arena *GlobalArena = F_ArenaReserveButCommitFirstPage(F_GigaBytes(8));
	void **BroadcastPointers   = F_ArenaPushArrayType(GlobalArena, LaneCount, void*);
	f_thread              *Threads = F_ArenaPushArrayType(GlobalArena, LaneCount, f_thread);
	f_wave                *Waves   = F_ArenaPushArrayType(GlobalArena, LaneCount, f_wave);
	f_thread_entry_params *Params  = F_ArenaPushArrayType(GlobalArena, LaneCount, f_thread_entry_params);
	f_barrier Barrier = {0};
	Barrier.LaneCount = LaneCount;

	// Only defined for the first lane
	Waves[0].GlobalArena = GlobalArena;

	_Atomic(f_u64) Vote = 0;
	_Atomic(f_u64) Ballot = 0;
	for(int I = 0; I < LaneCount; I++)
	{
		Waves[I].Lane = I;
		Waves[I].LaneCount = LaneCount;
		Waves[I].Barrier = &Barrier;
		Waves[I].BroadcastPointers = BroadcastPointers;
		Waves[I].Vote = &Vote;
		Waves[I].Ballot = &Ballot;
		// Need a better idea for this
		Waves[I].Arena = F_ArenaReserveButCommitFirstPage(F_GigaBytes(8));
		
		Params[I].WaveArgument = Waves + I;
		Params[I].LaneEntry = F_LaneEntry;
		F_OS_ThreadStart(Threads + I, (f_thread_function)F_Internal_ThreadEntry, Params + I);
	}

	F_OS_WaitForThreads(Threads, LaneCount);
}

f_s64 F_Str8ListCount(f_str8_list List)
{
	f_s64 Count = 0;
	for(f_str8_list_node *Node = List.First; Node; Node = Node->Next)
	{
		Count++;
	}
	return Count;
}

f_s64 F_Str8ListLength(f_str8_list List)
{
	f_s64 Length = 0;
	for(f_str8_list_node *Node = List.First; Node; Node = Node->Next)
	{
		Length += Node->String.Size;
	}
	return Length;
}

f_str8 F_Str8PushFV(f_memory_arena *Arena, char *Format, va_list InputArgs)
{
	va_list ArgList;
	va_copy(ArgList, InputArgs);
	f_u32 BytesNeeded = vsnprintf(0, 0, Format, InputArgs) + 1;
	f_str8 Result;
	Result.Bytes = F_ArenaPushArrayType(Arena, BytesNeeded, f_s8);
	Result.Size = vsnprintf((char *)Result.Bytes, BytesNeeded, Format, ArgList);
	Result.Bytes[Result.Size] = 0;
	va_end(ArgList);
	return Result;
}

f_str8 F_Str8PushF(f_memory_arena *Arena, char *Format, ...)
{
	va_list args;
	va_start(args, Format);
	f_str8 Result = F_Str8PushFV(Arena, Format, args);
	va_end(args);
	return Result;
}

void F_Str8ListAppend(f_memory_arena *Arena, f_str8_list *List, f_str8 StringToAppend)
{
	if(List->Last)
	{
		f_str8_list_node *Next = F_ArenaPushType(Arena, f_str8_list_node);
		List->Last->Next = Next;
		List->Last = Next;
		Next->String = StringToAppend;
	}
	else
	{
		f_str8_list_node *Next = F_ArenaPushType(Arena, f_str8_list_node);
		List->Last = Next;
		List->First = Next;
		Next->String = StringToAppend;
	}
}

f_str8_array F_Str8ArrayFromList(f_memory_arena *Arena, f_str8_list List)
{
	f_s64 Count = F_Str8ListCount(List);
	f_str8_array Result;
	Result.Items = F_ArenaPushArrayType(Arena, Count, f_str8);
	Result.Count = Count;
	f_s64 Index = 0;
	for(f_str8_list_node *Node = List.First; Node; Node = Node->Next)
	{
		Result.Items[Index] = Node->String;
		Index++;
	}
	return Result;
}

f_str8 F_Str8JoinArray(f_memory_arena *Arena, f_str8_array Array, f_str8 Separator)
{
	f_s64 TotalStringSize = 0;
	for(f_s64 I = 0; I < Array.Count; I++)
	{
		TotalStringSize += Array.Items[I].Size;
		if(I < Array.Count - 1) TotalStringSize += Separator.Size;
	}
	f_str8 Result;
	Result.Bytes = F_ArenaPushArrayType(Arena, TotalStringSize, f_s8);
	Result.Size = TotalStringSize;

	f_s64 Start = 0;
	for(f_s64 I = 0; I < Array.Count; I++)
	{
		F_MemoryCopy(Result.Bytes + Start, Array.Items[I].Bytes, Array.Items[I].Size);
		Start += Separator.Size;
		F_MemoryCopy(Result.Bytes + Start, Separator.Bytes, Separator.Size);
		Start += Array.Items[I].Size;
	}
	return Result;
}

f_str8 F_Str8JoinList(f_memory_arena *Arena, f_str8_list List, f_str8 Separator)
{
	f_s64 TotalStringSize = 0;
	for(f_str8_list_node *Node = List.First; Node; Node = Node->Next)
	{
		TotalStringSize += Node->String.Size;
		if(Node->Next != List.Last) TotalStringSize += Separator.Size;
	}
	f_str8 Result;
	Result.Bytes = F_ArenaPushArrayType(Arena, TotalStringSize, f_s8);
	Result.Size = TotalStringSize;

	f_s64 Start = 0;
	for(f_str8_list_node *Node = List.First; Node; Node = Node->Next)
	{
		F_MemoryCopy(Result.Bytes + Start, Node->String.Bytes, Node->String.Size);
		Start += Separator.Size;
		F_MemoryCopy(Result.Bytes + Start, Separator.Bytes, Separator.Size);
		Start += Node->String.Size;
	}
	return Result;
}

f_b32 F_Str8Equal(f_str8 A, f_str8 B)
{
	if(A.Size != B.Size) return F_False;
	f_b32 Result = F_True;
	for(f_s64 I = 0; I < A.Size; I++)
	{
		if(A.Bytes[I] != B.Bytes[I])
		{
			Result = F_False;
			break;
		}
	}
	return Result;
}

f_b32 F_Str8EndsWith(f_str8 String, f_str8 Suffix)
{
	if(String.Size < Suffix.Size) return F_False;
	f_str8 StringTail = (f_str8)
	{
		Suffix.Size, String.Bytes + (Suffix.Size - String.Size)
	};
	f_b32 Result = F_Str8Equal(StringTail, Suffix);
	return Result;
}

char *F_CStrFromStr8(f_memory_arena *Arena, f_str8 String)
{
	char *CString = (char *)F_ArenaPush(Arena, String.Size + 1);
	F_MemoryCopy(CString, String.Bytes, String.Size);
	CString[String.Size] = 0;
	return CString;
}

f_file_info F_OS_FileInfo(f_memory_arena *Arena, f_str8 Filename)
{
	f_file_info FileInfo = {0};
#if defined(OS_WINDOWS)
#error "Implement F_OS_FileInfo for OS_WINDOWS"
#elif defined(OS_DARWIN)
	struct stat FileStat;
	stat(F_CStrFromStr8(Arena, Filename), &FileStat);
	FileInfo.SizeInBytes = FileStat.st_size;
	FileInfo.Exists = F_True;
	f_u64 MillisecondsSinceEpoch = 
		(f_u64)FileStat.st_mtimespec.tv_sec * 1000ull
		+ (uint64_t)FileStat.st_mtimespec.tv_nsec / 1000000ull;
	FileInfo.LastModified = MillisecondsSinceEpoch;
#else
#error "Unsupported OS"
#endif
	return FileInfo;
}

f_watch F_WatchFilesFromArray(f_wave *Wave, f_str8_array FilePaths)
{
	f_watch Result = {0};
	if(F_WaveIsFirstLane(Wave))
	{
		Result.Files = F_ArenaPushArrayType(Wave->GlobalArena, FilePaths.Count, f_str8);
		Result.TimeStamps = F_ArenaPushArrayType(Wave->GlobalArena, FilePaths.Count, f_u64);
		Result.FileCount = FilePaths.Count;
	}
	
	F_BroadcastFirstLane(Wave, Result);

	F_WaveFor(Wave, I, FilePaths.Count)
	{
		f_file_info FileInfo = F_OS_FileInfo(Wave->Arena, FilePaths.Items[I]);
		Result.TimeStamps[I] = FileInfo.LastModified;
		Result.Files[I] = FilePaths.Items[I];
		Result.FileCount = FilePaths.Count;
	}

	return Result;
}

f_str8_array F_FilesChangedInWatch (f_wave *Wave, f_watch *Watch)
{
	f_str8_array Result = {0};
	f_s32 FilesChanged = 0;
	F_WaveFor(Wave, I, Watch->FileCount)
	{
		f_file_info FileInfo = F_OS_FileInfo(Wave->Arena, Watch->Files[I]);
		if(Watch->TimeStamps[I] != FileInfo.LastModified)
		{
			
		}
	}
	// TODO
}

f_b32 F_AnyFilesChangedInWatch(f_wave *Wave, f_watch *Watch)
{
	f_b32 AnyChanged = F_False;
	f_s64 Mark = Wave->Arena->At;
	F_WaveFor(Wave, I, Watch->FileCount)
	{
		f_file_info FileInfo = F_OS_FileInfo(Wave->Arena, Watch->Files[I]);
		if(Watch->TimeStamps[I] != FileInfo.LastModified)
		{
			AnyChanged = F_True;
		}
	}
	F_ArenaResetTo(Wave->Arena, Mark);
	return F_WaveAnyTrue(Wave, AnyChanged);
}

void  F_ProcessSpawn (f_memory_arena *Arena, f_process *Process, f_str8_list Args)
{
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	posix_spawn_file_actions_t SpawnActions;

	int MasterFD = -1;
	int SlaveFD = -1;
	openpty(&MasterFD, &SlaveFD, F_Null, F_Null, F_Null);

	posix_spawn_file_actions_init(&SpawnActions);

	posix_spawn_file_actions_adddup2(&SpawnActions, SlaveFD, STDOUT_FILENO);
	posix_spawn_file_actions_adddup2(&SpawnActions, SlaveFD, STDERR_FILENO);
	posix_spawn_file_actions_addclose(&SpawnActions, MasterFD);
	posix_spawn_file_actions_addclose(&SpawnActions, SlaveFD);

	f_s32 MaxArguments = 1024;
	char **CompilerArguments = F_ArenaPushArrayType(Arena, MaxArguments, char*);
	f_s32 ArgumentCount = 0;
	for(f_str8_list_node *Node = Args.First; Node; Node = Node->Next)
	{
		CompilerArguments[ArgumentCount++] = F_CStrFromStr8(Arena, Node->String);
	}
	CompilerArguments[ArgumentCount] = F_Null;


	f_s32 ProcessStatus = posix_spawnp(&Process->ID, CompilerArguments[0], &SpawnActions, F_Null, CompilerArguments, environ);
	posix_spawn_file_actions_destroy(&SpawnActions);
	
	if(ProcessStatus == 0)
	{
		Process->State = F_PROCESS_RUNNING;
	}
	else
	{
		Process->State = F_PROCESS_FAILED_TO_LAUNCH;
	}
#endif
}

f_b32 F_ProcessPoll  (f_process *Process);
void  F_ProcessWait  (f_process *Process);

#endif