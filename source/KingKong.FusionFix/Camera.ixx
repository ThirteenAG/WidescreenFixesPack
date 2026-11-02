module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;

import ComVars;
import Hud;

void* retAddrFading = nullptr;
bool bSkipFOVHook = false;
SafetyHookInline shsub_9B1C80 = {};
void __cdecl sub_9B1C80(float* a1)
{
    auto raddr = _ReturnAddress();
    if (raddr == retAddrFading)
        bSkipFOVHook = true;

    shsub_9B1C80.unsafe_ccall(a1);
    bSkipFOVHook = false;
}

float BoundedProjection(float tangent)
{
    constexpr float minimumScale = 0.2679491924f; // cot(150 degrees / 2)
    if (!std::isfinite(tangent) || tangent <= 0.0f ||
        !std::isfinite(Screen.fFieldOfView) || Screen.fFieldOfView <= 0.0f)
        return 1.0f;
    // Menus keep the original framing
    static const auto& configuredFactor = FixSettings().Float("MAIN.FOVFactor");
    const float factor = !KingKongInMenu() && std::isfinite(configuredFactor) ? std::clamp(configuredFactor, 0.5f, 2.0f) : 1.0f;
    return std::max(minimumScale, Screen.fFieldOfView / (tangent * factor));
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            // FOV
            auto pattern = find_pattern("83 EC 1C 53 55 56 8B 74 24 ? 8B 46", "A1 ? ? ? ? 81 EC ? ? ? ? 83 F8 03", "A1 ? ? ? ? 81 EC A4 00 00 00 53");
            shsub_9B1C80 = safetyhook::create_inline(pattern.get_first(), sub_9B1C80);

            pattern = find_pattern("E8 ? ? ? ? D9 44 24 ? A1 ? ? ? ? 8A 88");
            if (!Screen.bSignatureEdition) // Gamer's Edition
            {
                static auto FOVHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
                {
                    if (bSkipFOVHook)
                        *(float*)(regs.esp + 0x34) = 1.0f / tan(*(float*)(regs.esp + 0x28 + 4) * 0.5f);
                    else
                        *(float*)(regs.esp + 0x34) = BoundedProjection(std::tan(*(float*)(regs.esp + 0x28 + 4) * 0.5f));
                });
            }
            else
            {
                pattern = hook::pattern("E8 ? ? ? ? D9 44 24 ? 8B ? ? ? ? ? 8A");
                static auto offset = *pattern.get_first<int8_t>(-1);
                static auto FOVHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
                {
                    if (bSkipFOVHook)
                        *(float*)(regs.esp + offset) = 1.0f / *(float*)(regs.esp + 0x18 + 4);
                    else
                        *(float*)(regs.esp + offset) = BoundedProjection(*(float*)(regs.esp + 0x18 + 4));
                });

                // Some interface elements
                pattern = hook::pattern("A1 ? ? ? ? 56 05 C8 00 00 00");
                retAddrFading = pattern.get_first();
            }

            // Objects disappear at the screen edges with Hor+
            pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 10 8B 5C 24 10 53"); //0x98B8CF
            injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Screen.fCutOffArea, true);
            static auto cullingHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                const float nativeFOV = *reinterpret_cast<float*>(regs.esi + 0x0C);
                if (std::isfinite(nativeFOV) && nativeFOV > 0.0f && nativeFOV < static_cast<float>(M_PI))
                {
                    const float scale = BoundedProjection(std::tan(nativeFOV * 0.5f));
                    Screen.fCutOffArea = std::atan(1.0f / scale) / nativeFOV;
                }
                else
                    Screen.fCutOffArea = 0.5f;
            });

            // Grass at the screen edges
            static float f0 = 0.0f;
            pattern = hook::pattern("D8 0D ? ? ? ? D9 C0 D9 FF D9 9C 24");
            if (!Screen.bSignatureEdition) // Gamer's Edition
                injector::WriteMemory<float*>(pattern.get_first<float*>(2), &f0, true);
            else
            {
                pattern = hook::pattern("D8 0D ? ? ? ? D9 9D ? ? ? ? D9 85 ? ? ? ? 83 EC 08 DD 1C 24 E8 ? ? ? ? 83 C4 08 D9 9D ? ? ? ? D9 85 ? ? ? ? D8 3D ? ? ? ? D9 9D ? ? ? ? 8B 85 ? ? ? ? D9 05 ? ? ? ? D9 80 ? ? ? ? DA E9 DF E0 F6 C4 44 7A ? 8B 8D ? ? ? ? C7 81 ? ? ? ? ? ? ? ? 8B 15 ? ? ? ? 89 95 ? ? ? ? A1 ? ? ? ? 6B C0 18");
                injector::WriteMemory<float*>(pattern.get_first<float*>(2), &f0, true);
            }

            // Camera shake at any frame rate
            pattern = hook::pattern("D9 05 ? ? ? ? D8 49 ? 8B 55 ? D8 6A ? 8B 45 ? D9 58 ? 8B 4D ? D9 05");
            fTimeStep.SetAddress(*pattern.get_first<float*>(2));

            static auto CameraShakeFix = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (std::isfinite(fTimeStep.get()) && fTimeStep > 0.0f)
                {
                    const float targetFPS = 1.0f / fTimeStep;
                    *(float*)(regs.ecx + 0x2C) *= targetFPS / 30.0f;
                }
            });
        };
    }
} Camera;
