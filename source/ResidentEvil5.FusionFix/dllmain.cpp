#include "stdafx.h"
#include "LogitechLEDLib.h"
#pragma comment(lib, "LogitechLEDLib_x86.lib")

static bool bLogiLedInitialized = false;

HMODULE hm = NULL;
HMODULE ual = NULL;

typedef HANDLE(WINAPI* tCreateFileA)(LPCSTR lpFilename, DWORD dwAccess, DWORD dwSharing, LPSECURITY_ATTRIBUTES saAttributes, DWORD dwCreation, DWORD dwAttributes, HANDLE hTemplate);
typedef HANDLE(WINAPI* tCreateFileW)(LPCWSTR lpFilename, DWORD dwAccess, DWORD dwSharing, LPSECURITY_ATTRIBUTES saAttributes, DWORD dwCreation, DWORD dwAttributes, HANDLE hTemplate);
typedef DWORD(WINAPI* tGetFileAttributesA)(LPCSTR lpFileName);
typedef DWORD(WINAPI* tGetFileAttributesW)(LPCWSTR lpFileName);
typedef BOOL(WINAPI* tGetFileAttributesExA)(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
typedef BOOL(WINAPI* tGetFileAttributesExW)(LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
tCreateFileA ptrCreateFileA;
tCreateFileW ptrCreateFileW;
tGetFileAttributesA ptrGetFileAttributesA;
tGetFileAttributesW ptrGetFileAttributesW;
tGetFileAttributesExA ptrGetFileAttributesExA;
tGetFileAttributesExW ptrGetFileAttributesExW;

injector::memory_pointer_raw p6D1650;
void __fastcall sub_6D1650(float* _this, void* edx, float a2, float a3, float a4, float a5)
{
    a2 /= 1280.0f / 800.0f;
    return injector::fastcall<void(float*, void*, float, float, float, float)>::call(p6D1650, _this, edx, a2, a3, a4, a5);
}

template <typename T, typename PtrSize = uint32_t>
std::optional<T> PtrWalkthrough(auto addr, std::convertible_to<ptrdiff_t> auto&& ...offsets)
{
    auto list = std::vector<ptrdiff_t>{ offsets... };
    auto last = list.back(); list.pop_back();
    auto a = injector::ReadMemory<PtrSize>(addr);
    for (auto v : list)
    {
        auto ptr = injector::ReadMemory<PtrSize>(a + v);
        if (ptr)
            a = ptr;
        else
        {
            a = 0;
            break;
        }
    }

    if (a)
        return injector::ReadMemory<T>(a + last);
    else
        return std::nullopt;
};

void Init()
{
    Sleep(10000);

    static std::once_flag flag;
    std::call_once(flag, []()
    {
        ModuleList dlls;
        dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);
        for (auto& e : dlls.m_moduleList)
        {
            auto m = std::get<HMODULE>(e);
            auto pCreateFileA = (tCreateFileA)GetProcAddress(m, "CustomCreateFileA");
            auto pCreateFileW = (tCreateFileW)GetProcAddress(m, "CustomCreateFileW");
            auto pGetFileAttributesA = (tGetFileAttributesA)GetProcAddress(m, "CustomGetFileAttributesA");
            auto pGetFileAttributesW = (tGetFileAttributesW)GetProcAddress(m, "CustomGetFileAttributesW");
            auto pGetFileAttributesExA = (tGetFileAttributesExA)GetProcAddress(m, "CustomGetFileAttributesExA");
            auto pGetFileAttributesExW = (tGetFileAttributesExW)GetProcAddress(m, "CustomGetFileAttributesExW");
            if (pCreateFileA && pCreateFileW)
            {
                ual = m;
                ptrCreateFileA = pCreateFileA;
                ptrCreateFileW = pCreateFileW;
                if (pGetFileAttributesA)
                    ptrGetFileAttributesA = pGetFileAttributesA;
                if (pGetFileAttributesW)
                    ptrGetFileAttributesW = pGetFileAttributesW;
                if (pGetFileAttributesExA)
                    ptrGetFileAttributesExA = pGetFileAttributesExA;
                if (pGetFileAttributesExW)
                    ptrGetFileAttributesExW = pGetFileAttributesExW;
                break;
            }
        }
        //MessageBox(0,0,0,0);
        auto start = (uintptr_t)injector::ReadMemory<uint32_t>(hook::get_pattern("FF 15 ? ? ? ? 85 C0 75 66", 2));
        auto end = start + 0x47C;

        for (auto i = start; i < end; i += sizeof(size_t))
        {
            DWORD dwProtect[2];
            VirtualProtect((size_t*)i, sizeof(size_t), PAGE_EXECUTE_READWRITE, &dwProtect[0]);

            auto ptr = *(size_t*)i;
            if (!ptr)
                continue;

            if (ptr == (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "CreateFileA"))
            {
                *(size_t*)i = (size_t)ptrCreateFileA;
            }
            else if (ptr == (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "CreateFileW"))
            {
                *(size_t*)i = (size_t)ptrCreateFileW;
            }
            else if (ptrGetFileAttributesA && ptr == (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetFileAttributesA"))
            {
                *(size_t*)i = (size_t)ptrGetFileAttributesA;
            }
            else if (ptrGetFileAttributesW && ptr == (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetFileAttributesW"))
            {
                *(size_t*)i = (size_t)ptrGetFileAttributesW;
            }
            else if (ptrGetFileAttributesExA && ptr == (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetFileAttributesExA"))
            {
                *(size_t*)i = (size_t)ptrGetFileAttributesExA;
            }
            else if (ptrGetFileAttributesExW && ptr == (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetFileAttributesExW"))
            {
                *(size_t*)i = (size_t)ptrGetFileAttributesExW;
            }

            VirtualProtect((size_t*)i, sizeof(size_t), dwProtect[0], &dwProtect[1]);
        }
    });

    //CIniReader iniReader("");

    // Camera near clip fix
    p6D1650 = injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7"));
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 55"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 80 BF ? ? ? ? ? 74 0E"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? F3 0F 10 AC 24 ? ? ? ? F3 0F 10 84 24 ? ? ? ? F3 0F 10 A4 24"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8D 97 ? ? ? ? 52 8D 87"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 83 EC 28"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? F3 0F 10 44 24 ? F3 0F 10 4C 24 ? F3 0F 10 7C 24 ? 0F 28 D1"), sub_6D1650, true);

    bLogiLedInitialized = LogiLedInit();

    if (bLogiLedInitialized)
    {
        static std::vector<LogiLed::KeyName> keys = {
            LogiLed::KeyName::CAPS_LOCK,
            LogiLed::KeyName::A,
            LogiLed::KeyName::S,
            LogiLed::KeyName::D,
            LogiLed::KeyName::F,
            LogiLed::KeyName::T,
            LogiLed::KeyName::SIX,
            LogiLed::KeyName::Y,
            LogiLed::KeyName::H,
            LogiLed::KeyName::N,
            LogiLed::KeyName::J,
            LogiLed::KeyName::I,
            LogiLed::KeyName::NINE,
            LogiLed::KeyName::O,
            LogiLed::KeyName::L,
            LogiLed::KeyName::SEMICOLON,
            LogiLed::KeyName::APOSTROPHE,
            LogiLed::KeyName::ENTER,
        };

        static std::vector<LogiLed::KeyName> keys_dead = {
            LogiLed::KeyName::CAPS_LOCK,
            LogiLed::KeyName::A,
            LogiLed::KeyName::S,
            LogiLed::KeyName::D,
            LogiLed::KeyName::F,
            LogiLed::KeyName::G,
            LogiLed::KeyName::H,
            LogiLed::KeyName::J,
            LogiLed::KeyName::K,
            LogiLed::KeyName::L,
            LogiLed::KeyName::SEMICOLON,
            LogiLed::KeyName::APOSTROPHE,
            LogiLed::KeyName::ENTER,
        };

        static auto sPlayerPtr = *hook::get_pattern<void*>("8B 0D ? ? ? ? 56 E8 ? ? ? ? A1 ? ? ? ? 8B 48 20 69 C9 ? ? ? ? 8B 94 01", 2);

        std::thread t([]() 
        {
            while (true)
            {
                if (bLogiLedInitialized)
                {
                    if (sPlayerPtr)
                    {
                        auto PlayerHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x28, 0x2A74, 0x1364);

                        if (PlayerHealth)
                        {
                            auto health = *PlayerHealth;
                            if (health > 1)
                            {
                                if (health == 2)
                                    LogiLedSetLighting(26, 4, 4); //red
                                else if (health <= 250)
                                    LogiLedSetLighting(25, 15, 2); //orange
                                else
                                    LogiLedSetLighting(5, 15, 2);  //green

                                for (auto& key : keys)
                                {
                                    auto s = 5 * keys.size();
                                    std::this_thread::sleep_for(std::chrono::milliseconds(s));
                                    if (health == 2)
                                        LogiLedPulseSingleKey(key, 255, 0, 0, 0, 0, 0, s * 50, false); //red
                                    else if (health <= 250)
                                        LogiLedPulseSingleKey(key, 67, 0, 0, 0, 0, 0, s * 50, false); //orange
                                    else
                                        LogiLedPulseSingleKey(key, 0, 255, 0, 0, 0, 0, s * 50, false); //green
                                }
                            }
                            else
                            {
                                for (auto& key : keys_dead)
                                {
                                    auto s = 5 * keys_dead.size();
                                    std::this_thread::sleep_for(std::chrono::milliseconds(s));
                                    LogiLedPulseSingleKey(key, 255, 0, 0, 0, 0, 0, s * 50, false);
                                }
                            }
                        }
                        else
                        {
                            LogiLedSetLighting(98, 87, 24); //logo gold
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
                else
                    break;
            }
        });

        t.detach();
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        if (bLogiLedInitialized) {
            LogiLedShutdown();
            bLogiLedInitialized = false;
        }
    }
    return TRUE;
}
