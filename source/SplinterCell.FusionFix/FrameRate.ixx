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
            // MAIN.FPSLimit: the engine's shortest frame time (5 ms, 200 FPS, in the original)
            auto pattern = hook::pattern("8B 15 ? ? ? ? 89 55 D4 A1 ? ? ? ? 89 45 F8");
            {
                SplinterCellFrameLimit::ReadSetting();
                SplinterCellFrameLimit::Update(hGameWindow);
                FixSettings().OnChange("MAIN.FPSLimit", SplinterCellFrameLimit::ReadSetting);
                WFP::onGameProcessEvent() += []()
                {
                    SplinterCellFrameLimit::Update(hGameWindow);
                };
                injector::WriteMemory<float*>(pattern.get_first<float*>(2), &SplinterCellFrameLimit::minFrameTime, true);
            }
        };
    }
} FrameRate;
