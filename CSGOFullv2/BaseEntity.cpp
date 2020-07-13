#include "precompiled.h"
#include <Windows.h>
#include "BaseEntity.h"
#include "Offsets.h"
#include "VTHook.h"
#include "Trace.h"
#include "Math.h"
#include "Interfaces.h"
#include "Overlay.h"
#include "BaseAnimating.h"
#include "Interpolation.h"
#include "Animation.h"
#include "LocalPlayer.h"
#include "ThirdPerson.h"
#include "ConVar.h"
#include <intrin.h>
#include "CBaseHandle.h"
#include "CPlayerResource.h"
#include "VMProtectDefs.h"
#include "ErrorCodes.h"
#include "NetworkedVariables.h"
#include "CPlayerrecord.h"
#include <atomic>
#include "utlvectorsimple.h"
#include "IClientUnknown.h"
#include "IModelInfoClient.h"
#include "IClientRenderable.h"
#include "IClientNetworkable.h"
#include "CViewModel.h"
#include "UsedConvars.h"
#include "bone_setup.h"
#include "utlbuffer.h"
#include "utlsymbol.h"
#include "GetValveAllocator.h"

HookedEntity::~HookedEntity()
{
	if (phook)
	{
		phook->ClearClassBase();
		//can't do this in the header or else it doesn't call destructor
		delete phook;
		phook = nullptr;
	}
}

IKInitFn IKInit;
UpdateTargetsFn UpdateTargets;
SolveDependenciesFn SolveDependencies;
AttachmentHelperFn AttachmentHelper;
ConstructIKFn ConstructIK;
TeleportedFn Teleported;
bool AllowShouldSkipAnimationFrame = true;
ShouldSkipAnimationFrameFn ShouldSkipAnimationFrame;
DWORD ShouldSkipAnimationFrameIsPlayerReturnAdr;
MDLCacheCriticalSectionCallFn MDLCacheCriticalSectionCall;
MarkForThreadedBoneSetupFn MarkForThreadedBoneSetupCall;
GetSequenceNameFn GetSequenceName;
GetSequenceActivityFn GetSequenceActivity;
GetSequenceActivityNameForModelFn GetSequenceActivityNameForModel;
ActivityList_NameForIndexFn ActivityList_NameForIndex;
SequencesAvailableCallFn SequencesAvailableCall;
ReevaluateAnimLodFn ReevaluateAnimLod;
LockStudioHdrFn oLockStudioHdr;
GetFirstSequenceAnimTagFn oGetFirstSequenceAnimTag;
SurpressLadderChecksFn oSurpressLadderChecks;
SetPunchVMTFn oSetPunchVMT;
IsInAVehicleFn oIsInAVehicle;
IsCarryingHostageFn oIsCarryingHostage;
EyeVectorsFn oEyeVectors;
GetBonePositionFn oGetBonePosition;
LookupBoneFn oLookupBone;

bool* s_bEnableInvalidateBoneCache;
bool* bAllowBoneAccessForViewModels;
bool* bAllowBoneAccessForNormalModels;
unsigned long* g_iModelBoneCounter;
bool* g_bInThreadedBoneSetup;
bool* s_bAbsRecomputationEnabled;
bool* s_bAbsQueriesValid;
extern bool bIsSettingUpBones;

bool AllowSetupBonesToUpdateAttachments = false;

UpdateClientSideAnimationFn oUpdateClientSideAnimation;

std::unordered_map< int, HookedEntity* > HookedNonPlayerEntities;
std::list<CBaseEntity*> g_Infernos;

void __fastcall HookedUpdateClientSideAnimation(CBaseEntity* me)
{
	return;

#if 0
	if (g_Convars.Compatibility.disable_all->GetBool() || m_bAnimationUpdateAllowed)
		oUpdateClientSideAnimation(me);
#endif
}

INetChannelInfo* GetPlayerNetInfoServer(int entindex)
{
	static DWORD EngineInterfaceServer = NULL;
	const char* sig					   = "8B  0D  ??  ??  ??  ??  52  8B  01  8B  40  54";

	if (!EngineInterfaceServer)
	{
		EngineInterfaceServer = FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)sig, strlen(sig));
		if (!EngineInterfaceServer)
			DebugBreak();

		EngineInterfaceServer = *(DWORD*)(EngineInterfaceServer + 2);
	}

	DWORD table = *(DWORD*)EngineInterfaceServer;

	return ((INetChannelInfo * (__thiscall*)(DWORD, int)) * (DWORD*)(*(DWORD*)table + 0x54))(table, entindex);
}

IClientUnknown* CBaseEntity::GetClientUnknown() const
{
	return (IClientUnknown*)this;
}

IClientNetworkable* CBaseEntity::GetClientNetworkable()
{
	return GetClientUnknown()->GetClientNetworkable();
}; //this+8

IClientRenderable* CBaseEntity::GetClientRenderable()
{
	return GetClientUnknown()->GetClientRenderable();
}; //this+4

void CBaseEntity::PreDataUpdate(DataUpdateType_t updateType)
{
	GetClientNetworkable()->PreDataUpdate(updateType);
	//auto networkable = GetClientNetworkable();
	//GetVFunc<void(__thiscall*)(IClientNetworkable*)>(networkable, m_dwPreDataUpdate)(networkable);
}

void CBaseEntity::PostDataUpdate(DataUpdateType_t updateType)
{
	GetClientNetworkable()->PostDataUpdate(updateType);
	//auto networkable = GetClientNetworkable();
	//GetVFunc<void(__thiscall*)(IClientNetworkable*)>(networkable, m_dwPostDataUpdate)(networkable);
}

// This event is triggered during the simulation phase if an entity's data has changed. It is 
// better to hook this instead of PostDataUpdate() because in PostDataUpdate(), server entity origins
// are incorrect and attachment points can't be used.
void CBaseEntity::OnDataChanged(DataUpdateType_t type)
{
	GetClientNetworkable()->OnDataChanged(type);
}

// This is called once per frame before any data is read in from the server.
void CBaseEntity::OnPreDataChanged(DataUpdateType_t type)
{
	GetClientNetworkable()->OnPreDataChanged(type);
}

//-----------------------------------------------------------------------------
// Global methods related to when abs data is correct
//-----------------------------------------------------------------------------
void CBaseEntity::SetAbsQueriesValid(bool bValid)
{
	// @MULTICORE: Always allow in worker threads, assume higher level code is handling correctly
	if (!ThreadInMainThread())
		return;

	if (!bValid)
	{
		*s_bAbsQueriesValid = false;
	}
	else
	{
		*s_bAbsQueriesValid = true;
	}
}

bool CBaseEntity::IsAbsQueriesValid(void)
{
	if (!ThreadInMainThread())
		return true;
	return *s_bAbsQueriesValid;
}

void CBaseEntity::PushEnableAbsRecomputations(bool bEnable)
{
#ifdef FIXED
	if (!ThreadInMainThread())
		return;
	if (*g_iAbsRecomputationStackPos < ARRAYSIZE(g_bAbsRecomputationStack))
	{
		g_bAbsRecomputationStack[g_iAbsRecomputationStackPos] = s_bAbsRecomputationEnabled;
		*g_iAbsRecomputationStackPos						  = *g_iAbsRecomputationStackPos + 1;
		*s_bAbsRecomputationEnabled							  = bEnable;
	}
	else
	{
		//Assert(false);
	}
#endif
}

void CBaseEntity::PopEnableAbsRecomputations()
{
#ifdef FIXED
	if (!ThreadInMainThread())
		return;
	if (*g_iAbsRecomputationStackPos > 0)
	{
		*g_iAbsRecomputationStackPos = *g_iAbsRecomputationStackPos - 1;
		s_bAbsRecomputationEnabled   = g_bAbsRecomputationStack[*g_iAbsRecomputationStackPos];
	}
	else
	{
		//Assert(false);
	}
#endif
}

void CBaseEntity::EnableAbsRecomputations(bool bEnable)
{
	if (!ThreadInMainThread())
		return;
	// This should only be called at the frame level. Use PushEnableAbsRecomputations
	// if you're blocking out a section of code.
	//Assert(g_iAbsRecomputationStackPos == 0);

	*s_bAbsRecomputationEnabled = bEnable;
}

bool CBaseEntity::IsAbsRecomputationsEnabled()
{
	if (!ThreadInMainThread())
		return true;
	return *s_bAbsRecomputationEnabled;
}

CPlayerrecord* CBaseEntity::ToPlayerRecord()
{
	return g_LagCompensation.GetPlayerrecord(index);
}

bool CBaseEntity::LockBones()
{
	CThreadFastMutex* pBoneSetupLock = GetBoneSetupLock();

	if (*g_bInThreadedBoneSetup)
	{
		if (!pBoneSetupLock->TryLock())
		{
			// someone else is handling
#ifdef _DEBUG
			DebugBreak();
#endif
			return false;
		}
		// else, we have the lock
	}
	else
	{
		pBoneSetupLock->Lock();
	}

	return true;
}

void CBaseEntity::UnlockBones()
{
	CThreadFastMutex* pBoneSetupLock = GetBoneSetupLock();
	pBoneSetupLock->Unlock();
}

int CBaseEntity::GetForceBone()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nForceBone);
}

void CBaseEntity::SetForceBone(int bone)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nForceBone) = bone;
}

QAngle CBaseEntity::GetAngleFromHitbox(int hitboxid)
{
	CBoneAccessor* accessor = GetBoneAccessor();
	mstudiohitboxset_t* set = Interfaces::ModelInfoClient->GetStudioModel(GetModel())->pHitboxSet(GetHitboxSet());
	mstudiobbox_t* hitbox   = set->pHitbox(hitboxid);
	Vector vMin, vMax;
	TransformAABB(accessor->GetBone(hitbox->bone), hitbox->bbmin, hitbox->bbmax, vMin, vMax);

	QAngle AngleFromMinsMaxs = CalcAngle(vMin, vMax);

	return AngleFromMinsMaxs;

#ifdef _DEBUG
	Vector vecForward, vecRight, vecUp;
	AngleVectors(AngleFromMinsMaxs, &vecForward, &vecRight, &vecUp);
	VectorNormalizeFast(vecForward);

	Vector topofhitbox	= vMax + vecForward * hitbox->radius;
	Vector bottomofhitbox = vMin - vecForward * hitbox->radius;
	Vector newcenter	  = (bottomofhitbox + topofhitbox) * 0.5f;
	/*
	//front of skull top left
	TargetBonePos = { vMax.x + radius * 0.5f, vMax.y + radius * 0.5f, vCenter.z + radius };

	//front of skull top right
	TargetBonePos = { vMax.x + radius * 0.5f, vMax.y - radius * 0.5f, vCenter.z + radius };

	//front of skull bottom left
	TargetBonePos = { vMin.x + radius * 0.5f, vMin.y + radius * 0.5f, vCenter.z - radius };

	//front of skull bottom right
	TargetBonePos = { vMin.x + radius * 0.5f, vMin.y - radius * 0.5f, vCenter.z - radius };

	//back of skull bottom left
	TargetBonePos = { vMin.x - radius * 0.5f, vMin.y + radius * 0.5f, vCenter.z - radius };

	//back of skull bottom right
	TargetBonePos = { vMin.x - radius * 0.5f, vMin.y - radius * 0.5f, vCenter.z - radius };

	//back of skull top left
	TargetBonePos = { vMax.x - radius * 0.5f, vMax.y + radius * 0.5f, vCenter.z + radius };

	//back of skull top right
	TargetBonePos = { vMax.x - radius * 0.5f, vMax.y - radius * 0.5f, vCenter.z + radius };
	*/

	//Interfaces::DebugOverlay->AddLineOverlay(newcenter, newcenter + vecForward * (hitbox->radius * 50), 0, 255, 0, 0, Interfaces::Globals->interval_per_tick * 2);
	//Interfaces::DebugOverlay->AddBoxOverlay(topofhitbox, Vector(-0.5, -0.5, -0.5), Vector(0.5, 0.5, 0.5), AngleFromMinsMaxs, 0, 0, 255, 255, Interfaces::Globals->interval_per_tick * 2);
	//Interfaces::DebugOverlay->AddBoxOverlay(bottomofhitbox, Vector(-0.5, -0.5, -0.5), Vector(0.5, 0.5, 0.5), AngleFromMinsMaxs, 0, 0, 255, 255, Interfaces::Globals->interval_per_tick * 2);

	//float rotation = (5 * M_PI) / 3;
	//Vector rotated;
	//rotated.x = cos(rotation) * ((topofhitbox.x + hitbox->radius) - vCenter.x) - sin(rotation) * ((topofhitbox.y + hitbox->radius) - vCenter.y) + vCenter.x;
	//rotated.y = sin(rotation) * ((topofhitbox.x + hitbox->radius) - vCenter.x) + cos(rotation) * ((topofhitbox.y + hitbox->radius) - vCenter.y) + vCenter.y; //math for y is fucked
	//rotated.z = vCenter.z;

	//Interfaces::DebugOverlay->AddBoxOverlay(rotated, Vector(-0.5, -0.5, -0.5), Vector(0.5, 0.5, 0.5), AngleFromMinsMaxs, 255, 0, 255, 255, Interfaces::Globals->interval_per_tick * 2);
	return AngleFromMinsMaxs;
#endif
}

void CBaseEntity::GetDirectionFromHitbox(Vector* vForward, Vector* vRight, Vector* vUp, int hitboxid)
{
	QAngle AngleFromMinsMaxs = GetAngleFromHitbox(hitboxid);
	AngleVectors(AngleFromMinsMaxs, vForward, vRight, vUp);
	if (vForward)
		vForward->NormalizeInPlace();
	if (vRight)
		vRight->NormalizeInPlace();
	if (vUp)
		vUp->NormalizeInPlace();
}

bool CBaseEntity::IsStrafing()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bStrafing);
}

void CBaseEntity::SetIsStrafing(bool strafing)
{
	*(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bStrafing) = strafing;
}

int CBaseEntity::GetHealth()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_iHealth); //*(int*)((DWORD)this + m_iHealth);
}

void CBaseEntity::SetHealth(int health)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_iHealth) = health;
}

int CBaseEntity::GetTeam()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_iTeamNum); //*(int*)((DWORD)this + m_iTeamNum);
}

int CBaseEntity::GetMoney()
{
	return *(int*)((DWORD)this + 0xB354);
}

void CBaseEntity::Simulate()
{
	StaticOffsets.GetVFuncByType<void(__thiscall*)(CBaseEntity*)>(_Simulate, this)(this);
}

eEntityType CBaseEntity::GetEntityType()
{
	ClassID iClassID = (ClassID)this->GetClientClass()->m_ClassID;

	if (iClassID == _CChicken)
		return chicken;

	if (iClassID == _CCSPlayer)
		return player;

	if (iClassID == _CC4)
		return c4;

	if (iClassID == _CPlantedC4)
		return plantedc4;

	if (iClassID == _CInferno || iClassID == _CBaseCSGrenadeProjectile || iClassID == _CDecoyProjectile || iClassID == _CMolotovProjectile || iClassID == _CSmokeGrenadeProjectile || iClassID == _CSensorGrenadeProjectile)
		return projectile;

	if (this->IsWeapon() && !this->GetOwner())
		return weapon;

	return none;
}
int CBaseEntity::GetFlags()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_fFlags); //*(int*)((DWORD)this + m_fFlags);
}

void CBaseEntity::SetFlags(int flags)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_fFlags) = flags;
}

void CBaseEntity::AddFlag(int flag)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_fFlags) |= flag;
}

void CBaseEntity::RemoveFlag(int flag)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_fFlags) &= ~flag;
}

int CBaseEntity::HasFlag(int flag)
{
	return GetFlags() & flag;
}

int CBaseEntity::IsOnGround()
{
	return HasFlag(FL_ONGROUND);
}

int CBaseEntity::IsInAir()
{
	return !HasFlag(FL_ONGROUND);
}

int CBaseEntity::HasEFlag(int flag)
{
	return *StaticOffsets.GetOffsetValueByType< int* >(_m_iEFlags, this) & flag;
}

int CBaseEntity::GetEFlags()
{
	return *StaticOffsets.GetOffsetValueByType< int* >(_m_iEFlags, this);
}

void CBaseEntity::SetEFlags(int flags)
{
	*StaticOffsets.GetOffsetValueByType< int* >(_m_iEFlags, this) = flags;
}

void CBaseEntity::AddEFlag(int flag)
{
	*StaticOffsets.GetOffsetValueByType< int* >(_m_iEFlags, this) |= flag;
}

void CBaseEntity::RemoveEFlag(int flag)
{
	*StaticOffsets.GetOffsetValueByType< int* >(_m_iEFlags, this) &= ~flag;
}

bool CBaseEntity::IsScoped()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bIsScoped); // 0x387C);
}
int CBaseEntity::GetTickBase()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nTickBase); //*(int*)((DWORD)this + m_nTickBase);
}

float CBaseEntity::GetThirdPersonRecoil()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flThirdpersonRecoil);
}

void CBaseEntity::SetThirdPersonRecoil(float recoil)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flThirdpersonRecoil) = recoil;
}

void CBaseEntity::SetTickBase(int base)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nTickBase) = base;
}

int CBaseEntity::GetShotsFired()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_iShotsFired); //*(int*)((DWORD)this + m_iShotsFired);
}

int CBaseEntity::GetMoveType()
{
	return *StaticOffsets.GetOffsetValueByType< int* >(_MoveType, this);
}

void CBaseEntity::SetMoveType(int type)
{
	*StaticOffsets.GetOffsetValueByType< int* >(_MoveType, this) = type;
}

int CBaseEntity::GetModelIndex()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nModelIndex); // *(int*)((DWORD)this + m_nModelIndex);
}

void CBaseEntity::SetModelIndex(int index)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nModelIndex) = index;
}

int CBaseEntity::GetHitboxSet()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nHitboxSet); //*(int*)((DWORD)this + m_nHitboxSet);
}

int CBaseEntity::GetHitboxSetServer()
{
	return *(DWORD*)((DWORD)this + m_nHitboxSetServer);
}

void CBaseEntity::SetHitboxSet(int set)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nHitboxSet) = set;
}

int CBaseEntity::GetUserID()
{
	player_info_t info;
	GetPlayerInfo(&info);
	return info.userid; //this->GetPlayerInfo().userid; //DYLAN FIX
}

int CBaseEntity::GetArmor()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_ArmorValue); //*(int*)((DWORD)this + m_ArmorValue);
}

void CBaseEntity::SetArmor(int armor)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_ArmorValue) = armor;
}

unsigned CBaseEntity::PhysicsSolidMaskForEntity()
{
	typedef unsigned int(__thiscall * OriginalFn)(void*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_PhysicsSolidMaskForEntityVMT, this)(this); //154  //8B  06  8B  CE  FF  90  ??  ??  00  00  A9  00  00  01  00  74  27
}

CBaseEntity* CBaseEntity::GetOwner()
{
	DWORD Handle = *(DWORD*)((DWORD)this + g_NetworkedVariables.Offsets.m_hOwnerEntity);
	return Interfaces::ClientEntList->GetBaseEntityFromHandle(Handle);
}

void CBaseEntity::SetOwnerHandle(EHANDLE handle)
{
	*(EHANDLE*)((DWORD)this + g_NetworkedVariables.Offsets.m_hOwnerEntity) = handle;
}

int CBaseEntity::GetGlowIndex()
{
	return *(int*)((DWORD)this + m_iGlowIndex); //*(int*)((DWORD)this + m_iGlowIndex);
}

float CBaseEntity::GetBombTimer()
{
	float bombTime	= *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flC4Blow);
	float returnValue = bombTime - Interfaces::Globals->curtime;
	return (returnValue < 0) ? 0.f : returnValue;
}

bool CBaseEntity::GetBombDefused()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bBombDefused);
}

float CBaseEntity::GetFlashDuration()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flFlashDuration); //*(float*)((DWORD)this + m_flFlashDuration);
}

void CBaseEntity::SetFlashDuration(float dur)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flFlashDuration) = dur;
}

void CBaseEntity::SetFlashMaxAlpha(float a)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flFlashMaxAlpha) = a;
}

BOOLEAN CBaseEntity::IsFlashed()
{
	return (BOOLEAN)GetFlashDuration() > 0 ? true : false;
}

bool CBaseEntity::IsSpectating()
{
	if (GetTeam() == TEAM_GOTV)
		return true;

	CBaseEntity* hObserverTarget = GetObserverTarget(); // &0xFFF;
	if (hObserverTarget && hObserverTarget != this)
	{
		auto packet = IsPlayer() ? ToPlayerRecord()->GetFarESPPacket() : nullptr;
		if (packet)
			return false;

		return true;
	}

	return false;
}

void CBaseEntity::SetMoveCollide(MoveCollide_t c)
{
	*StaticOffsets.GetOffsetValueByType< MoveCollide_t* >(_MoveCollide, this) = c;
}

MoveCollide_t CBaseEntity::GetMoveCollide()
{
	return *StaticOffsets.GetOffsetValueByType< MoveCollide_t* >(_MoveCollide, this);
}

bool CBaseEntity::GetDeadFlag()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.deadflag);
}

void CBaseEntity::SetDeadFlag(bool flag)
{
	*(bool*)((DWORD)this + g_NetworkedVariables.Offsets.deadflag) = flag;
}

int CBaseEntity::GetLifeState()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_lifeState);
}

void CBaseEntity::SetLifeState(int state)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_lifeState) = state;
}

BOOL CBaseEntity::GetAlive()
{
	//return (bool)(*(int*)((DWORD)this + m_lifeState) == 0);
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_lifeState) == LIFE_ALIVE ? TRUE : FALSE;
}

BOOLEAN CBaseEntity::GetAliveServer()
{
	typedef BOOLEAN(__thiscall * OriginalFn)(CBaseEntity*);
	return GetVFunc< OriginalFn >(this, (0x114 / 4))(this);
}

void CBaseEntity::CalcAbsolutePosition()
{
	((void(__thiscall*)(CBaseEntity*))AdrOf_CalcAbsolutePosition)(this);
}

void CBaseEntity::CalcAbsolutePositionServer()
{
#ifdef HOOK_LAG_COMPENSATION
	static DWORD absposfunc = NULL;
	if (!absposfunc)
	{
		const char* sig = "55  8B  EC  83  E4  F0  83  EC  68  56  8B  F1  57  8B  8E  D0  00  00  00";
		absposfunc		= FindMemoryPattern(GetModuleHandleA("server.dll"), (char*)sig, strlen(sig));
		if (!absposfunc)
			DebugBreak();
	}
	((void(__thiscall*)(CBaseEntity*))absposfunc)(this);
#endif
}

bool CBaseEntity::GetDormant()
{
	return *(bool*)((DWORD)this + m_bDormant_); //*(bool*)((DWORD)this + m_bDormant);
}

void CBaseEntity::SetDormant(bool dormant)
{
	*(bool*)((DWORD)this + m_bDormant_) = dormant;
}

void CBaseEntity::SetDormantVMT(bool dormant)
{
	((void (*)(CBaseEntity*, bool))OffsetOf_SetDormant)(this, dormant);
}

int CBaseEntity::GetvphysicsCollisionState()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_vphysicsCollisionState);
}

void CBaseEntity::SetvphysicsCollisionState(int state)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_vphysicsCollisionState) = state;
}

bool CBaseEntity::GetImmune()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bGunGameImmunity); //*(bool*)((DWORD)this + m_bGunGameImmunity);
}

BOOLEAN CBaseEntity::HasHelmet()
{
	return (BOOLEAN) * (BOOLEAN*)((DWORD)this + g_NetworkedVariables.Offsets.m_bHasHelmet); //*(bool*)((DWORD)this + m_bHasHelmet);
}

void CBaseEntity::SetHasHelmet(BOOLEAN helmet)
{
	*(BOOLEAN*)((DWORD)this + g_NetworkedVariables.Offsets.m_bHasHelmet) = helmet; //*(bool*)((DWORD)this + m_bHasHelmet);
}

BOOLEAN CBaseEntity::HasDefuseKit()
{
	return (BOOLEAN) * (BOOLEAN*)((DWORD)this + g_NetworkedVariables.Offsets.m_bHasDefuser);
}

BOOLEAN CBaseEntity::IsDefusing()
{
	return (BOOLEAN) * (BOOLEAN*)((DWORD)this + g_NetworkedVariables.Offsets.m_bIsDefusing);
}

Vector CBaseEntity::GetVehicleViewOrigin()
{
	return *StaticOffsets.GetOffsetValueByType< Vector* >(_VehicleViewOrigin, this);
}

void CBaseEntity::LockStudioHdr()
{
	oLockStudioHdr(this);
}

model_t* CBaseEntity::GetModel()
{
#if 1
	IClientUnknown* unk = GetClientUnknown();
	auto renderable = unk->GetClientRenderable();
	if (renderable)
		return (model_t*)renderable->GetModel();
	return nullptr;
#else
	CBaseEntity* renderable = (CBaseEntity*)((DWORD)this + 4);
	typedef model_t*(__thiscall * OriginalFn)(CBaseEntity*);
#ifdef _DEBUG
	model_t* ret = GetVFunc< OriginalFn >(renderable, 8)(renderable);
	return ret;
#else
	return GetVFunc< OriginalFn >(renderable, 8)(renderable);
#endif
	//return (model_t*)*(DWORD*)((DWORD)this + 0x6C); //DYLAN TEST THIS  //*(model_t**)((DWORD)this + 0x6C);
#endif
}

CStudioHdr* CBaseEntity::GetModelPtr()
{
	CStudioHdr* hdr = *StaticOffsets.GetOffsetValueByType< CStudioHdr** >(_m_pStudioHdr, this);
	if (!hdr && GetModel())
	{
		LockStudioHdr();
	}
	return (hdr && hdr->IsValid()) ? hdr : NULL;
}

CStudioHdr* CBaseEntity::GetStudioHdr()
{
	return GetModelPtr();
	//return (studiohdr_t*)*(DWORD*)((DWORD)this + m_pStudioHdr2);
}

mstudioseqdesc_t* CBaseEntity::pSeqdesc(int seq)
{
	return opSeqdesc((studiohdr_t*)GetModelPtr(), seq);
}

void CBaseEntity::SetModel(model_t* mod)
{
	*(DWORD*) ((DWORD)this + 0x6C) = (DWORD)mod;
}

BOOLEAN CBaseEntity::IsBroken()
{
	return (BOOLEAN) * (BOOLEAN*)((DWORD)this + g_NetworkedVariables.Offsets.m_bIsBroken); //*(bool*)((DWORD)this + m_bIsBroken);
}

QAngle* CBaseEntity::GetViewPunchAdr()
{
	return (QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_viewPunchAngle);
}

QAngle CBaseEntity::GetViewPunch()
{
	return *(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_viewPunchAngle);
}

void CBaseEntity::SetViewPunch(QAngle& punch)
{
	*(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_viewPunchAngle) = punch;
}

QAngle CBaseEntity::GetPunch()
{
	return *(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_aimPunchAngle);
}

QAngle* CBaseEntity::GetPunchAdr()
{
	return (QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_aimPunchAngle);
}

void CBaseEntity::GetPunchVMT(QAngle& dest)
{
	typedef void(__thiscall * OriginalFn)(CBaseEntity*, QAngle&);
	StaticOffsets.GetVFuncByType< OriginalFn >(_GetPunchAngleVMT, this)(this, dest);
}

void CBaseEntity::SetPunchVMT(QAngle& punch)
{
	oSetPunchVMT(this, punch);
}

void CBaseEntity::SetPunch(QAngle& punch)
{
	*(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_aimPunchAngle) = punch;
}

Vector CBaseEntity::GetPunchVel()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_aimPunchAngleVel);
}

void CBaseEntity::SetPunchVel(Vector& vel)
{
	*(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_aimPunchAngleVel) = vel;
}

QAngle CBaseEntity::GetEyeAngles()
{
	return *(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_angEyeAngles);
}

QAngle CBaseEntity::GetEyeAnglesServer()
{
	return *(QAngle*)((DWORD)this + m_angEyeAnglesServer);
}

QAngle* CBaseEntity::EyeAngles()
{
	typedef QAngle*(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_EyeAnglesVMT, this)(this);
}

void CBaseEntity::SetEyeAngles(QAngle &angles)
{
	*(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_angEyeAngles) = angles;
}

/*
QAngle CBaseEntity::GetRenderAngles()
{
	return *(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.deadflag + 0x4);
}

void CBaseEntity::SetRenderAngles(QAngle angles)
{
	*(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.deadflag + 0x4) = angles;
}
*/

Vector* CBaseEntity::GetLocalOriginVMT()
{
	IClientRenderable* renderable = GetClientRenderable();
	typedef Vector*(__thiscall * OriginalFn)(IClientRenderable*);
	return GetVFunc< OriginalFn >(renderable, 1)(renderable);
}

QAngle* CBaseEntity::GetLocalAnglesVMT()
{
	IClientRenderable* renderable = GetClientRenderable();
	typedef QAngle*(__thiscall * OriginalFn)(IClientRenderable*);
	return GetVFunc< OriginalFn >(renderable, 2)(renderable);
}

// Prevent these for now until hierarchy is properly networked
void CBaseEntity::SetLocalOrigin(const Vector& origin)
{
	Vector* dest = StaticOffsets.GetOffsetValueByType< Vector* >(_LocalOrigin, this);
	if (*dest != origin)
	{
		// This will cause the velocities of all children to need recomputation
		InvalidatePhysicsRecursive(POSITION_CHANGED);
		*dest = origin;
	}
}

void CBaseEntity::SetLocalOriginDirect(const Vector& origin)
{
	*StaticOffsets.GetOffsetValueByType< Vector* >(_LocalOrigin, this) = origin;
}

Vector CBaseEntity::GetLocalOriginDirect()
{
	return *StaticOffsets.GetOffsetValueByType< Vector* >(_LocalOrigin, this);
}

Vector CBaseEntity::GetLocalOrigin()
{
	return GetLocalOriginDirect();
}

// Prevent these for now until hierarchy is properly networked
void CBaseEntity::SetLocalAngles(const QAngle& angles)
{
	QAngle* dest = StaticOffsets.GetOffsetValueByType< QAngle* >(_LocalAngles, this);
	if (*dest != angles)
	{
		// This will cause the velocities of all children to need recomputation
		InvalidatePhysicsRecursive(ANGLES_CHANGED);
		*dest = angles;
	}
}

QAngle& CBaseEntity::GetLocalAnglesDirect()
{
	return *StaticOffsets.GetOffsetValueByType< QAngle* >(_LocalAngles, this);
}

void CBaseEntity::SetLocalAnglesDirect(QAngle& angles)
{
	*StaticOffsets.GetOffsetValueByType< QAngle* >(_LocalAngles, this) = angles;
}

float CBaseEntity::GetSpawnTime()
{
	return *(float*)((DWORD)this + OffsetOf_PlayerSpawnTime);
}

Vector CBaseEntity::GetNetworkOrigin()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecOrigin);
}

Vector CBaseEntity::GetOldOrigin()
{
	return *StaticOffsets.GetOffsetValueByType< Vector* >(_OldOrigin, this);
}

void CBaseEntity::SetOldOrigin(Vector& origin)
{
	*StaticOffsets.GetOffsetValueByType< Vector* >(_OldOrigin, this) = origin;
}

matrix3x4_t* CBaseEntity::GetCoordinateFrame()
{
	return StaticOffsets.GetOffsetValueByType< matrix3x4_t* >(_m_rgflCoordinateFrame, this);
}

void CBaseEntity::SetNetworkOrigin(Vector& origin)
{
	*(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecOrigin) = origin;
}

void CBaseEntity::SetAbsOrigin(const Vector& origin)
{
	((void(__thiscall*)(CBaseEntity*, const Vector&))AdrOf_SetAbsOrigin)(this, origin);
}

QAngle CBaseEntity::GetAbsAngles()
{
	typedef QAngle*(__thiscall * OriginalFn)(void*);
	uint8_t* indexadr = StaticOffsets.GetOffsetValueByType< uint8_t* >(_GetAbsAnglesVMT);
	DWORD index		  = *indexadr;
	return *GetVFunc< OriginalFn >(this, index / 4)(this);
}

QAngle& CBaseEntity::GetAbsAnglesDirect()
{
	return *StaticOffsets.GetOffsetValueByType< QAngle* >(_SetAbsAnglesDirect, this);
}

void CBaseEntity::SetAbsAnglesDirect(QAngle& angles)
{
	*StaticOffsets.GetOffsetValueByType< QAngle* >(_SetAbsAnglesDirect, this) = angles;
}

QAngle CBaseEntity::GetAbsAnglesServer()
{
	if ((*(DWORD*)((DWORD)this + 0x0D0) >> 11) & 1)
		CalcAbsolutePositionServer();
	return *(QAngle*)((DWORD)this + 0x1E4);
}

Vector CBaseEntity::WorldSpaceCenter()
{
	typedef Vector*(__thiscall * OriginalFn)(void*);
	return *StaticOffsets.GetVFuncByType< OriginalFn >(_WorldSpaceCenter, this)(this);
}

Vector CBaseEntity::GetEyePosition()
{
	//return ((Vector(__thiscall*)(CBaseEntity* me))ReadInt(ReadInt((uintptr_t)this) + m_OffsetEyePos))(this);
	if (this != LocalPlayer.Entity)
	{
		Vector origin = *GetAbsOrigin(); //GetOrigin();

		Vector vDuckHullMin  = Interfaces::GameMovement->GetPlayerMins(true);
		Vector vStandHullMin = Interfaces::GameMovement->GetPlayerMins(false);

		float fMore = (vDuckHullMin.z - vStandHullMin.z);

		Vector vecDuckViewOffset  = Interfaces::GameMovement->GetPlayerViewOffset(true);
		Vector vecStandViewOffset = Interfaces::GameMovement->GetPlayerViewOffset(false);
		float duckFraction		  = GetDuckAmount();
		//Vector temp = GetViewOffset();
		float tempz = ((vecDuckViewOffset.z - fMore) * duckFraction) + (vecStandViewOffset.z * (1 - duckFraction));

		origin.z += tempz;

		return (origin);
	}
	else
	{
		return GetLocalOrigin() + GetViewOffset();
	}
}

Vector CBaseEntity::Weapon_ShootPosition()
{
	Vector test;
	typedef void(__thiscall * OriginalFn)(void*, Vector*);
	StaticOffsets.GetVFuncByType< OriginalFn >(_Weapon_ShootPosition, this)(this, &test);

	return test;
}

void CBaseEntity::Weapon_ShootPosition(Vector& dest)
{
	typedef void(__thiscall * OriginalFn)(void*, Vector*);
	StaticOffsets.GetVFuncByType< OriginalFn >(_Weapon_ShootPosition, this)(this, &dest);
}

__declspec(naked) void __fastcall Static_Weapon_ShootPosition_Base(CBaseEntity*, DWORD EDX, Vector& dest)
{
	__asm
	{
		push ebp
		mov ebp, esp
		push esi
		push edi
		mov edi, [ecx]
		mov esi, [ebp + 8]
		push esi
		call [edi + weapon_shootposition_base_index]
		pop edi
		pop esi
		pop ebp
		retn 4
	}
}

CBaseEntity* CBaseEntity::GetViewEntity()
{
	return Interfaces::ClientEntList->GetBaseEntityFromHandle(*(DWORD*)((DWORD)this + g_NetworkedVariables.Offsets.m_hViewEntity));
}

//char* g_vecRenderOriginsigstr = new char[31]{ 66, 56, 90, 90, 78, 79, 90, 90, 74, 66, 90, 90, 60, 73, 90, 90, 74, 60, 90, 90, 77, 63, 90, 90, 74, 78, 90, 90, 66, 62, 0 }; /*8B  45  08  F3  0F  7E  04  8D*/
//DWORD g_vecRenderOrigin		  = NULL;

Vector MainViewOrigin(int nSlot)
{
	//if (!g_vecRenderOrigin)
	//{
	//	DecStr(g_vecRenderOriginsigstr, 30);
	//	g_vecRenderOrigin = FindMemoryPattern(ClientHandle, g_vecRenderOriginsigstr, 30);
	//	EncStr(g_vecRenderOriginsigstr, 30);
	//	delete[] g_vecRenderOriginsigstr;
	//
	//	if (!g_vecRenderOrigin)
	//	{
	//		THROW_ERROR(ERR_CANT_FIND_g_vecRenderOrigin);
	//		exit(EXIT_SUCCESS);
	//	}
	//
	//	g_vecRenderOrigin = *(DWORD*)(g_vecRenderOrigin + 8);
	//}

	DWORD slotoffset = nSlot * 2 + nSlot;
	return *(Vector*)(slotoffset * 4 + StaticOffsets.GetOffsetValue(_g_VecRenderOrigin));
}

int CBaseEntity::GetSplitScreenPlayerSlot()
{
	return *StaticOffsets.GetOffsetValueByType< int* >(_SplitScreenPlayerSlot, this);
}

//char* cachevehicleviewsigstr = new char[255]{ 59, 75, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 79, 76, 90, 90, 66, 56, 90, 90, 60, 75, 90, 90, 66, 56, 90, 90, 67, 76, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 73, 56, 90, 90, 79, 74, 90, 90, 74, 78, 90, 90, 77, 78, 90, 90, 76, 77, 90, 90, 66, 56, 90, 90, 66, 63, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 73, 90, 90, 60, 67, 90, 90, 60, 60, 90, 90, 77, 78, 90, 90, 79, 57, 90, 90, 74, 60, 90, 90, 56, 77, 90, 90, 57, 75, 90, 90, 57, 75, 90, 90, 63, 74, 90, 90, 74, 78, 90, 90, 74, 79, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 57, 75, 90, 90, 63, 67, 90, 90, 75, 74, 90, 90, 73, 67, 90, 90, 78, 66, 90, 90, 74, 78, 90, 90, 77, 79, 90, 90, 78, 67, 90, 90, 66, 56, 90, 90, 74, 66, 90, 90, 66, 79, 90, 90, 57, 67, 90, 90, 77, 78, 90, 90, 78, 73, 90, 90, 66, 56, 90, 90, 74, 75, 90, 90, 79, 77, 90, 90, 60, 60, 90, 90, 67, 74, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 0 }; /*A1  ??  ??  ??  ??  56  8B  F1  8B  96  ??  ??  ??  ??  3B  50  04  74  67  8B  8E  ??  ??  ??  ??  83  F9  FF  74  5C  0F  B7  C1  C1  E0  04  05  ??  ??  ??  ??  C1  E9  10  39  48  04  75  49  8B  08  85  C9  74  43  8B  01  57  FF  90  ??  ??  ??  ??*/
//DWORD CacheVehicleViewFunc   = NULL;
void CBaseEntity::CacheVehicleView()
{
	//if (!CacheVehicleViewFunc)
	//{
	//	DecStr(cachevehicleviewsigstr, 254);
	//	CacheVehicleViewFunc = FindMemoryPattern(ClientHandle, cachevehicleviewsigstr, 254);
	//	EncStr(cachevehicleviewsigstr, 254);
	//	delete[] cachevehicleviewsigstr;
	//
	//	if (!CacheVehicleViewFunc)
	//	{
	//		THROW_ERROR(ERR_CANT_FIND_CACHEVEHICLEVIEW_SIGNATURE);
	//		exit(EXIT_SUCCESS);
	//	}
	//}

	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*)>(_CacheVehicleView)(this);

	//((void(__thiscall*)(CBaseEntity*))CacheVehicleViewFunc)(this);
}

