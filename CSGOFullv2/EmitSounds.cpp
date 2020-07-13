#include "precompiled.h"
#include "misc.h"
#include "HitboxDefines.h"
#include "Targetting.h"
#include "VTHook.h"
#include "LocalPlayer.h"
#include "netchan.h"
#include "GameMemory.h"

EmitSoundFn oEmitSound;
PanoramaAutoAcceptFn PanoramaAutoAccept = NULL;

char *landstr = new char[6]{ 22, 27, 20, 30, 37, 0 }; /*land_*/
char *defaultwalkjumpstr = new char[17]{ 62, 31, 28, 27, 15, 22, 14, 84, 45, 27, 22, 17, 48, 15, 23, 10, 0 }; /*Default.WalkJump*/
char *defaultsuitlandstr = new char[18]{ 37, 62, 31, 28, 27, 15, 22, 14, 84, 41, 15, 19, 14, 54, 27, 20, 30, 0 }; /*_Default.SuitLand*/
char *uipanoramapopup_accept_match_beepstr = new char[35]{ 47, 51, 42, 27, 20, 21, 8, 27, 23, 27, 84, 10, 21, 10, 15, 10, 37, 27, 25, 25, 31, 10, 14, 37, 23, 27, 14, 25, 18, 37, 24, 31, 31, 10, 0 }; /*UIPanorama.popup_accept_match_beep*/
char *panorama_autoaccept_sigstr = new char[207]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 79, 75, 90, 90, 79, 76, 90, 90, 66, 56, 90, 90, 73, 79, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 79, 77, 90, 90, 66, 73, 90, 90, 56, 63, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 77, 73, 90, 90, 69, 69, 90, 90, 66, 56, 90, 90, 66, 63, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 62, 90, 90, 56, 63, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 57, 77, 90, 90, 66, 76, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 79, 90, 90, 57, 67, 90, 90, 77, 78, 90, 90, 69, 69, 90, 90, 66, 56, 90, 90, 74, 75, 90, 90, 60, 60, 90, 90, 79, 74, 90, 90, 69, 69, 0 }; /*55  8B  EC  51  56  8B  35  ??  ??  ??  ??  57  83  BE  ??  ??  ??  ??  ??  73  ??  8B  8E  ??  ??  ??  ??  8D  BE  ??  ??  ??  ??  C7  86  ??  ??  ??  ??  ??  ??  ??  ??  85  C9  74  ??  8B  01  FF  50  ??*/





double last_menu_sound_stopped_time = DBL_MIN;

char *footstepsstr = new char[10] {28, 21, 21, 14, 9, 14, 31, 10, 9, 0}; /*footsteps*/

void __fastcall Hooks::EmitSound(void* ecx, void* edx, void* filter, int iEntIndex, int iChannel, const char *pSoundEntry, unsigned int nSoundEntryHash, const char *pSample, float flVolume, float flAttenuation, int nSeed, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, Vector * pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity, int unknown)
{
	if (pSoundEntry)
	{
		//Fix common/null.wav exploit
		DWORD SoundAdr = (DWORD)pSoundEntry;
		//Music.StopMenuMusic

#if 0
		if (*(char*)SoundAdr == 'M' && *(char*)(SoundAdr + 4) == 'c' && *(char*)(SoundAdr + 6) == 'S' && *(char*)(SoundAdr + 9) == 'p' && *(char*)(SoundAdr + 10) == 'M' && *(char*)(SoundAdr + 14) == 'M')
		{
			double time = QPCTime();
			if (time - last_menu_sound_stopped_time >= 2.0f)
			{
				oEmitSound(ecx, edx, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, flAttenuation, nSeed, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity, unknown);
				last_menu_sound_stopped_time = time;
			}

			return;
		}
#endif
		//ToDo: imi cvar
		//if (AutoAcceptChk.Checked)
#if 0
		{
			if (!PanoramaAutoAccept)
			{
				DecStr(uipanoramapopup_accept_match_beepstr, 34);
				DecStr(panorama_autoaccept_sigstr, 206);
				PanoramaAutoAccept = (PanoramaAutoAcceptFn)((FindMemoryPattern(ClientHandle, panorama_autoaccept_sigstr, 206)));
				EncStr(panorama_autoaccept_sigstr, 206);
				delete[] panorama_autoaccept_sigstr;
				if (!PanoramaAutoAccept)
				{
					THROW_ERROR(ERR_CANT_FIND_ISREADY_SIG);
					exit(EXIT_SUCCESS);
				}
				PanoramaAutoAccept = (PanoramaAutoAcceptFn)PanoramaAutoAccept;
			}

			if (!Interfaces::EngineClient->IsInGame() && !strcmp(pSoundEntry, uipanoramapopup_accept_match_beepstr))
			{
				PanoramaAutoAccept();
				return;
			}
		}
#endif

	}

	bool bShouldEmitSound = true;
	CBaseEntity *pEnt = Interfaces::ClientEntList->GetBaseEntity(iEntIndex);

    DecStr(footstepsstr, 9);
	if (pEnt)
	{
		if (pEnt == LocalPlayer.Entity)
		{
			if (LocalPlayer.bInPrediction && pSoundEntry)
			{
				DecStr(landstr, 5);
				DecStr(defaultwalkjumpstr, 16);
				if (!strncmp(pSoundEntry, landstr, 5) || !strcmp(pSoundEntry, defaultwalkjumpstr))
				{
					bShouldEmitSound = false;
				}
				else
				{
					//first, make sure the string is longer than 2 characters
					bool bInvalid = false;
					for (int i = 0; i < 3; i++)
					{
						if (*(char*)((DWORD)pSoundEntry + i) == 0)
						{
							bInvalid = true;
							break;
						}
					}
					if (!bInvalid)
					{
						TEAMS team = (TEAMS)LocalPlayer.Entity->GetTeam();

						DecStr(defaultsuitlandstr, 17);

						//skip the team part of the string
						char *pBeginningOfString = team == TEAM_CT ? (char*)((DWORD)pSoundEntry + 2) : (char*)((DWORD)pSoundEntry + 1);
						if (!strcmp(pBeginningOfString, defaultsuitlandstr))
							bShouldEmitSound = false;

						EncStr(defaultsuitlandstr, 17);
					}
				}
				EncStr(landstr, 5);
				EncStr(defaultwalkjumpstr, 16);
			}
		}
		else if (strstr(pSample, footstepsstr) && pEnt->IsPlayer() && pOrigin && MTargetting.IsPlayerAValidTarget(pEnt))
		{
			//CustomPlayer *pCPlayer = &AllPlayers[pEnt->index];
			//pCPlayer->PersistentData.iLastFootStepTick = Interfaces::Globals->tickcount;
			//pCPlayer->PersistentData.vecLastFootStepOrigin = *pOrigin;
		}
	}

    EncStr(footstepsstr, 9);

	if (bShouldEmitSound)
		oEmitSound(ecx, edx, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, flAttenuation, nSeed, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity, unknown);
}