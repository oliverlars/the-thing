#ifndef FRIDAY_H
#define FRIDAY_H

#define FRIDAY_VERSION "2"

#if defined(_MSC_VER)
#define OS_WINDOWS
#endif
#if(__APPLE__)
#define OS_DARWIN
#endif

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
#define F_Str8(String) ((f_str8) { sizeof(String"") - 1, (f_s8*)String })

#define F_Min(X, Y) (X < Y ? X : Y)
#define F_Max(X, Y) (X > Y ? X : Y)

#if defined(OS_WINDOWS)
#define F_Trap() __debugbreak()
#else
#define F_Trap() __builtin_trap()
#endif

void F_Internal_AssertFailed(char *File, int Line, char *Expression);

#define F_Assert(Condition)\
do {\
				if(!(Condition)) {\
								F_Internal_AssertFailed(__FILE__, __LINE__, #Condition);\
								F_Trap();\
				}\
} while(0)

void *F_MemoryCopy(void *Destination, void *Source, f_u64 BytesToCopy);
void *F_MemoryZero(void *Destination, f_u64 BytesToZero);

#define F_True  ((f_b32)1)
#define F_False ((f_b32)0)

#define F_RED     "\x1B[31m"
#define F_GREEN   "\x1B[32m"
#define F_YELLOW  "\x1B[33m"
#define F_BLUE    "\x1B[34m"
#define F_MAGENTA "\x1B[35m"
#define F_CYAN    "\x1B[36m"
#define F_WHITE   "\x1B[37m"

#define F_BOLD    "\033[1m"
#define F_DIM       "\x1b[2m"
#define F_RESET   "\x1B[0m"


#define F_Temporary(Arena) for(f_s64 __Mark__ = Arena->At, _I_ = 0; !_I_; (_I_++, F_ArenaResetTo((Arena), __Mark__)))


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


#define F_PROCESS_STDOUT_CAPACITY (F_KiloBytes(64))

typedef enum f_process_state
{
	F_PROCESS_NONE,
	F_PROCESS_FAILED_TO_LAUNCH,
	F_PROCESS_RUNNING,
	F_PROCESS_EXITED,
}
f_process_state;

typedef struct f_exit_code
{
	f_u64 Values[2];
}
f_exit_code;

typedef struct f_process
{
	f_s32 ID;
	f_process_state State; 
	f_exit_code ExitCode;
	void *StdOut;
	f_str8 StdOutSinceLastPoll;
}
f_process;

typedef struct f_watch
{
	f_str8 *Files;
	f_u64  *TimeStamps;
	f_s32   FileCount;
}
f_watch;


typedef struct f_toolchain
{
	f_str8 CompilerPath;
	f_str8 LinkerPath;

	f_str8 IncludeFlag;
	f_str8 OutputFlag;
	f_str8 DebugFlag;
	f_str8 OptimiseFlag;
	f_str8 DynamicFlag;
	f_str8 StaticFlag;
	f_str8 CompileOnlyFlag;
	f_str8 LinkOnlyFlag;
}
f_toolchain;

#define F_TOOLCHAIN_CLANG ((f_toolchain)\
{\
	.CompilerPath = F_Str8("clang"),\
	.OutputFlag = F_Str8("-o"),\
	.DebugFlag = F_Str8("-g"),\
	.OptimiseFlag = F_Str8("-O2"),\
	.IncludeFlag = F_Str8("-I"),\
	.DynamicFlag = F_Str8("-dynamiclib"),\
	.StaticFlag = F_Str8("-emit-static-lib"),\
	.CompileOnlyFlag = F_Str8("-c")\
})

f_toolchain F_TOOLCHAIN_MSVC;
f_toolchain F_TOOLCHAIN_GCC;
f_toolchain F_TOOLCHAIN_CARNAGE;

typedef struct f_compile_options
{
	f_str8_array IncludePaths;
	f_str8 OutputPath;
	f_str8 OutputName;
	f_str8_array ExtraFlags;
	f_b32 Optimise;
	f_b32 Debug;
}
f_compile_options;

typedef enum f_output_kind
{
	F_OUTPUT_EXECUTABLE,
	F_OUTPUT_DYNAMIC,
	F_OUTPUT_STATIC,
}
f_output_kind;

typedef struct f_link_options
{
	f_str8 OutputPath;
	f_str8 OutputName;
	f_str8_array ExtraFlags;
	f_output_kind OutputKind;
}
f_link_options;

typedef struct f_compilation
{
	f_process Process;

	f_str8 SourcePath;
	f_str8 ObjectPath;
	f_u64 Hash;
}
f_compilation;

typedef struct f_link
{
	f_process Process;
	f_str8 OutputPath;
}
f_link;

