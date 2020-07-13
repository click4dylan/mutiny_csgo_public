#ifdef MUTINY
#ifdef MUTINY_FRAMEWORK
#include <string>
//#include "C:\Developer\Sync\Framework\Files\Logging\Logging.h"
#include "C:\Developer\Sync\Framework\Files\Includes\Frame-Include.h"
#include "misc.h"
#include "CSGO_HX.h"

extern MutinyFrame::CHeartBeat* pHeartbeat;

void THROW_ERROR_TO_MUTINY(const char* str, bool takeSS)
{
	return;
	//Framework->Heartbeat->Logs.LogError(takeSS,str);

	//auto Logs = MutinyFrame::WebLoggingApi(pHeartbeat->hKey, pHeartbeat->hIV, pHeartbeat->hUserAgent);
	//Logs.LogError(str);
}
#endif
#endif
