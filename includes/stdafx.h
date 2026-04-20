#pragma once
#include "targetver.h"
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES
#pragma warning(push)
#pragma warning(disable: 4178 4244 4305 4309 4510 4996)
#include <windows.h>
#include <stdint.h>
#include <array>
#include <math.h>
#include <subauth.h>
#include <thread>
#include <mutex>
#include <set>
#include <map>
#include <iomanip>
#include <future>
#include <shlobj.h>
#include "IniReader.h"
#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\assembly.hpp"
#include "injector\utility.hpp"
#include "Hooking.Patterns.h"
#include "callbacks.h"
#include "log.h"
#include "ModuleList.hpp"
#include "Trampoline.h"
#include <filesystem>
#include <stacktrace>
#include <shellapi.h>
#include <ranges>
#include <format>
#include <hidusage.h>
#pragma warning(pop)

#ifndef CEXP
#define CEXP extern "C" __declspec(dllexport)
#endif

#define force_return_address(addr) (*(uintptr_t*)(regs.esp - 4) = (addr))
#define return_to(addr) do { force_return_address(addr); return; } while (0)
#define WM_RAWINPUTMOUSE (WM_APP + 1000)

float GetFOV(float f, float ar);
float GetFOV2(float f, float ar);
float AdjustFOV(float f, float ar, float base_ar = (4.0f / 3.0f));
float CalculateWidescreenOffset(float fWidth, float fHeight, float fScaleToWidth = 640.0f, float fScaleToHeight = 480.0f, float fOffset = 0.0f, bool bScaleToActualRes = false);
std::optional<float> ParseWidescreenHudOffset(std::string_view input);
float ClampHudAspectRatio(float value, float screenAspect, float minAspect = 4.0f / 3.0f, float maxAspect = 32.0f / 9.0f);

bool IsModuleUAL(HMODULE mod);
bool IsUALPresent();
void CreateThreadAutoClose(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
std::tuple<int32_t, int32_t> GetDesktopRes();
std::vector<std::tuple<int, int, int>> GetResolutionsList(bool uniqueByRefresh = true);
void GetResolutionsList(std::vector<std::string>& list, bool includeRefreshRate = false);
uint32_t GetDesktopRefreshRate();
uint32_t crc32(uint32_t crc, const void* buf, size_t size);

HICON CreateIconFromBMP(UCHAR* data);
HICON CreateIconFromResourceICO(UINT nID, int32_t cx, int32_t cy);

BOOL CreateProcessInJob(
    HANDLE hJob,
    LPCTSTR lpApplicationName,
    LPTSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCTSTR lpCurrentDirectory,
    LPSTARTUPINFO lpStartupInfo,
    LPPROCESS_INFORMATION ppi);

BOOL CreateProcessInJobAsAdmin(
    HANDLE hJob,
    LPCTSTR lpApplicationName,
    LPTSTR lpCommandLine,
    int nShow,
    LPCTSTR lpCurrentDirectory,
    LPPROCESS_INFORMATION ppi);

template<typename T>
std::array<uint8_t, sizeof(T)> to_bytes(const T& object)
{
    std::array<uint8_t, sizeof(T)> bytes;
    const uint8_t* begin = reinterpret_cast<const uint8_t*>(std::addressof(object));
    const uint8_t* end = begin + sizeof(T);
    std::copy(begin, end, std::begin(bytes));
    return bytes;
}

template<typename T, size_t N>
auto to_bytes(const T(&arr)[N]) -> std::array<uint8_t, N - 1>
{
    std::array<uint8_t, N - 1> bytes;
    const uint8_t* begin = reinterpret_cast<const uint8_t*>(arr);
    std::copy(begin, begin + N - 1, std::begin(bytes));
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
    result.reserve(n * 3);
    for (size_t i = 0; i < n; i++)
    {
        result += std::format("{:02X} ", bytes[i]);
    }
    return result;
}

template <typename T>
std::string pattern_str(T t)
{
    if constexpr (std::is_same<T, char>::value)
        return std::format("{} ", t);
    else
        return std::format("{:02X} ", t);
}

template <typename T, typename... Rest>
std::string pattern_str(T t, Rest... rest)
{
    std::string prefix;
    if constexpr (std::is_same<T, char>::value)
        prefix = std::format("{} ", t);
    else
        prefix = std::format("{:02X} ", t);
    return prefix + pattern_str(rest...);
}

template <size_t count = 1, typename... Args>
hook::pattern find_pattern(Args... args)
{
    hook::pattern pattern;
    ((pattern = hook::pattern(args), !pattern.count_hint(count).empty()) || ...);
    return pattern;
}

template <size_t count = 1, typename... Args>
hook::pattern find_module_pattern(HMODULE hModule, Args... args)
{
    hook::pattern pattern;
    ((pattern = hook::module_pattern(hModule, args), !pattern.count_hint(count).empty()) || ...);
    return pattern;
}

template <size_t count = 1, typename... Args>
hook::pattern find_range_pattern(uintptr_t range_start, size_t range_size, Args... args)
{
    hook::pattern pattern;
    ((pattern = hook::range_pattern(range_start, range_size, args), !pattern.count_hint(count).empty()) || ...);
    return pattern;
}

template<size_t N>
constexpr size_t length(char const (&)[N])
{
    return N - 1;
}

inline bool iequals(const std::string_view s1, const std::string_view s2)
{
    std::string str1(s1); std::string str2(s2);
    std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
    std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);
    return (str1 == str2);
}

inline bool iequals(const std::wstring_view s1, const std::wstring_view s2)
{
    std::wstring str1(s1); std::wstring str2(s2);
    std::transform(str1.begin(), str1.end(), str1.begin(), ::towlower);
    std::transform(str2.begin(), str2.end(), str2.begin(), ::towlower);
    return (str1 == str2);
}

inline bool starts_with(const std::string_view str, const std::string_view prefix, bool case_sensitive)
{
    if (!case_sensitive)
    {
        std::string str1(str); std::string str2(prefix);
        std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
        std::transform(str2.begin(), str2.end(), str2.begin(), ::tolower);
        return str1.starts_with(str2);
    }
    return str.starts_with(prefix);
}

inline bool starts_with(const std::wstring_view str, const std::wstring_view prefix, bool case_sensitive)
{
    if (!case_sensitive)
    {
        std::wstring str1(str); std::wstring str2(prefix);
        std::transform(str1.begin(), str1.end(), str1.begin(), ::towlower);
        std::transform(str2.begin(), str2.end(), str2.begin(), ::towlower);
        return str1.starts_with(str2);
    }
    return str.starts_with(prefix);
}

template<class T = std::filesystem::path>
T GetModulePath(HMODULE hModule)
{
    static constexpr auto INITIAL_BUFFER_SIZE = MAX_PATH;
    static constexpr auto MAX_ITERATIONS = 7;

    if constexpr (std::is_same_v<T, std::filesystem::path>)
    {
        std::u16string ret;
        std::filesystem::path pathret;
        auto bufferSize = INITIAL_BUFFER_SIZE;
        for (size_t iterations = 0; iterations < MAX_ITERATIONS; ++iterations)
        {
            ret.resize(bufferSize);
            size_t charsReturned = 0;
            charsReturned = GetModuleFileNameW(hModule, (LPWSTR)&ret[0], bufferSize);
            if (charsReturned < ret.length())
            {
                ret.resize(charsReturned);
                pathret = ret;
                return pathret;
            }
            else
            {
                bufferSize *= 2;
            }
        }
    }
    else
    {
        T ret;
        auto bufferSize = INITIAL_BUFFER_SIZE;
        for (size_t iterations = 0; iterations < MAX_ITERATIONS; ++iterations)
        {
            ret.resize(bufferSize);
            size_t charsReturned = 0;
            if constexpr (std::is_same_v<T, std::string>)
                charsReturned = GetModuleFileNameA(hModule, &ret[0], bufferSize);
            else
                charsReturned = GetModuleFileNameW(hModule, &ret[0], bufferSize);
            if (charsReturned < ret.length())
            {
                ret.resize(charsReturned);
                return ret;
            }
            else
            {
                bufferSize *= 2;
            }
        }
    }
    return T();
}

