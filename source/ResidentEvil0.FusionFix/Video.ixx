module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <array>
#include <mutex>
#include <set>
#include <d3d9.h>

export module Video;

import ComVars;
import EngineText;

export namespace RE0Video
{
    inline std::atomic<int> mode{ 1 };
    inline SafetyHookInline drawHook;
    inline SafetyHookInline compositeHook;
    inline SafetyHookInline quadHook;
    inline SafetyHookInline releaseHook;
    inline SafetyHookInline aspectHook;
    inline SafetyHookMid verticesHook;
    using Viewport = int(__thiscall*)(void*, const RECT*);
    inline Viewport viewport = nullptr;
    struct Target
    {
        void* resource = nullptr;
        int width = 0, height = 0;
        void* unit = nullptr;
    };
    inline std::array<Target, 8> targets;
    inline std::atomic<size_t> count{ 0 };
    inline std::mutex mutex;
    inline std::set<void*> fullscreenMovies;
    inline std::atomic<size_t> fullscreenCount{ 0 };
    inline bool expanded = false;
    inline uint32_t chosen = 0;
    inline std::mutex aspectMutex;
    inline thread_local Target image;
    inline thread_local int targetWidth = 0, targetHeight = 0;
    inline std::atomic<uint32_t> lastSourceSize{ 0 }, lastTargetSize{ 0 };
    inline bool ready = false;

    inline bool Transform(float* vertices, int width, int height, int targetWidth, int targetHeight, int selected)
    {
        if (!vertices || width <= 0 || height <= 0 || targetWidth <= 0 || targetHeight <= 0 ||
            width > 16384 || height > 16384 || targetWidth > 16384 || targetHeight > 16384 ||
            selected < 0 || selected > 1) return false;
        const float ratio = (float(width) * targetHeight) / (float(height) * targetWidth);
        // The whole movie at its own aspect within the target: the game's frame (Original) or the screen (Fill)
        const float x = std::min(1.0f, ratio);
        const float y = std::min(1.0f, 1.0f / ratio);
        const float u = 1.0f;
        const float v = 1.0f;
        for (size_t i = 0; i < 4; ++i)
        {
            RE0Text::Write(vertices, 16 * i, vertices[4 * i] * x);
            RE0Text::Write(vertices, 16 * i + 4, vertices[4 * i + 1] * y);
            RE0Text::Write(vertices, 16 * i + 8, 0.5f + (vertices[4 * i + 2] - 0.5f) * u);
            RE0Text::Write(vertices, 16 * i + 12, 0.5f + (vertices[4 * i + 3] - 0.5f) * v);
        }
        return true;
    }

    inline void __fastcall Draw(void* movie, void*, void* context)
    {
        const auto resource = RE0Text::Read<void*>(movie, 80);
        const auto width = RE0Text::Read<int>(movie, 88), height = RE0Text::Read<int>(movie, 92);
        if (resource && width > 0 && height > 0 && width <= 16384 && height <= 16384)
        {
            const std::lock_guard lock(mutex);
            size_t i = 0;
            while (i < count.load() && targets[i].resource != resource) ++i;
            if (i < targets.size())
            {
                targets[i] = { resource, width, height, movie };
                if (i == count.load()) count.store(i + 1);
            }
        }
        drawHook.thiscall<void>(movie, context);
    }

    inline uint32_t __fastcall Release(void* unit, void*)
    {
        const auto result = releaseHook.thiscall<uint32_t>(unit);
        const auto state = RE0Text::Read<uint32_t>(unit, 12) & 7;
        if (fullscreenCount.load() && state != 1 && state != 2)
        {
            const std::lock_guard lock(mutex);
            fullscreenMovies.erase(unit);
            fullscreenCount.store(fullscreenMovies.size());
        }
        return result;
    }

