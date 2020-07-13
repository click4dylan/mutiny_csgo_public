#include "precompiled.h"
#if 0
#include <Winternl.h>

typedef struct tagPROCESSENTRY32W
{
	DWORD   dwSize;
	DWORD   cntUsage;
	DWORD   th32ProcessID;          // this process
	ULONG_PTR th32DefaultHeapID;
	DWORD   th32ModuleID;           // associated exe
	DWORD   cntThreads;
	DWORD   th32ParentProcessID;    // this process's parent process
	LONG    pcPriClassBase;         // Base priority of process's threads
	DWORD   dwFlags;
	WCHAR   szExeFile[MAX_PATH];    // Path
} PROCESSENTRY32W;
typedef PROCESSENTRY32W *  PPROCESSENTRY32W;
typedef PROCESSENTRY32W *  LPPROCESSENTRY32W;
#define TH32CS_SNAPHEAPLIST 0x00000001
#define TH32CS_SNAPPROCESS  0x00000002
#define TH32CS_SNAPTHREAD   0x00000004
#define TH32CS_SNAPMODULE   0x00000008
#define TH32CS_SNAPMODULE32 0x00000010
#define TH32CS_SNAPALL      (TH32CS_SNAPHEAPLIST | TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD | TH32CS_SNAPMODULE)
#define TH32CS_INHERIT      0x80000000

typedef unsigned char byte;

#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif


#define ObjectBasicInformation 0
#define ObjectNameInformation 1
#define ObjectTypeInformation 2

