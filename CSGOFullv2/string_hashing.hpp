// modern c++ string hashing header
#pragma once

#if _MSVC_LANG > 201402L

#ifndef _STRING_HASHING_
#define _STRING_HASHING_
#ifndef RC_INVOKED

#include <cstdint>

namespace hh
{
#ifndef _WIN64
template < class _Ty >
_NODISCARD _CONSTEXPR17 uint64_t fnv(const _Ty* const _Str,
									 const uint64_t _Hash = 0x811c9dc5) noexcept
{
	return (_Str[0] == NULL) ? _Hash : fnv< _Ty >(&_Str[1], (_Hash ^ uint32_t(_Str[0])) * 0x1000193);
}
#else
template < class _Ty >
_NODISCARD _CONSTEXPR17 uint64_t fnv(const _Ty* const _Str,
									 const uint64_t _Hash = 0xcbf29ce484222325) noexcept
{
	return (_Str[0] == NULL) ? _Hash : fnv< _Ty >(&_Str[1], (_Hash ^ uint64_t(_Str[0])) * 0x100000001b3);
}
#endif
} // namespace hh

#define HASH_STRINGA(string) hh::fnv< char >(string)
#define HASH_STRINGW(string) hh::fnv< wchar_t >(string)

#endif /* RC_INVOKED */
#endif /* _STRING_HASHING_ */
#endif

/*
 * Copyright (c) by Sebastian Redinger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V1.0*/