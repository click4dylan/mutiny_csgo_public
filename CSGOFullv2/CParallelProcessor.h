#ifndef PARALLEL_H
#define PARALLEL_H

#pragma once
#include "NetworkedVariables.h"
#include "VTHook.h"

#include "interlocked_ptr.h"
#include "threadtools.h"
#include "refcount.h"
#include "utllinkedlist.h"
#include "utlvector.h"
#include "functors.h"
#include "IThreadPool.h"

#include "memdbgon.h"

extern IThreadPool* g_pThreadPool;

using ParallelProcessFn = void(__thiscall *)(void*, void*, unsigned, void*, void*, void*);
extern ParallelProcessFn ParallelProcess_original;

#pragma warning(push)
#pragma warning(disable:4348)
template <typename ITEM_TYPE, class ITEM_PROCESSOR_TYPE, int ID_TO_PREVENT_COMDATS_IN_PROFILES = 1>
class CParallelProcessor;

#pragma warning(pop)

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
enum JobStatusEnum_t
{
	// Use negative for errors
	JOB_OK,						// operation is successful
	JOB_STATUS_PENDING,			// file is properly queued, waiting for service
	JOB_STATUS_INPROGRESS,		// file is being accessed
	JOB_STATUS_ABORTED,			// file was aborted by caller
	JOB_STATUS_UNSERVICED,		// file is not yet queued
};

typedef int JobStatus_t;

enum JobFlags_t
{
	JF_IO = (1 << 0),	// The job primarily blocks on IO or hardware
	JF_BOOST_THREAD = (1 << 1),	// Up the thread priority to max allowed while processing task
	JF_SERIAL = (1 << 2),	// Job cannot be executed out of order relative to other "strict" jobs
	JF_QUEUE = (1 << 3),	// Queue it, even if not an IO job
};

/*
template <typename ITEM_TYPE, class ITEM_PROCESSOR_TYPE, int ID_TO_PREVENT_COMDATS_IN_PROFILES = 1>
class CParallelProcessor;
*/

//-----------------------------------------------------------------------------
// Class to combine the metadata for an operation and the ability to perform
// the operation. Meant for inheritance. All functions inline, defers to executor
//-----------------------------------------------------------------------------
DECLARE_POINTER_HANDLE(ThreadPoolData_t);
#define JOB_NO_DATA ((ThreadPoolData_t)-1)

class CJob : public CRefCounted1<IRefCounted, CRefCountServiceMT>
{
public:
	CJob(JobPriority_t priority = JP_NORMAL)
		: m_status(JOB_STATUS_UNSERVICED),
		m_ThreadPoolData(JOB_NO_DATA),
		m_priority(priority),
		m_flags(0),
		m_pThreadPool(NULL),
		m_CompleteEvent(true),
		m_iServicingThread(-1)
	{
	}

	//-----------------------------------------------------
	// Priority (not thread safe)
	//-----------------------------------------------------
	void SetPriority(JobPriority_t priority) { m_priority = priority; }
	JobPriority_t GetPriority() const { return m_priority; }

	//-----------------------------------------------------

	void SetFlags(unsigned flags) { m_flags = flags; }
	unsigned GetFlags() const { return m_flags; }

	//-----------------------------------------------------

	void SetServiceThread(int iServicingThread) { m_iServicingThread = (char)iServicingThread; }
	int GetServiceThread() const { return m_iServicingThread; }
	void ClearServiceThread() { m_iServicingThread = -1; }

	//-----------------------------------------------------
	// Fast queries
	//-----------------------------------------------------
	bool Executed() const { return (m_status == JOB_OK); }
	bool CanExecute() const { return (m_status == JOB_STATUS_PENDING || m_status == JOB_STATUS_UNSERVICED); }
	bool IsFinished() const { return (m_status != JOB_STATUS_PENDING && m_status != JOB_STATUS_INPROGRESS && m_status != JOB_STATUS_UNSERVICED); }
	JobStatus_t GetStatus() const { return m_status; }

	//-----------------------------------------------------
	// Try to acquire ownership (to satisfy). If you take the lock, you must either execute or abort.
	//-----------------------------------------------------
	bool TryLock() volatile { return m_mutex.TryLock(); }
	void Lock() volatile { m_mutex.Lock(); }
	void Unlock() volatile { m_mutex.Unlock(); }

	//-----------------------------------------------------
	// Thread event support (safe for NULL this to simplify code )
	//-----------------------------------------------------
	bool WaitForFinish(uint32 dwTimeout = TT_INFINITE) { if (!this) return true; return (!IsFinished()) ? g_pThreadPool->YieldWait(this, dwTimeout) : true; }
	bool WaitForFinishAndRelease(uint32 dwTimeout = TT_INFINITE) { if (!this) return true; bool bResult = WaitForFinish(dwTimeout); Release(); return bResult; }
	CThreadEvent *AccessEvent() { return &m_CompleteEvent; }

	//-----------------------------------------------------
	// Perform the job
	//-----------------------------------------------------
	JobStatus_t Execute();
	JobStatus_t TryExecute();
	JobStatus_t ExecuteAndRelease() { JobStatus_t status = Execute(); Release(); return status; }
	JobStatus_t TryExecuteAndRelease() { JobStatus_t status = TryExecute(); Release(); return status; }

