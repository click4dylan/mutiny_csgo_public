#pragma once
#include "CViewSetup.h"

class ConVar;
extern float LocalFOV;
void OverrideView(CViewSetup *pSetup);
ConVar* GetSkyConVar();
//void GetNamedSkys();
void DoSkyChanger();
