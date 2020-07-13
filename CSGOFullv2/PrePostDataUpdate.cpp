#include "precompiled.h"
#include "BaseEntity.h"
#include "LocalPlayer.h"
#include "CPlayerrecord.h"

PreDataUpdateFn oPreDataUpdate = nullptr;
PostDataUpdateFn oPostDataUpdate = nullptr;

static float simtime2 = 9999999237498237498723984729999999999.0f;
static float simtime3 = 9999999237498237498723984729999999999.0f;

void __fastcall HookedPreDataUpdate(CBaseEntity* me, DWORD edx, DataUpdateType_t updateType)
{
	START_PROFILING
	CBaseEntity* _Entity = (CBaseEntity*)((DWORD)me - 8);
	CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_Entity);
	C_CSGOPlayerAnimState* _animstate = _Entity->GetPlayerAnimState();
	CTickrecord* _currentRecord = _playerRecord->GetCurrentRecord();

	if (updateType != DataUpdateType_t::DATA_UPDATE_CREATED && _currentRecord && !_currentRecord->m_Dormant && _currentRecord->m_PlayerBackup.Entity == _Entity)
	{
		_currentRecord->m_PlayerBackup.RestoreData();
	}
	
	_playerRecord->m_bNetUpdate = updateType <= DATA_UPDATE_DATATABLE_CHANGED;

	auto& DataUpdateVars = _playerRecord->DataUpdateVars;

	_Entity->CopyPoseParameters(DataUpdateVars.m_flPrePoseParams);
	_Entity->CopyAnimLayers(DataUpdateVars.m_PreAnimLayers);
	_Entity->CopyBoneControllers(DataUpdateVars.m_flPreBoneControllers);

	DataUpdateVars.m_flPreSimulationTime = _Entity->GetSimulationTime();
	DataUpdateVars.m_vecPreNetOrigin = _Entity->GetNetworkOrigin();
	DataUpdateVars.m_angPreAbsAngles = _Entity->GetAbsAnglesDirect();
	DataUpdateVars.m_angPreLocalAngles = _Entity->GetLocalAnglesDirect();
	DataUpdateVars.m_angPreEyeAngles = _playerRecord->m_angPristineEyeAngles; //Gaben decided that CBasePlayer::EyeAngles returns the netvar now instead of v_angle, so we have to back it up -.-
	DataUpdateVars.m_flPreVelocityModifier = _Entity->GetVelocityModifier();
	DataUpdateVars.m_flPreShotTime = 0.0f;
	if (auto _Weapon = _Entity->GetWeapon())
		DataUpdateVars.m_flPreShotTime = _Weapon->GetLastShotTime();

	if (_animstate)
	{
		DataUpdateVars.m_flPreFeetCycle = _animstate->m_flFeetCycle;
		DataUpdateVars.m_flPreFeetWeight = _animstate->m_flFeetWeight;

		//Stop forcing an animstate->Update call
		_animstate->m_iLastClientSideAnimationUpdateFramecount = Interfaces::Globals->framecount;
	}

	//Call original PreDataUpdate
	PreDataUpdateFn originalfunction = oPreDataUpdate;
	if (_playerRecord->Hooks.m_bHookedClientNetworkable)
		originalfunction = (PreDataUpdateFn)_playerRecord->Hooks.ClientNetworkable->GetOriginalHookedSub1();

	if (_Entity != LocalPlayer.Entity)
		simtime3 = _Entity->GetSimulationTime();

	originalfunction(me, updateType);

	if (_Entity != LocalPlayer.Entity)
	simtime2 = _Entity->GetSimulationTime();
	END_PROFILING
}

