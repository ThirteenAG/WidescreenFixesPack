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
                    //std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                    //std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                    std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                    std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook),
                    std::forward_as_tuple("ShowCursor", WindowedModeWrapper::ShowCursor_Hook)
                );

                auto pattern = hook::pattern("89 5D 3C 89 5D 18 89 5D 44"); //0x74FD5C
                struct WindowedMode
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *(uint32_t*)(regs.ebp + 0x3C) = 1;
                        *(uint32_t*)(regs.ebp + 0x18) = regs.ebx;
                    }
                }; injector::MakeInline<WindowedMode>(pattern.get_first(0), pattern.get_first(6));
            }
        };
    }
} Windowed;