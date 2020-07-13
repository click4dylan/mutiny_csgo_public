#include "precompiled.h"
#include "GameMemory.h"
#include "Overlay.h"
#ifdef VISTA
#define PSAPI_VERSION 1
#endif
#include <Psapi.h>
#include "CSGO_HX.h"
#include "LocalPlayer.h"
#include "Interfaces.h"
#include <vector>
#include "CPlayerrecord.h"

__declspec (naked) int __cdecl HashString2(char * guidstr)
{
	__asm {
		//Mov [Esp-8], Edi
		//Mov Edi, [Esp+4]
		//Mov Ecx, -1
		//Mov Al, 0
		//Repne Scasb
		//Neg Ecx
		//Sub Ecx, 6
		Mov[Esp - 4], EBX
		Mov[Esp - 8], ECX
		Mov Ebx, [Esp + 4]
		Mov Ecx, 32
		Mov Eax, 0
		HashLoop:
		Xor Eax, [Ebx + Ecx]
			Sub Ecx, 4
			Jns HashLoop
			Mov Ebx, [Esp - 4]
			Mov Ecx, [Esp - 8]
			Retn //4
	}
}

uint32_t HashString_MUTINY(const char * s)
{
	uint32_t hash = 0;

	for (; *s; ++s)
	{
		hash += *s;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

int ActualNumPlayers = 0;

unsigned char(__cdecl *ReadByte) (uintptr_t);
int(__cdecl *ReadInt) (uintptr_t);
float(__cdecl *ReadFloat) (uintptr_t);
double(__cdecl *ReadDouble) (uintptr_t);
short(__cdecl *ReadShort) (uintptr_t);
void(__cdecl *WriteByte) (uintptr_t, unsigned char);
void(__cdecl *WriteInt) (uintptr_t, int);
void(__cdecl *WriteFloat) (uintptr_t, float);
void(__cdecl *WriteDouble) (uintptr_t, double);
void(__cdecl *WriteShort) (uintptr_t, short);

HANDLE EngineHandle = NULL;
HANDLE ClientHandle = NULL;
HMODULE ThisDLLHandle = NULL;
HANDLE MatchmakingHandle = NULL;
HANDLE VPhysicsHandle = NULL;
HANDLE VSTDLIBHandle = NULL;
HANDLE SHADERAPIDX9Handle = NULL;
HANDLE DatacacheHandle = NULL;
HANDLE Tier0Handle = NULL;
HANDLE MaterialSystemHandle = NULL;
HANDLE VGUIMatSurfaceHandle = NULL;
HANDLE VGUI2Handle = NULL;
HANDLE StudioRenderHandle = NULL;
HANDLE FileSystemStdioHandle = NULL;
HANDLE ServerHandle = NULL;
RECT rc; //screen rectangle
HWND tWnd = NULL; //target game window
HWND hWnd = NULL; //hack window

void ClearAllPlayers()
{
	LocalPlayer.NetvarMutex.Lock();

	for (int i = 1; i <= 64; ++i)
	{
		m_PlayerRecords[i].Reset(true);
	}
	/*
	ClearHighlightedPlayer();
	for (int i = 1; i <= 64; i++)
	{
		CPlayerrecord *_playerRecord = &m_PlayerRecords[i];
		if (_playerRecord->m_pEntity)
			_playerRecord->Reset(true);
	}
	*/
	LocalPlayer.NetvarMutex.Unlock();
}

float GetGlobalTickInterval()
{
	return Interfaces::Globals->interval_per_tick;
}

int GetGlobalTickCount()
{
	return Interfaces::Globals->tickcount;
}

float GetGlobalCurTime()
{
	return Interfaces::Globals->curtime;
}

float GetGlobalRealTime()
{
	return Interfaces::Globals->realtime;
}

int GetExactTick(float SimulationTime)
{
	return TIME_TO_TICKS(SimulationTime + g_LagCompensation.GetLerpTime());
	//Below was found to work amazingly well for shot backtracking..
	//return TIME_TO_TICKS((SimulationTime - Interfaces::Globals->interval_per_tick) + gLagCompensation.GetLerpTime()) + 1;
}

int GetExactServerTick()
{
	return g_ClientState->m_ClockDriftMgr.m_nServerTick + TIME_TO_TICKS(g_LagCompensation.GetLerpTime());
}

int GetServerTick()
{
	return g_ClientState->m_ClockDriftMgr.m_nServerTick;
}

float GetServerTime()
{
	return TICKS_TO_TIME(g_ClientState->m_ClockDriftMgr.m_nServerTick);
}

#if 0
char *silver1str = new char[9]{ 41, 19, 22, 12, 31, 8, 90, 51, 0 }; /*Silver I*/
char *silver2str = new char[10]{ 41, 19, 22, 12, 31, 8, 90, 51, 51, 0 }; /*Silver II*/
char *silver3str = new char[11]{ 41, 19, 22, 12, 31, 8, 90, 51, 51, 51, 0 }; /*Silver III*/
char *silver4str = new char[10]{ 41, 19, 22, 12, 31, 8, 90, 51, 44, 0 }; /*Silver IV*/
char *silver5str = new char[13]{ 41, 19, 22, 12, 31, 8, 90, 63, 22, 19, 14, 31, 0 }; /*Silver Elite*/
char *silver6str = new char[20]{ 41, 19, 22, 12, 31, 8, 90, 63, 22, 19, 14, 31, 90, 55, 27, 9, 14, 31, 8, 0 }; /*Silver Elite Master*/
char *nova1str = new char[12]{ 61, 21, 22, 30, 90, 52, 21, 12, 27, 90, 51, 0 }; /*Gold Nova I*/
char *nova2str = new char[13]{ 61, 21, 22, 30, 90, 52, 21, 12, 27, 90, 51, 51, 0 }; /*Gold Nova II*/
char *nova3str = new char[14]{ 61, 21, 22, 30, 90, 52, 21, 12, 27, 90, 51, 51, 51, 0 }; /*Gold Nova III*/
char *nova4str = new char[17]{ 61, 21, 22, 30, 90, 52, 21, 12, 27, 90, 55, 27, 9, 14, 31, 8, 0 }; /*Gold Nova Master*/
char *mg1str = new char[18]{ 55, 27, 9, 14, 31, 8, 90, 61, 15, 27, 8, 30, 19, 27, 20, 90, 51, 0 }; /*Master Guardian I*/
char *mg2str = new char[19]{ 55, 27, 9, 14, 31, 8, 90, 61, 15, 27, 8, 30, 19, 27, 20, 90, 51, 51, 0 }; /*Master Guardian II*/
char *mg3str = new char[22]{ 55, 27, 9, 14, 31, 8, 90, 61, 15, 27, 8, 30, 19, 27, 20, 90, 63, 22, 19, 14, 31, 0 }; /*Master Guardian Elite*/
char *mg4str = new char[30]{ 62, 19, 9, 14, 19, 20, 29, 15, 19, 9, 18, 31, 30, 90, 55, 27, 9, 14, 31, 8, 90, 61, 15, 27, 8, 30, 19, 27, 20, 0 }; /*Distinguished Master Guardian*/
char *lestr = new char[16]{ 54, 31, 29, 31, 20, 30, 27, 8, 3, 90, 63, 27, 29, 22, 31, 0 }; /*Legendary Eagle*/
char *lemstr = new char[23]{ 54, 31, 29, 31, 20, 30, 27, 8, 3, 90, 63, 27, 29, 22, 31, 90, 55, 27, 9, 14, 31, 8, 0 }; /*Legendary Eagle Master*/
char *supremestr = new char[27]{ 41, 15, 10, 8, 31, 23, 31, 90, 55, 27, 9, 14, 31, 8, 90, 60, 19, 8, 9, 14, 90, 57, 22, 27, 9, 9, 0 }; /*Supreme Master First Class*/
char *globalstr = new char[17]{ 46, 18, 31, 90, 61, 22, 21, 24, 27, 22, 90, 63, 22, 19, 14, 31, 0 }; /*The Global Elite*/
#endif

//credits for bits: https://github.com/A5-/Gamerfood_CSGO/blob/38c08cde08d9ac60fb1a84ec10facc74d711bbc9/csgo_internal/csgo_internal/Util.h
#define INRANGE(x,a,b) (x >= a && x <= b) 
#define getBits( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define getByte( x ) (getBits(x[0]) << 4 | getBits(x[1]))

void GetModuleStartEndPoints(HANDLE ModuleHandle, uintptr_t& start, uintptr_t& end) {
	MODULEINFO dllinfo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)ModuleHandle, &dllinfo, sizeof(MODULEINFO));
	start = (uintptr_t)ModuleHandle;
	end = (uintptr_t)ModuleHandle + dllinfo.SizeOfImage;
}