Vector CBaseEntity::EyePosition()
{
	Vector dest;
	StaticOffsets.GetVFuncByType<void(__thiscall*)(CBaseEntity*, Vector&)>(_EyePositionVMT, this)(this, dest);
	return dest;
}

void CBaseEntity::EyePosition(Vector& dest)
{
	StaticOffsets.GetVFuncByType<void(__thiscall*)(CBaseEntity*, Vector&)>(_EyePositionVMT, this)(this, dest);
}

void CBaseEntity::Weapon_ShootPosition_Base(Vector& dest)
{
	//Static_Weapon_ShootPosition_Base(this, 0, dest);
	EyePosition(dest);
}

BOOLEAN CBaseEntity::ShouldCollide(int collisionGroup, int contentsMask)
{
	return StaticOffsets.GetVFuncByType< BOOLEAN(__thiscall*)(CBaseEntity*, int, int) >(_ShouldCollide, this)(this, collisionGroup, contentsMask);
	//return ((BOOLEAN(__thiscall*)(CBaseEntity*, int, int))*(DWORD*)(*(DWORD*)((uintptr_t)this) + StaticOffsets.GetOffsetValue(_ShouldCollide)))(this, collisionGroup, contentsMask);
	//int adr = (*(int**)this)[193];
	//adr = adr - (int)ClientHandle;
	//typedef BOOLEAN(__thiscall* OriginalFn)(void*, int, int);
	//return GetVFunc<OriginalFn>(this, 193)(this, collisionGroup, contentsMask);
}

BOOLEAN CBaseEntity::IsTransparent()
{
	typedef BOOLEAN(__thiscall * OriginalFn)(void*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_IsTransparent, this)(this);
}

bool CBaseEntity::IsLocalPlayer(CBaseEntity* pOther)
{
	return pOther == Interfaces::ClientEntList->GetBaseEntity(Interfaces::EngineClient->GetLocalPlayer());
}

bool CBaseEntity::IsLocalPlayer()
{
	return this == LocalPlayer.Entity;
}

SolidType_t CBaseEntity::GetSolid()
{
	return StaticOffsets.GetVFuncByType< SolidType_t(__thiscall*)(CBaseEntity*) >(_GetSolid, this)(this);
}

int CBaseEntity::LookupBone(char* name)
{
	return oLookupBone((CBaseEntity*)this, name);
}

void CBaseEntity::GetBonePosition(int bone, Vector& destorigin)
{
	Vector vecs[4];
	oGetBonePosition((CBaseEntity*)this, bone, vecs);
	destorigin = { vecs[1][0], vecs[2][1], vecs[3][2] };

	//destangles = { vecs[2][0], vecs[2][1], vecs[2][2] }; //have no idea what the fuck the game is doing.
}

bool C_BaseEntity::IsBoneCacheValid()
{
	return GetMostRecentModelBoneCounter() == *g_iModelBoneCounter;
}

const matrix3x4a_t& C_BaseEntity::GetBone(int iBone)
{
	return GetBoneAccessor()->GetBone(iBone);
}

void C_BaseEntity::GetCachedBoneMatrix(int boneIndex, matrix3x4_t& out)
{
	MatrixCopy(GetBone(boneIndex), out);
}

void C_BaseEntity::GetBoneTransform(int iBone, matrix3x4_t& pBoneToWorld)
{
	CStudioHdr* hdr = GetModelPtr();
	bool bWrote		= false;
	if (hdr && iBone >= 0 && iBone < hdr->_m_pStudioHdr->numbones)
	{
		const int boneMask = BONE_USED_BY_HITBOX;
		if (hdr->_m_pStudioHdr->pBone(iBone)->flags & boneMask)
		{
			if (!IsBoneCacheValid())
			{
				SetupBones(NULL, -1, boneMask, Interfaces::Globals->curtime);
			}
			GetCachedBoneMatrix(iBone, pBoneToWorld);
			bWrote = true;
		}
	}
	if (!bWrote)
	{
		MatrixCopy(EntityToWorldTransform(), pBoneToWorld);
	}
	//Assert(GetModelPtr() && iBone >= 0 && iBone < GetModelPtr()->numbones());
}

void CBaseEntity::GetBonePositionRebuilt(int iBone, Vector& origin, QAngle& angles)
{
	matrix3x4_t bonetoworld;
	GetBoneTransform(iBone, bonetoworld);

	MatrixAngles(bonetoworld, angles, origin);
}

Vector CBaseEntity::GetBonePosition(int HitboxID)
{
	Vector minsmaxs[2];
	if (GetBoneTransformed(HitboxID, nullptr, nullptr, nullptr, minsmaxs))
		return (minsmaxs[0] + minsmaxs[1]) * 0.5f;

	return vecZero;
}

void CBaseEntity::GetBoneTransformed(matrix3x4_t* srcMatrix, mstudiobbox_t *hitbox, matrix3x4_t *rotatedmatrix, float* radius, Vector* minsmaxs, Vector* worldminsmaxs)
{
	if (minsmaxs)
	{
		minsmaxs[0] = hitbox->bbmin;
		minsmaxs[1] = hitbox->bbmax;
	}

	if (radius)
		*radius = hitbox->radius;

	if (hitbox->radius != -1.0f)
	{
		//capsule
		if (rotatedmatrix)
			*rotatedmatrix = srcMatrix[hitbox->bone];

		if (worldminsmaxs)
		{
			//TransformAABB(srcMatrix[hitbox->bone], hitbox->bbmin, hitbox->bbmax, worldminsmaxs[0], worldminsmaxs[1]);
			VectorTransformZ(hitbox->bbmin, srcMatrix[hitbox->bone], worldminsmaxs[0]);
			VectorTransformZ(hitbox->bbmax, srcMatrix[hitbox->bone], worldminsmaxs[1]);

			// get hitbox center point
			Vector forward, right, up;

			// get forward from Hitbox
			QAngle AngleFromMinsMaxs = CalcAngle(worldminsmaxs[0], worldminsmaxs[1]);

			AngleVectors(AngleFromMinsMaxs, &forward, &right, &up);
			VectorNormalizeFast(forward);
			VectorNormalizeFast(right);
			VectorNormalizeFast(up);

			float _radius = hitbox->radius;

			// get adjusted points
			auto _adjust = (forward * _radius) + (right * _radius) + (up *_radius);
			worldminsmaxs[0] -= _adjust; //bottom
			worldminsmaxs[1] += _adjust; //top
		}
	}
	else
	{
		//box
		if (rotatedmatrix || worldminsmaxs)
		{
			matrix3x4_t matrix_rotation;
			matrix3x4_t final_matrix;
			AngleMatrix(hitbox->angles, matrix_rotation);
			ConcatTransforms(srcMatrix[hitbox->bone], matrix_rotation, final_matrix);

			if (rotatedmatrix)
				*rotatedmatrix = final_matrix;

			if (worldminsmaxs)
			{
				TransformAABB(final_matrix, hitbox->bbmin, hitbox->bbmax, worldminsmaxs[0], worldminsmaxs[1]);
			}
		}
	}
}

bool CBaseEntity::GetBoneTransformed(int HitboxID, matrix3x4_t *rotatedmatrix, float* radius, Vector* minsmaxs, Vector* worldminsmaxs)
{
	matrix3x4_t* destBoneMatrixes = (matrix3x4_t*)GetCachedBoneData()->Base();

	auto modelptr = GetModelPtr();
	studiohdr_t* hdr;

	if (modelptr && (hdr = modelptr->_m_pStudioHdr, hdr))
	{
		mstudiohitboxset_t* set = hdr->pHitboxSet(GetHitboxSet());
		mstudiobbox_t* hitbox;
		if (set && (hitbox = set->pHitbox(HitboxID), hitbox))
		{
			GetBoneTransformed(destBoneMatrixes, hitbox, rotatedmatrix, radius, minsmaxs, worldminsmaxs);

			return true;
		}
	}

	return false;
}

void CBaseEntity::GetBonePosition(int iBone, Vector& origin, QAngle& angles)
{
	Vector worldminsmaxs[2];
	matrix3x4_t matrix;
	if (GetBoneTransformed(iBone, &matrix, nullptr, nullptr, worldminsmaxs))
	{
		origin = (worldminsmaxs[0] + worldminsmaxs[1]) * 0.5f;
		MatrixAngles(matrix, angles, origin);
		return;
	}

	origin = vecZero;
	angles = angZero;
}

Vector CBaseEntity::GetBonePositionCachedOnly(int HitboxID, matrix3x4_t* matrixes)
{
	auto ptr = GetModelPtr();
	studiohdr_t* hdr;
	if (ptr && (hdr = ptr->_m_pStudioHdr, hdr))
	{
		mstudiohitboxset_t* set = hdr->pHitboxSet(GetHitboxSet());
		mstudiobbox_t* hitbox   = set->pHitbox(HitboxID);
		Vector minsmaxs[2];
		GetBoneTransformed(matrixes, hitbox, nullptr, nullptr, nullptr, minsmaxs);
		Vector vCenter = (minsmaxs[0] + minsmaxs[1]) * 0.5f;
		return vCenter;
	}
	return vecZero;
}

//-----------------------------------------------------------------------------
// Purpose: Retrieves the coordinate frame for this entity.
// Input  : forward - Receives the entity's forward vector.
//			right - Receives the entity's right vector.
//			up - Receives the entity's up vector.
//-----------------------------------------------------------------------------
void CBaseEntity::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp)
{
	// This call is necessary to cause m_rgflCoordinateFrame to be recomputed
	const matrix3x4_t &entityToWorld = EntityToWorldTransform();

	if (pForward != NULL)
	{
		MatrixGetColumn(entityToWorld, 0, *pForward);
	}

	if (pRight != NULL)
	{
		MatrixGetColumn(entityToWorld, 1, *pRight);
		*pRight *= -1.0f;
	}

	if (pUp != NULL)
	{
		MatrixGetColumn(entityToWorld, 2, *pUp);
	}
}

bool CBaseEntity::IsViewModel()
{
	using OriginalFn = bool(__thiscall*)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_IsViewModel, this)(this);
}

bool CBaseEntity::IsInFiringActivity()
{
	CBaseCombatWeapon* weapon = GetWeapon();
	if (weapon)
	{
		int iWeaponIndex		   = weapon->GetItemDefinitionIndex();
		C_AnimationLayer* aimlayer = GetAnimOverlay(AIMSEQUENCE_LAYER1);
		Activities act			   = GetSequenceActivity(this, aimlayer->_m_nSequence);
		if (act == ACT_CSGO_FIRE_PRIMARY || ((act == ACT_CSGO_FIRE_SECONDARY || act == ACT_CSGO_FIRE_SECONDARY_OPT_1 || act == ACT_CSGO_FIRE_SECONDARY_OPT_2) && (iWeaponIndex == WEAPON_GLOCK || iWeaponIndex == WEAPON_REVOLVER || iWeaponIndex == WEAPON_FAMAS || weapon->IsKnife(iWeaponIndex))))
			return true;
	}
	return false;
}

int CBaseEntity::GetLastSetupBonesFrameCount()
{
	return *StaticOffsets.GetOffsetValueByType< int* >(_LastSetupBonesFrameCount, this);
}

void CBaseEntity::SetLastSetupBonesFrameCount(int framecount)
{
	*StaticOffsets.GetOffsetValueByType< int* >(_LastSetupBonesFrameCount, this) = framecount;
}

bool CBaseEntity::IsBoneAccessAllowed()
{
	if (!ThreadInMainThread())
	{
		return true;
	}

	if (IsViewModel())
		return *bAllowBoneAccessForViewModels;
	else
		return *bAllowBoneAccessForNormalModels;
}

int CBaseEntity::GetPreviousBoneMask()
{
	return *StaticOffsets.GetOffsetValueByType< DWORD* >(_m_iPrevBoneMask, this);
}

void CBaseEntity::SetPreviousBoneMask(int mask)
{
	*StaticOffsets.GetOffsetValueByType< DWORD* >(_m_iPrevBoneMask, this) = mask;
}

int CBaseEntity::GetAccumulatedBoneMask()
{
	return *StaticOffsets.GetOffsetValueByType< int* >(_m_iAccumulatedBoneMask, this);
}

void CBaseEntity::SetAccumulatedBoneMask(int mask)
{
	*StaticOffsets.GetOffsetValueByType< int* >(_m_iAccumulatedBoneMask, this) = mask;
}

float CBaseEntity::GetLastBoneSetupTime()
{
	return *StaticOffsets.GetOffsetValueByType< float* >(_m_flLastBoneSetupTime, this);
}

void CBaseEntity::SetLastBoneSetupTime(float time)
{
	*StaticOffsets.GetOffsetValueByType< float* >(_m_flLastBoneSetupTime, this) = time;
}

DWORD CBaseEntity::GetIK()
{
	return *StaticOffsets.GetOffsetValueByType< DWORD* >(_m_pIK_offset, this);
}

void CBaseEntity::SetIK(DWORD ik)
{
	*StaticOffsets.GetOffsetValueByType< DWORD* >(_m_pIK_offset, this) = ik;
}

unsigned char CBaseEntity::GetEntClientFlags()
{
	return *StaticOffsets.GetOffsetValueByType< unsigned char* >(_m_EntClientFlags, this);
}

void CBaseEntity::SetEntClientFlags(unsigned char flags)
{
	*StaticOffsets.GetOffsetValueByType< unsigned char* >(_m_EntClientFlags, this) = flags;
}

void CBaseEntity::AddEntClientFlag(unsigned char flag)
{
	*StaticOffsets.GetOffsetValueByType< unsigned char* >(_m_EntClientFlags, this) |= flag;
}

BOOLEAN CBaseEntity::IsToolRecording()
{
	return *StaticOffsets.GetOffsetValueByType< BOOLEAN* >(_m_bIsToolRecording, this);
}

BOOLEAN CBaseEntity::GetPredictable()
{
	return *StaticOffsets.GetOffsetValueByType< BOOLEAN* >(_GetPredictable, this);
}

CThreadFastMutex* CBaseEntity::GetBoneSetupLock()
{
	return StaticOffsets.GetOffsetValueByType< CThreadFastMutex* >(_m_BoneSetupLock, this);
}

#pragma optimize("", off)

float CBaseEntity::LastBoneChangedTime()
{
	static DWORD temp = StaticOffsets.GetOffsetValue(_LastBoneChangedTime_Offset);

	using OriginalFn = float(__thiscall*)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_LastBoneChangedTime_Offset, this)(this);
}

#pragma optimize("", on)

char* cl_setupallbonesstr = new char[17]{ 25, 22, 37, 41, 31, 14, 15, 10, 59, 22, 22, 56, 21, 20, 31, 9, 0 }; /*cl_SetupAllBones*/
ConVar* cl_setupallbones  = nullptr;

void CBaseEntity::FormatViewModelAttachment(int i, matrix3x4_t& world)
{
	using OriginalFn = void(__thiscall*)(CBaseEntity*, int, matrix3x4_t&);
	StaticOffsets.GetVFuncByType< OriginalFn >(_FormatViewModelAttachment, this)(this, i, world);
}

//-----------------------------------------------------------------------------
// Purpose: Put a value into an attachment point by index
// Input  : number - which point
// Output : float * - the attachment point
//-----------------------------------------------------------------------------
bool CBaseEntity::PutAttachment(int number, const matrix3x4_t& attachmentToWorld)
{
	CUtlVectorSimple& attachments = m_Attachments();
	if (number < 1 || number > attachments.Count())
		return false;

	CAttachmentData* pAtt = (CAttachmentData*)attachments.Retrieve(number - 1, sizeof(CAttachmentData));
	int framecount;

	if (Interfaces::Globals->frametime <= 0 || (framecount = 2 * pAtt->m_nLastFramecount >> 1, framecount <= 0) || framecount != (Interfaces::Globals->framecount - 1))
	{
		pAtt->m_vOriginVelocity.Init();
	}
	else
	{
		Vector vecPreviousOrigin, vecOrigin;
		MatrixPosition(pAtt->m_AttachmentToWorld, vecPreviousOrigin);
		MatrixPosition(attachmentToWorld, vecOrigin);
		pAtt->m_vOriginVelocity = (vecOrigin - vecPreviousOrigin) / Interfaces::Globals->frametime;
	}
	
	pAtt->m_nLastFramecount ^= (pAtt->m_nLastFramecount ^ Interfaces::Globals->framecount) & 0x7FFFFFFF;
	pAtt->m_bAnglesComputed &= 0xFE;
	pAtt->m_AttachmentToWorld = attachmentToWorld;

	return true;
}

//note: custommatrix is a newly added variable that does't exist in the game itself
void CBaseEntity::SetupBones_AttachmentHelper(CStudioHdr* hdr, matrix3x4a_t* custommatrix)
{
	//return Wrap_AttachmentHelper(hdr);

	if (!hdr || !hdr->GetNumAttachments())
		return;

	// calculate attachment points
	matrix3x4_t world;

	const int numAttachments = hdr->GetNumAttachments();

	for (int i = 0; i < numAttachments; i++)
	{
		mstudioattachment_t& pattachment = hdr->pAttachment(i);
		int iBone						 = hdr->GetAttachmentBone(i);

		if (pattachment.flags & ATTACHMENT_FLAG_WORLD_ALIGN)
		{
			Vector vecLocalBonePos, vecWorldBonePos;
			MatrixGetColumn(pattachment.local, 3, vecLocalBonePos);;
			VectorTransform(vecLocalBonePos, custommatrix ? custommatrix[iBone] : GetBone(iBone), vecWorldBonePos);

			SetIdentityMatrix(world);
			PositionMatrix(vecWorldBonePos, world);
		}
		else
		{
			ConcatTransforms(custommatrix ? custommatrix[iBone] : GetBone(iBone), pattachment.local, world);
		}
		FormatViewModelAttachment(i, world);
		PutAttachment(i + 1, world);
	}
}


//-----------------------------------------------------------------------------
// Purpose: build matrices first from the parent, then from the passed in arrays if the bone doesn't exist on the parent
//-----------------------------------------------------------------------------

void CBaseEntity::BuildMatricesWithBoneMerge(
	const CStudioHdr *pStudioHdr,
	const QAngle& angles,
	const Vector& origin,
	const Vector pos[MAXSTUDIOBONES],
	const Quaternion q[MAXSTUDIOBONES],
	matrix3x4_t bonetoworld[MAXSTUDIOBONES],
	CBaseEntity *pParent,
	matrix3x4_t *pParentCache
)
{
	CStudioHdr *fhdr = pParent->GetModelPtr();
	mstudiobone_t *pbones = pStudioHdr->pBone(0);

	matrix3x4_t rotationmatrix; // model to world transformation
	AngleMatrix(angles, origin, rotationmatrix);

	for (int i = 0; i < pStudioHdr->numbones(); i++)
	{
		// Now find the bone in the parent entity.
		bool merged = false;
		int parentBoneIndex = Studio_BoneIndexByName(fhdr, pbones[i].pszName());
		if (parentBoneIndex >= 0)
		{
			matrix3x4_t *pMat = &pParentCache[parentBoneIndex];
			if (pMat)
			{
				MatrixCopy(*pMat, bonetoworld[i]);
				merged = true;
			}
		}

		if (!merged)
		{
			// If we get down here, then the bone wasn't merged.
			matrix3x4_t bonematrix;
			QuaternionMatrix(q[i], pos[i], bonematrix);

			if (pbones[i].parent == -1)
			{
				ConcatTransforms(rotationmatrix, bonematrix, bonetoworld[i]);
			}
			else
			{
				ConcatTransforms(bonetoworld[pbones[i].parent], bonematrix, bonetoworld[i]);
			}
		}
	}
}

matrix3x4_t *CBaseEntity::GetBoneCache_Server(float curTime)
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return nullptr;

	auto pcache = GetBoneAccessor();
	int boneMask = BONE_USED_BY_HITBOX | BONE_USED_BY_ATTACHMENT;

	if (IsLocalPlayer())
		boneMask |= BONE_USED_BY_BONE_MERGE; //eye candy

	boneMask |= BONE_USED_BY_VERTEX_MASK; //this allows us to render the player model (not required for traceray or drawing hitboxes)
	boneMask |= BONE_USED_BY_WEAPON; //fixes weapons in their hands

	if (pcache)
	{
		if (curTime - GetLastBoneSetupTime() <= 0.1f && (pcache->GetReadableBones() & boneMask) == boneMask && GetLastBoneSetupTime() <= curTime)
		{
			// Msg("%s:%s:%s (%x:%x:%8.4f) cache\n", GetClassname(), GetDebugName(), STRING(GetModelName()), boneMask, pcache->m_boneMask, pcache->m_timeValid );
			// in memory and still valid, use it!
			return pcache->GetBoneArrayForWrite();
		}
		// in memory, but missing some of the bone masks
		if ((pcache->GetReadableBones() & boneMask) != boneMask)
		{
			InvalidateBoneCache();
			pcache = NULL;
		}
	}

	SetupBones_Server(boneMask, curTime);

	if (pcache)
	{
		// still in memory but out of date, refresh the bones.
		SetLastBoneSetupTime(curTime);
		SetLastSetupBonesFrameCount(Interfaces::Globals->framecount);
		SetMostRecentModelBoneCounter(*g_iModelBoneCounter);
	}
	else
	{
		pcache = GetBoneAccessor();
		pcache->SetReadableBones(boneMask);
		pcache->SetWritableBones(boneMask);
		SetLastBoneSetupTime(curTime);
		SetLastSetupBonesFrameCount(Interfaces::Globals->framecount);
		SetMostRecentModelBoneCounter(*g_iModelBoneCounter);
	}
	return pcache->GetBoneArrayForWrite();
}

class PoseDebugger;
class IKContext;

class _BoneSetup {
public:
	_BoneSetup(const CStudioHdr* pStudioHdr, int boneMask, const float poseParameter[], PoseDebugger* pPoseDebugger = nullptr)
	{
		m_boneMask = boneMask;
		m_flPoseParameters = poseParameter;
		m_pStudioHdr = pStudioHdr;
		m_pPoseDebugger = pPoseDebugger;
	}

	void InitPose(Vector* pos, Quaternion* q)
	{
		static void* fn = nullptr;
		if (!fn) fn = StaticOffsets.GetOffsetValueByType<void*>(_InitPose);

		auto studioHdr = m_pStudioHdr;
		auto boneMask = m_boneMask;

		__asm
		{
			pushad
			pushfd

			mov ecx, studioHdr
			mov edx, pos
			push boneMask
			push q
			call fn
			add esp, 8

			popfd
			popad
		}
	}

	void AccumulatePose(Vector pos[], Quaternion q[], int iSequence, float flCycle, float flWeight, float flTime, IKContext* pIKContext)
	{
#ifdef _DEBUG
		//Remove breakpoint when debugger is attached
		static bool onceOnly = false;
		if (!onceOnly)
		{
			onceOnly = true;
			auto pattern = (void*)FindMemoryPattern(ServerHandle, "CC  F3  0F  10  4D  ??  0F  57");
			if (pattern)
			{
				DWORD oldProt;
				VirtualProtect(pattern, 1, PAGE_EXECUTE_READWRITE, &oldProt);
				*(unsigned char*)pattern = 0x90;
				VirtualProtect(pattern, 1, oldProt, &oldProt);
			}
		}
#endif

		static void* fn = nullptr;
		if (!fn) fn =  (void*)(StaticOffsets.GetOffsetValueByType<DWORD>(_AccumulatePose) - 0x6);

		__asm
		{
			pushad
			pushfd

			mov ecx, this
			push pIKContext
			push flTime
			push flWeight
			push flCycle
			push iSequence
			push q
			push pos
			call fn

			popfd
			popad
		}
	}

	void CalcAutoplaySequences(Vector pos[], Quaternion q[], float flRealTime, IKContext* pIKContext)
	{
		static void* fn = nullptr;
		if (!fn) fn = StaticOffsets.GetOffsetValueByType<void*>(_CalcAutoplaySequences);
		__asm
		{
			mov     eax, Interfaces::Globals
			mov     ecx, this
			push    0
			push    q
			push    pos
			movss   xmm3, dword ptr[eax + 10h]; a3
			call    fn
		}
	}

	void CalcBoneAdj(Vector pos[], Quaternion q[], const float* encodedControllerArray)
	{
		static void* fn = nullptr;
		if (!fn) fn = StaticOffsets.GetOffsetValueByType<void*>(_CalcBoneAdj);

		auto studioHdr = m_pStudioHdr;
		auto boneMask = m_boneMask;

		__asm
		{
			pushad
			pushfd

			mov ecx, studioHdr
			mov edx, pos
			push boneMask
			push encodedControllerArray
			push q
			call fn
			add esp, 0ch

			popfd
			popad
		}

	}

	const CStudioHdr* m_pStudioHdr;
	int m_boneMask;
	const float* m_flPoseParameters;
	PoseDebugger* m_pPoseDebugger;
};

class BoneSetup
{
public:
	BoneSetup(const CStudioHdr* pStudioHdr, int boneMask, const float* poseParameter)
	{
		m_pBoneSetup = new _BoneSetup(pStudioHdr, boneMask, poseParameter);
	}
	~BoneSetup()
	{
		if (m_pBoneSetup)
			delete m_pBoneSetup;
	}
	void InitPose(Vector* pos, Quaternion* q)
	{
		m_pBoneSetup->InitPose(pos, q);
	}
	void AccumulatePose(Vector pos[], Quaternion q[], int sequence, float cycle, float flWeight, float flTime, IKContext* pIKContext)
	{
		m_pBoneSetup->AccumulatePose(pos, q, sequence, cycle, flWeight, flTime, pIKContext);
	}
	void CalcAutoplaySequences(Vector pos[], Quaternion q[], float flRealTime, IKContext* pIKContext)
	{
		m_pBoneSetup->CalcAutoplaySequences(pos, q, flRealTime, pIKContext);
	}
	void CalcBoneAdj(Vector pos[], Quaternion q[], const float* encodedControllerArray)
	{
		m_pBoneSetup->CalcBoneAdj(pos, q, encodedControllerArray);
	}
private:
	_BoneSetup* m_pBoneSetup;
};

class IKContext
{
public:
	static IKContext* CreateIKContext()
	{
		IKContext* ik = (IKContext*)MALLOC(sizeof(IKContext));
		//ik->Construct();
		if (ik)
			ConstructIK(ik);
		return ik;
	}

	static void DestroyIKContext(IKContext* ik)
	{
		StaticOffsets.GetOffsetValueByType<void(__thiscall*)(IKContext*)>(_IKDestruct)(ik);
		FREE_SPECIAL(ik);
	}

	void Construct()
	{
		m_target.EnsureCapacity(12); // FIXME: this sucks, shouldn't it be grown?
		m_iFramecounter = -1;
		m_pStudioHdr = NULL;
		m_flTime = -1.0f;
		m_target.SetSize(0);

		//typedef void(__thiscall * Fn)(IKContext * player);
		//static Fn fn = StaticOffsets.GetOffsetValueByType<Fn>(_IKConstruct);
		//if (!fn) fn = reinterpret_cast<Fn>(GETSIG("server.dll", "53 8B D9 F6 C3"));
		//fn(ik);
	}

	IKContext()
	{
		m_target.EnsureCapacity(12); // FIXME: this sucks, shouldn't it be grown?
		m_iFramecounter = -1;
		m_pStudioHdr = NULL;
		m_flTime = -1.0f;
		m_target.SetSize(0);
	}

	void _Init(const CStudioHdr* pStudioHdr, const QAngle& absAngles, const Vector& adjOrigin, const float curTime, int ikCounter, int boneMask)
	{
		typedef void(__thiscall * Fn)(IKContext * pIK, const CStudioHdr * pStudioHdr, const QAngle& absAngles, const Vector &adjOrigin, const float curTime, int ikCounter, int boneMask);
		IKInit((DWORD)this, (CStudioHdr*)pStudioHdr, (QAngle&)absAngles, (Vector&)adjOrigin, curTime, ikCounter, boneMask);
		//static Fn fn = nullptr;
		//if (!fn) fn = reinterpret_cast<Fn>(GETSIG("server.dll", "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D"));
		//return fn(this, pStudioHdr, absAngles, adjOrigin, curTime, ikCounter, boneMask);
	}

	void _UpdateTargets(Vector* pos, Quaternion* q, matrix3x4_t* pBoneToWorld, CBoneBitList* bonesComputed)
	{
		typedef void(__thiscall * Fn)(IKContext * pIK, Vector * pos, Quaternion * q, matrix3x4_t * pBoneToWorld, CBoneBitList * bonesComputed);
		UpdateTargets((DWORD)this, pos, q, pBoneToWorld, (byte*)bonesComputed);
		//static Fn fn = nullptr;
		//if (!fn) fn = reinterpret_cast<Fn>(GETSIG("server.dll", "55 8B EC 83 E4 F0 81 EC ? ? ? ? 33 D2"));
		//return fn(this, pos, q, pBoneToWorld, bonesComputed);
	}

	void _SolveDependencies(Vector* pos, Quaternion* q, matrix3x4_t* pBoneToWorld, CBoneBitList* bonesComputed)
	{
		typedef void(__thiscall * Fn)(IKContext * pIK, Vector * pos, Quaternion * q, matrix3x4_t * pBoneToWorld, CBoneBitList * bonesComputed);
		SolveDependencies((DWORD)this, pos, q, pBoneToWorld, (byte*)bonesComputed);
		//static Fn fn = nullptr;
		//if (!fn) fn = Wrap_Solv//reinterpret_cast<Fn>(GETSIG("server.dll", "55 8B EC 83 E4 F0 81 EC ? ? ? ? 8B 81"));
		//return fn(this, pos, q, pBoneToWorld, bonesComputed);
	}

	CUtlVectorFixed< CIKTarget, 12 >	m_target;

	//MVAR(matrix3x4_t, m_rootxform, 0x1030);
	//MVAR(float, m_iFrameCounter, 0x1060);
	//MVAR(float, m_flTime, 0x1064);
	//MVAR(int, m_iBoneMask, 0x1068);

private:
	CStudioHdr const *m_pStudioHdr;
	CUtlVector< CUtlVector< ikcontextikrule_t > > m_ikChainRule;
	CUtlVector< ikcontextikrule_t > m_ikLock;
	matrix3x4a_t m_rootxform;

	int m_iFramecounter;
	float m_flTime;
	int m_boneMask;

	//static constexpr auto size = 0x1070;
	//char _pad[size];
}; //0x1070

void CBaseEntity::GetSkeleton(CStudioHdr* studioHdr, Vector* pos, Quaternion* q, int boneMask)
{
	if (!studioHdr)
		return;

	if (!studioHdr->SequencesAvailable())
		return;

	BoneSetup boneSetup(studioHdr, boneMask, (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter));
	boneSetup.InitPose(pos, q);

	if (!GetIK())
	{
		SetIK((DWORD)IKContext::CreateIKContext());

		IKInit(GetIK(), studioHdr, GetAbsAngles(), *GetAbsOrigin(), Interfaces::Globals->curtime, 0, 0x40000);
	}

	boneSetup.AccumulatePose(pos, q, GetSequence(), GetCycle(), 1.f, Interfaces::Globals->curtime, (IKContext*)GetIK());

	// sort the layers
	int layer[MAX_OVERLAYS];
	int i;
	for (i = 0; i < GetNumAnimOverlays(); i++)
	{
		layer[i] = MAX_OVERLAYS;
	}
	for (i = 0; i < GetNumAnimOverlays(); i++)
	{
		C_AnimationLayer &pLayer = *GetAnimOverlay(i);
		if ((pLayer.m_flWeight > 0) && pLayer.IsActive() && pLayer.m_nOrder >= 0 && pLayer.m_nOrder < GetNumAnimOverlays())
		{
			layer[pLayer.m_nOrder] = i;
		}
	}

	for (auto animLayerIndex = 0; animLayerIndex < GetNumAnimOverlays(); animLayerIndex++)
	{
		auto pLayer = GetAnimOverlay(animLayerIndex);
		if (pLayer->m_flWeight > 0 && pLayer->m_nOrder >= 0 && pLayer->m_nOrder < GetNumAnimOverlays()) {

			boneSetup.AccumulatePose(
				pos, q,
				pLayer->_m_nSequence, pLayer->_m_flCycle, pLayer->m_flWeight,
				Interfaces::Globals->curtime, (IKContext*)GetIK());
		}
	}

	if (GetIK())
	{
		IKContext *auto_ik = IKContext::CreateIKContext();
		IKInit((DWORD)auto_ik, studioHdr, GetAbsAngles(), *GetAbsOrigin(), Interfaces::Globals->curtime, 0, boneMask);
		boneSetup.CalcAutoplaySequences(pos, q, Interfaces::Globals->curtime, auto_ik);

		IKContext::DestroyIKContext(auto_ik);
	}
	else {
		boneSetup.CalcAutoplaySequences(pos, q, Interfaces::Globals->curtime, 0);
	}

	boneSetup.CalcBoneAdj(pos, q, (float*)((DWORD)this + StaticOffsets.GetOffsetValue(_m_flEncodedController)));
}

