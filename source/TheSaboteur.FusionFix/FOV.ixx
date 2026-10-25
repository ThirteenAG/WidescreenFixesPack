module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <atomic>

export module FOV;

import ComVars;

static std::atomic<bool> FixFOV{ true };
static SafetyHookInline SetFOVHook;

struct OdinCamera
{
    bool m_bOrthographic;
    PblMatrix4x4 m_ViewMatrix;
    PblMatrix4x4 m_ViewMatrixInverse;
    PblMatrix4x4 m_ProjMatrix;
    PblMatrix4x4 m_ProjMatrixInverse;
    unsigned int m_Dirty;
    float m_NearPlane;
    float m_FarPlane;
    float m_TanHalfFOVWidth;
    float m_TanHalfFOVHeight;

    static void __fastcall SetFOV(OdinCamera* OdinCamera, void* edx, float a_FovX, float a_AspectRatio)
    {
        if (!FixFOV.load(std::memory_order_relaxed))
        {
            SetFOVHook.unsafe_fastcall(OdinCamera, edx, a_FovX, a_AspectRatio);
            return;
        }
        if (!std::isfinite(a_AspectRatio) || a_AspectRatio <= 0.0f)
            a_AspectRatio = 3.0f / 4.0f;
        if (!std::isfinite(a_FovX) || a_FovX <= 0.0f || a_FovX >= static_cast<float>(M_PI))
            a_FovX = 1.0471976f;

        // too high fov breaks rendering (sky and objects start to disappear)
        auto fAspectRatioDiff = std::clamp((1.0f / a_AspectRatio) / (4.0f / 3.0f), 0.0f, 2.1f); // 2.1 seems safe before that happens
        auto a_FovXa = a_FovX * 0.5f;
        auto a_FovXb = tan(a_FovXa);
        OdinCamera->m_Dirty |= 3u;
        OdinCamera->m_bOrthographic = 0;
        OdinCamera->m_TanHalfFOVWidth = a_FovXb * fAspectRatioDiff;
        OdinCamera->m_TanHalfFOVHeight = a_FovXb * a_AspectRatio * fAspectRatioDiff;
    }

    static void __fastcall SetPerspective(OdinCamera* OdinCamera, void* edx, float a_NearPlane, float a_FarPlane, float a_FovX, float a_AspectRatio)
    {
        auto a_FovXa = a_FovX * 0.5f;
        auto a_FovXb = tan(a_FovXa);
        OdinCamera->m_Dirty |= 3u;
        OdinCamera->m_bOrthographic = 0;
        OdinCamera->m_NearPlane = a_NearPlane;
        OdinCamera->m_FarPlane = a_FarPlane;
        OdinCamera->m_TanHalfFOVWidth = a_FovXb;
        OdinCamera->m_TanHalfFOVHeight = a_FovXb * a_AspectRatio;
    }

    static float __fastcall GetAspectRatio(OdinCamera* OdinCamera, void* edx)
    {
        return OdinCamera->m_TanHalfFOVHeight / OdinCamera->m_TanHalfFOVWidth;
    }
};

class FOV
{
public:
    FOV()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            FixFOV.store(settings.Bool("MAIN.FixFOV"), std::memory_order_relaxed);
            settings.OnChange("MAIN.FixFOV", []()
            {
                FixFOV.store(FixSettings().Bool("MAIN.FixFOV"), std::memory_order_relaxed);
            });
            auto pattern = hook::pattern("D9 44 24 04 56 DC 0D ? ? ? ? 8B F1 D9 5C 24 08");
            if (PatternUnique(pattern, "TheSaboteur.FusionFix: FOV"))
            {
                SetFOVHook = safetyhook::create_inline(pattern.get_first(), OdinCamera::SetFOV, safetyhook::InlineHook::StartDisabled);
                if (SetFOVHook && !SetFOVHook.enable())
                    SetFOVHook.reset();
            }
        };
    }
} FOV;