uintptr_t FindMemoryPattern(uintptr_t start, uintptr_t end, std::string strpattern, bool double_wide)
{
	return FindMemoryPattern(start, end, strpattern.c_str(), strpattern.length(), double_wide);
}

uintptr_t FindMemoryPattern(HANDLE ModuleHandle, std::string strpattern, bool double_wide)
{
	uintptr_t start, end;
	GetModuleStartEndPoints(ModuleHandle, start, end);
	return FindMemoryPattern(start, end, strpattern.c_str(), strpattern.length(), double_wide);
}

uintptr_t FindMemoryPattern(HANDLE ModuleHandle, char* strpattern, int length, bool double_wide)
{
	uintptr_t start, end;
	GetModuleStartEndPoints(ModuleHandle, start, end);
	return FindMemoryPattern(start, end, strpattern, (size_t)length, double_wide);
}

uintptr_t FindMemoryPattern(HANDLE ModuleHandle, char* strpattern, size_t length, bool double_wide)
{
	uintptr_t start, end;
	GetModuleStartEndPoints(ModuleHandle, start, end);
	return FindMemoryPattern(start, end, strpattern, length, double_wide);
}

uintptr_t FindMemoryPattern_(HANDLE ModuleHandle, char* strpattern, bool double_wide)
{
	uintptr_t start, end;
	GetModuleStartEndPoints(ModuleHandle, start, end);
	return FindMemoryPattern(start, end, strpattern, strlen(strpattern), double_wide);
}

