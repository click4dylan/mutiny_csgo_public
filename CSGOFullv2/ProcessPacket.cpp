#include "precompiled.h"
#include "VTHook.h"
#include "netadr.h"
#include "bfwrite.h"
#include "INetchannelInfo.h"
#include "Netchan.h"
#include "VMProtectDefs.h"
#include "tempents.h"
#include "beams.h"
#include "checksum_crc.h"
#include "LocalPlayer.h"

ProcessPacketFn oProcessPacket;
bool NextPacketShouldBeRealPing = false;

ShutdownNetchanFn oShutdownNetchan;

typedef struct netpacket_s
{
	netadr_t		from;		// sender IP //size 10 //0
	int				source;		// received source //10
	double			received;	// received time //18

	netadr_t		to;			// receiver IP //size 10 //26
	int				ssource;	// send source //36
	double			sendtime;	// send time //40
	
	unsigned char	*data;		// pointer to raw packet data //48
	bf_read			message;	// easy bitbuf data access //52 //size 36
	int				size;		// size in bytes //88
	int				wiresize;   // size in bytes before decompression //92
	bool			stream;		// was send as stream
	struct netpacket_s *pNext;	// for internal use, should be NULL in public
} netpacket_t;

struct bf_read_deconstructed
{
	char const * m_pDebugName; //52
	bool m_bOverflow; //56
	char pad[3];
	int m_nDataBits; //60
	size_t m_nDataBytes; //64

	uint32 m_nInBufWord; //68
	int m_nBitsAvail; //72
	uint32 const *m_pDataIn; //76
	uint32 const *m_pBufferEnd; //80
	uint32 const *m_pData; //84
};

typedef struct netpacket_t_deconstructed
{
	netadr_t		from;		// sender IP //size 10 //0
	int				source;		// received source //10
	double			received;	// received time //18

	netadr_t		to;			// receiver IP //size 10 //26
	int				ssource;	// send source //36
	double			sendtime;	// send time //40

	unsigned char	*data;		// pointer to raw packet data //48

	/*
	char const * m_pDebugName; //52
	bool m_bOverflow; //56
	char pad[3];
	int m_nDataBits; //60
	size_t m_nDataBytes; //64

	uint32 m_nInBufWord; //68
	int m_nBitsAvail; //72
	uint32 const *m_pDataIn; //76
	uint32 const *m_pBufferEnd; //80
	uint32 const *m_pData; //84
	*/

	bf_read_deconstructed message;

	int				size;		// size in bytes //88
	int				wiresize;   // size in bytes before decompression //92
	bool			stream;		// was send as stream
	struct netpacket_t_deconstructed *pNext;	// for internal use, should be NULL in public
} netpacket_s_deconstructed;

extern int instatebackup;
extern int inseqnrbackup;
extern int outrelstatebackupafter;
extern int outrelstatebackupbefore;
extern int nChokedTicks;
extern int ShouldGetChokedTicks;

typedef DWORD _DWORD;
typedef BYTE _BYTE;
typedef WORD _WORD;

//bool *NET_ISMULTIPLAYER;
//char *net_ismultiplayersig = new char[63]{ 66, 74, 90, 90, 73, 62, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 74, 74, 90, 90, 74, 60, 90, 90, 66, 78, 90, 90, 76, 78, 90, 90, 74, 75, 90, 90, 74, 74, 90, 90, 74, 74, 90, 90, 66, 56, 90, 90, 78, 76, 90, 90, 78, 66, 0 }; /*80  3D  ??  ??  ??  ??  00  0F  84  64  01  00  00  8B  46  48*/
//char *unknowncrcsig = new char[59]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 79, 73, 90, 90, 66, 56, 90, 90, 62, 67, 90, 90, 79, 76, 90, 90, 79, 77, 90, 90, 66, 56, 90, 90, 77, 62, 90, 90, 74, 66, 90, 90, 66, 56, 90, 90, 60, 72, 90, 90, 66, 56, 90, 90, 74, 73, 0 }; /*55  8B  EC  53  8B  D9  56  57  8B  7D  08  8B  F2  8B  03*/
//using UnknownCRC32Fn = int(__fastcall*)(unsigned int*, unsigned __int8*, int);

