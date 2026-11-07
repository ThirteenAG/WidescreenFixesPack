module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <LEDEffects.h>
#include <atomic>

export module LED;

import ComVars;

void AmmoInClip()
{
    static std::vector<LogiLed::KeyName> keys = {
        LogiLed::KeyName::F1, LogiLed::KeyName::F2, LogiLed::KeyName::F3,
        LogiLed::KeyName::F4, LogiLed::KeyName::F5, LogiLed::KeyName::F6,
        LogiLed::KeyName::F7, LogiLed::KeyName::F8, LogiLed::KeyName::F9,
        LogiLed::KeyName::F10, LogiLed::KeyName::F11, LogiLed::KeyName::F12,
        LogiLed::KeyName::PRINT_SCREEN, LogiLed::KeyName::SCROLL_LOCK,
        LogiLed::KeyName::PAUSE_BREAK,
    };

    const auto snapshot = ammoSnapshot.load(std::memory_order_relaxed);
    const auto ammo = static_cast<uint32_t>(snapshot);
    const auto capacity = static_cast<uint32_t>(snapshot >> 32);
    static uint64_t oldSnapshot = UINT64_MAX;
    if (snapshot != oldSnapshot)
    {
        const float ammoInClipPercent = capacity ? (static_cast<float>(ammo) / static_cast<float>(capacity)) * 100.0f : 0.0f;

        for (size_t i = 0; i < keys.size(); i++)
        {
            auto indexInPercent = ((float)i / (float)keys.size()) * 100.0f;
            if (ammoInClipPercent > indexInPercent)
                LogiLedSetLightingForKeyWithKeyName(keys[i], 100, 100, 100);
            else
                LogiLedSetLightingForKeyWithKeyName(keys[i], 10, 10, 10);
        }
    }
    oldSnapshot = snapshot;
}

class LED
{
public:
    LED()
    {
        WFP::onInitEvent() += []()
        {
            if (FixSettings().Int("MAIN.LightSyncRGB"))
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

                    fPlayerVisibility = std::clamp(fPlayerVisibility, 0.0f, 1.0f);

                    uint32_t rgb = 0x01FF01;
                    if (gColor >= 1 && gColor <= SuitLightColors.size())
                        rgb = SuitLightColors[gColor - 1];
                    auto [R, G, B] = LEDEffects::RGBtoPercent((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, fPlayerVisibility);

                    LEDEffects::SetLighting(R, G, B, false, false, true);
                    AmmoInClip();
                });
            }
        };
    }
} LED;
