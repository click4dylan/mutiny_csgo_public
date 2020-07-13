#include "precompiled.h"
#include "INetchannelInfo.h"
#include "Netchan.h"
#include "misc.h"
#include "LocalPlayer.h"

INetChannel our_netchan;
INetChannel server_netchan;
INetChannel client_netchan;

#define NET_FRAMES_MASK 127
#define NET_FRAMES_BACKUP 128
// How fast to converge flow estimates
#define FLOW_AVG ( 3.0 / 4.0 )
// Don't compute more often than this
#define FLOW_INTERVAL 0.25

double flNextTime = 0.f;
void UpdateCustomNetchannels()
{
	const bool bNetchanIsHooked = HMessageHandler && HNetchan && HNetchan->GetCurrentVT() == HNetchan->GetNewVT();
	if (bNetchanIsHooked)
	{
		if (flNextTime == 0.f)
			flNextTime = QPCTime();
		const double time = QPCTime();
		if (time >= flNextTime)
		{
			const float commandInterval = Interfaces::Globals->interval_per_tick;//1.0f / cl_cmdrate->GetFloat();
			const double maxDelta = (double)fminf(Interfaces::Globals->interval_per_tick, commandInterval);
			const float delta = (float)clamp(time - flNextTime, 0.0, maxDelta);
			flNextTime = time + commandInterval - delta;

			const int nTotalSize = 50 + UDP_HEADER_SIZE;
			FlowNewPacket(&server_netchan, FLOW_OUTGOING, server_netchan.m_nOutSequenceNr, server_netchan.m_nInSequenceNr, server_netchan.m_nChokedPackets, 0, nTotalSize);
			FlowUpdate(&server_netchan, FLOW_OUTGOING, nTotalSize);
			LocalPlayer.Last_Server_ChokeCountSent = server_netchan.m_nChokedPackets;
			LocalPlayer.Last_Server_InSequenceNrSent = server_netchan.m_nInSequenceNr;
			LocalPlayer.Last_Server_OutSequenceNrSent = server_netchan.m_nOutSequenceNr;

			server_netchan.m_nChokedPackets = 0;
			server_netchan.m_nOutSequenceNr++;
		}

		if (LocalPlayer.Last_Server_OutSequenceNrSent > client_netchan.m_nInSequenceNr)
		{
			FlowUpdate(&client_netchan, FLOW_INCOMING, 50 + UDP_HEADER_SIZE);

			// dropped packets don't keep the message from being used
			client_netchan.m_PacketDrop = LocalPlayer.Last_Server_OutSequenceNrSent - (client_netchan.m_nInSequenceNr + LocalPlayer.Last_Server_ChokeCountSent + 1);

			client_netchan.m_nInSequenceNr = LocalPlayer.Last_Server_OutSequenceNrSent;
			client_netchan.m_nOutSequenceNrAck = LocalPlayer.Last_Server_InSequenceNrSent;

			FlowNewPacket(&client_netchan, FLOW_INCOMING, client_netchan.m_nInSequenceNr, client_netchan.m_nOutSequenceNrAck, LocalPlayer.Last_Server_ChokeCountSent, client_netchan.m_PacketDrop, 50 + UDP_HEADER_SIZE);
		}

		if (LocalPlayer.Last_OutSequenceNrSent > server_netchan.m_nInSequenceNr)
		{
			FlowUpdate(&server_netchan, FLOW_INCOMING, LocalPlayer.Last_Packet_Size);

			// dropped packets don't keep the message from being used
			server_netchan.m_PacketDrop = LocalPlayer.Last_OutSequenceNrSent - (server_netchan.m_nInSequenceNr + LocalPlayer.Last_ChokeCountSent + 1);

			server_netchan.m_nInSequenceNr = LocalPlayer.Last_OutSequenceNrSent;
			server_netchan.m_nOutSequenceNrAck = LocalPlayer.Last_InSequenceNrSent;

			FlowNewPacket(&server_netchan, FLOW_INCOMING, server_netchan.m_nInSequenceNr, server_netchan.m_nOutSequenceNrAck, LocalPlayer.Last_ChokeCountSent, server_netchan.m_PacketDrop, LocalPlayer.Last_Packet_Size);
		}
	}
}

