module;

#include <stdafx.h>

export module Camera;

import ComVars;

export namespace THPS4Camera
{
    inline float Limit(float corrected, float factor)
    {
        if (!std::isfinite(corrected) || !std::isfinite(factor) || factor == 1.0f)
            return corrected;
        return std::clamp(corrected * std::clamp(factor, 0.5f, 2.0f), 1.0f, std::max(150.0f, corrected));
    }
}

export namespace THPS4Camera
{
    uintptr_t SkaterType = 0;

    float Correct(uintptr_t camera, float fov, float value)
    {
        float factor = 1.0f;
        if (SkaterType && *reinterpret_cast<uintptr_t*>(camera) == SkaterType && CameraLayout.is_initialized() && NativeZoomScale.is_initialized() &&
            std::abs(NativeZoomScale.get() - 1.0f) < 0.001f)
        {
            const int mode = *reinterpret_cast<int*>(camera + 0xBC);
            const auto skater = *reinterpret_cast<uintptr_t*>(camera + 0xC4);
            if (mode >= 1 && mode <= 4 && skater && *reinterpret_cast<uint8_t*>(skater + 13624))
            {
                const float baseline = *reinterpret_cast<float*>(camera + 0xE8) * (CameraLayout.get() == 1 ? 0.5f : 1.0f);
                const float nativeFov = *reinterpret_cast<float*>(camera + 0xA0);
                const auto animations = *reinterpret_cast<uintptr_t*>(skater + 14300);
                const auto first = animations ? *reinterpret_cast<uintptr_t*>(animations + 0x4C) : 0;
                const bool animated = first && *reinterpret_cast<int*>(first + 4) != -1;
                if (!animated && std::abs(nativeFov - baseline) < 0.001f && std::isfinite(value))
                    factor = std::clamp(value, 0.5f, 2.0f);
            }
        }
        return Limit(AdjustFOV(fov, Screen.fAspectRatio), factor);
    }

    void Init()
    {
        auto type = hook::pattern("C7 06 ? ? ? ? 89 9E BC 00 00 00 39 1D");
        auto layout = hook::pattern("8B F9 E8 ? ? ? ? 48 6A 00 74 ? 48 74 ? 68 ? ? ? ? EB ? 68 ? ? ? ? EB ? 68 ? ? ? ? E8");
        auto scale = hook::pattern("E8 ? ? ? ? D9 44 24 04 D9 F2 DD D8 D9 54 24 04 DE C9 D9 E8 D9 F3");
        auto animation = hook::pattern("51 53 8B D9 55 56 8B 43 4C 57 83 CF FF 33 C9 39 78 04 74 ? 85 C0 74 ? 8B 40 0C 41");
        (void)animation.get_first();
        auto layoutGetter = injector::GetBranchDestination(layout.get_first(2)).as_int();
        auto scaleGetter = injector::GetBranchDestination(scale.get_first()).as_int();
        CameraLayout.SetAddress(*reinterpret_cast<int**>(layoutGetter + 1));
        NativeZoomScale.SetAddress(*reinterpret_cast<float**>(scaleGetter + 2));
        SkaterType = *type.get_first<uintptr_t>(2);
    }
}

export void InstallAspectRatio()
{
    hook::pattern pattern("");
    //Aspect Ratio
    pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 33 FF 57 68"); //4616E2
    injector::WriteMemory<float>(pattern.get_first<float>(1), Screen.fAspectRatio, true);
}

export void InstallCameraFix(const float* factor)
{
    static const float* cameraFactor = factor;
    hook::pattern pattern("");
    THPS4Camera::Init();
    //FOV
    pattern = hook::pattern("D9 96 A4 00 00 00 5E 59 C3");
    struct FovHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float fov = 0.0f;
            _asm { fstp dword ptr[fov] }
            fov = THPS4Camera::Correct(regs.esi, fov, *cameraFactor);
            *reinterpret_cast<float*>(regs.esi + 0xA4) = fov;
            _asm { fld dword ptr[fov] }
        }
    };
    injector::MakeInline<FovHook>(pattern.get_first(0), pattern.get_first(6));
}
