#pragma once
#include "bitvec.h"
#include "dt_common.h"

#define ABSOLUTE_PLAYER_LIMIT 255

class SendProp;

// ------------------------------------------------------------------------ //
// Send proxies can be used to convert a variable into a networkable type 
// (a good example is converting an edict pointer into an integer index).

// These allow you to translate data. For example, if you had a user-entered 
// string number like "10" (call the variable pUserStr) and wanted to encode 
// it as an integer, you would use a SendPropInt32 and write a proxy that said:
// pOut->m_Int = atoi(pUserStr);

// pProp       : the SendProp that has the proxy
// pStructBase : the base structure (like CBaseEntity*).
// pData       : the address of the variable to proxy.
// pOut        : where to output the proxied value.
// iElement    : the element index if this data is part of an array (or 0 if not).
// objectID    : entity index for debugging purposes.

// Return false if you don't want the engine to register and send a delta to
// the clients for this property (regardless of whether it actually changed or not).
// ------------------------------------------------------------------------ //
typedef void(*SendVarProxyFn)(const SendProp *pProp, const void *pStructBase, const void *pData, DVariant *pOut, int iElement, int objectID);

// Return the pointer to the data for the datatable.
// If the proxy returns null, it's the same as if pRecipients->ClearAllRecipients() was called.
class CSendProxyRecipients;

typedef void* (*SendTableProxyFn)(
	const SendProp *pProp,
	const void *pStructBase,
	const void *pData,
	CSendProxyRecipients *pRecipients,
	int objectID);


class CNonModifiedPointerProxy
{
public:
	CNonModifiedPointerProxy(SendTableProxyFn fn);

public:

	SendTableProxyFn m_Fn;
	CNonModifiedPointerProxy *m_pNext;
};


// This tells the engine that the send proxy will not modify the pointer
// - it only plays with the recipients. This must be set on proxies that work
// this way, otherwise the engine can't track which properties changed
// in NetworkStateChanged().
#define REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( sendProxyFn ) static CNonModifiedPointerProxy __proxy_##sendProxyFn( sendProxyFn );


class CStandardSendProxiesV1
{
public:
	CStandardSendProxiesV1();

	SendVarProxyFn m_Int8ToInt32;
	SendVarProxyFn m_Int16ToInt32;
	SendVarProxyFn m_Int32ToInt32;

	SendVarProxyFn m_UInt8ToInt32;
	SendVarProxyFn m_UInt16ToInt32;
	SendVarProxyFn m_UInt32ToInt32;

	SendVarProxyFn m_FloatToFloat;
	SendVarProxyFn m_VectorToVector;

#ifdef SUPPORTS_INT64
	SendVarProxyFn m_Int64ToInt64;
	SendVarProxyFn m_UInt64ToInt64;
#endif
};

class CStandardSendProxies : public CStandardSendProxiesV1
{
public:
	CStandardSendProxies();

	SendTableProxyFn m_DataTableToDataTable;
	SendTableProxyFn m_SendLocalDataTable;
	CNonModifiedPointerProxy **m_ppNonModifiedPointerProxies;
};

extern CStandardSendProxies g_StandardSendProxies;


// Max # of datatable send proxies you can have in a tree.
#define MAX_DATATABLE_PROXIES	32

// ------------------------------------------------------------------------ //
// Datatable send proxies are used to tell the engine where the datatable's 
// data is and to specify which clients should get the data. 
//
// pRecipients is the object that allows you to specify which clients will
// receive the data.
// ------------------------------------------------------------------------ //
class CSendProxyRecipients
{
public:
	void	SetAllRecipients();					// Note: recipients are all set by default when each proxy is called.
	void	ClearAllRecipients();

	void	SetRecipient(int iClient);		// Note: these are CLIENT indices, not entity indices (so the first player's index is 0).
	void	ClearRecipient(int iClient);

	// Clear all recipients and set only the specified one.
	void	SetOnly(int iClient);

public:
	// Make sure we have enough room for the max possible player count
	CBitVec< ABSOLUTE_PLAYER_LIMIT >	m_Bits;
};

inline void CSendProxyRecipients::SetAllRecipients()
{
	m_Bits.SetAll();
}

inline void CSendProxyRecipients::ClearAllRecipients()
{
	m_Bits.ClearAll();
}

inline void CSendProxyRecipients::SetRecipient(int iClient)
{
	m_Bits.Set(iClient);
}

inline void	CSendProxyRecipients::ClearRecipient(int iClient)
{
	m_Bits.Clear(iClient);
}

inline void CSendProxyRecipients::SetOnly(int iClient)
{
	m_Bits.ClearAll();
	m_Bits.Set(iClient);
}