uintptr_t FindMemoryPattern(uintptr_t start, uintptr_t end, const char *strpattern, size_t length, bool double_wide) {
	uintptr_t	   adrafterfirstmatch = 0;
	size_t		   indextofind = 0;
	size_t		   numhexvalues = 0;
	std::unique_ptr<unsigned char[]>  hexvalues(new unsigned char[length + 1]);
	std::unique_ptr<bool[]>			  shouldskip(new bool[length + 1]);

	if (double_wide) { //DOUBLE SPACES AND QUESTION MARKS, THIS IS FASTER TO RUN
		for (size_t i = 0; i < length - 1; i += 2) {
			//Get the ascii version of the hex values out of the pattern
			char ascii[4];
			*(short*)ascii = *(short*)&strpattern[i];

			//Filter out spaces
			if (ascii[0] != ' ') {
				//Filter out wildcards
				if (ascii[0] == '?') {
					shouldskip[numhexvalues] = true;
				}
				else {
					//Convert ascii to hex
					ascii[2] = NULL; //add null terminator
					hexvalues[numhexvalues] = (unsigned char)std::stoul(ascii, nullptr, 16);
					shouldskip[numhexvalues] = false;
				}
				numhexvalues++;
			}
		}
	} 
	else {
		for (size_t i = 0, maxlength = length - 1; i < maxlength; i++) {
			//Get the ascii version of the hex values out of the pattern
			char ascii[4];
			*(short*)ascii = *(short*)&strpattern[i];

			//Filter out spaces
			if (ascii[0] != ' ') {
				//Filter out wildcards
				if (ascii[0] == '?') {
					shouldskip[numhexvalues] = true;
				}
				else {
					//Convert ascii to hex
					ascii[2] = NULL; //add null terminator
					hexvalues[numhexvalues] = (unsigned char)std::stoul(ascii, nullptr, 16);
					shouldskip[numhexvalues] = false;
				}
				i++;
				numhexvalues++;
			}
		}
	}

	//Search for the hex signature in memory	
	for (uintptr_t adr = start; adr < end; adr++)
	{
		if (shouldskip[indextofind] || *(char*)adr == hexvalues[indextofind] || *(unsigned char*)adr == hexvalues[indextofind]) {
			if (indextofind++ == 0)
				adrafterfirstmatch = adr + 1;

			if (indextofind >= numhexvalues)
				return adr - (numhexvalues - 1); //FOUND PATTERN!

		}
		else if (adrafterfirstmatch) {
			adr = adrafterfirstmatch;
			indextofind = 0;
			adrafterfirstmatch = 0;
		}
	}
	return NULL; //NOT FOUND!
}

//Todo: replace with evolve version
size_t GetModuleSize(HANDLE ModuleHandle)
{
	MODULEINFO dllinfo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)ModuleHandle, &dllinfo, sizeof(MODULEINFO));
	return dllinfo.SizeOfImage;
}

