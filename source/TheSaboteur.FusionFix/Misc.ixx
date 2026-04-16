module;

#include <stdafx.h>

export module Misc;

import ComVars;

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
            auto nFPSLimit = iniReader.ReadInteger("MAIN", "FPSLimit", 0);

            // Unlock resolutions with any refresh rate
            auto pattern = hook::pattern("74 ? 83 C6 01 83 C0 20");
            injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);

            if (bBorderlessWindowed)
            {
                IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                    std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                    std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                    std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                    std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                    std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                    std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
                );
            }

            if (nFPSLimit > 0)
            {
                static float f = 1.0f / static_cast<float>(nFPSLimit);
                auto pattern = hook::pattern("D9 05 ? ? ? ? A2 ? ? ? ? D9 1D ? ? ? ? E8");
                injector::WriteMemory(pattern.get_first(2), &f, true);
            }
        };
    }
} Misc;