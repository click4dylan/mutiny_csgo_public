#include "precompiled.h"
#include "misc.h"
#include "WaypointSystem.h"
#include "Interfaces.h"
#include "LocalPlayer.h"
#include "Adriel/variable.hpp"
#include "Adriel/input.hpp"
#include "Adriel/ui.hpp"
#include "Adriel/config.hpp"
#include "buildserver_chars.h"
#include "UsedConvars.h"
#include "WeaponController.h"

#ifdef NDEBUG
#include "C:/Developer/Sync/libcurl/x86/release/include/curl.h"
#pragma comment(lib,"C:/Developer/Sync/libcurl/x86/release/lib/libcurl_a.lib")
#else
#include "C:/Developer/Sync/libcurl/x86/debug/include/curl.h"

#pragma comment(lib,"C:/Developer/Sync/libcurl/x86/debug/lib/libcurl_a_debug.lib")
#endif

std::vector<Waypoint*> m_Waypoints;
std::recursive_mutex m_WaypointsMutex;
Waypoint* m_CurrentWaypoint;
std::deque<Waypoint*> m_LastWaypoints;
float m_NextMoveTime = 0.0f;
float m_TotalTimeWeCouldntTraceWaypoint = 0.0f;
float m_TotalTimeWeCouldntMove = 0.0f;
#define WAYPOINT_FILE_VERSION 1
#define MAX_WAYPOINTS 100000
#define TRIANGLE_SIDE Vector(4, 4, 0)
#define TRIANGLE_TOP Vector(0, 0, 12)
#define PLAYER_RENDER_DIST_TO_TEXT 512.0f
const Vector waypoint_trace_mins = { 0, 0, 0 };
const Vector waypoint_trace_maxs = { 0, 0, 12.f };
const float waypoint_trace_diameter = 6.0f;

void ResetGlobalWalkBotVariables()
{
	m_LastWaypoints.clear();
	m_CurrentWaypoint = nullptr;
	m_NextMoveTime = 0.0f;
	m_TotalTimeWeCouldntTraceWaypoint = 0.0f;
	m_TotalTimeWeCouldntMove = 0.0f;
}

void ResetGlobalWaypointSystemVariables()
{
	auto& var = variable::get();
	var.waypoints.ResetSelectedInformation();
	ResetGlobalWalkBotVariables();
}

void StoreCurrentWaypoint()
{
	if (m_CurrentWaypoint)
	{
		m_LastWaypoints.push_front(m_CurrentWaypoint);
		if (m_LastWaypoints.size() > 10)
			m_LastWaypoints.pop_back();
	}
}

void DrawOneWay(Waypoint* waypoint)
{
	if (waypoint->m_iFlags & WAYPOINT_FLAG_ONEWAY)
	{
		Interfaces::DebugOverlay->AddBoxOverlay(waypoint->m_vecOrigin, waypoint->m_vecMins, waypoint->m_vecMaxs, waypoint->m_vecAngles, 0, 255, 0, 50, TICKS_TO_TIME(2));
		Interfaces::DebugOverlay->AddSphereOverlay(waypoint->m_vecOrigin, 5.0f, 5, 5, 0, 255, 0, 75, TICKS_TO_TIME(2));
		if (waypoint->m_vecOrigin.Dist(LocalPlayer.Current_Origin) < PLAYER_RENDER_DIST_TO_TEXT)
		{
			//decrypts(0)
			Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("ONE-WAY"));
			//encrypts(0)
		}
	}
	else if (waypoint->m_iFlags & WAYPOINT_FLAG_ONEWAY_VICTIM)
	{
		Interfaces::DebugOverlay->AddBoxOverlay(waypoint->m_vecOrigin, waypoint->m_vecMins, waypoint->m_vecMaxs, waypoint->m_vecAngles, 255, 0, 0, 50, TICKS_TO_TIME(2));
		Interfaces::DebugOverlay->AddSphereOverlay(waypoint->m_vecOrigin, 5.0f, 5, 5, 255, 0, 0, 75, TICKS_TO_TIME(2));
		if (waypoint->m_vecOrigin.Dist(LocalPlayer.Current_Origin) < PLAYER_RENDER_DIST_TO_TEXT)
		{
			//decrypts(0)
			Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("ONE-WAY DISADVANTAGE"));
			//encrypts(0)
		}
	}
}

void DrawFakeDuckSpot(Waypoint* waypoint)
{
	Interfaces::DebugOverlay->AddBoxOverlay(waypoint->m_vecOrigin, waypoint->m_vecMins, waypoint->m_vecMaxs, waypoint->m_vecAngles, 35, 101, 62, 30, TICKS_TO_TIME(2));
	Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 35, 101, 62, 75, false, TICKS_TO_TIME(2));
	Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 35, 101, 62, 75, false, TICKS_TO_TIME(2));
	if (waypoint->m_vecOrigin.Dist(LocalPlayer.Current_Origin) < PLAYER_RENDER_DIST_TO_TEXT)
	{
		//decrypts(0)
		Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("FAKE DUCK"));
		//encrypts(0)
	}
}

void DrawCampSpot(Waypoint* waypoint)
{
	Interfaces::DebugOverlay->AddBoxOverlay(waypoint->m_vecOrigin, waypoint->m_vecMins, waypoint->m_vecMaxs, waypoint->m_vecAngles, 163, 73, 164, 30, TICKS_TO_TIME(2));
	Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 163, 73, 164, 75, false, TICKS_TO_TIME(2));
	Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 163, 73, 164, 75, false, TICKS_TO_TIME(2));
	if (waypoint->m_vecOrigin.Dist(LocalPlayer.Current_Origin) < PLAYER_RENDER_DIST_TO_TEXT)
	{
		//decrypts(0)
		Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("CAMP SPOT"));
		//encrypts(0)
	}
}

void DeleteClosestWaypoint()
{
	LocalPlayer.Get(&LocalPlayer);
	if (!LocalPlayer.Entity)
		return;

	Vector FootPosition = *LocalPlayer.Entity->GetAbsOrigin();
	Waypoint *ClosestWaypoint = nullptr;
	float ClosestDist;
	m_WaypointsMutex.lock();
	for (auto& waypoint : m_Waypoints)
	{
		float dist = waypoint->m_vecOrigin.Dist(FootPosition);
		if (dist <= 128.0f && (!ClosestWaypoint || dist < ClosestDist))
		{
			ClosestDist = dist;
			ClosestWaypoint = waypoint;
		}
	}
	if (ClosestWaypoint)
	{
		DeleteWaypoint(ClosestWaypoint);
		//decrypts(0)
		Interfaces::Surface->Play_Sound(XorStr("items\\suitchargeno1.wav"));
		//encrypts(0)
	}
	m_WaypointsMutex.unlock();
}

