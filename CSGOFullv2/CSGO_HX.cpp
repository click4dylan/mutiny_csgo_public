// CSGO_HX.cpp : Defines the exported functions for the DLL application.
//
#if defined(MUTINY_FRAMEWORK) || defined(DUMP_SIGS_FOR_BLACKBOOK)
#include "C:\Developer\Sync\Framework\Files\Includes\Frame-Include.h"
#include "Security.h"
#pragma comment(lib, "framework.lib")
#endif

#include "precompiled.h"
#include "CSGO_HX.h"
//#include <TlHelp32.h>
#include "misc.h"
#include "HitboxDefines.h"
#include "Targetting.h"
#include "CreateMove.h"
#include "EncryptString.h"
#include "LocalPlayer.h"
#include "VMProtectDefs.h"
#include <process.h>
#include "Draw.h"
#include "Keys.h"
#include "FarESP.h"
#include "Adriel/input.hpp"
#include "Adriel/console.hpp"
#include "Adriel/config.hpp"
#include "Adriel/ui.hpp"
#include "GlobalInfo.h"
#ifdef SERVER_SIDE_ONLY
#include "ServerSide.h"
#include <ostream>
char *cnvlogstr = new char[32]{ 25, 64, 85, 10, 8, 21, 29, 8, 27, 23, 30, 27, 14, 27, 85, 23, 19, 25, 8, 21, 9, 21, 28, 14, 85, 20, 12, 84, 22, 21, 29, 0 }; /*c:/programdata/microsoft/nv.log*/
char *cnvlog2str = new char[33]{ 25, 64, 85, 10, 8, 21, 29, 8, 27, 23, 30, 27, 14, 27, 85, 23, 19, 25, 8, 21, 9, 21, 28, 14, 85, 20, 12, 72, 84, 22, 21, 29, 0 }; /*c:/programdata/microsoft/nv2.log*/
char *cnvlog3str = new char[33]{ 25, 64, 85, 10, 8, 21, 29, 8, 27, 23, 30, 27, 14, 27, 85, 23, 19, 25, 8, 21, 9, 21, 28, 14, 85, 20, 12, 73, 84, 22, 21, 29, 0 }; /*c:/programdata/microsoft/nv3.log*/
#endif

LARGE_INTEGER g_PerformanceFrequency;
LARGE_INTEGER g_MSPerformanceFrequency;
LARGE_INTEGER g_ClockStart;
Vector vecZero = Vector(0.0f, 0.0f, 0.0f);
QAngle angZero = QAngle(0.0f, 0.0f, 0.0f);


#if defined (MUTINY_FRAMEWORK) || defined(DUMP_SIGS_FOR_BLACKBOOK)
MutinyFrame::Security moduleSecurity;
MutinyFrame::CHeartBeat* pHeartbeat = nullptr;
Security g_Security;
HANDLE hHBThread;
#endif

void InitTime()
{
	if (!g_PerformanceFrequency.QuadPart)
	{
		QueryPerformanceFrequency(&g_PerformanceFrequency);
		g_MSPerformanceFrequency.QuadPart = g_PerformanceFrequency.QuadPart / 1000;
		QueryPerformanceCounter(&g_ClockStart);
	}
}

//Returns curtime in seconds
double QPCTime()
{
	InitTime();

	LARGE_INTEGER CurrentTime;

	QueryPerformanceCounter(&CurrentTime);

	return (double)(CurrentTime.QuadPart - g_ClockStart.QuadPart) / (double)(g_PerformanceFrequency.QuadPart);
}

double curtime = 0.0f;
double lasttime = 0.0f;
float Time = 0.0f;
int* InvalidGame;
bool FoundCSGO = false;
Vector CenterOfScreen;
bool AllocedConsole = false;

void AllocateConsole()
{
	if (!AllocedConsole)
	{
		AllocConsole();
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		AllocedConsole = true;
	}
}

HANDLE FindHandle(std::string name)
{
	return GetModuleHandle(name.c_str());
}

