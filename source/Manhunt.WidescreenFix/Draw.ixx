module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"

export module Draw;

std::optional<float> fHudAspectRatioConstraint;

export namespace CDraw
{
    // The aspect ratio of the box the HUD is kept in on a screen of this aspect ratio
    float GetHudAspectRatio(float screenAspect)
    {
        // Auto: the whole width of the screen, elements at the sides go to the screen's edges. A ratio limits the HUD
        // to a centered box of that aspect. Values outside the ratio range are the pixel offsets of older settings
        // files: those count as Auto.
        float value = screenAspect;
        if (fHudAspectRatioConstraint && *fHudAspectRatioConstraint > 0.0f && *fHudAspectRatioConstraint <= 32.0f / 9.0f)
            value = *fHudAspectRatioConstraint;
        return ClampHudAspectRatio(value, screenAspect);
    }

    // How much narrower the 4:3 design is than the screen (1 on 4:3 and narrower screens)
    float GetDesignScale(float screenAspect)
    {
        return screenAspect > DEFAULT_ASPECT_RATIO ? DEFAULT_ASPECT_RATIO / screenAspect : 1.0f;
    }
}

class Draw
{
public:
    Draw()
    {
        WFP::onInitEvent() += []()
        {
            fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            OnSettingChange("MAIN.HudAspectRatioConstraint", []()
            {
                fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            });
        };
    }
} Draw;
