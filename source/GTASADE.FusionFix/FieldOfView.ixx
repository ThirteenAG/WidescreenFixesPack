module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module FieldOfView;

namespace
{
    // CCamera: m_WideScreenOn, ActiveCam, Cams[0].Mode and the CCam size.
    constexpr size_t wideScreenOn = 65, activeCam = 91, camMode = 432, camSize = 440;
    // Sniper, rocket launcher (incl. heat-seeking), fixed, flyby, M16, heli
    // cannon, camera, attached and cinematic views, and the runabout variants:
    // zoom and scripted views keep their FOV.
    constexpr std::array excludedModes{7, 8, 15, 17, 34, 39, 40, 42, 45, 46, 47, 51, 52, 56, 57, 58, 59, 60, 61, 62, 63, 64};
    const uint8_t* cutsceneRunning = nullptr;
    const uint8_t* theCamera = nullptr;

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
            // CDraw::SetFOV, called once per frame by CCamera::Process, widens
            // the active camera's FOV for the aspect ratio and stores it. The
            // Unreal camera, culling, sprites and the third-person aim ray read
            // it from there; CCam::FOV, which mouse and aim sensitivity use,
            // stays the game's.
            auto storePattern = hook::pattern("F3 0F 58 C1 F3 0F 11 05 ? ? ? ? C3");
            auto runningPattern = hook::pattern("38 15 ? ? ? ? 74 ? F3 0F 10 15 ? ? ? ? 0F 2F CA");
            auto cameraPattern = hook::pattern("48 8D 05 ? ? ? ? 0F B7 84 01 B0 01 00 00 66 83 F8 2E");
            if (storePattern.empty() || runningPattern.empty() || cameraPattern.empty())
                return;
            const auto store = storePattern.get_first(4);
            const auto running = runningPattern.get_first<uint8_t>(2);
            const auto camera = cameraPattern.get_first<uint8_t>(3);
            cutsceneRunning = reinterpret_cast<const uint8_t*>(injector::ReadRelativeOffset(running).as_int());
            theCamera = reinterpret_cast<const uint8_t*>(injector::ReadRelativeOffset(camera).as_int());
            static auto fovHook = safetyhook::create_mid(store, [](SafetyHookContext& c)
            {
                if (IsGameplayView(theCamera))
                    c.xmm0.f32[0] = ScaledFOV(c.xmm0.f32[0]);
            });
        };
    }
} FieldOfViewModuleInstance;