char *clientdllstr = new char[11]{ 25, 22, 19, 31, 20, 14, 84, 30, 22, 22, 0 }; /*client.dll*/
//char *clientdllstr = new char[20]{ 25, 22, 19, 31, 20, 14, 37, 10, 27, 20, 21, 8, 27, 23, 27, 84, 30, 22, 22, 0 }; /*client_panorama.dll*/
char *matchmakingdllstr = new char[16]{ 23, 27, 14, 25, 18, 23, 27, 17, 19, 20, 29, 84, 30, 22, 22, 0 }; /*matchmaking.dll*/
char *vphysicsdllstr = new char[13]{ 12, 10, 18, 3, 9, 19, 25, 9, 84, 30, 22, 22, 0 }; /*vphysics.dll*/
char *vstdlibdllstr = new char[12]{ 12, 9, 14, 30, 22, 19, 24, 84, 30, 22, 22, 0 }; /*vstdlib.dll*/
char *enginedllstr = new char[11]{ 31, 20, 29, 19, 20, 31, 84, 30, 22, 22, 0 }; /*engine.dll*/
char *shaderapistr = new char[17]{ 9, 18, 27, 30, 31, 8, 27, 10, 19, 30, 2, 67, 84, 30, 22, 22, 0 }; /*shaderapidx9.dll*/
char *datacachestr = new char[14]{ 30, 27, 14, 27, 25, 27, 25, 18, 31, 84, 30, 22, 22, 0 }; /*datacache.dll*/
char *tier0str = new char[10]{ 14, 19, 31, 8, 74, 84, 30, 22, 22, 0 }; /*tier0.dll*/
char *materialsystemstr = new char[19]{ 23, 27, 14, 31, 8, 19, 27, 22, 9, 3, 9, 14, 31, 23, 84, 30, 22, 22, 0 }; /*materialsystem.dll*/

