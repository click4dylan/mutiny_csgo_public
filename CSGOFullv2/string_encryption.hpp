// modern c++ string encryption header
#pragma once

#if _MSVC_LANG > 201402L

#ifndef _STRING_ENCRYPTION_
#define _STRING_ENCRYPTION_
#ifndef RC_INVOKED

#include "rng.hpp"
#include <array>
#include <execution>
#include <limits>

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

namespace hh::polymorph
{
namespace detail
{
	// CLASS TEMPLATE char_crypted_rt
	template < class _Ty >
	class char_crypted_rt
	{
	public:
		using value_type	  = _Ty;
		using const_reference = const _Ty&;

		static _NODISCARD _CONSTEXPR17 value_type
		crypt(value_type _Char, const_reference _Key) noexcept
		{
			return _Char ^ _Key;
		}

		_CONSTEXPR17 char_crypted_rt(value_type _Ch, value_type _CryptK) noexcept :
			_Char(crypt(_Ch, _CryptK)), _CryptKey(_CryptK)
		{
		}

		_CONSTEXPR17 void crypt() noexcept
		{
			_Char ^= _CryptKey;
		}

		_NODISCARD _CONSTEXPR17 value_type get_decrypted() const noexcept
		{
			return crypt(_Char, _CryptKey);
		}

		_Ty _Char;
		_Ty _CryptKey;
	};

	// CLASS TEMPLATE string_decrypted_rt
	template < class _Ty >
	class string_decrypted_rt
	{
	public:
		using value_type	= _Ty;
		using size_type		= size_t;
		using const_pointer = const _Ty*;

		string_decrypted_rt(string_decrypted_rt&&) noexcept		 = default;
		string_decrypted_rt(const string_decrypted_rt&) noexcept = default;
		string_decrypted_rt& operator=(const string_decrypted_rt&) noexcept = delete;
		string_decrypted_rt& operator=(string_decrypted_rt&&) noexcept = delete;

		_CONSTEXPR17 string_decrypted_rt(const std::vector< char_crypted_rt< value_type > >& data) noexcept
		{
			for (size_type i = 0; i < data.size(); ++i)
				_StringDecrypted.emplace_back(data[i].get_decrypted());
			_StringDecrypted.push_back('\0');
		}

		template < size_t _Size >
		_CONSTEXPR17 string_decrypted_rt(const std::array< value_type, _Size >& data) noexcept
		{
			for (size_type i = 0; i < _Size; ++i)
				_StringDecrypted.emplace_back(data[i]);
			_StringDecrypted.push_back('\0');
		}

		~string_decrypted_rt() noexcept
		{
			for (size_type i = 0; i < _StringDecrypted.size(); ++i)
				_StringDecrypted[i] = '\0';
		}

		__forceinline _NODISCARD _CONSTEXPR17 const_pointer get() const noexcept
		{
			return _StringDecrypted.data();
		}

		_NODISCARD _CONSTEXPR17 size_type length() const noexcept
		{
			return _StringDecrypted.size();
		}

		std::vector< value_type > _StringDecrypted;
	};
} // namespace detail

// CLASS TEMPLATE string_crypted_rt
template < class _Ty = char >
class string_crypted_rt
{
public:
	using value_type	= _Ty;
	using size_type		= size_t;
	using pointer		= _Ty*;
	using const_pointer = const _Ty*;

	template < size_t _Size >
	_CONSTEXPR17 string_crypted_rt(_Ty const (&str)[_Size]) noexcept
	{
		for (size_t i = 0; i < _Size - 1; ++i)
		{
			_StringCrypted.emplace_back(detail::char_crypted_rt< value_type >(str[i], (value_type)hh::random_generator< _Size >::value));
		}
	}

	template < size_t... _Is, size_t _Size >
	explicit _CONSTEXPR17
	string_crypted_rt(value_type const (&str)[_Size],
					  std::integer_sequence< size_t, _Is... >,
					  value_type _CryptKey = static_cast< value_type >(
						  hh::random_generator< _Size >::value)) noexcept :
		_StringCrypted{
			detail::char_crypted< value_type >(str[_Is], _CryptKey)...
		} {}

	explicit _CONSTEXPR17 string_crypted_rt(const_pointer str) noexcept
	{
		if (!str)
			return;
		for (size_t i = 0; i < std::char_traits< value_type >::length(str); ++i)
			_StringCrypted.emplace_back(detail::char_crypted_rt< value_type >(str[i], static_cast< value_type >(rand() % std::numeric_limits< value_type >::max())));
	}

	_CONSTEXPR17 string_crypted_rt(nullptr_t) noexcept {}

	_NODISCARD string_crypted_rt& operator=(const nullptr_t&) noexcept
	{
		return *this;
	}

	_NODISCARD string_crypted_rt& operator=(nullptr_t&&) noexcept
	{
		return *this;
	}

	_NODISCARD string_crypted_rt& operator+(string_crypted_rt< value_type >&& other)
	{
		for (size_t i = 0; i < other.length(); ++i)
			_StringCrypted.emplace_back(other.get_char_crypted(i));
		return *this;
	}

	_NODISCARD string_crypted_rt& operator+(detail::char_crypted_rt< value_type >&& other)
	{
		_StringCrypted.emplace_back(other);
		return *this;
	}

	_NODISCARD _CONSTEXPR17 size_type length() const noexcept
	{
		return _StringCrypted.size();
	}

