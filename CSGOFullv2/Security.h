#pragma once

#if defined (MUTINY_FRAMEWORK) || defined(DUMP_SIGS_FOR_BLACKBOOK)

//#include "C:\Developer\Sync\Framework\Files\Includes\Frame-Include.h"
#define _WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <chrono>
#include <mutex>
#include "C:\Developer\Sync\Framework\Files\JSON\json.hpp"
#include "C:\Developer\Sync\Framework\Files\Networking\Network.h"
#include "C:\Developer\Sync\Framework\Files\Logging\Logging.h"
#include "C:\Developer\Sync\Framework\Files\Scanning\DigitalCertificates\DigiCert.h"
#include "C:\Developer\Sync\Framework\Files\Scanning\DriverScan\DriverScan.h"
#include "C:\Developer\Sync\Framework\Files\Heartbeat\Heartbeat.h"
#include "C:\Developer\Sync\Framework\Files\Blackbook\Blackbook.h"
#include "NetworkedVariables.h"
class StreamedData : public ProjectBB::HandlerData
{
public:
	StreamedData(nlohmann::json a, std::string b)
	{
		std::swap(json_data, a);
		tablename = b;
	}

	nlohmann::json json_data;
	std::string tablename;
};
class CustomReturnData : public ProjectBB::ReturnData
{
public: 
	CustomReturnData(ReturnData a) : ReturnData(a)
	{
	}
	CustomReturnData() = default;
	uint32_t uintret;
};



class StreamedOffsetData : public ProjectBB::HandlerData
{
public:
	StreamedOffsetData() = default;
	StreamedOffsetData(HandlerData a) : HandlerData(a)
	{
	}
	StreamedOffsetData(nlohmann::json a, const StaticOffset& _var)
	{
		std::swap(json_data, a);
		Offset = _var;
	}
	nlohmann::json json_data;
	StaticOffset Offset;
};

class StreamedReturnOffsetData : public ProjectBB::ReturnData
{
public:
	StreamedReturnOffsetData() = default;
	StreamedReturnOffsetData(const ReturnData & a) : ReturnData(a)
	{
	}

	int offset;
	bool datamap;
};

class Security
{
public:

	Security() = default;


#ifdef DUMP_SIGS_FOR_BLACKBOOK
	void setup_data_for_server();
	void dump();
#endif


	void setup(MutinyFrame::CHeartBeat* heartbeat);
	ProjectBB::ReturnData get_sig_data(std::string key);
	CustomReturnData get_offset_data( const StaticOffset& offset);

	MutinyFrame::CHeartBeat	 *m_heartbeat = nullptr;
	ProjectBB::Blackbook *m_blackbook = nullptr;
};

extern Security	g_Security;
extern MutinyFrame::CHeartBeat	*pHeartbeat;

#endif