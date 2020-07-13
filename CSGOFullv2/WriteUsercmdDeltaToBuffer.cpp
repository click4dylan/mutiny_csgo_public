#include "precompiled.h"
#include "CreateMove.h"
#include "LocalPlayer.h"

WriteUsercmdDeltaToBufferFn oWriteUsercmdDeltaToBuffer;
WriteUsercmdFn oWriteUserCmd;
CUserCmd* m_pCommands;

CUserCmd* GetUserCmdStruct(int slot)
{
	CUserCmd* pInputPtr = (CUserCmd*)Interfaces::Input;
	if (slot != -1)
		pInputPtr = (CUserCmd*)&Interfaces::Input[220 * slot];

	CUserCmd *m_pCommands = (CUserCmd*) *(DWORD*)((DWORD)pInputPtr + 0xF4); //236

	return m_pCommands;
}

CUserCmd* GetUserCmd(int slot, int sequence_number, bool DisableValidation)
{	
	CUserCmd *m_pCommands = GetUserCmdStruct(slot);

	CUserCmd* usercmd = &m_pCommands[sequence_number % 150];

	if (!DisableValidation && usercmd->command_number != sequence_number)
		return nullptr;

	return usercmd;
}

struct INetMessage {
	virtual ~INetMessage();
};

template<typename T>
class CNetMessagePB : public INetMessage, public T {};

class CCLCMsg_Move {
private:
	char __PAD0[0x8];
public:
	int numBackupCommands;
	int numNewCommands;
};

using CCLCMsg_Move_t = CNetMessagePB<CCLCMsg_Move>;

#if 0
char *cl_sendmovesig = new char[87]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 59, 75, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 75, 90, 90, 63, 57, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 56, 67, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 79, 73, 90, 90, 66, 56, 90, 90, 67, 66, 0 }; /*55  8B  EC  A1  ??  ??  ??  ??  81  EC  ??  ??  ??  ??  B9  ??  ??  ??  ??  53  8B  98*/
int sinceUse = 0;
int sinceUse2 = 0;
#endif

																																																																																																  // Largest # of commands to send in a packet
#define NUM_NEW_COMMAND_BITS		4
#define MAX_NEW_COMMANDS			((1 << NUM_NEW_COMMAND_BITS)-1)

																																																																																																  // Max number of history commands to send ( 2 by default ) in case of dropped packets
#define NUM_BACKUP_COMMAND_BITS		3
#define MAX_BACKUP_COMMANDS			((1 << NUM_BACKUP_COMMAND_BITS)-1)
																																																																																																																																																																																												  // Input  : *buf - 

bool __fastcall Hooks::WriteUsercmdDeltaToBuffer(void* ecx, void* edx, int slot, void* buf, int from, int to, bool isnewcommand)
{
	bool isvalveserver = GetGamerules() && GetGamerules()->IsValveServer();
	if (isvalveserver && !LocalPlayer.IsAllowedUntrusted())
		return oWriteUsercmdDeltaToBuffer(ecx, slot, buf, from, to, isnewcommand);
	
	//don't allow game to do this, we rebuilt CL_SendMove
	return false;
}