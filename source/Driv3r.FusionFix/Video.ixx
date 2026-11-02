module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"


export module Video;

import ComVars;

export namespace Driv3rVideo
{
    struct Frame
    {
        float x = 0, y = 0, width = 1, height = 1;
        float u = 0, v = 0, uScale = 1, vScale = 1;
    };

    Frame Fit(float sourceAspect, float screenAspect, int mode)
    {
        Frame frame;
        if (!std::isfinite(sourceAspect) || !std::isfinite(screenAspect) || sourceAspect <= 0 || screenAspect <= 0)
            return frame;
        // The whole movie at its own aspect: Original inside the game's centred 4:3 area, Fill inside the whole screen
        const float area = mode == 0 ? 4.0f / 3.0f : screenAspect;
        frame.width = std::min(1.0f, area / screenAspect) * std::min(1.0f, sourceAspect / area);
        frame.height = std::min(1.0f, screenAspect / area) * std::min(1.0f, area / sourceAspect);
        frame.x = (1.0f - frame.width) * 0.5f;
        frame.y = (1.0f - frame.height) * 0.5f;
        return frame;
    }

    SafetyHookMid drawHook, endHook;
    thread_local bool drawing = false;
    bool ready = false;

    void Install()
    {
        FixSettings().Availability("MAIN.FMVWidescreenMode", [](std::string_view) { return ready; });
        auto site = hook::pattern("D9 46 28 D8 76 30 D8 4E 38 D9 5C 24 5C E8 ? ? ? ? 8B 46 0C 85 C0 74 06 57 E8");
        drawHook = safetyhook::create_mid(site.get_first(13), [](SafetyHookContext& regs)
        {
            drawing = true;
            auto* player = reinterpret_cast<const uint8_t*>(regs.esi);
            const float width = *reinterpret_cast<const float*>(player + 36);
            const float height = *reinterpret_cast<const float*>(player + 40);
            const int screenWidth = BackbufferWidth, screenHeight = BackbufferHeight;
            if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0 || screenWidth <= 0 || screenHeight <= 0)
                return;
            const auto frame = Fit(width / height, static_cast<float>(screenWidth) / screenHeight, FixSettings().Int("MAIN.FMVWidescreenMode"));
            auto* quad = *reinterpret_cast<float**>(regs.eax + 8);
            const float u = quad[10], v = quad[11];
            quad[4] = frame.x;
            quad[5] = frame.y;
            quad[6] = frame.width;
            quad[7] = frame.height;
            quad[8] = frame.u * u;
            quad[9] = frame.v * v;
            quad[10] = (frame.u + frame.uScale) * u;
            quad[11] = (frame.v + frame.vScale) * v;
        });
        endHook = safetyhook::create_mid(site.get_first(18), [](SafetyHookContext& regs) { drawing = false; });
        ready = drawHook && endHook;
        if (!ready)
        {
            drawHook.reset();
            endHook.reset();
            OutputDebugStringA("Driv3r: fullscreen movie draw hook unavailable.\n");
        }
    }
}
