module;

#include <stdafx.h>
#include <d3d9.h>
#include <atomic>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include "../Shared/ResidentEvil/IdleCursor.hpp"
#include "LEDEffects.h"

export module System;

// Start-up, window and quality-of-life options: no logos at start (MAIN.SkipIntro), the window of the game's
// windowed mode (MAIN.WindowMode), door animations skipped (MAIN.DoorSkip), the mouse cursor hidden when it doesn't
// move, and the GPU sync workaround (MAIN.DisableCreateQuery).

namespace
{
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

        // The logo scene (aLogo) waits (state 0), shows the division and copyright logos (state 1) and then changes
        // to the title scene (state 2). Starting it at state 2 goes to the title at once.
        if (settings.Bool("MAIN.SkipIntro"))
        {
            auto pattern = hook::pattern("56 8B F1 8B 46 30 83 E8 00 74 ? 48 74 ? 48 75 ? 8B 0D");
            if (PatternUnique(pattern, "ResidentEvil0.FusionFix: System"))
            {
                static SafetyHookMid LogoHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    static bool bSkipped = false;
                    if (!bSkipped && regs.eax == 0)
                    {
                        bSkipped = true;
                        regs.eax = 2;
                        *reinterpret_cast<uint32_t*>(regs.esi + 0x30) = 2;
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
            auto pattern = hook::pattern("80 BE ? ? ? ? ? 74 13 FF B6");
            if (pattern.size() == 1 && *pattern.get_first<uint32_t>(2) == 0x2684 && *pattern.get_first<uint32_t>(11) == 0x2680)
            {
                // The native cursor refresh stays intact. Hide only its shape, and restore its current handle after movement.
                static auto MouseCursorHook = safetyhook::create_mid(pattern.get_first(28), [](SafetyHookContext& regs)
                {
                    auto window = *reinterpret_cast<HWND*>(regs.esi + 0x12C);
                    auto region = *reinterpret_cast<HWND*>(regs.esi + 0x2664);
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
                            SetCursor(*reinterpret_cast<HCURSOR*>(regs.esi + 0x2680));
                            break;
                        default:
                            break;
                    }
                });
                if (!MouseCursorHook)
                    OutputDebugStringA("Resident Evil 0 Fusion Fix: cursor hook unavailable.\n");
            }
            else
            {
                OutputDebugStringA("Resident Evil 0 Fusion Fix: cursor signature unavailable or ambiguous.\n");
            }
        }

        if (settings.Bool("MAIN.DoorSkip"))
        {
            if (CheckPatterns("ResidentEvil0.FusionFix: door skipping", {
                "0F 84 CC 00 00 00 8B 47 3C",
                "F3 0F 11 47 ? 81 49 ? ? ? ? ? EB 07",
            }))
            {
                injector::MakeNOP(hook::get_pattern("0F 84 CC 00 00 00 8B 47 3C"), 6, true);
                auto pattern = hook::pattern("F3 0F 11 47 ? 81 49 ? ? ? ? ? EB 07");
                struct DoorkSkipHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(float*)(regs.edi + 0x2C) = -1.0f;
                    }
                }; injector::MakeInline<DoorkSkipHook>(pattern.get_first(0));
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
            auto pattern = hook::pattern("51 80 B9 ? ? ? ? ? 8B 91 ? ? ? ? 75 7B 8B 02 56");
            if (PatternAvailable(pattern, "ResidentEvil0.FusionFix: System"))
                injector::MakeJMP(pattern.get_first(), gpuCommandBufferSync, true);
            else
                OutputDebugStringA("ResidentEvil0.FusionFix: GPU sync pattern unavailable; workaround disabled.\n");
        }
    }
}
