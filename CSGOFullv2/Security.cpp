#include "C:\Developer\Sync\Framework\Files\Includes\Frame-Include.h"
#include "Security.h"
#include "NetworkedVariables.h"
#include "string_encrypt_include.h"
#include "EncryptString.h"

#if defined(MUTINY_FRAMEWORK) || defined(DUMP_SIGS_FOR_BLACKBOOK)
ProjectBB::ReturnData handle_signature_data(ProjectBB::HandlerData* data)
{
	StreamedData* _data = static_cast<StreamedData *>(data);

	ProjectBB::ReturnData ret_data;
	
	//decrypts(0)
	ret_data.RetString = _data->json_data[XorStr("patterns")][_data->tablename.c_str()][XorStr("sig")].get< std::string >();
	//encrypts(0)

	return ret_data;
}
 
CustomReturnData handle_netvar_data(ProjectBB::HandlerData* data)
{
	StreamedOffsetData* _data = static_cast<StreamedOffsetData *>(data);

	CustomReturnData ret_data;
	uint32_t key = _data->Offset.GetDecryptionKey();
	ret_data.RetInt = -12;

	return ret_data;
}

#ifdef DUMP_SIGS_FOR_BLACKBOOK
void Security::setup_data_for_server()
{
	auto *Offsets = new std::array<StaticOffset, MAX_STATIC_OFFSETS>;
	*Offsets = StaticOffsets.Offsets;

	for (const auto &x : *Offsets)
	{
		if (!x.IsHardCoded() && x.GetName() != INVALID_STATIC_OFFSET)
		{
			//decrypts(0)
			m_blackbook->WriteServerData()[XorStr("patterns")][std::to_string(x.GetName())][XorStr("sig")] = x.GetSignature();
			//encrypts(0)
		}
	}
	delete Offsets;
}

void Security::dump()
{
	std::fstream fstream;
	fstream.open("C:\\clientdata\\server_offsets_mutiny.json", std::ios_base::out);
	if (fstream.is_open())
	{
		fstream << m_blackbook->WriteServerData().dump();	
		fstream.close();
	}
}
#endif

void Security::setup(MutinyFrame::CHeartBeat *heartbeat)
{
	//decrypts(0)
	m_blackbook = new ProjectBB::Blackbook(XorStr("nSUlYT99JDbT0kzpUAHP0Mkvl"), XorStr("IxARW9cCazjpGF9m3KLd2HLH8"), XorStr("GameCSGO"), XorStr("47"));
	//encrypts(0)
	m_heartbeat = heartbeat;

	//decrypts(0)
	m_blackbook->RegisterHandler(XorStr("signatures"), handle_signature_data);
	m_blackbook->RegisterHandler(XorStr("offsets"), handle_netvar_data);
	//encrypts(0)
#ifndef DEVELOPER
	m_blackbook->Setup();
#endif
}

ProjectBB::ReturnData Security::get_sig_data(std::string key)
{
#ifdef DEVELOPER
	m_heartbeat->LastHeartbeatExecution = std::chrono::high_resolution_clock::now();
#endif

	StreamedData _data(m_blackbook->WriteServerData(), key);

	char signaturesstr[11] = {9, 19, 29, 20, 27, 14, 15, 8, 31, 9, 0}; /*signatures*/

	DecStr(signaturesstr, 10);
	auto ret = m_blackbook->Invoke(signaturesstr, _data, m_heartbeat);
	EncStr(signaturesstr, 10);


	return ret;
}

CustomReturnData Security::get_offset_data(const StaticOffset& offset)
{
#ifdef DEVELOPER
	m_heartbeat->LastHeartbeatExecution = std::chrono::high_resolution_clock::now();
#endif

	char offsetsstr[8] = {21, 28, 28, 9, 31, 14, 9, 0}; /*offsets*/

	DecStr(offsetsstr, 7);
	StreamedOffsetData _data(m_blackbook->WriteServerData()[offsetsstr],offset );
	CustomReturnData returnData = (CustomReturnData)m_blackbook->Invoke(offsetsstr, _data, m_heartbeat);
	EncStr(offsetsstr, 7);

	if ( returnData.RetInt == -12)
	returnData.uintret = offset.Offset ^ offset.GetDecryptionKey();
	return returnData;
}
#endif