unsigned short BufferToShortChecksum(const void *pvData, size_t nLength)
{
	CRC32_t crc = CRC32_ProcessSingleBuffer(pvData, nLength);

	unsigned short lowpart = (crc & 0xffff);
	unsigned short highpart = ((crc >> 16) & 0xffff);

	return (unsigned short)(lowpart ^ highpart);
}

int ProcessPacketHeader_GetChokedTicks(INetChannel* chan, netpacket_t_deconstructed* packet)
{



	//static UnknownCRC32Fn pUnknownCRCFunc = nullptr;
	//if (!pUnknownCRCFunc)
	//{
	//	DecStr(unknowncrcsig, 58);
	//	pUnknownCRCFunc = (UnknownCRC32Fn)FindMemoryPattern(EngineHandle, unknowncrcsig, 58);
	//	EncStr(unknowncrcsig, 58);
	//	delete[]unknowncrcsig;
	//	if (!pUnknownCRCFunc)
	//	{
	//		THROW_ERROR(ERR_CANT_FIND_UNKNOWN_CRC32_SIGNATURE);
	//		exit(EXIT_SUCCESS);
	//	}
	//	DecStr(net_ismultiplayersig, 62);
	//	NET_ISMULTIPLAYER = (bool*)FindMemoryPattern(EngineHandle, net_ismultiplayersig, 62);
	//	EncStr(net_ismultiplayersig, 62);
	//	delete[]net_ismultiplayersig;
	//	if (!NET_ISMULTIPLAYER)
	//	{
	//		THROW_ERROR(ERR_CANT_FIND_NET_ISMULTIPLAYER_SIGNATURE);
	//		exit(EXIT_SUCCESS);
	//	}
	//	NET_ISMULTIPLAYER = (bool*)((DWORD)NET_ISMULTIPLAYER + 2);
	//}

	bool* NET_ISMULTIPLAYER = (bool*)StaticOffsets.GetOffsetValue(_NetIsMultiplayer);

	netpacket_t_deconstructed *packetcopy; // esi
	_DWORD *netchancopy; // ebx
	int bitsavailable; // edi
	unsigned int *v5; // eax
	unsigned int *v6; // ecx
	unsigned int *v7; // ecx
	unsigned int *v8; // edx
	int v9; // eax
	unsigned int v10; // edx
	signed int v11; // edi
	unsigned int *v12; // eax
	unsigned int *v13; // ecx
	unsigned int *v14; // ecx
	unsigned int *v15; // edx
	int v16; // eax
	unsigned int v17; // edx
	unsigned int v18; // edx
	signed int v19; // eax
	unsigned int v20; // ecx
	unsigned int *v21; // eax
	unsigned int *v22; // ecx
	unsigned int *v23; // ecx
	int v24; // edi
	unsigned int *v25; // eax
	unsigned int v26; // edx
	signed int v27; // eax
	unsigned int v28; // ecx
	unsigned __int16 usCheckSum; // di
	unsigned int *v30; // eax
	unsigned int *v31; // ecx
	unsigned int *v32; // ecx
	int v33; // edi
	unsigned int *v34; // eax
	unsigned int v35; // edx
	int v36; // cl
	int v37; // eax
	unsigned int *v38; // ecx
	DWORD v39; // eax
	int v40; // eax
	unsigned int *v41; // edx
	signed int nOffset; // ecx
	int nCheckSumBytes; // eax
	int v44; // eax
	signed int v46; // eax
	unsigned int v47; // ecx
	unsigned int *v48; // eax
	unsigned int *v49; // ecx
	unsigned int *v50; // ecx
	int v51; // edi
	unsigned int *v52; // eax
	unsigned int v53; // edx
	int v54; // edi
	signed int v55; // eax
	unsigned int v56; // ecx
	unsigned int *v57; // eax
	unsigned int *v58; // ecx
	unsigned int *v59; // ecx
	int v60; // edi
	unsigned int *v61; // eax
	unsigned int v62; // edx
	char v63; // cl
	int v64; // eax
	int v65; // ecx
	int v66; // edx
	unsigned int v67; // eax
	int v68; // eax
	bool v69; // zf
	int v70; // eax
	int v71; // eax
	int v72; // eax
	unsigned int v73; // eax
	int v74; // eax
	int v75; // eax
	_DWORD *v76; // edi
	int v77; // ebx
	int v78; // eax
	signed int v79; // eax
	netpacket_t_deconstructed *v80; // ecx
	_DWORD *v81; // edx
	int v82; // ecx
	int v83; // ebx
	int v84; // ecx
	signed int v85; // eax
	int v86; // eax
	signed int v87; // eax
	signed int v88; // eax
	int v89; // eax
	_DWORD *v90; // edx
	signed int v91; // ebx
	int v92; // eax
	int v93; // ecx
	_DWORD *v94; // edi
	int v95; // [esp-4h] [ebp-28h]
	CRC32_t v96; // [esp+Ch] [ebp-18h]
	int v97; // [esp+10h] [ebp-14h]
	unsigned int v98; // [esp+14h] [ebp-10h]
	int nChoked; // [esp+18h] [ebp-Ch]
	_DWORD *netchancopy2; // [esp+1Ch] [ebp-8h]
	int v101; // [esp+20h] [ebp-4h]
	int v102; // [esp+2Ch] [ebp+8h]
	netpacket_t_deconstructed *v103; // [esp+2Ch] [ebp+8h]
	unsigned int v104; // [esp+2Ch] [ebp+8h]
	netpacket_t_deconstructed *v105; // [esp+2Ch] [ebp+8h]

	packetcopy = packet;
	netchancopy = (DWORD*)chan;
	netchancopy2 = (DWORD*)chan;
	bitsavailable = packet->message.m_nBitsAvail;
	v101 = packet->message.m_nInBufWord;
	if (bitsavailable >= 32)
	{
		packet->message.m_nBitsAvail = bitsavailable - 32;
		if (bitsavailable == 32)
		{
			v5 = (unsigned int*)packet->message.m_pBufferEnd;
			v6 = (unsigned int*)packet->message.m_pDataIn;
			packet->message.m_nBitsAvail = 32;
			if (v6 == v5)
			{
				packet->message.m_nBitsAvail = 1;
				packet->message.m_nInBufWord = 0;
				packet->message.m_pDataIn = v6 + 1;
			}
			else if (v6 <= v5)
			{
				packet->message.m_nInBufWord = *v6;
				packet->message.m_pDataIn = v6 + 1;
			}
			else
			{
				packet->message.m_bOverflow = 1;
				packet->message.m_nInBufWord = 0;
			}
		}
		else
		{
			packet->message.m_nInBufWord = 0;
		}
		goto LABEL_18;
	}
	v7 = (unsigned int*)packet->message.m_pDataIn;
	v8 = (unsigned int*)packet->message.m_pBufferEnd;
	v9 = 32 - bitsavailable;
	v102 = (32 - bitsavailable);
	if (v7 == v8)
	{
		packetcopy->message.m_nBitsAvail = 1;
		packetcopy->message.m_nInBufWord = 0;
		packetcopy->message.m_bOverflow = 1;
	}
	else
	{
		if (v7 > v8)
		{
			packetcopy->message.m_bOverflow = 1;
			packetcopy->message.m_nInBufWord = 0;
			goto LABEL_15;
		}
		packetcopy->message.m_nInBufWord = *v7;
	}
	packetcopy->message.m_pDataIn = v7 + 1;
	v9 = 32 - bitsavailable;
LABEL_15:
	if (packetcopy->message.m_bOverflow)
	{
		v101 = 0;
	}
	else
	{
		v10 = packetcopy->message.m_nInBufWord;
		v101 |= (v10 & CBitBuffer::s_nMaskTable[v9]) << packetcopy->message.m_nBitsAvail;
		packetcopy->message.m_nBitsAvail = bitsavailable;
		packetcopy->message.m_nInBufWord = v10 >> v102;
	}
LABEL_18:
	v11 = packetcopy->message.m_nBitsAvail;
	if (v11 >= 32)
	{
		v103 = (netpacket_t_deconstructed *)packetcopy->message.m_nInBufWord;
		packetcopy->message.m_nBitsAvail = v11 - 32;
		if (v11 == 32)
		{
			v12 = (unsigned int*)packetcopy->message.m_pBufferEnd;
			v13 = (unsigned int*)packetcopy->message.m_pDataIn;
			packetcopy->message.m_nBitsAvail = 32;
			if (v13 == v12)
			{
				packetcopy->message.m_nBitsAvail = 1;
				packetcopy->message.m_nInBufWord = 0;
				packetcopy->message.m_pDataIn = v13 + 1;
			}
			else if (v13 <= v12)
			{
				packetcopy->message.m_nInBufWord = *v13;
				packetcopy->message.m_pDataIn = v13 + 1;
			}
			else
			{
				packetcopy->message.m_bOverflow = 1;
				packetcopy->message.m_nInBufWord = 0;
			}
		}
		else
		{
			packetcopy->message.m_nInBufWord = 0;
		}
		goto LABEL_35;
	}
	v14 = (unsigned int*)packetcopy->message.m_pDataIn;
	v104 = packetcopy->message.m_nInBufWord;
	v15 = (unsigned int*)packetcopy->message.m_pBufferEnd;
	v16 = 32 - v11;
	v97 = 32 - v11;
	if (v14 == v15)
	{
		packetcopy->message.m_nBitsAvail = 1;
		packetcopy->message.m_nInBufWord = 0;
		packetcopy->message.m_bOverflow = 1;
	}
	else
	{
		if (v14 > v15)
		{
			packetcopy->message.m_bOverflow = 1;
			packetcopy->message.m_nInBufWord = 0;
			goto LABEL_32;
		}
		packetcopy->message.m_nInBufWord = *v14;
	}
	packetcopy->message.m_pDataIn = v14 + 1;
	v16 = v97;
LABEL_32:
	if (packetcopy->message.m_bOverflow)
	{
		v103 = 0;
	}
	else
	{
		v17 = packetcopy->message.m_nInBufWord;
		v103 = (netpacket_t_deconstructed *)(((v17 & CBitBuffer::s_nMaskTable[v16]) << packetcopy->message.m_nBitsAvail) | v104);
		v18 = v17 >> v97;
		packetcopy->message.m_nBitsAvail = v11;
		packetcopy->message.m_nInBufWord = v18;
	}
LABEL_35:
	v19 = packetcopy->message.m_nBitsAvail;
	v20 = packetcopy->message.m_nInBufWord;
	if (v19 >= 8)
	{
		v98 = *(unsigned __int8*)&v20;
		packetcopy->message.m_nBitsAvail = v19 - 8;
		if (v19 == 8)
		{
			v21 = (unsigned int*)packetcopy->message.m_pBufferEnd;
			v22 = (unsigned int*)packetcopy->message.m_pDataIn;
			packetcopy->message.m_nBitsAvail = 32;
			if (v22 == v21)
			{
				packetcopy->message.m_nBitsAvail = 1;
				packetcopy->message.m_nInBufWord = 0;
				packetcopy->message.m_pDataIn = v22 + 1;
			}
			else if (v22 <= v21)
			{
				packetcopy->message.m_nInBufWord = *v22;
				packetcopy->message.m_pDataIn = v22 + 1;
			}
			else
			{
				packetcopy->message.m_bOverflow = 1;
				packetcopy->message.m_nInBufWord = 0;
			}
		}
		else
		{
			packetcopy->message.m_nInBufWord = v20 >> 8;
		}
		goto LABEL_52;
	}
	v98 = packetcopy->message.m_nInBufWord;
	v23 = (unsigned int*)packetcopy->message.m_pDataIn;
	v24 = 8 - v19;
	v25 = (unsigned int*)packetcopy->message.m_pBufferEnd;
	if (v23 == v25)
	{
		packetcopy->message.m_nBitsAvail = 1;
		packetcopy->message.m_nInBufWord = 0;
		packetcopy->message.m_bOverflow = 1;
	}
	else
	{
		if (v23 > v25)
		{
			packetcopy->message.m_bOverflow = 1;
			packetcopy->message.m_nInBufWord = 0;
			goto LABEL_49;
		}
		packetcopy->message.m_nInBufWord = *v23;
	}
	packetcopy->message.m_pDataIn = v23 + 1;
LABEL_49:
	if (packetcopy->message.m_bOverflow)
	{
		v98 = 0;
	}
	else
	{
		v26 = packetcopy->message.m_nInBufWord;
		v98 |= (v26 & CBitBuffer::s_nMaskTable[v24]) << packetcopy->message.m_nBitsAvail;
		packetcopy->message.m_nBitsAvail = 32 - v24;
		packetcopy->message.m_nInBufWord = v26 >> v24;
	}
LABEL_52:
	if (!*NET_ISMULTIPLAYER)
		goto LABEL_75;
	v27 = packetcopy->message.m_nBitsAvail;
	v28 = packetcopy->message.m_nInBufWord;
	if (v27 >= 16)
	{
		usCheckSum = packetcopy->message.m_nInBufWord;
		packetcopy->message.m_nBitsAvail = v27 - 16;
		if (v27 == 16)
		{
			v30 = (unsigned int*)packetcopy->message.m_pBufferEnd;
			v31 = (unsigned int*)packetcopy->message.m_pDataIn;
			packetcopy->message.m_nBitsAvail = 32;
			if (v31 == v30)
			{
				packetcopy->message.m_nBitsAvail = 1;
				packetcopy->message.m_nInBufWord = 0;
				packetcopy->message.m_pDataIn = v31 + 1;
			}
			else if (v31 <= v30)
			{
				packetcopy->message.m_nInBufWord = *v31;
				packetcopy->message.m_pDataIn = v31 + 1;
			}
			else
			{
				packetcopy->message.m_bOverflow = 1;
				packetcopy->message.m_nInBufWord = 0;
			}
		}
		else
		{
			packetcopy->message.m_nInBufWord = v28 >> 16;
		}
		goto LABEL_70;
	}
	v97 = packetcopy->message.m_nInBufWord;
	v32 = (unsigned int*)packetcopy->message.m_pDataIn;
	v33 = 16 - v27;
	v34 = (unsigned int*)packetcopy->message.m_pBufferEnd;
	if (v32 == v34)
	{
		packetcopy->message.m_nBitsAvail = 1;
		packetcopy->message.m_nInBufWord = 0;
		packetcopy->message.m_bOverflow = 1;
	}
	else
	{
		if (v32 > v34)
		{
			packetcopy->message.m_bOverflow = 1;
			packetcopy->message.m_nInBufWord = 0;
			goto LABEL_67;
		}
		packetcopy->message.m_nInBufWord = *v32;
	}
	packetcopy->message.m_pDataIn = v32 + 1;
LABEL_67:
	if (packetcopy->message.m_bOverflow)
	{
		usCheckSum = 0;
	}
	else
	{
		v35 = packetcopy->message.m_nInBufWord;
		v36 = v33;
		v97 |= (v35 & CBitBuffer::s_nMaskTable[v33]) << packetcopy->message.m_nBitsAvail;
		v37 = 32 - v33;
		usCheckSum = v97;
		packetcopy->message.m_nBitsAvail = v37;
		packetcopy->message.m_nInBufWord = v35 >> v36;
	}
LABEL_70:
	v38 = (unsigned int*)packetcopy->message.m_pData;
	if (v38)
	{
		v39 = (DWORD)packetcopy->message.m_pDataIn - (DWORD)v38;
		v38 = (unsigned int *)packetcopy->message.m_nDataBits;
		v40 = 8 * ((packetcopy->message.m_nDataBytes & 3) + 4 * (v39 / 4)) - packetcopy->message.m_nBitsAvail;
		if (v40 < (signed int)v38)
			v38 = (unsigned int *)v40;
	}
	v41 = (unsigned int*)packetcopy->message.m_pData;
	nOffset = (signed int)v38 >> 3;
	nCheckSumBytes = packetcopy->message.m_nDataBytes - nOffset;

	//sub_102DD6F0((unsigned __int8 *)v41 + v42, (unsigned int *)&v96, v43);

	const void *pvData = packet->message.m_pData + nOffset;
	unsigned short usDataCheckSum = BufferToShortChecksum(pvData, nCheckSumBytes);

	if (usDataCheckSum != usCheckSum)
	//if (((unsigned __int16)~(_WORD)v96 ^ ((unsigned int)~v96 >> 16)) != usCheckSum)
	{
		//v44 = (*(int(__thiscall **)(_DWORD *, int, _DWORD))(*netchancopy + 4))(netchancopy, v101, netchancopy[7]);
		//ConMsg("%s:corrupted packet %i at %i\n", v44);
		return -1;
	}
LABEL_75:
	v46 = packetcopy->message.m_nBitsAvail;
	v47 = packetcopy->message.m_nInBufWord;
	if (v46 >= 8)
	{
		v97 = (unsigned __int8)v47;
		packetcopy->message.m_nBitsAvail = v46 - 8;
		if (v46 == 8)
		{
			v48 = (unsigned int*)packetcopy->message.m_pBufferEnd;
			v49 = (unsigned int*)packetcopy->message.m_pDataIn;
			packetcopy->message.m_nBitsAvail = 32;
			if (v49 == v48)
			{
				packetcopy->message.m_nBitsAvail = 1;
				packetcopy->message.m_nInBufWord = 0;
				packetcopy->message.m_pDataIn = v49 + 1;
			}
			else if (v49 <= v48)
			{
				packetcopy->message.m_nInBufWord = *v49;
				packetcopy->message.m_pDataIn = v49 + 1;
			}
			else
			{
				packetcopy->message.m_bOverflow = 1;
				packetcopy->message.m_nInBufWord = 0;
			}
		}
		else
		{
			packetcopy->message.m_nInBufWord = v47 >> 8;
		}
		goto LABEL_92;
	}
	v97 = packetcopy->message.m_nInBufWord;
	v50 = (unsigned int*)packetcopy->message.m_pDataIn;
	v51 = 8 - v46;
	v52 = (unsigned int*)packetcopy->message.m_pBufferEnd;
	if (v50 == v52)
	{
		packetcopy->message.m_nBitsAvail = 1;
		packetcopy->message.m_nInBufWord = 0;
		packetcopy->message.m_bOverflow = 1;
	}
	else
	{
		if (v50 > v52)
		{
			packetcopy->message.m_bOverflow = 1;
			packetcopy->message.m_nInBufWord = 0;
			goto LABEL_89;
		}
		packetcopy->message.m_nInBufWord = *v50;
	}
	packetcopy->message.m_pDataIn = v50 + 1;
LABEL_89:
	if (packetcopy->message.m_bOverflow)
	{
		v97 = 0;
	}
	else
	{
		v53 = packetcopy->message.m_nInBufWord;
		v97 |= (v53 & CBitBuffer::s_nMaskTable[v51]) << packetcopy->message.m_nBitsAvail;
		packetcopy->message.m_nBitsAvail = 32 - v51;
		packetcopy->message.m_nInBufWord = v53 >> v51;
	}
LABEL_92:
	v54 = 0;
	nChoked = 0;
	if (!(v98 & 0x10))
		goto LABEL_110;
	v55 = packetcopy->message.m_nBitsAvail;
	v56 = packetcopy->message.m_nInBufWord;
	if (v55 >= 8)
	{
		v54 = (unsigned __int8)v56;
		nChoked = (unsigned __int8)v56;
		packetcopy->message.m_nBitsAvail = v55 - 8;
		if (v55 == 8)
		{
			v57 = (unsigned int*)packetcopy->message.m_pBufferEnd;
			v58 = (unsigned int*)packetcopy->message.m_pDataIn;
			packetcopy->message.m_nBitsAvail = 32;
			if (v58 == v57)
			{
				packetcopy->message.m_nBitsAvail = 1;
				packetcopy->message.m_nInBufWord = 0;
				packetcopy->message.m_pDataIn = v58 + 1;
			}
			else if (v58 <= v57)
			{
				packetcopy->message.m_nInBufWord = *v58;
				packetcopy->message.m_pDataIn = v58 + 1;
			}
			else
			{
				packetcopy->message.m_bOverflow = 1;
				packetcopy->message.m_nInBufWord = 0;
			}
		}
		else
		{
			packetcopy->message.m_nInBufWord = v56 >> 8;
		}
		goto LABEL_110;
	}
	nChoked = packetcopy->message.m_nInBufWord;
	v59 = (unsigned int*)packetcopy->message.m_pDataIn;
	v60 = 8 - v55;
	v61 = (unsigned int*)packetcopy->message.m_pBufferEnd;
	if (v59 == v61)
	{
		packetcopy->message.m_nBitsAvail = 1;
		packetcopy->message.m_nInBufWord = 0;
		packetcopy->message.m_bOverflow = 1;
	}
	else
	{
		if (v59 > v61)
		{
			packetcopy->message.m_bOverflow = 1;
			packetcopy->message.m_nInBufWord = 0;
			goto LABEL_107;
		}
		packetcopy->message.m_nInBufWord = *v59;
	}
	packetcopy->message.m_pDataIn = v59 + 1;
LABEL_107:
	if (packetcopy->message.m_bOverflow)
	{
		v54 = 0;
		nChoked = 0;
	}
	else
	{
		v62 = packetcopy->message.m_nInBufWord;
		v63 = v60;
		nChoked |= (v62 & CBitBuffer::s_nMaskTable[v60]) << packetcopy->message.m_nBitsAvail;
		v64 = 32 - v60;
		v54 = nChoked;
		packetcopy->message.m_nBitsAvail = v64;
		packetcopy->message.m_nInBufWord = v62 >> v63;
	}
LABEL_110:

	return nChoked;
}