typedef struct _SYSTEM_HANDLE
{
	ULONG ProcessId;
	BYTE ObjectTypeNumber;
	BYTE Flags;
	USHORT Handle;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG HandleCount;
	SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef enum _POOL_TYPE
{
	NonPagedPool,
	PagedPool,
	NonPagedPoolMustSucceed,
	DontUseThisType,
	NonPagedPoolCacheAligned,
	PagedPoolCacheAligned,
	NonPagedPoolCacheAlignedMustS
} POOL_TYPE, *PPOOL_TYPE;


typedef struct _OBJECT_TYPE_INFORMATION
{
	UNICODE_STRING Name;
	ULONG TotalNumberOfObjects;
	ULONG TotalNumberOfHandles;
	ULONG TotalPagedPoolUsage;
	ULONG TotalNonPagedPoolUsage;
	ULONG TotalNamePoolUsage;
	ULONG TotalHandleTableUsage;
	ULONG HighWaterNumberOfObjects;
	ULONG HighWaterNumberOfHandles;
	ULONG HighWaterPagedPoolUsage;
	ULONG HighWaterNonPagedPoolUsage;
	ULONG HighWaterNamePoolUsage;
	ULONG HighWaterHandleTableUsage;
	ULONG InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG ValidAccess;
	BOOLEAN SecurityRequired;
	BOOLEAN MaintainHandleCount;
	USHORT MaintainTypeList;
	POOL_TYPE PoolType;
	ULONG PagedPoolUsage;
	ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _PEB2_LDR_DATA
{
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB2_LDR_DATA, *PPEB2_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY2
{
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;

	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY2, *PLDR_DATA_TABLE_ENTRY2;

typedef struct _RTL_USER_PROCESS_PARAMETERS22
{
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS2, *PRTL_USER_PROCESS_PARAMETERS2;

typedef
VOID
(NTAPI *PPS_POST_PROCESS_INIT_ROUTINE)(
	VOID);

typedef struct _PEB2_FREE_BLOCK
{
	_PEB2_FREE_BLOCK* Next;
	ULONG Size;
} PEB2_FREE_BLOCK, *PPEB2_FREE_BLOCK;

typedef void(*PPEB2LOCKROUTINE)(
	PVOID PebLock
	);

typedef struct _PEB2
{
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	BOOLEAN Spare;
	HANDLE Mutant;
	PVOID ImageBaseAddress;
	PPEB2_LDR_DATA LoaderData;
	PRTL_USER_PROCESS_PARAMETERS2 ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;
	PVOID FastPebLock;
	PPEB2LOCKROUTINE FastPebLockRoutine;
	PPEB2LOCKROUTINE FastPebUnlockRoutine;
	ULONG EnvironmentUpdateCount;
	PVOID* KernelCallbackTable;
	PVOID EventLogSection;
	PVOID EventLog;
	PPEB2_FREE_BLOCK FreeList;
	ULONG TlsExpansionCounter;
	PVOID TlsBitmap;
	ULONG TlsBitmapBits[0x2];
	PVOID ReadOnlySharedMemoryBase;
	PVOID ReadOnlySharedMemoryHeap;
	PVOID* ReadOnlyStaticServerData;
	PVOID AnsiCodePageData;
	PVOID OemCodePageData;
	PVOID UnicodeCaseTableData;
	ULONG NumberOfProcessors;
	ULONG NtGlobalFlag;
	BYTE Spare2[0x4];
	LARGE_INTEGER CriticalSectionTimeout;
	ULONG HeapSegmentReserve;
	ULONG HeapSegmentCommit;
	ULONG HeapDeCommitTotalFreeThreshold;
	ULONG HeapDeCommitFreeBlockThreshold;
	ULONG NumberOfHeaps;
	ULONG MaximumNumberOfHeaps;
	PVOID* * ProcessHeaps;
	PVOID GdiSharedHandleTable;
	PVOID ProcessStarterHelper;
	PVOID GdiDCAttributeList;
	PVOID LoaderLock;
	ULONG OSMajorVersion;
	ULONG OSMinorVersion;
	ULONG OSBuildNumber;
	ULONG OSPlatformId;
	ULONG ImageSubSystem;
	ULONG ImageSubSystemMajorVersion;
	ULONG ImageSubSystemMinorVersion;
	ULONG GdiHandleBuffer[0x22];
	ULONG PostProcessInitRoutine;
	ULONG TlsExpansionBitmap;
	BYTE TlsExpansionBitmapBits[0x80];
	ULONG SessionId;
} PEB2, *PPEB2;

typedef struct _CLIENT_ID
{
	PVOID UniqueProcess;
	PVOID UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef   struct   _THREAD_BASIC_INFORMATION {   //   Information   Class   0  
	LONG        ExitStatus;
	PVOID       TebBaseAddress;
	CLIENT_ID   ClientId;
	LONG        AffinityMask;
	LONG        Priority;
	LONG        BasePriority;
}   THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _ACTIVATION_CONTEXT_STACK
{
	struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME* ActiveFrame;
	LIST_ENTRY FrameListCache;
	ULONG Flags;
	ULONG NextCookieSequenceNumber;
	ULONG StackId;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

#define GDI_BATCH_BUFFER_SIZE 310

typedef struct _GDI_TEB_BATCH
{
	ULONG Offset;
	ULONG_PTR HDC;
	ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT
{
	ULONG Flags;
	PSTR FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME
{
	ULONG Flags;
	struct _TEB_ACTIVE_FRAME* Previous;
	PTEB_ACTIVE_FRAME_CONTEXT Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

struct TEB2
{
	NT_TIB NtTib;
	PVOID EnvironmentPointer;
	CLIENT_ID ClientId;
	PVOID ActiveRpcHandle;
	PVOID ThreadLocalStoragePointer;
	PPEB ProcessEnvironmentBlock;
	ULONG LastErrorValue;
	ULONG CountOfOwnedCriticalSections;
	PVOID CsrClientThread;
	PVOID Win32ThreadInfo;
	ULONG User32Reserved[26];
	ULONG UserReserved[5];
	PVOID WOW32Reserved;
	ULONG CurrentLocale;
	ULONG FpSoftwareStatusRegister;
	VOID* SystemReserved1[54];
	LONG ExceptionCode;
	PACTIVATION_CONTEXT_STACK ActivationContextStackPointer;
	UCHAR SpareBytes1[36];
	ULONG TxFsContext;
	GDI_TEB_BATCH GdiTebBatch;
	CLIENT_ID RealClientId;
	PVOID GdiCachedProcessHandle;
	ULONG GdiClientPID;
	ULONG GdiClientTID;
	PVOID GdiThreadLocalInfo;
	ULONG Win32ClientInfo[62];
	VOID* glDispatchTable[233];
	ULONG glReserved1[29];
	PVOID glReserved2;
	PVOID glSectionInfo;
	PVOID glSection;
	PVOID glTable;
	PVOID glCurrentRC;
	PVOID glContext;
	ULONG LastStatusValue;
	UNICODE_STRING StaticUnicodeString;
	WCHAR StaticUnicodeBuffer[261];
	PVOID DeallocationStack;
	VOID* TlsSlots[64];
	LIST_ENTRY TlsLinks;
	PVOID Vdm;
	PVOID ReservedForNtRpc;
	VOID* DbgSsReserved[2];
	ULONG HardErrorMode;
	VOID* Instrumentation[9];
	GUID ActivityId;
	PVOID SubProcessTag;
	PVOID EtwLocalData;
	PVOID EtwTraceData;
	PVOID WinSockData;
	ULONG GdiBatchCount;
	UCHAR SpareBool0;
	UCHAR SpareBool1;
	UCHAR SpareBool2;
	UCHAR IdealProcessor;
	ULONG GuaranteedStackBytes;
	PVOID ReservedForPerf;
	PVOID ReservedForOle;
	ULONG WaitingOnLoaderLock;
	PVOID SavedPriorityState;
	ULONG SoftPatchPtr1;
	PVOID ThreadPoolData;
	VOID* * TlsExpansionSlots;
	ULONG ImpersonationLocale;
	ULONG IsImpersonating;
	PVOID NlsCache;
	PVOID pShimData;
	ULONG HeapVirtualAffinity;
	PVOID CurrentTransactionHandle;
	PTEB_ACTIVE_FRAME ActiveFrame;
	PVOID FlsData;
	PVOID PreferredLanguages;
	PVOID UserPrefLanguages;
	PVOID MergedPrefLanguages;
	ULONG MuiImpersonation;
	WORD CrossTebFlags;
	ULONG SpareCrossTebBits : 16;
	WORD SameTebFlags;
	ULONG DbgSafeThunkCall : 1;
	ULONG DbgInDebugPrint : 1;
	ULONG DbgHasFiberData : 1;
	ULONG DbgSkipThreadAttach : 1;
	ULONG DbgWerInShipAssertCode : 1;
	ULONG DbgRanProcessInit : 1;
	ULONG DbgClonedThread : 1;
	ULONG DbgSuppressDebugMsg : 1;
	ULONG SpareSameTebBits : 8;
	PVOID TxnScopeEnterCallback;
	PVOID TxnScopeExitCallback;
	PVOID TxnScopeContext;
	ULONG LockCount;
	ULONG ProcessRundown;
	UINT64 LastSwitchTime;
	UINT64 TotalSwitchOutTime;
	LARGE_INTEGER WaitReasonBitMap;
};

typedef TEB2* PTEB2;
#endif

#if 0
HANDLE gamethread = 0;

extern "C" NTSYSAPI VOID NTAPI RtlAcquirePebLock();
extern "C" NTSYSAPI VOID NTAPI RtlReleasePebLock();
typedef NTSTATUS(NTAPI *pNtSetInformationThread)
(HANDLE, UINT, PVOID, ULONG);
NTSTATUS Status;
#include <processthreadsapi.h>

typedef enum _THREAD_INFORMATION_CLASS2 {
	ThreadBasicInformation2,
	ThreadTimes,
	ThreadPriority,
	ThreadBasePriority,
	ThreadAffinityMask,
	ThreadImpersonationToken,
	ThreadDescriptorTableEntry,
	ThreadEnableAlignmentFaultFixup,
	ThreadEventPair,
	ThreadQuerySetWin32StartAddress,
	ThreadZeroTlsCell,
	ThreadPerformanceCount,
	ThreadAmILastThread,
	ThreadIdealProcessor,
	ThreadPriorityBoost,
	ThreadSetTlsArrayAddress,
	ThreadIsIoPending2,
	ThreadHideFromDebugger
} THREAD_INFORMATION_CLASS2, *PTHREAD_INFORMATION_CLASS2;

void __stdcall TestThread()
{
	pNtSetInformationThread NtSetInformationThread = (pNtSetInformationThread)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");
	HANDLE thishandle;
	DuplicateHandle(GetCurrentProcess(), // Source Process Handle.
		GetCurrentThread(),  // Source Handle to dup.
		GetCurrentProcess(), // Target Process Handle.
		&thishandle,        // Target Handle pointer.
		0,                   // Options flag.
		TRUE,                // Inheritable flag
		DUPLICATE_SAME_ACCESS);// Options

	for (;;)
	{
		if (gamethread != 0)
		{
			THREAD_BASIC_INFORMATION tbi = { 0 };
			NTSTATUS status = NtQueryInformationThread(gamethread, (THREADINFOCLASS)ThreadBasicInformation2, &tbi, sizeof(tbi), NULL);
			if (NT_SUCCESS(status))
			{
				TEB2* pTargetTeb = (TEB2*)tbi.TebBaseAddress;
				TEB2* pCurrentTeb = (TEB2*)NtCurrentTeb();
				PVOID OldTlsSlots[64];//openthread
				Ray_t Ray;
				Ray.Init(LocalPlayer.Entity->GetEyePosition(), LocalPlayer.Entity->GetEyePosition() + Vector(1000, 150, 15));
				CTraceFilter filter;
				filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;
				trace_t tr;
				PEB2* peb = (PEB2*)pCurrentTeb->ProcessEnvironmentBlock;

				ULONG CurrentTlsBitmapBits[0x2];


				//NTSTATUS status2 = NtSetInformationThread(GetCurrentThread(), ThreadSetTlsArrayAddress, pTargetTeb->ThreadLocalStoragePointer, sizeof(pTargetTeb->ThreadLocalStoragePointer));
				//if (NT_SUCCESS(status2))
				{
					//Interfaces::EngineTrace->TraceRay(Ray, 0x4600400B, &filter, &tr);
				}
#if 1
				// The following code is fully optional. 
				// If target TLS slot was allocated, bit mask is already set and there is no need to set it one more time
				// But it won't hurt anyway...
				RtlAcquirePebLock();
				for (int i = 0; i < 64; i++)
				{
					OldTlsSlots[i] = pCurrentTeb->TlsSlots[i];
					pCurrentTeb->TlsSlots[i] = pTargetTeb->TlsSlots[i];
				}
				CurrentTlsBitmapBits[0] = peb->TlsExpansionBitmapBits[0];
				CurrentTlsBitmapBits[1] = peb->TlsExpansionBitmapBits[1];
				peb->TlsBitmapBits[0] = ((PEB2*)pTargetTeb->ProcessEnvironmentBlock)->TlsBitmapBits[0];
				peb->TlsBitmapBits[1] = ((PEB2*)pTargetTeb->ProcessEnvironmentBlock)->TlsBitmapBits[1];
				RtlReleasePebLock();

				Interfaces::EngineTrace->TraceRay(Ray, 0x4600400B, &filter, &tr);
				//Restore value
				RtlAcquirePebLock();
				for (int index = 0; index < 64; index++)
				{
					pCurrentTeb->TlsSlots[index] = OldTlsSlots[index];
				}
				peb->TlsExpansionBitmapBits[0] = CurrentTlsBitmapBits[0];
				peb->TlsExpansionBitmapBits[1] = CurrentTlsBitmapBits[1];
				RtlReleasePebLock();
#endif
			}
		}
		Sleep(100);
	}
}

#endif

#if 0
static bool madethread = false;
TEB2* pCurrentTeb = (TEB2*)NtCurrentTeb();
if (madethread == false)
{
	DuplicateHandle(GetCurrentProcess(), // Source Process Handle.
		GetCurrentThread(),  // Source Handle to dup.
		GetCurrentProcess(), // Target Process Handle.
		&gamethread,        // Target Handle pointer.
		0,                   // Options flag.
		TRUE,                // Inheritable flag
		DUPLICATE_SAME_ACCESS);// Options


	std::thread tst(TestThread);
	tst.detach();
	madethread = true;
}
#endif