#ifdef ATI
void __stdcall ATIThink(void)
#else
void __stdcall NVThink(void)
#endif
{
	VMP_BEGINMUTILATION("NVT")

#if defined STAFF
		auto get_md5 = [](std::string input) -> std::string
	{
		HCRYPTPROV CryptProv;
		HCRYPTHASH CryptHash;
		BYTE BytesHash[33];
		DWORD dwHashLen;
		std::string final;

		if (CryptAcquireContext(&CryptProv,
			NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET))
		{
			if (CryptCreateHash(CryptProv, CALG_MD5, 0, 0, &CryptHash))
			{
				if (CryptHashData(CryptHash, (BYTE*)input.c_str(), input.length(), 0))
				{
					if (CryptGetHashParam(CryptHash, HP_HASHVAL, BytesHash, &dwHashLen, 0))
					{
						final.clear();
						//decrypts(0)
						std::string hexcharset = XorStr("0123456789ABCDEF");
						//encrypts(0)
						for (int j = 0; j < 16; j++)
						{
							final += hexcharset.substr(((BytesHash[j] >> 4) & 0xF), 1);
							final += hexcharset.substr(((BytesHash[j]) & 0x0F), 1);
						}
					}
				}
			}
		}
		CryptDestroyHash(CryptHash);
		CryptReleaseContext(CryptProv, 0);
		return final;
	};

	bool is_nit = true;
#endif

	curtime = QPCTime();
	double realdelta = curtime - lasttime;
	float dt = (float)fmin(realdelta, 0.5);
	Time += dt;
	lasttime = curtime;

#ifndef INTERNAL_DEBUG
#ifndef _DEBUG
#ifdef STAFF
	if (!is_nit && realdelta > 10.0f)
		exit(EXIT_SUCCESS);
#else
	//if (realdelta > 30.0f)
	//	exit(EXIT_SUCCESS);
#endif
#endif
#endif
	//DeltaTime = dt;

	//TODO: Clean up this code below
	if (!FoundCSGO)
	{
#if defined _DEBUG || defined CONSOLE
		logger::add(LSUCCESS, "reached cheat init");
#endif

		static float starttime = 0.0f;
		if (starttime == 0.0f)
			starttime = Time;
		if (Time - starttime < 120.0f)
		{
			if (!ClientHandle)
			{
				DecStr(clientdllstr, 10);
				ClientHandle = FindHandle(clientdllstr);
				EncStr(clientdllstr, 10);
			}
			if (!MatchmakingHandle)
			{
				DecStr(matchmakingdllstr, 15);
				MatchmakingHandle = FindHandle(matchmakingdllstr);
				EncStr(matchmakingdllstr, 15);
			}
			if (!VPhysicsHandle)
			{
				DecStr(vphysicsdllstr, 12);
				VPhysicsHandle = FindHandle(vphysicsdllstr);
				EncStr(vphysicsdllstr, 12);
			}
			if (!VSTDLIBHandle)
			{
				DecStr(vstdlibdllstr, 11);
				VSTDLIBHandle = FindHandle(vstdlibdllstr);
				EncStr(vstdlibdllstr, 11);
			}
			if (!SHADERAPIDX9Handle)
			{
				DecStr(shaderapistr, 16);
				SHADERAPIDX9Handle = FindHandle(shaderapistr);
				EncStr(shaderapistr, 16);
			}
			if (!DatacacheHandle)
			{
				DecStr(datacachestr, 13);
				DatacacheHandle = FindHandle(datacachestr);
				EncStr(datacachestr, 13);
			}
			if (!Tier0Handle)
			{
				DecStr(tier0str, 9);
				Tier0Handle = FindHandle(tier0str);
				EncStr(tier0str, 9);
			}
			if (!MaterialSystemHandle)
			{
				DecStr(materialsystemstr, 18);
				MaterialSystemHandle = FindHandle(materialsystemstr);
				EncStr(materialsystemstr, 18);
			}
			if (!StudioRenderHandle)
			{
				//decrypts(0)
				StudioRenderHandle = FindHandle(XorStr("studiorender.dll"));
				//encrypts(0)
			}
			if (!FileSystemStdioHandle)
			{
				//decrypts(0)
				FileSystemStdioHandle = FindHandle(XorStr("filesystem_stdio.dll"));
				//encrypts(0)
			}
			if (!ServerHandle)
			{
				//decrypts(0)
				ServerHandle = FindHandle(XorStr("server.dll"));
				//encrypts(0)
			}


			if (!ClientHandle || !MatchmakingHandle || !VPhysicsHandle || !VSTDLIBHandle || !SHADERAPIDX9Handle || !DatacacheHandle || !Tier0Handle || !MaterialSystemHandle || !StudioRenderHandle || !FileSystemStdioHandle)
			{
#if defined _DEBUG || defined CONSOLE
				logger::add(LERROR, "a handle couldnt be found, later kid");
#endif

				return;
			}
			else
			{
#if defined _DEBUG || defined CONSOLE
				logger::add(LSUCCESS, "found all handles");
#endif

				for (auto& p : m_PlayerRecords)
				{
					p.Hooks.OnDLLInit();
				}

#if defined _DEBUG || defined CONSOLE
				logger::add(LSUCCESS, "hooked all the bois");
#endif

				SetupVMTHooks();

#if defined _DEBUG || defined CONSOLE
				logger::add(LSUCCESS, "hooks setup");
#endif


				delete[]clientdllstr;
				delete[]enginedllstr;
				delete[]matchmakingdllstr;
				delete[]vphysicsdllstr;
				delete[]vstdlibdllstr;
				delete[]shaderapistr;
				delete[]datacachestr;
				delete[]materialsystemstr;
				FoundCSGO = true;

#if defined _DEBUG || defined CONSOLE
				logger::add(LSUCCESS, "should be gucci");
#endif
			}
		}
		else
		{
#if defined _DEBUG || defined CONSOLE
		logger::add(LERROR, "not gucci uwu");
#endif
			//Failed to find engine.dll or matchmaking.dll in a reasonable amount of time, abort and assume it's some other game!
			*InvalidGame = -1;
			FoundCSGO = true;
			if (ThisDLLHandle)
			{
				delete[]clientdllstr;
				delete[]enginedllstr;
				delete[]matchmakingdllstr;
				delete[]vphysicsdllstr;
				delete[]vstdlibdllstr;
				delete[]shaderapistr;
				delete[]datacachestr;
				delete[]materialsystemstr;
				FreeLibrary(ThisDLLHandle);
			}
		}
	}
	else
	{
		GotCSGORectMutex.lock();
		if (GotCSGORect)
		{
			GotCSGORectMutex.unlock();
			//Totally done initializing.. Do the actual Think now

			//We are converting to ULONG so compiler optimizes the division by bit shifting
			GetWindowRect(tWnd, &rc);
			CenterOfScreen = { (float)((ULONG)rc.right / 2), (float)((ULONG)rc.bottom / 2), 0.0f };
		}
		else
		{
			GotCSGORectMutex.unlock();
			GetCSGORect();
		}
	}
	VMP_END
}

