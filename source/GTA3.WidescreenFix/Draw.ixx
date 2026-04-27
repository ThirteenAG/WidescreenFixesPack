module;

#include <stdafx.h>
#include "common.h"

export module Draw;

import Skeleton;
import CutsceneMgr;
import Camera;

template<typename... Args>
class ResChange : public WFP::Event<Args...>
{
public:
    using WFP::Event<Args...>::Event;
};

export __declspec(noinline) ResChange<int, int>& onResChange()
{
    static ResChange<int, int> ResChangeEvent;
    return ResChangeEvent;
}

std::optional<float> fHudAspectRatioConstraint;
export float fWidescreenHudOffset = 0.0f;

export class CDraw
{
private:
    static inline GameRef<float> ms_fFOV;
    static inline GameRef<float> ms_fNearClipZ;
    static inline GameRef<float> ms_fFarClipZ;

    static inline float ms_fAspectRatio;
    static inline float ms_fScaledFOV;
public:
    static inline bool ms_bProperScaling = true;
    static inline bool ms_bFixRadar = true;
    static inline bool ms_bFixSprites = true;

    static void SetFOV(float fov);
    static float GetFOV() { return ms_fFOV; }
    static float GetScaledFOV() { return ms_fScaledFOV; }

    static float FindAspectRatio();
    static float ConvertFOV(float fov);
    static float ConvertFOVforCutscene(float fov);
    static float GetAspectRatio() { return ms_fAspectRatio; }
    static void SetAspectRatio(float ratio)
    {
        if (ms_fAspectRatio == ratio)
            return;

        ms_fAspectRatio = ratio;

        fWidescreenHudOffset = CalculateWidescreenOffset(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, true);
        if (fHudAspectRatioConstraint.has_value())
        {
            float value = fHudAspectRatioConstraint.value();
            if (value < 0.0f || value > (32.0f / 9.0f))
                fWidescreenHudOffset = value;
            else
            {
                value = ClampHudAspectRatio(value, FindAspectRatio());
                fWidescreenHudOffset = CalculateWidescreenOffset(SCREEN_HEIGHT * value, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, true);
            }
        }

        onResChange().executeAll(RsGlobal->width, RsGlobal->height);
    }
    static float ScaleY(float y);

    static void SetNearClipZ(float nearclip) { ms_fNearClipZ = nearclip; }
    static float GetNearClipZ(void) { return ms_fNearClipZ; }
    static void SetFarClipZ(float farclip) { ms_fFarClipZ = farclip; }
    static float GetFarClipZ(void) { return ms_fFarClipZ; }

    CDraw()
    {
        WFP::onGameInitEvent() += []()
        {
            auto pattern = hook::pattern("D8 35 ? ? ? ? D8 0D");
            ms_fFOV.SetAddress(*pattern.get_first<float*>(2));

            pattern = hook::pattern("E8 ? ? ? ? 80 7B ? ? 59 75 ? 8B 83");
            static auto SetFOV = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CDraw::SetFOV);

            pattern = hook::pattern("D8 1D ? ? ? ? DF E0 80 E4 05 80 FC 01 75 ? 30 C0 E9 BC 00 00 00");
            ms_fNearClipZ.SetAddress(*pattern.get_first<float*>(2));

            pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 45 75 ? 30 C0 E9 A0 00 00 00");
            ms_fFarClipZ.SetAddress(*pattern.get_first<float*>(2));
        };
    }
} Draw;

float CDraw::FindAspectRatio()
{
    return SCREEN_WIDTH / SCREEN_HEIGHT;
}

float CDraw::ConvertFOV(float hfov)
{
    float ar1 = DEFAULT_ASPECT_RATIO;
    float ar2 = GetAspectRatio();
    hfov = DEGTORAD(hfov);
    float vfov = Atan(tan(hfov / 2) / ar1) * 2;
    hfov = Atan(tan(vfov / 2) * ar2) * 2;
    return RADTODEG(hfov);
}

