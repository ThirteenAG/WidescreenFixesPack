module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SplinterCell/FrameLimit.hpp"

export module FrameRate;

import ComVars;

class FrameRate
{
public:
    FrameRate()
    {
        WFP::onInitEvent() += []()
        {
            // MAIN.FPSLimit: the engine's shortest frame time (5 ms in the retail release, 16.67 ms in the current one)
            auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 45 F0 F3 0F 10 05 ? ? ? ? F3 0F 11 45 EC");
            int offset = 4;
            if (pattern.empty()) // retail
            {
                pattern = hook::pattern("8B 0D ? ? ? ? 89 4D C8 8B 15 ? ? ? ? 89 55 E0");
                offset = 2;
            }
            {
                SplinterCellFrameLimit::ReadSetting();
                SplinterCellFrameLimit::Update(hGameWindow);
                FixSettings().OnChange("MAIN.FPSLimit", SplinterCellFrameLimit::ReadSetting);
                WFP::onGameProcessEvent() += []()
                {
                    SplinterCellFrameLimit::Update(hGameWindow);
                };
                injector::WriteMemory<float*>(pattern.get_first<float*>(offset), &SplinterCellFrameLimit::minFrameTime, true);
            }
        };
    }
} FrameRate;
