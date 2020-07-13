#pragma once

void LogMessage(char* msg);

inline void LogMessage(const char* msg)
{
	LogMessage((char*)msg);
}