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
#include "windowed.hpp"
#include "rawinput.hpp"
#include "iathook.hpp"
#include "gameref.hpp"
#include <filesystem>
#include <stacktrace>
#include <shellapi.h>
#include <ranges>
#include <format>
#pragma warning(pop)

#ifndef CEXP
#define CEXP extern "C" __declspec(dllexport)
#endif

#define force_return_address(addr) (*(uintptr_t*)(regs.esp - 4) = (addr))
#define return_to(addr) do { force_return_address(addr); return; } while (0)

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
[[nodiscard]] uint32_t GetDesktopRefreshRate() noexcept;
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

namespace module_path_detail
{
    // Text from a Win32 call that fills a buffer and returns the length it wrote. The buffer grows while the call says it
    // was too small: a returned length that reaches the capacity (a cut path) or exceeds it (the size it needs).
    template<typename Char, typename Call>
    std::basic_string<Char> ReadText(Call&& call)
    {
        constexpr size_t limit = 32768; // the longest path Windows has
        std::basic_string<Char> text(MAX_PATH, Char{});
        while (text.size() <= limit)
        {
            const auto capacity = static_cast<DWORD>(text.size());
            const DWORD length = call(text.data(), capacity);
            if (length == 0)
                return {};
            if (length < capacity)
            {
                text.resize(length);
                return text;
            }
            text.resize((std::max)(text.size() * 2, size_t(length) + 1));
        }
        return {};
    }

    template<typename Char>
    constexpr Char Separators[] = { Char('/'), Char('\\'), Char(0) };

    // A file's path with its name in the case it has on disk. GetModuleFileName returns the name in the case the loader
    // asked for (some lowercase it), and files named after a plugin should match the plugin's name.
    template<typename Char>
    std::basic_string<Char> OnDiskName(std::basic_string<Char> file)
    {
        const auto slash = file.find_last_of(Separators<Char>);
        if (slash == std::basic_string<Char>::npos)
            return file;
        if constexpr (std::same_as<Char, char>)
        {
            WIN32_FIND_DATAA found{};
            if (const HANDLE search = ::FindFirstFileExA(file.c_str(), FindExInfoBasic, &found, FindExSearchNameMatch, nullptr, 0); search != INVALID_HANDLE_VALUE)
            {
                ::FindClose(search);
                file.replace(slash + 1, std::string::npos, found.cFileName);
            }
        }
        else
        {
            WIN32_FIND_DATAW found{};
            if (const HANDLE search = ::FindFirstFileExW(file.c_str(), FindExInfoBasic, &found, FindExSearchNameMatch, nullptr, 0); search != INVALID_HANDLE_VALUE)
            {
                ::FindClose(search);
                file.replace(slash + 1, std::wstring::npos, found.cFileName);
            }
        }
        return file;
    }

    // std::string is the ANSI text Windows gives; std::wstring and std::filesystem::path are Unicode
    template<typename T>
    T ModuleFile(HMODULE module)
    {
        if constexpr (std::same_as<T, std::string>)
            return OnDiskName(ReadText<char>([module](char* buffer, DWORD size) { return ::GetModuleFileNameA(module, buffer, size); }));
        else
            return T(OnDiskName(ReadText<wchar_t>([module](wchar_t* buffer, DWORD size) { return ::GetModuleFileNameW(module, buffer, size); })));
    }

    template<typename T>
    T CurrentDirectory()
    {
        if constexpr (std::is_same_v<T, std::string>)
            return ReadText<char>([](char* buffer, DWORD size) { return ::GetCurrentDirectoryA(size, buffer); });
        else
            return T(ReadText<wchar_t>([](wchar_t* buffer, DWORD size) { return ::GetCurrentDirectoryW(size, buffer); }));
    }

    // A path's folder, and a string's folder with its trailing separator
    template<typename T>
    T FolderOf(const T& file)
    {
        if constexpr (std::same_as<T, std::filesystem::path>)
            return file.parent_path();
        else
        {
            const auto end = file.find_last_of(Separators<typename T::value_type>);
            return end == T::npos ? T() : file.substr(0, end + 1);
        }
    }