__declspec (noinline) void __fastcall ProcessPacket_Real(void* netchan, void* edx, void* packet, bool bHasHeader)
{
	VMP_BEGINMUTILATION("PCSPKT2")
	netpacket_t *pack = (netpacket_t*)packet;
	INetChannel* chan = (INetChannel*)netchan;
	float original_last_received = chan->last_received;

	//Only grab choked value if it is a client packet
	ShouldGetChokedTicks = chan->m_Name[0] == 'C' && chan->m_Name[5] == 'T'; //CLIENT

	bool bNetchanIsHooked = HMessageHandler && HNetchan && HNetchan->GetCurrentVT() == HNetchan->GetNewVT();

	//Update our own netchannel 
	if (ShouldGetChokedTicks)
	{
		if (bNetchanIsHooked)
		{
			FlowUpdate(&our_netchan, FLOW_INCOMING, pack->wiresize + UDP_HEADER_SIZE);

#if 0
			if (LocalPlayer.Last_Server_OutSequenceNrSent > client_netchan.m_nInSequenceNr)
				FlowUpdate(&client_netchan, FLOW_INCOMING, 50 + UDP_HEADER_SIZE);

			if (LocalPlayer.Last_OutSequenceNrSent > server_netchan.m_nInSequenceNr)
				FlowUpdate(&server_netchan, FLOW_INCOMING, pack->wiresize + UDP_HEADER_SIZE);
#endif
		}
	}

	//Call original ProcessPacket
	oProcessPacket(netchan, packet, bHasHeader);

	if (ShouldGetChokedTicks && chan->last_received != original_last_received)
	{
		//Packet received was a valid packet

		//Pretend we are the server
		//Rebuild what server process packet header does to get the actual server latency for our player

		// discard stale or duplicated packets
		if (bNetchanIsHooked)
		{
			//Update our flow
			FlowNewPacket(&our_netchan, FLOW_INCOMING, chan->m_nInSequenceNr, chan->m_nOutSequenceNrAck, nChokedTicks, chan->m_PacketDrop, pack->wiresize + UDP_HEADER_SIZE);

#if 0
			if (LocalPlayer.Last_Server_OutSequenceNrSent > client_netchan.m_nInSequenceNr)
			{
				// dropped packets don't keep the message from being used
				client_netchan.m_PacketDrop = LocalPlayer.Last_Server_OutSequenceNrSent - (client_netchan.m_nInSequenceNr + LocalPlayer.Last_Server_ChokeCountSent + 1);

				client_netchan.m_nInSequenceNr = LocalPlayer.Last_Server_OutSequenceNrSent;
				client_netchan.m_nOutSequenceNrAck = LocalPlayer.Last_Server_InSequenceNrSent;

				FlowNewPacket(&client_netchan, FLOW_INCOMING, client_netchan.m_nInSequenceNr, client_netchan.m_nOutSequenceNrAck, LocalPlayer.Last_Server_ChokeCountSent, client_netchan.m_PacketDrop, 50 + UDP_HEADER_SIZE);
			}

			if (LocalPlayer.Last_OutSequenceNrSent > server_netchan.m_nInSequenceNr)
			{
				// dropped packets don't keep the message from being used
				server_netchan.m_PacketDrop = LocalPlayer.Last_OutSequenceNrSent - (server_netchan.m_nInSequenceNr + LocalPlayer.Last_ChokeCountSent + 1);

				server_netchan.m_nInSequenceNr = LocalPlayer.Last_OutSequenceNrSent;
				server_netchan.m_nOutSequenceNrAck = LocalPlayer.Last_InSequenceNrSent;

				FlowNewPacket(&server_netchan, FLOW_INCOMING, server_netchan.m_nInSequenceNr, server_netchan.m_nOutSequenceNrAck, LocalPlayer.Last_ChokeCountSent, server_netchan.m_PacketDrop, pack->wiresize + UDP_HEADER_SIZE);
			}
#endif
		}
	}

	ShouldGetChokedTicks = 0;

	ProcessOnDataChangedEvents();
	CL_FireEvents();
	oTempEntsUpdate(tempents);
	oUpdateTempEntBeams(beams);

	static int LastReliableState = chan->m_nInReliableState;

	if (chan->m_nInReliableState != LastReliableState)
	{
		NextPacketShouldBeRealPing = true;
	}

	LastReliableState = chan->m_nInReliableState;


	VMP_END
}

void __fastcall Hooks::ProcessPacket(void* netchan, void* edx, void* packet, bool bHasHeader)
{
	VMP_BEGINMUTILATION("PCSPKT")
	ProcessPacket_Real(netchan, edx, packet, bHasHeader);
	VMP_END
}
