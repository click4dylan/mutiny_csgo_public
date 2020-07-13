#include "precompiled.h"
#include "VTHook.h"
#include "Interfaces.h"
#include "ReadPacketEntities.h"
#include "bfwrite.h"
#include "IClientEntityList.h"
#include "CClientEntityList.h"
#include "IClientNetworkable.h"
#include "dt_send.h"
#include "NetVarManager.h"
#include "utlmemory.h"

ReadPacketEntitiesFn oReadPacketEntities;

// follow the relative jump - 32 bits
uint32_t GetRelativeJumpAddress(uint32_t adr, size_t offset) {
	uintptr_t   out;
	uint32_t    r;

	out = adr + offset;

	// get rel32 offset.
	r = *(uint32_t *)out;
	if (!r)
		return 0;

	// relative to address of next instruction.
	out = (out + 4) + r;

	return out;
}

// Networked ehandles use less bits to encode the serial number.
#define NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS	10
#define NUM_NETWORKED_EHANDLE_BITS					(MAX_EDICT_BITS + NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS)
#define INVALID_NETWORKED_EHANDLE_VALUE				((1 << NUM_NETWORKED_EHANDLE_BITS) - 1)

// Flags for delta encoding header
enum
{
	FHDR_ZERO = 0x0000,
	FHDR_LEAVEPVS = 0x0001,
	FHDR_DELETE = 0x0002,
	FHDR_ENTERPVS = 0x0004,
};


int FindNextSetBit(CBitVec<MAX_EDICTS>* vec, int num)
{
	static uint32_t func = FindMemoryPattern(EngineHandle, XorStr("55  8B  EC  8B  55  08  81  FA  ??  ??  ??  ??  7D  25  8B  C2  83  E2"));
	return ((int(__thiscall*)(CBitVec<MAX_EDICTS>*, int))func)(vec, num);
//	return StaticOffsets.GetOffsetValueByType<int(__thiscall*)(void*, int)>(_FindNextSetBit)(vec, num);
}

void ReadDeletions(CClientState *clientstateplus8, CEntityReadInfo &u)
{
   //static uint32_t adr = StaticOffsets.GetOffsetValue(_ReadDeletions);
	static uint32_t adr = FindMemoryPattern(EngineHandle, XorStr("E8  ??  ??  ??  ??  8B  47  ??  80  78  ??"));
	if (!adr)
		exit(EXIT_SUCCESS);
	static uint32_t func = GetRelativeJumpAddress(adr, 1);

	((void(__thiscall*)(CClientState*, CEntityReadInfo&))func)(clientstateplus8, u);
}

#pragma runtime_checks( "s", off )
void CL_CopyNewEntity(CEntityReadInfo &u, int iClass, int iSerialNum)
{
	static uint32_t adr = FindMemoryPattern(EngineHandle, XorStr("E8  ??  ??  ??  ??  83  C4  04  39  5F"));
	if (!adr)
		exit(EXIT_SUCCESS);
	static uint32_t func = GetRelativeJumpAddress(adr, 1);

	((void(__fastcall*)(CEntityReadInfo&, int, int))func)(u, iClass, iSerialNum);
	__asm add esp, 4
}
#pragma runtime_checks( "s", restore )

void ReadEnterPVS(CClientState *clientstate, CEntityReadInfo &u)
{
	int iClass = u.m_pBuf->ReadUBitLong(clientstate->m_nServerClassBits);

	int iSerialNum = u.m_pBuf->ReadUBitLong(NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS);

	CL_CopyNewEntity(u, iClass, iSerialNum);

	if (u.entityinfo.m_nNewEntity == u.entityinfo.m_nOldEntity) // that was a recreate
		u.entityinfo.NextOldEntity();
}

void CL_DeleteDLLEntity(int iEnt, bool bOnRecreatingAllEntities = false)
{
	IClientNetworkable *pNet = Interfaces::ClientEntList->GetClientNetworkable(iEnt);

	if (pNet)
	{
		ClientClass *pClientClass = pNet->GetClientClass();

		if (bOnRecreatingAllEntities)
		{
			pNet->SetDestroyedOnRecreateEntities();
		}

		pNet->Release();
	}
}

void ReadLeavePVS(CClientState *clientstate, CEntityReadInfo &u)
{
	// Sanity check.
	if (!u.m_bAsDelta)
	{
		//Assert(0); // cl.validsequence = 0;
		//ConMsg("WARNING: LeavePVS on full update");
		u.m_UpdateType = Failed;	// break out
		return;
	}

	//Assert(!u.m_pTo->transmit_entity.Get(u.m_nOldEntity));

	if (u.m_UpdateFlags & FHDR_DELETE)
	{
		CL_DeleteDLLEntity(u.entityinfo.m_nOldEntity);
	}

	u.entityinfo.NextOldEntity();
}

