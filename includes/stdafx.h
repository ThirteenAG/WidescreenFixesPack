#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES
#pragma warning(push)
#pragma warning(disable: 4178 4305 4309 4510 4996)
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
#include "IniReader.h"
#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#ifdef _M_IX86
#include "injector\assembly.hpp"
#endif
#include "injector\utility.hpp"
#include "Hooking.Patterns.h"
#include "log.h"
#include "ModuleList.hpp"
#include <filesystem>
#pragma warning(pop)

#ifndef CEXP
#define CEXP extern "C" __declspec(dllexport)
#endif

float GetFOV(float f, float ar);
float GetFOV2(float f, float ar);
float AdjustFOV(float f, float ar);

bool IsUALPresent();
void CreateThreadAutoClose(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
std::tuple<int32_t, int32_t> GetDesktopRes();
void GetResolutionsList(std::vector<std::string>& list);
std::string format(const char* fmt, ...);
HICON CreateIconFromBMP(UCHAR* data);
HICON CreateIconFromResourceICO(UINT nID, int32_t cx, int32_t cy);

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

template<class T>
T GetModulePath(HMODULE hModule)
{
    static constexpr auto INITIAL_BUFFER_SIZE = MAX_PATH;
    static constexpr auto MAX_ITERATIONS = 7;
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
    return T();
}

template<class T>
T GetThisModulePath()
{
    HMODULE hm = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetResolutionsList, &hm);
    T r = GetModulePath<T>(hm);
    if constexpr (std::is_same_v<T, std::string>)
        r = r.substr(0, r.find_last_of("/\\") + 1);
    else
        r = r.substr(0, r.find_last_of(L"/\\") + 1);
    return r;
}

template<class T>
T GetThisModuleName()
{
    HMODULE hm = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetResolutionsList, &hm);
    const T moduleFileName = GetModulePath<T>(hm);
    if constexpr (std::is_same_v<T, std::string>)
        return moduleFileName.substr(moduleFileName.find_last_of("/\\") + 1);
    else
        return moduleFileName.substr(moduleFileName.find_last_of(L"/\\") + 1);
}

template<class T>
T GetExeModulePath()
{
    T r = GetModulePath<T>(NULL);
    if constexpr (std::is_same_v<T, std::string>)
        r = r.substr(0, r.find_last_of("/\\") + 1);
    else
        r = r.substr(0, r.find_last_of(L"/\\") + 1);
    return r;
}

template<class T>
T GetExeModuleName()
{
    const T moduleFileName = GetModulePath<T>(NULL);
    if constexpr (std::is_same_v<T, std::string>)
        return moduleFileName.substr(moduleFileName.find_last_of("/\\") + 1);
    else
        return moduleFileName.substr(moduleFileName.find_last_of(L"/\\") + 1);
}

template<class T>
T GetCurrentDirectoryW()
{
    static constexpr auto INITIAL_BUFFER_SIZE = MAX_PATH;
    static constexpr auto MAX_ITERATIONS = 7;
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

class CallbackHandler
{
public:
    static inline void RegisterCallback(std::function<void()>&& fn)
    {
        fn();
    }

    static inline void RegisterCallback(std::wstring_view module_name, std::function<void()>&& fn)
    {
        if (module_name.empty() || GetModuleHandleW(module_name.data()) != NULL)
        {
            fn();
        }
        else
        {
            RegisterDllNotification();
            GetCallbackList().emplace(module_name, std::forward<std::function<void()>>(fn));
        }
    }

    static inline void RegisterCallback(std::function<void()>&& fn, bool bPatternNotFound, ptrdiff_t offset = 0x1100, uint32_t* ptr = nullptr)
    {
        if (!bPatternNotFound)
        {
            fn();
        }
        else
        {
            auto mh = GetModuleHandle(NULL);
            IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)((DWORD)mh + ((IMAGE_DOS_HEADER*)mh)->e_lfanew);
            if (ptr == nullptr)
                ptr = (uint32_t*)((DWORD)mh + ntHeader->OptionalHeader.BaseOfCode + ntHeader->OptionalHeader.SizeOfCode - offset);
            std::thread([](std::function<void()>&& fn, uint32_t* ptr, uint32_t val)
            {
                while (*ptr == val)
                    std::this_thread::yield();

                fn();
            }, fn, ptr, *ptr).detach();
        }
    }

    static inline void RegisterCallback(std::function<void()>&& fn, hook::pattern pattern)
    {
        if (!pattern.empty())
        {
            fn();
        }
        else
        {
            auto* ptr = new ThreadParams{ fn, pattern };
            CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ThreadProc, (LPVOID)ptr, 0, NULL);
        }
    }

private:
    static inline void call(std::wstring_view module_name)
    {
        if (GetCallbackList().count(module_name.data()))
        {
            GetCallbackList().at(module_name.data())();
            //GetCallbackList().erase(module_name.data()); //shouldn't do that in case dll with callback gets unloaded and loaded again
        }

        //if (GetCallbackList().empty()) //win7 crash in splinter cell
        //    UnRegisterDllNotification();
    }

    static inline void invoke_all()
    {
        for (auto&& fn : GetCallbackList())
            fn.second();
    }