template<class T = std::filesystem::path>
T GetThisModulePath()
{
    HMODULE hm = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetDesktopRefreshRate, &hm);
    T r = GetModulePath<T>(hm);
    if constexpr (std::is_same_v<T, std::filesystem::path>)
        return r.parent_path();
    else if constexpr (std::is_same_v<T, std::string>)
        r = r.substr(0, r.find_last_of("/\\") + 1);
    else
        r = r.substr(0, r.find_last_of(L"/\\") + 1);
    return r;
}

template<class T = std::filesystem::path>
T GetThisModuleName()
{
    HMODULE hm = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetDesktopRefreshRate, &hm);
    const T moduleFileName = GetModulePath<T>(hm);

    if constexpr (std::is_same_v<T, std::filesystem::path>)
        return moduleFileName.filename();
    else if constexpr (std::is_same_v<T, std::string>)
        return moduleFileName.substr(moduleFileName.find_last_of("/\\") + 1);
    else
        return moduleFileName.substr(moduleFileName.find_last_of(L"/\\") + 1);
}

template<class T = std::filesystem::path>
T GetExeModulePath()
{
    T r = GetModulePath<T>(NULL);

    if constexpr (std::is_same_v<T, std::filesystem::path>)
        return r.parent_path();
    else if constexpr (std::is_same_v<T, std::string>)
        r = r.substr(0, r.find_last_of("/\\") + 1);
    else
        r = r.substr(0, r.find_last_of(L"/\\") + 1);
    return r;
}

template<class T = std::filesystem::path>
T GetExeModuleName()
{
    const T moduleFileName = GetModulePath<T>(NULL);
    if constexpr (std::is_same_v<T, std::filesystem::path>)
        return moduleFileName.filename();
    else if constexpr (std::is_same_v<T, std::string>)
        return moduleFileName.substr(moduleFileName.find_last_of("/\\") + 1);
    else
        return moduleFileName.substr(moduleFileName.find_last_of(L"/\\") + 1);
}

template<class T = std::filesystem::path>
T GetCurrentDirectoryW()
{
    static constexpr auto INITIAL_BUFFER_SIZE = MAX_PATH;
    static constexpr auto MAX_ITERATIONS = 7;
    if constexpr (std::is_same_v<T, std::filesystem::path>)
    {
        std::u16string ret;
        std::filesystem::path pathret;
        auto bufferSize = INITIAL_BUFFER_SIZE;
        for (size_t iterations = 0; iterations < MAX_ITERATIONS; ++iterations)
        {
            ret.resize(bufferSize);
            size_t charsReturned = 0;
            charsReturned = GetCurrentDirectoryW(bufferSize, (LPWSTR)&ret[0]);
            if (charsReturned < ret.length())
            {
                ret.resize(charsReturned);
                pathret = ret;
                return pathret;
            }
            else
            {
                bufferSize *= 2;
            }
        }
    }
    else
    {
        T ret;
        auto bufferSize = INITIAL_BUFFER_SIZE;
        for (size_t iterations = 0; iterations < MAX_ITERATIONS; ++iterations)
        {
            ret.resize(bufferSize);
            size_t charsReturned = 0;
            if constexpr (std::is_same_v<T, std::string>)
                charsReturned = GetCurrentDirectoryA(bufferSize, &ret[0]);
            else
                charsReturned = GetCurrentDirectoryW(bufferSize, &ret[0]);
            if (charsReturned < ret.length())
            {
                ret.resize(charsReturned);
                return ret;
            }
            else
            {
                bufferSize *= 2;
            }
        }
    }
    return T();
}

template <typename T>
inline std::wstring int_to_hex(T val, size_t width = sizeof(T) * 2)
{
    std::wstringstream ss;
    ss << std::uppercase << std::setfill(L'0') << std::setw(width) << std::hex << (val | 0);
    return ss.str();
}

template <typename T>
inline std::string int_to_hex_str(T val, size_t width = sizeof(T) * 2)
{
    std::stringstream ss;
    ss << std::uppercase << std::setfill('0') << std::setw(width) << std::hex << (val | 0);
    return ss.str();
}

template <typename T>
bool fileExists(T fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

inline std::filesystem::path GetKnownFolderPath(REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken)
{
    std::filesystem::path r;
    WCHAR* szSystemPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(rfid, dwFlags, hToken, &szSystemPath)))
    {
        r = szSystemPath;
    }
    CoTaskMemFree(szSystemPath);
    return r;
};

template<typename... Ts>
FARPROC FindProcAddress(HMODULE hModule, Ts... procNames)
{
    FARPROC result = nullptr;
    ((result = ::GetProcAddress(hModule, procNames)) || ...);
    return result;
}

class RegistryWrapper
{
private:
    static std::string filter;
    static CIniReader RegistryReader;
    static std::map<std::string, std::string> DefaultStrings;
    static std::set<std::string> PathStrings;

    // Thread-local state so concurrent registry calls on different threads don't corrupt each other
    static thread_local std::string section;

public:
    static inline DWORD OverrideTypeREG_NONE = REG_NONE;

    RegistryWrapper(std::string_view searchString, std::filesystem::path iniPath)
    {
        filter = searchString;
        RegistryReader.SetIniPath(iniPath);
        std::filesystem::create_directories(std::filesystem::path(iniPath).parent_path());
    }

    static void AddDefault(std::string_view key, std::string_view value)
    {
        DefaultStrings.emplace(key, value);
    }

    template<typename... Args>
    static void AddPathWriter(Args... args)
    {
        PathStrings = { std::string(args)... };  // explicit std::string for safety with string_view
        for (const auto& key : PathStrings)
        {
            AddDefault(key, GetExeModulePath<std::string>());
        }
    }

    static void AddPathWriterWithPath(std::string_view key, std::string_view merge)
    {
        PathStrings.emplace(key);
        AddDefault(key, GetExeModulePath<std::string>() + std::string(merge) + "\\");
    }

    static LSTATUS WINAPI RegCloseKey(HKEY hKey)
    {
        if (hKey == NULL)
            return ERROR_SUCCESS;
        return ::RegCloseKey(hKey);
    }

    static LSTATUS WINAPI RegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
    {
        if (hKey == NULL)
        {
            *phkResult = NULL;
            return ERROR_SUCCESS;
        }
        if (strstr(lpSubKey, filter.c_str()) != NULL)
        {
            *phkResult = NULL;
            section = lpSubKey;
            auto pos = section.find_last_of('\\');
            section = (pos == std::string::npos) ? section : section.substr(pos + 1);
            return ERROR_SUCCESS;
        }
        return ::RegCreateKeyA(hKey, lpSubKey, phkResult);
    }

    static LSTATUS WINAPI RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
    {
        if (strstr(lpSubKey, filter.c_str()) != NULL)
        {
            *phkResult = NULL;
            section = lpSubKey;
            auto pos = section.find_last_of('\\');
            section = (pos == std::string::npos) ? section : section.substr(pos + 1);
            return ERROR_SUCCESS;
        }
        return ::RegOpenKeyA(hKey, lpSubKey, phkResult);
    }

    static LSTATUS WINAPI RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
    {
        if (strstr(lpSubKey, filter.c_str()) != NULL)
        {
            *phkResult = NULL;
            section = lpSubKey;
            auto pos = section.find_last_of('\\');
            section = (pos == std::string::npos) ? section : section.substr(pos + 1);
            return ERROR_SUCCESS;
        }
        return ::RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
    }