	//-----------------------------------------------------
	// Terminate the job, discard if partially or wholly fulfilled
	//-----------------------------------------------------
	JobStatus_t Abort(bool bDiscard = true);

	//virtual ~CJob() {};

	virtual char const *Describe() { return "Job"; }

private:
	//-----------------------------------------------------
	friend class CThreadPool;

	JobStatus_t			m_status;
	JobPriority_t		m_priority;
	CThreadFastMutex	m_mutex;
	unsigned char		m_flags;
	char				m_iServicingThread;
	short				m_reserved;
	ThreadPoolData_t	m_ThreadPoolData;
	IThreadPool *		m_pThreadPool;
	CThreadEvent		m_CompleteEvent;

private:
	//-----------------------------------------------------
	CJob(const CJob &fromRequest);
	void operator=(const CJob &fromRequest);

	virtual JobStatus_t DoExecute() = 0;
	virtual JobStatus_t DoAbort(bool bDiscard) { return JOB_STATUS_ABORTED; }
	virtual void DoCleanup() {}
};

class CFunctorJob : public CJob
{
public:
	CFunctorJob(CFunctor *pFunctor, const char *pszDescription = NULL)
		: m_pFunctor(pFunctor)
	{
		if (pszDescription)
		{
			strncpy(m_szDescription, pszDescription, sizeof(m_szDescription));
		}
		else
		{
			m_szDescription[0] = 0;
		}
	}

	const char *Describe()
	{
		return m_szDescription;
	}

	virtual JobStatus_t DoExecute()
	{
		(*m_pFunctor)();
		return JOB_OK;
	}

private:
	CRefPtr<CFunctor> m_pFunctor;
	char m_szDescription[16];
};

//-----------------------------------------------------------------------------
// Utility for managing multiple jobs
//-----------------------------------------------------------------------------

class CJobSet
{
public:
	CJobSet(CJob *pJob = NULL)
	{
		if (pJob)
		{
			m_jobs.AddToTail(pJob);
		}
	}

	CJobSet(CJob **ppJobs, int nJobs)
	{
		if (ppJobs)
		{
			m_jobs.AddMultipleToTail(nJobs, ppJobs);
		}
	}

	~CJobSet()
	{
		for (int i = 0; i < m_jobs.Count(); i++)
		{
			m_jobs[i]->Release();
		}
	}

	void operator+=(CJob *pJob)
	{
		m_jobs.AddToTail(pJob);
	}

	void operator-=(CJob *pJob)
	{
		m_jobs.FindAndRemove(pJob);
	}

	void Execute(bool bRelease = true)
	{
		for (int i = 0; i < m_jobs.Count(); i++)
		{
			m_jobs[i]->Execute();
			if (bRelease)
			{
				m_jobs[i]->Release();
			}
		}

		if (bRelease)
		{
			m_jobs.RemoveAll();
		}
	}

	void Abort(bool bRelease = true)
	{
		for (int i = 0; i < m_jobs.Count(); i++)
		{
			m_jobs[i]->Abort();
			if (bRelease)
			{
				m_jobs[i]->Release();
			}
		}

		if (bRelease)
		{
			m_jobs.RemoveAll();
		}
	}

	void WaitForFinish(bool bRelease = true)
	{
		for (int i = 0; i < m_jobs.Count(); i++)
		{
			m_jobs[i]->WaitForFinish();
			if (bRelease)
			{
				m_jobs[i]->Release();
			}
		}

		if (bRelease)
		{
			m_jobs.RemoveAll();
		}
	}

	void WaitForFinish(IThreadPool *pPool, bool bRelease = true)
	{
		pPool->YieldWait(m_jobs.Base(), m_jobs.Count());

		if (bRelease)
		{
			for (int i = 0; i < m_jobs.Count(); i++)
			{
				m_jobs[i]->Release();
			}

			m_jobs.RemoveAll();
		}
	}

private:
	CUtlVectorFixed<CJob *, 16> m_jobs;
};

//-----------------------------------------------------------------------------
// Job helpers
//-----------------------------------------------------------------------------

#define ThreadExecute g_pThreadPool->QueueCall
#define ThreadExecuteRef g_pThreadPool->QueueRefCall

#define BeginExecuteParallel()	do { CJobSet jobSet
#define EndExecuteParallel()	jobSet.WaitForFinish( g_pThreadPool ); } while (0)

#define ExecuteParallel			jobSet += g_pThreadPool->QueueCall
#define ExecuteRefParallel		jobSet += g_pThreadPool->QueueCallRef


//-----------------------------------------------------------------------------
// Work splitting: array split, best when cost per item is roughly equal
//-----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable:4389)
#pragma warning(disable:4018)
#pragma warning(disable:4701)

