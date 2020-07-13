#include "framework.h"
#include "C:/ClientFramework/Framework/Menu/Fonts.h"
#include "C:\ClientFramework\Framework\ImGUI\imgui.h"
#include "C:\ClientFramework\Framework\ImGUI\imgui_internal.h"

#include "visuals.h"
#include "localPlayer.h"
#include "trace.h"
#include "AutoWall.h"

Renderer* Renderer::m_pInstance;
void Renderer::Rainbow()
{
	int speed = 3;
	if (state == 0) 
	{
		g+= speed;
		if (g >= 255)
		{
			state = 1;
			g = 255;
		}
	}
	if (state == 1) 
	{
		r -= speed;
		if (r <= 0) 
		{
			state = 2;
			r = 0;
		}
	}
	if (state == 2) 
	{
		b += speed;
		if (b >= 255)
		{
			state = 3;
			b = 255;
		}
	}
	if (state == 3)
	{
		g -= speed;
		if (g <= 0)
		{
			state = 4;
			g = 0;
		}
	}
	if (state == 4) 
	{
		r += speed;
		if (r >= 255)
		{
			state = 5;
			r = 255;
		}
	}
	if (state == 5) {
		b-= speed;
		if (b <= 0)
		{
			state = 0;
			b = 0;
		}
	}
	counter+= speed;
	healthb = abs(sin(counter) * 255);
}

void Renderer::SkeletonEsp(skeleton SkeletonPoints, ImColor color)
{
	rDrawLine(SkeletonPoints.head.x, SkeletonPoints.head.y, SkeletonPoints.neck.x, SkeletonPoints.neck.y, color);
	rDrawLine(SkeletonPoints.neck.x, SkeletonPoints.neck.y, SkeletonPoints.chest.x, SkeletonPoints.neck.y, color);
	rDrawLine(SkeletonPoints.chest.x, SkeletonPoints.chest.y, SkeletonPoints.leftShoulder.x, SkeletonPoints.leftShoulder.y, color);
	rDrawLine(SkeletonPoints.chest.x, SkeletonPoints.chest.y, SkeletonPoints.rightShoulder.x, SkeletonPoints.rightShoulder.y, color);
	rDrawLine(SkeletonPoints.leftShoulder.x, SkeletonPoints.leftShoulder.y, SkeletonPoints.leftElbow.x, SkeletonPoints.leftElbow.y, color);
	rDrawLine(SkeletonPoints.rightShoulder.x, SkeletonPoints.rightShoulder.y, SkeletonPoints.rightElbow.x, SkeletonPoints.rightElbow.y, color);
	rDrawLine(SkeletonPoints.leftElbow.x, SkeletonPoints.leftElbow.y, SkeletonPoints.leftHand.x, SkeletonPoints.leftHand.y, color);
	rDrawLine(SkeletonPoints.rightElbow.x, SkeletonPoints.rightElbow.y, SkeletonPoints.rightHand.x, SkeletonPoints.rightHand.y, color);
	rDrawLine(SkeletonPoints.chest.x, SkeletonPoints.chest.y, SkeletonPoints.pelvis.x, SkeletonPoints.pelvis.y, color);
	rDrawLine(SkeletonPoints.pelvis.x, SkeletonPoints.pelvis.y, SkeletonPoints.leftKnee.x, SkeletonPoints.leftKnee.y, color);
	rDrawLine(SkeletonPoints.pelvis.x, SkeletonPoints.pelvis.y, SkeletonPoints.rightKnee.x, SkeletonPoints.rightKnee.y, color);
	rDrawLine(SkeletonPoints.leftKnee.x, SkeletonPoints.leftKnee.y, SkeletonPoints.leftFoot.x, SkeletonPoints.leftFoot.y, color);
	rDrawLine(SkeletonPoints.rightKnee.x, SkeletonPoints.rightKnee.y, SkeletonPoints.rightFoot.x, SkeletonPoints.rightFoot.y, color);
}

void Renderer::SkeltonEsp(CustomPlayer* player)
{
	Vector W2S;
	skeleton data;

	float time = Interfaces::Globals->curtime;
	for (int i = 0; i < 1; i++)
	{

		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_HEAD, Interfaces::Globals->curtime, false, false), W2S) == TRUE)
			data.head = Vector2D(W2S.x, W2S.y);
		else return;

		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_NECK, time, false, false), W2S) == TRUE)
			data.neck = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_CHEST, time, false, false), W2S) == TRUE)
			data.chest = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_PELVIS, time, false, false), W2S) == TRUE)
			data.pelvis = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_LEFT_UPPER_ARM, time, false, false), W2S) == TRUE)
			data.leftShoulder = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_RIGHT_UPPER_ARM, time, false, false), W2S) == TRUE)
			data.rightShoulder = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_LEFT_FOREARM, time, false, false), W2S) == TRUE)
			data.leftElbow = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_RIGHT_FOREARM, time, false, false), W2S) == TRUE)
			data.rightElbow = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_LEFT_HAND, time, false, false), W2S) == TRUE)
			data.leftHand = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_RIGHT_HAND, time, false, false), W2S) == TRUE)
			data.rightHand = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_LEFT_CALF, time, false, false), W2S) == TRUE)
			data.leftKnee = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_RIGHT_CALF, time, false, false), W2S) == TRUE)
			data.rightKnee = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_LEFT_FOOT, time, false, false), W2S) == TRUE)
			data.leftFoot = Vector2D(W2S.x, W2S.y);
		if (WorldToScreenCapped(player->BaseEntity->GetBonePosition(HITBOX_RIGHT_FOOT, time, false, false), W2S) == TRUE)
			data.rightFoot = Vector2D(W2S.x, W2S.y);

		SkeletonEsp(data, ImColor(0,255,0,255));
		int temptime = TIME_TO_TICKS(Interfaces::Globals->curtime);
		temptime -= i;
		time = TICKS_TO_TIME(temptime);
	}
}


