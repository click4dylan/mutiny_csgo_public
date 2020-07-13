#if 0

#include "precompiled.h"
#include "Spycam.h"
#include "LocalPlayer.h"

CSpycam g_Spycam;

void CSpycam::UpdateEntities()
{
	m_Entities.clear();

	// loop through all entities
	for (int i = 1; i < Interfaces::EngineClient->GetMaxClients(); i++)
	{
		// skip localplayer
		if (i == LocalPlayer.Entity->index)
			continue;

		// get current entity
		CBaseEntity* Entity = Interfaces::ClientEntList->GetBaseEntity(i);

		// skip invalid entity
		if (!Entity || !Entity->GetClientClass() || !Entity->IsPlayer())
			continue;

		// skip dead or dormant players
		if (!Entity->GetAlive() || Entity->GetDormant())
			continue;

		// skip friendly players
		if (!Entity->IsEnemy(LocalPlayer.Entity))
			continue;

		// get fov
		float _fov = GetFov(LocalPlayer.ShootPosition, Entity->GetBonePosition(HITBOX_HEAD), LocalPlayer.CurrentEyeAngles);

		// add to vector
		m_Entities.emplace_back(std::make_pair(_fov, Entity));
	}

	// sort vector
	std::sort(m_Entities.begin(), m_Entities.end());
}

void CSpycam::CalcView()
{
	// helpers
	Vector EyePos;

	// we do have possible targets
	if (!m_Entities.empty())
	{
		// set first entity as target
		m_pTargetEntity = m_Entities.front().second;


		EyePos = LocalPlayer.Current_Origin - (m_pTargetEntity->GetLocalOriginDirect() + Vector(0, 0, 64));
		VectorAngles(EyePos, m_TargetAngles);
	}
	// we don't have targets
	else
	{
		// set localplayer as target
		m_pTargetEntity = LocalPlayer.Entity;

		// turn 180 for mirrorcam
		m_TargetAngles = LocalPlayer.CurrentEyeAngles;
		m_TargetAngles.y += 180;
	}

	// not real eyepos but good enough for here
	Vector eyePos = m_pTargetEntity->GetLocalOriginDirect() + Vector(0, 0, 64);

	// set camera above player
	m_CamPos = Vector(m_TargetAngles.x, m_TargetAngles.y, 75);

	// get forward
	AngleVectors(QAngle(m_CamPos.x, m_CamPos.y, 0), &m_CamForward);

	// init helpers
	static Vector CAM_HULL_MIN(-CAM_HULL_OFFSET, -CAM_HULL_OFFSET, -CAM_HULL_OFFSET);
	static Vector CAM_HULL_MAX(CAM_HULL_OFFSET, CAM_HULL_OFFSET, CAM_HULL_OFFSET);

	// init trace to see if we hit a wall
	CGameTrace trace;
	CTraceFilterWorldOnly traceFilter;
	Ray_t ray;
	ray.Init(eyePos, eyePos - (m_CamForward * m_CamPos.z), CAM_HULL_MIN, CAM_HULL_MAX);

	// run trace
	Interfaces::EngineTrace->TraceRay(ray, MASK_SOLID, &traceFilter, &trace);

	// move the camera closer if it hit something
	if (trace.fraction < 1.0)
		m_CamPos.z *= trace.fraction;
}

void CSpycam::RenderView(void* ecx, CViewSetup* view)
{
	// copy original view
	CViewSetup spyView;
	memcpy(static_cast<void*>(&spyView), static_cast<void*>(view), sizeof CViewSetup);

	// setup new view
	spyView.x = spyView.x_old = 0;
	spyView.y = spyView.y_old = 0;
	spyView.width = spyView.width_old = g_Menu.Items.cam_spy->GetParentWindow()->m_Pos.width - g_Menu.Items.cam_spy->GetParentWindow()->m_RenderOffset.x;
	spyView.height = spyView.height_old = g_Menu.Items.cam_spy->GetParentWindow()->m_Pos.height - g_Menu.Items.cam_spy->GetParentWindow()->m_RenderOffset.y;
	spyView.origin = m_pTargetEntity->GetLocalOriginDirect() + Vector(0, 0, 64) - (m_CamForward * m_CamPos.z);
	spyView.fov = 90;
	spyView.angles = m_TargetAngles;
	spyView.m_flAspectRatio = float(spyView.width) / float(spyView.height);

	// get render context
	auto renderCtx = Interfaces::MatSystem->GetRenderContext();

	// render new view
	renderCtx->PushRenderTargetAndViewport();
	renderCtx->SetRenderTarget(m_SpyTexture);

	GetVFunc<void(__thiscall*)(void*, CViewSetup&, ClearFlags_t, RenderViewInfo_t)>(Interfaces::Client, 28)(ecx, spyView, ClearFlags_t(VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH | VIEW_CLEAR_STENCIL), RENDERVIEW_SUPPRESSMONITORRENDERING);

	renderCtx->PopRenderTargetAndViewport();
	renderCtx->Release();
}

void CSpycam::DoSpyCam(void* ecx, CViewSetup* view)
{
	// don't run if we're turned off
	if (!g_Convars.Cam.cam_spy_enable->GetBool())
		return;

	// update vector of possible targets
	UpdateEntities();

	// calculate the spycam view
	CalcView();

	// render the actual view
	RenderView(ecx, view);
}
#endif