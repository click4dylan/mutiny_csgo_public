#include "precompiled.h"
#include "threadtools.h"
#include "string_encrypt_include.h"

#if 0
#include "LocalPlayer.h"
#include <mutex>
std::mutex threadprintmutex;
inline void CThreadMutex::Lock()
{
	if (this == &LocalPlayer.NetvarMutex)
	{
		threadprintmutex.lock();
		printf("Trying to lock LocalPlayer.NetvarMutex on threadid %i at %#010x\n", GetCurrentThreadId(), (DWORD)_ReturnAddress());
		threadprintmutex.unlock();
	}
#ifdef THREAD_MUTEX_TRACING_ENABLED
	uint thisThreadID = ThreadGetCurrentId();
	if (m_bTrace && m_currentOwnerID && (m_currentOwnerID != thisThreadID))
		Msg(_T("Thread %u about to wait for lock %x owned by %u\n"), ThreadGetCurrentId(), (CRITICAL_SECTION *)&m_CriticalSection, m_currentOwnerID);
#endif

	LockSilent();

#ifdef THREAD_MUTEX_TRACING_ENABLED
	if (m_lockCount == 0)
	{
		// we now own it for the first time.  Set owner information
		m_currentOwnerID = thisThreadID;
		if (m_bTrace)
			Msg(_T("Thread %u now owns lock 0x%x\n"), m_currentOwnerID, (CRITICAL_SECTION *)&m_CriticalSection);
	}
	m_lockCount++;
#endif
}

void CThreadMutex::Unlock()
{
	if (this == &LocalPlayer.NetvarMutex)
	{
		threadprintmutex.lock();
		printf("Unlocking LocalPlayer.NetvarMutex on threadid %i at %#010x\n", GetCurrentThreadId(), (DWORD)_ReturnAddress());
		threadprintmutex.unlock();
	}

#ifdef THREAD_MUTEX_TRACING_ENABLED
	AssertMsg(m_lockCount >= 1, "Invalid unlock of thread lock");
	m_lockCount--;
	if (m_lockCount == 0)
	{
		if (m_bTrace)
			Msg(_T("Thread %u releasing lock 0x%x\n"), m_currentOwnerID, (CRITICAL_SECTION *)&m_CriticalSection);
		m_currentOwnerID = 0;
	}
#endif
	UnlockSilent();
}

#endif

typedef BOOL(WINAPI*TryEnterCriticalSectionFunc_t)(LPCRITICAL_SECTION);
static CDynamicFunction<TryEnterCriticalSectionFunc_t> DynTryEnterCriticalSection(XorStrCT("Kernel32.dll"), XorStrCT("TryEnterCriticalSection"));

//-----------------------------------------------------------------------------

#ifndef ThreadGetCurrentId
uint ThreadGetCurrentId()
{
#ifdef _WIN32
	return GetCurrentThreadId();
#elif _LINUX
	return pthread_self();
#endif
}
#endif

//-----------------------------------------------------------------------------
//
// Wrappers for other simple threading operations
//
//-----------------------------------------------------------------------------

void ThreadSleep(unsigned duration)
{
#ifdef _WIN32
	Sleep(duration);
#elif _LINUX
	usleep(duration * 1000);
#endif
}


