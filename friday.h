#pragma once
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#define FRIDAY_VERSION "IV"

/*
Thanks to
- shachaf
- graphitemaster

For giving me a hand with the synchronisation barrier on mac
*/

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

#define F_KiloBytes(X) (1024ll * X)
#define F_MegaBytes(X) (1024ll * F_KiloBytes(X))
#define F_GigaBytes(X) (1024ll * F_MegaBytes(X))

size_t strlen(const char *str);

#define F_Min(a, b) ((a) < (b) ? (a) : (b))

#define F_Str8ToPrintfArgs(String) (int)String.Size, (char*)String.Bytes
#define F_Str8(String) (f_str8) { strlen(String), (f_s8*)String }

#define F_ArenaPushType(Arena, Type) (Type *)F_ArenaPush(Arena, sizeof(Type))

struct f_shared_wave_info;
struct f_wave_info;

f_u8 *F_SharedArenaPush(struct f_wave_info *WaveInfo, f_s64 Bytes);

#define F_False 0
#define F_True (!F_False)
#define F_Null (void *)0

#define F_STR_COLOUR_RED     "\x1B[31m"
#define F_STR_COLOUR_GREEN   "\x1B[32m"
#define F_STR_COLOUR_YELLOW  "\x1B[33m"
#define F_STR_COLOUR_BLUE    "\x1B[34m"
#define F_STR_COLOUR_MAGENTA "\x1B[35m"
#define F_STR_COLOUR_CYAN    "\x1B[36m"
#define F_STR_COLOUR_WHITE   "\x1B[37m"
#define F_STR_COLOUR_RESET   "\x1B[0m"

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
} f_memory_arena;

typedef struct f_str8
{
	f_s64 Size;
	f_s8 *Bytes;
} f_str8;

typedef struct f_str8_list_node
{
	struct f_str8_list_node *Next;
	struct f_str8_list_node *Previous;
	f_str8 String;
} f_str8_list_node;

typedef struct f_str8_list
{
	f_str8_list_node *First;
	f_str8_list_node *Last;
} f_str8_list;

#define f_array(T) struct { T *Items; f_s64 Count; }

typedef f_array(f_str8) f_str8_array;

#if defined(__APPLE__)

#include <util.h>
typedef enum f_compiler
{
	F_COMPILER_PLATFORM_DEFAULT = 0,
	F_COMPILER_CLANG = 0,
	F_COMPILER_GCC = 1,
	F_COMPILER_CARNAGE = 2,

	F_COMPILER_MSVC = F_COMPILER_PLATFORM_DEFAULT,

	F_COMPILER_COUNT,
} f_compiler;

#define F_OUTPUT_FLAG_STATIC F_Str8("-static")
#define F_OUTPUT_FLAG_DYNAMIC F_Str8("-dynamiclib")

#elif defined(_WIN32)
#include <stdatomic.h>

typedef enum f_compiler
{
	F_COMPILER_PLATFORM_DEFAULT = 0,
	F_COMPILER_MSVC = 0,
	F_COMPILER_CLANG = 1,
	F_COMPILER_CARNAGE = 2, // glep is mac-only; falls back to clang-cl on Windows.

	F_COMPILER_GCC = F_COMPILER_PLATFORM_DEFAULT,
	F_COMPILER_COUNT,
} f_compiler;
#endif

typedef enum f_diagnostic_type
{
	F_D_WARNING,
	F_D_ERROR,
	F_D_INFO,
}
f_diagnostic_type;

typedef struct f_diagnostic
{
	f_diagnostic_type Type;
	f_str8 Message;
	struct f_diagnostic *Next;
}
f_diagnostic;


typedef struct f_diagnostic_info
{
	f_diagnostic *Diagnostics;
}
f_diagnostic_info;

typedef enum f_output_type
{
	F_OUTPUT_EXECUTABLE,
	F_OUTPUT_STATIC_LIBRARY,
	F_OUTPUT_DYNAMIC_LIBRARY,
	F_OUTPUT_COUNT,
} f_output_type;

typedef struct f_build_options
{
	f_compiler Compiler;

	f_output_type OutputType;
	f_str8 OutputName;
	f_str8 OutputPath;

	f_str8 WorkingDirectory;

	f_str8_list IncludePaths;
	f_str8_list SourceFiles;
	f_str8_list ExtraLinkFlags;
	f_str8_list ExtraCompileFlags;

	f_b32 Optimise;
	f_b32 FullRebuild;
	f_b32 GenerateDebugInfo;
	f_b32 PrintHelpMenu;
} f_build_options;


f_u8* F_ArenaPush(f_memory_arena *Arena, f_s64 Bytes);
f_u8 *F_ArenaPush(f_memory_arena *Arena, f_s64 Bytes);
f_str8 F_Str8PushF(f_memory_arena *Arena, char *Format, ...);
f_str8 F_Str8ListJoin(f_memory_arena *Arena, f_str8_list List);
f_str8 F_Str8ArrayJoin(f_memory_arena *Arena, f_str8_array Array);
void F_Str8ListAppend(f_memory_arena *Arena, f_str8_list *List, f_str8 String);
f_b32  F_Str8EndsWith(f_str8 String, f_str8 EndsWith);
char *F_CStrFromStr8(f_memory_arena *Arena, f_str8 String);
f_str8_list F_ListOfDirectoryEntriesFromPath(f_memory_arena *Arena, f_str8 Path);


#define F_ERROR F_STR_COLOUR_WHITE "[" F_STR_COLOUR_RED "F_ERROR" F_STR_COLOUR_WHITE "]\n" F_STR_COLOUR_RESET
#define F_WARN F_STR_COLOUR_WHITE "[" F_STR_COLOUR_YELLOW "SUCCESS" F_STR_COLOUR_YELLOW "]\n" F_STR_COLOUR_RESET
#define F_SUCCESS F_STR_COLOUR_WHITE "[" F_STR_COLOUR_GREEN "SUCCESS" F_STR_COLOUR_WHITE "]\n" F_STR_COLOUR_RESET

void F_GobbleWhitespace(f_str8 File, f_u64 *At);
void F_GobbleUpToWhitespace(f_str8 File, f_u64 *At);

f_operating_system F_GetOperatingSystem();



#if __APPLE__
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdatomic.h>
#include <spawn.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <mach/mach_time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <os/os_sync_wait_on_address.h>
#include <os/lock.h>

extern char **environ;

#define F_MemoryReserve(NumberOfBytes) (f_u8 *)mmap(NULL, NumberOfBytes, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0)
#define F_MemoryCommit(Address, NumberOfBytes) mprotect(Address, NumberOfBytes, PROT_READ|PROT_WRITE)

#define F_MemoryCopy(Destination, Source, NumberOfBytes) memcpy(Destination, Source, NumberOfBytes)
#define F_MemoryZero(Destination, NumberOfBytes) memset(Destination, 0, NumberOfBytes)

#define f_thread_local _Thread_Local

f_operating_system F_GetOperatingSystem()
{
	return F_OS_DARWIN;
}

f_u64 F_GetPageSize()
{
	return getpagesize();
}

f_str8 F_LoadEntireFileIntoMemory(f_memory_arena *Arena, f_str8 Filename)
{
	f_s32 FileHandle = open(F_CStrFromStr8(Arena, Filename), O_RDONLY);
	if (FileHandle == -1)
	{
		printf("File not found");
		return (f_str8){0};
	}

	struct stat FileStat;
	fstat(FileHandle, &FileStat);

	f_u8 *BackingMemory = F_ArenaPush(Arena, (f_u64)FileStat.st_size + 1);
	BackingMemory[FileStat.st_size] = 0;

	f_str8 Result = {0};
	Result.Bytes = (f_s8*)BackingMemory;
	Result.Size = FileStat.st_size;

	ssize_t BytesRead = read(FileHandle, BackingMemory, FileStat.st_size);

	return Result;
}

void F_WriteStringToDisk(f_memory_arena *Arena, f_str8 DestinationPath, f_str8 StringToWrite)
{
	f_s32 FileDescriptor = creat(F_CStrFromStr8(Arena, DestinationPath), S_IRUSR | S_IWUSR );
	f_s64 BytesWritten = write(FileDescriptor, StringToWrite.Bytes, StringToWrite.Size);
}

f_str8 F_GetExecutableDirectory(f_memory_arena *Arena)
{
	f_s32 NumberOfBytes = PATH_MAX;
	f_u8 *BackingMemory = F_ArenaPush(Arena, NumberOfBytes);

	getcwd((char*)BackingMemory, PATH_MAX);

	f_str8 Result = {0};
	Result.Size = NumberOfBytes;
	Result.Bytes = (f_s8*)BackingMemory;

	return Result;
}