void RunWalkBot()
{
	LocalPlayer.Get(&LocalPlayer);
	if (LocalPlayer.Entity && LocalPlayer.Entity->GetAlive())
	{
		Waypoint *LastWaypoint = !m_LastWaypoints.empty() ? m_LastWaypoints.front() : nullptr;
		Waypoint *SecondToLastWaypoint = m_LastWaypoints.size() > 1 ? m_LastWaypoints[1] : nullptr;
		CUserCmd* cmd = CurrentUserCmd.cmd;
		Vector FeetPosition = *LocalPlayer.Entity->GetAbsOrigin();
		//FeetPosition.z = LocalPlayer.Entity->GetBonePosition(HITBOX_LEFT_FOOT).z;
		Vector EyePosition = LocalPlayer.Entity->EyePosition();
		QAngle EyeAngles, CustomEyeAngles;
		bool UseCustomEyeAngles = false;
		Interfaces::EngineClient->GetViewAngles(EyeAngles);

		m_WaypointsMutex.lock();

		if (!m_CurrentWaypoint)
		{
			//Find new waypoint to walk to
			m_LastWaypoints.clear();
			m_CurrentWaypoint = ScanForNewWaypoint();
		}

#ifdef _DEBUG
		if (GetAsyncKeyState(VK_CAPITAL))
			int test = 1;
#endif

		if (m_CurrentWaypoint)
		{	
			float CustomPitch = 0.0f;
			cmd->buttons |= IN_FORWARD;

			float MinDistanceToWaypoint = 32.0f;

			bool ReachedWaypoint;
			if (m_CurrentWaypoint->IsLadder())
			{
				float DistLeftRight = m_CurrentWaypoint->m_vecOrigin.Dist2D(FeetPosition);
				float DistZ = fabsf(m_CurrentWaypoint->m_vecOrigin.z - FeetPosition.z);

				auto IsTop = m_CurrentWaypoint->IsLadderTop();
				auto IsStep = m_CurrentWaypoint->IsLadderStep();
				auto IsBottom = m_CurrentWaypoint->IsLadderBottom();
				float MinZ = IsTop ? 32.0f : 32.0f;

				ReachedWaypoint = DistLeftRight < 50.0f && DistZ < MinZ;
			}
			else
			{
				ReachedWaypoint = m_CurrentWaypoint->m_vecOrigin.Dist(FeetPosition) < MinDistanceToWaypoint || m_CurrentWaypoint->m_vecOrigin.Dist(EyePosition) < MinDistanceToWaypoint;
			}

			if (ReachedWaypoint)
			{
				//We reached the waypoint. Find a new one

				StoreCurrentWaypoint();
				LastWaypoint = !m_LastWaypoints.empty() ? m_LastWaypoints.front() : nullptr;
				SecondToLastWaypoint = m_LastWaypoints.size() > 1 ? m_LastWaypoints[1] : nullptr;

				if (m_CurrentWaypoint->m_iFlags & (WAYPOINT_FLAG_CAMP | WAYPOINT_FLAG_FAKEDUCK))
				{
					//We want to camp here for a little bit
#ifdef _DEBUG
					m_NextMoveTime = Interfaces::Globals->realtime + 2.0f;
#else
					m_NextMoveTime = Interfaces::Globals->realtime + 10.0f;
#endif
				}

				if (m_CurrentWaypoint->m_iFlags & WAYPOINT_FLAG_JUMP)
					cmd->buttons |= IN_JUMP;
				
				if (m_CurrentWaypoint->m_iFlags & WAYPOINT_FLAG_CROUCH)
					cmd->buttons |= IN_DUCK;

				if (LastWaypoint->IsLadder())
				{
					if (LastWaypoint->IsLadderBottom())
					{
						if (SecondToLastWaypoint && SecondToLastWaypoint->m_iFlags & (WAYPOINT_FLAG_LADDER_TOP | WAYPOINT_FLAG_LADDER_STEP))
						{
							//We just travelled down this ladder, we want to get off now
							CurrentUserCmd.cmd->buttons |= IN_JUMP;
						}
					}
					/*
					else if (LastWaypoint->IsLadderTop())
					{
						if (SecondToLastWaypoint && SecondToLastWaypoint->m_iFlags & (WAYPOINT_FLAG_LADDER_STEP | WAYPOINT_FLAG_LADDER_BOTTOM))
						{
							//We just travelled up this ladder, we want to get off now
							CurrentUserCmd.cmd->buttons |= IN_JUMP;
						}
					}
					*/
				}

				//TODO FAKEDUCK

				m_CurrentWaypoint = ScanForNewWaypoint();
			}
			else
			{
				//Not yet reached the waypoint. Run to the one we already have

				if (variable::get().waypoints.b_enable_creator)
				{
					//Draw the highlight of the waypoint we are running to
					Interfaces::DebugOverlay->AddBoxOverlay(m_CurrentWaypoint->m_vecOrigin, Vector(-20, -20, -20), Vector(20, 20, 20), m_CurrentWaypoint->m_vecAngles, 145, 215, 232, 150, TICKS_TO_TIME(2));
					//decrypts(0)
					Interfaces::DebugOverlay->AddTextOverlay(m_CurrentWaypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("Walkbot Destination"));
					//encrypts(0)
				}

				//Check to see if we can see the waypoint still
				CTraceFilterSimple filter((IHandleEntity*)LocalPlayer.Entity, COLLISION_GROUP_NONE);
				if (!TraceWaypoint(m_CurrentWaypoint, EyePosition, &filter, waypoint_trace_diameter, waypoint_trace_mins, waypoint_trace_maxs))
				{
					m_TotalTimeWeCouldntTraceWaypoint += TICK_INTERVAL;
				}
				else
				{
					m_TotalTimeWeCouldntTraceWaypoint = 0.0f;

					if (m_CurrentWaypoint->IsBreakable() && !m_CurrentWaypoint->IsBroken())
					{
						//Try to shoot this breakable object
						CBaseCombatWeapon *weapon = LocalPlayer.Entity->GetWeapon();
						if (weapon && LocalPlayer.WeaponVars.IsGun && weapon->GetClipOne() > 0)
						{
							QAngle AngleToWaypoint = CalcAngle(EyePosition, m_CurrentWaypoint->m_vecOrigin);
							if (fabsf(AngleDiff(AngleToWaypoint.y, EyeAngles.y)) < 20.0f)
							{
								if (g_WeaponController.WeaponDidFire(cmd->buttons | IN_ATTACK))
								{
									cmd->buttons |= IN_ATTACK;
									CurrentUserCmd.cmd_backup.buttons |= IN_ATTACK;
									CustomPitch = AngleToWaypoint.x;
									NormalizeAngle(CustomPitch);
									m_CurrentWaypoint->Break();
								}
							}
						}
						else
						{
							//Can't fire, so we will get stuck on this waypoint, find a new one
							StoreCurrentWaypoint();
							m_CurrentWaypoint = ScanForNewWaypoint();
						}
					}
					else
					{
						if (Interfaces::Globals->realtime >= m_NextMoveTime)
						{
							//Now check to see if we haven't moved for a while
							if (LocalPlayer.Entity->GetVelocity().Length() < 15.0f)
							{
								m_TotalTimeWeCouldntMove += TICK_INTERVAL;

								//Search for a breakable near by our FOV
								
								Waypoint* NearestBreakable = ScanForNearbyBreakable();
								if (NearestBreakable)
								{
									QAngle AngleToWaypoint = CalcAngle(EyePosition, m_CurrentWaypoint->m_vecOrigin);
									if (fabsf(AngleDiff(AngleToWaypoint.y, EyeAngles.y)) < 20.0f)
									{
										if (g_WeaponController.WeaponDidFire(cmd->buttons | IN_ATTACK))
										{
											cmd->buttons |= IN_ATTACK;
											CurrentUserCmd.cmd_backup.buttons |= IN_ATTACK;
											CustomPitch = AngleToWaypoint.x;
											NormalizeAngle(CustomPitch);
											m_CurrentWaypoint->Break();
										}
									}
									else
									{
										CustomEyeAngles = AngleToWaypoint;
									}
								}

							}
							else
							{
								m_TotalTimeWeCouldntMove = 0.0f;
							}
						}
					}
				}

				if (m_CurrentWaypoint && m_TotalTimeWeCouldntMove > 0.6f)
				{
					//Try to find something else, we were blocked too long
					StoreCurrentWaypoint();
					m_CurrentWaypoint = ScanForNewWaypoint();
				}

				if (m_CurrentWaypoint && m_TotalTimeWeCouldntTraceWaypoint > 1.25f)
				{
					//Try to find something else, we were blocked too long
					StoreCurrentWaypoint();
					m_CurrentWaypoint = ScanForNewWaypoint();
				}

				if (m_CurrentWaypoint && m_CurrentWaypoint->IsLadder())
				{
					//Trace forward and search for a wall
					trace_t tr;
					Ray_t ray;
					Vector vecDir;
					AngleVectors(EyeAngles, &vecDir);
					VectorNormalizeFast(vecDir);
					ray.Init(EyePosition, EyePosition + vecDir * 32.0f);
					CTraceFilterSimple filter((IHandleEntity*)LocalPlayer.Entity, COLLISION_GROUP_NONE);
					Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

					if (tr.DidHitWorld())
					{
						UseCustomEyeAngles = true;
						QAngle LadderAngle;
						VectorAngles(-tr.plane.normal, LadderAngle);
						CustomEyeAngles = QAngle(0.0f, LadderAngle.y, 0.0f);
						CustomEyeAngles.ClampY();
					}

					if (m_CurrentWaypoint->m_vecOrigin.z < FeetPosition.z)
					{
						auto IsTop = m_CurrentWaypoint->IsLadderTop();
						auto IsStep = m_CurrentWaypoint->IsLadderStep();
						auto IsBottom = m_CurrentWaypoint->IsLadderBottom();
						int LastIsTop = 0;
						int LastIsStep = 0;
						int LastIsBottom = 0;
						if (LastWaypoint)
						{
							LastIsTop = LastWaypoint->IsLadderTop();
							LastIsStep = LastWaypoint->IsLadderStep();
							LastIsBottom = LastWaypoint->IsLadderBottom();
						}


						if (LastWaypoint && LastWaypoint->m_vecOrigin.z > m_CurrentWaypoint->m_vecOrigin.z)
						{
							//CustomPitch = 45.0f;
							cmd->buttons &= ~IN_FORWARD;
							cmd->buttons |= IN_BACK;
						}
					}
					else
					{
						//CustomPitch = -45.0f;
					}
				}
			}

			if (m_CurrentWaypoint && Interfaces::Globals->realtime >= m_NextMoveTime)
			{
				//Start walking to the destination waypoint

				Vector vecDir = m_CurrentWaypoint->m_vecOrigin - EyePosition;
				cmd->forwardmove = 450.0f;
				cmd->sidemove = cmd->upmove = 0.0f;
				VectorAngles(vecDir, cmd->viewangles);
				NormalizeAngles(cmd->viewangles);
				if (UseCustomEyeAngles)
					cmd->viewangles = CustomEyeAngles;

				cmd->viewangles.x = CustomPitch;
				cmd->viewangles.z = 0.0f;
				cmd->viewangles.Clamp();
				Interfaces::EngineClient->SetViewAngles(cmd->viewangles);
				LocalPlayer.CurrentEyeAngles = LocalPlayer.FinalEyeAngles = cmd->viewangles;
			}
		}

		if (!m_CurrentWaypoint)
		{
			//Just walk forward in hopes of finding a new waypoint somewhere..
			cmd->forwardmove = 450.0f;
			cmd->sidemove = cmd->upmove = 0.0f;

			//Try to find an area that wont cause us to get blocked
			for (int i = 0; i < 360; i += 30)
			{
				QAngle NewEyeAngles = EyeAngles;
				Vector vecDir;
				NewEyeAngles.y += (float)i;
				NormalizeAngle(NewEyeAngles.y);
				AngleVectors(NewEyeAngles, &vecDir);
				VectorNormalizeFast(vecDir);

				CTraceFilterSimple filter((IHandleEntity*)LocalPlayer.Entity, COLLISION_GROUP_NONE);
				Ray_t ray;
				trace_t tr;
				ray.Init(EyePosition, EyePosition + vecDir * 75.0f);
				Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

				if (!tr.m_pEnt)
				{
					cmd->viewangles = NewEyeAngles;
					cmd->viewangles.Clamp();
					Interfaces::EngineClient->SetViewAngles(cmd->viewangles);
					LocalPlayer.CurrentEyeAngles = LocalPlayer.FinalEyeAngles = cmd->viewangles;
					break;
				}
			}
		}

		m_WaypointsMutex.unlock();
	}
}