bool CBaseEntity::SetupBones_Server(int boneMask, float currentTime)
{
#ifdef _DEBUG
	if (!IsBoneAccessAllowed())
	{
		static float lastWarning = 0.0f;

		// Prevent spammage!!!
		if (Interfaces::Globals->realtime >= lastWarning + 1.0f)
		{
			AllocateConsole();
			printf("*** ERROR: Bone access not allowed (entity %i:%s)\n", index, GetClassname());
			lastWarning = Interfaces::Globals->realtime;
		}
	}
#endif

	//if (GetSequence() == -1)
	//	return false;

	CBoneAccessor* m_BoneAccessor = GetBoneAccessor();
	CStudioHdr* hdr = GetModelPtr();
	if (hdr && hdr->_m_pStudioHdr)
		hdr->_m_pStudioHdr->eyeposition = vecZero;

	//if (!hdr || !SequencesAvailable())
	//	return false;

	AddEFlag(EFL_SETTING_UP_BONES);

	DWORD m_pIk = (DWORD)GetIK();

	// only allocate an ik block if the npc can use it
	if (!m_pIk && hdr->_m_pStudioHdr->numikchains > 0 && !(GetEntClientFlags() & ENTCLIENTFLAG_DONTUSEIK))
	{
		m_pIk = Wrap_CreateIK();
		SetIK(m_pIk);
	}

	Vector pos[MAXSTUDIOBONES];
	QuaternionAligned q[MAXSTUDIOBONES];

	if (m_pIk)
	{
		if (Teleported(this) || GetEffects() & EF_NOINTERP)
			ClearTargets(m_pIk);

		Wrap_IKInit(m_pIk, hdr, GetAbsAngles(), *GetAbsOrigin(), currentTime, Interfaces::Globals->framecount, boneMask);
	}

	int raxerBoneMask = BONE_USED_BY_HITBOX;
	if (boneMask & BONE_USED_BY_VERTEX_MASK)
		raxerBoneMask |= BONE_USED_BY_VERTEX_MASK;
	if (boneMask & BONE_USED_BY_BONE_MERGE)
		raxerBoneMask |= BONE_USED_BY_BONE_MERGE; //eye candy

	//Remove lean animation from lean_root
	//raxerBoneMask &= ~(BONE_USED_BY_BONE_MERGE | BONE_USED_BY_WEAPON);

	//Fixes StandardBlendingRules not calling AccumulateLayers
#if 1
	GetSkeleton(hdr, pos, q, raxerBoneMask);
#else
	bIsSettingUpBones = true;
	StandardBlendingRules(hdr, pos, q, currentTime, raxerBoneMask);
	bIsSettingUpBones = false;
#endif

	byte computed[32] = { 0 };
	bool _DoFancyFeet = false;

	// don't calculate IK on ragdolls
	if (GetIK() && !IsRagdoll())
	{
		UpdateIKLocks(currentTime);
		Wrap_UpdateTargets(GetIK(), pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
		CalculateIKLocks(currentTime);
		Wrap_SolveDependencies(GetIK(), pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
		//if (!IsLocalPlayer())		//dylan added 
		{
			if ((IsPlayer() && (boneMask & BONE_USED_BY_VERTEX_LOD0)) || (IsLocalPlayer() && LocalPlayer.RunningFakeAngleBones))
			{
				DoExtraBoneProcessing(hdr, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed, GetIK());
				_DoFancyFeet = true;
			}
		}
	}

	if (!_DoFancyFeet)
	{
		Studio_BuildMatrices(hdr, GetAbsAngles(), *GetAbsOrigin(), pos, q, -1, 1.0f, m_BoneAccessor->GetBoneArrayForWrite(), boneMask);
	}
	else
    {
	   __declspec(align(16)) matrix3x4_t parentTransform;
	   AngleMatrix(*GetLocalAnglesVMT(), *GetLocalOriginVMT(), parentTransform);
	   BuildTransformations(hdr, pos, q, parentTransform, boneMask, computed);
    }

	RemoveEFlag(EFL_SETTING_UP_BONES);
	ControlMouth(hdr);

	if (AllowSetupBonesToUpdateAttachments && ((boneMask & BONE_USED_BY_ATTACHMENT) || IsLocalPlayer()))
		SetupBones_AttachmentHelper(hdr);

	return true;
}

bool CBaseEntity::SetupBonesRebuilt(matrix3x4_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	//if (!LocalPlayer.RunningFakeAngleBones)
		AddEffect(EF_NOINTERP);

//#define USE_SERVER_SETUP_BONES
#ifdef USE_SERVER_SETUP_BONES
	auto cache = GetBoneCache_Server(currentTime);
	RemoveEffect(EF_NOINTERP);
	if (cache)
	{
		if (pBoneToWorldOut && nMaxBones != -1)
		{
			auto thecache = GetCachedBoneData();
			memcpy(pBoneToWorldOut, thecache->Base(), sizeof(matrix3x4_t) * max(0, min(thecache->Count(), nMaxBones)));
		}
		return true;
	}
	return false;
#endif

#ifdef _DEBUG
	if (!IsBoneAccessAllowed())
	{
		static float lastWarning = 0.0f;

		// Prevent spammage!!!
		if (Interfaces::Globals->realtime >= lastWarning + 1.0f)
		{
			AllocateConsole();
			printf("*** ERROR: Bone access not allowed (entity %i:%s)\n", index, GetClassname());
			lastWarning = Interfaces::Globals->realtime;
		}
	}
#endif

	boneMask = boneMask | 0x80000; // HACK HACK - this is a temp fix until we have accessors for bones to find out where problems are.

	if (GetSequence() == -1)
	{
		RemoveEffect(EF_NOINTERP);
		return false;
#ifdef _DEBUG
		DebugBreak();
#endif
	}

	if (boneMask == -1)
	{
		boneMask = GetPreviousBoneMask();
	}

	// We should get rid of this someday when we have solutions for the odd cases where a bone doesn't
	// get setup and its transform is asked for later.
	if (!cl_setupallbones)
	{
		DecStr(cl_setupallbonesstr, 16);
		cl_setupallbones = Interfaces::Cvar->FindVar(cl_setupallbonesstr);
		EncStr(cl_setupallbonesstr, 16);
		delete[] cl_setupallbonesstr;
	}

	if (cl_setupallbones->GetInt())
	{
		boneMask |= BONE_USED_BY_ANYTHING;
	}

	// Set up all bones if recording, too
	if (IsToolRecording())
	{
		boneMask |= BONE_USED_BY_ANYTHING;
	}

	CThreadFastMutex* pBoneSetupLock = GetBoneSetupLock();

	if (*g_bInThreadedBoneSetup)
	{
		if (!pBoneSetupLock->TryLock())
		{
			// someone else is handling
#ifdef _DEBUG
			DebugBreak();
#endif
			RemoveEffect(EF_NOINTERP);
			return false;
		}
		// else, we have the lock
	}
	else
	{
		pBoneSetupLock->Lock();
	}

	// If we're setting up LOD N, we have set up all lower LODs also
	// because lower LODs always use subsets of the bones of higher LODs.
	int nLOD  = 0;
	int nMask = BONE_USED_BY_VERTEX_LOD0;
	for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
	{
		if (boneMask & nMask)
			break;
	}
	for (; nLOD < MAX_NUM_LODS; ++nLOD, nMask <<= 1)
	{
		boneMask |= nMask;
	}

#ifdef _DEBUG
	if (!pBoneSetupLock->TryLock())
	{
		DebugBreak();
		printf("Contested bone setup in frame %d!\n", Interfaces::Globals->framecount);
	}
	else
	{
		pBoneSetupLock->Unlock();
	}
#endif

	// A bit of a hack, but this way when in prediction we use the "correct" gpGlobals->curtime -- rather than the
	// one that the player artificially advances
	using InPredictionFn = bool(__thiscall*)(DWORD);

	if (GetPredictable() && GetVFunc< InPredictionFn >((PVOID)Interfaces::Prediction, 14)((DWORD)Interfaces::Prediction)) //Interfaces::Prediction->InPrediction())
	{
		currentTime = Interfaces::Prediction->m_SavedVars.curtime; //Interfaces::Prediction->GetSavedTime();
	}

	CBoneAccessor* m_BoneAccessor = GetBoneAccessor();

	if (GetMostRecentModelBoneCounter() != *g_iModelBoneCounter /*&& s_bEnableNewBoneSetupRequest*/)
	{
		// Clear out which bones we've touched this frame if this is
		// the first time we've seen this object this frame.
		// BUGBUG: Time can go backward due to prediction, catch that here until a better solution is found
		float flLastBoneSetupTime = GetLastBoneSetupTime();
		//LastBoneChangedTime return FLT_MAX
		if (LastBoneChangedTime() >= flLastBoneSetupTime || currentTime < flLastBoneSetupTime)
		{
			m_BoneAccessor->SetReadableBones(0); //2694
			m_BoneAccessor->SetWritableBones(0); //2698
			SetLastBoneSetupTime(currentTime);
		}
		SetPreviousBoneMask(GetAccumulatedBoneMask());
		SetAccumulatedBoneMask(0);
		LockStudioHdrIfNoModel();
		CStudioHdr* ptr = GetModelPtr();
		if (ptr && ptr->_m_pStudioHdr)
		{
			ptr->_m_pStudioHdr->eyeposition = vecZero;
		}
	}

	MarkForThreadedBoneSetup();

	// Keep track of everything asked for over the entire frame
	// But not those things asked for during bone setup
	//	if ( !g_bInThreadedBoneSetup )
	{
		SetAccumulatedBoneMask(GetAccumulatedBoneMask() | boneMask);
	}

	// Make sure that we know that we've already calculated some bone stuff this time around.
	SetMostRecentModelBoneCounter(*g_iModelBoneCounter);

	// Have we cached off all bones meeting the flag set?
	if ((m_BoneAccessor->GetReadableBones() & boneMask) != boneMask)
	{
		LockStudioHdrIfNoModel();

		CStudioHdr* hdr = GetModelPtr();
		if (!hdr || !SequencesAvailable())
		{
			pBoneSetupLock->Unlock();
#ifdef _DEBUG
			DebugBreak();
#endif
			RemoveEffect(EF_NOINTERP);
			return false;
		}

		DWORD EntityPlus4 = (DWORD)((DWORD)this + 4);

		// Setup our transform based on render angles and origin.
		Vector LocalOrigin = *GetLocalOriginVMT();
		QAngle LocalAngles = *GetLocalAnglesVMT();

		__declspec(align(16)) matrix3x4_t parentTransform;
		AngleMatrix(LocalAngles, LocalOrigin, parentTransform);

		// Load the boneMask with the total of what was asked for last frame.
		boneMask |= GetPreviousBoneMask();

		// Allow access to the bones we're setting up so we don't get asserts in here.
		int oldReadableBones = m_BoneAccessor->GetReadableBones();
		int oldWritableBones = m_BoneAccessor->GetWritableBones();
		int newWritableBones = oldReadableBones | boneMask;
		m_BoneAccessor->SetWritableBones(newWritableBones);
		m_BoneAccessor->SetReadableBones(newWritableBones);

		if (hdr->_m_pStudioHdr->flags & STUDIOHDR_FLAGS_STATIC_PROP)
		{
			MatrixCopy(parentTransform, m_BoneAccessor->GetBoneForWrite(0));
		}
		else
		{
			// This is necessary because it's possible that CalculateIKLocks will trigger our move children
			// to call GetAbsOrigin(), and they'll use our OLD bone transforms to get their attachments
			// since we're right in the middle of setting up our new transforms.
			//
			// Setting this flag forces move children to keep their abs transform invalidated.
			AddEFlag(EFL_SETTING_UP_BONES);

			DWORD m_pIk = (DWORD)GetIK();

			// only allocate an ik block if the npc can use i
			if (!m_pIk && hdr->_m_pStudioHdr->numikchains > 0 && !(GetEntClientFlags() & ENTCLIENTFLAG_DONTUSEIK))
			{
				m_pIk = Wrap_CreateIK();
				SetIK(m_pIk);
			}

			Vector pos[MAXSTUDIOBONES];
			QuaternionAligned q[MAXSTUDIOBONES];
#ifdef _DEBUG
			// Having these uninitialized means that some bugs are very hard
			// to reproduce. A memset of 0xFF is a simple way of getting NaNs.
			memset(pos, 0xFF, sizeof(pos));
			memset(q, 0xFF, sizeof(q));
#endif

			//int bonesMaskNeedRecalc = boneMask | oldReadableBones; // Hack to always recalc bones, to fix the arm jitter in the new CS player anims until Ken makes the real fix

			if (m_pIk)
			{
				if (Teleported(this) || GetEffects() & EF_NOINTERP)
				{
					ClearTargets(m_pIk);
				}

				Vector LocalOrigin2 = *GetLocalOriginVMT();
				QAngle LocalAngles2 = *GetLocalAnglesVMT();

				Wrap_IKInit(m_pIk, hdr, LocalAngles2, LocalOrigin2, currentTime, Interfaces::Globals->framecount, boneMask);
			}

			// Let pose debugger know that we are blending
			//g_pPoseDebugger->StartBlending(this, hdr);

			bool bSkipAnimFrame = !AllowShouldSkipAnimationFrame ? false : ShouldSkipAnimationFrame(this);

			if (bSkipAnimFrame)
			{
				memcpy(&pos[0], (const void*)(EntityPlus4 + 0xA68), sizeof(Vector) * hdr->_m_pStudioHdr->numbones);
				memcpy(&q[0], (const void*)(EntityPlus4 + 0x166C), sizeof(Quaternion) * hdr->_m_pStudioHdr->numbones);
				boneMask = m_BoneAccessor->GetWritableBones();
			}
			else
			{
				//8B  97  ??  ??  ??  ??  8B  44  24  14  8B  C8  83  FA  FF  74  04 + 2 dec
				int occlusionMask = *(int*)(EntityPlus4 + 2592);
				int newBoneMask   = boneMask;
				int fixedBoneMask = newBoneMask;

				//Dylan added this if check
				if (!IsPlayer())
				{
					//Standard game behavior always
					if (occlusionMask != -1)
						newBoneMask = boneMask & occlusionMask;

					newBoneMask |= 0x80000;

					//dylan added
					fixedBoneMask = newBoneMask;
				}
				else
				{
					//Raxer's bone fixes
					fixedBoneMask = BONE_USED_BY_HITBOX | BONE_USED_BY_VERTEX_MASK;
					if (IsLocalPlayer())
						fixedBoneMask |= BONE_USED_BY_BONE_MERGE; //eye candy
				}
#if 0
				if (cl_countbones.GetBool())
				{
					int v117 = 0;
					DWORD a9 = *(DWORD*)hdr;
					if (*(DWORD *)(*(DWORD *)hdr + 156) > 0)
					{
						DWORD *v118 = *(DWORD **)((DWORD)hdr + 48);
						DWORD v119 = g_nNumBonesSetupBlendingRulesOnlyTemp;
						do
						{
							if (tempBoneMask & *v118)
								g_nNumBonesSetupBlendingRulesOnlyTemp = ++v119;
							++v117;
							++v118;
						} while (v117 < *(DWORD *)(a9 + 156));
					}
				}
#endif
				//float time = Interfaces::Globals->curtime;
				//float ft = Interfaces::Globals->frametime;
				//Interfaces::Globals->curtime = FLT_EPSILON;
				//Interfaces::Globals->frametime = FLT_EPSILON;

				//Fixes StandardBlendingRules not calling AccumulateLayers
				bIsSettingUpBones = true;

				StandardBlendingRules(hdr, pos, q, currentTime, fixedBoneMask);

				bIsSettingUpBones = false;

				//Interfaces::Globals->curtime = time;
				//Interfaces::Globals->frametime = ft;

				if (IsPlayer())
				{
					if (newBoneMask != boneMask)
					{
						studiohdr_t* hdrt = hdr->_m_pStudioHdr;
						if (hdrt->numbones > 0)
						{
							//8D  8F  ??  ??  ??  ??  8D  84  24  ??  ??  ??  ??  89  4C  24  1C  89  44  24  10  8D  97  ??  ??  ??  ??  8B  46  30
							intptr_t* pFlags		 = *(intptr_t**)((DWORD)hdr + 48);
							Vector* pEntPos			 = (Vector*)((DWORD)EntityPlus4 + (2672 - sizeof(float) * 2));
							QuaternionAligned* pEntQ = (QuaternionAligned*)((DWORD)EntityPlus4 + 5740);

							for (int i = 0; i < hdrt->numbones; i++)
							{
								if (!(pFlags[i] & 0x80300))
								{
									pos[i] = pEntPos[i];
									q[i]   = pEntQ[i];
								}
							}
						}
					}
				}

#if 0
				//this shit seems to draw skeletons or something
				if (cl_countbones.GetBool())
				{
					DWORD tmp = 0;
					DWORD v126 = 0;
					do
					{
						DWORD v127 = tmp;
						float tmpflt;
						if (v158 & *(DWORD *)(*(DWORD *)(hdr + 48) + 4 * v126))
						{
							v128 = *(DWORD *)(*(DWORD *)(hdr + 68) + 4 * v126);
							if (v128 >= 0)
							{
								v129 = *(DWORD *)(renderable + 9876);
								v130 = 6 * v128;
								a18 = *(DWORD *)(v129 + tmp + 12);
								tmpflt = *(float *)(v129 + tmp + 28);
								a20 = *(DWORD *)(v129 + tmp + 44);
								v161 = *(DWORD *)(v129 + 8 * v130 + 12);
								a16 = *(float *)(v129 + 8 * v130 + 28);
								v131 = *(DWORD *)(v129 + 8 * v130 + 44);
								v132 = *(DWORD *)(renderable - 4);
								a17 = v131;
								v133 = (unsigned __int8)(*(int(__stdcall **)(int))(v132 + 608))(vars0) == 0;
								vars0 = 0;
								v134 = *(DWORD *)dword_219DAED8;
								if (v133)
								{
									a27 = a16 + 2.0;
									a28 = a17;
									a29 = a18;
									a30 = tmpflt + 2.0;
									a31 = a20;
									*(float *)&m_pIK = a21;
									(*(void(__stdcall **)(float *, float *, signed int, signed int, signed int, signed int))(v134 + 20))(
										&a30,
										&a27,
										200,
										0,
										255,
										1);
			}
								else
								{
									(*(void(__stdcall **)(float *, float *, signed int, signed int, signed int, signed int))(v134 + 20))(
										&tmpflt,
										&a16,
										0,
										255,
										255,
										1);
								}
								v127 = tmp;
								v126 = boneMask;
							}
						}
						boneMask = ++v126;
						tmp = v127 + 48;
					} while (v126 < *(DWORD *)(*(DWORD *)hdr + 156));
				}
#endif
				SetLastSetupBonesFrameCount(Interfaces::Globals->framecount);
			}

			byte computed[32] = { 0 };

			// don't calculate IK on ragdolls
			if (GetIK() && !IsRagdoll())
			{
				UpdateIKLocks(currentTime);
				Wrap_UpdateTargets(GetIK(), pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
				CalculateIKLocks(currentTime);
				Wrap_SolveDependencies(GetIK(), pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed);
				if (IsPlayer() && (boneMask & BONE_USED_BY_VERTEX_LOD0))
				{
					DoExtraBoneProcessing(hdr, pos, q, m_BoneAccessor->GetBoneArrayForWrite(), computed, GetIK());
				}
			}

			BuildTransformations(hdr, pos, q, parentTransform, boneMask, computed);

#if 0
			if (cl_countbones.GetBool())
			{
				v159 = *(_DWORD *)LODWORD(hdr3);
				if (*(_DWORD *)(*(_DWORD *)LODWORD(hdr3) + 156) > 0)
				{
					v141 = *(_DWORD **)(LODWORD(hdr3) + 48);
					v142 = 0;
					v143 = dword_1F91977C;
					do
					{
						if (boneMaskArgumenta & *v141)
							dword_1F91977C = ++v143;
						++v142;
						++v141;
					} while (v142 < *(_DWORD *)(v159 + 156));
					EntPlus4 = v158;
					hdr3 = a9;
				}
			}
#endif

#if 0
			// Draw skeleton?
			//if (enable_skeleton_draw.GetBool())
			if (((int(__thiscall *)(int(__stdcall ***)(char)))off_1D4F5B00[13])(&off_1D4F5B00))
				sub_1CBFC000(LODWORD(hdr3), boneMaskArgumenta);
#endif

			RemoveEFlag(EFL_SETTING_UP_BONES);

			ControlMouth(hdr);

			if (!bSkipAnimFrame)
			{
				memcpy((void*)(EntityPlus4 + 0xA68), &pos[0], sizeof(Vector) * hdr->_m_pStudioHdr->numbones);
				memcpy((void*)(EntityPlus4 + 0x166C), &q[0], sizeof(Quaternion) * hdr->_m_pStudioHdr->numbones);
			}
		}

		if (AllowSetupBonesToUpdateAttachments) //Custom added by dylan
		{
			if (!(oldReadableBones & BONE_USED_BY_ATTACHMENT) && ((boneMask & BONE_USED_BY_ATTACHMENT) || IsLocalPlayer()))
				SetupBones_AttachmentHelper(hdr);
		}
	}

	// Do they want to get at the bone transforms? If it's just making sure an aiment has
	// its bones setup, it doesn't need the transforms yet.
	if (pBoneToWorldOut)
	{
		CUtlVectorSimple* CachedBoneData = GetCachedBoneData();
		if ((unsigned int)nMaxBones >= CachedBoneData->count)
		{
			memcpy((void*)pBoneToWorldOut, CachedBoneData->Base(), sizeof(matrix3x4_t) * CachedBoneData->Count());
		}
		else
		{
#ifdef _DEBUG
			AllocateConsole();
			printf("SetupBones: invalid bone array size(%d - needs %d)\n", nMaxBones, CachedBoneData->Count());
			DebugBreak();
#endif
			pBoneSetupLock->Unlock();
			RemoveEffect(EF_NOINTERP);
			return false;
		}
	}

	pBoneSetupLock->Unlock();
	RemoveEffect(EF_NOINTERP);
	return true;
}

bool CBaseEntity::SetupBones(matrix3x4_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	START_PROFILING
	bool ret;
	if (IsPlayer())
	{
		if (boneMask == BONE_USED_BY_ANYTHING)
			ReevaluateAnimLOD();
		AllowShouldSkipAnimationFrame = boneMask != BONE_USED_BY_ANYTHING;
		ret							  = SetupBonesRebuilt(pBoneToWorldOut, nMaxBones, boneMask, currentTime);
		AllowShouldSkipAnimationFrame = true;
		END_PROFILING
		return ret;
	}
	else
	{
		AllowShouldSkipAnimationFrame = boneMask != BONE_USED_BY_ANYTHING;
		//ReevaluateAnimLOD();
		ret							  = SetupBonesRebuilt(pBoneToWorldOut, nMaxBones, boneMask, currentTime);
		AllowShouldSkipAnimationFrame = true;
	}
	END_PROFILING
	return ret;

#if 0
		START_PROFILING
		//*(int*)((DWORD)this + AdrOf_m_nWritableBones) = 0;
		//WriteInt((uintptr_t)this + (AdrOf_m_nWritableBones - 4), 0); //Readable bones
		//*(int*)((DWORD)this + AdrOf_m_iDidCheckForOcclusion) = reinterpret_cast<int*>(AdrOf_m_dwOcclusionArray)[1];

		AllowShouldSkipAnimationFrame = false;
		ret = GetVFunc<SetupBonesFn>((void*)((uintptr_t)this + 0x4), 13)((void*)((uintptr_t)this + 0x4), pBoneToWorldOut, nMaxBones, boneMask, currentTime);
		AllowShouldSkipAnimationFrame = true;
		END_PROFILING
		return ret;
#if 0
		__asm
		{
			mov edi, this
			lea ecx, dword ptr ds : [edi + 0x4]
			mov edx, dword ptr ds : [ecx]
			push currentTime
			push boneMask
			push nMaxBones
			push pBoneToWorldOut
			call dword ptr ds : [edx + 0x34]
		}
#endif
#endif
}

void CBaseEntity::LockStudioHdrIfNoModel()
{
	if (!GetModelPtr() && GetModel())
	{
		oLockStudioHdr(this);
	}
}

float CBaseEntity::SequenceDuration(int iSequence)
{
	float retval;
	((float(__thiscall*)(CBaseEntity*, int))AdrOf_SequenceDuration)(this, iSequence);
	__asm movss retval, xmm0;
	return retval;
}

float CBaseEntity::GetSequenceCycleRate(int iSequence)
{
	//crashes
	return StaticOffsets.GetVFuncByType<float(__thiscall*)(CBaseEntity*, CStudioHdr*, int)>(_GetSequenceCycleRate, this)(this, GetModelPtr(), iSequence);
}

float CBaseEntity::GetSequenceCycleRate_Server(int iSequence)
{
	float t = SequenceDuration(iSequence);

	if (t > 0.0f)
	{
		return 1.0f / t;
	}
	else
	{
		return 1.0f / 0.1f;
	}
}

float CBaseEntity::GetLayerSequenceCycleRate(C_AnimationLayer* layer, int sequence)
{
	return StaticOffsets.GetVFuncByType<float(__thiscall*)(CBaseEntity*, C_AnimationLayer*, int)>(_GetLayerSequenceCycleRate, this)(this, layer, sequence);
}

float CBaseEntity::GetSequenceMoveDist(CStudioHdr *pStudioHdr, int iSequence)
{
	Vector				vecReturn;

	GetSequenceLinearMotion(pStudioHdr, iSequence, (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter), &vecReturn);

	return vecReturn.Length();
}

void CBaseEntity::GetSequenceLinearMotion(CStudioHdr *pStudioHdr, int iSequence, float* poseParameters, Vector* pVec)
{
	StaticOffsets.GetOffsetValueByType<void(__fastcall*)(CStudioHdr*, int, float*, Vector*)>(_GetSequenceLinearMotion)(pStudioHdr, iSequence, poseParameters, pVec);
	__asm add esp, 8
}

void CBaseEntity::GetSequenceLinearMotion(int iSequence, Vector *pVec)
{
	GetSequenceLinearMotion(GetModelPtr(), iSequence, (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter), pVec);
}

bool CBaseEntity::IsSequenceLooping(int iSequence)
{
	return (GetSequenceFlags(iSequence) & 0x0001) != 0; //STUDIO_LOOPING
}

CBaseHandle CBaseEntity::GetRefEHandle()
{
	//int(__thiscall *v21)(DWORD*);
	//v21 = *(int(__thiscall **)(DWORD*))(*(DWORD*)(DWORD)this + 8);
	//int* v22 = (int *)v21((DWORD*)this);
	//return (CBaseHandle)*v22;
	return GetClientUnknown()->GetRefEHandle();
}

void CBaseEntity::SetRefEHandle(CBaseHandle handle)
{
	GetClientUnknown()->SetRefEHandle(handle);
	//*(unsigned long*)((DWORD)this + m_RefEHandle) = handle.ToUnsignedLong();
}

unsigned long CBaseEntity::GetRefEHandleDirect()
{
	return GetRefEHandle().ToUnsignedLong();
	/*return *(unsigned long*)((DWORD)this + m_RefEHandle);*/
};

void CBaseEntity::SetRefEHandle(unsigned long handle)
{
	SetRefEHandle((CBaseHandle)handle);
	/**(unsigned long*)((DWORD)this + m_RefEHandle) = handle;*/
};

const Vector& CBaseEntity::GetVelocity()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecVelocity);
}

void CBaseEntity::SetVelocity(const Vector& velocity)
{
	*(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecVelocity) = velocity;
}

Vector* CBaseEntity::GetAbsVelocity()
{
	//Assert(s_bAbsQueriesValid);
#ifdef _DEBUG
	if (!*s_bAbsQueriesValid)
	{
		printf("Assertion failed in GetAbsVelocity: s_bAbsQueriesValid is false\n");
		DebugBreak();
	}
#endif
	CalcAbsoluteVelocity();
	return StaticOffsets.GetOffsetValueByType< Vector* >(_AbsVelocityDirect, this);
}

void CBaseEntity::GetAbsVelocity(Vector& vel)
{
	vel = *GetAbsVelocity();
}

Vector CBaseEntity::GetAbsVelocityDirect()
{
	return *StaticOffsets.GetOffsetValueByType< Vector* >(_AbsVelocityDirect, this);
}

void CBaseEntity::SetAbsVelocityDirect(Vector& vel)
{
	*StaticOffsets.GetOffsetValueByType< Vector* >(_AbsVelocityDirect, this) = vel;
}

void CBaseEntity::SetAbsVelocity(Vector& velocity)
{
	((void(__thiscall*)(CBaseEntity*, Vector*))AdrOf_SetAbsVelocity)(this, &velocity);
}

void CBaseEntity::CalcAbsoluteVelocity()
{
	((void(__thiscall*)(CBaseEntity*))AdrOf_CalcAbsoluteVelocity)(this);
}

void CBaseEntity::EstimateAbsVelocity(const Vector& vel)
{
	typedef void(__thiscall * OriginalFn)(CBaseEntity*, const Vector&);
	StaticOffsets.GetVFuncByType< OriginalFn >(_EstimateAbsVelocity, this)(this, vel);
}

Vector CBaseEntity::GetBaseVelocity()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecBaseVelocity);
}

void CBaseEntity::SetBaseVelocity(Vector& velocity)
{
	*(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecBaseVelocity) = velocity;
}

ICollideable* CBaseEntity::GetCollideable()
{
	return (ICollideable*)((DWORD)this + g_NetworkedVariables.Offsets.m_Collision);
}

void CBaseEntity::GetPlayerInfo(player_info_t* dest)
{
	Interfaces::EngineClient->GetPlayerInfo(index, dest);
}

std::string CBaseEntity::GetName()
{
	player_info_t info;
	GetPlayerInfo(&info);
	return std::string(info.name);
}

void CBaseEntity::GetSteamID(char* dest)
{
	player_info_t info;
	GetPlayerInfo(&info);
	memcpy(dest, &info.guid, 33);
}

CBaseCombatWeapon* CBaseEntity::GetWeapon()
{
#ifdef _DEBUG
	static DWORD returnadr;
	returnadr = (DWORD)_ReturnAddress();
#endif
	DWORD weaponData = *(DWORD*)((DWORD)this + g_NetworkedVariables.Offsets.m_hActiveWeapon);
	return (CBaseCombatWeapon*)Interfaces::ClientEntList->GetBaseEntityFromHandle(weaponData);
}

void CBaseEntity::SetWeaponHandle(EHANDLE handle)
{
	*(EHANDLE*)((DWORD)this + g_NetworkedVariables.Offsets.m_hActiveWeapon) = handle;
}

EHANDLE CBaseEntity::GetWeaponHandle()
{
	return *(EHANDLE*)((DWORD)this + g_NetworkedVariables.Offsets.m_hActiveWeapon);
}

void CBaseEntity::Precache()
{
	StaticOffsets.GetVFuncByType<void(__thiscall*)(CBaseEntity*)>(_Precache, this)(this);
}

void CBaseEntity::Spawn()
{
	StaticOffsets.GetVFuncByType<void(__thiscall*)(CBaseEntity*)>(_Spawn, this)(this);
}

ClientClass* CBaseEntity::GetClientClass()
{
	return GetClientNetworkable()->GetClientClass();
	//PVOID pNetworkable = (PVOID)((DWORD)(this) + 0x8);
	//typedef ClientClass*(__thiscall* OriginalFn)(PVOID);
	//return GetVFunc<OriginalFn>(pNetworkable, 2)(pNetworkable);
}

int CBaseEntity::GetCollisionGroup()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_CollisionGroup); //*(int*)((DWORD)this + m_CollisionGroup);
}

void CBaseEntity::SetCollisionGroup(int group)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_CollisionGroup) = group;
}

CBaseAnimating* CBaseEntity::GetBaseAnimating(void)
{
	int adr = *(DWORD*)((uintptr_t)this);
	if (adr)
	{
		return ((CBaseAnimating * (__thiscall*)(CBaseEntity * me)) * (DWORD*)(adr + StaticOffsets.GetOffsetValue(_GetBaseAnimating)))(this);
	}
	return NULL;
}

void CBaseEntity::InvalidateBoneCache()
{
	bool orig					  = *s_bEnableInvalidateBoneCache;
	*s_bEnableInvalidateBoneCache = true;
	((CBaseAnimating * (__fastcall*)(CBaseEntity*)) AdrOfInvalidateBoneCache)(this);
	*s_bEnableInvalidateBoneCache = orig;
}

HANDLE CBaseEntity::GetObserverTargetDirect()
{
	return (HANDLE) * (DWORD*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_hObserverTarget);
}

CBaseEntity* CBaseEntity::GetObserverTarget()
{
	//8B  4E  ??  8B  01  8B  80  ??  ??  00  00  FF  D0  8B  F8 + 7 dec
	typedef CBaseEntity*(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_m_dwObserverTargetVMT, this)(this);
}

BOOLEAN CBaseEntity::IsPlayer()
{
	typedef BOOLEAN(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_IsPlayer, this)(this);
}

CUserCmd* CBaseEntity::GetLastUserCommand()
{
	return (CUserCmd*)*(DWORD*)(*(DWORD*)((uintptr_t)this) + m_dwGetLastUserCommand);
}

QAngle& CBaseEntity::GetAngleRotation()
{
	return *(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_angRotation);
}

void CBaseEntity::SetAngleRotation(QAngle& rot)
{
	*(QAngle*)((DWORD)this + g_NetworkedVariables.Offsets.m_angRotation) = rot;
}

QAngle& CBaseEntity::GetLocalAngles()
{
	return *StaticOffsets.GetOffsetValueByType< QAngle* >(_LocalAngles, this);
}

QAngle CBaseEntity::GetOldAngRotation()
{
	return *(QAngle*)((DWORD)this + m_vecOldAngRotation);
}

void CBaseEntity::SetOldAngRotation(QAngle& rot)
{
	*(QAngle*)((DWORD)this + m_vecOldAngRotation) = rot;
}

void CBaseEntity::SetAbsAngles(const QAngle& rot)
{
	((void(__thiscall*)(CBaseEntity*, const QAngle&))AdrOf_SetAbsAngles)(this, rot);
}

Vector CBaseEntity::GetMins()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecMins);
}

void CBaseEntity::SetMins(Vector& mins)
{
	*(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecMins) = mins;
}

Vector CBaseEntity::GetMaxs()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecMaxs);
}

void CBaseEntity::SetMaxs(Vector& maxs)
{
	*(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecMaxs) = maxs;
}

Vector& CBaseEntity::GetPlayerMins()
{
	if (GetObserverMode())
		return (*g_pGameRules)->GetViewVectors()->m_vObsHullMin;

	if (HasFlag(FL_DUCKING))
		return (*g_pGameRules)->GetViewVectors()->m_vDuckHullMin;

	return (*g_pGameRules)->GetViewVectors()->m_vHullMin;
}

Vector& CBaseEntity::GetPlayerMaxs()
{
	if (GetObserverMode())
		return (*g_pGameRules)->GetViewVectors()->m_vObsHullMax;

	if (HasFlag(FL_DUCKING))
		return (*g_pGameRules)->GetViewVectors()->m_vDuckHullMax;

	return (*g_pGameRules)->GetViewVectors()->m_vHullMax;
}

unsigned int CBaseEntity::PlayerSolidMask(bool brushOnly, CBaseEntity* testPlayer)
{
	bool isBot = true;

	if (this != nullptr)
	{
		if (!IsBot())
			isBot = false;
	}

	unsigned int v9 = 0x1400B;

	if (!brushOnly)
		// v9 = (*(int (**)(void))(**(_DWORD **)(v2 + 4) + 592))();
		// Calls CBasePlayer's 148th function
		v9 = PhysicsSolidMaskForEntity();

	unsigned int result = v9 | 0x20000;

	if (!isBot)
		result = v9;

	return result;
}

void CBaseEntity::TracePlayerBBox(const Vector& rayStart, const Vector& rayEnd, int fMask, int collisionGroup, trace_t& tracePtr)
{
	Ray_t ray;

	Vector playerMaxs = GetPlayerMaxs();
	Vector playerMins = GetPlayerMins();

	ray.Init(rayStart, rayEnd, playerMins, playerMaxs);

	CTraceFilterSimple filter;
	filter.SetPassEntity((IHandleEntity*)this);
	filter.SetCollisionGroup(collisionGroup);

	Interfaces::EngineTrace->TraceRay(ray, fMask, &filter, &tracePtr);
}

//-----------------------------------------------------------------------------
// Traces the player's collision bounds in quadrants, looking for a plane that
// can be stood upon (normal's z >= 0.7f).  Regardless of success or failure,
// replace the fraction and endpos with the original ones, so we don't try to
// move the player down to the new floor and get stuck on a leaning wall that
// the original trace hit first.
//-----------------------------------------------------------------------------
void CBaseEntity::TracePlayerBBoxForGround(const Vector& start, const Vector& end, const Vector& minsSrc, const Vector& maxsSrc, IHandleEntity* player, unsigned int fMask, int collisionGroup, trace_t& pm)
{
	Ray_t ray;
	Vector mins, maxs;

	float fraction = pm.fraction;
	Vector endpos = pm.endpos;

	// Check the -x, -y quadrant
	mins = minsSrc;
	maxs.Init(min(0, maxsSrc.x), min(0, maxsSrc.y), maxsSrc.z);
	ray.Init(start, end, mins, maxs);
	UTIL_TraceRay(ray, fMask, player, collisionGroup, &pm);
	if (pm.m_pEnt && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the +x, +y quadrant
	mins.Init(max(0, minsSrc.x), max(0, minsSrc.y), minsSrc.z);
	maxs = maxsSrc;
	ray.Init(start, end, mins, maxs);
	UTIL_TraceRay(ray, fMask, player, collisionGroup, &pm);
	if (pm.m_pEnt && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the -x, +y quadrant
	mins.Init(minsSrc.x, max(0, minsSrc.y), minsSrc.z);
	maxs.Init(min(0, maxsSrc.x), maxsSrc.y, maxsSrc.z);
	ray.Init(start, end, mins, maxs);
	UTIL_TraceRay(ray, fMask, player, collisionGroup, &pm);
	if (pm.m_pEnt && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the +x, -y quadrant
	mins.Init(max(0, minsSrc.x), minsSrc.y, minsSrc.z);
	maxs.Init(maxsSrc.x, min(0, maxsSrc.y), maxsSrc.z);
	ray.Init(start, end, mins, maxs);
	UTIL_TraceRay(ray, fMask, player, collisionGroup, &pm);
	if (pm.m_pEnt && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	pm.fraction = fraction;
	pm.endpos = endpos;
}

int CBaseEntity::GetSequence()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nSequence);
}

void CBaseEntity::SetSequence(int seq)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nSequence) = seq;
}

void CBaseEntity::SetSequenceVMT(int seq)
{
	StaticOffsets.GetVFuncByType<void(__thiscall*)(CBaseEntity*, int)>(_SetSequenceVMT, this)(this, seq);
}

float CBaseEntity::GetBoneController(int index)
{
	return *(float*)((DWORD)this + StaticOffsets.GetOffsetValue(_m_flEncodedController) + (sizeof(float) * index));
}

void CBaseEntity::SetBoneController(int index, float val)
{
	*(float*)((DWORD)this + StaticOffsets.GetOffsetValue(_m_flEncodedController) + (sizeof(float) * index)) = val;
}

void CBaseEntity::GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS])
{
	// interpolate two 0..1 encoded controllers to a single 0..1 controller
	const float* controller = (float*)((DWORD)this + StaticOffsets.GetOffsetValue(_m_flEncodedController));
	int i;
	for (i = 0; i < MAXSTUDIOBONECTRLS; i++)
	{
		controllers[i] = controller[i];
	}
}

void CBaseEntity::CopyBoneControllers(float controllers[MAXSTUDIOBONECTRLS])
{
	GetBoneControllers(controllers);
}

void CBaseEntity::WriteBoneControllers(float controllers[MAXSTUDIOBONECTRLS])
{
	// interpolate two 0..1 encoded controllers to a single 0..1 controller
	float* controller = (float*)((DWORD)this + StaticOffsets.GetOffsetValue(_m_flEncodedController));
	int i;
	for (i = 0; i < MAXSTUDIOBONECTRLS; i++)
	{
		controller[i] = controllers[i];
	}
}

float CBaseEntity::GetPoseParameter(int index)
{
	float retval;
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*, int)>(_GetPoseParameter)(this, index);	
	__asm movss retval, xmm0
	return retval;
}

void CBaseEntity::GetPoseParameterRange(int index, float& flMin, float& flMax)
{
	GetPoseParameterRangeGame(this, index, flMin, flMax);
}

float CBaseEntity::GetPoseParameterScaled(int index)
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter + (sizeof(float) * index));
}

float CBaseEntity::GetPoseParameterUnscaled(int index)
{
	return GetPoseParameter(index);
	//	float flMin, flMax;
	//	GetPoseParameterRangeGame(this, index, flMin, flMax);
	//	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter + (sizeof(float) * index)) * (flMax - flMin) + flMin;
}

float CBaseEntity::GetOldPoseParameterScaled(int index)
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flOldPoseParameter + (sizeof(float) * index));
}

float CBaseEntity::GetOldPoseParameterUnscaled(int index)
{
	float flMin, flMax;
	GetPoseParameterRangeGame(this, index, flMin, flMax);
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flOldPoseParameter + (sizeof(float) * index)) * (flMax - flMin) + flMin;
}

int CBaseEntity::LookupPoseParameter(CStudioHdr* hdr, char* name)
{
	return LookupPoseParameterGame(hdr, name);
}

//-----------------------------------------------------------------------------
// Purpose: converts a ranged pose parameter value into a 0..1 encoded value
// Output: 	ctlValue contains 0..1 encoding.
//			returns clamped ranged value
//-----------------------------------------------------------------------------

float Studio_SetPoseParameter(const CStudioHdr* pStudioHdr, int iParameter, float flValue, float& ctlValue)
{
	if (iParameter < 0 || iParameter >= pStudioHdr->GetNumPoseParameters())
	{
		return 0;
	}

	const mstudioposeparamdesc_t& PoseParam = ((CStudioHdr*)pStudioHdr)->pPoseParameter(iParameter);

	//Assert(IsFinite(flValue));

	if (PoseParam.loop)
	{
		float wrap  = (PoseParam.start + PoseParam.end) / 2.0 + PoseParam.loop / 2.0;
		float shift = PoseParam.loop - wrap;

		flValue = flValue - PoseParam.loop * floor((flValue + shift) / PoseParam.loop);
	}

	ctlValue = (flValue - PoseParam.start) / (PoseParam.end - PoseParam.start);

	if (ctlValue < 0)
		ctlValue = 0;
	if (ctlValue > 1)
		ctlValue = 1;

	//Assert(IsFinite(ctlValue));

	return ctlValue * (PoseParam.end - PoseParam.start) + PoseParam.start;
}

//C_BaseAnimating rebuild
void CBaseEntity::SetPoseParameterGame(CStudioHdr* hdr, float flValue, int index)
{
	if (hdr && index >= 0)
	{
		float flNewValue;
		Studio_SetPoseParameter(hdr, index, flValue, flNewValue);
		*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter + (sizeof(float) * index)) = flNewValue;
	}
}

void CBaseEntity::SetPoseParameter(int index, float p)
{
	float* pose = (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter + (sizeof(float) * index));

	float old = *pose;

	SetPoseParameterGame(GetModelPtr(), p, index);

#if 0
	float flMin, flMax;
	GetPoseParameterRangeGame(this, index, flMin, flMax);
	float scaledValue = (p - flMin) / (flMax - flMin);
#if 0
	DWORD dc = ReadInt(AdrOf_DataCacheSetPoseParmaeter);
	DWORD me = (DWORD)this;
	__asm {
		mov esi, dc
		mov ecx, esi
		mov eax, [esi]
		call [eax + 0x80]
		push index
		movss xmm2, scaledValue
		mov ecx, me
		call SetPoseParameterGame
		mov eax, [esi]
		mov ecx, esi
		call [eax + 0x84]
	}
#endif
	float* pose = (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter + (sizeof(float) * index));

	if (*pose != scaledValue)
	{
		*pose = scaledValue;
		InvalidatePhysicsRecursive(ANIMATION_CHANGED);
	}
#else
	if (old != *pose)
		InvalidatePhysicsRecursive(ANIMATION_CHANGED);
#endif
}

void CBaseEntity::SetPoseParameterScaled(int index, float p)
{
	float* pose = (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter + (sizeof(float) * index));
	if (*pose != p)
	{
		*pose = p;
		InvalidatePhysicsRecursive(ANIMATION_CHANGED);
	}
}

void CBaseEntity::CopyPoseParameters(float* dest)
{
	float* flPose = (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPoseParameter);
	memcpy(dest, flPose, sizeof(float) * MAX_CSGO_POSE_PARAMS);
}

int CBaseEntity::WritePoseParameters(float* src)
{
	int flags	  = 0;
	float* pflPose = (float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flPoseParameter);

	for (int i = 0; i < MAX_CSGO_POSE_PARAMS; i++)
	{
		const float flPose		 = *pflPose;
		const float flStoredPose = src[i];
		if (flPose != flStoredPose)
		{
			*pflPose = flStoredPose;
			flags |= ANIMATION_CHANGED;
		}
		pflPose++;
	}

	return flags;
}

