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

typedef struct f_translation_unit
{
	f_u64   FileCount;
	f_str8 *PathsForSourceAndIncludes;
	f_u64  *TimeStamps;
	f_process Process;
	f_u64   Hash;
}
f_translation_unit;

typedef struct f_build_watch
{
	f_translation_unit *TUs;
	f_u64               TUCount;

	_Atomic(f_s32)      TUsCompiling;
}
f_build_watch;

// TODO: Set up proper archiver flags since 
// msvc needs to use lib.exe to combine object files
// into static library
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

#define F_TOOLCHAIN_MSVC ((f_toolchain)\
{\
	.CompilerPath = F_Str8("cl"),\
	.OutputFlag = F_Str8("/OUT"),\
	.DebugFlag = F_Str8("/Zi"),\
	.OptimiseFlag = F_Str8("/O2"),\
	.IncludeFlag = F_Str8("/I"),\
	.DynamicFlag = F_Str8("/LD"),\
	.StaticFlag = F_Str8("/c /MT"),\
	.CompileOnlyFlag = F_Str8("/c")\
})

f_toolchain F_TOOLCHAIN_GCC;
f_toolchain F_TOOLCHAIN_CARNAGE;

typedef struct f_compile_options
{
	f_str8_array IncludePaths;
	f_str8 OutputPath;
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

typedef enum f_build_status
{
	F_BUILD_COMPILING,
	F_BUILD_COMPILE_FAILED,
	F_BUILD_LINKING,
	F_BUILD_SUCCEEDED,
	F_BUILD_FAILED,
}
f_build_status;


typedef struct f_build_cache
{
	f_u64 *Items;
	f_s64  Count;
}
f_build_cache;

typedef struct f_build_report
{
	f_b32  Succeeded;
	f_b32  Changed;
	f_str8 OutputPath;
	f_str8 Diagnostics;
	f_s64  TUsCompiled;
	f_f32  CompiledTimeInMilliseconds;
	f_f32  LinkedTimeInMilliseconds;
}
f_build_report;

typedef struct f_link_options
{
	f_str8 OutputPath;
	f_str8 OutputName;
	f_str8_array ExtraFlags;
	f_output_kind OutputKind;
}
f_link_options;

typedef enum f_compilation_status
{
	F_NOT_STARTED_COMPILING,
	F_NOTHING_TO_COMPILE,
	F_COMPILING,
	F_COMPILE_SUCCEEDED,
	F_COMPILE_FAILED,
}
f_compilation_status;

typedef struct f_build
{
	f_memory_arena     *Arena;
	f_translation_unit *TUs;
	f_u64               TUCount;

	f_str8_array        SourceDirectories;
	f_u64               PrevHashOfAllFiles;

	f_u64               TimeOfLastCheck;

	_Atomic(f_s64) TUsToProcess;
	_Atomic(f_s64) NextTU;
}
f_build;


typedef struct f_build_options
{
	f_output_kind  Kind;

	f_str8         OutputPathWithFilename;

	f_str8_array   SourceDirectories;
	f_str8_array   IncludeDirectories;

	f_str8_array   CompileFlags;
	f_str8_array   LinkFlags;

	f_b32          Debug;
	f_b32          Optimise;
	f_str8_array   Defines;
	f_str8_array   Libraries;
	f_str8_array   Frameworks;

	f_str8_array   CompilerSpecificFlags;
	f_toolchain    Toolchain;
}
f_build_options;


typedef struct f_compilation
{
	f_s32               Count;
	f_process          *Processes;
	f_str8             *SourcePaths;
	f_str8             *ObjectPaths;
	f_translation_unit *TUs;
	f_u64              *Hashes;
	f_u64               CompileOptionsHash;

	// HACK: we're gonna hijack this 
	// until we're sure of the API
	_Atomic(f_s64)     TUsCompiling;


	f_compilation_status Status;
}
f_compilation;

typedef enum f_link_status
{
	F_LINK_NOT_STARTED,
	F_LINKING,
	F_LINK_SUCCEEDED,
	F_LINK_FAILED,
}
f_link_status;

typedef struct f_link
{
	f_process Process;
	f_str8 OutputPath;

	f_link_status Status;
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
	void (*LaneEntry)();
	f_lane Lane;
	f_group Group;
}
f_thread_entry_params;

#define F_ArenaPushType(Arena, Type) (Type*)F_Raw_ArenaPush(Arena, sizeof(Type), F_True)
#define F_ArenaPushArrayType(Arena, Count, Type) (Type*)F_Raw_ArenaPush(Arena, Count * (sizeof(Type)), F_True)
#define F_ArenaPush(Arena, BytesToPush) F_Raw_ArenaPush(Arena, BytesToPush, F_True)

#define F_ArenaPushTypeNoZero(Arena, Type) (Type*)F_Raw_ArenaPush(Arena, sizeof(Type), F_False)
#define F_ArenaPushArrayTypeNoZero(Arena, Count, Type) (Type*)F_Raw_ArenaPush(Arena, Count * (sizeof(Type)), F_False)
#define F_ArenaPushNoZero(Arena, BytesToPush) F_Raw_ArenaPush(Arena, BytesToPush, F_False)

f_memory_arena *F_ArenaReserveButCommitFirstPage(f_s64 NumberOfBytesToReserve);
f_u8 *F_Raw_ArenaPush(f_memory_arena *Arena, f_s64 BytesToPush, f_b32 ShouldZero);
void  F_ArenaReset(f_memory_arena *Arena);

void  F_ArenaResetTo(f_memory_arena *Arena, f_s64 Mark);

#define F_ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define F_Str8ArrayFromCStrs(Arena, ...) \
F_Internal_Str8ArrayFromCStrs(Arena, ((char *[]){ __VA_ARGS__ }), ((sizeof(char *[]){ __VA_ARGS__})/sizeof(char *)))

#define F_Str8ArrayFromStr8s(Arena, ...) \
F_Internal_Str8ArrayFromStr8s(Arena, ((f_str8[]){ __VA_ARGS__ }), ((sizeof(f_str8[]){ __VA_ARGS__})/sizeof(f_str8)))

f_str8_array F_Internal_Str8Array(f_memory_arena *Arena, f_str8 *Str8s, f_u64 Str8Count);

#define F_Str8ListAppendF(Arena, List, ...) \
F_Str8ListAppend(Arena, List, F_Str8PushF(Arena, __VA_ARGS__))


char         *F_CStrFromStr8      (f_memory_arena *Arena, f_str8 String);
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


#define F_AppendErrorF(ErrorList, ...) F_Str8ListAppend(&F_ErrorArena, ErrorList, F_Str8PushF(&F_ErrorArena, __VA_ARGS__))

void F_DispatchAcrossNLanes (f_s64 LaneCount, char **Args, int ArgCount);

f_group F_GroupFromPartition (f_group ParentGroup, f_b32 Condition);

f_b32   F_IsCallerInGroup (f_group Group);

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

void  F_StartProcess             (f_memory_arena *Arena, f_process *Process, f_str8_list Args);
f_b32 F_PollProcess              (f_process *Process);
void  F_WaitForProcess           (f_process *Process);
void  F_WaitForMultipleProcesses (f_group Group, f_process *Processes, f_s32 ProcessCount);

f_b32 F_ProcessExitedCleanly(f_process *Process);

void F_Internal_WaitForMultipleProcesses (f_process **Processes, f_u64 ProcessCount);


#define F_DEFAULT_TIMEOUT_MS (16)

f_watch      F_WatchFilesFromArray    (f_str8_array FilePaths);

f_str8_array F_FilesChangedInWatch    (f_group Group, f_memory_arena *Arena, f_watch *Watch, f_u64 ChangeGranularityInMilliseconds);
f_b32        F_AnyFilesChangedInWatch (f_group Group, f_watch *Watch, f_u64 ChangeGranularityInMilliseconds);


f_b32 *F_MaskOfFilesChangedInTU (f_group Group, f_memory_arena *Arena, f_translation_unit *TU, f_u64 ChangeGranularityInMilliseconds);
f_b32 *F_MaskOfTUsChanged (f_group Group, f_memory_arena *Arena, f_build_watch BuildWatch, f_u64 ChangeGranularityInMilliseconds);


#define F_HashStart (0xCBF29CE484222325ULL)

f_u64 F_HashBytes(f_u64 HashStart, void *Data, f_u64 NumberOfBytesToHash);

#define F_Hash(HashStart, Variable) F_HashBytes(HashStart, &(Variable), sizeof Variable)

f_u64 F_HashStr8(f_str8 String);

f_u64 F_HashCompileOptions(f_compile_options CompileOptions);

f_u64 F_TUHash(f_str8 PathToSource, f_compile_options CompileOptions);

f_u64 F_HashSourceAndIncludes (f_memory_arena *Arena, f_str8 PathToSource, f_str8_array IncludePaths);

void F_CompileNewAndChangedTUs
(
	f_group            Group,
	f_memory_arena    *Arena,
	f_compilation     *Compilation,
	f_compile_options CompileOptions,
	f_str8_array      SourceFiles,
	f_toolchain       Toolchain
);

void  F_WaitForCompilation (f_group Group, f_compilation *Compilation);
f_b32 F_PollCompilation    (f_group Group, f_compilation *Compilation);


f_str8_array F_ObjectFilesFromCompilation(f_group Group, f_memory_arena *Arena, f_compilation Compilation);

f_link F_LinkTUs (f_memory_arena *Arena, f_link_options LinkOptions, f_str8_array ObjectFiles, f_toolchain Toolchain);

void  F_WaitForLinking (f_link *Link);
f_b32 F_PollLinking    (f_link *Link);

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
	f_str8_list ErrorStream;
	f_u64 Handle;
}
f_file;

