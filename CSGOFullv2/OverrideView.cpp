#include "precompiled.h"
#include "OverrideView.h"
#include "Overlay.h"
#include "ConVar.h"
#include "LocalPlayer.h"
#include "UsedConvars.h"
//#include "Convars.h"

#include "Adriel/renderer.hpp"
#include "Adriel/nade_prediction.hpp"

float LocalFOV = 90.0f;
OverrideViewFN oOverrideView;
ConVar* sky = nullptr;

void __fastcall Hooks::OverrideView(void* ecx, void* edx, CViewSetup *pSetup)
{
    if (Interfaces::EngineClient->IsInGame() && Interfaces::EngineClient->IsConnected())
    {
        LocalPlayer.Get(&LocalPlayer);
	
		// invalid LocalPlayer
        if (!LocalPlayer.Entity)
        {
			// reset ThirdPerson if needed
            if (Interfaces::Input->CAM_IsThirdPerson())
				Interfaces::Input->CAM_ToFirstPerson();
            
			// return original call
            //LocalPlayer.LastViewPos = pSetup->origin;
            return oOverrideView(ecx, pSetup);
        }

		//g_Visuals.SpectateAll();
		g_Visuals.ThirdPerson(pSetup);

		nade_prediction::get().override_view(pSetup);
    }

    oOverrideView(ecx, pSetup);

	g_Visuals.FOVChanger(pSetup);

    //LocalPlayer.LastViewPos = pSetup->origin;
}
