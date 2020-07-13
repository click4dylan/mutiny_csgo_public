#include "precompiled.h"
#include "Profiler.h"
#include "CSGO_HX.h"
#include "Draw.h"
#include "cx_strenc.h"

#include "Adriel/stdafx.hpp"
#include "Adriel/renderer.hpp"

void ProfStats::EncName_s::Encrypt() const
{
	DWORD dwKey = 0x13371337;
	for (size_t i = 0; i < 128; i++, dwKey = _rotr(dwKey, 8))
		*(BYTE*)((uintptr_t)this + i) = *(BYTE*)((uintptr_t)this + i) ^ (char)dwKey;
}

ProfStats::EncName_s ProfStats::EncName_s::Decrypt()
{
	//			/*EncName_s ret(*this);
	//
	//			int dwKey = 0x13371337;
	//			for (size_t i = 0; i < ret.m_iNameLength; i++, dwKey = _rotr(dwKey, 8))
	//				*(unsigned char*)((uintptr_t)ret.m_szName + i) = *(unsigned char*)((uintptr_t)ret.m_szName + i) ^ (char)dwKey;
	//			*(unsigned char*)((uintptr_t)ret.m_szName + m_iNameLength) = '\0';
	//
	//			return ret;
	//*/

	EncName_s ret(*this);
	int dwKey = 0x13371337;
	for (size_t i = 0; i < 128; i++, dwKey = _rotr(dwKey, 8))
		*(unsigned char*)((uintptr_t)ret.m_szName + i) = *(unsigned char*)((uintptr_t)ret.m_szName + i) ^ (char)dwKey;

	return ret;
}

ProfStats::ProfStats(const char* name)
{
	m_name = EncName_s(name);
	m_iNumCalls = 0;
	m_TotalTime = m_LastFullResult = m_LastSingleResult = m_LastSlowestCall = 0;
	m_StartTime = QPCTime();//std::chrono::steady_clock::time_point();

	g_ProfStatsMutex.lock();
	m_pNext = g_pProfStats;
	g_pProfStats = this;
	g_ProfStatsMutex.unlock();
}

void ProfStats::StartProfiling()
{
	m_Mutex.lock();
	m_StartTime = QPCTime();//std::chrono::high_resolution_clock::now();
}

void ProfStats::EndProfiling()
{
	m_iNumCalls++;

	const auto now = QPCTime();//std::chrono::high_resolution_clock::now();
	const auto deltaSecs = now - m_StartTime;//std::chrono::duration_cast<std::chrono::milliseconds>(now - m_StartTime).count();

	m_TotalTime += deltaSecs;
	m_LastSingleResult = deltaSecs;

	if (deltaSecs > m_LastSlowestCall)
		m_LastSlowestCall = deltaSecs;

	if (m_iNumCalls == PROFILE_CALLS)
	{
		m_LastFullResult = m_TotalTime;
		m_iNumCalls = m_TotalTime = 0;
	}

	m_Mutex.unlock();
}