typedef struct f_internal_file_options
{
	// Create the file if missing, or clear its contents if it exists.
	f_b32 Overwrite;
}
f_internal_file_options;

#define F_OS_FileFromFilename(Arena, Filename, ...) \
F_OS_Internal_FileFromFilename(Arena, Filename, (f_internal_file_options){ __VA_ARGS__ })

#define F_OS_WriteStr8ToPath(Arena, PathToFile, StringToWrite, ...) \
F_OS_Internal_WriteStr8ToPath(Arena, PathToFile, StringToWrite, (f_internal_file_options){ __VA_ARGS__ })

f_file F_OS_Internal_FileFromFilename (f_memory_arena *Arena, f_str8 Filename, f_internal_file_options Options);
void   F_OS_ReleaseFile      (f_file *File);

f_file_info F_OS_FileInfoFromFilename          (f_memory_arena *Arena, f_str8 Filename);
f_file_info F_OS_FileInfoFromFile              (f_file *File);
f_str8      F_OS_ReadEntireFile                (f_memory_arena *Arena, f_file *File);
f_str8      F_OS_ReadEntireFileFromPath        (f_memory_arena *Arena, f_str8 PathToFile);
void        F_OS_WriteStr8ToFile               (f_file *File, f_str8 StringToWrite);
void        F_OS_Internal_WriteStr8ToPath      (f_memory_arena *Arena, f_str8 PathToFile, f_str8 StringToWrite, f_internal_file_options Options);
f_b32       F_OS_DeleteFile                    (f_memory_arena *Arena, f_str8 PathToFile);
void        F_OS_RenameFileTo                  (f_memory_arena *Arena, f_str8 From, f_str8 To);
void        F_OS_MakeDirectoryIfNotAlreadyMade (f_memory_arena *Arena, f_str8 Path);

f_dll  F_OS_DLLFromPath (f_str8 Path);
void *F_OS_DLLSymbolFromName (f_dll DLL, f_str8 ProcedureName);

typedef struct f_internal_directory_options
{
	char *Directories[16];
	f_b32 ExcludeDirectories;
	f_b32 PrependPath;
	f_str8_array FilterByExtension;
}
f_internal_directory_options;

#define F_OS_ListOfEntriesInDirectory(Arena, DirectoryPath, ...) \
F_OS_Internal_ListOfEntriesInDirectories(Arena, (f_internal_directory_options){ .Directories = {DirectoryPath},__VA_ARGS__ })

#define F_OS_ArrayOfEntriesInDirectory(Arena, DirectoryPath, ...) \
F_Str8ArrayFromList(Arena, F_OS_ListOfEntriesInDirectories(Arena,  __VA_ARGS__))

#define F_OS_ListOfFilesInDirectory(Arena, DirectoryPath, ...) \
F_OS_Internal_ListOfEntriesInDirectories(Arena,  (f_internal_directory_options){ .Directories = {DirectoryPath}, .ExcludeDirectories = F_True, __VA_ARGS__ })

#define F_OS_ArrayOfFilesInDirectory(Arena, DirectoryPath, ...) \
F_Str8ArrayFromList(Arena, F_OS_ListOfEntriesInDirectories(Arena, .Directories = {DirectoryPath}, .ExcludeDirectories = F_True, __VA_ARGS__))


#define F_OS_ListOfEntriesInDirectories(Arena, ...) \
F_OS_Internal_ListOfEntriesInDirectories(Arena, (f_internal_directory_options){ __VA_ARGS__ })

#define F_OS_ArrayOfEntriesInDirectories(Arena, ...) \
F_Str8ArrayFromList(Arena, F_OS_ListOfEntriesInDirectories(Arena, __VA_ARGS__))

#define F_OS_ListOfFilesInDirectories(Arena, ...) \
F_OS_Internal_ListOfEntriesInDirectories(Arena, (f_internal_directory_options){ .ExcludeDirectories = F_True, __VA_ARGS__ })

