module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <chrono>

export module Cutscenes;

import ComVars;
import HUD;

namespace CutsceneGeometry
{
    using Rect = std::array<int, 4>;

    inline Rect Frame(int width, int height, int borders)
    {
        if (width <= 0 || height <= 0) return {};
        if (!(borders & 2)) return { 0, 0, width, height };
        const double scale = (std::min)(width / 1280.0, height / 720.0);
        const int w = static_cast<int>(std::lround(1280 * scale));
        const int h = static_cast<int>(std::lround(720 * scale));
        return { (width - w) / 2, (height - h) / 2, (width + w) / 2, (height + h) / 2 };
    }

    inline Rect Fit(const Rect& native, int width, int height, int borders)
    {
        if (width <= 0 || height <= 0 || native[0] < 0 || native[1] < 0 || native[2] > width ||
            native[3] > height || native[0] >= native[2] || native[1] >= native[3]) return native;
        const auto frame = Frame(width, height, borders);
        Rect result;
        for (size_t i = 0; i < result.size(); ++i)
        {
            const int axis = static_cast<int>(i & 1);
            result[i] = frame[axis] + static_cast<int>(std::lround(static_cast<double>(native[i]) *
                (frame[axis + 2] - frame[axis]) / (axis ? height : width)));
        }
        if (!(borders & 1))
        {
            result[1] = frame[1];
            result[3] = frame[3];
        }
        return result;
    }

    struct Transition
    {
        Rect current{}, from{}, target{};
        int width = 0, height = 0, mode = -1, borders = -1;
        double started = 0;
        bool moving = false;

        Rect Update(const Rect& native, int w, int h, int view, int value, double now)
        {
            const auto desired = Fit(native, w, h, value);
            if (width != w || height != h || mode != view || borders < 0 || !std::isfinite(now))
            {
                width = w; height = h; mode = view; borders = value;
                current = from = target = desired;
                moving = false;
                return current;
            }
            if (borders != value)
            {
                from = current;
                target = desired;
                started = now;
                borders = value;
                moving = from != target;
            }
            else if (target != desired)
            {
                current = from = target = desired;
                moving = false;
            }
            if (moving)
            {
                const double progress = std::clamp((now - started) / 0.25, 0.0, 1.0);
                for (size_t i = 0; i < current.size(); ++i)
                    current[i] = static_cast<int>(std::lround(from[i] + (target[i] - from[i]) * progress));
                if (progress == 1) moving = false;
            }
            return current;
        }
    };
}

// Cutscenes. The game renders them in a 1280x720 frame with cinematic bars above and below. The fix widens the frame
// to the screen's aspect, and MAIN.CutsceneBorders chooses the borders: 1 keeps the original bars (letterbox), 2 keeps
// the image inside a 16:9 canvas (pillarbox), 3 both, 0 none. Without the letterbox the frame covers the
// whole height and keeps its vertical framing, so nothing hidden by the bars is shown. Subtitles move into the image.

namespace
{
    const int* pCutsceneBorders = nullptr;
    int32_t nBorderSize = 0;
    CutsceneGeometry::Transition transition;

    bool Letterbox() { return (*pCutsceneBorders & 1) != 0; }
    bool Pillarbox() { return (*pCutsceneBorders & 2) != 0; }
}

export bool IsCutsceneBorderTransitionActive()
{
    return IsCutsceneViewActive() && transition.moving;
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
            injector::MakeNOP(pattern.get_first(), 8, true);
            static auto CutsceneDimensionsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                *(int32_t*)(regs.esp + 0x24) = static_cast<int32_t>(720.0f * GetAspectRatio());
            });

            // The cutscene viewport (left, top, right, bottom)
            pattern = hook::pattern("E8 ? ? ? ? 8B 8E ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C7 86");
            static auto CutsceneViewportHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                const int32_t resX = *(int32_t*)(regs.esp + 0x34);
                const int32_t resY = *(int32_t*)(regs.esp + 0x38);

                int32_t& x = *(int32_t*)(regs.esp + 0x3C);
                int32_t& y = *(int32_t*)(regs.esp + 0x40);
                int32_t& w = *(int32_t*)(regs.esp + 0x44);
                int32_t& h = *(int32_t*)(regs.esp + 0x48);

                const CutsceneGeometry::Rect native{ x, y, w, h };
                const auto now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
                const auto view = *reinterpret_cast<const int*>(pGameMode.get() + 0x86C);
                const auto rect = transition.Update(native, resX, resY, view, *pCutsceneBorders, now);
                const auto letterbox = CutsceneGeometry::Fit(native, resX, resY, *pCutsceneBorders | 1);
                nBorderSize = (std::max)(letterbox[1] - rect[1], 0);
                x = rect[0]; y = rect[1]; w = rect[2]; h = rect[3];
            });

            // Subtitles: the cutscene style (inside the image) everywhere, placed for the whole screen
            pattern = find_pattern("E8 ? ? ? ? 85 C0 75 ? 0F 57 C0 A1", "E8 ? ? ? ? F3 0F 10 05 ? ? ? ? F3 0F 11 44 24 ? F3 0F 10 05 ? ? ? ? 89 7C 24");
            injector::MakeNOP(pattern.get_first(), 5, true);
            static auto ForceSubtitlesStyle = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if ((Pillarbox() && IsCutsceneViewActive()) || IsSplitScreenActive())
                    SetSubtitlesNow();
                regs.eax = *(uint32_t*)(regs.ecx + 0x20);
                if (regs.eax == 1)
                    regs.eax = 0;
            });

            // Subtitles' height: moved down by the removed bar
            pattern = hook::pattern("E8 ? ? ? ? 85 C0 75 ? 83 C7 ? 8B 86");
            injector::MakeNOP(pattern.get_first(), 5, true);
            static auto SubtitlesVerticalPos = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.eax = *(uint32_t*)(regs.ecx + 0x20);
                if (regs.eax == 1)
                    regs.edi += nBorderSize;
            });
        };
    }
} Cutscenes;