uint32_t FindPattern(HANDLE Module, std::string Pattern, uint32_t StartAddress, bool UseStartAddressAsStartPoint, bool SubtractBaseAddressFromStart, bool Dereference, uint32_t AddBeforeDereference, int DereferenceTimes, uint32_t *rawadr, int sizeofvalue)
{
	auto Start = ((uint32_t)Module + StartAddress);

	if (UseStartAddressAsStartPoint)
		Start = StartAddress;

	if (SubtractBaseAddressFromStart)
		Start -= (uint32_t)Module;

	struct sSignature {
		sSignature() {
			Index = 0;
			Byte = 0;
			IsWildcard = false;
		}

		sSignature(int _Index, uint8_t _Byte, bool _IsWildcard) {
			Index = _Index;
			Byte = _Byte;
			IsWildcard = _IsWildcard;
		}

		int Index;
		uint8_t Byte;
		bool IsWildcard;
	};

	std::vector<sSignature> vecSig;
	int TotalBytes = 0;

	auto _patternLength = Pattern.length();

	for (size_t i = 0; i < _patternLength; i++)
	{
		if (Pattern.data()[i] == ' ')
			continue;

		if (Pattern.data()[i] != '?')
			vecSig.emplace_back(sSignature(TotalBytes, (uint8_t)std::strtoul(Pattern.data() + i, nullptr, 16), false));
		else {
			if (i + 1 < _patternLength && Pattern.data()[i + 1] == '?')
				++i; //Fix patterns with double wide wildcards

			vecSig.emplace_back(sSignature(TotalBytes, 0, true));
		}

		TotalBytes++;
		i++;
	}

	auto CompareBytes = [](uint32_t _Address, std::vector<sSignature> &_vecSig)
	{
		auto _sigSize = _vecSig.size();
		for (size_t i = 0; i < _sigSize; ++i)
		{
			auto it = _vecSig.at(i);

			if (it.IsWildcard)
				continue;

			if (it.Byte != *reinterpret_cast<uint8_t*>(_Address + it.Index))
				return false;
		}

		return true;
	};

	size_t ModuleSize = GetModuleSize(Module);

	for (uint32_t idx = 0; idx < ModuleSize; ++idx)
	{
		auto retn = Start + idx;

		if (CompareBytes(retn, vecSig))
		{
			//decrypts(0)
			DEBUGPRINT(XorStr("Found Signature: 0x%x."), retn);
			//encrypts(0)

			if (rawadr)
				*rawadr = retn;

			if (Dereference)
			{
				retn += AddBeforeDereference;

				for (int i = 0; i < DereferenceTimes; ++i)
				{
					switch (sizeofvalue)
					{
					case 4:
						retn = *reinterpret_cast<uint32_t*>(retn);
						break;
					case 2:
						retn = *reinterpret_cast<uint16_t*>(retn);
						break;
					case 1:
						retn = *reinterpret_cast<uint8_t*>(retn);
						break;
					default:
						//not supported! retn is 32 bit
						DebugBreak();
						break;
					}
				}
			}

			return retn;
		}
	}

	//decrypts(0)
	DEBUGPRINT(XorStr("WARNING! Signature not found."));
	//encrypts(0)

	if (rawadr)
		*rawadr = 0;

	return 0;
}