void CBaseEntity::SetOldPoseParameter(int index, float p)
{
	float flMin, flMax;
	GetPoseParameterRangeGame(this, index, flMin, flMax);
	float scaledValue = (p - flMin) / (flMax - flMin);
#if 0
	DWORD dc = ReadInt(AdrOf_DataCacheSetPoseParmaeter);
	DWORD me = (DWORD)this;
	__asm {
		mov esi, dc
		mov ecx, esi
		mov eax, [esi]
		call[eax + 0x80]
		push index
		movss xmm2, scaledValue
		mov ecx, me
		call SetPoseParameterGame
		mov eax, [esi]
		mov ecx, esi
		call[eax + 0x84]
	}
#endif
	float* pose = (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flOldPoseParameter + (sizeof(float) * index));
	if (*pose != scaledValue)
	{
		*pose = scaledValue;
		InvalidatePhysicsRecursive(ANIMATION_CHANGED);
	}
}

void CBaseEntity::SetOldPoseParameterScaled(int index, float p)
{
	float* pose = (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flOldPoseParameter + (sizeof(float) * index));
	if (*pose != p)
	{
		*pose = p;
		InvalidatePhysicsRecursive(ANIMATION_CHANGED);
	}
}

void CBaseEntity::CopyOldPoseParameters(float* dest)
{
	float* flPose = (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flOldPoseParameter);
	memcpy(dest, flPose, sizeof(float) * MAX_CSGO_POSE_PARAMS);
}

int CBaseEntity::WriteOldPoseParameters(float* src)
{
	int flags	  = 0;
	float* pflPose = (float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flOldPoseParameter);

	for (int i = 0; i < MAX_CSGO_POSE_PARAMS; i++)
	{
		const float flPose		 = *pflPose;
		const float flStoredPose = src[i];
		if (flPose != flStoredPose)
		{
			*pflPose = flStoredPose;
			flags |= ANIMATION_CHANGED;
		}
		pflPose++;
	}

	return flags;
}

unsigned char CBaseEntity::GetClientSideAnimation()
{
	return *(unsigned char*)((DWORD)this + g_NetworkedVariables.Offsets.m_bClientSideAnimation);
}

void CBaseEntity::SetClientSideAnimation(unsigned char a)
{
	*(unsigned char*)((DWORD)this + g_NetworkedVariables.Offsets.m_bClientSideAnimation) = a;
}

float CBaseEntity::GetCycle()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flCycle);
}

void CBaseEntity::SetCycle(float cycle)
{
	float* pcycle = (float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flCycle);
	if (*pcycle != cycle)
	{
		*pcycle = cycle;
		InvalidatePhysicsRecursive(ANIMATION_CHANGED);
	}
}

float CBaseEntity::GetPlaybackRate()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPlaybackRate);
}

void CBaseEntity::SetPlaybackRate(float playbackrate)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flPlaybackRate) = playbackrate;
}

float CBaseEntity::GetNextAttack()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flNextAttack);
}

void CBaseEntity::SetNextAttack(float att)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flNextAttack) = att;
}

bool CBaseEntity::IsDucked()
{
	return GetLocalData()->localdata.m_bDucked_;
}

void CBaseEntity::SetDucked(bool ducked)
{
	GetLocalData()->localdata.m_bDucked_ = ducked;
}

bool CBaseEntity::IsDucking()
{
	return GetLocalData()->localdata.m_bDucking_;
}

void CBaseEntity::SetDucking(bool ducking)
{
	GetLocalData()->localdata.m_bDucking_ = ducking;
}

bool CBaseEntity::IsInDuckJump()
{
	return GetLocalData()->localdata.m_bInDuckJump_;
}

void CBaseEntity::SetInDuckJump(bool induckjump)
{
	GetLocalData()->localdata.m_bInDuckJump_ = induckjump;
}

int CBaseEntity::GetDuckJumpTimeMsecs()
{
	return GetLocalData()->localdata.m_nDuckJumpTimeMsecs_;
}

void CBaseEntity::SetDuckJumpTimeMsecs(int msecs)
{
	GetLocalData()->localdata.m_nDuckJumpTimeMsecs_ = msecs;
}

int CBaseEntity::GetDuckTimeMsecs()
{
	return GetLocalData()->localdata.m_nDuckTimeMsecs_;
}

void CBaseEntity::SetDuckTimeMsecs(int msecs)
{
	GetLocalData()->localdata.m_nDuckTimeMsecs_ = msecs;
}

int CBaseEntity::GetJumpTimeMsecs()
{
	return GetLocalData()->localdata.m_nJumpTimeMsecs_;
}

void CBaseEntity::SetJumpTimeMsecs(int msecs)
{
	GetLocalData()->localdata.m_nJumpTimeMsecs_ = msecs;
}

float CBaseEntity::GetFallVelocity()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flFallVelocity);
}

void CBaseEntity::SetFallVelocity(float vel)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flFallVelocity) = vel;
}

Vector CBaseEntity::GetViewOffset()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecViewOffset);
}

Vector& CBaseEntity::GetViewOffsetVMT()
{
	return StaticOffsets.GetVFuncByType<Vector&(__thiscall*)(CBaseEntity*)>(_GetViewOffsetVMT, this)(this);
}

void CBaseEntity::SetViewOffset(Vector& off)
{
	*(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecViewOffset) = off;
}

float CBaseEntity::GetDuckAmount()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flDuckAmount);
}

void CBaseEntity::SetDuckAmount(float duckamt)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flDuckAmount) = duckamt;
}

float CBaseEntity::GetDuckSpeed()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flDuckSpeed);
}

void CBaseEntity::SetDuckSpeed(float spd)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flDuckSpeed) = spd;
}

float CBaseEntity::GetVelocityModifier()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flVelocityModifier);
}

void CBaseEntity::SetVelocityModifier(float vel)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flVelocityModifier) = vel;
}

float CBaseEntity::GetStamina()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flStamina);
};

void CBaseEntity::SetStamina(float stamina)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flStamina) = stamina;
};

int CBaseEntity::GetSurfaceProps()
{
	return *StaticOffsets.GetOffsetValueByType< int* >(_m_SurfaceProps, this);
};

void CBaseEntity::SetSurfaceProps(int props)
{
	*StaticOffsets.GetOffsetValueByType< int* >(_m_SurfaceProps, this) = props;
};

surfacedata_t* CBaseEntity::GetSurfaceData()
{
	return *StaticOffsets.GetOffsetValueByType< surfacedata_t** >(_m_iSurfaceData, this);
};

void CBaseEntity::SetSurfaceData(surfacedata_t* data)
{
	*StaticOffsets.GetOffsetValueByType< surfacedata_t** >(_m_iSurfaceData, this) = data;
};

bool CBaseEntity::GetIsWalking()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bIsWalking);
};

void CBaseEntity::SetIsWalking(bool walking)
{
	*(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bIsWalking) = walking;
};

float CBaseEntity::GetTimeNotOnLadder()
{
	return *StaticOffsets.GetOffsetValueByType< float* >(_m_flTimeNotOnLadder, this);
};

void CBaseEntity::SetTimeNotOnLadder(float time)
{
	*StaticOffsets.GetOffsetValueByType< float* >(_m_flTimeNotOnLadder, this) = time;
};

Vector2D CBaseEntity::GetDuckingOrigin()
{
	return *StaticOffsets.GetOffsetValueByType< Vector2D* >(_m_vecDuckingOrigin, this);
};

void CBaseEntity::SetDuckingOrigin(Vector2D& origin)
{
	*StaticOffsets.GetOffsetValueByType< Vector2D* >(_m_vecDuckingOrigin, this) = origin;
};

void CBaseEntity::SetDuckingOrigin(Vector& origin)
{
	*StaticOffsets.GetOffsetValueByType< Vector2D* >(_m_vecDuckingOrigin, this) = { origin.x, origin.y };
}

int CBaseEntity::IsSpawnRappelling()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_bIsSpawnRappelling);
}

float CBaseEntity::GetMaxFallVelocity()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flMaxFallVelocity);
}

void CBaseEntity::SetMaxFallVelocity(float vel)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flMaxFallVelocity) = vel;
}

bool CBaseEntity::GetDuckOverride()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bDuckOverride);
};

void CBaseEntity::SetDuckOverride(bool over)
{
	*(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bDuckOverride) = over;
};

bool CBaseEntity::GetDuckUntilOnGround()
{
	return *StaticOffsets.GetOffsetValueByType< bool* >(_m_duckUntilOnGround, this);
};

void CBaseEntity::SetDuckUntilOnGround(bool duckuntilonground)
{
	*StaticOffsets.GetOffsetValueByType< bool* >(_m_duckUntilOnGround, this) = duckuntilonground;
};

bool CBaseEntity::UsesServerSideJumpAnimation()
{
	return *StaticOffsets.GetOffsetValueByType< bool* >(_m_bServerSideJumpAnimation, this);
};

bool CBaseEntity::GetSlowMovement()
{
	return *StaticOffsets.GetOffsetValueByType< bool* >(_m_bSlowMovement, this);
};

bool CBaseEntity::HasWalkMovedSinceLastJump()
{
	return *StaticOffsets.GetOffsetValueByType< bool* >(_m_bHasWalkMovedSinceLastJump, this);
};

void CBaseEntity::SetHasWalkMovedSinceLastJump(bool OnGround)
{
	*StaticOffsets.GetOffsetValueByType< bool* >(_m_bHasWalkMovedSinceLastJump, this) = OnGround;
};

unsigned char CBaseEntity::GetWaterTypeDirect()
{
	return *StaticOffsets.GetOffsetValueByType< unsigned char* >(_m_nWaterType, this);
}

void CBaseEntity::SetWaterTypeDirect(unsigned char watertype)
{
	*StaticOffsets.GetOffsetValueByType< unsigned char* >(_m_nWaterType, this) = watertype;
}

int CBaseEntity::GetWaterType()
{
	unsigned char watertype = *StaticOffsets.GetOffsetValueByType< unsigned char* >(_m_nWaterType, this);
	int out					= 0;
	if (watertype & 1)
		out |= CONTENTS_WATER;
	if (watertype & 2)
		out |= CONTENTS_SLIME;
	return out;
}

void CBaseEntity::SetWaterType(int nType)
{
	int watertype = 0;
	if (nType & CONTENTS_WATER)
		watertype |= 1;
	if (nType & CONTENTS_SLIME)
		watertype |= 2;

	*StaticOffsets.GetOffsetValueByType< unsigned char* >(_m_nWaterType, this) = watertype;
}

/*
float CBaseEntity::GetPlaybackRate()
{
	return *(float*)((DWORD)this + m_flPlaybackRate);
}

void CBaseEntity::SetPlaybackRate(float rate)
{
	*(float*)((DWORD)this + m_flPlaybackRate) = rate;
}

float CBaseEntity::GetAnimTime()
{
	return *(float*)((DWORD)this + m_flAnimTime);
}

void CBaseEntity::SetAnimTime(float time)
{
	*(float*)((DWORD)this + m_flAnimTime) = time;
}
*/

float CBaseEntity::GetSimulationTime()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flSimulationTime);
}

float CBaseEntity::GetOldSimulationTime()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flOldSimulationTime);
}

void CBaseEntity::SetOldSimulationTime(float time)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flOldSimulationTime) = time;
}

void CBaseEntity::SetSimulationTime(float time)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flSimulationTime) = time;
}

float CBaseEntity::GetLaggedMovement()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flLaggedMovementValue);
}

void CBaseEntity::SetLaggedMovement(float mov)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flLaggedMovementValue) = mov;
}

CBaseEntity* CBaseEntity::GetGroundEntity()
{
	Interfaces::MDLCache->BeginLock();
	EHANDLE handle = (EHANDLE) * (DWORD*)((DWORD)this + g_NetworkedVariables.Offsets.m_hGroundEntity);
	if (handle.IsValid())
	{
		CBaseEntity *ent = handle.Get();
		Interfaces::MDLCache->EndLock();
		return ent;
	}
	Interfaces::MDLCache->EndLock();
	return nullptr;
}

EHANDLE CBaseEntity::GetGroundEntityDirect()
{
	Interfaces::MDLCache->BeginLock();
	auto ret = *(EHANDLE*)((DWORD)this + g_NetworkedVariables.Offsets.m_hGroundEntity);
	Interfaces::MDLCache->EndLock();
	return ret;
}

void CBaseEntity::SetGroundEntityDirect(EHANDLE groundent)
{
	Interfaces::MDLCache->BeginLock();
	*(EHANDLE*)((DWORD)this + g_NetworkedVariables.Offsets.m_hGroundEntity) = groundent;
	Interfaces::MDLCache->EndLock();
}

void CBaseEntity::SetGroundEntity(CBaseEntity* groundent)
{
	Interfaces::MDLCache->BeginLock();
	((void(__thiscall*)(CBaseEntity*, CBaseEntity*))AdrOf_SetGroundEntity)(this, groundent);
	Interfaces::MDLCache->EndLock();
}

Vector CBaseEntity::GetVecLadderNormal()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecLadderNormal);
}

void CBaseEntity::SetVecLadderNormal(Vector& norm)
{
	*(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecLadderNormal) = norm;
}

float CBaseEntity::GetLowerBodyYaw()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flLowerBodyYawTarget);
}

void CBaseEntity::SetLowerBodyYaw(float yaw)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flLowerBodyYawTarget) = yaw;
}

bool CBaseEntity::CanUpdateAnimations()
{
	auto animstate = GetPlayerAnimState();
	return !animstate || (Interfaces::Globals->curtime != animstate->m_flLastClientSideAnimationUpdateTime && Interfaces::Globals->framecount != animstate->m_iLastClientSideAnimationUpdateFramecount);
}

void CBaseEntity::InvalidateAnimations()
{
	return; //Do nothing since rebuilt animstate->Update has this removed

	auto animstate = GetPlayerAnimState();
	if (animstate)
	{
		if (Interfaces::Globals->curtime == animstate->m_flLastClientSideAnimationUpdateTime)
			animstate->m_flLastClientSideAnimationUpdateTime -= Interfaces::Globals->interval_per_tick;
		if (Interfaces::Globals->framecount == animstate->m_iLastClientSideAnimationUpdateFramecount)
			--animstate->m_iLastClientSideAnimationUpdateFramecount;
	}
}

int CBaseEntity::DrawModel(int flags)
{
	ASSIGNVARANDIFNZERODO(_DrawModel, GetVFunc< int(__thiscall*)(void*, int, int) >(this->GetClientRenderable(), 9))
	return _DrawModel(this->GetClientRenderable(), flags, 0);

	return 0;
}

float CBaseEntity::GetMaxDesyncMultiplier(CTickrecord* sourcerecord)
{
	if (IsLocalPlayer())
		DebugBreak();

	CCSGOPlayerAnimState *animstate;
	if (!sourcerecord)
		animstate = ToPlayerRecord()->m_pAnimStateServer[ResolveSides::NONE];
	else
		animstate = sourcerecord->m_pAnimStateServer[sourcerecord->m_iResolveSide];

	if (!animstate)
		return 1.0f;

	float speed, newduckamount;

	if (!sourcerecord)
	{
		float flLastDuckAmount = animstate->m_fDuckAmount;
		float flNewDuckAmount = clamp(sourcerecord ? sourcerecord->m_DuckAmount : GetDuckAmount() + animstate->m_flHitGroundCycle, 0.0f, 1.0f);
		float flDuckSmooth = TICKS_TO_TIME(1) * 6.0f;
		float flDuckDelta = flNewDuckAmount - flLastDuckAmount;

		if (flDuckDelta <= flDuckSmooth) {
			if (-flDuckSmooth > flDuckDelta)
				flNewDuckAmount = flLastDuckAmount - flDuckSmooth;
		}
		else {
			flNewDuckAmount = flDuckSmooth + flLastDuckAmount;
		}

		newduckamount = clamp(flNewDuckAmount, 0.0f, 1.0f);

		Vector &velocity = sourcerecord ? sourcerecord->m_AbsVelocity : *animstate->pBaseEntity->GetAbsVelocity();
		Vector newvelocity = g_LagCompensation.GetSmoothedVelocity(animstate->m_flLastClientSideAnimationUpdateTimeDelta * 2000.0f, velocity, animstate->m_vVelocity);

		speed = std::fmin(newvelocity.Length(), 260.0f);

	}
	else
	{
		speed = animstate->m_flSpeed;
		newduckamount = animstate->m_fDuckAmount;
	}

	CBaseCombatWeapon *weapon = GetWeapon();

	float flMaxMovementSpeed = 260.0f;
	if (weapon)
		flMaxMovementSpeed = std::fmax(weapon->GetMaxSpeed3(), 0.001f);

	const float m_flRunningSpeed = clamp(speed / (flMaxMovementSpeed * 0.520f), 0.0f, 1.0f);

	float flYawModifier = (((animstate->m_flGroundFraction * -0.3f) - 0.2f) * m_flRunningSpeed) + 1.0f;

	if (newduckamount > 0.f)
	{
		const float m_flDuckingSpeed = clamp(speed / (flMaxMovementSpeed * 0.340f), 0.0f, 1.0f);
		flYawModifier += (newduckamount * m_flDuckingSpeed) * (0.5f - flYawModifier);
	}
	return flYawModifier;
}

float CBaseEntity::GetMaxDesyncDelta(CTickrecord *sourcerecord)
{
	if (IsLocalPlayer())
		DebugBreak();
	CCSGOPlayerAnimState *animstate;
	if (!sourcerecord)
		animstate = ToPlayerRecord()->m_pAnimStateServer[ResolveSides::NONE];
	else
		animstate = sourcerecord->m_pAnimStateServer[sourcerecord->m_iResolveSide];

	if (!animstate)
		return 58.0f;

	float maxyaw = animstate->m_flMaxYaw;
	return maxyaw * GetMaxDesyncMultiplier(sourcerecord);
}

C_CSGOPlayerAnimState* CBaseEntity::GetPlayerAnimState()
{
	return (C_CSGOPlayerAnimState*)*(DWORD*)((uintptr_t)this + StaticOffsets.GetOffsetValue(_m_hPlayerAnimState));
}


void* CBaseEntity::GetBasePlayerAnimState()
{
	//auto val = StaticOffsets.GetOffsetValue(_m_hPlayerAnimState);
	//val -= sizeof(uintptr_t);
	//return (C_CSGOPlayerAnimState*)*(DWORD*)((uintptr_t)this + val);
	return (void*)*StaticOffsets.GetOffsetValueByType<DWORD*>(_DoAnimationEvent1, this);
}

void CBaseEntity::EnableClientSideAnimation(clientanimating_t*& animating, int& out_originalflags)
{
	for (unsigned int i = 0; i < g_ClientSideAnimationList->count; i++)
	{
		clientanimating_t* tanimating = (clientanimating_t*)g_ClientSideAnimationList->Retrieve(i, sizeof(clientanimating_t));
		CBaseEntity* pAnimEntity	  = (CBaseEntity*)tanimating->pAnimating;
		if (pAnimEntity == this)
		{
			animating		  = tanimating;
			out_originalflags = tanimating->flags;
			tanimating->flags |= FCLIENTANIM_SEQUENCE_CYCLE;
			return;
		}
	}
	animating = nullptr;
}

void CBaseEntity::DisableClientSideAnimation(clientanimating_t*& animating, int& out_originalflags)
{
	for (unsigned int i = 0; i < g_ClientSideAnimationList->count; i++)
	{
		clientanimating_t* tanimating = (clientanimating_t*)g_ClientSideAnimationList->Retrieve(i, sizeof(clientanimating_t));
		CBaseEntity* pAnimEntity	  = (CBaseEntity*)tanimating->pAnimating;
		if (pAnimEntity == this)
		{
			animating		  = tanimating;
			out_originalflags = tanimating->flags;
			tanimating->flags &= ~FCLIENTANIM_SEQUENCE_CYCLE;
			return;
		}
	}
	animating = nullptr;
}

void CBaseEntity::RestoreClientSideAnimation(clientanimating_t*& animating, int& flags)
{
	if (animating)
	{
		animating->flags = flags;
	}
}

bool CBaseEntity::IsPlayerGhost()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bIsPlayerGhost);
}

#include "ClassIDS.h"
bool CBaseEntity::IsWeapon()
{
	// IsBaseCombatWeapon is actually unreliable and can throw false-positives.
	//return StaticOffsets.GetVFuncByType<bool(__thiscall*)(CBaseEntity*)>(_IsBaseCombatWeapon, this)(this);

	ClientClass* clclass = GetClientClass();
	if (!clclass)
		return false;

	int classid			 = clclass->m_ClassID;//ReadInt((uintptr_t)&clclass->m_ClassID);
	char* networkname	= clclass->m_pNetworkName;

	//CWeapon
	return ((networkname[0] == 'C' && networkname[1] == 'W' && networkname[2] == 'e') || (classid == _CAK47 || classid == _CDEagle));
}

bool CBaseEntity::IsProjectile()
{
	if (!GetClientNetworkable() || !GetClientClass())
		return false;

	switch (GetClientClass()->m_ClassID)
	{
		case _CSmokeGrenadeProjectile:
		case _CSensorGrenadeProjectile:
		case _CMolotovProjectile:
		case _CDecoyProjectile:
		case _CBaseCSGrenadeProjectile:
			return true;
	}
	return false;
}

bool CBaseEntity::IsGrenadeWeapon()
{
	switch (GetClientClass()->m_ClassID)
	{
		case _CHEGrenade:
		case _CMolotovGrenade:
		case _CSmokeGrenade:
		case _CIncendiaryGrenade:
		case _CDecoyGrenade:
		case _CFlashbang:
			return true;
	}
	return false;
}

bool CBaseEntity::IsKnifeWeapon()
{
	switch (GetClientClass()->m_ClassID)
	{
		case _CKnifeGG:
		case _CKnife:
		return true;
	}
	return false;
}

bool CBaseEntity::IsFlashGrenade()
{
	return GetClientClass()->m_ClassID == _CBaseCSGrenadeProjectile;
}

bool CBaseEntity::IsChicken()
{
	return GetClientClass()->m_ClassID == _CChicken;
}

void CBaseEntity::DrawHitboxes(ColorRGBA color, float livetimesecs)
{
	model_t* pmodel = GetModel();
	if (!pmodel)
		return;

	studiohdr_t* pStudioHdr = Interfaces::ModelInfoClient->GetStudioModel(pmodel);

	if (!pStudioHdr)
		return;

	mstudiohitboxset_t* set = pStudioHdr->pHitboxSet(GetHitboxSet());
	if (!set)
		return;

	matrix3x4_t boneMatrixes[MAXSTUDIOBONES];
	SetupBones(boneMatrixes, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, /*GetSimulationTime()*/ Interfaces::Globals->curtime);

	Vector hitBoxVectors[8];

	for (int i = 0; i < set->numhitboxes; i++)
	{
		mstudiobbox_t* pbox = set->pHitbox(i);
		if (pbox)
		{
			if (pbox->radius == -1.0f)
			{
				//Slow DirectX method, but works
#if 0
				Vector points[8] = { Vector(pbox->bbmin.x, pbox->bbmin.y, pbox->bbmin.z),
					Vector(pbox->bbmin.x, pbox->bbmax.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmax.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmin.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmax.y, pbox->bbmax.z),
					Vector(pbox->bbmin.x, pbox->bbmax.y, pbox->bbmax.z),
					Vector(pbox->bbmin.x, pbox->bbmin.y, pbox->bbmax.z),
					Vector(pbox->bbmax.x, pbox->bbmin.y, pbox->bbmax.z) };

				for (int index = 0; index < 8; ++index)
				{
					// scale down the hitbox size a tiny bit (default is a little too big)
#if 0
					points[index].x *= 0.9f;
					points[index].y *= 0.9f;
					points[index].z *= 0.9f;
#endif

					// transform the vector
					VectorTransformZ(points[index], boneMatrixes[pbox->bone], hitBoxVectors[index]);
				}

				DrawHitbox(color, hitBoxVectors);
#else
				matrix3x4_t matrix_rotation;
				AngleMatrix(pbox->angles, matrix_rotation);

				matrix3x4_t matrix_new;
				ConcatTransforms(boneMatrixes[pbox->bone], matrix_rotation, matrix_new);

				QAngle angles;
				MatrixAngles(matrix_new, angles);

				Interfaces::DebugOverlay->AddBoxOverlay(matrix_new.GetOrigin(), pbox->bbmin, pbox->bbmax, angles, color.r, color.g, color.b, 0, livetimesecs);

#endif
			}
			else
			{
				Vector vMin, vMax;
				//TransformAABB(boneMatrixes[pbox->bone], pbox->bbmin, pbox->bbmax, vMin, vMax);
				VectorTransformZ(pbox->bbmin, boneMatrixes[pbox->bone], vMin);
				VectorTransformZ(pbox->bbmax, boneMatrixes[pbox->bone], vMax);

				Interfaces::DebugOverlay->DrawPill(vMin, vMax, pbox->radius, color.r, color.g, color.b, color.a, livetimesecs, 0, 0);
			}
		}
	}
}

void CBaseEntity::DrawHitboxesFromCache(ColorRGBA color, float livetimesecs, matrix3x4_t* matrix)
{
	model_t* pmodel = GetModel();
	if (!pmodel)
		return;

	studiohdr_t* pStudioHdr = Interfaces::ModelInfoClient->GetStudioModel(pmodel);

	if (!pStudioHdr)
		return;

	mstudiohitboxset_t* set = pStudioHdr->pHitboxSet(GetHitboxSet());
	if (!set)
		return;

	for (int i = 0; i < set->numhitboxes; i++)
	{
		mstudiobbox_t* pbox = set->pHitbox(i);
		if (pbox)
		{
			if (pbox->radius == -1.0f)
			{
				//Slow DirectX method, but works
#if 0
				Vector points[8] = { Vector(pbox->bbmin.x, pbox->bbmin.y, pbox->bbmin.z),
					Vector(pbox->bbmin.x, pbox->bbmax.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmax.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmin.y, pbox->bbmin.z),
					Vector(pbox->bbmax.x, pbox->bbmax.y, pbox->bbmax.z),
					Vector(pbox->bbmin.x, pbox->bbmax.y, pbox->bbmax.z),
					Vector(pbox->bbmin.x, pbox->bbmin.y, pbox->bbmax.z),
					Vector(pbox->bbmax.x, pbox->bbmin.y, pbox->bbmax.z) };

				for (int index = 0; index < 8; ++index)
				{
					// scale down the hitbox size a tiny bit (default is a little too big)
#if 0
					points[index].x *= 0.9f;
					points[index].y *= 0.9f;
					points[index].z *= 0.9f;
#endif

					// transform the vector
					VectorTransformZ(points[index], boneMatrixes[pbox->bone], hitBoxVectors[index]);
				}

				DrawHitbox(color, hitBoxVectors);
#else

				matrix3x4_t matrix_rotation;
				AngleMatrix(pbox->angles, matrix_rotation);

				matrix3x4_t matrix_new;
				ConcatTransforms(matrix[pbox->bone], matrix_rotation, matrix_new);

				QAngle angles;
				MatrixAngles(matrix_new, angles);

				Interfaces::DebugOverlay->AddBoxOverlay(matrix_new.GetOrigin(), pbox->bbmin, pbox->bbmax, angles, color.r, color.g, color.b, 0, livetimesecs);

#endif
			}
			else
			{
				Vector vMin, vMax;
				VectorTransformZ(pbox->bbmin, matrix[pbox->bone], vMin);
				VectorTransformZ(pbox->bbmax, matrix[pbox->bone], vMax);

				Interfaces::DebugOverlay->DrawPill(vMin, vMax, pbox->radius, color.r, color.g, color.b, color.a, livetimesecs, 0, 1);
			}
		}
	}
}

bool CBaseEntity::IsCustomPlayer()
{
	return *StaticOffsets.GetOffsetValueByType<bool*>(_m_bIsCustomPlayer, this);
}

void CBaseEntity::SetIsCustomPlayer(bool iscustom)
{
	*StaticOffsets.GetOffsetValueByType<bool*>(_m_bIsCustomPlayer, this) = iscustom;
}

bool CBaseEntity::WasKilledByTaser()
{
	return *(bool*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_bKilledByTaser);
}

int CBaseEntity::GetViewModelIndex()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_nViewModelIndex);
}

void CBaseEntity::HandleTaserAnimation()
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*)>(_HandleTaserAnimation)(this);
}

C_BaseViewModel* CBaseEntity::GetViewModel(int modelindex)
{
	return StaticOffsets.GetOffsetValueByType<C_BaseViewModel*(__thiscall*)(CBaseEntity*, int)>(_GetViewModel)(this, modelindex);
}

void CBaseEntity::UpdateClientSideAnimation()
{
	START_PROFILING
	if (IsCustomPlayer())
	{
		GetPlayerAnimState()->Update(EyeAngles()->y, EyeAngles()->x);
	}
	else
	{
		if (GetSequence() != -1)
			FrameAdvance(0);

		if (IsLocalPlayer())
			((C_CSGOPlayerAnimState*)GetBasePlayerAnimState())->Update(EyeAngles()->y, EyeAngles()->x);
		else
			((C_CSGOPlayerAnimState*)GetBasePlayerAnimState())->Update(GetEyeAngles().y, GetEyeAngles().x);
	}

	if (GetSequence() != -1)
		OnLatchInterpolatedVariables(1);

	if (WasKilledByTaser())
		HandleTaserAnimation();

	if (IsLocalPlayer())
	{
		CBaseCombatWeapon *weapon = GetActiveCSWeapon();
		if (weapon)
		{
			C_BaseViewModel *viewmodel = GetViewModel(GetViewModelIndex());
			if (viewmodel)
				viewmodel->UpdateAllViewmodelAddons();
		}
		else
		{
			for (int i = 0; i < 3; ++i)
			{
				C_BaseViewModel *viewmodel = GetViewModel(i);
				if (viewmodel)
				{
					viewmodel->RemoveViewmodelArmModels();
					viewmodel->RemoveViewmodelLabel();
					viewmodel->RemoveViewmodelStatTrak();
					viewmodel->RemoveViewmodelStickers();
				}
			}
		}
	}

	/*
	m_bAnimationUpdateAllowed = true;
	GetVFunc< UpdateClientSideAnimationFn >(this, OffsetOf_UpdateClientSideAnimation)(this);
	m_bAnimationUpdateAllowed = false;
	*/

	END_PROFILING
}

void CBaseEntity::OnLatchInterpolatedVariables(int unk)
{
	StaticOffsets.GetVFuncByType<void(__thiscall*)(CBaseEntity*, int)>(_OnLatchInterpolatedVariables, this)(this, unk);
}

void CBaseEntity::FrameAdvance(int frame)
{
	StaticOffsets.GetVFuncByType<void(__thiscall*)(CBaseEntity*, int)>(_FrameAdvance, this)(this, frame);
}

float CBaseEntity::GetLastClientSideAnimationUpdateTime()
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		return animstate->m_flLastClientSideAnimationUpdateTime;
	}

	return 0.0f;
}

void CBaseEntity::SetLastClientSideAnimationUpdateTime(float time)
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		animstate->m_flLastClientSideAnimationUpdateTime = time;
	}
}

int CBaseEntity::GetLastClientSideAnimationUpdateGlobalsFrameCount()
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		return animstate->m_iLastClientSideAnimationUpdateFramecount;
	}

	return 0;
}

void CBaseEntity::SetLastClientSideAnimationUpdateGlobalsFrameCount(int framecount)
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		animstate->m_iLastClientSideAnimationUpdateFramecount = framecount;
	}
}

int CBaseEntity::GetEffects()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_fEffects);
}

void CBaseEntity::SetEffects(int effects)
{
	*(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_fEffects) = effects;
}

void CBaseEntity::AddEffect(int effect)
{
	*(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_fEffects) |= effect;
}

void CBaseEntity::RemoveEffect(int effect)
{
	*(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_fEffects) &= ~effect;
}

int CBaseEntity::GetObserverMode()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iObserverMode);
}

int CBaseEntity::GetObserverModeVMT()
{
	//56  8B  F1  80  BE  ??  ??  00  00  00  74  7D  8B  06  FF  90  ??  ??  00  00  + 16 DEC
	typedef int(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_GetObserverModeVMT, this)(this);
}

void CBaseEntity::SetObserverMode(int mode)
{
	*(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iObserverMode) = mode;
}

CUtlVectorSimple* CBaseEntity::GetAnimOverlayStruct() const
{
	return StaticOffsets.GetOffsetValueByType< CUtlVectorSimple* >(_AnimOverlay, this);
}

C_AnimationLayer* CBaseEntity::GetAnimOverlay(int i)
{
	if (i >= 0 && i < MAX_OVERLAYS)
	{
		CUtlVectorSimple* m_AnimOverlay = GetAnimOverlayStruct();
		return (C_AnimationLayer*)m_AnimOverlay->Retrieve(i, sizeof(C_AnimationLayer));
		//DWORD v19 = 7 * i;
		//DWORD m = *(DWORD*)((DWORD)m_AnimOverlay);
		//C_AnimationLayer* test = (C_AnimationLayer*)(m + 8 * v19);
		//DWORD sequence = *(DWORD *)(m + 8 * v19 + 24);
	}
	return nullptr;
}

C_AnimationLayer* CBaseEntity::GetAnimOverlayDirect(int i)
{
	CUtlVectorSimple* m_AnimOverlay = GetAnimOverlayStruct();
	if (m_AnimOverlay->Count())
		return (C_AnimationLayer*)m_AnimOverlay->Retrieve(i, sizeof(C_AnimationLayer));
	return nullptr;
}

int CBaseEntity::GetNumAnimOverlays() const
{
	CUtlVectorSimple* m_AnimOverlay = GetAnimOverlayStruct();
	return m_AnimOverlay ? m_AnimOverlay->count : 0;
}

void CBaseEntity::CopyAnimLayers(C_AnimationLayer* dest)
{
	int count = GetNumAnimOverlays();
	for (int i = 0; i < count; i++)
	{
		dest[i] = *GetAnimOverlay(i);
	}
}

int CBaseEntity::WriteAnimLayers(C_AnimationLayer* src)
{
	int flags	 = 0;
	int animcount = GetNumAnimOverlays();
	for (int i = 0; i < animcount; i++)
	{
		C_AnimationLayer* pLayer	   = GetAnimOverlay(i);
		C_AnimationLayer* pStoredLayer = &src[i];

		if (pLayer->_m_fFlags != pStoredLayer->_m_fFlags)
		{
			pLayer->_m_fFlags = pStoredLayer->_m_fFlags;
			flags |= ANIMATION_CHANGED;
		}

		if (pLayer->_m_flPlaybackRate != pStoredLayer->_m_flPlaybackRate)
		{
			pLayer->_m_flPlaybackRate = pStoredLayer->_m_flPlaybackRate;
			flags |= ANIMATION_CHANGED;
		}

		if (pLayer->m_flWeightDeltaRate != pStoredLayer->m_flWeightDeltaRate)
		{
			pLayer->m_flWeightDeltaRate = pStoredLayer->m_flWeightDeltaRate;
			flags |= ANIMATION_CHANGED;
		}

		if (pLayer->m_iActivity != pStoredLayer->m_iActivity)
		{
			pLayer->m_iActivity = pStoredLayer->m_iActivity;
			flags |= ANIMATION_CHANGED;
		}

		if (pLayer->m_flWeight != pStoredLayer->m_flWeight)
		{
			pLayer->m_flWeight = pStoredLayer->m_flWeight;
			flags |= ANIMATION_CHANGED;
		}

		if (pLayer->_m_nSequence != pStoredLayer->_m_nSequence)
		{
			pLayer->_m_nSequence = pStoredLayer->_m_nSequence;
			flags |= SEQUENCE_CHANGED;
			flags |= ANIMATION_CHANGED;
			flags |= BOUNDS_CHANGED;
		}

		if (pLayer->_m_flCycle != pStoredLayer->_m_flCycle)
		{
			pLayer->_m_flCycle = pStoredLayer->_m_flCycle;
			flags |= ANIMATION_CHANGED;
		}

		if (pLayer->m_nOrder != pStoredLayer->m_nOrder)
		{
			pLayer->m_nOrder = pStoredLayer->m_nOrder;
			flags |= BOUNDS_CHANGED;
		}
	}
	return flags;
}

int CBaseEntity::WriteAnimLayer(C_AnimationLayer* src, C_AnimationLayer* dest)
{
	int flags					   = 0;
	C_AnimationLayer* pStoredLayer = src;
	C_AnimationLayer* pLayer	   = dest;
	if (pLayer->_m_fFlags != pStoredLayer->_m_fFlags)
	{
		pLayer->_m_fFlags = pStoredLayer->_m_fFlags;
		flags |= ANIMATION_CHANGED;
	}

	if (pLayer->_m_flPlaybackRate != pStoredLayer->_m_flPlaybackRate)
	{
		pLayer->_m_flPlaybackRate = pStoredLayer->_m_flPlaybackRate;
		flags |= ANIMATION_CHANGED;
	}

	if (pLayer->m_flWeightDeltaRate != pStoredLayer->m_flWeightDeltaRate)
	{
		pLayer->m_flWeightDeltaRate = pStoredLayer->m_flWeightDeltaRate;
		flags |= ANIMATION_CHANGED;
	}

	if (pLayer->m_iActivity != pStoredLayer->m_iActivity)
	{
		pLayer->m_iActivity = pStoredLayer->m_iActivity;
		flags |= ANIMATION_CHANGED;
	}

	if (pLayer->m_flWeight != pStoredLayer->m_flWeight)
	{
		pLayer->m_flWeight = pStoredLayer->m_flWeight;
		flags |= ANIMATION_CHANGED;
	}

	if (pLayer->_m_nSequence != pStoredLayer->_m_nSequence)
	{
		pLayer->_m_nSequence = pStoredLayer->_m_nSequence;
		flags |= SEQUENCE_CHANGED;
		flags |= ANIMATION_CHANGED;
		flags |= BOUNDS_CHANGED;
	}

	if (pLayer->_m_flCycle != pStoredLayer->_m_flCycle)
	{
		pLayer->_m_flCycle = pStoredLayer->_m_flCycle;
		flags |= ANIMATION_CHANGED;
	}

	if (pLayer->m_nOrder != pStoredLayer->m_nOrder)
	{
		pLayer->m_nOrder = pStoredLayer->m_nOrder;
		flags |= BOUNDS_CHANGED;
	}

	return flags;
}

int CBaseEntity::WriteAnimLayersFromPacket(C_AnimationLayerFromPacket* src)
{
	int flags = 0;
	int count = GetNumAnimOverlays();
	for (int i = 0; i < count; i++)
	{
		C_AnimationLayer* pLayer				 = GetAnimOverlay(i);
		C_AnimationLayerFromPacket* pStoredLayer = &src[i];

		//if (pLayer->_m_fFlags != pStoredLayer->_m_fFlags)
		{
			//	pLayer->_m_fFlags = pStoredLayer->_m_fFlags;
			//	flags |= ANIMATION_CHANGED;
		}

		if (pLayer->_m_flPlaybackRate != pStoredLayer->_m_flPlaybackRate)
		{
			pLayer->_m_flPlaybackRate = pStoredLayer->_m_flPlaybackRate;
			flags |= ANIMATION_CHANGED;
		}

		//if (pLayer->m_flWeightDeltaRate != pStoredLayer->m_flWeightDeltaRate)
		//{
		//	pLayer->m_flWeightDeltaRate = pStoredLayer->m_flWeightDeltaRate;
		//	flags |= ANIMATION_CHANGED;
		//}

		//if (pLayer->m_iActivity != pStoredLayer->m_iActivity)
		{
			//	pLayer->m_iActivity = pStoredLayer->m_iActivity;
			//	flags |= ANIMATION_CHANGED;
		}

		if (pLayer->m_flWeight != pStoredLayer->m_flWeight)
		{
			pLayer->m_flWeight = pStoredLayer->m_flWeight;
			flags |= ANIMATION_CHANGED;
		}

		if (pLayer->_m_nSequence != pStoredLayer->_m_nSequence)
		{
			pLayer->_m_nSequence = pStoredLayer->_m_nSequence;
			flags |= ANIMATION_CHANGED;
			flags |= BOUNDS_CHANGED;
		}

		if (pLayer->_m_flCycle != pStoredLayer->_m_flCycle)
		{
			pLayer->_m_flCycle = pStoredLayer->_m_flCycle;
			flags |= ANIMATION_CHANGED;
		}

		if (pLayer->m_nOrder != pStoredLayer->_m_nOrder)
		{
				pLayer->m_nOrder = pStoredLayer->_m_nOrder;
				flags |= BOUNDS_CHANGED;
		}
	}

	return flags;
}

