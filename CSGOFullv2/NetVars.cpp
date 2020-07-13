#include "precompiled.h"
#include "NetVars.h"
#include "CreateMove.h"
#include "LocalPlayer.h"
#include "INetchannelInfo.h"
#include "UsedConvars.h"
#include <intrin.h> //VS2017 requires this for _ReturnAddress

//NETVAR HOOKS ARE AT THE BOTTOM

void AddNetvarProxyHook(RecvProp* netvar, DWORD& oldfunc, DWORD newfunc, hook_types m_HookType)
{
#ifdef DYLAN_VAC

#endif

	oldfunc = (DWORD)netvar->m_ProxyFn;
	netvar->m_ProxyFn = (RecvVarProxyFn)newfunc;
}

DWORD oAimProxyX = NULL;
void AntiAntiAimProxyX(const CRecvProxyData *pData, void *ent, void *pOut)
{
#ifdef PRINT_ANGLE_CHANGES
	float old = pOut ? *(float*)pOut : 0;
#endif

	if (oAimProxyX)
	{
		((void(*)(const CRecvProxyData *, void *, void *))oAimProxyX)(pData, ent, pOut);
	}

	if (!pOut)
		return;

	float val = pData->m_Value.m_Float;

	CBaseEntity* pent = (CBaseEntity*)ent;
	if (pent && pent->IsPlayer())
	{
		CPlayerrecord *record = g_LagCompensation.GetPlayerrecord(pent);
		if (record)
		{
			record->m_angPristineEyeAngles.x = val;

#ifdef PRINT_ANGLE_CHANGES
			if (val != old && !pent->IsLocalPlayer())
				printf("Pitch updated %f\n", val);
#endif
		}
	}
	//WriteFloat((uintptr_t)pOut, val);
}

DWORD oAimProxyY = NULL;
void AntiAntiAimProxyY(const CRecvProxyData *pData, void *ent, void *pOut)
{
#ifdef PRINT_ANGLE_CHANGES
	float old = pOut ? *(float*)pOut : 0;
#endif

	if (oAimProxyY)
	{
		((void(*)(const CRecvProxyData *, void *, void *))oAimProxyY)(pData, ent, pOut);
	}

	if (!pOut)
		return;

	float val = pData->m_Value.m_Float;

	CBaseEntity* pent = (CBaseEntity*)ent;
	if (pent && pent->IsPlayer())
	{
		CPlayerrecord *record = g_LagCompensation.GetPlayerrecord(pent);
		if (record)
		{
			record->m_angPristineEyeAngles.y = val;
#ifdef PRINT_ANGLE_CHANGES
			if (val != old && !pent->IsLocalPlayer())
				printf("Yaw updated %f\n", val);
#endif
		}
	}

	//WriteFloat((uintptr_t)pOut, val);
}

DWORD oAimProxyZ = NULL;
void AntiAntiAimProxyZ(const CRecvProxyData *pData, void *ent, void *pOut)
{
#ifdef PRINT_ANGLE_CHANGES
	float old = pOut ? *(float*)pOut : 0;
#endif

	if (oAimProxyY)
	{
		((void(*)(const CRecvProxyData *, void *, void *))oAimProxyZ)(pData, ent, pOut);
	}

	if (!pOut)
		return;

	float val = pData->m_Value.m_Float;

	CBaseEntity* pent = (CBaseEntity*)ent;
	if (pent && pent->IsPlayer() && pent != LocalPlayer.Entity)
	{
		CPlayerrecord *record = g_LagCompensation.GetPlayerrecord(pent);
		if (record)
		{
			record->m_angPristineEyeAngles.z = val;

#ifdef PRINT_ANGLE_CHANGES
			if (val != old && !pent->IsLocalPlayer())
				printf("Roll updated %f\n", val);
#endif
		}
	}

	//WriteFloat((uintptr_t)pOut, val);
}