#if 1
#include "Interfaces.h"
//char *viewmatrixstr = new char[47]{ 59, 75, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 73, 90, 90, 60, 66, 90, 90, 74, 75, 90, 90, 77, 63, 90, 90, 75, 75, 90, 90, 76, 67, 90, 90, 57, 66, 0 }; /*A1  ??  ??  ??  ??  83  F8  01  7E  11  69  C8*/
char *viewmatrixstr = new char[51]{ 56, 67, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 59, 75, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 60, 60, 90, 90, 76, 74, 90, 90, 73, 66, 0 }; /*B9  ??  ??  ??  ??  A1  ??  ??  ??  ??  FF  60  38*/
DWORD ViewMatrixPtr = 0;
BOOL WorldToScreenCapped(Vector& from, Vector& to)
{
	if (!ViewMatrixPtr)
	{
		DecStr(viewmatrixstr, 50);
		DWORD ViewMatrix = FindMemoryPattern(EngineHandle, viewmatrixstr, 50);
		EncStr(viewmatrixstr, 50);
		if (!ViewMatrix)
		{
			return false;
			//THROW_ERROR(ERR_CANT_FIND_VIEWMATRIX_SIGNATURE);
			//exit(EXIT_SUCCESS);
		}
		ViewMatrixPtr = *(DWORD*)(ViewMatrix + 0x1); //0x11
	}
	//const VMatrix& WorldToScreenMatrix = (const VMatrix&) ((*(PDWORD)(*(PDWORD)ViewMatrixPtr + 0x11)) + 0x3DC);//Interfaces::Engine->WorldToScreenMatrix();
	
	//DWORD dwVMatrix = *(DWORD*)ViewMatrixPtr;
	DWORD dwVMatrix = 2 * 528; //*(DWORD*)(ViewMatrixPtr + 0x0E8) * 528;// 2 * 528;
	DWORD dwVMatrix2 = *(DWORD*)(ViewMatrixPtr + 0x0DC) - 68;
	//DWORD dwResult = dwVMatrix + 2 * 528 - 68;
	DWORD dwResult = dwVMatrix + dwVMatrix2;

#if 0
	__asm {
		mov ecx, ViewMatrixPtr;
		imul edx, dword ptr ds : [ecx + 0x0e8], 0x210;
		mov eax, dword ptr ds : [ecx + 0x0dc];
		sub eax, 68
		add eax, edx;
		mov dwResult, eax
	}
#endif

	const VMatrix& WorldToScreenMatrix = (VMatrix&)(*(DWORD_PTR*)dwResult);

	for (int i = 0; i < 4; ++i)
	{
		for (int f = 0; f < 4; ++f)
		{
			if (!std::isfinite(WorldToScreenMatrix.m[i][f]))
			{
				return false;
			}
		}
	}


	to.z = 0.0f;
	to.y = WorldToScreenMatrix.m[1][0] * from.x + WorldToScreenMatrix.m[1][1] * from.y + WorldToScreenMatrix.m[1][2] * from.z + WorldToScreenMatrix.m[1][3];
	to.x = WorldToScreenMatrix.m[0][0] * from.x + WorldToScreenMatrix.m[0][1] * from.y + WorldToScreenMatrix.m[0][2] * from.z + WorldToScreenMatrix.m[0][3];

	float w = WorldToScreenMatrix.m[3][0] * from.x + WorldToScreenMatrix.m[3][1] * from.y + WorldToScreenMatrix.m[3][2] * from.z + WorldToScreenMatrix.m[3][3];

	if (w < 0.001f) {
		return FALSE;
	}

	float invw = 1.0f / w;

	int width, height;
	Interfaces::EngineClient->GetScreenSize(width, height);
	float flwidth = (float)width;
	float flheight = (float)height;

	to.y *= invw;
	to.x *= invw;

	to.y = (flheight / 2.0f) - (to.y * flheight) / 2;
	to.x = (flwidth / 2.0f) + (to.x * flwidth) / 2;

	return TRUE;
}
BOOL WorldToScreen(Vector& from, Vector& to)
{
	//if (!ViewMatrixPtr)
	//{
	//	DecStr(viewmatrixstr, 50);
	//	DWORD ViewMatrix = FindMemoryPattern(EngineHandle, viewmatrixstr, 50);
	//	EncStr(viewmatrixstr, 50);
	//	if (!ViewMatrix)
	//	{
	//		return false;
	//		//THROW_ERROR(ERR_CANT_FIND_VIEWMATRIX_SIGNATURE);
	//		//exit(EXIT_SUCCESS);
	//	}
	//	ViewMatrixPtr = *(DWORD*)(ViewMatrix + 0x1); //0x11
	//}

	ViewMatrixPtr = StaticOffsets.GetOffsetValue(_ViewMatrixPtr);

	//const VMatrix& WorldToScreenMatrix = (const VMatrix&) ((*(PDWORD)(*(PDWORD)ViewMatrixPtr + 0x11)) + 0x3DC);//Interfaces::Engine->WorldToScreenMatrix();

	//DWORD dwVMatrix = *(DWORD*)ViewMatrixPtr;
	DWORD dwVMatrix = 2 * 528; //*(DWORD*)(ViewMatrixPtr + 0x0E8) * 528;// 2 * 528;
	DWORD dwVMatrix2 = *(DWORD*)(ViewMatrixPtr + 0x0DC) - 68;
	//DWORD dwResult = dwVMatrix + 2 * 528 - 68;
	DWORD dwResult = dwVMatrix + dwVMatrix2;

#if 0
	__asm {
		mov ecx, ViewMatrixPtr;
		imul edx, dword ptr ds : [ecx + 0x0e8], 0x210;
		mov eax, dword ptr ds : [ecx + 0x0dc];
		sub eax, 68
			add eax, edx;
		mov dwResult, eax
	}
#endif
	const VMatrix& WorldToScreenMatrix = (VMatrix&)(*(DWORD_PTR*)dwResult);

	for (int i = 0; i < 4; ++i)
	{
		for (int f = 0; f < 4; ++f)
		{
			if (!std::isfinite(WorldToScreenMatrix.m[i][f]))
			{
				return false;
			}
		}
	}


	to.z = 0.0f;
	to.y = WorldToScreenMatrix.m[1][0] * from.x + WorldToScreenMatrix.m[1][1] * from.y + WorldToScreenMatrix.m[1][2] * from.z + WorldToScreenMatrix.m[1][3];
	to.x = WorldToScreenMatrix.m[0][0] * from.x + WorldToScreenMatrix.m[0][1] * from.y + WorldToScreenMatrix.m[0][2] * from.z + WorldToScreenMatrix.m[0][3];

	float w = WorldToScreenMatrix.m[3][0] * from.x + WorldToScreenMatrix.m[3][1] * from.y + WorldToScreenMatrix.m[3][2] * from.z + WorldToScreenMatrix.m[3][3];

	if (w < 0.001f)
		return false;

	float invw = 1.0f / w;

	int width, height;
	Interfaces::EngineClient->GetScreenSize(width, height);
	float flwidth = (float)width;
	float flheight = (float)height;

	to.y *= invw;
	to.x *= invw;

	to.y = (flheight * 0.5f) - (to.y * flheight) * 0.5f;
	to.x = (flwidth * 0.5f) + (to.x * flwidth) * 0.5f;

	return TRUE;
}
#else
BOOL WorldToScreenCapped(Vector& from, Vector& to)
{
	static WorldToScreenMatrix_t WorldToScreenMatrix;

	ReadIntArray((DWORD)ClientHandle + m_dwViewMatrix, (char*)&WorldToScreenMatrix, sizeof(WorldToScreenMatrix_t));

	float w = 0.0f;

	to.y = WorldToScreenMatrix.flMatrix[1][0] * from.x + WorldToScreenMatrix.flMatrix[1][1] * from.y + WorldToScreenMatrix.flMatrix[1][2] * from.z + WorldToScreenMatrix.flMatrix[1][3];
	to.x = WorldToScreenMatrix.flMatrix[0][0] * from.x + WorldToScreenMatrix.flMatrix[0][1] * from.y + WorldToScreenMatrix.flMatrix[0][2] * from.z + WorldToScreenMatrix.flMatrix[0][3];
	w = WorldToScreenMatrix.flMatrix[3][0] * from.x + WorldToScreenMatrix.flMatrix[3][1] * from.y + WorldToScreenMatrix.flMatrix[3][2] * from.z + WorldToScreenMatrix.flMatrix[3][3];

	if (w < 0.01f) {
		return FALSE;
	}

	float width = (float)(rc.right - rc.left);

	float invw = 1.0f / w;
	to.x *= invw;
	to.y *= invw;

	float height = (float)(rc.bottom - rc.top);

	float x = width / 2.0f;
	float y = height / 2.0f;

	y -= 0.5f * to.y * height + 0.5f;
	x += 0.5f * to.x * width + 0.5f;

	to.y = y + (float)rc.top;
	to.x = x + (float)rc.left;

	return TRUE;
}
BOOL WorldToScreen(Vector& from, Vector& to)
{
	static WorldToScreenMatrix_t WorldToScreenMatrix;

	ReadIntArray((DWORD)ClientHandle + m_dwViewMatrix, (char*)&WorldToScreenMatrix, sizeof(WorldToScreenMatrix_t));

	float w = 0.0f;

	to.y = WorldToScreenMatrix.flMatrix[1][0] * from.x + WorldToScreenMatrix.flMatrix[1][1] * from.y + WorldToScreenMatrix.flMatrix[1][2] * from.z + WorldToScreenMatrix.flMatrix[1][3];
	to.x = WorldToScreenMatrix.flMatrix[0][0] * from.x + WorldToScreenMatrix.flMatrix[0][1] * from.y + WorldToScreenMatrix.flMatrix[0][2] * from.z + WorldToScreenMatrix.flMatrix[0][3];
	w = WorldToScreenMatrix.flMatrix[3][0] * from.x + WorldToScreenMatrix.flMatrix[3][1] * from.y + WorldToScreenMatrix.flMatrix[3][2] * from.z + WorldToScreenMatrix.flMatrix[3][3];

	if (w < 0.01f && NoAntiAimsAreOn(false)) {
		if (AimbotMaxDistFromCrosshairTxt.iValue < 6000)
			return FALSE;
	}

	float width = (float)(rc.right - rc.left);

	float invw = 1.0f / w;
	to.x *= invw;
	to.y *= invw;

	float height = (float)(rc.bottom - rc.top);

	float x = width / 2.0f;
	float y = height / 2.0f;

	y -= 0.5f * to.y * height + 0.5f;
	x += 0.5f * to.x * width + 0.5f;

	to.y = y + (float)rc.top;
	to.x = x + (float)rc.left;

	return TRUE;
}
#endif

