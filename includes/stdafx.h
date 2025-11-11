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
void GetResolutionsList(std::vector<std::string>& list);
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
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetResolutionsList, &hm);
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
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetResolutionsList, &hm);
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
    static /*inline c a e*/ std::string filter;
    static /*inline r t x*/ std::string section;
    static /*inline a   i*/ CIniReader RegistryReader;
    static /*inline s   t*/ std::map<std::string, std::string> DefaultStrings;
    static /*inline h    */ std::set<std::string, std::less<>> PathStrings;
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
        PathStrings = { args... };
        for (size_t i = 0; i < PathStrings.size(); i++)
        {
            AddDefault(*std::next(PathStrings.begin(), i), GetExeModulePath<std::string>());
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
        {
            return ERROR_SUCCESS;
        }
        else
            return ::RegCloseKey(hKey);
    }
    static LSTATUS WINAPI RegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
    {
        if (hKey == NULL)
        {
            return ERROR_SUCCESS;
        }
        else
            return ::RegCreateKeyA(hKey, lpSubKey, phkResult);
    }
    static LSTATUS WINAPI RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
    {
        if (strstr(lpSubKey, filter.c_str()) != NULL)
        {
            *phkResult = NULL;
            section = lpSubKey;
            section = section.substr(section.find_last_of('\\') + 1);
            return ERROR_SUCCESS;
        }
        else
            return ::RegOpenKeyA(hKey, lpSubKey, phkResult);
    }
    static LSTATUS WINAPI RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
    {
        if (strstr(lpSubKey, filter.c_str()) != NULL)
        {
            *phkResult = NULL;
            section = lpSubKey;
            section = section.substr(section.find_last_of('\\') + 1);
            return ERROR_SUCCESS;
        }
        else
            return ::RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
    }

    static LSTATUS WINAPI RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
    {
        std::string ValueName = "@";
        if (lpValueName)
            ValueName = lpValueName;

        if (hKey == NULL)
        {
            if (OverrideTypeREG_NONE)
                lpType = &OverrideTypeREG_NONE;

            if (lpType)
            {
                switch (*lpType)
                {
                case REG_BINARY:
                {
                    std::string str = RegistryReader.ReadString(section, ValueName, DefaultStrings[ValueName]);
                    if (!str.empty())
                    {
                        std::istringstream input(str);
                        std::string number;
                        size_t i = 0;
                        while (std::getline(input, number, ','))
                        {
                            lpData[i] = (BYTE)std::stoul(number, nullptr, 16);
                            ++i;
                        }
                        *lpcbData = i;
                    }
                    else
                        RegistryReader.WriteString(section, ValueName, DefaultStrings[ValueName].empty() ? "" : DefaultStrings[ValueName]);
                    break;
                }
                case REG_DWORD:
                {
                    DWORD def = UINT_MAX;
                    if (!DefaultStrings[ValueName].empty())
                    {
                        try
                        {
                            def = (DWORD)std::stoul(DefaultStrings[ValueName]);
                        }
                        catch (const std::invalid_argument&)
                        {
                            def = UINT_MAX;
                        }
                    }
                    try
                    {
                        *(DWORD*)lpData = RegistryReader.ReadInteger(section, ValueName, def);
                    }
                    catch (const std::invalid_argument&)
                    {
                        *(DWORD*)lpData = UINT_MAX;
                    }
                    if (*(DWORD*)lpData == UINT_MAX)
                    {
                        RegistryReader.WriteString(section, ValueName, DefaultStrings[ValueName].empty() ? "INSERTDWORDHERE" : DefaultStrings[ValueName]);
                        *(DWORD*)lpData = 0;
                    }
                    *lpcbData = sizeof(DWORD);
                    break;
                }
                case REG_MULTI_SZ: //not implemented
                    break;
                case REG_NONE:
                {
                    if (lpData != NULL)
                    {
                        *(bool*)lpData = RegistryReader.ReadBoolean("MAIN", ValueName, false);
                        *lpcbData = sizeof(bool);
                    }
                    else
                    {
                        auto s = DefaultStrings.find(ValueName);
                        if (s != DefaultStrings.end())
                        {
                            *lpcbData = s->second.size();
                            *lpType = 1;
                            return ERROR_SUCCESS;
                        }
                        else
                            return ERROR_FILE_NOT_FOUND;
                    }
                    break;
                }
                case REG_SZ:
                case REG_EXPAND_SZ:
                {
                    if (lpData != NULL)
                    {
                        std::string_view str((char*)lpData, *lpcbData);
                        auto ret = RegistryReader.ReadString(section, ValueName, DefaultStrings[ValueName]);
                        *lpcbData = std::min(ret.length(), str.length());
                        ret.copy((char*)str.data(), *lpcbData, 0);
                        lpData[*lpcbData] = '\0';
                        if ((ret.empty() || DefaultStrings[ValueName] == ret) && PathStrings.find(ValueName) == PathStrings.end())
                            RegistryReader.WriteString(section, ValueName, DefaultStrings[ValueName].empty() ? "INSERTSTRINGDATAHERE" : DefaultStrings[ValueName]);
                    }
                    else
                    {
                        auto ret = RegistryReader.ReadString(section, ValueName, DefaultStrings[ValueName]);
                        *lpcbData = ret.length();
                    }
                    break;
                }
                default:
                {
                    auto ret = RegistryReader.ReadString(section, ValueName, DefaultStrings[ValueName]);
                    *lpcbData = ret.length();
                    *lpType = REG_SZ;
                    break;
                }
                }
            }
            return ERROR_SUCCESS;
        }
        else
            return ::RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    }
    static LSTATUS WINAPI RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData)
    {
        if (hKey == NULL)
        {
            if (OverrideTypeREG_NONE)
                dwType = OverrideTypeREG_NONE;

            switch (dwType)
            {
            case REG_BINARY:
            {
                std::string str;
                for (size_t i = 0; i < cbData; i++)
                {
                    str += std::format("{:02x}", lpData[i]);
                    if (i != cbData - 1)
                        str += ',';
                }
                RegistryReader.WriteString(section, lpValueName, str);
                break;
            }
            case REG_DWORD:
                RegistryReader.WriteInteger(section, lpValueName, *(DWORD*)lpData);
                break;
            case REG_MULTI_SZ:
            {
                std::string str;
                const char* temp = (const char*)lpData;
                size_t index = 0;
                size_t len = strlen(&temp[0]) + 1;
                while (len > 1)
                {
                    str += temp[index];
                    str += ',';
                    index += len + 1;
                    len = strlen(&temp[index]) + 1;
                }
                str.resize(str.size() - 1);
                RegistryReader.WriteString(section, lpValueName, str);
                break;
            }
            case REG_NONE:
                RegistryReader.WriteBoolean("MAIN", lpValueName, true);
                break;
            case REG_SZ:
            case REG_EXPAND_SZ:
                RegistryReader.WriteString(section, lpValueName, std::string((char*)lpData, cbData));
                break;
            default:
                break;
            }
            return ERROR_SUCCESS;
        }
        else
            return ::RegSetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
    }
    static LSTATUS WINAPI RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
    {
        if (hKey == NULL)
        {
            return ERROR_SUCCESS; //not implemented
        }
        else
            return ::RegDeleteKeyA(hKey, lpSubKey);
    }
    static LSTATUS WINAPI RegEnumKeyA(HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cchName)
    {
        if (hKey == NULL)
        {
            return ERROR_SUCCESS; //not implemented
        }
        else
            return ::RegEnumKeyA(hKey, dwIndex, lpName, cchName);
    }
    static LSTATUS WINAPI RegQueryValueA(HKEY hKey, LPCSTR lpSubKey, LPSTR lpData, PLONG lpcbData)
    {
        if (hKey == NULL)
        {
            return ERROR_SUCCESS; //not implemented
        }
        else
            return ::RegQueryValueA(hKey, lpSubKey, lpData, lpcbData);
    }
    static LSTATUS WINAPI RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, CONST LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
    {
        if (strstr(lpSubKey, filter.c_str()) != NULL)
        {
            *phkResult = NULL;
            section = lpSubKey;
            section = section.substr(section.find_last_of('\\') + 1);
            return ERROR_SUCCESS;
        }
        else
            return ::RegCreateKeyExA(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
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

        int WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)newWidth / 2.0f));
        int WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)newHeight / 2.0f));

        SetProcessDPIAware();

        return std::make_tuple(WindowPosX, WindowPosY, newWidth, newHeight);
    }

    static void afterCreateWindow()
    {
        LONG lStyle = GetWindowLong(GameHWND, GWL_STYLE);

        if (bBorderlessWindowed)
            lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        else
        {
            lStyle |= (WS_MINIMIZEBOX | WS_SYSMENU);
            if (bEnableWindowResize)
                lStyle |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
        }

        SetWindowLong(GameHWND, GWL_STYLE, lStyle);
        SetWindowPos(GameHWND, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
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
        auto[WindowPosX, WindowPosY, newWidth, newHeight] = beforeCreateWindow(nWidth, nHeight);
        GameHWND = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, WindowPosX, WindowPosY, newWidth, newHeight, hWndParent, hMenu, hInstance, lpParam);
        afterCreateWindow();
        return GameHWND;
    }

    static HWND WINAPI CreateWindowExW_Hook(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
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
            dwNewLong |= (WS_MINIMIZEBOX | WS_SYSMENU);
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
            dwNewLong |= (WS_MINIMIZEBOX | WS_SYSMENU);
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
                        VirtualProtect(pAddress, sizeof(pAddress), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                        ([&]
                        {
                            auto name = std::string_view(std::get<0>(inputs));
                            auto num = std::string("-1");
                            if (name.contains("@")) {
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
                                }
                                catch (...) {}
                            }
                            else if ((*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), name.data())) ||
                            (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(instance + pThunk[j].u1.AddressOfData)->Name, name.data()) == 0))
                            {
                                originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                originalPtrs[std::get<0>(inputs)].wait();
                                *pAddress = std::get<1>(inputs);
                            }
                        } (), ...);
                        VirtualProtect(pAddress, sizeof(pAddress), dwProtect[0], &dwProtect[1]);
                    }
                }
                else
                {
                    auto pFunctions = reinterpret_cast<void**>(instance + pImports->FirstThunk);

                    for (ptrdiff_t j = 0; pFunctions[j] != nullptr; j++)
                    {
                        auto pAddress = reinterpret_cast<void**>(pFunctions[j]);
                        VirtualProtect(pAddress, sizeof(pAddress), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                        ([&]
                        {
                            if (*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), std::get<0>(inputs)))
                            {
                                originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                originalPtrs[std::get<0>(inputs)].wait();
                                *pAddress = std::get<1>(inputs);
                            }
                        } (), ...);
                        VirtualProtect(pAddress, sizeof(pAddress), dwProtect[0], &dwProtect[1]);
                    }
                }
            }
        }

        if (originalPtrs.empty())
        {
            PIMAGE_DELAYLOAD_DESCRIPTOR pDelayed = reinterpret_cast<PIMAGE_DELAYLOAD_DESCRIPTOR>(instance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress);
            if (pDelayed)
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
                                auto pAddress = reinterpret_cast<void**>(pFThunk[j].u1.Function);
                                if (!pAddress) continue;
                                if (pThunk[j].u1.Ordinal & IMAGE_ORDINAL_FLAG)
                                    pAddress = *reinterpret_cast<void***>(pFThunk[j].u1.Function + 1); // mov     eax, offset *

                                VirtualProtect(pAddress, sizeof(pAddress), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                                ([&]
                                {
                                    auto name = std::string_view(std::get<0>(inputs));
                                    auto num = std::string("-1");
                                    if (name.contains("@")) {
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
                                                    VirtualProtect(pAddress, sizeof(pAddress), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                                                    MEMORY_BASIC_INFORMATION mbi;
                                                    mbi.AllocationBase = instance;
                                                    do
                                                    {
                                                        VirtualQuery(*pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
                                                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                                    } while (mbi.AllocationBase == instance);
                                                    auto r = *pAddress;
                                                    *pAddress = value;
                                                    VirtualProtect(pAddress, sizeof(pAddress), dwProtect[0], &dwProtect[1]);
                                                    return r;
                                                }, pAddress, std::get<1>(inputs), (PVOID)instance);
                                            }
                                        }
                                        catch (...) {}
                                    }
                                    else if ((*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), name.data())) ||
                                    (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(instance + pThunk[j].u1.AddressOfData)->Name, name.data()) == 0))
                                    {
                                        originalPtrs[std::get<0>(inputs)] = std::async(std::launch::async,
                                        [](void** pAddress, void* value, PVOID instance) -> void*
                                        {
                                            DWORD dwProtect[2];
                                            VirtualProtect(pAddress, sizeof(pAddress), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                                            MEMORY_BASIC_INFORMATION mbi;
                                            mbi.AllocationBase = instance;
                                            do
                                            {
                                                VirtualQuery(*pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
                                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                            } while (mbi.AllocationBase == instance);
                                            auto r = *pAddress;
                                            *pAddress = value;
                                            VirtualProtect(pAddress, sizeof(pAddress), dwProtect[0], &dwProtect[1]);
                                            return r;
                                        }, pAddress, std::get<1>(inputs), (PVOID)instance);
                                    }
                                } (), ...);
                                VirtualProtect(pAddress, sizeof(pAddress), dwProtect[0], &dwProtect[1]);
                            }
                        }
                    }
                }
            }
        }

        if (originalPtrs.empty()) // e.g. re5dx9.exe steam
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
                    auto pAddress = reinterpret_cast<void**>(&pFunctions[j]);
                    VirtualProtect(pAddress, sizeof(pAddress), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                    ([&]
                    {
                        auto name = std::string_view(std::get<0>(inputs));
                        auto num = std::string("-1");
                        if (name.contains("@")) {
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
                    VirtualProtect(pAddress, sizeof(pAddress), dwProtect[0], &dwProtect[1]);
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
    static Event<>& onInitEvent() {
        static Event<> InitEvent;
        return InitEvent;
    }
    static Event<>& onInitEventAsync() {
        static Event<> InitEventAsync;
        return InitEventAsync;
    }
    static Event<>& onAfterUALRestoredIATEvent() {
        static Event<> AfterUALRestoredIATEvent;
        return AfterUALRestoredIATEvent;
    }
    static Event<>& onShutdownEvent() {
        static Event<> ShutdownEvent;
        return ShutdownEvent;
    }
    static Event<>& onGameInitEvent() {
        static Event<> GameInitEvent;
        return GameInitEvent;
    }
    static Event<>& onGameProcessEvent() {
        static Event<> GameProcessEvent;
        return GameProcessEvent;
    }
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

template<typename T = int16_t, class W = int32_t, class H = W>
class RawInputHandler
{
public:
    static inline T RawMouseCursorX = 0;
    static inline T RawMouseCursorY = 0;
    static inline T RawMouseDeltaX = 0;
    static inline T RawMouseDeltaY = 0;

    static void RegisterRawInput(HWND hWnd, W& width, H& height, float sensitivity = 1.0f)
    {
        GameWidth = std::ref(width);
        GameHeight = std::ref(height);
        Sensitivity = sensitivity;

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
        if (GameWidth.has_value() && GameHeight.has_value())
        {
            RawMouseCursorX = static_cast<T>(GameWidth.value().get() / 2);
            RawMouseCursorY = static_cast<T>(GameHeight.value().get() / 2);
        }
        else
        {
            RECT clientRect;
            GetClientRect(rid[0].hwndTarget, &clientRect);
            RawMouseCursorX = static_cast<T>(clientRect.right / 2);
            RawMouseCursorY = static_cast<T>(clientRect.bottom / 2);
        }
    }

private:
    static inline WNDPROC DefaultWndProc;
    static inline RAWINPUTDEVICE rid[1];
    static inline int MouseAcceleration[3] = { 0 };  // [0]=threshold1, [1]=threshold2, [2]=level (0=off, 1 or 2=on with varying strength)
    static inline int MouseSpeed = 10; // Default to 10 if query fails
    static inline std::optional<std::reference_wrapper<W>> GameWidth;
    static inline std::optional<std::reference_wrapper<H>> GameHeight;
    static inline float Sensitivity = 1.0f;

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

                    W maxWidth = clientRect.right;
                    H maxHeight = clientRect.bottom;

                    if (GameWidth.has_value() && GameHeight.has_value())
                    {
                        maxWidth = GameWidth.value().get();
                        maxHeight = GameHeight.value().get();
                    }

                    RawMouseCursorX = std::max(T(0), std::min(RawMouseCursorX, static_cast<T>(maxWidth)));
                    RawMouseCursorY = std::max(T(0), std::min(RawMouseCursorY, static_cast<T>(maxHeight)));
                }
            }
            PostMessage(hWnd, WM_RAWINPUTMOUSE, 0, 0);
            return 0;  // Consume the message to avoid game interference.
        }
        return CallWindowProc(DefaultWndProc, hWnd, uMsg, wParam, lParam);
    }
};