//Rebuilt functions from netchannel

void FlowNewPacket(INetChannel *chan, int flow, int seqnr, int acknr, int nChoked, int nDropped, int nSize)
{
	return;


	//netflow_t * m_DataFlow = (netflow_t*)((DWORD)chan + 0x5C0);
	netflow_t * pflow = &chan->m_DataFlow[flow];

	// if frame_number != ( current + 1 ) mark frames between as invalid

	netframe_t_firstpart *pfirstpart = NULL;
	netframe_t_secondpart *psecondpart = NULL;

	if (seqnr > pflow->currentindex)
	{
		int framecount = 0;
		for (int i = pflow->currentindex + 1; i <= seqnr && framecount < NET_FRAMES_BACKUP; i++)
		{
			int nBackTrack = seqnr - i;

			pfirstpart = &pflow->frames[i & NET_FRAMES_MASK];
			pfirstpart->time = *net_time;	// now
			pfirstpart->valid = false;
			pfirstpart->size = 0;
			pfirstpart->latency = -1.0f; // not acknowledged yet
			pfirstpart->choked = 0; // not acknowledged yet

			psecondpart = &pflow->frames2[i & NET_FRAMES_MASK];
			psecondpart->avg_latency = chan->GetAvgLatency(FLOW_OUTGOING);
			psecondpart->dropped = 0;
			psecondpart->m_flInterpolationAmount = 0.0f;
			memset(&psecondpart->msggroups, 0, sizeof(psecondpart->msggroups));

			if (nBackTrack < (nChoked + nDropped))
			{
				if (nBackTrack < nChoked)
					pfirstpart->choked = 1;
				else
					psecondpart->dropped = 1;
			}
			framecount++;
		}

		psecondpart->dropped = nDropped;
		pfirstpart->choked = nChoked;
		pfirstpart->size = nSize;
		pfirstpart->valid = true;
		psecondpart->avg_latency = chan->GetAvgLatency(FLOW_OUTGOING);
		psecondpart->m_flInterpolationAmount = chan->m_flInterpolationAmount;
	}
	else
	{
		//Assert(demoplayer->IsPlayingBack() || seqnr > pflow->currentindex);
	}

	pflow->totalpackets++;
	pflow->currentindex = seqnr;
	pflow->currentframe = pfirstpart;

	// updated ping for acknowledged packet

	int aflow = (flow == FLOW_OUTGOING) ? FLOW_INCOMING : FLOW_OUTGOING;

	if (acknr <= (chan->m_DataFlow[aflow].currentindex - NET_FRAMES_BACKUP))
		return;	// acknowledged packet isn't in backup buffer anymore

	netframe_t_firstpart * aframe = &chan->m_DataFlow[aflow].frames[acknr & NET_FRAMES_MASK];

	if (aframe->valid && aframe->latency == -1.0f)
	{
		// update ping for acknowledged packet, if not already acknowledged before

		aframe->latency = *net_time - aframe->time;

		if (aframe->latency < 0.0f)
			aframe->latency = 0.0f;
	}

}

