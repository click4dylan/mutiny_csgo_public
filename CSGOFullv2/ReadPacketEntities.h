#pragma once
#include "dataupdatetypes.h"
#include "utlvector.h"
#include "bitvec.h"
class bf_read;

enum UpdateType
{
	EnterPVS = 0x0,
	LeavePVS = 0x1,
	DeltaEnt = 0x2,
	PreserveEnt = 0x3,
	Finished = 0x4,
	Failed = 0x5,
};

#pragma pack(push, 1)
struct CPostDataUpdateCall
{
	int m_iEnt;
	DataUpdateType_t m_UpdateType;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CClientFrame
{
	void *vtable;
	int last_entity; //unknown if correct
	int tick_count;//unknown if correct
	DWORD unknown;
	CBitVec<MAX_EDICTS>	transmit_entity;
	CBitVec<MAX_EDICTS>	*from_baseline;	// if bit n is set, this entity was send as update from baseline
	CBitVec<MAX_EDICTS>	*transmit_always; // if bit is set, don't do PVS checks before sending (HLTV only)

	CClientFrame*		m_pNext;

private:

	// Index of snapshot entry that stores the entities that were active and the serial numbers
	// for the frame number this packed entity corresponds to
	// m_pSnapshot MUST be private to force using SetSnapshot(), see reference counters
	void/*CFrameSnapshot*/		*m_pSnapshot;
};
#pragma pack(pop)

#define ENTITY_SENTINEL 9999

int FindNextSetBit(CBitVec<MAX_EDICTS>& vec, int num);

#pragma pack(push, 1)
struct CEntityInfo
{
	void *entityinfo_vtable;
	CClientFrame *m_pFrom;
	CClientFrame *m_pTo;
	int m_nOldEntity;
	int m_nNewEntity;
	int m_nHeaderBase;
	int m_nHeaderCount;

#if 1
	void	NextOldEntity(void);
	void	NextNewEntity(void);
#else
	inline void	NextOldEntity(void)
	{
		if (m_pFrom)
		{
			
			m_nOldEntity = FindNextSetBit(m_pFrom->transmit_entity, m_nOldEntity + 1);

			if (m_nOldEntity < 0)
			{
				// Sentinel/end of list....
				m_nOldEntity = ENTITY_SENTINEL;
			}
		}
		else
		{
			m_nOldEntity = ENTITY_SENTINEL;
		}
	}

	inline void	NextNewEntity(void)
	{
		m_nNewEntity = FindNextSetBit(m_pTo->transmit_entity, m_nNewEntity + 1); 

		if (m_nNewEntity < 0)
		{
			// Sentinel/end of list....
			m_nNewEntity = ENTITY_SENTINEL;
		}
	}
#endif
};
#pragma pack(pop)

struct CSerializedEntity;

#pragma pack(push, 1)
struct CEntityReadInfo
{
	CEntityInfo entityinfo;
	UpdateType m_UpdateType;
	bool m_bAsDelta;
	char fuckpad[3];
	CSerializedEntity *m_pSerializedEntity;
	bf_read *m_pBuf;
	int m_UpdateFlags;
	bool m_bIsEntity;
	char m_bIsEntityPad[3];
	int m_nBaseline;
	int unknown;
	int m_nLocalPlayerBits;
	int m_nOtherPlayerBits;
	CPostDataUpdateCall m_PostDataUpdateCalls[2048]; //MAX_EDICTS
	int m_nPostDataUpdateCalls;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CSerializedEntity
{
	__int16 size;
	__int16 size0;
	__int32 bytesRead;
	__int32 *sendPropIndices;
	__int32 *bytesReadDeltas;
	bf_read *m_pData;
	bool unkBool;
	char pad[3];
};
#pragma pack(pop)


extern void __fastcall Hooked_ReadPacketEntities(CClientState *clientstate, DWORD edx, CEntityReadInfo &u);

using ReadPacketEntitiesFn = void(__thiscall*)(CClientState *clientstate, CEntityReadInfo &u);
extern ReadPacketEntitiesFn oReadPacketEntities;