#define DEFINE_NON_MEMBER_ITER_RANGE_PARALLEL(N) \
	template <typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N, typename ITERTYPE1, typename ITERTYPE2> \
	void IterRangeParallel(FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( ITERTYPE1, ITERTYPE2 FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ), ITERTYPE1 from, ITERTYPE2 to FUNC_ARG_FORMAL_PARAMS_##N ) \
	{ \
		const int MAX_THREADS = 16; \
		int nIdle = g_pThreadPool->NumIdleThreads(); \
		ITERTYPE1 range = to - from; \
		int nThreads = min( nIdle + 1, range ); \
		if ( nThreads > MAX_THREADS ) \
		{ \
			nThreads = MAX_THREADS; \
		} \
		if ( nThreads < 2 ) \
		{ \
			FunctorDirectCall( pfnProxied, from, to FUNC_FUNCTOR_CALL_ARGS_##N ); \
		} \
		else \
		{ \
			ITERTYPE1 nIncrement = range / nThreads; \
			\
			CJobSet jobSet; \
			while ( --nThreads ) \
			{ \
				ITERTYPE2 thisTo = from + nIncrement; \
				jobSet += g_pThreadPool->AddCall( pfnProxied, from, thisTo FUNC_FUNCTOR_CALL_ARGS_##N ); \
				from = thisTo; \
			} \
			FunctorDirectCall( pfnProxied, from, to FUNC_FUNCTOR_CALL_ARGS_##N ); \
			jobSet.WaitForFinish( g_pThreadPool ); \
		} \
		\
	}

FUNC_GENERATE_ALL(DEFINE_NON_MEMBER_ITER_RANGE_PARALLEL);

#define DEFINE_MEMBER_ITER_RANGE_PARALLEL(N) \
	template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N, typename ITERTYPE1, typename ITERTYPE2> \
	void IterRangeParallel(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( ITERTYPE1, ITERTYPE2 FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ), ITERTYPE1 from, ITERTYPE2 to FUNC_ARG_FORMAL_PARAMS_##N ) \
	{ \
		const int MAX_THREADS = 16; \
		int nIdle = g_pThreadPool->NumIdleThreads(); \
		ITERTYPE1 range = to - from; \
		int nThreads = min( nIdle + 1, range ); \
		if ( nThreads > MAX_THREADS ) \
		{ \
			nThreads = MAX_THREADS; \
		} \
		if ( nThreads < 2 ) \
		{ \
			FunctorDirectCall( pObject, pfnProxied, from, to FUNC_FUNCTOR_CALL_ARGS_##N ); \
		} \
		else \
		{ \
			ITERTYPE1 nIncrement = range / nThreads; \
			\
			CJobSet jobSet; \
			while ( --nThreads ) \
			{ \
				ITERTYPE2 thisTo = from + nIncrement; \
				jobSet += g_pThreadPool->AddCall( pObject, pfnProxied, from, thisTo FUNC_FUNCTOR_CALL_ARGS_##N ); \
				from = thisTo; \
			} \
			FunctorDirectCall( pObject, pfnProxied, from, to FUNC_FUNCTOR_CALL_ARGS_##N ); \
			jobSet.WaitForFinish( g_pThreadPool ); \
		} \
		\
	}

FUNC_GENERATE_ALL(DEFINE_MEMBER_ITER_RANGE_PARALLEL);


//-----------------------------------------------------------------------------
// Work splitting: competitive, best when cost per item varies a lot
//-----------------------------------------------------------------------------

template <typename T>
class CJobItemProcessor
{
public:
	typedef T ItemType_t;
	void Begin() {}
	// void Process( ItemType_t & ) {}
	void End() {}
};

template <typename T>
class CFuncJobItemProcessor : public CJobItemProcessor<T>
{
public:
	void Init(void(*pfnProcess)(T &), void(*pfnBegin)() = NULL, void(*pfnEnd)() = NULL)
	{
		m_pfnProcess = pfnProcess;
		m_pfnBegin = pfnBegin;
		m_pfnEnd = pfnEnd;
	}

	//CFuncJobItemProcessor(OBJECT_TYPE_PTR pObject, void (FUNCTION_CLASS::*pfnProcess)( ITEM_TYPE & ), void (*pfnBegin)() = NULL, void (*pfnEnd)() = NULL );
	void Begin() { if (m_pfnBegin) (*m_pfnBegin)(); }
	void Process(T &item) { (*m_pfnProcess)(item); }
	void End() { if (m_pfnEnd) (*m_pfnEnd)(); }

protected:
	void(*m_pfnProcess)(T &);
	void(*m_pfnBegin)();
	void(*m_pfnEnd)();
};

template <typename T, class OBJECT_TYPE, class FUNCTION_CLASS = OBJECT_TYPE >
class CMemberFuncJobItemProcessor : public CJobItemProcessor<T>
{
public:
	void Init(OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)(T &), void (FUNCTION_CLASS::*pfnBegin)() = NULL, void (FUNCTION_CLASS::*pfnEnd)() = NULL)
	{
		m_pObject = pObject;
		m_pfnProcess = pfnProcess;
		m_pfnBegin = pfnBegin;
		m_pfnEnd = pfnEnd;
	}

	void Begin() { if (m_pfnBegin) ((*m_pObject).*m_pfnBegin)(); }
	void Process(T &item) { ((*m_pObject).*m_pfnProcess)(item); }
	void End() { if (m_pfnEnd) ((*m_pObject).*m_pfnEnd)(); }

protected:
	OBJECT_TYPE *m_pObject;

	void (FUNCTION_CLASS::*m_pfnProcess)(T &);
	void (FUNCTION_CLASS::*m_pfnBegin)();
	void (FUNCTION_CLASS::*m_pfnEnd)();
};

template <typename T>
class CLoopFuncJobItemProcessor : public CJobItemProcessor<T>
{
public:
	void Init(void(*pfnProcess)(T*, int, int), void(*pfnBegin)() = NULL, void(*pfnEnd)() = NULL)
	{
		m_pfnProcess = pfnProcess;
		m_pfnBegin = pfnBegin;
		m_pfnEnd = pfnEnd;
	}

	void Begin() { if (m_pfnBegin) (*m_pfnBegin)(); }
	void Process(T* pContext, int nFirst, int nCount) { (*m_pfnProcess)(pContext, nFirst, nCount); }
	void End() { if (m_pfnEnd) (*m_pfnEnd)(); }

protected:
	void(*m_pfnProcess)(T*, int, int);
	void(*m_pfnBegin)();
	void(*m_pfnEnd)();
};

template <typename T, class OBJECT_TYPE, class FUNCTION_CLASS = OBJECT_TYPE >
class CLoopMemberFuncJobItemProcessor : public CJobItemProcessor<T>
{
public:
	void Init(OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)(T*, int, int), void (FUNCTION_CLASS::*pfnBegin)() = NULL, void (FUNCTION_CLASS::*pfnEnd)() = NULL)
	{
		m_pObject = pObject;
		m_pfnProcess = pfnProcess;
		m_pfnBegin = pfnBegin;
		m_pfnEnd = pfnEnd;
	}

	void Begin() { if (m_pfnBegin) ((*m_pObject).*m_pfnBegin)(); }
	void Process(T *item, int nFirst, int nCount) { ((*m_pObject).*m_pfnProcess)(item, nFirst, nCount); }
	void End() { if (m_pfnEnd) ((*m_pObject).*m_pfnEnd)(); }

protected:
	OBJECT_TYPE *m_pObject;

	void (FUNCTION_CLASS::*m_pfnProcess)(T*, int, int);
	void (FUNCTION_CLASS::*m_pfnBegin)();
	void (FUNCTION_CLASS::*m_pfnEnd)();
};

template <typename ITEM_TYPE, class ITEM_PROCESSOR_TYPE, int ID_TO_PREVENT_COMDATS_IN_PROFILES>
class CFunctor_ParallelProcessor
{
private:
	uintptr_t vtable1;
	void* unknown;
	uintptr_t vtable2;
	int   setto1;
	void(__thiscall CParallelProcessor< ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES >::* doexecute)();
	CParallelProcessor< ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES > *processorclass;

public:
	__forceinline void __declspec(safebuffers) Construct(CParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES>* processor, void(__thiscall CParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES>::* DoExecuteFunc)())
	{
		setto1 = 1;
		vtable1 = StaticOffsets.GetOffsetValue(_ParallelQueueVT1);
		vtable2 = StaticOffsets.GetOffsetValue(_ParallelQueueVT2);
		doexecute = DoExecuteFunc;//&CParallelProcessor<ITEM_TYPE>::DoExecute;//StaticOffsets.GetOffsetValue(_DoExecute);
		processorclass = processor;
	}
	__declspec(safebuffers) ::CFunctor_ParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES>() {}
};

template <typename ITEM_TYPE, class ITEM_PROCESSOR_TYPE, int ID_TO_PREVENT_COMDATS_IN_PROFILES>
__forceinline __declspec(safebuffers) CFunctor* CreateFunctor(CParallelProcessor< ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES >* processor, void(__thiscall CParallelProcessor< ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES >::* DoExecuteFunc)())
{
	CFunctor_ParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES> *queue = (CFunctor_ParallelProcessor< ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES >*)MALLOC(sizeof(CFunctor_ParallelProcessor< ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES >));
	if (queue)
		queue->Construct(processor, DoExecuteFunc);
	
	return (CFunctor*)queue;
}

#pragma warning(push)
#pragma warning(disable:4189)
#pragma warning(disable:4348)

template <typename ITEM_TYPE, class ITEM_PROCESSOR_TYPE, int ID_TO_PREVENT_COMDATS_IN_PROFILES = 1>
class CParallelProcessor 
{
public:
	CParallelProcessor()
	{
		m_pItems = m_pLimit = 0;
	}

	void __declspec(safebuffers) Run(ITEM_TYPE *pItems, unsigned nItems, int nChunkSize = 1, int nMaxParallel = INT_MAX, IThreadPool *pThreadPool = NULL)
	{
		if (nItems == 0)
			return;

		m_nChunkSize = nChunkSize;

		if (!pThreadPool)
		{
			pThreadPool = g_pThreadPool;
		}

		m_pItems = pItems;
		m_pLimit = (pItems + nItems);

		int nJobs = nItems - 1;

		if (nJobs > nMaxParallel)
		{
			nJobs = nMaxParallel;
		}

		if (!pThreadPool)									// only possible on linux
		{
			DoExecute();
			return;
		}

		int nThreads = pThreadPool->NumThreads();
		if (nJobs > nThreads)
		{
			nJobs = nThreads;
		}

		if (nJobs > 0)
		{
			CJob **jobs = (CJob **)malloc(nJobs * sizeof(CJob **));//stackalloc(nJobs * sizeof(CJob **));
			int i = nJobs;

			while (i--)
			{
				jobs[i] = g_pThreadPool->QueueCall_Game<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES>(this, &CParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES>::DoExecute);//pThreadPool->QueueCall(this, &CParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES>::DoExecute);
			}
		
			DoExecute();

			for (i = 0; i < nJobs; i++)
			{
				CJob *job = jobs[i];
				StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CJob*, CParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES> *processor)>(_AbortJob)(job, this);
				GetVFunc<void(__thiscall*)(CJob*)>(job, 1)(job); //Release
				//job->Abort(/*this*/); // will either abort ones that never got a thread, or noop on ones that did
				//job->Release();
			}
			free(jobs);
		}
		else
		{
			DoExecute();
		}
	}

	ITEM_PROCESSOR_TYPE m_ItemProcessor; //0
	//void(*m_pfnProcess)(ITEM_TYPE&) //0
	//void(*m_pfnBegin)(); //4
	//void(*m_pfnEnd)(); //8

private:
	void __declspec(safebuffers) DoExecute()
	{
		//auto thisptr = this;
		//auto tmp = this->m_pfnProcess;
		//auto tmp2 = this->m_pfnBegin;
		//auto tmp3 = this->m_pItems;
		//auto tmp4 = this->m_pLimit;
		//auto tmp5 = this->m_nChunkSize;
		if (m_pItems < m_pLimit)
		{
			m_ItemProcessor.Begin();

			ITEM_TYPE *pLimit = m_pLimit;

			int nChunkSize = m_nChunkSize;
			for (;;)
			{
				ITEM_TYPE *pCurrent = m_pItems.AtomicAdd(nChunkSize);
				ITEM_TYPE *pLast = min(pLimit, pCurrent + nChunkSize);
				while (pCurrent < pLast)
				{
					m_ItemProcessor.Process(*pCurrent);
					pCurrent++;
				}
				if (pCurrent >= pLimit)
				{
					break;
				}
			}
			m_ItemProcessor.End();
		}
	}
	
	CInterlockedPtr<ITEM_TYPE>	m_pItems; //12
	ITEM_TYPE *					m_pLimit; //16
	int m_nChunkSize; //20
};


template <typename ITEM_TYPE>
inline void ParallelProcess(ITEM_TYPE *pItems, unsigned nItems, void(*pfnProcess)(ITEM_TYPE &), void(*pfnBegin)() = NULL, void(*pfnEnd)() = NULL, int nMaxParallel = INT_MAX)
{
	CParallelProcessor<ITEM_TYPE, CFuncJobItemProcessor<ITEM_TYPE> > processor;
	processor.m_ItemProcessor.Init(pfnProcess, pfnBegin, pfnEnd);
	processor.Run(pItems, nItems, 1, nMaxParallel);
}


template <typename ITEM_TYPE, typename OBJECT_TYPE, typename FUNCTION_CLASS >
inline void ParallelProcess(ITEM_TYPE *pItems, unsigned nItems, OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)(ITEM_TYPE &), void (FUNCTION_CLASS::*pfnBegin)() = NULL, void (FUNCTION_CLASS::*pfnEnd)() = NULL, int nMaxParallel = INT_MAX)
{
	CParallelProcessor<ITEM_TYPE, CMemberFuncJobItemProcessor<ITEM_TYPE, OBJECT_TYPE, FUNCTION_CLASS> > processor;
	processor.m_ItemProcessor.Init(pObject, pfnProcess, pfnBegin, pfnEnd);
	processor.Run(pItems, nItems, 1, nMaxParallel);
}

// Parallel Process that lets you specify threadpool
template <typename ITEM_TYPE>
inline void ParallelProcess(IThreadPool *pPool, ITEM_TYPE *pItems, unsigned nItems, void(*pfnProcess)(ITEM_TYPE &), void(*pfnBegin)() = NULL, void(*pfnEnd)() = NULL, int nMaxParallel = INT_MAX)
{
	CParallelProcessor<ITEM_TYPE, CFuncJobItemProcessor<ITEM_TYPE> > processor;
	processor.m_ItemProcessor.Init(pfnProcess, pfnBegin, pfnEnd);
	processor.Run(pItems, nItems, 1, nMaxParallel, pPool);
}

template <typename ITEM_TYPE, typename OBJECT_TYPE, typename FUNCTION_CLASS >
inline void ParallelProcess(IThreadPool *pPool, ITEM_TYPE *pItems, unsigned nItems, OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)(ITEM_TYPE &), void (FUNCTION_CLASS::*pfnBegin)() = NULL, void (FUNCTION_CLASS::*pfnEnd)() = NULL, int nMaxParallel = INT_MAX)
{
	CParallelProcessor<ITEM_TYPE, CMemberFuncJobItemProcessor<ITEM_TYPE, OBJECT_TYPE, FUNCTION_CLASS> > processor;
	processor.m_ItemProcessor.Init(pObject, pfnProcess, pfnBegin, pfnEnd);
	processor.Run(pItems, nItems, 1, nMaxParallel, pPool);
}

// ParallelProcessChunks lets you specify a minimum # of items to process per job. Use this when
// you may have a large set of work items which only take a small amount of time per item, and so
// need to reduce dispatch overhead.
template <typename ITEM_TYPE>
inline void ParallelProcessChunks(ITEM_TYPE *pItems, unsigned nItems, void(*pfnProcess)(ITEM_TYPE &), int nChunkSize, int nMaxParallel = INT_MAX)
{
	CParallelProcessor<ITEM_TYPE, CFuncJobItemProcessor<ITEM_TYPE> > processor;
	processor.m_ItemProcessor.Init(pfnProcess, NULL, NULL);
	processor.Run(pItems, nItems, nChunkSize, nMaxParallel);
}

template <typename ITEM_TYPE, typename OBJECT_TYPE, typename FUNCTION_CLASS >
inline void ParallelProcessChunks(ITEM_TYPE *pItems, unsigned nItems, OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)(ITEM_TYPE &), int nChunkSize, int nMaxParallel = INT_MAX)
{
	CParallelProcessor<ITEM_TYPE, CMemberFuncJobItemProcessor<ITEM_TYPE, OBJECT_TYPE, FUNCTION_CLASS> > processor;
	processor.m_ItemProcessor.Init(pObject, pfnProcess, NULL, NULL);
	processor.Run(pItems, nItems, nChunkSize, nMaxParallel);
}

template <typename ITEM_TYPE, typename OBJECT_TYPE, typename FUNCTION_CLASS >
inline void ParallelProcessChunks(IThreadPool *pPool, ITEM_TYPE *pItems, unsigned nItems, OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)(ITEM_TYPE &), int nChunkSize, int nMaxParallel = INT_MAX)
{
	CParallelProcessor<ITEM_TYPE, CMemberFuncJobItemProcessor<ITEM_TYPE, OBJECT_TYPE, FUNCTION_CLASS> > processor;
	processor.m_ItemProcessor.Init(pObject, pfnProcess, NULL, NULL);
	processor.Run(pItems, nItems, nChunkSize, nMaxParallel, pPool);
}


template <class CONTEXT_TYPE, class ITEM_PROCESSOR_TYPE>
class CParallelLoopProcessor
{
public:
	CParallelLoopProcessor()
	{
		m_nIndex = m_nLimit = 0;
		m_nChunkCount = 0;
		m_nActive = 0;
	}

	void Run(CONTEXT_TYPE *pContext, int nBegin, int nItems, int nChunkCount, int nMaxParallel = INT_MAX, IThreadPool *pThreadPool = NULL)
	{
		if (!nItems)
			return;

		if (!pThreadPool)
		{
			pThreadPool = g_pThreadPool;
		}

		m_pContext = pContext;
		m_nIndex = nBegin;
		m_nLimit = nBegin + nItems;
		nChunkCount = MAX(MIN(nItems, nChunkCount), 1);
		m_nChunkCount = (nItems + nChunkCount - 1) / nChunkCount;
		int nJobs = (nItems + m_nChunkCount - 1) / m_nChunkCount;
		if (nJobs > nMaxParallel)
		{
			nJobs = nMaxParallel;
		}

		if (!pThreadPool)									// only possible on linux
		{
			DoExecute();
			return;
		}

		int nThreads = pThreadPool->NumThreads();
		if (nJobs > nThreads)
		{
			nJobs = nThreads;
		}

		if (nJobs > 0)
		{
			CJob **jobs = (CJob **)stackalloc(nJobs * sizeof(CJob **));
			int i = nJobs;

			while (i--)
			{
				jobs[i] = pThreadPool->QueueCall(this, &CParallelLoopProcessor<CONTEXT_TYPE, ITEM_PROCESSOR_TYPE>::DoExecute);
			}

			DoExecute();

			for (i = 0; i < nJobs; i++)
			{
				jobs[i]->Abort(); // will either abort ones that never got a thread, or noop on ones that did
				jobs[i]->Release();
			}
		}
		else
		{
			DoExecute();
		}
	}

	ITEM_PROCESSOR_TYPE m_ItemProcessor;

private:
	void DoExecute()
	{
		m_ItemProcessor.Begin();
		for (;;)
		{
			int nIndex = m_nIndex.AtomicAdd(m_nChunkCount);
			if (nIndex < m_nLimit)
			{
				int nCount = MIN(m_nChunkCount, m_nLimit - nIndex);
				m_ItemProcessor.Process(m_pContext, nIndex, nCount);
			}
			else
			{
				break;
			}
		}
		m_ItemProcessor.End();
		--m_nActive;
	}

	CONTEXT_TYPE				*m_pContext;
	CInterlockedInt				m_nIndex;
	int							m_nLimit;
	int							m_nChunkCount;
	CInterlockedInt				m_nActive;
};

template < typename CONTEXT_TYPE >
inline void ParallelLoopProcess(IThreadPool *pPool, CONTEXT_TYPE *pContext, int nStart, int nCount, void(*pfnProcess)(CONTEXT_TYPE*, int, int), void(*pfnBegin)() = NULL, void(*pfnEnd)() = NULL, int nMaxParallel = INT_MAX)
{
	CParallelLoopProcessor< CONTEXT_TYPE, CLoopFuncJobItemProcessor< CONTEXT_TYPE > > processor;
	processor.m_ItemProcessor.Init(pfnProcess, pfnBegin, pfnEnd);
	processor.Run(pContext, nStart, nCount, 1, nMaxParallel, pPool);
}

template < typename CONTEXT_TYPE, typename OBJECT_TYPE, typename FUNCTION_CLASS >
inline void ParallelLoopProcess(IThreadPool *pPool, CONTEXT_TYPE *pContext, int nStart, int nCount, OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)(CONTEXT_TYPE*, int, int), void (FUNCTION_CLASS::*pfnBegin)() = NULL, void (FUNCTION_CLASS::*pfnEnd)() = NULL, int nMaxParallel = INT_MAX)
{
	CParallelLoopProcessor< CONTEXT_TYPE, CLoopMemberFuncJobItemProcessor<CONTEXT_TYPE, OBJECT_TYPE, FUNCTION_CLASS> > processor;
	processor.m_ItemProcessor.Init(pObject, pfnProcess, pfnBegin, pfnEnd);
	processor.Run(pContext, nStart, nCount, 1, nMaxParallel, pPool);
}