void FlowUpdate(INetChannel* chan, int flow, int addbytes)
{
	//netflow_t * m_DataFlow = (netflow_t*)((DWORD)chan + 0x5C0);
	netflow_t * pflow = &chan->m_DataFlow[flow];
	pflow->totalbytes += addbytes;

	if (pflow->nextcompute > *net_time)
		return;

	pflow->nextcompute = *net_time + FLOW_INTERVAL;

	int		totalvalid = 0;
	int		totalinvalid = 0;
	int		totalbytes = 0;
	float	totallatency = 0.0f;
	int		totallatencycount = 0;
	int		totalchoked = 0;

	float   starttime = FLT_MAX;
	float	endtime = 0.0f;

	//netframe_t_firstpart *pprev = &pflow->frames[NET_FRAMES_BACKUP - 1];

	for (int i = 0; i < NET_FRAMES_BACKUP; i++)
	{
		// Most recent message then backward from there
		netframe_t_firstpart * pcurr = &pflow->frames[i];

		if (pcurr->valid)
		{
			if (pcurr->time < starttime)
				starttime = pcurr->time;

			if (pcurr->time > endtime)
				endtime = pcurr->time;

			totalvalid++;
			totalchoked += pcurr->choked;
			totalbytes += pcurr->size;

			if (pcurr->latency > -1.0f)
			{
				totallatency += pcurr->latency;
				totallatencycount++;
			}
		}
		else
		{
			totalinvalid++;
		}

		//pprev = pcurr;
	}

	float totaltime = endtime - starttime;

	if (totaltime > 0.0f)
	{
		pflow->avgbytespersec *= FLOW_AVG;
		pflow->avgbytespersec += (1.0f - FLOW_AVG) * ((float)totalbytes / totaltime);

		pflow->avgpacketspersec *= FLOW_AVG;
		pflow->avgpacketspersec += (1.0f - FLOW_AVG) * ((float)totalvalid / totaltime);
	}

	int totalPackets = totalvalid + totalinvalid;

	if (totalPackets > 0)
	{
		pflow->avgloss *= FLOW_AVG;
		pflow->avgloss += (1.0f - FLOW_AVG) * ((float)(totalinvalid - totalchoked) / totalPackets);

		if (pflow->avgloss < 0)
			pflow->avgloss = 0;

		pflow->avgchoke *= FLOW_AVG;
		pflow->avgchoke += (1.0f - FLOW_AVG) * ((float)totalchoked / totalPackets);
	}
	else
	{
		pflow->avgloss = 0.0f;
	}

	if (totallatencycount>0)
	{
		float newping = totallatency / totallatencycount;
		pflow->latency = newping;
		pflow->avglatency *= FLOW_AVG;
		pflow->avglatency += (1.0f - FLOW_AVG) * newping;
	}
}

