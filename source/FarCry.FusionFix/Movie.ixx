module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Movie;


export namespace FarCryMovie
{
    inline const int* Mode = nullptr;
    inline bool Ready = false;
#ifndef _WIN64
    inline SafetyHookMid AspectHook;
    inline uintptr_t AspectContinue = 0;
#endif

    struct Rectangle
    {
        float x, y, width, height;
    };

    struct Geometry
    {
        Rectangle canvas;
        Rectangle image;
        float u0, v0, u1, v1;
    };

    inline Geometry Layout(Rectangle canvas, float screenAspect, float sourceAspect, int mode,
        float u0, float v0, float u1, float v1)
    {
        Geometry result{ canvas, canvas, u0, v0, u1, v1 };
        if (!std::isfinite(screenAspect) || !std::isfinite(sourceAspect) || screenAspect <= 0.0f || sourceAspect <= 0.0f ||
            !std::isfinite(canvas.x) || !std::isfinite(canvas.y) || !std::isfinite(canvas.width) || !std::isfinite(canvas.height) ||
            canvas.width <= 0.0f || canvas.height <= 0.0f)
            return result;
        // Draw2dImage uses an 800x600 coordinate canvas, stretched to the render target.
        const auto aspect = canvas.width / canvas.height * screenAspect / (4.0f / 3.0f);
        // The whole movie at its own aspect: Original inside the game's centred 4:3 area, Fill inside the whole canvas
        const auto area = mode == 0 ? 4.0f / 3.0f : aspect;
        result.image.width *= std::min(1.0f, area / aspect) * std::min(1.0f, sourceAspect / area);
        result.image.height *= std::min(1.0f, aspect / area) * std::min(1.0f, area / sourceAspect);
        result.image.x += (canvas.width - result.image.width) * 0.5f;
        result.image.y += (canvas.height - result.image.height) * 0.5f;
        return result;
    }

    template<class Draw>
    inline void ClearBars(const Geometry& geometry, Draw draw)
    {
        const auto& canvas = geometry.canvas;
        const auto& image = geometry.image;
        const Rectangle bars[] = {
            { canvas.x, canvas.y, canvas.width, image.y - canvas.y },
            { canvas.x, image.y + image.height, canvas.width, canvas.y + canvas.height - image.y - image.height },
            { canvas.x, image.y, image.x - canvas.x, image.height },
            { image.x + image.width, image.y, canvas.x + canvas.width - image.x - image.width, image.height }
        };
        for (const auto& bar : bars)
        {
            if (bar.width > 0.0001f && bar.height > 0.0001f)
                draw(bar);
        }
    }

    inline bool Install(HMODULE module)
    {
#ifndef _WIN64
        if (AspectHook)
        {
            Ready = true;
            return true;
        }
#endif
        Ready = false;
#ifndef _WIN64
        auto aspect = hook::module_pattern(module, "8A 8E FB 00 00 00 D9 45 F4 3A CB");
        AspectContinue = reinterpret_cast<uintptr_t>(aspect.get_first(6));
        AspectHook = safetyhook::create_mid(aspect.get_first(), [](SafetyHookContext& regs)
        {
            // Leave the full widget rectangle for the common movie layout.
            regs.ecx &= 0xFFFFFF00;
            regs.eip = AspectContinue;
        });
#endif
        Ready = true;
        return Ready;
    }
}