void F_AppendSourceFilesInFolder(f_memory_arena *Arena, f_str8_list *SourceFilesFromFolder, f_str8 Path)
{
	f_str8 SourceFilesInDirectory = F_Str8PushF(Arena, "%.*s/", F_Str8ToPrintfArgs(Path));

	DIR *Directory = opendir((char*)SourceFilesInDirectory.Bytes);
	struct dirent *Entry;
	if(Directory == F_Null)
	{ 
		printf("Unknown directory %.*s", F_Str8ToPrintfArgs(Path));
		return;
	}

	while((Entry = readdir(Directory)))
	{
		if (Entry->d_name[0] != '.' && (F_Str8EndsWith(F_Str8(Entry->d_name), F_Str8(".c")) ||
										F_Str8EndsWith(F_Str8(Entry->d_name), F_Str8(".m"))))
		{
			struct stat FileStat;
			stat(Entry->d_name, &FileStat);
			if (!S_ISDIR(FileStat.st_mode))
			{
				f_str8 SourceFilepath = F_Str8PushF(Arena, "%.*s/%s", F_Str8ToPrintfArgs(Path), Entry->d_name);
				F_Str8ListAppend(Arena, SourceFilesFromFolder, SourceFilepath);
			}
		}
	}
}

f_b32 F_PathExists(f_memory_arena *Arena, f_str8 Filename)
{
	return access(F_CStrFromStr8(Arena, Filename), F_OK) == 0;
}

f_s64 F_AtomicAdd(f_s64 *Value, f_s64 Addend)
{
	return __sync_fetch_and_add(Value, Addend);
}

f_u64 F_TimeStampNow()
{
	return mach_absolute_time();
}

f_f32 F_MilliSecondsFromTimeStamp(f_u64 TimeStamp)
{
	mach_timebase_info_data_t Info;
	mach_timebase_info(&Info);

	return (f_f32)((TimeStamp) * Info.numer / Info.denom) / 1000000.0;

}

f_s64 F_GetNumberOfCPUs()
{
	f_s32 NumberOfCPUs;
	size_t len = sizeof(NumberOfCPUs);

	sysctlbyname("hw.ncpu", &NumberOfCPUs, &len, NULL, 0);

	return NumberOfCPUs;
}

f_str8_list F_ListOfDirectoryEntriesFromPath(f_memory_arena *Arena, f_str8 Path)
{
	f_str8_list Entries = { 0 };
	DIR *Directory = opendir(F_CStrFromStr8(Arena, Path));
	struct dirent *Entry;
	while((Entry = readdir(Directory)))
	{
		if(Entry->d_name[0] != '.')
		{
			F_Str8ListAppend(Arena, &Entries, F_Str8PushF(Arena, "%s", Entry->d_name));
		}
	}
	return Entries;
}

void F_DeleteFileFromDisk(f_memory_arena *Arena, f_str8 Filename)
{
	remove(F_CStrFromStr8(Arena, Filename));
}

int F_MoveFileOnDisk(f_memory_arena *Arena, f_str8 Source, f_str8 Destination)
{
	return rename(F_CStrFromStr8(Arena, Source), F_CStrFromStr8(Arena, Destination));
}

/*

I don't like how I need spaces at the ends of the commands.
This whole thing is dumb

HACK(Oliver): Fix F_ExecuteCommand spaces

 */
f_b32 F_ExecuteCommand(f_memory_arena *Arena, f_str8 EntireCommandAndArguments, f_b32 *FailedToExecute)
{
	pid_t ProcessID;
	posix_spawn_file_actions_t SpawnActions;

	int MasterFD = -1;
	int SlaveFD = -1;
	openpty(&MasterFD, &SlaveFD, F_Null, F_Null, F_Null);

	posix_spawn_file_actions_init(&SpawnActions);

	posix_spawn_file_actions_adddup2(&SpawnActions, SlaveFD, STDOUT_FILENO);
	posix_spawn_file_actions_adddup2(&SpawnActions, SlaveFD, STDERR_FILENO);
	posix_spawn_file_actions_addclose(&SpawnActions, MasterFD);
	posix_spawn_file_actions_addclose(&SpawnActions, SlaveFD);

	char** CompilerArguments = (char**)F_ArenaPush(Arena, 1024*sizeof(char*));
	f_u64 ArgumentIndex = 0;
	for(f_u64 Index = 0; Index < EntireCommandAndArguments.Size; Index++, ArgumentIndex++)
	{
		F_GobbleWhitespace(EntireCommandAndArguments, &Index);
		f_u64 IndexUpToWhitespace = Index;
		F_GobbleUpToWhitespace(EntireCommandAndArguments, &IndexUpToWhitespace);
		f_str8 Argument = { 0 };
		Argument.Bytes = EntireCommandAndArguments.Bytes + Index;
		Argument.Size = IndexUpToWhitespace - Index;
		CompilerArguments[ArgumentIndex] = F_CStrFromStr8(Arena, Argument);
		Index += Argument.Size;
	}

	CompilerArguments[ArgumentIndex] = F_Null;

	f_s32 ProcessStatus = posix_spawnp(&ProcessID, CompilerArguments[0], &SpawnActions, F_Null, CompilerArguments, environ);
	posix_spawn_file_actions_destroy(&SpawnActions);
	
	if (ProcessStatus == 0)
	{
		close(SlaveFD);

		f_str8_list StdOutList = {0};

		f_s32 ExitCode;
		f_u8 *BackingMemory = F_ArenaPush(Arena, 4096);
		f_s64 NumberOfBytesRead = 0;  
		do
		{
			NumberOfBytesRead = read(MasterFD, BackingMemory, 4096);
			f_str8 String = {.Bytes = (f_s8*)BackingMemory, .Size = NumberOfBytesRead};
			write(STDOUT_FILENO, String.Bytes, String.Size);
		} while(NumberOfBytesRead > 0);
		waitpid(ProcessID, &ExitCode, WUNTRACED);
		close(MasterFD);
		


		FailedToExecute[0] = (WIFEXITED(ExitCode) && WEXITSTATUS(ExitCode) != 0);
	}

	return ProcessStatus == 0;
}

f_str8 F_CompileCommandFromBuildOptions(f_memory_arena *Arena, f_build_options *BuildOptions, f_str8 HashedFilename, f_str8 SourceFilename)
{
	f_b32 IsObjectiveCFile = F_Str8EndsWith(SourceFilename, F_Str8(".m"));

	// Carnage (glep) can't compile Objective-C — fall back to clang for .m files.
	f_compiler EffectiveCompiler = BuildOptions->Compiler;
	if (IsObjectiveCFile && EffectiveCompiler == F_COMPILER_CARNAGE)
	{
		EffectiveCompiler = F_COMPILER_CLANG;
	}

	f_str8_list IncludePathsWithFlag = {0};
	for (f_str8_list_node *Node = BuildOptions->IncludePaths.First; Node != F_Null; Node = Node->Next)
	{
		F_Str8ListAppend(Arena, &IncludePathsWithFlag, F_Str8PushF(Arena, "-I%.*s ", F_Str8ToPrintfArgs(Node->String)));
	}

	if (EffectiveCompiler == F_COMPILER_CARNAGE)
	{
		// glep uses -debug/-opt instead of -g/-O2, and -o takes a separate argument.
		f_str8 DebugOpt[2] = { F_Str8("-debug"), F_Str8("-opt") };
		return F_Str8PushF(
			Arena,
			"glep -c -no-debug-artifacts %.*s %.*s %.*s -o %.*s%.*s %.*s",
			F_Str8ToPrintfArgs(SourceFilename),
			F_Str8ToPrintfArgs(F_Str8ListJoin(Arena, BuildOptions->ExtraCompileFlags)),
			F_Str8ToPrintfArgs(DebugOpt[BuildOptions->Optimise]),
			F_Str8ToPrintfArgs(BuildOptions->OutputPath),
			F_Str8ToPrintfArgs(HashedFilename),
			F_Str8ToPrintfArgs(F_Str8ListJoin(Arena, IncludePathsWithFlag)));
	}

	f_str8 DebugOrRelease[2] =
	{
		F_Str8("-g -O0"), F_Str8("-O2")
	};

	f_str8 Compilers[3] =
	{
		F_Str8("clang"), F_Str8("gcc"), F_Str8("glep")
	};

	f_str8 ObjectiveC[2] =
	{
		F_Str8(""), F_Str8("-ObjC -fobjc-arc")
	};

	f_str8 CompileCommand = F_Str8PushF(
		Arena,
		"%.*s %.*s %.*s -c %.*s %.*s -o%.*s%.*s %.*s ",
		F_Str8ToPrintfArgs(Compilers[EffectiveCompiler]),
		F_Str8ToPrintfArgs(F_Str8ListJoin(Arena, BuildOptions->ExtraCompileFlags)),
		F_Str8ToPrintfArgs(ObjectiveC[IsObjectiveCFile]),
		F_Str8ToPrintfArgs(SourceFilename),
		F_Str8ToPrintfArgs(DebugOrRelease[BuildOptions->Optimise]),
		F_Str8ToPrintfArgs(BuildOptions->OutputPath),
		F_Str8ToPrintfArgs(HashedFilename),
		F_Str8ToPrintfArgs(F_Str8ListJoin(Arena, IncludePathsWithFlag)));

	return CompileCommand;
}

