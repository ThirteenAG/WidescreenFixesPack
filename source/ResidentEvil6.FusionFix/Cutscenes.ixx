module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module Cutscenes;

import ComVars;
import HUD;

// Cutscenes. The game renders them in a 1280x720 frame with cinematic bars above and below. The fix widens the frame
// to the screen's aspect, and MAIN.CutsceneBorders chooses the borders: 1 keeps the original bars (letterbox), 2 keeps
// the image at 16:9 with bars at the sides (pillarbox), 3 both, 0 none. Without the letterbox the frame covers the
// whole height and keeps its vertical framing, so nothing hidden by the bars is shown. Subtitles move into the image.

namespace
{
    const int* pCutsceneBorders = nullptr;
    int32_t nBorderSize = 0;

    bool Letterbox() { return (*pCutsceneBorders & 1) != 0; }
    bool Pillarbox() { return (*pCutsceneBorders & 2) != 0; }
}

class Cutscenes
{
public:
    Cutscenes()
    {
        WFP::onInitEvent() += []()
        {
            pCutsceneBorders = &FixSettings().Int("MAIN.CutsceneBorders");

            // The frame's width: 720 * aspect instead of 1280
            auto pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 8B 8E ? ? ? ? 89 6C 24");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Cutscenes"))
            {
                injector::MakeNOP(pattern.get_first(), 8, true);
                static auto CutsceneDimensionsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    *(int32_t*)(regs.esp + 0x24) = static_cast<int32_t>(720.0f * GetAspectRatio());
                });
            }

            // The cutscene viewport (left, top, right, bottom)
            pattern = hook::pattern("E8 ? ? ? ? 8B 8E ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C7 86");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Cutscenes"))
            {
                static auto CutsceneViewportHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    const int32_t resX = *(int32_t*)(regs.esp + 0x34);
                    const int32_t resY = *(int32_t*)(regs.esp + 0x38);

                    int32_t& x = *(int32_t*)(regs.esp + 0x3C);
                    int32_t& y = *(int32_t*)(regs.esp + 0x40);
                    int32_t& w = *(int32_t*)(regs.esp + 0x44);
                    int32_t& h = *(int32_t*)(regs.esp + 0x48);

                    if (Pillarbox())
                    {
                        const int32_t targetWidth = (resY * 16) / 9;
                        const int32_t border = (std::max)((resX - targetWidth) / 2, 0);
                        x += border;
                        w -= border;
                    }

                    nBorderSize = 0;
                    if (!Letterbox())
                    {
                        nBorderSize = y;
                        h += y;
                        y = 0;
                    }
                });
            }

            // Subtitles: the cutscene style (inside the image) everywhere, placed for the whole screen
            pattern = find_pattern("E8 ? ? ? ? 85 C0 75 ? 0F 57 C0 A1", "E8 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 ? F3 0F 10 05 ? ? ? ? 89 7C 24");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Cutscenes"))
            {
                injector::MakeNOP(pattern.get_first(), 5, true);
                static auto ForceSubtitlesStyle = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if ((Pillarbox() && IsCutsceneViewActive()) || IsSplitScreenActive())
                        SetSubtitlesNow();
                    regs.eax = *(uint32_t*)(regs.ecx + 0x20);
                    if (regs.eax == 1)
                        regs.eax = 0;
                });
            }

            // Subtitles' height: moved down by the removed bar
            pattern = hook::pattern("E8 ? ? ? ? 85 C0 75 ? 83 C7 ? 8B 86");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Cutscenes"))
            {
                injector::MakeNOP(pattern.get_first(), 5, true);
                static auto SubtitlesVerticalPos = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    regs.eax = *(uint32_t*)(regs.ecx + 0x20);
                    if (regs.eax == 1)
                        regs.edi += nBorderSize;
                });
            }
        };
    }
} Cutscenes;
