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
#include <future>
#include <shlobj.h>
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
#include <stacktrace>
#pragma warning(pop)

#ifndef CEXP
#define CEXP extern "C" __declspec(dllexport)
#endif

float GetFOV(float f, float ar);
float GetFOV2(float f, float ar);
float AdjustFOV(float f, float ar);

bool IsModuleUAL(HMODULE mod);
bool IsUALPresent();
void CreateThreadAutoClose(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
std::tuple<int32_t, int32_t> GetDesktopRes();
void GetResolutionsList(std::vector<std::string>& list);
uint32_t GetDesktopRefreshRate();
std::string format(const char* fmt, ...);
uint32_t crc32(uint32_t crc, const void* buf, size_t size);

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

template<class T>
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

template<class T>
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

template<class T>
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

template<class T>
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

template<class T>
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

template<class T>
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

class CallbackHandler
{
public:
    static inline void RegisterCallback(std::function<void()>&& fn)
    {
        fn();
    }

    static inline void RegisterCallback(std::wstring_view module_name, std::function<void()>&& fn, bool bOnUnload = false)
    {
        if (!bOnUnload && (module_name.empty() || GetModuleHandleW(module_name.data()) != NULL))
        {
            fn();
        }
        else
        {
            RegisterDllNotification();
            if (!bOnUnload)
                GetOnModuleLoadCallbackList().emplace(module_name, std::forward<std::function<void()>>(fn));
            else
                GetOnModuleUnloadCallbackList().emplace(module_name, std::forward<std::function<void()>>(fn));
        }
    }

    static inline void RegisterCallback(std::function<void(HMODULE)>&& fn)
    {
        RegisterDllNotification();
        GetOnAnyModuleLoadCallbackList().emplace_back(std::forward<std::function<void(HMODULE)>>(fn));
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
    static inline void invokeOnModuleLoad(std::wstring_view module_name)
    {
        if (GetOnModuleLoadCallbackList().count(module_name.data()))
        {
            GetOnModuleLoadCallbackList().at(module_name.data())();
        }
    }

    static inline void invokeOnUnload(std::wstring_view module_name)
    {
        if (GetOnModuleUnloadCallbackList().count(module_name.data()))
        {
            GetOnModuleUnloadCallbackList().at(module_name.data())();
        }
    }

    static inline void invokeOnAnyModuleLoad(HMODULE mod)
    {
        if (!GetOnAnyModuleLoadCallbackList().empty())
        {
            for (auto& f : GetOnAnyModuleLoadCallbackList())
            {
                f(mod);
            }
        }
    }

    static inline void invokeOnAnyModuleUnload(HMODULE mod)
    {
        if (!GetOnAnyModuleUnloadCallbackList().empty())
        {
            for (auto& f : GetOnAnyModuleUnloadCallbackList())
            {
                f(mod);
            }
        }
    }

    static inline void InvokeAll()
    {
        for (auto&& fn : GetOnModuleLoadCallbackList())
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

    static std::map<std::wstring, std::function<void()>, Comparator>& GetOnModuleLoadCallbackList()
    {
        return onModuleLoad;
    }

    static std::map<std::wstring, std::function<void()>, Comparator>& GetOnModuleUnloadCallbackList()
    {
        return onModuleUnload;
    }

    static inline std::vector<std::function<void(HMODULE)>>& GetOnAnyModuleLoadCallbackList()
    {
        return onAnyModuleLoad;
    }

    static inline std::vector<std::function<void(HMODULE)>>& GetOnAnyModuleUnloadCallbackList()
    {
        return onAnyModuleUnload;
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
        static constexpr auto LDR_DLL_NOTIFICATION_REASON_UNLOADED = 2;
        if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
        {
            invokeOnModuleLoad(NotificationData->Loaded.BaseDllName->Buffer);
            invokeOnAnyModuleLoad((HMODULE)NotificationData->Loaded.DllBase);
        }
        else if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_UNLOADED)
        {
            invokeOnUnload(NotificationData->Loaded.BaseDllName->Buffer);
            invokeOnAnyModuleUnload((HMODULE)NotificationData->Loaded.DllBase);
        }
    }

    static inline NTSTATUS NTAPI ProbeCallback(IN HMODULE DllBase, IN PCWSTR FullDllPath, OUT PHANDLE ActivationContext)
    {
        //wprintf(L"ProbeCallback: Base %p, path '%ls', context %p\r\n", DllBase, FullDllPath, *ActivationContext);

        std::wstring str(FullDllPath);
        invokeOnModuleLoad(str.substr(str.find_last_of(L"/\\") + 1));
        invokeOnAnyModuleLoad(DllBase);

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

private:
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
    static std::map<std::wstring, std::function<void()>, Comparator> onModuleLoad;
    static std::map<std::wstring, std::function<void()>, Comparator> onModuleUnload;
    static inline std::vector<std::function<void(HMODULE)>> onAnyModuleLoad;
    static inline std::vector<std::function<void(HMODULE)>> onAnyModuleUnload;
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
                auto pFunctions = reinterpret_cast<void**>(instance + max(sec->PointerToRawData, sec->VirtualAddress));

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