#include "precompiled.h"
#include "BaseCombatWeapon.h"
#include "Overlay.h"
#include "Events.h"
#include "LocalPlayer.h"

#ifdef GENERATE_LEGIT_RCS_SCRIPT
std::vector<PunchStruct>punchangles;
static bool done = true;
#endif

#ifdef SPARK_SCRIPT_TESTING
#include "Aimbot.h"
void SparkDebugNoRecoil()
{
	LocalPlayer.TargetEyeAngles = LocalPlayer.CurrentEyeAngles;
	static int lastshotsfired = 1;
	int shots = LocalPlayer.Entity->GetShotsFired();
	static float lastrecoilx = 0.0f;
	static float lastrecoily = 0.0f;
	//if (shots != lastshotsfired)
	{

		static QAngle lastpunch = angZero;

		QAngle punch = LocalPlayer.Entity->GetPunch();
		int fov = GetFOV(LocalPlayer.Entity);
		if (fov == 0)
			fov = 90;
		int dx = ((punch.x - lastpunch.x) * 2.0f) * -((1920.0f / fov * 1.0f));
		int dy = (((punch.y - lastpunch.y) * 2.0f)) * (1920.0f / fov * 1.0f);

		QAngle dt = (punch - lastpunch) * 2.0f;
		QAngle nopunch = LocalPlayer.TargetEyeAngles - LocalPlayer.Entity->GetPunch() * 2.0f;
		//dt = nopunch - LocalPlayer.TargetEyeAngles;
		static QAngle lasttargetangles = LocalPlayer.LastEyeAngles;
		QAngle targetangles = nopunch;
		static Vector lastaimpixels = AnglesToPixels(lasttargetangles, targetangles, 2.0f, 0.022f, 0.022f);;

		Vector AimPixels = AnglesToPixels(lasttargetangles, targetangles, 2.0f, 0.022f, 0.022f);

		lasttargetangles = targetangles;

		AimPixels.x = round(AimPixels.x / 2.0f) * 2.0f;
		AimPixels.y = round(AimPixels.y / 2.0f) * 2.0f;

		QAngle delta_angle = PixelsDeltaToAnglesDelta(AimPixels, 2.0f, 0.022f, 0.0222f);

		int pixelsx = AimPixels.x - lastaimpixels.x;
		int pixelsy = AimPixels.y - lastaimpixels.y;

		float x = -(dt.x / (0.022f * 2.0f)); //m_pitch //sensitivity
		float y = (dt.y / (0.022f * 2.0f)); //m_yaw //sensitivity

		float dy2 = (float)(CenterOfScreen.y / 90.0f);
		float dx2 = (float)(CenterOfScreen.x / 90.0f);
		float recoilx = (dx2 * punch.y * 0.5f);
		float recoily = (dy2 * punch.x);


		RecoilCrosshairScreen.x = CenterOfScreen.x - recoilx;
		RecoilCrosshairScreen.y = CenterOfScreen.y + recoily;


		//mouse_event(MOUSEEVENTF_MOVE, y, x, 0, 0);
		//mouse_event(MOUSEEVENTF_MOVE, (int)((recoilx - lastrecoilx) * 7.5f), (int)(-((recoily - lastrecoily) * 6.5f)), 0, 0);


		int dx3 = ((punch.x - lastpunch.x) * 1.925f) * -((1920 / fov * 1.0f));
		int dy3 = (((punch.y - lastpunch.y) * 1.925f)) * (1920 / fov * 1.0f);
		mouse_event(MOUSEEVENTF_MOVE, dy3, dx3, 0, 0);

		lastrecoilx = recoilx;
		lastrecoily = recoily;

		//mouse_event(MOUSEEVENTF_MOVE, dy, dx, 0, 0);
		lastaimpixels = AimPixels;
		lastpunch = LocalPlayer.Entity->GetPunch();
	}
	lastshotsfired = shots;

	if (shots == 0)
	{
		//lastrecoilx = 0;
		//lastrecoily = 0;
	}
}
#endif