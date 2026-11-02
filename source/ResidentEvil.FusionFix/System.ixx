module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"

#include "Shared/ResidentEvil/IdleCursor.hpp"
#include "LEDEffects.h"

export module System;

import DeveloperOverlay;

// Startup, door transitions and cursor visibility.

namespace
{
    char __fastcall DoorEventStart(uint32_t* _this, void* edx)
    {
        _this[33] = 3;
        _this[31] = 0;
        return 0;
    }
}

class System
{
public:
    System()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();

            // The logo scene (aLogo) shows the division, copyright and Capcom logos (exec 0), then the logo animation
            // (exec 1), then hands over to aInit (exec 2). Starting it at exec 2 goes on to the title at once; the
            // opening movie and everything after it stay.
            if (settings.Bool("MAIN.SkipIntro"))
            {
                auto pattern = hook::pattern("8B 86 A8 00 00 00 83 F8 03 72");
                static SafetyHookMid LogoHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    static bool bSkipped = false;
                    if (!bSkipped && regs.eax == 0)
                    {
                        bSkipped = true;
                        regs.eax = 2;
                        *reinterpret_cast<uint32_t*>(regs.esi + 0xA8) = 2;
                    }
                });

                // Confirm the title's OK-only autosave notice once, after the native notice is ready.
                auto notice = hook::pattern("8B 0D ? ? ? ? 85 B9 6C 0B 00 00 75 ? 84 C0 0F 84 ? ? ? ? FF 86 D8 00 00 00");
                static SafetyHookMid NoticeHook = safetyhook::create_mid(notice.get_first(12), [](SafetyHookContext& regs)
                {
                    static bool skipped = false;
                    if (!skipped)
                    {
                        skipped = true;
                        regs.eflags &= ~uint32_t{ 1 << 6 };
                    }
                });

            }

            static std::atomic<int> nHideMouseCursorAfterMs{ settings.Int("MAIN.HideMouseCursorAfterMs") };
            settings.OnChange("MAIN.HideMouseCursorAfterMs", []()
            {
                nHideMouseCursorAfterMs.store(FixSettings().Int("MAIN.HideMouseCursorAfterMs"), std::memory_order_relaxed);
            });
            // Keep the hook installed so a timeout of zero can be enabled while playing.
            {
                auto pattern = hook::pattern("80 BB ? ? ? ? ? 74 13 FF B3");
                // The native cursor refresh stays intact. Hide only its shape, and restore its current handle after movement.
                static auto MouseCursorHook = safetyhook::create_mid(pattern.get_first(28), [](SafetyHookContext& regs)
                {
                    auto window = *reinterpret_cast<HWND*>(regs.ebx + 0x12C);
                    auto region = *reinterpret_cast<HWND*>(regs.ebx + 0x2664);
                    auto foreground = GetForegroundWindow();
                    POINT point{};
                    const bool focused = foreground && (foreground == window || foreground == region) && GetCursorPos(&point);
                    const bool overlay = REOverlay::BlocksInput();
                    const int timeout = nHideMouseCursorAfterMs.load(std::memory_order_relaxed);
                    static IdleCursor::State state;
                    static bool alwaysHidden = false;
                    if (focused && timeout == -1 && !overlay)
                    {
                        state.Update(point.x, point.y, GetTickCount64(), true, 0);
                        SetCursor(nullptr);
                        alwaysHidden = true;
                        return;
                    }
                    if (focused && alwaysHidden && !overlay)
                    {
                        SetCursor(*reinterpret_cast<HCURSOR*>(regs.ebx + 0x2680));
                        alwaysHidden = false;
                    }
                    switch (state.Update(point.x, point.y, GetTickCount64(), focused && !overlay, timeout))
                    {
                        case IdleCursor::Action::Hide:
                            SetCursor(nullptr);
                            break;
                        case IdleCursor::Action::Restore:
                            SetCursor(*reinterpret_cast<HCURSOR*>(regs.ebx + 0x2680));
                            break;
                        default:
                            break;
                    }
                });
            }

            if (settings.Bool("MAIN.DoorSkip"))
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 6A 00 51 8B 0D");
                injector::MakeCALL(pattern.get_first(), DoorEventStart, true);
                injector::MakeJMP(hook::get_pattern("C7 46 ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? 3B"), hook::get_pattern("8B 46 04 57 33 FF"));
                std::vector<uint8_t> DoorEventReturn = { 0x5F, 0xC7, 0x86, 0x84, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x5E, 0x5D, 0x5B, 0xC2, 0x10, 0x00 };
                auto doorReturn = hook::get_pattern<uint8_t>("81 7E ? ? ? ? ? 75 07");
                for (size_t i = 0; i < DoorEventReturn.size(); ++i)
                    injector::WriteMemory<uint8_t>(doorReturn + i, DoorEventReturn[i], true);
                injector::WriteMemory<uint8_t>(hook::get_pattern("68 FB 00 00 00 EB 1D", 1), 0xFA, true); // Lift Fix
            }
        };
    }
} System;
