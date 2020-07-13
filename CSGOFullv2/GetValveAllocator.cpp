#include "precompiled.h"
#include "Includes.h"
#include "memalloc.h"
#include "GetValveAllocator.h"
#include "VTHook.h"



IMemAlloc* FullGetValveAllocator()
{
	//char tier0dllstr[10] = {14, 19, 31, 8, 74, 84, 30, 22, 22, 0}; /*tier0.dll*/
	//char gpmemallocstr[12] = {29, 37, 10, 55, 31, 23, 59, 22, 22, 21, 25, 0}; /*g_pMemAlloc*/

	//DecStr(tier0dllstr, 9);
	HMODULE tier0 = (HMODULE)GetModuleHandle(XorStrCT("tier0.dll"));
	//EncStr(tier0dllstr, 9);

	if (!tier0)
	{
		exit(EXIT_SUCCESS);
		return 0;
	}

	//DecStr(gpmemallocstr, 11);
	IMemAlloc* allocator = *(IMemAlloc**)(DWORD)GetProcAddress(tier0, XorStrCT("g_pMemAlloc"));
	//EncStr(gpmemallocstr, 11);

	g_pMemAlloc = allocator;
	return allocator;
}

__declspec(naked) IMemAlloc* GetValveAllocator()
{
	__asm
	{
		mov eax, [g_pMemAlloc]
		test eax, eax
		jz getallocator
		retn
		getallocator:
		call FullGetValveAllocator
		retn
	}
	/*

	IMemAlloc* allocator = g_pMemAlloc;
	if (allocator)
		return allocator;

	HMODULE tier0 = (HMODULE)GetModuleHandle(XorStr("tier0.dll"));
	if (!tier0)
	{
		exit(EXIT_SUCCESS);
		return 0;
	}
	allocator = *(IMemAlloc**)(DWORD)GetProcAddress(tier0, XorStr("g_pMemAlloc"));
	g_pMemAlloc = allocator;
	return allocator;
	*/
}