#define F_OS_ArrayOfFilesInDirectories(Arena, ...) \
F_Str8ArrayFromList(Arena, F_OS_ListOfEntriesInDirectories(Arena, .ExcludeDirectories = F_True, __VA_ARGS__))


f_str8_list F_OS_Internal_ListOfEntriesInDirectories(f_memory_arena *Arena, f_internal_directory_options Options);

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

void F_Print(f_str8 String);
#define F_PrintF(...) \
F_Print(F_Str8PushF(F_TArena(), __VA_ARGS__))

f_file F_OS_StandardOut(void);

f_s64 F_OS_NumberOFHighPerformanceCPUs (void);

#define F_LaneEntry void F_Internal_LaneEntry()

void F_Internal_LaneEntry();


f_build *F_BuildFromBuildOptions (f_memory_arena *Arena, f_build_options BuildOptions);

f_build_cache F_BuildCacheFromOutputPath (f_group Group, f_memory_arena *Arena, f_str8 OutputPath);

f_b32         F_HashIsInBuildCache (f_group Group, f_build_cache Cache, f_u64 Hash);

f_build_watch F_BuildWatchFromSourceFiles (f_memory_arena *Arena, f_str8_array SourceFilePaths, f_str8_array IncludePaths);


f_b32 F_UpdateBuild (f_build *Build);

void F_UpdateTU(f_memory_arena *Arena, f_compilation *Compilation, f_translation_unit *TU, f_compile_options CompileOptions, f_toolchain Toolchain);

f_b32 F_CompilationFinished(f_compilation *Compilation);

f_translation_unit F_TranslationUnitFromSourceFile(f_memory_arena *Arena, f_str8 SourceFile, f_compile_options CompileOptions);

void F_Report(f_memory_arena *Arena, f_build_report Report);

f_memory_arena *F_PArena();
f_memory_arena *F_TArena();
f_group F_DefaultGroup();

f_b32 F_IsBuildLane();
f_b32 F_IsUpdateLane();

#endif

#ifdef FRIDAY_IMPLEMENTATION

#ifdef FRIDAY_MAIN