float INetChannel::GetTime() {
	return *net_time;
};
float INetChannel::GetTimeConnected()
{
	float t = *net_time - connect_time;
	return (t>0.0f) ? t : 0.0f;
}
float INetChannel::GetTimeSinceLastReceived()
{
	float t = *net_time - last_received;
	return (t>0.0f) ? t : 0.0f;
}
int INetChannel::GetDataRate()
{
	return m_Rate;
}
int INetChannel::GetBufferSize()
{
	return NET_FRAMES_BACKUP;
}
bool INetChannel::IsLoopback()
{
	return remote_address.IsLoopback();
}
bool INetChannel::IsNull()
{
	return remote_address.GetType() == NA_NULL ? true : false;
}
#define CONNECTION_PROBLEM_TIME 4.0f
bool INetChannel::IsTimingOut()
{
	if (m_Timeout == -1.0f)
		return false;
	else
		return (last_received + CONNECTION_PROBLEM_TIME) < *net_time;
}
bool INetChannel::IsPlayback()
{
	return false;
}
float INetChannel::GetLatency(int flow)
{
	return m_DataFlow[flow].latency;
}
float INetChannel::GetAvgLatency(int flow)
{
	return m_DataFlow[flow].avglatency;
}
float INetChannel::GetAvgLoss(int flow)
{
	return m_DataFlow[flow].avgloss;
}
float INetChannel::GetAvgData(int flow)
{
	return m_DataFlow[flow].avgbytespersec;
}
float INetChannel::GetAvgChoke(int flow)
{
	return m_DataFlow[flow].avgchoke;
}
float INetChannel::GetAvgPackets(int flow)
{
	return m_DataFlow[flow].avgpacketspersec;
}
int INetChannel::GetTotalData(int flow)
{
	return m_DataFlow[flow].totalbytes;
}
int INetChannel::GetSequenceNr(int flow)
{
	if (flow == FLOW_OUTGOING)
	{
		return m_nOutSequenceNr;
	}
	else if (flow == FLOW_INCOMING)
	{
		return m_nInSequenceNr;
	}

	return 0;
}
bool INetChannel::IsValidPacket(int flow, int frame_number)
{
	return m_DataFlow[flow].frames[frame_number & NET_FRAMES_MASK].valid;
}
float INetChannel::GetPacketTime(int flow, int frame_number)
{
	return m_DataFlow[flow].frames[frame_number & NET_FRAMES_MASK].time;
}
int	INetChannel::GetPacketBytes(int flow, int frame_number, int group)
{
	if (group >= 16)
	{
		return m_DataFlow[flow].frames[frame_number & NET_FRAMES_MASK].size;
	}
	else
	{
		return Bits2Bytes(m_DataFlow[flow].frames2[frame_number & NET_FRAMES_MASK].msggroups[group]);
	}
}
bool INetChannel::GetStreamProgress(int flow, int *received, int *total)
{
	return false; // TODO TCP progress
}
float INetChannel::GetCommandInterpolationAmount(int flow, int frame_number)
{
	return m_DataFlow[flow].frames2[frame_number & NET_FRAMES_MASK].m_flInterpolationAmount;
}
void INetChannel::GetPacketResponseLatency(int flow, int frame_number, int *pnLatencyMsecs, int *pnChoke)
{
	const netframe_t_secondpart &nf = m_DataFlow[flow].frames2[frame_number & NET_FRAMES_MASK];
	if (pnLatencyMsecs)
	{
		if (nf.dropped)
		{
			*pnLatencyMsecs = 9999;
		}
		else
		{
			*pnLatencyMsecs = (int)(1000.0f * nf.avg_latency);
		}
	}
	if (pnChoke)
	{
		*pnChoke = m_DataFlow[flow].frames[frame_number & NET_FRAMES_MASK].choked;
	}
}
void INetChannel::GetRemoteFramerate(float *pflFrameTime, float *pflRemoteFrameTimeStdDeviation)
{
	if (pflFrameTime)
	{
		*pflFrameTime = m_flRemoteFrameTime;
	}
	if (pflRemoteFrameTimeStdDeviation)
	{
		*pflRemoteFrameTimeStdDeviation = m_flRemoteFrameTimeStdDeviation;
	}
}
float INetChannel::GetTimeoutSeconds()
{
	return m_Timeout;
}
void INetChannel::Func25() {};
void INetChannel::Func26() {};
void INetChannel::Func27() {};
void INetChannel::Func28() {};
void INetChannel::Func29() {};
void INetChannel::Func30() {};
void INetChannel::Func31() {};
void INetChannel::Func32() {};
void INetChannel::Func33() {};
void INetChannel::Func34() {};
void INetChannel::Func35() {};
void INetChannel::Func36() {};
void INetChannel::Func37() {};
void INetChannel::Func38() {};
//void INetChannel::Func39() {};
//void INetChannel::Func40() {};
void INetChannel::ProcessPacket(netpacket_t* packet, bool bHasHeader) {};
void INetChannel::SendNetMsg(void* msg, bool bForceReliable, bool bVoice) {};
void INetChannel::Func41() {};
void INetChannel::Func42() {};
void INetChannel::Func43() {};
void INetChannel::Func44() {};
void INetChannel::Func45() {};
int INetChannel::SendDatagram(void* datagram) { return 0; };
bool INetChannel::Transmit(bool bOnlyReliable) { return 0; };
void INetChannel::Func48() {};
void INetChannel::Func49() {};
void INetChannel::Func50() {};
void INetChannel::Func51() {};
void INetChannel::Func52() {};
void INetChannel::Func53() {};
void INetChannel::Func54() {};
void INetChannel::Func55() {};
bool INetChannel::CanPacket() { return false; };