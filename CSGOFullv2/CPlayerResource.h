#pragma once
#include "misc.h"
class CPlayerResource
{
public: // IGameResources intreface

		//// Team data access
		//virtual int           GetTeamScore(int index); //0
		//virtual const char *GetTeamName(int index); //4
		//virtual void      GetTeamColor(int index); //8

		//                                           // Player data access
		//virtual void  pad();                      //C
		//virtual bool  IsConnected(int index);// 10
		//virtual bool  IsAlive(int index); //14
		//virtual bool  IsFakePlayer(int index); //18
		//virtual bool  IsLocalPlayer(int index); //1C
		//                                        //virtual bool    IsHLTV(int index); // 20

		//virtual const char *GetPlayerName(int index); //20
		//virtual void  pad1(); // 24
		//virtual int       GetPing(int index); // 28
		//virtual int       GetDeaths(int index); // 0x2C
		//virtual int       GetFrags(int index); // 30 

		//virtual int       GetPacketloss(int index); //28
		//virtual int       GetPlayerScore(int index); // 2C
		//                                         //30
		//virtual int       GetTeam(int index); // 34
		//                                  // 38
		//virtual int       GetHealth(int index);

		//virtual void ClientThink();
		//virtual   void    OnDataChanged();

		// Data for each player that's propagated to all clients
		// Stored in individual arrays so they can be sent down via datatables
		/*char*     m_szName[65];
		int         m_iPing[65];
		int         m_iKills[65];
		int         m_iAssists[65];
		int         m_iDeaths[65];
		bool        m_bConnected[65];
		int         m_iTeam[65];
		int         m_iPendingTeam[65];
		bool        m_bAlive[65];
		int         m_iHealth[65];*/

	//char pad_0x0000[0x8]; //0x0000
	//char* m_szName[65]; //0x0008
	//int m_iPing[65]; //0x010C
	//int m_iKills[65]; //0x0210
	//int m_iAssists[65]; //0x0314
	//int m_iDeaths[65]; //0x0418
	//bool m_bConnected[65]; //0x051C
	//char pad_0x055D[0x3]; //0x055D
	//int m_iTeam[65]; //0x0560
	//int m_iPendingTeam[65]; //0x0664
	//bool m_bAlive[65]; //0x0768
	//char pad_0x07A9[0x3]; //0x07A9
	//int _m_iHealth[65]; //0x07AC
	//char pad_0x08B0[0x80]; //0x08B0
	//int m_iCoachingTeam[65]; //0x0930
	//char pad_0x0A34[0x218]; //0x0A34
	//Vector m_bombsiteCenterA; //0x0C4C
	//Vector m_bombsiteCenterB; //0x0C58
	//char pad_0x0C64[0x17C]; //0x0C64
	//bool _m_bHasDefuser[65]; //0x0DE0
	//bool _m_bHasHelmet[65]; //0x0E21
	//char pad_0x0E62[0x2]; //0x0E62
	//int m_iArmour[65]; //0x0E64
	//int m_iScore[65]; //0x0F68
	//int m_iCompetetiveRanking[65]; //0x106C
	//int m_iCompetetiveWins[65]; //0x1170
	//int m_iCompTeammateColor[65]; //0x1274
	//bool m_bControllingBot[65]; //0x1378
	//char pad_0x13B9[0x3]; //0x13B9
	//int m_iControlledPlayer[65]; //0x13BC
	//int m_iControlledByPlayer[65]; //0x14C0

	// m_szClan
	const char* GetClan(int id);
};

extern CPlayerResource* g_PR;