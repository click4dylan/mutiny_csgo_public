#pragma once

#define MD5_DIGEST_LENGTH 16 

// MD5 Hash
typedef struct
{
	unsigned int	buf[4];
	unsigned int	bits[2];
	unsigned char	in[64];
} MD5Context_t;

//-----------------------------------------------------------------------------
// Purpose: Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious initialization constants.

// Input  : *ctx - 
//-----------------------------------------------------------------------------
void MD5Init(MD5Context_t *ctx);

//-----------------------------------------------------------------------------
// Purpose: Update context to reflect the concatenation of another buffer full of bytes.
// Input  : *ctx - 
//			*buf - 
//			len - 
//-----------------------------------------------------------------------------
void MD5Update(MD5Context_t *ctx, unsigned char const *buf, unsigned int len);

//-----------------------------------------------------------------------------
// Purpose: Final wrapup - pad to 64-byte boundary with the bit pattern 
// 1 0* (64-bit count of bits processed, MSB-first)
// Input  : digest[MD5_DIGEST_LENGTH] - 
//			*ctx - 
//-----------------------------------------------------------------------------
void MD5Final(unsigned char digest[MD5_DIGEST_LENGTH], MD5Context_t *ctx);
//-----------------------------------------------------------------------------
// Purpose: generate pseudo random number from a seed number
// Input  : seed number
// Output : pseudo random number
//-----------------------------------------------------------------------------
unsigned int MD5_PseudoRandom(unsigned int nSeed);