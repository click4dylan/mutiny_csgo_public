#include "precompiled.h"
#include "SetClanTag.h"
void(__fastcall* pSetClanTag) (const char*tag, const char*name);

void SetClanTag(const char *tag, const char *name)
{
	pSetClanTag(tag, (const char*)name);
}