ServerRankRevealAllFn ServerRankRevealAllEx;

void ServerRankRevealAll()
{
	static float fArray[3] = { 0.f, 0.f, 0.f };
	ServerRankRevealAllEx(fArray);
}

wchar_t* GetPlayerNameAddress(DWORD RadarAdr, int index)
{
	return (wchar_t*)(RadarAdr + (0x1E0 * index) + 0x24);
}

void GetPlayerName(DWORD RadarAdr, int index, wchar_t* dest)
{
	//radarsize = 1e0, radarname = 24, radarpointer = 50
	//index++;
#if 0
	//Dylan's method to read, would be better with doubles possibly
	* (int*)(dest) = ReadInt(RadarAdr + (0x1E0 * index) + 0x24); //Get first four bytes

	for (int stroffset = 4; stroffset < 64; stroffset += 4)
	{//1B8, 1E0
	 //Get the rest of the bytes in the name
		*(int*)(dest + (stroffset / 2)) = ReadInt(RadarAdr + (0x1E0 * index) + 0x24 + stroffset);
	}
#else
	memcpy(dest, (void*)(RadarAdr + (0x1E0 * index) + 0x24), 64);
	//wcsncpy(dest, (wchar_t*)(RadarAdr + (0x1E0 * index) + 0x24), 32);
#endif

#if 0
	//Now filter the player name from garbage
	for (int nameind = 0; nameind < 64; nameind++)
	{
		unsigned char *val = (unsigned char*)(dest + nameind);
		if (*val == '\n' || *val == '\t')
			*val = ' ';
	}
#endif
}

