module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <LEDEffects.h>
#include <atomic>

export module LED;

import ComVars;

class LED
{
public:
    LED()
    {
        WFP::onInitEvent() += []()
        {
            auto bLightSyncRGB = FixSettings().Bool("MAIN.LightSyncRGB");

            if (bLightSyncRGB)
            {
                LEDEffects::Inject([]()
                {
                    static float fPlayerVisibility = gVisibility.load(std::memory_order_relaxed);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    auto gVisCmp = static_cast<float>(static_cast<int>(gVisibility.load(std::memory_order_relaxed) * 10.0f)) / 10.0f;
                    auto fPlVisCmp = static_cast<float>(static_cast<int>(fPlayerVisibility * 10.0f)) / 10.0f;

                    if (fPlVisCmp > gVisCmp)
                        fPlayerVisibility -= 0.05f;
                    else if (fPlVisCmp < gVisCmp)
                        fPlayerVisibility += 0.05f;

                    constexpr auto minBrightness = 0.3f;
                    constexpr auto maxBrightness = 1.0f;

                    fPlayerVisibility = std::clamp(fPlayerVisibility, minBrightness, maxBrightness);

                    auto [R, G, B] = LEDEffects::RGBtoPercent(255, 39, 26, gBlacklistIndicators.load(std::memory_order_relaxed) ? fPlayerVisibility : ((maxBrightness + minBrightness) - fPlayerVisibility));
                    LEDEffects::SetLighting(R, G, B, false, false, false);
                });
            }
        };
    }
} LED;