void __fastcall HookedPostDataUpdate(CBaseEntity* me, DWORD edx, DataUpdateType_t updateType)
{
	START_PROFILING
	CBaseEntity* _Entity = (CBaseEntity*)((DWORD)me - 8);
	CPlayerrecord* _playerRecord = g_LagCompensation.GetPlayerrecord(_Entity);
	C_CSGOPlayerAnimState* _animstate = _Entity->GetPlayerAnimState();

	//Call original PostDataUpdate
	PostDataUpdateFn originalfunction = oPostDataUpdate;
	if (_playerRecord->Hooks.m_bHookedClientNetworkable)
		originalfunction = (PreDataUpdateFn)_playerRecord->Hooks.ClientNetworkable->GetOriginalHookedSub2();

	auto& DataUpdateVars = _playerRecord->DataUpdateVars;

	//Modified bool
	bool _modified = false;

	//Check to see if origin changed
	Vector networkorigin = _Entity->GetNetworkOrigin();
	if (DataUpdateVars.m_vecPreNetOrigin != networkorigin)
	{
		DataUpdateVars.m_vecPostNetOrigin = networkorigin;
		_playerRecord->m_vecCurNetOrigin = networkorigin;
		_playerRecord->Changed.Origin = true;
		_modified = true;
	}

	//Check if velocity modifier changed
	float _VelocityModifier = _Entity->GetVelocityModifier();
	if (_VelocityModifier != DataUpdateVars.m_flPreVelocityModifier)
	{
		DataUpdateVars.m_flPostVelocityModifier = _VelocityModifier;
		_modified = true;
	}

	//Check if shot time changed
	DataUpdateVars.m_flPostShotTime = DataUpdateVars.m_flPreShotTime;
	if (auto _Weapon = _Entity->GetWeapon())
	{
		float _ShotTime = _Weapon->GetLastShotTime();
		DataUpdateVars.m_flPostShotTime = _ShotTime;
		//Check if shot time changed and we actually had a weapon during predataupdate
		if (DataUpdateVars.m_flPreShotTime != 0.0f && DataUpdateVars.m_flPreShotTime != _ShotTime)
		{
			_playerRecord->Changed.ShotTime = true;
			_modified = true;
		}
	}

	LocalPlayer.Get(&LocalPlayer);

	//Check to see if the server changed this player's animations

	for (int i = 0; i < _Entity->GetNumAnimOverlays(); i++)
	{
		C_AnimationLayer* _curLayer = _Entity->GetAnimOverlay(i);
		C_AnimationLayer* _prevLayer = &DataUpdateVars.m_PreAnimLayers[i];
		if (
			_curLayer->_m_nSequence != _prevLayer->_m_nSequence
			|| _curLayer->m_flWeightDeltaRate != _prevLayer->m_flWeightDeltaRate
			|| _curLayer->_m_flCycle != _prevLayer->_m_flCycle
			|| _curLayer->m_flWeight != _prevLayer->m_flWeight
			|| _curLayer->_m_flPlaybackRate != _prevLayer->_m_flPlaybackRate
		)
		{
			
			{
				//printf("%i  oldcycle: %f, playbackrate: %f\n", i,  _prevLayer->_m_flCycle, _prevLayer->_m_flPlaybackRate);
				//printf("cycle: %f, playbackrate: %f\n", _curLayer->_m_flCycle, _curLayer->_m_flPlaybackRate);
				
			}
			_playerRecord->Changed.Animations = true;
			_modified = true;
			break;
		}
	}

#if 0
	for (int i = 0; i < MAX_CSGO_POSE_PARAMS; i++)
	{
		const float newpose = _Entity->GetPoseParameterScaled(i);
		if (DataUpdateVars.m_flPrePoseParams[i] != newpose)
		{
			//Server updated body_pitch pose parameter
			if (i == 12)
				_playerRecord->m_flServerBodyPitch = newpose;

			_modified = true;
		}
	}
#endif

	//Now check to see if any bone controllers were modified
	for (int i = 0; i < MAXSTUDIOBONECTRLS; i++)
	{
		const float _curBoneCntrl = _Entity->GetBoneController(i);

		if (DataUpdateVars.m_flPreBoneControllers[i] != _curBoneCntrl)
		{
			DataUpdateVars.m_flPostBoneControllers[i] = _curBoneCntrl;
			_modified = true;
		}
	}

	//Now check to see if the server modified any of the feet data
	if (_animstate)
	{
		const float flFeetCycle = _animstate->m_flFeetCycle;
		const float flFeetWeight = _animstate->m_flFeetWeight;

		if (DataUpdateVars.m_flPreFeetCycle != flFeetCycle)
		{
			DataUpdateVars.m_flPostFeetCycle = flFeetCycle;
			_animstate->m_flFeetCycle = DataUpdateVars.m_flPreFeetCycle;
			_playerRecord->Changed.FeetCycle = true;
			_modified = true;
		}

		if (DataUpdateVars.m_flPreFeetWeight != flFeetWeight)
		{
			DataUpdateVars.m_flPostFeetWeight = flFeetWeight;
			_animstate->m_flFeetWeight = DataUpdateVars.m_flPreFeetWeight;
			_playerRecord->Changed.FeetWeight = true;
			_modified = true;
		}
	}

	//Now check angles to see if they changed
	QAngle absangles = _Entity->GetAbsAnglesDirect();
	if (DataUpdateVars.m_angPreAbsAngles != absangles)
		DataUpdateVars.m_angPostAbsAngles = absangles;
	QAngle localangles = _Entity->GetLocalAnglesDirect();
	if (DataUpdateVars.m_angPreLocalAngles != localangles)
		DataUpdateVars.m_angPostLocalAngles = localangles;
	QAngle eyeangles = _Entity->GetEyeAngles();
	if (eyeangles.x < 0.0f)
		eyeangles.x += 360.0f;
	if (eyeangles.y < 0.0f)
		eyeangles.y += 360.0f;
	if (eyeangles.z < 0.0f)
		eyeangles.z += 360.0f;

	if (DataUpdateVars.m_angPreEyeAngles != eyeangles)
	{
		DataUpdateVars.m_angPostEyeAngles = eyeangles;
		_playerRecord->Changed.EyeAngles = true;
		_modified = true;
	}

	//Finally, check to see if the simulationtime changed
	const float _simtime = _Entity->GetSimulationTime();
	if (DataUpdateVars.m_flPreSimulationTime != _simtime)
	{
		_playerRecord->Changed.SimulationTime = true;
		_modified = true;
	}

	if (_modified)
	{
		//Store server animations (note: won't be exact if client forced animstate->Update)
		if (_playerRecord->Changed.Animations)
			_Entity->CopyAnimLayers(DataUpdateVars.m_PostAnimLayers);

		if (!_playerRecord->Changed.SimulationTime || _simtime < DataUpdateVars.m_flPreSimulationTime)
			_playerRecord->m_bNetUpdateSilent = true;
	}
	
	if (_Entity == LocalPlayer.Entity)
	{
		//is local player

		float _SpawnTime = _Entity->GetSpawnTime();
		if (_playerRecord->m_flSpawnTime == _SpawnTime)
		{
			//don't allow server to change our animations
			//only do this if we ever rebuild the entirety of server animations
			//_Entity->WriteAnimLayers(DataUpdateVas.m_PreAnimLayers);

			//stop the server from changing our feet animations when moving

			//printf("feetweight %f\n", DataUpdateVars.m_flPostFeetYawRate);
			auto animstate = _Entity->GetPlayerAnimState();
			if (animstate)
			{
				C_AnimationLayer &feetlayer = *_Entity->GetAnimOverlayDirect(FEET_LAYER);
				C_AnimationLayer &oldfeetlayer = DataUpdateVars.m_PreAnimLayers[FEET_LAYER];
				if (feetlayer._m_flCycle != oldfeetlayer._m_flCycle)
				{
					if (feetlayer._m_nSequence == oldfeetlayer._m_nSequence)
					{
						//decrypts(0)
						if (strstr(GetSequenceName(_Entity, feetlayer._m_nSequence), XorStr("move")))
						{
							//stop server from overriding the feet movement!!!!!!!!!!!!!!!!!
							animstate->m_flFeetCycle = DataUpdateVars.m_flPreFeetCycle;
							animstate->m_flFeetWeight = DataUpdateVars.m_flPostFeetWeight;
							feetlayer = oldfeetlayer;
						}
						//encrypts(0)
					}
				}

			}
		}
		_playerRecord->m_flSpawnTime = _SpawnTime;
	}

	originalfunction(me, updateType);
	
	END_PROFILING
}