void Waypoint_OnCreateMove()
{
	auto& var = variable::get();

	m_WaypointsMutex.lock();

	if (!var.waypoints.b_enable_walkbot)
	{
		ResetGlobalWalkBotVariables();
	}

	if (var.waypoints.b_enable_creator && LocalPlayer.Entity)
	{
		static float last_creation_time = 0.0f;
		Vector EyePosition = LocalPlayer.Entity->EyePosition();
		Vector FootPosition = *LocalPlayer.Entity->GetAbsOrigin();
		QAngle EyeAngles;
		Interfaces::EngineClient->GetViewAngles(EyeAngles);

		if (var.waypoints.b_enable_autowaypoint)
		{
			bool found_close_waypoint = false;
			for (auto& waypoint : m_Waypoints)
			{
				if (waypoint->m_vecOrigin.Dist(FootPosition) < 128.0f)
				{
					found_close_waypoint = true;
					break;
				}
			}

			if (!found_close_waypoint)
			{
				Waypoint* waypoint = CreateWaypoint();
				if (waypoint)
				{
					waypoint->m_vecOrigin = FootPosition;
					waypoint->m_vecAngles = EyeAngles;
					waypoint->m_vecNormal.Init();
					waypoint->m_vecMins = { -5.0f, -5.0f, -5.0f };
					waypoint->m_vecMaxs = { 5.0f, 5.0f, 5.0f };
					waypoint->m_iFlags = 0;

					if (CurrentUserCmd.IsDucking())
						waypoint->m_iFlags |= WAYPOINT_FLAG_CROUCH;

					if (CurrentUserCmd.IsJumping())
						waypoint->m_iFlags |= WAYPOINT_FLAG_JUMP;

					if (CurrentUserCmd.IsUsing())
						waypoint->m_iFlags |= WAYPOINT_FLAG_USE;

					//decrypts(0)
					Interfaces::Surface->Play_Sound(XorStr("ui\\csgo_ui_crate_item_scroll.wav"));
					//encrypts(0)
				}
			}
		}

		//Create/Delete waypoint hotkeys will only be run if the menu is closed
		if (!ui::get().is_visible())
		{
			if (input::get().is_key_down(var.waypoints.i_createwaypoint_key))
			{
				if (Interfaces::Globals->realtime - last_creation_time > 1.5f)
				{
					Waypoint* waypoint = CreateWaypoint();
					if (waypoint)
					{
						waypoint->m_vecOrigin = FootPosition;
						waypoint->m_vecAngles = EyeAngles;
						waypoint->m_vecNormal.Init();
						waypoint->m_vecMins = { -5.0f, -5.0f, -5.0f };
						waypoint->m_vecMaxs = { 5.0f, 5.0f, 5.0f };
						waypoint->m_iFlags = 0;
						//decrypts(0)
						Interfaces::Surface->Play_Sound(XorStr("ui\\csgo_ui_crate_item_scroll.wav"));
						//encrypts(0)
					}
					last_creation_time = Interfaces::Globals->realtime;
				}
			}
			else if (input::get().is_key_down(var.waypoints.i_deletewaypoint_key))
			{
				if (Interfaces::Globals->realtime - last_creation_time > 1.5f)
				{
					DeleteClosestWaypoint();
					last_creation_time = Interfaces::Globals->realtime;
				}
			}
		}

		//draw all waypoints
		for (auto& waypoint : m_Waypoints)
		{
			if (waypoint->m_iFlags & (WAYPOINT_FLAG_ONEWAY | WAYPOINT_FLAG_ONEWAY_VICTIM))
			{
				DrawOneWay(waypoint);
			}
			else if (waypoint->m_iFlags & WAYPOINT_FLAG_CTSPAWN)
			{
				Interfaces::DebugOverlay->AddBoxOverlay(waypoint->m_vecOrigin, Vector(-16, -16, -16), Vector(16, 16, 16), waypoint->m_vecAngles, 133, 182, 241, 75, TICKS_TO_TIME(2));
				if (waypoint->m_vecOrigin.Dist(LocalPlayer.Current_Origin) < PLAYER_RENDER_DIST_TO_TEXT)
				{
					//decrypts(0)
					Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("CT SPAWN"));
					//encrypts(0)
				}
			}
			else if (waypoint->m_iFlags & WAYPOINT_FLAG_TSPAWN)
			{
				Interfaces::DebugOverlay->AddBoxOverlay(waypoint->m_vecOrigin, Vector(-16, -16, -16), Vector(16, 16, 16), waypoint->m_vecAngles, 255, 128, 0, 75, TICKS_TO_TIME(2));
				if (waypoint->m_vecOrigin.Dist(LocalPlayer.Current_Origin) < PLAYER_RENDER_DIST_TO_TEXT)
				{
					//decrypts(0)
					Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("T SPAWN"));
					//encrypts(0)
				}
			}
			else if (waypoint->m_iFlags & (WAYPOINT_FLAG_LADDER_TOP | WAYPOINT_FLAG_LADDER_STEP | WAYPOINT_FLAG_LADDER_BOTTOM))
			{
				Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 187, 125, 104, 75, false, TICKS_TO_TIME(2));
				Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 187, 125, 104, 75, false, TICKS_TO_TIME(2));
				if (waypoint->m_vecOrigin.Dist(LocalPlayer.Current_Origin) < PLAYER_RENDER_DIST_TO_TEXT)
				{
					if (waypoint->m_iFlags & WAYPOINT_FLAG_LADDER_TOP)
					{
						//decrypts(0)
						Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("LADDER TOP"));
						//encrypts(0)
					}
					else if (waypoint->m_iFlags & WAYPOINT_FLAG_LADDER_STEP)
					{
						//decrypts(0)
						Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("LADDER STEP"));
						//encrypts(0)
					}
					else if (waypoint->m_iFlags & WAYPOINT_FLAG_LADDER_BOTTOM)
					{
						//decrypts(0)
						Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("LADDER BOTTOM"));
						//encrypts(0)
					}
				}
			}
			else if (waypoint->m_iFlags & WAYPOINT_FLAG_BOMBSITE)
			{
				//Interfaces::DebugOverlay->AddBoxOverlay(waypoint->m_vecOrigin, waypoint->m_vecMins, waypoint->m_vecMaxs, waypoint->m_vecAngles, 254, 37, 37, 30, TICKS_TO_TIME(2));
				Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 254, 37, 37, 75, false, TICKS_TO_TIME(2));
				Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 254, 37, 37, 75, false, TICKS_TO_TIME(2));
				if (waypoint->m_vecOrigin.Dist(LocalPlayer.Current_Origin) < PLAYER_RENDER_DIST_TO_TEXT)
				{
					//decrypts(0)
					Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("BOMB SITE"));
					//encrypts(0)
				}
			}
			else if (waypoint->m_iFlags & WAYPOINT_FLAG_SAFEZONE)
			{
				//Interfaces::DebugOverlay->AddBoxOverlay(waypoint->m_vecOrigin, waypoint->m_vecMins, waypoint->m_vecMaxs, waypoint->m_vecAngles, 128, 255, 255, 30, TICKS_TO_TIME(2));
				Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 128, 255, 255, 75, false, TICKS_TO_TIME(2));
				Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 128, 255, 255, 75, false, TICKS_TO_TIME(2));
				if (waypoint->m_vecOrigin.Dist(LocalPlayer.Current_Origin) < PLAYER_RENDER_DIST_TO_TEXT)
				{
					//decrypts(0)
					Interfaces::DebugOverlay->AddTextOverlay(waypoint->m_vecOrigin, TICKS_TO_TIME(2), XorStr("SAFE ZONE"));
					//encrypts(0)
				}
			}
			else if (waypoint->m_iFlags & WAYPOINT_FLAG_FAKEDUCK)
			{
				DrawFakeDuckSpot(waypoint);
			}
			else if (waypoint->m_iFlags & WAYPOINT_FLAG_CAMP)
			{
				DrawCampSpot(waypoint);
			}
			else if (waypoint->m_iFlags & WAYPOINT_FLAG_PATH)
			{
				//draw it showing as something special
				Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 255, 201, 14, 200, false, TICKS_TO_TIME(2));
				Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 255, 201, 14, 200, false, TICKS_TO_TIME(2));
			}
			else
			{
				//draw normal waypoint
				if (waypoint->m_iFlags > 0)
				{
					//draw it showing as something special
					Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 140, 140, 255, 200, false, TICKS_TO_TIME(2));
					Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 140, 140, 255, 200, false, TICKS_TO_TIME(2));
				}
				else
				{
					Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 0, 0, 255, 255, false, TICKS_TO_TIME(2));
					Interfaces::DebugOverlay->AddTriangleOverlay(waypoint->m_vecOrigin + TRIANGLE_SIDE, waypoint->m_vecOrigin - TRIANGLE_SIDE, waypoint->m_vecOrigin + TRIANGLE_TOP, 0, 0, 255, 255, false, TICKS_TO_TIME(2));
				}
			}
		}
	}
	else if (var.waypoints.b_draw_oneways || var.waypoints.b_draw_campspots)
	{
		//draw oneways
		for (auto& waypoint : m_Waypoints)
		{
			if (waypoint->m_iFlags & (WAYPOINT_FLAG_ONEWAY | WAYPOINT_FLAG_ONEWAY_VICTIM))
			{
				if (var.waypoints.b_draw_oneways)
					DrawOneWay(waypoint);
			}
			else if (waypoint->m_iFlags & WAYPOINT_FLAG_FAKEDUCK)
			{
				if (var.waypoints.b_draw_campspots)
					DrawFakeDuckSpot(waypoint);
			}
			else if (waypoint->m_iFlags & WAYPOINT_FLAG_CAMP)
			{
				if (var.waypoints.b_draw_campspots)
					DrawCampSpot(waypoint);
			}
		}
	}

