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
            if (!Settings.restoreOriginalCamera)
                return;
            // Original SA 1.0 camera tables, compared with the latest PC DE.
            // The two ped profiles and the remaining car parameters match;
            // patch only the differing values, including all three car zooms.
            const auto carHeight = reinterpret_cast<void*>(injector::ReadRelativeOffset(hook::pattern("F3 0F 59 35 ? ? ? ? F3 44 0F 10 44 24 38 F3 44 0F 10 5C 24 34 F3 0F").get_first<uint8_t>(4)).as_int());
            const auto bikeDistance = reinterpret_cast<void*>(injector::ReadRelativeOffset(hook::pattern("F3 0F 59 35 ? ? ? ? F3 44 0F 10 44 24 38 F3 44 0F 10 5C 24 34 F3 0F").get_first<uint8_t>(4)).as_int() + 0x40);
            const auto nearPitch = reinterpret_cast<void*>(injector::ReadRelativeOffset(hook::pattern("F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 5C C6 48 8B 01 F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 5C C6 48 8B 01 F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11").get_first<uint8_t>(4)).as_int());
            const auto normalPitch = reinterpret_cast<void*>(injector::ReadRelativeOffset(hook::pattern("F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 5C C6 48 8B 01 F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ?").get_first<uint8_t>(4)).as_int());
            const auto farPitch = reinterpret_cast<void*>(injector::ReadRelativeOffset(hook::pattern("F3 0F 11 05 ? ? ? ? FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11 05 ? ? ? ? 48 8B 01 FF 90 98 00 00 00 48 8B 0D ? ? ? ? F3 0F 11").get_first<uint8_t>(4)).as_int());
            if (!carHeight || !bikeDistance || !nearPitch || !normalPitch || !farPitch)
                return;
            *static_cast<float*>(carHeight) = 1.3f;
            *static_cast<float*>(bikeDistance) = 1.0f;
            constexpr std::array nearValues{0.08f,0.08f,0.15f,0.08f,0.08f};
            constexpr std::array normal{0.07f,0.08f,0.30f,0.08f,0.08f};
            constexpr std::array farValues{0.055f,0.05f,0.15f,0.06f,0.08f};
            std::copy(nearValues.begin(), nearValues.end(), static_cast<float*>(nearPitch));
            std::copy(normal.begin(), normal.end(), static_cast<float*>(normalPitch));
            std::copy(farValues.begin(), farValues.end(), static_cast<float*>(farPitch));
        };
    }
} CameraProfilesModuleInstance;
