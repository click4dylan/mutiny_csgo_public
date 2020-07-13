#include "precompiled.h"
#include <Windows.h>
#include "threadtools.h"
#include "CParallelProcessor.h"

ParallelProcessFn ParallelProcess_original;

IThreadPool* g_pThreadPool;

#define USE_INTRINSIC_INTERLOCKED

#ifdef _WIN32
#ifdef _X360
#define TO_INTERLOCK_PARAM(p)		((long *)p)
#define TO_INTERLOCK_PTR_PARAM(p)	((void **)p)
#else
#define TO_INTERLOCK_PARAM(p)		(p)
#define TO_INTERLOCK_PTR_PARAM(p)	(p)
#endif

#ifndef USE_INTRINSIC_INTERLOCKED
long ThreadInterlockedIncrement(long volatile *pDest)
{
	//Assert((size_t)pDest % 4 == 0);
	return InterlockedIncrement(TO_INTERLOCK_PARAM(pDest));
}

long ThreadInterlockedDecrement(long volatile *pDest)
{
	//Assert((size_t)pDest % 4 == 0);
	return InterlockedDecrement(TO_INTERLOCK_PARAM(pDest));
}

long ThreadInterlockedExchange(long volatile *pDest, long value)
{
	//Assert((size_t)pDest % 4 == 0);
	return InterlockedExchange(TO_INTERLOCK_PARAM(pDest), value);
}

long ThreadInterlockedExchangeAdd(long volatile *pDest, long value)
{
	//Assert((size_t)pDest % 4 == 0);
	return InterlockedExchangeAdd(TO_INTERLOCK_PARAM(pDest), value);
}

long ThreadInterlockedCompareExchange(long volatile *pDest, long value, long comperand)
{
	//Assert((size_t)pDest % 4 == 0);
	return InterlockedCompareExchange(TO_INTERLOCK_PARAM(pDest), value, comperand);
}

bool ThreadInterlockedAssignIf(long volatile *pDest, long value, long comperand)
{
	//Assert((size_t)pDest % 4 == 0);

#if !(defined(_WIN64) || defined (_X360))
	__asm
	{
		mov	eax, comperand
		mov	ecx, pDest
		mov edx, value
		lock cmpxchg[ecx], edx
		mov eax, 0
		setz al
	}
#else
	return (InterlockedCompareExchange(TO_INTERLOCK_PARAM(pDest), value, comperand) == comperand);
#endif
}

#endif

#if !defined( USE_INTRINSIC_INTERLOCKED ) || defined( _WIN64 )
void *ThreadInterlockedExchangePointer(void * volatile *pDest, void *value)
{
	//Assert((size_t)pDest % 4 == 0);
	return InterlockedExchangePointer(TO_INTERLOCK_PARAM(pDest), value);
}

void *ThreadInterlockedCompareExchangePointer(void * volatile *pDest, void *value, void *comperand)
{
	//Assert((size_t)pDest % 4 == 0);
	return InterlockedCompareExchangePointer(TO_INTERLOCK_PTR_PARAM(pDest), value, comperand);
}

bool ThreadInterlockedAssignPointerIf(void * volatile *pDest, void *value, void *comperand)
{
	//Assert((size_t)pDest % 4 == 0);
#if !(defined(_WIN64) || defined (_X360))
	__asm
	{
		mov	eax, comperand
		mov	ecx, pDest
		mov edx, value
		lock cmpxchg[ecx], edx
		mov eax, 0
		setz al
	}
#else
	return (InterlockedCompareExchangePointer(TO_INTERLOCK_PTR_PARAM(pDest), value, comperand) == comperand);
#endif
}
#endif

int64 ThreadInterlockedCompareExchange64(int64 volatile *pDest, int64 value, int64 comperand)
{
	//Assert((size_t)pDest % 8 == 0);

#if defined(_WIN64) || defined (_X360)
	return InterlockedCompareExchange64(pDest, value, comperand);
#else
	__asm
	{
		lea esi, comperand;
		lea edi, value;

		mov eax, [esi];
		mov edx, 4[esi];
		mov ebx, [edi];
		mov ecx, 4[edi];
		mov esi, pDest;
		lock CMPXCHG8B[esi];
	}
#endif
}

