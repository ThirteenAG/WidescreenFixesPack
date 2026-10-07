module;

#include "stdafx.h"

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
                if (auto address = hook::pattern("48 BD 80 01 00 00 04 60 00 00 4C 89 A4 24 B8 00 00 00 4C 89 BC 24 A0 00").get_first())
                {
                    static auto rifleHook = safetyhook::create_mid(address, [](SafetyHookContext& c)
                    {
                        if (c.rdi && (c.rsi == 26 || c.rsi == 27 || c.rsi == 32))
                            c.rdi = 0;
                    });
                }
            }
            if (!Settings.improveCameraPC)
                return;
            if (auto aim = hook::pattern("F3 0F 59 1D ? ? ? ? F3 0F 59 0D ? ? ? ? F3 0F 58 1F 0F 28 74 24").get_first())
            {
                static auto aimHook = safetyhook::create_mid(aim, [](SafetyHookContext& c)
                {
                    const auto camera = reinterpret_cast<const uint8_t*>(c.rdi);
                    const auto mode = *reinterpret_cast<const uint16_t*>(camera + 38);
                    const auto fov = *reinterpret_cast<const float*>(camera + 8);
                    if (IsMouseCameraActive() && (mode == 34 || mode == 45) && fov > 0.0f)
                    {
                        // Aim axes already contain one FOV factor; remove
                        // the helper's second factor and its distinct gain.
                        const auto reference = 0.875f * (70.0f / fov) / 1.2f;
                        c.xmm3.f32[0] = c.xmm0.f32[0] * (25.0f / 6.0f) * reference;
                        c.xmm1.f32[0] = c.xmm6.f32[0] * 3.125f * reference;
                    }
                });
            }
            // CCam::Process_Syphon is VC's normal third-person weapon aim.
            // Its pad axes contain FOV and a 25% lower vertical gain already.
            // Normalize after native wheel/control gates and before integration.
            if (auto aim = hook::pattern("F3 41 0F 10 4D 00 41 BB 50 00 00 00 F3 41 0F 58 75 10 F3 0F 10 3D ? ?").get_first())
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
            InstallClassicOrbit({{hook::pattern("F3 0F 58 C6 F3 0F 58 05 ? ? ? ? F3 0F 11 43 10 48 8B D7 48 8B CB E8").get_first(),
                hook::pattern("F3 0F 11 1D ? ? ? ? F3 0F 10 0D ? ? ? ? 0F 28 C3 0F 28 7C 24 20").get_first(),
                hook::pattern("F3 0F 59 35 ? ? ? ? EB ? 48 8B 05 ? ? ? ? F3 0F 10 70 08 F3 0F").get_first(),
                hook::pattern("F3 0F 59 05 ? ? ? ? F3 0F 58 D8 0F 54 05 ? ? ? ? 0F 2F 05 ? ?").get_first(),
                hook::pattern("F3 0F 11 0D ? ? ? ? 41 0F 2F CA 76 ? F3 44 0F 10 05 ? ? ? ? 41").get_first(),
                hook::pattern("E9 ? ? ? ? 48 8B 15 ? ? ? ? 48 8B 0D ? ? ? ? F3 0F 10 72 0C").get_first(),
                hook::pattern("E9 ? ? ? ? 8B 15 ? ? ? ? 48 63 C2 48 8B 0C C3 48 85 C9 0F 84 ?").get_first(),
                hook::pattern("F3 0F 58 F8 F3 0F 10 43 38 F3 41 0F 5C C3 F3 44 0F 10 9E 00 01 00 00 48").get_first(),
                hook::pattern("F3 0F 10 05 ? ? ? ? 41 0F 28 E8 0F 57 2D ? ? ? ? F3 0F 58 C6 F3").get_first(),
                hook::pattern("F3 0F 59 F0 EB ? 48 63 05 ? ? ? ? 48 8B 0C C6 48 85 C9 74 ? 48 8B").get_first(),
                hook::pattern("F3 0F 58 1D ? ? ? ? F3 0F 11 1D ? ? ? ? E9 ? ? ? ? 8B 15 ?").get_first(),
                hook::pattern("0F 28 C6 41 0F 54 C1 0F 2F 05 ? ? ? ? 0F 86 ? ? ? ? C7 05 ? ? ? ? 00 00 00 3F E9 ?").get_first()},
                reinterpret_cast<const uint8_t*>((injector::ReadRelativeOffset(hook::pattern("0F B6 05 ? ? ? ? 48 8B 7C 24 58 48 8B 74 24 50 F3 0F 59 D2 F3 0F 59").get_first<uint8_t>(3)).as_int())),
                reinterpret_cast<const float*>((injector::ReadRelativeOffset(hook::pattern("C7 05 ? ? ? ? 00 00 00 00 F3 0F 58 DF F3 41 0F 58 C0 0F 14 C3 F2 41").get_first<uint8_t>(2)).as_int() + 4)),
                reinterpret_cast<float*>((injector::ReadRelativeOffset(hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 58 C0 F3 44 0F 5C C0 F3 0F 10 05 ? ? ?").get_first<uint8_t>(4)).as_int())),
                reinterpret_cast<const float*>((injector::ReadRelativeOffset(hook::pattern("F3 0F 10 35 ? ? ? ? 0F 29 7C 24 60 F3 0F 10 3D ? ? ? ? 45 0F 29").get_first<uint8_t>(4)).as_int())), true});
        };
    }
} CameraModuleInstance;
