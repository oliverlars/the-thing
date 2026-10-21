#if 0 /*
import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv) == 2:
    source = Path(sys.argv[1]).resolve()
    output = source.with_suffix('.exe' if os.name == 'nt' else '')
    command = None
    if os.name == 'nt':
        command = ['cl', '/std:c11', '/experimental:c11atomics', '/O2', 'utf-8', '/TC', str(source), '/Fe:', str(output), '/link', 'Synchronization.lib']
    else:
        command = ['clang', '-x', 'c', '-O0', '-g', '-fobjc-arc', '-std=c11', str(source), '-framework', 'Cocoa', '-o', str(output)]
    subprocess.call(command)
else:
    print("Usage: python3 friday2.h <your implementation>.c")
r'''
*/
#endif

#ifndef FRIDAY_H
#define FRIDAY_H

#define FRIDAY_VERSION "2"

#if defined(_WIN32)
#define OS_WINDOWS
// cl /std:c11 /experimental:c11atomics /Z7 /Od /utf-8 /Tcfriday2.c /Fe:friday2.exe /link /DEBUG Synchronization.lib
#endif
#if defined(__APPLE__)
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

 
typedef struct f_string_list_node
{
	struct f_string_list_node *Next;
	struct f_string_list_node *Previous;
	char *String;
}
f_string_list_node;

typedef struct f_string_list
{
	f_string_list_node *First;
	f_string_list_node *Last;
}
f_string_list;

typedef struct f_string_array
{
	char **Items;
	f_s64   Count;
}
f_string_array;

typedef struct f_barrier
{
	_Atomic(f_u64) Generation;
	_Atomic(f_u64) LanesWaiting;
	f_s64          LaneCount;
}
f_barrier;

typedef struct f_lane
{
	// Each lane has one of these.
	// Maybe a better idea for this
	f_memory_arena *PermanentArena;
	f_memory_arena *TemporaryArena;
}
f_lane;

typedef struct f_group
{
	f_b32           ConditionToBeInGroup;
	f_s32           ID;
	f_s32           Count;
	f_barrier      *Barrier;
	void          **BroadcastPointers;
	_Atomic(f_u64) *Vote;
	_Atomic(f_u64) *Ballot;
}
f_group;

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

typedef struct f_dll
{
	f_u64 Handle;
}
f_dll;

typedef struct f_process
{
	f_u64 ID;
	f_process_state State; 
	f_exit_code ExitCode;
	void *StdOut;
	char *StdOutSinceLastPoll;
}
f_process;

// TODO: Set up proper archiver flags since 
// msvc needs to use lib.exe to combine object files
// into static library
typedef struct f_toolchain
{
	char *CompilerPath;
	char *LinkerPath;

	char *IncludeFlag;
	char *OutputFlag;
	char *DebugFlag;
	char *OptimiseFlag;
	char *DynamicFlag;
	char *StaticFlag;
	char *CompileOnlyFlag;
	char *LinkOnlyFlag;
}
f_toolchain;

#define F_TOOLCHAIN_CLANG ((f_toolchain)\
{\
	.CompilerPath = "clang",\
	.OutputFlag = "-o",\
	.DebugFlag = "-g",\
	.OptimiseFlag = "-O2",\
	.IncludeFlag = "-I",\
	.DynamicFlag = "-dynamiclib",\
	.StaticFlag = "-emit-static-lib",\
	.CompileOnlyFlag = "-c"\
})

#define F_TOOLCHAIN_MSVC ((f_toolchain)\
{\
	.CompilerPath = "cl",\
	.OutputFlag = "/OUT",\
	.DebugFlag = "/Zi",\
	.OptimiseFlag = "/O2",\
	.IncludeFlag = "/I",\
	.DynamicFlag = "/LD",\
	.StaticFlag = "/c /MT",\
	.CompileOnlyFlag = "/c"\
})

f_toolchain F_TOOLCHAIN_GCC;
f_toolchain F_TOOLCHAIN_CARNAGE;


typedef enum f_output_kind
{
	F_OUTPUT_EXECUTABLE,
	F_OUTPUT_DYNAMIC,
	F_OUTPUT_STATIC,
}
f_output_kind;

typedef struct f_build_cache
{
	f_u64 *Objects;
	f_b32 *NeededObjects;
	f_s64  Count;
}
f_build_cache;

typedef struct f_build_report
{
	f_b32  Succeeded;
	f_b32  Changed;
	char *OutputPath;
	char *Diagnostics;
	f_s64  TUsCompiled;
	f_f32  CompiledTimeInMilliseconds;
	f_f32  LinkedTimeInMilliseconds;
}
f_build_report;

typedef struct f_build
{
	_Atomic(f_u64) TotalSuccessfulBuilds;
	f_u64 LastUpdated;
	void (*Update)(void *);
	f_dll UpdateDLL;
}
f_build;

#define F_STRING_OPTION_CAPACITY (64)

typedef struct f_build_options
{
	f_output_kind  Kind;

	char          *OutputPath;
	char          *OutputName;

	char          *SourceDirectories[F_STRING_OPTION_CAPACITY];
	char          *IncludeDirectories[F_STRING_OPTION_CAPACITY];
	char          *EmbedDirectories[F_STRING_OPTION_CAPACITY];

	char          *CompileFlags[F_STRING_OPTION_CAPACITY];
	char          *LinkFlags[F_STRING_OPTION_CAPACITY];

	f_b32          Debug;
	f_b32          Optimise;
	char          *Defines[F_STRING_OPTION_CAPACITY];
	char          *Libraries[F_STRING_OPTION_CAPACITY];
	char          *Frameworks[F_STRING_OPTION_CAPACITY];

	char          *CompilerSpecificFlags[F_STRING_OPTION_CAPACITY];

	f_toolchain    Toolchain;
}
f_build_options;

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

typedef struct f_file
{
	// In the future this can set more granular error bits
	f_u64 Errors;
	f_string_list ErrorStream;
	f_u64 Handle;
}
f_file;

typedef struct f_thread_entry_params
{
	void (*LaneEntry)();
	f_lane Lane;
	f_group Group;
}
f_thread_entry_params;

#define F_ArenaPushType(Arena, Type) (Type*)F_Raw_ArenaPush(Arena, sizeof(Type), F_True)
#define F_ArenaPushArrayType(Arena, Count, Type) (Type*)F_Raw_ArenaPush(Arena, (Count) * (sizeof(Type)), F_True)
#define F_ArenaPush(Arena, BytesToPush) F_Raw_ArenaPush(Arena, BytesToPush, F_True)

#define F_ArenaPushTypeNoZero(Arena, Type) (Type*)F_Raw_ArenaPush(Arena, sizeof(Type), F_False)
#define F_ArenaPushArrayTypeNoZero(Arena, Count, Type) (Type*)F_Raw_ArenaPush(Arena, (Count) * (sizeof(Type)), F_False)
#define F_ArenaPushNoZero(Arena, BytesToPush) F_Raw_ArenaPush(Arena, BytesToPush, F_False)

f_memory_arena *F_ArenaReserveButCommitFirstPage(f_s64 NumberOfBytesToReserve);
f_u8 *F_Raw_ArenaPush(f_memory_arena *Arena, f_s64 BytesToPush, f_b32 ShouldZero);
void  F_ArenaReset(f_memory_arena *Arena);

void  F_ArenaResetTo(f_memory_arena *Arena, f_s64 Mark);

#define F_ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


#define F_StringListAppendF(Arena, List, ...) \
F_StringListAppend(Arena, List, F_StringPushF(Arena, __VA_ARGS__))


char *F_StringCopy(f_memory_arena *Arena, char *String);
f_s64 F_StringLength(char *String);
char *      F_StringPushF          (f_memory_arena *Arena, char *Format, ...);
void         F_StringListAppend     (f_memory_arena *Arena, f_string_list *List, char *String);
void         F_StringListAppendMany (f_memory_arena *Arena, f_string_list *Head, f_string_list Tail);
f_string_array F_StringArrayFromList  (f_memory_arena *Arena, f_string_list List);
char *      F_StringJoinList       (f_memory_arena *Arena, f_string_list List,   char *Separator);
char *      F_StringJoinArray      (f_memory_arena *Arena, f_string_array Array, char *Separator);
f_b32        F_StringEqual          (char *A, char *B);
f_b32        F_StringEndsWith       (char *S, char *Suffix);
f_s64        F_StringListCount      (f_string_list List);
f_s64        F_StringListLength     (f_string_list List);


#define F_AppendErrorF(ErrorList, ...) F_StringListAppend(&F_ErrorArena, ErrorList, F_StringPushF(&F_ErrorArena, __VA_ARGS__))

void F_DispatchAcrossNLanes (f_s64 LaneCount, char **Args, int ArgCount);

f_group F_GroupFromPartition (f_group ParentGroup, f_b32 Condition);

f_b32   F_IsInGroup (f_group Group);

void            F_GroupBarrier (f_group Group);

f_u64           F_GroupInclusivePrefixSum (f_group Group, f_u64 Value);
f_u64           F_GroupExclusivePrefixSum (f_group Group, f_u64 Value);

f_u64           F_GroupSum (f_group Group, f_u64 Value);

#define         F_GroupAnyTrue(Group, Value) F_Internal_GroupAnyTrue(Group, Value != 0)
#define         F_GroupAllTrue(Group, Value) F_Internal_GroupAllTrue(Group, Value != 0)
f_b32           F_Internal_GroupAnyTrue   (f_group Group, f_b32 Condition);
f_b32           F_Internal_GroupAllTrue   (f_group Group, f_b32 Condition);
#define         F_GroupCountTrue(Group, Value) F_Internal_GroupCountTrue(Group, Value != 0)
f_u64           F_Internal_GroupCountTrue (f_group Group, f_b32 Condition);

// One bit per lane, set if that lane's Value was true. TODO: > 64 lanes
#define         F_GroupBallot(Group, Value) F_Internal_GroupBallot(Group, Value != 0)
f_u64           F_Internal_GroupBallot    (f_group Group, f_b32 Condition);
f_u64           F_CountBits               (f_u64 Bits);

typedef struct f_range
{
	f_s64 Start;
	f_s64 End;
}
f_range;

f_range F_SplitRange(f_group Group, f_u64 Value);

void *F_Internal_ReadFromGroupID(f_group Group, f_s32 ID, void *Variable, f_u64 SizeOfVariable);

#define F_ReadFromGroupID(Group, ID, Variable) \
										*((__typeof__(Variable)*)(F_Internal_ReadFromGroupID(Group, ID, &Variable, sizeof(Variable))))


void F_OS_WaitForNMilliseconds (f_group Group, f_u64 Milliseconds);

void  F_StartProcess             (f_memory_arena *Arena, f_process *Process, f_string_list Args);
f_b32 F_PollProcess              (f_process *Process);
void  F_WaitForProcess           (f_process *Process);
void  F_WaitForMultipleProcesses (f_process *Processes, f_s32 ProcessCount);

f_b32 F_ProcessFailed(f_process *Process);
f_b32 F_AnyProcessesFailed(f_process *Processes, f_s32 ProcessCount);

void F_Internal_WaitForMultipleProcesses (f_process **Processes, f_u64 ProcessCount);

#define F_Exit() exit(0);

#define F_DEFAULT_TIMEOUT_MS (16)


#define F_HashStart (0xCBF29CE484222325ULL)

f_u64 F_HashBytes(f_u64 HashStart, void *Data, f_u64 NumberOfBytesToHash);

#define F_Hash(HashStart, Variable) F_HashBytes(HashStart, &(Variable), sizeof Variable)

f_u64 F_HashString(char *String);

f_u64 F_HashSourceAndIncludes (f_memory_arena *Arena, f_file File, char **IncludePaths);

f_u8 *F_OS_Reserve   (f_s64 NumberOfBytes);
void  F_OS_Commit    (void *Address, f_s64 BytesToCommit);
f_s64 F_OS_PageSize  (void);

f_u64 F_OS_TimeStamp (void);
f_f32 F_MilliSecondsFromTimeStamp (f_u64 TimeStamp);
f_f32 F_SecondsFromTimeStamp (f_u64 TimeStamp);

typedef struct f_internal_file_options
{
	// Create the file if missing, or clear its contents if it exists.
	f_b32 Overwrite;
}
f_internal_file_options;

#define F_OS_FileFromFilename(Arena, Filename, ...) \
F_OS_Internal_FileFromFilename(Arena, Filename, (f_internal_file_options){ __VA_ARGS__ })

f_file F_OS_Internal_FileFromFilename (f_memory_arena *Arena, char *Filename, f_internal_file_options Options);
void   F_OS_ReleaseFile      (f_file *File);

typedef struct f_buffer
{
	u8 *Bytes;
	f_s64 NumberOfBytes;
}
f_buffer;

#define F_DIRECTORY_CAPACITY (16)

f_file_info  F_OS_FileInfoFromFilename          (f_memory_arena *Arena, char *Filename);
f_file_info  F_OS_FileInfoFromFile              (f_file *File);
f_buffer     F_OS_ReadEntireFile                (f_memory_arena *Arena, f_file *File);
f_buffer     F_OS_ReadEntireFileFromPath        (f_memory_arena *Arena, char *PathToFile);
void         F_OS_WriteBytesToFile              (f_file *File, u8 *BytesToWrite, f_u64 NumberOfBytes);
void         F_OS_WriteBytesToPath              (f_memory_arena *Arena, char *PathToFile, u8 *BytesToWrite, f_u64 NumberOfBytes);
f_b32        F_OS_DeleteFile                    (f_memory_arena *Arena, char *PathToFile);
void         F_OS_RenameFileTo                  (f_memory_arena *Arena, char *From, char *To);
void         F_OS_MakeDirectoryIfNotAlreadyMade (f_memory_arena *Arena, char *Path);

f_dll  F_OS_DLLFromPath (char *Path);
void *F_OS_DLLSymbolFromName (f_dll DLL, char *ProcedureName);

typedef struct f_internal_directory_options
{
	char *Directories[F_DIRECTORY_CAPACITY];
	f_b32 ExcludeDirectories;
	f_b32 PrependPath;
	char *FilterByExtension[F_DIRECTORY_CAPACITY];
}
f_internal_directory_options;

char *F_FilenameAndExtensionFromPath(f_memory_arena *Arena, char *Path);
char *F_FilenameNoExtensionFromPath(f_memory_arena *Arena, char *Path);
char *F_PathWithNoFilename(f_memory_arena *Arena, char *Path);

#define F_OS_ListOfEntriesInDirectory(Arena, DirectoryPath, ...) \
F_OS_Internal_ListOfEntriesInDirectories(Arena, (f_internal_directory_options){ .Directories = {DirectoryPath},__VA_ARGS__ })

#define F_OS_ArrayOfEntriesInDirectory(Arena, DirectoryPath, ...) \
F_StringArrayFromList(Arena, F_OS_Internal_ListOfEntriesInDirectories(Arena,  (f_internal_directory_options){ .Directories = {DirectoryPath},__VA_ARGS__ }))

#define F_OS_ListOfFilesInDirectory(Arena, DirectoryPath, ...) \
F_OS_Internal_ListOfEntriesInDirectories(Arena, (f_internal_directory_options){ .Directories = {DirectoryPath}, .ExcludeDirectories = F_True, __VA_ARGS__ })

#define F_OS_ArrayOfFilesInDirectory(Arena, DirectoryPath, ...) \
F_StringArrayFromList(Arena, F_OS_ListOfEntriesInDirectories(Arena, .Directories = {DirectoryPath}, .ExcludeDirectories = F_True, __VA_ARGS__))


#define F_OS_ListOfEntriesInDirectories(Arena, ...) \
F_OS_Internal_ListOfEntriesInDirectories(Arena, (f_internal_directory_options){ __VA_ARGS__ })

#define F_OS_ArrayOfEntriesInDirectories(Arena, ...) \
F_StringArrayFromList(Arena, F_OS_ListOfEntriesInDirectories(Arena, __VA_ARGS__))

#define F_OS_ListOfFilesInDirectories(Arena, ...) \
F_OS_Internal_ListOfEntriesInDirectories(Arena, (f_internal_directory_options){ .ExcludeDirectories = F_True, __VA_ARGS__ })

#define F_OS_ArrayOfFilesInDirectories(Arena, ...) \
F_StringArrayFromList(Arena, F_OS_ListOfEntriesInDirectories(Arena, .ExcludeDirectories = F_True, __VA_ARGS__))


f_string_list F_OS_Internal_ListOfEntriesInDirectories(f_memory_arena *Arena, f_internal_directory_options Options);

#if defined(OS_WINDOWS)
typedef unsigned long (__stdcall *f_thread_function)(void *);
#define F_Internal_ThreadEntry(Argument) unsigned long __stdcall F_Internal_ThreadEntry(f_thread_entry_params *Argument)
#elif defined(OS_DARWIN)
typedef void *(*f_thread_function)(void *);
#define F_Internal_ThreadEntry(Argument) void *F_Internal_ThreadEntry(f_thread_entry_params *Argument)
#endif

void  F_OS_ThreadStart         (f_thread *Thread, f_thread_function ThreadEntry, void *EntryArgument);
void  F_OS_WaitForSingleThread (f_thread Thread);
void  F_OS_WaitForThreads      (f_thread *Thread, f_s32 NumberOfThreadsToWaitFor);
void  F_OS_FutexWait    (void *Address, f_u64 Expected);
void  F_OS_FutexWakeAll (void *Address);


void F_EmitDataAsObjectFile(char *ObjectFileName, char *SymbolName, u8 *Data, f_u64 NumberOfBytes);

void F_Print(char *String);
#define F_PrintF(...) \
F_Print(F_StringPushF(F_TArena(), __VA_ARGS__))

f_file F_OS_StandardOut(void);

f_s64 F_OS_NumberOFHighPerformanceCPUs (void);

#define F_LaneEntry void F_Internal_LaneEntry()

void F_Internal_LaneEntry();


f_build *F_BuildFromGroup (f_memory_arena *Arena, f_group Group);
void     F_Build    (f_group Group, f_build *Build, f_build_options *BuildOptions);
void     F_Update   (f_group Group, f_build *Build, void *User);
f_b32    F_NeedsReloading (f_build *Build);



f_build_cache F_BuildCacheFromOutputPath (f_group Group, f_memory_arena *Arena, char *OutputPath);
f_b32         F_HashIsInBuildCache (f_build_cache Cache, f_u64 Hash);

void F_Report(f_memory_arena *Arena, f_build_report Report);

f_memory_arena *F_PArena();
f_memory_arena *F_TArena();
void F_ResetTArena();
f_group F_DefaultGroup();

#endif

#ifdef FRIDAY_IMPLEMENTATION

#ifdef FRIDAY_MAIN

int main(int ArgCount, char **Arguments)
{
	F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs(), Arguments, ArgCount);
}

#endif

#if defined(OS_WINDOWS)
#define thread_local __declspec(thread)
#else
#define thread_local __thread
#endif

// TODO: Prefix with 'F_'
thread_local f_lane  F_Internal_Lane;
thread_local f_group F_Internal_DefaultGroup;
f_string_array F_CommandLineArgs;

thread_local f_memory_arena F_ErrorArena;

#include <stdatomic.h> // atomic_fetch_add, atomic_store, atomic_load, atomic_memory_fence ...
#include <stdio.h> // printf()
#include <stdlib.h> // exit()
#include <assert.h> // assert()
#include <stdarg.h> // va_list, va_args etc
#include <string.h> // memset

#if defined(OS_WINDOWS)
#include <windows.h>
#elif defined(OS_DARWIN)
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
#include <dlfcn.h> // dlopen
#include <mach/machine.h>
#include <mach/vm_prot.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#endif

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
#define F_Yield() YieldProcessor()
#elif defined(__i386__) || defined(__x86_64__)
#define F_Yield() __builtin_ia32_pause()
#elif defined(__aarch64__)
#define F_Yield() __builtin_arm_isb(15)
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
	VirtualAlloc((void*)(Address), BytesToCommit, MEM_COMMIT, PAGE_READWRITE);
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
	LARGE_INTEGER Stamp = {0};
	QueryPerformanceCounter(&Stamp);
	Result = Stamp.QuadPart;
#elif defined(OS_DARWIN)
	Result = mach_absolute_time();
#endif
	return Result;
}

f_f32 F_MilliSecondsFromTimeStamp(f_u64 TimeStamp)
{
	f_f32 MilliSeconds;
#if defined(OS_WINDOWS)
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	f_u64 TimeStampInMilliseconds = TimeStamp * 1000 / Frequency.QuadPart;
	MilliSeconds = TimeStampInMilliseconds;
#elif defined(OS_DARWIN)
	mach_timebase_info_data_t Info;
	mach_timebase_info(&Info);

	MilliSeconds = (f_f32)((f_f32)(TimeStamp) * Info.numer / Info.denom) / 1000000.0;
#else
#error "Unsupported OS"
#endif
	return MilliSeconds;
}

f_f32 F_SecondsFromTimeStamp(f_u64 TimeStamp)
{
	return F_MilliSecondsFromTimeStamp(TimeStamp) * 1000.0;
}

f_file F_OS_StandardOut(void)
{
	f_file File = {0};
#if defined(OS_WINDOWS)
	File.Handle = (f_u64)GetStdHandle(STD_OUTPUT_HANDLE);
#elif defined(OS_DARWIN)
	File.Handle = STDOUT_FILENO;
#else
#error "Unsupported OS"
#endif
	return File;
}

void F_Print(char *String)
{
	f_file StdOut = F_OS_StandardOut();
	F_OS_WriteBytesToFile(&StdOut, (u8*)String, F_StringLength(String));
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
	DWORD ThreadId;
	Thread->Handle = (f_u64)CreateThread(0, 0, ThreadEntry, EntryArgument, 0, (LPDWORD)&ThreadId);
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

void F_OS_WaitForThreads(f_thread *Threads, f_s32 NumberOfThreadsToWaitFor)
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
	os_sync_wake_by_address_all(Address, sizeof(f_u64), OS_SYNC_WAKE_BY_ADDRESS_NONE);
#else
#error "Unsupported OS"
#endif
}


void F_EmitDataAsObjectFile(char *ObjectFileName, char *SymbolName, u8 *Data, f_u64 NumberOfBytes)
{
	f_file File = F_OS_FileFromFilename(F_TArena(), ObjectFileName, .Overwrite = F_True);
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)

	SymbolName = F_StringPushF(F_TArena(), "_%s", SymbolName);

	f_u64 CommandSize = sizeof(struct segment_command_64) + sizeof(struct section_64) + sizeof(struct symtab_command);
	f_u64 DataOffset = sizeof(struct mach_header_64) + CommandSize;
	f_u64 SymbolOffset = (DataOffset + (f_u64)NumberOfBytes + 7) & ~UINT64_C(7);
	f_u64 StringOffset = SymbolOffset + sizeof(struct nlist_64);
	f_u64 StringSize = (f_u64)F_StringLength(SymbolName) + 2;

	struct mach_header_64 Header = {0};
	Header.magic = MH_MAGIC_64;
	Header.cputype = CPU_TYPE_ARM64;
	Header.cpusubtype = CPU_SUBTYPE_ARM64_ALL;
	Header.filetype = MH_OBJECT;
	Header.ncmds = 2;
	Header.sizeofcmds = (f_u32)CommandSize;


	struct segment_command_64 Segment = {0};
	Segment.cmd = LC_SEGMENT_64;
	Segment.cmdsize = sizeof(Segment) + sizeof(struct section_64);
	Segment.vmsize = NumberOfBytes;
	Segment.fileoff = DataOffset;
	Segment.filesize = NumberOfBytes;
	Segment.maxprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
	Segment.initprot = VM_PROT_READ | VM_PROT_WRITE;
	Segment.nsects = 1;


	struct section_64 Section = {0};
	F_MemoryCopy(Section.sectname, "__data", sizeof("__data"));
	F_MemoryCopy(Section.segname, "__DATA", sizeof("__DATA"));
	Section.size = NumberOfBytes;
	Section.offset = (uint32_t)DataOffset;
	Section.align = 0;
	Section.flags = S_REGULAR;


	struct symtab_command SymTab = {0};
	SymTab.cmd = LC_SYMTAB;
	SymTab.cmdsize = sizeof(SymTab);
	SymTab.symoff = (uint32_t)SymbolOffset;
	SymTab.nsyms = 1;
	SymTab.stroff = (uint32_t)StringOffset;
	SymTab.strsize = (uint32_t)StringSize;

	struct nlist_64 Symbol = {0};
	Symbol.n_un.n_strx = 1;
	Symbol.n_type = N_SECT | N_EXT;
	Symbol.n_sect = 1;
	Symbol.n_value = Section.addr;

	u8 Zeros[8] = {0};
 f_u64 Padding = (f_u64)(SymbolOffset - (DataOffset + NumberOfBytes));


	f_u64 MachOFileSize = (f_u64)(StringOffset + StringSize); 
	f_u8 *MachOFile = F_ArenaPushArrayType(F_TArena(), MachOFileSize, f_u8);

	f_u64 Cursor = 0;
	F_MemoryCopy(MachOFile + Cursor, &Header, sizeof Header);
	Cursor += sizeof Header;
	F_MemoryCopy(MachOFile + Cursor, &Segment, sizeof Segment);
	Cursor += sizeof Segment;
	F_MemoryCopy(MachOFile + Cursor, &Section, sizeof Section);
	Cursor += sizeof Section;
	F_MemoryCopy(MachOFile + Cursor, &SymTab, sizeof SymTab);
	Cursor += sizeof SymTab;
	F_MemoryCopy(MachOFile + Cursor, Data, NumberOfBytes);
	Cursor += NumberOfBytes;
	F_MemoryCopy(MachOFile + Cursor, Zeros, Padding);
	Cursor += Padding;
	F_MemoryCopy(MachOFile + Cursor, &Symbol, sizeof Symbol);
	Cursor += sizeof Symbol;
	F_MemoryCopy(MachOFile + Cursor, Zeros, 1);
	Cursor += 1;
	F_MemoryCopy(MachOFile + Cursor, SymbolName, F_StringLength(SymbolName) + 1);
	Cursor += F_StringLength(SymbolName) + 1;

	F_OS_WriteBytesToFile(&File, MachOFile, MachOFileSize);

#else
#error "Unsupported"
#endif
}

f_memory_arena *F_PArena()
{
	return F_Internal_Lane.PermanentArena;
}

f_memory_arena *F_TArena()
{
	return F_Internal_Lane.TemporaryArena;
}

void F_ResetTArena()
{
	F_ArenaReset(F_TArena());
}

f_group F_DefaultGroup()
{
	return F_Internal_DefaultGroup;
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
	if(!Arena->Start) 
	{
		f_memory_arena *NewArena = F_ArenaReserveButCommitFirstPage(F_MegaBytes(32));
		Arena[0] = NewArena[0];
	}
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

f_u64 F_Internal_GroupCountTrue(f_group Group, f_b32 Condition)
{
	if(Group.Count == 1) return Condition != 0;
	if(Group.ID == 0) atomic_store_explicit(Group.Vote, 0, memory_order_relaxed);
	F_GroupBarrier(Group);
	atomic_fetch_add_explicit(Group.Vote, (Condition != 0), memory_order_relaxed);
	F_GroupBarrier(Group);
	f_u64 Result = atomic_load_explicit(Group.Vote, memory_order_relaxed);
	F_GroupBarrier(Group);
	return Result;
}

f_b32 F_Internal_GroupAnyTrue(f_group Group, f_b32 Condition)
{
	return F_Internal_GroupCountTrue(Group, Condition) > 0;
}

f_b32 F_Internal_GroupAllTrue(f_group Group, f_b32 Condition)
{
	return F_Internal_GroupCountTrue(Group, Condition) == (f_u64)Group.Count;
}

f_u64 F_Internal_GroupBallot(f_group Group, f_b32 Condition)
{
	if(Group.Count == 1) return Condition != 0;
	// Maybe this can be changed?
	F_Assert(Group.Count <= 64);
	if(Group.ID == 0) atomic_store_explicit(Group.Ballot, 0, memory_order_relaxed);
	F_GroupBarrier(Group);
	if(Condition) atomic_fetch_or_explicit(Group.Ballot, 1ull << Group.ID, memory_order_relaxed);
	F_GroupBarrier(Group);
	f_u64 Result = atomic_load_explicit(Group.Ballot, memory_order_relaxed);
	F_GroupBarrier(Group);
	return Result;
}

f_u64 F_CountBits(f_u64 Bits)
{
	f_u64 Result = 0;
	for(; Bits; Bits &= Bits - 1) Result++;
	return Result;
}

f_u64 F_GroupExclusivePrefixSum (f_group Group, f_u64 Value)
{
	if(Group.Count == 1) return 0;
	Group.BroadcastPointers[Group.ID] = &Value;
	F_GroupBarrier(Group);

	f_u64 ValueForThisLane = 0;
	for(f_u64 I = 0; I < Group.ID; I++)
	{
		ValueForThisLane += *(f_u64 *)Group.BroadcastPointers[I];
	}
	F_GroupBarrier(Group);
	return ValueForThisLane;
}

f_u64 F_GroupInclusivePrefixSum (f_group Group, f_u64 Value)
{
	return F_GroupExclusivePrefixSum(Group, Value) + Value;
}

f_u64 F_GroupSum(f_group Group, f_u64 Value)
{
	if(Group.Count == 1) return Value;
	Group.BroadcastPointers[Group.ID] = &Value;
	F_GroupBarrier(Group);
	f_u64 Sum = 0;
	for(f_u64 I = 0; I < Group.Count; I++)
	{
		Sum += *(f_u64 *)Group.BroadcastPointers[I];
	}
	F_GroupBarrier(Group);
	return Sum;
}

void F_GroupBarrier(f_group Group)
{
	f_s64 Generation = atomic_load_explicit(&Group.Barrier->Generation, memory_order_relaxed);
	f_s64 LanesArrived = atomic_fetch_add_explicit(&Group.Barrier->LanesWaiting, 1, memory_order_release);

	if(LanesArrived == Group.Barrier->LaneCount - 1)
	{
		atomic_thread_fence(memory_order_acquire);
		atomic_store_explicit(&Group.Barrier->LanesWaiting, 0, memory_order_relaxed);
		atomic_store_explicit(&Group.Barrier->Generation, Generation + 1, memory_order_release);
		F_OS_FutexWakeAll(&Group.Barrier->Generation);
	}
	else
	{
		f_s64 Spins = 0;
		for(;;)
		{
			if(atomic_load_explicit(&Group.Barrier->Generation, memory_order_acquire) != Generation) break;

			if(Spins < 4096)
			{
				F_Yield();
			}
			else
			{
				F_OS_FutexWait(&Group.Barrier->Generation, Generation);
			}
			Spins++;
		}
	}
}

#if defined(OS_WINDOWS)
F_Internal_ThreadEntry(ThreadParams)
{
	F_Internal_Lane = ThreadParams->Lane;
	F_Internal_DefaultGroup = ThreadParams->Group;
	assert(ThreadParams->Group.Count > 0);
	ThreadParams->LaneEntry();
	return 0;
}
#elif defined(OS_DARWIN)
F_Internal_ThreadEntry(ThreadParams)
{
	F_Internal_Lane = ThreadParams->Lane;
	F_Internal_DefaultGroup = ThreadParams->Group;
	assert(ThreadParams->Group.Count > 0);
	ThreadParams->LaneEntry();
	return F_Null;
}
#else
#error "Unsupported OS"
#endif

f_b32 F_IsInGroup(f_group Group)
{
	return Group.ConditionToBeInGroup;
}

f_group F_GroupFromPartition(f_group ParentGroup, f_b32 Condition)
{
	f_group Group = {0};
	Group.Count = F_GroupSum(ParentGroup, !!Condition);
	if(Group.ID == 0)
	{
		Group.Barrier = F_ArenaPushType(F_PArena(), f_barrier);
		Group.Barrier->LaneCount = Group.Count;
		Group.BroadcastPointers = F_ArenaPushArrayType(F_PArena(), Group.Count, void*);
		Group.Vote = F_ArenaPushType(F_PArena(), _Atomic(f_u64));
		Group.Ballot = F_ArenaPushType(F_PArena(), _Atomic(f_u64));
	}
	Group = F_ReadFromGroupID(ParentGroup, 0, Group);
	// Unique group stuff
	Group.ID = F_GroupExclusivePrefixSum(ParentGroup, !!Condition);
	Group.ConditionToBeInGroup = !!Condition;

	return Group;
}

void F_DispatchAcrossNLanes(f_s64 NLanes, char **Args, f_s32 ArgCount)
{
	f_memory_arena *FirstLaneArena = F_ArenaReserveButCommitFirstPage(F_GigaBytes(8));

	void **BroadcastPointers   = F_ArenaPushArrayType(FirstLaneArena, NLanes, void*);
	f_thread              *Threads = F_ArenaPushArrayType(FirstLaneArena, NLanes, f_thread);
	f_thread_entry_params *Params  = F_ArenaPushArrayType(FirstLaneArena, NLanes, f_thread_entry_params);
	f_barrier Barrier = {0};
	Barrier.LaneCount = NLanes;

	F_CommandLineArgs.Items = F_ArenaPushArrayType(FirstLaneArena, ArgCount, char *);
	F_CommandLineArgs.Count = ArgCount;
	for(f_s32 I = 0; I < ArgCount; I++)
	{
		F_CommandLineArgs.Items[I] = F_StringCopy(FirstLaneArena, Args[I]);
	}

	_Atomic(f_u64) Vote = 0;
	_Atomic(f_u64) Ballot = 0;

	// One lane pinned, the rest of the lanes go to update
	{
		f_group Group = {0};
		Group.Barrier = &Barrier;
		Group.Count = NLanes;
		Group.BroadcastPointers = BroadcastPointers;
		Group.Vote = &Vote;
		Group.Ballot = &Ballot;
		Group.ID = 0;
		Params[0].Lane.TemporaryArena = F_ArenaReserveButCommitFirstPage(F_GigaBytes(1));
		Params[0].Lane.PermanentArena = FirstLaneArena;
		Params[0].Group = Group;
		Params[0].LaneEntry = F_Internal_LaneEntry;
	}

	for(f_s32 I = 1; I < NLanes; I++)
	{
		f_group Group = {0};
		Group.Barrier = &Barrier;
		Group.Count = NLanes;
		Group.BroadcastPointers = BroadcastPointers;
		Group.Vote = &Vote;
		Group.Ballot = &Ballot;
		Group.ID = I;
		Params[I].Lane.TemporaryArena = F_ArenaReserveButCommitFirstPage(F_GigaBytes(1));
		Params[I].Lane.PermanentArena = F_ArenaReserveButCommitFirstPage(F_GigaBytes(8));
		Params[I].Group = Group;
		Params[I].LaneEntry = F_Internal_LaneEntry;
		F_OS_ThreadStart(Threads + I, (f_thread_function)F_Internal_ThreadEntry, Params + I);
	}

	F_Internal_DefaultGroup = Params[0].Group;
	F_Internal_Lane = Params[0].Lane;
	Params[0].LaneEntry();

	F_OS_WaitForThreads(Threads + 1, NLanes - 1);
}

f_s64 F_StringLength(char *String)
{
	if(!String) return 0;
	return (f_s64)strlen(String);
}

char *F_StringCopy(f_memory_arena *Arena, char *String)
{
	f_s64 Length = F_StringLength(String);
	char *Result = F_ArenaPushArrayType(Arena, Length + 1, char);
	if(Length) F_MemoryCopy(Result, String, Length);
	return Result;
}

f_s64 F_StringListCount(f_string_list List)
{
	f_s64 Count = 0;
	for(f_string_list_node *Node = List.First; Node; Node = Node->Next) Count++;
	return Count;
}

f_s64 F_StringListLength(f_string_list List)
{
	f_s64 Length = 0;
	for(f_string_list_node *Node = List.First; Node; Node = Node->Next) Length += F_StringLength(Node->String);
	return Length;
}

char *F_StringPushFV(f_memory_arena *Arena, char *Format, va_list InputArgs)
{
	va_list Args;
	va_copy(Args, InputArgs);
	int Length = vsnprintf(0, 0, Format, Args);
	va_end(Args);
	if(Length < 0) return F_StringCopy(Arena, "");
	char *Result = F_ArenaPushArrayType(Arena, (f_s64)Length + 1, char);
	va_copy(Args, InputArgs);
	vsnprintf(Result, (size_t)Length + 1, Format, Args);
	va_end(Args);
	return Result;
}

char *F_StringPushF(f_memory_arena *Arena, char *Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	char *Result = F_StringPushFV(Arena, Format, Args);
	va_end(Args);
	return Result;
}

void F_StringListAppend(f_memory_arena *Arena, f_string_list *List, char *String)
{
	f_string_list_node *Node = F_ArenaPushType(Arena, f_string_list_node);
	Node->String = String ? String : "";
	Node->Previous = List->Last;
	if(List->Last) List->Last->Next = Node;
	else List->First = Node;
	List->Last = Node;
}

void F_StringListAppendMany(f_memory_arena *Arena, f_string_list *Head, f_string_list Tail)
{
	for(f_string_list_node *Node = Tail.First; Node; Node = Node->Next)
	{
		F_StringListAppend(Arena, Head, Node->String);
		if(Node == Tail.Last) break;
	}
}

f_string_array F_StringArrayFromList(f_memory_arena *Arena, f_string_list List)
{
	f_string_array Result = {0};
	Result.Count = F_StringListCount(List);
	Result.Items = F_ArenaPushArrayType(Arena, Result.Count + 1, char *);
	f_s64 I = 0;
	for(f_string_list_node *Node = List.First; Node; Node = Node->Next) Result.Items[I++] = Node->String;
	return Result;
}

char *F_StringJoinArray(f_memory_arena *Arena, f_string_array Array, char *Separator)
{
	f_s64 SeparatorLength = F_StringLength(Separator);
	f_s64 Length = Array.Count > 0 ? (Array.Count - 1) * SeparatorLength : 0;
	for(f_s64 I = 0; I < Array.Count; I++) Length += F_StringLength(Array.Items[I]);
	char *Result = F_ArenaPushArrayType(Arena, Length + 1, char);
	char *At = Result;
	for(f_s64 I = 0; I < Array.Count; I++)
	{
		f_s64 ItemLength = F_StringLength(Array.Items[I]);
		if(ItemLength) F_MemoryCopy(At, Array.Items[I], ItemLength);
		At += ItemLength;
		if(I + 1 < Array.Count && SeparatorLength)
		{
			F_MemoryCopy(At, Separator, SeparatorLength);
			At += SeparatorLength;
		}
	}
	return Result;
}

char *F_StringJoinList(f_memory_arena *Arena, f_string_list List, char *Separator)
{
	return F_StringJoinArray(Arena, F_StringArrayFromList(Arena, List), Separator);
}

f_b32 F_StringEqual(char *A, char *B)
{
	return strcmp(A ? A : "", B ? B : "") == 0;
}

f_b32 F_StringEndsWith(char *String, char *Suffix)
{
	f_s64 Length = F_StringLength(String);
	f_s64 SuffixLength = F_StringLength(Suffix);
	return Length >= SuffixLength && F_StringEqual((String ? String : "") + Length - SuffixLength, Suffix);
}

f_file_info F_OS_FileInfoFromFile(f_file *File)
{
	f_file_info FileInfo = {0};
	if(File->Errors) return FileInfo;
#if defined(OS_WINDOWS)
	BY_HANDLE_FILE_INFORMATION Win32FileInfo = {0};
	GetFileInformationByHandle((HANDLE)File->Handle, &Win32FileInfo);
	FileInfo.Exists = F_True;
	ULARGE_INTEGER LastModified;
	LastModified.HighPart = Win32FileInfo.ftLastWriteTime.dwHighDateTime;
	LastModified.LowPart = Win32FileInfo.ftLastWriteTime.dwLowDateTime;

	FileInfo.LastModifiedInMillisecondsSinceEpoch = (LastModified.QuadPart - 116444736000000000LL) / 10000LL;
	ULARGE_INTEGER SizeInBytes;
	SizeInBytes.HighPart = Win32FileInfo.nFileSizeHigh;
	SizeInBytes.LowPart = Win32FileInfo.nFileSizeLow;
	FileInfo.SizeInBytes = SizeInBytes.QuadPart;
#elif defined(OS_DARWIN)
	struct stat FileStat = {0};
	f_s32 Status = fstat(File->Handle, &FileStat);
	if(Status) 
	{
		File->Errors = F_True;
		F_AppendErrorF(&File->ErrorStream, "Failed to get file information");
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

void F_OS_WriteBytesToFile(f_file *File, u8 *BytesToWrite, f_u64 NumberOfBytes)
{
	if(File->Errors) return;
#if defined(OS_WINDOWS)
	if(NumberOfBytes > 0xFFFFFFFFull)
	{
		File->Errors = F_True;
		return;
	}
	DWORD Written = 0;
	File->Errors = !WriteFile(
		(HANDLE)File->Handle,
		BytesToWrite,
		(DWORD)NumberOfBytes,
		&Written, F_Null
	) || Written != NumberOfBytes;
#elif defined(OS_DARWIN)
	ssize_t Status = write(File->Handle, BytesToWrite, NumberOfBytes);
	File->Errors = Status < 0 || (f_u64)Status != NumberOfBytes;
#else
#error "Unsupported OS"
#endif
}

void F_OS_WriteBytesToPath(f_memory_arena *Arena, char *PathToFile, u8 *BytesToWrite, f_u64 NumberOfBytes)
{
	f_file File = F_OS_FileFromFilename(Arena, PathToFile, .Overwrite = F_True);
	if(File.Errors) return;
	F_OS_WriteBytesToFile(&File, BytesToWrite, NumberOfBytes);
	F_OS_ReleaseFile(&File);
}

f_b32 F_OS_DeleteFile(f_memory_arena *Arena, char *PathToFile)
{
	f_b32 Result = F_True;
#if defined(OS_WINDOWS)
	Result = DeleteFileA(PathToFile) != 0;
#elif defined(OS_DARWIN)
	Result = remove(PathToFile);
#else
#error "Unsupported OS"
#endif
	return Result;
}

f_dll  F_OS_DLLFromPath(char *Path)
{
	f_dll DLL = {0};
#if defined(OS_WINDOWS)
	DLL.Handle = (f_u64)LoadLibraryA(Path);
#elif defined(OS_DARWIN)
	DLL.Handle = (f_u64)dlopen(Path, RTLD_NOW | RTLD_LOCAL);
#else
#endif
	return DLL;
}

void *F_OS_DLLSymbolFromName(f_dll DLL, char *ProcedureName)
{
	void *Procedure;
#if defined(OS_WINDOWS)
	Procedure = GetProcAddress(DLL.Handle, ProcedureName);
#elif defined(OS_DARWIN)
	Procedure = dlsym((void *)DLL.Handle, ProcedureName);
#else
#error "Unsupported OS"
#endif
	return Procedure;
}

void F_OS_RenameFileTo(f_memory_arena *Arena, char *From, char *To)
{
#if defined(OS_WINDOWS)
	MoveFileExA(From, To, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
#elif defined(OS_DARWIN)
	rename(From, To);
#else
#error "Unsupported OS"
#endif
}


f_buffer F_OS_ReadEntireFile(f_memory_arena *Arena, f_file *File)
{
		if(File->Errors) return (f_buffer){(u8*)"", 0};
		f_file_info Info = F_OS_FileInfoFromFile(File);
		char *Backing = F_ArenaPushArrayType(Arena, Info.SizeInBytes + 1, char);
#if defined(OS_WINDOWS)
	File->Errors = !ReadFile(
		(HANDLE)File->Handle,
		Backing,
		Info.SizeInBytes,
		F_Null,
		F_Null
);
#elif defined(OS_DARWIN)
	f_s32 Status = read(File->Handle, Backing, Info.SizeInBytes);
	File->Errors = Status < 0;
#else
#error "Unsupported OS"
#endif
	return (f_buffer){ (u8*)Backing, Info.SizeInBytes };
}

f_b32 F_FileHasErrors(f_file File)
{
	return File.ErrorStream.First || File.Errors;
}

void F_LogError(char *Message)
{
	F_PrintF(F_RED "error" F_RESET ":" "%s\n", Message);
}

void F_LogErrorStream(f_string_list Messages)
{
	for(f_string_list_node *Node = Messages.First; Node; Node = Node->Next)
	{
		F_LogError(Node->String);
	}
}

f_buffer F_OS_ReadEntireFileFromPath(f_memory_arena *Arena, char *PathToFile)
{
	f_file File = F_OS_FileFromFilename(Arena, PathToFile);
	f_buffer Buffer = F_OS_ReadEntireFile(Arena, &File);
	if(F_FileHasErrors(File))
	{
		F_LogErrorStream(File.ErrorStream);
	}
	F_OS_ReleaseFile(&File);
	return Buffer;
}

void F_OS_MakeDirectoryIfNotAlreadyMade(f_memory_arena *Arena, char *Path)
{
#if defined(OS_WINDOWS)
	CreateDirectoryA(Path, F_Null);
#elif defined(OS_DARWIN)
	mkdir(Path, 0777);
#else
#error "Unsupported OS"
#endif
}

f_file_info F_OS_FileInfoFromFilename(f_memory_arena *Arena, char *Filename)
{
	f_file_info FileInfo = {0};
#if defined(OS_WINDOWS)
	f_file File = F_OS_FileFromFilename(Arena, Filename);
	FileInfo = F_OS_FileInfoFromFile(&File);
#elif defined(OS_DARWIN)
	struct stat FileStat = {0};
	if(stat(Filename, &FileStat) == 0)
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

f_file F_OS_Internal_FileFromFilename(f_memory_arena *Arena, char *Filename, f_internal_file_options Options)
{
	f_file File = {0};
#if defined(OS_WINDOWS)
	File.Handle = (f_u64)CreateFileA(
		Filename,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		F_Null,
		Options.Overwrite ? CREATE_ALWAYS : OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		F_Null
	);
	if((HANDLE)File.Handle == INVALID_HANDLE_VALUE)
	{
		File.Handle = 0;
		File.Errors = F_True;
		F_AppendErrorF(&File.ErrorStream, "Could not open file '%s'", Filename);
	}
#elif defined(OS_DARWIN)
	f_s32 Handle = open(
		Filename,
		O_RDWR | (Options.Overwrite ? O_CREAT | O_TRUNC : 0),
		0666
	);

	if(Handle >= 0)
	{
		File.Handle = (f_u64)Handle;
	}
	else
	{
		File.Errors = F_True;
		F_AppendErrorF(&File.ErrorStream,
			"Could not open or create file '%s'", Filename);
	}
#else
#error "Unsupported OS"
#endif
	return File;
}

void F_OS_ReleaseFile(f_file *File)
{
#if defined(OS_WINDOWS)
	CloseHandle((HANDLE)File->Handle);
#elif defined(OS_DARWIN)
	close(File->Handle);
#else
#error "Unsupported OS"
#endif
	File->Handle = 0;
}

void F_UpdateStub(void *User) { (void)User; }

void F_Update(f_group Group, f_build *Build, void *User)
{
	if(!F_IsInGroup(Group)) return;
	Build->Update(User);
}

f_build *F_BuildFromGroup(f_memory_arena *Arena, f_group Group)
{
	f_build *Build = 0;
	if(Group.ID == 0)
	{
		Build = F_ArenaPushType(Arena, f_build);
		Build->Update = F_UpdateStub;
	}
	Build = F_ReadFromGroupID(Group, 0, Build);

	return Build;
}

void F_Build(f_group Group, f_build *Build, f_build_options *BuildOptions)
{
	if(!F_IsInGroup(Group)) return;
	
	typedef struct f_translation_unit
	{
		char *FullRelativePath;
		char *FilenameWithNoExtension;
		f_b32 IsEmbed;
	}
	f_translation_unit;

	f_build_cache BuildCache = F_BuildCacheFromOutputPath(Group, F_TArena(), "build/");

	f_translation_unit *TUs = {0};
	f_u64 TUCount = 0;
	char  **ObjectFiles = 0;
	char  **ObjectFilesToDelete = 0;
	f_b32 CompileStarted = F_False;
	if(Group.ID == 0) 
	{
		f_string_list Sources = {0};
		for(f_s64 I = 0; I < F_DIRECTORY_CAPACITY && BuildOptions->SourceDirectories[I]; I++)
		{
			F_StringListAppendMany(F_TArena(), &Sources, F_OS_ListOfFilesInDirectory(
				F_TArena(), BuildOptions->SourceDirectories[I], .PrependPath = F_True,
				.FilterByExtension = { ".m", ".c" }));
		}

		f_string_list Embeds = {0};
		for(f_s64 I = 0; I < F_DIRECTORY_CAPACITY && BuildOptions->EmbedDirectories[I]; I++)
		{
			F_StringListAppendMany(F_TArena(), &Embeds, F_OS_ListOfFilesInDirectory(F_TArena(), BuildOptions->EmbedDirectories[I], .PrependPath = F_True));
		}
		f_u64 EmbedCount = F_StringListCount(Embeds);
		f_u64 SourcesCount = F_StringListCount(Sources);
		TUCount = SourcesCount + EmbedCount;
		TUs = F_ArenaPushArrayType(F_TArena(), TUCount, f_translation_unit);

		f_u64 TUIndex = 0;
		for(f_string_list_node *Source = Sources.First; Source; Source = Source->Next)
		{
			TUs[TUIndex++] = (f_translation_unit){ .FullRelativePath = Source->String, .FilenameWithNoExtension = F_FilenameNoExtensionFromPath(F_TArena(), Source->String) };
		}


		f_string_list EmbedHeader = {0};
		for(f_string_list_node *Embed = Embeds.First; Embed; Embed = Embed->Next)
		{
			TUs[TUIndex] = (f_translation_unit){ .IsEmbed = F_True, .FullRelativePath = Embed->String, .FilenameWithNoExtension = F_FilenameNoExtensionFromPath(F_TArena(), Embed->String) };
			F_StringListAppend(F_TArena(), &EmbedHeader, F_StringPushF(F_TArena(), "extern unsigned char E_%s[];\n", TUs[TUIndex].FilenameWithNoExtension));
			TUIndex++;
		}

		char *EmbedHeaderString = F_StringJoinList(F_TArena(), EmbedHeader, "");
		F_OS_WriteBytesToPath(F_TArena(), "include/embeds.h", EmbedHeaderString, F_StringLength(EmbedHeaderString));

		ObjectFiles = F_ArenaPushArrayType(F_TArena(), TUCount, char *);
	}
	TUCount = F_ReadFromGroupID(Group, 0, TUCount);
	TUs = F_ReadFromGroupID(Group, 0, TUs);
	ObjectFiles = F_ReadFromGroupID(Group, 0, ObjectFiles);
	f_range TURange = F_SplitRange(Group, TUCount);
	f_process *Processes = F_ArenaPushArrayType(F_TArena(), TURange.End - TURange.Start, f_process);
	for(f_s32 I = TURange.Start; I < TURange.End; I++)
	{
		f_file File = F_OS_FileFromFilename(F_TArena(), TUs[I].FullRelativePath);
		f_file_info FileInfo = F_OS_FileInfoFromFile(&File);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch < Build->LastUpdated)
		{
			F_OS_ReleaseFile(&File);
			continue;
		}
		f_u64 Hash = 0;
		f_buffer EmbedContents = {0};
		if(TUs[I].IsEmbed)
		{
			EmbedContents = F_OS_ReadEntireFile(F_TArena(), &File);
			Hash = F_HashBytes(F_HashStart, EmbedContents.Bytes, EmbedContents.NumberOfBytes);
		}
		else
		{
			Hash = F_HashSourceAndIncludes(F_TArena(), File, BuildOptions->IncludeDirectories);
		}

		for(int I = 0; I < F_STRING_OPTION_CAPACITY && BuildOptions->CompileFlags[I]; I++)
		{
			Hash ^= F_HashString(BuildOptions->CompileFlags[I]);
		}
		for(int I = 0; I < F_STRING_OPTION_CAPACITY && BuildOptions->Defines[I]; I++)
		{
			Hash ^= F_HashString( BuildOptions->Defines[I]);
		}
		for(int I = 0; I < F_STRING_OPTION_CAPACITY && BuildOptions->Libraries[I]; I++)
		{
			Hash ^= F_HashString( BuildOptions->Libraries[I]);
		}
		for(int I = 0; I < F_STRING_OPTION_CAPACITY && BuildOptions->Frameworks[I]; I++)
		{
			Hash ^= F_HashString(BuildOptions->Frameworks[I]);
		}

		Hash ^= F_HashString(BuildOptions->Toolchain.CompilerPath);
		Hash ^= F_HashString(BuildOptions->Toolchain.LinkerPath);
		Hash ^= F_HashString(BuildOptions->Toolchain.IncludeFlag);
		Hash ^= F_HashString(BuildOptions->Toolchain.OutputFlag);
		Hash ^= F_HashString(BuildOptions->Toolchain.OptimiseFlag);
		Hash ^= F_HashString(BuildOptions->Toolchain.DynamicFlag);
		Hash ^= F_HashString(BuildOptions->Toolchain.StaticFlag);
		Hash ^= F_HashString(BuildOptions->Toolchain.CompileOnlyFlag);
		Hash ^= F_HashString(BuildOptions->Toolchain.LinkOnlyFlag);	

		Hash = F_HashBytes(Hash, &BuildOptions->Debug, sizeof BuildOptions->Debug);
		Hash = F_HashBytes(Hash, &BuildOptions->Optimise, sizeof BuildOptions->Optimise);
		char *ObjectPath = {0};
		if(F_StringLength(BuildOptions->OutputPath) > 0)
		{
			ObjectPath = F_StringPushF(F_TArena(), "%s/%llu.o", BuildOptions->OutputPath, Hash);
		}
		else
		{
			ObjectPath = F_StringPushF(F_TArena(), "%llu.o", Hash);
		}
		ObjectFiles[I] = ObjectPath;
		if(!F_HashIsInBuildCache(BuildCache, Hash))
		{
			f_string_list Args = {0};

			CompileStarted = F_True;


			if(TUs[I].IsEmbed)
			{
				F_EmitDataAsObjectFile(ObjectPath, F_StringPushF(F_TArena(), "E_%s", TUs[I].FilenameWithNoExtension), EmbedContents.Bytes, EmbedContents.NumberOfBytes);
			}
			else
			{
				F_StringListAppend(F_TArena(), &Args, BuildOptions->Toolchain.CompilerPath);
				F_StringListAppend(F_TArena(), &Args, BuildOptions->Toolchain.CompileOnlyFlag);
				F_StringListAppend(F_TArena(), &Args, TUs[I].FullRelativePath);

				if(BuildOptions->Debug)
				{
					F_StringListAppendF(F_TArena(), &Args, "%s", BuildOptions->Toolchain.DebugFlag);
				}

				for(int I = 0; I < F_STRING_OPTION_CAPACITY && BuildOptions->CompileFlags[I]; I++)
				{
					F_StringListAppend(F_TArena(), &Args, BuildOptions->CompileFlags[I]);
				}

				F_StringListAppendF(F_TArena(), &Args, "%s%s", BuildOptions->Toolchain.OutputFlag, ObjectPath);

				for(int I = 0; I < F_STRING_OPTION_CAPACITY && BuildOptions->IncludeDirectories[I]; I++)
				{
					F_StringListAppendF(F_TArena(), &Args, "%s%s", BuildOptions->Toolchain.IncludeFlag, BuildOptions->IncludeDirectories[I]);
				}

				F_StartProcess(F_TArena(), Processes + I - TURange.Start, Args);
			}
		}
	}
	F_WaitForMultipleProcesses(Processes, TURange.End - TURange.Start);
	
	f_b32 AnyProcessFailed = F_GroupAnyTrue(Group, F_AnyProcessesFailed(Processes, TURange.End - TURange.Start));
	f_b32 ShouldLinkTUs = !AnyProcessFailed && F_GroupAnyTrue(Group, CompileStarted);

	if(Group.ID == 0)
	{
		
		char *OutputPath = {0};
		if(F_StringLength(BuildOptions->OutputPath) > 0)
		{
			OutputPath = F_StringPushF(F_TArena(), "%s/%s%llu", BuildOptions->OutputPath, BuildOptions->OutputName, atomic_load(&Build->TotalSuccessfulBuilds));
		}
		else
		{
			OutputPath = F_StringPushF(F_TArena(), "%s%llu", BuildOptions->OutputName, atomic_load(&Build->TotalSuccessfulBuilds));
		}

		f_b32 NeedsToReload = F_False;
		
		if(ShouldLinkTUs || (!Build->UpdateDLL.Handle && !AnyProcessFailed))
		{
			f_string_list Args = {0};

			F_StringListAppend(F_TArena(), &Args, BuildOptions->Toolchain.CompilerPath);

			for(int I = 0; I < F_STRING_OPTION_CAPACITY && BuildOptions->LinkFlags[I]; I++)
			{
				F_StringListAppend(F_TArena(), &Args, BuildOptions->LinkFlags[I]);
			}

			for(int I = 0; I < F_STRING_OPTION_CAPACITY && BuildOptions->Frameworks[I]; I++)
			{
				F_StringListAppend(F_TArena(), &Args, "-framework");
				F_StringListAppend(F_TArena(), &Args, BuildOptions->Frameworks[I]);
			}

			if(BuildOptions->Kind == F_OUTPUT_DYNAMIC)
			{
				F_StringListAppend(F_TArena(), &Args, F_StringPushF(F_TArena(), "%s", BuildOptions->Toolchain.DynamicFlag));
			}
			else if(BuildOptions->Kind == F_OUTPUT_STATIC)
			{
				F_StringListAppend(F_TArena(), &Args, F_StringPushF(F_TArena(), "%s", BuildOptions->Toolchain.StaticFlag));
			}

			F_StringListAppend(F_TArena(), &Args, F_StringPushF(F_TArena(), "%s%s", BuildOptions->Toolchain.OutputFlag, OutputPath));
			
			for(int I = 0; I < TUCount; I++)
			{
				F_StringListAppend(F_TArena(), &Args, ObjectFiles[I]);
			}
			f_process LinkProcess = {0};
			F_StartProcess(F_TArena(), &LinkProcess, Args);
			F_WaitForProcess(&LinkProcess);

			f_process DSYMProcess = {0};
			f_string_list DSymArgs = {0};
			F_StringListAppend(F_TArena(), &DSymArgs, "dsymutil");
			F_StringListAppend(F_TArena(), &DSymArgs, OutputPath);
			F_StringListAppend(F_TArena(), &DSymArgs, "-o");
			F_StringListAppend(F_TArena(), &DSymArgs, F_StringPushF(F_TArena(), "%s.dSYM", OutputPath));

			F_StartProcess(F_TArena(), &DSYMProcess, DSymArgs);
			
			F_WaitForProcess(&DSYMProcess);

			if(!(F_ProcessFailed(&DSYMProcess) || F_ProcessFailed(&LinkProcess)))
			{
				printf(F_GREEN "\u2713" F_RESET F_DIM " [FRIDAY]" F_RESET " RELOAD SUCCESSFUL" "\n");
				NeedsToReload = F_True;
				atomic_fetch_add(&Build->TotalSuccessfulBuilds, 1);
			}
		}
		if(NeedsToReload)
		{
			Build->LastUpdated = F_OS_TimeStamp();
			Build->UpdateDLL = F_OS_DLLFromPath(OutputPath);
			Build->Update = F_OS_DLLSymbolFromName(Build->UpdateDLL, "AppUpdate");
		}
	}

	f_range BuildCacheRange = F_SplitRange(Group, BuildCache.Count);
	for(f_s32 I = BuildCacheRange.Start; I < BuildCacheRange.End; I++)
	{
		if(!BuildCache.NeededObjects[I])
		{
			F_OS_DeleteFile(F_TArena(), F_StringPushF(F_TArena(), "build/%llu.o", BuildCache.Objects[I]));
		}
	}


	F_GroupBarrier(Group);
}

f_build_cache F_BuildCacheFromOutputPath(f_group Group, f_memory_arena *Arena, char *OutputPath)
{
		f_string_array ObjectFiles = F_OS_ArrayOfEntriesInDirectory(
		Arena, OutputPath, .ExcludeDirectories = F_True,
		.FilterByExtension = { ".o" }
	);
	f_build_cache Cache = {0};
	if(Group.ID == 0)
	{
		Cache.Objects = F_ArenaPushArrayType(Arena, ObjectFiles.Count, f_u64);
		Cache.NeededObjects = F_ArenaPushArrayType(Arena, ObjectFiles.Count, f_b32);
	}
	Cache.Count = ObjectFiles.Count;
	Cache = F_ReadFromGroupID(Group, 0, Cache);
	f_range Range = F_SplitRange(Group, ObjectFiles.Count);
	for (f_s32 I = Range.Start; I < Range.End; I++)
	{
		char *HashAsString = ObjectFiles.Items[I];
		// Remove extension
		f_s64 HashLength = F_StringLength(HashAsString) - 2;
		f_u64 Hash = 0;
		for(f_s32 I = 0; I < HashLength; I++)
		{
			Hash = HashAsString[I] - '0' + Hash*10;
		}
		Cache.Objects[I] = Hash;
	}
	return Cache;
}

f_b32 F_HashIsInBuildCache(f_build_cache Cache, f_u64 Hash)
{
	for (f_s32 I = 0; I < Cache.Count; I++)
	{
		if(Cache.Objects[I] == Hash)
		{
			Cache.NeededObjects[I] = F_True;
			return F_True;
		}
	}

	return F_False;
}

f_u64 __F_Internal_HashSourceAndIncludes(f_memory_arena *Arena, f_file File, char **IncludePaths, f_string_list *IncludesSeenAlready);


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

f_u64 F_HashString(char *String)
{
	return F_HashBytes(F_HashStart, String, F_StringLength(String));
}

char *F_FilenameAndExtensionFromPath(f_memory_arena *Arena, char *Path)
{
	f_u64 IndexOfLastForwardSlash = 0;
	f_u64 PathLength = F_StringLength(Path);
	for(f_u64 I = 0; I < PathLength; I++)
	{
		if(Path[I] == '/') IndexOfLastForwardSlash = I;
	}

	return F_StringPushF(Arena, "%s", Path + IndexOfLastForwardSlash);
}

char *F_FilenameNoExtensionFromPath(f_memory_arena *Arena, char *Path)
{
	f_u64 IndexOfLastForwardSlash = 0;
	f_u64 IndexOfLastPeriod = 0;

	f_u64 PathLength = F_StringLength(Path);
	for(f_u64 I = 0; I < PathLength; I++)
	{
		if(Path[I] == '/') IndexOfLastForwardSlash = I;
		if(Path[I] == '.') IndexOfLastPeriod = I;
	}

	PathLength = IndexOfLastPeriod - (IndexOfLastForwardSlash + 1);

	return F_StringPushF(Arena, "%.*s", PathLength, Path + IndexOfLastForwardSlash + 1);
}

char *F_PathWithNoFilename(f_memory_arena *Arena, char *Path)
{
	f_u64 IndexOfLastForwardSlash = 0;

	f_u64 PathLength = F_StringLength(Path);
	for(f_u64 I = 0; I < PathLength; I++)
	{
		if(Path[I] == '/') IndexOfLastForwardSlash = I + 1;
	}

	return F_StringPushF(Arena, "%s", IndexOfLastForwardSlash, Path);
}

f_string_list F_OS_Internal_ListOfEntriesInDirectories(f_memory_arena *Arena, f_internal_directory_options Options)
{
#define IsCStrDotOrDotDot(Name) (Name[0] == '.' && (Name[1] == 0 || (Name[1] == '.' && Name[2] == 0)))
	f_string_list Entries = { 0 };
#if defined(OS_WINDOWS)
	// Error handling or something
	WIN32_FIND_DATA FindData;

	HANDLE FoundHandle;
	for(f_s32 Directory = 0; Directory < F_DIRECTORY_CAPACITY && Options.Directories[Directory]; Directory++)
	{
		FoundHandle = FindFirstFile(F_StringPushF(Arena, "%s\\*", Options.Directories[Directory]), &FindData);
		if(FoundHandle == INVALID_HANDLE_VALUE) continue;
		do
		{
			f_b32 ShouldExcludeDirectories = (Options.ExcludeDirectories && FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			f_b32 IsActualEntry = !IsCStrDotOrDotDot(FindData.cFileName);
			if(ShouldExcludeDirectories) continue;
			if(!IsActualEntry) continue;
			char *EntryName = F_StringPushF(Arena, "%s", FindData.cFileName);
			f_b32 MatchFound = F_True;
			if(Options.FilterByExtension[0] != F_Null)
			{
				MatchFound = F_False;
				for(f_s32 I = 0; I < F_DIRECTORY_CAPACITY && Options.FilterByExtension[I]; I++)
				{
					if(F_StringEndsWith(EntryName, Options.FilterByExtension[I]))
					{
						MatchFound = F_True;
					}
				}

			}
			if(!MatchFound) continue;
			if(Options.PrependPath)
			{
				F_StringListAppendF(Arena, &Entries, "%s\\%s", Options.Directories[Directory], EntryName);
			}
			else
			{
				F_StringListAppendF(Arena, &Entries, "%s", EntryName);
			}
		}
		while (FindNextFile(FoundHandle, &FindData) && FoundHandle != INVALID_HANDLE_VALUE);
		FindClose(FoundHandle);
	}
#elif defined(OS_DARWIN)
	for(f_s32 I = 0; I < F_STRING_OPTION_CAPACITY && Options.Directories[I]; I++)
	{
		DIR *Directory = opendir(Options.Directories[I]);
		if(!Directory) continue;
		struct dirent *Entry;
		while((Entry = readdir(Directory)))
		{
			f_b32 IsActualEntry = !IsCStrDotOrDotDot(Entry->d_name);
			f_b32 ShouldExcludeDirectories = (Options.ExcludeDirectories && Entry->d_type == DT_DIR);
			if(!IsActualEntry) continue;
			if(ShouldExcludeDirectories) continue;

			char *EntryName = F_StringPushF(Arena, "%s", Entry->d_name);

			f_b32 MatchFound = F_True;
			if(Options.FilterByExtension[0] != F_Null)
			{
				MatchFound = F_False;
				for(int I = 0; I < F_STRING_OPTION_CAPACITY && Options.FilterByExtension[I]; I++)
				{
					if(F_StringEndsWith(EntryName, Options.FilterByExtension[I]))
					{
						MatchFound = F_True;
						break;
					}
				}
			}
			if(!MatchFound) continue;
			
			if(Options.PrependPath)
			{
				F_StringListAppendF(Arena, &Entries, "%s/%s", Options.Directories[I], Entry->d_name);
			}
			else
			{
				F_StringListAppendF(Arena, &Entries, "%s", Entry->d_name);
			}
		}
		closedir(Directory);
	}
#else
#error "Unsupported OS"
#endif
	return Entries;
}

f_range F_SplitRange(f_group Group, f_u64 Value)
{
	f_range Result;
	Result.Start = Value * Group.ID / Group.Count;
	Result.End   = Value * (Group.ID + 1) / Group.Count;
	return Result;
}

void *F_Internal_ReadFromGroupID(f_group Group, f_s32 ID, void *Variable, f_u64 SizeOfVariable)
{
	if(Group.Count == 1) F_Assert(ID == 0);
	else
	{
		if(Group.ID == ID) Group.BroadcastPointers[0] = Variable;
		F_GroupBarrier(Group);
		F_MemoryCopy(Variable, Group.BroadcastPointers[0], SizeOfVariable);
		F_GroupBarrier(Group);
	}

	return Variable;
}

void F_OS_WaitForNMilliseconds(f_group Group, f_u64 Milliseconds)
{
	if(!F_IsInGroup(Group)) return;
#if defined(OS_WINDOWS)
#elif defined(OS_DARWIN)
	mach_timebase_info_data_t Info;
	mach_timebase_info(&Info);
	mach_wait_until(mach_absolute_time() + (f_u64)Milliseconds * 1000000ULL * Info.denom / Info.numer);
#else
#error "Unsupported OS"
#endif
}

void F_StartProcess(f_memory_arena *Arena, f_process *Process, f_string_list Args)
{
	if(!Process) return;
	Process->StdOutSinceLastPoll = F_ArenaPushArrayType(Arena, F_PROCESS_STDOUT_CAPACITY + 1, char);
#if defined(OS_WINDOWS)
	
	SECURITY_ATTRIBUTES Attributes = {0};
	Attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	Attributes.bInheritHandle = F_True;
	Attributes.lpSecurityDescriptor = F_Null;
	HANDLE ReadPipe, WritePipe;
	CreatePipe(&ReadPipe, &WritePipe, &Attributes, 0);
	SetHandleInformation(ReadPipe, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFO StartupInfo = {0};
	StartupInfo.dwFlags = STARTF_USESTDHANDLES;
	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.hStdOutput = WritePipe;
	StartupInfo.hStdError = WritePipe;
	StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	PROCESS_INFORMATION ProcessInfo = {0};
	f_b32 ProcessStatus = CreateProcessA(
			F_Null,
			F_StringJoinList(Arena, Args, " "),
			F_Null,
			F_Null,
			F_True,
			0,
			F_Null,
			F_Null,
			&StartupInfo,
			&ProcessInfo
	);
	CloseHandle(WritePipe);
	if(ProcessStatus)
	{
		Process->ID = (f_u64)ProcessInfo.hProcess;
		Process->StdOut = ReadPipe;
		Process->State = F_PROCESS_RUNNING;
		CloseHandle(ProcessInfo.hThread);
	}
	else
	{
		Process->State = F_PROCESS_FAILED_TO_LAUNCH;
		Process->ExitCode = (f_exit_code){ProcessStatus, 0};
	}
#elif defined(OS_DARWIN)

	f_s64 MaxArguments = F_StringListCount(Args) + 1;
	char **CompilerArguments = F_ArenaPushArrayType(Arena, MaxArguments, char*);
	f_s32 ArgumentCount = 0;
	for(f_string_list_node *Node = Args.First; Node; Node = Node->Next)
	{
		CompilerArguments[ArgumentCount++] = Node->String;
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

	pid_t ChildPID = 0;
	f_s32 ProcessStatus = posix_spawnp(&ChildPID, CompilerArguments[0], &SpawnActions, F_Null, CompilerArguments, environ);
	posix_spawn_file_actions_destroy(&SpawnActions);
	close(FileDescriptors[1]);

	if(ProcessStatus == 0)
	{
		Process->ID = ChildPID;
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
	f_u64 OutputLength = 0;
	if(Process->StdOutSinceLastPoll) Process->StdOutSinceLastPoll[0] = 0;

	if(Process->State != F_PROCESS_RUNNING) return F_False;
#if defined(OS_WINDOWS)
		DWORD BytesRead = 0;
	DWORD BytesAvailable = 0;
	DWORD BytesLeft = 0;

	f_b32 StillRunning = WaitForSingleObject((HANDLE)Process->ID, 0) == WAIT_TIMEOUT;

	if(PeekNamedPipe(Process->StdOut, F_Null, 0, F_Null, &BytesAvailable, &BytesLeft)
				&& BytesAvailable > 0)
	{
		BytesAvailable = F_Min(BytesAvailable, F_PROCESS_STDOUT_CAPACITY);
		if(ReadFile(Process->StdOut, Process->StdOutSinceLastPoll,
														BytesAvailable, &BytesRead, F_Null))
		{
			OutputLength = BytesRead;
			Process->StdOutSinceLastPoll[OutputLength] = 0;
		}
	}

	if(!StillRunning && OutputLength == 0)
	{
		DWORD ExitCode = 0;
		GetExitCodeProcess((HANDLE)Process->ID, &ExitCode);
		Process->ExitCode = (f_exit_code){ ExitCode, 0 };
		Process->State = F_PROCESS_EXITED;
		CloseHandle(Process->StdOut);
		CloseHandle((HANDLE)Process->ID);
		Process->StdOut = F_Null;
		return F_False;
	}

	return F_True;
#elif defined(OS_DARWIN)

	f_s64 BytesRead = read((f_s32)((f_u64)Process->StdOut), Process->StdOutSinceLastPoll, F_PROCESS_STDOUT_CAPACITY);
	if(BytesRead > 0)
	{
		OutputLength = BytesRead;
		Process->StdOutSinceLastPoll[OutputLength] = 0;
	}

	int Status = 0;
	if (waitpid(Process->ID, &Status, WNOHANG) != Process->ID)
	{
		return F_True;
	}

	for(;OutputLength < F_PROCESS_STDOUT_CAPACITY;)
	{
		char *BytesTail = Process->StdOutSinceLastPoll + OutputLength;
		f_u64 BytesLeft = F_PROCESS_STDOUT_CAPACITY - OutputLength;
		f_s64 Tail = read((f_u32)((f_u64)Process->StdOut), BytesTail, BytesLeft);

		if(Tail <= 0) break;
		else
		{
			OutputLength += Tail;
			Process->StdOutSinceLastPoll[OutputLength] = 0;
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

f_b32 F_ProcessFailed(f_process *Process)
{
	f_b32 Result = F_False;
#if defined(OS_WINDOWS)
	if(Process->ExitCode.Values[0] || Process->ExitCode.Values[1])
	{
		Result = F_True;
	}
#elif defined(OS_DARWIN)
	if(Process->ExitCode.Values[0] || Process->ExitCode.Values[1])
	{
		Result = F_True;
	}
#else
#error "Unsupported OS"
#endif
	return Result;
}

f_b32 F_AnyProcessesFailed(f_process *Processes, f_s32 ProcessCount)
{
	F_WaitForMultipleProcesses(Processes,ProcessCount);
	for(f_s32 I = 0; I < ProcessCount; I++)
	{
		if(Processes[I].State == F_PROCESS_NONE) continue;
		if(F_ProcessFailed(Processes + I)) return F_True;
	}
	return F_False;
}

void F_WaitForProcess(f_process *Process)
{
	while(F_PollProcess(Process))
	{
		if(F_StringLength(Process->StdOutSinceLastPoll) > 0) F_Print(Process->StdOutSinceLastPoll);
	}
	if(F_StringLength(Process->StdOutSinceLastPoll) > 0) F_Print(Process->StdOutSinceLastPoll);
}

void  F_WaitForMultipleProcesses(f_process *Processes, f_s32 ProcessCount)
{
	for (f_s32 I = 0; I < ProcessCount; I++)
	{
		if(Processes[I].ID > 0) F_WaitForProcess(Processes + I);
	}
}

f_u64 __F_Internal_HashSourceAndIncludes(f_memory_arena *Arena, f_file File, char **IncludePaths, f_string_list *IncludesSeenAlready)
{
#define CharAt(Index) ((Index) < Source.NumberOfBytes ? Source.Bytes[Index] : 0)
#define AdvanceBy(Index, Amount) ((Index += Amount), Index < Source.NumberOfBytes)
#define Advance(Index) (AdvanceBy(Index, 1))
#define MatchAt(String, Index) (strncmp((char*)Source.Bytes + (Index), String, F_StringLength(String)) == 0)
#define SkipWhiteSpace(Index) for(;CharAt(Index) == ' ' || CharAt(Index) == '\t';Advance(Index))
#define SkipUntilMatch(Index, Char) for(;CharAt(Index) != 0 && CharAt(Index) != Char; Advance(Index))
#define SkipUntilMatch2(Index, CharA, CharB) for(;CharAt(Index) != 0 && (CharAt(Index) != CharA || CharAt(Index + 1) != CharB); Advance(Index))

	f_buffer Source = F_OS_ReadEntireFile(Arena, &File);
	// Hash whilst advancing?
	f_u64 SourceAndIncludesHash = F_HashString((char*)Source.Bytes);
	for(f_s64 I = 0; I < Source.NumberOfBytes; I++)
	{
		switch(Source.Bytes[I])
		{
			case '#': {
				Advance(I);
				char *Include = "include";
				char *PathToInclude = {0};
				if(MatchAt(Include, I))
				{
					AdvanceBy(I, F_StringLength(Include));
					SkipWhiteSpace(I);
					if(CharAt(I) == '<')
					{
						Advance(I);
						f_u64 Start = I;
						SkipUntilMatch(I, '>');
						PathToInclude = F_StringPushF(Arena, "%.*s", (int)(I - Start), Source.Bytes + Start);
					}
					else if(CharAt(I) == '\"')
					{
						Advance(I);
						f_u64 Start = I;
						SkipUntilMatch(I, '\"');
						PathToInclude = F_StringPushF(Arena, "%.*s", (int)(I - Start), Source.Bytes + Start);
					}
				}
				if(F_StringLength(PathToInclude) > 0)
				{
					f_file_info Info = {0};
					Info = F_OS_FileInfoFromFilename(Arena, PathToInclude);
					
					for(f_s32 IncludePathIndex = 0; !Info.Exists && IncludePaths && IncludePathIndex < F_STRING_OPTION_CAPACITY && IncludePaths[IncludePathIndex]; IncludePathIndex++)
					{
						char *PrependedPathToInclude = F_StringPushF(Arena, "%s/%s", IncludePaths[IncludePathIndex], PathToInclude);
						Info = F_OS_FileInfoFromFilename(Arena, PrependedPathToInclude);
						if(Info.Exists)
						{
							PathToInclude = PrependedPathToInclude;
							break;
						}
					}
					f_b32 IncludeSeenAlready = F_False;
					for(f_string_list_node *Node = IncludesSeenAlready->First; Node; Node = Node->Next)
					{
						if(F_StringEqual(Node->String, PathToInclude)) IncludeSeenAlready = F_True;
					}
					if(Info.Exists && !IncludeSeenAlready)
					{
						// This isn't perfect but it's actually find since even though the order of hashing with xor doesn't now matter,
						// it's never an issue with includes.
						F_StringListAppend(Arena, IncludesSeenAlready, PathToInclude);
						f_file IncludeFile = F_OS_FileFromFilename(F_TArena(), PathToInclude);
						SourceAndIncludesHash ^= __F_Internal_HashSourceAndIncludes(Arena, IncludeFile, IncludePaths, IncludesSeenAlready);
						F_OS_ReleaseFile(&IncludeFile);
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
#undef MatchAt
#undef CharAt
#undef Advance
#undef AdvanceBy
#undef SkipWhiteSpace
#undef SkipUntilMatch
#undef SkipUntilMatch2
}

f_u64 F_HashSourceAndIncludes(f_memory_arena *Arena, f_file File, char **IncludePaths)
{
	f_string_list IncludesSeenAlready = {0};
	return __F_Internal_HashSourceAndIncludes(Arena, File, IncludePaths, &IncludesSeenAlready);
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

void F_FailedBanner(f_string_array Stats)
{
	for(int I = 0; I < 5; I++)
	{
		char *Stat = "";
		if(I < Stats.Count)
		{
			Stat = Stats.Items[I];
		}
		printf(F_RED "%-64s" F_RESET F_DIM "\u2502" F_RESET "  %s\n", __FailedASCII[I],  Stat);
	}
}

void F_PassedBanner(f_string_array Stats)
{
	for(int I = 0; I < 5; I++)
	{
		char *Stat = "";
		if(I < Stats.Count)
		{
			Stat = Stats.Items[I];
		}
		printf(F_GREEN "%-64s" F_RESET F_DIM "\u2502" F_RESET "  %s\n", __PassedASCII[I],  Stat);
	}
}

#endif

#if 0 /*
'''
# */
#endif