DWORD oVelocityModifier = NULL;
void ReceivedVelocityModifier(const CRecvProxyData *pData, void *ent, void *pOut)
{
#ifdef PRINT_ANGLE_CHANGES
	float old = pOut ? *(float*)pOut : 0;
#endif

	if (oVelocityModifier)
	{
		((void(*)(const CRecvProxyData *, void *, void *))oVelocityModifier)(pData, ent, pOut);
	}

	if (!pOut)
		return;

	float val = pData->m_Value.m_Float;

	CBaseEntity* pent = (CBaseEntity*)ent;
	if (pent && pent->IsPlayer())
	{
		CPlayerrecord *record = g_LagCompensation.GetPlayerrecord(pent);
		if (record)
		{
			record->m_flPristineVelocityModifier = val;
		}
	}

	//WriteFloat((uintptr_t)pOut, val);
}

DWORD oPostPoneFireReadyTime = NULL;
void ReceivedPostPoneFireReadyTime(const CRecvProxyData *pData, void *ent, void *pOut)
{
	CBaseEntity* pent = (CBaseEntity*)ent;
	//if (pent && pent->GetOwner() && pent->GetOwner()->IsLocalPlayer())
	//{
	//	return;
	//}

	if (oPostPoneFireReadyTime)
	{
		((void(*)(const CRecvProxyData *, void *, void *))oPostPoneFireReadyTime)(pData, ent, pOut);
	}

	if (!pOut)
		return;
}

DWORD oReceivedTickbase = NULL;
void ReceivedTickbase(CRecvProxyData *pData, void *ent, void *pOut)
{
	CBaseEntity* pent = (CBaseEntity*)ent;
	if (pent && pent->IsLocalPlayer())
		LocalPlayer.m_nTickBase_Pristine = pData->m_Value.m_Int;

	if (oReceivedTickbase)
	{
		((void(*)(const CRecvProxyData*, void*, void*))oReceivedTickbase)(pData, ent, pOut);
	}
}

DWORD oReceivedFlashDuration = NULL;
void ReceivedFlashDuration(CRecvProxyData *pData, void *ent, void *pOut)
{
	CBaseEntity* pent = (CBaseEntity*)ent;
	if (pent)
	{
		CPlayerrecord *_playerRecord = g_LagCompensation.GetPlayerrecord(pent);
		if (_playerRecord)
			_playerRecord->m_bAwaitingFlashedResult = true;

		if (pent->IsLocalPlayer())
			LocalPlayer.m_flFlashedDuration = pData->m_Value.m_Float;
	}

	if (oReceivedFlashDuration)
	{
		((void(*)(const CRecvProxyData*, void*, void*))oReceivedFlashDuration)(pData, ent, pOut);
	}
}

DWORD oReceivedVPhysicsCollisionState = NULL;
void ReceivedVPhysicsCollisionState(CRecvProxyData *pData, void *ent, void *pOut)
{
	CBaseEntity* pent = (CBaseEntity*)ent;
	if (pent && pent->IsPlayer())
	{
		CPlayerrecord *_playerRecord = g_LagCompensation.GetPlayerrecord(pent);

		if (_playerRecord)
			_playerRecord->m_ivphysicsCollisionState = pData->m_Value.m_Int;

		if (oReceivedVPhysicsCollisionState && (!pent->IsLocalPlayer() || (_playerRecord && pent->GetSpawnTime() != _playerRecord->m_flSpawnTime)))
		{
			((void(*)(const CRecvProxyData*, void*, void*))oReceivedVPhysicsCollisionState)(pData, ent, pOut);
		}
	}
	else if (oReceivedVPhysicsCollisionState)
	{
		((void(*)(const CRecvProxyData*, void*, void*))oReceivedVPhysicsCollisionState)(pData, ent, pOut);
	}
}

