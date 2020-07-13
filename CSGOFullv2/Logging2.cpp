#include "precompiled.h"
#include <fstream>
#include <Shlwapi.h>
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <ctime>   // localtime
#include <chrono>  // chrono::system_clock
#include <sstream> // stringstream
#include <iomanip> // put_time
//https://stackoverflow.com/questions/17933917/get-the-users-desktop-folder-using-windows-api

#pragma comment (lib, "shlwapi.lib")
//shlwapi.lib

// the caller can set buffer=NULL and buflen=0 to calculate the needed buffer size
void desktop_directory(LPWSTR buffer, int buflen)
{
	if (SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, buffer) != S_OK)
	{
		MessageBoxW(NULL, L"ERROR in SHGetFolderPathW", L"", MB_OK);
	}
}

//https://stackoverflow.com/questions/17223096/outputting-date-and-time-in-c-using-stdchrono
std::string return_current_time_and_date()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
	return ss.str();
}

void LogMessage(char* msg)
{
	static std::ofstream file;

	if (!file.is_open())
	{
		wchar_t path[MAX_PATH + 1] = { 0 };
		wchar_t fulladdress[MAX_PATH + 1];
		desktop_directory(path, sizeof(path));
		PathCombineW(fulladdress, path, TEXT(L"crash.txt"));
		file.open(fulladdress, std::ios::out);
	}

	if (file.is_open())
	{
		std::string timestamp = return_current_time_and_date();
		file << timestamp << " | " << msg << std::endl; //note: std::enl flushes to disk immediately so if we do lots of logging, use \n and flush when necessary
	}
	else
	{
		MessageBoxW(NULL, L"ERROR in Logging", L"", MB_OK);
	}
}
