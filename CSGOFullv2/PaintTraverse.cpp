#include "precompiled.h"

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Adriel/ImGUI/imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "Adriel/ImGUI/imgui_internal.h"

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

#include "VTHook.h"
#include "Interfaces.h"
#include "Draw.h"
#include "GlobalInfo.h"
#include "ServerSounds.h"
#include "Spycam.h"
#include "GiftWrapAlgorithm.h"

#include "Adriel/renderer.hpp"
#include "Adriel/nade_prediction.hpp"
#include "Adriel/adr_util.hpp"

PaintTraverseFn oPaintTraverse;

void CParticleCollection::RenderChildren(const ColorRGBA& color)
{
	for (auto child = GetChildren(); child; child = child->GetNext())
	{
		if (child->ToNewParticleEffect()->IsBoundsValid())
		{
			Vector mins = child->ToNewParticleEffect()->GetMinBounds();
			Vector maxs = child->ToNewParticleEffect()->GetMaxBounds();
			Vector origin = child->GetOrigin();

			//Bounds are already in world space so origin could just be 0,0,0 and use bounds instead of this math
			Interfaces::DebugOverlay->AddBoxOverlay(origin, maxs - origin, mins - origin, angZero, 255, 255, 255, 25, TICKS_TO_TIME(10));
		}

		child->RenderChildren(color);
	}
}

struct ImVecPolygon
{
	ImVec2 vertices[3];

	ImVecPolygon() {};

	ImVecPolygon(ImVec2& first, ImVec2& second, ImVec2& third)
	{
		vertices[0] = first;
		vertices[1] = second;
		vertices[2] = third;
	}
	int operator==(const ImVecPolygon& other) const
	{
		for (int i = 0; i < 3; ++i)
		{
			if (other.vertices[i].x != vertices[i].x || other.vertices[i].y != vertices[i].y)
				return false;
		}
		return true;
	}
	ImVecPolygon& operator=(const ImVecPolygon &vOther)
	{
		for (int i = 0; i < 3; ++i)
		{
			vertices[i].x = vOther.vertices[i].x;
			vertices[i].y = vOther.vertices[i].y;
		}
		return *this;
	}
};

float GetImVec2Length(const ImVec2& src)
{
	float root = 0.0f;

	float sqsr = src.x * src.x + src.y * src.y;

	__asm sqrtss xmm0, sqsr
	__asm movss root, xmm0

	return root;
};

float GetImVec2Dist(const ImVec2& first, const ImVec2& second)
{
	const ImVec2 delta = { first.x - second.x, first.y - second.y };

	return GetImVec2Length(delta);
};

std::vector<ImVec2>::const_iterator GetNextClosestPoint(ImVec2& src, const std::vector<ImVec2>&list, const std::vector<ImVec2>*excludelist = nullptr)
{
	std::vector<ImVec2>::const_iterator best = list.end();
	float closest_dist;
	for (size_t i = 0; i < list.size(); ++i)
	{
		std::vector<ImVec2>::const_iterator cur = list.begin() + i;

		//always exclude self
		if (src.x == cur->x && src.y == cur->y)
			continue;

		//a list of points to exclude from scanning
		if (excludelist != nullptr && !excludelist->empty())
		{
			if (std::find(excludelist->begin(), excludelist->end(), *cur) != excludelist->end())
				continue;
		}

		float dist = GetImVec2Dist(src, *cur);
		if (best == list.end() || dist < closest_dist)
		{
			closest_dist = dist;
			best = cur;
		}
	}
	return best;
}

//todo: just rewrite this instead of pasting later
//https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle

bool PointInTriangle(const ImVec2& p, const ImVec2& p0, const ImVec2& p1, const ImVec2& p2)
{
	auto s = p0.y * p2.x - p0.x * p2.y + (p2.y - p0.y) * p.x + (p0.x - p2.x) * p.y;
	auto t = p0.x * p1.y - p0.y * p1.x + (p0.y - p1.y) * p.x + (p1.x - p0.x) * p.y;

	if ((s < 0) != (t < 0))
		return false;

	auto A = -p1.y * p2.x + p0.y * (p2.x - p1.x) + p0.x * (p1.y - p2.y) + p1.x * p2.y;

	return A < 0 ?
		(s <= 0 && s + t >= A) :
		(s >= 0 && s + t <= A);
}

