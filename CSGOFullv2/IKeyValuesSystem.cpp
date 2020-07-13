#include "VTHook.h"
#include "IKeyValuesSystem.h"

IKeyValuesSystem* KeyValuesSystem()
{
	return ((IKeyValuesSystem*(__cdecl*)())gpKeyValuesVTable)(); 
}