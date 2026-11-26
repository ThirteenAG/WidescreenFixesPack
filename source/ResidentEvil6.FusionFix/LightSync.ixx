module;

#include <stdafx.h>
#include <array>
#include <mutex>
#include <optional>
#include "FixSettings.hpp"
#include "LEDEffects.h"

export module LightSync;

namespace
{
    struct Health
    {
        int left;
        int right;
        size_t campaign;
    };

    SafetyHookInline updateHook, resetHook;
    std::mutex healthMutex;
    std::optional<Health> health;

    void Publish(std::optional<Health> value)
    {
        const std::lock_guard lock(healthMutex);
        health = value;
    }

    std::optional<Health> Sample(const uint8_t* manager)
    {
        const auto slots = reinterpret_cast<const uint8_t* const*>(manager + 0x24);
        for (size_t campaign = 0; campaign < 4; ++campaign)
        {
            const auto* first = slots[2 * campaign];
            const auto* second = slots[2 * campaign + 1];
            if (first && (second || campaign == 3))
            {
                return Health{
                    *reinterpret_cast<const int16_t*>(first + 0xF10),
                    second ? *reinterpret_cast<const int16_t*>(second + 0xF10) : -1,
                    campaign
                };
            }
        }
        return std::nullopt;
    }

    float* __fastcall Update(const uint8_t* manager, void*)
    {
        auto* result = updateHook.unsafe_thiscall<float*>(manager);
        if (LEDEffects::bLogiLedInitialized)
            Publish(Sample(manager));
        return result;
    }

    int __fastcall Reset(void* manager, void*)
    {
        const auto result = resetHook.unsafe_thiscall<int>(manager);
        Publish(std::nullopt);
        return result;
    }

    void UpdateSide(int value, bool left, const std::array<int, 3>& fine, bool hasPlayer = true)
    {
        const bool dead = hasPlayer && value <= 0;
        const auto [r, g, b] = value <= 150 ? std::array{ 0x89, 0x13, 0x1D } : fine;
        const auto [red, green, blue] = LEDEffects::RGBtoPercent(r, g, b, 0.5f);
        if (left)
            LEDEffects::SetLightingLeftSide(red, green, blue, hasPlayer && !dead, dead);
        else
            LEDEffects::SetLightingRightSide(red, green, blue, hasPlayer && !dead, dead);

        if (hasPlayer)
        {
            const auto [pulseRed, pulseGreen, pulseBlue] = LEDEffects::RGBtoPercent(r, g, b);
            const auto draw = left ? LEDEffects::DrawCardiogram : LEDEffects::DrawCardiogramNumpad;
            draw(pulseRed, pulseGreen, pulseBlue, 0, 0, 0, dead);
        }
    }

    void UpdateLighting()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::optional<Health> snapshot;
        {
            const std::lock_guard lock(healthMutex);
            snapshot = health;
        }

        if (snapshot)
        {
            constexpr std::array fine{
                std::array{ 0x84, 0xDE, 0xFF },
                std::array{ 0xA4, 0xB8, 0x39 },
                std::array{ 0xC1, 0xD5, 0x42 },
                std::array{ 0x84, 0xDE, 0xFF }
            };
            const auto& color = fine[snapshot->campaign];
            UpdateSide(snapshot->left, true, color);
            if (snapshot->right >= 0)
                UpdateSide(snapshot->right, false, color);
            else
                UpdateSide(snapshot->left, false, color, false);
        }
        else
        {
            LogiLedStopEffects();
            LEDEffects::SetLighting(31, 25, 70);
        }
    }
}

class LightSync
{
public:
    LightSync()
    {
        WFP::onInitEvent() += []()
        {
            if (!FixSettings().Bool("MAIN.LightSyncRGB"))
                return;

            auto update = hook::pattern("83 EC 08 53 56 57 8B F9 8B 87 B8 31 00 00 33 DB 3B C3 74 ? 8B 40 10 83 E0 07");
            auto reset = hook::pattern("53 56 57 8B F9 8D 77 24 BB 08 00 00 00 8D 49 00 8B 06 50 8B CF E8 ? ? ? ? 83 C6 04 4B 75 ? 8B CF 5F 5E 5B E9");
            updateHook = safetyhook::create_inline(update.get_first(), Update);
            resetHook = safetyhook::create_inline(reset.get_first(), Reset);
            LEDEffects::Inject(UpdateLighting);
        };
    }
} LightSync;
