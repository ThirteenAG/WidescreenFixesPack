module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Display;

import ComVars;

// The resolution list, the render resolution and the window.

class Display
{
public:
    Display()
    {
        WFP::onInitEvent() += []()
        {
            // Per monitor DPI awareness, so the game gets real pixels on scaled desktops
            using SetContext = BOOL(WINAPI*)(HANDLE);
            if (auto set = reinterpret_cast<SetContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext")))
                set(reinterpret_cast<HANDLE>(-4));
            else
                SetProcessDPIAware();

            // The resolution list: the game drops modes above 4096x2160
            auto pattern = hook::pattern("77 5F 3D 70 08 00 00 77 58");
            injector::MakeNOP(pattern.get_first(0), 2);
            injector::MakeNOP(pattern.get_first(7), 2);

            // The render resolution. The game's Aspect option fits a fixed aspect into the screen; the fix always uses
            // the screen's own (the option's DEFAULT). Screens narrower than 16:9 get a 16:9 image (the HUD and menus
            // are laid out for 16:9 and would be cut off otherwise).
            // get_first() stops scanning after one match; use a fresh pattern for the
            // full scan so every resolution path receives the same correction.
            hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 8E B8 00 00 00").get_first();
            auto resolutionECX = hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 8E");
            resolutionECX.for_each_result([](hook::pattern_match match)
            {
                injector::MakeNOP(match.get<void>(0), 12, true);
                static std::vector<SafetyHookMid> hooks;
                hooks.emplace_back(safetyhook::create_mid(match.get<void>(0), [](SafetyHookContext& regs)
                {
                    if (regs.edx && (float)regs.ecx / (float)regs.edx < defaultAspectRatio)
                        regs.edx = 9 * regs.ecx / 16;
                }));
            });

            hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 9E B8 00 00 00").get_first();
            auto resolutionEBX = hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 9E");
            resolutionEBX.for_each_result([](hook::pattern_match match)
            {
                injector::MakeNOP(match.get<void>(0), 12, true);
                static std::vector<SafetyHookMid> hooks;
                hooks.emplace_back(safetyhook::create_mid(match.get<void>(0), [](SafetyHookContext& regs)
                {
                    if (regs.edi && (float)regs.ebx / (float)regs.edi < defaultAspectRatio)
                        regs.edi = 9 * regs.ebx / 16;
                }));
            });

            // MAIN.WindowMode: the window when the game's own Full Screen option is off; 1 = with a frame, 2 = borderless.
            // A framed window as big as the screen is placed with its frame outside the screen; no rounded corners.
            WindowedModeWrapper::bBorderlessWindowed = FixSettings().Int("MAIN.WindowMode") != 1;
            IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
            );
        };
    }
} Display;
