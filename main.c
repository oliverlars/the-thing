#include "misc.h"
#include "memory.h"
#include "strings.h"
#include "log.h"
#include "maths.h"
#include "os.h"

#include "app.h"

#define FRIDAY_IMPLEMENTATION
#define FRIDAY_MAIN
#include "friday2.h"

F_LaneEntry
{


	app_memory App = {0};

	f_dll AppDLL = F_OS_DLLFromPath(F_Str8("build/app.dynlib"));
	void (*AppUpdate)(void *, v2i) = F_OS_DLLSymbolFromName(AppDLL, F_Str8("AppUpdate"));

	f_build_options BuildOptions = {0};

	BuildOptions.LinkFlags = F_Str8ArrayFromCStrs(F_PArena(), "-fobjc-arc");
	BuildOptions.Frameworks = F_Str8ArrayFromCStrs(F_PArena(), 
		"Cocoa", "Metal", "QuartzCore"
	);

	BuildOptions.OutputPath = F_Str8("build");
	BuildOptions.OutputName = F_Str8("app.dynlib");

	BuildOptions.IncludeDirectories = F_Str8ArrayFromCStrs(F_PArena(),
	"include/",
	"external/",
	"build/",
	);

	/*
	app.c and app.h need moving to folders to make this new assumption work better
	*/
	BuildOptions.SourceDirectories = F_Str8ArrayFromCStrs(F_PArena(), "source/", "source/darwin/", "external/");
	BuildOptions.Kind = F_OUTPUT_DYNAMIC;

	BuildOptions.Toolchain = F_TOOLCHAIN_CLANG;

	assert(F_DefaultGroup().Count > 0);

	f_build *Build = 0;
	if(F_DefaultGroup().ID == 0)
	{
		Build = F_BuildFromBuildOptions(F_PArena(), BuildOptions);
		atomic_store_explicit(&Build->TUsToProcess, 3, memory_order_release);
	}
	Build = F_ReadFromGroupID(F_DefaultGroup(), 0, Build);
	f_s32 FutureWork = 3;

	f_group UpdateGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID == 0);
	f_group BuildGroup = F_GroupFromPartition(F_DefaultGroup(), F_DefaultGroup().ID > 0);
	assert(UpdateGroup.Count == 1);
	for(;;)
	{
		if(F_IsInGroup(UpdateGroup))
		{

			//for(f_s32 I = 0; I < 100; I++) AppUpdate(&App, v2i(0, 0));
			//printf("Finished Updating\n");

		}
		if(F_IsInGroup(BuildGroup))
		{
			
			f_build_cache BuildCache = F_BuildCacheFromOutputPath(BuildGroup, F_TArena(), F_Str8("build/"));
			
			f_str8_array  SourceFiles = {0};
			f_str8_array  ObjectFiles = {0};
			f_b32 ShouldLinkTUs = F_False;
			if(BuildGroup.ID == 0) 
			{
				SourceFiles = F_OS_ArrayOfEntriesInDirectories(
					F_TArena(), 
				 .Directories[0] = BuildOptions.SourceDirectories.Items[0],
					.Directories[1] = BuildOptions.SourceDirectories.Items[1],
					.Directories[2] = BuildOptions.SourceDirectories.Items[2],
				 .PrependPath = F_True,
				 .FilterByExtension = F_Str8ArrayFromCStrs(F_TArena(), ".m", ".c")
				);
				ObjectFiles.Items = F_ArenaPushArrayType(F_TArena(), SourceFiles.Count, f_str8);
				ObjectFiles.Count = SourceFiles.Count;
			}
			SourceFiles = F_ReadFromGroupID(BuildGroup, 0, SourceFiles);
			ObjectFiles = F_ReadFromGroupID(BuildGroup, 0, ObjectFiles);
			f_range SourceFileRange = F_SplitRange(BuildGroup, SourceFiles.Count);
			f_process *Processes = F_ArenaPushArrayType(F_TArena(), SourceFileRange.End - SourceFileRange.Start, f_process);
			for(f_s32 I = SourceFileRange.Start; I < SourceFileRange.End; I++)
			{
				f_u64 Hash = F_HashSourceAndIncludes(F_TArena(), SourceFiles.Items[I], BuildOptions.IncludeDirectories);
				f_str8 ObjectPath = {0};
				if(BuildOptions.OutputPath.Size > 0)
				{
					ObjectPath = F_Str8PushF(F_TArena(), "%.*s/%llu.o", F_Expand(BuildOptions.OutputPath), Hash);
				}
				else
				{
					ObjectPath = F_Str8PushF(F_TArena(), "%llu.o", Hash);
				}
				ObjectFiles.Items[I] = ObjectPath;
				if(!F_HashIsInBuildCache(BuildCache, Hash))
				{
					ShouldLinkTUs = F_True;
					f_str8_list Args = {0};

					F_Str8ListAppend(F_TArena(), &Args, BuildOptions.Toolchain.CompilerPath);
					F_Str8ListAppend(F_TArena(), &Args, BuildOptions.Toolchain.CompileOnlyFlag);
					F_Str8ListAppend(F_TArena(), &Args, SourceFiles.Items[I]);

					if(BuildOptions.Debug)
					{
						F_Str8ListAppendF(F_TArena(), &Args, "%.*s", F_Expand(BuildOptions.Toolchain.DebugFlag));
					}

					for(int I = 0; I < BuildOptions.CompileFlags.Count; I++)
					{
						F_Str8ListAppend(F_TArena(), &Args, BuildOptions.CompileFlags.Items[I]);
					}

					F_Str8ListAppendF(F_TArena(), &Args, "%.*s%.*s", F_Expand(BuildOptions.Toolchain.OutputFlag), F_Expand(ObjectPath));

					for(int I = 0; I < BuildOptions.IncludeDirectories.Count; I++)
					{
						F_Str8ListAppendF(F_TArena(), &Args, "%.*s%.*s", F_Expand(BuildOptions.Toolchain.IncludeFlag), F_Expand(BuildOptions.IncludeDirectories.Items[I]));
					}
					F_Print(F_Str8JoinList(F_TArena(), Args, F_Str8(" ")));
					F_Print(F_Str8("\n"));

					F_StartProcess(F_TArena(), Processes + I - SourceFileRange.Start, Args);
				}
				F_WaitForMultipleProcesses(Processes, SourceFileRange.End - SourceFileRange.Start);
			}
			ShouldLinkTUs = F_GroupAnyTrue(BuildGroup, ShouldLinkTUs);
			F_GroupBarrier(BuildGroup);

			if(ShouldLinkTUs && BuildGroup.ID == 0)
			{
	
				f_str8 OutputPath = {0};
				if(BuildOptions.OutputPath.Size > 0)
				{
					OutputPath = F_Str8PushF(F_TArena(), "%.*s/%.*s", F_Expand(BuildOptions.OutputPath), F_Expand(BuildOptions.OutputName));
				}
				else
				{
					OutputPath = F_Str8PushF(F_TArena(), "%.*s", F_Expand(BuildOptions.OutputName));
				}

				f_str8_list Args = {0};

				F_Str8ListAppend(F_TArena(), &Args, BuildOptions.Toolchain.CompilerPath);

				for(int I = 0; I < BuildOptions.LinkFlags.Count; I++)
				{
					F_Str8ListAppend(F_TArena(), &Args, BuildOptions.LinkFlags.Items[I]);
				}

				for(int I = 0; I < BuildOptions.Frameworks.Count; I++)
				{
					F_Str8ListAppend(F_TArena(), &Args, F_Str8("-framework"));
					F_Str8ListAppend(F_TArena(), &Args, BuildOptions.Frameworks.Items[I]);
				}

				if(BuildOptions.Kind == F_OUTPUT_DYNAMIC)
				{
					F_Str8ListAppend(F_TArena(), &Args, F_Str8PushF(F_TArena(), "%.*s", F_Expand(BuildOptions.Toolchain.DynamicFlag)));
				}
				else if(BuildOptions.Kind == F_OUTPUT_STATIC)
				{
					F_Str8ListAppend(F_TArena(), &Args, F_Str8PushF(F_TArena(), "%.*s", F_Expand(BuildOptions.Toolchain.StaticFlag)));
				}

				F_Str8ListAppend(F_TArena(), &Args, F_Str8PushF(F_TArena(), "%.*s%.*s", F_Expand(BuildOptions.Toolchain.OutputFlag), OutputPath));
				
				for(int I = 0; I < ObjectFiles.Count; I++)
				{
					F_Str8ListAppend(F_TArena(), &Args, ObjectFiles.Items[I]);
				}
				f_process LinkProcess = {0};
				F_StartProcess(F_TArena(), &LinkProcess, Args);
				F_WaitForProcess(&LinkProcess);

				F_Print(F_Str8("LINKING!!! "));
				F_Print(F_Str8JoinList(F_TArena(), Args, F_Str8(" ")));
			}
		}
		F_ResetTArena();
		F_GroupBarrier(F_DefaultGroup());
	}
}
