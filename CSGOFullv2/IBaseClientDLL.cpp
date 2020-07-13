#include "precompiled.h"
#include <Windows.h>
#include "IBaseClientDLL.h"
#include "CSGO_HX.h"
#include "VTHook.h"

ClientClass* IBaseClientDll::GetAllClasses()
{
	typedef ClientClass*(__thiscall* OriginalFn)(PVOID);
	return GetVFunc<OriginalFn>(this, 8)(this);
}