    static LSTATUS WINAPI RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass,
                                          DWORD dwOptions, REGSAM samDesired,
                                          CONST LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                          PHKEY phkResult, LPDWORD lpdwDisposition)
    {
        if (strstr(lpSubKey, filter.c_str()) != NULL)
        {
            *phkResult = NULL;
            section = lpSubKey;
            auto pos = section.find_last_of('\\');
            section = (pos == std::string::npos) ? section : section.substr(pos + 1);
            if (lpdwDisposition) *lpdwDisposition = REG_OPENED_EXISTING_KEY;  // reasonable default
            return ERROR_SUCCESS;
        }
        return ::RegCreateKeyExA(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired,
                                 lpSecurityAttributes, phkResult, lpdwDisposition);
    }

    static LSTATUS WINAPI RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
                                           LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
    {
        if (hKey != NULL)
            return ::RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

        // === INTERCEPTED KEY ===
        std::string ValueName = lpValueName ? lpValueName : "@";

        // Determine type hint (what the caller expects)
        DWORD regType = REG_SZ;
        if (lpType != nullptr)
            regType = *lpType;

        if (OverrideTypeREG_NONE)
            regType = OverrideTypeREG_NONE;

        // Fetch value from INI (uses DefaultStrings if key not present)
        auto it = DefaultStrings.find(ValueName);
        std::string defaultValue = (it != DefaultStrings.end()) ? it->second : std::string{};
        std::string readValue = RegistryReader.ReadString(section, ValueName, defaultValue);

        // Placeholder logic (original behavior): if value is still at default, write a marker
        // (skipped for PathStrings keys)
        bool isDefaultOrEmpty = readValue.empty() || readValue == defaultValue;
        bool shouldWritePlaceholder = isDefaultOrEmpty && (PathStrings.find(ValueName) == PathStrings.end());

        switch (regType)
        {
            case REG_BINARY:
            {
                std::vector<BYTE> bytes;
                if (!readValue.empty())
                {
                    std::istringstream input(readValue);
                    std::string token;
                    while (std::getline(input, token, ','))
                    {
                        if (!token.empty())
                        {
                            try
                            {
                                auto val = std::stoul(token, nullptr, 16);
                                bytes.push_back(static_cast<BYTE>(val & 0xFF));
                            } catch (...) {}
                        }
                    }
                }

                DWORD required = static_cast<DWORD>(bytes.size());
                if (lpType) *lpType = REG_BINARY;

                if (lpData == nullptr)
                {
                    if (lpcbData) *lpcbData = required;
                    return ERROR_SUCCESS;
                }

                if (*lpcbData < required)
                {
                    *lpcbData = required;
                    return ERROR_MORE_DATA;
                }

                if (required > 0)
                    std::memcpy(lpData, bytes.data(), required);

                *lpcbData = required;

                if (shouldWritePlaceholder && !defaultValue.empty())
                    RegistryReader.WriteString(section, ValueName, defaultValue);

                return ERROR_SUCCESS;
            }

            case REG_DWORD:
            {
                DWORD value = 0;
                if (!readValue.empty())
                {
                    try { value = static_cast<DWORD>(std::stoul(readValue)); } catch (...) { value = 0; }
                }
                else if (!defaultValue.empty())
                {
                    try { value = static_cast<DWORD>(std::stoul(defaultValue)); } catch (...) { value = 0; }
                }

                // Original special case for UINT_MAX
                if (value == UINT_MAX)
                {
                    value = 0;
                    RegistryReader.WriteString(section, ValueName,
                        defaultValue.empty() ? "INSERTDWORDHERE" : defaultValue);
                }

                if (lpType) *lpType = REG_DWORD;

                constexpr DWORD required = sizeof(DWORD);

                if (lpData == nullptr)
                {
                    if (lpcbData) *lpcbData = required;
                    return ERROR_SUCCESS;
                }

                if (*lpcbData < required)
                {
                    *lpcbData = required;
                    return ERROR_MORE_DATA;
                }

                *reinterpret_cast<DWORD*>(lpData) = value;
                *lpcbData = required;

                if (shouldWritePlaceholder)
                    RegistryReader.WriteString(section, ValueName,
                        defaultValue.empty() ? "INSERTDWORDHERE" : defaultValue);

                return ERROR_SUCCESS;
            }

            case REG_MULTI_SZ:
            {
                // Convert comma-separated INI value back to proper double-null-terminated multi-sz
                std::vector<std::string> parts;
                if (!readValue.empty())
                {
                    std::istringstream iss(readValue);
                    std::string part;
                    while (std::getline(iss, part, ','))
                        parts.push_back(std::move(part));
                }

                DWORD required = 0;
                for (const auto& p : parts)
                    required += static_cast<DWORD>(p.length() + 1);
                required += 1; // final null terminator

                if (lpType) *lpType = REG_MULTI_SZ;

                if (lpData == nullptr)
                {
                    if (lpcbData) *lpcbData = required;
                    return ERROR_SUCCESS;
                }

                if (*lpcbData < required)
                {
                    *lpcbData = required;
                    return ERROR_MORE_DATA;
                }

                BYTE* dst = lpData;
                for (const auto& p : parts)
                {
                    if (!p.empty())
                        std::memcpy(dst, p.data(), p.length());
                    dst += p.length();
                    *dst++ = '\0';
                }
                *dst = '\0'; // final terminator

                *lpcbData = required;

                if (shouldWritePlaceholder && !defaultValue.empty())
                    RegistryReader.WriteString(section, ValueName, defaultValue);

                return ERROR_SUCCESS;
            }

            case REG_NONE:
            {
                if (lpData != nullptr)
                {
                    // Special original behavior: read boolean from "MAIN" section
                    bool val = RegistryReader.ReadBoolean("MAIN", ValueName, false);
                    *reinterpret_cast<bool*>(lpData) = val;
                    if (lpcbData) *lpcbData = sizeof(bool);
                    if (lpType) *lpType = REG_NONE;
                    return ERROR_SUCCESS;
                }

                // Size-only query (original behavior)
                auto s = DefaultStrings.find(ValueName);
                if (s != DefaultStrings.end())
                {
                    if (lpcbData) *lpcbData = static_cast<DWORD>(s->second.size()); // original did NOT add +1
                    if (lpType) *lpType = 1; // REG_SZ
                    return ERROR_SUCCESS;
                }
                return ERROR_FILE_NOT_FOUND;
            }

            case REG_SZ:
            case REG_EXPAND_SZ:
            default:  // treat unknown types as REG_SZ (safest)
            {
                if (lpType) *lpType = (regType == REG_EXPAND_SZ) ? REG_EXPAND_SZ : REG_SZ;

                DWORD required = static_cast<DWORD>(readValue.length() + 1); // + null terminator

                if (lpData == nullptr)
                {
                    if (lpcbData) *lpcbData = required;
                    return ERROR_SUCCESS;
                }

                if (*lpcbData < required)
                {
                    *lpcbData = required;
                    return ERROR_MORE_DATA;
                }

                if (!readValue.empty())
                    std::memcpy(lpData, readValue.data(), readValue.length());
                lpData[readValue.length()] = '\0';

                *lpcbData = required;

                if (shouldWritePlaceholder)
                    RegistryReader.WriteString(section, ValueName,
                        defaultValue.empty() ? "INSERTSTRINGDATAHERE" : defaultValue);

                return ERROR_SUCCESS;
            }
        }
    }

    static LSTATUS WINAPI RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved,
                                         DWORD dwType, const BYTE* lpData, DWORD cbData)
    {
        if (hKey != NULL)
            return ::RegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);

        if (lpValueName == nullptr)
            return ERROR_INVALID_PARAMETER;

        std::string ValueName = lpValueName;

        if (OverrideTypeREG_NONE)
            dwType = OverrideTypeREG_NONE;

        switch (dwType)
        {
            case REG_BINARY:
            {
                std::string str;
                if (cbData > 0 && lpData != nullptr)
                {
                    for (DWORD i = 0; i < cbData; ++i)
                    {
                        str += std::format("{:02x}", lpData[i]);
                        if (i != cbData - 1)
                            str += ',';
                    }
                }
                RegistryReader.WriteString(section, ValueName, str);
                break;
            }

            case REG_DWORD:
            {
                DWORD val = (cbData >= sizeof(DWORD) && lpData != nullptr)
                    ? *reinterpret_cast<const DWORD*>(lpData)
                    : 0;
                RegistryReader.WriteInteger(section, ValueName, val);
                break;
            }

            case REG_MULTI_SZ:
            {
                std::string str;
                if (lpData != nullptr && cbData > 2)  // at least one empty string + double null
                {
                    const char* p = reinterpret_cast<const char*>(lpData);
                    const char* end = p + cbData;
                    while (p < end && *p != '\0')
                    {
                        if (!str.empty())
                            str += ',';
                        str.append(p);
                        p += std::strlen(p) + 1;
                    }
                }
                RegistryReader.WriteString(section, ValueName, str);
                break;
            }

            case REG_NONE:
            {
                // Original special behavior
                RegistryReader.WriteBoolean("MAIN", ValueName, true);
                break;
            }

            case REG_SZ:
            case REG_EXPAND_SZ:
            default:
            {
                std::string val;
                if (lpData != nullptr && cbData > 0)
                {
                    val.assign(reinterpret_cast<const char*>(lpData), cbData);
                    // Remove trailing null if present (registry usually includes it)
                    if (!val.empty() && val.back() == '\0')
                        val.pop_back();
                }
                RegistryReader.WriteString(section, ValueName, val);
                break;
            }
        }
        return ERROR_SUCCESS;
    }

    static LSTATUS WINAPI RegQueryValueA(HKEY hKey, LPCSTR lpSubKey, LPSTR lpData, PLONG lpcbData)
    {
        if (hKey != NULL)
        {
            return ::RegQueryValueA(hKey, lpSubKey, lpData, lpcbData);
        }

        DWORD dwType = REG_SZ;
        DWORD cbData = (lpcbData != nullptr) ? static_cast<DWORD>(*lpcbData) : 0;

        LSTATUS status = RegQueryValueExA(
            NULL,           // our magic intercepted key
            NULL,           // lpValueName = NULL becomes "@" inside RegQueryValueExA
            nullptr,        // lpReserved
            &dwType,        // we force REG_SZ (what RegQueryValueA expects)
            reinterpret_cast<LPBYTE>(lpData),
            &cbData
        );

        // Convert back to the PLONG size that the caller expects
        if (lpcbData != nullptr)
            *lpcbData = static_cast<LONG>(cbData);

        return status;
    }

    // Unimplemented stubs
    static LSTATUS WINAPI RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
    {
        if (hKey == NULL) return ERROR_SUCCESS;
        return ::RegDeleteKeyA(hKey, lpSubKey);
    }

    static LSTATUS WINAPI RegEnumKeyA(HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cchName)
    {
        if (hKey == NULL) return ERROR_SUCCESS;
        return ::RegEnumKeyA(hKey, dwIndex, lpName, cchName);
    }
};

