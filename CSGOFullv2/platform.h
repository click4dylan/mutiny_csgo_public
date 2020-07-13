#ifndef PLATFORM_H
#define PLATFORM_H

#include "valve_off.h"

#pragma once
#include <malloc.h>
#include <new>
#include <inttypes.h>

#define PLATFORM_WINDOWS 1 
#define IsLinux() false
#define IsOSX() false
#define IsPosix() false
#define IsXbox()	false
#define IsWindows() true
#define IsPC() true
#define IsConsole() false
#define IsX360() false
#define IsPS3() false
#define IS_WINDOWS_PC
#define PLATFORM_WINDOWS_PC 1 // Windows PC
#ifdef _WIN64
#define IsPlatformWindowsPC64() true
#define IsPlatformWindowsPC32() false
#define PLATFORM_WINDOWS_PC64 1
#else
#define IsPlatformWindowsPC64() false
#define IsPlatformWindowsPC32() true
#define PLATFORM_WINDOWS_PC32 1
#endif



#if 0
//-----------------------------------------------------------------------------
// Methods to invoke the constructor, copy constructor, and destructor
//-----------------------------------------------------------------------------

inline int UtlMemory_CalcNewAllocationCount(int nAllocationCount, int nGrowSize, int nNewSize, int nBytesItem)
{
	if (nGrowSize)
		nAllocationCount = ((1 + ((nNewSize - 1) / nGrowSize)) * nGrowSize);
	else
	{
		if (!nAllocationCount)
			nAllocationCount = (31 + nBytesItem) / nBytesItem;

		while (nAllocationCount < nNewSize)
			nAllocationCount *= 2;
	}

	return nAllocationCount;
}
#endif

//-----------------------------------------------------------------------------
// Stack-based allocation related helpers
//-----------------------------------------------------------------------------
#if defined( COMPILER_GCC )

#define stackalloc( _size )		alloca( ALIGN_VALUE( _size, 16 ) )

#ifdef PLATFORM_OSX
#define mallocsize( _p )	( malloc_size( _p ) )
#else
#define mallocsize( _p )	( malloc_usable_size( _p ) )
#endif

#else
// for when we don't care about how many bits we use
#define ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) //  need macro for constant expression
typedef unsigned int uint;
#define  FORCEINLINE_TEMPLATE		__forceinline
#define stackalloc( _size )		_alloca( ALIGN_VALUE( _size, 16 ) )
#define mallocsize( _p )		( _msize( _p ) )

//Little endian things
#define LittleDWord( val )			( val )

//Floating point
#define LittleFloat( pOut, pIn )	( *pOut = *pIn )

#define WordSwap  WordSwapAsm
#pragma warning(push)
#pragma warning (disable:4035) // no return value

template <typename T>
inline T WordSwapAsm(T w)
{
	__asm
	{
		mov ax, w
		xchg al, ah
	}
}

#define DWordSwap DWordSwapAsm
#define BigLong( val )				DWordSwap( val )
#define BigShort( val )				WordSwap( val )
template <typename T>
inline T DWordSwapAsm(T dw)
{
	__asm
	{
		mov eax, dw
		bswap eax
	}
}

inline uint32_t LoadLittleDWord(uint32_t *base, unsigned int dwordIndex)
{
	return LittleDWord(base[dwordIndex]);
}

inline void StoreLittleDWord(uint32_t *base, unsigned int dwordIndex, unsigned long dword)
{
	base[dwordIndex] = LittleDWord(dword);
}

//dbg
#ifdef _DEBUG
#define COMPILE_TIME_ASSERT( pred )	switch(0){case 0:case pred:;}
#define ASSERT_INVARIANT( pred )	static void UNIQUE_ID() { COMPILE_TIME_ASSERT( pred ) }
#else
#define COMPILE_TIME_ASSERT( pred )
#define ASSERT_INVARIANT( pred )
#endif
#endif

#define stackfree( _p ) 0


template <class T>
inline T* Construct(T* pMemory)
{
	return ::new(pMemory) T;
}

template <class T, typename ARG1>
inline T* Construct(T* pMemory, ARG1 a1)
{
	return ::new(pMemory) T(a1);
}

template <class T, typename ARG1, typename ARG2>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2)
{
	return ::new(pMemory) T(a1, a2);
}

template <class T, typename ARG1, typename ARG2, typename ARG3>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3)
{
	return ::new(pMemory) T(a1, a2, a3);
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4)
{
	return ::new(pMemory) T(a1, a2, a3, a4);
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
inline T* Construct(T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5)
{
	return ::new(pMemory) T(a1, a2, a3, a4, a5);
}

template <class T>
inline T* CopyConstruct(T* pMemory, T const& src)
{
	return ::new(pMemory) T(src);
}

template <class T>
inline void Destruct(T* pMemory)
{
	pMemory->~T();

#ifdef _DEBUG
	memset(pMemory, 0xDD, sizeof(T));
#endif
}

#include "valve_on.h"


inline void ThreadPause()
{
	__asm pause;
}

template <typename T>
inline T AlignValue(T val, unsigned alignment)
{
	return (T)(((uintptr_t)val + alignment - 1) & ~(alignment - 1));
}
#endif