bool CBaseEntity::GetAllowAutoMovement()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bAllowAutoMovement);
};

void CBaseEntity::SetAllowAutoMovement(bool allow)
{
	*(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bAllowAutoMovement) = allow;
};

CBaseEntity* CBaseEntity::GetMoveParent()
{
	EHANDLE handle = *(EHANDLE*)((uintptr_t)this + g_NetworkedVariables.Offsets.moveparent);
	if (handle.IsValid())
		return handle.Get();
	return nullptr;
}

const char* CBaseEntity::GetClassname()
{
	return StaticOffsets.GetVFuncByType< const char*(__thiscall*)(CBaseEntity*) >(_GetClassnameVMT, this)(this);
}

int CBaseEntity::GetMaxHealth()
{
	return StaticOffsets.GetVFuncByType< int(__thiscall*)(CBaseEntity*) >(_GetMaxHealthVMT, this)(this);
}

CBoneAccessor* CBaseEntity::GetBoneAccessor()
{
	return StaticOffsets.GetOffsetValueByType< CBoneAccessor* >(_BoneAccessor, this);
}

// Invalidates the abs state of all children
void CBaseEntity::InvalidatePhysicsRecursive(int nChangeFlags)
{
	((void(__thiscall*)(CBaseEntity*, int))AdrOf_InvalidatePhysicsRecursive)(this, nChangeFlags);
}

Vector* CBaseEntity::GetAbsOrigin()
{
	return StaticOffsets.GetVFuncByType< Vector*(__thiscall*)(CBaseEntity*) >(_GetAbsOriginVMT, this)(this);
}

Vector CBaseEntity::GetAbsOriginDirect()
{
	return *StaticOffsets.GetOffsetValueByType< Vector* >(_AbsOriginDirect, this);
}

void CBaseEntity::SetAbsOriginDirect(Vector& origin)
{
	*StaticOffsets.GetOffsetValueByType< Vector* >(_AbsOriginDirect, this) = origin;
}

int CBaseEntity::entindex()
{
	return index;
}

float CBaseEntity::GetCurrentFeetYaw()
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		return animstate->m_flCurrentFeetYaw;
	}
	return 0.0f;
}

void CBaseEntity::SetCurrentFeetYaw(float yaw)
{
	//0x7C = pitch
	//0x74 = some type of radians?
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		animstate->m_flCurrentFeetYaw = yaw;
	}
}

float CBaseEntity::GetGoalFeetYaw()
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		return animstate->m_flGoalFeetYaw;
	}
	return 0.0f;
}

void CBaseEntity::SetGoalFeetYaw(float yaw)
{
	C_CSGOPlayerAnimState* animstate = GetPlayerAnimState();
	if (animstate)
	{
		animstate->m_flGoalFeetYaw = yaw;
	}
}

float CBaseEntity::GetFriction()
{
	return *(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flFriction);
}

void CBaseEntity::SetFriction(float friction)
{
	*(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flFriction) = friction;
}

float CBaseEntity::GetElasticity()
{
	return *(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flElasticity);
}

void CBaseEntity::SetElasticity(float elasticity)
{
	*(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flElasticity) = elasticity;
}

float CBaseEntity::GetStepSize()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flStepSize);
}

void CBaseEntity::SetStepSize(float stepsize)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flStepSize) = stepsize;
}

float CBaseEntity::GetMaxSpeed()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flMaxSpeed);
}

void CBaseEntity::SetMaxSpeed(float maxspeed)
{
	*(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_flMaxSpeed) = maxspeed;
}

float CBaseEntity::GetPlayerMaxSpeed()
{
	typedef float(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_GetPlayerMaxSpeedVMT, this)(this);
}

bool CBaseEntity::IsParentChanging()
{
	//m_hNetworkMoveParent
	return GetMoveParent() != (CBaseEntity*)*(DWORD*)((DWORD)this + m_pMoveParent);
}

void CBaseEntity::SetLocalVelocity(const Vector& vecVelocity)
{
	if (GetVelocity() != vecVelocity)
	{
		InvalidatePhysicsRecursive(VELOCITY_CHANGED);
		SetVelocity(vecVelocity);
	}
}

int CBaseEntity::GetTakeDamage()
{
	return *StaticOffsets.GetOffsetValueByType< int* >(_TakeDamage, this);
};

void CBaseEntity::StandardBlendingRules(CStudioHdr* hdr, Vector pos[], QuaternionAligned q[], float currentTime, int boneMask)
{
	typedef void(__thiscall * OriginalFn)(CBaseEntity*, CStudioHdr*, Vector[], QuaternionAligned[], float, int);
	StaticOffsets.GetVFuncByType< OriginalFn >(_StandardBlendingRules, this)(this, hdr, pos, q, currentTime, boneMask);
}

void CBaseEntity::BuildTransformations(CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& cameraTransform, int boneMask, byte* boneComputed)
{
	typedef void(__thiscall * OriginalFn)(CBaseEntity*, CStudioHdr*, Vector*, Quaternion*, const matrix3x4_t&, int, byte*);
	StaticOffsets.GetVFuncByType< OriginalFn >(_BuildTransformations, this)(this, hdr, pos, q, cameraTransform, boneMask, boneComputed);
}

bool CBaseEntity::IsRagdoll()
{
	bool a = *StaticOffsets.GetOffsetValueByType< DWORD* >(_m_pRagdoll, this);
	bool b = *(char*)((DWORD)this + g_NetworkedVariables.Offsets.m_bClientSideRagdoll);
	//return *StaticOffsets.GetOffsetValueByType< DWORD* >(_m_pRagdoll, this) && *(char*)((DWORD)this + g_NetworkedVariables.Offsets.m_bClientSideRagdoll);
	return a && b;
}

int CBaseEntity::LookupSequence(char *seq)
{
	return StaticOffsets.GetOffsetValueByType< int(__thiscall*)(CBaseEntity*, char*) >(_LookupSequence)(this, seq);
}

CUtlVectorSimple* CBaseEntity::GetCachedBoneData()
{
	return (CUtlVectorSimple*)((DWORD)this + StaticOffsets.GetOffsetValue(_m_CachedBoneData));
}

unsigned long CBaseEntity::GetMostRecentModelBoneCounter()
{
	return *(unsigned long*)((DWORD)this + StaticOffsets.GetOffsetValue(_m_iMostRecentModelBoneCounter));
}

void CBaseEntity::SetMostRecentModelBoneCounter(unsigned long counter)
{
	*(unsigned long*)((DWORD)this + StaticOffsets.GetOffsetValue(_m_iMostRecentModelBoneCounter)) = counter;
}

int CBaseEntity::GetLastOcclusionCheckFrameCount()
{
	return *(int*)((DWORD)this + StaticOffsets.GetOffsetValue(_LastOcclusionCheckTime));
}

void CBaseEntity::SetLastOcclusionCheckFrameCount(int count)
{
	*(int*)((DWORD)this + StaticOffsets.GetOffsetValue(_LastOcclusionCheckTime)) = count;
}

int CBaseEntity::GetLastOcclusionCheckFlags()
{
	return *(int*)((DWORD)this + StaticOffsets.GetOffsetValue(_LastOcclusionCheckTime) - 0x8);
}

void CBaseEntity::SetLastOcclusionCheckFlags(int flags)
{
	*(int*)((DWORD)this + StaticOffsets.GetOffsetValue(_LastOcclusionCheckTime) - 0x8) = flags;
}

void CBaseEntity::UpdateIKLocks(float currentTime)
{
	typedef void(__thiscall * oUpdateIKLocks)(PVOID, float currentTime);
	StaticOffsets.GetVFuncByType< oUpdateIKLocks >(_UpdateIKLocks, this)(this, currentTime);
}

void CBaseEntity::CalculateIKLocks(float currentTime)
{
	typedef void(__thiscall * oCalculateIKLocks)(PVOID, float currentTime);
	StaticOffsets.GetVFuncByType< oCalculateIKLocks >(_CalculateIKLocks, this)(this, currentTime);
}

void CBaseEntity::ControlMouth(CStudioHdr* pStudioHdr)
{
	typedef void(__thiscall * oControlMouth)(PVOID, CStudioHdr*);
	StaticOffsets.GetVFuncByType< oControlMouth >(_ControlMouth, this)(this, pStudioHdr);
}
void CBaseEntity::Wrap_SolveDependencies(DWORD m_pIk, Vector* pos, Quaternion* q, matrix3x4_t* bonearray, byte* computed)
{
	//typedef void(__thiscall *pSolveDependencies) (DWORD *m_pIk, DWORD UNKNOWN, Vector *pos, Quaternion* q, matrix3x4_t* bonearray, byte *computed);
	SolveDependencies(m_pIk, pos, q, bonearray, computed);
}

void CBaseEntity::Wrap_UpdateTargets(DWORD m_pIk, Vector* pos, Quaternion* q, matrix3x4_t* bonearray, byte* computed)
{
	//typedef void(__thiscall *pUpdateTargets) (DWORD *m_pIk, Vector *pos, Quaternion* q, matrix3x4_t* bonearray, byte *computed);

	UpdateTargets(m_pIk, pos, q, bonearray, computed);
}

CUtlVectorSimple& CBaseEntity::m_Attachments()
{
	CUtlVectorSimple* attachments = StaticOffsets.GetOffsetValueByType< CUtlVectorSimple* >(_Attachments, this);
	return *attachments;
}

void CBaseEntity::Wrap_AttachmentHelper(CStudioHdr* hdr)
{
	AttachmentHelper(this, hdr);
}

void CBaseEntity::Wrap_AttachmentHelper()
{
	AttachmentHelper(this, this->GetModelPtr());
}

void CBaseEntity::ClearTargets(DWORD m_pIk)
{
	if (*(int*)(m_pIk + 4080) > 0)
	{
		int* v111 = (int*)(m_pIk + 208);
		for (int i = 0; i < *(int*)(m_pIk + 4080); i++)
		{
			*v111 = -9999;
			v111  = (int*)((DWORD)v111 + 85);
		}
	}
}

void CBaseEntity::Wrap_IKInit(DWORD m_pIk, CStudioHdr* hdrs, QAngle& angles, Vector& pos, float flTime, int iFramecounter, int boneMask)
{
	//typedef void(__thiscall *pInit) (DWORD *m_pIk, studiohdr_t * hdrs, Vector &angles, Vector &pos, float flTime, int iFramecounter, int boneMask);
	IKInit(m_pIk, hdrs, angles, pos, flTime, iFramecounter, boneMask);
}
void CBaseEntity::DoExtraBoneProcessing(CStudioHdr* hdr, Vector* pos, Quaternion* q, matrix3x4_t* bonearray, byte* computed, DWORD m_pIK)
{
	typedef void(__thiscall * OriginalFn)(CBaseEntity*, CStudioHdr*, Vector*, Quaternion*, matrix3x4_t*, byte*, DWORD);
	StaticOffsets.GetVFuncByType< OriginalFn >(_DoExtraBoneProcessing, this)(this, hdr, pos, q, bonearray, computed, m_pIK);
}
void CBaseEntity::MarkForThreadedBoneSetup()
{
	MarkForThreadedBoneSetupCall(this);
}

bool CBaseEntity::SequencesAvailable()
{
	DWORD hdr = (DWORD)GetModelPtr();
	int v29;

	if (!hdr
		|| *(DWORD*)hdr == 0
		|| *(DWORD*)(*(DWORD*)hdr + 0x150)
			&& !*(DWORD*)(hdr + 4)
			&& (*(DWORD*)(*(DWORD*)hdr + 0x150) ? (v29 = (*(int(__thiscall*)(DWORD, DWORD))(*(DWORD*)SequencesAvailableVMT + 0x68))(*(DWORD*)SequencesAvailableVMT, *(DWORD*)hdr)) : (v29 = 0),
				SequencesAvailableCall((studiohdr_t*)hdr, v29) == 0))
	{
		return false;
	}
	return true;
}

void CBaseEntity::ReevaluateAnimLOD()
{
	ReevaluateAnimLod(this, (CBaseEntity*)GetClientRenderable());
}

DWORD CBaseEntity::Wrap_CreateIK()
{
	using MemAllocFn = void*(__thiscall*)(void* thisptr, size_t);
	void* thisptr	= (void*)*(DWORD*)(gpMemAllocVTable);

	//F6  47  64  02  75  28  A1  ??  ??  ??  ??  68  ??  ??  00  00  8B  08  8B  01  8B  40  04  FF  D0 + 12 dec
	void* memory = GetVFunc< MemAllocFn >(thisptr, 1)(thisptr, 0x1070);
	if (memory)
	{
		return ConstructIK(memory);
	}
	return (DWORD)memory;
}

int CBaseEntity::GetButtons()
{
	return *StaticOffsets.GetOffsetValueByType<int*>(_m_nButtons, this);
}

void CBaseEntity::SetButtons(int buttons)
{
	*StaticOffsets.GetOffsetValueByType<int*>(_m_nButtons, this) = buttons;
}

void CBaseEntity::UpdateButtonState(int nUserCmdButtonMask)
{
	int* m_nButtons = StaticOffsets.GetOffsetValueByType<int*>(_m_nButtons, this);
	int* m_afButtonLast = StaticOffsets.GetOffsetValueByType<int*>(_m_afButtonLast, this);
	int* m_afButtonPressed = StaticOffsets.GetOffsetValueByType<int*>(_m_afButtonPressed, this);
	int* m_afButtonReleased = StaticOffsets.GetOffsetValueByType<int*>(_m_afButtonReleased, this);

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	*m_afButtonLast = *m_nButtons;

	// Get button states
	*m_nButtons = nUserCmdButtonMask;
	int buttonsChanged = *m_afButtonLast ^ *m_nButtons;

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	*m_afButtonPressed = buttonsChanged & *m_nButtons;		// The changed ones still down are "pressed"
	*m_afButtonReleased = buttonsChanged & (~*m_nButtons);	// The ones not down are "released"
}

void CBaseEntity::BackupButtonState(ButtonState_t &state)
{
	int* m_nButtons = StaticOffsets.GetOffsetValueByType<int*>(_m_nButtons, this);
	int* m_afButtonLast = StaticOffsets.GetOffsetValueByType<int*>(_m_afButtonLast, this);
	int* m_afButtonPressed = StaticOffsets.GetOffsetValueByType<int*>(_m_afButtonPressed, this);
	int* m_afButtonReleased = StaticOffsets.GetOffsetValueByType<int*>(_m_afButtonReleased, this);

	state.nButtons = *m_nButtons;
	state.afButtonLast = *m_afButtonLast;
	state.afButtonPressed = *m_afButtonPressed;
	state.afButtonReleased = *m_afButtonReleased;
}

void CBaseEntity::RestoreButtonState(ButtonState_t &state)
{
	int* m_nButtons = StaticOffsets.GetOffsetValueByType<int*>(_m_nButtons, this);
	int* m_afButtonLast = StaticOffsets.GetOffsetValueByType<int*>(_m_afButtonLast, this);
	int* m_afButtonPressed = StaticOffsets.GetOffsetValueByType<int*>(_m_afButtonPressed, this);
	int* m_afButtonReleased = StaticOffsets.GetOffsetValueByType<int*>(_m_afButtonReleased, this);

	*m_nButtons = state.nButtons;
	*m_afButtonLast = state.afButtonLast;
	*m_afButtonPressed = state.afButtonPressed;
	*m_afButtonReleased = state.afButtonReleased;
}

int CBaseEntity::GetSequenceFlags(int sequence)
{
	CStudioHdr* pstudiohdr = GetModelPtr();

	if (!SequencesAvailable())
		return 0;

	mstudioseqdesc_t* seqdesc = opSeqdesc((studiohdr_t*)pstudiohdr, sequence);

	return seqdesc->flags;
}

float CBaseEntity::GetFirstSequenceAnimTag(int sequence, int tag)
{
#ifdef _DEBUG
	int garbo = 0;
	float res;
	oGetFirstSequenceAnimTag(this, sequence, tag, &garbo);
	__asm movss res, xmm0
	return res;
#else
	CStudioHdr *modelptr = GetModelPtr();

	if (!modelptr)
		return 0.0f;

	virtualmodel_t *viewmodel = modelptr->m_pVModel;
	int numlocalsequences = viewmodel ? viewmodel->m_seq.count : modelptr->_m_pStudioHdr->numlocalseq;
	if (sequence >= numlocalsequences)
		return 0.0f;

	mstudioseqdesc_t *seqdesc;
	if (viewmodel)
		seqdesc = (mstudioseqdesc_t *)opSeqdesc((studiohdr_t*)modelptr, sequence);
	else
		seqdesc = modelptr->_m_pStudioHdr->pLocalSeqdesc(sequence);

	int num_animtags = seqdesc->num_animtags;
	if (num_animtags <= 0)
		return 0.0f;

	for (int i = 0; i < num_animtags; ++i)
	{
		animtag_t *pTag = seqdesc->pAnimTag(i);
		if (pTag->index != -1)
		{
			if (!pTag->index)
			{
				pTag->index = GetTagIndexFromAnimTagName(pTag->GetName());
			}
			if (pTag->index == tag)
			{
				float cycle = pTag->first_cycle;
				if (cycle >= 0.0f && cycle < 1.0f)
					return cycle;
			}
		}
	}
	return 0.0f;
#endif
}

float CBaseEntity::GetAnySequenceAnimTag(int sequence, int tag, float flDefault)
{
/*#ifdef _DEBUG
	float res;
	__asm movss xmm3, flDefault
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*, int, int)>(_GetAnySequenceAnimTag)(this, sequence, tag);
	__asm movss res, xmm0
	return res;
#else*/
	CStudioHdr *modelptr = GetModelPtr();

	if (!modelptr)
		return flDefault;

	virtualmodel_t *viewmodel = modelptr->m_pVModel;
	int numlocalsequences = viewmodel ? viewmodel->m_seq.count : modelptr->_m_pStudioHdr->numlocalseq;
	if (sequence >= numlocalsequences)
		return flDefault;

	mstudioseqdesc_t *seqdesc;
	if (viewmodel)
		seqdesc = (mstudioseqdesc_t *)opSeqdesc((studiohdr_t*)modelptr, sequence);
	else
		seqdesc = modelptr->_m_pStudioHdr->pLocalSeqdesc(sequence);

	int num_animtags = seqdesc->num_animtags;
	if (num_animtags <= 0)
		return flDefault;

	for (int i = 0; i < num_animtags; ++i)
	{
		animtag_t *pTag = seqdesc->pAnimTag(i);
		if (pTag->index != -1)
		{
			if (!pTag->index)
			{
				pTag->index = GetTagIndexFromAnimTagName(pTag->GetName());
			}
			if (pTag->index == tag)
			{
				return pTag->first_cycle;
			}
		}
	}
	return flDefault;
//#endif
}

float& CBaseEntity::GetUnknownAnimationFloat()
{
	return *StaticOffsets.GetOffsetValueByType< float* >(_UnknownAnimationFloat, this);
}

float& CBaseEntity::GetUnknownSetupMovementFloat()
{
	return *StaticOffsets.GetOffsetValueByType< float* >(_UnknownSetupMovementFloat, this);
}

void CBaseEntity::DoUnknownAnimationCode(DWORD offset, float val)
{
	uint32_t thecall	= StaticOffsets.GetOffsetValueByType< uint32_t >(_UnknownAnimationCall);
	uint32_t pointer = (offset - sizeof(uint32_t)) + (uint32_t)this;
	float val2		 = val;
	float oldxmm;
	__asm
	{
		mov ecx, pointer
		mov eax, thecall
		movss oldxmm, xmm1
		movss xmm1, val2
		call eax
		movss xmm1, oldxmm
	}
}

void CBaseEntity::OnJump(float flUpVelocity)
{
	CBaseCombatWeapon* weapon = GetWeapon();
	if (weapon)
		weapon->OnJump(flUpVelocity);
}

//char* ccsplayeronlandsigstr = new char[83]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 66, 73, 90, 90, 63, 78, 90, 90, 60, 66, 90, 90, 66, 75, 90, 90, 63, 57, 90, 90, 72, 66, 90, 90, 74, 72, 90, 90, 74, 74, 90, 90, 74, 74, 90, 90, 79, 76, 90, 90, 66, 56, 90, 90, 60, 75, 90, 90, 60, 73, 90, 90, 74, 60, 90, 90, 75, 75, 90, 90, 78, 57, 90, 90, 72, 78, 90, 90, 74, 66, 0 }; /*55  8B  EC  83  E4  F8  81  EC  28  02  00  00  56  8B  F1  F3  0F  11  4C  24  08*/
//char *svminjumplandingsoundstr = new char[26]{ 9, 12, 37, 23, 19, 20, 37, 16, 15, 23, 10, 37, 22, 27, 20, 30, 19, 20, 29, 37, 9, 21, 15, 20, 30, 0 }; /*sv_min_jump_landing_sound*/
//void(__thiscall* CCSPlayer_OnLand)(CBaseEntity*) = NULL;
ConVar* sv_min_jump_landing_sound;

static bool bAllowHitGroundSounds = false;

void CBaseEntity::OnLand(float flUpVelocity)
{
	if (bAllowHitGroundSounds)
	{
		//if (!CCSPlayer_OnLand)
		//{
		//	DecStr(ccsplayeronlandsigstr, 82);
		//	CCSPlayer_OnLand = (void(__thiscall*)(CBaseEntity*))FindMemoryPattern(ClientHandle, ccsplayeronlandsigstr, 82);
		//	EncStr(ccsplayeronlandsigstr, 82);
		//	delete[] ccsplayeronlandsigstr;
		//	if (!CCSPlayer_OnLand)
		//	{
		//		THROW_ERROR(ERR_CANT_FIND_CSPLAYER_ONLAND_SIGNATURE);
		//		exit(EXIT_SUCCESS);
		//	}
		//}

		__asm movss xmm1, flUpVelocity
		StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*)>(_OnLand)(this);
		//CCSPlayer_OnLand(this);
	}
	else
	{
		CBaseCombatWeapon* weapon = GetWeapon();
		if (weapon)
			weapon->OnLand(flUpVelocity);
	}
}

void CBaseEntity::PlayClientJumpSound()
{
	//FF  90  ??  ??  00  00  8B  8E  54  0E  00  00  80  B9  ??  ??  00  00  00 + 2 dec
	using OriginalFn = void*(__thiscall*)(CBaseEntity*);
	StaticOffsets.GetVFuncByType< OriginalFn >(_PlayClientJumpSound, this)(this);
}

void CBaseEntity::PlayClientUnknownSound(Vector& origin, surfacedata_t* surf)
{
	using OriginalFn = void*(__thiscall*)(CBaseEntity*, Vector&, surfacedata_t*);
	StaticOffsets.GetVFuncByType< OriginalFn >(_PlayClientUnknownSound, this)(this, origin, surf);
}

void CBaseEntity::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	//this is actually Base Animstate
	//8B  89  ??  ??  00  00  6A  00  6A  08  8B  01  FF  50  ?? + 2 dec
	void* somevtable = (void*)*StaticOffsets.GetOffsetValueByType< DWORD* >(_DoAnimationEvent1, this);
	//+ 14 dec
	uint32_t vtableindex = (uint32_t)*StaticOffsets.GetOffsetValueByType< uint8_t* >(_DoAnimationEvent2) / 4; //0x18 / 4

	using OriginalFn = void(__thiscall*)(void*, PlayerAnimEvent_t, int);
	GetVFunc< OriginalFn >(somevtable, vtableindex)(somevtable, event, nData);

	//*GetVFunc<OriginalFn>(somevtable, vtableindex)(somevtable, event, nData);
	//using OriginalFn = void(__thiscall **)(void*, PlayerAnimEvent_t, int);
	//(*OriginalFn( *(DWORD*)somevtable + vtableindex)) (somevtable, event, nData);
}

bool CBaseEntity::IsInAVehicle()
{
	return false;

	//return oIsInAVehicle(this);
}

int CBaseEntity::GetVehicleHandle()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_hVehicle);
}

IClientVehicle* CBaseEntity::GetClientVehicle()
{
	using OriginalFn = IClientVehicle*(__thiscall*)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_GetClientVehicle, this)(this);
}

IClientVehicle* CBaseEntity::GetVehicle()
{
	CBaseEntity* pVehicleEnt = Interfaces::ClientEntList->GetBaseEntityFromHandle(GetVehicleHandle());

	return pVehicleEnt ? pVehicleEnt->GetClientVehicle() : NULL;
}

void CBaseEntity::ResetLatched()
{
	using OriginalFn = void*(__thiscall*)(CBaseEntity*);
	StaticOffsets.GetVFuncByType< OriginalFn >(_ResetLatched, this)(this);
}

bool CBaseEntity::Interpolate(float flCurrentTime)
{
	using OriginalFn = bool(__thiscall*)(CBaseEntity*, float);
	return StaticOffsets.GetOffsetValueByType<bool(__thiscall*)(CBaseEntity*, float)>(_Interpolate)(this, flCurrentTime);
}

VarMapping_t* CBaseEntity::GetVarMapping()
{
	return StaticOffsets.GetOffsetValueByType<VarMapping_t*>(_InterpolationVarMap, this);
}

float CBaseEntity::GetLastDuckTime()
{
	return GetLocalData()->localdata.m_flLastDuckTime_;
}

void CBaseEntity::SetLastDuckTime(float time)
{
	GetLocalData()->localdata.m_flLastDuckTime_ = time;
}

bool CBaseEntity::IsBot()
{
	DWORD resource = *(DWORD*)(DWORD)g_PR;
	if (!resource)
		return false;

	DWORD resource2 = resource + StaticOffsets.GetOffsetValue(_IsBotPlayerResourceOffset);

	if (!resource2)
		return false;

	DWORD networkable = (DWORD)this + 8;

	typedef DWORD(__thiscall * OriginalFn)(DWORD);
	DWORD val = GetVFunc< OriginalFn >((void*)networkable, (0x28 / 4))(networkable);

	typedef bool(__thiscall * OriginalFn2)(DWORD, DWORD);
	bool isbot = GetVFunc< OriginalFn2 >((void*)resource2, (0x18 / 4))(resource2, val);

	return isbot;
}

int CBaseEntity::GetStuckLast()
{
	return *StaticOffsets.GetOffsetValueByType< int* >(_m_StuckLast, this);
};

void CBaseEntity::SetStuckLast(int stucklast)
{
	*StaticOffsets.GetOffsetValueByType< int* >(_m_StuckLast, this) = stucklast;
};

IPhysicsObject* CBaseEntity::VPhysicsGetObject()
{
	return *StaticOffsets.GetOffsetValueByType< IPhysicsObject** >(_m_VPhysicsObject, this);
};

Vector CBaseEntity::GetWaterJumpVel()
{
	return *StaticOffsets.GetOffsetValueByType< Vector* >(_m_vecWaterJumpVel, this);
};

void CBaseEntity::SetWaterJumpVel(Vector& vel)
{
	*StaticOffsets.GetOffsetValueByType< Vector* >(_m_vecWaterJumpVel, this) = vel;
};

float CBaseEntity::GetSwimSoundTime()
{
	return *StaticOffsets.GetOffsetValueByType< float* >(_m_flSwimSoundTime, this);
};

void CBaseEntity::SetSwimSoundTime(float time)
{
	*StaticOffsets.GetOffsetValueByType< float* >(_m_flSwimSoundTime, this) = time;
};

CPlayerLocalData* CBaseEntity::GetLocalData()
{
	return (CPlayerLocalData*)((DWORD)this + g_NetworkedVariables.Offsets.m_Local);
};

float CBaseEntity::GetWaterJumpTime()
{
	return *StaticOffsets.GetOffsetValueByType< float* >(_m_flWaterJumpTime, this);
}

void CBaseEntity::SetWaterJumpTime(float time)
{
	*StaticOffsets.GetOffsetValueByType< float* >(_m_flWaterJumpTime, this) = time;
}

float* CBaseEntity::GetSurfaceFrictionAdr()
{
	return StaticOffsets.GetOffsetValueByType< float* >(_SurfaceFriction, this);
};

float CBaseEntity::GetSurfaceFriction()
{
	return *StaticOffsets.GetOffsetValueByType< float* >(_SurfaceFriction, this);
};

void CBaseEntity::SetSurfaceFriction(float friction)
{
	*StaticOffsets.GetOffsetValueByType< float* >(_SurfaceFriction, this) = friction;
};

char CBaseEntity::GetTextureType()
{
	return *StaticOffsets.GetOffsetValueByType< char* >(_m_chTextureType, this);
};

void CBaseEntity::SetTextureType(char type)
{
	*StaticOffsets.GetOffsetValueByType< char* >(_m_chTextureType, this) = type;
};

int CBaseEntity::GetFireCount()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_fireCount);
}

void CBaseEntity::SetFireCount(int count)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_fireCount) = count;
}

int CBaseEntity::GetRelativeDirectionOfLastInjury()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nRelativeDirectionOfLastInjury);
}

void CBaseEntity::SetRelativeDirectionOfLastInjury(int dir)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nRelativeDirectionOfLastInjury) = dir;
}

int CBaseEntity::GetLastHitgroup()
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_LastHitGroup);
}

void CBaseEntity::SetLastHitgroup(int hitgroup)
{
	*(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_LastHitGroup) = hitgroup;
}

unsigned char CBaseEntity::GetWaterLevel()
{
	return *(unsigned char*)((DWORD)this + g_NetworkedVariables.Offsets.m_nWaterLevel);
};

void CBaseEntity::SetWaterLevel(unsigned char level)
{
	*(unsigned char*)((DWORD)this + g_NetworkedVariables.Offsets.m_nWaterLevel) = level;
};

void CBaseEntity::UpdateStepSound(surfacedata_t* psurface, const Vector& vecOrigin, const Vector& vecVelocity)
{
	typedef void(__thiscall * OriginalFn)(CBaseEntity*, surfacedata_t*, const Vector&, const Vector&);
	StaticOffsets.GetVFuncByType< OriginalFn >(_UpdateStepSound, this)(this, psurface, vecOrigin, vecVelocity);
}

int CBaseEntity::GetPlayerState()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iPlayerState);
}

void CBaseEntity::SetPlayerState(int state)
{
	*(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iPlayerState) = state;
}

int CBaseEntity::GetMoveState()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iMoveState);
};

void CBaseEntity::SetMoveState(int state)
{
	*(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iMoveState) = state;
};

bool CBaseEntity::IsObserver()
{
	return GetObserverModeVMT() != 0;
}

bool CBaseEntity::HasHeavyArmor()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bHasHeavyArmor);
}

void CBaseEntity::SetHasHeavyArmor(bool has)
{
	*(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bHasHeavyArmor) = has;
}

bool CBaseEntity::IsCarryingHostage(int unknown)
{
	return oIsCarryingHostage(this, unknown);
}

float CBaseEntity::GetGroundAccelLinearFracLastTime()
{
	return *(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flGroundAccelLinearFracLastTime);
}

void CBaseEntity::CBaseEntity::SetGroundAccelLinearFracLastTime(float time)
{
	*(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flGroundAccelLinearFracLastTime) = time;
}

CBaseCombatWeapon* CBaseEntity::GetActiveCSWeapon()
{
	typedef CBaseCombatWeapon*(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_GetActiveCSWeapon, this)(this);
}

CBaseCombatWeapon* CBaseEntity::GetCSWeapon(ClassID classname)
{
	CBaseEntity* weapon = (CBaseEntity*)GetWeapon();
	if (weapon && weapon->GetClientClass()->m_ClassID == classname)
		return (CBaseCombatWeapon*)weapon;
	return nullptr;
}

float CBaseEntity::GetGravity()
{
	return *(float*)((DWORD)this + m_flGravity); //TODO
}

void CBaseEntity::SetGravity(float grav)
{
	*(float*)((DWORD)this + m_flGravity) = grav; //TODO
}

//BUG BY CSGO DEVELOPERS: This is the same exact location as 'm_nJumpTimeMsecs' as an int
float CBaseEntity::GetJumpTime()
{
	return *(float*)&GetLocalData()->localdata.m_nJumpTimeMsecs_;
}

//BUG BY CSGO DEVELOPERS: This is the same exact location as 'm_nJumpTimeMsecs' as an int
void CBaseEntity::SetJumpTime(float time)
{
	*(float*)&GetLocalData()->localdata.m_nJumpTimeMsecs_ = time;
}

bool CBaseEntity::IsTaunting()
{
	typedef bool(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_IsTaunting, this)(this);
}

bool CBaseEntity::IsInThirdPersonTaunt()
{
	typedef bool(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_IsInThirdPersonTaunt, this)(this);
}

float CBaseEntity::GetStepSoundTime()
{
	return *StaticOffsets.GetOffsetValueByType< float* >(_StepSoundTime, this);
}

void CBaseEntity::SetStepSoundTime(float time)
{
	*StaticOffsets.GetOffsetValueByType< float* >(_StepSoundTime, this) = time;
}

void CBaseEntity::SurpressLadderChecks(Vector* origin, Vector* normal)
{
	oSurpressLadderChecks(this, origin, normal);
}

void CBaseEntity::PlayStepSound(Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force, bool unknown)
{
	typedef void(__thiscall * OriginalFn)(CBaseEntity*, Vector&, surfacedata_t*, float, bool, bool);
	StaticOffsets.GetVFuncByType< OriginalFn >(_PlayFootstepSound, this)(this, vecOrigin, psurface, fvol, force, false);
}

void CBaseEntity::EyeVectors(Vector* pForward, Vector* pRight, Vector* pUp)
{
	oEyeVectors(this, pForward, pRight, pUp);
}

CUserCmd* CBaseEntity::m_PlayerCommand()
{
	return StaticOffsets.GetOffsetValueByType< CUserCmd* >(_m_pPlayerCommand, this);
}

CUserCmd** CBaseEntity::m_pCurrentCommand()
{
	return StaticOffsets.GetOffsetValueByType< CUserCmd** >(_m_pCurrentCommand, this);
};

int CBaseEntity::CurrentCommandNumber()
{
	return (*m_pCurrentCommand())->command_number;
}

bool CBaseEntity::IsServer()
{
	return false;
}

bool CBaseEntity::GetCanMoveDuringFreezePeriod()
{
	return *(bool*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_bCanMoveDuringFreezePeriod);
};

bool CBaseEntity::IsGrabbingHostage()
{
	return *(bool*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_bIsGrabbingHostage);
}

bool CBaseEntity::GetAliveVMT()
{
	//reads lifestate
	typedef bool(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_GetAliveVMT, this)(this);
}

bool CBaseEntity::GetWaitForNoAttack()
{
	return *(bool*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_bWaitForNoAttack);
}

void CBaseEntity::SetWaitForNoAttack(bool wait)
{
	*(bool*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_bWaitForNoAttack) = wait;
}


float CBaseEntity::GetDefaultFOV()
{
	typedef int(__thiscall * OriginalFn)(CBaseEntity*);
	return StaticOffsets.GetVFuncByType< OriginalFn >(_GetDefaultFOV, this)(this);
}

matrix3x4_t& CBaseEntity::EntityToWorldTransform()
{
	//Assert(s_bAbsQueriesValid);
	CalcAbsolutePosition();
	return *GetCoordinateFrame();
}

bool CBaseEntity::GetShouldUseAnimationEyeOffset()
{
	return *StaticOffsets.GetOffsetValueByType< bool* >(_m_bUseAnimationEyeOffset, this);
}

void CBaseEntity::SetShouldUseAnimationEyeOffset(bool use)
{
	*StaticOffsets.GetOffsetValueByType< bool* >(_m_bUseAnimationEyeOffset, this) = use;
}

int CBaseEntity::IsAutoMounting()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_nIsAutoMounting);
}

void CBaseEntity::SetIsAutoMounting(int automounting)
{
	*(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_nIsAutoMounting) = automounting;
}

Vector& CBaseEntity::GetAutoMoveOrigin()
{
	return *StaticOffsets.GetOffsetValueByType< Vector* >(_SurvivalModeOrigin, this);
}

void CBaseEntity::SetAutoMoveOrigin(Vector& origin)
{
	*StaticOffsets.GetOffsetValueByType< Vector* >(_SurvivalModeOrigin, this) = origin;
}

Vector& CBaseEntity::GetAutomoveTargetEnd()
{
	return *(Vector*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_vecAutomoveTargetEnd);
}

void CBaseEntity::SetAutomoveTargetEnd(Vector& end)
{
	*(Vector*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_vecAutomoveTargetEnd) = end;
}

float CBaseEntity::GetAutomoveStartTime()
{
	return *(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flAutoMoveStartTime);
}

void CBaseEntity::SetAutomoveStartTime(float time)
{
	*(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flAutoMoveStartTime) = time;
}

float CBaseEntity::GetAutomoveTargetTime()
{
	return *(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flAutoMoveTargetTime);
}

void CBaseEntity::SetAutomoveTargetTime(float time)
{
	*(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flAutoMoveTargetTime) = time;
}

#if 0
bool CBaseEntity::GetUnknownSurvivalBool()
{
	//removed in nov 18, 2019
	return *StaticOffsets.GetOffsetValueByType< bool* >(_UnknownSurvivalBool, this);
}

void CBaseEntity::SetUnknownSurvivalBool(bool val)
{
	//removed in nov 18, 2019
	*StaticOffsets.GetOffsetValueByType< bool* >(_UnknownSurvivalBool, this) = val;
}
#endif

void CBaseEntity::ThirdPersonSwitch(bool thirdperson)
{
	StaticOffsets.GetVFuncByType< void(__thiscall*)(CBaseEntity*, bool) >(_ThirdPersonSwitchVMT, this)(this, thirdperson);
}

int CBaseEntity::BlockingUseActionInProgress()
{
	return *(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iBlockingUseActionInProgress);
}

void CBaseEntity::SetBlockingUseActionInProgress(int block)
{
	*(int*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_iBlockingUseActionInProgress) = block;
}

bool CBaseEntity::CanWaterJump()
{
	return StaticOffsets.GetVFuncByType< bool(__thiscall*)(CBaseEntity*) >(_CanWaterJumpVMT, this)(this);
}

float CBaseEntity::GetEncumberance()
{
	float enc;
	uintptr_t thisptr = (uintptr_t)this;
	uintptr_t func	= StaticOffsets.GetOffsetValue(_GetEncumberance);
	__asm
	{
		mov ecx, thisptr
		call func
		movss enc, xmm0
	}
	return enc;
}

void CBaseEntity::Think()
{
	//FIXME
	GetVFunc<void(__thiscall*)(CBaseEntity*)>(this, 138)(this);
}

void CBaseEntity::PreThink()
{
	//FIXME
	GetVFunc<void(__thiscall*)(CBaseEntity*)>(this, 314)(this);
}