/* Check whether P and Q lie on the same side of line AB */
float Side(const ImVec2& p, const ImVec2& q, const ImVec2& a, const ImVec2& b)
{
	float z1 = (b.x - a.x) * (p.y - a.y) - (p.x - a.x) * (b.y - a.y);
	float z2 = (b.x - a.x) * (q.y - a.y) - (q.x - a.x) * (b.y - a.y);
	return z1 * z2;
}

enum IntersectResult_t
{
	NOT_INTERSECTING = 0,
	OVERLAPPING,
	TOUCHING,
	INTERSECTING
};


//https://gamedev.stackexchange.com/questions/21096/what-is-an-efficient-2d-line-segment-versus-triangle-intersection-test

/* Check whether segment P0P1 intersects with triangle t0t1t2 */
IntersectResult_t Intersecting(const ImVec2& p0, const ImVec2& p1, const ImVec2& t0, const ImVec2& t1, const ImVec2& t2)
{
	/* Check whether segment is outside one of the three half-planes
	 * delimited by the triangle. */
	float f1 = Side(p0, t2, t0, t1), f2 = Side(p1, t2, t0, t1);
	float f3 = Side(p0, t0, t1, t2), f4 = Side(p1, t0, t1, t2);
	float f5 = Side(p0, t1, t2, t0), f6 = Side(p1, t1, t2, t0);
	/* Check whether triangle is totally inside one of the two half-planes
	 * delimited by the segment. */
	float f7 = Side(t0, t1, p0, p1);
	float f8 = Side(t1, t2, p0, p1);

	/* If segment is strictly outside triangle, or triangle is strictly
	 * apart from the line, we're not intersecting */
	if ((f1 < 0 && f2 < 0) || (f3 < 0 && f4 < 0) || (f5 < 0 && f6 < 0)
		|| (f7 > 0 && f8 > 0))
		return NOT_INTERSECTING;

	/* If segment is aligned with one of the edges, we're overlapping */
	if ((f1 == 0 && f2 == 0) || (f3 == 0 && f4 == 0) || (f5 == 0 && f6 == 0))
		return OVERLAPPING;

	/* If segment is outside but not strictly, or triangle is apart but
	 * not strictly, we're touching */
	if ((f1 <= 0 && f2 <= 0) || (f3 <= 0 && f4 <= 0) || (f5 <= 0 && f6 <= 0)
		|| (f7 >= 0 && f8 >= 0))
		return TOUCHING;

	/* If both segment points are strictly inside the triangle, we
	 * are not intersecting either */
	if (f1 > 0 && f2 > 0 && f3 > 0 && f4 > 0 && f5 > 0 && f6 > 0)
		return NOT_INTERSECTING;

	/* Otherwise we're intersecting with at least one edge */
	return INTERSECTING;
}


//Author: Olivier renault
//Polycolly
IntersectResult_t ClipSegment(const ImVec2* pPolygon, const ImVec2& rayStart, const ImVec2& rayEnd, ImVec2& Nnear, ImVec2& Nfar, int numVertices = 3, float tnear = 0.0f, float tfar = 1.0f)
{
#if 1
	return Intersecting(rayStart, rayEnd, pPolygon[0], pPolygon[1], pPolygon[1]);
#else
#endif
}

//returns true if we intersected a polygon
bool ClipPolygons(ImVec2& startpos, const ImVec2& endpos, std::vector<ImVecPolygon>&polygons)
{
	for (auto& polygon : polygons)
	{
		ImVec2 nNear, nFar;

		IntersectResult_t result = Intersecting(startpos, endpos, polygon.vertices[0], polygon.vertices[1], polygon.vertices[2]);

		if (result == NOT_INTERSECTING)
		{
			continue;
		}
		else if (result == OVERLAPPING)
		{
			bool foundStart = false, foundEnd = false;

			for (int i = 0; i < 3; ++i)
			{
				if (polygon.vertices[i] == startpos)
					foundStart = true;
				else if (polygon.vertices[i] == endpos)
					foundEnd = true;
			}

			if (foundStart)
			{
				continue;
			}
			else
			{		
				//this point intersects with an existing polygon

				//find out how many polygons contain two points the same as the start and endpos
				int numMatchingPolygons = 0;

				for (auto& p : polygons)
				{
					bool matchedstartpos = false, matchedendpos = false;
					for (int i = 0; i < 3; ++i)
					{
						if (!matchedstartpos && p.vertices[i] == startpos)
							matchedstartpos = true;
						if (!matchedendpos && p.vertices[i] == endpos)
							matchedendpos = true;
					}
					if (matchedstartpos && matchedendpos && ++numMatchingPolygons >= 2)
						break;
				}

				//if there are already 2 polygons sharing the same two points, then we can't use it again
				if (numMatchingPolygons >= 2)
					return true;
			}
		}
	}

	return false;
}