f_str8 F_LinkCommandFromBuildOptions(f_memory_arena *Arena, f_build_options *BuildOptions, f_str8_array ObjectFiles)
{
	f_str8 OutputTypes[F_OUTPUT_COUNT] =
	{
		F_Str8(""),
		F_OUTPUT_FLAG_STATIC,
		F_OUTPUT_FLAG_DYNAMIC,
	};

	f_str8 LinkCommand = F_Str8PushF(
		Arena,
		"clang %.*s %.*s -o %.*s%.*s %.*s",
		F_Str8ToPrintfArgs(OutputTypes[BuildOptions->OutputType]),
		F_Str8ToPrintfArgs(F_Str8ListJoin(Arena, BuildOptions->ExtraLinkFlags)),
		F_Str8ToPrintfArgs(BuildOptions->OutputPath),
		F_Str8ToPrintfArgs(BuildOptions->OutputName),
		F_Str8ToPrintfArgs(F_Str8ArrayJoin(Arena, ObjectFiles))
	);

	return LinkCommand;
}

#define F_Entry(Data) void *F_Entry(void *Data)

F_Entry(Data);


typedef struct f_barrier
{
	atomic_ullong Generation;
	atomic_ullong LanesWaiting;
	f_s64 LaneCount;
}
f_barrier;

typedef struct f_shared_wave_info
{
	f_s32 ArgCount;
	char **Args;

	f_barrier *Barrier;

	f_s64 LaneCount;

	f_memory_arena Arena;
	f_build_options BuildOptions;


	atomic_llong Data[4096];
}
f_shared_wave_info;

typedef struct f_wave_info
{
	f_shared_wave_info *Shared;
	f_s32 Lane;
}
f_wave_info;

/*
Based on Raymond Chen's article on synchronization barriers
https://devblogs.microsoft.com/oldnewthing/20160824-00/?p=94155
*/
void F_WaveBarrier(f_barrier *Barrier)
{
	f_s64 Generation = atomic_load_explicit(&Barrier->Generation, memory_order_acquire);
	f_s64 LanesWaiting = atomic_fetch_add_explicit(&Barrier->LanesWaiting, 1, memory_order_acq_rel);

	if(LanesWaiting < Barrier->LaneCount - 1)
	{
		while(atomic_load_explicit(&Barrier->Generation, memory_order_acquire) == Generation)
		{
			os_sync_wait_on_address(&Barrier->Generation, Generation, sizeof(atomic_llong), OS_SYNC_WAIT_ON_ADDRESS_SHARED);
		}
	}
	else if(LanesWaiting == Barrier->LaneCount - 1)
	{
		atomic_store_explicit(&Barrier->LanesWaiting, 0, memory_order_release);
		atomic_fetch_add_explicit(&Barrier->Generation, 1, memory_order_release);
		os_sync_wake_by_address_all(&Barrier->Generation, sizeof(atomic_llong), OS_SYNC_WAKE_BY_ADDRESS_SHARED);
	}
}

f_s64 F_LaneValueFromTotal(f_wave_info *WaveInfo, f_s64 Count)
{
	f_s64 Base = Count / WaveInfo->Shared->LaneCount;
	f_s64 Remainder = Count % WaveInfo->Shared->LaneCount;
	f_s64 Result = Count / WaveInfo->Shared->LaneCount + (WaveInfo->Lane < Remainder ? 1 : 0);

	if (Count < WaveInfo->Shared->LaneCount)
	{
		Result = (f_s64)(WaveInfo->Lane < Count);
	}

	return Result;
}

void F_Dispatch(f_shared_wave_info *Shared, f_s64 LaneCount)
{
	f_wave_info *Entries = (f_wave_info*)F_ArenaPush(&Shared->Arena, LaneCount*sizeof(f_wave_info));
	pthread_t *Waves = (pthread_t*)F_ArenaPush(&Shared->Arena, LaneCount*sizeof(pthread_t));

	f_barrier Barrier = {0};

	Shared->LaneCount = LaneCount;
	Barrier.LaneCount = LaneCount;
	for (f_s64 Lane = 0; Lane < LaneCount; Lane++)
	{
		Entries[Lane].Shared = Shared;
		Entries[Lane].Lane = Lane;
		Entries[Lane].Shared->Barrier = &Barrier;
		assert(pthread_create(Waves + Lane, F_Null, F_Entry, Entries + Lane) == 0);
	}

	for (f_s64 Lane = 0; Lane < LaneCount; Lane++)
	{
		pthread_join(Waves[Lane], F_Null);
	}
}

f_b32 F_WaveIsFirstLane(f_wave_info *WaveInfo)
{
	return WaveInfo->Lane == 0;
}

f_u8 *F_ArenaPushTransient(f_memory_arena *Arena, f_s64 Bytes);

void F_BroadcastBytes(f_wave_info *WaveInfo, void *Bytes, f_s64 ByteCount)
{
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	f_u8 *Result = F_Null;
	if(F_WaveIsFirstLane(WaveInfo))
	{
		Result = F_ArenaPushTransient(&WaveInfo->Shared->Arena, ByteCount);
		F_MemoryCopy(Result, Bytes, ByteCount);
	}
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	Result = (f_u8*)(WaveInfo->Shared->Arena.Start + WaveInfo->Shared->Arena.At);
	F_MemoryCopy(Bytes, Result, ByteCount);
	F_WaveBarrier(WaveInfo->Shared->Barrier);
}

#define F_BroadcastToAllLanes(WaveInfo, Variable) F_BroadcastBytes(WaveInfo, &Variable, sizeof(Variable))

#define F_SharedBetweenLanes(WaveInfo, Variable)

f_s64 F_WaveGetLaneIndex(f_wave_info *WaveInfo)
{
	return WaveInfo->Lane;
}
/*
This could be faster probably but it's fine and works,
I don't own a threadripper so this will be fine for
32 core computers...

The following link shows a faster version that would work better.

GPU Gems 3
https://www.youtube.com/watch?v=1G8CZioSjnM
*/
f_u64 F_InclusiveWavePrefixSum(f_wave_info *WaveInfo, f_u64 Value)
{
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	WaveInfo->Shared->Data[WaveInfo->Lane] = Value;
	F_WaveBarrier(WaveInfo->Shared->Barrier);

	int PerLaneSum = 0;
	for(int I = 0; I <= WaveInfo->Lane; I++)
	{
		PerLaneSum += WaveInfo->Shared->Data[I];
	}

	F_WaveBarrier(WaveInfo->Shared->Barrier);
	return PerLaneSum;
}

f_u64 F_ExclusiveWavePrefixSum(f_wave_info *WaveInfo, f_u64 Value)
{
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	WaveInfo->Shared->Data[WaveInfo->Lane] = Value;
	F_WaveBarrier(WaveInfo->Shared->Barrier);

	f_u64 PerLaneSum = 0;
	for(int I = 0; I < WaveInfo->Lane; I++)
	{
		PerLaneSum += WaveInfo->Shared->Data[I];
	}
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	return PerLaneSum;
}

f_b32 F_WaveAnyTrue(f_wave_info *WaveInfo, f_b32 Expression)
{
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	atomic_store_explicit(WaveInfo->Shared->Data + 0, 0, memory_order_release);
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	atomic_fetch_add_explicit(WaveInfo->Shared->Data + 0, Expression, memory_order_consume);
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	return atomic_load_explicit(WaveInfo->Shared->Data + 0, memory_order_acquire) > 0;
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define F_MemoryCommit(Address, NumberOfBytes) VirtualAlloc((void*)(Address), NumberOfBytes, MEM_COMMIT, PAGE_READWRITE)
#define F_MemoryReserve(NumberOfBytes) (f_u8*)VirtualAlloc((void*)0, NumberOfBytes, MEM_RESERVE, PAGE_READWRITE)

#define F_MemoryCopy(Destination, Source, NumberOfBytes) RtlCopyMemory(Destination, Source, NumberOfBytes)
#define F_MemoryZero(Destination, NumberOfBytes) RtlZeroMemory(Destination, NumberOfBytes)

f_operating_system F_GetOperatingSystem()
{
	return F_OS_WINDOWS;
}

f_u64 F_GetPageSize()
{
	SYSTEM_INFO SystemInfo = {0};
	GetSystemInfo(&SystemInfo);

	return SystemInfo.dwPageSize;
}

f_str8 F_LoadEntireFileIntoMemory(f_memory_arena *Arena, f_str8 Filename)
{
	HANDLE FileHandle = CreateFileA(F_CStrFromStr8(Arena, Filename),
					GENERIC_READ,
					FILE_SHARE_READ,
					F_Null,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					F_Null);

	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		printf("File: %.*s not found\n", F_Str8ToPrintfArgs(Filename));
		return (f_str8){0};
	}

	LARGE_INTEGER FileSize;
	DWORD BytesRead;
	GetFileSizeEx(FileHandle, &FileSize);

	f_u8* BackingMemory = F_ArenaPush(Arena, (f_u64)FileSize.QuadPart+1);
	BackingMemory[FileSize.QuadPart] = 0;

	f_str8 Result = {0};
	Result.Bytes = (f_s8*)BackingMemory;
	Result.Size = FileSize.QuadPart;

	ReadFile(FileHandle, BackingMemory, FileSize.QuadPart, &BytesRead, F_Null);

	CloseHandle(FileHandle);

	return Result;
}

