#include "precompiled.h"
#include "includes.h"
#include "bfwrite.h"
#include "CLC_Move.h"
#include "Netchan.h"
#include "LocalPlayer.h"
#include "TickbaseExploits.h"
#include "INetchannelInfo.h"
#include "UsedConvars.h"
#include "Eventlog.h"
#include "VMProtectDefs.h"

#include "Adriel/stdafx.hpp"
#include "Adriel/input.hpp"

CCLCMsg_Move_Deconstructor_Fn CCLCMsg_Move_Deconstructor;

void CLC_Move::FinishProtobuf() 
{
	flags |= 4;
	int v11 = (m_DataOut.m_iCurBit + 7) >> 3;
	if (allocatedmemory == StaticOffsets.GetOffsetValueByType<void*>(_CL_SendMove_DefaultMemory))
	{
		void* memory = MALLOC(24);
		if (memory)
		{
			*((DWORD *)memory + 5) = 15;
			*((DWORD *)memory + 4) = 0;
			*(BYTE *)memory = 0;
		}

		allocatedmemory = memory;
	}

	StaticOffsets.GetOffsetValueByType<void*(__thiscall*)(void*, unsigned char*, size_t)>(_CCLCMsg_Move_UnknownCall)(allocatedmemory, m_DataOut.GetData(), v11);
}

CLC_Move::CLC_Move() 
{
	unknown = 0;
	someint3 = 0;
	INetMessage_vtable = StaticOffsets.GetOffsetValueByType<uint32_t>(_CCLCMsg_Move_vtable1);
	CCLCMsg_Move_vtable = StaticOffsets.GetOffsetValueByType<uint32_t>(_CCLCMsg_Move_vtable2);
	allocatedmemory = StaticOffsets.GetOffsetValueByType<void*>(_CL_SendMove_DefaultMemory);
	somebyte1 = 0;
	someint = 15;
	someint2 = 0;
	somebyte2 = 0;

	flags = 0;
	flags |= 1;
	flags |= 2;
}

CLC_Move::~CLC_Move()
{
	CCLCMsg_Move_Deconstructor(this);
}

static bool WriteUserCmdDeltaInt(bf_write *buf, char *what, int from, int to, int bits = 32) 
{
	if (from != to) {
		buf->WriteOneBit(1);
		buf->WriteUBitLong(to, bits);
		return true;
	}

	buf->WriteOneBit(0);
	return false;
}

static bool WriteUserCmdDeltaShort(bf_write *buf, char *what, int from, int to)
{
	if (from != to) 
	{
		buf->WriteOneBit(1);
		buf->WriteShort(to);
		return true;
	}

	buf->WriteOneBit(0);
	return false;
}

static bool WriteUserCmdDeltaFloat(bf_write *buf, char *what, float from, float to) 
{
	if (from != to) 
	{
		buf->WriteOneBit(1);
		buf->WriteFloat(to);
		return true;
	}

	buf->WriteOneBit(0);
	return false;
}

static bool WriteUserCmdDeltaCoord(bf_write *buf, char *what, float from, float to)
{
	if (from != to) 
	{
		buf->WriteOneBit(1);
		buf->WriteBitCoord(to);
		return true;
	}

	buf->WriteOneBit(0);
	return false;
}

static bool WriteUserCmdDeltaAngle(bf_write *buf, char *what, float from, float to, int bits)
{
	if (from != to) 
	{
		buf->WriteOneBit(1);
		buf->WriteBitAngle(to, bits);
		return true;
	}

	buf->WriteOneBit(0);
	return false;
}

static bool WriteUserCmdDeltaVec3Coord(bf_write *buf, char *what, Vector &from, const Vector &to) 
{
	if (from != to) 
	{
		buf->WriteOneBit(1);
		buf->WriteBitVec3Coord(to);
		return true;
	}

	buf->WriteOneBit(0);
	return false;
}