float CDraw::ConvertFOVforCutscene(float fov)
{
    float reduction = TheCamera->m_ScreenReductionPercentage;
    float contentAspect = (4.0f / 3.0f) / std::max(1.0f - reduction / 100.0f, 0.01f);
    float screenAspect = FindAspectRatio();

    if (screenAspect > contentAspect)
    {
        float hfov = DEGTORAD(fov);
        return RADTODEG(2.0f * Atan(tan(hfov / 2.0f) * screenAspect / contentAspect));
    }
    else
    {
        return fov;
    }
}

export namespace FOVManager
{
    // Internal storage: hash -> multiplier
    inline std::unordered_map<uintptr_t, float> g_FOVMultipliers;
    inline float g_BaseFOVMultiplier = 1.0f; // fallback

    // Recalculate current total multiplier
    float GetCurrentMultiplier()
    {
        float total = g_BaseFOVMultiplier;
        for (const auto& [hash, mult] : g_FOVMultipliers)
            total *= mult;
        return total;
    }

    // API Exports
    void __cdecl GetCurrentFOV(float* out)
    {
        if (out)
            *out = CDraw::GetFOV() * GetCurrentMultiplier();
    }

    void __cdecl SetFOVMultiplier(void* hash, float value)
    {
        if (!hash) return;
        uintptr_t key = reinterpret_cast<uintptr_t>(hash);
        g_FOVMultipliers[key] = std::clamp(value, 0.5f, 2.0f);
    }

    void __cdecl RemoveFOVMultiplier(void* hash)
    {
        if (!hash) return;
        uintptr_t key = reinterpret_cast<uintptr_t>(hash);
        g_FOVMultipliers.erase(key);
    }
}

void CDraw::SetFOV(float fov)
{
    if (!CCutsceneMgr::IsRunning() && !TheCamera->m_WideScreenOn)
    {
        ms_fScaledFOV = ConvertFOV(fov) * FOVManager::GetCurrentMultiplier();
    }
    else
    {
        ms_fScaledFOV = ConvertFOVforCutscene(fov);
    }
    ms_fFOV = fov;
}

float CDraw::ScaleY(float y)
{
    return ms_bProperScaling ? y : y * ((float)DEFAULT_SCREEN_HEIGHT / SCREEN_HEIGHT_NTSC);
}

class Camera
{
public:
    Camera()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));

            static float fScaledFOV = 0.0f;
            auto pattern = hook::pattern("D9 05 ? ? ? ? D8 0D ? ? ? ? D8 0D ? ? ? ? DD DC");
            injector::WriteMemory(pattern.get_first(2), &fScaledFOV, true);
            injector::MakeNOP(pattern.get_first(6), 12, true);
            static auto CalculateDerivedValues = safetyhook::create_mid(pattern.get_first(-8), [](SafetyHookContext& regs)
            {
                fScaledFOV = DEGTORAD(CDraw::GetScaledFOV() / 2.0f);
            });

            static float fAspectRatio = 0.0f;
            pattern = hook::pattern("80 3D ? ? ? ? ? 74 ? D9 05 ? ? ? ? EB 06");
            injector::WriteMemory(pattern.get_first(11), &fAspectRatio, true);
            injector::WriteMemory(pattern.get_first(19), &fAspectRatio, true);
            static auto Find3rdPersonCamTargetVector = safetyhook::create_mid(pattern.get_first(0), [](SafetyHookContext& regs)
            {
                fAspectRatio = CDraw::GetAspectRatio();
            });

            // CCamera::Process FOV
            pattern = hook::pattern("74 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? EB ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 80 7B");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);

            // CBrightLights::Render, render only when far away
            pattern = hook::pattern("C6 44 24 ? ? EB ? ? ? ? ? ? ? DF E0");
            if (!pattern.empty())
                injector::WriteMemory<uint8_t>(pattern.get_first(4), 0, true);
        };
    }
} Camera;