template < typename CONTEXT_TYPE >
inline void ParallelLoopProcessChunks(IThreadPool *pPool, CONTEXT_TYPE *pContext, int nStart, int nCount, int nChunkSize, void(*pfnProcess)(CONTEXT_TYPE*, int, int), void(*pfnBegin)() = NULL, void(*pfnEnd)() = NULL, int nMaxParallel = INT_MAX)
{
	CParallelLoopProcessor< CONTEXT_TYPE, CLoopFuncJobItemProcessor< CONTEXT_TYPE > > processor;
	processor.m_ItemProcessor.Init(pfnProcess, pfnBegin, pfnEnd);
	processor.Run(pContext, nStart, nCount, nChunkSize, nMaxParallel, pPool);
}

template < typename CONTEXT_TYPE, typename OBJECT_TYPE, typename FUNCTION_CLASS >
inline void ParallelLoopProcessChunks(IThreadPool *pPool, CONTEXT_TYPE *pContext, int nStart, int nCount, int nChunkSize, OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)(CONTEXT_TYPE*, int, int), void (FUNCTION_CLASS::*pfnBegin)() = NULL, void (FUNCTION_CLASS::*pfnEnd)() = NULL, int nMaxParallel = INT_MAX)
{
	CParallelLoopProcessor< CONTEXT_TYPE, CLoopMemberFuncJobItemProcessor<CONTEXT_TYPE, OBJECT_TYPE, FUNCTION_CLASS> > processor;
	processor.m_ItemProcessor.Init(pObject, pfnProcess, pfnBegin, pfnEnd);
	processor.Run(pContext, nStart, nCount, nChunkSize, nMaxParallel, pPool);
}

