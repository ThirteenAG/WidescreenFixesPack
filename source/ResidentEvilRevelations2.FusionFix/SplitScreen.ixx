module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module SplitScreen;

import ComVars;

// Local split-screen co-op. The game gives each player an 800x360 viewport (of 1280x720) in a staggered layout and
// leaves the rest of the screen black. MAIN.SplitScreenLayout chooses between that layout and one half of the screen
// for each player: top and bottom, or left and right, which on a 32:9 screen gives each player a 16:9 view. The
// layout is set up in the 1280x720 space, which the game maps to a 16:9 area of the screen's height starting at the
// left edge; the fix's halves span 720 * aspect, and the game's own layout is centered on wider screens.

namespace
{
    const int* pLayout = nullptr;
    const int* pSwap = nullptr;

    void(__stdcall* VirtualToScreenRect)(void* out, int32_t* rect) = nullptr;

    void SetHalf(int32_t* rect, int32_t half)
    {
        // HUD transforms must use the layout actually installed for this session.
        const auto layout = (SplitScreen::Layout)std::clamp(*pLayout, 0, 2);
        SplitScreen::SetLayout(layout);
        if (*pSwap)
            half = 1 - half;
        const int32_t w = (int32_t)(720.0f * GetAspectRatio());
        if (layout == SplitScreen::Layout::Original)
        {
            const int32_t offset = std::max(0, (w - 1280) / 2);
            rect[0] = offset + (half == 0 ? 136 : 344);
            rect[1] = half == 0 ? 0 : 360;
            rect[2] = rect[0] + SplitScreen::NativeResX;
            rect[3] = rect[1] + SplitScreen::NativeResY;
        }
        else if (layout == SplitScreen::Layout::SideBySide)
        {
            rect[0] = half == 0 ? 0 : w / 2;
            rect[1] = 0;
            rect[2] = half == 0 ? w / 2 : w;
            rect[3] = 720;
        }
        else
        {
            rect[0] = 0;
            rect[1] = half == 0 ? 0 : 360;
            rect[2] = w;
            rect[3] = half == 0 ? 360 : 720;
        }
    }

    void __stdcall SplitScreenSetupTop(void* out, int32_t* rect)
    {
        SetHalf(rect, 0);
        VirtualToScreenRect(out, rect);
    }

    void __stdcall SplitScreenSetupBottom(void* out, int32_t* rect)
    {
        SetHalf(rect, 1);
        VirtualToScreenRect(out, rect);
    }
}

class SplitScreenFix
{
public:
    SplitScreenFix()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            pLayout = &settings.Int("MAIN.SplitScreenLayout");
            pSwap = &settings.Int("MAIN.SplitScreenSwapTopBottom");

            auto top = hook::pattern("E8 ? ? ? ? 6A 01 6A 00 8B CE E8 ? ? ? ? 8D 44 24 30");
            auto bottom = hook::pattern("E8 ? ? ? ? 6A 01 6A 01 8B CE E8 ? ? ? ? 8D 44 24 30");

            VirtualToScreenRect = (decltype(VirtualToScreenRect))injector::GetBranchDestination(top.get_first()).as_int();
            injector::MakeCALL(top.get_first(), SplitScreenSetupTop, true);
            injector::MakeCALL(bottom.get_first(), SplitScreenSetupBottom, true);
        };
    }
} SplitScreenFix;