void F_WriteStringToDisk(f_memory_arena *Arena, f_str8 DestinationPath, f_str8 StringToWrite)
{
	HANDLE FileHandle = CreateFileA(F_CStrFromStr8(Arena, DestinationPath),
					GENERIC_WRITE,
					0,
					F_Null,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					F_Null);

	DWORD BytesWritten = 0;
	WriteFile(FileHandle, StringToWrite.Bytes, StringToWrite.Size, &BytesWritten, F_Null);
	CloseHandle(FileHandle);
}

void F_AppendSourceFilesInFolder(f_memory_arena *Arena, f_str8_list *SourceFilesFromFolder, f_str8 Path)
{
	f_str8 PathWithWildcard = F_Str8PushF(Arena, "%.*s", F_Str8ToPrintfArgs(Path));
	f_str8_list Entries = F_ListOfDirectoryEntriesFromPath(Arena, PathWithWildcard);
	for(f_str8_list_node *Entry = Entries.First; Entry != F_Null; Entry = Entry->Next)
	{
		if(F_Str8EndsWith(Entry->String, F_Str8(".c")))
		{
			f_str8 SourceFilepath = F_Str8PushF(Arena, "%.*s/%.*s",
											F_Str8ToPrintfArgs(Path),
											F_Str8ToPrintfArgs(Entry->String));
			F_Str8ListAppend(Arena, SourceFilesFromFolder, SourceFilepath);
		}
	}
}

f_str8 F_GetExecutableDirectory(f_memory_arena *Arena)
{
	DWORD NumberOfBytes = GetCurrentDirectoryA(0, 0);
	f_u8* BackingMemory = F_ArenaPush(Arena, NumberOfBytes);

	GetCurrentDirectoryA(NumberOfBytes, (LPSTR)BackingMemory);

	f_str8 Result = {0};
	Result.Size = NumberOfBytes;
	Result.Bytes = (f_s8*)BackingMemory;

	return Result;
}