template <class Derived>
class CParallelProcessorBase
{
protected:
	typedef CParallelProcessorBase<Derived> ThisParallelProcessorBase_t;
	typedef Derived ThisParallelProcessorDerived_t;

public:
	CParallelProcessorBase()
	{
		m_nActive = 0;
	}

protected:
	void Run(int nMaxParallel = INT_MAX, int threadOverride = -1)
	{
		int i = g_pThreadPool->NumIdleThreads();

		if (nMaxParallel < i)
		{
			i = nMaxParallel;
		}

		while (i-- > 0)
		{
			if (threadOverride == -1 || i == threadOverride - 1)
			{
				++m_nActive;
				ThreadExecute(this, &ThisParallelProcessorBase_t::DoExecute)->Release();
			}
		}

		if (threadOverride == -1 || threadOverride == 0)
		{
			++m_nActive;
			DoExecute();
		}

		while (m_nActive)
		{
			ThreadPause();
		}
	}

protected:
	void OnBegin() {}
	bool OnProcess() { return false; }
	void OnEnd() {}

private:
	void DoExecute()
	{
		static_cast<Derived *>(this)->OnBegin();

		while (static_cast<Derived *>(this)->OnProcess())
			continue;

		static_cast<Derived *>(this)->OnEnd();

		--m_nActive;
	}

