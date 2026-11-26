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

// MAIN.FMVWidescreenMode (live): 0 Original plays the movies inside the frame chosen by the game's Display option,
// 1 Fill gives a playing movie the whole screen. Both keep the whole movie at its own aspect.
export namespace REVideo
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
    // The draw context's vertex shader constants
    constexpr size_t ContextConstants = 3276;
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
    inline bool ready = false;

    inline bool Transform(float* vertices, int width, int height, int targetWidth, int targetHeight)
    {
        if (!vertices || width <= 0 || height <= 0 || targetWidth <= 0 || targetHeight <= 0 ||
            width > 16384 || height > 16384 || targetWidth > 16384 || targetHeight > 16384) return false;
        const float ratio = (float(width) * targetHeight) / (float(height) * targetWidth);
        // The whole movie at its own aspect within the target: the game's frame (Original) or the screen (Fill)
        const float x = std::min(1.0f, ratio);
        const float y = std::min(1.0f, 1.0f / ratio);
        for (size_t i = 0; i < 4; ++i)
        {
            REText::Write(vertices, 16 * i, vertices[4 * i] * x);
            REText::Write(vertices, 16 * i + 4, vertices[4 * i + 1] * y);
        }
        return true;
    }

    inline void __fastcall Draw(void* movie, void*, void* context)
    {
        const auto resource = REText::Read<void*>(movie, 80);
        const auto width = REText::Read<int>(movie, 88), height = REText::Read<int>(movie, 92);
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
        const auto state = REText::Read<uint32_t>(unit, 12) & 7;
        if (fullscreenCount.load() && state != 1 && state != 2)
        {
            const std::lock_guard lock(mutex);
            fullscreenMovies.erase(unit);
            fullscreenCount.store(fullscreenMovies.size());
        }
        return result;
    }

    inline uint32_t __fastcall Aspect(void* renderer, void*, uint32_t selected)
    {
        {
            const std::lock_guard lock(aspectMutex);
            if (expanded)
            {
                // A Fill movie owns the whole screen until the game's scene resumes
                chosen = selected;
                return 0;
            }
        }
        return aspectHook.unsafe_thiscall<uint32_t>(renderer, selected);
    }

    // On the game's update thread
    inline void Scene()
    {
        if (!ready) return;
        void* renderer = sRender.get();
        if (!renderer || !REText::Read<void*>(renderer, 0x98) || (REText::Read<uint32_t>(renderer, 0x433E84) & 5)) return;
        const bool active = fullscreenCount.load() != 0 && mode.load() == 1;
        uint32_t requested = 0;
        uint32_t previous = 0;
        {
            const std::lock_guard lock(aspectMutex);
            if (active == expanded) return;
            previous = REText::Read<uint32_t>(renderer, 228);
            if (active)
            {
                if (previous == 0 || previous > 3 ||
                    (REText::Read<int>(renderer, 184) == REText::Read<int>(renderer, 0x433E50) &&
                     REText::Read<int>(renderer, 188) == REText::Read<int>(renderer, 0x433E54))) return;
                chosen = previous;
            }
            else
                requested = chosen;
        }
        // The game's setter can wait for the render thread: called without the lock
        if (aspectHook.unsafe_thiscall<uint32_t>(renderer, requested) == static_cast<uint32_t>(D3DERR_DEVICELOST))
        {
            // The setter stores the new value before checking the device: rolled back so the next update retries
            REText::Write(renderer, 228, previous);
            return;
        }
        const std::lock_guard lock(aspectMutex);
        expanded = active;
    }

    inline int __fastcall Composite(void* unit, void*, void* context)
    {
        const auto previous = image;
        image = {};
        if (REText::Read<uint16_t>(unit, 48) == 6)
        {
            const auto resource = REText::Read<void*>(unit, 80);
            const std::lock_guard lock(mutex);
            for (size_t i = 0; i < count.load(); ++i)
                if (targets[i].resource == resource) { image = targets[i]; break; }
        }
        const auto result = compositeHook.thiscall<int>(unit, context);
        image = previous;
        return result;
    }

    inline int __cdecl Quad(void* context, const RECT* gameViewport, const int* source, const int* size)
    {
        if (!image.resource || !source || !size || source[0] || source[1] ||
            source[2] != size[0] || source[3] != size[1])
            return quadHook.call<int>(context, gameViewport, source, size);
        const auto texture = REText::Read<void*>(image.resource, 212);
        const auto textureSize = texture ? REText::Read<uint32_t>(texture, 32) : 0;
        // The intermediate texture follows the scene resolution, independently of the decoded video
        if (size[0] <= 0 || size[1] <= 0 || size[0] != int(textureSize & 65535) ||
            size[1] != int((textureSize >> 16) & 16383))
            return quadHook.call<int>(context, gameViewport, source, size);
        const auto packed = REText::Read<uint32_t>(context, 252);
        const int width = packed & 65535, height = packed >> 16;
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
            return quadHook.call<int>(context, gameViewport, source, size);
        if (image.unit)
        {
            const std::lock_guard lock(mutex);
            const auto state = REText::Read<uint32_t>(image.unit, 12) & 7;
            if (state == 1 || state == 2) fullscreenMovies.insert(image.unit);
            fullscreenCount.store(fullscreenMovies.size());
        }
        const auto saved = REText::Read<RECT>(context, 188);
        const auto savedViewport = REText::Read<std::array<int32_t, 8>>(context, 188);
        const auto savedConstants = REText::Read<void*>(context, ContextConstants);
        const RECT full{ 0, 0, width, height };
        const auto previousWidth = targetWidth, previousHeight = targetHeight;
        targetWidth = width;
        targetHeight = height;
        REText::Write(context, 196, -1);
        viewport(context, &full);
        const auto result = quadHook.call<int>(context, &full, source, size);
        viewport(context, &saved);
        REText::Write(context, 188, savedViewport);
        if (savedConstants && REText::Read<void*>(context, ContextConstants) != savedConstants)
        {
            REText::Write(context, ContextConstants, savedConstants);
            REText::Write(context, 324, REText::Read<uint32_t>(context, 324) | 1u);
        }
        targetWidth = previousWidth;
        targetHeight = previousHeight;
        return result;
    }

    // Once per presented frame and before a device reset
    inline void Frame()
    {
        if (!count.load()) return;
        const std::lock_guard lock(mutex);
        count.store(0);
    }

    inline void Install()
    {
        ready = false;
        BindLiveInt("MAIN.FMVWidescreenMode", [](int value) { mode.store(std::clamp(value, 0, 1)); });
        // The current executable and the original 2015 one were built by different compilers
        auto draw = hook::pattern("83 EC 24 53 55 8B E9 56 8B 74 24 34 57 83 7D 50 00 89 6C 24 20 0F 84");
        const bool current = draw.size() == 1;
        if (!current)
            draw = hook::pattern("83 EC 18 53 56 57 8B F9 83 7F 50 00 0F 84 ? ? ? ? 8B 74 24 28 6A 00 6A 00 8B CE");
        auto composite = current ?
            hook::pattern("55 8B EC 83 E4 F0 81 EC C8 02 00 00 56 8B 75 08 57 8B F9 56 89 7C 24 70 E8 ? ? ? ? 83 7F 50") :
            hook::pattern("55 8B EC 83 E4 F0 81 EC 48 05 00 00 56 8B 75 08 57 8B F9 56 89 BC 24 F0 00 00 00 E8");
        auto quad = current ?
            hook::pattern("51 56 8B 74 24 0C 81 BE B8 01 00 00 09 70 C1 D6") :
            hook::pattern("83 EC 0C 56 8B 74 24 14 81 BE B8 01 00 00 09 70 C1 D6");
        auto view = current ?
            hook::pattern("8B 54 24 04 83 EC 1C 8B 42 08 56 8B F1 3B 86 C4 00 00 00 75 24") :
            hook::pattern("83 EC 18 56 57 8B 7C 24 24 8B F1 8B 47 08 3B 86 C4 00 00 00 75 24");
        auto aspect = current ?
            hook::pattern("8B 44 24 04 39 81 E4 00 00 00 74 ? 80 79 22 00 89 81 E4 00 00 00") :
            hook::pattern("8B 44 24 04 56 8B F1 39 86 E4 00 00 00 0F 84 ? ? ? ? 80 7E 22 00 89 86 E4 00 00 00 8B 0D");
        auto release = hook::pattern("8B 51 0C 8B C2 C1 E8 0D A8 01 75 18 8B C2 83 E0 07 83 F8 02 74 05 83 F8 01 75 09 83 E2 FB 83 CA 03 89 51 0C C3");
        viewport = reinterpret_cast<Viewport>(view.get_first());
        const auto quadAddress = quad.get_first<uint8_t>();
        // Where the quad's four vertices are written: eax (current) or edx (original) points at them
        const auto end = hook::range_pattern(reinterpret_cast<uintptr_t>(quadAddress), reinterpret_cast<uintptr_t>(quadAddress) + 768,
            current ? "8B CE 5E 83 C4 04 E9" : "89 42 3C 8B CE 5E").count(1).get_first<uint8_t>(current ? 0 : 3);
        const auto drawAddress = draw.get_first();
        const auto compositeAddress = composite.get_first();
        const auto releaseAddress = release.get_first();
        const auto aspectAddress = aspect.get_first();
        verticesHook = safetyhook::create_mid(end, current ?
            static_cast<safetyhook::MidHookFn>([](SafetyHookContext& context)
            {
                if (!image.resource || !targetWidth || !targetHeight) return;
                Transform(reinterpret_cast<float*>(context.eax), image.width, image.height, targetWidth, targetHeight);
            }) :
            static_cast<safetyhook::MidHookFn>([](SafetyHookContext& context)
            {
                if (!image.resource || !targetWidth || !targetHeight) return;
                Transform(reinterpret_cast<float*>(context.edx), image.width, image.height, targetWidth, targetHeight);
            }));
        quadHook = safetyhook::create_inline(quadAddress, Quad);
        compositeHook = safetyhook::create_inline(compositeAddress, Composite);
        drawHook = safetyhook::create_inline(drawAddress, Draw);
        releaseHook = safetyhook::create_inline(releaseAddress, Release);
        aspectHook = safetyhook::create_inline(aspectAddress, Aspect);
        ready = true;
    }
}