float ScaleFOVByWidthRatio(float fovDegrees, float ratio)
{
	float halfAngleRadians = fovDegrees * (0.5f * M_PI / 180.0f);
	float t = tan(halfAngleRadians);
	t *= ratio;
	float retDegrees = (180.0f / M_PI) * atan(t);
	return retDegrees * 2.0f;
}

//--------------------------------------------------------------------------
// Purpose:
// Given a field of view and mouse/screen positions as well as the current
// render origin and angles, returns a unit vector through the mouse position
// that can be used to trace into the world under the mouse click pixel.
// Input : 
// mousex -
// mousey -
// fov -
// vecRenderOrigin - 
// vecRenderAngles -
// Output :
// vecPickingRay
//--------------------------------------------------------------------------
void ScreenToWorld(int mousex, int mousey, float fov,
	const Vector& vecRenderOrigin,
	const QAngle& vecRenderAngles,
	Vector& vecPickingRay)
{
	float dx, dy;
	float c_x, c_y;
	float dist;
	Vector vpn, vup, vright;
	int w, h;
	Interfaces::EngineClient->GetScreenSize(w, h);

	float scaled_fov = ScaleFOVByWidthRatio(fov, ((float)w / (float)h) * 0.75f);

	c_x = w / 2;
	c_y = h / 2;

	dx = (float)mousex - c_x;
	// Invert Y
	dy = c_y - (float)mousey;

	// Convert view plane distance
	dist = c_x / tan(M_PI * scaled_fov / 360.0);

	// Decompose view angles
	AngleVectors((QAngle&)vecRenderAngles, &vpn, &vright, &vup);

	// Offset forward by view plane distance, and then by pixel offsets
	vecPickingRay = vpn * dist + vright * (dx)+vup * (dy);

	// Convert to unit vector
	VectorNormalizeFast(vecPickingRay);
}