	CInterlockedInt				m_nActive;
};




//-----------------------------------------------------------------------------
// Raw thread launching
//-----------------------------------------------------------------------------

inline unsigned FunctorExecuteThread(void *pParam)
{
	CFunctor *pFunctor = (CFunctor *)pParam;
	(*pFunctor)();
	pFunctor->Release();
	return 0;
}

inline ThreadHandle_t ThreadExecuteSoloImpl(CFunctor *pFunctor, const char *pszName = NULL)
{
	ThreadHandle_t hThread;
	hThread = CreateSimpleThread(FunctorExecuteThread, pFunctor);
	if (pszName)
	{
		ThreadSetDebugName((ThreadId_t)hThread, pszName);
	}
	return hThread;
}

inline ThreadHandle_t ThreadExecuteSolo(CJob *pJob) { return ThreadExecuteSoloImpl(CreateFunctor(pJob, &CJob::Execute), pJob->Describe()); }

template <typename T1>
inline ThreadHandle_t ThreadExecuteSolo(const char *pszName, T1 a1) { return ThreadExecuteSoloImpl(CreateFunctor(a1), pszName); }

template <typename T1, typename T2>
inline ThreadHandle_t ThreadExecuteSolo(const char *pszName, T1 a1, T2 a2) { return ThreadExecuteSoloImpl(CreateFunctor(a1, a2), pszName); }