void WriteUsercmd(bf_write *buf, CUserCmd *to, CUserCmd *from)
{
	WriteUserCmdDeltaInt(buf, (char*)0/*"command_number"*/, from->command_number + 1, to->command_number, 32);
	WriteUserCmdDeltaInt(buf, (char*)0/*"tick_count"*/, from->tick_count + 1, to->tick_count, 32);

	WriteUserCmdDeltaFloat(buf, (char*)0/*"viewangles[0]"*/, from->viewangles[0], to->viewangles[0]);
	WriteUserCmdDeltaFloat(buf, (char*)0/*"viewangles[1]"*/, from->viewangles[1], to->viewangles[1]);
	WriteUserCmdDeltaFloat(buf, (char*)0/*"viewangles[2]"*/, from->viewangles[2], to->viewangles[2]);

	WriteUserCmdDeltaFloat(buf, (char*)0/*"aimdirection[0]"*/, from->aimdirection[0], to->aimdirection[0]);
	WriteUserCmdDeltaFloat(buf, (char*)0/*"aimdirection[1]"*/, from->aimdirection[1], to->aimdirection[1]);
	WriteUserCmdDeltaFloat(buf, (char*)0/*"aimdirection[2]"*/, from->aimdirection[2], to->aimdirection[2]);

	WriteUserCmdDeltaFloat(buf, (char*)0/*"forwardmove"*/, from->forwardmove, to->forwardmove);
	WriteUserCmdDeltaFloat(buf, (char*)0/*"sidemove"*/, from->sidemove, to->sidemove);
	WriteUserCmdDeltaFloat(buf, (char*)0/*"upmove"*/, from->upmove, to->upmove);
	WriteUserCmdDeltaInt(buf, (char*)0/*"buttons"*/, from->buttons, to->buttons, 32);
	WriteUserCmdDeltaInt(buf, (char*)0/*"impulse"*/, from->impulse, to->impulse, 8);

	if (WriteUserCmdDeltaInt(buf, (char*)0/*"weaponselect"*/, from->weaponselect, to->weaponselect, MAX_EDICT_BITS))
		WriteUserCmdDeltaInt(buf, (char*)0/*"weaponsubtype"*/, from->weaponsubtype, to->weaponsubtype, 6);

	// TODO: Can probably get away with fewer bits.
	WriteUserCmdDeltaShort(buf, (char*)0/*"mousedx"*/, from->mousedx, to->mousedx);
	WriteUserCmdDeltaShort(buf, (char*)0/*"mousedy"*/, from->mousedy, to->mousedy);
}

void WriteUsercmd_Force(bf_write* buf, CUserCmd* to, CUserCmd* from)
{
	buf->WriteOneBit(1);
	buf->WriteUBitLong(to->command_number, 32);
	buf->WriteOneBit(1);
	buf->WriteUBitLong(to->tick_count, 32);
	buf->WriteOneBit(0);//viewangles
	buf->WriteOneBit(0);
	buf->WriteOneBit(0);
	buf->WriteOneBit(0);//aimdir
	buf->WriteOneBit(0);
	buf->WriteOneBit(0);
	buf->WriteOneBit(0);//forward
	buf->WriteOneBit(0);//side
	buf->WriteOneBit(0);//up
	buf->WriteOneBit(0);//buttons
	buf->WriteOneBit(0);//impulse
	buf->WriteOneBit(0);//weaponselect
	buf->WriteOneBit(0);//mousedx
	buf->WriteOneBit(0);//mousedy
}

void ValidateUserCmd(CInput* input, CUserCmd *usercmd, int sequence_number)
{
	// Validate that the usercmd hasn't been changed
	CRC32_t crc = usercmd->GetChecksum();
	CVerifiedUserCmd *verified_cmd = &input->m_pVerifiedCommands[sequence_number % MULTIPLAYER_BACKUP];
	if (crc != verified_cmd->m_crc)
		*usercmd = verified_cmd->m_cmd;
}

