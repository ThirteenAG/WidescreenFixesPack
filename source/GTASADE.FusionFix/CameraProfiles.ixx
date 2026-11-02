module;

#include "stdafx.h"

export module CameraProfiles;

import Build;
import Settings;

class CameraProfilesModule
{
public:
    CameraProfilesModule()
    {
        WFP::onInitEvent() += []()
        {
            // Original SA 1.0 camera tables, compared with the latest PC DE.
            // The two ped profiles and the remaining car parameters match;
            // patch only the differing values, including all three car zooms.

            const auto relative = [](const char* signature, size_t offset, ptrdiff_t trailing = 0) -> uintptr_t
            {
                return injector::ReadRelativeOffset(hook::pattern(signature).get_first<uint8_t>(offset)).as_int() + trailing;
            };
            const auto carHeight = reinterpret_cast<void*>(relative("F3 0F 59 35 ? ? ? ? F3 44 0F 10 44 24 38 F3 44 0F 10 5C 24 34 F3 0F", 4));
            const auto bikeDistance = reinterpret_cast<void*>(relative("F3 0F 59 35 ? ? ? ? F3 44 0F 10 44 24 38 F3 44 0F 10 5C 24 34 F3 0F", 4, 0x40));
            const auto nearPitch = reinterpret_cast<void*>(relative("F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 5C C6 48 8B 01 F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 5C C6 48 8B 01 F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11", 4));
            const auto normalPitch = reinterpret_cast<void*>(relative("F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 5C C6 48 8B 01 F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ?", 4));
            const auto farPitch = reinterpret_cast<void*>(relative("F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11", 4));
            // The DE's own values are kept, so turning the setting off restores them
            struct Value
            {
                float* address;
                float original, classic;
            };
            static std::vector<Value> values;
            const auto add = [](void* address, float classic)
            {
                values.push_back({ static_cast<float*>(address), *static_cast<float*>(address), classic });
            };
            add(carHeight, 1.3f);
            add(bikeDistance, 1.0f);
            constexpr std::array nearValues{0.08f,0.08f,0.15f,0.08f,0.08f};
            constexpr std::array normal{0.07f,0.08f,0.30f,0.08f,0.08f};
            constexpr std::array farValues{0.055f,0.05f,0.15f,0.06f,0.08f};
            for (size_t i = 0; i < nearValues.size(); ++i)
                add(static_cast<float*>(nearPitch) + i, nearValues[i]);
            for (size_t i = 0; i < normal.size(); ++i)
                add(static_cast<float*>(normalPitch) + i, normal[i]);
            for (size_t i = 0; i < farValues.size(); ++i)
                add(static_cast<float*>(farPitch) + i, farValues[i]);
            static const auto apply = []()
            {
                for (const auto& value : values)
                    injector::WriteMemory<float>(value.address, Settings.restoreOriginalCamera ? value.classic : value.original, true);
            };
            apply();
            onSettingsChanged() += apply;
        };
    }
} CameraProfilesModuleInstance;