    inline uint32_t __fastcall Aspect(void* value, void*, uint32_t selected)
    {
        const std::lock_guard lock(aspectMutex);
        if (expanded)
        {
            // A Fill movie owns the whole screen; the game's choice applies when it ends
            chosen = selected;
            return 0;
        }
        return aspectHook.thiscall<uint32_t>(value, selected);
    }

    inline void Scene()
    {
        if (!renderer.is_initialized() || !renderer) return;
        void* value = renderer;
        const bool active = fullscreenCount.load() != 0 && mode.load() == 1;
        const std::lock_guard lock(aspectMutex);
        if (active == expanded) return;
        if (active)
        {
            // Fill: the scene uses the whole output while the movie plays, unless the game's frame already does
            const auto selected = RE0Text::Read<uint32_t>(value, 228);
            if (selected == 0 || selected > 3 ||
                (RE0Text::Read<int>(value, 184) == RE0Text::Read<int>(value, 0x433E50) &&
                 RE0Text::Read<int>(value, 188) == RE0Text::Read<int>(value, 0x433E54))) return;
            chosen = selected;
            aspectHook.thiscall<uint32_t>(value, 0u);
            expanded = true;
        }
        else
        {
            expanded = false;
            aspectHook.thiscall<uint32_t>(value, chosen);
        }
    }

    inline int __fastcall Composite(void* unit, void*, void* context)
    {
        const auto previous = image;
        image = {};
        if (RE0Text::Read<uint16_t>(unit, 48) == 6)
        {
            const auto resource = RE0Text::Read<void*>(unit, 80);
            const std::lock_guard lock(mutex);
            for (size_t i = 0; i < count.load(); ++i)
                if (targets[i].resource == resource) { image = targets[i]; break; }
        }
        const auto result = compositeHook.thiscall<int>(unit, context);
        image = previous;
        return result;
    }

    inline int __cdecl Quad(void* context, const RECT* nativeViewport, const int* source, const int* size)
    {
        if (!image.resource || !source || !size || source[0] || source[1] ||
            source[2] != size[0] || source[3] != size[1])
            return quadHook.call<int>(context, nativeViewport, source, size);
        const auto texture = RE0Text::Read<void*>(image.resource, 212);
        const auto textureSize = texture ? RE0Text::Read<uint32_t>(texture, 32) : 0;
        // The intermediate texture follows the scene resolution, independently of the decoded video.
        if (size[0] <= 0 || size[1] <= 0 || size[0] != int(textureSize & 65535) ||
            size[1] != int((textureSize >> 16) & 16383))
            return quadHook.call<int>(context, nativeViewport, source, size);
        const auto packed = RE0Text::Read<uint32_t>(context, 252);
        const int width = packed & 65535, height = packed >> 16;
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
            return quadHook.call<int>(context, nativeViewport, source, size);
        if (image.unit)
        {
            const std::lock_guard lock(mutex);
            const auto state = RE0Text::Read<uint32_t>(image.unit, 12) & 7;
            if (state == 1 || state == 2) fullscreenMovies.insert(image.unit);
            fullscreenCount.store(fullscreenMovies.size());
        }
        const auto saved = RE0Text::Read<RECT>(context, 188);
        const auto savedViewport = RE0Text::Read<std::array<int32_t, 8>>(context, 188);
        const auto savedConstants = RE0Text::Read<void*>(context, 3332);
        const RECT full{ 0, 0, width, height };
        const auto previousWidth = targetWidth, previousHeight = targetHeight;
        targetWidth = width;
        targetHeight = height;
        lastSourceSize.store(uint32_t(image.width) | (uint32_t(image.height) << 16));
        lastTargetSize.store(packed);
        RE0Text::Write(context, 196, -1);
        viewport(context, &full);
        const auto result = quadHook.call<int>(context, &full, source, size);
        viewport(context, &saved);
        RE0Text::Write(context, 188, savedViewport);
        if (savedConstants && RE0Text::Read<void*>(context, 3332) != savedConstants)
        {
            RE0Text::Write(context, 3332, savedConstants);
            RE0Text::Write(context, 324, RE0Text::Read<uint32_t>(context, 324) | 1u);
        }
        targetWidth = previousWidth;
        targetHeight = previousHeight;
        return result;
    }

