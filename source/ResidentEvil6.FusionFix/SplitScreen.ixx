module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module SplitScreen;

import ComVars;

// Local split-screen co-op. The game gives each player a 20:9 viewport in a staggered layout (player 1 top left,
// player 2 bottom right) and leaves the rest of the screen black. The fix gives each player one half of the screen:
// top and bottom, or left and right with MAIN.SplitScreenVertical, which on a 32:9 screen gives each player a 16:9
// view.

namespace
{
    const int* pSwapTopBottom = nullptr;
    const int* pVertical = nullptr;

    void(__fastcall* SetPlayerViewport)(void* viewports, void* edx, int32_t player, int32_t* rect) = nullptr;

    // The game's SetPlayerViewport(player, rect), called for both players when the split layout is set up
    void __fastcall SetSplitViewport(void* viewports, void* edx, int32_t player, int32_t* rect)
    {
        const int32_t nativeW = rect[2] - rect[0], nativeH = rect[3] - rect[1];

        auto half = player;
        if (*pSwapTopBottom)
            half = half == 0 ? 1 : 0;

        const int32_t w = GetResX(), h = GetResY();
        int32_t r[4];
        if (*pVertical)
        {
            r[0] = half == 0 ? 0 : w / 2;
            r[1] = 0;
            r[2] = half == 0 ? w / 2 : w;
            r[3] = h;
        }
        else
        {
            r[0] = 0;
            r[1] = half == 0 ? 0 : h / 2;
            r[2] = w;
            r[3] = half == 0 ? h / 2 : h;
        }

        SplitScreen::SetSizes(nativeW, nativeH, r[2] - r[0], r[3] - r[1]);
        SetPlayerViewport(viewports, edx, player, r);
    }
}

export bool IsSplitScreenVertical()
{
    return pVertical && *pVertical;
}

class SplitScreenFix
{
public:
    SplitScreenFix()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            pSwapTopBottom = &settings.Int("MAIN.SplitScreenSwapTopBottom");
            pVertical = &settings.Int("MAIN.SplitScreenVertical");

            // Both calls of the split layout (game modes 1 and 4) go through SetSplitViewport; the game's other calls
            // of SetPlayerViewport (cutscenes, single view) stay as they are.
            auto pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 14 89 44 24 24");
            auto pattern2 = hook::pattern("E8 ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? 8B 0D");
            if (!PatternAvailable(pattern, "ResidentEvil6.FusionFix: SplitScreen") || !PatternAvailable(pattern2, "ResidentEvil6.FusionFix: SplitScreen"))
                return;

            SetPlayerViewport = (decltype(SetPlayerViewport))injector::GetBranchDestination(pattern.get_first()).as_int();
            injector::MakeCALL(pattern.get_first(), SetSplitViewport, true);
            injector::MakeCALL(pattern2.get_first(), SetSplitViewport, true);
        };
    }
} SplitScreenFix;
