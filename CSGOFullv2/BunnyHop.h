#pragma once
#include "CreateMove.h"
void Bunnyhop(int& buttons, int flags);
bool CStrafe(int& buttons, CUserCmd* cmd, float airaccfloat, float maxspeedfloat, double &AngleAdd, Vector &movements);
bool AutoStrafe(int& buttons, CUserCmd* cmd, float airaccfloat, float maxspeedfloat, QAngle &StrafeAngles, Vector &movements);
void RunBunnyHopping(CUserCmd *cmd, int flags, bool& StrafeModifiedAngles, QAngle& StrafeAngles);