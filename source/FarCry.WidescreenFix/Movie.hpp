#pragma once

namespace FarCryMovie
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
        if (mode == 0)
        {
            if (sourceAspect > aspect)
                result.image.height *= aspect / sourceAspect;
            else
                result.image.width *= sourceAspect / aspect;
            result.image.x += (canvas.width - result.image.width) * 0.5f;
            result.image.y += (canvas.height - result.image.height) * 0.5f;
        }
        else if (mode == 1)
        {
            if (sourceAspect > aspect)
            {
                const auto half = (u1 - u0) * aspect / sourceAspect * 0.5f;
                const auto center = (u0 + u1) * 0.5f;
                result.u0 = center - half;
                result.u1 = center + half;
            }
            else
            {
                const auto half = (v1 - v0) * sourceAspect / aspect * 0.5f;
                const auto center = (v0 + v1) * 0.5f;
                result.v0 = center - half;
                result.v1 = center + half;
            }
        }
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
        if (aspect.size() != 1)
        {
            OutputDebugStringA("Far Cry: movie aspect pattern unavailable.\n");
            return false;
        }
        AspectContinue = reinterpret_cast<uintptr_t>(aspect.get_first(6));
        AspectHook = safetyhook::create_mid(aspect.get_first(), [](SafetyHookContext& regs)
        {
            // Leave the full widget rectangle for the common movie layout.
            regs.ecx &= 0xFFFFFF00;
            regs.eip = AspectContinue;
        });
        Ready = bool(AspectHook);
        if (!Ready)
            OutputDebugStringA("Far Cry: movie aspect hook unavailable.\n");
#else
        auto movie = hook::module_pattern(module, "48 83 BB ? ? ? ? ? 66 44 0F 6F 94 24 ? ? ? ? 66 44 0F 6F 8C 24 ? ? ? ? 66 0F 6F BC 24");
        Ready = movie.size() == 1;
        if (!Ready)
            OutputDebugStringA("Far Cry: movie widget pattern unavailable.\n");
#endif
        return Ready;
    }
}