f_b32 F_PathExists(f_memory_arena *Arena, f_str8 Filename)
{
	DWORD FileAttributes = GetFileAttributesA((LPCSTR)F_CStrFromStr8(Arena, Filename));

	return FileAttributes != INVALID_FILE_ATTRIBUTES && !(FileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

f_s64 F_AtomicIncrement(f_s64 volatile *Addend, f_s64 Value)
{
	return InterlockedExchangeAdd64(Addend, Value);
}

f_u64 F_TimeStampNow()
{
	LARGE_INTEGER Stamp = {0};
	QueryPerformanceCounter(&Stamp);
	return Stamp.QuadPart;
}

f_f32 F_MilliSecondsFromTimeStamp(f_u64 TimeStamp)
{
	LARGE_INTEGER Frequency = {0};
	QueryPerformanceFrequency(&Frequency);

	return (f_f32)(1000.0*(TimeStamp) / Frequency.QuadPart);

}

f_s64 F_GetNumberOfCPUs()
{
	SYSTEM_INFO SystemInfo = {0};
	GetSystemInfo(&SystemInfo);

	return SystemInfo.dwNumberOfProcessors;
}

void F_DeleteFileFromDisk(f_memory_arena *Arena, f_str8 Filename)
{
	DeleteFileA(F_CStrFromStr8(Arena, Filename));
}

f_b32 F_ExecuteCommand(f_memory_arena *Arena, f_str8 EntireCommandAndArguments, f_b32 *FailedToExecute)
{
	STARTUPINFOA StartupInfo = {0};
	PROCESS_INFORMATION ProcessInfo = {0};

	SECURITY_ATTRIBUTES SecurityAttributes = {0};
	SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	SecurityAttributes.bInheritHandle = F_True;

	HANDLE StdOutHandleRead = 0;
	HANDLE StdOutHandleWrite = 0;

	CreatePipe(&StdOutHandleRead, &StdOutHandleWrite, &SecurityAttributes, 0);

	f_b32 ProcessStatus = CreateProcessA(0, F_CStrFromStr8(Arena, EntireCommandAndArguments),
									   0, 0, 1, 0, 0, 0, &StartupInfo, &ProcessInfo);
	if (ProcessStatus)
	{

		WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

		DWORD ExitCode;
		GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
		CloseHandle(ProcessInfo.hProcess);
		CloseHandle(ProcessInfo.hThread);
		CloseHandle(StdOutHandleWrite);

		FailedToExecute[0] = ExitCode != 0;
		DWORD NumberOfBytesRead = 0;
		f_str8_list StdOutList = {0};
		for (;;)
		{
			f_u8* BackingMemory = F_ArenaPush(Arena, 4096);
			BOOL Success = ReadFile(StdOutHandleRead, BackingMemory, 4096, &NumberOfBytesRead, NULL);
			if (!Success || NumberOfBytesRead == 0)
			{
				break;
			}
			f_str8 String = {.Bytes = (f_s8*)BackingMemory, .Size = NumberOfBytesRead };
			F_Str8ListAppend(Arena, &StdOutList, String);
		}
		f_str8 OutputString = F_Str8ListJoin(Arena, StdOutList);
		printf("%.*s", F_Str8ToPrintfArgs(OutputString));
	}

	return ProcessStatus;
}

f_str8 F_CompileCommandFromBuildOptions(f_memory_arena *Arena, f_build_options *BuildOptions, f_str8 HashedFilename, f_str8 SourceFilename)
{
	f_str8 DebugOrRelease[2] =
	{
		F_Str8("/Zi /Od"), F_Str8("/O2")
	};

	f_str8 Compilers[3] =
	{
		F_Str8("cl.exe"),
		F_Str8("clang-cl"),
		F_Str8("clang-cl") // F_COMPILER_CARNAGE: glep is mac-only, so fall back to clang-cl.
	};

	f_str8_list IncludePathsWithFlag = {0};
	for (f_str8_list_node *Node = BuildOptions->IncludePaths.First; Node != F_Null; Node = Node->Next)
	{
		F_Str8ListAppend(Arena, &IncludePathsWithFlag, F_Str8PushF(Arena, "-I%.*s ", F_Str8ToPrintfArgs(Node->String)));
	}

	f_str8 CompileCommand = F_Str8PushF(
		Arena,
		"%.*s /c /TC %.*s /nologo /FS /EHsc %.*s %.*s /Fo%.*s%.*s %.*s ",
		F_Str8ToPrintfArgs(Compilers[BuildOptions->Compiler]),
		F_Str8ToPrintfArgs(F_Str8ListJoin(Arena, BuildOptions->ExtraCompileFlags)),
		F_Str8ToPrintfArgs(SourceFilename),
		F_Str8ToPrintfArgs(DebugOrRelease[BuildOptions->Optimise]),
		F_Str8ToPrintfArgs(BuildOptions->OutputPath),
		F_Str8ToPrintfArgs(HashedFilename),
		F_Str8ToPrintfArgs(F_Str8ListJoin(Arena, IncludePathsWithFlag)));

	return CompileCommand;
}

f_str8 F_LinkCommandFromBuildOptions(f_memory_arena *Arena, f_build_options *BuildOptions, f_str8_array ObjectFiles)
{
	f_str8 LinkCommand = F_Str8PushF(
		Arena,
		"link.exe /OPT:NOREF /OPT:NOICF /nologo /INCREMENTAL:NO /DEBUG %.*s/PDB:build/%.*s.pdb /OUT:%.*s%.*s.exe %.*s",
		F_Str8ToPrintfArgs(F_Str8ListJoin(Arena, BuildOptions->ExtraLinkFlags)),
		F_Str8ToPrintfArgs(BuildOptions->OutputName),
		F_Str8ToPrintfArgs(BuildOptions->OutputPath),
		F_Str8ToPrintfArgs(BuildOptions->OutputName),
		F_Str8ToPrintfArgs(F_Str8ArrayJoin(Arena, ObjectFiles))
	);

	return LinkCommand;
}

#define F_Entry(Data) DWORD WINAPI F_Entry(void *Data)

F_Entry(Data);

typedef struct f_shared_wave_info
{
	f_s32 ArgCount;
	char **Args;
	RTL_BARRIER *Barrier;

	f_s64 LaneCount;

	f_memory_arena Arena;
	f_build_options BuildOptions;


	atomic_llong Data[4096];
}
f_shared_wave_info;

typedef struct f_wave_info
{
	f_shared_wave_info *Shared;
	f_s32 Lane;
}
f_wave_info;

void F_WaveBarrier(RTL_BARRIER *Barrier)
{
	EnterSynchronizationBarrier(Barrier, SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
}

f_s64 F_LaneValueFromTotal(f_wave_info *WaveInfo, f_s64 Count)
{
	f_s64 Base = Count / WaveInfo->Shared->LaneCount;
	f_s64 Remainder = Count % WaveInfo->Shared->LaneCount;
	f_s64 Result = Count / WaveInfo->Shared->LaneCount + (WaveInfo->Lane < Remainder ? 1 : 0);

	if (Count < WaveInfo->Shared->LaneCount)
	{
		Result = (f_s64)(WaveInfo->Lane < Count);
	}

	return Result;
}

void F_Dispatch(f_shared_wave_info *Shared, f_s64 LaneCount)
{
	f_wave_info *Entries = (f_wave_info*)F_ArenaPush(&Shared->Arena, LaneCount*sizeof(f_wave_info));
	HANDLE *Waves = (HANDLE*)F_ArenaPush(&Shared->Arena, LaneCount*sizeof(HANDLE));

	RTL_BARRIER Barrier;

	Shared->LaneCount = LaneCount;
	InitializeSynchronizationBarrier(&Barrier, LaneCount, -1);
	for (f_s64 Lane = 0; Lane < LaneCount; Lane++)
	{
		Entries[Lane].Shared = Shared;
		Entries[Lane].Lane = Lane;
		Entries[Lane].Shared->Barrier = &Barrier;
		DWORD ThreadID;
		Waves[Lane] = CreateThread(0, 0, F_Entry, Entries + Lane, 0, &ThreadID);
	}

	WaitForMultipleObjects(LaneCount, Waves, F_True, INFINITE);
}

f_b32 F_WaveIsFirstLane(f_wave_info *WaveInfo)
{
	return WaveInfo->Lane == 0;
}

f_u8 *F_ArenaPushTransient(f_memory_arena *Arena, f_s64 Bytes);

void F_BroadcastBytes(f_wave_info *WaveInfo, void *Bytes, f_s64 ByteCount)
{
	f_u8 *Result = F_Null;
	if(F_WaveIsFirstLane(WaveInfo))
	{
		Result = F_ArenaPushTransient(&WaveInfo->Shared->Arena, ByteCount);
		F_MemoryCopy(Result, Bytes, ByteCount);
	}
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	Result = (f_u8*)(WaveInfo->Shared->Arena.Start + WaveInfo->Shared->Arena.At);
	F_MemoryCopy(Bytes, Result, ByteCount);
	F_WaveBarrier(WaveInfo->Shared->Barrier);
}

#define F_BroadcastToAllLanes(WaveInfo, Variable) F_BroadcastBytes(WaveInfo, &Variable, sizeof(Variable))

#define F_SharedBetweenLanes(WaveInfo, Variable)

f_s64 F_WaveGetLaneIndex(f_wave_info *WaveInfo)
{
	return WaveInfo->Lane;
}

/*
GPU Gems 3
https://www.youtube.com/watch?v=1G8CZioSjnM
*/
f_u64 F_InclusiveWavePrefixSum(f_wave_info *WaveInfo, f_u64 Value)
{
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	WaveInfo->Shared->Data[WaveInfo->Lane] = Value;
	F_WaveBarrier(WaveInfo->Shared->Barrier);

	int PerLaneSum = 0;
	for(int I = 0; I <= WaveInfo->Lane; I++)
	{
		PerLaneSum += WaveInfo->Shared->Data[I];
	}

	F_WaveBarrier(WaveInfo->Shared->Barrier);
	return PerLaneSum;
}

f_u64 F_ExclusiveWavePrefixSum(f_wave_info *WaveInfo, f_u64 Value)
{
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	WaveInfo->Shared->Data[WaveInfo->Lane] = Value;
	F_WaveBarrier(WaveInfo->Shared->Barrier);

	f_u64 PerLaneSum = 0;
	for(int I = 0; I < WaveInfo->Lane; I++)
	{
		PerLaneSum += WaveInfo->Shared->Data[I];
	}
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	return PerLaneSum;
}

f_b32 F_WaveAnyTrue(f_wave_info *WaveInfo, f_b32 Expression)
{
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	atomic_store_explicit(WaveInfo->Shared->Data + 0, 0, memory_order_release);
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	atomic_fetch_add_explicit(WaveInfo->Shared->Data + 0, Expression, memory_order_consume);
	F_WaveBarrier(WaveInfo->Shared->Barrier);
	return atomic_load_explicit(WaveInfo->Shared->Data + 0, memory_order_acquire) > 0;
}

f_str8_list F_ListOfDirectoryEntriesFromPath(f_memory_arena *Arena, f_str8 Path)
{
	f_str8_list Entries = { 0 };

	WIN32_FIND_DATAA FindData = {0};
	f_str8 Directory = F_Str8PushF(Arena, "%.*s\\*", F_Str8ToPrintfArgs(Path));
	HANDLE FindHandle = FindFirstFileA(F_CStrFromStr8(Arena, Directory), &FindData);

	f_b32 NextFile = FindHandle != INVALID_HANDLE_VALUE;
	for(;NextFile != F_False; NextFile = FindNextFileA(FindHandle, &FindData))
	{
		if(FindData.dwFileAttributes != INVALID_FILE_ATTRIBUTES && !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			F_Str8ListAppend(Arena, &Entries, F_Str8PushF(Arena, "%s", FindData.cFileName));
		}
	}
	return Entries;
}

int F_MoveFileOnDisk(f_memory_arena *Arena, f_str8 Source, f_str8 Destination)
{
	return MoveFileExA(F_CStrFromStr8(Arena, Source),
					   F_CStrFromStr8(Arena, Destination),
					   MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
}
#endif

f_u8* F_ArenaPush(f_memory_arena *Arena, f_s64 Bytes)
{
	Arena->At = (Arena->At + 15) & ~(f_s64)15;

	if(Arena->At + Bytes >= Arena->Committed)
	{
		f_s64 BytesNeeded = (Arena->At + Bytes - Arena->Committed);
		f_u64 BytesToCommit = BytesNeeded + F_GetPageSize() - (BytesNeeded % F_GetPageSize());
		F_MemoryCommit(Arena->Start + Arena->Committed, BytesToCommit);
		Arena->Committed += BytesToCommit;
	}
	f_u8* Memory = Arena->Start + Arena->At;
	Arena->At += Bytes;
	F_MemoryZero(Memory, Bytes);

	return Memory;
}

f_u8 *F_ArenaPushTransient(f_memory_arena *Arena, f_s64 Bytes)
{
	if(Arena->At + Bytes >= Arena->Committed)
	{
		f_s64 BytesNeeded = (Arena->At + Bytes - Arena->Committed);
		f_u64 BytesToCommit = BytesNeeded + F_GetPageSize() - (BytesNeeded % F_GetPageSize());
		F_MemoryCommit(Arena->Start + Arena->Committed, BytesToCommit);
		Arena->Committed += BytesToCommit;
	}
	f_u8* Memory = Arena->Start + Arena->At;

	return Memory;
}

f_str8 F_Str8PushFV(f_memory_arena *Arena, char *Format, va_list InputArgList)
{
	va_list ArgList;
	va_copy(ArgList, InputArgList);
	f_u32 BytesNeeded = vsnprintf(0, 0, Format, InputArgList) +1;
	f_str8 Result = {0};
	Result.Bytes = (f_s8*)F_ArenaPush(Arena, BytesNeeded);
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

char *F_CStrFromStr8(f_memory_arena *Arena, f_str8 String)
{
	char *Result = (char *)F_ArenaPush(Arena, String.Size + 1);
	F_MemoryCopy(Result, String.Bytes, String.Size);
	return Result;
}

f_b32 F_Str8Equal(f_str8 A, f_str8 B)
{
	if (A.Size != B.Size)
		return F_False;
	for (f_u64 Index = 0; Index < A.Size; Index++)
	{
		if (A.Bytes[Index] != B.Bytes[Index])
		{
			return F_False;
		}
	}
	return F_True;
}

f_b32  F_Str8EndsWith(f_str8 String, f_str8 EndsWith)
{
	f_str8 SubString = String;
	SubString.Bytes += String.Size - EndsWith.Size;
	SubString.Size = EndsWith.Size;
	return F_Str8Equal(SubString, EndsWith);
}

void F_Str8ListAppend(f_memory_arena *Arena, f_str8_list *List, f_str8 String)
{
	f_str8_list_node *Node = F_ArenaPushType(Arena, f_str8_list_node);
	Node->String = F_Str8PushF(Arena, "%.*s", F_Str8ToPrintfArgs(String));
	if (List->First == F_Null)
	{
		List->First = Node;
		List->Last = List->First;
	}
	else
	{
		List->Last->Next = Node;
		Node->Previous = List->Last;
		List->Last = Node;
	}
}

f_str8 F_Str8ListJoin(f_memory_arena *Arena, f_str8_list List)
{
	f_u64 BytesToAllocate = 0;
	for (f_str8_list_node *Node = List.First; Node != F_Null; Node = Node->Next)
	{
		BytesToAllocate += Node->String.Size;
	}
	f_u8 *Bytes = F_ArenaPush(Arena, BytesToAllocate);
	f_u64 At = 0;
	for (f_str8_list_node *Node = List.First; Node != F_Null; Node = Node->Next)
	{
		F_MemoryCopy(Bytes + At, Node->String.Bytes, Node->String.Size);
		At += Node->String.Size;
	}
	f_str8 Result = {.Bytes = (f_s8*)Bytes, .Size = BytesToAllocate};

	return Result;
}

f_str8 F_Str8ArrayJoin(f_memory_arena *Arena, f_str8_array Array)
{
	f_u64 BytesToAllocate = 0;
	for (int I = 0; I < Array.Count; I++)
	{
		BytesToAllocate += Array.Items[I].Size;
	}
	f_u8 *Bytes = F_ArenaPush(Arena, BytesToAllocate);
	f_u64 At = 0;
	for (int I = 0; I < Array.Count; I++)
	{
		F_MemoryCopy(Bytes + At, Array.Items[I].Bytes, Array.Items[I].Size);
		At += Array.Items[I].Size;
	}
	f_str8 Result = {.Bytes = (f_s8*)Bytes, .Size = BytesToAllocate};

	return Result;
}

f_str8_array F_Str8ArrayFromList(f_memory_arena *Arena, f_str8_list List)
{
	f_str8_array Result = {0};
	f_s64 Count = 0;
	for(f_str8_list_node *Entry = List.First; Entry != F_Null; Entry = Entry->Next)
	{
		Count++;
	}
	Result.Count = Count;
	Result.Items = (f_str8*)F_ArenaPush(Arena, sizeof(f_str8)*Count);
	f_s64 Index = 0;
	for(f_str8_list_node *Entry = List.First; Entry != F_Null; Entry = Entry->Next)
	{
		Result.Items[Index++] = Entry->String;
	}
	return Result;
}

#define F_FNV_64_PRIME ((f_u64)0x100000001b3ULL)
#define F_FNV_HASH_START ((f_u64)0xcbf29ce484222325ULL)
void F_FNVHash64(f_u64 *Hash, f_str8 String)
{
	for (f_s64 Index = 0; Index < String.Size; Index++)
	{

		*Hash *= F_FNV_64_PRIME;
		*Hash ^= (f_u64)String.Bytes[Index];
	}
}

f_b32 F_AdvanceN(f_str8 File, f_u64 *At, f_u64 N)
{
	if (*At + N <= File.Size)
	{
		*At += N;
		return F_True;
	}
	return F_False;
}

void F_GobbleWhitespace(f_str8 File, f_u64 *At)
{
	while (*At < File.Size)
	{
		f_u64 Byte = File.Bytes[*At];
		switch (Byte)
		{
		case ' ':
		case '\r':
		case '\t':
		case '\n':
		{
			if(F_AdvanceN(File, At, 1) == F_False)
			{
				return;
			}
		}
		break;
		default:
		{
			return;
		}
		break;
		}
	}
}

void F_GobbleUpToWhitespace(f_str8 File, f_u64 *At)
{
	while (*At < File.Size)
	{
		f_u64 Byte = File.Bytes[*At];
		switch (Byte)
		{
		case ' ':
		case '\r':
		case '\t':
		case '\n':
		{
			return;
		}
		break;
		default:
		{
			if(F_AdvanceN(File, At, 1) == F_False)
			{
				return;
			}
		}
		break;
		}
	}
}

f_str8 F_Str8UpToLastCharacter(f_memory_arena *Arena, f_str8 InputPath, f_u8 Character)
{
	f_s64 SizeUptoLastCharacter = InputPath.Size - 1;
	f_b32 FoundCharacter = F_False;
	for (; SizeUptoLastCharacter >= 0; SizeUptoLastCharacter--)
	{
		if (InputPath.Bytes[SizeUptoLastCharacter] == Character)
		{
			FoundCharacter = F_True;
			break;
		}
	}
	f_str8 Path = {0};
	if (FoundCharacter)
	{
		Path.Size = SizeUptoLastCharacter;
		Path.Bytes = (f_s8*)F_ArenaPush(Arena, SizeUptoLastCharacter);
		F_MemoryCopy(Path.Bytes, InputPath.Bytes, SizeUptoLastCharacter);
	}
	return Path;
}

f_str8 F_Str8FromLastCharacter(f_memory_arena *Arena, f_str8 InputPath, f_u8 Character)
{
	f_s64 StartIndex = 0;
	f_b32 FoundCharacter = F_False;
	for (int I = InputPath.Size; I > 0; I--)
	{
		if (InputPath.Bytes[I-1] == Character)
		{
			FoundCharacter = F_True;
			StartIndex = I;
			break;
		}
	}

	f_str8 Result = {0};
	if (FoundCharacter)
	{
		Result.Size = InputPath.Size - StartIndex;
		Result.Bytes = (f_s8*)F_ArenaPush(Arena, Result.Size);
		F_MemoryCopy(Result.Bytes, InputPath.Bytes + StartIndex, Result.Size);
	}
	return Result;
}


void F_FindAllIncludesInFile(f_memory_arena *Arena, f_str8 Filename, f_str8_list IncludePaths, f_str8_list *IncludeChainFiles)
{
	// Support unicode...

	f_str8 CurrentFile = F_LoadEntireFileIntoMemory(Arena, Filename);
	F_Str8ListAppend(Arena, IncludeChainFiles, CurrentFile);

	int CommentNesting = 0;
	for (f_u64 At = 0; At < CurrentFile.Size; At++)
	{
		f_u8 Byte = CurrentFile.Bytes[At];
		f_u8 NextNonWhitespaceByte = 0;
		f_u64 NextAt = At;
		F_GobbleWhitespace(CurrentFile, &NextAt);
		NextNonWhitespaceByte = CurrentFile.Bytes[NextAt+1];

		if(Byte == '/' && ((NextAt > At && (NextNonWhitespaceByte == '/') || NextNonWhitespaceByte == '*')))
		{
			CommentNesting += 1;
		}

		if(CommentNesting > 0)
		{
			if((Byte == '*' && ((NextAt > At && (NextNonWhitespaceByte == '/') || Byte == '\n'))))
			{
				CommentNesting -= 1;
			}
		}
		else if (Byte == '#')
		{
			if (F_AdvanceN(CurrentFile, &At, 1) == F_False)
			{
				return;
			}
			f_str8 Token = {0};
			Token.Bytes = CurrentFile.Bytes + At;
			f_str8 IncludeToken = F_Str8("include");
			Token.Size = IncludeToken.Size;
			if (At + Token.Size < CurrentFile.Size)
			{
				if (F_Str8Equal(Token, IncludeToken))
				{
					F_AdvanceN(CurrentFile, &At, Token.Size);
					F_GobbleWhitespace(CurrentFile, &At);
					if (CurrentFile.Bytes[At] == '"')
					{
						if (F_AdvanceN(CurrentFile, &At, 1))
						{
							f_str8 IncludePath = {0};
							IncludePath.Bytes = CurrentFile.Bytes + At;
							for (; F_AdvanceN(CurrentFile, &At, 1) && CurrentFile.Bytes[At] != '"';);
							IncludePath.Size = CurrentFile.Bytes + At - IncludePath.Bytes;
							f_str8 SourceDir = F_Str8UpToLastCharacter(Arena, Filename, '/');
							f_str8 SourceDirPlusIncludePath = F_Str8PushF(Arena, "%.*s/%.*s", F_Str8ToPrintfArgs(SourceDir), F_Str8ToPrintfArgs(IncludePath));
							f_b32 FoundFile = F_False;
							f_str8_list AttemptedIncludes = {0};
							if (F_PathExists(Arena, SourceDirPlusIncludePath))
							{
								FoundFile = F_True;
								F_FindAllIncludesInFile(Arena, SourceDirPlusIncludePath, IncludePaths, IncludeChainFiles);
							}
							else
							{
								for (f_str8_list_node *Node = IncludePaths.First; Node != 0; Node = Node->Next)
								{
									f_str8 FullPath = F_Str8PushF(
										Arena,
										"%.*s/%.*s",
										F_Str8ToPrintfArgs(F_Str8UpToLastCharacter(Arena, Node->String, '/')),
										F_Str8ToPrintfArgs(IncludePath));

									if(F_PathExists(Arena, FullPath))
									{
										FoundFile = F_True;
										F_FindAllIncludesInFile(Arena, FullPath, IncludePaths, IncludeChainFiles);
										break;
									}
									else
									{
										F_Str8ListAppend(Arena, &AttemptedIncludes, FullPath);
									}
								}
							}

							if (FoundFile == F_False)
							{
								return;
							}
						}
					}
				}
			}
		}
	}
}

void F_GenerateEmbedHeader(f_memory_arena *Arena, f_str8 DestinationPath)
{
	f_str8 FilePath = F_Str8PushF(Arena, "%.*s/embed.h", F_Str8ToPrintfArgs(DestinationPath));
	F_WriteStringToDisk(Arena, FilePath, F_Str8("#pragma once \n typedef struct embed_data { unsigned char *Data; unsigned long long Length; } embed_data;"));
}

void F_EmbedFiles(f_memory_arena *Arena, f_str8_list SourcePaths, f_str8 DestinationPath, f_str8_list Names)
{
	f_str8_list Embed = {0};
	f_str8_list_node *Name = Names.First;

	f_str8 HeaderPath = F_Str8PushF(Arena, "%.*s.h", F_Str8ToPrintfArgs(F_Str8UpToLastCharacter(Arena, DestinationPath, '.')));

	F_Str8ListAppend(Arena, &Embed, F_Str8("#pragma once\n #include \"embed.h\" \n"));

	for(f_str8_list_node *SourcePath = SourcePaths.First;
		SourcePath != F_Null;
		SourcePath = SourcePath->Next)
	{
		F_Str8ListAppend(Arena, &Embed, F_Str8PushF(Arena, "extern embed_data %.*s;\n", F_Str8ToPrintfArgs(Name->String)));
		Name = Name->Next;
	}

	F_WriteStringToDisk(Arena, HeaderPath, F_Str8ListJoin(Arena, Embed));

	Embed = (f_str8_list){0};
	Name = Names.First;

	F_Str8ListAppend(Arena, &Embed, F_Str8PushF(Arena, "#include \"%.*s\"\n", F_Str8ToPrintfArgs(F_Str8FromLastCharacter(Arena, HeaderPath, '/'))));

	for(f_str8_list_node *SourcePath = SourcePaths.First;
		SourcePath != F_Null;
		SourcePath = SourcePath->Next)
	{
		if(!Name)
		{
			printf(F_ERROR "Number of names do not match number files.\n\n");
			return;
		}
		f_str8 File = F_LoadEntireFileIntoMemory(Arena, SourcePath->String);
		F_Str8ListAppend(Arena, &Embed, F_Str8PushF(Arena, "static unsigned char %.*sData[] =\n{\n", F_Str8ToPrintfArgs(Name->String)));
		for(f_s64 At = 0; At < File.Size;)
		{
			f_u8* Byte = (f_u8*)(File.Bytes + At);
			if(At + 16 <= File.Size)
			{
				f_str8 Elements = F_Str8PushF(Arena,
					"    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n"
					"    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n"
					"    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n"
					"    0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
					Byte[0],  Byte[1],  Byte[2],  Byte[3],
					Byte[4],  Byte[5],  Byte[6],  Byte[7],
					Byte[8],  Byte[9],  Byte[10], Byte[11],
					Byte[12], Byte[13], Byte[14], Byte[15]
				);
				F_Str8ListAppend(Arena, &Embed, Elements);
				At += 16;
			}
			else
			{
				f_str8 Element = {0};
				if(At % 4 == 0)
				{
					Element = F_Str8PushF(Arena, "    0x%02x,", Byte[0]);
				}
				else
				{
					if((At % 4) % 3 == 0)
					{
						Element = F_Str8PushF(Arena, "0x%02x,\n", Byte[0]);
					}
					else
					{
						Element = F_Str8PushF(Arena, "0x%02x, ", Byte[0]);
					}
				}
				F_Str8ListAppend(Arena, &Embed, Element);
				At += 1;
			}
		}
		F_Str8ListAppend(Arena, &Embed, F_Str8("\n};\n\n"));
		F_Str8ListAppend(Arena, &Embed, F_Str8PushF(Arena, "embed_data %.*s = {\n", F_Str8ToPrintfArgs(Name->String)));
		F_Str8ListAppend(Arena, &Embed, F_Str8PushF(Arena, "	.Data = %.*sData,\n", F_Str8ToPrintfArgs(Name->String)));
		F_Str8ListAppend(Arena, &Embed, F_Str8PushF(Arena, "	.Length = %llu,\n",  File.Size));
		F_Str8ListAppend(Arena, &Embed, F_Str8PushF(Arena, "};\n"));
		Name = Name->Next;
	}

	f_str8 JoinedEmbed = F_Str8ListJoin(Arena, Embed);
	F_WriteStringToDisk(Arena, DestinationPath, JoinedEmbed);
}

void F_EmbedFile(f_memory_arena *Arena, f_str8 SourcePath, f_str8 DestinationPath, f_str8 Name)
{
	f_str8_list SourcePaths = {0};
	F_Str8ListAppend(Arena, &SourcePaths, SourcePath);

	f_str8_list Names = {0};
	F_Str8ListAppend(Arena, &Names, Name);

	F_EmbedFiles(Arena, SourcePaths, DestinationPath, Names);
}

f_u64 F_HashSourceFileAndIncludes(f_memory_arena *Arena, f_str8 SourceFilename, f_str8_list IncludePaths, f_b32 Optimise, f_compiler Compiler)
{
	f_str8_list IncludeChainFiles = {0};
	F_FindAllIncludesInFile(Arena, SourceFilename, IncludePaths, &IncludeChainFiles);
	f_u64 Hash = F_FNV_HASH_START;
	F_FNVHash64(&Hash, SourceFilename);
	F_FNVHash64(&Hash, F_Str8PushF(Arena, "%d", Compiler));
	F_FNVHash64(&Hash, F_Str8PushF(Arena, "%d", Optimise));
	for (f_str8_list_node *File = IncludeChainFiles.First; File != 0; File = File->Next)
	{
		F_FNVHash64(&Hash, File->String);
	}

	return Hash;
}

f_b32 F_CompileTranslationUnit(f_memory_arena *Arena, f_build_options *BuildOptions, f_str8 SourceFilename, f_u64 Hash, f_b32 *OutCompiled)
{
	f_str8 HashedFilename = F_Str8PushF(Arena, "%llu.o", Hash);

	f_b32 FoundFile = F_PathExists(Arena, F_Str8PushF(Arena, "build/%.*s", F_Str8ToPrintfArgs(HashedFilename)));

	f_b32 FoundObjectFile = FoundFile;
	if (FoundObjectFile && !BuildOptions->FullRebuild)
	{
		return F_False;
	}

	*OutCompiled = F_True;

	f_b32 FailedToExecute = F_False;
	f_b32 FailedToBuild = F_False;
	f_str8 CompileCommand = F_CompileCommandFromBuildOptions(Arena, BuildOptions, HashedFilename, SourceFilename);
	if(F_ExecuteCommand(Arena, CompileCommand, &FailedToExecute) == F_False)
	{
		printf(F_ERROR "Failed to launch compiler\n");
		FailedToBuild = F_True;
	}

	return FailedToBuild || FailedToExecute;
}

f_b32 F_LinkObjectFiles(f_memory_arena *Arena, f_build_options *BuildOptions, f_str8_array ObjectFileNames)
{

	f_str8_array ObjectFilePaths = {0};
	ObjectFilePaths.Count = ObjectFileNames.Count;
	ObjectFilePaths.Items = (f_str8*)F_ArenaPush(Arena, ObjectFilePaths.Count*sizeof(f_str8));

	for (f_s64 I = 0; I < ObjectFilePaths.Count; I++)
	{
		ObjectFilePaths.Items[I] = F_Str8PushF(Arena, "%.*s%.*s ",
											 F_Str8ToPrintfArgs(BuildOptions->OutputPath),
											 F_Str8ToPrintfArgs(ObjectFileNames.Items[I]));
	}

	f_str8_list BuildDirectoryEntries = F_ListOfDirectoryEntriesFromPath(Arena, BuildOptions->OutputPath);
	for(f_str8_list_node *Entry = BuildDirectoryEntries.First; Entry != F_Null; Entry = Entry->Next)
	{
		if (F_Str8EndsWith(Entry->String, F_Str8(".o")))
		{
			f_b32 FoundMatch = F_False;
			f_str8 FilePathToCheck = F_Str8PushF(Arena, "%.*s%.*s", F_Str8ToPrintfArgs(BuildOptions->OutputPath),
																 F_Str8ToPrintfArgs(Entry->String));
			for (f_s64 J = 0; J < ObjectFileNames.Count; J++)
			{
				f_str8 ObjectFilePathWithoutSpace = ObjectFilePaths.Items[J];
				ObjectFilePathWithoutSpace.Size -= 1;
				if (F_Str8Equal(ObjectFilePathWithoutSpace, FilePathToCheck))
				{
					FoundMatch = F_True;
					break;
				}
			}
			if (FoundMatch == F_False)
			{
				F_DeleteFileFromDisk(Arena, FilePathToCheck);
			}
		}
	}

	f_str8 LinkCommand = F_LinkCommandFromBuildOptions(Arena, BuildOptions, ObjectFilePaths);

	f_b32 FailedToExecute = F_False;
	f_b32 FailedToLink = F_False;
	if(F_ExecuteCommand(Arena, LinkCommand, &FailedToExecute) == F_False)
	{
		printf(F_ERROR "Failed to launch linker\n");
		FailedToLink = F_True;
	}
	return FailedToLink || FailedToExecute;
}

f_b32 F_Build(f_wave_info *WaveInfo, f_memory_arena *Arena, f_build_options BuildOptions)
{
	f_u64 Start = F_TimeStampNow();

	f_str8_array RelativeSourceFilePaths = {0};
	f_str8_array ObjectFileNames = {0};
	if(F_WaveIsFirstLane(WaveInfo))
	{
		RelativeSourceFilePaths = F_Str8ArrayFromList(&WaveInfo->Shared->Arena, BuildOptions.SourceFiles);
		ObjectFileNames.Count = RelativeSourceFilePaths.Count;
		ObjectFileNames.Items = (f_str8*)F_ArenaPush(&WaveInfo->Shared->Arena, sizeof(f_str8)*RelativeSourceFilePaths.Count);
	}

	F_BroadcastToAllLanes(WaveInfo, RelativeSourceFilePaths);
	F_BroadcastToAllLanes(WaveInfo, ObjectFileNames);

	f_s64 Count = F_LaneValueFromTotal(WaveInfo, RelativeSourceFilePaths.Count);
	f_s64 Index = F_ExclusiveWavePrefixSum(WaveInfo, Count);

	f_b32 FailedToBuild = F_False;
	f_b32 CompiledAnything = F_False;
	f_u64 PerLaneCount = 0;
	for(int I = Index; I < Index + Count; I++)
	{
		PerLaneCount++;
		f_u64 PerFileStart = F_TimeStampNow();
		f_u64 Hash = F_HashSourceFileAndIncludes(Arena,
											  RelativeSourceFilePaths.Items[I],
											  BuildOptions.IncludePaths,
											  BuildOptions.Optimise,
											  BuildOptions.Compiler);
		f_b32 Compiled = F_False;
		f_b32 FailedToCompile = F_CompileTranslationUnit(Arena,
							   &BuildOptions,
								RelativeSourceFilePaths.Items[I],
								Hash,
								&Compiled);
		FailedToBuild |= FailedToCompile;
		CompiledAnything |= Compiled;

		ObjectFileNames.Items[I] = F_Str8PushF(Arena, "%llu.o", Hash);

		if(!FailedToCompile && Compiled)
		{
			printf("Compiled %.*s in %.2fms\n",
				   F_Str8ToPrintfArgs(RelativeSourceFilePaths.Items[I]),
				   F_MilliSecondsFromTimeStamp(F_TimeStampNow() - PerFileStart));
		}

	}

	f_b32 AnyFailedToBuild = F_WaveAnyTrue(WaveInfo, FailedToBuild);
	f_b32 AnyCompiled = F_WaveAnyTrue(WaveInfo, CompiledAnything);

	f_b32 FailedToLink = F_False;
	if(F_WaveIsFirstLane(WaveInfo) && !AnyFailedToBuild)
	{
		printf("\nCompiled all TUs in %.2fms\n", F_MilliSecondsFromTimeStamp(F_TimeStampNow() - Start));

		// Relinking rewrites the output even when nothing changed, which would
		// retrigger anything watching the binary's timestamp (see the hot
		// reload poll in main.c). Skip it when no TU was rebuilt this run.
		f_str8 OutputFilePath = F_Str8PushF(Arena, "%.*s%.*s",
										F_Str8ToPrintfArgs(BuildOptions.OutputPath),
										F_Str8ToPrintfArgs(BuildOptions.OutputName));
		if(AnyCompiled || !F_PathExists(Arena, OutputFilePath))
		{
			f_u64 LinkStart = F_TimeStampNow();

			FailedToLink = F_LinkObjectFiles(Arena, &BuildOptions, ObjectFileNames);
			printf("Linking finished in %.2fms\n", F_MilliSecondsFromTimeStamp(F_TimeStampNow() - LinkStart));
		}
		else
		{
			printf("%.*s already up to date\n", F_Str8ToPrintfArgs(BuildOptions.OutputName));
		}
	}

	f_b32 AnyFailedToLink = F_WaveAnyTrue(WaveInfo, FailedToLink);

	if(F_WaveIsFirstLane(WaveInfo) && (AnyFailedToBuild || AnyFailedToLink))
	{
		f_str8 OutputPathAndName = F_Str8PushF(Arena, "%.*s/%.*s",
								   F_Str8ToPrintfArgs(BuildOptions.OutputPath),
								   F_Str8ToPrintfArgs(BuildOptions.OutputName));
		F_DeleteFileFromDisk(Arena, OutputPathAndName);

		printf(F_ERROR);
	}
	else if(F_WaveIsFirstLane(WaveInfo))
	{

		printf(F_SUCCESS);
	}

	return (AnyFailedToBuild || AnyFailedToLink);

}

f_str8 F_ForwardSlashPathFromPath(f_str8 Path)
{
	for(f_s64 I = 0; I < Path.Size; I++)
	{
		if(Path.Bytes[I] == '\\')
		{
			Path.Bytes[I] = '/';
		}
	}
	return Path;
}

void F_EmitCompileCommandsJson(f_memory_arena *Arena, f_build_options BuildOptions)
{
	f_str8_list Output = {0};

	F_Str8ListAppend(Arena, &Output, F_Str8("[\n"));

	for(f_str8_list_node *SourceNode = BuildOptions.SourceFiles.First;
		SourceNode != F_Null;
		SourceNode = SourceNode->Next)
	{
		f_str8 FileName = F_ForwardSlashPathFromPath(SourceNode->String);
		f_str8 ObjectFileName = F_Str8PushF(Arena, "%.*s.o",
										F_Str8ToPrintfArgs(
										F_Str8UpToLastCharacter(Arena,
															  FileName,
															  '.')));
		F_Str8ListAppend(Arena, &Output, F_Str8("    {\n"));
		F_Str8ListAppend(Arena, &Output, F_Str8PushF(Arena,
			   "        \"directory\": \"%.*s\",\n",
					   F_Str8ToPrintfArgs(F_ForwardSlashPathFromPath(BuildOptions.WorkingDirectory))));
		F_Str8ListAppend(Arena,
				   &Output,
				   F_Str8PushF(Arena,
				   "        \"command\": \"%.*s\",\n",
				   F_Str8ToPrintfArgs(F_CompileCommandFromBuildOptions(Arena,
									&BuildOptions,
									ObjectFileName,
									FileName))));
		F_Str8ListAppend(Arena,
				 &Output,
			   F_Str8PushF(Arena,
			   "        \"file\": \"%.*s\"\n",
					   F_Str8ToPrintfArgs(FileName)));
		F_Str8ListAppend(Arena, &Output, F_Str8("    }"));
		if(SourceNode->Next)
		{
			F_Str8ListAppend(Arena, &Output, F_Str8(",\n"));
		}
		else
		{
			F_Str8ListAppend(Arena, &Output, F_Str8("\n"));
		}
	}

	F_Str8ListAppend(Arena, &Output, F_Str8("]\n"));

	F_WriteStringToDisk(Arena, F_Str8("compile_commands.json"), F_Str8ListJoin(Arena, Output));
}