bool WriteUsercmdDeltaToBuffer_Rebuilt(int slot, void* buf, int from, int to, bool isnewcommand, bool forcevalid)
{
	bf_write *buffer = (bf_write*)buf;
	CUserCmd nullcmd;
	memset(&nullcmd, 0, sizeof(CUserCmd));

	CUserCmd *f, *t;

	int startbit = buffer->GetNumBitsWritten();

	if (from == -1)
	{
		f = &nullcmd;
	}
	else
	{
		f = GetUserCmd(slot, from, forcevalid);

		if (!f)
		{
			// DevMsg( "WARNING! User command delta too old (from %i, to %i)\n", from, to );
			f = &nullcmd;
		}
		else 
		{
			if (!forcevalid)
				ValidateUserCmd(Interfaces::Input, f, from);
		}
	}

	t = GetUserCmd(slot, to, forcevalid);

	if (!t) 
	{
		// DevMsg( "WARNING! User command too old (from %i, to %i)\n", from, to );
		t = &nullcmd;
	}
	else 
	{
		if (!forcevalid)
			ValidateUserCmd(Interfaces::Input, t, to);
	}

	// Write it into the buffer
	WriteUsercmd(buffer, t, f);

	if (buffer->IsOverflowed()) 
	{
		int endbit = buffer->GetNumBitsWritten();

#ifdef _DEBUG
		printf("WARNING! User command buffer overflow(%i %i), last cmd was %i bits long\n", from, to, endbit - startbit);
#endif

		return false;
	}

	return true;
}

void WriteUsercmd_Force(bf_write* buf, CUserCmd* to, CUserCmd* from);
extern bool ResetCLC_Move_Variables;
bool DisableCLC_Move = false;
extern int firstcmdnumber;

