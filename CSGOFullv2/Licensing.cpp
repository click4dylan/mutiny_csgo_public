#include "precompiled.h"
#include "Licensing.h"
#include <string.h>
//#include "cx_strenc.h"
#include "ErrorCodes.h"
#include "EncryptString.h"
#include "VMProtectDefs.h"

const double magicnumber = 12694.0f;
void ObfuscateString(char* &dest, char* src, int inlen)
{
	for (int i = 0; i < inlen; i++)
	{
		char *val = new char;
		*val = src[i];
		*val ^= 72;
		*val ^= 50;
		char *newchar = new char[2];
		memset(newchar, 0, 2);
		*newchar = static_cast<char>(*val);
		strcat(dest, newchar);
		delete[] newchar;
		delete val;
	}
}
__declspec (naked) int GuidToInt(const char * guidstr) 
{
	__asm {
		//Mov [Esp-8], Edi
		//Mov Edi, [Esp+4]
		//Mov Ecx, -1
		//Mov Al, 0
		//Repne Scasb
		//Neg Ecx
		//Sub Ecx, 6
		Mov[Esp - 4], EBX
			Mov[Esp - 8], ECX
			Mov Ebx, [Esp + 4]
			Mov Ecx, 32
			Mov Eax, 0
		HashLoop:
		Xor Eax, [Ebx + Ecx]
			Sub Ecx, 4
			Jns HashLoop
			Mov Ebx, [Esp - 4]
			Mov Ecx, [Esp - 8]
			Retn //4
	}
}
#ifdef LICENSED
char *regkeystr = new char[32]{ 41, 53, 60, 46, 45, 59, 40, 63, 38, 55, 19, 25, 8, 21, 9, 21, 28, 14, 38, 57, 8, 3, 10, 14, 21, 29, 8, 27, 10, 18, 3, 0 }; /*SOFTWARE\Microsoft\Cryptography*/
char *guidstr = new char[12]{ 55, 27, 25, 18, 19, 20, 31, 61, 15, 19, 30, 0 }; /*MachineGuid*/
int GetHWID(void) {
	HKEY hkey;              // Handle to registry key
	DecStr(regkeystr, 31);
	DecStr(guidstr, 11);
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regkeystr, NULL, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hkey) == ERROR_SUCCESS) {
		char data[256];
		unsigned long datalen = 255;  // data field length(in), data returned length(out) 
		unsigned long datatype; // #defined in winnt.h (predefined types 0-11)
		// Datalen is an in/out field, so reset it for every query
		// I've ignored displays in the event of an error ...
		// The error is most likely to be a misspelled value field name
		if (RegQueryValueExA(hkey, guidstr, NULL, &datatype, (LPBYTE)data, &datalen) == ERROR_SUCCESS) {
			//printf("GUID:\n%s\n",data);
			if (datalen == 37) {
				EncStr(regkeystr, 31);
				EncStr(guidstr, 11);
				return GuidToInt(data);
			}
			//printf("Hash:\n%d", hash);
		}
		// That's all for this key. Close it.
		RegCloseKey(hkey);
	} /*else {
	  GetLastError();
	  }*/
	EncStr(regkeystr, 31);
	EncStr(guidstr, 11);
	return 0;
}
#endif
#include <fstream>
#ifndef LICENSED
void GenerateSerial(double allowrecording, double year, double month, double day)
{
	const double registered_to_machine = ((0 * 2.0) * -1.0) - (1024 + magicnumber);
	allowrecording = ((allowrecording * 4.0) * -1.0) - (2048 + magicnumber);
	year = ((year * 8.0) * -1.0) - (4096 + magicnumber);
	month = ((month * 16.0) * -1.0) - (16384.0 + magicnumber);
	day = ((day * 32.0) * -1.0) - (12288.0 + magicnumber);
	char *UnObfuscatedString = new char[1024];
	memset(UnObfuscatedString, 0, 1024);
	char *ObfuscatedString = new char[1024];
	memset(ObfuscatedString, 0, 1024);
	char *serialformat = new char[32 + 1];
	strcpy(serialformat, "%.0f+%.0f+%.0f+%.0f+%.0f+%.0f+%i");
	sprintf(UnObfuscatedString, serialformat, registered_to_machine, allowrecording, magicnumber, year, month, day, 17892084);
	ObfuscateString(ObfuscatedString, UnObfuscatedString, strlen(UnObfuscatedString));
	delete[] UnObfuscatedString;
#ifdef ATI
	remove("C:\\ProgramData\\ATI\\ollicense");
#else
	remove("C:\\ProgramData\\NVIDIA Corporation\\ollicense");
#endif
	char licensefile[1024];
	strcpy(licensefile, "ollicense");
	std::ofstream license(licensefile);
	if (license.is_open())
	{
		license << ObfuscatedString;
		//printf("Successfully generated ollicense\n");
	}
	else
	{
		//printf("Error: could not create ollicense!\n");
		THROW_ERROR(ERR_CANT_CREATE_LICENSE);
		exit(EXIT_SUCCESS);
	}
	license.close();
	delete[] ObfuscatedString;
}
#else
//Publicly executable Generation code
char *serialformatstr = new char[33]{ 95, 84, 74, 28, 81, 95, 84, 74, 28, 81, 95, 84, 74, 28, 81, 95, 84, 74, 28, 81, 95, 84, 74, 28, 81, 95, 84, 74, 28, 81, 95, 19, 0 }; /*%.0f+%.0f+%.0f+%.0f+%.0f+%.0f+%i*/
#ifndef ATI
char *licpath = new char[44]{ 57, 64, 38, 42, 8, 21, 29, 8, 27, 23, 62, 27, 14, 27, 38, 52, 44, 51, 62, 51, 59, 90, 57, 21, 8, 10, 21, 8, 27, 14, 19, 21, 20, 38, 21, 22, 22, 19, 25, 31, 20, 9, 31, 0 }; /*C:\ProgramData\NVIDIA Corporation\ollicense*/
#else
char *licpath = new char[29]{ 57, 64, 38, 42, 8, 21, 29, 8, 27, 23, 62, 27, 14, 27, 38, 59, 46, 51, 38, 21, 22, 22, 19, 25, 31, 20, 9, 31, 0 }; /*C:\ProgramData\ATI\ollicense*/
#endif
void GenerateSerial(double licensemagic, double registered_to_machine, double allowrecording, double year, double month, double day, int hwid)
{
	char *UnObfuscatedString = new char[1024];
	memset(UnObfuscatedString, 0, 1024);
	char *ObfuscatedString = new char[1024];
	memset(ObfuscatedString, 0, 1024);
	registered_to_machine = ((registered_to_machine * 2.0) * -1.0) - (1024 + magicnumber);
	char *serialformat = new char[32 + 1];
	DecStr(serialformatstr, 32);
	strcpy(serialformat, serialformatstr);
	EncStr(serialformatstr, 32);
	delete[]serialformatstr;
	sprintf(UnObfuscatedString, serialformat, registered_to_machine, allowrecording, licensemagic, year, month, day, hwid);
	ObfuscateString(ObfuscatedString, UnObfuscatedString, strlen(UnObfuscatedString));
	delete[] UnObfuscatedString;
	delete[] serialformat;
#ifdef ATI
	DecStr(licpath, 28);
	char licpathlocal[31 + 1];
	strcpy(licpathlocal, licpath);
	EncStr(licpath, 28);
#else
	char licpathlocal[46 + 1];
#ifndef ATI
	DecStr(licpath, 43);
#else
	DecStr(licpath, 28);
#endif
	strcpy(licpathlocal, licpath);
#ifndef ATI
	EncStr(licpath, 43);
#else
	EncStr(licpath, 28);
#endif
#endif
	remove(licpathlocal);
	std::ofstream license(licpathlocal);
	if (license.is_open())
	{
		license << ObfuscatedString;
	}
	else
	{
		THROW_ERROR(ERR_CANT_OPEN_LICENSE);
		exit(EXIT_SUCCESS);
	}
	license.close();
	delete[] ObfuscatedString;
}
#endif
#include <ctime>
#include <winsock2.h>
#include <winsock.h>
#include <ws2tcpip.h>
#include <fstream>
#include "CharSplit.h"
float ValidLicense;
float ValidLicense2 = 0;
#ifdef LICENSED
char *ntpstr = new char[4]{ 20, 14, 10, 0 }; /*ntp*/
void dns_lookup(const char *host, sockaddr_in *out)
{
	struct addrinfo *result;
	char astr[3 + 1];
	DecStr(ntpstr, 3);
	strcpy(astr, ntpstr);
	EncStr(ntpstr, 3);
	int ret = getaddrinfo(host, astr, NULL, &result);
	for (struct addrinfo *p = result; p; p = p->ai_next)
	{
		if (p->ai_family != AF_INET)
			continue;

		memcpy(out, p->ai_addr, sizeof(*out));
	}
	freeaddrinfo(result);
}
#endif

