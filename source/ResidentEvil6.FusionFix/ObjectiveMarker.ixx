module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module ObjectiveMarker;

import ComVars;
import HUD;

// The 3D objective markers. The game projects an objective into the view and places its marker on a plane in front of
// the HUD's 3D camera, 2650 units away.

namespace
{
    const int* pDisableObjectiveIndicator = nullptr;

    // The game's version scales the viewport's pixel size by constants made for 1280x720 (1097 / 360 for a split
    // viewport, for example), so the marker drifts away from its target at any other resolution, and it assumes a 16:9
    // view. The HUD's 3D space shows 1097 units from the viewport's center to its top edge at the marker's distance,
    // and as many times the viewport's aspect to its side edges, at any resolution. The cockpit screen's projection is
    // shifted for the gauges (HUD), so the markers move back by as much.
    float* PlaceMarker(float* out, const float* screen, int player)
    {
        auto viewport = GetPlayerViewport(player);
        const float width = (float)(viewport[2] - viewport[0]);
        const float height = (float)(viewport[3] - viewport[1]);
        const float aspect = width > 0.0f && height > 0.0f ? width / height : defaultAspectRatio;

        // Split screen: the HUD's 3D space of the second view is offset by the view's position on the screen
        const float offsetX = width > 0.0f ? (float)viewport[0] / width : 0.0f;
        const float offsetY = height > 0.0f ? (float)viewport[1] / height : 0.0f;

        constexpr float HalfHeight = 1097.0f;
        out[0] = HalfHeight * aspect * (screen[0] + offsetX - GetCockpitProjectionShift(player));
        out[1] = HalfHeight * (screen[1] - offsetY);
        out[2] = -2650.0f;
        out[3] = 0.0f;
        return out;
    }

    float* __stdcall ObjectiveIndicator(float* out, float* screen, int player)
    {
        if (*pDisableObjectiveIndicator)
            return out;
        return PlaceMarker(out, screen, player);
    }

    // The other markers placed the same way (partner and target indicators at the screen's edges)
    float* __stdcall OtherIndicator(float* out, float* screen, int player)
    {
        return PlaceMarker(out, screen, player);
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
            injector::MakeCALL(pattern.get_first(), ObjectiveIndicator, true);

            // The game's marker placement, called from two more places
            static const auto placement = injector::GetBranchDestination(pattern.get_first()).as_int();
            pattern = hook::pattern("E8 ? ? ? ? D9 00 0F 57 C0 D9 9E ? 02 00 00 D9 40 04 D9 9E");
            pattern.for_each_result([](hook::pattern_match match)
            {
                if (injector::GetBranchDestination(match.get<void>(0)).as_int() == placement)
                    injector::MakeCALL(match.get<void>(0), OtherIndicator, true);
            });
        };
    }
} ObjectiveMarker;
