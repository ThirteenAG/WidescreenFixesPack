#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES
#pragma warning(disable: 4178 4305 4309 4510 4996)
#include <windows.h>
#include <stdint.h>
#include <array>
#include <math.h>
#include <subauth.h>
#include "IniReader.h"
#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\assembly.hpp"
#include "injector\utility.hpp"
#include "Hooking.Patterns.h"
#pragma warning(default: 4178 4305 4309 4510)
#include "log.h"

#ifndef CEXP
#define CEXP extern "C" __declspec(dllexport)
#endif

float GetFOV(float f, float ar);
float GetFOV2(float f, float ar);
float AdjustFOV(float f, float ar);

void CreateThreadAutoClose(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
std::tuple<int32_t, int32_t> GetDesktopRes();
void GetResolutionsList(std::vector<std::string>& list);
std::string format(const char *fmt, ...);

template<typename T>
std::array<uint8_t, sizeof(T)> to_bytes(const T& object)
{
	std::array<uint8_t, sizeof(T)> bytes;
	const uint8_t* begin = reinterpret_cast<const uint8_t*>(std::addressof(object));
	const uint8_t* end = begin + sizeof(T);
	std::copy(begin, end, std::begin(bytes));
	return bytes;
}

template<typename T>
T& from_bytes(const std::array<uint8_t, sizeof(T)>& bytes, T& object)
{
	static_assert(std::is_trivially_copyable<T>::value, "not a TriviallyCopyable type");
	uint8_t* begin_object = reinterpret_cast<uint8_t*>(std::addressof(object));
	std::copy(std::begin(bytes), std::end(bytes), begin_object);
	return object;
}

template<class T, class T1>
T from_bytes(const T1& bytes)
{
	static_assert(std::is_trivially_copyable<T>::value, "not a TriviallyCopyable type");
	T object;
	uint8_t* begin_object = reinterpret_cast<uint8_t*>(std::addressof(object));
	std::copy(std::begin(bytes), std::end(bytes) - (sizeof(T1) - sizeof(T)), begin_object);
	return object;
}

template <size_t n>
std::string pattern_str(const std::array<uint8_t, n> bytes)
{
	std::string result;
	for (size_t i = 0; i < n; i++)
	{
		result += format("%02X ", bytes[i]);
	}
	return result;
}

template <typename T>
std::string pattern_str(T t)
{
	return std::string((std::is_same<T, char>::value ? format("%c ", t) : format("%02X ", t)));
}

template <typename T, typename... Rest>
std::string pattern_str(T t, Rest... rest)
{
	return std::string((std::is_same<T, char>::value ? format("%c ", t) : format("%02X ", t)) + pattern_str(rest...));
}

template<size_t N>
constexpr size_t length(char const (&)[N])
{
	return N - 1;
}

template <typename T, typename V>
bool iequals(const T& s1, const V& s2)
{
	T str1(s1); T str2(s2);
	std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
	std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);
	return (str1 == str2);
}

class ExeCallbackHandler
{
public:
	static inline void add(std::function<void()>&& fn, const std::string_view s, uint32_t expected = 1)
	{
		if (s.empty() || !hook::pattern(s).count_hint(expected).empty())
		{
			fn();
		}
		else
		{
			auto *p = new ThreadParams{ fn, s, expected };
			CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Thread, (LPVOID)p, 0, NULL);
		}
	}

private:
	struct ThreadParams
	{
		std::function<void()> fn;
		const std::string_view str;
		uint32_t expected;
	};

	static inline DWORD WINAPI Thread(LPVOID p)
	{
		auto params = *static_cast<ExeCallbackHandler::ThreadParams*>(p);
		delete p;

		auto pattern = hook::pattern(params.str);
		while (pattern.clear().count_hint(params.expected).empty()) { Sleep(0); };

		params.fn();

		return 0;
	}
};

class DllCallbackHandler
{
	typedef NTSTATUS(NTAPI* _LdrRegisterDllNotification) (ULONG, PVOID, PVOID, PVOID);
	typedef NTSTATUS(NTAPI* _LdrUnregisterDllNotification) (PVOID);

	typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA {
		ULONG Flags;                    //Reserved.
		PUNICODE_STRING FullDllName;    //The full path name of the DLL module.
		PUNICODE_STRING BaseDllName;    //The base file name of the DLL module.
		PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
		ULONG SizeOfImage;              //The size of the DLL image, in bytes.
	} LDR_DLL_LOADED_NOTIFICATION_DATA, LDR_DLL_UNLOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

	typedef union _LDR_DLL_NOTIFICATION_DATA {
		LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
		LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
	} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

	static inline _LdrRegisterDllNotification   LdrRegisterDllNotification;
	static inline _LdrUnregisterDllNotification LdrUnregisterDllNotification;
	static inline void* cookie;

	static inline void RegisterDllNotification()
	{
		LdrRegisterDllNotification = (_LdrRegisterDllNotification)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrRegisterDllNotification");
		if (LdrRegisterDllNotification && !cookie)
			LdrRegisterDllNotification(0, LdrDllNotification, 0, &cookie);
	}

	static inline void UnRegisterDllNotification()
	{
		LdrUnregisterDllNotification = (_LdrUnregisterDllNotification)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrUnregisterDllNotification");
		if (LdrUnregisterDllNotification && cookie)
			LdrUnregisterDllNotification(cookie);
	}

public:
	static inline void add(std::wstring_view s, std::function<void()>&& fn)
	{
		if (GetModuleHandleW(s.data()) != NULL)
		{
			fn();
		}
		else
		{
			RegisterDllNotification();
			functions.emplace(s, std::forward<std::function<void()>>(fn));
		}
	}
	static inline void call(std::wstring_view s)
	{
		if (functions.count(s.data()))
		{
			functions.at(s.data())();
			functions.erase(s.data());
		}

		if (functions.empty())
		{
			UnRegisterDllNotification();
		}
	}
	static inline void invoke_all()
	{
		for (auto && fn : functions)
			fn.second();
	}
	static inline void CALLBACK LdrDllNotification(ULONG NotificationReason, PLDR_DLL_NOTIFICATION_DATA NotificationData, PVOID Context)
	{
		static constexpr auto LDR_DLL_NOTIFICATION_REASON_LOADED = 1;
		if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
		{
			call(NotificationData->Loaded.BaseDllName->Buffer);
		}
	}

private:
	struct Comparator {
		bool operator() (const std::wstring& s1, const std::wstring& s2) const {
			std::wstring str1(s1.length(), ' ');
			std::wstring str2(s2.length(), ' ');
			std::transform(s1.begin(), s1.end(), str1.begin(), tolower);
			std::transform(s2.begin(), s2.end(), str2.begin(), tolower);
			return  str1 < str2;
		}
	};
	static /*inline*/ std::map<std::wstring, std::function<void()>, Comparator> functions;
};