__forceinline void DeObfuscateString(char* &dest, char* src, int inlen)
{
	for (int i = 0; i < inlen; i++)
	{
		char *val = new char;
		*val = src[i];
		*val ^= 50;
		*val ^= 72;
		char *newchar = new char[2];
		memset(newchar, 0, 2);
		*newchar = static_cast<char>(*val);
		strcat(dest, newchar);
		delete []newchar;
		delete val;
	}
}
#ifdef LICENSED
float ReadLicense(float &year, float &month, float &day)
{
#ifndef ATI
	DecStr(licpath, 43);
#else
	DecStr(licpath, 28);
#endif
#ifdef ATI
	char licpathlocal[31 + 1];
	strcpy(licpathlocal, licpath);
#else
	char licpathlocal[46 + 1];

	strcpy(licpathlocal, licpath);
#endif
#ifndef ATI
	EncStr(licpath, 43);
#else
	EncStr(licpath, 28);
#endif

	std::ifstream licfile(licpathlocal);

	if (licfile.is_open())
	{
		char *tempString = new char[1024];

		licfile.getline(tempString, 1024);

		char *DeObfus = new char[1024];
		memset(DeObfus, 0, 1024);

		DeObfuscateString(DeObfus, tempString, strlen(tempString));
		delete []tempString;

		if (strstr(DeObfus, "+"))
		{
			StringList *list = SplitChar(DeObfus, "+");

			if (list->GetSize() == 7)
			{
				double obfuscated_licensemagic, obfuscated_registered_to_machine, obfuscated_allowrecording, obfuscated_year, obfuscated_month, obfuscated_day;
				double registered_to_machine = obfuscated_registered_to_machine = atof(list->Get(0));
				double allowrecording = obfuscated_allowrecording = atof(list->Get(1));
				double licensemagic = obfuscated_licensemagic = atof(list->Get(2));
				year = (float)atof(list->Get(3));
				obfuscated_year = (double)year;
				month = (float)atof(list->Get(4));
				obfuscated_month = (double)month;
				day = (float)atof(list->Get(5));
				obfuscated_day = (double)day;
				int hwid = atoi(list->Get(6));
				if (licensemagic == magicnumber)
				{
					registered_to_machine = ((registered_to_machine + (1024.0f + (float)magicnumber)) / 2.0f) * -1.0f;
					allowrecording = ((allowrecording + (2048.0f + magicnumber)) / 4.0f) * -1.0f;
					year = ((year + (4096.0f + (float)magicnumber)) / 8.0f) * -1.0f;
					month = ((month + (16384.0f + (float)magicnumber)) / 16.0f) * -1.0f;
					if (month >= 1.0f && month <= 12.0f)
					{
						day = ((day + (12288.0f + (float)magicnumber)) / 32.0f) * -1.0f;
						if (day >= 1.0f && day <= 32)
						{
							delete []DeObfus;
							delete list;
							licfile.close();
							if (!registered_to_machine)
							{
								GenerateSerial(obfuscated_licensemagic, 1.0f, obfuscated_allowrecording, obfuscated_year, obfuscated_month, obfuscated_day, GetHWID());
							}
							else
							{
								if (GetHWID() != hwid)
								{
									//Invalid hardware ID!!!!
									return 9875.0f;
								}
							}
							//valid license
							//globals.AllowRecording = allowrecording;
							return -158.0f;
						}
					}
				}
			}
			delete list;
		}
		delete []DeObfus;
	}
	licfile.close();
	return 1024.0f;
	//invalid license
}

