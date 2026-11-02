module;

#include <stdafx.h>

export module ComVars;

import Movies;

export GameRef<uint32_t> ActiveCutscene;
export GameRef<uint32_t> RenderWidth, RenderHeight;
export ProtectedGameRef<float> FrontendAspect, OverlayAspect;
export GameRef<int32_t> HudLeftMargin, HudRightMargin;
export GameRef<float> CameraBarAmount;
export ProtectedGameRef<uint32_t> ChosenWidth, ChosenHeight, ChosenRefresh;
export ProtectedGameRef<uint32_t> DeviceWidth, DeviceHeight, DeviceRefresh;

// The 3D views and the frontend take the aspect ratio of the game's renderer size, also after a resolution change
export namespace Aspect
{
    float Factor = 0.25f * 4.0f / 3.0f;
    uint32_t Applied[2]{};

    void Update()
    {
        if (!RenderWidth.is_initialized() || !RenderHeight.is_initialized() || !RenderWidth || !RenderHeight || (Applied[0] == RenderWidth && Applied[1] == RenderHeight))
            return;
        Applied[0] = RenderWidth;
        Applied[1] = RenderHeight;
        const float ratio = float(RenderWidth.get()) / float(RenderHeight.get());
        Factor = 0.25f * ratio;
        FrontendAspect = ratio;
        OverlayAspect = ratio;
        SecondSightMovies::Resize(RenderWidth, RenderHeight);
    }
}

