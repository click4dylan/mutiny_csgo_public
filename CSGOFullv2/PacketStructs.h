#pragma once
#include "custompackettypes.h"

#pragma pack(push, 1)
struct hitboxinfo
{
	float flRadius;
	Vector position;
	QAngle angles;
	Vector vMin;
	Vector vMax;
};
#pragma pack(pop)

#define KEEPALIVE_VERIFY (int)80085
#pragma pack(push, 1)
class KeepAlivePacket
{
public:
	KeepAlivePacket::KeepAlivePacket()
	{
		size = sizeof(KeepAlivePacket);
		verify = KEEPALIVE_VERIFY;
	}
	KeepAlivePacket::~KeepAlivePacket() {}
	uint32_t size; //size of packet
	int32_t verify;
};
#pragma pack(pop)

#define CSGOPACKET_VERIFY (int32_t)8008
#pragma pack(push, 1)
class CSGOPacket
{
public:
	CSGOPacket::CSGOPacket()
	{
		size = sizeof(CSGOPacket);
		verify = CSGOPACKET_VERIFY;
	}
	CSGOPacket::~CSGOPacket() {}
private:
	uint32_t size; //size of packet
public:
	int32_t verify;
	CSGO_PACKET_TYPE typeofpacket;
	uint32_t hashedsteamid;
	BOOLEAN bAlive;
	BOOLEAN bShiftedTickbaseBackwards;
	uint8_t armor;
	int32_t health;
	uint8_t helmet;
	int32_t tickschoked;
	float goalfeetyaw;
	float flSimulationTime;
	float flFirstCmdCurTime;
	float flEndFrameCurTime;
	hitboxinfo hitboxes[20];
	//matrix3x4_t bonematrix[256];
	Vector absorigin;
	QAngle absangles;
	float flPoseParameters[MAX_CSGO_POSE_PARAMS];
	C_AnimationLayerFromPacket AnimLayer[MAX_OVERLAYS];
	Vector shoteyepos;
	QAngle shoteyeangles;
	QAngle shotpunchangle;
	int ticksallowedforprocessing;
};
#pragma pack(pop)


#pragma pack(push, 1)
struct FarESPPlayer
{
	bool islocalplayer : 1; //this indicates the player info was created by themself, which means they are perfectly resolved
	bool iswalking : 1;
	bool teleporting : 1;
	bool helmet : 1;
	bool ducking : 1;
	bool fakeducking : 1;
	bool strafing : 1;
	uint8_t movestate : 2;
	//all the above is 1 byte

	uint32_t hashedsteamid;
	uint32_t flags;

	//uint8_t entindex : 7; //0-127
	uint8_t movecollide : 3;
	uint8_t tickschoked : 5; //0-31
	uint8_t armor : 7;
	uint8_t movetype : 4;
	uint8_t relativedirectionoflastinjury : 3;
	uint8_t lasthitgroup : 4;

	int16_t weaponclassid;
	uint16_t weaponitemdefinitionindex;

	int32_t health;
	int32_t firecount;

	//localdata_t
	struct s_local_data
	{
		int32_t ducktimemsecs;
		int32_t duckjumptimemsecs;
		int32_t jumptimemsecs;
		float fallvelocity;
		float lastducktime;
		bool ducked : 1;
		bool ducking : 1;
		bool induckjump : 1;
	} localdata;

	Vector absorigin;
	Vector velocity;
	Vector laddernormal;
	Vector mins;
	Vector maxs;

	//animstate
	struct s_animstate
	{
		float totaltimeinair;
		float flashedstarttime;
		float flashedendtime;
		bool flashed : 1;
		bool onground : 1;
	} animstate;
	
	float duckamount;
	float eyepitch;
	float eyeyaw;
	float simulationtime;
	float absyaw;
	float maxspeed;
	float stamina;
	float velocitymodifier;
	float timenotonladder;
	float timeoflastinjury;
	float poseparameters[MAX_CSGO_POSE_PARAMS];
	
	C_AnimationLayerFromPacket animlayers[MAX_CSGO_ANIM_LAYERS];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PacketHeader
{
	uint32_t size;
	FARESP_PACKET_TYPE typeofpacket;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CompressedESPPacket
{
	PacketHeader header;
	uint32_t compressedsize;
	char data;
};
#pragma pack(pop)

