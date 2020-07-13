#pragma once
#include "CSGO_HX.h"
//credits: https://github.com/Snorflake/WideView/blob/81448566df51a1a75c163909ff8f67e00c65478d/WideView/WideView/CViewSetup.h
struct CViewSetup
{
	char _0x0000[16];
	__int32 width;
	__int32    width_old;
	__int32 height;
	__int32    height_old;
	__int32 x;
	__int32 x_old;
	__int32 y;
	__int32 y_old;
	char _0x0030[128];
	float fov;
	float fovViewmodel;
	Vector origin;
	QAngle angles; //Vector
	float zNear;
	float zFar;
	float zNearViewmodel;
	float zFarViewmodel;
	float m_flAspectRatio;
	float m_flNearBlurDepth;
	float m_flNearFocusDepth;
	float m_flFarFocusDepth;
	float m_flFarBlurDepth;
	float m_flNearBlurRadius;
	float m_flFarBlurRadius;
	float m_nDoFQuality;
	__int32 m_nMotionBlurMode;
	char _0x0104[68];
	__int32 m_EdgeBlur;
};