//excluded is points that we don't want to return
std::vector<ImVec2>::const_iterator FindClosestNonIntersectingPoint(ImVec2& startpos, std::vector<ImVec2> &excluded, std::vector<ImVec2>&points, std::vector<ImVecPolygon>&polygons)
{
	for (;;)
	{
		auto closest = GetNextClosestPoint(startpos, points, &excluded);
		if (closest == points.end())
			break;

		excluded.push_back(*closest);

		if (!ClipPolygons(startpos, *closest, polygons))
			return closest;
	}

	return points.end();
}

void __fastcall Hooks::PaintTraverse(void* thisptr, void* edx, unsigned int panel, bool forceRepaint, bool allowForce)
{
	static bool initialized = false;
	if (!initialized)
	{
#ifdef PROFILE
		SetupFonts();
#endif
		g_Visuals.FindMaterials();
		initialized = true;
	}

	static uint32_t draw_panel;
	if ( !draw_panel )
	{
		//decrypts(0)
		if ( !strcmp( Interfaces::VPanel->GetName(panel), XorStr("MatSystemTopPanel")))
			draw_panel = panel;
		//encrypts(0)
	}

	// remove the scope-overlay
	if (Interfaces::EngineClient->IsInGame())
	{
		if (variable::get().visuals.b_no_scope)
		{
			if (!strcmp(Interfaces::VPanel->GetName(panel), XorStrCT("HudZoom")))
			{
				// disable vignette while scoped
				*s_bOverridePostProcessingDisable = true;
				return;
			}
		}
		else
		{
			*s_bOverridePostProcessingDisable = false;
		}
	}

	oPaintTraverse(thisptr, panel, forceRepaint, allowForce);

	if ( panel != draw_panel )
		return;

	if ( !variable::get().ui.b_init || variable::get().global.b_reseting )
		return;

	DRAW_PROFILED_FUNCTIONS();

	static auto b_skip = false;
	b_skip = !b_skip;

	if ( b_skip )
		return;

	//decrypts(0)
	START_PROFILING_CUSTOM(adr_render, XorStr("AdrielRendering"))
	//encrypts(0)

	// begin rendering
	render::get().new_frame();
	{
		if (variable::get().misc.b_water_mark)
		{
		char watermark_buf[48];

		//decrypts(0)
		#if defined _DEBUG || defined INTERNAL_DEBUG
				sprintf_s<48>(watermark_buf, XorStr("MUTINY.PW - DEBUG %c %s"), XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3], XorStrCT(__DATE__));
		#elif defined STAFF
				sprintf_s<48>(watermark_buf, XorStr("MUTINY.PW - INSIDER %c %s"), XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3], XorStrCT(__DATE__));
		#elif defined TEST_BUILD
				sprintf_s<48>(watermark_buf, XorStr("MUTINY.PW - TEST %c %s"), XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3], XorStrCT(__DATE__));
		#else
				sprintf_s<48>(watermark_buf, XorStr("MUTINY.PW - BETA %c %s"), XorStr("|/-\\")[static_cast<int>(ImGui::GetTime() / 0.25f) & 3], XorStrCT(__DATE__));
		#endif
			//encrypts(0)
			render::get().add_text(ImVec2(render::get().get_viewport().Width - 5.f, 0.f), Color::White().ToImGUI(), RIGHT_ALIGN, BAKSHEESH_14, watermark_buf);
		}
		// only in game
		if (Interfaces::EngineClient->IsInGame() && Interfaces::EngineClient->IsConnected())
		{
			LocalPlayer.NetvarMutex.Lock();
#ifdef OLD_RENDERER
			g_Visuals.DrawIndicators();
#endif
			// handle all ESP/Visuals logic

			if (variable::get().visuals.b_enabled)
			{
//molotov esp removed


				g_Visuals.DrawESP();

				g_Visuals.DrawIndicators();

				g_Visuals.DrawScopeLines();

				g_Visuals.DrawSpreadCircle();

				g_Visuals.DrawHitmarker();

				g_Visuals.DrawAutowallCrosshair();

				g_Visuals.DrawAntiAim();

				nade_prediction::get().render();

				g_ServerSounds.Start();

				g_ServerSounds.Finish();
			}
			LocalPlayer.NetvarMutex.Unlock();
		}
	}
	// end rendering
	render::get().swap_data();

	END_PROFILING_CUSTOM(adr_render)
}