//#ifdef _DEBUG
	if (var.waypoints.b_enable_walkbot)
	{
		RunWalkBot();


		if (var.waypoints.b_walkbot_ragehack)
		{

		}
	}
//#endif
	m_WaypointsMutex.unlock();
}

void ClearAllWaypoints()
{
	m_WaypointsMutex.lock();

	ResetGlobalWaypointSystemVariables();

	for (auto& i = m_Waypoints.begin(); i != m_Waypoints.end();)
	{
		delete *i;
		i = m_Waypoints.erase(i);
	}
	m_WaypointsMutex.unlock();
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

std::string GetWaypointFileName()
{
	const char* levelname = Interfaces::EngineClient->GetLevelName();
	if (!levelname)
		return "";

	std::string mappath = levelname;
	char *waypointsstr = XorStrCT("waypoints");
	std::string fullpath = "\\";
	fullpath = fullpath + waypointsstr + "\\";

	char *maps = XorStrCT("maps");
	std::string mapsstr = maps;
	mapsstr = mapsstr + "\\";
	size_t pos = mappath.find(mapsstr);
	if (pos != std::string::npos)
		mappath.erase(pos, mapsstr.length());
	mappath = mappath.substr(0, mappath.find_last_of(".")); //remove file extension

	char *wptstr = XorStrCT(".wpt");
	fullpath = fullpath + mappath + wptstr;
	return fullpath;
}

std::string GetWaypointFileFullPath()
{
	const char* levelname = Interfaces::EngineClient->GetLevelName();
	const std::string configdir = config::get().get_config_directory();
	if (!levelname || configdir.empty())
		return "";

	std::string mappath = levelname;
	char *waypointsstr = XorStrCT("waypoints");
	std::string fullpath = configdir + "\\" + waypointsstr + "\\";
	CreateDirectory(fullpath.c_str(), nullptr);

	char *maps = XorStrCT("maps");
	std::string mapsstr = maps;
	mapsstr = mapsstr + "\\";
	size_t pos = mappath.find(mapsstr);
	if (pos != std::string::npos)
		mappath.erase(pos, mapsstr.length());
	mappath = mappath.substr(0, mappath.find_last_of(".")); //remove file extension
	pos = mappath.find_last_of("\\");
	std::string custom_directory;
	if (pos != std::string::npos)
	{
		custom_directory = mappath.substr(0, pos + 1); //get the full workshop directory without the map name
		pos = 0;

		while (pos = custom_directory.find("\\", pos), pos < custom_directory.back())
		{
			pos += 1;
			std::string built_directory = custom_directory.substr(0, pos); //workshop
			auto newdir = std::string(fullpath + built_directory);
			CreateDirectory(newdir.c_str(), nullptr);
		}
	}
	char *wptstr = XorStrCT(".wpt");
	fullpath = fullpath + mappath + wptstr;
	return fullpath;
}

bool ReadWaypointFile(std::ifstream& file)
{
	bool Success = false;
	if (file.is_open())
	{
		int version;
		if (file.read((char*)&version, sizeof(int)))
		{
			if (version == WAYPOINT_FILE_VERSION)
			{
				size_t numwaypoints;
				if (file.read((char*)&numwaypoints, sizeof(size_t)))
				{
					if (numwaypoints > 0 && numwaypoints <= MAX_WAYPOINTS)
					{
						Waypoint tmp;
						for (size_t i = 0; i < numwaypoints; ++i)
						{
							if (file.read((char*)&tmp, sizeof(Waypoint)))
							{
								if (tmp.m_vecOrigin.IsFinite() && tmp.m_vecNormal.IsFinite() && !tmp.m_vecAngles.IsNaN() && tmp.m_iFlags >= 0)
								{
									tmp.ResetTemporaryFlags();
									Waypoint *wp = CreateWaypoint();
									if (wp)
									{
										*wp = tmp;
										Success = true;
									}
									else
									{
										break;
									}
								}
							}
							else
							{
								break;
							}
						}
					}
				}
			}
		}
		file.close();
	}
	return Success;
}

void ReadWaypoints()
{
	m_WaypointsMutex.lock();

	ClearAllWaypoints();

	const char* levelname = Interfaces::EngineClient->GetLevelName();
	if (levelname)
	{
		//TODO: Try reading them from hard coded levels here first
		bool Success = false;

		if (!Success)
		{
			const std::string configdir = config::get().get_config_directory();
			if (!configdir.empty())
			{
				std::string fullpath = GetWaypointFileFullPath();
				if (!fullpath.empty())
				{
					std::ifstream file;
					file.open(fullpath, std::fstream::binary);
					if (!ReadWaypointFile(file) && variable::get().waypoints.b_download_waypoints)
					{
						//Try downloading the file off the server
						CURL *curl;
						FILE *fp;
						CURLcode res;
						std::string waypointfile = GetWaypointFileName();
						if (!waypointfile.empty())
						{
							char *urlstr = XorStrCT("http://nightfirepc.com");
							std::string url = urlstr + waypointfile;
							std::replace(url.begin(), url.end(), '\\', '/');
							curl = curl_easy_init();
							if (curl) {
								fp = fopen(fullpath.c_str(), "wb");
								curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
								curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
								curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
								res = curl_easy_perform(curl);
								/* always cleanup */
								curl_easy_cleanup(curl);
								fclose(fp);

								if (res == CURLE_OK)
								{
									//Try reading the file now that we downloaded it
									file.open(fullpath, std::fstream::binary);
									if (ReadWaypointFile(file))
									{
										//Save the waypoint file so we don't have to download it again
										SaveWaypoints();
									}
								}
							}
						}
					}
				}
			}
		}
	}
	m_WaypointsMutex.unlock();
}

void SaveWaypoints()
{
	m_WaypointsMutex.lock();
	if (!m_Waypoints.empty())
	{
		const char* levelname = Interfaces::EngineClient->GetLevelName();
		if (levelname)
		{
			const std::string configdir = config::get().get_config_directory();
			if (!configdir.empty())
			{
				std::string fullpath = GetWaypointFileFullPath();
				if (!fullpath.empty())
				{
					std::ofstream file;
					file.open(fullpath, std::fstream::out | std::fstream::trunc | std::fstream::binary);
					if (file.is_open())
					{
						int version = WAYPOINT_FILE_VERSION;
						file.write((char*)&version, sizeof(int));
						size_t numwaypoints = m_Waypoints.size();
						file.write((char*)&numwaypoints, sizeof(size_t));
						for (auto& i : m_Waypoints)
						{
							Waypoint *p = i;
							Waypoint tmp = *p;
							tmp.ResetTemporaryFlags();
							file.write((char*)&tmp, sizeof(Waypoint));
						}
					}
				}
			}
		}
	}
	m_WaypointsMutex.unlock();
}

Waypoint* CreateWaypoint()
{
	m_WaypointsMutex.lock();
	if (m_Waypoints.size() + 1 <= MAX_WAYPOINTS)
	{
		Waypoint* wp = new Waypoint;
		if (wp)
		{
			m_Waypoints.push_back(wp);
			m_WaypointsMutex.unlock();
			return wp;
		}
	}
	m_WaypointsMutex.unlock();
	return nullptr;
}

void DeleteWaypoint(Waypoint* wp)
{
	m_WaypointsMutex.lock();
	auto i = std::find(m_Waypoints.begin(), m_Waypoints.end(), wp);
	if (i != m_Waypoints.end())
	{
		auto& var = variable::get();
		if (var.waypoints.m_pSelectedWaypoint == wp)
			var.waypoints.m_pSelectedWaypoint = nullptr;

		for (auto i = m_LastWaypoints.begin(); i != m_LastWaypoints.end();)
		{
			if (*i == wp)
				i = m_LastWaypoints.erase(i);
			else
				++i;
		}

		if (m_CurrentWaypoint == wp)
			m_CurrentWaypoint = nullptr;

		delete wp;
		m_Waypoints.erase(i);
	}
	m_WaypointsMutex.unlock();
}

#if 0
void DeleteWaypoint(std::vector<Waypoint*>::iterator& wp)
{
	auto i = std::find(m_Waypoints.begin(), m_Waypoints.end(), wp);
	if (i != m_Waypoints.end())
	{
		auto& var = variable::get();
		if (var.waypoints.m_pSelectedWaypoint == *i)
			var.waypoints.m_pSelectedWaypoint = nullptr;

		delete *i;
		m_Waypoints.erase(i);
	}
}
#endif

Waypoint* GetWaypointFromScreenPosition(ImVec2 CursorPos, Vector* WorldPosition, Vector *Direction)
{
	LocalPlayer.Get(&LocalPlayer);
	if (LocalPlayer.Entity)
	{
		QAngle EyeAngles;
		Vector vecDir;
		Vector EyePosition = LocalPlayer.Entity->EyePosition();
		Interfaces::EngineClient->GetViewAngles(EyeAngles);
		CViewSetup *setup = new CViewSetup;
		setup->fov = 180.0f;
		g_Visuals.FOVChanger(setup);
		ScreenToWorld(CursorPos.x, CursorPos.y, setup->fov, EyePosition, EyeAngles, vecDir);
		delete setup;
		VectorNormalizeFast(vecDir);

		Ray_t ray;
		ray.Init(EyePosition, EyePosition + vecDir * 8192.0f);
		CTraceFilter filter;
		filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;
		trace_t tr;
		Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);
		Vector normal = tr.plane.normal;

		if (tr.DidHit())
		{
			if (WorldPosition)
				*WorldPosition = tr.endpos;
			if (Direction)
				*Direction = normal;

			Waypoint *wp = nullptr;
			std::vector<CSphere>Spheres;
			std::vector<COBB>OBBs;
			const Vector mins = { 0, 0, 0 };
			const Vector maxs = { 0, 0, 8.f };
			float diameter = 3.0f;
			ray.Init(EyePosition, tr.endpos);
			m_WaypointsMutex.lock();
			for (size_t i = 0; i < m_Waypoints.size(); ++i)
			{
				auto& waypoint = m_Waypoints[i];
				SetupCapsule(waypoint->m_vecOrigin + mins, waypoint->m_vecOrigin + maxs, diameter, HITBOX_HEAD, HITGROUP_HEAD, i, Spheres);
				//Interfaces::DebugOverlay->DrawPill(waypoint->m_vecOrigin + mins, waypoint->m_vecOrigin + maxs, diameter, 255, 255, 255, 75, 5.0f, 0, 1);
			}
			TRACE_HITBOX(LocalPlayer.Entity, ray, tr, Spheres, OBBs);
			if (tr.m_pEnt)
			{
				//we found a waypoint
				wp = m_Waypoints[tr.physicsbone];
			}
			m_WaypointsMutex.unlock();
			return wp;
		}
	}

	if (WorldPosition)
		WorldPosition->Init();
	if (Direction)
		Direction->Init();
	return nullptr;
}