private:
    struct Comparator
    {
        bool operator() (const std::wstring& s1, const std::wstring& s2) const
        {
            std::wstring str1(s1.length(), ' ');
            std::wstring str2(s2.length(), ' ');
            std::transform(s1.begin(), s1.end(), str1.begin(), tolower);
            std::transform(s2.begin(), s2.end(), str2.begin(), tolower);
            return  str1 < str2;
        }
    };

    static std::map<std::wstring, std::function<void()>, Comparator>& GetCallbackList()
    {
        return functions;
    }

    struct ThreadParams
    {
        std::function<void()> fn;
        hook::pattern pattern;
    };

    typedef NTSTATUS(NTAPI* _LdrRegisterDllNotification) (ULONG, PVOID, PVOID, PVOID);
    typedef NTSTATUS(NTAPI* _LdrUnregisterDllNotification) (PVOID);

    typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
    {
        ULONG Flags;                    //Reserved.
        PUNICODE_STRING FullDllName;    //The full path name of the DLL module.
        PUNICODE_STRING BaseDllName;    //The base file name of the DLL module.
        PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
        ULONG SizeOfImage;              //The size of the DLL image, in bytes.
    } LDR_DLL_LOADED_NOTIFICATION_DATA, LDR_DLL_UNLOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

    typedef union _LDR_DLL_NOTIFICATION_DATA
    {
        LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
        LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
    } LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

    typedef NTSTATUS(NTAPI* PLDR_MANIFEST_PROBER_ROUTINE)
    (
        IN HMODULE DllBase,
        IN PCWSTR FullDllPath,
        OUT PHANDLE ActivationContext
    );

    typedef NTSTATUS(NTAPI* PLDR_ACTX_LANGUAGE_ROURINE)
    (
        IN HANDLE Unk,
        IN USHORT LangID,
        OUT PHANDLE ActivationContext
    );

    typedef void(NTAPI* PLDR_RELEASE_ACT_ROUTINE)
    (
        IN HANDLE ActivationContext
    );

    typedef VOID(NTAPI* fnLdrSetDllManifestProber)
    (
        IN PLDR_MANIFEST_PROBER_ROUTINE ManifestProberRoutine,
        IN PLDR_ACTX_LANGUAGE_ROURINE CreateActCtxLanguageRoutine,
        IN PLDR_RELEASE_ACT_ROUTINE ReleaseActCtxRoutine
    );

private:
    static inline void CALLBACK LdrDllNotification(ULONG NotificationReason, PLDR_DLL_NOTIFICATION_DATA NotificationData, PVOID Context)
    {
        static constexpr auto LDR_DLL_NOTIFICATION_REASON_LOADED = 1;
        if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
        {
            call(NotificationData->Loaded.BaseDllName->Buffer);
        }
    }

    static inline NTSTATUS NTAPI ProbeCallback(IN HMODULE DllBase, IN PCWSTR FullDllPath, OUT PHANDLE ActivationContext)
    {
        //wprintf(L"ProbeCallback: Base %p, path '%ls', context %p\r\n", DllBase, FullDllPath, *ActivationContext);

        std::wstring str(FullDllPath);
        call(str.substr(str.find_last_of(L"/\\") + 1));

        //if (!*ActivationContext)
        //    return STATUS_INVALID_PARAMETER; // breaks on xp

        HANDLE actx = NULL;
        ACTCTXW act = { 0 };

        act.cbSize = sizeof(act);
        act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
        act.lpSource = FullDllPath;
        act.hModule = DllBase;
        act.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;

        // Reset pointer, crucial for x64 version
        *ActivationContext = 0;

        actx = CreateActCtxW(&act);

        // Report no manifest is present
        if (actx == INVALID_HANDLE_VALUE)
            return 0xC000008B; //STATUS_RESOURCE_NAME_NOT_FOUND;

        *ActivationContext = actx;

        return STATUS_SUCCESS;
    }

    static inline void RegisterDllNotification()
    {
        LdrRegisterDllNotification = (_LdrRegisterDllNotification)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrRegisterDllNotification");
        if (LdrRegisterDllNotification)
        {
            if (!cookie)
                LdrRegisterDllNotification(0, LdrDllNotification, 0, &cookie);
        }
        else
        {
            LdrSetDllManifestProber = (fnLdrSetDllManifestProber)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrSetDllManifestProber");
            if (LdrSetDllManifestProber)
            {
                LdrSetDllManifestProber(&ProbeCallback, NULL, &ReleaseActCtx);
            }
        }
    }

    static inline void UnRegisterDllNotification()
    {
        LdrUnregisterDllNotification = (_LdrUnregisterDllNotification)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrUnregisterDllNotification");
        if (LdrUnregisterDllNotification && cookie)
            LdrUnregisterDllNotification(cookie);
    }

    static inline DWORD WINAPI ThreadProc(LPVOID ptr)
    {
        auto paramsPtr = static_cast<CallbackHandler::ThreadParams*>(ptr);
        auto params = *paramsPtr;
        delete paramsPtr;

        HANDLE hTimer = NULL;
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = -30 * 10000000LL;
        hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
        SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);

        while (params.pattern.clear().empty())
        {
            Sleep(0);

            if (WaitForSingleObject(hTimer, 0) == WAIT_OBJECT_0)
            {
                CloseHandle(hTimer);
                return 0;
            }
        };

        params.fn();

        return 0;
    }
private:
    static inline _LdrRegisterDllNotification   LdrRegisterDllNotification;
    static inline _LdrUnregisterDllNotification LdrUnregisterDllNotification;
    static inline void* cookie;
    static inline fnLdrSetDllManifestProber     LdrSetDllManifestProber;
public:
    static inline std::once_flag flag;
    static std::map<std::wstring, std::function<void()>, Comparator> functions;
};

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

    RegistryWrapper(std::string_view searchString, std::string_view iniPath)
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
                        *lpcbData = min(ret.length(), str.length());
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
                    str += format("%02x", lpData[i]);
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