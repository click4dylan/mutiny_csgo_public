#pragma once
#include "misc.h"
class IInput;
class bf_write;
class bf_read;
class CUserCmd;
class C_BaseCombatWeapon;
struct kbutton_t;

class CKeyboardKey
{
public:
	// Name for key
	char				name[32];
	// Pointer to the underlying structure
	kbutton_t			*pkey;
	// Next key in key list.
	CKeyboardKey		*next;
};

class ConVar;

class CVerifiedUserCmd
{
public:
	CUserCmd	m_cmd;
	unsigned long		m_crc;
};

struct CameraThirdData_t
{
	float	m_flPitch;
	float	m_flYaw;
	float	m_flDist;
	float	m_flLag;
	Vector	m_vecHullMin;
	Vector	m_vecHullMax;
};

abstract_class IInput
{
public:
	// Initialization/shutdown of the subsystem
	virtual void Init_All(void) = 0;
	virtual void Shutdown_All(void) = 0;
	// Latching button states
	virtual int GetButtonBits(bool bResetState) = 0;
	// Create movement command
	virtual void CreateMove(int sequence_number, float input_sample_frametime, bool active) = 0;
	virtual void ExtraMouseSample(float frametime, bool active) = 0;
	virtual bool WriteUsercmdDeltaToBuffer(int nSlot, bf_write *buf, int from, int to, bool isnewcommand) = 0;
	virtual void EncodeUserCmdToBuffer(int nSlot, bf_write &buf, int slot) = 0;
	virtual void DecodeUserCmdFromBuffer(int nSlot, bf_read &buf, int slot) = 0;

	virtual CUserCmd *GetUserCmd(int nSlot, int sequence_number) = 0;

	virtual void MakeWeaponSelection(C_BaseCombatWeapon * weapon) = 0;

	// Retrieve key state
	virtual float KeyState(kbutton_t * key) = 0;
	// Issue key event
	virtual int KeyEvent(int eventcode, ButtonCode_t keynum, const char *pszCurrentBinding) = 0;
	// Look for key
	virtual kbutton_t *FindKey(const char *name) = 0;

	// Issue commands from controllers
	virtual void ControllerCommands(void) = 0;
	// Extra initialization for some joysticks
	virtual bool ControllerModeActive() = 0;
	virtual void Joystick_Advanced(bool bSilent) = 0;
	virtual void Joystick_SetSampleTime(float frametime) = 0;

	//
	virtual float Joystick_GetPitch(void) = 0;
	virtual float Joystick_GetYaw(void) = 0;
	virtual void Joystick_Querry(float &, float &, float &, float &) = 0;
	virtual void Joystick_ForceRecentering(int, bool) = 0;
	//

	virtual void IN_SetSampleTime(float frametime) = 0;

	// Accumulate mouse delta
	virtual void AccumulateMouse(int nSlot) = 0;
	// Activate/deactivate mouse
	virtual void ActivateMouse(void) = 0;
	virtual void DeactivateMouse(void) = 0;

	// Clear mouse state data
	virtual void ClearStates(void) = 0;
	// Retrieve lookspring setting
	virtual float GetLookSpring(void) = 0;

	// Retrieve mouse position
	virtual void GetFullscreenMousePos(int *mx, int *my, int *unclampedx = 0, int *unclampedy = 0) = 0;
	virtual void SetFullscreenMousePos(int mx, int my) = 0;
	virtual void ResetMouse() = 0;
	virtual float GetLastForwardMove(void) = 0;

	// Third Person camera ( TODO/FIXME:  Move this to a separate interface? )
	virtual void CAM_Think(void) = 0;
	virtual int CAM_IsThirdPerson(int nSlot = -1) = 0;
	virtual bool CAM_IsThirdPersonOverview(int nSlot = -1) = 0;
	virtual void CAM_GetCameraOffset(Vector & ofs) = 0;
	virtual void CAM_ToThirdPerson(void) = 0;
	virtual void CAM_ToFirstPerson(void) = 0;
	virtual void CAM_ToThirdPersonShoulder(void) = 0;
	virtual void CAM_ToThirdPersonOverview(void) = 0;
	virtual void CAM_StartMouseMove(void) = 0;
	virtual void CAM_EndMouseMove(void) = 0;
	virtual void CAM_StartDistance(void) = 0;
	virtual void CAM_EndDistance(void) = 0;
	virtual int CAM_InterceptingMouse(void) = 0;
	virtual void CAM_Command(int command) = 0;

	// orthographic camera info	( TODO/FIXME:  Move this to a separate interface? )
	virtual void CAM_ToOrthographic() = 0;
	virtual bool CAM_IsOrthographic() const = 0;
	virtual void CAM_OrthographicSize(float &w, float &h) const = 0;

#if defined(HL2_CLIENT_DLL)
	// IK back channel info
	virtual void AddIKGroundContactInfo(int entindex, float minheight, float maxheight) = 0;
#endif

	virtual void LevelInit(void) = 0;

	// Causes an input to have to be re-pressed to become active
	virtual void ClearInputButton(int bits) = 0;

	virtual void CAM_SetCameraThirdData(CameraThirdData_t * pCameraData, const QAngle &vecCameraOffset) = 0;
	virtual void CAM_CameraThirdThink(void) = 0;
};


#define MULTIPLAYER_BACKUP 150

class __declspec(align(4)) CInput : public IInput
{
public:
	char pad_0x00[8]; // 0x00
	bool m_fTrackIRAvailable;
	bool m_fMouseInitialized;
	bool m_fMouseActive;
	bool m_fJoystickAdvancedInit;
	unsigned char gap_9[1];
	DWORD field_C;
	DWORD field_10;
	char field_14[4];
	int m_rgOrigMouseParms[3];
	int m_rgNewMouseParms[3];
	bool m_rgCheckMouseParam[3];
	bool m_fMouseParmsValid;
	CKeyboardKey *m_pKeys;
	float m_flAccumulatedMouseXMovement;
	float m_flAccumulatedMouseYMovement;
	float m_flPreviousMouseXPosition;
	float m_flPreviousMouseYPosition;
	float m_flRemainingJoystickSampleTime;
	float m_flKeyboardSampleTime;
	BYTE gap_38[84];
	bool m_fCameraInterceptingMouse;
	bool m_fCameraInThirdPerson;
	bool m_fCameraMovingWithMouse;
	Vector m_vecCameraOffset;
	bool m_fCameraDistanceMove;
	int m_nCameraOldX;
	int m_nCameraOldY;
	int m_nCameraX;
	int m_nCameraY;
	bool m_CameraIsOrthographic;
	Vector m_angPreviousViewAngles;
	Vector m_angPreviousViewAnglesTilt;
	float m_flLastForwardMove;
	int m_nClearInputState;
	CUserCmd *m_pCommands;
	CVerifiedUserCmd *m_pVerifiedCommands;
	CameraThirdData_t *m_pCameraThirdData;
	int m_nCamCommand;

	CUserCmd* GetUserCmd(int sequence_number) const
	{
		return &m_pCommands[sequence_number % MULTIPLAYER_BACKUP];
	}

	CVerifiedUserCmd* GetVerifiedUserCmd(int sequence_number) const
	{
		return &m_pVerifiedCommands[sequence_number % MULTIPLAYER_BACKUP];
	}
};