    template<typename T>
    T NameOf(const T& file)
    {
        if constexpr (std::same_as<T, std::filesystem::path>)
            return file.filename();
        else
        {
            const auto end = file.find_last_of(Separators<typename T::value_type>);
            return end == T::npos ? file : file.substr(end + 1);
        }
    }

    // The module this code is linked into: every plugin has its own copy of this function
    inline HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }
}

template<class T = std::filesystem::path>
T GetModulePath(HMODULE hModule)
{
    return module_path_detail::ModuleFile<T>(hModule);
}

template<class T = std::filesystem::path>
T GetThisModulePath()
{
    return module_path_detail::FolderOf(module_path_detail::ModuleFile<T>(module_path_detail::ThisModule()));
}

template<class T = std::filesystem::path>
T GetThisModuleName()
{
    return module_path_detail::NameOf(module_path_detail::ModuleFile<T>(module_path_detail::ThisModule()));
}

template<class T = std::filesystem::path>
T GetExeModulePath()
{
    return module_path_detail::FolderOf(module_path_detail::ModuleFile<T>(nullptr));
}

template<class T = std::filesystem::path>
T GetExeModuleName()
{
    return module_path_detail::NameOf(module_path_detail::ModuleFile<T>(nullptr));
}

template<class T = std::filesystem::path>
T GetCurrentDirectoryW()
{
    return module_path_detail::CurrentDirectory<T>();
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

    // Registry values whose key contains the search text are read from and written to the ini instead; its folder is made
    RegistryWrapper(std::string_view searchString, const std::filesystem::path& iniFile)
    {
        filter.assign(searchString);
        RegistryReader.SetIniPath(iniFile);
        std::error_code ignored;
        if (const auto folder = iniFile.parent_path(); !folder.empty())
            std::filesystem::create_directories(folder, ignored);
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

// The game's settings key under HKCU (HKLM is redirected there). Nothing is written for the game: a key that doesn't
// exist opens as a pseudo handle, a missing value reads as its registered default, and the real key is only made when
// the game itself writes to it.
class RegistryFallback
{
public:
    // The real registry functions; the trampolines when the game's calls are hooked inline
    struct Api
    {
        decltype(&::RegCloseKey) CloseKey = ::RegCloseKey;
        decltype(&::RegCreateKeyA) CreateKeyA = ::RegCreateKeyA;
        decltype(&::RegOpenKeyA) OpenKeyA = ::RegOpenKeyA;
        decltype(&::RegOpenKeyExA) OpenKeyExA = ::RegOpenKeyExA;
        decltype(&::RegCreateKeyExA) CreateKeyExA = ::RegCreateKeyExA;
        decltype(&::RegQueryValueExA) QueryValueExA = ::RegQueryValueExA;
        decltype(&::RegSetValueExA) SetValueExA = ::RegSetValueExA;
        decltype(&::RegQueryValueA) QueryValueA = ::RegQueryValueA;
        decltype(&::RegDeleteKeyA) DeleteKeyA = ::RegDeleteKeyA;
        decltype(&::RegEnumKeyA) EnumKeyA = ::RegEnumKeyA;
    };
    static inline Api Real;

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

    static LSTATUS WINAPI RegCloseKey(HKEY hKey)
    {
        std::lock_guard lock(s_Mutex);
        if (auto it = s_Pseudo.find(hKey); it != s_Pseudo.end())
        {
            if (it->second.real)
                Real.CloseKey(it->second.real);
            CloseHandle(reinterpret_cast<HANDLE>(hKey));
            s_Pseudo.erase(it);
            return ERROR_SUCCESS;
        }
        s_GameKeys.erase(hKey);
        return Real.CloseKey(hKey);
    }

    static LSTATUS WINAPI RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
    {
        std::lock_guard lock(s_Mutex);
        if (auto p = Pseudo(hKey))
        {
            if (auto real = RealKey(*p, false))
                return Track(Real.OpenKeyExA(real, lpSubKey, ulOptions, samDesired, phkResult), IsEmpty(lpSubKey), phkResult);
            return OpenMissing(p->path, lpSubKey, phkResult);
        }

        hKey = Root(hKey);
        std::string path;
        LSTATUS status = Real.OpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
        if (!IsGameKey(hKey, lpSubKey, path))
            return status;
        if (status == ERROR_FILE_NOT_FOUND)
            return OpenMissing(path, nullptr, phkResult);
        return Track(status, true, phkResult);
    }

    static LSTATUS WINAPI RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
    {
        std::lock_guard lock(s_Mutex);
        if (auto p = Pseudo(hKey))
        {
            // An empty subkey hands back the handle passed in, as the real function does
            if (IsEmpty(lpSubKey) && phkResult)
            {
                *phkResult = hKey;
                return ERROR_SUCCESS;
            }
            if (auto real = RealKey(*p, false))
                return Real.OpenKeyA(real, lpSubKey, phkResult);
            return OpenMissing(p->path, lpSubKey, phkResult);
        }

        hKey = Root(hKey);
        std::string path;
        LSTATUS status = Real.OpenKeyA(hKey, lpSubKey, phkResult);
        if (!IsGameKey(hKey, lpSubKey, path))
            return status;
        if (status == ERROR_FILE_NOT_FOUND)
            return OpenMissing(path, nullptr, phkResult);
        return Track(status, true, phkResult);
    }

    // Creating a key is the game's write: a pseudo key becomes the real one first
    static LSTATUS WINAPI RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions,
                                          REGSAM samDesired, CONST LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                          PHKEY phkResult, LPDWORD lpdwDisposition)
    {
        std::lock_guard lock(s_Mutex);
        bool gameKey = false;
        if (auto p = Pseudo(hKey))
        {
            hKey = RealKey(*p, true);
            if (!hKey)
                return ERROR_ACCESS_DENIED;
            gameKey = IsEmpty(lpSubKey);
        }
        else
        {
            hKey = Root(hKey);
            std::string path;
            gameKey = IsGameKey(hKey, lpSubKey, path);
        }
        return Track(Real.CreateKeyExA(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes,
                                       phkResult, lpdwDisposition), gameKey, phkResult);
    }

    static LSTATUS WINAPI RegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
    {
        std::lock_guard lock(s_Mutex);
        if (auto p = Pseudo(hKey))
        {
            HKEY real = RealKey(*p, true);
            if (!real)
                return ERROR_ACCESS_DENIED;
            // An empty subkey hands back the handle passed in, as the real function does
            if (IsEmpty(lpSubKey) && phkResult)
            {
                *phkResult = hKey;
                return ERROR_SUCCESS;
            }
            return Real.CreateKeyA(real, lpSubKey, phkResult);
        }

        hKey = Root(hKey);
        std::string path;
        bool gameKey = IsGameKey(hKey, lpSubKey, path);
        return Track(Real.CreateKeyA(hKey, lpSubKey, phkResult), gameKey, phkResult);
    }

    static LSTATUS WINAPI RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData)
    {
        std::lock_guard lock(s_Mutex);
        auto p = Pseudo(hKey);
        if (!p)
            return Real.SetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);

        // A write that fails (the games pass Reserved = 1 for "FirstTime") leaves no key behind
        bool created = false;
        hKey = RealKey(*p, true, &created);
        if (!hKey)
            return ERROR_ACCESS_DENIED;
        LSTATUS status = Real.SetValueExA(hKey, lpValueName, Reserved, dwType, lpData, cbData);
        if (status != ERROR_SUCCESS && created)
        {
            Real.CloseKey(p->real);
            p->real = nullptr;
            Real.DeleteKeyA(HKEY_CURRENT_USER, p->path.c_str());
        }
        return status;
    }

    static LSTATUS WINAPI RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
    {
        std::lock_guard lock(s_Mutex);
        bool gameKey = s_GameKeys.contains(hKey);
        if (auto p = Pseudo(hKey))
        {
            gameKey = true;
            hKey = RealKey(*p, false);
        }
        if (hKey)
        {
            LSTATUS status = Real.QueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
            if (!gameKey || status != ERROR_FILE_NOT_FOUND)
                return status;
        }
        return QueryDefault(lpValueName, lpType, lpData, lpcbData);
    }

    static LSTATUS WINAPI RegQueryValueA(HKEY hKey, LPCSTR lpSubKey, LPSTR lpData, PLONG lpcbData)
    {
        std::lock_guard lock(s_Mutex);
        bool gameKey = s_GameKeys.contains(hKey);
        if (auto p = Pseudo(hKey))
        {
            gameKey = true;
            hKey = RealKey(*p, false);
            if (!hKey && !IsEmpty(lpSubKey))
                return ERROR_FILE_NOT_FOUND;
        }
        else
        {
            hKey = Root(hKey);
        }
        if (hKey)
        {
            LSTATUS status = Real.QueryValueA(hKey, lpSubKey, lpData, lpcbData);
            if (!gameKey || !IsEmpty(lpSubKey) || status != ERROR_FILE_NOT_FOUND)
                return status;
        }
        if (lpData && !lpcbData)
            return ERROR_INVALID_PARAMETER;
        DWORD cbData = lpcbData ? static_cast<DWORD>(*lpcbData) : 0;
        LSTATUS status = QueryDefault(nullptr, nullptr, reinterpret_cast<LPBYTE>(lpData), lpcbData ? &cbData : nullptr, true);
        if (lpcbData)
            *lpcbData = static_cast<LONG>(cbData);
        return status;
    }

    static LSTATUS WINAPI RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey)
    {
        std::lock_guard lock(s_Mutex);
        if (auto p = Pseudo(hKey))
        {
            hKey = RealKey(*p, false);
            if (!hKey)
                return ERROR_FILE_NOT_FOUND;
            return Real.DeleteKeyA(hKey, lpSubKey);
        }
        return Real.DeleteKeyA(Root(hKey), lpSubKey);
    }

    static LSTATUS WINAPI RegEnumKeyA(HKEY hKey, DWORD dwIndex, LPSTR lpName, DWORD cchName)
    {
        std::lock_guard lock(s_Mutex);
        if (auto p = Pseudo(hKey))
        {
            hKey = RealKey(*p, false);
            if (!hKey)
                return ERROR_NO_MORE_ITEMS;
            return Real.EnumKeyA(hKey, dwIndex, lpName, cchName);
        }
        return Real.EnumKeyA(Root(hKey), dwIndex, lpName, cchName);
    }