class RegistryFallback
{
private:
    static std::string s_MainPath;
    static std::string s_FallbackPath;
    static std::map<std::string, std::string> s_Defaults;

public:
    static void Init(std::string_view mainPath, std::string_view fallbackPath = {})
    {
        s_MainPath = mainPath;
        s_FallbackPath = fallbackPath;
        s_Defaults.clear();
    }

    static void AddDefault(std::string_view key, std::string_view value)
    {
        if (!key.empty())
            s_Defaults.emplace(key, value);
    }

    static void EnsureDefaults()
    {
        if (s_MainPath.empty())
            return;

        std::vector<std::string> paths = { s_MainPath };
        if (!s_FallbackPath.empty())
            paths.push_back(s_FallbackPath);

        for (const auto& path : paths)
        {
            HKEY hKey = nullptr;
            DWORD dwDisposition = 0;

            LSTATUS status = ::RegCreateKeyExA(
                HKEY_CURRENT_USER,
                path.c_str(),
                0, nullptr,
                REG_OPTION_NON_VOLATILE,
                KEY_READ | KEY_WRITE,
                nullptr,
                &hKey,
                &dwDisposition);

            if (status != ERROR_SUCCESS)
                continue;

            for (const auto& [key, valueStr] : s_Defaults)
            {
                char* end = nullptr;
                DWORD dwordVal = strtoul(valueStr.c_str(), &end, 10);
                if (end && *end == '\0')
                    SetDwordIfMissing(hKey, key.c_str(), dwordVal);
                else
                    SetStringIfMissing(hKey, key.c_str(), valueStr);
            }

            ::RegCloseKey(hKey);
        }
    }

    static LSTATUS WINAPI RegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
    {
        if (hKey == HKEY_LOCAL_MACHINE)
            hKey = HKEY_CURRENT_USER;

        return ::RegCreateKeyA(hKey, lpSubKey, phkResult);
    }

    static LSTATUS WINAPI RegQueryValueA(HKEY hKey, LPCSTR lpSubKey, LPSTR lpData, PLONG lpcbData)
    {
        if (hKey == HKEY_LOCAL_MACHINE)
            hKey = HKEY_CURRENT_USER;

        return ::RegQueryValueA(hKey, lpSubKey, lpData, lpcbData);
    }

    static LSTATUS WINAPI RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
    {
        if (hKey == HKEY_LOCAL_MACHINE)
            hKey = HKEY_CURRENT_USER;

        return ::RegDeleteKeyA(hKey, lpSubKey);
    }

    static LSTATUS WINAPI RegEnumKeyA(HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cchName)
    {
        if (hKey == HKEY_LOCAL_MACHINE)
            hKey = HKEY_CURRENT_USER;

        return ::RegEnumKeyA(hKey, dwIndex, lpName, cchName);
    }

    static LSTATUS WINAPI RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved,
                                               LPSTR lpClass, DWORD dwOptions, REGSAM samDesired,
                                               CONST LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                               PHKEY phkResult, LPDWORD lpdwDisposition)
    {
        if (hKey == HKEY_LOCAL_MACHINE)
            hKey = HKEY_CURRENT_USER;

        return ::RegCreateKeyExA(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired,
                                 lpSecurityAttributes, phkResult, lpdwDisposition);
    }

    static LSTATUS WINAPI RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions,
                                             REGSAM samDesired, PHKEY phkResult)
    {
        if (hKey == HKEY_LOCAL_MACHINE)
            hKey = HKEY_CURRENT_USER;

        return ::RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
    }

    static LSTATUS WINAPI RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
    {
        if (hKey == HKEY_LOCAL_MACHINE)
            hKey = HKEY_CURRENT_USER;

        return ::RegOpenKeyA(hKey, lpSubKey, phkResult);
    }

    static LSTATUS WINAPI RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved,
                                              DWORD dwType, const BYTE* lpData, DWORD cbData)
    {
        return ::RegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
    }

    static LSTATUS WINAPI RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
                                                LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
    {
        return ::RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    }

    static LSTATUS WINAPI RegCloseKey(HKEY hKey)
    {
        return ::RegCloseKey(hKey);
    }

private:
    static void SetDwordIfMissing(HKEY hKey, const char* valueName, DWORD defaultValue)
    {
        DWORD type = 0, data = 0, cb = sizeof(DWORD);
        LSTATUS query = RegQueryValueExA(hKey, valueName, nullptr, &type, (LPBYTE)&data, &cb);

        if (query != ERROR_SUCCESS || type != REG_DWORD || cb != sizeof(DWORD))
        {
            LSTATUS set = RegSetValueExA(hKey, valueName, 0, REG_DWORD,
                                              reinterpret_cast<const BYTE*>(&defaultValue), sizeof(DWORD));
        }
    }

    static void SetStringIfMissing(HKEY hKey, const char* valueName, std::string_view defaultValue)
    {
        const char* lpValue = (valueName && strcmp(valueName, "@") == 0) ? nullptr : valueName;

        DWORD type = 0, cb = 0;
        LSTATUS query = RegQueryValueExA(hKey, lpValue, nullptr, &type, nullptr, &cb);

        if (query != ERROR_SUCCESS || type != REG_SZ)
        {
            std::string val(defaultValue);
            if (val.empty()) val = " ";

            LSTATUS set = RegSetValueExA(hKey, lpValue, 0, REG_SZ,
                                              reinterpret_cast<const BYTE*>(val.c_str()),
                                              static_cast<DWORD>(val.length() + 1));
        }
    }
};

