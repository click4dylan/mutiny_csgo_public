#include "precompiled.h"
#include "ClassIDS.h"

#ifdef _DEBUG
#include <fstream>
#include "Interfaces.h"

void ClassIDDumper::DumpClassIDs()
{
	//Dump all class ids
	std::ofstream log("G:\\classids.txt", std::ios::trunc | std::ios::out);
	if (log.is_open())
	{
		log << "enum ClassID : int\n";
		log << "{\n";
		for (ClientClass* pClass = Interfaces::Client->GetAllClasses(); pClass; pClass = pClass->m_pNext)
		{
			log << "\t_";
			log.write(pClass->m_pNetworkName, strlen(pClass->m_pNetworkName));
			log << " = " << pClass->m_ClassID;
			if (pClass->m_pNext)
			{
				log << ",";
			}
			log << "\n";
		}
		log << "};";
		log.close();
		HasDumped = true;
	}
}
#endif