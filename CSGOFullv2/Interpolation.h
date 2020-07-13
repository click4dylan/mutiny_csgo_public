#pragma once

class IInterpolatedVar
{
public:
	virtual		 ~IInterpolatedVar() {}

	virtual void Setup(void *pValue, int type) = 0;
	virtual void SetInterpolationAmount(float seconds) = 0;

	// Returns true if the new value is different from the prior most recent value.
	virtual void NoteLastNetworkedValue() = 0;
	virtual bool NoteChanged(float flCurrentTime, float flChangeTime, bool bUpdateLastNetworkedValue) = 0;
	virtual void Reset(float flCurrentTime) = 0;

	// Returns 1 if the value will always be the same if currentTime is always increasing.
	virtual int Interpolate(float currentTime) = 0;

	virtual int	 GetType() const = 0;
	virtual void RestoreToLastNetworked() = 0;
	virtual void Copy(IInterpolatedVar *pSrc) = 0;

	virtual const char *GetDebugName() = 0;
	virtual void SetDebugName(const char* pName) = 0;
};

class VarMapEntry_t
{


public:
	unsigned short  type;
	unsigned short  m_bNeedsToInterpolate; // Set to false when this var doesn't
										   // need Interpolate() called on it anymore.
	void    *data;
	IInterpolatedVar *watcher;
};

#pragma once

struct VarMapping_t
{
	VarMapping_t()
	{
		m_nInterpolatedEntries = 0;
	}

	CUtlVector< VarMapEntry_t > m_Entries;
	//char m_Entries[20];
	int       m_nInterpolatedEntries;
	float      m_lastInterpolationTime;
};