bool ThreadInterlockedAssignIf64(volatile int64 *pDest, int64 value, int64 comperand)
{
	//Assert((size_t)pDest % 8 == 0);

#if defined(_WIN32) && !defined(_X360)
	__asm
	{
		lea esi, comperand;
		lea edi, value;

		mov eax, [esi];
		mov edx, 4[esi];
		mov ebx, [edi];
		mov ecx, 4[edi];
		mov esi, pDest;
		lock CMPXCHG8B[esi];
		mov eax, 0;
		setz al;
	}
#else
	return (ThreadInterlockedCompareExchange64(pDest, value, comperand) == comperand);
#endif
}


int64 ThreadInterlockedIncrement64(int64 volatile *pDest)
{
	//Assert((size_t)pDest % 8 == 0);

	int64 Old;

	do
	{
		Old = *pDest;
	} while (ThreadInterlockedCompareExchange64(pDest, Old + 1, Old) != Old);

	return Old + 1;
}

int64 ThreadInterlockedDecrement64(int64 volatile *pDest)
{
	//Assert((size_t)pDest % 8 == 0);
	int64 Old;

	do
	{
		Old = *pDest;
	} while (ThreadInterlockedCompareExchange64(pDest, Old - 1, Old) != Old);

	return Old - 1;
}

int64 ThreadInterlockedExchange64(int64 volatile *pDest, int64 value)
{
	//Assert((size_t)pDest % 8 == 0);
	int64 Old;

	do
	{
		Old = *pDest;
	} while (ThreadInterlockedCompareExchange64(pDest, value, Old) != Old);

	return Old;
}

int64 ThreadInterlockedExchangeAdd64(int64 volatile *pDest, int64 value)
{
	//Assert((size_t)pDest % 8 == 0);
	int64 Old;

	do
	{
		Old = *pDest;
	} while (ThreadInterlockedCompareExchange64(pDest, Old + value, Old) != Old);

	return Old;
}

#else
// This will perform horribly, what's the Linux alternative?
// atomic_set(), atomic_add() et al should work for i386 arch
// TODO: implement these if needed
CThreadMutex g_InterlockedMutex;

long ThreadInterlockedIncrement(long volatile *pDest)
{
	AUTO_LOCK(g_InterlockedMutex);
	return ++(*pDest);
}

long ThreadInterlockedDecrement(long volatile *pDest)
{
	AUTO_LOCK(g_InterlockedMutex);
	return --(*pDest);
}

long ThreadInterlockedExchange(long volatile *pDest, long value)
{
	AUTO_LOCK(g_InterlockedMutex);
	long retVal = *pDest;
	*pDest = value;
	return retVal;
}

void *ThreadInterlockedExchangePointer(void * volatile *pDest, void *value)
{
	AUTO_LOCK(g_InterlockedMutex);
	void *retVal = *pDest;
	*pDest = value;
	return retVal;
}

long ThreadInterlockedExchangeAdd(long volatile *pDest, long value)
{
	AUTO_LOCK(g_InterlockedMutex);
	long retVal = *pDest;
	*pDest += value;
	return retVal;
}

long ThreadInterlockedCompareExchange(long volatile *pDest, long value, long comperand)
{
	AUTO_LOCK(g_InterlockedMutex);
	long retVal = *pDest;
	if (*pDest == comperand)
		*pDest = value;
	return retVal;
}

void *ThreadInterlockedCompareExchangePointer(void * volatile *pDest, void *value, void *comperand)
{
	AUTO_LOCK(g_InterlockedMutex);
	void *retVal = *pDest;
	if (*pDest == comperand)
		*pDest = value;
	return retVal;
}


int64 ThreadInterlockedCompareExchange64(int64 volatile *pDest, int64 value, int64 comperand)
{
	//Assert((size_t)pDest % 8 == 0);
	AUTO_LOCK(g_InterlockedMutex);
	int64 retVal = *pDest;
	if (*pDest == comperand)
		*pDest = value;
	return retVal;
}

int64 ThreadInterlockedExchange64(int64 volatile *pDest, int64 value)
{
	//Assert((size_t)pDest % 8 == 0);
	int64 Old;

	do
	{
		Old = *pDest;
	} while (ThreadInterlockedCompareExchange64(pDest, value, Old) != Old);

	return Old;
}

bool ThreadInterlockedAssignIf64(volatile int64 *pDest, int64 value, int64 comperand)
{
	//Assert((size_t)pDest % 8 == 0);
	return (ThreadInterlockedCompareExchange64(pDest, value, comperand) == comperand);
}

bool ThreadInterlockedAssignIf(long volatile *pDest, long value, long comperand)
{
	//Assert((size_t)pDest % 4 == 0);
	return (ThreadInterlockedCompareExchange(pDest, value, comperand) == comperand);
}

#endif

//-----------------------------------------------------------------------------
