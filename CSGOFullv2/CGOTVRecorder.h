#pragma once
#include "CreateMove.h"
#include "bfwrite.h"
// Generated using ReClass 2015

#if 0

class CDemoRecorder;
class bf_write;
class CDemoFile;
class ppDemoRecorder;
class CHLTVDemoRecorder;
class IHLTVDirector;
class CGameServer;
class CHLTVFrame;
class MAIN;
class CHLTVServer;
class N00000930;
class N00000943;
class N0000095D;
class N00000970;
class N00000983;
class N00000986;

class CDemoFile
{
public:
	char m_szFileName[260]; //0x0000 
	char demofilestamp[8]; //0x0104 
	__int32 demoprotocol; //0x010C 
	__int32 networkprotocol; //0x0110 
	char servername[260]; //0x0114 
	char clientname[260]; //0x0218 
	char mapname[260]; //0x031C 
	char gamedirectory[260]; //0x0420 
	float playback_time; //0x0524 
	__int32 playback_ticks; //0x0528 
	__int32 playback_frames; //0x052C 
	__int32 signonlength; //0x0530 
	DWORD m_hDemoFile; //0x0534 

};//Size=0x0538

class ppDemoRecorder
{
public:
	CDemoRecorder* pDemoRecorder; //0x0000 
	char pad_0x0004[0x3C]; //0x0004

};//Size=0x0040

class CHLTVDemoRecorder
{
public:
	virtual CDemoFile* GetDemoFile(); //
	virtual int GetRecordingTick(); //
	virtual void StartRecording(const char *filename, bool bContinuously); //
	virtual void SetSignonState(int state); //not needed by HLTV recorder
	virtual bool IsRecording(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	CDemoFile m_DemoFile; //0x0004 
	unsigned char m_bIsRecording; //0x053C 
	char pad_0x053D[0x3]; //0x053D
	__int32 m_nFrameCount; //0x0540 
	float m_nStartTick; //0x0544 
	__int32 m_SequenceInfo; //0x0548 
	__int32 m_nDeltaTick; //0x054C 
	__int32 m_nSignonTick; //0x0550 
	bf_write m_MessageData; //0x0554 
	__int32 host_tickcount; //0x056C 
	CGameServer* m_Server; //0x0570 
	IHLTVDirector* m_Director; //0x0574 
	__int32 m_nFirstTick; //0x0578 
	__int32 m_nLastTick; //0x057C 
	CHLTVFrame* m_CurrentFrame; //0x0580 
	__int32 m_nViewEntity; //0x0584 
	__int32 m_nPlayerSlot; //0x0588 
	char pad_0x058C[0x2B4]; //0x058C

};//Size=0x0840

class IHLTVDirector
{
public:
	char pad_0x0000[0x44]; //0x0000

};//Size=0x0044

class CGameServer
{
public:
	char pad_0x0000[0x24]; //0x0000

};//Size=0x0024

class CHLTVFrame
{
public:
	char pad_0x0000[0x44]; //0x0000

};//Size=0x0044

class MAIN
{
public:
	char pad_0x0000[0xCE8]; //0x0000
	CHLTVServer* m_pHLTV; //0x0CE8 
	char pad_0x0CEC[0x8]; //0x0CEC
	float somefloat; //0x0CF4 
	char pad_0x0CF8[0xB48]; //0x0CF8

};//Size=0x1840

class CHLTVServer
{
public:
	char pad_0x0000[0x5044]; //0x0000

};//Size=0x5044

class CDemoRecorder
{
public:
	virtual CDemoFile* GetDemoFile(); //
	virtual int GetRecordingTick(); //
	virtual void StartRecording(const char *filename, bool bContinuously); //
	virtual void Function3(); //
	virtual void Function4(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	CDemoFile m_DemoFile; //0x0004 
	__int32 m_nStartTick; //0x053C 
	char m_szDemoBaseName[260]; //0x0540 
	unsigned char m_bIsDemoHeader; //0x0644 
	unsigned char m_bCloseDemoFile; //0x0645 
	unsigned char m_bRecording; //0x0646 
	unsigned char m_bContinuously; //0x0647 
	__int32 m_nDemoNumber; //0x0648 
	__int32 m_nFrameCount; //0x064C 
	bf_write m_MessageData; //0x0650 
	unsigned char m_bResetInterpolation; //0x0668 
	char pad_0x0669[0x13]; //0x0669

};//Size=0x067C

#endif