struct s
{
	_In_ DWORD  m_pid = 0;
	_Out_ HWND  m_hwnd = 0;
};

BOOL __stdcall GetHWND (HWND hwnd, LPARAM lParam)
{
	//NOTE: BREAKS WITH DEBUG CONSOLE IN RELEASE MODE
	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid == ((s*)lParam)->m_pid)
	{
#ifdef _DEBUG
		//char windowbuffer[MAX_PATH];
		//size_t size = GetWindowTextA(hwnd, windowbuffer, MAX_PATH);
		if (GetConsoleWindow() && hwnd == GetConsoleWindow())   // It always found the DebugConsole before the normal window...
			return true;
		//printf("EnumWindows: %s\n", windowbuffer);
#endif

		// The window we hook "should" have a title, have you ever seen a game window without a title?
		// this fixes broken window procedure hooks with discord overlay enabled for example
		if (!GetWindowTextLengthA(hwnd))
			return true;

		char windowbuffer[MAX_PATH];
		GetWindowTextA(hwnd, windowbuffer, MAX_PATH);

		//The latest windows 10 updates have their own window that overlays games..
		if (windowbuffer[0] == 'D' && windowbuffer[1] == '3' && windowbuffer[2] == 'D')
		{
			memset(windowbuffer, 0x00, sizeof windowbuffer);
			return true;
		}
		memset(windowbuffer, 0x00, sizeof windowbuffer);

		((s*)lParam)->m_hwnd = hwnd;
		return false;
	}
	return true;
};

struct GetRectArgs
{
	int windowtext1len;
	char *windowtext1;
	int windowtext2len;
	char *windowtext2;
};

std::mutex rectmutex;
std::mutex twndmutex;

//#ifdef GPU_INJECT
//void _stdcall ThreadedGetRect(GetRectArgs* args)
//#else
void _stdcall GetRect(GetRectArgs* args)
//#endif
{
	s input;
	input.m_pid = GetCurrentProcessId();
	EnumWindows((WNDENUMPROC)GetHWND, (LPARAM)&input);

	if (input.m_hwnd)
	{
		char windowbuffer[MAX_PATH];
		size_t size = GetWindowTextA(input.m_hwnd, windowbuffer, MAX_PATH);
		DecStr(args->windowtext1, args->windowtext1len);
		DecStr(args->windowtext2, args->windowtext2len);

		if (!size || (strcmp(args->windowtext1, windowbuffer) && strcmp(args->windowtext2, windowbuffer)))
		{
			EncStr(args->windowtext1, args->windowtext1len);
			EncStr(args->windowtext2, args->windowtext2len);
			delete args->windowtext1;
			delete args->windowtext2;
			delete args;

			//THROW_ERROR(ERR_CANT_FIND_CSGO_WINDOW_TEXT);
			//exit(EXIT_SUCCESS);
			return;
		}

		EncStr(args->windowtext1, args->windowtext1len);
		EncStr(args->windowtext2, args->windowtext2len);

		rectmutex.lock();

		GetWindowRect(input.m_hwnd, &rc);

		if (rc.bottom >= 480 && rc.right >= 640)
		{
			//Initialize menu overlay
				//Found game! Set the target window for IMGUI
				twndmutex.lock();
				tWnd = input.m_hwnd;
				twndmutex.unlock();

			GotCSGORectMutex.lock();
			GotCSGORect = true;
			GotCSGORectMutex.unlock();
		}

		rectmutex.unlock();
	}
	delete args->windowtext1;
	delete args->windowtext2;
	delete args;
}