void ReadDeltaEnt(CClientState *clientstate, CEntityReadInfo &u)
{
	//55  8B  EC  51  53  56  8B  F1  57  8B  7E  28  8B  4F  20  85  C9
	//CL_CopyExistingEntity(u);
	static uint32_t func = FindMemoryPattern(EngineHandle, XorStr("55  8B  EC  51  53  56  8B  F1  57  8B  7E  28  8B  4F  20  85  C9"));
	return ((void(__thiscall*)(CEntityReadInfo&))func)(u);

	u.entityinfo.NextOldEntity();
}

void ReadPreserveEnt(CClientState *clientstate, CEntityReadInfo &u)
{
	if (!u.m_bAsDelta)  // Should never happen on a full update.
	{
#ifdef _DEBUG
		//ConMsg("WARNING: PreserveEnt on full update");
		printf("WARNING: PreserveEnt on full update");
#endif
		u.m_UpdateType = Failed;	// break out
		return;
	}

	// copy one of the old entities over to the new packet unchanged
	if (u.entityinfo.m_nOldEntity >= MAX_EDICTS || u.entityinfo.m_nNewEntity >= MAX_EDICTS)
	{
#ifdef _DEBUG
		printf("CL_ReadPreserveEnt: entity out of bounds (old=%d, new=%d)\n", u.entityinfo.m_nOldEntity, u.entityinfo.m_nNewEntity);
		DebugBreak();
#endif
		//Host_Error("CL_ReadPreserveEnt: u.m_nNewEntity == MAX_EDICTS");
		exit(EXIT_SUCCESS);
	}

	u.entityinfo.m_pTo->last_entity = u.entityinfo.m_nOldEntity;
	u.entityinfo.m_pTo->transmit_entity.Set(u.entityinfo.m_nOldEntity);

	// Zero overhead
	//if (cl_entityreport.GetBool())
	//	CL_RecordEntityBits(u.m_nOldEntity, 0);

	u.entityinfo.NextOldEntity();
}

// Returns false if you should stop reading entities.
inline UpdateType CL_DetermineUpdateType(CEntityReadInfo &u)
{
	if (u.m_bIsEntity && u.entityinfo.m_nNewEntity <= u.entityinfo.m_nOldEntity)
	{
		if (u.m_UpdateFlags & FHDR_ENTERPVS)
			return EnterPVS;
		else if (u.m_UpdateFlags & FHDR_LEAVEPVS)
			return LeavePVS;

		return DeltaEnt;
	}

	// If we're at the last entity, preserve whatever entities followed it in the old packet.
	// If newnum > oldnum, then the server skipped sending entities that it wants to leave the state alone for.
	if (!u.entityinfo.m_pFrom || u.entityinfo.m_nOldEntity > u.entityinfo.m_pFrom->last_entity)
		return Finished;

	// Preserve entities until we reach newnum (ie: the server didn't send certain entities because
	// they haven't changed).
	return PreserveEnt;
}

//-----------------------------------------------------------------------------
// Purpose: When a delta command is received from the server
//  We need to grab the entity # out of it any the bit settings, too.
//  Returns -1 if there are no more entities.
// Input  : &bRemove - 
//			&bIsNew - 
// Output : int
//-----------------------------------------------------------------------------
static inline void CL_ParseDeltaHeader(CEntityReadInfo &u)
{
	u.m_UpdateFlags = FHDR_ZERO;

	u.entityinfo.m_nNewEntity = u.entityinfo.m_nHeaderBase + 1 + u.m_pBuf->ReadUBitVar();

	u.entityinfo.m_nHeaderBase = u.entityinfo.m_nNewEntity;

	// leave pvs flag
	if (u.m_pBuf->ReadOneBit() == 0)
	{
		// enter pvs flag
		if (u.m_pBuf->ReadOneBit() != 0)
		{
			u.m_UpdateFlags |= FHDR_ENTERPVS;
		}
	}
	else
	{
		u.m_UpdateFlags |= FHDR_LEAVEPVS;

		// Force delete flag
		if (u.m_pBuf->ReadOneBit() != 0)
		{
			u.m_UpdateFlags |= FHDR_DELETE;
		}
	}
}

