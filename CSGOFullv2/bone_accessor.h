//========= Copyright   1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#pragma once


#include "misc.h"


class C_BaseAnimating;


class CBoneAccessor
{
public:

	CBoneAccessor();
	CBoneAccessor(matrix3x4a_t *pBones); // This can be used to allow access to all bones.

										 // Initialize.
	void Init(const C_BaseAnimating *pAnimating, matrix3x4a_t *pBones);

	int GetReadableBones();
	void SetReadableBones(int flags);

	int GetWritableBones();
	void SetWritableBones(int flags);

	// Get bones for read or write access.
	const matrix3x4a_t&	GetBone(int iBone) const;
	const matrix3x4a_t&	operator[](int iBone) const;
	matrix3x4a_t&		GetBoneForWrite(int iBone);

	matrix3x4a_t			*GetBoneArrayForWrite() const;
	void SetBoneArrayForWrite(matrix3x4a_t* p_bones);

	//private:

#if defined( CLIENT_DLL ) && defined( _DEBUG )
	void SanityCheckBone(int iBone, bool bReadable) const;
#endif

	// Only used in the client DLL for debug verification.
	const C_BaseAnimating *m_pAnimating; //2694

	matrix3x4a_t *m_pBones; //0x2698

	int m_ReadableBones;	//0x269C	// Which bones can be read.
	int m_WritableBones;	//0x26A0	// Which bones can be written.
};


inline CBoneAccessor::CBoneAccessor()
{
	m_pAnimating = NULL;
	m_pBones = NULL;
	m_ReadableBones = m_WritableBones = 0;
}

inline CBoneAccessor::CBoneAccessor(matrix3x4a_t *pBones)
{
	m_pAnimating = NULL;
	m_pBones = pBones;
}

inline void CBoneAccessor::Init(const C_BaseAnimating *pAnimating, matrix3x4a_t *pBones)
{
	m_pAnimating = pAnimating;
	m_pBones = pBones;
}

inline int CBoneAccessor::GetReadableBones()
{
	return m_ReadableBones;
}

inline void CBoneAccessor::SetReadableBones(int flags)
{
	m_ReadableBones = flags;
}

inline int CBoneAccessor::GetWritableBones()
{
	return m_WritableBones;
}

inline void CBoneAccessor::SetWritableBones(int flags)
{
	m_WritableBones = flags;
}

inline const matrix3x4a_t& CBoneAccessor::GetBone(int iBone) const
{
#if defined( CLIENT_DLL ) && defined( _DEBUG )
	SanityCheckBone(iBone, true);
#endif
	return m_pBones[iBone];
}

inline const matrix3x4a_t& CBoneAccessor::operator[](int iBone) const
{
#if defined( CLIENT_DLL ) && defined( _DEBUG )
	SanityCheckBone(iBone, true);
#endif
	return m_pBones[iBone];
}

inline matrix3x4a_t& CBoneAccessor::GetBoneForWrite(int iBone)
{
#if defined( CLIENT_DLL ) && defined( _DEBUG )
	SanityCheckBone(iBone, false);
#endif
	return m_pBones[iBone];
}

inline matrix3x4a_t *CBoneAccessor::GetBoneArrayForWrite(void) const
{
	return m_pBones;
}

inline void CBoneAccessor::SetBoneArrayForWrite(matrix3x4a_t* p_bones)
{
	m_pBones = p_bones;
}
