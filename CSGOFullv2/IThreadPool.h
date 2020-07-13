#pragma once
#include "refcount.h"
#include "functors.h"

class CThreadEvent;
class CJob;
typedef bool(*JobFilter_t)(CJob *);

template <typename ITEM_TYPE, class ITEM_PROCESSOR_TYPE, int ID_TO_PREVENT_COMDATS_IN_PROFILES = 1>
class CParallelProcessor;

#define RetAddRef( p ) ( (p)->AddRef(), (p) )

enum ThreeState_t
{
	TRS_FALSE,
	TRS_TRUE,
	TRS_NONE,
};

enum JobPriority_t
{
	JP_LOW,
	JP_NORMAL,
	JP_HIGH
};

#define TP_MAX_POOL_THREADS	64
struct ThreadPoolStartParams_t
{
	ThreadPoolStartParams_t(bool bIOThreads = false, unsigned nThreads = -1, int *pAffinities = NULL, ThreeState_t fDistribute = TRS_NONE, unsigned nStackSize = -1, int iThreadPriority = SHRT_MIN)
		: bIOThreads(bIOThreads), nThreads(nThreads), fDistribute(fDistribute), nStackSize(nStackSize), iThreadPriority(iThreadPriority)
	{
		bUseAffinityTable = (pAffinities != NULL) && (fDistribute == TRS_TRUE) && (nThreads != -1);
		if (bUseAffinityTable)
		{
			// user supplied an optional 1:1 affinity mapping to override normal distribute behavior
			nThreads = min(TP_MAX_POOL_THREADS, nThreads);
			for (unsigned int i = 0; i < nThreads; i++)
			{
				iAffinityTable[i] = pAffinities[i];
			}
		}
	}

	int				nThreads;
	ThreeState_t	fDistribute;
	int				nStackSize;
	int				iThreadPriority;
	int				iAffinityTable[TP_MAX_POOL_THREADS];

	bool			bIOThreads : 1;
	bool			bUseAffinityTable : 1;
};

class IThreadPool : public IRefCounted
{
public:
	virtual ~IThreadPool() {};
	// Thread functions
	virtual bool Start(const ThreadPoolStartParams_t &startParams = ThreadPoolStartParams_t()) = 0;
	virtual bool Stop(int timeout = TT_INFINITE) = 0;
	// Functions for any thread
	virtual unsigned GetJobCount() = 0;
	virtual int NumThreads() = 0;
	virtual int NumIdleThreads() = 0;
	// Pause/resume processing jobs
	virtual int SuspendExecution() = 0;
	virtual int ResumeExecution() = 0;
	// Offer the current thread to the pool
	virtual int YieldWait(CThreadEvent **pEvents, int nEvents, bool bWaitAll = true, unsigned timeout = TT_INFINITE) = 0;
	virtual int YieldWait(CJob **, int nJobs, bool bWaitAll = true, unsigned timeout = TT_INFINITE) = 0;
	virtual void Yield_(unsigned timeout) = 0;

	bool YieldWait(CThreadEvent &event, unsigned timeout = TT_INFINITE);
	bool YieldWait(CJob *, unsigned timeout = TT_INFINITE);

	// Add a native job to the queue (master thread)
	// See AddPerFrameJob below if you want to add a job that
	// wants to be run before the end of the frame
	virtual void AddJob_(CJob *) = 0;
	// Add an function object to the queue (master thread)
	virtual void AddFunctor(CFunctor *pFunctor, CJob **ppJob = NULL, const char *pszDescription = NULL, unsigned flags = 0) { AddFunctorInternal(RetAddRef(pFunctor), ppJob, pszDescription, flags); }
	// Change the priority of an active job
	virtual void ChangePriority(CJob *p, JobPriority_t priority) = 0;
	// Bulk job manipulation (blocking)
	int ExecuteAll(JobFilter_t pfnFilter = NULL) { return ExecuteToPriority(JP_LOW, pfnFilter); }
	virtual int ExecuteToPriority(JobPriority_t toPriority, JobFilter_t pfnFilter = NULL) = 0;
	virtual int AbortAll() = 0;
	// Add a native job to the queue (master thread)
	// Call YieldWaitPerFrameJobs() to wait only until all per-frame jobs are done
	virtual void AddPerFrameJob(CJob *) = 0;

