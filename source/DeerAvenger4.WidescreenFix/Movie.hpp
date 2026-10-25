#pragma once
#include <ddraw.h>
#include "NativeMenuContext.hpp"

namespace DeerMovie
{
    struct Layout
    {
        RECT source{}, destination{};
    };

    inline SafetyHookMid branchHook;
    inline SafetyHookInline drawHook;
    inline void** movieRaster = nullptr;
    inline void** targetRaster = nullptr;
    inline int* extensionOffset = nullptr;
    inline uint8_t* (__cdecl* lockRaster)(void*, int, int) = nullptr;
    inline void* (__cdecl* unlockRaster)(void*) = nullptr;

    inline bool Geometry(int sw, int sh, int dw, int dh, int mode, Layout& result)
    {
        if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0 || sw > 16384 || sh > 16384 || dw > 16384 || dh > 16384)
            return false;
        result = { { 0, 0, sw, sh }, { 0, 0, dw, dh } };
        if (mode == 0)
        {
            int w = dw, h = dh;
            if (int64_t(sw) * dh > int64_t(dw) * sh)
                h = (std::max)(1, int(int64_t(dw) * sh / sw));
            else
                w = (std::max)(1, int(int64_t(dh) * sw / sh));
            const int x = (dw - w) / 2, y = (dh - h) / 2;
            result.destination = { x, y, x + w, y + h };
        }
        else if (mode == 1)
        {
            int w = sw, h = sh;
            if (int64_t(sw) * dh > int64_t(dw) * sh)
                w = (std::max)(1, int(int64_t(sh) * dw / dh));
            else
                h = (std::max)(1, int(int64_t(sw) * dh / dw));
            const int x = (sw - w) / 2, y = (sh - h) / 2;
            result.source = { x, y, x + w, y + h };
        }
        return true;
    }

    inline bool Raster(void* raster)
    {
        const auto address = reinterpret_cast<uintptr_t>(raster);
        return DeerMenu::Readable(raster, 52) &&
            DeerMenu::Read<void*>(address) == raster &&
            DeerMenu::Read<int16_t>(address + 28) == 0 && DeerMenu::Read<int16_t>(address + 30) == 0;
    }

    inline IDirectDrawSurface7* Surface(void* raster)
    {
        const int offset = extensionOffset ? *extensionOffset : 0;
        if (offset < 52 || offset > 65536)
            return nullptr;
        auto surface = DeerMenu::Read<IDirectDrawSurface7*>(reinterpret_cast<uintptr_t>(raster) + offset + 4);
        auto table = DeerMenu::Read<uintptr_t>(reinterpret_cast<uintptr_t>(surface));
        return DeerMenu::Readable(reinterpret_cast<void*>(table), 6 * sizeof(void*)) ? surface : nullptr;
    }

    template<size_t Bytes> inline void Copy(uint8_t* target, int targetPitch, const uint8_t* source, int sourcePitch, const Layout& layout)
    {
        const auto& s = layout.source;
        const auto& d = layout.destination;
        const int sw = s.right - s.left, sh = s.bottom - s.top;
        const int dw = d.right - d.left, dh = d.bottom - d.top;
        for (int y = 0; y < dh; ++y)
        {
            const int sy = s.top + int(int64_t(y) * sh / dh);
            auto output = target + size_t(d.top + y) * targetPitch + size_t(d.left) * Bytes;
            auto input = source + size_t(sy) * sourcePitch;
            for (int x = 0; x < dw; ++x)
            {
                const int sx = s.left + int(int64_t(x) * sw / dw);
                std::memcpy(output + size_t(x) * Bytes, input + size_t(sx) * Bytes, Bytes);
            }
        }
    }

    inline bool Software(void* source, void* target, const Layout& layout)
    {
        if (!lockRaster || !unlockRaster || source == target)
            return false;
        const auto src = reinterpret_cast<uintptr_t>(source), dst = reinterpret_cast<uintptr_t>(target);
        const int depth = DeerMenu::Read<int>(src + 20);
        if ((depth != 16 && depth != 24 && depth != 32) || depth != DeerMenu::Read<int>(dst + 20))
            return false;
        const auto input = lockRaster(source, 0, 2);
        if (!input)
            return false;
        const auto output = lockRaster(target, 0, 1);
        bool copied = false;
        if (output)
        {
            const int sourcePitch = DeerMenu::Read<int>(src + 24), targetPitch = DeerMenu::Read<int>(dst + 24);
            const auto& s = layout.source;
            const auto& d = layout.destination;
            const int bytes = depth / 8;
            if (s.left >= 0 && s.top >= 0 && s.right > s.left && s.bottom > s.top &&
                d.left >= 0 && d.top >= 0 && d.right > d.left && d.bottom > d.top &&
                s.right <= 16384 && s.bottom <= 16384 && d.right <= 16384 && d.bottom <= 16384 &&
                sourcePitch >= s.right * bytes && targetPitch >= d.right * bytes &&
                s.bottom <= DeerMenu::Read<int>(src + 16) && d.bottom <= DeerMenu::Read<int>(dst + 16) &&
                sourcePitch <= INT_MAX / s.bottom && targetPitch <= INT_MAX / d.bottom &&
                DeerMenu::Readable(input, size_t(sourcePitch) * s.bottom) &&
                DeerMenu::Readable(output, size_t(targetPitch) * d.bottom))
            {
                if (bytes == 2) Copy<2>(output, targetPitch, input, sourcePitch, layout);
                if (bytes == 3) Copy<3>(output, targetPitch, input, sourcePitch, layout);
                if (bytes == 4) Copy<4>(output, targetPitch, input, sourcePitch, layout);
                copied = true;
            }
            unlockRaster(target);
        }
        unlockRaster(source);
        return copied;
    }

    inline bool Render(void* source, void* target, const Layout& layout)
    {
        auto input = Surface(source), output = Surface(target);
        if (input && output)
        {
            auto s = layout.source, d = layout.destination;
            if (SUCCEEDED(output->Blt(&d, input, &s, DDBLT_WAIT, nullptr)))
                return true;
        }
        return Software(source, target, layout);
    }

    inline void* __cdecl Draw(void* raster, const void* nativeRect)
    {
        auto target = targetRaster ? *targetRaster : nullptr;
        if (!movieRaster || raster != *movieRaster || !Raster(raster) || !Raster(target))
            return drawHook.ccall<void*>(raster, nativeRect);
        auto src = reinterpret_cast<uintptr_t>(raster), dst = reinterpret_cast<uintptr_t>(target);
        Layout layout;
        if (!Geometry(DeerMenu::Read<int>(src + 12), DeerMenu::Read<int>(src + 16),
            DeerMenu::Read<int>(dst + 12), DeerMenu::Read<int>(dst + 16),
            std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 2), layout))
            return drawHook.ccall<void*>(raster, nativeRect);
        if (!Render(raster, target, layout))
        {
            Geometry(DeerMenu::Read<int>(src + 12), DeerMenu::Read<int>(src + 16),
                DeerMenu::Read<int>(dst + 12), DeerMenu::Read<int>(dst + 16), 0, layout);
            const auto& d = layout.destination;
            const int fitted[]{ d.left, d.top, d.right - d.left, d.bottom - d.top };
            return drawHook.ccall<void*>(raster, fitted);
        }
        return raster;
    }

    inline bool Init(HMODULE game = GetModuleHandleW(nullptr))
    {
        auto branch = hook::pattern(game, "A1 ? ? ? ? 83 C4 18 85 C0 74 16 8B 0D ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 83 C4 08 EB 1C");
        auto draw = hook::pattern(game, "53 8B 5C 24 08 83 EC 10 89 1C 24 8B 44 24 1C C7 44 24 08 00 00 00 00 89 44 24 04 A1 ? ? ? ? FF 90 90 00 00 00");
        auto scaled = hook::pattern(game, "83 EC 0C A1 ? ? ? ? 8B 15 ? ? ? ? 8B 44 10 04 85 C0 74 16 8B 44 24 10 8B 54 24 14 E8");
        auto lock = hook::pattern(game, "83 EC 18 33 C0 8A 44 24 20 8B 54 24 24 8B 4C 24 1C 89 7C 24 14 8D 7C 24 10 89 4C 24 04 89 3C 24 C1 E0 08");
        auto unlock = hook::pattern(game, "53 8B 5C 24 08 83 EC 10 33 C0 89 5C 24 04 89 44 24 08 8B 15 ? ? ? ? 89 04 24 FF 92 88 00 00 00");
        if (branch.size() != 1 || draw.size() != 1 || scaled.size() != 2 || lock.size() != 1 || unlock.size() != 1)
            return false;
        if (*scaled.get(0).get<int*>(4) != *scaled.get(1).get<int*>(4) ||
            *scaled.get(0).get<void**>(10) != *scaled.get(1).get<void**>(10))
            return false;
        movieRaster = *branch.get_first<void**>(14);
        extensionOffset = *scaled.get_first<int*>(4);
        targetRaster = *scaled.get_first<void**>(10);
        lockRaster = reinterpret_cast<decltype(lockRaster)>(lock.get_first());
        unlockRaster = reinterpret_cast<decltype(unlockRaster)>(unlock.get_first());
        branchHook = safetyhook::create_mid(branch.get_first(5), [](SafetyHookContext& context) { context.eax = 1; });
        drawHook = safetyhook::create_inline(draw.get_first(), Draw);
        if (!drawHook || !branchHook)
        {
            drawHook.reset(); branchHook.reset();
            return false;
        }
        return true;
    }
}