// ------------------------------------------------------------------------ //
// ArrayLengthSendProxies are used when you want to specify an array's length
// dynamically.
// ------------------------------------------------------------------------ //
typedef int(*ArrayLengthSendProxyFn)(const void *pStruct, int objectID);



class RecvProp;
class SendTable;
class CSendTablePrecalc;


// -------------------------------------------------------------------------------------------------------------- //
// SendProp.
// -------------------------------------------------------------------------------------------------------------- //

// If SendProp::GetDataTableProxyIndex() returns this, then the proxy is one that always sends
// the data to all clients, so we don't need to store the results.
#define DATATABLE_PROXY_INDEX_NOPROXY	255
#define DATATABLE_PROXY_INDEX_INVALID	254

class SendProp
{
public:
	SendProp();
	virtual				~SendProp();

	void				Clear();

	int					GetOffset() const;
	void				SetOffset(int i);

	SendVarProxyFn		GetProxyFn() const;
	void				SetProxyFn(SendVarProxyFn f);

	SendTableProxyFn	GetDataTableProxyFn() const;
	void				SetDataTableProxyFn(SendTableProxyFn f);

	SendTable*			GetDataTable() const;
	void				SetDataTable(SendTable *pTable);

	char const*			GetExcludeDTName() const;

	// If it's one of the numbered "000", "001", etc properties in an array, then
	// these can be used to get its array property name for debugging.
	const char*			GetParentArrayPropName() const;
	void				SetParentArrayPropName(char *pArrayPropName);

	const char*			GetName() const;

	bool				IsSigned() const;

	bool				IsExcludeProp() const;

	bool				IsInsideArray() const;	// Returns true if SPROP_INSIDEARRAY is set.
	void				SetInsideArray();

	// Arrays only.
	void				SetArrayProp(SendProp *pProp);
	SendProp*			GetArrayProp() const;

	// Arrays only.
	void					SetArrayLengthProxy(ArrayLengthSendProxyFn fn);
	ArrayLengthSendProxyFn	GetArrayLengthProxy() const;

	int					GetNumElements() const;
	void				SetNumElements(int nElements);

	// Return the # of bits to encode an array length (must hold GetNumElements()).
	int					GetNumArrayLengthBits() const;

	int					GetElementStride() const;

	SendPropType		GetType() const;

	int					GetFlags() const;
	void				SetFlags(int flags);

	// Some property types bind more data to the SendProp in here.
	const void*			GetExtraData() const;
	void				SetExtraData(const void *pData);

public:

	RecvProp		*m_pMatchingRecvProp;	// This is temporary and only used while precalculating
												// data for the decoders.

	SendPropType	m_Type;
	int				m_nBits;
	float			m_fLowValue;
	float			m_fHighValue;

	SendProp		*m_pArrayProp;					// If this is an array, this is the property that defines each array element.
	ArrayLengthSendProxyFn	m_ArrayLengthProxy;	// This callback returns the array length.

	int				m_nElements;		// Number of elements in the array (or 1 if it's not an array).
	int				m_ElementStride;	// Pointer distance between array elements.

	const char *m_pExcludeDTName;			// If this is an exclude prop, then this is the name of the datatable to exclude a prop from.
	const char *m_pParentArrayPropName;

	const char		*m_pVarName;
	float			m_fHighLowMul;

private:

	int					m_Flags;				// SPROP_ flags.

	SendVarProxyFn		m_ProxyFn;				// NULL for DPT_DataTable.
	SendTableProxyFn	m_DataTableProxyFn;		// Valid for DPT_DataTable.

	SendTable			*m_pDataTable;

	// SENDPROP_VECTORELEM makes this negative to start with so we can detect that and
	// set the SPROP_IS_VECTOR_ELEM flag.
	int					m_Offset;

	// Extra data bound to this property.
	const void			*m_pExtraData;
};


inline int SendProp::GetOffset() const
{
	return m_Offset;
}

inline void SendProp::SetOffset(int i)
{
	m_Offset = i;
}

inline SendVarProxyFn SendProp::GetProxyFn() const
{
	return m_ProxyFn;
}

inline void SendProp::SetProxyFn(SendVarProxyFn f)
{
	m_ProxyFn = f;
}

inline SendTableProxyFn SendProp::GetDataTableProxyFn() const
{
	return m_DataTableProxyFn;
}

inline void SendProp::SetDataTableProxyFn(SendTableProxyFn f)
{
	m_DataTableProxyFn = f;
}

inline SendTable* SendProp::GetDataTable() const
{
	return m_pDataTable;
}

