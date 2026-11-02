module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <LEDEffects.h>
#include <array>
#include <atomic>

export module LED;

struct GogglesColor
{
    std::string_view name;
    int red;
    int green;
    int blue;
};

constexpr std::array gogglesColors = {
    GogglesColor{ "LightGreen", 17, 138, 41 },
    GogglesColor{ "LightPaleOrange", 158, 118, 29 },
    GogglesColor{ "LightRed", 163, 18, 15 },
    GogglesColor{ "LightBlue", 24, 70, 213 },
    GogglesColor{ "LightPink", 163, 33, 116 },
    GogglesColor{ "LightPurple", 111, 34, 210 },
    GogglesColor{ "LightWhite", 162, 182, 208 },
    GogglesColor{ "LightGold", 145, 172, 30 },
    GogglesColor{ "LightOrange", 161, 89, 12 },
    GogglesColor{ "LightPaleBlue", 23, 159, 205 },
};
std::atomic<size_t> currentGogglesColor{ 0 };

int __cdecl appStricmpHook(const char* String1, const char* String2)
{
    auto ret = _stricmp(String1, String2);
    size_t color = 0;
    if (!ret)
    {
        for (size_t i = 0; i < gogglesColors.size(); ++i)
        {
            if (gogglesColors[i].name == String1)
            {
                color = i;
                break;
            }
        }
    }
    currentGogglesColor.store(color, std::memory_order_relaxed);
    return ret;
}

class LED
{
public:
    LED()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            auto bLightSyncRGB = settings.Int("MAIN.LightSyncRGB");

            if (bLightSyncRGB)
            {
                // LoadoutCommon::FsBaseLoadout::GetSuitLightColor
                auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 85 C0 0F 84 ? ? ? ? 46 83 FE 0A");
                injector::MakeCALL(pattern.get_first(0), appStricmpHook, true);

                LEDEffects::Inject([]()
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    const auto& color = gogglesColors[currentGogglesColor.load(std::memory_order_relaxed)];
                    auto [R, G, B] = LEDEffects::RGBtoPercent(color.red, color.green, color.blue, 1.0f);
                    LEDEffects::SetLighting(R, G, B, false, false, false);
                });
            }
        };
    }
} LED;
