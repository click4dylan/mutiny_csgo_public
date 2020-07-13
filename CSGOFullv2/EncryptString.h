#pragma once

#include "buildserver_chars.h"

inline void ServerSideEncryptDecrypt(char *adr, const unsigned len)
{
	for (unsigned i = 0; i < len; i++)
	{
		adr[i] ^= STRENC_KEY_1;
		adr[i] ^= STRENC_KEY_2;
	}
}

// manual encryption
void DecStr(char *adr, const unsigned len);
void EncStr(char *adr, const unsigned len);
void DecStr(char *adr);
void EncStr(char *adr);