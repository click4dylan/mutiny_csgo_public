#include "precompiled.h"
#include "VTHook.h"
#include "Interfaces.h"
#include <intrin.h> //VS2017 requires this for _ReturnAddress

IsPausedFn oIsPaused;

bool __stdcall Hooks::IsPaused()
{
	//Disallow extrapolation!
	//if (!g_Convars.Compatibility.disable_all->GetBool())
	{
		if (_ReturnAddress() == (void*)AdrOfIsPausedExtrapolate)
			return true;
	}

	return oIsPaused();
}