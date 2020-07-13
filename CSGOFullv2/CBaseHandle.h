#pragma once
#include "misc.h"
//#include "BaseEntity.h"

class CBaseEntity;
class IHandleEntity;

// How many bits to use to encode an edict.
#define	MAX_EDICT_BITS				11			// # of bits needed to represent max edicts
// Max # of edicts in a level
#define	MAX_EDICTS					(1<<MAX_EDICT_BITS)

// How many bits to use to encode an server class index
#define MAX_SERVER_CLASS_BITS		9
// Max # of networkable server classes
#define MAX_SERVER_CLASSES			(1<<MAX_SERVER_CLASS_BITS)

#define SIGNED_GUID_LEN 32 // Hashed CD Key (32 hex alphabetic chars + 0 terminator )

// Used for networking ehandles.
#define NUM_ENT_ENTRY_BITS		(MAX_EDICT_BITS + 5) //Swarm/old sdk is + 2, csgo is 5
#define NUM_ENT_ENTRIES			(1 << NUM_ENT_ENTRY_BITS)
#define NUM_SERIAL_NUM_BITS		(32 - NUM_ENT_ENTRY_BITS)
#define NUM_SERIAL_NUM_SHIFT_BITS (32 - NUM_SERIAL_NUM_BITS)
#define ENT_ENTRY_MASK			(( 1 << NUM_SERIAL_NUM_BITS) - 1)
#define INVALID_EHANDLE_INDEX	0xFFFFFFFF

// -------------------------------------------------------------------------------------------------- //
// CBaseHandle.
// -------------------------------------------------------------------------------------------------- //

class CBaseHandle
{
	friend class CBaseEntityList;

public:

	CBaseHandle();
	CBaseHandle(const CBaseHandle &other);
	CBaseHandle(unsigned long value);
	CBaseHandle(int iEntry, int iSerialNumber);

	void Init(int iEntry, int iSerialNumber);
	void Term();

	// Even if this returns true, Get() still can return return a non-null value.
	// This just tells if the handle has been initted with any values.
	bool IsValid() const;

	int GetEntryIndex() const;
	int GetSerialNumber() const;

	unsigned long ToUnsignedLong() const;
	bool operator !=(const CBaseHandle &other) const;
	bool operator ==(const CBaseHandle &other) const;
	bool operator ==(const IHandleEntity* pEnt) const;
	bool operator !=(const IHandleEntity* pEnt) const;
	bool operator <(const CBaseHandle &other) const;
	bool operator <(const IHandleEntity* pEnt) const;

	// Assign a value to the handle.
	const CBaseHandle& operator=(const IHandleEntity *pEntity);
	const CBaseHandle& Set(const IHandleEntity *pEntity);

	// Use this to dereference the handle.
	// Note: this is implemented in game code (ehandle.h)
	IHandleEntity* Get() const;


//protected:
	// The low NUM_SERIAL_BITS hold the index. If this value is less than MAX_EDICTS, then the entity is networkable.
	// The high NUM_SERIAL_NUM_BITS bits are the serial number.
	unsigned long	m_Index;
};


inline CBaseHandle::CBaseHandle()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline CBaseHandle::CBaseHandle(const CBaseHandle &other)
{
	m_Index = other.m_Index;
}

inline CBaseHandle::CBaseHandle(unsigned long value)
{
	m_Index = value;
}

inline CBaseHandle::CBaseHandle(int iEntry, int iSerialNumber)
{
	Init(iEntry, iSerialNumber);
}

inline void CBaseHandle::Init(int iEntry, int iSerialNumber)
{
	//Assert(iEntry >= 0 && (iEntry & ENT_ENTRY_MASK) == iEntry);
	//Assert(iSerialNumber >= 0 && iSerialNumber < (1 << NUM_SERIAL_NUM_BITS));

	m_Index = iEntry | (iSerialNumber << NUM_SERIAL_NUM_SHIFT_BITS);
}

inline void CBaseHandle::Term()
{
	m_Index = INVALID_EHANDLE_INDEX;
}

inline bool CBaseHandle::IsValid() const
{
	return m_Index != INVALID_EHANDLE_INDEX;
}

