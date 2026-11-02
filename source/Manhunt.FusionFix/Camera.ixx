module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"

export module Camera;

import Skeleton;

namespace
{
    // CScene::Render sets the camera's view window for the frame from these (the camera's own, with the game's
    // corrections for broken values)
    auto ViewWindowPattern()
    {
        return hook::pattern("53 8B 5C 24 08 83 EC 0C 8B 4B 68 89 0D ? ? ? ? 8B 43 6C A3 ? ? ? ? 83 3D ? ? ? ? 01");
    }


    float fFOVFactor = 1.0f;
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            CCutScene::ms_bCutSceneInProgess.SetAddress(*hook::pattern("83 3D ? ? ? ? 00 74 07 31 C0 C3 8D 44 20 00 E8 ? ? ? ? B9").get_first<int32_t*>(2));
            CScene::m_zoom.SetAddress(*hook::pattern("D9 04 24 D8 35 ? ? ? ? D9 1C 24 D9 44 24 04").get_first<float*>(5));
            fViewWindowWidth.SetAddress(*ViewWindowPattern().get_first<float*>(13));
            fViewWindowHeight.SetAddress(*ViewWindowPattern().get_first<float*>(21));
            ActiveMenuOverlay.SetAddress(*ViewWindowPattern().get_first<int32_t*>(27));
            static auto& factor = FixSettings().Float("MAIN.FOVFactor");
            fFOVFactor = std::clamp(factor, 0.5f, 2.0f);
            OnSettingChange("MAIN.FOVFactor", []()
            {
                fFOVFactor = std::clamp(factor, 0.5f, 2.0f);
            });

            // Hor+: the view window keeps its height and gets as wide as the screen. The game's own "Ratio" option
            // (an anamorphic 16:9 squeeze for TVs) is left out: the fix sets the aspect ratio. The game derives its
            // own visibility angles from this view window right after, so culling follows.
            auto pattern = hook::pattern("52 53 E8 ? ? ? ? D9 EE D9 EE");

            static auto SetViewWindow = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                auto camera = reinterpret_cast<void*>(regs.ebx);
                auto raster = camera ? RwCameraGetRaster(camera) : nullptr;
                if (!raster)
                    return;

                const float width = float(RwRasterGetWidth(raster));
                const float height = float(RwRasterGetHeight(raster));
                if (width <= 0.0f || height <= 0.0f)
                    return;

                float zoom = CScene::m_zoom;
                if (!(zoom > 0.0f))
                    zoom = 1.0f;

                float x = fViewWindowWidth / zoom;
                float y = fViewWindowHeight / zoom;

                const float aspect = width / height;
                if (aspect >= DEFAULT_ASPECT_RATIO)
                    x *= aspect / DEFAULT_ASPECT_RATIO;
                else
                    y *= DEFAULT_ASPECT_RATIO / aspect;  // narrower screens see more above and below instead

                // The FOV factor is for gameplay: not for cutscenes, zoomed views or the menus' scene
                const bool gameplay = !CCutScene::ms_bCutSceneInProgess && ActiveMenuOverlay != 1 && std::fabs(zoom - 1.0f) < 0.001f;
                if (gameplay && x > 0.0f && y > 0.0f && std::isfinite(x) && std::isfinite(y))
                {
                    const float maximumX = std::tan(DEGTORAD(75.0f));
                    if (fFOVFactor != 1.0f || x > maximumX)
                    {
                        const float maximum = std::atan(maximumX * y / x);
                        const float scaled = std::tan(std::min(std::max(std::atan(y) * fFOVFactor, 0.05f), maximum));
                        x *= scaled / y;
                        y = scaled;
                    }
                }

                auto viewWindow = reinterpret_cast<float*>(regs.esp);
                viewWindow[0] = x;
                viewWindow[1] = y;
            });
        };
    }
} Camera;
