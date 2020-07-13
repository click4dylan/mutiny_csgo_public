#pragma once

#if _MSVC_LANG > 201402L

#ifndef _RNG_
#define _RNG_
#ifndef RC_INVOKED

#include <algorithm>
#include <functional>
#include <random>
#include <vector>

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

namespace hh
{
constexpr const char* compiletime = __TIME__;
constexpr int seed				  = static_cast< int >(compiletime[7]) + static_cast< int >(compiletime[6]) * 10 + static_cast< int >(compiletime[4]) * 60 + static_cast< int >(compiletime[3]) * 600 + static_cast< int >(compiletime[1]) * 3600 + static_cast< int >(compiletime[0]) * 36000;

template < int N >
struct random_generator
{
private:
	static constexpr unsigned a = 16807;
	static constexpr unsigned b = 2147483647;

	static constexpr unsigned c = random_generator< (N > 400 ? N % 400 : N) - 1 >::value;
	static constexpr unsigned d = (c + 16807) << 1;
	static constexpr unsigned e = d << 1;
	static constexpr unsigned f = std::clamp(d, std::numeric_limits< unsigned >::min(), std::numeric_limits< unsigned >::max() / 2) + std::clamp(e, std::numeric_limits< unsigned >::min(), std::numeric_limits< unsigned >::max() / 2);

public:
	static constexpr unsigned g		= b;
	static constexpr unsigned value = f > b ? f ^ b : f;
};

template <>
struct random_generator< 0 >
{
	static constexpr unsigned value = seed;
};

template < int N >
struct RandomChar
{
	static const char value = static_cast< char >(1 + random_generator< N >::value);
};

namespace detail
{
	static std::default_random_engine rng_engine(std::random_device{}());

	static __forceinline std::vector< char > charsetAll()
	{
		return { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };
	};

	static __forceinline std::vector< char > charsetHex()
	{
		return { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
	};
} // namespace detail

const auto randcharNormal = []() {
	const std::uniform_int_distribution<> dist(0, (int)detail::charsetAll().size() - 1);
	return detail::charsetAll()[dist(detail::rng_engine)];
};

const auto randcharHex = []() {
	const std::uniform_int_distribution<> dist(0, (int)detail::charsetHex().size() - 1);
	return detail::charsetHex()[dist(detail::rng_engine)];
};

__forceinline std::string RandomString(const std::size_t length, const std::function< char(void) > randfunc = randcharNormal)
{
	std::string ret(length, 0);
	std::generate_n(ret.begin(), length, randfunc);
	return ret;
}
} // namespace hh

#pragma pop_macro("min")
#pragma pop_macro("max")

#endif /* RC_INVOKED */
#endif /* _RNG_ */
#endif

/*
 * Copyright (c) by Sebastian Redinger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V1.0*/