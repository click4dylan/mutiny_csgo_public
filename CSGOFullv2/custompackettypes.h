#pragma once
#include <stdint.h>

enum FARESP_PLAYERPACKET_TYPE : uint8_t
{
	FULL = 0,
	PARTIAL
};

enum FARESP_PACKET_TYPE : uint8_t
{
	ERRORPACKET = 0,
	CLIENTTOSERVER,
	SERVERTOCLIENT
};

enum CSGO_PACKET_TYPE : uint8_t
{
	BACKTRACK = 0,
	REALTIME,
	FIREBULLET
};