void SortWaypointsByFOV(std::vector<Waypoint*>&List)
{
	if (List.size() > 1)
	{
		QAngle EyeAngles;
		Vector EyePosition = LocalPlayer.Entity->EyePosition();
		Interfaces::EngineClient->GetViewAngles(EyeAngles);

		std::sort(List.begin(), List.end(), [&EyeAngles, &EyePosition](Waypoint*& lhs, Waypoint*& rhs)
		{
			QAngle AngleToLHS = CalcAngle(EyePosition, lhs->m_vecOrigin);
			QAngle AngleToRHS = CalcAngle(EyePosition, rhs->m_vecOrigin);
			return fabsf(AngleDiff(EyeAngles.y, AngleToLHS.y)) < fabsf(AngleDiff(EyeAngles.y, AngleToRHS.y));
		});
	}
}

void SortWaypointsByFOV(std::deque<Waypoint*>&List)
{
	if (List.size() > 1)
	{
		QAngle EyeAngles;
		Vector EyePosition = LocalPlayer.Entity->EyePosition();
		Interfaces::EngineClient->GetViewAngles(EyeAngles);

		std::sort(List.begin(), List.end(), [&EyeAngles, &EyePosition](Waypoint*& lhs, Waypoint*& rhs)
		{
			QAngle AngleToLHS = CalcAngle(EyePosition, lhs->m_vecOrigin);
			QAngle AngleToRHS = CalcAngle(EyePosition, rhs->m_vecOrigin);
			return fabsf(AngleDiff(EyeAngles.y, AngleToLHS.y)) < fabsf(AngleDiff(EyeAngles.y, AngleToRHS.y));
		});
	}
}

