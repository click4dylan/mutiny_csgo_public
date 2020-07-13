#pragma once
#include "raw_buffer.h"

class CUtlVectorSimple
{
public:
	unsigned memory;
	char pad[8];
	unsigned int count;
	unsigned pelements;
	inline void* Retrieve(int index, unsigned sizeofdata)
	{
		return (void*)((*(unsigned*)this) + (sizeofdata * index));
	}
	inline void* Base()
	{
		return (void*)memory;
	}
	inline int Count()
	{
		return count;
	}
};

class CUtlVectorSimpleWritable : public SmallForwardBuffer
{
public:
	unsigned int count;
	unsigned pelements;
	CUtlVectorSimpleWritable::CUtlVectorSimpleWritable()
	{
		count = 0;
		pelements = 0;
		SmallForwardBuffer::SmallForwardBuffer();
	}
	void write(void* data, size_t srcsize)
	{
		SmallForwardBuffer::write(data, srcsize);
		count++;
		pelements = (unsigned)buffer;
	}
	template <class T>
	void write(T data)
	{
		SmallForwardBuffer::write(data);
		count++;
		pelements = (unsigned)buffer;
	}
	inline void* Retrieve(int index, unsigned sizeofdata)
	{
		return (void*)((*(unsigned*)this) + (sizeofdata * index));
	}
	inline void* Base()
	{
		return (void*)buffer;
	}
	inline int Count()
	{
		return count;
	}
	inline void RemoveAll()
	{
		if (buffersize)
			free(buffer);
		position = 0;
		buffersize = 0;
		buffer = 0;
	}
	template <class T>
	void AddToTail(T data)
	{
		write(data);
	}
};

#if 0

struct CUtlVector
{
	void *m_pMemory;
	int m_nAllocationCount;
	int m_nGrowSize;
	int m_Size;
	void *m_pElements;
};
#endif