inline int CBaseHandle::GetEntryIndex() const
{
	// There is a hack here: due to a bug in the original implementation of the 
	// entity handle system, an attempt to look up an invalid entity index in 
	// certain cirumstances might fall through to the the mask operation below.
	// This would mask an invalid index to be in fact a lookup of entity number
	// NUM_ENT_ENTRIES, so invalid ent indexes end up actually looking up the
	// last slot in the entities array. Since this slot is always empty, the 
	// lookup returns NULL and the expected behavior occurs through this unexpected
	// route.
	// A lot of code actually depends on this behavior, and the bug was only exposed
	// after a change to NUM_SERIAL_NUM_BITS increased the number of allowable
	// static props in the world. So the if-stanza below detects this case and 
	// retains the prior (bug-submarining) behavior.
	if (!IsValid())
		return NUM_ENT_ENTRIES - 1;
	return m_Index & ENT_ENTRY_MASK;
}

inline int CBaseHandle::GetSerialNumber() const
{
	return m_Index >> NUM_ENT_ENTRY_BITS;
}

inline unsigned long CBaseHandle::ToUnsignedLong() const
{
	return m_Index;
}

inline bool CBaseHandle::operator !=(const CBaseHandle &other) const
{
	return m_Index != other.m_Index;
}

inline bool CBaseHandle::operator ==(const CBaseHandle &other) const
{
	return m_Index == other.m_Index;
}

inline bool CBaseHandle::operator ==(const IHandleEntity* pEnt) const
{
	return Get() == pEnt;
}

inline bool CBaseHandle::operator !=(const IHandleEntity* pEnt) const
{
	return Get() != pEnt;
}

inline bool CBaseHandle::operator <(const CBaseHandle &other) const
{
	return m_Index < other.m_Index;
}

inline const CBaseHandle& CBaseHandle::operator=(const IHandleEntity *pEntity)
{
	return Set(pEntity);
}


// -------------------------------------------------------------------------------------------------- //
// CHandle2.
// -------------------------------------------------------------------------------------------------- //
template< class T >
class CHandle2 : public CBaseHandle
{
public:

	CHandle2();
	CHandle2(int iEntry, int iSerialNumber);
	CHandle2(const CBaseHandle &handle);
	CHandle2(T *pVal);

	// The index should have come from a call to ToInt(). If it hasn't, you're in trouble.
	static CHandle2<T> FromIndex(int index);

	T*		Get() const;
	void	Set(const T* pVal);

	operator T*();
	operator T*() const;

	bool	operator !() const;
	bool	operator==(T *val) const;
	bool	operator!=(T *val) const;
	const CBaseHandle& operator=(const T *val);

	T*		operator->() const;
};

// ----------------------------------------------------------------------- //
// Inlines.
// ----------------------------------------------------------------------- //

template<class T>
CHandle2<T>::CHandle2()
{
}


template<class T>
CHandle2<T>::CHandle2(int iEntry, int iSerialNumber)
{
	Init(iEntry, iSerialNumber);
}


template<class T>
CHandle2<T>::CHandle2(const CBaseHandle &handle)
	: CBaseHandle(handle)
{
}


template<class T>
CHandle2<T>::CHandle2(T *pObj)
{
	Term();
	Set(pObj);
}


template<class T>
inline CHandle2<T> CHandle2<T>::FromIndex(int index)
{
	CHandle2<T> ret;
	ret.m_Index = index;
	return ret;
}


template<class T>
inline T* CHandle2<T>::Get() const
{
	return (T*)CBaseHandle::Get();
}


template<class T>
inline CHandle2<T>::operator T *()
{
	return Get();
}

template<class T>
inline CHandle2<T>::operator T *() const
{
	return Get();
}


template<class T>
inline bool CHandle2<T>::operator !() const
{
	return !Get();
}

template<class T>
inline bool CHandle2<T>::operator==(T *val) const
{
	return Get() == val;
}

template<class T>
inline bool CHandle2<T>::operator!=(T *val) const
{
	return Get() != val;
}

template<class T>
void CHandle2<T>::Set(const T* pVal)
{
	CBaseHandle::Set(reinterpret_cast<const IHandleEntity*>(pVal));
}

template<class T>
inline const CBaseHandle& CHandle2<T>::operator=(const T *val)
{
	Set(val);
	return *this;
}

template<class T>
T* CHandle2<T>::operator -> () const
{
	return Get();
}

typedef CHandle2<CBaseEntity> EHANDLE;