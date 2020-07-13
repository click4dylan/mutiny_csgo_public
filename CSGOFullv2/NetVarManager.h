#pragma once
#include <vector>
#include <fstream>
//#include "cx_strenc.h"

typedef enum
{
	DPT_Int = 0,
	DPT_Float,
	DPT_Vector,
	DPT_VectorXY, // Only encodes the XY of a vector, ignores Z
	DPT_String,
	DPT_Array,	// An array of the base types (can't be of datatables).
	DPT_DataTable,
#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
	DPT_Quaternion,
#endif

	DPT_Int64,

	DPT_NUMSendPropTypes

} SendPropType;

class DVariant
{
public:
	DVariant() { m_Type = DPT_Float; }
	DVariant(float val) { m_Type = DPT_Float; m_Float = val; }

	const char *ToString()
	{
		return nullptr;
#if 0
		static char text[128];
		switch (m_Type)
		{
		case DPT_Int:
			snprintf(text, sizeof(text), ("%i"), m_Int);
			break;
		case DPT_Float:
			snprintf(text, sizeof(text), ("%.3f"), m_Float);
			break;
		case DPT_Vector:
			snprintf(text, sizeof(text), ("(%.3f,%.3f,%.3f)"),
				m_Vector[0], m_Vector[1], m_Vector[2]);
			break;
		case DPT_VectorXY:
			snprintf(text, sizeof(text), ("(%.3f,%.3f)"),
				m_Vector[0], m_Vector[1]);
			break;
#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
		case DPT_Quaternion:
			Q_snprintf(text, sizeof(text), ("(%.3f,%.3f,%.3f %.3f)"),
				m_Vector[0], m_Vector[1], m_Vector[2], m_Vector[3]);
			break;
#endif
		case DPT_String:
			if (m_pString)
				return m_pString;
			else
				return  ("NULL");
			break;
		case DPT_Array:
			snprintf(text, sizeof(text), ("Array"));
			break;
		case DPT_DataTable:
			snprintf(text, sizeof(text), ("DataTable"));
			break;
#ifdef SUPPORTS_INT64
		case DPT_Int64:
			snprintf(text, sizeof(text), charenc("%I64d"), m_Int64);
			break;
#endif
		default:
			snprintf(text, sizeof(text), ("type %i unknown"), m_Type);
			break;
		}

		return text;
#endif
	}
	union
	{
		float	m_Float;
		long	m_Int;
		char	*m_pString;
		void	*m_pData;
		float	m_Vector[3];
		long long	m_Int64; //dylan added from evolve
	};
	SendPropType m_Type;
};

class RecvProp;

class CRecvProxyData
{
public:
	const RecvProp	*m_pRecvProp;
	//void * Unknown; //dylan commented when m_Int64 was added from evolve
	DVariant		m_Value;
	int				m_iElement;
	int				m_ObjectID;
};

class CBaseEntity;
typedef CBaseEntity*(*CreateClientClassFn)(int entnum, int serialNum);
typedef CBaseEntity*(*CreateEventFn)();
class CRecvDecoder;

typedef void(*RecvVarProxyFn)(CRecvProxyData *pData, void *pStruct, void *pOut);
typedef void(*DataTableRecvVarProxyFn)(const RecvProp *pProp, void **pOut, void *pData, int objectID);
typedef void(*ArrayLengthRecvProxyFn)(void *pStruct, int objectID, int currentArrayLength);


class RecvTable;

class RecvProp
{
public:
	int GetFlags() const
	{
		return m_Flags;
	}

	const char* GetName() const
	{
		return m_pVarName;
	}

	SendPropType GetType() const
	{
		return m_RecvType;
	}

	RecvTable* GetDataTable() const
	{
		return m_pDataTable;
	}

	RecvVarProxyFn GetProxyFn() const
	{
		return m_ProxyFn;
	}

	DataTableRecvVarProxyFn	GetDataTableProxyFn() const
	{
		return m_DataTableProxyFn;
	}

	void SetProxyFn(RecvVarProxyFn fn)
	{
		m_ProxyFn = fn;
	}

	int GetOffset() const
	{
		return m_Offset;
	}

	void SetOffset(int o)
	{
		m_Offset = o;
	}

public:
	char					*m_pVarName;
	int						m_Flags;
	SendPropType			m_RecvType;
	int						m_StringBufferSize;
	bool					m_bInsideArray;
	const void				*m_pExtraData;
	RecvProp				*m_pArrayProp;
	ArrayLengthRecvProxyFn	m_ArrayLengthProxy;
	RecvVarProxyFn			m_ProxyFn;
	DataTableRecvVarProxyFn	m_DataTableProxyFn;
	RecvTable				*m_pDataTable;
	int						m_Offset;
	int						m_ElementStride;
	int						m_nElements;
	const char				*m_pParentArrayPropName;
};

class RecvTable
{
public:
	int GetNumProps()
	{
		return m_nProps;
	}

	RecvProp* GetRecvProp(int i)
	{
		return &m_pProps[i];
	}

	const char* GetName()
	{
		return m_pNetTableName;
	}

public:
	RecvProp		*m_pProps;
	int				m_nProps;
	CRecvDecoder			*m_pDecoder;
	char			*m_pNetTableName;
	bool			m_bInitialized;
	bool			m_bInMainList;
};

struct ClientClass
{
	void*			m_pCreateFn;
	void*			m_pCreateEventFn;
	char			*m_pNetworkName;
	RecvTable		*m_pRecvTable;
	ClientClass		*m_pNext;
	int				m_ClassID;

	const char* GetName()
	{
		return m_pNetworkName;
	}
};

class CNetVarManager
{
public:
	void Initialize();
	//void GrabOffsets();
	int GetOffset(const char *tableName, const char *propName);
	bool HookProp(const char *tableName, const char *propName, RecvVarProxyFn fun);
	//void DumpNetvars(std::string path);
public:
	int Get_Prop(const char *tableName, const char *propName, RecvProp **prop = 0);
	int Get_Prop(RecvTable *recvTable, const char *propName, RecvProp **prop = 0);
	RecvTable *GetTable(const char *tableName);
	std::vector<RecvTable*> m_tables;
	//void DumpTable(RecvTable *table, int depth);
	std::ofstream m_file;
};

extern CNetVarManager* NetVarManager;