namespace AffinityChanges
{
    static inline DWORD_PTR gameThreadAffinity = 0;
    static inline bool Init()
    {
        DWORD_PTR processAffinity, systemAffinity;
        if (!GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity))
        {
            return false;
        }

        DWORD_PTR otherCoresAff = (processAffinity - 1) & processAffinity;
        if (otherCoresAff == 0) // Only one core is available for the game
        {
            return false;
        }
        gameThreadAffinity = processAffinity & ~otherCoresAff;

        SetThreadAffinityMask(GetCurrentThread(), gameThreadAffinity);

        return true;
    }

    static inline HANDLE WINAPI CreateThread_GameThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress,
        PVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
    {
        HANDLE hThread = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags | CREATE_SUSPENDED, lpThreadId);
        if (hThread != nullptr)
        {
            SetThreadAffinityMask(hThread, gameThreadAffinity);
            if ((dwCreationFlags & CREATE_SUSPENDED) == 0) // Resume only if the game didn't pass CREATE_SUSPENDED
            {
                ResumeThread(hThread);
            }
        }
        return hThread;
    }
}

namespace WindowedModeWrapper
{
    static bool bBorderlessWindowed = true;
    static bool bEnableWindowResize = false;
    static bool bScaleWindow = false;
    static bool bStretchWindow = false;
    static HWND GameHWND = NULL;
    static int desiredClientWidth = 0;
    static int desiredClientHeight = 0;
    static bool s_cursorOnNcArea = false;

    static std::tuple<int, int, int, int> beforeCreateWindow(int nWidth, int nHeight)
    {
        // fix the window to open at the center of the screen...
        HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX info = { sizeof(MONITORINFOEX) };
        GetMonitorInfo(monitor, &info);
        DEVMODE devmode = {};
        devmode.dmSize = sizeof(DEVMODE);
        EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
        DWORD DesktopX = devmode.dmPelsWidth;
        DWORD DesktopY = devmode.dmPelsHeight;

        int newWidth = nWidth;
        int newHeight = nHeight;

        if (bScaleWindow)
        {
            float fAspectRatio = static_cast<float>(nWidth) / static_cast<float>(nHeight);
            newHeight = DesktopY;
            newWidth = static_cast<int>(newHeight * fAspectRatio);
        }

        if (bStretchWindow)
        {
            newHeight = DesktopY;
            newWidth = DesktopX;
        }

        // Save the final desired client dimensions
        desiredClientWidth = newWidth;
        desiredClientHeight = newHeight;

        int WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)newWidth / 2.0f));
        int WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)newHeight / 2.0f));

        SetProcessDPIAware();

        return std::make_tuple(WindowPosX, WindowPosY, newWidth, newHeight);
    }

    static void afterCreateWindow()
    {
        LONG lStyle = GetWindowLong(GameHWND, GWL_STYLE);

        lStyle &= ~(WS_HSCROLL | WS_VSCROLL);

        if (bBorderlessWindowed)
            lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        else
        {
            lStyle |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
            if (bEnableWindowResize)
                lStyle |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
        }

        SetWindowLong(GameHWND, GWL_STYLE, lStyle);

        if (!bBorderlessWindowed && desiredClientWidth > 0 && desiredClientHeight > 0)
        {
            // Compute outer window size so client rect = desired dimensions
            RECT rect = { 0, 0, desiredClientWidth, desiredClientHeight };
            LONG exStyle = GetWindowLong(GameHWND, GWL_EXSTYLE);
            AdjustWindowRectEx(&rect, lStyle, FALSE, exStyle);
            int outerW = rect.right - rect.left;
            int outerH = rect.bottom - rect.top;

            // Center by client area, not outer window
            HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX info = { sizeof(MONITORINFOEX) };
            GetMonitorInfo(monitor, &info);
            DEVMODE devmode = {};
            devmode.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);

            int posX = ((int)devmode.dmPelsWidth - desiredClientWidth) / 2 + rect.left;
            int posY = ((int)devmode.dmPelsHeight - desiredClientHeight) / 2 + rect.top;

            SetWindowPos(GameHWND, 0, posX, posY, outerW, outerH, SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        else
        {
            SetWindowPos(GameHWND, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    static BOOL WINAPI AdjustWindowRect_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
    {
        DWORD newStyle = 0;

        if (!bBorderlessWindowed)
            newStyle = WS_CAPTION;
        else
        {
            afterCreateWindow();
            return AdjustWindowRect(lpRect, GetWindowLong(GameHWND, GWL_STYLE), bMenu);
        }

        return AdjustWindowRect(lpRect, newStyle, bMenu);
    }

    static BOOL WINAPI AdjustWindowRectEx_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
    {
        DWORD newStyle = 0;

        if (!bBorderlessWindowed)
            newStyle = WS_CAPTION;
        else
        {
            afterCreateWindow();
            return AdjustWindowRectEx(lpRect, GetWindowLong(GameHWND, GWL_STYLE), bMenu, dwExStyle);
        }

        return AdjustWindowRectEx(lpRect, newStyle, bMenu, dwExStyle);
    }

    static BOOL WINAPI CenterWindowPosition(int nWidth, int nHeight)
    {
        // fix the window to open at the center of the screen...
        HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX info = { sizeof(MONITORINFOEX) };
        GetMonitorInfo(monitor, &info);
        DEVMODE devmode = {};
        devmode.dmSize = sizeof(DEVMODE);
        EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
        DWORD DesktopX = devmode.dmPelsWidth;
        DWORD DesktopY = devmode.dmPelsHeight;

        int newWidth = nWidth;
        int newHeight = nHeight;

        if (bScaleWindow)
        {
            float fAspectRatio = static_cast<float>(nWidth) / static_cast<float>(nHeight);
            newHeight = DesktopY;
            newWidth = static_cast<int>(newHeight * fAspectRatio);
        }

        if (bStretchWindow)
        {
            newHeight = DesktopY;
            newWidth = DesktopX;
        }

        int WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)newWidth / 2.0f));
        int WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)newHeight / 2.0f));

        return SetWindowPos(GameHWND, 0, WindowPosX, WindowPosY, newWidth, newHeight, SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    static HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        if (hWndParent && hWndParent != GetDesktopWindow())
            return CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

        auto [WindowPosX, WindowPosY, newWidth, newHeight] = beforeCreateWindow(nWidth, nHeight);
        GameHWND = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, WindowPosX, WindowPosY, newWidth, newHeight, hWndParent, hMenu, hInstance, lpParam);
        afterCreateWindow();
        return GameHWND;
    }

    static HWND WINAPI CreateWindowExW_Hook(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        if (hWndParent && hWndParent != GetDesktopWindow())
            return CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

        auto [WindowPosX, WindowPosY, newWidth, newHeight] = beforeCreateWindow(nWidth, nHeight);
        GameHWND = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, WindowPosX, WindowPosY, newWidth, newHeight, hWndParent, hMenu, hInstance, lpParam);
        afterCreateWindow();
        return GameHWND;
    }

    static LONG WINAPI SetWindowLongA_Hook(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        if (bBorderlessWindowed)
        {
            afterCreateWindow();
            RECT rect;
            GetClientRect(hWnd, &rect);
            CenterWindowPosition(rect.right, rect.bottom);
            return dwNewLong;
        }
        else
        {
            dwNewLong |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
            if (bEnableWindowResize)
                dwNewLong |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
        }

        return SetWindowLongA(hWnd, GWL_STYLE, dwNewLong);
    }

    static LONG WINAPI SetWindowLongW_Hook(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        if (bBorderlessWindowed)
        {
            afterCreateWindow();
            RECT rect;
            GetClientRect(hWnd, &rect);
            CenterWindowPosition(rect.right, rect.bottom);
            return dwNewLong;
        }
        else
        {
            dwNewLong |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
            if (bEnableWindowResize)
                dwNewLong |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
        }

        return SetWindowLongW(hWnd, GWL_STYLE, dwNewLong);
    }

    static BOOL WINAPI SetWindowPos_Hook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
    {
        BOOL res = SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
        if (bBorderlessWindowed)
        {
            afterCreateWindow();
            RECT rect;
            GetClientRect(hWnd, &rect);
            CenterWindowPosition(rect.right, rect.bottom);
            return TRUE;
        }
        return res;
    }

    static BOOL WINAPI MoveWindow_Hook(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
    {
        BOOL res = MoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
        if (bBorderlessWindowed && GameHWND == hWnd)
        {
            CenterWindowPosition(nWidth, nHeight);
        }
        return res;
    }

    static int WINAPI ShowCursor_Hook(BOOL bShow)
    {
        if (!bBorderlessWindowed && GameHWND && !bShow)
        {
            POINT pt;
            GetCursorPos(&pt);
            RECT clientRect;
            GetClientRect(GameHWND, &clientRect);
            MapWindowPoints(GameHWND, NULL, (LPPOINT)&clientRect, 2);

            if (!PtInRect(&clientRect, pt))
            {
                if (!s_cursorOnNcArea)
                {
                    // Entering NC area — force counter to 0 (visible)
                    s_cursorOnNcArea = true;
                    int count;
                    do { count = ShowCursor(TRUE); } while (count < 0);
                }
                return 0; // Suppress the hide
            }
            else if (s_cursorOnNcArea)
            {
                // Back on client area — let game hide normally
                s_cursorOnNcArea = false;
            }
        }

        return ShowCursor(bShow);
    }
};

