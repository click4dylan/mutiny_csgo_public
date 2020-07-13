#ifndef THREADTOOLS_H
#define THREADTOOLS_H

#pragma once
#pragma warning(push)
#pragma warning(disable:4251)

#include <Windows.h>
#include <inttypes.h>
#include "interlocked_ptr.h"


#define TW_TIMEOUT	0xFFFF
#define TW_FAILED	0xFFFE

#define DECLARE_POINTER_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#define FORWARD_DECLARE_HANDLE(name) typedef struct name##__ *name

#define USE_INTRINSIC_INTERLOCKED
#define THREAD_MUTEX_TRACING_SUPPORTED
#define NOINLINE
#define PLATFORM_INTERFACE
#define DECL_ALIGN(x)			__declspec( align( x ) )


#define TT_INTERFACE	extern
#define TT_OVERLOAD	
#define TT_CLASS		

typedef uint32_t ThreadId_t;

TT_INTERFACE void ThreadSetDebugName(ThreadId_t id, const char *pszName);

FORWARD_DECLARE_HANDLE(ThreadHandle_t);
typedef uintptr_t(*ThreadFunc_t)(void *pParam);

const unsigned TT_INFINITE = 0xffffffff;

extern "C"
{
	long __cdecl _InterlockedIncrement(volatile long*);
	long __cdecl _InterlockedDecrement(volatile long*);
	long __cdecl _InterlockedExchange(volatile long*, long);
	long __cdecl _InterlockedExchangeAdd(volatile long*, long);
	long __cdecl _InterlockedCompareExchange(volatile long*, long, long);
}

#pragma intrinsic( _InterlockedCompareExchange )
#pragma intrinsic( _InterlockedDecrement )
#pragma intrinsic( _InterlockedExchange )
#pragma intrinsic( _InterlockedExchangeAdd ) 
#pragma intrinsic( _InterlockedIncrement )

inline int ThreadInterlockedIncrement(int volatile *p) { return _InterlockedIncrement((volatile long*)p); }
inline int ThreadInterlockedDecrement(int volatile *p) { return _InterlockedDecrement((volatile long*)p); }
inline int ThreadInterlockedExchange(int volatile *p, int value) { return _InterlockedExchange((volatile long*)p, value); }
inline int ThreadInterlockedExchangeAdd(int volatile *p, int value) { return _InterlockedExchangeAdd((volatile long*)p, value); }
inline int ThreadInterlockedCompareExchange(int volatile *p, int value, int comperand) { return _InterlockedCompareExchange((volatile long*)p, value, comperand); }
inline bool ThreadInterlockedAssignIf(int volatile *p, int value, int comperand) { return (_InterlockedCompareExchange((volatile long*)p, value, comperand) == comperand); }

#define TIPTR()
inline void *ThreadInterlockedExchangePointer(void * volatile *p, void *value) { return (void *)((intptr_t)ThreadInterlockedExchange(reinterpret_cast<intptr_t volatile *>(p), reinterpret_cast<intptr_t>(value))); }
inline void *ThreadInterlockedCompareExchangePointer(void * volatile *p, void *value, void *comperand) { return (void *)((intptr_t)ThreadInterlockedCompareExchange(reinterpret_cast<intptr_t volatile *>(p), reinterpret_cast<intptr_t>(value), reinterpret_cast<intptr_t>(comperand))); }
inline bool ThreadInterlockedAssignPointerIf(void * volatile *p, void *value, void *comperand) { return (ThreadInterlockedCompareExchange(reinterpret_cast<intptr_t volatile *>(p), reinterpret_cast<intptr_t>(value), reinterpret_cast<intptr_t>(comperand)) == reinterpret_cast<intptr_t>(comperand)); }

inline unsigned ThreadInterlockedExchangeSubtract(int volatile *p, int value) { return ThreadInterlockedExchangeAdd((int volatile *)p, -value); }

