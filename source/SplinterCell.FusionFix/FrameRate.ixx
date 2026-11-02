module;

#include <stdafx.h>
#include "FixSettings.hpp"

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
                static float fMinFrameTime = 0.005f;
                static auto ApplyFPSLimit = []()
                {
                    int limit = FixSettings().Int("MAIN.FPSLimit");
                    if (limit < 0)
                        limit = static_cast<int>(GetDesktopRefreshRate());
                    fMinFrameTime = limit > 0 ? 1.0f / static_cast<float>(std::clamp(limit, 30, 1000)) : 0.0f;
                };
                ApplyFPSLimit();
                FixSettings().OnChange("MAIN.FPSLimit", ApplyFPSLimit);
                injector::WriteMemory<float*>(pattern.get_first<float*>(2), &fMinFrameTime, true);
            }
        };
    }
} FrameRate;
