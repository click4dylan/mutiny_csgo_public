#pragma once

#include "MemoryLeaks.h"

class IMemAlloc;
extern IMemAlloc *g_pMemAlloc;
extern bool g_bIsExiting;

IMemAlloc* GetValveAllocator();