void CBaseEntity::PostThink()
{
	//FIXME
	GetVFunc<void(__thiscall*)(CBaseEntity*)>(this, 315)(this);
}

void CBaseEntity::RunPreThink()
{
	//FIXME
	static auto fn = reinterpret_cast<bool(__thiscall*)(void*, int)>(FindMemoryPattern(ClientHandle, std::string("55 8B EC 83 EC 10 53 56 57 8B F9 8B 87"), false));

	if (fn(this, 0))
		PreThink();
}

void CBaseEntity::RunThink()
{
	//FIXME
	static auto fn = reinterpret_cast<void(__thiscall*)(int)>(FindMemoryPattern(ClientHandle, std::string("55 8B EC 56 57 8B F9 8B B7 ? ? ? ? 8B C6"), false));
	int thinktick = GetThinkTick();

	if (thinktick != -1 && thinktick > 0
		&& thinktick < TIME_TO_TICKS(Interfaces::Globals->curtime))
	{
		SetThinkTick(-1);
		fn(0);
		Think();
	}
}

int CBaseEntity::GetThinkTick()
{
	//FIXME
	int* pThinkTick = (int*)((DWORD)this + 0x40);
	return *pThinkTick;
}

void CBaseEntity::SetThinkTick(int tick)
{
	//FIXME
	int* pThinkTick = (int*)((DWORD)this + 0x40);
	*pThinkTick = tick;
}

float CBaseEntity::GetHealthShotBoostExpirationTime()
{
	return *(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flHealthShotBoostExpirationTime);
}

void CBaseEntity::SetHealthShotBoostExpirationTime(float time)
{
	*(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flHealthShotBoostExpirationTime) = time;
}

ClientRenderHandle_t& CBaseEntity::RenderHandle()
{
	return GetClientUnknown()->GetClientRenderable()->RenderHandle();
}

float CBaseEntity::GetTimeOfLastInjury()
{
	return *(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flTimeOfLastInjury);
}

void CBaseEntity::SetTimeOfLastInjury(float time)
{
	*(float*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_flTimeOfLastInjury) = time;
}

int CBaseEntity::GetSolidFlags()
{
	return StaticOffsets.GetVFuncByType<int(__thiscall*)(CBaseEntity*)>(_GetSolidFlagsVMT, this)(this);
}

Vector CBaseEntity::GetAngularVelocity()
{
	return *(Vector*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_vecAngVelocity);
}

void CBaseEntity::SetAngularVelocity(Vector& vel)
{
	*(Vector*)((uintptr_t)this + g_NetworkedVariables.Offsets.m_vecAngVelocity) = vel;
}

bool CBaseEntity::IsStandable()
{
	if (GetSolidFlags() & FSOLID_NOT_STANDABLE)
		return false;

	if (GetSolid() == SOLID_BSP || GetSolid() == SOLID_VPHYSICS || GetSolid() == SOLID_BBOX)
		return true;

	return IsBSPModel();
}

bool CBaseEntity::IsBSPModel()
{
	if (GetSolid() == SOLID_BSP)
		return true;

	const model_t *model = Interfaces::ModelInfoClient->GetModel(GetModelIndex());

	if (GetSolid() == SOLID_VPHYSICS && Interfaces::ModelInfoClient->GetModelType(model) == mod_brush)
		return true;

	return false;
}

surfacedata_t* CBaseEntity::GetGroundSurface()
{
	//
	// Find the name of the material that lies beneath the player.
	//
	Vector start = *GetAbsOrigin();
	Vector end = start;

	// Straight down
	end.z -= 64;

	// Fill in default values, just in case.

	Ray_t ray;
	ray.Init(start, end, GetPlayerMins(), GetPlayerMaxs());

	trace_t	trace;
	UTIL_TraceRay(ray, MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace);

	if (trace.fraction == 1.0f)
		return NULL;	// no ground

	return Interfaces::Physprops->GetSurfaceData(trace.surface.surfaceProps);
}

CBaseEntity* CBaseEntity::FindGroundEntity()
{
	START_PROFILING
	Interfaces::MDLCache->BeginLock();
	Vector point = GetAbsOriginDirect();
	point.z -= 2.0f;

	Vector absvel = GetAbsVelocityDirect();
	bool bMovingUp = absvel.z > 0.f;
	bool bMovingUpRapidly = absvel.z > 140.f;
	CBaseEntity* groundent = GetGroundEntity();

	if (bMovingUpRapidly)
	{
		if (groundent)
		{
			bMovingUpRapidly = (groundent->GetAbsVelocity()->z > 140.f);
		}
	}

	//bool bMoveToEndPos = false;

	if (GetMoveType() == MOVETYPE_WALK)
	{
		if (groundent)
		{
			//bMoveToEndPos = true;
			point.z -= GetStepSize();
		}
	}

	if (bMovingUpRapidly || (bMovingUp && GetMoveType() == MOVETYPE_LADDER))
	{
		Interfaces::MDLCache->EndLock();
		END_PROFILING
		return nullptr;
	}
	else
	{
		trace_t tr;
		TracePlayerBBox(GetAbsOriginDirect(), point, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr);

		if (TraceIsOnGroundOrPlayer(&tr))
		{
			Interfaces::MDLCache->EndLock();
			END_PROFILING
			return tr.m_pEnt;
		}

		CTraceFilterSimple filter;
		filter.SetPassEntity((IHandleEntity*)this);
		filter.SetCollisionGroup(COLLISION_GROUP_PLAYER_MOVEMENT);
		TracePlayerBBoxForGround(GetAbsOriginDirect(), point, GetPlayerMins(), GetPlayerMaxs(), (IHandleEntity*)this, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr);

		if (TraceIsOnGroundOrPlayer(&tr))
		{
			Interfaces::MDLCache->EndLock();
			END_PROFILING
			return tr.m_pEnt;
		}
	}
	Interfaces::MDLCache->EndLock();
	END_PROFILING
	return nullptr;
}

/*
void CBaseEntity::ResolveFlyCollisionCustom(trace_t& trace, Vector& vecVelocity)
{
	// stop if on ground
	if (trace.plane.normal.z > 0.7f)
	{
		vecVelocity = *GetAbsVelocity() + GetBaseVelocity();
		float gravity = GetGravity();
		if (gravity == 0.0f)
			gravity = 1.0f;

		float svgravity = sv_gravity.GetVar()->GetFloat();
		if ((svgravity * gravity) * Interfaces::Globals->frametime > vecVelocity.z)
		{
			// we're rolling on the ground, add static friction.
			Vector newvel = *GetAbsVelocity();
			newvel.z = 0.0f;
			SetAbsVelocity(newvel);
		}
		if (trace.m_pEnt->IsStandable())
			SetGroundEntity(trace.m_pEnt);
	}
}

void CBaseEntity::ResolveFlyCollisionBounce(trace_t& trace, Vector& vecVelocity)
{
	float surfelasticity;
	IPhysicsSurfaceProps* props = (*Interfaces::MoveHelperClient)->GetSurfaceProps();
	props->GetPhysicsProperties(trace.surface.surfaceProps, nullptr, nullptr, nullptr, &surfelasticity);

	float totalelasticity = GetElasticity() * surfelasticity;
	if (totalelasticity >= 0.0f)
		totalelasticity = fminf(totalelasticity, 0.9f);
	else
		totalelasticity = 0.0f;

	Vector velocity = *GetAbsVelocity();
	float v7 = DotProduct(velocity, trace.plane.normal);
	float v8 = v7 + v7;

	Vector newvel = velocity - (trace.plane.normal * v8);

	if (newvel.x > -0.1f && newvel.x < 0.1f)
	{
		newvel.x = 0.0f;
	}

	if (newvel.y > -0.1f && newvel.y < 0.1f)
	{
		newvel.y = 0.0f;
	}

	if (newvel.z > -0.1f && newvel.z < 0.1f)
	{
		newvel.z = 0.0f;
	}

	Vector newabsvel = newvel * totalelasticity;
	Vector backupnewabsvel = newabsvel;

	newabsvel += GetBaseVelocity();
	vecVelocity = newabsvel;

	float vel = DotProduct(newabsvel, newabsvel);

	// stop if on ground
	if (trace.plane.normal.z > 0.7f)
	{
		float gravity = GetGravity();
		if (gravity == 0.0f)
			gravity = 1.0f;

		float svgravity = sv_gravity.GetVar()->GetFloat();
		if ((svgravity * gravity) * Interfaces::Globals->frametime > vecVelocity.z)
		{
			// we're rolling on the ground, add static friction.
			backupnewabsvel.z = 0.0f;

			vecVelocity.x = backupnewabsvel.x + GetBaseVelocity().x;
			vecVelocity.y = backupnewabsvel.y + GetBaseVelocity().y;
			vecVelocity.z = GetBaseVelocity().z;

			vel = DotProduct(vecVelocity, vecVelocity);
		}
		SetAbsVelocity(backupnewabsvel);

		if (vel >= (30 * 30))
		{
			Vector newvel2 = GetBaseVelocity() - backupnewabsvel;
			backupnewabsvel.z = GetBaseVelocity().z;

			Vector normalized = backupnewabsvel;
			VectorNormalizeFast(normalized);

			Vector endvel = normalized * GetBaseVelocity();
			float v35 = (normalized.y * GetBaseVelocity().y) + (normalized.x + GetBaseVelocity().x);
			float endvelz = v35 + (normalized.z * GetBaseVelocity().z);

			float scale = ((1.0f - trace.fraction) * Interfaces::Globals->frametime);

			vecVelocity = backupnewabsvel * scale;
			vecVelocity = ((GetBaseVelocity() * endvelz) * scale) + vecVelocity;
			PhysicsPushEntity(vecVelocity, &trace);
			return;
		}

		if (trace.m_pEnt->IsStandable())
			SetGroundEntity(trace.m_pEnt);

		SetAbsVelocity(vecZero);
		if (GetAngularVelocity() != vecZero)
			SetAngularVelocity(vecZero);

		return;
	}

	if (vel >= (30 * 30))
	{
		SetAbsVelocity(backupnewabsvel);
		return;
	}
	SetAbsVelocity(vecZero);
	if (GetAngularVelocity() != vecZero)
		SetAngularVelocity(vecZero);
}

void CBaseEntity::ResolveFlyCollisionSlide(trace_t& pm, Vector& move)
{
	float friction;
	IPhysicsSurfaceProps* props = (*Interfaces::MoveHelperClient)->GetSurfaceProps();
	props->GetPhysicsProperties(pm.surface.surfaceProps, nullptr, nullptr, &friction, nullptr);

	Vector velocity = *GetAbsVelocity();
	float v7 = DotProduct(velocity, pm.plane.normal);

	Vector newvel = velocity - (pm.plane.normal * v7);
	Vector newabsvel = newvel;

	if (newvel.x > -0.1f && newvel.x < 0.1f)
	{
		newvel.x = 0.0f;
		newabsvel.x = 0.0f;
	}

	if (newvel.y > -0.1f && newvel.y < 0.1f)
	{
		newvel.y = 0.0f;
		newabsvel.y = 0.0f;
	}

	if (newvel.z > -0.1f && newvel.z < 0.1f)
	{
		newvel.z = 0.0f;
		newabsvel.z = 0.0f;
	}

	// stop if on ground
	if (pm.plane.normal.z > 0.7f)
	{
		move = GetBaseVelocity() + newvel;
		float vel = DotProduct(move, move);

		float gravity = GetGravity();
		if (gravity == 0.0f)
			gravity = 1.0f;

		float svgravity = sv_gravity.GetVar()->GetFloat();
		if ((svgravity * gravity) * Interfaces::Globals->frametime > move.z)
		{
			// we're rolling on the ground, add static friction.
			newabsvel.z = 0.0f;
			move.x = newvel.x + GetBaseVelocity().x;
			move.x = newvel.y + GetBaseVelocity().y;
			move.z = GetBaseVelocity().z;
			vel = DotProduct(move, move);
		}
		SetAbsVelocity(newabsvel);

		if (vel >= (30 * 30))
		{
			float scale = ((1.0f - pm.fraction) * Interfaces::Globals->frametime) * friction;
			Vector newv;
			newv.x = (GetBaseVelocity().x + newvel.x) * scale;
			newv.y = (GetBaseVelocity().y + newvel.y) * scale;
			newv.z = (GetBaseVelocity().z + newabsvel.z) * scale;
			newabsvel = newv;
			PhysicsPushEntity(newabsvel, &pm);
		}
		else
		{
			if (pm.m_pEnt->IsStandable())
				SetGroundEntity(pm.m_pEnt);

			SetAbsVelocity(vecZero);
			if (GetAngularVelocity() != vecZero)
				SetAngularVelocity(vecZero);
		}
	}
	else
	{
		SetAbsVelocity(newabsvel);
	}
}

void CBaseEntity::PhysicsPushEntity(const Vector& push, trace_t* pTrace)
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*, const Vector&, trace_t*)>(_PhysicsPushEntity)(this, push, pTrace);
}
*/

void CBaseEntity::PhysicsCheckWaterTransition()
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*)>(_PhysicsCheckWaterTransition)(this);
}

int CBaseEntity::GetDesiredCollisionGroup()
{
	return *StaticOffsets.GetOffsetValueByType<int*>(_DesiredCollisionGroup, this);
}

void CBaseEntity::SetDesiredCollisionGroup(int group)
{
	*StaticOffsets.GetOffsetValueByType<int*>(_DesiredCollisionGroup, this) = group;
}

CBaseEntity* CBaseEntity::GetUnknownEntity(int slot)
{
	return StaticOffsets.GetVFuncByType<CBaseEntity*(__thiscall*)(CBaseEntity*, int)>(_GetUnknownEntity, this)(this, slot);
}

void CBaseEntity::Remove(IHandleEntity* pEnt)
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(IHandleEntity*)>(_RemoveEntity)(pEnt);
}

void CBaseEntity::SetNumAnimOverlays(int num)
{
	CUtlVector<C_AnimationLayer> *struc = (CUtlVector<C_AnimationLayer> *)GetAnimOverlayStruct();
	if (struc->Count() < num)
	{
		struc->AddMultipleToTail(num - struc->Count());
	}
	else if (struc->Count() > num)
	{
		struc->RemoveMultiple(num, struc->Count() - num);
	}
}

bool CBaseEntity::ComputeHitboxSurroundingBox(Vector *pVecWorldMins, Vector *pVecWorldMaxs, matrix3x4_t* pSourceMatrix, bool use_valve)
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->_m_pStudioHdr->pHitboxSet(GetHitboxSet());
	if (!set || !set->numhitboxes)
		return false;

	matrix3x4_t *hitboxbones = pSourceMatrix;
	if (!pSourceMatrix)
	{
		auto cache = GetCachedBoneData();
		hitboxbones = (matrix3x4_t*)cache->Base();
	}

	// Compute a box in world space that surrounds this entity
	pVecWorldMins->Init(FLT_MAX, FLT_MAX, FLT_MAX);
	pVecWorldMaxs->Init(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	if (!use_valve)
	{
		// sharklaser's fix for valve's broken version of this function
		for (int i = 0; i < set->numhitboxes; i++)
		{
			mstudiobbox_t *pbox = set->pHitbox(i);
			Vector _minsmaxs[2];
			GetBoneTransformed(hitboxbones, pbox, nullptr, nullptr, nullptr, _minsmaxs);
			pVecWorldMins->x = fmin(pVecWorldMins->x, _minsmaxs[0].x);
			pVecWorldMins->y = fmin(pVecWorldMins->y, _minsmaxs[0].y);
			pVecWorldMins->z = fmin(pVecWorldMins->z, _minsmaxs[0].z);
			pVecWorldMaxs->x = fmax(pVecWorldMaxs->x, _minsmaxs[1].x);
			pVecWorldMaxs->y = fmax(pVecWorldMaxs->y, _minsmaxs[1].y);
			pVecWorldMaxs->z = fmax(pVecWorldMaxs->z, _minsmaxs[1].z);
		}
	}
	else
	{
		Vector vecBoxAbsMins, vecBoxAbsMaxs;

		for (int i = 0; i < set->numhitboxes; i++)
		{
			mstudiobbox_t *pbox = set->pHitbox(i);

			TransformAABB(hitboxbones[pbox->bone], pbox->bbmin, pbox->bbmax, vecBoxAbsMins, vecBoxAbsMaxs);
			pVecWorldMins->x = fmin(pVecWorldMins->x, vecBoxAbsMins.x);
			pVecWorldMins->y = fmin(pVecWorldMins->y, vecBoxAbsMins.y);
			pVecWorldMins->z = fmin(pVecWorldMins->z, vecBoxAbsMins.z);
			pVecWorldMaxs->x = fmax(pVecWorldMaxs->x, vecBoxAbsMaxs.x);
			pVecWorldMaxs->y = fmax(pVecWorldMaxs->y, vecBoxAbsMaxs.y);
			pVecWorldMaxs->z = fmax(pVecWorldMaxs->z, vecBoxAbsMaxs.z);
		}
	}

	return true;
}

bool CBaseEntity::HasC4()
{
	using func_t = bool(__thiscall*)(void*);
	return StaticOffsets.GetOffsetValueByType<func_t>(_HasC4)(this);
}

float CBaseEntity::m_fMolotovDamageTime()
{
	return *(float*)((DWORD)this + g_NetworkedVariables.Offsets.m_fMolotovDamageTime);
}

float CBaseEntity::FlashbangTime()
{
	return *(float*)((DWORD)this + StaticOffsets.GetOffsetValue(_FlashbangTime));
}

bool CBaseEntity::IsEnemy(CBaseEntity *pLocal)
{
	if (IsPlayer() && !IsSpectating())
	{
		if (pLocal->GetTeam() != GetTeam() && GetTeam() != TEAM_GOTV)
			return true;

		if (mp_teammates_are_enemies.GetVar()->GetInt() == 1)
			return true;
	}

	return false;
}

void CBaseEntity::UpdatePartition()
{
	auto collisionprop = GetCollideable();
	if (collisionprop)
	{
		StaticOffsets.GetOffsetValueByType<void(__thiscall*)(void*)>(_UpdatePartition)(collisionprop);
	}
}

bool CBaseEntity::CanUseFastPath()
{
	return *(bool*)((DWORD)this + StaticOffsets.GetOffsetValue(_CanUseFastPath));
}

void CBaseEntity::SetCanUseFastPath(bool val)
{
	*(bool*)((DWORD)this + StaticOffsets.GetOffsetValue(_CanUseFastPath)) = val;
}

int CBaseEntity::GetExplodeEffectTickBegin() const
{
	return *(int*)((DWORD)this + g_NetworkedVariables.Offsets.m_nExplodeEffectTickBegin);
}

datamap_t* CBaseEntity::GetPredDescMap()
{
	return StaticOffsets.GetVFuncByType<datamap_t*(__thiscall*)(CBaseEntity*)>(_GetPredDescMap, this)(this);
}

void* CBaseEntity::GetPredictedFrame(int framenumber)
{
	return StaticOffsets.GetOffsetValueByType<void*(__thiscall*)(CBaseEntity*, int)>(_GetPredictedFrame)(this, framenumber);
}

void CBaseEntity::SetCheckUntouch(bool check)
{
	// Invalidate touchstamp
	if (check)
	{
		SetTouchStamp(GetTouchStamp() + 1);
		if (!IsEFlagSet(EFL_CHECK_UNTOUCH))
		{
			AddEFlags(EFL_CHECK_UNTOUCH);
		}
	}
	else
	{
		RemoveEFlags(EFL_CHECK_UNTOUCH);
	}
}

int CBaseEntity::GetTouchStamp()
{
	return *StaticOffsets.GetOffsetValueByType<int*>(StaticOffsetName::_touchStamp, this);
}

void CBaseEntity::SetTouchStamp(int stamp)
{
	*StaticOffsets.GetOffsetValueByType<int*>(StaticOffsetName::_touchStamp, this) = stamp;
}

void CBaseEntity::DoLocalPlayerPrePrediction()
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*)>(_doLocalPlayerPrePrediction)(this);
}

void CBaseEntity::MoveToLastReceivedPosition(int i)
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*, int)>(_MoveToLastReceivedPosition)(this, i);
}

void CBaseEntity::PhysicsCheckForEntityUntouch()
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*)>(_PhysicsCheckForEntityUntouch)(this);
}

void CBaseEntity::PhysicsTouchTriggers(const Vector *pPrevAbsOrigin)
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CBaseEntity*/*, const Vector**/)>(_PhysicsTouchTriggers)(this/*, pPrevAbsOrigin*/);
}

bool CBaseEntity::GetUnknownEntityPredictionBool()
{
	return *StaticOffsets.GetOffsetValueByType<bool*>(StaticOffsetName::_UnknownEntityPredictionBool, this);
}

void CBaseEntity::SetUnknownEntityPredictionBool(bool val)
{
	*StaticOffsets.GetOffsetValueByType<bool*>(StaticOffsetName::_UnknownEntityPredictionBool, this) = val;
}

int CBaseEntity::GetFinalPredictedTick()
{
	return *StaticOffsets.GetOffsetValueByType<int*>(StaticOffsetName::_m_nFinalPredictedTick, this);
}

void CBaseEntity::SetFinalPredictedTick(int tick)
{
	*StaticOffsets.GetOffsetValueByType<int*>(StaticOffsetName::_m_nFinalPredictedTick, this) = tick;
}

void* CBaseEntity::GetFirstPredictedFrame()
{
	return (void*) *StaticOffsets.GetOffsetValueByType<DWORD*>(StaticOffsetName::_m_pFirstPredictedFrame, this);
}

void CBaseEntity::VPhysicsCompensateForPredictionErrors(void* frame)
{
	StaticOffsets.GetVFuncByType<void(__thiscall*)(CBaseEntity*, void*)>(_VPhysicsCompensateForPredictionErrorsVMT, this)(this, frame);
}

// Causes an assert to happen if bones or attachments are used while this is false.
struct BoneAccess
{
	BoneAccess()
	{
		bAllowBoneAccessForNormalModels = false;
		bAllowBoneAccessForViewModels = false;
		tag = NULL;
	}

	bool bAllowBoneAccessForNormalModels;
	bool bAllowBoneAccessForViewModels;
	char const *tag;
};

// (static function)
void CBaseEntity::PushAllowBoneAccess(bool bAllowForNormalModels, bool bAllowForViewModels, char const *tagPush)
{
	if (!ThreadInMainThread())
	{
		return;
	}
	BoneAccess *g_BoneAcessBase = StaticOffsets.GetOffsetValueByType<BoneAccess*>(_g_BoneAcessBase);
	CUtlVector< BoneAccess >* g_BoneAccessStack = StaticOffsets.GetOffsetValueByType<CUtlVector< BoneAccess >*>(_g_BoneAccessStack);

	BoneAccess save = *g_BoneAcessBase;
	g_BoneAccessStack->AddToTail(save);

	g_BoneAcessBase->bAllowBoneAccessForNormalModels = bAllowForNormalModels;
	g_BoneAcessBase->bAllowBoneAccessForViewModels = bAllowForViewModels;
	g_BoneAcessBase->tag = tagPush;
}

void CBaseEntity::PopBoneAccess(char const *tagPop)
{
	if (!ThreadInMainThread())
	{
		return;
	}
	// Validate that pop matches the push
	BoneAccess *g_BoneAcessBase = StaticOffsets.GetOffsetValueByType<BoneAccess*>(_g_BoneAcessBase);
	CUtlVector< BoneAccess >& g_BoneAccessStack = *StaticOffsets.GetOffsetValueByType<CUtlVector< BoneAccess >*>(_g_BoneAccessStack);

	int lastIndex = g_BoneAccessStack.Count() - 1;
	if (lastIndex < 0)
	{
		return;
	}
	*g_BoneAcessBase = g_BoneAccessStack[lastIndex];
	g_BoneAccessStack.Remove(lastIndex);
}

int CBaseEntity::SaveData(/*const char *context, */int slot, int type)
{
	return StaticOffsets.GetOffsetValueByType<int(__thiscall*)(CBaseEntity*, CBaseEntity*, int, int)>(_SaveData)(this, this, slot, type);
}

CBaseEntity::AutoAllowBoneAccess::AutoAllowBoneAccess(bool bAllowForNormalModels, bool bAllowForViewModels)
{
	PushAllowBoneAccess(bAllowForNormalModels, bAllowForViewModels, (char const *)1);
}

CBaseEntity::AutoAllowBoneAccess::~AutoAllowBoneAccess()
{
	PopBoneAccess((char const *)1);
}

bool CBaseEntity::DelayUnscope(float& fov)
{
	return StaticOffsets.GetOffsetValueByType<bool(__thiscall*)(CBaseEntity*, float&)>(_DelayUnscope)(this, fov);
}

bool& CBaseEntity::IsFireBurning()
{
	return *(bool*)((DWORD)this + g_NetworkedVariables.Offsets.m_bFireIsBurning);
}

void CBaseEntity::GetFireDelta(Vector& dest, int index)
{
	int* fire_xdelta = (int*)((DWORD)this + g_NetworkedVariables.Offsets.m_fireXDelta);
	int* fire_ydelta = (int*)((DWORD)this + g_NetworkedVariables.Offsets.m_fireYDelta);
	int* fire_zdelta = (int*)((DWORD)this + g_NetworkedVariables.Offsets.m_fireZDelta);
	dest.x = (float)fire_xdelta[index];
	dest.y = (float)fire_ydelta[index];
	dest.z = (float)fire_zdelta[index];
}

Vector& CBaseEntity::GetSurroundingMins()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecSpecifiedSurroundingMins);
}

Vector& CBaseEntity::GetSurroundingMaxs()
{
	return *(Vector*)((DWORD)this + g_NetworkedVariables.Offsets.m_vecSpecifiedSurroundingMaxs);
}

CParticleProperty* CBaseEntity::ParticleProp()
{
	DWORD tmp = StaticOffsets.GetOffsetValue(_ParticleProp);
	return (CParticleProperty*)((DWORD)this + tmp + 4);//StaticOffsets.GetOffsetValueByType<CParticleProperty*>(_ParticleProp, this) - (8 / 4);
}

bool CEntIndexLessFunc::Less(CBaseEntity * const & lhs, CBaseEntity * const & rhs, void *pContext)
{
	int e1 = lhs->entindex();
	int e2 = rhs->entindex();

	// if an entity has an invalid entity index, then put it at the end of the list
	e1 = (e1 == -1) ? MAX_EDICTS : e1;
	e2 = (e2 == -1) ? MAX_EDICTS : e2;

	return e1 < e2;
}

CBaseEntity* GetHudPlayer()
{
	return StaticOffsets.GetOffsetValueByType< CBaseEntity*(__cdecl*)(void) >(_GetHudPlayer)();
}

__declspec (naked) void __stdcall SurvivalCalcView(void* survivalgamerules, CBaseEntity* _Entity, Vector& eyeOrigin, QAngle& eyeAngles)
{
	__asm 
	{
		push ebp
		mov ebp, esp
		sub esp, 4
	}
	DWORD func;
	func = StaticOffsets.GetOffsetValue(_SurvivalCalcView);
	__asm 
	{
		mov eax, func
		sub esp, 12
		push [ebp + 20] //eyeAngles
		push [ebp + 16] //eyeOrigin
		push [ebp + 12] //_Entity
		mov ecx, [ebp + 8] //survivalgamerules
		call eax
		mov esp, ebp
		pop ebp
		ret
	}
}

const CBaseEntity* EntityFromEntityHandle(const IHandleEntity* pConstHandleEntity)
{
	IHandleEntity* pHandleEntity = const_cast< IHandleEntity* >(pConstHandleEntity);
	IClientUnknown* pUnk		 = (IClientUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
}

CBaseEntity* EntityFromEntityHandle(IHandleEntity* pHandleEntity)
{
	IClientUnknown* pUnk = (IClientUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
}

RenderableTranslucencyType_t GetTranslucencyType(ClientRenderHandle_t handle)
{
	return GetVFunc< RenderableTranslucencyType_t(__thiscall*)(DWORD, ClientRenderHandle_t) >((void*)g_pClientLeafSystem, 0x8C)(g_pClientLeafSystem, handle);
}

CStudioHdr::CActivityToSequenceMapping* FindMapping(CStudioHdr* hdr)
{
	//TODO: move to NetworkedVariables.cpp
	//decrypts(0)
	static DWORD adr = FindMemoryPattern(ClientHandle, XorStr("55  8B  EC  83  E4  F8  81  EC  ??  ??  ??  ??  53  56  57  8B  F9  8B  17  83  BA  ??  ??  ??  ??  ??  74  34  83  7F  04  00  75  2E  83  BA  ??  ??  ??  ??  ??  75  04  33  C0  EB  0C  8B  0D  ??  ??  ??  ??  52  8B  01  FF  50  68"));
	//encrypts(0)
	if (!adr)
		exit(EXIT_SUCCESS);

	return ((CStudioHdr::CActivityToSequenceMapping*(__thiscall*)(CStudioHdr*))adr)(hdr);
}

CStudioHdr::CActivityToSequenceMapping* getEmptyMapping()
{
	//TODO: move to NetworkedVariables.cpp
	//decrypts(0)
	static DWORD adr = FindMemoryPattern(ClientHandle, XorStr("87  0A  5F  5E  5B  8B  E5  5D  C3  B8  ??  ??  ??  ??  5F  5E  5B  8B  E5  5D  C3"));
	//encrypts(0)
	if (!adr)
		exit(EXIT_SUCCESS);
	static DWORD offset = NULL;
	if (!offset)
	{
		offset = *(DWORD*)(adr + 10);
	}
	return (CStudioHdr::CActivityToSequenceMapping*)offset;
}

int CBaseEntity::SelectWeightedSequenceFromModifiers(int activity, CUtlSymbol *pActivityModifiers, int iModifierCount)
{
	CStudioHdr *pstudiohdr = GetModelPtr();
	if (!pstudiohdr || !pstudiohdr->m_pVModel)
		pstudiohdr = nullptr;

	if (!pstudiohdr->m_pActivityToSequence)
		pstudiohdr->m_pActivityToSequence = FindMapping(pstudiohdr);

	return pstudiohdr->m_pActivityToSequence->SelectWeightedSequenceFromModifiers(pstudiohdr, activity, pActivityModifiers, iModifierCount, this);
}

bool IsInPrediction()
{
	CBaseEntity** pPredictionPlayer = StaticOffsets.GetOffsetValueByType<CBaseEntity**>(_predictionplayer);
	return *pPredictionPlayer != NULL;
}

//static CStudioHdr::CActivityToSequenceMapping emptyMapping;

// double-check that the data I point to hasn't changed
bool CStudioHdr::CActivityToSequenceMapping::ValidateAgainst(const CStudioHdr * __restrict pstudiohdr) __restrict
{
	return (this == /*&emptyMapping*/getEmptyMapping() ||
		(m_pStudioHdr == pstudiohdr->_m_pStudioHdr && m_expectedVModel == pstudiohdr->GetVirtualModel()));
}

/// Force Initialize() to occur again, even if it has already occured.
void CStudioHdr::CActivityToSequenceMapping::Reinitialize(CStudioHdr *pstudiohdr)
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CStudioHdr::CActivityToSequenceMapping*, CStudioHdr*)>(_CActivityToSequenceMapping_Reinitialize)(this, pstudiohdr);
}

void IndexModelSequences(CStudioHdr *pstudiohdr)
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(CStudioHdr*)>(_IndexModelSequences)(pstudiohdr);
}

void VerifySequenceIndex(CStudioHdr *pstudiohdr)
{
	if (!pstudiohdr)
	{
		return;
	}

	if (pstudiohdr->GetActivityListVersion() < *StaticOffsets.GetOffsetValueByType<int*>(_g_nActivityListVersion))
	{
		// this model's sequences have not yet been indexed by activity
		IndexModelSequences(pstudiohdr);
	}
}

int CStudioHdr::CActivityToSequenceMapping::SelectWeightedSequence(CStudioHdr *pstudiohdr, int activity, int curSequence)
{
	return StaticOffsets.GetOffsetValueByType<int(__thiscall*)(CStudioHdr::CActivityToSequenceMapping*, CStudioHdr*, int, int)>(_SelectWeightedSequence)(this, pstudiohdr, activity, curSequence);
}

//NOTE: parent wasn't an activity but cba to rebuild GetSequenceActivity yet
int CStudioHdr::CActivityToSequenceMapping::SelectWeightedSequenceFromModifiers(CStudioHdr *pstudiohdr, int activity, CUtlSymbol *pActivityModifiers, int iModifierCount, CBaseEntity* parent)
{
	if (!pstudiohdr->SequencesAvailable())
	{
		return -1;
	}

	VerifySequenceIndex(pstudiohdr);

	if (pstudiohdr->GetNumSeq() == 1)
	{
		return (GetSequenceActivity(parent, 0) == activity) ? 0 : -1;
	}
	
	//added in nov 18, 2019 patch
	if (!iModifierCount)
		return SelectWeightedSequence(pstudiohdr, activity, -1);
	
	if (!ValidateAgainst(pstudiohdr))
	{
#ifdef _DEBUG
		if (IsDebuggerPresent())
			DebugBreak();
#endif
		//AssertMsg1(false, "CStudioHdr %s has changed its vmodel pointer without reinitializing its activity mapping! Now performing emergency reinitialization.", pstudiohdr->pszName());
		//ExecuteOnce(DebuggerBreakIfDebugging());
		Reinitialize(pstudiohdr);
	}

	// a null m_pSequenceTuples just means that this studio header has no activities.
	if (!m_pSequenceTuples)
		return -1;

	// get the data for the given activity
	HashValueType dummy(activity, 0, 0, 0);
	UtlHashHandle_t handle = m_ActToSeqHash.Find(dummy);
	if (!m_ActToSeqHash.IsValidHandle(handle))
		return -1;

	const HashValueType * __restrict actData = &m_ActToSeqHash[handle];

	// go through each sequence and give it a score
	CUtlVector<int> topScoring(actData->count, actData->count);
#if 1
	int local_8 = -1;
	if (actData->count > 0)
	{
		int topScoringCount = topScoring.Count();
		bool bFoundMatchingModifier = false;

		for (int i = 0; i < actData->count; ++i)
		{
			int k = 0;
			int iVar8 = 0;
			SequenceTuple * __restrict sequenceInfo = m_pSequenceTuples + actData->startingIdx + i;
			if (0 < sequenceInfo->iNumActivityModifiers)
			{
				for (int k = 0; k < sequenceInfo->iNumActivityModifiers; ++k)
				{
					bFoundMatchingModifier = false;
					if (0 < iModifierCount)
					{
						for (int m = 0; m < iModifierCount; ++m)
						{
							if (sequenceInfo->pActivityModifiers[k] == pActivityModifiers[m])
							{
								bFoundMatchingModifier = true;
								break;
							}
						};
					}
					int iUnknown = sequenceInfo->iUnknown[k];
					if (iUnknown == 0)
					{
						iUnknown = -1;
					LAB_101b07e7:
						if (bFoundMatchingModifier)
						{
							iUnknown = 2;
						}
					LAB_101b07f1:
						iVar8 = iVar8 + iUnknown;
					}
					else
					{
						if (iUnknown == 1)
						{
							iUnknown = 0;
							goto LAB_101b07e7;
						}
						if (iUnknown == 2)
						{
							iUnknown = (uint)bFoundMatchingModifier - 1;
							goto LAB_101b07f1;
						}
						if (iUnknown == 3)
						{
							bFoundMatchingModifier = (bool)(bFoundMatchingModifier ^ 1);
						LAB_101b07fe:
							if (bFoundMatchingModifier) goto LAB_101b0863;
							break;
						}
						if (iUnknown == 4) goto LAB_101b07fe;
					}
				}
			}
			if (local_8 <= iVar8)
			{
				if (local_8 < iVar8)
					topScoringCount = 0;

				topScoring.SetSizeDirect(topScoringCount);
				//VALVE IS DOING THIS FOR SOME REASON, MEMORY LEAK!!!!!!!!!

				local_8 = iVar8;
				if (0 < sequenceInfo->weight)
				{
					for (int w = 0; w < sequenceInfo->weight; ++w)
					{
						topScoring.AddToTail(sequenceInfo->seqnum);
					}
					topScoringCount = topScoring.Count();
				}
			}
		LAB_101b0863:
			int nag;
		}
	}

	if (topScoring.Count())
		return topScoring[RandomInt(0, topScoring.Count() - 1)];

	return -1;

#else
	for (int i = 0; i < actData->count; i++)
	{
		SequenceTuple * __restrict sequenceInfo = m_pSequenceTuples + actData->startingIdx + i;
		int score = 0;
		// count matching activity modifiers
		for (int iUnknown = 0; iUnknown < iModifierCount; iUnknown++)
		{
			int num_modifiers = sequenceInfo->iNumActivityModifiers;
			for (int k = 0; k < num_modifiers; k++)
			{
				if (sequenceInfo->pActivityModifiers[k] == pActivityModifiers[iUnknown])
				{
					score++;
					break;
				}
			}
		}
		if (score > top_score)
		{
			topScoring.RemoveAll();
			topScoring.AddToTail(sequenceInfo->seqnum);
			top_score = score;
		}
	}

	// randomly pick between the highest scoring sequences ( NOTE: this method of selecting a sequence ignores activity weights )
	//FIXME
	//if (IsInPrediction())
	//{
	//	return topScoring[SharedRandomInt("SelectWeightedSequence", 0, topScoring.Count() - 1)];
	//}
	return topScoring[RandomInt(0, topScoring.Count() - 1)];
#endif
}

bool animstate_pose_param_cache_t::Init(CBaseEntity* pPlayer, char* name)
{
	Interfaces::MDLCache->BeginLock();
	m_szPoseParameter = name;
	int pose_index	= pPlayer->LookupPoseParameter(pPlayer->GetModelPtr(), name);
	m_iPoseParameter  = pose_index;
	if (pose_index != -1)
		m_bInitialized = true;
	Interfaces::MDLCache->EndLock();
	return m_bInitialized;
}

void animstate_pose_param_cache_t::SetValue(CBaseEntity* pPlayer, float flValue)
{
	bool initialized = m_bInitialized;

	if (!m_bInitialized)
		initialized = Init(pPlayer, m_szPoseParameter);

	if (initialized && pPlayer)
	{
		Interfaces::MDLCache->BeginLock();
		pPlayer->SetPoseParameterGame(pPlayer->GetModelPtr(), flValue, m_iPoseParameter);
		Interfaces::MDLCache->EndLock();
	}
}

float animstate_pose_param_cache_t::GetValue(CBaseEntity *pPlayer)
{
	if (!m_bInitialized)
		Init(pPlayer, m_szPoseParameter);

	if (m_bInitialized && pPlayer)
		return pPlayer->GetPoseParameter(m_iPoseParameter);
	
	return 0.0f;
}

void C_CSGOPlayerAnimState::IncrementLayerCycle(int layer, bool dont_clamp_cycle)
{
	C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(layer);
	if (!pLayer || fabs(pLayer->_m_flPlaybackRate) <= 0.0f)
		return;

	float newcycle = (pLayer->_m_flPlaybackRate * m_flLastClientSideAnimationUpdateTimeDelta) + pLayer->_m_flCycle;

	if (!dont_clamp_cycle && newcycle >= 1.0f)
		newcycle = 0.999f;

	newcycle -= (float)(int)newcycle; //round to integer

	if (newcycle < 0.0f)
		newcycle += 1.0f;

	if (newcycle > 1.0f)
		newcycle -= 1.0f;

	pLayer->SetCycle(newcycle);
}

