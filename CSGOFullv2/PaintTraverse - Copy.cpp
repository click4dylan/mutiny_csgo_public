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

bool PointInTriangle(ImVec2& p, ImVec2& p0, ImVec2& p1, ImVec2& p2)
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


//Author: Olivier renault
//Polycolly
bool ClipSegment(const ImVec2* pPolygon, const ImVec2& rayStart, const ImVec2& rayEnd, ImVec2& Nnear, ImVec2& Nfar, int numVertices = 3, float tnear = 0.0f, float tfar = 1.0f)
{
	if (!pPolygon)
		return false;

	ImVec2 &end = (ImVec2&)rayEnd;
	ImVec2 one = pPolygon[0], two = pPolygon[2], three = pPolygon[3];
	if (PointInTriangle((ImVec2&)end, one, two, three))
	{
		return true;
	}

	ImVec2 xDir = rayEnd - rayStart;

	//test separation axes of pPolygon
	for (int j = numVertices - 1, i = 0; i < numVertices; j = i, i++)
	{
		ImVec2 E0 = pPolygon[j];
		ImVec2 E1 = pPolygon[i];
		ImVec2 E = E1 - E0;
		ImVec2 En(E.y, -E.x);
		ImVec2 D = E0 - rayStart;
		float denom2 = D.Dot(En);
		float denom = D.Dot(En);
		float numer = xDir.Dot(En);

		//ray parallel to plane
		if (fabsf(numer) < 1.0E-8f)
		{
			// origin outside the plane, no intersection
			if (denom < 0.0f)
				return false;
		}
		else
		{
			float tclip = denom / numer;
			//near intersection
			if (numer < 0.0f)
			{
				if (tclip > tnear)
				{
					tnear = tclip;
					Nnear = En;
					Nnear.Normalise();
				}
			}
			// far intersection
			else
			{
				if (tclip < tnear)
					return false;
				if (tclip < tfar)
				{
					tfar = tclip;
					Nfar = En;
					Nfar.Normalise();
				}
			}
		}
	}

	return true;
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
				if (!g_Infernos.empty())
				{
					for (auto& inferno : g_Infernos)
					{
						if (inferno->IsFireBurning())
						{
							auto prop = inferno->ParticleProp();
							for (int i = 0; i < prop->m_ParticleEffects.Count(); ++i)
							{
								ParticleEffectList_t *eff = &prop->m_ParticleEffects[i];
								if (eff->pParticleEffect)
								{
									CNewParticleEffect *particle = eff->pParticleEffect;
									CParticleCollection* baseclass = particle->ToParticleCollection();

									if (baseclass->GetNumControlPointsAllocated())
									{
										std::vector<Vector> positions;
										for (int p = 0; p <= baseclass->GetHighestControlPoint(); ++p)
										{
											//these are all the unsorted points for the inferno
											//todo: figure out a way to sort these in a way we can render lines or polygons
											//in an efficient manner
											
											positions.push_back(baseclass->GetPosition(p));
										}

										//std::sort(positions.begin(), positions.end(), [particle](Vector& a, Vector& b) {return a.Dist(particle->GetRenderOrigin()) > b.Dist(particle->GetRenderOrigin()); });

										// 1. make these positions in screen space
										std::vector<ImVec2> screenpoints;
										std::vector<std::vector<ImVec2>*> list_of_sorted_screenpoints;


										for (int p = 0; p < positions.size(); ++p)
										{
											Vector scn;
											if (WorldToScreen(positions[p], scn))
												screenpoints.push_back({ scn.x, scn.y });
										}

										//Recursively create a ring of points furthest from the center of the inferno, excluding the previously created ring
										while (screenpoints.size() >= 3)
										{
											std::vector<ImVec2> *sorted_points = new std::vector<ImVec2>;
											convexHull(*sorted_points, screenpoints);

											if (!sorted_points->empty())
											{
												list_of_sorted_screenpoints.push_back(sorted_points);
												for (auto& point : *sorted_points)
												{
													auto matching_point = std::find_if(screenpoints.begin(), screenpoints.end(), [&point](const ImVec2& x) { return x.x == point.x && x.y == point.y; });
													if (matching_point != screenpoints.end())
														screenpoints.erase(matching_point);
												}
											}
											else
											{
												delete sorted_points;
												break;
											}
										}

										SmallForwardBuffer buf;

										for (int p = 0; p < list_of_sorted_screenpoints.size(); ++p)
										{
											Color clr;
											if (p == 0)
												clr = Color::Red();
											else if (p == 1)
												clr = Color::Green();
											else if (p == 2)
												clr = Color::Blue();
											else if (p == 3)
												clr = Color::Yellow();
											else if (p == 4)
												clr = Color::Orange();
											else
												clr = Color::White();

											clr.SetAlpha(255);

											for (auto& a : *list_of_sorted_screenpoints[p])
											{
												buf.write<ImVec2>(a);

												render::get().add_circle(a, 5.0f, clr.ToImGUI());
												render::get().add_text(a, clr.ToImGUI(), NO_TFLAG, BAKSHEESH_14, "%i", p);
											}
										}

										std::vector<ImVecPolygon> polygons;

#if 1
										if (list_of_sorted_screenpoints.size() > 1)
										{
											std::vector<ImVec2> allpoints;
											for (auto& list : list_of_sorted_screenpoints)
											{
												for (auto& point : *list)
													allpoints.push_back(point);
											}


											for (auto& point : allpoints)
											{
												std::vector<ImVec2> excluded;
												bool failed = false;

												ImVecPolygon newpolygon;
												newpolygon.vertices[0] = point;
												int numvertices = 1;

												//Find two more points that don't intersect another polygon that are closest to this point
#if 0
												for (;;)
												{
													auto closest = GetNextClosestPoint(point, allpoints, &excluded);
													if (closest != allpoints.end())
													{
														excluded.push_back(*closest);

														for (auto& polygon : polygons)
														{
															ImVec2 nNear, nFar;
															if (ClipSegment(polygon.vertices, point, *closest, nNear, nFar, 3, 0.0f, 1.0f))
															{
																//this point intersects with an existing polygon, find a new point
																failed = true;
																break;
															}
														}
													}
													else
													{
														failed = true;
														break;
													}

													if (!failed)
													{
														//found a non intersecting point, add it to the new polygon
														newpolygon.vertices[numvertices++] = *closest;
														break;
													}

													failed = false;
												}

												if (!failed)
												{
													//find the second nearest point that doesn't intersect an existing polygon
													excluded.clear();
													excluded.push_back(newpolygon.vertices[1]);

													for (;;)
													{
														auto closest = GetNextClosestPoint(point, allpoints, &excluded);
														if (closest != allpoints.end())
														{
															excluded.push_back(*closest);

															for (auto& polygon : polygons)
															{
																ImVec2 nNear, nFar;
																if (ClipSegment(polygon.vertices, point, *closest, nNear, nFar, 3, 0.0f, 1.0f))
																{
																	//this point intersects with an existing polygon, find a new point
																	failed = true;
																	break;
																}
															}
														}
														else
														{
															failed = true;
															break;
														}

														if (!failed)
														{
															//found a non intersecting point, add it to the new polygon
															newpolygon.vertices[numvertices++] = *closest;
															break;
														}

														failed = false;
													}
												}
#else
												for (int j = 1; j < 3; ++j)
												{
													failed = false;

													for (;;)
													{
														auto closest = GetNextClosestPoint(point, allpoints, &excluded);
														if (closest != allpoints.end())
														{
															excluded.push_back(*closest);
															for (auto& polygon : polygons)
															{
																ImVec2 nNear, nFar;
																if (ClipSegment(polygon.vertices, point, *closest, nNear, nFar, 3, 0.0f, 1.0f))
																{
																	//this point intersects with an existing polygon, find a new point
																	failed = true;
																	break;
																}
															}

															if (!failed)
															{
																//found a non intersecting point, add it to the new polygon
																newpolygon.vertices[numvertices++] = *closest;
																break;
															}

															failed = false;
														}
														else
														{
															failed = true;
															break;
														}
													}

													if (failed)
														break;
												}
#endif

												if (numvertices == 3)
												{
													polygons.push_back(newpolygon);
												}
											}

											for (auto& poly : polygons)
											{
												render::get().add_triangle_filled(poly.vertices[0], poly.vertices[1], poly.vertices[2], Color::Red().ToImGUI());
											}

											for (auto& poly : polygons)
											{
												render::get().add_triangle(poly.vertices[0], poly.vertices[1], poly.vertices[2], Color::LightBlue().ToImGUI(), 5.0f);
											}
										}
#else
										if (list_of_sorted_screenpoints.size() > 1)
										{
											for (int p = 0; p < list_of_sorted_screenpoints.size() - 1; ++p)
											{
												std::vector<ImVec2> &outer = *list_of_sorted_screenpoints[p];
												std::vector<ImVec2> &inner = *list_of_sorted_screenpoints[p + 1];

												ImVecPolygon polygon;

												for (auto& point : outer)
												{
													std::vector<ImVec2> excluded_points;

													int generated_points = 0;

													for (int points = 0; points < 3; ++points)
													{
														auto closest_outer_point = GetNextClosestPoint(point, outer, &excluded_points);
														auto closest_inner_point = GetNextClosestPoint(point, inner, &excluded_points);

														if (closest_outer_point != outer.end())
														{
															if (closest_inner_point != inner.end())
															{
																if (GetImVec2Dist(*closest_inner_point, point) < GetImVec2Dist(*closest_outer_point, point))
																	polygon.vertices[points] = *closest_inner_point;
																else
																	polygon.vertices[points] = *closest_outer_point;

																excluded_points.push_back(polygon.vertices[points]);
																++generated_points;

															}
															else
															{
																polygon.vertices[points] = *closest_outer_point;
																excluded_points.push_back(polygon.vertices[points]);
																++generated_points;
															}
														}
														else if (closest_inner_point != inner.end())
														{
															if (closest_outer_point != outer.end())
															{
																if (GetImVec2Dist(*closest_outer_point, point) < GetImVec2Dist(*closest_inner_point, point))
																	polygon.vertices[points] = *closest_outer_point;
																else
																	polygon.vertices[points] = *closest_inner_point;
																excluded_points.push_back(polygon.vertices[points]);
																++generated_points;
															}
															else
															{
																polygon.vertices[points] = *closest_inner_point;
																excluded_points.push_back(polygon.vertices[points]);
																++generated_points;
															}
														}
													}

													if (generated_points == 3)
													{
														polygons.push_back(polygon);
													}
												}
											}

											//now do center outwards
											Vector centerscn;
											if (0 && WorldToScreen(particle->GetRenderOrigin(), centerscn))
											{
												ImVec2 centerpoint = { centerpoint.x, centerpoint.y };

												render::get().add_circle(centerpoint, 10.0f, Color::Green().ToImGUI(), 12, 5.0f);
												Interfaces::DebugOverlay->AddBoxOverlay(particle->GetRenderOrigin(), Vector(-5, -5, -5), Vector(5, 5, 5), angZero, 0, 255, 0, 255, TICKS_TO_TIME(2));
												ImVecPolygon polygon;
												std::vector<ImVec2> excluded_points;

												int generated_points = 0;

												for (int points = 0; points < 3; ++points)
												{
													auto closest_point = GetNextClosestPoint(centerpoint, *list_of_sorted_screenpoints.back(), &excluded_points);
													if (closest_point != list_of_sorted_screenpoints.back()->end())
													{
														polygon.vertices[points] = *closest_point;
														excluded_points.push_back(polygon.vertices[points]);
														++generated_points;
													}
												}

												if (generated_points == 3)
												{
													polygons.push_back(polygon);
												}
											}
										}
										else
										{
											//just render polygons
										}

										for (auto& poly : polygons)
										{
											render::get().add_triangle_filled(poly.vertices[0], poly.vertices[1], poly.vertices[2], Color::Red().ToImGUI());
										}

										for (auto& poly : polygons)
										{
											render::get().add_triangle(poly.vertices[0], poly.vertices[1], poly.vertices[2], Color::LightBlue().ToImGUI(), 5.0f);
										}

#endif


										//render::get().add_polyline(screenpoints_sorted.data(), screenpoints_sorted.size(), Color(255, 0, 0, 200).ToImGUI(), true, 5.f);
										//if (buf.getsize())
										//{
										//	std::vector<ImVec2> &bleh = *list_of_sorted_screenpoints[0];
										//	if (bleh.size() > 0)
										//		render::get().add_convex_poly_filled(bleh.data(), bleh.size() /*buf.getdata<const ImVec2>(), buf.getcount<const ImVec2>()*/, Color(255, 0, 0, 100).ToImGUI());
										//}

										for (auto& sorted_screenpoints : list_of_sorted_screenpoints)
										{
											delete sorted_screenpoints;
										}

										//renders the bounding box of all the child particles
										//baseclass->RenderChildren(ColorRGBA(0, 255, 255, 75));
									}
								}
							}
						}
					}
				}


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