void CEntityInfo::NextOldEntity(void)
{
	if (m_pFrom)
	{
		m_nOldEntity = FindNextSetBit(&m_pFrom->transmit_entity, m_nOldEntity + 1);

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

void CEntityInfo::NextNewEntity(void)
{
	m_nNewEntity = FindNextSetBit(&m_pTo->transmit_entity, m_nNewEntity + 1);

	if (m_nNewEntity < 0)
	{
		// Sentinel/end of list....
		m_nNewEntity = ENTITY_SENTINEL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the receive table for the specified entity
// Input  : *pEnt - 
// Output : RecvTable*
//-----------------------------------------------------------------------------
static inline RecvTable* GetEntRecvTable(int entnum)
{
	IClientNetworkable *pNet = Interfaces::ClientEntList->GetClientNetworkable(entnum);
	if (pNet)
		return pNet->GetClientClass()->m_pRecvTable;
	else
		return NULL;
}

class DecodeInfo;
class SendProp;

typedef struct
{
	// Encode a value.
	// pStruct : points at the base structure
	// pVar    : holds data in the correct type (ie: PropVirtualsInt will have DVariant::m_Int set).
	// pProp   : describes the property to be encoded.
	// pOut    : the buffer to encode into.
	// objectID: for debug output.
	void(*Encode)(const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID);

	// Decode a value.
	// See the DecodeInfo class for a description of the parameters.
	void(*Decode)(DecodeInfo *pInfo);

	// Compare the deltas in the two buffers. The property in both buffers must be fully decoded
	int(*CompareDeltas)(const SendProp *pProp, bf_read *p1, bf_read *p2);

	// Used for the local single-player connection to copy the data straight from the server ent into the client ent.
	void(*FastCopy)(
		const SendProp *pSendProp,
		const RecvProp *pRecvProp,
		const unsigned char *pSendData,
		unsigned char *pRecvData,
		int objectID);

	// Return a string with the name of the type ("DPT_Float", "DPT_Int", etc).
	const char*		(*GetTypeNameString)();

	// Returns true if the property's value is zero.
	// NOTE: this does NOT strictly mean that it would encode to zeros. If it were a float with
	// min and max values, a value of zero could encode to some other integer value.
	bool(*IsZero)(const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp);

	// This writes a zero value in (ie: a value that would make IsZero return true).
	void(*DecodeZero)(DecodeInfo *pInfo);

	// This reades this property from stream p and returns true, if it's a zero value
	bool(*IsEncodedZero) (const SendProp *pProp, bf_read *p);
	void(*SkipProp) (const SendProp *pProp, bf_read *p);
} PropTypeFns;

PropTypeFns *g_PropTypeFns = nullptr;

bool RecvTable_ReadFieldList_Guts_False(RecvTable* pTable, bf_read& read, CSerializedEntity* pSerializedEntity)
{
#if 0
	static DWORD proptypefnadr = FindMemoryPattern(EngineHandle, XorStr("8B  04  85  ??  ??  ??  ??  FF  D0  0F  B7  07  43  83  C4  08  3B  D8  7C"));
	if (!proptypefnadr)
		exit(EXIT_SUCCESS);
	g_PropTypeFns = (PropTypeFns*) (*(DWORD*)(proptypefnadr + 3) - 32);

	//CSerializedEntity::Clear(pSerializedEntity);
	static DWORD adr1 = FindMemoryPattern(EngineHandle, XorStr("56  8B  F1  B8  ??  ??  ??  ??  66  39  46  02  A1  ??  ??  ??  ??  FF  76  08  8B  08  8B  01  74  0F  FF  50  14  A1  ??  ??  ??  ??  FF  76  10  8B  08  8B  01  FF  50  14  33  C0  C7  46  ??  ??  ??  ??  ??  C7  46  ??  ??  ??  ??  ??"));
	if (!adr1)
		exit(EXIT_SUCCESS);
	((void(__thiscall*)(CSerializedEntity*))adr1)(pSerializedEntity);

	//CSerializedEntity::ReadFieldPaths(pSerializedEntity, read, 0);
	static DWORD adr2 = FindMemoryPattern(EngineHandle, XorStr("55  8B  EC  83  EC  14  56  57  FF  75  08  8B  F1  8D  4D  EC  E8  ??  ??  ??  ??  E8  ??  ??  ??  ??  8B  F8  83  FF  FF  74  56  53  8B  5D  0C  0F  B7  46  02  66  39  06  75  14  83  7E  08  00  8B  CE  75  04  6A  02  EB  03  03  C0  50  E8  ??  ??  ??  ??  0F  B7  0E  8B  46  08  66  89  3C  48  66  FF  06  85  DB  74  14  8B  45  F8  8B  CB  89  45  08  8D  45  08  50  FF  73  0C  E8  ??  ??  ??  ??  8D  4D  EC  E8  ??  ??  ??  ??"));
	if (!adr2)
		exit(EXIT_SUCCESS);
	((void(__thiscall*)(CSerializedEntity*, bf_read&, int))adr2)(pSerializedEntity, read, 0);

	if (pTable)
	{
		DWORD m_pDecoder = (DWORD)pTable->m_pDecoder;
		if (!m_pDecoder)
		{
			//v7 = pTable->m_pNetTableName;
			//Error("RecvTable_ReadFieldList: table '%s' missing a decoder.");
			exit(EXIT_SUCCESS);
		}
		int numBitsRead;
		if (read.m_pData)
		{
			DWORD v9 = (DWORD)((char *)read.m_pDataIn - (char *)read.m_pData);
			int nDataBits = read.m_nDataBits;
			int nCurOfs_Plus_nAdjust = 8 * ((read.m_nDataBytes & 3) + 4 * (v9 / 4)) - read.m_nBitsAvail;// nCurOfs + nAdjust
			if (nCurOfs_Plus_nAdjust < (signed int)nDataBits)
				nDataBits = nCurOfs_Plus_nAdjust;
			numBitsRead = nDataBits;
		}
		else
		{
			numBitsRead = 0;
		}

		for (__int16 i = 0; i < pSerializedEntity->size; ++i)
		{
			int numBitsRead_1;
			if (read.m_pData)
			{
				DWORD v15 = (DWORD)((char *)read.m_pDataIn - (char *)read.m_pData);
				numBitsRead_1 = read.m_nDataBits;
				if (8 * ((read.m_nDataBytes & 3) + 4 * v15) - read.m_nBitsAvail < numBitsRead_1)
					numBitsRead_1 = 8 * ((read.m_nDataBytes & 3) + 4 * v15) - read.m_nBitsAvail;
			}
			else
			{
				numBitsRead_1 = 0;
			}

			int v16 = *(pSerializedEntity->sendPropIndices + i);
			pSerializedEntity->bytesReadDeltas[i] = read.GetNumBitsRead() - numBitsRead;
			SendProp *pProp = (SendProp*)(*(DWORD *)(*(DWORD *)(m_pDecoder + 52) + 4 * v16));
			g_PropTypeFns[pProp->GetType()].SkipProp(pProp, &read);
		}

		int numBitsRead_2;
		if (read.m_pData)
		{
			numBitsRead_2 = read.m_nDataBits;
			if (8 * ((read.m_nDataBytes & 3) + 4 * (read.m_pDataIn - read.m_pData)) - read.m_nBitsAvail < numBitsRead_2)
				numBitsRead_2 = 8 * ((read.m_nDataBytes & 3) + 4 * (read.m_pDataIn - read.m_pData)) - read.m_nBitsAvail;
		}
		else
		{
			numBitsRead_2 = 0;
		}
		read.Seek(numBitsRead);

		//CSerializedEntity::PackWithFieldData(pSerializedEntity, read, numBitsRead_2 - numBitsRead);
		static DWORD adr = FindMemoryPattern(EngineHandle, XorStr("55  8B  EC  51  53  8B  5D  0C  56  57  8B  F9  53  8B  47  08  8B  77  0C  89  45  FC  0F  B7  07  50  E8  ??  ??  ??  ??  0F  B7  07  03  C0  50  FF  75  FC  FF  77  08  E8  ??  ??  ??  ??  0F  B7  07  C1  E0  02  50  56  FF  77  0C  E8  ??  ??  ??  ??  8B  4D  08  83  C4  18  53  FF  77  10  E8  ??  ??  ??  ??  8B  57  10  8B  CB  83  E1  07  76  0C  B0  01  C1  EB  03  D2  E0  FE  C8  20  04  13  A1  ??  ??  ??  ??  FF  75  FC  8B  08  8B  01  FF  50  14"));
		if (!adr)
			exit(EXIT_SUCCESS);
		((void(__thiscall*)(CSerializedEntity*, bf_read&, int))adr)(pSerializedEntity, read, numBitsRead_2 - numBitsRead);
		read.Seek(numBitsRead_2);
		return true;
	}
	else
	{
		//Error("RecvTable_ReadFieldListt: Missing RecvTable for class\n");
		exit(EXIT_SUCCESS);
	}
#endif
	return false;
}

class CSendNode
{
public:

	CSendNode();
	~CSendNode();

	int				GetNumChildren() const;
	CSendNode*		GetChild(int i) const;


	// Returns true if the specified prop is in this node or any of its children.
	bool			IsPropInRecursiveProps(int i) const;

	// Each datatable property (without SPROP_PROXY_ALWAYS_YES set) gets a unique index here.
	// The engine stores arrays of CSendProxyRecipients with the results of the proxies and indexes the results
	// with this index.
	//
	// Returns DATATABLE_PROXY_INDEX_NOPROXY if the property has SPROP_PROXY_ALWAYS_YES set.
	unsigned short	GetDataTableProxyIndex() const;
	void			SetDataTableProxyIndex(unsigned short val);

	// Similar to m_DataTableProxyIndex, but doesn't use DATATABLE_PROXY_INDEX_INVALID,
	// so this can be used to index CDataTableStack::m_pProxies. 
	unsigned short	GetRecursiveProxyIndex() const;
	void			SetRecursiveProxyIndex(unsigned short val);


public:

	// Child datatables.
	CUtlVector<CSendNode*>	m_Children;

	// The datatable property that leads us to this CSendNode.
	// This indexes the CSendTablePrecalc or CRecvDecoder's m_DatatableProps list.
	// The root CSendNode sets this to -1.
	short					m_iDatatableProp;

	// The SendTable that this node represents.
	// ALL CSendNodes have this.
	const SendTable	*m_pTable;

	//
	// Properties in this table.
	//

	// m_iFirstRecursiveProp to m_nRecursiveProps defines the list of propertise
	// of this node and all its children.
	unsigned short	m_iFirstRecursiveProp;
	unsigned short	m_nRecursiveProps;


	// See GetDataTableProxyIndex().
	unsigned short	m_DataTableProxyIndex;

	// See GetRecursiveProxyIndex().
	unsigned short	m_RecursiveProxyIndex;
};

class CDTISendTable;

class CFastLocalTransferPropInfo
{
public:
	unsigned short	m_iRecvOffset;
	unsigned short	m_iSendOffset;
	unsigned short	m_iProp;
};

class CFastLocalTransferInfo
{
public:
	CUtlVector<CFastLocalTransferPropInfo> m_FastInt32;
	CUtlVector<CFastLocalTransferPropInfo> m_FastInt16;
	CUtlVector<CFastLocalTransferPropInfo> m_FastInt8;
	CUtlVector<CFastLocalTransferPropInfo> m_FastVector;
	CUtlVector<CFastLocalTransferPropInfo> m_OtherProps;	// Props that must be copied slowly (proxies and all).
};

__declspec(align(4)) class CSendTablePrecalc
{
public:
	CSendTablePrecalc();
	virtual				~CSendTablePrecalc();

	// This function builds the flat property array given a SendTable.
	bool				SetupFlatPropertyArray();

	int					GetNumProps() const;
	const SendProp*		GetProp(int i) const;

	int					GetNumDatatableProps() const;
	const SendProp*		GetDatatableProp(int i) const;

	SendTable*			GetSendTable() const;
	CSendNode*			GetRootNode();

	int			GetNumDataTableProxies() const;
	void		SetNumDataTableProxies(int count);


public:

	class CProxyPathEntry
	{
	public:
		unsigned short m_iDatatableProp;	// Lookup into CSendTablePrecalc or CRecvDecoder::m_DatatableProps.
		unsigned short m_iProxy;
	};
	class CProxyPath
	{
	public:
		unsigned short m_iFirstEntry;	// Index into m_ProxyPathEntries.
		unsigned short m_nEntries;
	};
	DWORD unk[2];
	CUtlVector<CProxyPathEntry> m_ProxyPathEntries;	// For each proxy index, this is all the DT proxies that generate it.
	CUtlVector<CProxyPath> m_ProxyPaths;			// CProxyPathEntries lookup into this.

	// These are what CSendNodes reference.
	// These are actual data properties (ints, floats, etc).
	CUtlVector<const SendProp*>	m_Props;

	DWORD unk2[2];

	// Each datatable in a SendTable's tree gets a proxy index, and its properties reference that.
	CUtlVector<unsigned char> m_PropProxyIndices;

	// CSendNode::m_iDatatableProp indexes this.
	// These are the datatable properties (SendPropDataTable).
	CUtlVector<const SendProp*>	m_DatatableProps;

	// This is the property hierarchy, with the nodes indexing m_Props.
	CSendNode				m_Root;

	// From whence we came.
	SendTable				*m_pSendTable;

	// For instrumentation.
	CDTISendTable			*m_pDTITable;

	// This is precalculated in single player to allow faster direct copying of the entity data
	// from the server entity to the client entity.
	CFastLocalTransferInfo	m_FastLocalTransfer;

	// This tells how many data table properties there are without SPROP_PROXY_ALWAYS_YES.
	// Arrays allocated with this size can be indexed by CSendNode::GetDataTableProxyIndex().
	int						m_nDataTableProxies;

	// Map prop offsets to indices for properties that can use it.
	//CUtlMap<unsigned short, unsigned short> m_PropOffsetToIndexMap;
	char m_PropOffsetToIndexMap[112];
};

inline int CSendTablePrecalc::GetNumProps() const
{
	return m_Props.Count();
}


inline int CSendTablePrecalc::GetNumDatatableProps() const
{
	return m_DatatableProps.Count();
}

inline const SendProp* CSendTablePrecalc::GetDatatableProp(int i) const
{
	return m_DatatableProps[i];
}

inline SendTable* CSendTablePrecalc::GetSendTable() const
{
	return m_pSendTable;
}

inline CSendNode* CSendTablePrecalc::GetRootNode()
{
	return &m_Root;
}

inline int CSendTablePrecalc::GetNumDataTableProxies() const
{
	return m_nDataTableProxies;
}


inline void CSendTablePrecalc::SetNumDataTableProxies(int count)
{
	m_nDataTableProxies = count;
}


inline const SendProp* CSendTablePrecalc::GetProp(int i) const
{
	return m_Props[i];
}


class CSendNode;

class CDatatableStack
{
public:

	CDatatableStack(CSendTablePrecalc *pPrecalc, unsigned char *pStructBase, int objectID);

	// This must be called before accessing properties.
	void Init(bool bExplicitRoutes = false);

	// The stack is meant to be used by calling SeekToProp with increasing property
	// numbers.
	void			SeekToProp(int iProp);

	bool			IsCurProxyValid() const;
	bool			IsPropProxyValid(int iProp) const;
	int				GetCurPropIndex() const;

	unsigned char*	GetCurStructBase() const;

	int				GetObjectID() const;

	// Derived classes must implement this. The server gets one and the client gets one.
	// It calls the proxy to move to the next datatable's data.
	virtual void RecurseAndCallProxies(CSendNode *pNode, unsigned char *pStructBase) = 0;


public:
	CSendTablePrecalc *m_pPrecalc;

	enum
	{
		MAX_PROXY_RESULTS = 64
	};

	// These point at the various values that the proxies returned. They are setup once, then 
	// the properties index them.
	unsigned char *m_pProxies[MAX_PROXY_RESULTS];
	unsigned char *m_pStructBase;
	int m_iCurProp;

protected:

	const SendProp *m_pCurProp;

	int m_ObjectID;

	bool m_bInitted;
};

CDatatableStack::CDatatableStack(CSendTablePrecalc *pPrecalc, unsigned char *pStructBase, int objectID)
{
	m_pPrecalc = pPrecalc;

	m_pStructBase = pStructBase;
	m_ObjectID = objectID;

	m_iCurProp = 0;
	m_pCurProp = NULL;

	m_bInitted = false;

#ifdef _DEBUG
	memset(m_pProxies, 0xFF, sizeof(m_pProxies));
#endif
}

void CDatatableStack::Init(bool bExplicitRoutes)
{
	if (bExplicitRoutes)
	{
		memset(m_pProxies, 0xFF, sizeof(m_pProxies[0]) * m_pPrecalc->m_ProxyPaths.Count());
	}
	else
	{
		// Walk down the tree and call all the datatable proxies as we hit them.
		RecurseAndCallProxies(&m_pPrecalc->m_Root, m_pStructBase);
	}

	m_bInitted = true;
}

inline bool CDatatableStack::IsPropProxyValid(int iProp) const
{
	return m_pProxies[m_pPrecalc->m_PropProxyIndices[iProp]] != 0;
}

inline bool CDatatableStack::IsCurProxyValid() const
{
	return m_pProxies[m_pPrecalc->m_PropProxyIndices[m_iCurProp]] != 0;
}

inline int CDatatableStack::GetCurPropIndex() const
{
	return m_iCurProp;
}

inline unsigned char* CDatatableStack::GetCurStructBase() const
{
	return m_pProxies[m_pPrecalc->m_PropProxyIndices[m_iCurProp]];
}

inline void CDatatableStack::SeekToProp(int iProp)
{
	//Assert(m_bInitted);

	m_iCurProp = iProp;
	m_pCurProp = m_pPrecalc->GetProp(iProp);
}

inline int CDatatableStack::GetObjectID() const
{
	return m_ObjectID;
}

class CDTIRecvTable;

class CClientSendProp
{
public:

	CClientSendProp();
	~CClientSendProp();

	const char*	GetTableName() { return m_pTableName; }
	void		SetTableName(char *pName) { m_pTableName = pName; }


private:

	char	*m_pTableName;	// For DPT_DataTable properties.. this tells the table name.
};


class CClientSendTable
{
public:
	CClientSendTable();
	~CClientSendTable();

	int							GetNumProps() const { return m_SendTable.m_nProps; }
	CClientSendProp*			GetClientProp(int i) { return &m_Props[i]; }

	const char*					GetName() { return m_SendTable.GetName(); }
	SendTable*					GetSendTable() { return &m_SendTable; }


public:

	SendTable					m_SendTable;
	CUtlVector<CClientSendProp>	m_Props;	// Extra data for the properties.
};

class CRecvDecoder
{
public:

	CRecvDecoder();

	const char*		GetName() const;
	SendTable*		GetSendTable() const;
	RecvTable*		GetRecvTable() const;

	int				GetNumProps() const;
	const RecvProp*	GetProp(int i) const;
	const SendProp*	GetSendProp(int i) const;

	int				GetNumDatatableProps() const;
	const RecvProp*	GetDatatableProp(int i) const;


public:

	RecvTable			*m_pTable;
	CClientSendTable	*m_pClientSendTable;

	// This is from the data that we've received from the server.
	CSendTablePrecalc	m_Precalc;

	// This mirrors m_Precalc.m_Props. 
	CUtlVector<const RecvProp*>	m_Props;
	CUtlVector<const RecvProp*>	m_DatatableProps;

	CDTIRecvTable *m_pDTITable;
};

class CClientDatatableStack : public CDatatableStack
{
public:
	CClientDatatableStack(CRecvDecoder *pDecoder, unsigned char *pStructBase, int objectID) :
		CDatatableStack(&pDecoder->m_Precalc, pStructBase, objectID)
	{
		m_pDecoder = pDecoder;
	}

	inline unsigned char*	CallPropProxy(CSendNode *pNode, int iProp, unsigned char *pStructBase)
	{
		const RecvProp *pProp = m_pDecoder->GetDatatableProp(iProp);

		void *pVal = NULL;

		//Assert(pProp);

		pProp->GetDataTableProxyFn()(
			pProp,
			&pVal,
			pStructBase + pProp->GetOffset(),
			GetObjectID()
			);

		return (unsigned char*)pVal;
	}

	virtual void RecurseAndCallProxies(CSendNode *pNode, unsigned char *pStructBase)
	{
		// Remember where the game code pointed us for this datatable's data so 
		m_pProxies[pNode->GetRecursiveProxyIndex()] = pStructBase;

		for (int iChild = 0; iChild < pNode->GetNumChildren(); iChild++)
		{
			CSendNode *pCurChild = pNode->GetChild(iChild);

			unsigned char *pNewStructBase = NULL;
			if (pStructBase)
			{
				pNewStructBase = CallPropProxy(pCurChild, pCurChild->m_iDatatableProp, pStructBase);
			}

			RecurseAndCallProxies(pCurChild, pNewStructBase);
		}
	}

	class CRecvProxyCaller
	{
	public:
		static inline unsigned char* CallProxy(CClientDatatableStack *pStack, unsigned char *pStructBase, unsigned short iDatatableProp)
		{
			const RecvProp *pProp = pStack->m_pDecoder->GetDatatableProp(iDatatableProp);

			void *pVal = NULL;
			pProp->GetDataTableProxyFn()(
				pProp,
				&pVal,
				pStructBase + pProp->GetOffset(),
				pStack->m_ObjectID
				);

			return (unsigned char*)pVal;
		}
	};

	//inline unsigned char* UpdateRoutesExplicit()
	//{
	//	return UpdateRoutesExplicit_Template(this, (CRecvProxyCaller*)NULL);
	//}


public:

	CRecvDecoder	*m_pDecoder;
};


class DecodeInfo : public CRecvProxyData
{
public:

	// Copy everything except val.
	void			CopyVars(const DecodeInfo *pOther);

public:

	//
	// NOTE: it's valid to pass in m_pRecvProp and m_pData and m_pSrtuct as null, in which 
	// case the buffer is advanced but the property is not stored anywhere. 
	//
	// This is used by SendTable_CompareDeltas.
	//
	void			*m_pStruct;			// Points at the base structure
	void			*m_pData;			// Points at where the variable should be encoded. 

	const SendProp 	*m_pProp;		// Provides the client's info on how to decode and its proxy.
	bf_read			*m_pIn;			// The buffer to get the encoded data from.

	char			m_TempStr[DT_MAX_STRING_BUFFERSIZE];	// m_Value.m_pString is set to point to this.
};

//TODO: FINISH ME
#if 0
bool RecvTable_Decode(RecvTable *pTable, void *pStruct, bf_read *pIn, int objectID)
{
	CRecvDecoder *pDecoder = pTable->m_pDecoder;
	if (!pDecoder)
	{
		exit(EXIT_SUCCESS);
		//printf("RecvTable_Decode: table '%s' missing a decoder.", pTable->GetName());
	}

	// While there are properties, decode them.. walk the stack as you go.
	CClientDatatableStack theStack(pDecoder, (unsigned char*)pStruct, objectID);

	CBitRead read("CFlattenedSerializer::Decode", nullptr, 0);

	theStack.Init();
	int iStartBit = 0, nIndexBits = 0, iLastBit = pIn->GetNumBitsRead();
	int iProp;
	CDeltaBitsReader deltaBitsReader(pIn);
	while (-1 != (iProp = deltaBitsReader.ReadNextPropIndex()))
	{
		theStack.SeekToProp(iProp);

		const RecvProp *pProp = pDecoder->GetProp(iProp);

		DecodeInfo decodeInfo;
		decodeInfo.m_pStruct = theStack.GetCurStructBase();
		decodeInfo.m_pData = theStack.GetCurStructBase() + pProp->GetOffset();
		decodeInfo.m_pRecvProp = theStack.IsCurProxyValid() ? pProp : NULL; // Just skip the data if the proxies are screwed.
		decodeInfo.m_pProp = pDecoder->GetSendProp(iProp);
		decodeInfo.m_pIn = pIn;
		decodeInfo.m_ObjectID = objectID;

		g_PropTypeFns[pProp->GetType()].Decode(&decodeInfo);
	}

	return !pIn->IsOverflowed();
}

void CL_CopyExistingEntity(CEntityReadInfo &u)
{
	int start_bit = u.m_pBuf->GetNumBitsRead();

	IClientNetworkable *pEnt = Interfaces::ClientEntList->GetClientNetworkable(u.entityinfo.m_nNewEntity);
	if (!pEnt)
	{
		//Host_Error("CL_CopyExistingEntity: missing client entity %d.\n", u.entityinfo.m_nNewEntity);
		return;
	}

	//Assert(u.m_pFrom->transmit_entity.Get(u.m_nNewEntity));

	// Read raw data from the network stream
	pEnt->PreDataUpdate(DATA_UPDATE_DATATABLE_CHANGED);

	RecvTable *pRecvTable = GetEntRecvTable(u.entityinfo.m_nNewEntity);

	if (!pRecvTable)
	{
		//Host_Error("CL_ParseDelta: invalid recv table for ent %d.\n", u.entityinfo.m_nNewEntity);
		return;
	}

	CSerializedEntity* pSerializedEntity = u.m_pSerializedEntity;
	RecvTable_ReadFieldList_Guts_False(pRecvTable, *u.m_pBuf, pSerializedEntity);

	RecvTable_Decode(pRecvTable, pEnt->GetDataTableBasePtr(), pSerializedEntity, u.entityinfo.m_nNewEntity);

	CL_AddPostDataUpdateCall(u, u.entityinfo.m_nNewEntity, DATA_UPDATE_DATATABLE_CHANGED);

	u.entityinfo.m_pTo->last_entity = u.entityinfo.m_nNewEntity;
	//Assert(!u.m_pTo->transmit_entity.Get(u.entityinfo.m_nNewEntity));
	u.entityinfo.m_pTo->transmit_entity.Set(u.entityinfo.m_nNewEntity);

	int bit_count = u.m_pBuf->GetNumBitsRead() - start_bit;

	if (CL_IsPlayerIndex(u.entityinfo.m_nNewEntity))
	{
		if (u.entityinfo.m_nNewEntity == g_ClientState->m_nPlayerSlot + 1)
		{
			u.m_nLocalPlayerBits += bit_count;
		}
		else
		{
			u.m_nOtherPlayerBits += bit_count;
		}
	}
}
#endif

void __fastcall Hooked_ReadPacketEntities(CClientState *clientstateplus8, DWORD edx, CEntityReadInfo &u)
{
	CClientState *clientstate = (CClientState*)((DWORD)clientstateplus8 - 8);
	// Loop until there are no more entities to read

	u.entityinfo.NextOldEntity();

	UpdateType updateType = u.m_UpdateType;

	while (updateType < Finished)
	{
		u.entityinfo.m_nHeaderCount--;

		u.m_bIsEntity = u.entityinfo.m_nHeaderCount >= 0;

		if (u.m_bIsEntity)
		{
			CL_ParseDeltaHeader(u);
		}

		Preserve:
		updateType = CL_DetermineUpdateType(u);

		if (updateType == Finished)
			break;

		// Figure out what kind of an update this is.
		switch (updateType)
		{
			case EnterPVS:		ReadEnterPVS(clientstate, u);
				break;
			case LeavePVS:		ReadLeavePVS(clientstate, u);
				break;
			case DeltaEnt:		ReadDeltaEnt(clientstate, u);
				break;
			case PreserveEnt:	ReadPreserveEnt(clientstate, u);
				goto Preserve; //for some reason, in csgo, possibly due to bug CL_ReadPreserveEnt: u.m_nNewEntity == MAX_EDICTS in 2017, it does this now
				//break;
			default:	
#ifdef _DEBUG
				//DevMsg(1, "ReadPacketEntities: unknown updatetype %i\n", u.m_UpdateType);
#endif
				break;
		}
	}

	u.m_UpdateType = updateType;

	// Now process explicit deletes 
	if (u.m_bAsDelta && u.m_UpdateType == Finished)
	{
		ReadDeletions(clientstateplus8, u);
	}

	// Something didn't parse...
	if (u.m_pBuf->IsOverflowed())
	{
		//Host_Error("CL_ParsePacketEntities:  buffer read overflow\n");
		exit(EXIT_SUCCESS);
	}

	// If we get an uncompressed packet, then the server is waiting for us to ack the validsequence
	// that we got the uncompressed packet on. So we stop reading packets here and force ourselves to
	// send the clc_move on the next frame.

	if (!u.m_bAsDelta)
	{
		clientstate->m_flNextCmdTime = 0.0; // answer ASAP to confirm full update tick
	}
}