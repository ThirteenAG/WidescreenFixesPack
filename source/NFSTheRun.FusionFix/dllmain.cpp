#include "stdafx.h"
#include "FixSettings.hpp"

// Runtime settings interface for menu hosts and the hub.
ASI_SETTINGS_EXPORT(FixSettings())

import ComVars;

std::once_flag of;

namespace fb
{
    namespace Main
    {
        SafetyHookInline shinternalUpdate = {};
        void __fastcall internalUpdate(void* Main, void* edx, char lowLatencyUpdate)
        {
            std::call_once(of, []()
            {
                WFP::onGameInitEvent().executeAll();
            });

            static LARGE_INTEGER frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
            static LARGE_INTEGER last = {};
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            if (last.QuadPart)
                fb::GameTimer::frameDeltaTime = std::min(float(double(now.QuadPart - last.QuadPart) / double(frequency.QuadPart)), 0.1f);
            last = now;

            WFP::onGameProcessEvent().executeAll();

            shinternalUpdate.unsafe_fastcall(Main, edx, lowLatencyUpdate);
        }
    }
}

void Init()
{
    auto pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? 50 FF 15 ? ? ? ? 85 C0");
    hWnd.SetAddress(*pattern.get_first<HWND*>(1));

    pattern = hook::pattern("81 EC ? ? ? ? 53 55 56 57 8B F1 E8 ? ? ? ? 88 44 24");
    fb::Main::shinternalUpdate = safetyhook::create_inline(pattern.get_first(), fb::Main::internalUpdate);

    pattern = hook::pattern("8B 0D ? ? ? ? 8B E8 E8 ? ? ? ? 8B 4C BE");
    pRenderDevice.SetAddress(*pattern.get_first<void**>(2));

    pattern = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 5F 5E B8");
    fb::g_profileOptionsManager.SetAddress(*pattern.get_first<void***>(2));

    pattern = hook::pattern("E8 ? ? ? ? 83 F8 ? 75 ? 83 BE");
    fb::ProfileOptions::getInt = (decltype(fb::ProfileOptions::getInt))injector::GetBranchDestination(pattern.get_first()).as_int();

    // The settings manager and its lookup, where the game reads GameTimeSettings
    pattern = hook::pattern("A1 ? ? ? ? 85 C0 75 ? 8B 0D ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? A3 ? ? ? ? 8A 48 40");
    if (!pattern.empty())
    {
        fb::Settings::pManager = *pattern.get_first<void**>(11);
        fb::Settings::getContainer = (decltype(fb::Settings::getContainer))injector::GetBranchDestination(pattern.get_first(20)).as_int();
    }

    // The update of the player's car control
    pattern = hook::pattern("80 7E ? ? 57 74 ? 33 C0");
    if (!pattern.empty())
    {
        static auto VehicleControlHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            fb::Vehicle::bHasControl = *(uint8_t*)(regs.esi + 0x4) != 0;
            fb::Vehicle::LastUpdate = GetTickCount64();
        });
    }

    pattern = hook::pattern("8B 8E ? ? ? ? 3B ? 74 ? E8 ? ? ? ? E8");
    static auto GetActualDeltaTime = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        fb::GameTimer::actualDeltaTime = *(float*)(regs.esp + 0x18);
    });

    WFP::onInitEvent().executeAll();
    asi_settings::LiveLink::Start();

    static auto futures = WFP::onInitEventAsync().executeAllAsync();

    WFP::onGameInitEvent() += []()
    {
        for (auto& f : futures.get())
            f.wait();
        futures.get().clear();
    };
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("81 EC ? ? ? ? 53 55 56 57 8B F1 E8 ? ? ? ? 88 44 24"));
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
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}
