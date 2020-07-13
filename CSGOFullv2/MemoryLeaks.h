#if 0
#pragma once
#include "raw_buffer.h"
#include <mutex>

struct MemoryInfo_t
{
	void* returnaddress;
	void* ptr;
	unsigned size;
	::MemoryInfo_t(void* ret, void* p, unsigned sz)
	{
		returnaddress = ret;
		ptr = p;
		size = sz;
	}
};

extern SmallForwardBuffer m_MemoryAllocationTable;
static std::mutex m_MemoryAllocationMutex;
#endif