char *cautomaticyawstr2 = new char[14]{ 59, 15, 14, 21, 23, 27, 14, 19, 25, 90, 35, 27, 13, 0 }; /*Automatic Yaw*/
char *cfakespinstr2 = new char[20]{ 60, 27, 17, 31, 90, 41, 22, 21, 13, 85, 60, 27, 9, 14, 90, 41, 10, 19, 20, 0 }; /*Fake Slow/Fast Spin*/
char *clinearfakeyawstr2 = new char[12]{ 54, 19, 20, 31, 27, 8, 90, 60, 27, 17, 31, 0 }; /*Linear Fake*/
char *crandomfakeyawstr2 = new char[12]{ 40, 27, 20, 30, 21, 23, 90, 60, 27, 17, 31, 0 }; /*Random Fake*/
char *cstaticantiresolveyawstr2 = new char[18]{ 59, 20, 14, 19, 87, 40, 31, 9, 21, 22, 12, 31, 90, 60, 27, 17, 31, 0 }; /*Anti-Resolve Fake*/
char *cbodyrealdeltastr2 = new char[16]{ 56, 21, 30, 3, 90, 40, 31, 27, 22, 90, 62, 31, 22, 14, 27, 0 }; /*Body Real Delta*/
char *ceyerealdeltastr2 = new char[15]{ 63, 3, 31, 90, 40, 31, 27, 22, 90, 62, 31, 22, 14, 27, 0 }; /*Eye Real Delta*/
char *clastrealyawstr2 = new char[14]{ 54, 27, 9, 14, 90, 40, 31, 27, 22, 90, 35, 27, 13, 0 }; /*Last Real Yaw*/
char *cattargetstr2 = new char[10]{ 59, 14, 90, 46, 27, 8, 29, 31, 14, 0 }; /*At Target*/
char *cinverseattargetstr2 = new char[18]{ 51, 20, 12, 31, 8, 9, 31, 90, 59, 14, 90, 46, 27, 8, 29, 31, 14, 0 }; /*Inverse At Target*/
char *cavglowerbodydeltastr2 = new char[21]{ 59, 12, 29, 90, 54, 21, 13, 31, 8, 90, 56, 21, 30, 3, 90, 62, 31, 22, 14, 27, 0 }; /*Avg Lower Body Delta*/
char *cstaticdynamicfakestr2 = new char[20]{ 41, 14, 27, 14, 19, 25, 90, 60, 27, 17, 31, 90, 62, 3, 20, 27, 23, 19, 25, 0 }; /*Static Fake Dynamic*/
char *cstaticfakeyawstr2 = new char[12]{ 41, 14, 27, 14, 19, 25, 90, 60, 27, 17, 31, 0 }; /*Static Fake*/
char *cinverseyawstr2 = new char[8]{ 51, 20, 12, 31, 8, 9, 31, 0 }; /*Inverse*/
char *cyawforceback2 = new char[11]{ 60, 21, 8, 25, 31, 90, 56, 27, 25, 17, 0 }; /*Force Back*/
char *cyawleftstr2 = new char[11]{ 60, 21, 8, 25, 31, 90, 54, 31, 28, 14, 0 }; /*Force Left*/
char *cyawrightstr2 = new char[12]{ 60, 21, 8, 25, 31, 90, 40, 19, 29, 18, 14, 0 }; /*Force Right*/
char *cwhaeldongstr = new char[11]{ 45, 18, 27, 22, 31, 90, 62, 21, 20, 29, 0 }; /*Whale Dong*/
char *cBackTrackFirestr = new char[14]{ 56, 27, 25, 17, 46, 8, 27, 25, 17, 60, 19, 8, 31, 0 }; /*BackTrackFire*/
char *cBackTrackHitstr = new char[13]{ 56, 27, 25, 17, 46, 8, 27, 25, 17, 50, 19, 14, 0 }; /*BackTrackHit*/
char *cBackTrackRealstr = new char[14]{ 56, 27, 25, 17, 46, 8, 27, 25, 17, 40, 31, 27, 22, 0 }; /*BackTrackReal*/
char *cBackTrackLbystr = new char[13]{ 56, 27, 25, 17, 46, 8, 27, 25, 17, 54, 24, 3, 0 }; /*BackTrackLby*/
char *cBackTrackUpstr = new char[12]{ 56, 27, 25, 17, 46, 8, 27, 25, 17, 47, 10, 0 }; /*BackTrackUp*/
char *cNotBackTrackedstr = new char[15]{ 52, 21, 14, 56, 27, 25, 17, 46, 8, 27, 25, 17, 31, 30, 0 }; /*NotBackTracked*/
char *cWD_StaticFakestr = new char[14]{ 45, 62, 37, 41, 14, 27, 14, 19, 25, 60, 27, 17, 31, 0 }; /*WD_StaticFake*/
char *cWD_Velocitystr = new char[12]{ 45, 62, 37, 44, 31, 22, 21, 25, 19, 14, 3, 0 }; /*WD_Velocity*/
char *cWD_Targetstr = new char[10]{ 45, 62, 37, 46, 27, 8, 29, 31, 14, 0 }; /*WD_Target*/
char *cWD_Spinstr = new char[8]{ 45, 62, 37, 41, 10, 19, 20, 0 }; /*WD_Spin*/
char *cWD_Jitterstr = new char[10]{ 45, 62, 37, 48, 19, 14, 14, 31, 8, 0 }; /*WD_Jitter*/
char *cWD_Staticstr = new char[10]{ 45, 62, 37, 41, 14, 27, 14, 19, 25, 0 }; /*WD_Static*/
char *cWD_FuckItstr = new char[10]{ 45, 62, 37, 60, 15, 25, 17, 51, 14, 0 }; /*WD_FuckIt*/

