#pragma once

#pragma pack(push, 1)
struct CLC_Move
{
	uint32_t INetMessage_vtable; //0x58
	uint32_t CCLCMsg_Move_vtable; //0x54
	uint32_t unknown; //0x50
	int	m_nBackupCommands; //0x4C
	int	m_nNewCommands; //0x48
	void* allocatedmemory; //0x44
	uint32_t someint3; //0x40
	uint32_t flags;  //0x3C
	uint8_t somebyte1; //0x38
	uint8_t somebyte1_pad[3]; //0x37
	uint8_t somebyte2; //0x34
	uint8_t somebyte2_pad[3]; //0x33
	uint32_t unknownpad[3]; //0x30
	uint32_t someint2; //0x24
	uint32_t someint; //0x20
	bf_write m_DataOut; //0x1C

	CLC_Move();
	~CLC_Move();
	void FinishProtobuf();
};
#pragma pack(pop)

extern void CL_SendMove_Rebuilt();