    inline void Frame(IDirect3DDevice9* device = nullptr)
    {
        if (device && ready && device->TestCooperativeLevel() == D3D_OK) Scene();
        if (!count.load()) return;
        const std::lock_guard lock(mutex);
        count.store(0);
    }

    inline void Disable()
    {
        ready = false;
        drawHook.reset();
        compositeHook.reset();
        quadHook.reset();
        verticesHook.reset();
        releaseHook.reset();
        aspectHook.reset();
        Frame();
    }

    inline void Install()
    {
        ready = false;
        mode.store(std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1));
        FixSettings().OnChange("MAIN.FMVWidescreenMode", []()
        {
            mode.store(std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1));
        });
        auto draw = hook::pattern("83 EC 18 53 56 57 8B F9 83 7F 50 00 0F 84 ? ? ? ? 8B 74 24 28 6A 00 6A 00 8B CE");
        auto composite = hook::pattern("55 8B EC 83 E4 F0 81 EC 48 05 00 00 56 8B 75 08 57 8B F9 56 89 BC 24 F0 00 00 00 E8");
        auto quad = hook::pattern("83 EC 0C 56 8B 74 24 14 81 BE B8 01 00 00 09 70 C1 D6");
        auto view = hook::pattern("83 EC 18 56 57 8B 7C 24 24 8B F1 8B 47 08 3B 86 C4 00 00 00 75 24");
        auto release = hook::pattern("8B 51 0C 8B C2 C1 E8 0D A8 01 75 18 8B C2 83 E0 07 83 F8 02 74 05 83 F8 01 75 09 83 E2 FB 83 CA 03 89 51 0C C3");
        auto aspect = hook::pattern("8B 44 24 04 56 8B F1 39 86 E4 00 00 00 0F 84 ? ? ? ? 80 7E 22 00 89 86 E4 00 00 00 8B 0D");
        auto render = hook::pattern("3D 69 08 76 88 74 11 8B 0D ? ? ? ? 68 ? ? ? ? 50 E8");
        renderer.SetAddress(*render.get_first<void**>(9));
        viewport = reinterpret_cast<Viewport>(view.get_first());
        const auto quadAddress = quad.get_first<uint8_t>();
        const auto end = hook::range_pattern(reinterpret_cast<uintptr_t>(quadAddress), reinterpret_cast<uintptr_t>(quadAddress) + 768,
            "89 42 3C 8B CE 5E").count(1).get_first<uint8_t>();
        const auto drawAddress = draw.get_first();
        const auto compositeAddress = composite.get_first();
        const auto releaseAddress = release.get_first();
        const auto aspectAddress = aspect.get_first();
        verticesHook = safetyhook::create_mid(end + 3, [](SafetyHookContext& context)
        {
            if (!image.resource || !targetWidth || !targetHeight) return;
            Transform(reinterpret_cast<float*>(context.edx), image.width, image.height,
                targetWidth, targetHeight, mode.load());
        });
        quadHook = safetyhook::create_inline(quadAddress, Quad);
        compositeHook = safetyhook::create_inline(compositeAddress, Composite);
        drawHook = safetyhook::create_inline(drawAddress, Draw);
        releaseHook = safetyhook::create_inline(releaseAddress, Release);
        aspectHook = safetyhook::create_inline(aspectAddress, Aspect);
        ready = verticesHook && quadHook && compositeHook && drawHook && releaseHook && aspectHook;
        if (!ready) { verticesHook.reset(); quadHook.reset(); compositeHook.reset(); drawHook.reset(); releaseHook.reset(); aspectHook.reset(); }
        if (!ready) OutputDebugStringA("Resident Evil 0: movie aspect hook unavailable; native playback retained.\n");
        FixSettings().Availability("MAIN.FMVWidescreenMode", [](std::string_view) { return ready; });
    }
}