inline void SendProp::SetDataTable(SendTable *pTable)
{
	m_pDataTable = pTable;
}

inline char const* SendProp::GetExcludeDTName() const
{
	return m_pExcludeDTName;
}

inline const char* SendProp::GetParentArrayPropName() const
{
	return m_pParentArrayPropName;
}

inline void	SendProp::SetParentArrayPropName(char *pArrayPropName)
{
	m_pParentArrayPropName = pArrayPropName;
}

inline const char* SendProp::GetName() const
{
	return m_pVarName;
}


inline bool SendProp::IsSigned() const
{
	return !(m_Flags & SPROP_UNSIGNED);
}

inline bool SendProp::IsExcludeProp() const
{
	return (m_Flags & SPROP_EXCLUDE) != 0;
}

inline bool	SendProp::IsInsideArray() const
{
	return (m_Flags & SPROP_INSIDEARRAY) != 0;
}

inline void SendProp::SetInsideArray()
{
	m_Flags |= SPROP_INSIDEARRAY;
}

inline void SendProp::SetArrayProp(SendProp *pProp)
{
	m_pArrayProp = pProp;
}

inline SendProp* SendProp::GetArrayProp() const
{
	return m_pArrayProp;
}

inline void SendProp::SetArrayLengthProxy(ArrayLengthSendProxyFn fn)
{
	m_ArrayLengthProxy = fn;
}

inline ArrayLengthSendProxyFn SendProp::GetArrayLengthProxy() const
{
	return m_ArrayLengthProxy;
}

inline int SendProp::GetNumElements() const
{
	return m_nElements;
}

inline void SendProp::SetNumElements(int nElements)
{
	m_nElements = nElements;
}

inline int SendProp::GetElementStride() const
{
	return m_ElementStride;
}

inline SendPropType SendProp::GetType() const
{
	return m_Type;
}

inline int SendProp::GetFlags() const
{
	return m_Flags;
}

inline void SendProp::SetFlags(int flags)
{
	// Make sure they're using something from the valid set of flags.
	m_Flags = flags;
}

inline const void* SendProp::GetExtraData() const
{
	return m_pExtraData;
}

inline void SendProp::SetExtraData(const void *pData)
{
	m_pExtraData = pData;
}


// -------------------------------------------------------------------------------------------------------------- //
// SendTable.
// -------------------------------------------------------------------------------------------------------------- //

class SendTable
{
public:

	typedef SendProp PropType;

	SendTable();
	SendTable(SendProp *pProps, int nProps, const char *pNetTableName);
	~SendTable();

	void		Construct(SendProp *pProps, int nProps, const char *pNetTableName);

	const char*	GetName() const;

	int			GetNumProps() const;
	SendProp*	GetProp(int i);

	// Used by the engine.
	bool		IsInitialized() const;
	void		SetInitialized(bool bInitialized);

	// Used by the engine while writing info into the signon.
	void		SetWriteFlag(bool bHasBeenWritten);
	bool		GetWriteFlag() const;

	bool		HasPropsEncodedAgainstTickCount() const;
	void		SetHasPropsEncodedAgainstTickcount(bool bState);

public:

	SendProp	*m_pProps;
	int			m_nProps;

	const char	*m_pNetTableName;	// The name matched between client and server.

	// The engine hooks the SendTable here.
	CSendTablePrecalc	*m_pPrecalc;


protected:
	bool		m_bInitialized : 1;
	bool		m_bHasBeenWritten : 1;
	bool		m_bHasPropsEncodedAgainstCurrentTickCount : 1; // m_flSimulationTime and m_flAnimTime, e.g.
};


inline const char* SendTable::GetName() const
{
	return m_pNetTableName;
}


inline int SendTable::GetNumProps() const
{
	return m_nProps;
}


inline SendProp* SendTable::GetProp(int i)
{
	return &m_pProps[i];
}


inline bool SendTable::IsInitialized() const
{
	return m_bInitialized;
}


inline void SendTable::SetInitialized(bool bInitialized)
{
	m_bInitialized = bInitialized;
}


inline bool SendTable::GetWriteFlag() const
{
	return m_bHasBeenWritten;
}


inline void SendTable::SetWriteFlag(bool bHasBeenWritten)
{
	m_bHasBeenWritten = bHasBeenWritten;
}

inline bool SendTable::HasPropsEncodedAgainstTickCount() const
{
	return m_bHasPropsEncodedAgainstCurrentTickCount;
}

inline void SendTable::SetHasPropsEncodedAgainstTickcount(bool bState)
{
	m_bHasPropsEncodedAgainstCurrentTickCount = bState;
}