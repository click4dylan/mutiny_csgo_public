
#pragma once
#include "utlmemory.h"

#if 0
#ifndef UTLMEMORY_H
#define UTLMEMORY_H

#pragma once

#include "platform.h"

#include "memalloc.h"
#include "memdbgon.h"

#pragma warning (disable:4100)
#pragma warning (disable:4514)

#ifdef UTLMEMORY_TRACK
#define UTLMEMORY_TRACK_ALLOC()		MemAlloc_RegisterAllocation( "Sum of all UtlMemory", 0, m_nAllocationCount * sizeof(T), m_nAllocationCount * sizeof(T), 0 )
#define UTLMEMORY_TRACK_FREE()		if ( !m_pMemory ) ; else MemAlloc_RegisterDeallocation( "Sum of all UtlMemory", 0, m_nAllocationCount * sizeof(T), m_nAllocationCount * sizeof(T), 0 )
#else
#define UTLMEMORY_TRACK_ALLOC()		((void)0)
#define UTLMEMORY_TRACK_FREE()		((void)0)
#endif

template< class T, class I = int >
class CUtlMemory
{
public:

	enum
	{
		EXTERNAL_BUFFER_MARKER = -1,
		EXTERNAL_CONST_BUFFER_MARKER = -2,
	};

	//-----------------------------------------------------------------------------
	// constructor, destructor
	//-----------------------------------------------------------------------------

	CUtlMemory(int nGrowSize = 0, int nInitAllocationCount = 0) : m_pMemory(0),
		m_nAllocationCount(nInitAllocationCount), m_nGrowSize(nGrowSize)
	{
		ValidateGrowSize();
		//Assert(nGrowSize >= 0);
		if (m_nAllocationCount)
		{
			UTLMEMORY_TRACK_ALLOC();
			MEM_ALLOC_CREDIT_CLASS();
			m_pMemory = (T*)malloc(m_nAllocationCount * sizeof(T));
		}
	}

	CUtlMemory(T* pMemory, int numElements) : m_pMemory(pMemory),
		m_nAllocationCount(numElements)
	{
		// Special marker indicating externally supplied modifyable memory
		m_nGrowSize = EXTERNAL_BUFFER_MARKER;
	}

	CUtlMemory(const T* pMemory, int numElements) : m_pMemory((T*)pMemory),
		m_nAllocationCount(numElements)
	{
		// Special marker indicating externally supplied modifyable memory
		m_nGrowSize = EXTERNAL_CONST_BUFFER_MARKER;
	}

	~CUtlMemory()
	{
		Purge();
	}

	void ValidateGrowSize()
	{
	}


	T & operator[](I i)
	{
		return m_pMemory[i];
	}

	const T& operator[](I i) const
	{
		return m_pMemory[(unsigned int)i];
	}

	T* Base()
	{
		return m_pMemory;
	}

	int NumAllocated() const
	{
		return m_nAllocationCount;
	}

	void Grow(int num = 1)
	{
		if (IsExternallyAllocated())
			return;

		int nAllocationRequested = m_nAllocationCount + num;
		int nNewAllocationCount = UtlMemory_CalcNewAllocationCount(m_nAllocationCount, m_nGrowSize, nAllocationRequested, sizeof(T));

		if ((int)(I)nNewAllocationCount < nAllocationRequested)
		{
			if ((int)(I)nNewAllocationCount == 0 && (int)(I)(nNewAllocationCount - 1) >= nAllocationRequested)
				--nNewAllocationCount;
			else
			{
				if ((int)(I)nAllocationRequested != nAllocationRequested)
					return;

				while ((int)(I)nNewAllocationCount < nAllocationRequested)
					nNewAllocationCount = (nNewAllocationCount + nAllocationRequested) / 2;
			}
		}

		m_nAllocationCount = nNewAllocationCount;

		if (m_pMemory)
		{
			T* new_data = (T*)realloc(m_pMemory, m_nAllocationCount * sizeof(T));
			if (new_data == nullptr)
			{
				abort();
			}
			else
				m_pMemory = new_data;
		}
		else
			m_pMemory = (T*)malloc(m_nAllocationCount * sizeof(T));
	}