void ProfStats::DrawProfiledFunctions()
{
	static bool GotCommandLine = false;
	static bool DoDraw = false;
	static int longestName = 95;
	if (!GotCommandLine)
	{
		char* cmdline = GetCommandLineA();
		DoDraw = strstr(cmdline, charenc("-benchmark")) ? true : false;
		GotCommandLine = true;
	}
	if (DoDraw)
	{
		//const float x_sep = 120.f;
		//render::get().add_text(ImVec2(5.f, 5.f), Color::White().ToImGUI(), NO_TFLAG, TAHOMA_14, XorStr("Benchmarking"));
		//render::get().add_text(ImVec2(5.f + longestName, 5.f), Color::White().ToImGUI(), NO_TFLAG, TAHOMA_14, "| 500 calls");
		//render::get().add_text(ImVec2(5.f + longestName + (1.f * x_sep), 5.f), Color::White().ToImGUI(), NO_TFLAG, TAHOMA_14, "| last call");
		//render::get().add_text(ImVec2(5.f + longestName + (2.f * x_sep), 5.f), Color::White().ToImGUI(), NO_TFLAG, TAHOMA_14, "| slowest call");
		//render::get().add_text(ImVec2(5.f + longestName + (3.f * x_sep), 5.f), Color::White().ToImGUI(), NO_TFLAG, TAHOMA_14, "| time since last call");
		
		DrawString(ESPFONT, 5, 5, Color(255, 255, 255), FONT_LEFT, charenc("Benchmark Results:"));
		DrawString(ESPFONT, 5 + longestName, 5, Color(255, 255, 255), FONT_LEFT, charenc("| 500 calls"));
		DrawString(ESPFONT, 5 + longestName + 120, 5, Color(255, 255, 255), FONT_LEFT, charenc("| last call"));
		DrawString(ESPFONT, 5 + longestName + 240, 5, Color(255, 255, 255), FONT_LEFT, charenc("| slowest call"));
		DrawString(ESPFONT, 5 + longestName + 360, 5, Color(255, 255, 255), FONT_LEFT, charenc("| time since last call"));
		
		int numdrawn = 0;

		g_ProfStatsMutex.lock();
		ProfStats* statistic = g_pProfStats;
		while (statistic)
		{
			static char tmpstr[512];
			sprintf(tmpstr, "%s", statistic->m_name.Decrypt().m_szName);
			statistic->m_Mutex.lock();
			auto strsize = MultiByteToWideChar(CP_UTF8, 0, statistic->m_name.Decrypt().m_szName, strlen(statistic->m_name.Decrypt().m_szName) + 1, nullptr, 0);
			auto pszStringWide = new wchar_t[strsize];
			MultiByteToWideChar(CP_UTF8, 0, statistic->m_name.Decrypt().m_szName, strlen(statistic->m_name.Decrypt().m_szName) + 1, pszStringWide, strsize);
			int wide, tall;
			Interfaces::Surface->GetTextSize(ESPFONT, pszStringWide, wide, tall);
			if (wide > longestName)
				longestName = wide + 5;

			delete[] pszStringWide;

		
			//ImVec2 text_size = render::get().tahoma_14->CalcTextSizeA(render::get().tahoma_14->FontSize, FLT_MAX, 0.f, statistic->m_name.Decrypt().m_szName);
			//if (text_size.x > longestName)
			//	longestName = text_size.x + 5.f;

			//decrypts(0)
			// Name
			DrawString(ESPFONT, 5, 20 + (10 * numdrawn), Color(255, 255, 255), FONT_LEFT, statistic->m_name.Decrypt().m_szName);
			//render::get().add_text(ImVec2(5.f, 20.f + (10.f * numdrawn)), Color::White().ToImGUI(), NO_TFLAG, font_flags::TAHOMA_14, statistic->m_name.Decrypt().m_szName);

#ifdef MORE_THREAD_SEFETY
			// 500 calls
			DrawString(ESPFONT, 5 + longestName, 20 + (10 * numdrawn), Color(255, 255, 255), FONT_LEFT, charenc("| %f msecs"), statistic->m_LastFullResult.load() * 1000.0);

			// last single call
			DrawString(ESPFONT, 5 + longestName + 60, 20 + (10 * numdrawn), Color(255, 255, 255), FONT_LEFT, charenc("| %f msecs"), statistic->m_LastSingleResult.load() * 1000.0);

			// last slowest call
			DrawString(ESPFONT, 5 + longestName + 120, 20 + (10 * numdrawn), Color(255, 255, 255), FONT_LEFT, charenc("| %f msecs"), statistic->m_LastSlowestCall.load() * 1000.0);

			// time since last START_PROFILING
			DrawString(ESPFONT, 5 + longestName + 360, 20 + (10 * numdrawn), Color(255, 255, 255), FONT_LEFT, charenc("| %f msecs"), (QPCTime() - statistic->m_StartTime.load()) * 1000.0);
#else
			// 500 calls
			DrawString(ESPFONT, 5 + longestName, 20 + (10 * numdrawn), Color(255, 255, 255), FONT_LEFT, charenc("| %f msecs"), statistic->m_LastFullResult * 1000.0);
			//render::get().add_text(ImVec2(5.f + longestName, 20.f + (10.f * numdrawn)), Color::White().ToImGUI(), NO_TFLAG, font_flags::TAHOMA_14, "| %f msecs", statistic->m_LastFullResult * 1000.0);
			
			// last single call
			DrawString(ESPFONT, 5 + longestName + 120, 20 + (10 * numdrawn), Color(255, 255, 255), FONT_LEFT, charenc("| %f msecs"), statistic->m_LastSingleResult * 1000.0);
			//render::get().add_text(ImVec2(5.f + longestName + 120.f, 20.f + (10.f * numdrawn)), Color::White().ToImGUI(), NO_TFLAG, font_flags::TAHOMA_14, "| %f msecs", statistic->m_LastSingleResult * 1000.0);

			// last slowest call
			DrawString(ESPFONT, 5 + longestName + 240, 20 + (10 * numdrawn), Color(255, 255, 255), FONT_LEFT, charenc("| %f msecs"), statistic->m_LastSlowestCall * 1000.0);
			//render::get().add_text(ImVec2(5.f + longestName + 240.f, 20.f + (10.f * numdrawn)), Color::White().ToImGUI(), NO_TFLAG, font_flags::TAHOMA_14, "| %f msecs", statistic->m_LastSlowestCall * 1000.0);

			// time since last START_PROFILING
			DrawString(ESPFONT, 5 + longestName + 360, 20 + (10 * numdrawn), Color(255, 255, 255), FONT_LEFT, charenc("| %f msecs"), (QPCTime() - statistic->m_StartTime) * 1000.0);
			//render::get().add_text(ImVec2(5.f + longestName + 360.f, 20.f + (10.f * numdrawn)), Color::White().ToImGUI(), NO_TFLAG, font_flags::TAHOMA_14, "| %f msecs", statistic->m_LastSlowestCall * 1000.0);
#endif

			numdrawn++;

			statistic->m_Mutex.unlock();
			statistic = statistic->m_pNext;
		}
		g_ProfStatsMutex.unlock();

	}
}
