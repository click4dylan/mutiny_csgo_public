#pragma once

class GOTV
{
public:
	class CHLTVFrame;

	class CHLTVDemoRecorder
	{
	public:
#if 0
		char _pad[0x540];
		bool m_bIsRecording;
		int m_nFrameCount;
		float m_nStartTick;
		int m_SequenceInfo;
		int m_nDeltaTick;
		int m_nSignonTick;
		/*bf_write*/char* m_MessageData; // temp buffer for all network messages
#endif
		//CDemoFile m_DemoFile; //0x0004 
		char _pad[0x540];
		unsigned char m_bIsRecording; //0x053C 
		char pad_0x053D[0x3]; //0x053D
		__int32 m_nFrameCount; //0x0540 
		float m_nStartTick; //0x0544 
		__int32 m_SequenceInfo; //0x0548 
		__int32 m_nDeltaTick; //0x054C 
		__int32 m_nSignonTick; //0x0550 
		char m_MessageData[0x18];
		//bf_write m_MessageData; //0x0554 
		__int32 host_tickcount; //0x056C 
		/*CGameServer**/void* m_Server; //0x0570 
		/*IHLTVDirector**/void* m_Director; //0x0574 
		__int32 m_nFirstTick; //0x0578 
		__int32 m_nLastTick; //0x057C 
		CHLTVFrame* m_CurrentFrame; //0x0580 
		__int32 m_nViewEntity; //0x0584 
		__int32 m_nPlayerSlot; //0x0588 
	};

	class CHLTVServer
	{
	public:
		char _pad[0x5040];
		CHLTVDemoRecorder m_DemoRecorder;
	};
};


class AntiGOTV
{
public:
	bool GOTVIsRecording(int tick);
};

//extern AntiGOTV gAntiGOTV;