	_NODISCARD _CONSTEXPR17 detail::char_crypted_rt< value_type >
	get_char_crypted(const size_type i) const noexcept
	{
		return _StringCrypted.at(i);
	}

	__forceinline _NODISCARD detail::string_decrypted_rt< value_type > get() const noexcept
	{
		return detail::string_decrypted_rt< value_type >(_StringCrypted);
	}

	std::vector< detail::char_crypted_rt< value_type > > _StringCrypted;
};

// CLASS TEMPLATE string_crypted_ct
template < size_t _Size = 0,
		   class _Ty	= char,
		   int _Counter = __COUNTER__ >
class string_crypted_ct
{
public:
	using value_type	= _Ty;
	using size_type		= size_t;
	using pointer		= _Ty*;
	using const_pointer = const _Ty*;

	string_crypted_ct(string_crypted_ct&&)		= delete;
	string_crypted_ct(const string_crypted_ct&) = delete;
	_CONSTEXPR17 string_crypted_ct() noexcept   = default;

	__forceinline explicit _CONSTEXPR17 string_crypted_ct(std::nullptr_t) noexcept
	{
	}

	template < size_t... Is >
	__forceinline explicit _CONSTEXPR17 string_crypted_ct(
		const_pointer str,
		std::integer_sequence< size_t, Is... >) noexcept :
		_Crypt_key(hh::random_generator< _Counter + _Size >::value),
		_Crypt_key_rotated(_Crypt_key),
		_Encrypted{ crypt(str[Is], _Crypt_key_rotated)... }
	{
	}

	_NODISCARD _CONSTEXPR17 string_crypted_ct& operator=(const string_crypted_ct& other) noexcept
	{
		_Crypt_key		   = other.get_crypt_key();
		_Crypt_key_rotated = other.get_crypt_key_rotated();
		_Encrypted		   = other._Encrypted;

		return this;
	}
	_NODISCARD _CONSTEXPR17 string_crypted_ct& operator=(string_crypted_ct&& other) noexcept
	{
		_Crypt_key		   = other.get_crypt_key();
		_Crypt_key_rotated = other.get_crypt_key_rotated();
		_Encrypted		   = other._Encrypted;

		return this;
	}

	~string_crypted_ct() noexcept
	{
		std::fill_n(std::execution::par_unseq, reinterpret_cast< char* >(this), sizeof(*this), 0);
	}

	_CONSTEXPR17 size_type length() const noexcept
	{
		return _Size;
	}

	_NODISCARD _CONSTEXPR17 value_type get_char_crypted(const size_t i) const noexcept
	{
		return _Encrypted[i];
	}

	__forceinline _NODISCARD detail::string_decrypted_rt< value_type > get() const noexcept
	{
		auto ck = get_crypt_key();
		std::array< _Ty, _Size > m_decrypted;
		for (size_t i = 0; i < _Size; ++i)
			m_decrypted[i] = crypt(_Encrypted[i], ck);

		return detail::string_decrypted_rt< _Ty >(m_decrypted);
	}

	static __forceinline _NODISCARD _CONSTEXPR17 value_type crypt(const value_type c, uint32_t& key)
	{
		// TODO: fix metamorph continuos
		return c ^ /*(key = _rotr(key, key))*/ key;
	}

	_NODISCARD _CONSTEXPR17 uint32_t get_crypt_key() const
	{
		return _Crypt_key;
	}

	_NODISCARD _CONSTEXPR17 uint32_t get_crypt_key_rotated() const
	{
		return _Crypt_key_rotated;
	}

	const uint32_t _Crypt_key   = 0;
	uint32_t _Crypt_key_rotated = _Crypt_key;
	const std::array< _Ty, _Size + 1 > _Encrypted{};
};

} // namespace hh::polymorph

#pragma pop_macro("min")
#pragma pop_macro("max")

// TODO: fix these below

typedef hh::polymorph::string_crypted_rt< char > EncStrA;
typedef hh::polymorph::string_crypted_rt< wchar_t > EncStrW;

#define ENCRYPT_STRINGA(string) (hh::polymorph::string_crypted_ct< sizeof string - 1, char, __COUNTER__ >(string, std::make_index_sequence< sizeof(string) - 1 >()))
#define ENCRYPT_STRINGW(string) (hh::polymorph::string_crypted_ct< sizeof string - 1, wchar_t, __COUNTER__ >(string, std::make_index_sequence< sizeof(string) - 1 >()))
#define ENCRYPT_MEMORY(memory) (hh::polymorph::string_crypted_ct< sizeof memory - 1, uint8_t, __COUNTER__ >(memory, std::make_index_sequence< sizeof(memory) - 1 >()))

#define GET_ENCRYPT_STRINGA(string) \
	([]() { return ENCRYPT_STRINGA(string).get(); }().get())
#define GET_ENCRYPT_STRINGW(string) \
	([]() { return ENCRYPT_STRINGW(string).get(); }().get())
//#define GET_ENCRYPT_MEMORY(string) ([]() { return
//ENCRYPT_MEMORY(string).get(); }().get())

#endif /* RC_INVOKED */
#endif /* _STRING_ENCRYPTION_ */
#endif

/*
 * Copyright (c) by Sebastian Redinger. All rights reserved.
 * Consult your license regarding permissions and restrictions.
V1.0*/