char *cnonestr = new char[5]{ 52, 21, 20, 31, 0 }; /*None*/
char *cotherstr2 = new char[6]{ 53, 14, 18, 31, 8, 0 }; /*Other*/
char *cbacktrackedstr = new char[12]{ 56, 27, 25, 17, 14, 8, 27, 25, 17, 31, 30, 0 }; /*Backtracked*/
std::string resolverMode(CustomPlayer *pCPlayer, ImColor &color)
{
	char* destinationstring;
	color = ImColor(255, 255, 255, 255);
	switch (pCPlayer->Personalize.LastResolveModeUsed)
	{
	case NoYaw:
		destinationstring = cnonestr;
		break;
	case FakeSpins:
		destinationstring = cfakespinstr2;
		break;
	case LinearFake:
		destinationstring = clinearfakeyawstr2;
		break;
	case RandomFake:
		destinationstring = crandomfakeyawstr2;
		break;
	case CloseFake:
		destinationstring = cstaticantiresolveyawstr2;
		break;
	case BloodBodyRealDelta:
		destinationstring = cbodyrealdeltastr2;
		break;
	case BloodEyeRealDelta:
		destinationstring = ceyerealdeltastr2;
		break;
	case BloodReal:
		destinationstring = clastrealyawstr2;
		break;
	case AtTarget:
		destinationstring = cattargetstr2;
		break;
	case InverseAtTarget:
		destinationstring = cinverseattargetstr2;
		break;
	case AverageLBYDelta:
		destinationstring = cavglowerbodydeltastr2;
		break;
	case StaticFakeDynamic:
		destinationstring = cstaticdynamicfakestr2;
		break;
	case StaticFake:
		destinationstring = cstaticfakeyawstr2;
		break;
	case Inverse:
		destinationstring = cinverseyawstr2;
		break;
	case Back:
		destinationstring = cyawforceback2;
		break;
	case Left:
		destinationstring = cyawleftstr2;
		break;
	case Right:
		destinationstring = cyawrightstr2;
		break;
	case WD:
		destinationstring = cwhaeldongstr;
		break;
	case BackTrackFire:
		destinationstring = cBackTrackFirestr;
		color = ImColor(0, 255, 100, 255);
		break;
	case BackTrackUp:
		destinationstring = cBackTrackUpstr;
		break;
	case BackTrackHit:
		destinationstring = cBackTrackHitstr;
		color = ImColor(0, 255, 100, 255);
		break;
	case BackTrackReal:
		destinationstring = cBackTrackRealstr;
		color = ImColor(0, 255, 100, 255);
		break;
	case BackTrackLby:
		destinationstring = cBackTrackLbystr;
		color = ImColor(0, 255, 100, 255);
		break;
	case Backtracked:
		destinationstring = cbacktrackedstr;
		color = ImColor(0, 255, 100, 255);
		break;
	case NotBackTracked:
		destinationstring = cNotBackTrackedstr;
		break;
	case WD_StaticFake:
		destinationstring = cWD_StaticFakestr;
		break;
	case WD_Velocity:
		destinationstring = cWD_Velocitystr;
		break;
	case WD_Target:
		destinationstring = cWD_Targetstr;
		break;
	case WD_Spin:
		destinationstring = cWD_Spinstr;
		break;
	case WD_Jitter:
		destinationstring = cWD_Jitterstr;
		break;
	case WD_Static:
		destinationstring = cWD_Staticstr;
		break;
	case WD_FuckIt:
		destinationstring = cWD_FuckItstr;
		break;
	default:
		destinationstring = cotherstr2;
		break;
	}
	int len = strlen(destinationstring);
	DecStr(destinationstring, len);
	std::string returnval(destinationstring);
	EncStr(destinationstring, len);
	if (pCPlayer->IsBreakingLagCompensation)
		color = ImColor(255, 0, 0, 255);

	return returnval;
}
void Renderer::DrawEntityBox(ImColor Colour, CBaseEntity* TempTarget, CustomPlayer *pCPlayer)
{
	Vector Origin = TempTarget->GetOrigin();
	Vector Head = TempTarget->GetBonePosition(HITBOX_HEAD, Interfaces::Globals->curtime, false, true);
	Origin.z -= 5;
	Head.z += 7;
	Vector HeadW2S, OriginW2S;
	float targetHealth = TempTarget->GetHealth();
	float entityarmour = TempTarget->GetArmor();
	bool helement = TempTarget->HasHelmet();
	int top = 0;
	int left = 0;
	int right = 0;
	int bottom = 0;
	float distance = (LocalPlayer.Entity->GetOrigin() - Origin).Length();
	float fontSize = 2000 / distance;
	if (fontSize > 15)
		fontSize = 15;
	if (fontSize < 6)
		fontSize = 6;
	fontSize = 14;
#ifdef PRINT_FUNCTION_NAMES
	printf("Drawing\n");
#endif

	if (WorldToScreenCapped(Head, HeadW2S) == TRUE && WorldToScreenCapped(Origin, OriginW2S) == TRUE)
	{
		float height = abs(HeadW2S.y - OriginW2S.y);
		float width = height * 0.55f;
		float thick = 0.7;
		float blackThick = 0.4;
		int alpha = (Colour.Value.w * 255);
		if (Framework->Menu->Config.GetConfigValue("ESP_Fill"))
		{
			ImColor FillColour = ImColor(100, 100, 100, 30);// getColour("fill", TempTarget->GetTeam() == LocalPlayer.Entity->GetTeam(), false, targetHealth);

			rDrawFilledBox(OriginW2S.x - width * 0.5, HeadW2S.y, OriginW2S.x - width * 0.5 + width, HeadW2S.y + height, FillColour);

		}
		switch (Framework->Menu->Config.GetConfigValue("ESP_Box"))
		{
		case 0:
			break;
		case 2:
		{
			r2DrawBox(OriginW2S.x - width / 2, HeadW2S.y, OriginW2S.x + width / 2, HeadW2S.y + height, Colour, thick, blackThick);
			Vector topRight, topLeft, bottomRight, bottomLeft;
			topRight = Vector(OriginW2S.x - width / 2, HeadW2S.y, 0);
			topLeft = Vector(OriginW2S.x + width / 2, HeadW2S.y, 0);
			bottomRight = Vector(OriginW2S.x - width / 2, HeadW2S.y + height, 0);
			bottomLeft = Vector(OriginW2S.x + width / 2, HeadW2S.y + height, 0);


			DrawLinePadded(topRight, topRight + Vector(0, height*0.2, 0), Colour, thick, blackThick);
			DrawLinePadded(topRight, topRight + Vector(width * 0.2, 0, 0), Colour, thick, blackThick);

			DrawLinePadded(topLeft, topLeft + Vector(0, height*0.2, 0), Colour, thick, blackThick);
			DrawLinePadded(topLeft, topLeft - Vector(width * 0.2, 0, 0), Colour, thick, blackThick);

			DrawLinePadded(bottomRight, bottomRight - Vector(0, height*0.2, 0), Colour, thick, blackThick);
			DrawLinePadded(bottomRight, bottomRight + Vector(width * 0.2, 0, 0), Colour, thick, blackThick);

			DrawLinePadded(bottomLeft, bottomLeft - Vector(0, height*0.2, 0), Colour, thick, blackThick);
			DrawLinePadded(bottomLeft, bottomLeft -Vector(width * 0.2, 0, 0), Colour, thick, blackThick);

		}
		break;
		case 3:
		{
			float Height3D = abs(Head.z - Origin.z);
			float Width3D = (Height3D * 0.55f) * 0.5;
			Vector topFrontLeft, topFrontRight, topBackLeft, topBackRight;
			Vector backFrontLeft, backFrontRight, backBackLeft, backBackRight;
			float top = Head.z;
			float bottom = Origin.z;
			topFrontLeft = Vector(Head.x + Width3D, Head.y + Width3D, top);
			topFrontRight = Vector(Head.x + Width3D, Head.y - Width3D, top);
			topBackLeft = Vector(Head.x - Width3D, Head.y + Width3D, top);
			topBackRight = Vector(Head.x - Width3D, Head.y - Width3D, top);

			backFrontLeft = Vector(Head.x + Width3D, Head.y + Width3D, bottom);
			backFrontRight = Vector(Head.x + Width3D, Head.y - Width3D, bottom);
			backBackLeft = Vector(Head.x - Width3D, Head.y + Width3D, bottom);
			backBackRight = Vector(Head.x - Width3D, Head.y - Width3D, bottom);

			Vector stopFrontLeft, stopFrontRight, stopBackLeft, stopBackRight;
			Vector sbackFrontLeft, sbackFrontRight, sbackBackLeft, sbackBackRight;

			if (WorldToScreenCapped(topFrontLeft, stopFrontLeft) == TRUE
			&& WorldToScreenCapped(topFrontRight, stopFrontRight) == TRUE
			&& WorldToScreenCapped(topBackLeft, stopBackLeft) == TRUE
			&& WorldToScreenCapped(topBackRight, stopBackRight) == TRUE
			&& WorldToScreenCapped(backFrontLeft, sbackFrontLeft) == TRUE
			&& WorldToScreenCapped(backFrontRight, sbackFrontRight) == TRUE
			&& WorldToScreenCapped(backBackLeft, sbackBackLeft) == TRUE
			&& WorldToScreenCapped(backBackRight, sbackBackRight) == TRUE)
			{
				DrawLinePadded(stopFrontLeft, stopFrontRight, Colour, thick, blackThick);
				DrawLinePadded(stopFrontRight, stopBackRight, Colour, thick, blackThick);
				DrawLinePadded(stopBackRight, stopBackLeft, Colour, thick, blackThick);
				DrawLinePadded(stopBackLeft, stopFrontLeft, Colour, thick, blackThick);

				DrawLinePadded(sbackFrontLeft, sbackFrontRight, Colour, thick, blackThick);
				DrawLinePadded(sbackFrontRight, sbackBackRight, Colour, thick, blackThick);
				DrawLinePadded(sbackBackRight, sbackBackLeft, Colour, thick, blackThick);
				DrawLinePadded(sbackBackLeft, sbackFrontLeft, Colour, thick, blackThick);

				DrawLinePadded(stopFrontLeft, sbackFrontLeft, Colour, thick, blackThick);
				DrawLinePadded(stopFrontRight, sbackFrontRight, Colour, thick, blackThick);
				DrawLinePadded(stopBackRight, sbackBackRight, Colour, thick, blackThick);
				DrawLinePadded(stopBackLeft, sbackBackLeft, Colour, thick, blackThick);

			}
			break;
		}
		default:
			r2DrawBox(OriginW2S.x - width * 0.5, HeadW2S.y,  width , height, Colour, thick, blackThick);
		}
		switch (Framework->Menu->Config.GetConfigValue("ESP_Health"))
		{
		case 0:
		{

		}
			break;
		case 2:
		{
			float trueHeight = targetHealth * 0.01 * height;

			DrawLinePadded(Vector(OriginW2S.x - width / 2 - width*0.1, HeadW2S.y, 0), Vector(OriginW2S.x - width / 2 - width*0.1, HeadW2S.y + height - trueHeight, 0), ImColor(25, 127, 35, 190), thick*3.5, blackThick);

			DrawLinePadded(Vector(OriginW2S.x - width / 2 - width*0.1, HeadW2S.y + height - trueHeight, 0), Vector(OriginW2S.x - width / 2 - width*0.1, HeadW2S.y + height, 0), ImColor(50, 255, 70, 255), thick*3.5 + blackThick*1.9, blackThick *0.1);

			rRenderText(std::to_string((int)targetHealth), OriginW2S.x - width / 2 - 25, HeadW2S.y + height - trueHeight - 6, fontSize, ImColor(255, 255, 255), true);

		}
		break;
		case 3:
		{
			r2DrawBox(OriginW2S.x - width * 0.5, HeadW2S.y - width * 0.25, width, width * 0.23, ImColor((255 - 2.55*targetHealth), (2.55*targetHealth), 0, 255), thick, blackThick);
			int repeats = floor(targetHealth / 10);
			float offset = width * 0.1;
			for (int i = 1; i < repeats; i++)
			{
				DrawLinePadded(Vector(OriginW2S.x - width * 0.45 + offset * i, HeadW2S.y - width * 0.25, 0), Vector(OriginW2S.x - width * 0.45 + offset * i, HeadW2S.y - width * 0.04, 0), ImColor((255 - 2.55*targetHealth), (2.55*targetHealth), 0, 255), offset * 0.7, 4);
			}
			top++;
		}
		break; 
		case 4:
		{
			float trueWidth = targetHealth * 0.01 * width;

			DrawLinePadded(Vector(OriginW2S.x - width / 2, HeadW2S.y + height, 0), Vector(OriginW2S.x - width / 2 + width - trueWidth, HeadW2S.y + height, 0), ImColor(25, 127, 35, 190), thick*1.5, blackThick);

			DrawLinePadded(Vector(OriginW2S.x - width / 2, HeadW2S.y + height * 1.2, 0), Vector(OriginW2S.x - width / 2 + trueWidth, HeadW2S.y + height* 1.2, 0), ImColor(50, 255, 70, 255), thick*1.5, blackThick);

			bottom++;
		}
		break;
		default:
			float trueHeight = targetHealth * 0.01 * height;

			DrawLinePadded(Vector(OriginW2S.x - width / 2, HeadW2S.y, 0), Vector(OriginW2S.x - width / 2, HeadW2S.y + height - trueHeight, 0), ImColor(25, 127, 35, 190), thick*1.5, blackThick);

			DrawLinePadded(Vector(OriginW2S.x - width / 2, HeadW2S.y + height - trueHeight, 0), Vector(OriginW2S.x - width / 2, HeadW2S.y + height, 0), ImColor(50, 255, 70, 255), thick*1.5, blackThick);

			rRenderText(std::to_string((int)targetHealth), OriginW2S.x - width / 2 - 15, HeadW2S.y + height - trueHeight - 6, fontSize, ImColor(255, 255, 255), true);

			break;
		}

		switch (Framework->Menu->Config.GetConfigValue("ESP_Armour"))
		{
		case 0:
		{

		}
		break;
		case 2:
		{
			float trueHeight = entityarmour * 0.01 * height;

			DrawLinePadded(Vector(OriginW2S.x + width / 2 + width*0.1, HeadW2S.y, 0), Vector(OriginW2S.x + width / 2 + width*0.1, HeadW2S.y + height - trueHeight, 0), ImColor(0, 100, 127, 190), thick*3.5, blackThick);

			DrawLinePadded(Vector(OriginW2S.x + width / 2 + width*0.1, HeadW2S.y + height - trueHeight, 0), Vector(OriginW2S.x + width / 2 + width*0.1, HeadW2S.y + height, 0), ImColor(0, 200, 255, 255), thick*3.5 + blackThick*1.9, blackThick *0.1);
			std::string armour = std::to_string((int)entityarmour);
			if (helement)
				armour += " [H]";

			rRenderText(armour, OriginW2S.x + width / 2 + 25, HeadW2S.y + height - trueHeight - 6, fontSize, ImColor(255, 255, 255), true);
		}
		break;
		case 3:
		{
			float trueWidth = entityarmour * 0.01 * width;

			DrawLinePadded(Vector(OriginW2S.x - width / 2, HeadW2S.y + height + height* bottom * 0.2, 0), Vector(OriginW2S.x - width / 2 + width - trueWidth, HeadW2S.y + height + height* bottom * 0.2, 0), ImColor(0, 100, 127, 190), thick*1.5, blackThick);

			DrawLinePadded(Vector(OriginW2S.x - width / 2, HeadW2S.y + height * 1.2 + height* bottom * 0.2, 0), Vector(OriginW2S.x - width / 2 + trueWidth, HeadW2S.y + height* 1.2 + height* bottom * 0.2, 0), ImColor(0, 200, 255, 255), thick*1.5, blackThick);

			bottom++;
		}
		break;
		default:
			float trueHeight = entityarmour * 0.01 * height;

			DrawLinePadded(Vector(OriginW2S.x + width / 2, HeadW2S.y, 0), Vector(OriginW2S.x + width / 2, HeadW2S.y + height - trueHeight, 0), ImColor(0, 100, 127, 190), thick*1.5, blackThick);

			DrawLinePadded(Vector(OriginW2S.x + width / 2, HeadW2S.y + height - trueHeight, 0), Vector(OriginW2S.x + width / 2, HeadW2S.y + height, 0), ImColor(0, 200, 255, 255), thick*1.5, blackThick);

			std::string armour = std::to_string((int)entityarmour);
			if (helement)
				armour += " [H]";
			rRenderText(armour, OriginW2S.x + width / 2 + 20, HeadW2S.y + height - trueHeight - 6, fontSize, ImColor(255, 255, 255), true);

			break;
		}

		switch (Framework->Menu->Config.GetConfigValue("ESP_Name"))
		{
		case 0:
		{

		}
		break;
		case 2:
		{
			std::string string = TempTarget->GetName();
			rRenderText(string, OriginW2S.x + width / 2 + 40, HeadW2S.y + 20 * left, fontSize, ImColor(255, 255, 255), true);
			right++;
		}
		break;
		case 3:
		{
			std::string string = TempTarget->GetName();
			rRenderText(string, OriginW2S.x, HeadW2S.y - 20 - 20 * top, fontSize, ImColor(255, 255, 255), true);
			top++;
		}
		break;
		case 4:
		{
			std::string string = TempTarget->GetName();
			rRenderText(string, OriginW2S.x, HeadW2S.y + height + height* bottom * 0.2, fontSize, ImColor(255, 255, 255), true);
			bottom++;
		}
		break;

		default:
			
			std::string string = TempTarget->GetName();
			rRenderText(string, OriginW2S.x - width / 2 - 40, HeadW2S.y + 20 * left, fontSize, ImColor(255, 255, 255), true);
			left++;
			break;
		}

		CBaseCombatWeapon *pWeapon = TempTarget->GetWeapon();
		if (pWeapon)
		{
			switch (Framework->Menu->Config.GetConfigValue("ESP_Weapon"))
			{
			case 0:
			{

			}
			break;
			case 2:
			{
				std::string string = pWeapon->GetWeaponName();;
				string += " [" + std::to_string(pWeapon->GetClipOne()) + "] ";
				if (pWeapon->IsReloading())
					string += "[R]";
				if (GetZoomLevel(TempTarget))
					string += " [S]";

				rRenderText(string, OriginW2S.x + width / 2 + 40, HeadW2S.y + 20 * left, fontSize, ImColor(255, 255, 255), true);
				right++;
			}
			break;
			case 3:
			{
				std::string string = pWeapon->GetWeaponName();;
				string += " [" + std::to_string(pWeapon->GetClipOne()) + "] ";
				if (pWeapon->IsReloading())
					string += "[R]";
				if (GetZoomLevel(TempTarget))
					string += " [S]";

				rRenderText(string, OriginW2S.x, HeadW2S.y - 20 - 20 * top, fontSize, ImColor(255, 255, 255), true);
				top++;
			}
			break;
			case 4:
			{
				std::string string = pWeapon->GetWeaponName();;
				string += " [" + std::to_string(pWeapon->GetClipOne()) + "] ";
				if (pWeapon->IsReloading())
					string += "[R]";
				if (GetZoomLevel(TempTarget))
					string += " [S]";

				rRenderText(string, OriginW2S.x, HeadW2S.y + height + height* bottom * 0.2, fontSize, ImColor(255, 255, 255), true);
				bottom++;
			}
			break;

			default:

				std::string string = pWeapon->GetWeaponName();;
				string += " [" + std::to_string(pWeapon->GetClipOne()) + "] ";
				if (pWeapon->IsReloading())
					string += "[R]";
				if (GetZoomLevel(TempTarget))
					string += " [S]";

				rRenderText(string, OriginW2S.x - width / 2 - 40, HeadW2S.y + 20 * left, fontSize, ImColor(255, 255, 255), true);
				left++;
				break;
			}
		}
		switch (Framework->Menu->Config.GetConfigValue("ESP_Resolver"))
		{
		case 0:
		{

		}
		break;
		case 2:
		{
			ImColor color = ImColor(255, 255, 255);
			std::string string = resolverMode(pCPlayer, color);

			rRenderText(string, OriginW2S.x + width / 2 + 60, HeadW2S.y + 20 * left, fontSize, color, true);
			right++;
		}
		break;
		case 3:
		{
			ImColor color = ImColor(255, 255, 255, 255);
			std::string string = resolverMode(pCPlayer, color);

			rRenderText(string, OriginW2S.x, HeadW2S.y - 20 - 20 * top, fontSize, color, true);
			top++;
		}
		break;
		case 4:
		{
			ImColor color = ImColor(255, 255, 255, 255);
			std::string string = resolverMode(pCPlayer, color);
			rRenderText(string, OriginW2S.x, HeadW2S.y + height + height* bottom * 0.2, fontSize, color, true);
			bottom++;
		}
		break;

		default:

			ImColor color = ImColor(255, 255, 255, 255);
			std::string string = resolverMode(pCPlayer, color);

			rRenderText(string, OriginW2S.x - width / 2 - 60, HeadW2S.y + 20 * left, fontSize, color, true);
			left++;
			break;
		}

		switch (Framework->Menu->Config.GetConfigValue("ESP_Info"))
		{
		case 0:
		{

		}
		break;
		case 2:
		{
			ImColor color = ImColor(255, 255, 255);
			std::string string = std::to_string((int)distance);
			if (TempTarget->IsFlashed())
				string += " [F]";
			rRenderText(string, OriginW2S.x + width / 2 + 60, HeadW2S.y + 20 * left, fontSize, color, true);
			right++;
		}
		break;
		case 3:
		{
			ImColor color = ImColor(255, 255, 255);
			std::string string = std::to_string((int)distance);
			if (TempTarget->IsFlashed())
				string += " [F]";

			rRenderText(string, OriginW2S.x, HeadW2S.y - 20 - 20 * top, fontSize, color, true);
			top++;
		}
		break;
		case 4:
		{
			ImColor color = ImColor(255, 255, 255);
			std::string string = std::to_string((int)distance);
			if (TempTarget->IsFlashed())
				string += " [F]";
			rRenderText(string, OriginW2S.x, HeadW2S.y + height + height* bottom * 0.2, fontSize, color, true);
			bottom++;
		}
		break;

		default:

			ImColor color = ImColor(255, 255, 255);
			std::string string = std::to_string((int)distance);
			if (TempTarget->IsFlashed())
				string += " [F]";

			rRenderText(string, OriginW2S.x - width / 2 - 60, HeadW2S.y + 20 * left, fontSize, color, true);
			left++;
			break;
		}

	}
	SkeltonEsp(pCPlayer);
#ifdef PRINT_FUNCTION_NAMES
	printf("EndDrawing\n");
#endif

}
ImColor Renderer::getColour(std::string setting, bool teamMate, bool visible, int entityHealth)
{
#ifdef PRINT_FUNCTION_NAMES
	printf("GetColor\n");
#endif

	if (teamMate)
	{
		if (visible)
			setting.append("_team_vis");
		else
			setting.append("_team");
	}
	else
	{
		if (visible)
			setting.append("_vis");
	}
	int alpha = Framework->Menu->Config.GetPreciseConfigValue(setting + ("_a"));
	bool rainbow = Framework->Menu->Config.GetPreciseConfigValue(setting+("_rainbow"));
	if (Framework->Menu->Config.GetPreciseConfigValue(setting+("_health")))
	{
		int blue = 0;
		if (rainbow)
			blue = healthb;
		return ImColor((255 - 2.55*entityHealth), (2.55*entityHealth), blue, alpha);
	}
	if (rainbow)
	{
		return ImColor(r, g, b, alpha);
	}
	ImColor returnval = ImColor(Framework->Menu->Config.GetPreciseConfigValue(setting+("_r"))*255, Framework->Menu->Config.GetPreciseConfigValue(setting+("_g")) * 255, Framework->Menu->Config.GetPreciseConfigValue(setting+("_b")) * 255, alpha);
#ifdef PRINT_FUNCTION_NAMES
	printf("EndGetColor\n");
#endif

	return returnval;

}
bool Renderer::isVisible(CBaseEntity* TempTarget, CBaseEntity* LocalEntity, Vector LocalEyePos, QAngle LocalAngles)
{
	trace_t tr;
	Vector targetPos = TempTarget->GetBonePosition(HITBOX_HEAD, Interfaces::Globals->curtime, false, true);
	UTIL_TraceLine(LocalEyePos, targetPos, MASK_SHOT, LocalPlayer.Entity, &tr);
	Vector vecDir;
	AngleVectors(LocalAngles, &vecDir);
	VectorNormalizeFast(vecDir);
	CTraceFilter filter;
	filter.pSkip = (IHandleEntity*)LocalPlayer.Entity;
	UTIL_ClipTraceToPlayers(LocalEyePos, targetPos + vecDir * 40.0f, 0x4600400B, &filter, &tr);

	if (tr.m_pEnt && tr.m_pEnt == TempTarget) // (bforcehitgroup ? tr.hitgroup == forcehitgroup : IsHitgroupACurrentTarget(tr.hitgroup)))
	{
		return true;
	}
	return false;
}
bool Renderer::showRenderESP(CBaseEntity* TempTarget, CBaseEntity* LocalEntity)
{
	if (!TempTarget)return false;
	if (!TempTarget->GetAlive()
		|| TempTarget->GetDormant())
		return false;
	if (TempTarget->GetTeam() == LocalEntity->GetTeam() && !Framework->Menu->Config.GetConfigValue("ESP_Team"))
		return false;
	return true;
}
void Renderer::EntityLoop()
{
	if (!LocalPlayer.Entity)
		return;

	CBaseEntity *localplayer = LocalPlayer.Entity;
	Vector LocalEyePos = localplayer->GetEyePosition();
	QAngle LocalAngles = localplayer->GetEyeAngles();

	for (int i = 0; i < NumStreamedPlayers; i++)
	{
		CustomPlayer *pCPlayer = StreamedPlayers[i];
		if (!pCPlayer)continue;
		if (!pCPlayer->IsLocalPlayer && !pCPlayer->Dormant)
		{

			CBaseEntity* TempTarget = pCPlayer->BaseEntity;
			//I think this is all i need to check for?
			if(!showRenderESP(TempTarget, localplayer))
			continue;
			bool visible = isVisible(TempTarget, localplayer, LocalEyePos, LocalAngles);
			if (!visible && Framework->Menu->Config.GetConfigValue("ESP_Vis_Checks"))
				continue;
			int EntityHealth = TempTarget->GetHealth();
			//We have now found an entity we want to draw

			ImColor Colour = getColour("esp", TempTarget->GetTeam() == localplayer->GetTeam(), visible, EntityHealth);
			DrawEntityBox(Colour, TempTarget, pCPlayer);
		}
	}
}
void Renderer::DrawESP()
{
#ifdef PRINT_FUNCTION_NAMES
	AllocateConsole();
	printf("DrawESP\n");
#endif

	rRenderText("Hello world, This is me", 100, 100, 14, ImColor(190, 190, 190, 255), false);

	if (!Interfaces::Engine->IsInGame() || !Interfaces::Engine->IsConnected())
		return;

	if (Framework->Menu->Config.GetConfigValue("Enable_ESP"))
	{

		Rainbow();
		EntityLoop();
	}
#ifdef PRINT_FUNCTION_NAMES
	printf("EndDrawESP\n");
#endif

}


