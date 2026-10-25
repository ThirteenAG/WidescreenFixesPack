module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module FieldOfView;

namespace
{
    // CCamera: m_WideScreenOn, ActiveCam, Cams[0].Mode and the CCam size.
    constexpr size_t wideScreenOn = 109, activeCam = 116, camMode = 438, camSize = 344;
    // Sniper, rocket launcher, fixed, flyby, M16, heli cannon and camera
    // and the runabout variants: zoom and scripted views keep their FOV.
    constexpr std::array excludedModes{7, 8, 15, 17, 34, 39, 40, 42, 45, 46};
    const uint8_t* cutsceneRunning = nullptr;

    bool IsGameplayView(const uint8_t* camera)
    {
        if (*cutsceneRunning || camera[wideScreenOn])
            return false;
        const int mode = *reinterpret_cast<const uint16_t*>(camera + camMode + camSize * camera[activeCam]);
        return std::find(excludedModes.begin(), excludedModes.end(), mode) == excludedModes.end();
    }

    float ScaledFOV(float fov)
    {
        static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
        if (!std::isfinite(factor) || factor == 1.0f || !std::isfinite(fov) || fov <= 0.0f)
            return fov;
        return std::min(fov * std::clamp(factor, 0.5f, 2.0f), 170.0f);
    }
}

class FieldOfViewModule
{
public:
    FieldOfViewModule()
    {
        WFP::onInitEvent() += []()
        {
            // CCamera::Process stores the active camera's FOV, already widened
            // for the aspect ratio, as CDraw's FOV. The Unreal camera, culling,
            // sprites and the third-person aim ray read it from there; CCam::FOV,
            // which mouse and aim sensitivity use, stays the game's.
            auto storePattern = hook::pattern("F3 0F 11 1D ? ? ? ? F3 0F 11 45 C0 84 D2");
            auto runningPattern = hook::pattern("0F B6 15 ? ? ? ? 84 D2 75 ? 0F B6 05 ? ? ? ? 48 69 C8 58 01 00 00");
            if (storePattern.empty() || runningPattern.empty())
                return;
            const auto store = storePattern.get_first();
            const auto running = runningPattern.get_first<uint8_t>(3);
            cutsceneRunning = reinterpret_cast<const uint8_t*>(injector::ReadRelativeOffset(running).as_int());
            static auto fovHook = safetyhook::create_mid(store, [](SafetyHookContext& c)
            {
                if (IsGameplayView(reinterpret_cast<const uint8_t*>(c.rbx)))
                    c.xmm3.f32[0] = ScaledFOV(c.xmm3.f32[0]);
            });
        };
    }
} FieldOfViewModuleInstance;
