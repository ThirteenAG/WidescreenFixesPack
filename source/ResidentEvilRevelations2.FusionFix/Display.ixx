module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

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
            if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Display"))
            {
                injector::MakeNOP(pattern.get_first(0), 2);
                injector::MakeNOP(pattern.get_first(7), 2);
            }

            // The render resolution. The game's Aspect option fits a fixed aspect into the screen; the fix always uses
            // the screen's own (the option's DEFAULT). Screens narrower than 16:9 get a 16:9 image (the HUD and menus
            // are laid out for 16:9 and would be cut off otherwise).
            hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 8E").for_each_result([](hook::pattern_match match)
            {
                injector::MakeNOP(match.get<void>(0), 12, true);
                static std::vector<SafetyHookMid> hooks;
                hooks.emplace_back(safetyhook::create_mid(match.get<void>(0), [](SafetyHookContext& regs)
                {
                    nResX = regs.ecx;
                    nResY = regs.edx;
                    if ((float)nResX / (float)nResY < defaultAspectRatio)
                        regs.edx = nResY = 9 * nResX / 16;
                }));
            });

            hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 9E").for_each_result([](hook::pattern_match match)
            {
                injector::MakeNOP(match.get<void>(0), 12, true);
                static std::vector<SafetyHookMid> hooks;
                hooks.emplace_back(safetyhook::create_mid(match.get<void>(0), [](SafetyHookContext& regs)
                {
                    nResX = regs.ebx;
                    nResY = regs.edi;
                    if ((float)nResX / (float)nResY < defaultAspectRatio)
                        regs.edi = nResY = 9 * nResX / 16;
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