void SortWaypointsByFOVAndDistance(std::vector<Waypoint*>&List)
{
	if (List.size() > 1)
	{
		QAngle EyeAngles;
		Vector EyePosition = LocalPlayer.Entity->EyePosition();
		Interfaces::EngineClient->GetViewAngles(EyeAngles);

		std::sort(List.begin(), List.end(), [&EyeAngles, &EyePosition](Waypoint*& lhs, Waypoint*& rhs)
		{
			QAngle AngleToLHS = CalcAngle(EyePosition, lhs->m_vecOrigin);
			QAngle AngleToRHS = CalcAngle(EyePosition, rhs->m_vecOrigin);
			return fabsf(AngleDiff(EyeAngles.y, AngleToLHS.y)) < fabsf(AngleDiff(EyeAngles.y, AngleToRHS.y));
		});

		std::sort(List.begin(), List.end(), [&EyeAngles, &EyePosition](Waypoint*& lhs, Waypoint*& rhs)
		{
			QAngle AngleToLHS = CalcAngle(EyePosition, lhs->m_vecOrigin);
			bool LHSIsCloserThanRHS = lhs->m_vecOrigin.Dist(EyePosition) < rhs->m_vecOrigin.Dist(EyePosition);
			float FOVToLHS = fabsf(AngleDiff(EyeAngles.y, AngleToLHS.y));
			return FOVToLHS < 45.0f && LHSIsCloserThanRHS;
		});
	}
}

void SortWaypointsByFOVAndDistance(std::deque<Waypoint*>&List)
{
	if (List.size() > 1)
	{
		QAngle EyeAngles;
		Vector EyePosition = LocalPlayer.Entity->EyePosition();
		Interfaces::EngineClient->GetViewAngles(EyeAngles);

		std::sort(List.begin(), List.end(), [&EyeAngles, &EyePosition](Waypoint*& lhs, Waypoint*& rhs)
		{
			QAngle AngleToLHS = CalcAngle(EyePosition, lhs->m_vecOrigin);
			QAngle AngleToRHS = CalcAngle(EyePosition, rhs->m_vecOrigin);
			return fabsf(AngleDiff(EyeAngles.y, AngleToLHS.y)) < fabsf(AngleDiff(EyeAngles.y, AngleToRHS.y));
		});

		std::sort(List.begin(), List.end(), [&EyeAngles, &EyePosition](Waypoint*& lhs, Waypoint*& rhs)
		{
			QAngle AngleToLHS = CalcAngle(EyePosition, lhs->m_vecOrigin);
			bool LHSIsCloserThanRHS = lhs->m_vecOrigin.Dist(EyePosition) < rhs->m_vecOrigin.Dist(EyePosition);
			float FOVToLHS = fabsf(AngleDiff(EyeAngles.y, AngleToLHS.y));
			return FOVToLHS < 45.0f && LHSIsCloserThanRHS;
		});
	}
}

void SortWaypointsByDistance(std::vector<Waypoint*>&List)
{
	if (List.size() > 1)
	{
		Vector EyePosition = LocalPlayer.Entity->EyePosition();

		std::sort(List.begin(), List.end(), [&EyePosition](Waypoint*& lhs, Waypoint*& rhs)
		{
			return lhs->m_vecOrigin.Dist(EyePosition) < rhs->m_vecOrigin.Dist(EyePosition);
		});
	}
}

void SortWaypointsByDistance(std::deque<Waypoint*>&List)
{
	if (List.size() > 1)
	{
		Vector EyePosition = LocalPlayer.Entity->EyePosition();

		std::sort(List.begin(), List.end(), [&EyePosition](Waypoint*& lhs, Waypoint*& rhs)
		{
			return lhs->m_vecOrigin.Dist(EyePosition) < rhs->m_vecOrigin.Dist(EyePosition);
		});
	}
}