	void Purge()
	{
		if (!IsExternallyAllocated())
		{
			if (m_pMemory)
			{
				free((void*)m_pMemory);
				m_pMemory = 0;
			}
			m_nAllocationCount = 0;
		}
	}

	bool IsExternallyAllocated() const
	{
		return m_nGrowSize < 0;
	}

	void EnsureCapacity(int num)
	{
		if (m_nAllocationCount >= num)
			return;

		if (IsExternallyAllocated())
		{
			return;
		}

		m_nAllocationCount = num;

		if (m_pMemory)
		{
			T* new_data = (T*)realloc(m_pMemory, m_nAllocationCount * sizeof(T));
			if (new_data == nullptr)
			{
				abort();
			}
			else
				m_pMemory = new_data;
		}
		else
		{
			m_pMemory = (T*)malloc(m_nAllocationCount * sizeof(T));
		}
	}

	bool IsIdxValid(int i) const
	{
		return false;
	}

	static const I INVALID_INDEX = (I)-1; // For use with COMPILE_TIME_ASSERT
	static I InvalidIndex() { return INVALID_INDEX; }

protected:
	T * m_pMemory;
	int m_nAllocationCount;
	int m_nGrowSize;
};

template< typename T, size_t SIZE, int nAlignment = 0 >
class CUtlMemoryFixed
{
public:
	// constructor, destructor
	CUtlMemoryFixed(int nGrowSize = 0, int nInitSize = 0) {  }
	CUtlMemoryFixed(T* pMemory, int numElements) {  }

	// Can we use this index?
	bool IsIdxValid(int i) const { return (i >= 0) && (i < SIZE); }

	// Specify the invalid ('null') index that we'll only return on failure
	static const int INVALID_INDEX = -1; // For use with COMPILE_TIME_ASSERT
	static int InvalidIndex() { return INVALID_INDEX; }

	// Gets the base address
	T* Base() { if (nAlignment == 0) return (T*)(&m_Memory[0]); else return (T*)AlignValue(&m_Memory[0], nAlignment); }
	const T* Base() const { if (nAlignment == 0) return (T*)(&m_Memory[0]); else return (T*)AlignValue(&m_Memory[0], nAlignment); }

	// element access
	T& operator[](int i) { return Base()[i]; }
	const T& operator[](int i) const { return Base()[i]; }
	T& Element(int i) { return Base()[i]; }
	const T& Element(int i) const { return Base()[i]; }

	// Attaches the buffer to external memory....
	void SetExternalBuffer(T* pMemory, int numElements) {  }

	// Size
	int NumAllocated() const { return SIZE; }
	int Count() const { return SIZE; }

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow(int num = 1) {  }

	// Makes sure we've got at least this much memory
	void EnsureCapacity(int num) {  }

	// Memory deallocation
	void Purge() {}

	// Purge all but the given number of elements (NOT IMPLEMENTED IN CUtlMemoryFixed)
	void Purge(int numElements) {  }

	// is the memory externally allocated?
	bool IsExternallyAllocated() const { return false; }

	// Set the size by which the memory grows
	void SetGrowSize(int size) {}

	class Iterator_t
	{
	public:
		Iterator_t(int i) : index(i) {}
		int index;
		bool operator==(const Iterator_t it) const { return index == it.index; }
		bool operator!=(const Iterator_t it) const { return index != it.index; }
	};
	Iterator_t First() const { return Iterator_t(IsIdxValid(0) ? 0 : InvalidIndex()); }
	Iterator_t Next(const Iterator_t &it) const { return Iterator_t(IsIdxValid(it.index + 1) ? it.index + 1 : InvalidIndex()); }
	int GetIndex(const Iterator_t &it) const { return it.index; }
	bool IsIdxAfter(int i, const Iterator_t &it) const { return i > it.index; }
	bool IsValidIterator(const Iterator_t &it) const { return IsIdxValid(it.index); }
	Iterator_t InvalidIterator() const { return Iterator_t(InvalidIndex()); }

private:
	char m_Memory[SIZE * sizeof(T) + nAlignment];
};

#include "memdbgoff.h"

#endif
#endif