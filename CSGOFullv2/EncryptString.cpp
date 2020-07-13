#include "precompiled.h"
#include "EncryptString.h"
#include <string>

// manual encryption
void DecStr(char *adr, const unsigned len)
{
	for (unsigned i = 0; i < len; i++)
	{
		adr[i] ^= 50;
		adr[i] ^= 72;
	}
}

void EncStr(char *adr, const unsigned len)
{
	for (unsigned i = 0; i < len; i++)
	{
		adr[i] ^= 72;
		adr[i] ^= 50;
	}
}

void DecStr(char *adr)
{
	size_t len = strlen(adr);
	for (size_t i = 0; i < len; i++)
	{
		adr[i] ^= 50;
		adr[i] ^= 72;
	}
}

void EncStr(char *adr)
{
	size_t len = strlen(adr);
	for (size_t i = 0; i < len; i++)
	{
		adr[i] ^= 72;
		adr[i] ^= 50;
	}
}