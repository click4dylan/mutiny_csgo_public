#pragma once
#include "GameMemory.h"
#include "CSGO_HX.h"

typedef struct GlowObjectDefinition_t
{
	void*              m_pEntity;				//0000
	float              m_flGlowRed;				//0004
	float              m_flGlowGreen;			//0008
	float              m_flGlowBlue;			//000C
	float              m_flGlowAlpha;			//0010
	unsigned char      unk0[0x10];				//0014
	bool               m_bRenderWhenOccluded;	//0024
	bool               m_bRenderWhenUnoccluded;	//0025
	unsigned char      unk2[0x12];				//0026
} GlowObjectDefinition;							//0038

typedef struct GlowObjectManager_t
{
	GlowObjectDefinition* DataPtr;	    //0000
	unsigned int Max;                   //0004
	unsigned int unk02;                 //0008
	unsigned int Count;                 //000C
	unsigned int DataPtrBack;			//0010
	int m_nFirstFreeSlot;				//0014
	unsigned int unk1;					//0018
	unsigned int unk2;					//001C
	unsigned int unk3;					//0020
	unsigned int unk4;					//0024
	unsigned int unk5;					//0028
} GlowObjectManager;