void Renderer::rDrawBox(float x, float y, float x1, float y1, ImColor Color, float thickNess)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRect(ImVec2(x, y), ImVec2(x1, y1), Color, 0, 0, thickNess);
}
void Renderer::r2DrawBox(float x, float y, float x1, float y1, ImColor Color, float thickNess, float pad)
{
	Renderer Render;

	Render.DrawLinePadded(Vector(x, y, 0), Vector(x + x1, y, 0), Color, thickNess, pad);
	Render.DrawLinePadded(Vector(x + x1, y, 0), Vector(x + x1, y + y1, 0), Color, thickNess, pad);
	Render.DrawLinePadded(Vector(x + x1, y + y1, 0), Vector(x, y + y1, 0), Color, thickNess, pad);
	Render.DrawLinePadded(Vector(x, y + y1, 0), Vector(x, y, 0), Color, thickNess, pad);
}


void Renderer::rDrawFilledBox(float x, float y, float x1, float y1, ImColor Color)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->AddRectFilled(ImVec2(x, y), ImVec2(x1, y1), Color);
}

void Renderer::rDrawLine(float x, float y, float x1, float y1, ImColor Color)
{
	Renderer Render;
	Render.DrawLine(ImVec2(x, y), ImVec2(x1, y1), Color, 1);
}
void Renderer::rRenderText(std::string Text, float x, float y, float size, ImColor Color, bool center)
{
	Renderer Render;
	Render.RenderText(m_pFont, Text, ImVec2(x, y), size, Color, center);
}
void Renderer::rDrawCircle(float x, float y, float radius, ImColor Color, float thickness)
{
	Renderer Render;
	Render.DrawCircle(ImVec2(x, y), radius, Color, thickness);

}
void Renderer::Initialize(HWND g_hWindow)
{
	ImGuiIO& io = ImGui::GetIO(); 
	//m_pFont = io.Fonts->AddFontFromMemoryCompressedTTF(RudaCompressed, RudaSize, 16.0f);
	//DWORD Font_Tahoma;
	ImFontConfig font_config;
	font_config.OversampleH = 1; //or 2 is the same
	font_config.OversampleV = 1;
	font_config.PixelSnapH = 1;
	static const ImWchar ranges[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0400, 0x044F, // Cyrillic
		0,
	};

	m_pFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Tahoma.ttf", 14, &font_config, ranges);
}