DWORD oReceivedSimulationTime = NULL;
void ReceivedSimulationTime(CRecvProxyData *pData, void *ent, void *pOut)
{
	if (oReceivedSimulationTime)
		((void(*)(const CRecvProxyData*, void*, void*))oReceivedSimulationTime)(pData, ent, pOut);

	CBaseEntity* pent = (CBaseEntity*)ent;
	if (pent && pent->IsPlayer())
	{
		CPlayerrecord *_playerRecord = g_LagCompensation.GetPlayerrecord(pent);

		float *simulationtime = (float*)pOut;
		int simulationticks = TIME_TO_TICKS(*simulationtime);
		int tickbase = simulationticks - pData->m_Value.m_Int;
		int fuck = 1;
		
		//*simulationtime = TICKS_TO_TIME(tickbase);
	}
}

typedef void(*EnsureCapacityFn)(void *pVoid, int offsetToUtlVector, int len);
typedef void(*ResizeUtlVectorFn)(void *pVoid, int offsetToUtlVector, int len);

class CRecvPropExtra_UtlVector
{
public:
	DataTableRecvVarProxyFn m_DataTableProxyFn;	// If it's a datatable, then this is the proxy they specified.
	RecvVarProxyFn m_ProxyFn;				// If it's a non-datatable, then this is the proxy they specified.
	ResizeUtlVectorFn m_ResizeFn;			// The function used to resize the CUtlVector.
	EnsureCapacityFn m_EnsureCapacityFn;
	int m_ElementStride;					// Distance between each element in the array.
	int m_Offset;							// Offset of the CUtlVector from its parent structure.
	int m_nMaxElements;						// For debugging...
};

void ReceivedPoseParameter2(CRecvProxyData *pData, void *ent, void *pOut)
{
	
}

void DataTableRecvProxy_StaticDataTable(const RecvProp *pProp, void **pOut, void *pData, int objectID)
{
	float *pFloatArray = (float*)pData;
	for (int i = 0; i < 24; ++i)
	{
		if (pFloatArray[i] != 0.0f)
			DebugBreak();
	}
	*pOut = pData;
}

DWORD oReceivedPoseParameter = NULL;
void ReceivedPoseParameter(const RecvProp *pProp, void **pOut, void *pData, int objectID)
{
	CRecvPropExtra_UtlVector *pExtra = (CRecvPropExtra_UtlVector*)pProp->m_pExtraData;

	int iElement = pProp->m_ElementStride;
	((RecvProp*)pProp)->m_ProxyFn = &ReceivedPoseParameter2;
	//g_ClientState->m_nDeltaTick = -1;
	//Assert(iElement < pExtra->m_nMaxElements);

	// NOTE: this is cheesy, but it does the trick.
	CUtlVector<float> *pUtlVec = (CUtlVector<float>*)((char*)pData + 0 /*pExtra->m_Offset*/);
	DataTableRecvProxy_StaticDataTable(pProp, pOut, pData, objectID);

	if (oReceivedPoseParameter)
	{
		DWORD nagger = *(DWORD*)pData;
		if (nagger != 0)
			DebugBreak();
		auto blah = pUtlVec->Count();

		DWORD tmp = (DWORD)LocalPlayer.Entity;
		DWORD tmp2 = (DWORD)0x51933924 - tmp;

		//printf("%#010x | %#010x\n ", (DWORD)pOut, (DWORD)pData);

		//this is all the original function is
		DataTableRecvProxy_StaticDataTable(pProp, pOut, pData, objectID);
		((void(*)(const RecvProp *pProp, void **pOut, void *pData, int objectID))oReceivedPoseParameter)(pProp, pOut, pData, objectID);
	}
}

//////////////////////////
//                      //   
//     HOOK NETVARS     //
//                      //
//////////////////////////

bool HookedEyeAnglesRecv = false;

