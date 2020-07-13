#ifndef USEFULS_H
#define USEFULS_H
#pragma once
//#include "SourceEngine\IEngineTrace.hpp"

/*
class EngineTrace
{
public:

	void TraceRay(const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, CGameTrace *pTrace);
};

namespace VTManager
{
	template<typename T> static T vfunc(void *base, int index)
	{
		DWORD *vTabella = *(DWORD**)base;
		return (T)vTabella[index];
	}
}

void EngineTrace::TraceRay(const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, CGameTrace *pTrace)
{
	typedef void(__thiscall* o_TraceRay)(void*, const Ray_t&, unsigned int, ITraceFilter *, CGameTrace*);
	VTManager::vfunc<o_TraceRay>(this, 5)(this, ray, fMask, pTraceFilter, pTrace);
}
*/

/*

#include <Winternl.h>
#define ThreadBasicInformation 0

extern "C" NTSYSAPI VOID NTAPI RtlAcquirePebLock();
extern "C" NTSYSAPI VOID NTAPI RtlReleasePebLock();

template <typename T>
struct _CLIENT_ID_T
{
T UniqueProcess;
T UniqueThread;
};

template<typename T>
struct _THREAD_BASIC_INFORMATION_T
{
NTSTATUS ExitStatus;
T TebBaseAddress;
_CLIENT_ID_T<T> ClientID;
T AffinityMask;
LONG Priority;
LONG BasePriority;
};

template <typename T>
struct _NT_TIB_T
{
T ExceptionList;
T StackBase;
T StackLimit;
T SubSystemTib;
T FiberData;
T ArbitraryUserPointer;
T Self;
};

template <typename T>
struct _UNICODE_STRING_T
{
WORD Length;
WORD MaximumLength;
T Buffer;
};

template <typename T>
struct _GDI_TEB_BATCH_T
{
DWORD Offset;
T HDC;
DWORD Buffer[310];
};

template <typename T>
struct _LIST_ENTRY_T
{
T Flink;
T Blink;
};

template <typename T>
struct _TEB_T
{
typedef T type;

_NT_TIB_T<T> NtTib;
T EnvironmentPointer;
_CLIENT_ID_T<T> ClientId;
T ActiveRpcHandle;
T ThreadLocalStoragePointer;
T ProcessEnvironmentBlock;
DWORD LastErrorValue;
DWORD CountOfOwnedCriticalSections;
T CsrClientThread;
T Win32ThreadInfo;
DWORD User32Reserved[26];
T UserReserved[5];
T WOW32Reserved;
DWORD CurrentLocale;
DWORD FpSoftwareStatusRegister;
T SystemReserved1[54];
DWORD ExceptionCode;
T ActivationContextStackPointer;
BYTE SpareBytes[36];
DWORD TxFsContext;
_GDI_TEB_BATCH_T<T> GdiTebBatch;
_CLIENT_ID_T<T> RealClientId;
T GdiCachedProcessHandle;
DWORD GdiClientPID;
DWORD GdiClientTID;
T GdiThreadLocalInfo;
T Win32ClientInfo[62];
T glDispatchTable[233];
T glReserved1[29];
T glReserved2;
T glSectionInfo;
T glSection;
T glTable;
T glCurrentRC;
T glContext;
DWORD LastStatusValue;
_UNICODE_STRING_T<T> StaticUnicodeString;
wchar_t StaticUnicodeBuffer[261];
T DeallocationStack;
T TlsSlots[64];
_LIST_ENTRY_T<T> TlsLinks;
T Vdm;
T ReservedForNtRpc;
T DbgSsReserved[2];
DWORD HardErrorMode;
T Instrumentation[11];
_GUID ActivityId;
T SubProcessTag;
T PerflibData;
T EtwTraceData;
T WinSockData;
DWORD GdiBatchCount;
DWORD IdealProcessorValue;
DWORD GuaranteedStackBytes;
T ReservedForPerf;
T ReservedForOle;
DWORD WaitingOnLoaderLock;
T SavedPriorityState;
T ReservedForCodeCoverage;
T ThreadPoolData;
T TlsExpansionSlots;
T DeallocationBStore;
T BStoreLimit;
DWORD MuiGeneration;
DWORD IsImpersonating;
T NlsCache;
T pShimData;
USHORT HeapVirtualAffinity;
USHORT LowFragHeapDataSlot;
T CurrentTransactionHandle;
T ActiveFrame;
T FlsData;
T PreferredLanguages;
T UserPrefLanguages;
T MergedPrefLanguages;
DWORD MuiImpersonation;
USHORT CrossTebFlags;
USHORT SameTebFlags;
T TxnScopeEnterCallback;
T TxnScopeExitCallback;
T TxnScopeContext;
DWORD LockCount;
DWORD SpareUlong0;
T ResourceRetValue;
T ReservedForWdf;
};

template <typename T, typename NGF, int A>
struct _PEB_T
{
typedef T type;

union
{
struct
{
BYTE InheritedAddressSpace;
BYTE ReadImageFileExecOptions;
BYTE BeingDebugged;
BYTE BitField;
};
T dummy01;
};
T Mutant;
T ImageBaseAddress;
T Ldr;
T ProcessParameters;
T SubSystemData;
T ProcessHeap;
T FastPebLock;
T AtlThunkSListPtr;
T IFEOKey;
T CrossProcessFlags;
T UserSharedInfoPtr;
DWORD SystemReserved;
DWORD AtlThunkSListPtr32;
T ApiSetMap;
T TlsExpansionCounter;
T TlsBitmap;
DWORD TlsBitmapBits[2];
T ReadOnlySharedMemoryBase;
T HotpatchInformation;
T ReadOnlyStaticServerData;
T AnsiCodePageData;
T OemCodePageData;
T UnicodeCaseTableData;
DWORD NumberOfProcessors;
union
{
DWORD NtGlobalFlag;
NGF dummy02;
};
LARGE_INTEGER CriticalSectionTimeout;
T HeapSegmentReserve;
T HeapSegmentCommit;
T HeapDeCommitTotalFreeThreshold;
T HeapDeCommitFreeBlockThreshold;
DWORD NumberOfHeaps;
DWORD MaximumNumberOfHeaps;
T ProcessHeaps;
T GdiSharedHandleTable;
T ProcessStarterHelper;
T GdiDCAttributeList;
T LoaderLock;
DWORD OSMajorVersion;
DWORD OSMinorVersion;
WORD OSBuildNumber;
WORD OSCSDVersion;
DWORD OSPlatformId;
DWORD ImageSubsystem;
DWORD ImageSubsystemMajorVersion;
T ImageSubsystemMinorVersion;
T ActiveProcessAffinityMask;
T GdiHandleBuffer[A];
T PostProcessInitRoutine;
T TlsExpansionBitmap;
DWORD TlsExpansionBitmapBits[32];
T SessionId;
ULARGE_INTEGER AppCompatFlags;
ULARGE_INTEGER AppCompatFlagsUser;
T pShimData;
T AppCompatInfo;
_UNICODE_STRING_T<T> CSDVersion;
T ActivationContextData;
T ProcessAssemblyStorageMap;
T SystemDefaultActivationContextData;
T SystemAssemblyStorageMap;
T MinimumStackCommit;
T FlsCallback;
_LIST_ENTRY_T<T> FlsListHead;
T FlsBitmap;
DWORD FlsBitmapBits[4];
T FlsHighIndex;
T WerRegistrationData;
T WerShipAssertPtr;
T pContextData;
T pImageHeaderHash;
T TracingFlags;
T CsrServerReadOnlySharedMemoryBase;
};

typedef _TEB_T<DWORD_PTR> TEB_T;
typedef _PEB_T<DWORD, DWORD64, 34> _PEB32;

#ifdef USE64
typedef _PEB64 PEB_T;
#else
typedef _PEB32 PEB_T;
#endif

NTSTATUS CopyTls(HANDLE hTargetThread, int index)
{
_THREAD_BASIC_INFORMATION_T<DWORD_PTR> info = { 0 };

// Tls Expansion slots aren't supported here
if (index > 63)
return STATUS_INVALID_PARAMETER;

NTSTATUS status = NtQueryInformationThread(hTargetThread, (THREADINFOCLASS)ThreadBasicInformation, &info, sizeof(info), NULL);
if (NT_SUCCESS(status))
{
TEB_T* pTargetTeb = (TEB_T*)info.TebBaseAddress;
TEB_T* pCurrentTeb = (TEB_T*)NtCurrentTeb();
PEB_T* pPeb = (PEB_T*)pCurrentTeb->ProcessEnvironmentBlock;

// Copy value
pCurrentTeb->TlsSlots[index] = pTargetTeb->TlsSlots[index];

// The following code is fully optional.
// If target TLS slot was allocated, bit mask is already set and there is no need to set it one more time
// But it won't hurt anyway...
RtlAcquirePebLock();

// Use high 32 bits if required
int ArrayIdx = 0;
if (index > 31)
{
ArrayIdx = 1;
index -= 32;
}

// Set used slot bit
#ifdef _M_AMD64
_bittestandset64((LONG64*)&pPeb->TlsBitmapBits[ArrayIdx], index);
#else
_bittestandset((LONG*)&pPeb->TlsBitmapBits[ArrayIdx], index);
#endif

RtlReleasePebLock();
}

return status;
}



#include <memory>
DWORD GetMainThreadId() {
const std::tr1::shared_ptr<void> hThreadSnapshot(
CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0), CloseHandle);
if (hThreadSnapshot.get() == INVALID_HANDLE_VALUE) {
throw std::runtime_error("GetMainThreadId failed");
}
THREADENTRY32 tEntry;
tEntry.dwSize = sizeof(THREADENTRY32);
DWORD result = 0;
DWORD currentPID = GetCurrentProcessId();
for (BOOL success = Thread32First(hThreadSnapshot.get(), &tEntry);
!result && success && GetLastError() != ERROR_NO_MORE_FILES;
success = Thread32Next(hThreadSnapshot.get(), &tEntry))
{
if (tEntry.th32OwnerProcessID == currentPID) {
result = tEntry.th32ThreadID;
}
}
return result;
}

typedef struct {
HANDLE UniqueProcess;
HANDLE UniqueThread;
} CLIENT_ID;


typedef LONG       KPRIORITY;

typedef struct _THREAD_BASIC_INFORMATION {
NTSTATUS                ExitStatus;
PVOID                   TebBaseAddress;
CLIENT_ID               ClientId;
KAFFINITY               AffinityMask;
KPRIORITY               Priority;
KPRIORITY               BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;
*/


