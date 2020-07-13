#ifndef UTLFIXEDMEMORY_H
#define UTLFIXEDMEMORY_H

#pragma once

#include "platform.h"

#include "memalloc.h"
#include "memdbgon.h"

#pragma warning (disable:4100)
#pragma warning (disable:4514)

template< class T >
class CUtlFixedMemory
{
protected:
	struct BlockHeader_t;
public:
	// constructor, destructor
	CUtlFixedMemory(int nGrowSize = 0, int nInitAllocationCount = 0)
		: m_pBlocks(0), m_nAllocationCount(0), m_nGrowSize(0)
	{
		Init(nGrowSize, nInitAllocationCount);
	}

	~CUtlFixedMemory()
	{
		Purge();
	}


	T & operator[](int i)
	{
		return *(T*)i;
	}

	const T& operator[](int i) const
	{
		return *(T*)i;
	}

	class Iterator_t
	{
	public:
		Iterator_t(BlockHeader_t *p, int i) : m_pBlockHeader(p), m_nIndex(i) {}
		BlockHeader_t *m_pBlockHeader;
		int m_nIndex;

		bool operator==(const Iterator_t it) const { return m_pBlockHeader == it.m_pBlockHeader && m_nIndex == it.m_nIndex; }
		bool operator!=(const Iterator_t it) const { return m_pBlockHeader != it.m_pBlockHeader || m_nIndex != it.m_nIndex; }
	};

	static const int INVALID_INDEX = 0; // For use with COMPILE_TIME_ASSERT
	static int InvalidIndex() { return INVALID_INDEX; }

	bool IsIdxValid(int i) const
	{
		return i != InvalidIndex();
	}

	bool IsInBlock(int i, BlockHeader_t *pBlockHeader) const
	{
		T *p = (T*)i;
		const T *p0 = HeaderToBlock(pBlockHeader);
		return p >= p0 && p < p0 + pBlockHeader->m_nBlockSize;
	}

	int GetIndex(const Iterator_t &it) const
	{
		if (!IsValidIterator(it))
			return InvalidIndex();

		return (int)(HeaderToBlock(it.m_pBlockHeader) + it.m_nIndex);
	}

	bool IsIdxAfter(int i, const Iterator_t &it) const
	{
		if (!IsValidIterator(it))
			return false;

		if (IsInBlock(i, it.m_pBlockHeader))
			return i > GetIndex(it);

		for (BlockHeader_t * __restrict pbh = it.m_pBlockHeader->m_pNext; pbh; pbh = pbh->m_pNext)
		{
			if (IsInBlock(i, pbh))
				return true;
		}

		return false;
	}

protected:
	bool IsValidIterator(const Iterator_t &it) const { return it.m_pBlockHeader && it.m_nIndex >= 0 && it.m_nIndex < it.m_pBlockHeader->m_nBlockSize; }

	const T *HeaderToBlock(const BlockHeader_t *pHeader) const { return (T*)(pHeader + 1); }

	struct BlockHeader_t
	{
		BlockHeader_t *m_pNext;
		int m_nBlockSize;
	};

	BlockHeader_t* m_pBlocks;
	int m_nAllocationCount;
	int m_nGrowSize;
};

#include "memdbgoff.h"

#endif