class IATHook
{
public:
    template <class... Ts>
    static auto Replace(HMODULE target_module, std::string_view dll_name, Ts&& ... inputs)
    {
        std::map<std::string, std::future<void*>> originalPtrs;

        const DWORD_PTR instance = reinterpret_cast<DWORD_PTR>(target_module);
        const PIMAGE_NT_HEADERS ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(instance + reinterpret_cast<PIMAGE_DOS_HEADER>(instance)->e_lfanew);
        PIMAGE_IMPORT_DESCRIPTOR pImports = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(instance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        DWORD dwProtect[2];

        // Regular imports
        for (; pImports->Name != 0; pImports++)
        {
            if (_stricmp(reinterpret_cast<const char*>(instance + pImports->Name), dll_name.data()) == 0)
            {
                if (pImports->OriginalFirstThunk != 0)
                {
                    const PIMAGE_THUNK_DATA pThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(instance + pImports->OriginalFirstThunk);

                    for (ptrdiff_t j = 0; pThunk[j].u1.AddressOfData != 0; j++)
                    {
                        auto pAddress = reinterpret_cast<void**>(instance + pImports->FirstThunk) + j;
                        if (!pAddress) continue;
                        VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                        ([&]
                        {
                            auto name = std::string_view(std::get<0>(inputs));
                            auto num = std::string("-1");
                            if (name.contains("@"))
                            {
                                num = name.substr(name.find_last_of("@") + 1);
                                name = name.substr(0, name.find_last_of("@"));
                            }

                            if (pThunk[j].u1.Ordinal & IMAGE_ORDINAL_FLAG)
                            {
                                try
                                {
                                    if (IMAGE_ORDINAL(pThunk[j].u1.Ordinal) == std::stoi(num.data()))
                                    {
                                        originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                        originalPtrs[std::get<0>(inputs)].wait();
                                        *pAddress = std::get<1>(inputs);
                                    }
                                } catch (...) {}
                            }
                            else if ((*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), name.data())) ||
                            (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(instance + pThunk[j].u1.AddressOfData)->Name, name.data()) == 0))
                            {
                                originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                originalPtrs[std::get<0>(inputs)].wait();
                                *pAddress = std::get<1>(inputs);
                            }
                        } (), ...);
                        VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                    }
                }
                else
                {
                    auto pFunctions = reinterpret_cast<void**>(instance + pImports->FirstThunk);

                    for (ptrdiff_t j = 0; pFunctions[j] != nullptr; j++)
                    {
                        auto pAddress = &pFunctions[j];
                        VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                        ([&]
                        {
                            if (*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), std::get<0>(inputs)))
                            {
                                originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                originalPtrs[std::get<0>(inputs)].wait();
                                *pAddress = std::get<1>(inputs);
                            }
                        } (), ...);
                        VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                    }
                }
            }
        }

        // Delay imports
        PIMAGE_DELAYLOAD_DESCRIPTOR pDelayed = reinterpret_cast<PIMAGE_DELAYLOAD_DESCRIPTOR>(instance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress);
        if (pDelayed && ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress != 0 && ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size != 0)
        {
            for (; pDelayed->DllNameRVA != 0; pDelayed++)
            {
                if (_stricmp(reinterpret_cast<const char*>(instance + pDelayed->DllNameRVA), dll_name.data()) == 0)
                {
                    if (pDelayed->ImportAddressTableRVA != 0)
                    {
                        const PIMAGE_THUNK_DATA pThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(instance + pDelayed->ImportNameTableRVA);
                        const PIMAGE_THUNK_DATA pFThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(instance + pDelayed->ImportAddressTableRVA);

                        for (ptrdiff_t j = 0; pThunk[j].u1.AddressOfData != 0; j++)
                        {
                            auto pAddress = reinterpret_cast<void**>(&pFThunk[j].u1.Function);
                            if (!pAddress) continue;
                            VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                            ([&]
                            {
                                auto name = std::string_view(std::get<0>(inputs));
                                auto num = std::string("-1");
                                if (name.contains("@"))
                                {
                                    num = name.substr(name.find_last_of("@") + 1);
                                    name = name.substr(0, name.find_last_of("@"));
                                }

                                if (pThunk[j].u1.Ordinal & IMAGE_ORDINAL_FLAG)
                                {
                                    try
                                    {
                                        if (IMAGE_ORDINAL(pThunk[j].u1.Ordinal) == std::stoi(num.data()))
                                        {
                                            originalPtrs[std::get<0>(inputs)] = std::async(std::launch::async,
                                            [](void** pAddress, void* value, PVOID instance) -> void*
                                            {
                                                DWORD dwProtect[2];
                                                VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                                                MEMORY_BASIC_INFORMATION mbi;
                                                mbi.AllocationBase = instance;
                                                do
                                                {
                                                    VirtualQuery(*pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
                                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                                } while (mbi.AllocationBase == instance);
                                                auto r = *pAddress;
                                                *pAddress = value;
                                                VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                                                return r;
                                            }, pAddress, std::get<1>(inputs), (PVOID)instance);
                                        }
                                    } catch (...) {}
                                }
                                else if (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(instance + pThunk[j].u1.AddressOfData)->Name, name.data()) == 0)
                                {
                                    originalPtrs[std::get<0>(inputs)] = std::async(std::launch::async,
                                    [](void** pAddress, void* value, PVOID instance) -> void*
                                    {
                                        DWORD dwProtect[2];
                                        VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                                        MEMORY_BASIC_INFORMATION mbi;
                                        mbi.AllocationBase = instance;
                                        do
                                        {
                                            VirtualQuery(*pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
                                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                        } while (mbi.AllocationBase == instance);
                                        auto r = *pAddress;
                                        *pAddress = value;
                                        VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                                        return r;
                                    }, pAddress, std::get<1>(inputs), (PVOID)instance);
                                }
                            } (), ...);
                            VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                        }
                    }
                }
            }
        }

        // Fallback section scan (e.g. re5dx9.exe steam)
        if (originalPtrs.empty())
        {
            static auto getSection = [](const PIMAGE_NT_HEADERS nt_headers, unsigned section) -> PIMAGE_SECTION_HEADER
            {
                return reinterpret_cast<PIMAGE_SECTION_HEADER>(
                    (UCHAR*)nt_headers->OptionalHeader.DataDirectory +
                    nt_headers->OptionalHeader.NumberOfRvaAndSizes * sizeof(IMAGE_DATA_DIRECTORY) +
                    section * sizeof(IMAGE_SECTION_HEADER));
            };

            for (auto i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
            {
                auto sec = getSection(ntHeader, i);
                auto pFunctions = reinterpret_cast<void**>(instance + std::max(sec->PointerToRawData, sec->VirtualAddress));

                for (ptrdiff_t j = 0; j < 300; j++)
                {
                    auto pAddress = &pFunctions[j];
                    VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                    ([&]
                    {
                        auto name = std::string_view(std::get<0>(inputs));
                        auto num = std::string("-1");
                        if (name.contains("@"))
                        {
                            num = name.substr(name.find_last_of("@") + 1);
                            name = name.substr(0, name.find_last_of("@"));
                        }

                        if (*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), name.data()))
                        {
                            originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                            originalPtrs[std::get<0>(inputs)].wait();
                            *pAddress = std::get<1>(inputs);
                        }
                    } (), ...);
                    VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                }

                if (!originalPtrs.empty())
                    return originalPtrs;
            }
        }

        return originalPtrs;
    }
};

