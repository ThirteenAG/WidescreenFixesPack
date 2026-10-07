module;

#include "stdafx.h"

export module CameraProfiles;

import Build;
import Settings;

namespace
{
    int(*getVehicleAppearance)(uintptr_t) = nullptr;
}

class CameraProfilesModule
{
public:
    CameraProfilesModule()
    {
        WFP::onInitEvent() += []()
        {
            if (!Settings.restoreOriginalCamera)
                return;
            const auto appearance = hook::pattern("48 8B 81 E0 01 00 00 8B 88 CC 00 00 00 81 E1 00 00 0F 00 74 ? 81 F9 00").get_first();
            const auto carZoom = hook::pattern("F3 0F 11 A3 EC 00 00 00 75 ? 85 C9 75 ? B8 10 00 00 00 66 89 05 ? ?").get_first();
            if (appearance && carZoom)
            {
                getVehicleAppearance = reinterpret_cast<decltype(getVehicleAppearance)>(appearance);
                static auto carDistanceHook = safetyhook::create_mid(carZoom, [](SafetyHookContext& c)
                {
                    if (c.rcx < 1 || c.rcx > 3)
                        return;
                    const auto vehicle = *reinterpret_cast<const uintptr_t*>(c.rbx + 1736);
                    if (!vehicle || (*reinterpret_cast<const uint32_t*>(vehicle + 108) & 7) != 2)
                        return;
                    // revc's original zoom tables: car, bike, heli, plane, boat.
                    constexpr std::array<std::array<float, 5>, 3> distances{{
                        {-0.6f, 0.05f, -3.2f, 0.05f, -2.41f},
                        {1.9f, 1.4f, 0.65f, 1.9f, 6.49f},
                        {15.9f, 15.9f, 15.9f, 15.9f, 25.25f}}};
                    const auto appearance = getVehicleAppearance(vehicle);
                    constexpr std::array indices{-1, 0, 1, 2, 4, 3};
                    if (appearance > 0 && appearance < indices.size())
                        c.xmm4.f32[0] = distances[c.rcx - 1][indices[appearance]];
                });
            }
            if (auto address = hook::pattern("0F 2E 9B E4 00 00 00 F3 0F 11 9B D4 00 00 00 74 ? 8B 05 ? ? ? ? 0F").get_first())
            {
                static auto distanceHook = safetyhook::create_mid(address, [](SafetyHookContext& c)
                {
                    // revc Process_FollowPedWithMouse: 2 + PedZoomValue.
                    // VC's collision placement uses this distance directly.
                    constexpr std::array distances{2.25f, 3.5f, 4.9f};
                    const auto zoom = *reinterpret_cast<const int*>(c.rbx + 208);
                    if (zoom >= 0 && zoom < distances.size())
                        c.xmm3.f32[0] = distances[zoom];
                });
            }
        };
    }
} CameraProfilesModuleInstance;