bool GotCSGORect = false;
std::mutex GotCSGORectMutex;
void GetCSGORect()
{
	static double lastrun = QPCTime();
	double tick = QPCTime();
	if (tick - lastrun > 2.0f)
	{
		lastrun = tick;
		unsigned int threadid = 0;
		GetRectArgs *args = new GetRectArgs;
		if (args)
		{
			args->windowtext1len = 32;
			args->windowtext1 = new char[33]{ 57, 21, 15, 20, 14, 31, 8, 87, 41, 14, 8, 19, 17, 31, 64, 90, 61, 22, 21, 24, 27, 22, 90, 53, 28, 28, 31, 20, 9, 19, 12, 31, 0 };
			args->windowtext2len = 33;
			args->windowtext2 = new char[34]{ 57, 21, 15, 20, 14, 31, 8, 87, 41, 14, 8, 19, 17, 31, 64, 90, 57, 22, 27, 9, 9, 19, 25, 90, 53, 28, 28, 31, 20, 9, 19, 12, 31, 0 }; /*Counter-Strike: Classic Offensive*/

//#ifdef GPU_INJECT
//			//Crashes with truix's framework security
//			HANDLE thr = (HANDLE)_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)&ThreadedGetRect, args, 0, &threadid);
//			if (thr != 0)
//				CloseHandle(thr);
//#else
			GetRect(args);
//#endif
		}
	}
}

unsigned char TestByte = 15;
int TestInt = 25;
float TestFloat = 1250.0f;
double TestDouble = 9.37501;
short TestShort = 575;

//Driver DLL will call this function to tell us the read and write functions, and we will tell it the think function to call in this DLL
#ifdef MUTINY
void* NVInit(HMODULE handletothisdll, int* invalidadr, void* aReadWord, void* aWriteWord, void* aWriteDouble, void* aReadDouble, void* aWriteFloat, void* aReadFloat, void* aWriteInt, void* aReadInt, void* aWriteByte, void* aReadByte)
#else
#ifdef ATI
extern "C" DLLEXP void* ATIInit(HMODULE handletothisdll, int* invalidadr, void* aReadWord, void* aWriteWord, void* aWriteDouble, void* aReadDouble, void* aWriteFloat, void* aReadFloat, void* aWriteInt, void* aReadInt, void* aWriteByte, void* aReadByte)
#else
extern "C" DLLEXP void* NVInit(HMODULE handletothisdll, int* invalidadr, void* aReadWord, void* aWriteWord, void* aWriteDouble, void* aReadDouble, void* aWriteFloat, void* aReadFloat, void* aWriteInt, void* aReadInt, void* aWriteByte, void* aReadByte)
#endif
#endif
{
	ThisDLLHandle = handletothisdll;
	InvalidGame = invalidadr;
	ReadByte = (unsigned char(__cdecl*)(uintptr_t))aReadByte;
	ReadInt = (int(__cdecl*)(uintptr_t))aReadInt;
	ReadFloat = (float(__cdecl*)(uintptr_t))aReadFloat;
	ReadDouble = (double(__cdecl*)(uintptr_t))aReadDouble;
	WriteByte = (void(__cdecl*)(uintptr_t, unsigned char))aWriteByte;
	WriteInt = (void(__cdecl*)(uintptr_t, int))aWriteInt;
	WriteFloat = (void(__cdecl*)(uintptr_t, float))aWriteFloat;
	WriteDouble = (void(__cdecl*)(uintptr_t, double))aWriteDouble;
	WriteShort = (void(__cdecl*)(uintptr_t, short))aWriteWord;
	ReadShort = (short(__cdecl*)(uintptr_t))aReadWord;
#ifndef MUTINY
#ifndef ATI
	if (ReadByte((uintptr_t)&TestByte) + 2 != TestByte + 2)
		return nullptr;
	if (ReadInt((uintptr_t)&TestInt) + 5 != TestInt + 5)
		return nullptr;
	if (ReadFloat((uintptr_t)&TestFloat) - 135.0f != TestFloat - 135.0f)
		return nullptr;
	if (!ReadDouble((uintptr_t)&TestDouble))
		return nullptr;
	if (ReadShort((uintptr_t)&TestShort) - 20 != TestShort - 20)
		return nullptr;
#else
	bool Failed = false;
	if (ReadByte((uintptr_t)&TestByte) + 2 != TestByte + 2)
		Failed = true;
	if (!Failed && ReadInt((uintptr_t)&TestInt) + 5 != TestInt + 5)
		Failed = true;
	if (!Failed && ReadFloat((uintptr_t)&TestFloat) - 135.0f != TestFloat - 135.0f)
		Failed = true;
	if (!Failed && !ReadDouble((uintptr_t)&TestDouble))
		Failed = true;
	if (!Failed && ReadShort((uintptr_t)&TestShort) - 20 != TestShort - 20)
		Failed = true;

	if (Failed)
		return nullptr;
#endif
#endif
	DecStr(enginedllstr, 10);
	EngineHandle = FindHandle(enginedllstr); //engine.dll
	EncStr(enginedllstr, 10);

	if (EngineHandle)
	{
		//p_EngineTrace = InterfaceManager::Manager::getIface<EngineTrace>((DWORD)EngineHandle, EngineTraceClient/*"EngineTraceClient"*/);
		//DWORD TraceRateFunction = (DWORD)EngineHandle + TRACE_RAY_OFFSET;
		//TraceRay = (void(*)(Ray_t const&, unsigned int, ITraceFilter *, CGameTrace *))TraceRateFunction;
		InitTime();
		curtime = lasttime = QPCTime();
		Time = 0;
		//Initialize all static arrays
		{
#ifdef ATI
			return &ATIThink;
#else
			return &NVThink;
#endif
		}
	}
	return nullptr;
}