void CL_SendMove_Rebuilt() 
{
	VMP_BEGINMUTILATION("CLSNDMVE")
	byte data[4000];

	int nextcommandnr = g_ClientState->lastoutgoingcommand + g_ClientState->chokedcommands + 1;

	if (g_SplitScreenMgr->IsDisconnecting(0) || DisableCLC_Move)
		return;

	CLC_Move moveMsg;

	moveMsg.m_DataOut.StartWriting(data, sizeof(data));

	bool isvalve = GetGamerules() && GetGamerules()->IsValveServer();
	if (isvalve && !LocalPlayer.IsAllowedUntrusted())
	{
		LocalPlayer.ApplyTickbaseShift = false;
		return; //allow game's own CL_SendMove to be used
	}

	const int max_new_commands = 17;// : 15;

	static int lastservertick = g_ClientState->m_ClockDriftMgr.m_nServerTick;
	int servertick = g_ClientState->m_ClockDriftMgr.m_nServerTick;

	bool received_server_update = servertick != lastservertick;

	lastservertick = servertick;

	// Determine number of backup commands to send along
	moveMsg.m_nBackupCommands = 2;

	// How many real new commands have queued up
	moveMsg.m_nNewCommands = 1 + g_ClientState->chokedcommands;

	moveMsg.m_nNewCommands = min(max(moveMsg.m_nNewCommands, 0), max_new_commands);

	auto& var = variable::get();

	if (var.ragebot.exploits.b_hide_record || var.ragebot.exploits.b_hide_shots || var.ragebot.exploits.b_multi_tap.get() || var.ragebot.exploits.b_nasa_walk.get())
		moveMsg.m_nBackupCommands = 0;

	int numcmds = moveMsg.m_nNewCommands + moveMsg.m_nBackupCommands;

#if 0
		int numfake = 0;
		if (LocalPlayer.ApplyTickbaseShift)
			numfake = g_Tickbase.m_iNumFakeCommandsToSend;

		int totalcmds = moveMsg.m_nNewCommands + numfake;
		int cmdsleft = 17 - totalcmds;

		moveMsg.m_nBackupCommands = clamp(cmdsleft, 0, 2);

		numcmds = moveMsg.m_nNewCommands + moveMsg.m_nBackupCommands;

		if (moveMsg.m_nBackupCommands)
		{
			int numbackupstocheck = moveMsg.m_nBackupCommands;

			int to = nextcommandnr - numcmds + 1;
			CUserCmd* tocmd = GetUserCmd(0, to);
			if (!tocmd || tocmd->tick_count > Interfaces::Globals->tickcount + sv_max_usercmd_future_ticks.GetVar()->GetInt())
			{
				--moveMsg.m_nBackupCommands; //don't send this backup command
				numcmds = moveMsg.m_nNewCommands + moveMsg.m_nBackupCommands;
			}

			--numbackupstocheck;
			++to;

			if (numbackupstocheck)
			{
				tocmd = GetUserCmd(0, to);
				if (!tocmd || tocmd->tick_count > Interfaces::Globals->tickcount + sv_max_usercmd_future_ticks.GetVar()->GetInt())
				{
					moveMsg.m_nBackupCommands = 0; //can't send any backup commands
					numcmds = moveMsg.m_nNewCommands + moveMsg.m_nBackupCommands;
				}
			}
		}
#endif

	int from = -1;	// first command is deltaed against zeros 

	bool bOK = true;

	int _originalTicksAllowedForProcessing = g_Tickbase.m_iTicksAllowedForProcessing;

	for (int to = nextcommandnr - numcmds + 1; to <= nextcommandnr; to++)
	{
		bool isnewcmd = to >= (nextcommandnr - moveMsg.m_nNewCommands + 1);

		// first valid command number is 1
		CUserCmd* tocmd = GetUserCmd(0, to);

		if (tocmd && tocmd->tick_count <= Interfaces::Globals->tickcount + sv_max_usercmd_future_ticks.GetVar()->GetInt())
		{
			bOK = bOK && WriteUsercmdDeltaToBuffer_Rebuilt(0, &moveMsg.m_DataOut, from, to, isnewcmd, false); //WriteUsercmdDeltaToBuffer((void*)g_csgo.m_input2, 0, &moveMsg.m_DataOut, from, to, isnewcmd);
#ifdef _DEBUG
			//if (!g_Tickbase.m_iTicksAllowedForProcessing)
			//	printf("ERROR: no ticks allowed for processing but running command anyway!\n");
#endif
			g_Tickbase.m_iTicksAllowedForProcessing = max(0, g_Tickbase.m_iTicksAllowedForProcessing - 1);
			//g_Tickbase.m_iDummyCommandsToProcess = 0;

#if 0
			if (to == nextcommandnr && g_Tickbase.m_iTicksAllowedForProcessing > 0 && GetAsyncKeyState(VK_RETURN))
			{
				for (int i = 0; i < g_Tickbase.m_iTicksAllowedForProcessing; ++i)
				{
					moveMsg.m_nNewCommands++;
					bOK = bOK && WriteUsercmdDeltaToBuffer_Rebuilt(0, &moveMsg.m_DataOut, from, to, isnewcmd, false);
					g_Tickbase.m_iTicksAllowedForProcessing = max(0, g_Tickbase.m_iTicksAllowedForProcessing - 1);
				}
			}
#endif
			from = to;
		}
		else
		{
			//printf("WARNING: Client delta out of sync\n");
			if (isnewcmd)
				--moveMsg.m_nNewCommands;
			else
				--moveMsg.m_nBackupCommands;

			--numcmds;
		}
	}


	if (LocalPlayer.ApplyTickbaseShift)
	{
#if defined _DEBUG || defined INTERNAL_DEBUG
		int clockcorrect = TIME_TO_TICKS(0.03f); //sv_clockcorrectmsecs
		int old_tickbase = LocalPlayer.m_nTickBase_Pristine;

		int tickssincereceive = TIME_TO_TICKS(QPCTime() - g_Tickbase.m_flLastServerPacketReceiveTime);
#ifdef _DEBUG
		if (tickssincereceive != 1)
		{
			g_Eventlog.PrintToConsole(Color(255, 0, 0, 255), "CLC_Move: tickssincereceive delta not 1, was %i\n", tickssincereceive);
			printf("CLC_Move: tickssincereceive delta not 1, was %i\n", tickssincereceive);
		}
#endif
		int latencyticks = max(0, TIME_TO_TICKS(Interfaces::EngineClient->GetNetChannelInfo()->GetAvgLatency(FLOW_OUTGOING)));
		int tickb = g_ClientState->m_ClockDriftMgr.m_nServerTick + latencyticks + 1;
#ifdef _DEBUG
		printf("server tickcount %i latencyticks %i maxprocessable %i\n", tickb, latencyticks, _originalTicksAllowedForProcessing);
		printf("client - server tick delta %i\n", Interfaces::Globals->tickcount - g_Tickbase.m_iLastServerPacketReceiveTickcount);
#endif
		//int maxseq = g_ClientState->last_command_ack + min(g_ClientState->chokedcommands + 1, 150);
		//if (maxseq > g_ClientState->last_command_ack)
		//	tickb += maxseq - g_ClientState->last_command_ack;

		int	nIdealFinalTick = tickb + clockcorrect;

		int simulation_ticks = moveMsg.m_nNewCommands + g_Tickbase.m_iNumFakeCommandsToSend;

		int nEstimatedFinalTick = old_tickbase + simulation_ticks;

		// If client gets ahead of this, we'll need to correct
		int	 too_fast_limit = nIdealFinalTick + clockcorrect;
		// If client falls behind this, we'll also need to correct
		int	 too_slow_limit = nIdealFinalTick - clockcorrect;

		//if (nEstimatedFinalTick > too_fast_limit ||
		//	nEstimatedFinalTick < too_slow_limit)
		{
			int nCorrectedTick = nIdealFinalTick - simulation_ticks + 1;
			int nFinalTickbase = nCorrectedTick + moveMsg.m_nNewCommands;
			int old_tickbase = g_Tickbase.m_iCalculatedTickbase[g_ClientState->lastoutgoingcommand % 150];
			printf("predicted start simulationtime %f\npredicted end simulationtime %f\nshifted back by %i ticks oldsimtime %f\n", 
				TICKS_TO_TIME(nCorrectedTick),
				TICKS_TO_TIME(nFinalTickbase),
				TIME_TO_TICKS(TICKS_TO_TIME(old_tickbase) - TICKS_TO_TIME(nCorrectedTick )),
				TICKS_TO_TIME(old_tickbase));
		}
#endif

		if (g_Tickbase.m_iNumFakeCommandsToSend && g_Tickbase.m_iTicksAllowedForProcessing)
		{
			CUserCmd *last_real_cmd = GetUserCmd(0, from);

			if (last_real_cmd != nullptr)
			{
				moveMsg.m_nNewCommands += g_Tickbase.m_iNumFakeCommandsToSend;

				auto from_cmd = *last_real_cmd;
				auto to_cmd = from_cmd;
				to_cmd.command_number++;
				to_cmd.tick_count += 300;

				for (int i = 0; i < g_Tickbase.m_iNumFakeCommandsToSend; ++i)
				{
					WriteUsercmd(&moveMsg.m_DataOut, &to_cmd, &from_cmd);
					from_cmd = to_cmd;
					to_cmd.command_number++;
					to_cmd.tick_count++;
				}
			}
		}

		LocalPlayer.ApplyTickbaseShift = false;
	}

#ifdef USE_SERVER_SIDE
	if (LocalPlayer.m_RealMatrixBackups.getsize())
	{
		for (BackupMatrixStruct* pk = LocalPlayer.m_RealMatrixBackups.rbegin<BackupMatrixStruct>(); pk != LocalPlayer.m_RealMatrixBackups.rend<BackupMatrixStruct>(); --pk)
		{
			if (pk->m_TicksAllowedForProcessing == -999)
				pk->m_TicksAllowedForProcessing = g_Tickbase.m_iTicksAllowedForProcessing;
			else
				break;
		}
	}
#endif

	if (bOK)
	{
		moveMsg.FinishProtobuf();

		// only write message if all usercmds were written correctly, otherwise parsing would fail
		((INetChannel*)g_ClientState->m_pNetChannel)->SendNetMsg(&moveMsg, false, false);
	}
	VMP_END
}