template <typename T, typename PtrSize = uintptr_t>
std::optional<T> PtrWalkthrough(auto addr, std::convertible_to<ptrdiff_t> auto&& ...offsets)
{
    auto list = std::vector<ptrdiff_t>{ offsets... };
    auto last = list.back(); list.pop_back();
    auto a = injector::ReadMemory<PtrSize>(addr, true);
    if (a)
    {
        for (auto v : list)
        {
            auto ptr = injector::ReadMemory<PtrSize>(a + v, true);
            if (ptr)
                a = ptr;
            else
            {
                a = 0;
                break;
            }
        }

        if (a)
            return injector::ReadMemory<T>(a + last, true);
    }
    return std::nullopt;
};

class WFP
{
public:
    template<typename... Args>
    class Event : public std::function<void(Args...)>
    {
    public:
        using std::function<void(Args...)>::function;

    private:
        std::vector<std::function<void(Args...)>> handlers;

    public:
        void operator+=(std::function<void(Args...)>&& handler)
        {
            handlers.push_back(handler);
        }

        void executeAll(Args... args) const
        {
            if (!handlers.empty())
            {
                for (auto& handler : handlers)
                {
                    handler(args...);
                }
            }
        }

        std::reference_wrapper<std::vector<std::future<void>>> executeAllAsync(Args... args) const
        {
            static std::vector<std::future<void>> pendingFutures;
            if (!handlers.empty())
            {
                for (auto& handler : handlers)
                {
                    pendingFutures.push_back(std::async(std::launch::async, handler, args...));
                }
            }
            return std::ref(pendingFutures);
        }
    };

public:
    __declspec(noinline) static Event<>& onInitEvent();
    __declspec(noinline) static Event<>& onInitEventAsync();
    __declspec(noinline) static Event<>& onShutdownEvent();
    __declspec(noinline) static Event<>& onGameInitEvent();
    __declspec(noinline) static Event<>& onGameProcessEvent();
    __declspec(noinline) static Event<>& onMenuDrawingEvent();
    __declspec(noinline) static Event<>& onMenuEnterEvent();
    __declspec(noinline) static Event<>& onMenuExitEvent();
    __declspec(noinline) static Event<bool>& onActivateApp();
    __declspec(noinline) static Event<>& onBeforeReset();
    __declspec(noinline) static Event<>& onEndScene();
    __declspec(noinline) static Event<>& onReadGameConfig();
};

namespace injector
{
    inline bool UnprotectMemory(memory_pointer_tr addr, size_t size)
    {
        DWORD out_oldprotect = 0;
        return VirtualProtect(addr.get(), size, PAGE_EXECUTE_READWRITE, &out_oldprotect) != 0;
    }

    #ifdef _WIN64
    inline injector::memory_pointer_raw MakeCALLTrampoline(injector::memory_pointer_tr at, injector::memory_pointer_raw dest, bool vp = true)
    {
        auto trampoline = Trampoline::MakeTrampoline((void*)at.as_int());
        return MakeCALL(at, trampoline->Jump(dest));
    }
    #endif
};

template<typename T = int16_t>
class RawInputHandler
{
public:
    static inline T RawMouseCursorX = 0;
    static inline T RawMouseCursorY = 0;
    static inline T RawMouseDeltaX = 0;
    static inline T RawMouseDeltaY = 0;

    static void RegisterRawInput(HWND hWnd, float sensitivity = 1.0f, bool bUseRawData = false)
    {
        Sensitivity = sensitivity;
        UseRawData = bUseRawData;
        SystemParametersInfo(SPI_GETMOUSE, 0, MouseAcceleration, 0);
        SystemParametersInfo(SPI_GETMOUSESPEED, 0, &MouseSpeed, 0);
        DefaultWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)RawInputWndProc);
        rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
        rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
        rid[0].dwFlags = RIDEV_INPUTSINK;
        rid[0].hwndTarget = hWnd;
        RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE));
    }

    static void OnResChange()
    {
        RECT windowRect;
        GetWindowRect(rid[0].hwndTarget, &windowRect);
        RawMouseCursorX = static_cast<T>((windowRect.right - windowRect.left) / 2 + windowRect.left);
        RawMouseCursorY = static_cast<T>((windowRect.bottom - windowRect.top) / 2 + windowRect.top);
    }

private:
    static inline WNDPROC DefaultWndProc;
    static inline RAWINPUTDEVICE rid[1];
    static inline int MouseAcceleration[3] = { 0 };  // [0]=threshold1, [1]=threshold2, [2]=level (0=off, 1 or 2=on with varying strength)
    static inline int MouseSpeed = 10; // Default to 10 if query fails
    static inline float Sensitivity = 1.0f;
    static inline bool UseRawData = false;

    static LRESULT CALLBACK RawInputWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        static float SubpixelX = 0.0f;          // Carry-over fractions for X
        static float SubpixelY = 0.0f;          // Carry-over fractions for Y

        if (uMsg == WM_INPUT)
        {
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);

            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
            std::vector<BYTE> buffer(dwSize);
            RAWINPUT* raw = (RAWINPUT*)buffer.data();
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)
            {
                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    float dx = static_cast<float>(raw->data.mouse.lLastX);
                    float dy = static_cast<float>(raw->data.mouse.lLastY);

                    if (UseRawData)
                    {
                        // Raw input mode: only apply sensitivity
                        dx *= Sensitivity;
                        dy *= Sensitivity;
                    }
                    else
                    {
                        // Standard mode: apply acceleration, speed, and sensitivity
                        // Apply acceleration (independent for each axis, using original abs values)
                        float abs_dx = std::fabsf(dx);
                        if (MouseAcceleration[2] > 0 && MouseAcceleration[0] < abs_dx)
                            dx *= 2.0f;
                        if (MouseAcceleration[2] == 2 && MouseAcceleration[1] < abs_dx)
                            dx *= 2.0f;

                        float abs_dy = std::fabsf(dy);
                        if (MouseAcceleration[2] > 0 && MouseAcceleration[0] < abs_dy)
                            dy *= 2.0f;
                        if (MouseAcceleration[2] == 2 && MouseAcceleration[1] < abs_dy)
                            dy *= 2.0f;

                        // Apply mouse speed scaling
                        dx *= (static_cast<float>(MouseSpeed) / 10.0f);
                        dy *= (static_cast<float>(MouseSpeed) / 10.0f);

                        // Apply custom sensitivity factor
                        dx *= Sensitivity;
                        dy *= Sensitivity;
                    }

                    // Add subpixel carry-over, round to int for accumulation, save new fractions
                    dx += SubpixelX;
                    dy += SubpixelY;
                    T int_dx = static_cast<T>(std::roundf(dx));
                    T int_dy = static_cast<T>(std::roundf(dy));
                    SubpixelX = dx - static_cast<float>(int_dx);
                    SubpixelY = dy - static_cast<float>(int_dy);

                    // Accumulate and clamp
                    T oldX = RawMouseCursorX;
                    T oldY = RawMouseCursorY;

                    RawMouseCursorX += int_dx;
                    RawMouseCursorY += int_dy;

                    RawMouseDeltaX += (RawMouseCursorX - oldX);
                    RawMouseDeltaY += (RawMouseCursorY - oldY);

                    RawMouseCursorX = std::max(T(0), std::min(RawMouseCursorX, static_cast<T>(clientRect.right)));
                    RawMouseCursorY = std::max(T(0), std::min(RawMouseCursorY, static_cast<T>(clientRect.bottom)));
                }
            }
            PostMessage(hWnd, WM_RAWINPUTMOUSE, 0, 0);
            return 0;  // Consume the message to avoid game interference.
        }
        return CallWindowProc(DefaultWndProc, hWnd, uMsg, wParam, lParam);
    }

    static void SetSensitivity(float sensitivity)
    {
        Sensitivity = sensitivity;
    }
};

