module;

#include <stdafx.h>

export module Windowed;

import ComVars;

class Windowed
{
public:
    Windowed()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            int32_t nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);

            if (nWindowedMode)
            {
                switch (nWindowedMode)
                {
                    case 5:
                        WindowedModeWrapper::bStretchWindow = true;
                        break;
                    case 4:
                        WindowedModeWrapper::bScaleWindow = true;
                        break;
                    case 3:
                        WindowedModeWrapper::bEnableWindowResize = true;
                    case 2:
                        WindowedModeWrapper::bBorderlessWindowed = false;
                        break;
                    default:
                        break;
                }

                dwWindowedMode = 1;

                IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                    std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                    std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                    std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                    std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                    std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                    std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                    std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)//,
                    //std::forward_as_tuple("ShowCursor", WindowedModeWrapper::ShowCursor_Hook)
                );

                auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 84 C0 75 ? A1 ? ? ? ? 85 C0 74 ? 6A");
                static auto ShowCursorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    WindowedModeWrapper::ShowCursor_Hook(FALSE);
                });
            }
        };
    }
} Windowed;