module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

import LightSync;
import NativeMenu;

export namespace REUpdates
{
    inline SafetyHookInline quitHook;
    inline SafetyHookMid captureQuitCode, preserveQuitCode;
    inline thread_local WPARAM quitCode{};
    inline constexpr size_t quitCodeOffset = 0x1C;

    inline LRESULT __fastcall Quit(void* system, void*)
    {
        RELightSync::Shutdown();
        return quitHook.thiscall<LRESULT>(system);
    }

    inline void WINAPI Exit(UINT code)
    {
        RELightSync::Shutdown();
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Install()
    {
        if (StartupOption("MAIN.LightSyncRGB"))
        {
            auto quit = hook::pattern("6A 00 C6 41 34 01 A1 ? ? ? ? 6A 00 6A 02 FF B0 2C 01 00 00 FF 15 ? ? ? ? C3");
            quitHook = safetyhook::create_inline(quit.get_first(), Quit);
        }
        auto receivedQuit = hook::pattern("84 DB 74 ? 6A 01 FF 15 ? ? ? ? 8B ? E8 ? ? ? ? 33");
        captureQuitCode = safetyhook::create_mid(receivedQuit.get_first(4), [](SafetyHookContext& regs)
        {
            // The shutdown loop reuses this MSG while joining the game thread.
            quitCode = *reinterpret_cast<const WPARAM*>(regs.esp + quitCodeOffset);
            RELightSync::Shutdown();
            RWGTA::UpdateCheck::OnExit();
        });
        auto returning = hook::pattern("FF 15 ? ? ? ? 8B 44 24 1C 5F 5E 5D 5B 83 C4 20 C3");
        preserveQuitCode = safetyhook::create_mid(returning.get_first(6), [](SafetyHookContext& regs)
        {
            *reinterpret_cast<WPARAM*>(regs.esp + quitCodeOffset) = quitCode;
        });
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "ResidentEvil.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = REMenu::NoticeReady;
        config.show = REMenu::ShowNotice;
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}
