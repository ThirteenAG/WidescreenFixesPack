module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Windowed;

import ComVars;

class Windowed
{
public:
    Windowed()
    {
        WFP::onInitEvent() += []()
        {
            // 0 full screen, 1 windowed, 2 borderless at the monitor's size, 3 resizable window, 4 borderless
            const auto mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 4);

            WindowedModeWrapper::EnableDpiAwareness();

            if (mode != 0)
            {
                WindowedModeWrapper::bBorderlessWindowed = mode == 2 || mode == 4;
                WindowedModeWrapper::bScaleWindow = mode == 2;
                WindowedModeWrapper::bEnableWindowResize = mode == 3;

                dwWindowedMode = 1;

                auto hooks = IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                    std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                    std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                    std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                    std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                    std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
                );

                // These five ANSI import slots are present in each supported executable.
                if (hooks.size() != 5)
                    std::abort();
                for (auto& [name, original] : hooks)
                {
                    if (!original.get())
                        std::abort();
                }

                // The cursor is hidden in the message loop's focus check (Resolution's focus hook)
                auto pattern = hook::pattern("8B 0D ? ? ? ? 68 ? ? ? ? 6A ? 51 FF 15");
                {
                    if (*pattern.get_first<HWND*>(2) != hWnd.get_ptr())
                        injector::WriteMemory(pattern.get_first(2), hWnd.get_ptr(), true);
                }
            }
        };
    }
} Windowed;
