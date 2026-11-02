module;

#include <stdafx.h>

export module Camera;

import Cutscene;
import ComVars;

export namespace GameplayCamera
{

    float Factor(float value)
    {
        if (!CameraManager.is_initialized() || !CameraBlending.is_initialized())
            return 1.0f;
        const int current = CameraManager->currentMode;
        const int previous = CameraManager->previousMode;
        auto gameplay = [](int mode)
        {
            return mode == 1 || mode == 8; // follow and vehicle controllers
        };
        if (!gameplay(current) || (CameraBlending && !gameplay(previous)) ||
            CameraManager->blocked != 0 || CameraManager->overridden != 0)
            return 1.0f;
        return std::isfinite(value) ? std::clamp(value, 0.5f, 2.0f) : 1.0f;
    }

    float Adjust(float original, float aspect, float factor, bool primary, bool overridden)
    {
        const float value = std::isfinite(original) ? std::clamp(original, 1.0f, 179.0f) : 60.0f;
        if (primary && BullyCutscene::Active())
        {
            const float cutscene = BullyCutscene::FOV(value, aspect);
            const int mode = CameraManager.is_initialized() ? CameraManager->currentMode : 0;
            if (!overridden && CameraManager.is_initialized() && CameraManager->cutscene == 0 && (mode == 1 || mode == 8))
            {
                constexpr float radians = 0.017453292519943295f;
                const float base = std::clamp(AdjustFOV(value, aspect), 1.0f, 179.0f);
                const float gameplay = std::clamp(base * Factor(factor), 1.0f, std::max(base, 150.0f));
                const float phase = BullyCutscene::Calculate(aspect).progress;
                const float tangent = std::lerp(std::tan(cutscene * radians * 0.5f), std::tan(gameplay * radians * 0.5f), phase);
                return std::clamp(2.0f * std::atan(tangent) / radians, 1.0f, 179.0f);
            }
            return cutscene;
        }
        if (primary && overridden)
            return value;
        // A larger multiplier stops at 150 degrees: wider views smear the scene and break clipping
        const float base = std::clamp(AdjustFOV(value, aspect), 1.0f, 179.0f);
        const float scaled = base * (primary ? Factor(factor) : 1.0f);
        return std::clamp(scaled, 1.0f, std::max(base, 150.0f));
    }

    void Install(const float& factor)
    {
        static auto& fFOVFactor = factor;
        //FOV
        auto primary = hook::pattern("D9 05 ? ? ? ? EB 06 D9 05 ? ? ? ? D9 1C 24 D9 04 24 DC 0D ? ? ? ? D9 1C 24 D9 04 24 E8 ? ? ? ? D9 1C 24 80 7C 24 34 00").get_first<uint8_t>();
        auto secondary = hook::pattern("D9 05 ? ? ? ? EB 06 D9 05 ? ? ? ? D9 1C 24 D9 04 24 DC 0D ? ? ? ? D9 1C 24 D9 04 24 E8 ? ? ? ? D9 1C 24 80 3D ? ? ? ? 00").get_first<uint8_t>();
        static auto flt_C3CD04 = *reinterpret_cast<float**>(primary + 2);
        CameraManager.SetAddress(reinterpret_cast<uint8_t*>(flt_C3CD04) - 0x9C);
        auto blend = hook::pattern("51 56 8B F1 83 BE A4 01 00 00 00 0F 84 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 85 C0 0F 84");
        CameraBlending.SetAddress(*blend.get_first<int32_t*>(18) + 2);

        static std::array<SafetyHookMid, 2> projectionHooks;
        static auto applyProjection = [](SafetyHookContext& regs)
        {
            auto value = reinterpret_cast<float*>(regs.esp);
            const bool overridden = CameraManager->overridden != 0;
            Screen.fFieldOfView = GameplayCamera::Adjust(*value, Screen.fAspectRatio, fFOVFactor, true, overridden);
            *value = Screen.fFieldOfView;
        };
        projectionHooks[0] = safetyhook::create_mid(primary + 17, [](SafetyHookContext& regs)
        {
            // Fixed-aspect auxiliary renders keep their own projection.
            if (!*reinterpret_cast<uint8_t*>(regs.esp + 0x34))
                applyProjection(regs);
        });
        projectionHooks[1] = safetyhook::create_mid(secondary + 17, applyProjection);
        if (!projectionHooks[0] || !projectionHooks[1])
            std::terminate();

        //Render
        auto pattern = hook::pattern("D9 86 ? ? ? ? DC 0D ? ? ? ? DC 0D ? ? ? ? D9 5C 24 04 D9 44 24 04 E8 ? ? ? ? D9 5C 24 08 D9 44 24 08");
        struct CullingFOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                const float original = *reinterpret_cast<float*>(regs.esi + 0x9C);
                const bool primary = reinterpret_cast<void*>(regs.esi) == CameraManager.get_ptr();
                const bool overridden = primary && CameraManager->overridden != 0;
                const float value = overridden ? CameraManager->overrideFov : original;
                Screen.fFieldOfView = GameplayCamera::Adjust(value, Screen.fAspectRatio, fFOVFactor, primary, overridden);
                _asm fld[Screen.fFieldOfView]
            }
        };
        injector::MakeInline<CullingFOVHook>(pattern.get_first(0), pattern.get_first(6)); //0x4F1D16
    }
}