int main(int ArgCount, char **Arguments)
{
#ifdef FRIDAY_SINGLE_THREAD
	F_DispatchAcrossNLanes(1, Arguments, ArgCount);
#else
	F_DispatchAcrossNLanes(F_OS_NumberOFHighPerformanceCPUs(), Arguments, ArgCount);
#endif
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
f_str8_array F_CommandLineArgs;

thread_local f_memory_arena F_ErrorArena;

#include <stdatomic.h> // atomic_fetch_add, atomic_store, atomic_load, atomic_memory_fence ...
#include <stdio.h> // printf()
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

f_b32   F_IsCallerInGroup(f_group Group)
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

	F_CommandLineArgs.Items = F_ArenaPushArrayType(FirstLaneArena, ArgCount, f_str8);
	F_CommandLineArgs.Count = ArgCount;
	for(f_s32 I = 0; I < ArgCount; I++)
	{
		f_s32 ArgLength = strlen(Args[I]);
		F_CommandLineArgs.Items[I].Bytes = F_ArenaPushArrayType(FirstLaneArena, ArgLength, f_s8);
		F_MemoryCopy(F_CommandLineArgs.Items[I].Bytes, Args[I], ArgLength);
		F_CommandLineArgs.Items[I].Size = ArgLength;
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

f_str8_array F_Internal_Str8ArrayFromCStrs(f_memory_arena *Arena, char **CStrs, f_u64 Str8Count)
{
	f_str8_array Result = {0};
	Result.Items = F_ArenaPushArrayType(Arena, Str8Count, f_str8);
	Result.Count = Str8Count;

	for(int I = 0; I < Str8Count; I++)
	{
		f_s32 CStrLength = strlen(CStrs[I]);
		Result.Items[I].Bytes = F_ArenaPushArrayType(Arena, CStrLength, f_s8);
		Result.Items[I].Size = CStrLength;
		F_MemoryCopy(Result.Items[I].Bytes, CStrs[I], CStrLength);
	}

	return Result;
}

f_str8_array F_Internal_Str8ArrayFromStr8s(f_memory_arena *Arena, f_str8 *Str8s, f_u64 Str8Count)
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

void F_OS_WriteStr8ToFile(f_file *File, f_str8 FileToWrite)
{
	if(File->Errors) return;
#if defined(OS_WINDOWS)
		DWORD Written = 0;
	File->Errors = !WriteFile(
		(HANDLE)File->Handle,
		FileToWrite.Bytes,
		(DWORD)FileToWrite.Size,
		&Written, F_Null
	);
#elif defined(OS_DARWIN)
	f_s32 Status = write(File->Handle, FileToWrite.Bytes, FileToWrite.Size);
	File->Errors = Status < 0;
#else
#error "Unsupported OS"
#endif
}

void F_OS_Internal_WriteStr8ToPath(f_memory_arena *Arena, f_str8 PathToFile, f_str8 StringToWrite, f_internal_file_options Options)
{
	f_file File = F_OS_Internal_FileFromFilename(Arena, PathToFile, Options);
	F_OS_WriteStr8ToFile(&File, StringToWrite);
	F_OS_ReleaseFile(&File);
}

f_b32 F_OS_DeleteFile(f_memory_arena *Arena, f_str8 PathToFile)
{
	f_b32 Result = F_True;
#if defined(OS_WINDOWS)
	Result = DeleteFileA(F_CStrFromStr8(F_TArena(), PathToFile)) != 0;
#elif defined(OS_DARWIN)
	Result = remove(F_CStrFromStr8(F_TArena(), PathToFile));
#else
#error "Unsupported OS"
#endif
	return Result;
}

f_dll  F_OS_DLLFromPath(f_str8 Path)
{
	f_dll DLL = {0};
#if defined(OS_WINDOWS)
	DLL.Handle = (f_u64)LoadLibraryA(F_CStrFromStr8(F_TArena(), Path));
#elif defined(OS_DARWIN)
	DLL.Handle = (f_u64)dlopen(F_CStrFromStr8(F_TArena(), Path), RTLD_NOW | RTLD_LOCAL);
#else
#endif
	return DLL;
}

void *F_OS_DLLSymbolFromName(f_dll DLL, f_str8 ProcedureName)
{
	void *Procedure;
#if defined(OS_WINDOWS)
	Procedure = GetProcAddress(DLL.Handle, F_CStrFromStr8(F_TArena(), ProcedureName));
#elif defined(OS_DARWIN)
	Procedure = dlsym((void *)DLL.Handle, F_CStrFromStr8(F_TArena(), ProcedureName));
#else
#error "Unsupported OS"
#endif
	return Procedure;
}

void F_OS_RenameFileTo(f_memory_arena *Arena, f_str8 From, f_str8 To)
{
#if defined(OS_WINDOWS)
	MoveFileExA(F_CStrFromStr8(F_TArena(), From), F_CStrFromStr8(F_TArena(), To), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
#elif defined(OS_DARWIN)
	rename(F_CStrFromStr8(F_TArena(), From), F_CStrFromStr8(F_TArena(), To));
#else
#error "Unsupported OS"
#endif
}


f_str8 F_OS_ReadEntireFile(f_memory_arena *Arena, f_file *File)
{
		if(File->Errors) return F_Str8("");
		f_file_info Info = F_OS_FileInfoFromFile(File);
		f_s8 *Backing = F_ArenaPushArrayType(Arena, Info.SizeInBytes, f_s8);
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
	return (f_str8) { (f_s64)Info.SizeInBytes, Backing };
}

f_b32 F_FileHasErrors(f_file File)
{
	return File.ErrorStream.First || File.Errors;
}

void F_LogError(f_str8 Message)
{
	F_PrintF(F_RED "error" F_RESET ":" "%s\n", Message);
}

void F_LogErrorStream(f_str8_list Messages)
{
	for(f_str8_list_node *Node = Messages.First; Node; Node = Node->Next)
	{
		F_LogError(Node->String);
	}
}

f_str8 F_OS_ReadEntireFileFromPath(f_memory_arena *Arena, f_str8 PathToFile)
{
	f_file File = F_OS_FileFromFilename(Arena, PathToFile);
	f_str8 Contents = F_OS_ReadEntireFile(Arena, &File);
	if(F_FileHasErrors(File))
	{
		F_LogErrorStream(File.ErrorStream);
	}
	F_OS_ReleaseFile(&File);
	return Contents;
}

void F_OS_MakeDirectoryIfNotAlreadyMade(f_memory_arena *Arena, f_str8 Path)
{
#if defined(OS_WINDOWS)
	CreateDirectoryA(F_CStrFromStr8(Arena, Path), F_Null);
#elif defined(OS_DARWIN)
	mkdir(F_CStrFromStr8(F_TArena(), Path), 0777);
#else
#error "Unsupported OS"
#endif
}

f_file_info F_OS_FileInfoFromFilename(f_memory_arena *Arena, f_str8 Filename)
{
	f_file_info FileInfo = {0};
#if defined(OS_WINDOWS)
	f_file File = F_OS_FileFromFilename(Arena, Filename);
	FileInfo = F_OS_FileInfoFromFile(&File);
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

f_file F_OS_Internal_FileFromFilename(f_memory_arena *Arena, f_str8 Filename, f_internal_file_options Options)
{
	f_file File = {0};
#if defined(OS_WINDOWS)
	File.Handle = (f_u64)CreateFileA(
		F_CStrFromStr8(Arena, Filename),
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
		F_AppendErrorF(&File.ErrorStream, "Could not open file '%.*s'", F_Expand(Filename));
	}
#elif defined(OS_DARWIN)
	f_s32 Handle = open(
		F_CStrFromStr8(Arena, Filename),
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
			"Could not open or create file '%.*s'", F_Expand(Filename));
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

f_build_cache F_BuildCacheFromOutputPath(f_group Group, f_memory_arena *Arena, f_str8 OutputPath)
{
		f_str8_array ObjectFiles = F_OS_ArrayOfEntriesInDirectory(
		Arena, OutputPath, .ExcludeDirectories = F_True,
		.FilterByExtension = F_Str8ArrayFromCStrs(Arena, ".o"),
	);
	f_build_cache Cache = {0};
	if(Group.ID == 0) Cache.Items = F_ArenaPushArrayType(Arena, ObjectFiles.Count, f_u64);
	Cache.Count = ObjectFiles.Count;
	Cache = F_ReadFromGroupID(Group, 0, Cache);
	f_range Range = F_SplitRange(Group, ObjectFiles.Count);
	for (f_s32 I = Range.Start; I < Range.End; I++)
	{
		f_str8 HashAsString = ObjectFiles.Items[I];
		// Remove extension
		HashAsString.Size -= 2;
		f_u64 Hash = 0;
		for(f_s32 I = 0; I < HashAsString.Size; I++)
		{
			Hash = HashAsString.Bytes[I] - '0' + Hash*10;
		}
		Cache.Items[I] = Hash;
	}
	return Cache;
}

f_b32 F_HashIsInBuildCache(f_group Group, f_build_cache Cache, f_u64 Hash)
{
	f_b32 Found = F_False;
	f_range Range = F_SplitRange(Group, Cache.Count);
	for (f_s32 I = Range.Start; I < Range.End; I++)
	{
		Found |= Cache.Items[I] == Hash;
	}
	return F_GroupAnyTrue(Group, Found);
}

f_u64 __F_Internal_HashSourceAndIncludes(f_memory_arena *Arena, f_str8 PathToSource, f_str8_array IncludePaths, f_str8_list *IncludesSeenAlready);

f_translation_unit F_TranslationUnitFromSourceFile(f_memory_arena *Arena, f_str8 SourceFile, f_compile_options CompileOptions)
{
	f_translation_unit TU = {0};
	f_str8_list IncludesSeenAlreadyList = {0};
	TU.Hash = __F_Internal_HashSourceAndIncludes(Arena, SourceFile, CompileOptions.IncludePaths, &IncludesSeenAlreadyList);
	f_str8_array IncludesSeenAlready = F_Str8ArrayFromList(Arena, IncludesSeenAlreadyList);
	TU.FileCount = 1 + IncludesSeenAlready.Count;
	TU.PathsForSourceAndIncludes = F_ArenaPushArrayType(Arena, TU.FileCount, f_str8);
	TU.TimeStamps = F_ArenaPushArrayType(Arena, TU.FileCount, f_u64);
	for(f_s32 FileIndex = 0; FileIndex < TU.FileCount; FileIndex++) 
	{
		f_file_info FileInfo;
		if(FileIndex == 0)
		{
			FileInfo = F_OS_FileInfoFromFilename(Arena, SourceFile);
		}
		else
		{
			FileInfo = F_OS_FileInfoFromFilename(Arena, IncludesSeenAlready.Items[FileIndex - 1]);
		}
		TU.TimeStamps[FileIndex] = FileInfo.LastModifiedInMillisecondsSinceEpoch;
		TU.PathsForSourceAndIncludes[0] = F_Str8PushF(Arena, "%.*s", F_Expand(SourceFile));
		for(f_s32 IncludeIndex = 0; IncludeIndex < IncludesSeenAlready.Count; IncludeIndex++)
		{
			TU.PathsForSourceAndIncludes[IncludeIndex + 1] = F_Str8PushF(Arena, "%.*s", F_Expand(IncludesSeenAlready.Items[IncludeIndex]));
		}
	}
	return TU;
}

f_build_watch F_BuildWatchFromSourceFiles(f_memory_arena *Arena, f_str8_array SourceFilePaths, f_str8_array IncludePaths)
{
	f_build_watch BuildWatch = {0};
		BuildWatch.TUs = F_ArenaPushArrayType(Arena, SourceFilePaths.Count, f_translation_unit);
		BuildWatch.TUCount = SourceFilePaths.Count;

	for (f_s32 I = 0;I < SourceFilePaths.Count; I++)
	{
		f_str8_list IncludesSeenAlreadyList = {0};
		BuildWatch.TUs[I].Hash = __F_Internal_HashSourceAndIncludes(Arena, SourceFilePaths.Items[I], IncludePaths, &IncludesSeenAlreadyList);
		f_str8_array IncludesSeenAlready = F_Str8ArrayFromList(Arena, IncludesSeenAlreadyList);
		BuildWatch.TUs[I].FileCount = 1 + IncludesSeenAlready.Count;
		BuildWatch.TUs[I].PathsForSourceAndIncludes = F_ArenaPushArrayType(Arena, BuildWatch.TUs[I].FileCount, f_str8);
		BuildWatch.TUs[I].TimeStamps = F_ArenaPushArrayType(Arena, BuildWatch.TUs[I].FileCount, f_u64);
		for(f_s32 FileIndex = 0; FileIndex < BuildWatch.TUs[I].FileCount; FileIndex++) 
		{
			f_file_info FileInfo;
			if(FileIndex == 0)
			{
				FileInfo = F_OS_FileInfoFromFilename(F_TArena(), SourceFilePaths.Items[I]);
			}
			else
			{
				FileInfo = F_OS_FileInfoFromFilename(F_TArena(), IncludesSeenAlready.Items[FileIndex - 1]);
			}
			BuildWatch.TUs[I].TimeStamps[FileIndex] = FileInfo.LastModifiedInMillisecondsSinceEpoch;
			BuildWatch.TUs[I].PathsForSourceAndIncludes[0] = F_Str8PushF(Arena, "%.*s", F_Expand(SourceFilePaths.Items[I]));
			for(f_s32 IncludeIndex = 0; IncludeIndex < IncludesSeenAlready.Count; IncludeIndex++)
			{
				BuildWatch.TUs[I].PathsForSourceAndIncludes[IncludeIndex + 1] = F_Str8PushF(Arena, "%.*s", F_Expand(IncludesSeenAlready.Items[IncludeIndex]));
			}
		}
	}

	return BuildWatch;
}

f_b32 *F_MaskOfFilesChangedInTU(f_group Group, f_memory_arena *Arena, f_translation_unit *TU, f_u64 ChangeGranularityInMilliseconds)
{
	f_b32 *FilesChangedMask = F_Null;
	if(Group.ID == 0)
	{
		FilesChangedMask = F_ArenaPushArrayType(Arena, TU->FileCount, f_b32);
	}
	FilesChangedMask = F_ReadFromGroupID(Group, 0, FilesChangedMask);

	f_range Range = F_SplitRange(Group, TU->FileCount);
	for (f_s32 I = Range.Start; I < Range.End; I++)
	{
		f_file_info FileInfo = F_OS_FileInfoFromFilename(F_TArena(), TU->PathsForSourceAndIncludes[I]);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch - TU->TimeStamps[I] > 0) printf("Change time in ms: %llu\n", FileInfo.LastModifiedInMillisecondsSinceEpoch - TU->TimeStamps[I]);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch - TU->TimeStamps[I] > ChangeGranularityInMilliseconds)
		{
			TU->TimeStamps[I] = FileInfo.LastModifiedInMillisecondsSinceEpoch;
			FilesChangedMask[I] = F_True;
		}
	}

	return FilesChangedMask;
}

f_b32 *F_MaskOfTUsChanged(f_group Group, f_memory_arena *Arena, f_build_watch BuildWatch, f_u64 ChangeGranularityInMilliseconds)
{
	f_b32 *TUsChanged = F_Null;
	if(Group.ID == 0)
	{
		TUsChanged = F_ArenaPushArrayType(Arena, BuildWatch.TUCount, f_b32);
	}
	TUsChanged = F_ReadFromGroupID(Group, 0, TUsChanged);

	f_range Range = F_SplitRange(Group, BuildWatch.TUCount);
	for (f_s32 TUIndex = Range.Start; TUIndex < Range.End; TUIndex++)
	{
		f_b32 AnyFileInTUChanged = F_False;
		f_translation_unit *TU = BuildWatch.TUs + TUIndex;
		for(f_s32 FileIndex = 0; FileIndex < TU->FileCount; FileIndex++)
		{
			f_file_info FileInfo = F_OS_FileInfoFromFilename(F_TArena(), TU->PathsForSourceAndIncludes[FileIndex]);
			if(FileInfo.LastModifiedInMillisecondsSinceEpoch - TU->TimeStamps[FileIndex] > ChangeGranularityInMilliseconds)
			{
				TU->TimeStamps[FileIndex] = FileInfo.LastModifiedInMillisecondsSinceEpoch;
				AnyFileInTUChanged |= F_True;
			}
		}
		TUsChanged[TUIndex] = AnyFileInTUChanged;
	}

	return TUsChanged;
}

f_str8_array F_FilesChangedInWatch(f_group Group, f_memory_arena *Arena, f_watch *Watch, f_u64 ChangeGranularityInMilliseconds)
{
	f_str8_array Result = {0};
	if(Group.ID == 0)
	{
		Result.Items = F_ArenaPushArrayType(Arena, Watch->FileCount, f_str8);
	}
	Result = F_ReadFromGroupID(Group, 0, Result);

	f_b32 *FileChanged = F_ArenaPushArrayType(F_TArena(), Watch->FileCount, f_b32);

	f_u64 FilesChangedThisLane = 0;
	f_range WatchRange = F_SplitRange(Group, Watch->FileCount);
	for (f_s32 I = WatchRange.Start; I < WatchRange.End; I++)
	{
		f_file_info FileInfo = F_OS_FileInfoFromFilename(F_TArena(), Watch->Files[I]);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch->TimeStamps[I] > 0) printf("Change time in ms: %llu\n", FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch->TimeStamps[I]);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch->TimeStamps[I] > ChangeGranularityInMilliseconds)
		{
			Watch->TimeStamps[I] = FileInfo.LastModifiedInMillisecondsSinceEpoch;
			FileChanged[I] = F_True;
			FilesChangedThisLane++;
		}
	}

	f_u64 WriteIndex = F_GroupExclusivePrefixSum(Group, FilesChangedThisLane);
	for (f_s32 I = WatchRange.Start; I < WatchRange.End; I++)
	{
		if(FileChanged[I])
		{
			Result.Items[WriteIndex++] = Watch->Files[I];
		}
	}

	// After its own writes the last lane has walked the whole array, so it has the total
	f_u64  TotalFilesChanged = F_GroupSum(Group, WriteIndex);
	Result.Count = (f_s64)TotalFilesChanged;

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

f_b32 F_AnyFilesChangedInWatch(f_group Group, f_watch *Watch, f_u64 ChangeGranularityInMilliseconds)
{
	f_b32 AnyChanged = F_False;
	f_range WatchRange = F_SplitRange(Group, Watch->FileCount);
	for (f_s32 I = WatchRange.Start; I < WatchRange.End; I++)
	{
		f_file_info FileInfo = F_OS_FileInfoFromFilename(F_TArena(), Watch->Files[I]);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch - Watch->TimeStamps[I] > ChangeGranularityInMilliseconds)
		{
			AnyChanged = F_True;
		}
	}
	return F_GroupAnyTrue(Group, AnyChanged);
}

f_str8_list F_OS_Internal_ListOfEntriesInDirectories(f_memory_arena *Arena, f_internal_directory_options Options)
{
#define IsCStrDotOrDotDot(Name) (Name[0] == '.' && (Name[1] == 0 || (Name[1] == '.' && Name[2] == 0)))
	f_str8_list Entries = { 0 };
#if defined(OS_WINDOWS)
 // Error handling or something
	WIN32_FIND_DATA FindData;

	HANDLE FoundHandle;
	for(f_s32 Directory = 0; Options.Directories[Directory] && Directory < 16; Directory++)
	{
		FoundHandle = FindFirstFile(F_CStrFromStr8(Arena, F_Str8PushF(Arena, "%s\\*", Options.Directories[Directory])), &FindData);
		if(FoundHandle == INVALID_HANDLE_VALUE) return Entries;
		do
		{
			f_b32 ShouldExcludeDirectories = (Options.ExcludeDirectories && FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			f_b32 IsActualEntry = !IsCStrDotOrDotDot(FindData.cFileName);
			if(ShouldExcludeDirectories) continue;
			if(!IsActualEntry) continue;
			f_str8 EntryName = F_Str8PushF(Arena, "%s", FindData.cFileName);
			f_b32 MatchFound = F_True;
			if(Options.FilterByExtension.Count > 0)
			{
				MatchFound = F_False;
				for(f_s32 I = 0; I < Options.FilterByExtension.Count; I++)
				{
					if(F_Str8EndsWith(EntryName, Options.FilterByExtension.Items[I]))
					{
						MatchFound = F_True;
					}
				}

			}
			if(!MatchFound) continue;
			if(Options.PrependPath)
			{
				F_Str8ListAppendF(Arena, &Entries, "%.*s\\%.*s", F_Expand(DirectoryPath), F_Expand(EntryName));
			}
			else
			{
				F_Str8ListAppendF(Arena, &Entries, "%.*s", F_Expand(EntryName));
			}
		}
		while (FindNextFile(FoundHandle, &FindData) && FoundHandle != INVALID_HANDLE_VALUE);
	}
#elif defined(OS_DARWIN)
	for(f_s32 I = 0; Options.Directories[I] && I < 16; I++)
	{
		DIR *Directory = opendir(Options.Directories[I]);
		struct dirent *Entry;
		while((Entry = readdir(Directory)))
		{
			f_b32 IsActualEntry = !IsCStrDotOrDotDot(Entry->d_name);
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
				F_Str8ListAppendF(Arena, &Entries, "%s/%s", Options.Directories[I], Entry->d_name);
			}
			else
			{
				F_Str8ListAppendF(Arena, &Entries, "%s", Entry->d_name);
			}
		}
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

void F_StartProcess(f_memory_arena *Arena, f_process *Process, f_str8_list Args)
{
	if(!Process) return;
	Process->StdOutSinceLastPoll.Bytes = F_ArenaPushArrayType(Arena, F_PROCESS_STDOUT_CAPACITY, f_s8);
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
			F_CStrFromStr8(Arena, F_Str8JoinList(Arena, Args, F_Str8(" "))),
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
	Process->StdOutSinceLastPoll.Size = 0;

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
		if(ReadFile(Process->StdOut, Process->StdOutSinceLastPoll.Bytes,
		            BytesAvailable, &BytesRead, F_Null))
		{
			Process->StdOutSinceLastPoll.Size = (f_s64)BytesRead;
		}
	}

	if(!StillRunning && Process->StdOutSinceLastPoll.Size == 0)
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

	f_s64 BytesRead = read((f_s32)((f_u64)Process->StdOut), Process->StdOutSinceLastPoll.Bytes, F_PROCESS_STDOUT_CAPACITY);
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
	if(Process->ExitCode.Values[0] || Process->ExitCode.Values[1])
	{
		Result = F_False;
	}
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
	while(F_PollProcess(Process))
	{
		if(Process->StdOutSinceLastPoll.Size > 0) F_Print(Process->StdOutSinceLastPoll);
	}
	if(Process->StdOutSinceLastPoll.Size > 0) F_Print(Process->StdOutSinceLastPoll);
}

void  F_WaitForMultipleProcesses(f_group Group, f_process *Processes, f_s32 ProcessCount)
{
	f_range Range = F_SplitRange(Group, ProcessCount);
	for (f_s32 I = Range.Start; I < Range.End; I++)
	{
		F_WaitForProcess(Processes + I);
	}
}


f_u64 __F_Internal_HashSourceAndIncludes(f_memory_arena *Arena, f_str8 PathToSource, f_str8_array IncludePaths, f_str8_list *IncludesSeenAlready)
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
					for(f_str8_list_node *Node = IncludesSeenAlready->First; Node; Node = Node->Next)
					{
						if(F_Str8Equal(Node->String, PathToInclude)) IncludeSeenAlready = F_True;
					}
					if(Info.Exists && !IncludeSeenAlready)
					{
						// This isn't perfect but it's actually find since even though the order of hashing with xor doesn't now matter,
						// it's never an issue with includes.
						F_Str8ListAppend(Arena, IncludesSeenAlready, PathToInclude);
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


f_u64 F_HashCompileOptions(f_compile_options CompileOptions)
{
	f_u64 Hash = F_HashStart;
	Hash = F_Hash(Hash, CompileOptions.Debug);
	Hash = F_Hash(Hash, CompileOptions.Optimise);
	for(f_s32 I = 0; I < CompileOptions.IncludePaths.Count; I++)
	{
		f_str8 Include = CompileOptions.IncludePaths.Items[I];
		Hash = F_HashBytes(Hash, Include.Bytes, Include.Size);
	}

	for(f_s32 I = 0; I < CompileOptions.ExtraFlags.Count; I++)
	{
		f_str8 Flag = CompileOptions.ExtraFlags.Items[I];
		Hash = F_HashBytes(Hash, Flag.Bytes, Flag.Size);
	}

	return Hash;
}

f_u64 F_TUHash(f_str8 PathToSource, f_compile_options CompileOptions)
{
	f_u64 Hash = F_HashSourceAndIncludes(F_TArena(), PathToSource, CompileOptions.IncludePaths);
	Hash ^= F_HashCompileOptions(CompileOptions);
	return Hash;
}

f_u64 F_HashSourceAndIncludes(f_memory_arena *Arena, f_str8 PathToSource, f_str8_array IncludePaths)
{
	f_str8_list IncludesSeenAlready = {0};
	return __F_Internal_HashSourceAndIncludes(Arena, PathToSource, IncludePaths, &IncludesSeenAlready);
}

f_str8_array F_ObjectFilesFromCompilation(f_group Group, f_memory_arena *Arena, f_compilation Compilation)
{
	f_str8_array ObjectFiles = {0};
	ObjectFiles.Items = F_ArenaPushArrayType(Arena, Compilation.Count, f_str8);
	ObjectFiles.Count = Compilation.Count;

	f_range Range = F_SplitRange(Group, ObjectFiles.Count);
	for (f_s32 I = Range.Start; I < Range.End; I++)
	{
		ObjectFiles.Items[I] = F_Str8PushF(Arena, "%llu.o", Compilation.Hashes[I]);
	}

	return ObjectFiles;
}

void F_WaitForCompilation(f_group Group, f_compilation *Compilation)
{
	F_WaitForMultipleProcesses(Group, Compilation->Processes, Compilation->Count);
	f_b32 Succeeded = F_True;
	f_range Range = F_SplitRange(Group, Compilation->Count);
	for (f_s32 I = Range.Start; I < Range.End; I++)
	{
		Succeeded &= F_ProcessExitedCleanly(Compilation->Processes + I);
	}
	if(F_GroupAllTrue(Group, Succeeded))
	{
		Compilation->Status = F_COMPILE_SUCCEEDED;
	}
	else
	{
		Compilation->Status = F_COMPILE_FAILED;
	}
}

f_b32 F_PollCompilation(f_group Group, f_compilation *Compilation)
{
	f_b32 ProcessesInLaneRunning = F_False;
	f_range CompilationRange = F_SplitRange(Group, Compilation->Count);
	for (f_s32 I = CompilationRange.Start; I < CompilationRange.End; I++)
	{
		ProcessesInLaneRunning |= F_PollProcess(Compilation->Processes + I);
	}
	f_b32 ProccessesInLaneAllFinished = !F_GroupAnyTrue(Group, ProcessesInLaneRunning); 
	if(ProccessesInLaneAllFinished)
	{
		f_b32 Succeeded = F_True;
		for (f_s32 I = CompilationRange.Start; I < CompilationRange.End; I++)
		{
			Succeeded &= F_ProcessExitedCleanly(Compilation->Processes + I);
		}
		if(F_GroupAllTrue(Group, Succeeded))
		{
			Compilation->Status = F_COMPILE_SUCCEEDED;
		}
		else
		{
			Compilation->Status = F_COMPILE_FAILED;
		}
	}
	return F_GroupAnyTrue(Group, ProcessesInLaneRunning);
}

f_build *F_BuildFromBuildOptions(f_memory_arena *Arena, f_build_options BuildOptions)
{
	f_build *Build = F_ArenaPushType(Arena, f_build);
	// Probably enough?
	Build->Arena = F_ArenaReserveButCommitFirstPage(MegaBytes(64));

	Build->SourceDirectories.Items = F_ArenaPushArrayType(F_PArena(), BuildOptions.SourceDirectories.Count, f_str8);
	Build->SourceDirectories.Count = BuildOptions.SourceDirectories.Count;
	return Build;
}

f_b32 F_UpdateBuild(f_build *Build)
{
	f_u64 Count = 0;
	for(f_s32 I = 0; I < Build->SourceDirectories.Count; I++)
	{
		f_str8_array RelevantFilesInDirectory = F_OS_ArrayOfFilesInDirectory(
			F_TArena(),
			F_CStrFromStr8(F_TArena(), Build->SourceDirectories.Items[I]),
			.PrependPath = F_True,
			.FilterByExtension = F_Str8ArrayFromCStrs(F_PArena(), ".c", ".m", ".h")
		);
		Count += RelevantFilesInDirectory.Count;
	}

	f_u64 TUIndexToProcess = 0;
	//for(;TUIndexToProcess = atomic_fetch_add_expicit(Build->))
}

void F_UpdateTU(f_memory_arena *Arena, f_compilation *Compilation, f_translation_unit *TU, f_compile_options CompileOptions, f_toolchain Toolchain)
{
	if(TU->Process.State == F_PROCESS_RUNNING)
	{
		F_PollProcess(&TU->Process);
		return;
	}
	f_b32 TUChangedOnDisk = F_False;
	for(f_s32 FileIndex = 0; !TUChangedOnDisk && FileIndex < TU->FileCount; FileIndex++)
	{
		f_file_info FileInfo = F_OS_FileInfoFromFilename(F_TArena(), TU->PathsForSourceAndIncludes[FileIndex]);
		if(FileInfo.LastModifiedInMillisecondsSinceEpoch - TU->TimeStamps[FileIndex] > F_DEFAULT_TIMEOUT_MS)
		{
			TU->TimeStamps[FileIndex] = FileInfo.LastModifiedInMillisecondsSinceEpoch;
			TUChangedOnDisk = F_True;
		}
	}

	if(!TUChangedOnDisk) return;

	f_u64 OldHash = TU->Hash;
	f_translation_unit NextTU = F_TranslationUnitFromSourceFile(Arena, TU->PathsForSourceAndIncludes[0], CompileOptions);
	TU->FileCount = NextTU.FileCount;
	TU->PathsForSourceAndIncludes = NextTU.PathsForSourceAndIncludes;
	TU->TimeStamps = NextTU.TimeStamps;
	TU->Hash = NextTU.Hash;
	if(OldHash == TU->Hash) return;
		
	f_str8_list Args = {0};
	f_str8 ObjectPath = {0};
	if(CompileOptions.OutputPath.Size > 0)
	{
		ObjectPath = F_Str8PushF(F_TArena(), "%.*s/%llu.o", F_Expand(CompileOptions.OutputPath), TU->Hash);
	}
	else
	{
		ObjectPath = F_Str8PushF(F_TArena(), "%llu.o", TU->Hash);
	}

	F_Str8ListAppend(F_TArena(), &Args, Toolchain.CompilerPath);
	F_Str8ListAppend(F_TArena(), &Args, Toolchain.CompileOnlyFlag);
	F_Str8ListAppend(F_TArena(), &Args, TU->PathsForSourceAndIncludes[0]);

	if(CompileOptions.Debug)
	{
		F_Str8ListAppendF(F_TArena(), &Args, "%.*s", F_Expand(Toolchain.DebugFlag));
	}

	for(int I = 0; I < CompileOptions.ExtraFlags.Count; I++)
	{
		F_Str8ListAppend(F_TArena(), &Args, CompileOptions.ExtraFlags.Items[I]);
	}

	F_Str8ListAppendF(F_TArena(), &Args, "%.*s%.*s", F_Expand(Toolchain.OutputFlag), F_Expand(ObjectPath));

	for(int I = 0; I < CompileOptions.IncludePaths.Count; I++)
	{
		F_Str8ListAppendF(F_TArena(), &Args, "%.*s%.*s", F_Expand(Toolchain.IncludeFlag), F_Expand(CompileOptions.IncludePaths.Items[I]));
	}
	F_Print(F_Str8JoinList(F_TArena(), Args, F_Str8(" ")));
	F_Print(F_Str8("\n"));

	F_StartProcess(Arena, &TU->Process, Args);
}

void F_CompileNewAndChangedTUs
(
	f_group             Group,
	f_memory_arena     *Arena,
	f_compilation      *Compilation,
	f_compile_options CompileOptions,
	f_str8_array      SourceFiles,
	f_toolchain       Toolchain
)
{

	if(Group.ID == 0 && Compilation->Status == F_NOT_STARTED_COMPILING)
	{
		Compilation->CompileOptionsHash = F_HashCompileOptions(CompileOptions);
		Compilation->Count = SourceFiles.Count;

		Compilation->SourcePaths = F_ArenaPushArrayType(Arena, Compilation->Count, f_str8);
		Compilation->Hashes = F_ArenaPushArrayType(Arena, Compilation->Count, f_u64);
		Compilation->ObjectPaths = F_ArenaPushArrayType(Arena, Compilation->Count, f_str8);
		Compilation->Processes = F_ArenaPushArrayType(Arena, Compilation->Count, f_process);

		Compilation->TUs = F_ArenaPushArrayType(Arena, Compilation->Count, f_translation_unit);
		
	}

	if(Compilation->Status == F_NOT_STARTED_COMPILING)
	{
		// Don't need this function, should just do it inline
		f_build_watch BuildWatch = F_BuildWatchFromSourceFiles(Arena, SourceFiles, CompileOptions.IncludePaths);
		Compilation->TUs = BuildWatch.TUs;
	}

	*Compilation = F_ReadFromGroupID(Group, 0, *Compilation);

	// HACK: Remove this concept!
	f_build_watch HackBuildWatchFixMe = {0};
	HackBuildWatchFixMe.TUs = Compilation->TUs;
	HackBuildWatchFixMe.TUCount = Compilation->Count;

	f_b32 *TUsChanged = F_MaskOfTUsChanged(Group, F_TArena(), HackBuildWatchFixMe, 100);

	if(Compilation->Status != F_NOT_STARTED_COMPILING) Compilation->Status = F_NOTHING_TO_COMPILE;

	f_range Range = F_SplitRange(Group, SourceFiles.Count);
	for (f_s32 I = Range.Start; I < Range.End; I++)
	{
		if(Compilation->Status != F_NOT_STARTED_COMPILING && !TUsChanged[I]) continue;

		Compilation->SourcePaths[I] = SourceFiles.Items[I];
		Compilation->Hashes[I] = F_TUHash(SourceFiles.Items[I], CompileOptions);
		if(CompileOptions.OutputPath.Size > 0)
		{
			Compilation->ObjectPaths[I] = F_Str8PushF(Arena, "%.*s/%llu.o", F_Expand(CompileOptions.OutputPath), Compilation->Hashes[I]);
		}
		else
		{
			Compilation->ObjectPaths[I] = F_Str8PushF(Arena, "%llu.o", Compilation->Hashes[I]);
		}

		f_build_cache HackBuildCacheFixMe = {0};
		HackBuildCacheFixMe.Items = Compilation->Hashes;
		HackBuildCacheFixMe.Count = Compilation->Count;
		if(F_HashIsInBuildCache(Group, HackBuildCacheFixMe, Compilation->Hashes[I])) continue;

		f_str8_list Args = {0};
		{
			F_Str8ListAppend(F_TArena(), &Args, Toolchain.CompilerPath);
			F_Str8ListAppend(F_TArena(), &Args, Toolchain.CompileOnlyFlag);
			F_Str8ListAppend(F_TArena(), &Args, SourceFiles.Items[I]);

			if(CompileOptions.Debug)
			{
				F_Str8ListAppendF(F_TArena(), &Args, "%.*s", F_Expand(Toolchain.DebugFlag));
			}

			for(int I = 0; I < CompileOptions.ExtraFlags.Count; I++)
			{
				F_Str8ListAppend(F_TArena(), &Args, CompileOptions.ExtraFlags.Items[I]);
			}

			F_Str8ListAppendF(F_TArena(), &Args, "%.*s%.*s", F_Expand(Toolchain.OutputFlag), F_Expand(Compilation->ObjectPaths[I]));

			for(int I = 0; I < CompileOptions.IncludePaths.Count; I++)
			{
				F_Str8ListAppendF(F_TArena(), &Args, "%.*s%.*s", F_Expand(Toolchain.IncludeFlag), F_Expand(CompileOptions.IncludePaths.Items[I]));
			}
		}
		Compilation->Status = F_COMPILING;
		F_StartProcess(Arena, Compilation->Processes + I, Args);
	}

	if(Compilation->Status == F_COMPILING) F_PollCompilation(Group, Compilation);
}

void F_WaitForLinking(f_link *Link)
{
	F_WaitForProcess(&Link->Process);
	f_b32 Succeeded = F_ProcessExitedCleanly(&Link->Process);
	if(Succeeded)
	{
		Link->Status = F_LINK_SUCCEEDED;
	}
	else
	{
		Link->Status = F_LINK_FAILED;
	}
}

f_b32 F_PollLinking(f_link *Link)
{
	f_b32 StillLinking = F_PollProcess(&Link->Process);
	if(!StillLinking)
	{
		f_b32 Succeeded = F_ProcessExitedCleanly(&Link->Process);	
		if(Succeeded)
		{
			Link->Status = F_LINK_SUCCEEDED;
		}
		else
		{
			Link->Status = F_LINK_FAILED;
		}
	}
	return StillLinking;
}

f_link F_LinkTUs(f_memory_arena *Arena, f_link_options LinkOptions, f_str8_array ObjectFiles, f_toolchain Toolchain)
{
	f_link Link = {0};
	Link.Status = F_LINKING;
	if(LinkOptions.OutputPath.Size > 0)
	{
		Link.OutputPath = F_Str8PushF(Arena, "%.*s/%.*s", F_Expand(LinkOptions.OutputPath), F_Expand(LinkOptions.OutputName));
	}
	else
	{
		Link.OutputPath = F_Str8PushF(Arena, "%.*s", F_Expand(LinkOptions.OutputName));
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

	F_Str8ListAppend(Arena, &Args, F_Str8PushF(Arena, "%.*s%.*s", F_Expand(Toolchain.OutputFlag), Link.OutputPath));
	
	for(int I = 0; I < ObjectFiles.Count; I++)
	{
		F_Str8ListAppend(Arena, &Args, ObjectFiles.Items[I]);
	}

	F_StartProcess(Arena, &Link.Process, Args);

	F_Print(F_Str8("LINKING!!! "));
	F_Print(F_Str8JoinList(Arena, Args, F_Str8(" ")));

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