private:
    struct PseudoKey
    {
        std::string path;
        HKEY real = nullptr;
    };

    static inline std::string s_MainPath;
    static inline std::string s_FallbackPath;
    static inline std::map<std::string, std::string> s_Defaults;
    static inline std::recursive_mutex s_Mutex;
    static inline std::map<HKEY, PseudoKey> s_Pseudo;
    static inline std::set<HKEY> s_GameKeys;

    static HKEY Root(HKEY hKey)
    {
        return hKey == HKEY_LOCAL_MACHINE ? HKEY_CURRENT_USER : hKey;
    }

    static bool IsEmpty(LPCSTR s)
    {
        return s == nullptr || *s == '\0';
    }

    static bool IsGameKey(HKEY root, LPCSTR lpSubKey, std::string& path)
    {
        if (root != HKEY_CURRENT_USER || IsEmpty(lpSubKey))
            return false;
        std::string_view sub(lpSubKey);
        while (!sub.empty() && sub.front() == '\\')
            sub.remove_prefix(1);
        while (!sub.empty() && sub.back() == '\\')
            sub.remove_suffix(1);
        for (const auto* candidate : { &s_MainPath, &s_FallbackPath })
        {
            if (!candidate->empty() && candidate->size() == sub.size() && _strnicmp(candidate->data(), sub.data(), sub.size()) == 0)
            {
                path = *candidate;
                return true;
            }
        }
        return false;
    }

    static PseudoKey* Pseudo(HKEY hKey)
    {
        auto it = s_Pseudo.find(hKey);
        return it != s_Pseudo.end() ? &it->second : nullptr;
    }

    // The real key behind a pseudo handle: opened once it exists (the game may have written it through another
    // handle), created only for the game's own write
    static HKEY RealKey(PseudoKey& p, bool create, bool* created = nullptr)
    {
        if (!p.real)
        {
            HKEY real = nullptr;
            if (Real.OpenKeyExA(HKEY_CURRENT_USER, p.path.c_str(), 0, KEY_READ | KEY_WRITE, &real) != ERROR_SUCCESS)
            {
                real = nullptr;
                DWORD disposition = 0;
                if (create && Real.CreateKeyExA(HKEY_CURRENT_USER, p.path.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                                                KEY_READ | KEY_WRITE, nullptr, &real, &disposition) != ERROR_SUCCESS)
                    real = nullptr;
                if (real && created)
                    *created = disposition == REG_CREATED_NEW_KEY;
            }
            p.real = real;
        }
        return p.real;
    }

    // Opening the missing game key (subKey empty) gives a pseudo handle; anything below it doesn't exist
    static LSTATUS OpenMissing(const std::string& path, LPCSTR lpSubKey, PHKEY phkResult)
    {
        if (!phkResult)
            return ERROR_INVALID_PARAMETER;
        *phkResult = nullptr;
        if (!IsEmpty(lpSubKey))
            return ERROR_FILE_NOT_FOUND;

        // An event handle: a value no real key handle can have
        HANDLE handle = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (!handle)
            return ERROR_OUTOFMEMORY;
        s_Pseudo.emplace(reinterpret_cast<HKEY>(handle), PseudoKey{ path });
        *phkResult = reinterpret_cast<HKEY>(handle);
        return ERROR_SUCCESS;
    }

    static LSTATUS Track(LSTATUS status, bool gameKey, PHKEY phkResult)
    {
        if (status == ERROR_SUCCESS && gameKey && phkResult && *phkResult)
            s_GameKeys.insert(*phkResult);
        return status;
    }

    // A registered default as the real API answers it: REG_DWORD for numbers, REG_SZ otherwise
    static LSTATUS QueryDefault(LPCSTR lpValueName, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData, bool asString = false)
    {
        auto it = s_Defaults.find(IsEmpty(lpValueName) ? "@" : lpValueName);
        if (it == s_Defaults.end())
            return ERROR_FILE_NOT_FOUND;
        if (lpData && !lpcbData)
            return ERROR_INVALID_PARAMETER;

        const std::string& value = it->second;
        char* end = nullptr;
        DWORD number = strtoul(value.c_str(), &end, 10);
        bool isDword = !asString && !value.empty() && end && *end == '\0';

        DWORD type = isDword ? REG_DWORD : REG_SZ;
        DWORD size = isDword ? sizeof(DWORD) : static_cast<DWORD>(value.size() + 1);
        if (lpType)
            *lpType = type;
        if (!lpData)
        {
            if (lpcbData)
                *lpcbData = size;
            return ERROR_SUCCESS;
        }
        if (*lpcbData < size)
        {
            *lpcbData = size;
            return ERROR_MORE_DATA;
        }
        if (isDword)
            memcpy(lpData, &number, sizeof(number));
        else
            memcpy(lpData, value.c_str(), size);
        *lpcbData = size;
        return ERROR_SUCCESS;
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
    // The developer overlay (Ctrl+F10, Back + Start) exists only in Debug builds: Release builds never draw it,
    // so it never initializes and its hotkey, pad combo and input blocking stay inert
#ifdef DEBUG
    static constexpr bool DeveloperOverlay = true;
#else
    static constexpr bool DeveloperOverlay = false;
#endif

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
