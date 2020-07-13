#include "precompiled.h"
#include "netmessages.h"
#include "VMProtectDefs.h"

static char s_text[1024];
#define Bits2Bytes(b) ((b+7)>>3)

#if 0
bool SVC_Sounds::WriteToBuffer(bf_write &buffer)
{
	m_nLength = m_DataOut.GetNumBitsWritten();

	buffer.WriteUBitLong(GetType(), 5); //NETMSG_TYPE_BITS

	//Assert(m_nNumSounds > 0);

	if (m_bReliableSound)
	{
		// as single sound message is 32 bytes long maximum
		buffer.WriteOneBit(1);
		buffer.WriteUBitLong(m_nLength, 8);
	}
	else
	{
		// a bunch of unreliable messages
		buffer.WriteOneBit(0);
		buffer.WriteUBitLong(m_nNumSounds, 8);
		buffer.WriteUBitLong(m_nLength, 16);
	}

	return buffer.WriteBits(m_DataOut.GetData(), m_nLength);
}

bool SVC_Sounds::ReadFromBuffer(bf_read &buffer)
{
	//VPROF("SVC_Sounds::ReadFromBuffer");

	m_bReliableSound = buffer.ReadOneBit() != 0;

	if (m_bReliableSound)
	{
		m_nNumSounds = 1;
		m_nLength = buffer.ReadUBitLong(8);

	}
	else
	{
		m_nNumSounds = buffer.ReadUBitLong(8);
		m_nLength = buffer.ReadUBitLong(16);
	}

	m_DataIn = buffer;
	return buffer.SeekRelative(m_nLength);
}

const char *SVC_Sounds::ToString(void) const
{
	return nullptr;
#if 0
	snprintf(s_text, sizeof(s_text), "%s: number %i,%s bytes %i",
		GetName(), m_nNumSounds, m_bReliableSound ? " reliable," : "", Bits2Bytes(m_nLength));
	return s_text;
#endif
}

bool NET_SetConVar::WriteToBuffer(bf_write &buffer)
{
	buffer.WriteUBitLong(GetType(), 5);

	int numvars = m_ConVars.Count();

	// Note how many we're sending
	buffer.WriteByte(numvars);

	for (int i = 0; i< numvars; i++)
	{
		cvar_t * cvar = (cvar_t*)m_ConVars.Retrieve(i, sizeof(cvar_t));// &m_ConVars[i];
		buffer.WriteString(cvar->name);
		buffer.WriteString(cvar->value);
	}

	return !buffer.IsOverflowed();
}

bool NET_SetConVar::ReadFromBuffer(bf_read &buffer)
{
	//VPROF("NET_SetConVar::ReadFromBuffer");

	int numvars = buffer.ReadByte();

	m_ConVars.RemoveAll();

	for (int i = 0; i< numvars; i++)
	{
		cvar_t cvar;
		buffer.ReadString(cvar.name, sizeof(cvar.name));
		buffer.ReadString(cvar.value, sizeof(cvar.value));
		m_ConVars.AddToTail(cvar);

	}
	return !buffer.IsOverflowed();
}

const char *NET_SetConVar::ToString(void) const
{
	return nullptr;
#if 0
	snprintf(s_text, sizeof(s_text), "%s: %i cvars, \"%s\"=\"%s\"",
		GetName(), m_ConVars.count,
		m_ConVars[0].name, m_ConVars[0].value);
	return s_text;
#endif
}
#endif

//char *setconvarconstructor = new char[99]{ 60, 60, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 79, 76, 90, 90, 66, 56, 90, 90, 60, 75, 90, 90, 57, 77, 90, 90, 74, 76, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 62, 0 }; /*FF  CC  CC  CC  CC  CC  CC  CC  CC  CC  CC  CC  CC  CC  CC  56  8B  F1  C7  06  ??  ??  ??  ??  8D*/
//char *setconvarinit = new char[55]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 79, 76, 90, 90, 66, 56, 90, 90, 60, 75, 90, 90, 66, 73, 90, 90, 78, 63, 90, 90, 75, 78, 90, 90, 74, 75, 90, 90, 66, 73, 90, 90, 77, 63, 90, 90, 74, 57, 90, 90, 74, 74, 0 }; /*55  8B  EC  56  8B  F1  83  4E  14  01  83  7E  0C  00*/
//char *setconvardestructor = new char[75]{ 74, 74, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 57, 57, 90, 90, 79, 73, 90, 90, 66, 56, 90, 90, 62, 67, 90, 90, 79, 76, 90, 90, 79, 77, 0 }; /*00  CC  CC  CC  CC  CC  CC  CC  CC  CC  CC  CC  CC  CC  53  8B  D9  56  57*/

NET_SetConVarNew::NET_SetConVarNew(const char* name, const char* value)
{
	//typedef void(__thiscall* oConstructor)(PVOID);	
	//typedef void(__thiscall* oInit)(PVOID, const char*, const char*);
	//static oConstructor Constructor = nullptr;
	//static oInit Init;
	//if (!Constructor)
	//{
	//	DecStr(setconvarconstructor, 98);
	//	Constructor = (oConstructor)FindMemoryPattern(EngineHandle, setconvarconstructor, 98);
	//	EncStr(setconvarconstructor, 98);
	//	delete[]setconvarconstructor;
	//	if (!Constructor)
	//	{
	//		THROW_ERROR(ERR_CANT_FIND_NET_SETCONVAR_CONSTRUCTOR_SIGNATURE);
	//		exit(EXIT_SUCCESS);
	//	}
	//	Constructor = (oConstructor)((DWORD)Constructor + 15);
	//
	//	DecStr(setconvarinit, 54);
	//	Init = (oInit)FindMemoryPattern(EngineHandle, setconvarinit, 54);
	//	EncStr(setconvarinit, 54);
	//	delete[]setconvarinit;
	//
	//	if (!Init)
	//	{
	//		THROW_ERROR(ERR_CANT_FIND_NET_SETCONVAR_INIT_SIGNATURE);
	//		exit(EXIT_SUCCESS);
	//	}
	//}

	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(void*)>(_NetSetConVar_Constructor)(this);
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(void*, const char*, const char*)>(_NetSetConVar_Init)(this, name, value);

	//Constructor(this);
	//Init(this, name, value);
}

NET_SetConVarNew::~NET_SetConVarNew()
{
	//typedef void(__thiscall* oDestructor)(PVOID);
	//static oDestructor Destructor = nullptr;
	//if (!Destructor)
	//{
	//	DecStr(setconvardestructor, 74);
	//	Destructor = (oDestructor)FindMemoryPattern(EngineHandle, setconvardestructor, 74);
	//	EncStr(setconvardestructor, 74);
	//	delete[]setconvardestructor;
	//
	//	if (!Destructor)
	//	{
	//		THROW_ERROR(ERR_CANT_FIND_NET_SETCONVAR_DESTRUCTOR_SIGNATURE);
	//		exit(EXIT_SUCCESS);
	//	}
	//
	//	Destructor = (oDestructor)((DWORD)Destructor + 14);
	//}

	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(void*)>(_NetSetConVar_Destructor)(this);

	//Destructor(this);
}