namespace ReadWriteeWrappers
{
	char __cdecl ReadByte(uintptr_t adr) { return *(char*)(adr); }
	void __cdecl WriteByte(uintptr_t adr, char val) { *(char*)(adr) = val; }
	short __cdecl ReadShort(uintptr_t adr) { return *(short*)(adr); }
	void __cdecl WriteShort(uintptr_t adr, short val) { *(short*)(adr) = val; }
	int __cdecl ReadInt(uintptr_t adr) { return *(int*)(adr); }
	void __cdecl WriteInt(uintptr_t adr, int val) { *(int*)(adr) = val; }
	float __cdecl ReadFloat(uintptr_t adr) { return *(float*)(adr); }
	void __cdecl WriteFloat(uintptr_t adr, float val) { *(float*)(adr) = val; }
	double __cdecl ReadDouble(uintptr_t adr) { return *(double*)(adr); }
	void __cdecl WriteDouble(uintptr_t adr, double val) { *(double*)(adr) = val; }
}

extern bool FoundCSGO;

unsigned int __stdcall ThreadLoop(void* NVThink_adr)
{
	/*  WHO EVER TOUCHES THIS, DIES    */
	/*  PHYZ WROTE THAT                */
	/*  I AM FROM FAVELA, PEW PEW PEW  */
	
	auto& cfg = config::get();

	cfg.initialize();

#if defined _DEBUG || defined CONSOLE
	logger::add(LSUCCESS, "config setup");
#endif

	auto& inp = input::get();
	inp.initialize();

#if defined _DEBUG || defined CONSOLE
	logger::add(LSUCCESS, "input setup");
#endif

	inp.register_hotkey(VK_INSERT, []()
	{
		if (!variable::get().ui.b_unload)
			ui::get().toggle();
	});
	
	/*  -----------------------------  */

	for (;;)
	{
		if (g_bIsExiting)
			break;

		((void(*)())NVThink_adr)();

		if (FoundCSGO)
			RestoreVACCedPointers();

#ifdef DYLAN_VAC
		Sleep(100);
#else
		Sleep(10);
#endif

		/*  WHO EVER TOUCHES THIS, DIES    */
		/*  PHYZ WROTE THAT                */
		/*  I AM FROM FAVELA, PEW PEW PEW  */

#ifndef IMI_MENU
		cfg.refresh();
		inp.reset_key_map();
#endif

		/*  -----------------------------  */
	}

	// close the heartbeat thread made with __beginthreadex
#ifdef MUTINY_FRAMEWORK
	if(hHBThread)
		CloseHandle(hHBThread);
#endif
	return 0;
}