typedef struct f_file_info
{
	f_b32 Exists;
	f_u64 LastModifiedInMillisecondsSinceEpoch;
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

#define F_ArenaPushType(Arena, Type) (Type*)F_Raw_ArenaPush(Arena, sizeof(Type), F_True)
#define F_ArenaPushArrayType(Arena, Count, Type) (Type*)F_Raw_ArenaPush(Arena, Count * (sizeof(Type)), F_True)
#define F_ArenaPush(Arena, BytesToPush) F_Raw_ArenaPush(Arena, BytesToPush, F_True)

#define F_ArenaPushTypeNoZero(Arena, Type) (Type*)F_Raw_ArenaPush(Arena, sizeof(Type), F_False)
#define F_ArenaPushArrayTypeNoZero(Arena, Count, Type) (Type*)F_Raw_ArenaPush(Arena, Count * (sizeof(Type)), F_False)
#define F_ArenaPushNoZero(Arena, BytesToPush) F_Raw_ArenaPush(Arena, BytesToPush, F_False)

f_u8 *F_Raw_ArenaPush(f_memory_arena *Arena, f_s64 BytesToPush, f_b32 ShouldZero);
void  F_ArenaReset(f_memory_arena *Arena);

void  F_ArenaResetTo(f_memory_arena *Arena, f_s64 Mark);

#define F_ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define F_Str8ArrayFromMany(Arena, ...) \
F_Internal_Str8ArrayFromMany(Arena, ((f_str8[]){ __VA_ARGS__ }), ((sizeof(f_str8[]){ __VA_ARGS__})/sizeof(f_str8)))


f_str8_array F_Internal_Str8ArrayFromMany(f_memory_arena *Arena, f_str8 *Str8s, f_u64 Str8Count);

#define F_Str8ListAppendF(Arena, List, ...) \
F_Str8ListAppend(Arena, List, F_Str8PushF(Arena, __VA_ARGS__))

f_str8       F_Str8PushF          (f_memory_arena *Arena, char *Format, ...);
void         F_Str8ListAppend     (f_memory_arena *Arena, f_str8_list *List, f_str8 Sstring);
void         F_Str8ListAppendMany (f_memory_arena *Arena, f_str8_list *Head, f_str8_list Tail);
f_str8_array F_Str8ArrayFromList  (f_memory_arena *Arena, f_str8_list List);
f_str8       F_Str8JoinList       (f_memory_arena *Arena, f_str8_list List,   f_str8 Separator);
f_str8       F_Str8JoinArray      (f_memory_arena *Arena, f_str8_array Array, f_str8 Separator);
f_b32        F_Str8Equal          (f_str8 A, f_str8 B);
f_b32        F_Str8EndsWith       (f_str8 S, f_str8 Suffix);
f_s64        F_Str8ListCount      (f_str8_list List);
f_s64        F_Str8ListLength     (f_str8_list List);


void F_DispatchAcrossNLanes(f_s64 LaneCount);

f_b32           F_WaveIsFirstLane        (f_wave *Wave);
f_u64           F_WaveInclusivePrefixSum (f_wave *Wave, f_u64 Value);
f_u64           F_WaveExclusivePrefixSum (f_wave *Wave, f_u64 Value);

f_u64           F_WaveSum (f_wave *Wave, f_u64 Value);

#define         F_WaveAnyTrue(Wave, Value) F_Internal_WaveAnyTrue(Wave, Value != 0)
#define         F_WaveAllTrue(Wave, Value) F_Internal_WaveAllTrue(Wave, Value != 0)
f_b32           F_Internal_WaveAnyTrue   (f_wave *Wave, f_b32 Condition);
f_b32           F_Internal_WaveAllTrue   (f_wave *Wave, f_b32 Condition);
#define         F_WaveCountTrue(Wave, Value) F_Internal_WaveCountTrue(Wave, Value != 0)
f_u64           F_Internal_WaveCountTrue (f_wave *Wave, f_b32 Condition);

// One bit per lane, set if that lane's Value was true. TODO: > 64 lanes
#define         F_WaveBallot(Wave, Value) F_Internal_WaveBallot(Wave, Value != 0)
f_u64           F_Internal_WaveBallot    (f_wave *Wave, f_b32 Condition);
f_u64           F_CountBits              (f_u64 Bits);

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

#define F_EachWaveSplit(Wave, Index, Count) \
    (f_s64 Index                       = (Wave)->Lane, \
           __F_Concat(F_ForEnd__,  __LINE__) = (f_s64)(Count), \
           __F_Concat(F_ForStep__, __LINE__) = (Wave)->LaneCount; \
     Index < __F_Concat(F_ForEnd__, __LINE__); \
     Index += __F_Concat(F_ForStep__, __LINE__))

void  F_StartProcess   (f_memory_arena *Arena, f_process *Process, f_str8_list Args);
f_b32 F_PollProcess    (f_process *Process);
void  F_WaitForProcess (f_process *Process);

f_b32 F_ProcessExitedCleanly(f_process *Process);

#define F_WaitForMultipleProcesses(...)\
do\
{\
	f_process *__F_Processes[] = { __VA_ARGS__ };\
	F_Internal_ProcessWaitMany( __F_Processes, F_ArrayCount(__F_Processes));\
}\
while(0)

void F_Internal_WaitForMultipleProcesses (f_process **Processes, f_u64 ProcessCount);


#define F_DEFAULT_TIMEOUT_MS (16)

f_watch      F_WatchFilesFromArray    (f_wave *Wave, f_str8_array FilePaths);

f_str8_array F_FilesChangedInWatch    (f_memory_arena *Arena, f_wave *Wave, f_watch *Watch, f_u64 ChangeGranularityInMilliseconds);
f_b32        F_AnyFilesChangedInWatch (f_wave *Wave, f_watch *Watch, f_u64 ChangeGranularityInMilliseconds);


#define F_HashStart (0xCBF29CE484222325ULL)

f_u64 F_HashBytes(f_u64 HashStart, void *Data, f_u64 NumberOfBytesToHash);

f_u64 F_HashStr8(f_str8 String);


f_u64 F_HashSourceAndIncludes(f_memory_arena *Arena, f_str8 PathToSource, f_str8_array IncludePaths);
f_compilation F_CompileTU (f_memory_arena *Arena, f_str8 SourceFile, f_compile_options CompileOptions, f_toolchain Toolchain);

f_link F_LinkTUs (f_memory_arena *Arena, f_str8_array ObjectFiles, f_link_options LinkOptions, f_toolchain Toolchain);

f_u8 *F_OS_Reserve   (f_s64 NumberOfBytes);
void  F_OS_Commit    (void *Address, f_s64 BytesToCommit);
f_s64 F_OS_PageSize  (void);

f_u64 F_OS_TimeStamp (void);
f_f32 F_MilliSecondsFromTimeStamp (f_u64 TimeStamp);
f_f32 F_SecondsFromTimeStamp (f_u64 TimeStamp);

typedef struct f_file
{
	// In the future this can set more granular error bits
	f_u64 Errors;
	f_u64 Handle;
}
f_file;

f_file F_OS_FileFromFilename (f_memory_arena *Arena, f_str8 Filename);
void   F_OS_ReleaseFile      (f_file *File);

f_file_info F_OS_FileInfoFromFilename          (f_memory_arena *Arena, f_str8 Filename);
f_file_info F_OS_FileInfoFromFile              (f_file *File);
f_str8      F_OS_ReadEntireFile                (f_memory_arena *Arena, f_file *File);
f_str8      F_OS_ReadEntireFileFromPath        (f_memory_arena *Arena, f_str8 PathToFile);
void        F_OS_WriteStr8ToFile               (f_file *File, f_str8 StringToWrite);
f_b32       F_OS_DeleteFile                    (f_memory_arena *Arena, f_str8 PathToFile);
void        F_OS_RenameFileTo                  (f_memory_arena *Arena, f_str8 From, f_str8 To);
void        F_OS_MakeDirectoryIfNotAlreadyMade (f_memory_arena *Arena, f_str8 Path);

typedef struct f_internal_directory_options
{
	f_b32 ExcludeDirectories;
	f_b32 PrependPath;
	f_str8_array FilterByExtension;
}
f_internal_directory_options;

#define F_OS_ListOfEntriesInDirectory(Arena, DirectoryPath, ...) \
F_OS_Internal_ListOfEntriesInDirectory(Arena, DirectoryPath, (f_internal_directory_options){ __VA_ARGS__ })

f_str8_list F_OS_Internal_ListOfEntriesInDirectory (f_memory_arena *Arena, f_str8 Directory, f_internal_directory_options Options);

void  F_OS_ThreadStart         (f_thread *Thread, void *(*Entry)(void *), void *EntryArgument);
void  F_OS_WaitForSingleThread (f_thread Thread);
void  F_OS_WaitForThreads      (f_thread *Thread, f_s32 NumberOfThreadsToWaitFor);
void  F_OS_FutexWait    (void *Address, f_u64 Expected);
void  F_OS_FutexWakeAll (void *Address);

void F_Print(f_str8 String);
#define F_PrintF(Arena, ...) \
F_Print(F_Str8PushF(Arena, __VA_ARGS__))

f_file F_OS_StandardOut(void);

f_s64 F_OS_NumberOFHighPerformanceCPUs (void);

#if defined(OS_WINDOWS)
typedef DWORD (WINAPI *f_thread_function)(void *);
#define F_Internal_ThreadEntry(Argument) DWORD WINAPI F_Internal_ThreadEntry(f_thread_entry_params *Argument)
#elif defined(OS_DARWIN)
typedef void *(*f_thread_function)(void *);
#define F_Internal_ThreadEntry(Argument) void *F_Internal_ThreadEntry(f_thread_entry_params *Argument)
#endif

#define F_LaneEntry(Wave) void F_LaneEntry(f_wave *Wave)

F_LaneEntry(Wave);


typedef enum f_build_status
{
	F_BUILD_COMPILING,
	F_BUILD_COMPILE_FAILED,
	F_BUILD_LINKING,
	F_BUILD_SUCCEEDED,
	F_BUILD_FAILED,
}
f_build_status;

typedef struct f_build_report
{
	f_compile_options CompileOptions;
	f_link_options    LinkOptions;
	f_toolchain       Toolchain;
	f_u64             TotalTUsToCompile;
	f_u64             TUsCompiledSoFar;
	f_build_status    Status;
}
f_build_report;

void F_Report(f_memory_arena *Arena, f_build_report Report);

#endif
#define FRIDAY_IMPLEMENTATION
#ifdef FRIDAY_IMPLEMENTATION

#include <stdatomic.h> // atomic_fetch_add, atomic_store, atomic_load, atomic_memory_fence ...
#include <stdio.h> // printf()
#include <assert.h> // assert()
#include <stdarg.h> // va_list, va_args etc
#include <string.h> // memset
#include <sys/mman.h> // mmap, mprotect
#include <unistd.h> // getpagesize()
#include <os/os_sync_wait_on_address.h> // os_sync_wake_on_address(), os_sync_wait_on_address()
#include <pthread.h> // pthread_create(), pthread_join()
#include <sys/sysctl.h> // sysctlbyname()
#include <mach/mach_time.h> // mach_absolute_time()
#include <fcntl.h> // open()
#include <sys/stat.h> // stat
#include <spawn.h> // posix_spawn()
#include <sys/wait.h> // waitpid()
#include <dirent.h>

extern char **environ;


void *F_MemoryCopy(void *Destination, void *Source, f_u64 BytesToCopy)
{
	return memcpy(Destination, Source, BytesToCopy);
}

void *F_MemoryZero(void *Destination, f_u64 BytesToZero)
{
	return memset(Destination, 0, BytesToZero);
}

void F_Internal_AssertFailed(char *File, int Line, char *Expression)
{
	// HACK: switch to user provided logger
	printf("%s:%d:" F_RED " assert" F_RESET ": %s\n", File, Line, Expression);
}

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

f_f32 F_MilliSecondsFromTimeStamp(f_u64 TimeStamp)
{
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	mach_timebase_info_data_t Info;
	mach_timebase_info(&Info);

	return (f_f32)((f_f32)(TimeStamp) * Info.numer / Info.denom) / 1000000.0;
#else
#error "Unsupported OS"
#endif
}

f_f32 F_SecondsFromTimeStamp(f_u64 TimeStamp)
{
	return F_MilliSecondsFromTimeStamp(TimeStamp) * 1000.0;
}

f_file F_OS_StandardOut(void)
{
	f_file File = {0};
#if defined(OS_WINDOWS)
#error "TODO: implement on Windows"
#elif defined(OS_DARWIN)
	File.Handle = STDOUT_FILENO;
#else
#error "Unsupported OS"
#endif
	return File;
}

void F_Print(f_str8 String)
{
	f_file StdOut = F_OS_StandardOut();
	F_OS_WriteStr8ToFile(&StdOut, String);
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

f_u8 *F_Raw_ArenaPush(f_memory_arena *Arena, f_s64 BytesToPush, f_b32 ShouldZero)
{
	if(BytesToPush > Arena->Committed - Arena->At)
	{
		f_s64 PagesToCommitInBytes = (BytesToPush + F_OS_PageSize() - 1) & ~(F_OS_PageSize() - 1);
		F_OS_Commit(Arena->Start + Arena->Committed, PagesToCommitInBytes);
		Arena->Committed += PagesToCommitInBytes;
	}
	f_u8 *Result = Arena->Start + Arena->At;
	Arena->At += (BytesToPush + 15) & ~15;
	if(ShouldZero)
	{
		F_MemoryZero(Result, BytesToPush);
	}
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

f_u64 F_Internal_WaveBallot(f_wave *Wave, f_b32 Condition)
{
	F_Assert(Wave->LaneCount <= 64);
	if(F_WaveIsFirstLane(Wave)) atomic_store_explicit(Wave->Ballot, 0, memory_order_relaxed);
	F_WaveBarrier(Wave);
	if(Condition) atomic_fetch_or_explicit(Wave->Ballot, 1ull << Wave->Lane, memory_order_relaxed);
	F_WaveBarrier(Wave);
	f_u64 Result = atomic_load_explicit(Wave->Ballot, memory_order_relaxed);
	F_WaveBarrier(Wave);
	return Result;
}

f_u64 F_CountBits(f_u64 Bits)
{
	f_u64 Result = 0;
	for(; Bits; Bits &= Bits - 1) Result++;
	return Result;
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

f_u64 F_WaveSum(f_wave *Wave, f_u64 Value)
{
	Wave->BroadcastPointers[Wave->Lane] = &Value;
	F_WaveBarrier(Wave);
	f_u64 Sum = 0;
	for(f_u64 I = 0; I < Wave->LaneCount; I++)
	{
		Sum += *(f_u64 *)Wave->BroadcastPointers[I];
	}
	F_WaveBarrier(Wave);
	return Sum;
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
		if(I >= 1) F_OS_ThreadStart(Threads + I, (f_thread_function)F_Internal_ThreadEntry, Params + I);
	}

	Params[0].LaneEntry(Params[0].WaveArgument);

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

f_str8_array F_Internal_Str8ArrayFromMany(f_memory_arena *Arena, f_str8 *Str8s, f_u64 Str8Count)
{
	f_str8_array Result = {0};
	Result.Items = F_ArenaPushArrayType(Arena, Str8Count, f_str8);
	Result.Count = Str8Count;

	for(int I = 0; I < Str8Count; I++)
	{
		Result.Items[I].Bytes = F_ArenaPushArrayType(Arena, Str8s[I].Size, f_s8);
		Result.Items[I].Size = Str8s[I].Size;
		F_MemoryCopy(Result.Items[I].Bytes, Str8s[I].Bytes, Str8s[I].Size);
	}

	return Result;
}

void F_Str8ListAppendMany(f_memory_arena *Arena, f_str8_list *Head, f_str8_list Tail)
{
	for(f_str8_list_node *Node = Tail.First; Node; Node = Node->Next)
	{
		F_Str8ListAppend(Arena, Head, Node->String);
	}
}

void F_Str8ListAppend(f_memory_arena *Arena, f_str8_list *List, f_str8 StringToAppend)
{
	f_str8_list_node *Next = F_ArenaPushType(Arena, f_str8_list_node);
	if(List->Last)
	{
		List->Last->Next = Next;
		List->Last = Next;
	}
	else
	{
		List->Last = Next;
		List->First = Next;
	}
	Next->String = StringToAppend;
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
		Start += Array.Items[I].Size;
		if(I < Array.Count - 1)
		{
			F_MemoryCopy(Result.Bytes + Start, Separator.Bytes, Separator.Size);
			Start += Separator.Size;
		}
	}
	return Result;
}

f_str8 F_Str8JoinList(f_memory_arena *Arena, f_str8_list List, f_str8 Separator)
{
	f_s64 TotalStringSize = 0;
	for(f_str8_list_node *Node = List.First; Node; Node = Node->Next)
	{
		TotalStringSize += Node->String.Size;
		if(Node != List.Last) TotalStringSize += Separator.Size;
	}
	f_str8 Result;
	Result.Bytes = F_ArenaPushArrayType(Arena, TotalStringSize, f_s8);
	Result.Size = TotalStringSize;

	f_s64 Start = 0;
	for(f_str8_list_node *Node = List.First; Node; Node = Node->Next)
	{
		F_MemoryCopy(Result.Bytes + Start, Node->String.Bytes, Node->String.Size);
		Start += Node->String.Size;
		if(Node != List.Last)
		{
			F_MemoryCopy(Result.Bytes + Start, Separator.Bytes, Separator.Size);
			Start += Separator.Size;
		}
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
		Suffix.Size, String.Bytes + (String.Size - Suffix.Size)
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

f_file_info F_OS_FileInfoFromFile(f_file *File)
{
	f_file_info FileInfo = {0};
#if defined(OS_WINDOWS)
#error "Implement F_OS_FileInfo for OS_WINDOWS"
#elif defined(OS_DARWIN)
	struct stat FileStat = {0};
	f_s32 Status = fstat(File->Handle, &FileStat);
	if(Status) 
	{
		File->Errors = F_True;
		return (f_file_info){0};
	}
	FileInfo.SizeInBytes = FileStat.st_size;
	FileInfo.Exists = F_True;
	f_u64 MillisecondsSinceEpoch = 
		(f_u64)FileStat.st_mtimespec.tv_sec * 1000ull
		+ (uint64_t)FileStat.st_mtimespec.tv_nsec / 1000000ull;
	FileInfo.LastModifiedInMillisecondsSinceEpoch = MillisecondsSinceEpoch;
#else
#error "Unsupported OS"
#endif
	return FileInfo;
}

void F_OS_WriteStr8ToFile(f_file *File, f_str8 FileToWrite)
{
	if(File->Errors) return;
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	f_s32 Status = write(File->Handle, FileToWrite.Bytes, FileToWrite.Size);
	File->Errors = Status < 0;
#else
#error "Unsupported OS"
#endif
}

f_b32 F_OS_DeleteFile(f_memory_arena *Arena, f_str8 PathToFile)
{
	f_b32 Result = F_True;
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	F_Temporary(Arena) Result = remove(F_CStrFromStr8(Arena, PathToFile)) == 0;
#else
#error "Unsupported OS"
#endif
	return Result;
}

f_b32 F_OS_RenameFile(f_memory_arena *Arena, f_str8 From, f_str8 To)
{
	f_b32 Result = F_True;
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	F_Temporary(Arena) Result = rename(F_CStrFromStr8(Arena, From), F_CStrFromStr8(Arena, To)) == 0;
#else
#error "Unsupported OS"
#endif
	return Result;
}


f_str8 F_OS_ReadEntireFile(f_memory_arena *Arena, f_file *File)
{
		if(File->Errors) return F_Str8("");
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	f_file_info Info = F_OS_FileInfoFromFile(File);
	f_s8 *Backing = F_ArenaPushArrayType(Arena, Info.SizeInBytes, f_s8);
	f_s32 Status = read(File->Handle, Backing, Info.SizeInBytes);
	File->Errors = Status < 0;
	return (f_str8) { Info.SizeInBytes, Backing };
#else
#error "Unsupported OS"
#endif
}

f_str8 F_OS_ReadEntireFileFromPath(f_memory_arena *Arena, f_str8 PathToFile)
{
	f_file File = F_OS_FileFromFilename(Arena, PathToFile);
	f_str8 Contents = F_OS_ReadEntireFile(Arena, &File);
	F_OS_ReleaseFile(&File);
	return Contents;
}

void F_OS_MakeDirectoryIfNotAlreadyMade(f_memory_arena *Arena, f_str8 Path)
{
#if defined(OS_WINDOWS)
#error "TODO"
#elif defined(OS_DARWIN)
	F_Temporary(Arena)
	{
		mkdir(F_CStrFromStr8(Arena, Path), 0777);
	}
#else
#error "Unsupported OS"
#endif
}

f_file_info F_OS_FileInfoFromFilename(f_memory_arena *Arena, f_str8 Filename)
{
	f_file_info FileInfo = {0};
#if defined(OS_WINDOWS)
#error "Implement F_OS_FileInfo for OS_WINDOWS"
#elif defined(OS_DARWIN)
	struct stat FileStat = {0};
	if(stat(F_CStrFromStr8(Arena, Filename), &FileStat) == 0)
	{
		FileInfo.SizeInBytes = FileStat.st_size;
		FileInfo.Exists = F_True;
		f_u64 MillisecondsSinceEpoch = 
			(f_u64)FileStat.st_mtimespec.tv_sec * 1000ull
			+ (uint64_t)FileStat.st_mtimespec.tv_nsec / 1000000ull;
		FileInfo.LastModifiedInMillisecondsSinceEpoch = MillisecondsSinceEpoch;
	}
#else
#error "Unsupported OS"
#endif
	return FileInfo;
}

f_file F_OS_FileFromFilename(f_memory_arena *Arena, f_str8 Filename)
{
#if defined(OS_WINDOWS)
#error "TODO"
#elif defined(OS_DARWIN)
	f_file File = {0};
	f_u64 Handle = open(F_CStrFromStr8(Arena, Filename), O_RDWR);
	if(Handle != 0)
	{
		File.Handle = Handle;
	}
	else
	{
		File.Errors = F_True;
	}
#else
#error "Unsupported OS"
#endif
	return File;
}

void F_OS_ReleaseFile(f_file *File)
{
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	close(File->Handle);
#else
#error "Unsupported OS"
#endif
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

	for F_EachWaveSplit(Wave, I, FilePaths.Count)
	{
		f_file_info FileInfo = F_OS_FileInfoFromFilename(Wave->Arena, FilePaths.Items[I]);
		Result.TimeStamps[I] = FileInfo.LastModifiedInMillisecondsSinceEpoch;
		Result.Files[I] = FilePaths.Items[I];
		Result.FileCount = FilePaths.Count;
	}

	return Result;
}

f_str8_array F_FilesChangedInWatch(f_memory_arena *Arena, f_wave *Wave, f_watch *Watch, f_u64 ChangeGranularityInMilliseconds)
{
	f_str8_array Result = {0};
	if(F_WaveIsFirstLane(Wave))
	{
		Result.Items = F_ArenaPushArrayType(Arena, Watch->FileCount, f_str8);
	}
	F_BroadcastFirstLane(Wave, Result);

	f_s64 Mark = Wave->Arena->At;
	f_b32 *FileChanged = F_ArenaPushArrayType(Wave->Arena, Watch->FileCount, f_b32);

	f_u64 FilesChangedThisLane = 0;
	for F_EachWaveSplit(Wave, I, Watch->FileCount)
	{
		f_file_info FileInfo = F_OS_FileInfoFromFilename(Wave->Arena, Watch->Files[I]);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch->TimeStamps[I] > 0) printf("Change time in ms: %llu\n", FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch->TimeStamps[I]);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch->TimeStamps[I] > ChangeGranularityInMilliseconds)
		{
			Watch->TimeStamps[I] = FileInfo.LastModifiedInMillisecondsSinceEpoch;
			FileChanged[I] = F_True;
			FilesChangedThisLane++;
		}
	}

	f_u64 WriteIndex = F_WaveExclusivePrefixSum(Wave, FilesChangedThisLane);
	for F_EachWaveSplit(Wave, I, Watch->FileCount)
	{
		if(FileChanged[I])
		{
			Result.Items[WriteIndex++] = Watch->Files[I];
		}
	}

	// After its own writes the last lane has walked the whole array, so it has the total
	f_u64  TotalFilesChanged = F_WaveSum(Wave, WriteIndex);
	Result.Count = (f_s64)TotalFilesChanged;

	F_ArenaResetTo(Wave->Arena, Mark);
	return Result;
}

f_u64 F_HashBytes(f_u64 HashStart, void *Data, f_u64 NumberOfBytesToHash)
{
	f_u8 *Bytes = Data;
	f_u64 Hash = HashStart;
	for(;NumberOfBytesToHash--;)
	{
		Hash ^= *Bytes++;
		Hash *= 0x100000001B3ULL;
	}
	return Hash;
}

f_u64 F_HashStr8(f_str8 String)
{
	return F_HashBytes(F_HashStart, String.Bytes, String.Size);
}

f_b32 F_AnyFilesChangedInWatch(f_wave *Wave, f_watch *Watch, f_u64 ChangeGranularityInMilliseconds)
{
	f_b32 AnyChanged = F_False;
	f_s64 Mark = Wave->Arena->At;
	for F_EachWaveSplit(Wave, I, Watch->FileCount)
	{
		f_file_info FileInfo = F_OS_FileInfoFromFilename(Wave->Arena, Watch->Files[I]);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch->TimeStamps[I] > ChangeGranularityInMilliseconds)
		{
			AnyChanged = F_True;
		}
	}
	F_ArenaResetTo(Wave->Arena, Mark);
	return F_WaveAnyTrue(Wave, AnyChanged);
}

f_str8_list F_OS_Internal_ListOfEntriesInDirectory(f_memory_arena *Arena, f_str8 DirectoryPath, f_internal_directory_options Options)
{
	f_str8_list Entries = { 0 };
#if defined(OS_WINDOWS)
#error "TODO"
#elif defined(OS_DARWIN)
	DIR *Directory = opendir(F_CStrFromStr8(Arena, DirectoryPath));
	struct dirent *Entry;
	while((Entry = readdir(Directory)))
	{
		f_b32 IsActualEntry = Entry->d_name[0] != '.';
		f_b32 ShouldExcludeDirectories = (Options.ExcludeDirectories && Entry->d_type == DT_DIR);
		if(!IsActualEntry) continue;
		if(ShouldExcludeDirectories) continue;

		f_str8 EntryName = F_Str8PushF(Arena, "%s", Entry->d_name);

		f_b32 MatchFound = F_True;
		if(Options.FilterByExtension.Count > 0)
		{
			MatchFound = F_False;
			for(int I = 0; I < Options.FilterByExtension.Count; I++)
			{
				if(F_Str8EndsWith(EntryName, Options.FilterByExtension.Items[I]))
				{
					MatchFound = F_True;
					break;
				}
			}
		}
		if(!MatchFound) continue;
		
		if(Options.PrependPath)
		{
			F_Str8ListAppendF(Arena, &Entries, "%.*s/%s", F_Expand(DirectoryPath), Entry->d_name);
		}
		else
		{
			F_Str8ListAppendF(Arena, &Entries, "%s", Entry->d_name);
		}
	}
#else
#error "Unsupported OS"
#endif
	return Entries;
}

void F_StartProcess(f_memory_arena *Arena, f_process *Process, f_str8_list Args)
{
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)

	Process->StdOutSinceLastPoll.Bytes = F_ArenaPushArrayType(Arena, F_PROCESS_STDOUT_CAPACITY, f_s8);

	f_s32 MaxArguments = 1024;
	char **CompilerArguments = F_ArenaPushArrayType(Arena, MaxArguments, char*);
	f_s32 ArgumentCount = 0;
	for(f_str8_list_node *Node = Args.First; Node; Node = Node->Next)
	{
		CompilerArguments[ArgumentCount++] = F_CStrFromStr8(Arena, Node->String);
	}
	CompilerArguments[ArgumentCount] = F_Null;

	f_s32 FileDescriptors[2];
	if(pipe(FileDescriptors) == -1)
	{
		Process->State = F_PROCESS_FAILED_TO_LAUNCH;
		return;
	}

	posix_spawn_file_actions_t SpawnActions;
	posix_spawn_file_actions_init(&SpawnActions);
	posix_spawn_file_actions_adddup2(&SpawnActions, FileDescriptors[1], STDOUT_FILENO);
	posix_spawn_file_actions_adddup2(&SpawnActions, FileDescriptors[1], STDERR_FILENO);
	posix_spawn_file_actions_addclose(&SpawnActions, FileDescriptors[0]);
	posix_spawn_file_actions_addclose(&SpawnActions, FileDescriptors[1]);

	f_s32 ProcessStatus = posix_spawnp(&Process->ID, CompilerArguments[0], &SpawnActions, F_Null, CompilerArguments, environ);
	posix_spawn_file_actions_destroy(&SpawnActions);
	close(FileDescriptors[1]);

	if(ProcessStatus == 0)
	{
		fcntl(FileDescriptors[0], F_SETFL, O_NONBLOCK);
		Process->StdOut = (void*)((f_u64)FileDescriptors[0]);
		Process->State = F_PROCESS_RUNNING;
	}
	else
	{
		close(FileDescriptors[0]);
		Process->State = F_PROCESS_FAILED_TO_LAUNCH;
		Process->ExitCode = (f_exit_code){ProcessStatus, 0};
	}
#endif

}

f_b32 F_PollProcess(f_process *Process)
{
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	Process->StdOutSinceLastPoll.Size = 0;


	if(Process->State != F_PROCESS_RUNNING) return F_False;

	f_u64 BytesRead = read((f_s32)((f_u64)Process->StdOut), Process->StdOutSinceLastPoll.Bytes, F_PROCESS_STDOUT_CAPACITY);
	if(BytesRead > 0)
	{
		Process->StdOutSinceLastPoll.Size = (f_u64)BytesRead;
	}

	int Status = 0;
	if (waitpid(Process->ID, &Status, WNOHANG) != Process->ID)
	{
		return F_True;
	}

	for(;Process->StdOutSinceLastPoll.Size < F_PROCESS_STDOUT_CAPACITY;)
	{
		f_s8 *BytesTail = Process->StdOutSinceLastPoll.Bytes + Process->StdOutSinceLastPoll.Size;
		f_u64 BytesLeft = F_PROCESS_STDOUT_CAPACITY - Process->StdOutSinceLastPoll.Size;
		f_s64 Tail = read((f_u32)((f_u64)Process->StdOut), BytesTail, BytesLeft);

		if(Tail <= 0) break;
		else
		{
			Process->StdOutSinceLastPoll.Size += (f_u64)Tail;
		}
	}
	if (WIFEXITED(Status))
	{
		Process->ExitCode = (f_exit_code){WEXITSTATUS(Status), 0};
	}
	else if (WIFSIGNALED(Status))
	{
		Process->ExitCode = (f_exit_code){0, WTERMSIG(Status)};
	}
	else
	{
		return F_True;
	}
	Process->State = F_PROCESS_EXITED;
	close((f_s32)((f_u64)Process->StdOut));
	Process->StdOut = (void*)(-1);
	return F_False;
#else
#error "Unsupported OS"
#endif
}

f_b32 F_ProcessExitedCleanly(f_process *Process)
{
	f_b32 Result = F_True;
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	if(Process->ExitCode.Values[0] || Process->ExitCode.Values[1])
	{
		Result = F_False;
	}
#else
#error "Unsupported OS"
#endif
	return Result;
}

void F_WaitForProcess(f_process *Process)
{
	while(F_PollProcess(Process));
}

void F_Internal_WaitForMultipleProcesses(f_process **Processes, f_u64 ProcessCount)
{
	for(int I = 0; I < ProcessCount; I++)
	{
		F_WaitForProcess(Processes[I]);
	}
}


f_u64 __F_Internal_HashSourceAndIncludes(f_memory_arena *Arena, f_str8 PathToSource, f_str8_array IncludePaths, f_str8_list IncludesSeenAlready)
{
#define CharAt(Index) ((Index) < Source.Size ? Source.Bytes[Index] : 0)
#define AdvanceBy(Index, Amount) ((Index += Amount), Index < Source.Size)
#define Advance(Index) (AdvanceBy(Index, 1))
#define MatchAt(String, Index) F_Str8Equal((f_str8){ F_Min(String.Size, (Source.Size - (Index))), Source.Bytes + (Index) }, String)
#define SkipWhiteSpace(Index) for(;CharAt(Index) == ' ' || CharAt(Index) == '\t';Advance(Index))
#define SkipUntilMatch(Index, Char) for(;CharAt(Index) != 0 && CharAt(Index) != Char; Advance(Index))
#define SkipUntilMatch2(Index, CharA, CharB) for(;CharAt(Index) != 0 && CharAt(Index) != CharA && CharAt(Index + 1) != CharB; Advance(Index))

	f_str8 Source = F_OS_ReadEntireFileFromPath(Arena, PathToSource);
	// Hash whilst advancing?
	f_u64 SourceAndIncludesHash = F_HashStr8(Source);
	for(f_s64 I = 0; I < Source.Size; I++)
	{
		switch(Source.Bytes[I])
		{
			case '#': {
				Advance(I);
				f_str8 Include = F_Str8("include");
				f_str8 PathToInclude = {0};
				if(MatchAt(Include, I))
				{
					AdvanceBy(I, Include.Size);
					SkipWhiteSpace(I);
					if(CharAt(I) == '<')
					{
						Advance(I);
						f_u64 Start = I;
						SkipUntilMatch(I, '>');
						PathToInclude = (f_str8){I - Start, Source.Bytes + Start};
					}
					else if(CharAt(I) == '\"')
					{
						Advance(I);
						f_u64 Start = I;
						SkipUntilMatch(I, '\"');
						PathToInclude = (f_str8){I - Start, Source.Bytes + Start};
					}
				}
				if(PathToInclude.Size > 0)
				{
					f_file_info Info = {0};
					Info = F_OS_FileInfoFromFilename(Arena, PathToInclude);
					
					for(f_s32 IncludePathIndex = 0; IncludePathIndex < IncludePaths.Count; IncludePathIndex++)
					{
						f_str8 PrependedPathToInclude = F_Str8PushF(Arena, "%.*s/%.*s", F_Expand(IncludePaths.Items[IncludePathIndex]), F_Expand(PathToInclude));
						Info = F_OS_FileInfoFromFilename(Arena, PrependedPathToInclude);
						if(Info.Exists)
						{
							PathToInclude = PrependedPathToInclude;
							break;
						}
					}
					f_b32 IncludeSeenAlready = F_False;
					for(f_str8_list_node *Node = IncludesSeenAlready.First; Node; Node = Node->Next)
					{
						if(F_Str8Equal(Node->String, PathToInclude)) IncludeSeenAlready = F_True;
					}
					if(Info.Exists && !IncludeSeenAlready)
					{
						// This isn't perfect but it's actually find since even though the order of hashing with xor doesn't now matter,
						// it's never an issue with includes.
						F_Str8ListAppend(Arena, &IncludesSeenAlready, PathToInclude);
						SourceAndIncludesHash ^= __F_Internal_HashSourceAndIncludes(Arena, PathToInclude, IncludePaths, IncludesSeenAlready);
					}
				}
			} break;
			case '/':
			{
				Advance(I);
				f_u64 CommentStack = 0;
				if(CharAt(I) == '/')
				{
					Advance(I);
					SkipUntilMatch(I, '\n');
				}
				else if(CharAt(I) == '*')
				{
					Advance(I);
					SkipUntilMatch2(I, '*', '/');
					AdvanceBy(I, 2);
				}
			} break;
		}
	}
	return SourceAndIncludesHash;
#undef CharAt
#undef Advance
#undef AdvanceBy
#undef SkipWhiteSpace
#undef SkipUntilMatch
#undef SkipUntilMatch2
}

f_u64 F_HashSourceAndIncludes(f_memory_arena *Arena, f_str8 PathToSource, f_str8_array IncludePaths)
{
	f_str8_list IncludesSeenAlready = {0};
	return __F_Internal_HashSourceAndIncludes(Arena, PathToSource, IncludePaths, IncludesSeenAlready);
}

f_compilation F_CompileTU(f_memory_arena *Arena, f_str8 PathToSourceFile, f_compile_options CompileOptions, f_toolchain Toolchain)
{
	f_compilation Compilation = {0};
	if(CompileOptions.OutputPath.Size > 0)
	{
		Compilation.ObjectPath = F_Str8PushF(Arena, "%.*s/%.*s", F_Expand(CompileOptions.OutputPath), F_Expand(CompileOptions.OutputName));	}
	else
	{
		Compilation.ObjectPath = F_Str8PushF(Arena, "%.*s", F_Expand(CompileOptions.OutputName));
	}

	f_file SourceFile = F_OS_FileFromFilename(Arena, PathToSourceFile);
	f_str8 Source = F_OS_ReadEntireFile(Arena, &SourceFile);
	F_OS_ReleaseFile(&SourceFile);


	Compilation.SourcePath = PathToSourceFile;
	Compilation.Hash = F_HashStr8(Source);

	f_str8_list Args = {0};
	F_Str8ListAppend(Arena, &Args, Toolchain.CompilerPath);
	F_Str8ListAppend(Arena, &Args, Toolchain.CompileOnlyFlag);
	F_Str8ListAppend(Arena, &Args, PathToSourceFile);

	F_Str8ListAppendF(Arena, &Args, "%.*s%.*s", F_Expand(Toolchain.OutputFlag), F_Expand(Compilation.ObjectPath));

	for(int I = 0; I < CompileOptions.IncludePaths.Count; I++)
	{
		F_Str8ListAppendF(Arena, &Args, "%.*s%.*s", F_Expand(Toolchain.IncludeFlag), F_Expand(CompileOptions.IncludePaths.Items[I]));
	}

	F_StartProcess(Arena, &Compilation.Process, Args);

	return Compilation;
}

f_link F_LinkTUs(f_memory_arena *Arena, f_str8_array ObjectFiles, f_link_options LinkOptions, f_toolchain Toolchain)
{
	f_link Link = {0};
	if(LinkOptions.OutputPath.Size > 0)
	{
		Link.OutputPath = F_Str8PushF(Arena, "%.*s%.*s/%.*s", F_Expand(Toolchain.OutputFlag), F_Expand(LinkOptions.OutputPath), F_Expand(LinkOptions.OutputName));
	}
	else
	{
		Link.OutputPath = F_Str8PushF(Arena, "%.*s%.*s", F_Expand(Toolchain.OutputFlag), F_Expand(LinkOptions.OutputName));
	}

	f_str8_list Args = {0};

	F_Str8ListAppend(Arena, &Args, Toolchain.CompilerPath);

	for(int I = 0; I < LinkOptions.ExtraFlags.Count; I++)
	{
		F_Str8ListAppend(Arena, &Args, LinkOptions.ExtraFlags.Items[I]);
	}

	if(LinkOptions.OutputKind == F_OUTPUT_DYNAMIC)
	{
		F_Str8ListAppend(Arena, &Args, F_Str8PushF(Arena, "%.*s", F_Expand(Toolchain.DynamicFlag)));
	}
	else if(LinkOptions.OutputKind == F_OUTPUT_STATIC)
	{
		F_Str8ListAppend(Arena, &Args, F_Str8PushF(Arena, "%.*s", F_Expand(Toolchain.StaticFlag)));
	}

	F_Str8ListAppend(Arena, &Args, F_Str8PushF(Arena, "%.*s%.*s%.*s", F_Expand(Toolchain.OutputFlag), F_Expand(LinkOptions.OutputPath), F_Expand(LinkOptions.OutputName)));
	
	for(int I = 0; I < ObjectFiles.Count; I++)
	{
		F_Str8ListAppend(Arena, &Args, ObjectFiles.Items[I]);
	}

	F_StartProcess(Arena, &Link.Process, Args);

	return Link;
}

static char *__FailedASCII[] = {
" ______   ______     __     __         ______     _____",
"/\\  ___\\ /\\  __ \\   /\\ \\   /\\ \\       /\\  ___\\   /\\  __-.",
"\\ \\  __\\ \\ \\  __ \\  \\ \\ \\  \\ \\ \\____  \\ \\  __\\   \\ \\ \\/\\ \\",
" \\ \\_\\    \\ \\_\\ \\_\\  \\ \\_\\  \\ \\_____\\  \\ \\_____\\  \\ \\____-",
"  \\/_/     \\/_/\\/_/   \\/_/   \\/_____/   \\/_____/   \\/____/",
};

static char *__PassedASCII[] = {
" ______   ______     ______     ______     ______     _____",
"/\\  == \\ /\\  __ \\   /\\  ___\\   /\\  ___\\   /\\  ___\\   /\\  __-.",
"\\ \\  _-/ \\ \\  __ \\  \\ \\___  \\  \\ \\___  \\  \\ \\  __\\   \\ \\ \\/\\ \\",
" \\ \\_\\    \\ \\_\\ \\_\\  \\/\\_____\\  \\/\\_____\\  \\ \\_____\\  \\ \\____-",
"  \\/_/     \\/_/\\/_/   \\/_____/   \\/_____/   \\/_____/   \\/____/",
};

void F_FailedBanner(f_str8_array Stats)
{
	for(int I = 0; I < 5; I++)
	{
		f_str8 Stat = F_Str8("");
		if(I < Stats.Count)
		{
			Stat = Stats.Items[I];
		}
		printf(F_RED "%-64s" F_RESET F_DIM "\u2502" F_RESET "  %.*s\n", __FailedASCII[I],  F_Expand(Stat));
	}
}

void F_PassedBanner(f_str8_array Stats)
{
	for(int I = 0; I < 5; I++)
	{
		f_str8 Stat = F_Str8("");
		if(I < Stats.Count)
		{
			Stat = Stats.Items[I];
		}
		printf(F_GREEN "%-64s" F_RESET F_DIM "\u2502" F_RESET "  %.*s\n", __PassedASCII[I],  F_Expand(Stat));
	}
}


#endif