char *csplayerstr = new char[12]{ 62, 46, 37, 57, 41, 42, 22, 27, 3, 31, 8, 0 }; /*DT_CSPlayer*/
char *eyeang0str = new char[18]{ 23, 37, 27, 20, 29, 63, 3, 31, 59, 20, 29, 22, 31, 9, 33, 74, 39, 0 }; /*m_angEyeAngles[0]*/
char *eyeang1str = new char[18]{ 23, 37, 27, 20, 29, 63, 3, 31, 59, 20, 29, 22, 31, 9, 33, 75, 39, 0 }; /*m_angEyeAngles[1]*/
char *eyeang2str = new char[18]{ 23, 37, 27, 20, 29, 63, 3, 31, 59, 20, 29, 22, 31, 9, 33, 72, 39, 0 }; /*m_angEyeAngles[2]*/
char *baseentitystr = new char[14]{ 62, 46, 37, 56, 27, 9, 31, 63, 20, 14, 19, 14, 3, 0 }; /*DT_BaseEntity*/
char *dtbaseanimatingstr = new char[17]{ 62, 46, 37, 56, 27, 9, 31, 59, 20, 19, 23, 27, 14, 19, 20, 29, 0 }; /*DT_BaseAnimating*/
char *baseplayerstr = new char[14]{ 62, 46, 37, 56, 27, 9, 31, 42, 22, 27, 3, 31, 8, 0 }; /*DT_BasePlayer*/
char *m_flvelocitymodifierstr = new char[21]{ 23, 37, 28, 22, 44, 31, 22, 21, 25, 19, 14, 3, 55, 21, 30, 19, 28, 19, 31, 8, 0 }; /*m_flVelocityModifier*/
char *m_ntickbasestr = new char[12]{ 23, 37, 20, 46, 19, 25, 17, 56, 27, 9, 31, 0 }; /*m_nTickBase*/
char *mflflashdurationstr = new char[18]{ 23, 37, 28, 22, 60, 22, 27, 9, 18, 62, 15, 8, 27, 14, 19, 21, 20, 0 }; /*m_flFlashDuration*/
char *m_vphysicscollisionstatestr = new char[25]{ 23, 37, 12, 10, 18, 3, 9, 19, 25, 9, 57, 21, 22, 22, 19, 9, 19, 21, 20, 41, 14, 27, 14, 31, 0 }; /*m_vphysicsCollisionState*/


int Get_Prop(RecvTable *recvTable, const char *propName, RecvProp **prop)
{
	int extraOffset = 0;
	for (int i = 0; i < recvTable->m_nProps; ++i)
	{
		RecvProp *recvProp = &recvTable->m_pProps[i];
		RecvTable *child = recvProp->m_pDataTable;

		if (child && (ReadInt((uintptr_t)&child->m_nProps) > 0))
		{
			int tmp = Get_Prop(child, propName, prop);
			if (tmp)
				extraOffset += (ReadInt((uintptr_t)&recvProp->m_Offset) + tmp);
		}

		if (stricmp(recvProp->m_pVarName, propName))
			continue;

		if (prop)
			*prop = recvProp;

		return (ReadInt((uintptr_t)&recvProp->m_Offset) + extraOffset);
	}

	return extraOffset;
}

int Get_Prop(const char *tableName, const char *propName, RecvProp **prop)
{
	ClientClass *pClass = Interfaces::Client->GetAllClasses();
	while (pClass) {
		const char *pszName = pClass->m_pRecvTable->m_pNetTableName;
		if (!strcmp(pszName, tableName))
		{
			RecvTable *recvTable = pClass->m_pRecvTable;
			if (!recvTable)
				return 0;

			int offset = Get_Prop(recvTable, propName, prop);
			if (!offset)
				return 0;
			return offset;
		}
		pClass = (ClientClass*)ReadInt((uintptr_t)&pClass->m_pNext);
	}
	return 0;
}

