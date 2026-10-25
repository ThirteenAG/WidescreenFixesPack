#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"
#include "Display.hpp"

import ComVars;

SafetyHookInline shWindowProc;
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_ACTIVATEAPP)
        WFP::onActivateApp().executeAll(wParam != 0);
    return shWindowProc.stdcall<LRESULT>(window, message, wParam, lParam);
}

void Init()
{
    hook::pattern pattern;
    FixSettings();
    asi_settings::LiveLink::Start();
    if (wfp::PatchPreflight("Ultimate Spider-Man window handle: signatures unavailable; feature disabled.\n",
    {
        { "A1 ? ? ? ? 53 50 FF 15", 1 },
    }))
    {
        pattern = hook::pattern("A1 ? ? ? ? 53 50 FF 15"); // 0x581CBB + 1
        hWnd.SetAddress(*pattern.get_first<HWND*>(1));

    }

    if (wfp::PatchPreflight("Ultimate Spider-Man focus events: signatures unavailable; feature disabled.\n",
    {
        { "53 8B 5C 24 0C 81 FB 01 01 00 00 55 8B 6C 24 18", 1 },
    }))
    {
        pattern = hook::pattern("53 8B 5C 24 0C 81 FB 01 01 00 00 55 8B 6C 24 18"); // 0x5941A0
        shWindowProc = safetyhook::create_inline(pattern.get_first(), WindowProc);

    }

    // Only the final nglFlip scene: offscreen radar and intermediate scenes
    // must not receive post-processing or apply gamma multiple times per frame.
    if (wfp::PatchPreflight("Ultimate Spider-Man frame events: signatures unavailable; feature disabled.\n",
    {
        { "A1 ? ? ? ? 8B 10 50 FF 92 A8 00 00 00 A0", 1 },
    }))
    {
        pattern = hook::pattern("A1 ? ? ? ? 8B 10 50 FF 92 A8 00 00 00 A0"); // 0x76E99E + 1
        Direct3DDevice.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));
        static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            WFP::onEndScene().executeAll();
        });

    }

    if (wfp::PatchPreflight("Ultimate Spider-Man reset events: signatures unavailable; feature disabled.\n",
    {
        { "53 56 E8 ? ? ? ? 6A 01 E8 ? ? ? ? A0 ? ? ? ? 33 DB 83 C4 04", 1 },
    }))
    {
        pattern = hook::pattern("53 56 E8 ? ? ? ? 6A 01 E8 ? ? ? ? A0 ? ? ? ? 33 DB 83 C4 04"); // 0x76E800
        static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            WFP::onBeforeReset().executeAll();
        });

    }

    if (wfp::PatchPreflight("Ultimate Spider-Man initialization events: signatures unavailable; feature disabled.\n",
    {
        { "E8 ? ? ? ? 83 C4 ? 85 C0 74 ? 68 ? ? ? ? 68 ? ? ? ? 8B C8 E8 ? ? ? ? EB ? 33 C0 6A", 1 },
    }))
    {
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 85 C0 74 ? 68 ? ? ? ? 68 ? ? ? ? 8B C8 E8 ? ? ? ? EB ? 33 C0 6A"); //0x5AC1E0
        static auto WinMainHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            WFP::onInitEvent().executeAll();
        });

    }

    // WinMain's per-frame call to the game update/render routine.
    if (wfp::PatchPreflight("Ultimate Spider-Man update events: signatures unavailable; feature disabled.\n",
    {
        { "E8 ? ? ? ? FF D5", 1 },
    }))
    {
        pattern = hook::pattern("E8 ? ? ? ? FF D5"); //0x5AD495
        static const auto resumeFrame = reinterpret_cast<uintptr_t>(pattern.get_first(5));
        static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            static std::once_flag of;
            std::call_once(of, []()
            {
                WFP::onGameInitEvent().executeAll();
            });

            WFP::onGameProcessEvent().executeAll();
            if (USMWindow::ResourcesLost)
                regs.eip = resumeFrame;
        });
    }


}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("81 EC EC 03 00 00 68"));
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
