module;

#include "stdafx.h"
#include "../Shared/CheckedPattern.hpp"

export module Camera;

import Build;
import Settings;
import ClassicOrbit;
import Input;

class CameraModule
{
public:
    CameraModule()
    {
        WFP::onInitEvent() += []()
        {
            if (Settings.disableFirstPersonAimForRifles)
            {
                if (auto address = wfp::patterns::First("83 F8 02 40 0F 96 C6 E8 ? ? ? ? 48 8D 0D ? ? ? ? 44 0F B6 F0 E8"))
                {
                    static auto rifleHook = safetyhook::create_mid(address, [](SafetyHookContext& c)
                    {
                        if (c.rdi == 5 || c.rdi == 6)
                            c.rax = INT_MAX;
                    });
                }
            }
            if (!Settings.improveCameraPC)
                return;
            if (auto aim = wfp::patterns::First("F3 0F 10 53 08 0F 28 C2 F3 0F 59 D3 F3 41 0F 59 C0 F3 0F 59 15 ? ? ?"))
            {
                static auto aimHook = safetyhook::create_mid(aim, [](SafetyHookContext& c)
                {
                    const auto camera = reinterpret_cast<const uint8_t*>(c.rbx);
                    const auto mode = *reinterpret_cast<const uint16_t*>(camera + 38);
                    const auto fov = *reinterpret_cast<const float*>(camera + 8);
                    if (IsMouseCameraActive() && (mode == 34 || mode == 44) && fov > 0.0f)
                    {
                        c.xmm8.f32[0] *= 70.0f / fov;
                        c.xmm3.f32[0] *= (4.0f / 3.0f) * (70.0f / fov);
                    }
                });
            }
            // CCam::Process_Syphon is the normal third-person weapon aim,
            // separate from the rifle first-person helper above. Its native
            // pad gates, aim target, angle limits and collision stay active.
            if (auto aim = wfp::patterns::First("F3 41 0F 58 45 00 F3 0F 10 1D ? ? ? ? 41 BB 64 00 00 00 F3 41 0F 58"))
            {
                static auto syphonAimHook = safetyhook::create_mid(aim, [](SafetyHookContext& c)
                {
                    const auto camera = reinterpret_cast<const uint8_t*>(c.r13);
                    const auto fov = *reinterpret_cast<const float*>(camera + 8);
                    if (IsMouseCameraActive() && std::isfinite(fov) && fov > 0.0f)
                    {
                        const auto reference = 70.0f / fov;
                        c.xmm6.f32[0] *= 3.125f * reference;
                        c.xmm0.f32[0] *= (25.0f / 6.0f) * reference;
                    }
                });
            }
            const auto requiredSite0 = wfp::patterns::First("F3 0F 58 C6 F3 0F 58 05 ? ? ? ? F3 0F 11 43 10 8B 8F C0 04 00 00 B8");
            const auto requiredSite1 = wfp::patterns::First("F3 0F 11 1D ? ? ? ? F3 0F 10 0D ? ? ? ? 0F 28 C3 0F 28 7C 24 20");
            const auto requiredSite2 = wfp::patterns::First("F3 0F 59 35 ? ? ? ? EB ? 48 8B 05 ? ? ? ? F3 0F 10 70 10 F3 0F");
            const auto requiredSite3 = wfp::patterns::First("F3 0F 59 05 ? ? ? ? F3 0F 58 D8 0F 54 05 ? ? ? ? 0F 2F 05 ? ?");
            const auto requiredSite4 = wfp::patterns::First("F3 0F 11 0D ? ? ? ? 41 0F 2F C9 76 ? F3 44 0F 10 05 ? ? ? ? 41");
            const auto requiredSite5 = wfp::patterns::First("E9 ? ? ? ? 48 8B 15 ? ? ? ? 48 8B 0D ? ? ? ? F3 0F 10 72 14");
            const auto requiredSite6 = wfp::patterns::First("E9 ? ? ? ? 8B 15 ? ? ? ? 48 63 C2 48 8B 0C C3 48 85 C9 0F 84 ?");
            const auto requiredSite7 = wfp::patterns::First("F3 0F 58 F8 F3 0F 10 43 38 F3 41 0F 5C C2 F3 44 0F 10 97 E8 00 00 00 48");
            const auto requiredSite8 = wfp::patterns::First("F3 0F 10 05 ? ? ? ? 41 0F 28 CB 0F 57 0D ? ? ? ? F3 0F 58 C6 F3");
            const auto requiredSite9 = wfp::patterns::First("F3 0F 59 F0 EB ? 48 63 05 ? ? ? ? 48 8B 0C C7 48 85 C9 74 ? 48 8B");
            const auto requiredSite10 = wfp::patterns::First("F3 0F 58 1D ? ? ? ? F3 0F 11 1D ? ? ? ? E9 ? ? ? ? 8B 15 ?");
            const auto requiredSite11 = wfp::patterns::First("0F 28 C6 41 0F 54 C1 0F 2F 05 ? ? ? ? 0F 86 ? ? ? ? C7 05 ? ? ? ? 00 00 00 3F E9 ?");
            const auto requiredSite12 = wfp::patterns::First<uint8_t>("0F B6 05 ? ? ? ? 48 8B 7C 24 58 48 8B 74 24 50 F3 0F 59 D2 F3 0F 59", 3);
            const auto requiredSite13 = wfp::patterns::First<uint8_t>("C7 05 ? ? ? ? 00 00 00 00 0F C6 C0 01 F3 0F 59 05 ? ? ? ? 48 C7", 2);
            const auto requiredSite14 = wfp::patterns::First<uint8_t>("F3 0F 10 05 ? ? ? ? F3 41 0F 5C C4 F3 44 0F 10 25 ? ? ? ? F3 44", 4);
            const auto requiredSite15 = wfp::patterns::First<uint8_t>("F3 44 0F 10 05 ? ? ? ? 48 8D 0C 80 49 8B 41 10 41 0F 28 F0 41 0F 28", 5);
            if (!requiredSite0 || !requiredSite1 || !requiredSite2 || !requiredSite3 || !requiredSite4 || !requiredSite5 || !requiredSite6 || !requiredSite7 || !requiredSite8 || !requiredSite9 || !requiredSite10 || !requiredSite11 || !requiredSite12 || !requiredSite13 || !requiredSite14 || !requiredSite15)
                return;
            InstallClassicOrbit({{requiredSite0,
                requiredSite1,
                requiredSite2,
                requiredSite3,
                requiredSite4,
                requiredSite5,
                requiredSite6,
                requiredSite7,
                requiredSite8,
                requiredSite9,
                requiredSite10,
                requiredSite11},
                reinterpret_cast<const uint8_t*>((injector::ReadRelativeOffset(requiredSite12).as_int())),
                reinterpret_cast<const float*>((injector::ReadRelativeOffset(requiredSite13).as_int() + 4)),
                reinterpret_cast<float*>((injector::ReadRelativeOffset(requiredSite14).as_int())),
                reinterpret_cast<const float*>((injector::ReadRelativeOffset(requiredSite15).as_int()))});
        };
    }
} CameraModuleInstance;
