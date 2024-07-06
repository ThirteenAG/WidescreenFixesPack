#pragma once
#include <Windows.h>
#include <functional>
#include <string>
#include <string_view>
#include <optional>
#include "safetyhook.hpp"

class CallbackHandler
{
public:
    static inline void RegisterCallback(std::function<void()>&& fn)
    {
        return fn();
    }

    static inline void RegisterCallback(std::wstring_view module_name, std::function<void()>&& fn)
    {
        return RegisterModuleLoadCallback(module_name, std::forward<std::function<void()>>(fn));
    }

    static inline void RegisterModuleLoadCallback(std::wstring_view module_name, std::function<void()>&& fn)
    {
        if ((module_name.empty() || GetModuleHandleW(module_name.data()) != NULL))
        {
            return fn();
        }
        else
        {
            RegisterDllNotification();
            GetOnModuleLoadCallbackList().emplace(module_name, std::forward<std::function<void()>>(fn));
        }
    }

    static inline void RegisterModuleUnloadCallback(std::wstring_view module_name, std::function<void()>&& fn)
    {
        RegisterDllNotification();
        GetOnModuleUnloadCallbackList().emplace(module_name, std::forward<std::function<void()>>(fn));
    }

    static inline void RegisterAnyModuleLoadCallback(std::function<void(HMODULE)>&& fn)
    {
        RegisterDllNotification();
        GetOnAnyModuleLoadCallbackList().emplace_back(std::forward<std::function<void(HMODULE)>>(fn));
    }

    static inline void RegisterAnyModuleUnloadCallback(std::function<void(std::wstring_view module_name)>&& fn)
    {
        RegisterDllNotification();
        GetOnAnyModuleUnloadCallbackList().emplace_back(std::forward<std::function<void(std::wstring_view)>>(fn));
    }

    //[[deprecated("Use RegisterCallbackAtGetSystemTimeAsFileTime instead.")]]
    //static inline void RegisterCallback(std::function<void()>&& fn, bool bPatternNotFound, ptrdiff_t offset = 0x1100, uint32_t* ptr = nullptr)
    //{
    //    if (!bPatternNotFound)
    //    {
    //        return fn();
    //    }
    //    else
    //    {
    //        auto mh = GetModuleHandle(NULL);
    //        IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)((DWORD)mh + ((IMAGE_DOS_HEADER*)mh)->e_lfanew);
    //        if (ptr == nullptr)
    //            ptr = (uint32_t*)((DWORD)mh + ntHeader->OptionalHeader.BaseOfCode + ntHeader->OptionalHeader.SizeOfCode - offset);
    //        std::thread([](std::function<void()>&& fn, uint32_t* ptr, uint32_t val)
    //        {
    //            while (*ptr == val)
    //                std::this_thread::yield();
    //
    //            fn();
    //        }, fn, ptr, *ptr).detach();
    //    }
    //}
    
    [[deprecated("Use RegisterCallbackAtGetSystemTimeAsFileTime instead.")]]
    static inline void RegisterCallback(std::function<void()>&& fn, hook::pattern pattern)
    {
        if (!pattern.empty())
        {
            fn();
        }
        else
        {
            auto* ptr = new ThreadParams{ fn, pattern };
            CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&ThreadProc, (LPVOID)ptr, 0, NULL));
        }
    }

    static inline void RegisterCallbackAtGetSystemTimeAsFileTime(std::function<void()>&& fn)
    {
        GetCallbackParamsList().emplace_back(fn, std::nullopt);
        if (!shGetSystemTimeAsFileTime)
            shGetSystemTimeAsFileTime = safetyhook::create_inline(GetSystemTimeAsFileTime, GetSystemTimeAsFileTimeHook);
    }

    static inline void RegisterCallbackAtGetSystemTimeAsFileTime(std::function<void()>&& fn, hook::pattern pattern)
    {
        if (!pattern.empty())
        {
            return fn();
        }
        else
        {
            GetCallbackParamsList().emplace_back(fn, pattern);
            if (!shGetSystemTimeAsFileTime)
                shGetSystemTimeAsFileTime = safetyhook::create_inline(GetSystemTimeAsFileTime, GetSystemTimeAsFileTimeHook);
        }
    }

private:
    static inline void invokeOnModuleLoad(std::wstring_view module_name)
    {
        if (GetOnModuleLoadCallbackList().count(module_name.data()))
            GetOnModuleLoadCallbackList().at(module_name.data())();
    }

    static inline void invokeOnUnload(std::wstring_view module_name)
    {
        if (GetOnModuleUnloadCallbackList().count(module_name.data()))
            GetOnModuleUnloadCallbackList().at(module_name.data())();
    }

    static inline void invokeOnAnyModuleLoad(HMODULE mod)
    {
        if (!GetOnAnyModuleLoadCallbackList().empty())
        {
            for (auto& f : GetOnAnyModuleLoadCallbackList())
                f(mod);
        }
    }

    static inline void invokeOnAnyModuleUnload(std::wstring_view module_name)
    {
        if (!GetOnAnyModuleUnloadCallbackList().empty())
        {
            for (auto& f : GetOnAnyModuleUnloadCallbackList())
                f(module_name);
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

    struct ThreadParams
    {
        std::function<void()> fn;
        std::optional<hook::pattern> pattern;
    };

    static inline auto& GetOnModuleLoadCallbackList()
    {
        static std::map<std::wstring, std::function<void()>, Comparator> onModuleLoad;
        return onModuleLoad;
    }

    static inline auto& GetOnModuleUnloadCallbackList()
    {
        static std::map<std::wstring, std::function<void()>, Comparator> onModuleUnload;
        return onModuleUnload;
    }

    static inline auto& GetOnAnyModuleLoadCallbackList()
    {
        static std::vector<std::function<void(HMODULE)>> onAnyModuleLoad;
        return onAnyModuleLoad;
    }

    static inline auto& GetOnAnyModuleUnloadCallbackList()
    {
        static std::vector<std::function<void(std::wstring_view)>> onAnyModuleUnload;
        return onAnyModuleUnload;
    }

    static inline auto& GetCallbackParamsList()
    {
        static std::vector<ThreadParams> callbackParams;
        return callbackParams;
    }

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
            invokeOnAnyModuleUnload(NotificationData->Loaded.BaseDllName->Buffer);
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

    template<typename Callable>
    using ReturnType = typename decltype(std::function{ std::declval<Callable>() })::result_type;

    static inline SafetyHookInline shGetSystemTimeAsFileTime = {};
    static inline void WINAPI GetSystemTimeAsFileTimeHook(LPFILETIME lpSystemTimeAsFileTime)
    {
        auto& threadParams = GetCallbackParamsList();

        for (auto& it : threadParams)
        {
            if (!it.pattern.has_value() || !it.pattern.value().clear().empty())
                it.fn();
        }

        shGetSystemTimeAsFileTime.stdcall<ReturnType<decltype(GetSystemTimeAsFileTime)>>(lpSystemTimeAsFileTime);

        if (threadParams.empty())
            shGetSystemTimeAsFileTime = {};
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

        while (params.pattern.value().clear().empty())
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
};