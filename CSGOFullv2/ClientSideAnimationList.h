#pragma once
class CUtlVectorSimple;
class C_BaseAnimating;
const unsigned int FCLIENTANIM_SEQUENCE_CYCLE = 0x00000001;

struct clientanimating_t
{
	C_BaseAnimating *pAnimating;
	unsigned int	flags;
	clientanimating_t(C_BaseAnimating *_pAnim, unsigned int _flags) : pAnimating(_pAnim), flags(_flags) {}
};

extern CUtlVectorSimple *g_ClientSideAnimationList;