inline void const *ThreadInterlockedExchangePointerToConst(void const * volatile *p, void const *value) { return ThreadInterlockedExchangePointer(const_cast <void * volatile *> (p), const_cast <void *> (value)); }
inline void const *ThreadInterlockedCompareExchangePointerToConst(void const * volatile *p, void const *value, void const *comperand) { return ThreadInterlockedCompareExchangePointer(const_cast <void * volatile *> (p), const_cast <void *> (value), const_cast <void *> (comperand)); }
inline bool ThreadInterlockedAssignPointerToConstIf(void const * volatile *p, void const *value, void const *comperand) { return ThreadInterlockedAssignPointerIf(const_cast <void * volatile *> (p), const_cast <void *> (value), const_cast <void *> (comperand)); }


PLATFORM_INTERFACE int64_t ThreadInterlockedCompareExchange64(int64_t volatile *, int64_t value, int64_t comperand) NOINLINE;
PLATFORM_INTERFACE bool ThreadInterlockedAssignIf64(volatile int64_t *pDest, int64_t value, int64_t comperand) NOINLINE;

PLATFORM_INTERFACE int64_t ThreadInterlockedExchange64(int64_t volatile *, int64_t value) NOINLINE;
PLATFORM_INTERFACE int64_t ThreadInterlockedIncrement64(int64_t volatile *) NOINLINE;
PLATFORM_INTERFACE int64_t ThreadInterlockedDecrement64(int64_t volatile *) NOINLINE;
PLATFORM_INTERFACE int64_t ThreadInterlockedExchangeAdd64(int64_t volatile *, int64_t value) NOINLINE;

inline unsigned ThreadInterlockedExchangeSubtract(uint32_t volatile *p, uint32_t value) { return ThreadInterlockedExchangeAdd((int32_t volatile *)p, value); }
inline unsigned ThreadInterlockedIncrement(uint32_t volatile *p) { return ThreadInterlockedIncrement((int32_t volatile *)p); }
inline unsigned ThreadInterlockedDecrement(uint32_t volatile *p) { return ThreadInterlockedDecrement((int32_t volatile *)p); }
inline unsigned ThreadInterlockedExchange(uint32_t volatile *p, uint32_t value) { return ThreadInterlockedExchange((int32_t volatile *)p, value); }
inline unsigned ThreadInterlockedExchangeAdd(uint32_t volatile *p, uint32_t value) { return ThreadInterlockedExchangeAdd((int32_t volatile *)p, value); }
inline unsigned ThreadInterlockedCompareExchange(uint32_t volatile *p, uint32_t value, uint32_t comperand) { return ThreadInterlockedCompareExchange((int32_t volatile *)p, value, comperand); }
inline bool ThreadInterlockedAssignIf(uint32_t volatile *p, uint32_t value, uint32_t comperand) { return ThreadInterlockedAssignIf((int32_t volatile *)p, value, comperand); }

class CThreadMutex
{
public:
	CThreadMutex();
	~CThreadMutex();

	//------------------------------------------------------
	// Mutex acquisition/release. Const intentionally defeated.
	//------------------------------------------------------
	void Lock();
	void Lock() const { (const_cast<CThreadMutex *>(this))->Lock(); }
	void Unlock();
	void Unlock() const { (const_cast<CThreadMutex *>(this))->Unlock(); }

	bool TryLock();
	bool TryLock() const { return (const_cast<CThreadMutex *>(this))->TryLock(); }

	void LockSilent(); // A Lock() operation which never spews.  Required by the logging system to prevent badness.
	void UnlockSilent(); // An Unlock() operation which never spews.  Required by the logging system to prevent badness.

						 //------------------------------------------------------
						 // Use this to make deadlocks easier to track by asserting
						 // when it is expected that the current thread owns the mutex
						 //------------------------------------------------------
	bool AssertOwnedByCurrentThread();

