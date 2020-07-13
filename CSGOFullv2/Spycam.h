#pragma once
#include "Includes.h"

class CSpycam
{
protected:
	CBaseEntity* m_pTargetEntity = nullptr;
	Vector m_CamPos = Vector(0.f, 0.f, 0.f);
	Vector m_CamForward = Vector(0.f, 0.f, 0.f);
	QAngle m_TargetAngles = QAngle(0.f, 0.f, 0.f);
	int m_iSelectedEntity = -1;
	std::vector<std::pair<float, CBaseEntity*>> m_Entities;

	void UpdateEntities();
	void CalcView();
	void RenderView(void* ecx, CViewSetup* view);
public:
	IMaterial* m_SpyMaterial;
	ITexture* m_SpyTexture;
	void DoSpyCam(void* ecx, CViewSetup* view);
};

extern CSpycam g_Spycam;