void HookEyeAnglesProxy()
{
	//ClientClass *pClass = Interfaces::Client->GetAllClasses();

	DecStr(csplayerstr, 11);
	DecStr(eyeang0str, 17);
	DecStr(eyeang1str, 17);
	DecStr(baseentitystr, 13);
	DecStr(dtbaseanimatingstr, 16);
	DecStr(baseplayerstr, 13);
	DecStr(m_flvelocitymodifierstr, 20);
	DecStr(m_ntickbasestr, 11);
	DecStr(mflflashdurationstr, 17);
	DecStr(m_vphysicscollisionstatestr, 24);

	RecvProp* prop = nullptr;

	Get_Prop(csplayerstr, eyeang0str, &prop);
	if (prop)
		AddNetvarProxyHook(prop, oAimProxyX, (DWORD)&AntiAntiAimProxyX, _EyeAnglesProxyX);

	prop = nullptr;
	Get_Prop(csplayerstr, eyeang1str, &prop);
	if (prop)
		AddNetvarProxyHook(prop, oAimProxyY, (DWORD)&AntiAntiAimProxyY, _EyeAnglesProxyY);

	prop = nullptr;
	Get_Prop(csplayerstr, eyeang2str, &prop);
	if (prop)
		AddNetvarProxyHook(prop, oAimProxyZ, (DWORD)&AntiAntiAimProxyZ, _EyeAnglesProxyZ);

	prop = nullptr;
	Get_Prop(csplayerstr, m_flvelocitymodifierstr, &prop);
	if (prop)
		AddNetvarProxyHook(prop, oVelocityModifier, (DWORD)&ReceivedVelocityModifier, _VelocityModifier);

	//prop = nullptr;
	//Get_Prop("DT_WeaponCSBaseGun", "m_flPostponeFireReadyTime", &prop);
	//if (prop)
	//	AddNetvarProxyHook(prop, oPostPoneFireReadyTime, (DWORD)&ReceivedPostPoneFireReadyTime, _PostPoneFireReadyTime);

	prop = nullptr;
	Get_Prop(baseplayerstr, m_ntickbasestr, &prop);
	if (prop)
		AddNetvarProxyHook(prop, oReceivedTickbase, (DWORD)&ReceivedTickbase, _Tickbase);

	prop = nullptr;
	Get_Prop(csplayerstr, mflflashdurationstr, &prop);
	if (prop)
		AddNetvarProxyHook(prop, oReceivedFlashDuration, (DWORD)&ReceivedFlashDuration, _FlashDuration);

	prop = nullptr;
	Get_Prop(csplayerstr, m_vphysicscollisionstatestr, &prop);
	if (prop)
		AddNetvarProxyHook(prop, oReceivedVPhysicsCollisionState, (DWORD)&ReceivedVPhysicsCollisionState, _VPhysicsCollisionState);

	prop = nullptr;
	Get_Prop("DT_BaseEntity", "m_flSimulationTime", &prop);
	if (prop)
		AddNetvarProxyHook(prop, oReceivedSimulationTime, (DWORD)&ReceivedSimulationTime, _SimulationTime);


	//prop = nullptr;
	//Get_Prop("DT_BaseAnimating", "m_flPoseParameter", &prop);
	//if (prop)
	//{
	//	oReceivedPoseParameter = (DWORD)prop->GetDataTableProxyFn();
	//	prop->m_DataTableProxyFn = &ReceivedPoseParameter;
	//	AddNetvarProxyHook(prop, oReceivedPoseParameter, (DWORD)&ReceivedPoseParameter, _PoseParameter);
	//}

	EncStr(csplayerstr, 11);
	EncStr(eyeang0str, 17);
	EncStr(eyeang1str, 17);
	EncStr(baseentitystr, 13);
	EncStr(dtbaseanimatingstr, 16);
	EncStr(baseplayerstr, 13);
	EncStr(m_flvelocitymodifierstr, 20);
	EncStr(m_ntickbasestr, 11);
	EncStr(mflflashdurationstr, 17);
	EncStr(m_vphysicscollisionstatestr, 24);

	delete[] csplayerstr;
	delete[] eyeang0str;
	delete[] eyeang1str;
	delete[] baseentitystr;
	delete[] dtbaseanimatingstr;
	delete[] baseplayerstr;
	delete[] m_flvelocitymodifierstr;
	delete[] m_ntickbasestr;
	delete[] mflflashdurationstr;
	delete[] m_vphysicscollisionstatestr;
	
	HookedEyeAnglesRecv = true;
}