bool TraceWaypoint(Waypoint* waypoint, const Vector& startpos, ITraceFilter* filter, const float diameter, const Vector& mins, const Vector& maxs, const int referenceindex)
{
	if (waypoint->m_iFlags & WAYPOINT_FLAG_CAMP)
		int test = 0;
	trace_t tr;
	Ray_t ray;
	ray.Init(startpos, waypoint->m_vecOrigin);
	Interfaces::EngineTrace->TraceRay(ray, MASK_SHOT, filter, &tr);
	if (tr.endpos.IsFinite())
	{
		std::vector<CSphere>Spheres;
		std::vector<COBB>OBBs;
		SetupCapsule(waypoint->m_vecOrigin + mins, waypoint->m_vecOrigin + maxs, diameter, HITBOX_HEAD, HITGROUP_HEAD, referenceindex, Spheres);
		//Interfaces::DebugOverlay->DrawPill(waypoint->m_vecOrigin + mins, waypoint->m_vecOrigin + maxs, diameter, 255, 255, 255, 75, 5.0f, 0, 1);
		ray.Init(startpos, tr.endpos);
		TRACE_HITBOX(LocalPlayer.Entity, ray, tr, Spheres, OBBs);
		return tr.m_pEnt != nullptr;
	}

	return false;
}

Waypoint* ScanForNearbyBreakable()
{
	m_WaypointsMutex.lock();

	float FOV = 50.0f;
	QAngle EyeAngles;
	Vector EyePosition = LocalPlayer.Entity->EyePosition();
	Vector FeetPosition = *LocalPlayer.Entity->GetAbsOrigin();
	Interfaces::EngineClient->GetViewAngles(EyeAngles);

	std::vector<Waypoint*> PotentialWaypoints;
	CTraceFilterSimple filter((IHandleEntity*)LocalPlayer.Entity, COLLISION_GROUP_NONE);

	for (size_t i = 0; i < m_Waypoints.size(); ++i)
	{
		auto& waypoint = m_Waypoints[i];
		if (waypoint->IsBreakable() && !waypoint->IsBroken())
		{
			float DistToOurFeet = (FeetPosition - waypoint->m_vecOrigin).Length();

			//Don't pick waypoints that are too far
			if (DistToOurFeet < 512.0f)
			{
				QAngle AngleToWaypoint = CalcAngle(EyePosition, waypoint->m_vecOrigin);
				if (fabsf(AngleDiff(EyeAngles.y, AngleToWaypoint.y)) <= FOV)
					//if (GetFov(EyePosition, waypoint->m_vecOrigin, QAngle(AngleToWaypoint.x, EyeAngles.y, EyeAngles.z)) <= FOV)
				{
					if (TraceWaypoint(waypoint, EyePosition, &filter, waypoint_trace_diameter, waypoint_trace_mins, waypoint_trace_maxs, i))
					{
						//we were able to trace this waypoint, add it to the list of potential waypoints to go to
						PotentialWaypoints.push_back(waypoint);
					}
				}
			}
		}
	}

	SortWaypointsByFOV(PotentialWaypoints);

	Waypoint *NearestBreakable = !PotentialWaypoints.empty() ? PotentialWaypoints[0] : nullptr;

	m_WaypointsMutex.unlock();

	return NearestBreakable;
}

