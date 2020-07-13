#include "precompiled.h"
#include "Interfaces.h"
#include "ICvar.h"
#include "ConVar.h"

char *mouseenable1str = new char[15]{ 25, 22, 37, 23, 21, 15, 9, 31, 31, 20, 27, 24, 22, 31, 0 }; /*cl_mouseenable*/
void ToggleMouseCursor(int enablecursor)
{
	static ConVar *cl_mouseenable = nullptr;
	if (!cl_mouseenable)
	{
		DecStr(mouseenable1str, 14);
		cl_mouseenable = Interfaces::Cvar->FindVar(mouseenable1str);
		EncStr(mouseenable1str, 14);
	}
	if (cl_mouseenable)
	{
		cl_mouseenable->SetValue(enablecursor);
	}
}