void C_CSGOPlayerAnimState::SetFeetCycle(float cycle)
{
	cycle -= (float)(int)cycle; //round to integer

	if (cycle < 0.0f)
		cycle += 1.0f;

	if (cycle > 1.0f)
		cycle -= 1.0f;

	m_flFeetCycle = cycle;
}

void C_CSGOPlayerAnimState::SetLayerCycle(int layer, float cycle)
{
	C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(layer);
	if (!pLayer)
		return;

	cycle -= (float)(int)cycle; //round to integer

	if (cycle < 0.0f)
		cycle += 1.0f;

	if (cycle > 1.0f)
		cycle -= 1.0f;

	pLayer->SetCycle(cycle);
}

void C_CSGOPlayerAnimState::SetLayerWeight(int layer, float weight)
{
	C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(layer);
	if (!pLayer)
		return;

	weight = clamp(weight, 0.0f, 1.0f);

	pLayer->SetWeight(weight);
}

void C_CSGOPlayerAnimState::SetLayerSequence(int layer, int sequence)
{
	if (sequence >= 2)
	{
		Interfaces::MDLCache->BeginLock();
		C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(layer);
		if (pLayer)
		{
			pLayer->SetSequence(sequence);
			pLayer->_m_flPlaybackRate = pBaseEntity->GetLayerSequenceCycleRate(pLayer, sequence);
			pLayer->SetCycle(0.0f);
			pLayer->SetWeight(0.0f);
			UpdateLayerOrderPreset(0.0f, layer, sequence);
		}
		Interfaces::MDLCache->EndLock();
	}
}

float C_CSGOPlayerAnimState::GetLayerCycle(int layer)
{
	C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(layer);
	if (!pLayer)
		return 0.0f;

	return pLayer->_m_flCycle;
}

float C_CSGOPlayerAnimState::GetLayerWeight(int layer)
{
	C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(layer);
	if (!pLayer)
		return 0.0f;

	return pLayer->m_flWeight;
}

void C_CSGOPlayerAnimState::IncrementLayerCycleGeneric(int layer)
{
	C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(layer);
	if (!pLayer || fabs(pLayer->_m_flPlaybackRate) <= 0.0f)
		return;

	float newcycle = (pLayer->_m_flPlaybackRate * m_flLastClientSideAnimationUpdateTimeDelta) + pLayer->_m_flCycle;

	newcycle -= newcycle;

	if (newcycle < 0.0f)
		newcycle += 1.0f;

	if (newcycle > 1.0f)
		newcycle -= 1.0f;

	pLayer->SetCycle(newcycle);
}

void C_CSGOPlayerAnimState::SetupWeaponAction()
{
	//((void(__thiscall*)(C_CSGOPlayerAnimState*))StaticOffsets.GetOffsetValue(_SetupWeaponAction))(this);

	C_AnimationLayer *layer1 = pBaseEntity->GetAnimOverlay(1);
	if (layer1)
	{
		if (layer1->m_flWeight > 0.0f)
		{
			IncrementLayerCycle(1, false);
			LayerWeightAdvance(1);
		}

		if (pActiveWeapon)
		{
			CBaseEntity* worldmodel = Interfaces::ClientEntList->GetBaseEntityFromHandle(pActiveWeapon->GetWorldModelHandle());
			if (worldmodel)
			{
				Interfaces::MDLCache->BeginLock();
				if (layer1->m_iPriority <= 0 || layer1->m_flWeight <= 0.0f)
				{
					worldmodel->SetSequenceVMT(0);
					worldmodel->SetCycle(0.0f);
					worldmodel->SetPlaybackRate(0.0f);
				}
				else
				{
					worldmodel->SetSequenceVMT(layer1->m_iPriority);
					worldmodel->SetCycle(layer1->_m_flCycle);
					worldmodel->SetPlaybackRate(layer1->_m_flPlaybackRate);
				}
				Interfaces::MDLCache->EndLock();
			}
		}
	}
}

void C_CSGOPlayerAnimState::UpdateAimLayer(AimLayer *array, float timedelta, float multiplier, bool somebool)
{
	float v4;
	bool v5;
	float v7;
	float v8;

	if (somebool)
	{
		v4 = array->m_flUnknown0 + timedelta;
		array->m_flTotalTime = 0.0;
		v5 = v4 < array->m_flUnknown1;
		array->m_flUnknown0 = v4;

		if (v5)
			return;

		if ((1.0f - array->m_flWeight) <= multiplier)
		{
			if (-multiplier <= (1.0f - array->m_flWeight))
			{
				array->m_flWeight = 1.0f;
				return;
			}
			array->m_flWeight = array->m_flWeight - multiplier;
			return;
		}
		array->m_flWeight = array->m_flWeight + multiplier;
		return;
	}

	v7 = array->m_flTotalTime + timedelta;
	array->m_flUnknown0 = 0.0;
	v5 = v7 < array->m_flUnknown2;
	array->m_flTotalTime = v7;

	if (v5)
		return;

	if (-array->m_flWeight > multiplier)
	{
		array->m_flWeight = array->m_flWeight + multiplier;
		return;
	}

	if (-multiplier > -array->m_flWeight)
	{
		array->m_flWeight = array->m_flWeight - multiplier;
		return;
	}
	array->m_flWeight = 0.0;
}

void C_CSGOPlayerAnimState::SetupAimMatrix()
{
	START_PROFILING
#ifdef _DEBUG
	((void(__thiscall*)(C_CSGOPlayerAnimState*))StaticOffsets.GetOffsetValue(_SetupAimMatrix))(this);
	END_PROFILING;
	return;
#endif

	Interfaces::MDLCache->BeginLock();
	if (m_fDuckAmount <= 0.0f || m_fDuckAmount >= 1.0f)
	{
		bool is_walking = pBaseEntity && pBaseEntity->GetIsWalking();
		bool is_scoped;
		float speedmultiplier;
		if (pBaseEntity && pBaseEntity->IsScoped())
		{
			speedmultiplier = 4.2f;
			is_scoped = true;
		}
		else
		{
			speedmultiplier = 0.8f;
			is_scoped = false;
		}

		speedmultiplier *= m_flLastClientSideAnimationUpdateTimeDelta;

		if (is_scoped)
		{
			m_AimLayers.layers[0].m_flTotalTime = m_AimLayers.layers[0].m_flUnknown2;
			m_AimLayers.layers[1].m_flTotalTime = m_AimLayers.layers[1].m_flUnknown2;
			m_AimLayers.layers[2].m_flTotalTime = m_AimLayers.layers[2].m_flUnknown2;
		}

		bool v10 = is_walking && !is_scoped && m_flRunningSpeed > 0.7f && m_flSpeedNormalized < 0.7f;
		UpdateAimLayer(m_AimLayers.layers, m_flLastClientSideAnimationUpdateTimeDelta, speedmultiplier, v10);

		bool v12 = !is_scoped && m_flSpeedNormalized >= 0.7;
		UpdateAimLayer(&m_AimLayers.layers[1], m_flLastClientSideAnimationUpdateTimeDelta, speedmultiplier, v12);

		bool v14 = !is_scoped && m_flDuckingSpeed >= 0.5;
		UpdateAimLayer(&m_AimLayers.layers[2], m_flLastClientSideAnimationUpdateTimeDelta, speedmultiplier, v14);
	}

	float pose_crouch_idle = 1.0f;
	float strafe_weight = 1.0f;
	float aimlayer0_weight = m_AimLayers.layers[0].m_flWeight;
	float aimlayer1_weight = m_AimLayers.layers[1].m_flWeight;
	float aimlayer2_weight = m_AimLayers.layers[2].m_flWeight;
	if (aimlayer0_weight >= 1.0f)
		pose_crouch_idle = 0.0f;
	if (aimlayer1_weight >= 1.0f)
	{
		pose_crouch_idle = 0.0f;
		aimlayer0_weight = 0.0;
	}
	if (aimlayer2_weight >= 1.0f)
	{
		strafe_weight = 0.0f;
	}
	if (m_fDuckAmount < 1.0f)
	{
		if (m_fDuckAmount <= 0.0f)
		{
			strafe_weight = 0.0f;
			aimlayer2_weight = 0.0f;
		}
	}
	else
	{
		pose_crouch_idle = 0.0f;
		aimlayer0_weight = 0.0f;
		aimlayer1_weight = 0.0f;
	}

	float duck_pose = 1.0f - m_fDuckAmount;
	float pose_strafe_yaw = m_fDuckAmount * strafe_weight;
	float pose_move_blend_walk = m_fDuckAmount * aimlayer2_weight;

	float pose_stand_run = duck_pose * aimlayer0_weight;
	float pose_crouch_walk = duck_pose * aimlayer1_weight;

	if (pose_strafe_yaw < 1.0f && pose_move_blend_walk < 1.0f && pose_stand_run < 1.0f && pose_crouch_walk < 1.0f)
	{
		pose_crouch_idle = 1.0f;
	}

	m_arrPoseParameters[11].SetValue(pBaseEntity, pose_crouch_idle); //aim_blend_crouch_idle
	m_arrPoseParameters[14].SetValue(pBaseEntity, pose_stand_run); //aim_blend_stand_run
	m_arrPoseParameters[15].SetValue(pBaseEntity, pose_crouch_walk); //aim_blend_crouch_walk
	m_arrPoseParameters[12].SetValue(pBaseEntity, pose_strafe_yaw); //strafe_yaw
	m_arrPoseParameters[16].SetValue(pBaseEntity, pose_move_blend_walk); //move_blend_walk

	char animation[64];
	//decrypts(0)
	sprintf_s(animation, XorStr("%s_aim"), GetWeaponMoveAnimation());
	//encrypts(0)

	int aim_sequence = pBaseEntity->LookupSequence(animation);
	C_AnimationLayer *layer0 = pBaseEntity->GetAnimOverlay(0);
	if (layer0)
	{
		if (pActiveWeapon)
		{
			CBaseEntity* worldmodel = Interfaces::ClientEntList->GetBaseEntityFromHandle(pActiveWeapon->GetWorldModelHandle());
			CBaseEntity* sourcemodel = pBaseEntity;
			int sourcesequence = aim_sequence;
			if (worldmodel && layer0->m_iPriority != -1)
			{
				sourcemodel = worldmodel;
				sourcesequence = layer0->m_iPriority;
			}

			if (sourcesequence > 0)
			{
				float yawmin_idle = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMIN_IDLE, -58.0f);
				float yawmax_idle = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMAX_IDLE, 58.0f);
				float yawmin_walk = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMIN_WALK, yawmin_idle);
				float yawmax_walk = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMAX_WALK, yawmax_idle);
				float yawmin_run = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMIN_RUN, yawmin_walk);
				float yawmax_run = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMAX_RUN, yawmax_walk);
				float yawmin_crouchidle = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMIN_CROUCHIDLE, -58.0f);
				float yawmax_crouchidle = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMAX_CROUCHIDLE, 58.0f);
				float yawmin_crouchwalk = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMIN_CROUCHWALK, yawmin_crouchidle);
				float yawmax_crouchwalk = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_YAWMAX_CROUCHWALK, yawmax_crouchidle);

				float pose_blend_stand_run = m_arrPoseParameters[14].GetValue(pBaseEntity); //aim_blend_stand_run
				float pose_blend_crouch_walk = m_arrPoseParameters[15].GetValue(pBaseEntity); //aim_blend_crouch_walk
				pose_move_blend_walk = m_arrPoseParameters[16].GetValue(pBaseEntity); //move_blend_walk

				if (!pBaseEntity)
				{
					pose_move_blend_walk = 0.0f;
				}

				float v62 = ((yawmax_crouchwalk - yawmax_crouchidle) * pose_move_blend_walk) + yawmax_crouchidle;
				float v63 = ((yawmin_run - (((yawmin_walk - yawmin_idle) * pose_blend_stand_run) + yawmin_idle)) * pose_blend_crouch_walk)
					+ (((yawmin_walk - yawmin_idle) * pose_blend_stand_run) + yawmin_idle);

				m_flMinYaw = (((((yawmin_crouchwalk - yawmin_crouchidle) * pose_move_blend_walk) + yawmin_crouchidle)
					- v63)
					* m_fDuckAmount)
					+ v63;

				m_flMaxYaw = ((v62
					- (((yawmax_run
						- (((yawmax_walk - yawmax_idle) * pose_blend_stand_run) + yawmax_idle))
						* pose_blend_crouch_walk)
						+ (((yawmax_walk - yawmax_idle) * pose_blend_stand_run) + yawmax_idle)))
					* m_fDuckAmount)
					+ (((yawmax_run
						- (((yawmax_walk - yawmax_idle) * pose_blend_stand_run) + yawmax_idle))
						* pose_blend_crouch_walk)
						+ (((yawmax_walk - yawmax_idle) * pose_blend_stand_run) + yawmax_idle));

				float pitchmin_idle = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_PITCHMIN_IDLE, -90.0f);
				float pitchmax_idle = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_PITCHMAX_IDLE, 90.0f);
				float pitchmin_walkrun = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_PITCHMIN_WALKRUN, pitchmin_idle);
				float pitchmax_walkrun = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_PITCHMAX_WALKRUN, pitchmax_idle);
				float pitchmin_crouch = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_PITCHMIN_CROUCH, -90.0f);
				float pitchmax_crouch = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_PITCHMAX_CROUCH, 90.0f);
				float pitchmin_crouchwalk = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_PITCHMIN_CROUCHWALK, pitchmin_crouch);
				float pitchmax_crouchwalk = sourcemodel->GetAnySequenceAnimTag(sourcesequence, ANIMTAG_AIMLIMIT_PITCHMAX_CROUCHWALK, pitchmax_crouch);

				float v67 = (((((pitchmax_crouchwalk - pitchmax_crouch) * pose_move_blend_walk) + pitchmax_crouch)
					- (((pitchmax_walkrun - pitchmax_idle) * pose_blend_stand_run) + pitchmax_idle))
					* m_fDuckAmount)
					+ (((pitchmax_walkrun - pitchmax_idle) * pose_blend_stand_run) + pitchmax_idle);

				m_flMinPitch = (((((pitchmin_crouchwalk - pitchmin_crouch) * pose_move_blend_walk) + pitchmin_crouch)
					- (((pitchmin_walkrun - pitchmin_idle) * pose_blend_stand_run)
						+ pitchmin_idle))
					* m_fDuckAmount)
					+ (((pitchmin_walkrun - pitchmin_idle) * pose_blend_stand_run)
						+ pitchmin_idle);

				m_flMaxPitch = v67;
			}
		}
	}

	UpdateAnimLayer(0.0f, 0, aim_sequence, 1.0f, 0.0f);

	Interfaces::MDLCache->EndLock();
	END_PROFILING
}

void C_CSGOPlayerAnimState::SetupLean()
{
	START_PROFILING
#ifdef _DEBUG
	((void(__thiscall*)(C_CSGOPlayerAnimState*))StaticOffsets.GetOffsetValue(_SetupLean))(this);
	END_PROFILING
	return;
#endif

	float timedelta = Interfaces::Globals->curtime - m_flLastSetupLeanCurtime;
	if (timedelta > 0.025f)
	{
		if (timedelta >= 0.1f)
			timedelta = 0.1f;

		Vector vecVelocity = pBaseEntity->GetVelocity();
		m_flLastSetupLeanCurtime = Interfaces::Globals->curtime;
		m_vecSetupLeanVelocityDelta.x = (vecVelocity.x - m_vecLastSetupLeanVelocity.x) * (1.0f / timedelta);
		m_vecSetupLeanVelocityDelta.y = (vecVelocity.y - m_vecLastSetupLeanVelocity.y) * (1.0f / timedelta);
		m_vecSetupLeanVelocityDelta.z = 0.0f;
		m_vecLastSetupLeanVelocity = vecVelocity;
	}
	
	m_vecSetupLeanVelocityInterpolated = g_LagCompensation.GetSmoothedVelocity(m_flLastClientSideAnimationUpdateTimeDelta * 800.0f, m_vecSetupLeanVelocityDelta, m_vecSetupLeanVelocityInterpolated);

	Vector pseudoup = Vector(0.0f, 0.0f, 1.0f);
	QAngle angles;
	VectorAngles(m_vecSetupLeanVelocityInterpolated, pseudoup, angles);

	float speed = (m_vecSetupLeanVelocityInterpolated.Length() * (1.0f / _CS_PLAYER_SPEED_RUN)) * m_flSpeedNormalized;

	//*(float *)&animstate->m_bNotRunning ON SERVER
	m_flLeanWeight = (1.0f - m_flLadderCycle) * clamp(speed, 0.0f, 1.0f);

	float flGoalFeetLeanDelta = AngleNormalize(m_flGoalFeetYaw - angles.y);
	
	m_arrPoseParameters[0].SetValue(pBaseEntity, flGoalFeetLeanDelta);

	C_AnimationLayer *layer12 = pBaseEntity->GetAnimOverlay(12);
	if (!layer12 || layer12->_m_nSequence <= 0)
	{
		Interfaces::MDLCache->BeginLock();
		//decrypts(0)
		int seq = pBaseEntity->LookupSequence(XorStr("lean"));
		//encrypts(0)
		SetLayerSequence(12, seq);
		Interfaces::MDLCache->EndLock();
	}

	SetLayerWeight(12, m_flLeanWeight);

	//Other shit is done here on the server
	END_PROFILING
}

char *leanyawstr = new char[9]{ 22, 31, 27, 20, 37, 3, 27, 13, 0 }; /*lean_yaw*/
char *speedstr = new char[6]{ 9, 10, 31, 31, 30, 0 }; /*speed*/
char *ladderspeedstr = new char[13]{ 22, 27, 30, 30, 31, 8, 37, 9, 10, 31, 31, 30, 0 }; /*ladder_speed*/
char *ladderyawstr = new char[11]{ 22, 27, 30, 30, 31, 8, 37, 3, 27, 13, 0 }; /*ladder_yaw*/
char *moveyawstr = new char[9]{ 23, 21, 12, 31, 37, 3, 27, 13, 0 }; /*move_yaw*/
char *runstr = new char[4]{ 8, 15, 20, 0 }; /*run*/
char *bodyyawstr = new char[9]{ 24, 21, 30, 3, 37, 3, 27, 13, 0 }; /*body_yaw*/
char *bodypitchstr = new char[11]{ 24, 21, 30, 3, 37, 10, 19, 14, 25, 18, 0 }; /*body_pitch*/
char *deathyawstr = new char[10]{ 30, 31, 27, 14, 18, 37, 3, 27, 13, 0 }; /*death_yaw*/
char *standstr = new char[6]{ 9, 14, 27, 20, 30, 0 }; /*stand*/
char *jumpfallstr = new char[10]{ 16, 15, 23, 10, 37, 28, 27, 22, 22, 0 }; /*jump_fall*/
char *aimblendstandidlestr = new char[21]{ 27, 19, 23, 37, 24, 22, 31, 20, 30, 37, 9, 14, 27, 20, 30, 37, 19, 30, 22, 31, 0 }; /*aim_blend_stand_idle*/
char *aimblendcrouchidlestr = new char[22]{ 27, 19, 23, 37, 24, 22, 31, 20, 30, 37, 25, 8, 21, 15, 25, 18, 37, 19, 30, 22, 31, 0 }; /*aim_blend_crouch_idle*/
char *strafeyawstr = new char[11]{ 9, 14, 8, 27, 28, 31, 37, 3, 27, 13, 0 }; /*strafe_yaw*/
char *aimblendstandwalkstr = new char[21]{ 27, 19, 23, 37, 24, 22, 31, 20, 30, 37, 9, 14, 27, 20, 30, 37, 13, 27, 22, 17, 0 }; /*aim_blend_stand_walk*/
char *aimblendstandrunstr = new char[20]{ 27, 19, 23, 37, 24, 22, 31, 20, 30, 37, 9, 14, 27, 20, 30, 37, 8, 15, 20, 0 }; /*aim_blend_stand_run*/
char *aimblendcrouchwalkstr = new char[22]{ 27, 19, 23, 37, 24, 22, 31, 20, 30, 37, 25, 8, 21, 15, 25, 18, 37, 13, 27, 22, 17, 0 }; /*aim_blend_crouch_walk*/
char *moveblendwalkstr = new char[16]{ 23, 21, 12, 31, 37, 24, 22, 31, 20, 30, 37, 13, 27, 22, 17, 0 }; /*move_blend_walk*/
char *moveblendrunstr = new char[15]{ 23, 21, 12, 31, 37, 24, 22, 31, 20, 30, 37, 8, 15, 20, 0 }; /*move_blend_run*/
char *moveblendcrouchstr = new char[18]{ 23, 21, 12, 31, 37, 24, 22, 31, 20, 30, 37, 25, 8, 21, 15, 25, 18, 0 }; /*move_blend_crouch*/
bool _didDecrypt = false;

bool C_CSGOPlayerAnimState::CacheSequences()
{
#ifdef _DEBUG
	return ((bool(__thiscall*)(C_CSGOPlayerAnimState*))StaticOffsets.GetOffsetValue(_CacheSequences))(this);
#else
	if (!pBaseEntity)
		return false;

	if (m_nModelIndex != pBaseEntity->GetModelIndex())
	{
		m_iAnimsetVersion = 0;

		CUtlBuffer buffer(1024, 0, CUtlBuffer::TEXT_BUFFER);
		//decrypts(0)
		buffer.PutString(XorStr("keyvalues {\n"));
		//encrypts(0)

		if (Interfaces::ModelInfoClient->GetModelKeyValue(pBaseEntity->GetModel(), buffer))
		{
			buffer.PutString("\n}");
			auto keyValues = new KeyValues("");
			if (keyValues)
			{
				if (keyValues->LoadFromBuffer(Interfaces::ModelInfoClient->GetModelName(pBaseEntity->GetModel()), buffer.String()))
				{
					//decrypts(0)
					for (KeyValues *sub = keyValues->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
					{
						if (sub->FindKey(XorStr("animset_version"), false))
						{
							m_iAnimsetVersion = sub->GetInt(XorStr("animset_version"), 2);
							break;
						}
					}
					//encrypts(0)
				}

				keyValues->deleteThis();
			}
		}

		if (!_didDecrypt)
		{

			DecStr(leanyawstr, 8);
			DecStr(speedstr, 5);
			DecStr(ladderspeedstr, 12);
			DecStr(ladderyawstr, 10);
			DecStr(moveyawstr, 8);
			DecStr(runstr, 3);
			DecStr(bodyyawstr, 8);
			DecStr(bodypitchstr, 10);
			DecStr(deathyawstr, 9);
			DecStr(standstr, 5);
			DecStr(jumpfallstr, 9);
			DecStr(aimblendstandidlestr, 20);
			DecStr(aimblendcrouchidlestr, 21);
			DecStr(strafeyawstr, 10);
			DecStr(aimblendstandwalkstr, 20);
			DecStr(aimblendstandrunstr, 19);
			DecStr(aimblendcrouchwalkstr, 21);
			DecStr(moveblendwalkstr, 15);
			DecStr(moveblendrunstr, 14);
			DecStr(moveblendcrouchstr, 17);
			_didDecrypt = true;
		}


		m_arrPoseParameters[0].Init(pBaseEntity, leanyawstr);
		m_arrPoseParameters[1].Init(pBaseEntity, speedstr);
		m_arrPoseParameters[2].Init(pBaseEntity, ladderspeedstr);
		m_arrPoseParameters[3].Init(pBaseEntity, ladderyawstr);
		m_arrPoseParameters[4].Init(pBaseEntity, moveyawstr);
		if (m_iAnimsetVersion < 2)
			m_arrPoseParameters[5].Init(pBaseEntity, runstr);
		m_arrPoseParameters[6].Init(pBaseEntity, bodyyawstr);
		m_arrPoseParameters[7].Init(pBaseEntity, bodypitchstr);
		m_arrPoseParameters[8].Init(pBaseEntity, deathyawstr);
		m_arrPoseParameters[9].Init(pBaseEntity, standstr);
		m_arrPoseParameters[10].Init(pBaseEntity, jumpfallstr);
		m_arrPoseParameters[11].Init(pBaseEntity, aimblendstandidlestr);
		m_arrPoseParameters[12].Init(pBaseEntity, aimblendcrouchidlestr);
		m_arrPoseParameters[13].Init(pBaseEntity, strafeyawstr);
		m_arrPoseParameters[14].Init(pBaseEntity, aimblendstandwalkstr);
		m_arrPoseParameters[15].Init(pBaseEntity, aimblendstandrunstr);
		m_arrPoseParameters[16].Init(pBaseEntity, aimblendcrouchwalkstr);
		if (m_iAnimsetVersion > 0)
		{
			m_arrPoseParameters[17].Init(pBaseEntity, moveblendwalkstr);
			m_arrPoseParameters[18].Init(pBaseEntity, moveblendrunstr);
			m_arrPoseParameters[19].Init(pBaseEntity, moveblendcrouchstr);
		}
		m_nModelIndex = pBaseEntity->GetModelIndex();
	}
	return m_nModelIndex > 0;
#endif
}

char* C_CSGOPlayerAnimState::GetWeaponMoveAnimation()
{
	return ((char*(__thiscall*)(C_CSGOPlayerAnimState*))StaticOffsets.GetOffsetValue(_GetWeaponMoveAnimation))(this);
}

bool IsPlayingOldDemo()
{
	return (Interfaces::EngineClient->IsHLTV() || Interfaces::EngineClient->IsPlayingDemo()) && Interfaces::EngineClient->GetEngineBuildNumber() <= 13546;
}

void C_CSGOPlayerAnimState::Update(float yaw, float pitch)
{
	if (!pBaseEntity || (!pBaseEntity->GetAliveVMT() && !pBaseEntity->IsPlayerGhost()) || !CacheSequences())
		return;

	START_PROFILING

	pitch = AngleNormalize(pBaseEntity->GetThirdPersonRecoil() + pitch);

	//if (m_flLastClientSideAnimationUpdateTime == Interfaces::Globals->curtime 
	//	|| m_iLastClientSideAnimationUpdateFramecount == Interfaces::Globals->framecount)
	//	return;

	*s_bEnableInvalidateBoneCache = false;

	m_flLastClientSideAnimationUpdateTimeDelta = fmaxf(Interfaces::Globals->curtime - m_flLastClientSideAnimationUpdateTime, 0.0f);
	m_flEyeYaw = AngleNormalize(yaw);
	m_flPitch = AngleNormalize(pitch);
	m_vOrigin = *pBaseEntity->GetAbsOrigin();
	pActiveWeapon = pBaseEntity->GetActiveCSWeapon();
	if (pActiveWeapon != pLastActiveWeapon || m_bIsReset)
	{
		for (int i = 0; i < 13; ++i)
		{
			C_AnimationLayer *layer = pBaseEntity->GetAnimOverlay(i);
			if (layer)
			{
				layer->_m_fFlags = 0;
				layer->m_iActivity = -1;
				layer->m_iPriority = -1;
			}
		}
	}

	float flNewDuckAmount;

	if (!IsPlayingOldDemo())
	{
		flNewDuckAmount = clamp(pBaseEntity->GetDuckAmount() + m_flHitGroundCycle, 0.0f, 1.0f);
		float flDuckSmooth = m_flLastClientSideAnimationUpdateTimeDelta * 6.0f;
		float flDuckDelta = flNewDuckAmount - m_fDuckAmount;

		if (flDuckDelta <= flDuckSmooth) {
			if (-flDuckSmooth > flDuckDelta)
				flNewDuckAmount = m_fDuckAmount - flDuckSmooth;
		}
		else {
			flNewDuckAmount = flDuckSmooth + m_fDuckAmount;
		}

		flNewDuckAmount = clamp(flNewDuckAmount, 0.0f, 1.0f);
	}
	else
	{
		float flLandingAdjustment;
		if (pBaseEntity->GetFlags() & FL_ANIMDUCKING)
			flLandingAdjustment = 1.0f;
		else
			flLandingAdjustment = m_flHitGroundCycle;

		float flLandTimeAdjust;
		if (flLandingAdjustment <= m_fDuckAmount)
			flLandTimeAdjust = 6.0f;
		else
			flLandTimeAdjust = 3.1f;

		float flNewTimeDelta = m_flLastClientSideAnimationUpdateTimeDelta * flLandTimeAdjust;
		if (flLandingAdjustment - m_fDuckAmount <= flNewTimeDelta)
		{
			if (-flNewTimeDelta <= flLandingAdjustment - m_fDuckAmount)
				flNewDuckAmount = flLandingAdjustment;
			else
				flNewDuckAmount = m_fDuckAmount - flNewTimeDelta;
		}
		else
		{
			flNewDuckAmount = m_fDuckAmount + flNewTimeDelta;
		}

		m_fDuckAmount = flNewDuckAmount;

		flNewDuckAmount = clamp(flNewDuckAmount, 0.0f, 1.0f);
	}

	m_fDuckAmount = flNewDuckAmount;

	Interfaces::MDLCache->BeginLock();
	pBaseEntity->SetSequenceVMT(0);
	pBaseEntity->SetPlaybackRate(0.0f);
	if (pBaseEntity->GetCycle() != 0.0f) //SetCycle inlined
	{
		pBaseEntity->SetCycle(0.0f);
		pBaseEntity->InvalidatePhysicsRecursive(ANIMATION_CHANGED);
	}
	Interfaces::MDLCache->EndLock();

	//char *sig = "55  8B  EC  83  E4  F8  83  EC  30  56  57  8B  3D  ??  ??  ??  ??  8B  F1  8B  CF  89  74  24  14  8B  07  FF  90  ??  ??  ??  ??  8B  0D  ??  ??  ??  ??  F3  0F  7E  86  ??  ??  ??  ??  8B  86  ??  ??  ??  ??  89  44  24  28  66  0F  D6  44  24  ??  8B  01  8B  80  ??  ??  ??  ??  FF  D0  84  C0  75  38  8B  0D  ??  ??  ??  ??  8B  01  8B  80  ??  ??  ??  ??";
	//static DWORD func = FindPattern(ClientHandle, std::string(sig), 0, false, 0, 0, 0, 0, 0, 4);
	//((void(__thiscall*)(C_CSGOPlayerAnimState*))func)(this);
	//StaticOffsets.GetOffsetValueByType<void(__thiscall*)(C_CSGOPlayerAnimState*)>(_SetupVelocity)(this);
	SetupVelocity();
	SetupAimMatrix();
	SetupWeaponAction();
#if 0
	PlayerBackup_t *backup = new PlayerBackup_t;
	PlayerBackup_t *backup2 = new PlayerBackup_t;
	PlayerBackup_t *backup3 = new PlayerBackup_t;
	backup->Get(pBaseEntity);

	((void(__thiscall*)(C_CSGOPlayerAnimState*))StaticOffsets.GetOffsetValue(_SetupMovement))(this);
	printf("correct layer5 weight: %f onground: %i feetweight: %f\n", pBaseEntity->GetAnimOverlay(5)->m_flWeight, m_bOnGround ? 1 : 0, m_flFeetWeight);
	backup2->Get(pBaseEntity);

	backup->RestoreData();
	SetupMovement();
	printf("bad layer5 weight: %f onground: %i feetweight: %f\n", pBaseEntity->GetAnimOverlay(5)->m_flWeight, m_bOnGround ? 1 : 0, m_flFeetWeight);
	backup3->Get(pBaseEntity);
	if (m_iStrafeSequence != backup2->m_BackupClientAnimState.m_iStrafeSequence)
	{
		printf("");
		backup->RestoreData();
		printf("call original\n");
		((void(__thiscall*)(C_CSGOPlayerAnimState*))StaticOffsets.GetOffsetValue(_SetupMovement))(this);
		backup->RestoreData();
		printf("call ours\n");
		SetupMovement();
	}
	backup2->RestoreData();

	

	C_CSGOPlayerAnimState *fucked = &backup3->m_BackupClientAnimState;
	C_CSGOPlayerAnimState *correct = &backup2->m_BackupClientAnimState;

	C_AnimationLayer *fuckedlayer5 = &backup3->m_AnimLayers[5];
	C_AnimationLayer *correctlayer5 = &backup2->m_AnimLayers[5];
	C_AnimationLayer *fuckedlayer4 = &backup3->m_AnimLayers[4];
	C_AnimationLayer *correctlayer4 = &backup2->m_AnimLayers[4];

	float correcttotaltime = backup2->m_BackupClientAnimState.m_flStrafeWeight;
	float fuckedtotaltime = backup3->m_BackupClientAnimState.m_flStrafeWeight;

	float correcttotaltime2 = backup2->m_BackupClientAnimState.m_flDuckRate;
	float fuckedtotaltime2 = backup3->m_BackupClientAnimState.m_flDuckRate;

	//backup2->backupanimstate.m_iStrafeSequence;
	//backup3->backupanimstate.m_iStrafeSequence;

	if (fuckedlayer5->m_flWeight != correctlayer5->m_flWeight)
		printf("");

	if (CurrentUserCmd.cmd && CurrentUserCmd.cmd->buttons & IN_SPEED)
		printf(" ");

	for (int i = 0; i < MAX_OVERLAYS; ++i)
	{
		if (fabsf(backup3->m_AnimLayers[i].m_flWeight - backup2->m_AnimLayers[i].m_flWeight) > FLT_EPSILON
			|| 
			fabsf(backup3->m_AnimLayers[i]._m_flCycle - backup2->m_AnimLayers[i]._m_flCycle) > FLT_EPSILON
			|| 
			fabsf(backup3->m_AnimLayers[i]._m_flPlaybackRate - backup2->m_AnimLayers[i]._m_flPlaybackRate) > FLT_EPSILON
			)
		{
			printf("");
		}
	}

	delete backup;
	delete backup2;
	delete backup3;
#else
	SetupMovement();
#endif
	SetupAliveloop();
	SetupWholeBodyAction();
	SetupFlashedReaction();
	SetupFlinch();
	SetupLean();

	for (int i = 0; i < 13; ++i)
	{
		C_AnimationLayer *layer = pBaseEntity->GetAnimOverlay(i);
		if (layer && !layer->_m_nSequence && layer->m_pOwner && layer->m_flWeight != 0.0f) //SetWeight inlined
		{
			layer->m_pOwner->InvalidatePhysicsRecursive(BOUNDS_CHANGED);
			layer->m_flWeight = 0.0f;
		}
	}

	pBaseEntity->SetAbsAngles(QAngle(0.0f, m_flGoalFeetYaw, 0.0f));
	pLastActiveWeapon = pActiveWeapon;
	m_vLastOrigin = m_vOrigin;
	m_bIsReset = 0;
	m_flLastClientSideAnimationUpdateTime = Interfaces::Globals->curtime;
	m_iLastClientSideAnimationUpdateFramecount = Interfaces::Globals->framecount;
	*s_bEnableInvalidateBoneCache = true;
	END_PROFILING
}

void C_CSGOPlayerAnimState::SetupVelocity()
{
	START_PROFILING
	Interfaces::MDLCache->BeginLock();

	Vector velocity = m_vVelocity;
	//if (Interfaces::EngineClient->IsHLTV() || Interfaces::EngineClient->IsPlayingDemo())
		pBaseEntity->GetAbsVelocity(velocity);
	//else
	//	pBaseEntity->EstimateAbsVelocity(velocity);

	float spd = velocity.LengthSqr();

	// Valve's bandaid for when players respawn or come in from dormancy. Fixes legs facing the wrong direction as soon as they spawn due to velocity being huge due to origin delta
	//if (spd > std::pow(1.2f * 260.0f, 2))
	//{
	//	Vector velocity_normalized = velocity;
	//	VectorNormalizeFast(velocity_normalized);
	//	velocity = velocity_normalized * (1.2f * 260.0f);
	//}

	m_flAbsVelocityZ = velocity.z;
	velocity.z = 0.0f;

	float leanspd = m_vecLastSetupLeanVelocity.LengthSqr();

	m_bIsAccelerating = velocity.Length2DSqr() > leanspd;

	m_vVelocity = g_LagCompensation.GetSmoothedVelocity(m_flLastClientSideAnimationUpdateTimeDelta * 2000.0f, velocity, m_vVelocity);

	m_vVelocityNormalized = VectorNormalizeReturn(m_vVelocity);

	float speed = std::fmin(m_vVelocity.Length(), 260.0f);
	m_flSpeed = speed;

	if (speed > 0.0f)
		m_vecLastAcceleratingVelocity = m_vVelocityNormalized;

	CBaseCombatWeapon *weapon = pBaseEntity->GetActiveCSWeapon();
	pActiveWeapon = weapon;

	float flMaxMovementSpeed = 260.0f;
	if (weapon)
		flMaxMovementSpeed = std::fmax(weapon->GetMaxSpeed3(), 0.001f);

	m_flSpeedNormalized = clamp(m_flSpeed / flMaxMovementSpeed, 0.0f, 1.0f);

	m_flRunningSpeed = m_flSpeed / (flMaxMovementSpeed * 0.520f);
	m_flDuckingSpeed = m_flSpeed / (flMaxMovementSpeed * 0.340f);

	if (m_flRunningSpeed < 1.0f)
	{
		if (m_flRunningSpeed < 0.5f)
		{
			float vel = m_flVelocityUnknown;
			float delta = m_flLastClientSideAnimationUpdateTimeDelta * 60.0f;
			float newvel;
			if ((80.0f - vel) <= delta)
			{
				if (-delta <= (80.0f - vel))
					newvel = 80.0f;
				else
					newvel = vel - delta;
			}
			else
			{
				newvel = vel + delta;
			}
			m_flVelocityUnknown = newvel;
		}
	}
	else
	{
		m_flVelocityUnknown = m_flSpeed;
	}

	bool bWasMovingLastUpdate = false;
	bool bJustStartedMovingLastUpdate = false;
	if (m_flSpeed <= 0.0f)
	{
		m_flTimeSinceStartedMoving = 0.0f;
		bWasMovingLastUpdate = m_flTimeSinceStoppedMoving <= 0.0f;
		m_flTimeSinceStoppedMoving += m_flLastClientSideAnimationUpdateTimeDelta;
	}
	else
	{
		m_flTimeSinceStoppedMoving = 0.0f;
		bJustStartedMovingLastUpdate = m_flTimeSinceStartedMoving <= 0.0f;
		m_flTimeSinceStartedMoving = m_flLastClientSideAnimationUpdateTimeDelta + m_flTimeSinceStartedMoving;
	}

	m_flCurrentFeetYaw = m_flGoalFeetYaw;
	m_flGoalFeetYaw = clamp(m_flGoalFeetYaw, -360.0f, 360.0f);

	float eye_feet_delta = AngleDiff(m_flEyeYaw, m_flGoalFeetYaw);

	float flRunningSpeed = clamp(m_flRunningSpeed, 0.0f, 1.0f);

	float flYawModifier = (((m_flGroundFraction * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f;

	if (m_fDuckAmount > 0.0f)
	{
		float flDuckingSpeed = clamp(m_flDuckingSpeed, 0.0f, 1.0f);
		flYawModifier = flYawModifier + ((m_fDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier));
	}

	float flMaxYawModifier = flYawModifier * m_flMaxYaw;
	float flMinYawModifier = flYawModifier * m_flMinYaw;

	if (eye_feet_delta <= flMaxYawModifier)
	{
		if (flMinYawModifier > eye_feet_delta)
			m_flGoalFeetYaw = fabs(flMinYawModifier) + m_flEyeYaw;
	}
	else
	{
		m_flGoalFeetYaw = m_flEyeYaw - fabs(flMaxYawModifier);
	}

	NormalizeAngle(m_flGoalFeetYaw);

	if (m_flSpeed > 0.1f || fabs(m_flAbsVelocityZ) > 100.0f)
	{
		m_flGoalFeetYaw = ApproachAngle(
			m_flEyeYaw,
			m_flGoalFeetYaw,
			((m_flGroundFraction * 20.0f) + 30.0f)
			* m_flLastClientSideAnimationUpdateTimeDelta);
	}
	else
	{
		m_flGoalFeetYaw = ApproachAngle(
			pBaseEntity->GetLowerBodyYaw(),
			m_flGoalFeetYaw,
			m_flLastClientSideAnimationUpdateTimeDelta * 100.0f);
	}

	C_AnimationLayer *layer3 = pBaseEntity->GetAnimOverlayDirect(3);
	if (layer3 && layer3->m_flWeight > 0.0f)
	{
		IncrementLayerCycle(3, false);
		LayerWeightAdvance(3);
	}

	if (m_flSpeed > 0.0f)
	{
		//turns speed into an angle
		float velAngle = (atan2(-m_vVelocity.y, -m_vVelocity.x) * 180.0f) * (1.0f / M_PI);

		if (velAngle < 0.0f)
			velAngle += 360.0f;

		m_flGoalMoveDirGoalFeetDelta = AngleNormalize(AngleDiff(velAngle, m_flGoalFeetYaw));
	}

	m_flFeetVelDirDelta = AngleNormalize(AngleDiff(m_flGoalMoveDirGoalFeetDelta, m_flCurrentMoveDirGoalFeetDelta));

	if (bJustStartedMovingLastUpdate && m_flFeetWeight <= 0.0f)
	{
		m_flCurrentMoveDirGoalFeetDelta = m_flGoalMoveDirGoalFeetDelta;

		C_AnimationLayer *layer = pBaseEntity->GetAnimOverlayDirect(6);
		int sequence = layer ? layer->_m_nSequence : 0;
		if (sequence != -1)
		{
			if (*(DWORD*)((DWORD)pBaseEntity->pSeqdesc(sequence) + 0xC4) > 0)
			{
				int tag = ANIMTAG_UNINITIALIZED;

				if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 180.0f)) > 22.5f)
				{
					if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 135.0f)) > 22.5f)
					{
						if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 90.0f)) > 22.5f)
						{
							if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 45.0f)) > 22.5f)
							{
								if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, 0.0f)) > 22.5f)
								{
									if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, -45.0f)) > 22.5f)
									{
										if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, -90.0f)) > 22.5f)
										{
											if (std::fabs(AngleDiff(m_flCurrentMoveDirGoalFeetDelta, -135.0f)) <= 22.5f)
												tag = ANIMTAG_STARTCYCLE_NW;
										}
										else
										{
											tag = ANIMTAG_STARTCYCLE_W;
										}
									}
									else
									{
										tag = ANIMTAG_STARTCYCLE_SW;
									}
								}
								else
								{
									tag = ANIMTAG_STARTCYCLE_S;
								}
							}
							else
							{
								tag = ANIMTAG_STARTCYCLE_SE;
							}
						}
						else
						{
							tag = ANIMTAG_STARTCYCLE_E;
						}
					}
					else
					{
						tag = ANIMTAG_STARTCYCLE_NE;
					}
				}
				else
				{
					tag = ANIMTAG_STARTCYCLE_N;
				}
				m_flFeetCycle = pBaseEntity->GetFirstSequenceAnimTag(sequence, tag);
			}
		}

		if (m_flDuckRate >= 1.0f && !m_bIsReset && std::fabs(m_flFeetVelDirDelta) > 45.0f)
		{
			if (m_bOnGround)
			{
				if (pBaseEntity->GetUnknownAnimationFloat() <= 0.0f)
					pBaseEntity->DoUnknownAnimationCode(StaticOffsets.GetOffsetValue(_UnknownAnimationFloat), 0.3f);
			}
		}
	}
	else
	{
		C_AnimationLayer *layer = pBaseEntity->GetAnimOverlayDirect(7);
		if (layer && layer->m_flWeight >= 1.0f)
		{
			m_flCurrentMoveDirGoalFeetDelta = m_flGoalMoveDirGoalFeetDelta;
		}
		else
		{
			if (m_flDuckRate >= 1.0f
				&& !m_bIsReset
				&& std::fabs(m_flFeetVelDirDelta) > 100.0f
				&& m_bOnGround
				&& pBaseEntity->GetUnknownAnimationFloat() <= 0.0f)
			{
				pBaseEntity->DoUnknownAnimationCode(StaticOffsets.GetOffsetValue(_UnknownAnimationFloat), 0.3f);
			}
			float flDuckSpeedClamp = clamp(m_flDuckingSpeed, 0.0f, 1.0f);
			float flRunningSpeedClamp = clamp(m_flRunningSpeed, 0.0f, 1.0f);
			float flLerped = ((flDuckSpeedClamp - flRunningSpeedClamp) * m_fDuckAmount) + flRunningSpeedClamp;
			float flBiasMove = /*powf(flLerped, 2.4739397f); //*/Bias(flLerped, 0.18f);
			m_flCurrentMoveDirGoalFeetDelta = AngleNormalize(((flBiasMove + 0.1f) * m_flFeetVelDirDelta) + m_flCurrentMoveDirGoalFeetDelta);
		}
	}

	m_arrPoseParameters[4].SetValue(pBaseEntity, m_flCurrentMoveDirGoalFeetDelta);

	float eye_goalfeet_delta = AngleDiff(m_flEyeYaw, m_flGoalFeetYaw);

	float new_body_yaw_pose = 0.0f; //not initialized?

	if (eye_goalfeet_delta < 0.0f || m_flMaxYaw == 0.0f)
	{
		if (m_flMinYaw != 0.0f)
			new_body_yaw_pose = (eye_goalfeet_delta / m_flMinYaw) * -58.0f;
	}
	else
	{
		new_body_yaw_pose = (eye_goalfeet_delta / m_flMaxYaw) * 58.0f;
	}

	m_arrPoseParameters[6].SetValue(pBaseEntity, new_body_yaw_pose);

	float eye_pitch_normalized = AngleNormalize(m_flPitch);
	float new_body_pitch_pose;

	if (eye_pitch_normalized <= 0.0f)
		new_body_pitch_pose = (eye_pitch_normalized / m_flMinPitch) * -90.0f;
	else
		new_body_pitch_pose = (eye_pitch_normalized / m_flMaxPitch) * 90.0f;

	m_arrPoseParameters[7].SetValue(pBaseEntity, new_body_pitch_pose);

	//Set pose parameter speed to m_flRunningSpeed
	m_arrPoseParameters[1].SetValue(pBaseEntity, m_flRunningSpeed);

	//Set pose parameter stand
	m_arrPoseParameters[9].SetValue(pBaseEntity, 1.0f - (m_flDuckRate * m_fDuckAmount));

	Interfaces::MDLCache->EndLock();
	END_PROFILING
}