Waypoint* ScanForNewWaypoint()
{
	m_TotalTimeWeCouldntTraceWaypoint = 0.0f;
	m_TotalTimeWeCouldntMove = 0.0f;

	LocalPlayer.Get(&LocalPlayer);
	if (!LocalPlayer.Entity || !LocalPlayer.Entity->GetAlive())
		return nullptr;

	m_WaypointsMutex.lock();

	auto& var = variable::get();
	QAngle EyeAngles;
	Vector EyePosition = LocalPlayer.Entity->EyePosition();
	Vector FeetPosition = *LocalPlayer.Entity->GetAbsOrigin();
	Interfaces::EngineClient->GetViewAngles(EyeAngles);

	Waypoint *LastWaypoint = !m_LastWaypoints.empty() ? m_LastWaypoints.front() : nullptr;
	Waypoint *SecondToLastWaypoint = m_LastWaypoints.size() > 1 ? m_LastWaypoints[1] : nullptr;

	float FOV = 180.0f;//!LastWaypoint || !LastWaypoint->IsLadder() ? 60.0f : 180.0f;

	//First look for waypoints near where the player is looking at
	std::vector<Waypoint*> PotentialWaypoints;
	CTraceFilterSimple filter((IHandleEntity*)LocalPlayer.Entity, COLLISION_GROUP_NONE);

	bool JustWentUpLadder = false;
	bool JustWentDownLadder = false;
	if (LastWaypoint && LastWaypoint->IsLadder() && SecondToLastWaypoint)
	{
		if (LastWaypoint->IsLadderTop())
		{
			if (SecondToLastWaypoint->m_iFlags & (WAYPOINT_FLAG_LADDER_STEP | WAYPOINT_FLAG_LADDER_BOTTOM))
			{
				//We just travelled up a ladder
				JustWentUpLadder = true;
			}
		}
		else if (LastWaypoint->IsLadderBottom())
		{
			if (SecondToLastWaypoint->m_iFlags & (WAYPOINT_FLAG_LADDER_TOP | WAYPOINT_FLAG_LADDER_STEP))
			{
				//We just travelled down a ladder
				JustWentDownLadder = true;
			}
		}
	}

	for (size_t i = 0; i < m_Waypoints.size(); ++i)
	{
		auto& waypoint = m_Waypoints[i];

		if (!waypoint->ShouldWalkHere())
			continue;

		if (m_LastWaypoints.empty() || std::find(m_LastWaypoints.begin(), m_LastWaypoints.end(), waypoint) == m_LastWaypoints.end())
		{
			float DistToOurFeet = (FeetPosition - waypoint->m_vecOrigin).Length();

			if (JustWentUpLadder)
			{
				//Don't pick waypoints that are far below us
				if (FeetPosition.z - waypoint->m_vecOrigin.z >= 72.0f)
					continue;
			}
			else if (JustWentDownLadder)
			{
				//Don't pick waypoints that are far above us
				if (waypoint->m_vecOrigin.z - FeetPosition.z >= 72.0f)
					continue;
			}

			//Don't pick waypoints that are too far
			if (DistToOurFeet < 512.0f)
			{
				QAngle AngleToWaypoint = CalcAngle(EyePosition, waypoint->m_vecOrigin);
				if (fabsf(AngleDiff(EyeAngles.y, AngleToWaypoint.y)) <= FOV)
					//if (GetFov(EyePosition, waypoint->m_vecOrigin, QAngle(AngleToWaypoint.x, EyeAngles.y, EyeAngles.z)) <= FOV)
				{
					if (TraceWaypoint(waypoint, EyePosition, &filter, waypoint_trace_diameter, waypoint_trace_mins, waypoint_trace_maxs, i))
					{
						//we were able to trace this waypoint, add it to the list of potential waypoints to go to
						PotentialWaypoints.push_back(waypoint);
					}
				}
			}
		}
	}

	//Deque so we can pre-sort it
	std::deque<Waypoint*>PotentialPathWaypoints;
	std::deque<Waypoint*>PotentialCampSpots;
	std::deque<Waypoint*>PotentialSafeZones;
	std::deque<Waypoint*>PotentialBombSites;
	std::deque<Waypoint*>PotentialLadderPoints;

	//Sort the potential waypoints by distance first and get a list of interesting waypoints
	Waypoint* NextWaypoint = nullptr;

	SortWaypointsByFOVAndDistance(PotentialWaypoints);
	//SortWaypointsByDistance(PotentialWaypoints);

	//Fill the interesting lists
	for (auto& waypoint : PotentialWaypoints)
	{
		if (waypoint->IsPath())
		{
			float HeightDifferenceFromOurFeet = fabsf(waypoint->m_vecOrigin.z - FeetPosition.z);
			QAngle AngleToWaypoint = CalcAngle(EyePosition, waypoint->m_vecOrigin);
			if (HeightDifferenceFromOurFeet <= 18.0f && fabsf(AngleDiff(EyeAngles.y, AngleToWaypoint.y)) < 90.0f)
				PotentialPathWaypoints.push_front(waypoint);
		}
		if (waypoint->IsLadder())

		if (waypoint->m_iFlags & (WAYPOINT_FLAG_CAMP | WAYPOINT_FLAG_ONEWAY | WAYPOINT_FLAG_FAKEDUCK))
			PotentialCampSpots.push_front(waypoint);
			PotentialLadderPoints.push_front(waypoint);
		if (waypoint->m_iFlags & WAYPOINT_FLAG_BOMBSITE)
			PotentialBombSites.push_front(waypoint);
		if (waypoint->m_iFlags & WAYPOINT_FLAG_SAFEZONE)
			PotentialSafeZones.push_front(waypoint);
	}

	SortWaypointsByDistance(PotentialCampSpots);
	SortWaypointsByDistance(PotentialPathWaypoints);
	SortWaypointsByDistance(PotentialLadderPoints);
	SortWaypointsByDistance(PotentialBombSites);
	SortWaypointsByDistance(PotentialSafeZones);

	if (!PotentialWaypoints.empty())
	{
		//Find out what the best waypoint to go to is

		if (LastWaypoint)
		{
			if (LastWaypoint->IsPath())
			{
				//try to find a new path waypoint
				if (!PotentialPathWaypoints.empty())
				{
					NextWaypoint = PotentialPathWaypoints[0];
				}
			}
			else if (LastWaypoint->IsLadder())
			{
				//deal with ladders
				Vector FootPosition = LocalPlayer.Entity->GetAbsOriginDirect();
				float ClosestLadderWaypointDist = FLT_MAX;

				if (LastWaypoint->m_iFlags & WAYPOINT_FLAG_LADDER_TOP)
				{
					if (!PotentialLadderPoints.empty())
					{
						//Try to find a step or bottom
						for (auto& point : PotentialLadderPoints)
						{
							//We want to go down, not up!
							if (point->m_vecOrigin.z < LastWaypoint->m_vecOrigin.z)
							{
								float DistanceToOurFeet = point->m_vecOrigin.Dist(FootPosition);

								if (!NextWaypoint || DistanceToOurFeet < ClosestLadderWaypointDist)
								{
									NextWaypoint = point;
									ClosestLadderWaypointDist = DistanceToOurFeet;
								}
							}
						}
					}
				}
				else if (LastWaypoint->m_iFlags & WAYPOINT_FLAG_LADDER_BOTTOM)
				{
					if (!PotentialLadderPoints.empty())
					{
						//Try to find a step or top
						for (auto& point : PotentialLadderPoints)
						{
							//We want to go up, not down!
							if (point->m_vecOrigin.z > LastWaypoint->m_vecOrigin.z)
							{
								float DistanceToOurFeet = point->m_vecOrigin.Dist(FootPosition);

								if (!NextWaypoint || DistanceToOurFeet < ClosestLadderWaypointDist)
								{
									NextWaypoint = point;
									ClosestLadderWaypointDist = DistanceToOurFeet;
								}
							}
						}
					}
				}
				else if (LastWaypoint->m_iFlags & WAYPOINT_FLAG_LADDER_STEP)
				{
					if (!PotentialLadderPoints.empty())
					{
						//Try to find a step or end point
						bool GoUp = !SecondToLastWaypoint || SecondToLastWaypoint->m_vecOrigin.z < LastWaypoint->m_vecOrigin.z;

						for (auto& point : PotentialLadderPoints)
						{
							if (GoUp ? point->m_vecOrigin.z > LastWaypoint->m_vecOrigin.z : point->m_vecOrigin.z < LastWaypoint->m_vecOrigin.z)
							{
								float DistanceToOurFeet = point->m_vecOrigin.Dist(FootPosition);

								if (!NextWaypoint || DistanceToOurFeet < ClosestLadderWaypointDist)
								{
									NextWaypoint = point;
									ClosestLadderWaypointDist = DistanceToOurFeet;
								}
							}
						}
					}
				}

				if (!NextWaypoint)
				{
					//press jump to get the fuck off this ladder
					CurrentUserCmd.cmd->buttons |= IN_JUMP;
				}
			}
			else if (LastWaypoint->m_iFlags & (WAYPOINT_FLAG_CAMP | WAYPOINT_FLAG_FAKEDUCK))
			{
				//Prefer to find something that isn't a camp waypoint now

				//First, search for a path
				if (!PotentialPathWaypoints.empty())
					NextWaypoint = PotentialPathWaypoints[0];

				//No path, search for anything that isn't a camp spot

				if (!NextWaypoint)
				{
					for (auto& waypoint : PotentialWaypoints)
					{
						if (!(waypoint->m_iFlags & (WAYPOINT_FLAG_CAMP | WAYPOINT_FLAG_FAKEDUCK)))
						{
							NextWaypoint = waypoint;
							break;
						}
					}
				}
			}
		}

		if (!NextWaypoint)
		{
			//Find any new waypoint

			//First, search for a path
			if (!PotentialPathWaypoints.empty())
			{
				NextWaypoint = PotentialPathWaypoints[0];
			}
			//Then, search for a camp spot of interest
			else if (!PotentialCampSpots.empty())
			{
				NextWaypoint = PotentialCampSpots[0];
			}
			//Search for anything for now
			else if (!PotentialWaypoints.empty())
			{
				NextWaypoint = PotentialWaypoints[0];
				if (NextWaypoint)
				{
					if (NextWaypoint->IsLadder())
					{
						if (NextWaypoint->IsLadderStep())
						{
							float ClosestZToOurFeet = FLT_MAX;
							Waypoint* ClosestWaypointToOurFeet = nullptr;
							//Try to find a bottom or top rather than step
							
							for (auto& waypoint : PotentialLadderPoints)
							{
								if (waypoint->IsLadderStep())
									continue;
								float Dist2D = waypoint->m_vecOrigin.Dist2D(NextWaypoint->m_vecOrigin);
								float DistZToOurFeet = fabsf(waypoint->m_vecOrigin.z - FeetPosition.z);
								//Dist2D is used to avoid multiple ladders in the map, we only want the one closest to us
								if (Dist2D < 256.0f && (!ClosestWaypointToOurFeet || DistZToOurFeet < ClosestZToOurFeet))
								{
									ClosestWaypointToOurFeet = waypoint;
									ClosestZToOurFeet = DistZToOurFeet;
								}
							}

							NextWaypoint = ClosestWaypointToOurFeet;
						}
					}
				}
			}
		}
	}
	else if (LastWaypoint && LastWaypoint->IsLadder())
	{
		//Get off the ladder if there's no waypoints available
		CurrentUserCmd.cmd->buttons |= IN_JUMP;
	}

#ifdef _DEBUG
	if (NextWaypoint)
	{
		std::string str = "Found ";
		if (NextWaypoint->IsLadder())
		{
			if (NextWaypoint->IsLadderBottom())
				str += "Bottom of ladder";
			else if (NextWaypoint->IsLadderStep())
				str += "Step of ladder";
			else if (NextWaypoint->IsLadderTop())
				str += "Top of ladder";
		}
		else
		{
			str += "a random waypoint";
		}
		str += " at ";
		str += std::to_string(NextWaypoint->m_vecOrigin.x);
		str += " ";
		str += std::to_string(NextWaypoint->m_vecOrigin.y);
		str += " ";
		str += std::to_string(NextWaypoint->m_vecOrigin.z);
		str += "\n";
		printf(str.c_str());
	}
	else
	{
		printf("No waypoint found!\n");
	}
#endif

	m_WaypointsMutex.unlock();

	return NextWaypoint;
}

void ResetAllWaypointTemporaryFlags()
{
	m_WaypointsMutex.lock();

	for (auto& waypoint : m_Waypoints)
	{
		waypoint->ResetTemporaryFlags();
	}

	m_WaypointsMutex.unlock();
}