	//------------------------------------------------------
	// Enable tracing to track deadlock problems
	//------------------------------------------------------
	void SetTrace(bool);

private:
	// Disallow copying
	CThreadMutex(const CThreadMutex &);
	CThreadMutex &operator=(const CThreadMutex &);

#if defined( _WIN32 )
	// Efficient solution to breaking the windows.h dependency, invariant is tested.
#ifdef _WIN64
#define TT_SIZEOF_CRITICALSECTION 40	
#else
#ifndef _X360
#define TT_SIZEOF_CRITICALSECTION 24
#else
#define TT_SIZEOF_CRITICALSECTION 28
#endif // !_X360
#endif // _WIN64
	/*byte*/unsigned char m_CriticalSection[TT_SIZEOF_CRITICALSECTION];
#elif defined(POSIX)
	pthread_mutex_t m_Mutex;
	pthread_mutexattr_t m_Attr;
#else
#error
#endif

#ifdef THREAD_MUTEX_TRACING_SUPPORTED
	// Debugging (always herge to allow mixed debug/release builds w/o changing size)
	UINT	m_currentOwnerID;
	UINT16	m_lockCount;
	bool	m_bTrace;
#endif
};

//-----------------------------------------------------------------------------
//
// An alternative mutex that is useful for cases when thread contention is 
// rare, but a mutex is required. Instances should be declared volatile.
// Sleep of 0 may not be sufficient to keep high priority threads from starving 
// lesser threads. This class is not a suitable replacement for a critical
// section if the resource contention is high.
//
//-----------------------------------------------------------------------------

class CThreadFastMutex
{
public:
	CThreadFastMutex()
		: m_ownerID(0),
		m_depth(0)
	{
	}

private:
	FORCEINLINE bool TryLockInline(const UINT32 threadId) volatile
	{
		if (threadId != m_ownerID && !ThreadInterlockedAssignIf((volatile INT32 *)&m_ownerID, (INT32)threadId, 0))
			return false;

		++m_depth;
		return true;
	}

	bool TryLock(const UINT32 threadId) volatile
	{
		return TryLockInline(threadId);
	}

	void Lock(const UINT32 threadId, unsigned nSpinSleepTime) volatile;

public:
	bool TryLock() volatile
	{
#ifdef _DEBUG
		if (m_depth == INT_MAX)
			DebugBreak();

		if (m_depth < 0)
			DebugBreak();
#endif
		return TryLockInline(GetCurrentThreadId());
	}

#ifndef _DEBUG 
	FORCEINLINE
#endif
		void Lock(unsigned int nSpinSleepTime = 0) volatile
	{
		const UINT32 threadId = GetCurrentThreadId();

		if (!TryLockInline(threadId))
		{
			_mm_pause();
			Lock(threadId, nSpinSleepTime);
		}
#ifdef _DEBUG
		if (m_ownerID != GetCurrentThreadId())
			DebugBreak();

		if (m_depth == INT_MAX)
			DebugBreak();

		if (m_depth < 0)
			DebugBreak();
#endif
	}

#ifndef _DEBUG
	FORCEINLINE
#endif
		void Unlock() volatile
	{
#ifdef _DEBUG
		if (m_ownerID != GetCurrentThreadId())
			DebugBreak();

		if (m_depth <= 0)
			DebugBreak();
#endif

		--m_depth;
		if (!m_depth)
		{
			ThreadInterlockedExchange(&m_ownerID, 0);
		}
	}

	bool TryLock() const volatile { return (const_cast<CThreadFastMutex *>(this))->TryLock(); }
	void Lock(unsigned nSpinSleepTime = 0) const volatile { (const_cast<CThreadFastMutex *>(this))->Lock(nSpinSleepTime); }
	void Unlock() const	volatile { (const_cast<CThreadFastMutex *>(this))->Unlock(); }

	// To match regular CThreadMutex:
	bool AssertOwnedByCurrentThread() { return true; }
	void SetTrace(bool) {}

	UINT32 GetOwnerId() const { return m_ownerID; }
	int	GetDepth() const { return m_depth; }
private:
	volatile UINT32 m_ownerID;
	int				m_depth;
};

