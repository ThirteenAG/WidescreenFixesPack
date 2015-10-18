/*
 * This file is part of the CitizenFX project - http://citizen.re/
 *
 * See LICENSE and MENTIONS in the root of the source tree for information
 * regarding licensing.
 */

#pragma once
#include <vector>
#include "assert.h"

static ptrdiff_t baseAddressDifference;

// sets the base address difference based on an obtained pointer
inline void set_base(uintptr_t address)
{
#ifdef _M_IX86
	uintptr_t addressDiff = (address - 0x400000);
#elif defined(_M_AMD64)
	uintptr_t addressDiff = (address - 0x140000000);
#endif

	// pointer-style cast to ensure unsigned overflow ends up copied directly into a signed value
	baseAddressDifference = *(ptrdiff_t*)&addressDiff;
}

// sets the base to the process main base
inline void set_base()
{
	set_base((uintptr_t)GetModuleHandle(NULL));
}

template<typename T>
inline T* getRVA(uintptr_t rva)
{
	set_base();
#ifdef _M_IX86
	return (T*)(baseAddressDifference + 0x400000 + rva);
#elif defined(_M_AMD64)
	return (T*)(0x140000000 + rva);
#endif
}

namespace hook
{
	class pattern_match
	{
	private:
		void* m_pointer;

	public:
		inline pattern_match(void* pointer)
		{
			m_pointer = pointer;
		}

		template<typename T>
		T* get(int offset)
		{
			char* ptr = reinterpret_cast<char*>(m_pointer);
			return reinterpret_cast<T*>(ptr + offset);
		}

		template<typename T>
		T* get()
		{
			return get<T>(0);
		}
	};

	class pattern
	{
	private:
		std::string m_bytes;
		std::string m_mask;

		uint64_t m_hash;

		size_t m_size;

		std::vector<pattern_match> m_matches;

		bool m_matched;

	protected:
		void* m_module;

	protected:
		inline pattern(void* module)
			: m_module(module)
		{
		}

		void Initialize(const char* pattern, size_t length);

	private:
		bool ConsiderMatch(uintptr_t offset);

		void EnsureMatches(int maxCount);

	public:
		template<size_t Len>
		pattern(const char (&pattern)[Len])
		{
			m_module = getRVA<void>(0);

			Initialize(pattern, Len);
		}

		inline pattern& count(int expected)
		{
			if (!m_matched)
			{
				EnsureMatches(expected);
			}

			assert(m_matches.size() == expected);

			return *this;
		}

		inline size_t size()
		{
			if (!m_matched)
			{
				EnsureMatches(INT_MAX);
			}

			return m_matches.size();
		}

		inline pattern_match& get(int index)
		{
			if (!m_matched)
			{
				EnsureMatches(INT_MAX);
			}

			return m_matches[index];
		}

	public:
		// define a hint
		static void hint(uint64_t hash, uintptr_t address);
	};

	class module_pattern
		: public pattern
	{
	public:
		template<size_t Len>
		module_pattern(void* module, const char(&pattern)[Len])
			: pattern(module)
		{
			Initialize(pattern, Len);
		}
	};
}