CThreadEvent::CThreadEvent(bool bManualReset)
{
#ifdef _WIN32
	m_hSyncObject = CreateEvent(NULL, bManualReset, FALSE, NULL);
	//AssertMsg1(m_hSyncObject, "Failed to create event (error 0x%x)", GetLastError());
#elif _LINUX
	pthread_mutexattr_t Attr;
	pthread_mutexattr_init(&Attr);
	pthread_mutex_init(&m_Mutex, &Attr);
	pthread_mutexattr_destroy(&Attr);
	pthread_cond_init(&m_Condition, NULL);
	m_bInitalized = true;
	m_cSet = 0;
	m_bManualReset = bManualReset;
#else
#error "Implement me"
#endif
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------


//---------------------------------------------------------

bool CThreadEvent::Set()
{
	AssertUseable();
#ifdef _WIN32
	return (SetEvent(m_hSyncObject) != 0);
#elif _LINUX
	pthread_mutex_lock(&m_Mutex);
	m_cSet = 1;
	int ret = pthread_cond_broadcast(&m_Condition);
	pthread_mutex_unlock(&m_Mutex);
	return ret == 0;
#endif
}

//---------------------------------------------------------

bool CThreadEvent::Reset()
{
#ifdef THREADS_DEBUG
	AssertUseable();
#endif
#ifdef _WIN32
	return (ResetEvent(m_hSyncObject) != 0);
#elif _LINUX
	pthread_mutex_lock(&m_Mutex);
	m_cSet = 0;
	pthread_mutex_unlock(&m_Mutex);
	return true;
#endif
}

//---------------------------------------------------------

bool CThreadEvent::Check()
{
#ifdef THREADS_DEBUG
	AssertUseable();
#endif
	return Wait(0);
}



bool CThreadEvent::Wait(uint32_t dwTimeout)
{
	return CThreadSyncObject::Wait(dwTimeout);
}

#ifndef _LINUX
CThreadMutex::CThreadMutex()
{
#ifdef THREAD_MUTEX_TRACING_ENABLED
	memset(&m_CriticalSection, 0, sizeof(m_CriticalSection));
#endif
	InitializeCriticalSectionAndSpinCount((CRITICAL_SECTION *)&m_CriticalSection, 4000);
#ifdef THREAD_MUTEX_TRACING_SUPPORTED
	// These need to be initialized unconditionally in case mixing release & debug object modules
	// Lock and unlock may be emitted as COMDATs, in which case may get spurious output
	m_currentOwnerID = m_lockCount = 0;
	m_bTrace = false;
#endif
}

CThreadMutex::~CThreadMutex()
{
	DeleteCriticalSection((CRITICAL_SECTION *)&m_CriticalSection);
}
#endif // !linux

bool CThreadMutex::TryLock()
{	

#if defined( _WIN32 )
#ifdef THREAD_MUTEX_TRACING_ENABLED
	uint thisThreadID = ThreadGetCurrentId();
	if (m_bTrace && m_currentOwnerID && (m_currentOwnerID != thisThreadID))
		Msg("Thread %u about to try-wait for lock %x owned by %u\n", ThreadGetCurrentId(), (CRITICAL_SECTION *)&m_CriticalSection, m_currentOwnerID);
#endif
	if (DynTryEnterCriticalSection != NULL)
	{
		if ((*DynTryEnterCriticalSection)((CRITICAL_SECTION *)&m_CriticalSection) != FALSE)
		{
#ifdef THREAD_MUTEX_TRACING_ENABLED
			if (m_lockCount == 0)
			{
				// we now own it for the first time.  Set owner information
				m_currentOwnerID = thisThreadID;
				if (m_bTrace)
					Msg("Thread %u now owns lock 0x%x\n", m_currentOwnerID, (CRITICAL_SECTION *)&m_CriticalSection);
			}
			m_lockCount++;
#endif
			return true;
		}
		return false;
	}
	Lock();
	return true;
#elif defined( _LINUX )
	return pthread_mutex_trylock(&m_Mutex) == 0;
#else
#error "Implement me!"
	return true;
#endif
}

void CThreadFastMutex::Lock(const UINT32 threadId, unsigned nSpinSleepTime) volatile
{
	int i;
	if (nSpinSleepTime != TT_INFINITE)
	{
		for (i = 1000; i != 0; --i)
		{
			if (TryLock(threadId))
			{
				return;
			}
			_mm_pause();
		}

#ifdef _WIN32
		if (!nSpinSleepTime && GetThreadPriority(GetCurrentThread()) > THREAD_PRIORITY_NORMAL)
		{
			nSpinSleepTime = 1;
		}
		else
#endif

			if (nSpinSleepTime)
			{
				for (i = 4000; i != 0; --i)
				{
					if (TryLock(threadId))
					{
						return;
					}

					_mm_pause();
					Sleep(0);
				}

			}

		for (;; ) // coded as for instead of while to make easy to breakpoint success
		{
			if (TryLock(threadId))
			{
				return;
			}

			_mm_pause();
			Sleep(nSpinSleepTime);
		}
	}
	else
	{
		for (;; ) // coded as for instead of while to make easy to breakpoint success
		{
			if (TryLock(threadId))
			{
				return;
			}

			_mm_pause();
		}
	}
}


void ThreadSetDebugName(ThreadId_t id, const char *pszName)
{
#if 0
#ifdef _WIN32
	if (Plat_IsInDebugSession())
	{
#define MS_VC_EXCEPTION 0x406d1388

		typedef struct tagTHREADNAME_INFO
		{
			DWORD dwType;        // must be 0x1000
			LPCSTR szName;       // pointer to name (in same addr space)
			DWORD dwThreadID;    // thread ID (-1 caller thread)
			DWORD dwFlags;       // reserved for future use, most be zero
		} THREADNAME_INFO;

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = pszName;
		info.dwThreadID = id;
		info.dwFlags = 0;

		__try
		{
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (DWORD *)&info);
		}
		__except (EXCEPTION_CONTINUE_EXECUTION)
		{
		}
	}
#endif
#endif
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

CThreadSyncObject::CThreadSyncObject()
#ifdef _WIN32
	: m_hSyncObject(NULL)
#elif _LINUX
	: m_bInitalized(false)
#endif
{
}

//---------------------------------------------------------

CThreadSyncObject::~CThreadSyncObject()
{
#ifdef _WIN32
	if (m_hSyncObject)
	{
		if (!CloseHandle(m_hSyncObject))
		{
			//Assert(0);
		}
	}
#elif _LINUX
	if (m_bInitalized)
	{
		pthread_cond_destroy(&m_Condition);
		pthread_mutex_destroy(&m_Mutex);
		m_bInitalized = false;
	}
#endif
}

//---------------------------------------------------------

bool CThreadSyncObject::operator!() const
{
#ifdef _WIN32
	return !m_hSyncObject;
#elif _LINUX
	return !m_bInitalized;
#endif
}

//---------------------------------------------------------

void CThreadSyncObject::AssertUseable()
{
#ifdef THREADS_DEBUG
#ifdef _WIN32
	AssertMsg(m_hSyncObject, "Thread synchronization object is unuseable");
#elif _LINUX
	AssertMsg(m_bInitalized, "Thread synchronization object is unuseable");
#endif
#endif
}

//---------------------------------------------------------

bool CThreadSyncObject::Wait(uint32_t dwTimeout)
{
#ifdef THREADS_DEBUG
	AssertUseable();
#endif
#ifdef _WIN32
	return (WaitForSingleObject(m_hSyncObject, dwTimeout) == WAIT_OBJECT_0);
#elif _LINUX
	pthread_mutex_lock(&m_Mutex);
	bool bRet = false;
	if (m_cSet > 0)
	{
		bRet = true;
	}
	else
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		volatile struct timespec tm;

		volatile uint64 nSec = (uint64)tv.tv_usec * 1000 + (uint64)dwTimeout * 1000000;
		tm.tv_sec = tv.tv_sec + nSec / 1000000000;
		tm.tv_nsec = nSec % 1000000000;

		volatile int ret = 0;

		do
		{
			ret = pthread_cond_timedwait(&m_Condition, &m_Mutex, &tm);
		} while (ret == EINTR);

		bRet = (ret == 0);
	}
	if (!m_bManualReset)
		m_cSet = 0;
	pthread_mutex_unlock(&m_Mutex);
	return bRet;
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// CThreadRWLock
//
//-----------------------------------------------------------------------------

void CThreadRWLock::WaitForRead()
{
	m_nPendingReaders++;

	do
	{
		m_mutex.Unlock();
		m_CanRead.Wait();
		m_mutex.Lock();
	} while (m_nWriters);

	m_nPendingReaders--;
}


void CThreadRWLock::LockForWrite()
{
	m_mutex.Lock();
	bool bWait = (m_nWriters != 0 || m_nActiveReaders != 0);
	m_nWriters++;
	m_CanRead.Reset();
	m_mutex.Unlock();

	if (bWait)
	{
		m_CanWrite.Wait();
	}
}

void CThreadRWLock::UnlockWrite()
{
	m_mutex.Lock();
	m_nWriters--;
	if (m_nWriters == 0)
	{
		if (m_nPendingReaders)
		{
			m_CanRead.Set();
		}
	}
	else
	{
		m_CanWrite.Set();
	}
	m_mutex.Unlock();
}

//-----------------------------------------------------------------------------
//
// CThreadSpinRWLock
//
//-----------------------------------------------------------------------------

void CThreadSpinRWLock::SpinLockForWrite(const uint32_t threadId)
{
	int i;

	for (i = 1000; i != 0; --i)
	{
		if (TryLockForWrite(threadId))
		{
			return;
		}
		__asm pause
	}

	for (i = 20000; i != 0; --i)
	{
		if (TryLockForWrite(threadId))
		{
			return;
		}

		__asm pause
		ThreadSleep(0);
	}

	for (;; ) // coded as for instead of while to make easy to breakpoint success
	{
		if (TryLockForWrite(threadId))
		{
			return;
		}

		__asm pause
		ThreadSleep(1);
	}
}

void CThreadSpinRWLock::LockForRead()
{
	int i;

	// In order to grab a read lock, the number of readers must not change and no thread can own the write lock
	LockInfo_t oldValue;
	LockInfo_t newValue;

	oldValue.m_nReaders = m_lockInfo.m_nReaders;
	oldValue.m_writerId = 0;
	newValue.m_nReaders = oldValue.m_nReaders + 1;
	newValue.m_writerId = 0;

	if (m_nWriters == 0 && AssignIf(newValue, oldValue))
		return;
	__asm pause
	oldValue.m_nReaders = m_lockInfo.m_nReaders;
	newValue.m_nReaders = oldValue.m_nReaders + 1;

	for (i = 1000; i != 0; --i)
	{
		if (m_nWriters == 0 && AssignIf(newValue, oldValue))
			return;
		__asm pause
		oldValue.m_nReaders = m_lockInfo.m_nReaders;
		newValue.m_nReaders = oldValue.m_nReaders + 1;
	}

	for (i = 20000; i != 0; --i)
	{
		if (m_nWriters == 0 && AssignIf(newValue, oldValue))
			return;
		__asm pause
		ThreadSleep(0);
		oldValue.m_nReaders = m_lockInfo.m_nReaders;
		newValue.m_nReaders = oldValue.m_nReaders + 1;
	}

	for (;; ) // coded as for instead of while to make easy to breakpoint success
	{
		if (m_nWriters == 0 && AssignIf(newValue, oldValue))
			return;
		__asm pause
		ThreadSleep(1);
		oldValue.m_nReaders = m_lockInfo.m_nReaders;
		newValue.m_nReaders = oldValue.m_nReaders + 1;
	}
}

void CThreadSpinRWLock::UnlockRead()
{
	int i;

	Assert(m_lockInfo.m_nReaders > 0 && m_lockInfo.m_writerId == 0);
	LockInfo_t oldValue;
	LockInfo_t newValue;

	oldValue.m_nReaders = m_lockInfo.m_nReaders;
	oldValue.m_writerId = 0;
	newValue.m_nReaders = oldValue.m_nReaders - 1;
	newValue.m_writerId = 0;

	if (AssignIf(newValue, oldValue))
		return;
	__asm pause
	oldValue.m_nReaders = m_lockInfo.m_nReaders;
	newValue.m_nReaders = oldValue.m_nReaders - 1;

	for (i = 500; i != 0; --i)
	{
		if (AssignIf(newValue, oldValue))
			return;
		__asm pause
		oldValue.m_nReaders = m_lockInfo.m_nReaders;
		newValue.m_nReaders = oldValue.m_nReaders - 1;
	}

	for (i = 20000; i != 0; --i)
	{
		if (AssignIf(newValue, oldValue))
			return;
		__asm pause
		ThreadSleep(0);
		oldValue.m_nReaders = m_lockInfo.m_nReaders;
		newValue.m_nReaders = oldValue.m_nReaders - 1;
	}

	for (;; ) // coded as for instead of while to make easy to breakpoint success
	{
		if (AssignIf(newValue, oldValue))
			return;
		__asm pause
		ThreadSleep(1);
		oldValue.m_nReaders = m_lockInfo.m_nReaders;
		newValue.m_nReaders = oldValue.m_nReaders - 1;
	}
}

void CThreadSpinRWLock::UnlockWrite()
{
	Assert(m_lockInfo.m_writerId == ThreadGetCurrentId() && m_lockInfo.m_nReaders == 0);
	static const LockInfo_t newValue = { 0, 0 };
#if defined(_X360)
	// X360TBD: Serious Perf implications, not yet. __sync();
#endif
	ThreadInterlockedExchange64((int64_t *)&m_lockInfo, *((int64_t *)&newValue));
	m_nWriters--;
}