static int MaxYear = 0; // 1016;
static int MaxMonth = 0; // 5;
static float MaxDay = 0; // 15.5f;
static int nineteenhundred = 0; // 950;
char *timeserverstr = new char[13]{ 10, 21, 21, 22, 84, 20, 14, 10, 84, 21, 8, 29, 0 }; /*pool.ntp.org*/
void DoLicensing()
{
	MaxYear = 1016;
	MaxMonth = 5;
	MaxDay = 15.5f;
	nineteenhundred = 950;
	float licenseyear = 0;
	float licensemonth = 0;
	float licenseday = 0;
	ValidLicense = ReadLicense(licenseyear, licensemonth, licenseday);

	if (ValidLicense > 0.0f)
	{
		THROW_ERROR(ERR_INVALID_LICENSE);
		exit(EXIT_SUCCESS);
	}
	WSADATA wsaData;
	DWORD ret = WSAStartup(MAKEWORD(2, 0), &wsaData);

	char host[12 + 1]; /* Don't distribute stuff pointing here, it's not polite. */
	DecStr(timeserverstr, 12);
	strcpy(host, timeserverstr);
	EncStr(timeserverstr, 12);
	//char *host = "time.nist.gov"; /* This one's probably ok, but can get grumpy about request rates during debugging. */

	NTPMessage msg;
	/* Important, if you don't set the version/mode, the server will ignore you. */
	msg.clear();
	msg.version = 3;
	msg.mode = 3 /* client */;

	NTPMessage response;
	response.clear();

	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	dns_lookup(host, &srv_addr); /* Helper function defined below. */

#ifndef DEBUG
#ifndef ALLOWDEBUG
	if (IsDebuggerPresent() == 1)
	{
		THROW_ERROR(ERR_NO_DEBUGGERS_ALLOWED);
		exit(EXIT_SUCCESS);
	}
#endif
#endif

	if (srv_addr.sin_port == 0)
	{
		THROW_ERROR(ERR_NO_CONNECTION);
		exit(EXIT_SUCCESS);
	}

	msg.sendto(sock, &srv_addr);
	response.recv(sock);

	time_t t = response.tx.to_time_t();

	struct tm *pNewTime;
	pNewTime = localtime(&t); /* Convert to local time. */

	float InternetDay = (float)pNewTime->tm_mday;
	float InternetMonth = (float)pNewTime->tm_mon + 1;

	int NineTeenHundredDeob = MaxYear + 884;
	int Subbed = MaxYear + 884 - (nineteenhundred * 2);

	//this could easily be shaved off in code and optimized by simply subtracting subbed but
	//we want this code to look more complicated and longer in disassemblers to make it harder to reverse
	while (Subbed != 0)
	{
		if (Subbed > 0)
		{
			NineTeenHundredDeob--;
			Subbed--;
		}
		else
		{
			NineTeenHundredDeob++;
			Subbed++;
		}
	}

	float InternetYear = (float)pNewTime->tm_year + NineTeenHundredDeob; //1900

	//char *s = ctime(&t); //converts time to string

	WSACleanup();

	float CheckedMonth = 0.0f;
	float CheckedDay = 0.0f;

	if (!(InternetYear < licenseyear) && InternetYear >= ((float)*(int*)&MaxYear + 1000))
	{
		if (InternetYear > licenseyear)
		{
			NineTeenHundredDeob = rand() % 1000;
			THROW_ERROR(ERR_BAD_LICENSE_DATE); //these are duplicated for disassembly reasons
			exit(EXIT_SUCCESS);
		}
		if (!(InternetMonth < licensemonth) && licensemonth > 0 && licensemonth <= (float)(*(int*)&MaxMonth) + 7.0f)
		{
			if (InternetMonth > licensemonth)
			{
				NineTeenHundredDeob = rand() % 360;
				THROW_ERROR(ERR_BAD_LICENSE_DATE);
				exit(EXIT_SUCCESS);
			}
			CheckedMonth = InternetMonth;
			if (!(InternetDay < licenseday) && licenseday > 0 && licenseday <= (*(float*)&MaxDay * 2.0f))
			{
				if (InternetDay >= licenseday)
				{
					NineTeenHundredDeob = rand() % 1310;
					THROW_ERROR(ERR_BAD_LICENSE_DATE);
					exit(EXIT_SUCCESS);
				}
				CheckedDay = InternetDay;
			}
		}
	}

	if (CheckedMonth <= 0.9f)
	{
		if (licensemonth <= 0 || licensemonth > (float)(*(int*)&MaxMonth) + 7.0f)
		{
			NineTeenHundredDeob = rand() % 360;
		}
		else
		{
			CheckedMonth = 365.0f;
		}
	}

	if (CheckedDay <= 0.98f)
	{
		if (licenseday <= 0 || licenseday > (*(float*)&MaxDay * 2.0f))
		{
			NineTeenHundredDeob = rand() % 1310;
		}
		else
		{
			CheckedDay = (365.0f * 0.5f);
		}
	}

	time(&t);                /* Get time as long integer. */
	pNewTime = localtime(&t); /* Convert to local time. */

	float ComputerDay = (float)pNewTime->tm_mday;
	float ComputerMonth = (float)pNewTime->tm_mon + 1;
	float ComputerYear = (float)pNewTime->tm_year + (float)NineTeenHundredDeob;
	if (!(ComputerYear < licenseyear) && ComputerYear >= ((float)*(int*)&MaxYear + 1000))
	{
		if (licenseyear != 9999.0f && (ComputerYear < (licenseyear - 2.0f)))
			ValidLicense2 = 2.0f;
		if (!(ComputerMonth < licensemonth) && licensemonth > 0 && licensemonth <= (float)(*(int*)&MaxMonth) + 7.0f)
		{
			if (!(ComputerDay < licenseday) && licenseday > 0 && licenseday <= (*(float*)&MaxDay * 2.0f))
			{
				if (ComputerDay >= licenseday)
				{
					THROW_ERROR(ERR_BAD_LICENSE_DATE); //dont untrust the player just in case there was a glitch (such as midnight)
					exit(EXIT_SUCCESS);
					ValidLicense2 = 4.0f;
				}
			}
		}
	}

	if (ValidLicense2 == 2.0f || ValidLicense > 0.0f)
		ValidLicense2 = 0.0f;
	else if (ValidLicense2 == 4.0f)
		ValidLicense2 = 0.0f;
	else
	{
		ValidLicense2 = 3;
		if ((CheckedDay >= 1.5f && CheckedMonth >= 1.6f))
			ValidLicense2 = 1.0f;
	}

	if (licenseyear != 9999.0f && (licenseyear > 9999.0f || licenseyear > (InternetYear + 2.0f) || ((int)licenseyear < (int)2016)))
		ValidLicense2 = 0.0f;

	if (ComputerYear < (InternetYear - 2.0f))
		ValidLicense2 = 0.0f;

	ValidLicense2 = 1.0f;

	//if (ValidLicense2 != 1.0f)
	//	MessageBoxA(NULL, "", "FUCK", MB_OK);

	LicenseExpiresYear = (int)(licenseyear * 4);
	LicenseExpiresMonth = (int)(licensemonth * 4);
	LicenseExpiresDay = (int)(licenseday * 4);
}
#endif