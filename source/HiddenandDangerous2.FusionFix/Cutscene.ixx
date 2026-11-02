module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Cutscene;

import ComVars;

// Scripted cutscenes are framed for 16:9: the game sets a 16:9 viewport (letterboxed on 4:3) and a 16:9 camera.
// MAIN.CutsceneBorders picks the bars; the camera gets the viewport's aspect and keeps the 16:9 vertical framing.
export namespace HD2Cutscene
{
    constexpr float Authored = 16.0f / 9.0f;

    inline void* (*world)(){};
    inline std::atomic<int> mode{ 3 };
    inline std::atomic<bool> dirty{};
    inline uint8_t* rebuilt{};
    inline SafetyHookMid startRect;
    inline SafetyHookMid updateRect;

    struct Rect
    {
        int left, top, right, bottom;
    };

    inline Rect Fit(int width, int height)
    {
        const auto screen = float(width) / float(height);
        const auto borders = mode.load(std::memory_order_relaxed);
        Rect rect{ 0, 0, width, height };
        if (screen < Authored && (borders & 1))
        {
            const auto h = int(std::lround(width / Authored));
            rect.top = (height - h) / 2;
            rect.bottom = rect.top + h;
        }
        else if (screen > Authored && (borders & 2))
        {
            const auto w = int(std::lround(height * Authored));
            rect.left = (width - w) / 2;
            rect.right = rect.left + w;
        }
        return rect;
    }

    inline bool Active()
    {
        return CutsceneActive.is_initialized() && CutsceneActive != 0;
    }

    inline uint8_t* SceneCamera()
    {
        auto game = static_cast<uint8_t*>(world());
        auto scene = game ? *reinterpret_cast<uint8_t**>(game + 0x18) : nullptr;
        return scene ? *reinterpret_cast<uint8_t**>(scene + 0x15C) : nullptr;
    }

    inline float Aspect(int width, int height)
    {
        const auto rect = Fit(width, height);
        return float(rect.right - rect.left) / float(std::max(1, rect.bottom - rect.top));
    }

    // The native call takes (scene, left, top, right, bottom)
    inline void Viewport(SafetyHookContext& context, int width, int height)
    {
        const auto rect = Fit(width, height);
        auto args = reinterpret_cast<int*>(context.esp);
        args[1] = rect.left;
        args[2] = rect.top;
        args[3] = rect.right;
        args[4] = rect.bottom;
    }

    // Game thread, once per frame: a new active camera or a changed border mode rebuilds the cutscene projection
    inline void Frame(void(__stdcall* setFOV)(void*, float))
    {
        if (!Active())
        {
            rebuilt = nullptr;
            return;
        }
        auto camera = SceneCamera();
        if (!camera || *reinterpret_cast<bool*>(camera + 0x13C))
            return;
        if (camera != rebuilt || dirty.exchange(false, std::memory_order_acq_rel))
        {
            rebuilt = camera;
            setFOV(camera, *reinterpret_cast<float*>(camera + 0x120));
        }
    }

    inline void Init(const int* width, const int* height)
    {
        static const int* w = width;
        static const int* h = height;
        CutsceneActive.SetAddress(*hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B CF E8 ? ? ? ? 8B 8C 24 ? ? ? ? 5F B0 01 64 89 0D ? ? ? ? 5E 8B E5 5D C2 04 00").get_first<uint8_t*>(1) + 0x824);
        world = reinterpret_cast<void* (*)()>(injector::GetBranchDestination(
            hook::pattern("E8 ? ? ? ? 89 5D F4 C7 45 F8 00 00 00 00 DF 6D F4 D9 45 08 8B 40 18").get_first()).as_int());
        startRect = safetyhook::create_mid(hook::pattern("8B 55 F0 50 6A 00 52 FF 93 48 01 00 00").get_first(7),
            [](SafetyHookContext& context) { Viewport(context, *w, *h); });
        updateRect = safetyhook::create_mid(hook::pattern("50 6A 00 57 FF 95 48 01 00 00 5F 5E 5D 83 C4 08 C2 04 00").get_first(4),
            [](SafetyHookContext& context) { Viewport(context, *w, *h); });
        const auto update = []()
        {
            mode.store(std::clamp(std::stoi(FixSettings().Get("MAIN.CutsceneBorders")), 0, 3), std::memory_order_relaxed);
            dirty.store(true, std::memory_order_release);
        };
        update();
        FixSettings().OnChange("MAIN.CutsceneBorders", update);
    }
}
