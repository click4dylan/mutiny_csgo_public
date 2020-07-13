#pragma once

#define WIN32_LEAN_AND_MEAN
//#define NOMINMAX
#define FIRST_TIME_HERE ([] { static std::atomic<bool> first_time(true); return first_time.exchange(false); } ())
#define _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <cassert>
#include <unordered_map>
#include <fstream>
#include <deque>
#include <string>
#include <vector>
#include <typeindex>

/* DIRECT X */
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )

/* JSON */
//#include "json.hpp"
#include "C:\Developer\Sync\Framework\Files\JSON\JSON.hpp"
using json = nlohmann::json;

/* IMGUI */
#include "ImGui/imgui.h"
#include "ImGui/dx9/imgui_impl_dx9.h"
#include "ImGui/win32/imgui_impl_win32.h"

/* GLOBAL */
#include "singleton.hpp"
#include "../buildserver_chars.h"
#include "../misc.h"
#include "../EncryptString.h"
#include "../string_encrypt_include.h"
#include "variable.hpp"