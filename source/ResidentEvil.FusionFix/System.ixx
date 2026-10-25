module;

#include <stdafx.h>
#include <d3d9.h>
#include <atomic>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include "../Shared/ResidentEvil/IdleCursor.hpp"
#include "LEDEffects.h"

export module System;

// Start-up, window, frame rate and quality-of-life options: no logos at start (MAIN.SkipIntro), the window of the
// game's windowed mode (MAIN.WindowMode), the frame rate cap of the "Variable" frame rate option (MAIN.UnlockFPS),
// door and lift animations skipped (MAIN.DoorSkip), the mouse cursor hidden when it doesn't move, and the GPU sync
// workaround (MAIN.DisableCreateQuery).

namespace
{
    char __fastcall DoorEventStart(uint32_t* _this, void* edx)
    {
        _this[33] = 3;
        _this[31] = 0;
        return 0;
    }

    // The renderer's command buffer sync: an event query issued and waited for with Sleep(0) spins
    void __fastcall gpuCommandBufferSync(IDirect3DDevice9** m_ppD3DDevice, void* edx)
    {
    }
}

export namespace System
{
    void Init()
    {
        auto& settings = FixSettings();

        // The logo scene (aLogo) shows the division, copyright and Capcom logos (exec 0), then the logo animation
        // (exec 1), then hands over to aInit (exec 2). Starting it at exec 2 goes on to the title at once; the
        // opening movie and everything after it stay.
        if (settings.Bool("MAIN.SkipIntro"))
        {
            auto pattern = hook::pattern("8B 86 A8 00 00 00 83 F8 03 72");
            if (PatternUnique(pattern, "ResidentEvil.FusionFix: System"))
            {
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
            }
        }

        static std::atomic<int> nHideMouseCursorAfterMs{ settings.Int("MAIN.HideMouseCursorAfterMs") };
        settings.OnChange("MAIN.HideMouseCursorAfterMs", []()
        {
            nHideMouseCursorAfterMs.store(FixSettings().Int("MAIN.HideMouseCursorAfterMs"), std::memory_order_relaxed);
        });
        // Keep the checked hook installed so a timeout of zero can be enabled while playing.
        {
            auto pattern = hook::pattern("80 BB ? ? ? ? ? 74 13 FF B3");
            if (pattern.size() == 1 && *pattern.get_first<uint32_t>(2) == 0x2684 && *pattern.get_first<uint32_t>(11) == 0x2680)
            {
                // The native cursor refresh stays intact. Hide only its shape, and restore its current handle after movement.
                static auto MouseCursorHook = safetyhook::create_mid(pattern.get_first(28), [](SafetyHookContext& regs)
                {
                    auto window = *reinterpret_cast<HWND*>(regs.ebx + 0x12C);
                    auto region = *reinterpret_cast<HWND*>(regs.ebx + 0x2664);
                    auto foreground = GetForegroundWindow();
                    POINT point{};
                    bool active = foreground && (foreground == window || foreground == region) && GetCursorPos(&point);
                    static IdleCursor::State state;
                    switch (state.Update(point.x, point.y, GetTickCount64(), active, nHideMouseCursorAfterMs.load(std::memory_order_relaxed)))
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
                if (!MouseCursorHook)
                    OutputDebugStringA("Resident Evil Fusion Fix: cursor hook unavailable.\n");
            }
            else
            {
                OutputDebugStringA("Resident Evil Fusion Fix: cursor signature unavailable or ambiguous.\n");
            }
        }

        if (settings.Bool("MAIN.DoorSkip"))
        {
            if (CheckPatterns("ResidentEvil.FusionFix: door skipping", {
                "E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 6A 00 51 8B 0D",
                "C7 46 ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? 3B",
                "8B 46 04 57 33 FF",
                "81 7E ? ? ? ? ? 75 07",
                "68 FB 00 00 00 EB 1D",
            }))
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 6A 00 51 8B 0D");
                injector::MakeCALL(pattern.get_first(), DoorEventStart, true);
                injector::MakeJMP(hook::get_pattern("C7 46 ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? 3B"), hook::get_pattern("8B 46 04 57 33 FF"));
                std::vector<uint8_t> DoorEventReturn = { 0x5F, 0xC7, 0x86, 0x84, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x5E, 0x5D, 0x5B, 0xC2, 0x10, 0x00 };
                auto doorReturn = hook::get_pattern<uint8_t>("81 7E ? ? ? ? ? 75 07");
                for (size_t i = 0; i < DoorEventReturn.size(); ++i)
                    ProtectedGameRef<uint8_t>{doorReturn + i} = DoorEventReturn[i];
                WriteGameValue<uint8_t>(hook::get_pattern("68 FB 00 00 00 EB 1D", 1), 0xFA); // Lift Fix
            }
        }

        // MAIN.WindowMode: the window when the game's own fullscreen option is off; 1 = with a frame, 2 = borderless.
        // A framed window as big as the screen is placed with its frame outside the screen; no rounded corners.
        {
            WindowedModeWrapper::bBorderlessWindowed = settings.Int("MAIN.WindowMode") != 1;
            IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
            );
        }

        if (settings.Bool("MAIN.DisableCreateQuery"))
        {
            auto pattern = find_pattern("51 8B 91 ? ? ? ? 56 8B 35", "51 8B 91 ? ? ? ? 8D 0C 24 56 8B 35");
            if (PatternAvailable(pattern, "ResidentEvil.FusionFix: System"))
                injector::MakeJMP(pattern.get_first(), gpuCommandBufferSync, true);
            else
                OutputDebugStringA("ResidentEvil.FusionFix: GPU sync pattern unavailable; workaround disabled.\n");
        }

        // sMain's maximum frame rate for the "Variable" frame rate option: 120 -> 1000
        if (settings.Bool("MAIN.UnlockFPS"))
        {
            auto pattern = hook::pattern("C7 40 ? ? ? ? ? EB ? 6A 01");
            if (PatternAvailable(pattern, "ResidentEvil.FusionFix: System"))
                WriteGameValue(pattern.get_first(3), 1000.0f);
            else
                OutputDebugStringA("Resident Evil: frame cap pattern unavailable; unlock disabled.\n");
        }
    }
}