template<typename T = int16_t>
class RawCursorHandler
{
public:
    static inline T MouseCursorX = 0;
    static inline T MouseCursorY = 0;
    static inline T MouseDeltaX = 0;
    static inline T MouseDeltaY = 0;

    static void Initialize(HWND hWnd, float sensitivity = 1.0f)
    {
        Sensitivity = sensitivity;
        TargetWindow = hWnd;
        OnResChange();
    }

    static void OnResChange()
    {
        RECT windowRect;
        GetWindowRect(TargetWindow, &windowRect);
        MouseCursorX = static_cast<T>((windowRect.right - windowRect.left) / 2 + windowRect.left);
        MouseCursorY = static_cast<T>((windowRect.bottom - windowRect.top) / 2 + windowRect.top);
        SetCursorPos(static_cast<int>(MouseCursorX), static_cast<int>(MouseCursorY));
    }

    static HWND UpdateMouseInput(bool bIsAbsoluteValue = false)
    {
        if (TargetWindow != GetForegroundWindow())
        {
            if (bIsAbsoluteValue)
                ClipCursor(NULL);

            return TargetWindow;
        }

        RECT windowRect;
        GetWindowRect(TargetWindow, &windowRect);

        if (bIsAbsoluteValue)
        {
            POINT cursorPos;
            ClipCursor(&windowRect);
            GetCursorPos(&cursorPos);
            MouseCursorX = static_cast<T>(cursorPos.x - windowRect.left);
            MouseCursorY = static_cast<T>(cursorPos.y - windowRect.top);
        }
        else
        {
            POINT cursorPos;
            GetCursorPos(&cursorPos);

            T centerX = static_cast<T>((windowRect.right - windowRect.left) / 2 + windowRect.left);
            T centerY = static_cast<T>((windowRect.bottom - windowRect.top) / 2 + windowRect.top);

            T deltaX = static_cast<T>(cursorPos.x - centerX);
            T deltaY = static_cast<T>(cursorPos.y - centerY);

            float scaledDeltaX = static_cast<float>(deltaX) * Sensitivity;
            float scaledDeltaY = static_cast<float>(deltaY) * Sensitivity;

            MouseDeltaX += static_cast<T>(scaledDeltaX);
            MouseDeltaY -= static_cast<T>(scaledDeltaY);

            SetCursorPos(static_cast<int>(centerX), static_cast<int>(centerY));
        }

        return TargetWindow;
    }

    static void SetSensitivity(float sensitivity)
    {
        Sensitivity = sensitivity;
    }

private:
    static inline HWND TargetWindow = nullptr;
    static inline float Sensitivity = 1.0f;
};

template<typename T>
class GameRef
{
private:
    std::optional<T*> ptr{ std::nullopt };

public:
    GameRef() = default;

    void SetAddress(T* address)
    {
        if (address == nullptr)
            throw std::invalid_argument("GameRef::SetAddress called with null pointer");

        ptr = address;
    }

    T& get()
    {
        if (!ptr.has_value())
        {
            assert(false && "GameRef accessed before SetAddress()!");
            throw std::runtime_error("GameRef accessed before SetAddress() was called");
        }
        return **ptr;
    }

    const T& get() const
    {
        if (!ptr.has_value())
        {
            assert(false && "GameRef accessed before SetAddress()!");
            throw std::runtime_error("GameRef accessed before SetAddress() was called");
        }
        return **ptr;
    }

    bool is_initialized() const noexcept { return ptr.has_value(); }
    T* get_ptr() noexcept { return ptr.value_or(nullptr); }
    const T* get_ptr() const noexcept { return ptr.value_or(nullptr); }

    operator T& () { return get(); }
    operator const T& () const { return get(); }

    T& operator=(const T& value)
    {
        *reinterpret_cast<volatile T*>(*ptr) = value;
        return **ptr;
    }
    T& operator=(T&& value) { return get() = std::move(value); }

    template<typename U> T& operator+=(const U& v) { return get() += v; }
    template<typename U> T& operator-=(const U& v) { return get() -= v; }
    template<typename U> T& operator*=(const U& v) { return get() *= v; }
    template<typename U> T& operator/=(const U& v) { return get() /= v; }
    template<typename U> T& operator%=(const U& v) { return get() %= v; }

    template<typename U> T& operator&=(const U& v) { return get() &= v; }
    template<typename U> T& operator|=(const U& v) { return get() |= v; }
    template<typename U> T& operator^=(const U& v) { return get() ^= v; }
    template<typename U> T& operator<<=(const U& v) { return get() <<= v; }
    template<typename U> T& operator>>=(const U& v) { return get() >>= v; }

    T& operator++() { return ++get(); }
    T  operator++(int) { return get()++; }

    T& operator--() { return --get(); }
    T  operator--(int) { return get()--; }

    T operator+() const { return +get(); }
    T operator-() const { return -get(); }
    bool operator!() const { return !get(); }
    T operator~() const { return ~get(); }

    template<typename U> auto operator+(const U& v) const { return get() + v; }
    template<typename U> auto operator-(const U& v) const { return get() - v; }
    template<typename U> auto operator*(const U& v) const { return get() * v; }
    template<typename U> auto operator/(const U& v) const { return get() / v; }
    template<typename U> auto operator%(const U& v) const { return get() % v; }

    template<typename U> auto operator&(const U& v) const { return get() & v; }
    template<typename U> auto operator|(const U& v) const { return get() | v; }
    template<typename U> auto operator^(const U& v) const { return get() ^ v; }
    template<typename U> auto operator<<(const U& v) const { return get() << v; }
    template<typename U> auto operator>>(const U& v) const { return get() >> v; }

    template<typename U> bool operator==(const U& v) const { return get() == v; }
    template<typename U> bool operator!=(const U& v) const { return get() != v; }
    template<typename U> bool operator<(const U& v)  const { return get() < v; }
    template<typename U> bool operator>(const U& v)  const { return get() > v; }
    template<typename U> bool operator<=(const U& v) const { return get() <= v; }
    template<typename U> bool operator>=(const U& v) const { return get() >= v; }

    T& operator*() { return get(); }
    const T& operator*() const { return get(); }

    auto operator->()
    {
        if constexpr (std::is_pointer_v<T>)
            return get();
        else
            return &get();
    }

    auto operator->() const
    {
        if constexpr (std::is_pointer_v<T>)
            return get();
        else
            return &get();
    }

    template<typename U>
    auto operator[](const U& index) { return get()[index]; }

    template<typename U>
    auto operator[](const U& index) const { return get()[index]; }

    explicit operator bool() const { return static_cast<bool>(get()); }
};

