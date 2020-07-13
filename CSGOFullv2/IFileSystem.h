#pragma once
#include "NetworkedVariables.h"

class IFileSystem
{
public:
	bool String(const void *handle, char *buf, int size)
	{
		//using T = bool(__thiscall*)(void*, const void*, char*, int);
		//return (*(T**)(void*)this)[42](this, handle, buf, size);
		return StaticOffsets.GetVFuncByType<bool(__thiscall*)(void*, const void*&, char*, int)>(_FileSystemStringVMT, this)(this, handle, buf, size);
	}

	DWORD Open(const char* name, const char*n, const char* pathid)
	{
		void* v = (void*) ((uintptr_t)this + 4);
		return GetVFunc<DWORD(__thiscall*)(void*, const char*, const char*, const char*)>(v, 8 / 4)(v, name, n, pathid);
	}

	void Close(DWORD handle)
	{
		void* v = (void*)((uintptr_t)this + 4);
		return GetVFunc<void(__thiscall*)(void*, DWORD)>(v, 12 / 4)(v, handle);
	}

	bool FileExists(const char* name, const char*n)
	{
		void* v = (void*)((uintptr_t)this + 4);
		return GetVFunc<bool(__thiscall*)(void*, const char*, const char*)>(v, 0x28 / 4)(v, name, n);
	}

	DWORD Size(const char* name, const char*n)
	{
		void* v = (void*)((uintptr_t)this + 4);
		return GetVFunc<DWORD(__thiscall*)(void*, const char*, const char*)>(v, 0x18 / 4)(v, name, n);
	}

	void Seek(DWORD handle, int offset, int seektype)
	{
		void* v = (void*)((uintptr_t)this + 4);
		return GetVFunc<void(__thiscall*)(void*, DWORD, int, int)>(v, 0x10 / 4)(v, handle, offset, seektype);
	}
	
	int Read(void* poutput, int size, DWORD handle)
	{
		void* v = (void*)((uintptr_t)this + 4);
		return GetVFunc<int(__thiscall*)(void*, void*, int, DWORD)>(v, 0)(v, poutput, size, handle);
	}
	//g_pFileSystem->Read(tmpbuf, length, data->file);


};