/*
HANDLE hTargetThread = (HANDLE)GetMainThreadId();
DWORD Thr2 = (DWORD)GetModuleHandleA("csgo.exe");
//p_EngineTrace->TraceRay(ray, MASK_SHOT, (ITraceFilter *)&filter, &tr);

THREAD_BASIC_INFORMATION tbi = { 0 };
float ret;
trace_t Trace;
Ray_t Ray;
NTSTATUS status = NtQueryInformationThread(hTargetThread, (THREADINFOCLASS)ThreadBasicInformation, &tbi, sizeof(tbi), NULL);
if (NT_SUCCESS(status))
{
TEB* pTargetTeb = (TEB*)tbi.TebBaseAddress;
TEB* pCurrentTeb = (TEB*)NtCurrentTeb();
PVOID OldTlsSlots[64];
// Copy value
for (int index = 0; index < 64; index++)
{
OldTlsSlots[index] = pCurrentTeb->TlsSlots[index];
pCurrentTeb->TlsSlots[index] = pTargetTeb->TlsSlots[index];
}
p_EngineTrace->TraceRay(ray, MASK_SHOT, (ITraceFilter *)&filter, &tr);
//Restore value
for (int index = 0; index < 64; index++)
{
pCurrentTeb->TlsSlots[index] = OldTlsSlots[index];
}
}

if (tr.fraction == 1.0f)
{
Sleep(0);
}
*/

#endif