template <typename T1, typename T2, typename T3>
inline ThreadHandle_t ThreadExecuteSolo(const char *pszName, T1 a1, T2 a2, T3 a3) { return ThreadExecuteSoloImpl(CreateFunctor(a1, a2, a3), pszName); }

template <typename T1, typename T2, typename T3, typename T4>
inline ThreadHandle_t ThreadExecuteSolo(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4) { return ThreadExecuteSoloImpl(CreateFunctor(a1, a2, a3, a4), pszName); }

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline ThreadHandle_t ThreadExecuteSolo(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) { return ThreadExecuteSoloImpl(CreateFunctor(a1, a2, a3, a4, a5), pszName); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline ThreadHandle_t ThreadExecuteSolo(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6) { return ThreadExecuteSoloImpl(CreateFunctor(a1, a2, a3, a4, a5, a6), pszName); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline ThreadHandle_t ThreadExecuteSolo(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7) { return ThreadExecuteSoloImpl(CreateFunctor(a1, a2, a3, a4, a5, a6, a7), pszName); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline ThreadHandle_t ThreadExecuteSolo(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8) { return ThreadExecuteSoloImpl(CreateFunctor(a1, a2, a3, a4, a5, a6, a7, a8), pszName); }

template <typename T1, typename T2>
inline ThreadHandle_t ThreadExecuteSoloRef(const char *pszName, T1 a1, T2 a2) { return ThreadExecuteSoloImpl(CreateRefCountingFunctor(a1, a2), pszName); }

template <typename T1, typename T2, typename T3>
inline ThreadHandle_t ThreadExecuteSoloRef(const char *pszName, T1 a1, T2 a2, T3 a3) { return ThreadExecuteSoloImpl(CreateRefCountingFunctor(a1, a2, a3), pszName); }

template <typename T1, typename T2, typename T3, typename T4>
inline ThreadHandle_t ThreadExecuteSoloRef(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4) { return ThreadExecuteSoloImpl(CreateRefCountingFunctor(a1, a2, a3, a4), pszName); }

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline ThreadHandle_t ThreadExecuteSoloRef(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) { return ThreadExecuteSoloImpl(CreateRefCountingFunctor(a1, a2, a3, a4, a5), pszName); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline ThreadHandle_t ThreadExecuteSoloRef(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6) { return ThreadExecuteSoloImpl(CreateRefCountingFunctor(a1, a2, a3, a4, a5, a6), pszName); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline ThreadHandle_t ThreadExecuteSoloRef(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7) { return ThreadExecuteSoloImpl(CreateRefCountingFunctor(a1, a2, a3, a4, a5, a6, a7), pszName); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline ThreadHandle_t ThreadExecuteSoloRef(const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8) { return ThreadExecuteSoloImpl(CreateRefCountingFunctor(a1, a2, a3, a4, a5, a6, a7, a8), pszName); }

//-----------------------------------------------------------------------------

inline bool IThreadPool::YieldWait(CThreadEvent &theEvent, unsigned timeout)
{
	CThreadEvent *pEvent = &theEvent;
	return (YieldWait(&pEvent, 1, true, timeout) != TW_TIMEOUT);
}

inline bool IThreadPool::YieldWait(CJob *pJob, unsigned timeout)
{
	return (YieldWait(&pJob, 1, true, timeout) != TW_TIMEOUT);
}

//-----------------------------------------------------------------------------

inline JobStatus_t CJob::Execute()
{
	if (IsFinished())
	{
		return m_status;
	}

	AUTO_LOCK(m_mutex);
	AddRef();

	JobStatus_t result;

	switch (m_status)
	{
	case JOB_STATUS_UNSERVICED:
	case JOB_STATUS_PENDING:
	{
		// Service it
		m_status = JOB_STATUS_INPROGRESS;
		result = m_status = DoExecute();
		DoCleanup();
		m_CompleteEvent.Set();
		break;
	}

	case JOB_STATUS_INPROGRESS:
		//AssertMsg(0, "Mutex Should have protected use while processing");
		// fall through...

	case JOB_OK:
	case JOB_STATUS_ABORTED:
		result = m_status;
		break;

	default:
		//AssertMsg(m_status < JOB_OK, "Unknown job state");
		result = m_status;
	}

	Release();

	return result;
}


//---------------------------------------------------------

inline JobStatus_t CJob::TryExecute()
{
	// TryLock() would only fail if another thread has entered
	// Execute() or Abort()
	if (!IsFinished() && TryLock())
	{
		// ...service the request
		Execute();
		Unlock();
	}
	return m_status;
}

//---------------------------------------------------------

inline JobStatus_t CJob::Abort(bool bDiscard)
{
	if (IsFinished())
	{
		return m_status;
	}

	AUTO_LOCK(m_mutex);
	AddRef();

	JobStatus_t result;

	switch (m_status)
	{
	case JOB_STATUS_UNSERVICED:
	case JOB_STATUS_PENDING:
	{
		result = m_status = DoAbort(bDiscard);
		if (bDiscard)
			DoCleanup();
		m_CompleteEvent.Set();
	}
	break;

	case JOB_STATUS_ABORTED:
	case JOB_STATUS_INPROGRESS:
	case JOB_OK:
		result = m_status;
		break;

	default:
		//AssertMsg(m_status < JOB_OK, "Unknown job state");
		result = m_status;
	}

	Release();

	return result;
}

#include "memdbgoff.h"

#endif