#include "precompiled.h"
#include "CBaseHandle.h"
#include "Interfaces.h"
// -------------------------------------------------------------------------------------------------- //
// Game-code CBaseHandle implementation.
// -------------------------------------------------------------------------------------------------- //
IHandleEntity* CBaseHandle::Get() const
{
	return Interfaces::ClientEntList->LookupEntity(*this);
}

bool CBaseHandle::operator <(const IHandleEntity *pEntity) const
{
	unsigned long otherIndex = (pEntity) ? ((CBaseEntity*)pEntity)->GetClientUnknown()->GetRefEHandle().m_Index : INVALID_EHANDLE_INDEX;
	return m_Index < otherIndex;
}

const CBaseHandle& CBaseHandle::Set(const IHandleEntity *pEntity)
{
	if (pEntity)
	{
		*this = ((CBaseEntity*)pEntity)->GetRefEHandle();
	}
	else
	{
		m_Index = INVALID_EHANDLE_INDEX;
	}

	return *this;
}