void Renderer::BeginScene()
{
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::Begin("BackBuffer", reinterpret_cast<bool*>(true), ImVec2(0, 0), 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);

	ImGui::SetWindowPos(ImVec2(0, 0), ImGuiSetCond_Always);
	ImGui::SetWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImGuiSetCond_Always);
}

void Renderer::DrawScene()
{
	ImGuiIO& io = ImGui::GetIO();

}

void Renderer::EndScene()
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->PushClipRectFullScreen();
	{

	}
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::Render();
}

float Renderer::RenderText(ImFont* pFont, const std::string& text, const ImVec2& pos, float size, ImColor color, bool center)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();


	std::stringstream steam(text);
	std::string line;

	float y = 0.0f;
	int i = 0;

	while (std::getline(steam, line))
	{
		ImVec2 textSize = pFont->CalcTextSizeA(size, FLT_MAX, 0.0f, line.c_str());

		if (center)
		{
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f), (pos.y + textSize.y * i) ), color, line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) + 1), ImColor(0,0,0,255), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) + 1), ImColor(0, 0, 0, 255), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) - 1), ImColor(0, 0, 0, 255), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) - 1), ImColor(0, 0, 0, 255), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f), (pos.y + textSize.y * i)), color, line.c_str());

		}
		else
		{
			window->DrawList->AddText(pFont, size, ImVec2((pos.x), (pos.y + textSize.y * i)), color, line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) + 1), ImColor(0, 0, 0, 255), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) + 1), ImColor(0, 0, 0, 255), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) - 1), ImColor(0, 0, 0, 255), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) - 1), ImColor(0, 0, 0, 255), line.c_str());
			window->DrawList->AddText(pFont, size, ImVec2((pos.x), (pos.y + textSize.y * i)), color, line.c_str());

		}

		y = pos.y + textSize.y * (i + 1);
		i++;
	}

	return y;
}
void Renderer::DrawLinePadded(const Vector& from, const Vector& to, ImColor color, float thickness, float pad)
{

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	int alpha = (color.Value.w * 255);
	window->DrawList->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), ImColor(0, 0, 0, alpha), thickness + pad * 2);
	window->DrawList->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), color, thickness);

}

void Renderer::DrawLine(const ImVec2& from, const ImVec2& to, ImColor color, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddLine(from, to, color, thickness);
}

void Renderer::DrawCircle(const ImVec2& position, float radius, ImColor color, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddCircle(position, radius, color, 64, thickness);
}

void Renderer::DrawCircleFilled(const ImVec2& position, float radius, ImColor color)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();


	window->DrawList->AddCircleFilled(position, radius, color, 12);
}

Renderer* Renderer::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new Renderer();

	return m_pInstance;
}