#include "stdafx.h"
#include "LEDEffects.h"

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

template <class... Ts>
void ReplaceUSER32IAT(Ts&& ... inputs)
{
    auto start = *hook::get_pattern<uintptr_t>("FF 15 ? ? ? ? 85 C0 75 66", 2);
    auto end = start + 0x47C;
    for (auto i = start; i < end; i += sizeof(size_t))
    {
        DWORD dwProtect[2];
        VirtualProtect((size_t*)i, sizeof(size_t), PAGE_EXECUTE_READWRITE, &dwProtect[0]);

        auto ptr = *(size_t*)i;
        if (!ptr)
            continue;

        ([&]
        {
            auto func_name = std::get<0>(inputs);
            auto func_hook = std::get<1>(inputs);
            if (func_hook && ptr == (size_t)GetProcAddress(GetModuleHandleA("USER32.DLL"), func_name))
                *(size_t*)i = (size_t)func_hook;
        } (), ...);

        VirtualProtect((size_t*)i, sizeof(size_t), dwProtect[0], &dwProtect[1]);
    }
}

void Init()
{
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

    CIniReader iniReader("");
    auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
    auto bLightSyncRGB = iniReader.ReadInteger("MAIN", "LightSyncRGB", 1) != 0;

    if (bBorderlessWindowed)
    {
        auto ptr = *hook::get_pattern<HWND*>("8B 0D ? ? ? ? 89 44 24 20", 2);
        std::thread([](HWND* ptr)
        {
            while (!*ptr)
                std::this_thread::yield();
            WindowedModeWrapper::GameHWND = *ptr;
            WindowedModeWrapper::SetWindowLongA_Hook(*ptr, 0, GetWindowLong(*ptr, GWL_STYLE));
        }, ptr).detach();

        ReplaceUSER32IAT(
            std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
            std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
            std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
            std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
            std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
            std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
        );
    }

    // Camera near clip fix
    p6D1650 = injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7"));
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 55"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 80 BF ? ? ? ? ? 74 0E"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? F3 0F 10 AC 24 ? ? ? ? F3 0F 10 84 24 ? ? ? ? F3 0F 10 A4 24"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8D 97 ? ? ? ? 52 8D 87"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 83 EC 28"), sub_6D1650, true);
    injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? F3 0F 10 44 24 ? F3 0F 10 4C 24 ? F3 0F 10 7C 24 ? 0F 28 D1"), sub_6D1650, true);

    if (bLightSyncRGB)
    {
        bLogiLedInitialized = LogiLedInit();

        if (bLogiLedInitialized)
        {
            static auto sPlayerPtr = *hook::get_pattern<void*>("8B 0D ? ? ? ? 56 E8 ? ? ? ? A1 ? ? ? ? 8B 48 20 69 C9 ? ? ? ? 8B 94 01", 2);

            std::thread t([]() 
            {
                while (true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    if (bLogiLedInitialized)
                    {
                        if (sPlayerPtr)
                        {
                            auto Player1Health = PtrWalkthrough<int16_t>(sPlayerPtr, 0x28, 0x2A74, 0x1364);
                            auto Player2Health = PtrWalkthrough<int16_t>(sPlayerPtr, 0x28, 0x1364);

                            if (Player1Health && Player2Health)
                            {
                                auto health1 = *Player1Health;
                                auto health2 = *Player2Health;
                                if (health1 > 1)
                                {
                                    if (health1 == 2) {
                                        LEDEffects::SetLightingLeftSide(26, 4, 4, true, false); //red
                                        LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                    }
                                    else if (health1 <= 250) {
                                        LEDEffects::SetLightingLeftSide(50, 30, 4, true, false); //orange
                                        LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                                    }
                                    else {
                                        LEDEffects::SetLightingLeftSide(10, 30, 4, true, false);  //green
                                        LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                                    }                                   
                                }
                                else
                                {
                                    LEDEffects::SetLightingLeftSide(26, 4, 4, false, true); //red
                                    LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                                }

                                if (health2 > 1)
                                {
                                    if (health2 == 2) {
                                        LEDEffects::SetLightingRightSide(26, 4, 4, true, false); //red
                                        LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0); //red
                                    }
                                    else if (health2 <= 250) {
                                        LEDEffects::SetLightingRightSide(50, 30, 4, true, false); //orange
                                        LEDEffects::DrawCardiogramNumpad(67, 0, 0, 0, 0, 0); //orange
                                    }
                                    else {
                                        LEDEffects::SetLightingRightSide(10, 30, 4, true, false);  //green
                                        LEDEffects::DrawCardiogramNumpad(0, 100, 0, 0, 0, 0); //green
                                    }                                    
                                }
                                else
                                {
                                    LEDEffects::SetLightingRightSide(26, 4, 4, false, true); //red
                                    LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0, true);
                                }
                            }
                            else
                            {
                                LogiLedStopEffects();
                                LEDEffects::SetLighting(98, 87, 24); //logo gold
                            }
                        }
                    }
                    else
                        break;
                }
            });

            t.detach();
        }
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
