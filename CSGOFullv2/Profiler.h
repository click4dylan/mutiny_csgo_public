#ifndef PROFILE_H
#define PROFILE_H

#define PROFILE_CALLS 500
#include <chrono>
#include <mutex>
#ifdef MORE_THREAD_SEFETY
#include <atomic>
#endif

static std::mutex g_ProfStatsMutex;
static class ProfStats* g_pProfStats = nullptr;

class ProfStats
{
public:
	struct EncName_s
	{
		EncName_s()
		{
			memset(m_szName, 0x0, 128);
		}

		explicit EncName_s(const char* name)
		{
			const size_t m_iNameLength = strlen(name);
			memset(m_szName, 0x0, 128);
			memcpy(m_szName, name, m_iNameLength + 1);
			Encrypt();
		}

		EncName_s(EncName_s& other)
		{
			memset(m_szName, 0x0, 128);
			memcpy(m_szName, other.m_szName, 128);
		}

		~EncName_s()
		{
			memset(m_szName, 0x0, 128);
		}

		__forceinline void Encrypt() const;

		__forceinline EncName_s Decrypt();

		char	m_szName[128]{};
	};

	explicit ProfStats(const char* name);
	~ProfStats() = default;

	void StartProfiling();

	void EndProfiling();

	static void DrawProfiledFunctions();

private:
	EncName_s m_name;

	int		m_iNumCalls;

	//std::chrono::steady_clock::time_point	m_StartTime;
	double m_StartTime;

	//long long	m_TotalTime;
	double m_TotalTime;

#ifdef MORE_THREAD_SEFETY
	std::atomic_llong m_LastFullResult;
	std::atomic_llong m_LastSingleResult;
	std::atomic_llong m_LastSlowestCall;
#else
	//long long	m_LastFullResult;
	//long long	m_LastSingleResult;
	//long long	m_LastSlowestCall;
	double m_LastFullResult;
	double m_LastSingleResult;
	double m_LastSlowestCall;
#endif

	std::mutex m_Mutex;

	ProfStats*	m_pNext;
};

#ifdef PROFILE

#define START_PROFILING static ProfStats* prof = new ProfStats(__FUNCTION__); \
						prof->StartProfiling();

#define END_PROFILING			prof->EndProfiling();

#define START_PROFILING_CUSTOM(varname, printname) static ProfStats* varname = new ProfStats(##printname); \
													varname->StartProfiling();

#define END_PROFILING_CUSTOM(varname) varname->EndProfiling();

#define DRAW_PROFILED_FUNCTIONS() ProfStats::DrawProfiledFunctions();

#else

#define START_PROFILING
#define START_PROFILING_CUSTOM(varname, printname)
#define END_PROFILING
#define END_PROFILING_CUSTOM(varname)
#define DRAW_PROFILED_FUNCTIONS()

#endif

#endif