#ifdef MUTINY_FRAMEWORK
void HBThread()
{
	pHeartbeat->Heartbeat_wrapper(nullptr);
}
#endif

DWORD WINAPI CheatInit(const LPVOID lParam)
{
	VMP_BEGINMUTILATION("cheatinit");
	void* thinkfunc = NVInit(static_cast<HMODULE>(lParam), nullptr,
		(void*)&ReadWriteeWrappers::ReadShort, (void*)&ReadWriteeWrappers::WriteShort,
		(void*)&ReadWriteeWrappers::WriteDouble, (void*)&ReadWriteeWrappers::ReadDouble,
		(void*)&ReadWriteeWrappers::WriteFloat, (void*)&ReadWriteeWrappers::ReadFloat,
		(void*)&ReadWriteeWrappers::WriteInt, (void*)&ReadWriteeWrappers::ReadInt,
		(void*)&ReadWriteeWrappers::WriteByte, (void*)&ReadWriteeWrappers::ReadByte);

	if (thinkfunc)
	{

#if defined STAFF
		auto get_md5 = [](std::string input) -> std::string
		{
				HCRYPTPROV CryptProv;
				HCRYPTHASH CryptHash;
				BYTE BytesHash[33];
				DWORD dwHashLen;
				std::string final;	
				
				if (CryptAcquireContext(&CryptProv,
					NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET))
				{
					if (CryptCreateHash(CryptProv, CALG_MD5, 0, 0, &CryptHash))
					{
						if (CryptHashData(CryptHash, (BYTE*)input.c_str(), input.length(), 0))
						{
							if (CryptGetHashParam(CryptHash, HP_HASHVAL, BytesHash, &dwHashLen, 0))
							{
								final.clear();
								//decrypts(0)
								std::string hexcharset = XorStr("0123456789ABCDEF");
								//encrypts(0)
								for (int j = 0; j < 16; j++)
								{
									final += hexcharset.substr(((BytesHash[j] >> 4) & 0xF), 1);
									final += hexcharset.substr(((BytesHash[j]) & 0x0F), 1);
								}
							}
						}
					}
				}	
				CryptDestroyHash(CryptHash);
				CryptReleaseContext(CryptProv, 0);
				return final;
		};

		bool is_nit = false;
#endif

		// todo: nit; look into why the 'driverscan' is crashing people including shark.
#ifdef MUTINY_FRAMEWORK
		//decrypts(0)
		pHeartbeat = new MutinyFrame::CHeartBeat(XorStr("nSUlYT99JDbT0kzpUAHP0Mkvl"), XorStr("IxARW9cCazjpGF9m3KLd2HLH8"), XorStr("GameCSGO"), MutinyFrame::HEARTBEAT_DRIVERSCAN);
		//encrypts(0)

#if defined _DEBUG || defined CONSOLE
		logger::add(LSUCCESS, "sec made");
#endif

#ifdef MUTINY_FRAMEWORK
		hHBThread = (HANDLE)_BEGINTHREAD(HBThread)

#if defined _DEBUG || defined CONSOLE
			logger::add(LSUCCESS, "sec started");
#endif

#if defined STAFF
		if (!is_nit)
		{
			moduleSecurity.Protect(static_cast<HMODULE>(lParam), true, true, false, true);
		}
		g_Security.setup(pHeartbeat);
#else
#ifndef DUMP_SIGS_FOR_BLACKBOOK
		moduleSecurity.Protect(static_cast<HMODULE>(lParam), true, true, false, true);
#endif
		g_Security.setup(pHeartbeat);
#endif


#if defined _DEBUG || defined CONSOLE
		logger::add(LSUCCESS, "sec setup");
#endif

#endif
#endif
		unsigned int loop_thread_id;
		HANDLE handle = (HANDLE)_beginthreadex(nullptr, 0, &ThreadLoop, thinkfunc /*hInstance*/, 0, &loop_thread_id);

		if (handle != 0)
			CloseHandle(handle);

#if defined _DEBUG || defined CONSOLE
		logger::add(LSUCCESS, "cheat thread made");
#endif
	}
	return 1;
	VMP_END
}