typedef struct _RTL_CRITICAL_SECTION RTL_CRITICAL_SECTION;
typedef RTL_CRITICAL_SECTION CRITICAL_SECTION;

#if 1
inline void CThreadMutex::Lock()
{
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

//---------------------------------------------------------

inline void CThreadMutex::Unlock()
{
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

//---------------------------------------------------------

inline void CThreadMutex::LockSilent()
{
	EnterCriticalSection((CRITICAL_SECTION *)&m_CriticalSection);
}

//---------------------------------------------------------

inline void CThreadMutex::UnlockSilent()
{
	LeaveCriticalSection((CRITICAL_SECTION *)&m_CriticalSection);
}

//---------------------------------------------------------

inline bool CThreadMutex::AssertOwnedByCurrentThread()
{
#ifdef THREAD_MUTEX_TRACING_ENABLED
	if (ThreadGetCurrentId() == m_currentOwnerID)
		return true;
	AssertMsg3(0, "Expected thread %u as owner of lock 0x%x, but %u owns", ThreadGetCurrentId(), (CRITICAL_SECTION *)&m_CriticalSection, m_currentOwnerID);
	return false;
#else
	return true;
#endif
}

//---------------------------------------------------------

inline void CThreadMutex::SetTrace(bool bTrace)
{
#ifdef THREAD_MUTEX_TRACING_ENABLED
	m_bTrace = bTrace;
#endif
}

template <typename FUNCPTR_TYPE>
class CDynamicFunction
{
public:
	CDynamicFunction(const char *pszModule, const char *pszName, FUNCPTR_TYPE pfnFallback = NULL)
	{
		m_pfn = pfnFallback;

		HMODULE hModule = ::LoadLibrary(pszModule);
		if (hModule)
			m_pfn = (FUNCPTR_TYPE)::GetProcAddress(hModule, pszName);
	}

	operator bool() { return m_pfn != NULL; }
	bool operator !() { return !m_pfn; }
	operator FUNCPTR_TYPE() { return m_pfn; }

private:
	FUNCPTR_TYPE m_pfn;
};


enum NamedEventResult_t
{
	TT_EventDoesntExist = 0,
	TT_EventNotSignaled,
	TT_EventSignaled
};


class CThreadSyncObject
{
public:
	~CThreadSyncObject();

	//-----------------------------------------------------
	// Query if object is useful
	//-----------------------------------------------------
	bool operator!() const;

	//-----------------------------------------------------
	// Access handle
	//-----------------------------------------------------
#ifdef _WIN32
	operator HANDLE() { return GetHandle(); }
	const HANDLE GetHandle() const { return m_hSyncObject; }
#endif
	//-----------------------------------------------------
	// Wait for a signal from the object
	//-----------------------------------------------------
	bool Wait(uint32_t dwTimeout = TT_INFINITE);

	//-----------------------------------------------------
	// Wait for a signal from any of the specified objects.
	//
	// Returns the index of the object that signaled the event
	// or THREADSYNC_TIMEOUT if the timeout was hit before the wait condition was met.
	//
	// Returns TW_FAILED if an incoming object is invalid.
	//
	// If bWaitAll=true, then it'll return 0 if all the objects were set.
	//-----------------------------------------------------
	static uint32_t WaitForMultiple(int nObjects, CThreadSyncObject **ppObjects, bool bWaitAll, uint32_t dwTimeout = TT_INFINITE);

	// This builds a list of pointers and calls straight through to the other WaitForMultiple.
	static uint32_t WaitForMultiple(int nObjects, CThreadSyncObject *ppObjects, bool bWaitAll, uint32_t dwTimeout = TT_INFINITE);

protected:
	CThreadSyncObject();
	void AssertUseable();

#ifdef _WIN32
	HANDLE m_hSyncObject;
#elif defined(POSIX)
	pthread_mutex_t	m_Mutex;
	pthread_cond_t	m_Condition;
	bool m_bInitalized;
	CInterlockedInt m_cSet;
	bool m_bManualReset;
#else
#error "Implement me"
#endif

private:
	CThreadSyncObject(const CThreadSyncObject &);
	CThreadSyncObject &operator=(const CThreadSyncObject &);
};


class CThreadEvent : public CThreadSyncObject
{
public:
	CThreadEvent(bool fManualReset = false);
#ifdef PLATFORM_WINDOWS
	CThreadEvent(const char *name, bool initialState = false, bool bManualReset = false);
	static NamedEventResult_t CheckNamedEvent(const char *name, uint32_t dwTimeout = 0);
#endif
#ifdef WIN32
	CThreadEvent(HANDLE hHandle);
#endif
	//-----------------------------------------------------
	// Set the state to signaled
	//-----------------------------------------------------
	bool Set();

	//-----------------------------------------------------
	// Set the state to nonsignaled
	//-----------------------------------------------------
	bool Reset();

	//-----------------------------------------------------
	// Check if the event is signaled
	//-----------------------------------------------------
	bool Check();

	bool Wait(uint32_t dwTimeout = TT_INFINITE);

	// See CThreadSyncObject for definitions of these functions.
	static uint32_t WaitForMultiple(int nObjects, CThreadEvent **ppObjects, bool bWaitAll, uint32_t dwTimeout = TT_INFINITE);
	static uint32_t WaitForMultiple(int nObjects, CThreadEvent *ppObjects, bool bWaitAll, uint32_t dwTimeout = TT_INFINITE);

private:
	CThreadEvent(const CThreadEvent &);
	CThreadEvent &operator=(const CThreadEvent &);
};

PLATFORM_INTERFACE ThreadHandle_t CreateSimpleThread(ThreadFunc_t, void *pParam, unsigned stackSize = 0);
PLATFORM_INTERFACE bool ReleaseThreadHandle(ThreadHandle_t);


//-----------------------------------------------------------------------------

PLATFORM_INTERFACE void ThreadSleep(unsigned duration = 0);
PLATFORM_INTERFACE ThreadId_t ThreadGetCurrentId();
PLATFORM_INTERFACE ThreadHandle_t ThreadGetCurrentHandle();
PLATFORM_INTERFACE int ThreadGetPriority(ThreadHandle_t hThread = NULL);
PLATFORM_INTERFACE bool ThreadSetPriority(ThreadHandle_t hThread, int priority);
inline		 bool ThreadSetPriority(int priority) { return ThreadSetPriority(NULL, priority); }
//PLATFORM_INTERFACE bool ThreadInMainThread();
PLATFORM_INTERFACE void DeclareCurrentThreadIsMainThread();

// NOTE: ThreadedLoadLibraryFunc_t needs to return the sleep time in milliseconds or TT_INFINITE
typedef int(*ThreadedLoadLibraryFunc_t)();
PLATFORM_INTERFACE void SetThreadedLoadLibraryFunc(ThreadedLoadLibraryFunc_t func);
PLATFORM_INTERFACE ThreadedLoadLibraryFunc_t GetThreadedLoadLibraryFunc();

//DYLAN FIXME
//#if defined( PLATFORM_WINDOWS_PC32 )
//DLL_IMPORT unsigned long STDCALL GetCurrentThreadId();
//#define ThreadGetCurrentId GetCurrentThreadId
//#endif


//-----------------------------------------------------------------------------
//
// Class to Lock a critical section, and unlock it automatically
// when the lock goes out of scope
//
//-----------------------------------------------------------------------------

template <class MUTEX_TYPE = CThreadMutex>
class CAutoLockT
{
public:
	FORCEINLINE CAutoLockT(MUTEX_TYPE &lock)
		: m_lock(lock)
	{
		m_lock.Lock();
	}

	FORCEINLINE CAutoLockT(const MUTEX_TYPE &lock)
		: m_lock(const_cast<MUTEX_TYPE &>(lock))
	{
		m_lock.Lock();
	}

	FORCEINLINE ~CAutoLockT()
	{
		m_lock.Unlock();
	}


private:
	MUTEX_TYPE &m_lock;

	// Disallow copying
	CAutoLockT<MUTEX_TYPE>(const CAutoLockT<MUTEX_TYPE> &);
	CAutoLockT<MUTEX_TYPE> &operator=(const CAutoLockT<MUTEX_TYPE> &);
};

typedef CAutoLockT<CThreadMutex> CAutoLock;

//---------------------------------------------------------

template <int size>	struct CAutoLockTypeDeducer {};
template <> struct CAutoLockTypeDeducer<sizeof(CThreadMutex)> { typedef CThreadMutex Type_t; };
//template <> struct CAutoLockTypeDeducer<sizeof(CThreadNullMutex)> { typedef CThreadNullMutex Type_t; };
#if defined(_WIN32) && !defined(THREAD_PROFILER)
template <> struct CAutoLockTypeDeducer<sizeof(CThreadFastMutex)> { typedef CThreadFastMutex Type_t; };
//template <> struct CAutoLockTypeDeducer<sizeof(CAlignedThreadFastMutex)> { typedef CAlignedThreadFastMutex Type_t; };
#endif

#define AUTO_LOCK_( type, mutex ) \
	CAutoLockT< type > UNIQUE_ID( static_cast<const type &>( mutex ) )

#define AUTO_LOCK( mutex ) \
	AUTO_LOCK_( CAutoLockTypeDeducer<sizeof(mutex)>::Type_t, mutex )


#define AUTO_LOCK_FM( mutex ) \
	AUTO_LOCK_( CThreadFastMutex, mutex )

#define LOCAL_THREAD_LOCK_( tag ) \
	; \
	static CThreadFastMutex autoMutex_##tag; \
	AUTO_LOCK( autoMutex_##tag )

#define LOCAL_THREAD_LOCK() \
	LOCAL_THREAD_LOCK_(_)

class /*PLATFORM_CLASS*/ CThreadRWLock
{
public:
	CThreadRWLock();

	void LockForRead();
	void UnlockRead();
	void LockForWrite();
	void UnlockWrite();

	void LockForRead() const { const_cast<CThreadRWLock *>(this)->LockForRead(); }
	void UnlockRead() const { const_cast<CThreadRWLock *>(this)->UnlockRead(); }
	void LockForWrite() const { const_cast<CThreadRWLock *>(this)->LockForWrite(); }
	void UnlockWrite() const { const_cast<CThreadRWLock *>(this)->UnlockWrite(); }

private:
	void WaitForRead();

#ifdef WIN32
	CThreadFastMutex m_mutex;
#else
	CThreadMutex m_mutex;
#endif
	CThreadEvent m_CanWrite;
	CThreadEvent m_CanRead;

	int m_nWriters;
	int m_nActiveReaders;
	int m_nPendingReaders;
};

//-----------------------------------------------------------------------------
//
// CThreadSpinRWLock
//
//-----------------------------------------------------------------------------

__declspec(align(8))
class /*TT_CLASS*/ CThreadSpinRWLock
{
public:
	CThreadSpinRWLock() { /*COMPILE_TIME_ASSERT(sizeof(LockInfo_t) == sizeof(int64));*/ /*Assert((int)this % 8 == 0);*/ memset(this, 0, sizeof(*this)); }

	bool TryLockForWrite();
	bool TryLockForRead();

	void LockForRead();
	void UnlockRead();
	void LockForWrite();
	void UnlockWrite();

	bool TryLockForWrite() const { return const_cast<CThreadSpinRWLock *>(this)->TryLockForWrite(); }
	bool TryLockForRead() const { return const_cast<CThreadSpinRWLock *>(this)->TryLockForRead(); }
	void LockForRead() const { const_cast<CThreadSpinRWLock *>(this)->LockForRead(); }
	void UnlockRead() const { const_cast<CThreadSpinRWLock *>(this)->UnlockRead(); }
	void LockForWrite() const { const_cast<CThreadSpinRWLock *>(this)->LockForWrite(); }
	void UnlockWrite() const { const_cast<CThreadSpinRWLock *>(this)->UnlockWrite(); }

private:
	struct LockInfo_t
	{
		uint32_t	m_writerId;
		int		m_nReaders;
	};

	bool AssignIf(const LockInfo_t &newValue, const LockInfo_t &comperand);
	bool TryLockForWrite(const uint32_t threadId);
	void SpinLockForWrite(const uint32_t threadId);

	volatile LockInfo_t m_lockInfo;
	CInterlockedInt m_nWriters;
};

//-----------------------------------------------------------------------------
//
// CThreadRWLock inline functions
//
//-----------------------------------------------------------------------------

inline CThreadRWLock::CThreadRWLock()
	: m_CanRead(true),
	m_nWriters(0),
	m_nActiveReaders(0),
	m_nPendingReaders(0)
{
}

inline void CThreadRWLock::LockForRead()
{
	m_mutex.Lock();
	if (m_nWriters)
	{
		WaitForRead();
	}
	m_nActiveReaders++;
	m_mutex.Unlock();
}

inline void CThreadRWLock::UnlockRead()
{
	m_mutex.Lock();
	m_nActiveReaders--;
	if (m_nActiveReaders == 0 && m_nWriters != 0)
	{
		m_CanWrite.Set();
	}
	m_mutex.Unlock();
}

//-----------------------------------------------------------------------------
//
// CThreadSpinRWLock inline functions
//
//-----------------------------------------------------------------------------

inline bool CThreadSpinRWLock::AssignIf(const LockInfo_t &newValue, const LockInfo_t &comperand)
{
	return ThreadInterlockedAssignIf64((int64_t *)&m_lockInfo, *((int64_t *)&newValue), *((int64_t *)&comperand));
}

inline bool CThreadSpinRWLock::TryLockForWrite(const uint32_t threadId)
{
	// In order to grab a write lock, there can be no readers and no owners of the write lock
	if (m_lockInfo.m_nReaders > 0 || (m_lockInfo.m_writerId && m_lockInfo.m_writerId != threadId))
	{
		return false;
	}

	static const LockInfo_t oldValue = { 0, 0 };
	LockInfo_t newValue = { threadId, 0 };
	const bool bSuccess = AssignIf(newValue, oldValue);
#if defined(_X360)
	if (bSuccess)
	{
		// X360TBD: Serious perf implications. Not Yet. __sync();
	}
#endif
	return bSuccess;
}

inline bool CThreadSpinRWLock::TryLockForWrite()
{
	m_nWriters++;
	if (!TryLockForWrite(ThreadGetCurrentId()))
	{
		m_nWriters--;
		return false;
	}
	return true;
}

inline bool CThreadSpinRWLock::TryLockForRead()
{
	if (m_nWriters != 0)
	{
		return false;
	}
	// In order to grab a write lock, the number of readers must not change and no thread can own the write
	LockInfo_t oldValue;
	LockInfo_t newValue;

	oldValue.m_nReaders = m_lockInfo.m_nReaders;
	oldValue.m_writerId = 0;
	newValue.m_nReaders = oldValue.m_nReaders + 1;
	newValue.m_writerId = 0;

	const bool bSuccess = AssignIf(newValue, oldValue);
#if defined(_X360)
	if (bSuccess)
	{
		// X360TBD: Serious perf implications. Not Yet. __sync();
	}
#endif
	return bSuccess;
}

inline void CThreadSpinRWLock::LockForWrite()
{
	const uint32_t threadId = ThreadGetCurrentId();

	m_nWriters++;

	if (!TryLockForWrite(threadId))
	{
		__asm pause
		SpinLockForWrite(threadId);
	}
}

// read data from a memory address
template<class T> FORCEINLINE T ReadVolatileMemory(T const *pPtr)
{
	volatile const T * pVolatilePtr = (volatile const T *)pPtr;
	return *pVolatilePtr;
}



#endif