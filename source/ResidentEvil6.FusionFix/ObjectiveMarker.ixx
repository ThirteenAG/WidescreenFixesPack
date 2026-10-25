module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module ObjectiveMarker;

import ComVars;

// The 3D objective markers. The game projects an objective into the view and places its marker on a plane in front of
// the HUD's 3D camera, 2650 units away.

namespace
{
    const int* pDisableObjectiveIndicator = nullptr;

    // The game's version scales the viewport's pixel size by constants made for 1280x720 (1097 / 360 for a split
    // viewport, for example), so the marker drifts away from its target at any other resolution, and it assumes a 16:9
    // view. The HUD's 3D space shows 1097 units from the viewport's center to its top edge at the marker's distance,
    // and as many times the viewport's aspect to its side edges, at any resolution.
    float* __stdcall ObjectiveIndicator(float* out, float* screen, int player)
    {
        if (*pDisableObjectiveIndicator)
            return out;

        auto viewport = GetPlayerViewport(player);
        const float width = (float)(viewport[2] - viewport[0]);
        const float height = (float)(viewport[3] - viewport[1]);
        const float aspect = width > 0.0f && height > 0.0f ? width / height : defaultAspectRatio;

        // Split screen: the HUD's 3D space of the second view is offset by the view's position on the screen
        const float offsetX = width > 0.0f ? (float)viewport[0] / width : 0.0f;
        const float offsetY = height > 0.0f ? (float)viewport[1] / height : 0.0f;

        constexpr float HalfHeight = 1097.0f;
        out[0] = HalfHeight * aspect * (screen[0] + offsetX);
        out[1] = HalfHeight * (screen[1] - offsetY);
        out[2] = -2650.0f;
        out[3] = 0.0f;
        return out;
    }
}

class ObjectiveMarker
{
public:
    ObjectiveMarker()
    {
        WFP::onInitEvent() += []()
        {
            pDisableObjectiveIndicator = &FixSettings().Int("MAIN.DisableObjectiveIndicator");

            auto pattern = hook::pattern("E8 ? ? ? ? D9 00 0F 57 C0 D9 5E 40 D9 40 04 5F");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: ObjectiveMarker"))
                injector::MakeCALL(pattern.get_first(), ObjectiveIndicator, true);
        };
    }
} ObjectiveMarker;
