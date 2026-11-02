module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"


export module Video;

import ComVars;

export namespace DPLVideo
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
        auto site = hook::pattern("F3 0F 11 45 DC C7 45 E0 01 00 00 00 FF 50 40 8B 4B 0C 85 C9 74 06");
        drawHook = safetyhook::create_mid(site.get_first(12), [](SafetyHookContext& regs)
        {
            drawing = true;
            auto* quad = reinterpret_cast<float*>(regs.ebp - 0x50);
            const int width = BackbufferWidth, height = BackbufferHeight;
            if (width <= 0 || height <= 0 || !std::isfinite(quad[6]) || !std::isfinite(quad[7]) || quad[6] <= 0 || quad[7] <= 0)
                return;
            // Native movie geometry includes the authored non-square pixel aspect.
            const float aspect = (16.0f / 9.0f) * quad[6] / quad[7];
            const auto frame = Fit(aspect, static_cast<float>(width) / height, FixSettings().Int("MAIN.FMVWidescreenMode"));
            const float u = quad[10], v = quad[11];
            quad[4] = frame.x; quad[5] = frame.y;
            quad[6] = frame.width; quad[7] = frame.height;
            quad[8] = frame.u * u; quad[9] = frame.v * v;
            quad[10] = (frame.u + frame.uScale) * u;
            quad[11] = (frame.v + frame.vScale) * v;
        });
        endHook = safetyhook::create_mid(site.get_first(18), [](SafetyHookContext&) { drawing = false; });
        ready = drawHook && endHook;
        if (!ready)
        {
            drawHook.reset(); endHook.reset();
            OutputDebugStringA("DriverParallelLines: native movie draw hooks unavailable.\n");
        }
    }
}