void C_CSGOPlayerAnimState::SetupMovement()
{
	START_PROFILING
#if 0
#ifdef _DEBUG
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(C_CSGOPlayerAnimState*)>(_SetupMovement)(this);
	END_PROFILING
	return;
#endif
#endif
	Interfaces::MDLCache->BeginLock();

	float old_ground_fraction = m_flGroundFraction;
	if (old_ground_fraction > 0.0f && old_ground_fraction < 1.0f)
	{
		float twotickstime = m_flLastClientSideAnimationUpdateTimeDelta + m_flLastClientSideAnimationUpdateTimeDelta;
		if (m_bNotRunning)
			m_flGroundFraction = old_ground_fraction - twotickstime;
		else
			m_flGroundFraction = twotickstime + old_ground_fraction;

		m_flGroundFraction = clamp(m_flGroundFraction, 0.0f, 1.0f);
	}

	if (m_flSpeed > (260.0f * _CS_PLAYER_SPEED_WALK_MODIFIER) && m_bNotRunning)
	{
		m_bNotRunning = false;
		m_flGroundFraction = fmaxf(m_flGroundFraction, 0.01f);
	}
	else if (m_flSpeed < (260.0f * _CS_PLAYER_SPEED_WALK_MODIFIER) && !m_bNotRunning)
	{
		m_bNotRunning = true;
		m_flGroundFraction = fminf(m_flGroundFraction, 0.99f);
	}

	float duck_pose;

	if (m_iAnimsetVersion < 2)
	{
		m_arrPoseParameters[5].SetValue(pBaseEntity, m_flGroundFraction);
	}
	else
	{
		m_arrPoseParameters[17].SetValue(pBaseEntity, (1.0f - m_flGroundFraction) * (1.0f - m_fDuckAmount));
		m_arrPoseParameters[18].SetValue(pBaseEntity, (1.0f - m_fDuckAmount) * m_flGroundFraction);
		m_arrPoseParameters[19].SetValue(pBaseEntity, m_fDuckAmount);
	}

	char dest[64];
	//decrypts(0)
	sprintf_s(dest, XorStr("move_%s"), GetWeaponMoveAnimation());
	//encrypts(0)

	int seq = pBaseEntity->LookupSequence(dest);
	if (seq == -1)
	{
		//decrypts(0)
		seq = pBaseEntity->LookupSequence(XorStr("move"));
		//encrypts(0)
	}

	if (pBaseEntity->GetMoveState() != m_iMoveState)
		m_flMovePlaybackRate += 10.0f;

	m_iMoveState = pBaseEntity->GetMoveState();

	float movement_time_delta = m_flLastClientSideAnimationUpdateTimeDelta * 40.0f;

	if (-m_flMovePlaybackRate <= movement_time_delta)
	{
		if (-movement_time_delta <= -m_flMovePlaybackRate)
			m_flMovePlaybackRate = 0.0f;
		else
			m_flMovePlaybackRate = m_flMovePlaybackRate - movement_time_delta;
	}
	else
	{
		m_flMovePlaybackRate = m_flMovePlaybackRate + movement_time_delta;
	}

	m_flMovePlaybackRate = clamp(m_flMovePlaybackRate, 0.0f, 100.0f);

	float duckspeed_clamped = clamp(m_flDuckingSpeed, 0.0f, 1.0f);
	float runspeed_clamped = clamp(m_flRunningSpeed, 0.0f, 1.0f);

	float speed_weight = ((duckspeed_clamped - runspeed_clamped) * m_fDuckAmount) + runspeed_clamped;

	if (speed_weight < m_flFeetWeight)
	{
		float v34 = clamp(m_flMovePlaybackRate * 0.01f, 0.0f, 1.0f);
		float feetweight_elapsed = ((v34 * 18.0f) + 2.0f) * m_flLastClientSideAnimationUpdateTimeDelta;
		if (speed_weight - m_flFeetWeight <= feetweight_elapsed)
			m_flFeetWeight = -feetweight_elapsed <= (speed_weight - m_flFeetWeight) ? speed_weight : m_flFeetWeight - feetweight_elapsed;
		else
			m_flFeetWeight = feetweight_elapsed + m_flFeetWeight;
	}
	else
	{
		m_flFeetWeight = speed_weight;
	}

	float yaw = AngleNormalize((m_flCurrentMoveDirGoalFeetDelta + m_flGoalFeetYaw) + 180.0f);
	QAngle angle = { 0.0f, yaw, 0.0f };
	Vector vecDir;
	AngleVectors(angle, &vecDir);

	float movement_side = DotProduct(m_vecLastAcceleratingVelocity, vecDir);
	if (movement_side < 0.0f)
		movement_side = -movement_side;

	float newfeetweight = Bias(movement_side, 0.2f) * m_flFeetWeight;

	m_flFeetWeight = newfeetweight; //TODO: find out how to resolve based off this

	float newfeetweight2 = newfeetweight * m_flDuckRate;

	float layer5_weight = GetLayerWeight(5);

	float new_weight = 0.55f;
	if (1.0f - layer5_weight > 0.55f)
		new_weight = 1.0f - layer5_weight;

	float new_feet_layer_weight = new_weight * newfeetweight2;
	float feet_cycle_rate = 0.0f;

	if (m_flSpeed > 0.00f)
	{
		float seqcyclerate = pBaseEntity->GetSequenceCycleRate_Server(seq);

		float seqmovedist = pBaseEntity->GetSequenceMoveDist(pBaseEntity->GetModelPtr(), seq);
		seqmovedist *= 1.0f / (1.0f / seqcyclerate);
		if (seqmovedist <= 0.001f)
			seqmovedist = 0.001f;

		float speed_multiplier = m_flSpeed / seqmovedist;
		feet_cycle_rate = (1.0f - (m_flGroundFraction * 0.15f)) * (speed_multiplier * seqcyclerate);
	}

	float feetcycle_playback_rate = (m_flLastClientSideAnimationUpdateTimeDelta * feet_cycle_rate);
	SetFeetCycle(feetcycle_playback_rate + m_flFeetCycle); //TODO: find out how to resolve based off this

	UpdateAnimLayer(feetcycle_playback_rate, 6, seq, clamp(new_feet_layer_weight, 0.0f, 1.0f), m_flFeetCycle); //TODO: find out how to resolve based off this

	if (pBaseEntity->IsStrafing())
	{
		if (!m_bStrafing)
		{
			m_flTotalStrafeTime = 0.0f;
			if (!m_bIsReset && m_bOnGround && pBaseEntity->GetUnknownSetupMovementFloat() <= 0.0f)
				pBaseEntity->DoUnknownAnimationCode(StaticOffsets.GetOffsetValue(_UnknownSetupMovementFloat), 0.15f);
		}

		float strafe_weight_time_delta = m_flLastClientSideAnimationUpdateTimeDelta * 20.0f;
		m_bStrafing = true;

		//if this is fucked, redo again
		if (1.0f - m_flStrafeWeight <= strafe_weight_time_delta)
			m_flStrafeWeight = -strafe_weight_time_delta <= (1.0f - m_flStrafeWeight) ? 1.0f : m_flStrafeWeight - strafe_weight_time_delta;
		else
			m_flStrafeWeight = m_flStrafeWeight + strafe_weight_time_delta;

		float strafe_cycle_time_delta = m_flLastClientSideAnimationUpdateTimeDelta * 10.0f;
		if (-m_flStrafeCycle <= strafe_cycle_time_delta)
			m_flStrafeCycle = -strafe_cycle_time_delta <= -m_flStrafeCycle ? 0.0f : m_flStrafeCycle - strafe_cycle_time_delta;
		else
			m_flStrafeCycle = m_flStrafeCycle + strafe_cycle_time_delta;

		float CurrentMoveDirGoalFeetDelta = AngleNormalize(m_flCurrentMoveDirGoalFeetDelta);

		m_arrPoseParameters[13].SetValue(pBaseEntity, CurrentMoveDirGoalFeetDelta);
	}
	else
	{
		if (m_arrPoseParameters[13].m_bInitialized)
		{
			if (m_flStrafeWeight > 0.0f)
			{
				m_flTotalStrafeTime += m_flLastClientSideAnimationUpdateTimeDelta;
				if (m_flTotalStrafeTime > 0.08f)
				{
					float newtimedelta3 = m_flLastClientSideAnimationUpdateTimeDelta * 5.0f;
					if (-m_flStrafeWeight <= newtimedelta3)
						m_flStrafeWeight = -newtimedelta3 <= -m_flStrafeWeight ? 0.0f : m_flStrafeWeight - newtimedelta3;
					else
						m_flStrafeWeight = m_flStrafeWeight + newtimedelta3;
				}

				//decrypts(0)
				m_iStrafeSequence = pBaseEntity->LookupSequence(XorStr("strafe"));
				//encrypts(0)

				float seqcyclerate2 = pBaseEntity->GetSequenceCycleRate_Server(m_iStrafeSequence);

				m_flStrafeCycle += (m_flLastClientSideAnimationUpdateTimeDelta * seqcyclerate2);
				m_flStrafeCycle = clamp(m_flStrafeCycle, 0.0f, 1.0f);
			}
		}
	}

	if (0.0f >= m_flStrafeWeight)
		m_bStrafing = false;

	bool old_onground = m_bOnGround;
	bool new_onground = pBaseEntity->GetFlags() & FL_ONGROUND || pBaseEntity->GetWaterLevel() >= WL_Feet;
	m_bOnGround = new_onground;

	bool just_landed;
	if (!m_bIsReset && old_onground != new_onground && new_onground)
		just_landed = true;
	else
		just_landed = false;

	m_bJust_Landed = just_landed;

	bool just_left_ground = old_onground != new_onground && !new_onground;

	m_bJust_LeftGround = just_left_ground;

	if (just_left_ground)
		m_flStartJumpZOrigin = m_vOrigin.z;

	if (just_landed)
	{
		float zdelta = fabsf(m_flStartJumpZOrigin - m_vOrigin.z);
		zdelta = (zdelta - 12.0f) * 0.017f ;
		zdelta = clamp(zdelta, 0.0f, 1.0f);

		float v91 = Bias(zdelta, 0.4f);
		float v94 = clamp(Bias(m_flTotalTimeInAir, 0.3f), 0.1f, 1.0f);

		m_flHitGroundWeight = v94;

		m_flHitGroundCycle = v94 <= v91 ? v91 : v94;
	}
	else
	{
		float twotickstime = m_flLastClientSideAnimationUpdateTimeDelta + m_flLastClientSideAnimationUpdateTimeDelta;
		if (-m_flHitGroundCycle <= twotickstime)
			m_flHitGroundCycle = -twotickstime <= -m_flHitGroundCycle ? 0.0f : m_flHitGroundCycle - twotickstime;
		else
			m_flHitGroundCycle = m_flHitGroundCycle + twotickstime;
	}

	float flOnGround = (float)(new_onground != 0);
	float newduckrate = ((m_fDuckAmount + 1.0f) * 8.0f) * m_flLastClientSideAnimationUpdateTimeDelta;

	if ((flOnGround - m_flDuckRate) <= newduckrate)
		newduckrate = -newduckrate <= flOnGround - m_flDuckRate ? flOnGround : m_flDuckRate - newduckrate;
	else
		newduckrate = m_flDuckRate + newduckrate;

	m_flDuckRate = clamp(newduckrate, 0.0f, 1.0f);

	float new_strafe_weight = ((1.0f - m_fDuckAmount) * m_flStrafeWeight) * m_flDuckRate;
	m_flStrafeWeight = clamp(new_strafe_weight, 0.0f, 1.0f);
	int strafe_sequence = m_iStrafeSequence;
	if (strafe_sequence != -1)
		UpdateAnimLayer(0.0f, 7, strafe_sequence, m_flStrafeWeight, m_flStrafeCycle);

	bool old_onladder = m_bOnLadder;
	bool new_onladder = !m_bOnGround && pBaseEntity->GetMoveType() == MOVETYPE_LADDER;
	m_bOnLadder = new_onladder;

	//bool not_onladder = !old_onladder || !new_onladder;
	bool not_onladder;
	if (!old_onladder || (not_onladder = true, new_onladder))
		not_onladder = false;

	if (m_flLadderCycle <= 0.0f && !new_onladder)
	{
		m_flLadderWeight = 0.0f;
	}
	else
	{
		float laddertimedelta = m_flLastClientSideAnimationUpdateTimeDelta * 10.0f;
		float v113;
		if (fabsf(m_flAbsVelocityZ) <= 100.0f)
		{
			if (-m_flLadderWeight <= laddertimedelta)
				v113 = -laddertimedelta <= -m_flLadderWeight ? 0.0f : m_flLadderWeight - laddertimedelta;
			else
				v113 = m_flLadderWeight + laddertimedelta;
		}
		else if (1.0f - m_flLadderWeight <= laddertimedelta)
		{
			v113 = -laddertimedelta <= 1.0f - m_flLadderWeight ? 1.0f : m_flLadderWeight - laddertimedelta;
		}
		else
		{
			v113 = m_flLadderWeight + laddertimedelta;
		}

		m_flLadderWeight = clamp(v113, 0.0f, 1.0f);

		float v116;
		if (new_onladder)
		{
			float timedelta4 = m_flLastClientSideAnimationUpdateTimeDelta * 5.0f;
			if (1.0f - m_flLadderCycle <= timedelta4)
				v116 = -timedelta4 <= 1.0f - m_flLadderCycle ? 1.0f : m_flLadderCycle - timedelta4;
			else
				v116 = m_flLadderCycle + timedelta4;
		}
		else if (-m_flLadderCycle <= laddertimedelta)
		{
			v116 = -laddertimedelta <= -m_flLadderCycle ? 0.0f : m_flLadderCycle - laddertimedelta;
		}
		else
		{
			v116 = m_flLadderCycle + laddertimedelta;
		}
		m_flLadderCycle = clamp(v116, 0.0f, 1.0f);

		Vector laddernormal = pBaseEntity->GetVecLadderNormal();
		QAngle ladderAngles;
		VectorAngles(laddernormal, ladderAngles);

		float ladderGoalFeetYawDelta = AngleDiff(ladderAngles.y, m_flGoalFeetYaw);

		m_arrPoseParameters[3].SetValue(pBaseEntity, ladderGoalFeetYawDelta);

		float layer5_cycle = GetLayerCycle(5);

		float unk = m_flLadderWeight * 0.006f;
		float newpose2 = ((m_vOrigin.z - m_vLastOrigin.z) * (0.01f - unk)) + layer5_cycle;
		m_arrPoseParameters[2].SetValue(pBaseEntity, m_flLadderWeight);

		if (GetLayerActivity(5) == 987)
			SetLayerWeight(5, m_flLadderCycle);

		SetLayerCycle(5, newpose2);

		if (m_bOnLadder)
		{
			float layer4_idealweight = 1.0f - m_flLadderCycle;

			if (GetLayerWeight(4) > layer4_idealweight)
				SetLayerWeight(4, layer4_idealweight);
		}
	}
	
	if (!m_bOnGround)
	{
		if (!m_bOnLadder)
		{
			//PLAYER IS IN THE AIR

			m_bInHitGroundAnimation = false;
			if (m_bJust_LeftGround || not_onladder)
				m_flTotalTimeInAir = 0.0f;

			m_flTotalTimeInAir += m_flLastClientSideAnimationUpdateTimeDelta;
			IncrementLayerCycle(4, false);

			float layer4_weight = GetLayerWeight(4);
			float layer4_idealweight = GetLayerIdealWeightFromSeqCycle(4);

			if (layer4_idealweight > layer4_weight)
				SetLayerWeight(4, layer4_idealweight);

			C_AnimationLayer *layer5 = pBaseEntity->GetAnimOverlay(5);
			if (layer5)
			{
				if (layer5->m_flWeight > 0.0f)
				{
					float v175 = (m_flTotalTimeInAir - 0.2f) * -5.0f;
					v175 = clamp(v175, 0.0f, 1.0f);
					float newlayer5_weight = ((3.0f - (v175 + v175)) * (v175 * v175)) * layer5->m_flWeight;
					SetLayerWeight(5, newlayer5_weight);
				}
			}

			float v203 = (m_flTotalTimeInAir - 0.72f) * 1.25f;
			v203 = clamp(v203, 0.0f, 1.0f);
			float newpose10 = (3.0f - (v203 + v203)) * (v203 * v203);
			newpose10 = clamp(newpose10, 0.0f, 1.0f);
			m_arrPoseParameters[10].SetValue(pBaseEntity, newpose10);
			Interfaces::MDLCache->EndLock();
			END_PROFILING
			return;
		}
		Interfaces::MDLCache->EndLock();
		END_PROFILING
		return;
	}

	if (!m_bInHitGroundAnimation)
	{
		if (m_bJust_Landed || not_onladder)
		{
			//Start the hit ground animation , went here when fucked
			SetLayerCycle(5, 0.0f);
			m_bInHitGroundAnimation = true;
		}
	}

	if (!m_bInHitGroundAnimation || GetLayerActivity(5) == 987) //ACT_CSGO_CLIMB_LADDER
	{
		Interfaces::MDLCache->EndLock();
		END_PROFILING
		return;
	}
	//went here when fucked
	IncrementLayerCycle(5, false);
	IncrementLayerCycle(4, false);
	m_arrPoseParameters[10].SetValue(pBaseEntity, 0.0f);

	C_AnimationLayer* layer5 = pBaseEntity->GetAnimOverlay(5);
	if (layer5)
	{
		float layer5_nextcycle = (layer5->_m_flPlaybackRate * m_flLastClientSideAnimationUpdateTimeDelta) + layer5->_m_flCycle;

		//check to see if the hit ground animation is finished
		if (layer5_nextcycle >= 1.0f)
		{
			//FINISHED HITTING GROUND ANIMATION
			m_bInHitGroundAnimation = false;
			SetLayerWeight(5, 0.0f);
			SetLayerWeight(4, 0.0f);
			m_flHitGroundWeight = 1.0f;
			Interfaces::MDLCache->EndLock();
			END_PROFILING
			return;
		}
	}

	//STILL IN HITTING GROUND ANIMATION

	float layer5_idealweight = GetLayerIdealWeightFromSeqCycle(5) * m_flHitGroundWeight;
	float weight_mult = 0.2f;
	float v168 = 1.0f - m_fDuckAmount;
	if (v168 >= 0.2f)
		weight_mult = fminf(v168, 1.0f);

	SetLayerWeight(5, weight_mult * layer5_idealweight);

	float layer4_weight = GetLayerWeight(4);
	if (layer4_weight <= 0.0f)
	{
		Interfaces::MDLCache->EndLock();
		END_PROFILING
		return;
	}

	float timedelta5 = m_flLastClientSideAnimationUpdateTimeDelta * 10.0f;

	C_AnimationLayer *layer4 = GetAnimOverlay(4);
	if (layer4)
	{
		if (-layer4_weight <= timedelta5)
		{
			if (-timedelta5 <= -layer4_weight)
				SetLayerWeight(4, 0.0f);
			else
				SetLayerWeight(4, layer4_weight - timedelta5);
		}
		else
		{
			SetLayerWeight(4, timedelta5 + layer4_weight);
		}
	}

	Interfaces::MDLCache->EndLock();
	END_PROFILING
}

void C_CSGOPlayerAnimState::SetupFlinch()
{
	START_PROFILING
	IncrementLayerCycle(10, false);
	END_PROFILING
}

void C_CSGOPlayerAnimState::SetupAliveloop()
{
	START_PROFILING
	IncrementLayerCycleGeneric(11);
	END_PROFILING
}

void C_CSGOPlayerAnimState::SetupWholeBodyAction()
{
	START_PROFILING
	C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(8);
	if (pLayer && pLayer->m_flWeight > 0.0f)
	{
		IncrementLayerCycle(8, false);
		LayerWeightAdvance(8);
	}
	END_PROFILING
}

void C_CSGOPlayerAnimState::UpdateAnimLayer(float playbackrate, int layer, int sequence, float weight, float cycle)
{
	if (sequence >= 2)
	{
		Interfaces::MDLCache->BeginLock();
		C_AnimationLayer *pLayer = pBaseEntity->GetAnimOverlay(layer);
		if (pLayer)
		{
			pLayer->SetSequence(sequence);
			pLayer->_m_flPlaybackRate = playbackrate;
			pLayer->SetCycle(clamp(cycle, 0.0f, 1.0f));
			pLayer->SetWeight(clamp(weight, 0.0f, 1.0f));
			UpdateLayerOrderPreset(weight, layer, sequence);
		}
		Interfaces::MDLCache->EndLock();
	}
}

void C_CSGOPlayerAnimState::UpdateLayerOrderPreset(float weight, int layer, int sequence)
{
	auto func = StaticOffsets.GetOffsetValueByType<void(__thiscall*)(C_CSGOPlayerAnimState*, int, int)>(_UpdateLayerOrderPreset);
	__asm movss xmm0, weight;
	func(this, layer, sequence);
}

float C_CSGOPlayerAnimState::GetLayerIdealWeightFromSeqCycle(int layer)
{
	//auto func = StaticOffsets.GetOffsetValueByType<void(__thiscall*)(C_CSGOPlayerAnimState*, int)>(_GetLayerIdealWeightFromSeqCycle);
	//float retval;
	//func(this, layer);
	//__asm movss retval, xmm0
	//return retval;

	Interfaces::MDLCache->BeginLock();
	C_AnimationLayer *overlay = pBaseEntity->GetAnimOverlay(layer);
	if (!overlay)
	{
		Interfaces::MDLCache->EndLock();
		return 0.0f;
	}

	int sequence = overlay->_m_nSequence;
	CStudioHdr *modelptr = pBaseEntity->GetModelPtr();
	mstudioseqdesc_t *seqdesc;
	//NOTE TO VALVE: you don't check for valid pointer here!
	if (modelptr->m_pVModel)
		seqdesc = opSeqdesc((studiohdr_t*)modelptr, sequence);
	else
		seqdesc = modelptr->_m_pStudioHdr->pLocalSeqdesc(sequence);

	float cycle = overlay->_m_flCycle;
	if (cycle >= 0.99f)
		cycle = 1.0f;

	float fadeintime = seqdesc->fadeintime;
	float fadeouttime = seqdesc->fadeouttime;
	float weight = 1.0f;
	float v15;

	if (fadeintime <= 0.0f || fadeintime <= cycle)
	{
		if (fadeouttime >= 1.0f || cycle <= fadeouttime)
		{
			weight = fminf(weight, 1.0f);
			Interfaces::MDLCache->EndLock();
			return weight;
		}
		v15 = (cycle - 1.0f) / (fadeouttime - 1.0f);
		v15 = clamp(v15, 0.0f, 1.0f);
	}
	else
	{
		v15 = clamp(cycle / fadeintime, 0.0f, 1.0f);
	}
	weight = (3.0f - (v15 + v15)) * (v15 * v15);
	if (weight < 0.0015f)
		weight = 0.0f;
	else
		weight = clamp(weight, 0.0f, 1.0f);

	Interfaces::MDLCache->EndLock();
	return weight;
}

void C_CSGOPlayerAnimState::SetupFlashedReaction()
{
	START_PROFILING
	C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(9);
	if (pLayer && pLayer->m_flWeight > 0.0f && pLayer->m_flWeightDeltaRate < 0.0f)
		LayerWeightAdvance(9);
	END_PROFILING
}

void C_CSGOPlayerAnimState::LayerWeightAdvance(int layer)
{
	C_AnimationLayer* pLayer = pBaseEntity->GetAnimOverlay(layer);
	if (!layer || fabs(pLayer->m_flWeightDeltaRate) <= 0.0f)
		return;

	float newweight = (pLayer->m_flWeightDeltaRate * m_flLastClientSideAnimationUpdateTimeDelta) + pLayer->m_flWeight;
	newweight		= clamp(newweight, 0.0f, 1.0f);
	pLayer->SetWeight(newweight);
}

C_AnimationLayer* C_CSGOPlayerAnimState::GetAnimOverlay(int layer)
{
	CUtlVectorSimple* m_AnimOverlay = pBaseEntity->GetAnimOverlayStruct();
	int numoverlays					= m_AnimOverlay->count;
	int maxindex					= numoverlays - 1;
	int index = layer;
	if (maxindex >= 0)
	{
		if (layer > maxindex)
			index = numoverlays - 1;
	}
	else
	{
		index = 0;
	}

	if (numoverlays)
		return (C_AnimationLayer*)m_AnimOverlay->Retrieve(index, sizeof(C_AnimationLayer));

	return nullptr;
}

int C_CSGOPlayerAnimState::GetLayerActivity(int layer)
{
	C_AnimationLayer* pLayer = GetAnimOverlay(layer);
	if (!pLayer)
		return -1;

	//This is not the same as the game code, but cba to rebuild this function properly right now
	return GetSequenceActivity(pBaseEntity, pLayer->_m_nSequence);
}

CPredictableList *GetPredictables(int nSlot)
{
	//AssertMsg1(nSlot >= 0, "Tried to get prediction slot for player %d. This probably means you are predicting something that isn't local. Crash ensues.", nSlot);
	CPredictableList* g_Predictables = StaticOffsets.GetOffsetValueByType<CPredictableList*>(_g_Predictables);
	return &g_Predictables[nSlot];
}

//-----------------------------------------------------------------------------
// Purpose: Add entity to list
// Input  : add - 
// Output : int
//-----------------------------------------------------------------------------
void CPredictableList::AddToPredictableList(CBaseEntity *add)
{
	// This is a hack to remap slot to index
	if (m_Predictables.Find(add) != m_Predictables.InvalidIndex())
	{
		return;
	}

	// Add to general sorted list
	m_Predictables.Insert(add);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : remove - 
//-----------------------------------------------------------------------------
void CPredictableList::RemoveFromPredictablesList(CBaseEntity *remove)
{
	m_Predictables.FindAndRemove(remove);
}

#if 0
class CBoneSetup
{
public:
	CBoneSetup(const CStudioHdr *pStudioHdr, int boneMask, const float poseParameter[], IPoseDebugger *pPoseDebugger = NULL);
	void InitPose(Vector pos[], Quaternion q[]);
	void AccumulatePose(Vector pos[], Quaternion q[], int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext);
	void CalcAutoplaySequences(Vector pos[], Quaternion q[], float flRealTime, CIKContext *pIKContext);
private:
	void AddSequenceLayers(Vector pos[], Quaternion q[], mstudioseqdesc_t &seqdesc, int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext);
	void AddLocalLayers(Vector pos[], Quaternion q[], mstudioseqdesc_t &seqdesc, int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext);
public:
	const CStudioHdr *m_pStudioHdr;
	int m_boneMask;
	const float *m_flPoseParameter;
	IPoseDebugger *m_pPoseDebugger;
};

CBoneSetup::CBoneSetup(const CStudioHdr *pStudioHdr, int boneMask, const float poseParameter[], IPoseDebugger *pPoseDebugger)
{
	m_pStudioHdr = pStudioHdr;
	m_boneMask = boneMask;
	m_flPoseParameter = poseParameter;
	m_pPoseDebugger = pPoseDebugger;
}

//-----------------------------------------------------------------------------
// Purpose: accumulate a pose for a single sequence on top of existing animation
//			adds autolayers, runs local ik rukes
//-----------------------------------------------------------------------------
void CBoneSetup::AccumulatePose(
	Vector pos[],
	Quaternion q[],
	int sequence,
	float cycle,
	float flWeight,
	float flTime,
	CIKContext *pIKContext
)
{
	Vector		pos2[MAXSTUDIOBONES];
	QuaternionAligned	q2[MAXSTUDIOBONES];

	//Assert(flWeight >= 0.0f && flWeight <= 1.0f);
	// This shouldn't be necessary, but the Assert should help us catch whoever is screwing this up
	flWeight = clamp(flWeight, 0.0f, 1.0f);

	if (sequence < 0)
		return;

#ifdef CLIENT_DLL
	// Trigger pose debugger
	if (m_pPoseDebugger)
	{
		m_pPoseDebugger->AccumulatePose(m_pStudioHdr, pIKContext, pos, q, sequence, cycle, m_flPoseParameter, m_boneMask, flWeight, flTime);
	}
#endif
	mstudioseqdesc_t	*pseqdesc = opSeqdesc((studiohdr_t *)m_pStudioHdr, sequence);
	mstudioseqdesc_t	&seqdesc = *pseqdesc;

	// add any IK locks to prevent extremities from moving
	CIKContext seq_ik;
	if (seqdesc.numiklocks)
	{
		seq_ik.Init(m_pStudioHdr, angZero, vecZero, 0.0, 0, m_boneMask);  // local space relative so absolute position doesn't mater
		seq_ik.AddSequenceLocks(seqdesc, pos, q);
	}

	if (seqdesc.flags & STUDIO_LOCAL)
	{
		::InitPose(m_pStudioHdr, pos2, q2, m_boneMask);
	}

	if (CalcPoseSingle(m_pStudioHdr, pos2, q2, seqdesc, sequence, cycle, m_flPoseParameter, m_boneMask, flTime))
	{
		// this weight is wrong, the IK rules won't composite at the correct intensity
		AddLocalLayers(pos2, q2, seqdesc, sequence, cycle, 1.0, flTime, pIKContext);
		SlerpBones(m_pStudioHdr, q, pos, seqdesc, sequence, q2, pos2, flWeight, m_boneMask);
	}


	if (pIKContext)
	{
		pIKContext->AddDependencies(seqdesc, sequence, cycle, m_flPoseParameter, flWeight);
	}

	AddSequenceLayers(pos, q, seqdesc, sequence, cycle, flWeight, flTime, pIKContext);

	if (seqdesc.numiklocks)
	{
		seq_ik.SolveSequenceLocks(seqdesc, pos, q);
	}
}

IBoneSetup::IBoneSetup(const CStudioHdr *pStudioHdr, int boneMask, const float poseParameter[], IPoseDebugger *pPoseDebugger)
{
	m_pBoneSetup = new CBoneSetup(pStudioHdr, boneMask, poseParameter, pPoseDebugger);
}

IBoneSetup::~IBoneSetup(void)
{
	if (m_pBoneSetup)
	{
		delete m_pBoneSetup;
	}
}

void IBoneSetup::InitPose(Vector pos[], Quaternion q[])
{
	::InitPose(m_pBoneSetup->m_pStudioHdr, pos, q, m_pBoneSetup->m_boneMask);
}

void IBoneSetup::AccumulatePose(Vector pos[], Quaternion q[], int sequence, float cycle, float flWeight, float flTime, CIKContext *pIKContext)
{
	m_pBoneSetup->AccumulatePose(pos, q, sequence, cycle, flWeight, flTime, pIKContext);
}

void IBoneSetup::CalcAutoplaySequences(Vector pos[], Quaternion q[], float flRealTime, CIKContext *pIKContext)
{
	m_pBoneSetup->CalcAutoplaySequences(pos, q, flRealTime, pIKContext);
}

void CalcBoneAdj(const CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], const float controllers[], int boneMask);

// takes a "controllers[]" array normalized to 0..1 and adds in the adjustments to pos[], and q[].
void IBoneSetup::CalcBoneAdj(Vector pos[], Quaternion q[], const float controllers[])
{
	::CalcBoneAdj(m_pBoneSetup->m_pStudioHdr, pos, q, controllers, m_pBoneSetup->m_boneMask);
}

CStudioHdr *IBoneSetup::GetStudioHdr()
{
	return (CStudioHdr *)m_pBoneSetup->m_pStudioHdr;
}

void CIKContext::Init(const CStudioHdr *pStudioHdr, const QAngle &angles, const Vector &pos, float flTime, int iFramecounter, int boneMask)
{
	m_pStudioHdr = pStudioHdr;
	m_ikChainRule.RemoveAll(); // m_numikrules = 0;
	if (pStudioHdr->numikchains())
	{
		m_ikChainRule.SetSize(pStudioHdr->numikchains());

		// FIXME: Brutal hackery to prevent a crash
		if (m_target.Count() == 0)
		{
			m_target.SetSize(12);
			memset(m_target.Base(), 0, sizeof(m_target[0])*m_target.Count());
			ClearTargets();
		}

	}
	else
	{
		m_target.SetSize(0);
	}
	AngleMatrix(angles, pos, m_rootxform);
	m_iFramecounter = iFramecounter;
	m_flTime = flTime;
	m_boneMask = boneMask;
}

void CIKContext::ClearTargets(void)
{
	int i;
	for (i = 0; i < m_target.Count(); i++)
	{
		m_target[i].latched.iFramecounter = -9999;
	}
}
#endif