	//-----------------------------------------------------
	// Add an arbitrary call to the queue (master thread) 
	//
	// Avert thy eyes! Imagine rather:
	//
	// CJob *AddCall( <function>, [args1, [arg2,]...]
	// CJob *AddCall( <object>, <function>, [args1, [arg2,]...]
	// CJob *AddRefCall( <object>, <function>, [args1, [arg2,]...]
	// CJob *QueueCall( <function>, [args1, [arg2,]...]
	// CJob *QueueCall( <object>, <function>, [args1, [arg2,]...]
	//-----------------------------------------------------

#define DEFINE_NONMEMBER_ADD_CALL(N) \
		template <typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddCall(FUNCTION_RETTYPE (*pfnProxied)( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateFunctor( pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-------------------------------------

#define DEFINE_MEMBER_ADD_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-------------------------------------

#define DEFINE_CONST_MEMBER_ADD_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-------------------------------------

#define DEFINE_REF_COUNTING_MEMBER_ADD_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddRefCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-------------------------------------

#define DEFINE_REF_COUNTING_CONST_MEMBER_ADD_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddRefCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-----------------------------------------------------------------------------

#define DEFINE_NONMEMBER_QUEUE_CALL(N) \
		template <typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueCall(FUNCTION_RETTYPE (*pfnProxied)( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateFunctor( pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			return pJob; \
		}

	//-------------------------------------

#define DEFINE_MEMBER_QUEUE_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			return pJob; \
		}

	//-------------------------------------

#define DEFINE_CONST_MEMBER_QUEUE_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			return pJob; \
		}

	//-------------------------------------

#define DEFINE_REF_COUNTING_MEMBER_QUEUE_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueRefCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			return pJob; \
		}

	//-------------------------------------

#define DEFINE_REF_COUNTING_CONST_MEMBER_QUEUE_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueRefCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			\
			return pJob; \
		}

	FUNC_GENERATE_ALL(DEFINE_NONMEMBER_ADD_CALL);
	FUNC_GENERATE_ALL(DEFINE_MEMBER_ADD_CALL);
	FUNC_GENERATE_ALL(DEFINE_CONST_MEMBER_ADD_CALL);
	FUNC_GENERATE_ALL(DEFINE_REF_COUNTING_MEMBER_ADD_CALL);
	FUNC_GENERATE_ALL(DEFINE_REF_COUNTING_CONST_MEMBER_ADD_CALL);
	FUNC_GENERATE_ALL(DEFINE_NONMEMBER_QUEUE_CALL);
	FUNC_GENERATE_ALL(DEFINE_MEMBER_QUEUE_CALL);
	FUNC_GENERATE_ALL(DEFINE_CONST_MEMBER_QUEUE_CALL);
	FUNC_GENERATE_ALL(DEFINE_REF_COUNTING_MEMBER_QUEUE_CALL);
	FUNC_GENERATE_ALL(DEFINE_REF_COUNTING_CONST_MEMBER_QUEUE_CALL);

#undef DEFINE_NONMEMBER_ADD_CALL
#undef DEFINE_MEMBER_ADD_CALL
#undef DEFINE_CONST_MEMBER_ADD_CALL
#undef DEFINE_REF_COUNTING_MEMBER_ADD_CALL
#undef DEFINE_REF_COUNTING_CONST_MEMBER_ADD_CALL
#undef DEFINE_NONMEMBER_QUEUE_CALL
#undef DEFINE_MEMBER_QUEUE_CALL
#undef DEFINE_CONST_MEMBER_QUEUE_CALL
#undef DEFINE_REF_COUNTING_MEMBER_QUEUE_CALL
#undef DEFINE_REF_COUNTING_CONST_MEMBER_QUEUE_CALL


private:
	virtual void AddFunctorInternal(CFunctor *, CJob ** = NULL, const char *pszDescription = NULL, unsigned flags = 0) = 0;


	// Services for internal use by job instances
	friend class CJob;

	virtual CJob *GetDummyJob() = 0;

public:
	virtual void Distribute(bool bDistribute = true, int *pAffinityTable = NULL) = 0;

	virtual bool Start(const ThreadPoolStartParams_t &startParams, const char *pszNameOverride) = 0;

	virtual int YieldWaitPerFrameJobs() = 0;

	
	template <typename ITEM_TYPE, class ITEM_PROCESSOR_TYPE, int ID_TO_PREVENT_COMDATS_IN_PROFILES>
	__forceinline CJob* QueueCall_Game(CParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES> *processor, void(__thiscall CParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE, ID_TO_PREVENT_COMDATS_IN_PROFILES>::* DoExecuteFunc)())
	{
		CJob* pJob;
		CFunctor *q = CreateFunctor(processor, DoExecuteFunc);
		AddFunctorInternal(q, &pJob, NULL, JF_QUEUE);
		return pJob;
		//return (*(CJob*(__thiscall **)(IThreadPool*, CParallelProcessor_Queue_t *, ITEM_TYPE **, DWORD, signed int))(*(DWORD *)(DWORD)g_pThreadPool + 80))(g_pThreadPool, queue, &pJob, 0, JF_QUEUE);
	}
};