module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include "Hooking.Patterns.h"
#include "gameref.hpp"
#include "safetyhook.hpp"

export module Camera;

import Frontend;
import ComVars;

export namespace DeerCamera
{
    inline bool applying = false;
    inline SafetyHookMid viewHook;
    inline SafetyHookInline beginHook;
    inline void* (__cdecl* setView)(void*, const float*) = nullptr;
    inline int (__cdecl* setCanvasCamera)(void*) = nullptr;

    struct View
    {
        void* camera = nullptr;
        float x = 0.0f;
        float y = 0.0f;
        float lastX = 0.0f;
        float lastY = 0.0f;
        bool adjusted = false;
    };
    inline View view;

    inline bool Extents(float x, float y)
    {
        return std::isfinite(x) && std::isfinite(y) && x > 0.0f && y > 0.0f;
    }

    inline float Aspect(void* camera)
    {
        auto raster = reinterpret_cast<uintptr_t>(DeerMenu::Read<void*>(reinterpret_cast<uintptr_t>(camera) + 96));
        if (!raster)
            return 0.0f;
        int width = DeerMenu::Read<int>(raster + 12);
        int height = DeerMenu::Read<int>(raster + 16);
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
            return 0.0f;
        return float(width) / float(height);
    }

    inline void Correct(void* camera)
    {
        if (applying || !camera || !cameraPrimary.is_initialized() || cameraPrimary.get() != camera)
            return;
        auto address = reinterpret_cast<uintptr_t>(camera);
        float x = DeerMenu::Read<float>(address + 104);
        float y = DeerMenu::Read<float>(address + 108);
        float aspect = Aspect(camera);
        if (!Extents(x, y) || aspect <= 0.0f)
        {
            view = {};
            return;
        }
        // A native reapplication of the current extent must not become a new base.
        if (view.camera == camera && view.adjusted && x == view.lastX && y == view.lastY)
            return;
        view = { camera, x, y };
        if (std::abs(x / y - 4.0f / 3.0f) <= 0.001f)
            *reinterpret_cast<float*>(address + 104) = y * aspect;
    }

    inline bool Gameplay()
    {
        // All of these are game globals checked once in Init.
        return game_scope.is_initialized() && game_scripted.is_initialized() && game_followBlocked.is_initialized() && game_controlBlocked.is_initialized() &&
            game_followBlocked.get() == 0 && game_controlBlocked.get() == 0 && game_scope.get() == 0 && game_scripted.get() == 0 &&
            (DeerMenu::Game(), game_worldState.get()) == 1 && (DeerMenu::Game(), game_demoState.get()) == 0 &&
            (DeerMenu::Game(), game_levelState.get()) != 0 && (DeerMenu::Game(), game_transitionState.get()) == 0 &&
            (DeerMenu::Game(), game_consoleState.get()) == 0;
    }

    inline void Update(void* camera)
    {
        if (!setView || !camera || camera != view.camera || !cameraPrimary.is_initialized() ||
            cameraPrimary.get() != camera)
            return;
        const float wanted = FixSettings().Float("MAIN.FOVFactor");
        if (wanted == 1.0f && !view.adjusted)
            return;
        float aspect = Aspect(camera);
        if (aspect <= 0.0f || !Extents(view.x, view.y))
            return;
        const auto address = reinterpret_cast<uintptr_t>(camera);
        const float currentX = DeerMenu::Read<float>(address + 104);
        const float currentY = DeerMenu::Read<float>(address + 108);
        const float baseX = std::abs(view.x / view.y - 4.0f / 3.0f) <= 0.001f ? view.y * aspect : view.x;
        if (view.adjusted && (currentX != view.lastX || currentY != view.lastY))
        {
            view = {};
            return;
        }
        const bool normalExtent = std::abs(view.x / view.y - 4.0f / 3.0f) <= 0.001f;
        const float factor = normalExtent && Gameplay() && std::isfinite(wanted) ? std::clamp(wanted, 0.5f, 2.0f) : 1.0f;
        float extent[2]{ baseX, view.y };
        if (factor != 1.0f)
        {
            constexpr float maxHalfAngle = 1.56206968f;
            extent[0] = std::tan(std::min(std::atan(baseX) * factor, maxHalfAngle));
            extent[1] = view.y * extent[0] / baseX;
        }
        if (!Extents(extent[0], extent[1]) || (currentX == extent[0] && currentY == extent[1]))
            return;
        // Use the native setter so reciprocals, culling and dirty-frame propagation agree.
        applying = true;
        setView(camera, extent);
        applying = false;
        // Native 2D projection caches these extents separately from the camera.
        if (setCanvasCamera && cameraCanvas.is_initialized() && cameraCanvas.get() == camera)
            setCanvasCamera(camera);
        view.lastX = extent[0];
        view.lastY = extent[1];
        view.adjusted = factor != 1.0f;
    }

    inline void* __cdecl Begin(void* camera)
    {
        Update(camera);
        return beginHook.ccall<void*>(camera);
    }

    inline bool Init(HMODULE game = GetModuleHandleW(nullptr))
    {
        auto owner = hook::pattern(game, "A1 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 34 EB ? D9 05");
        auto setter = hook::pattern(game, "D9 05 ? ? ? ? D8 73 68 D9 05 ? ? ? ? D8 73 6C D9 C9");
        if (auto address = *owner.get_first<void**>(1))
            cameraPrimary.SetAddress(address);
        // A window resize rebuilds the camera with the window's own extent (fixed width, Vert-);
        // give it the 4:3 extent fullscreen uses, so the setter below widens it like at startup.
        auto resize = hook::pattern(game, "A8 01 74 16 D9 44 24 34 D8 74 24 38");
        injector::MakeNOP(resize.get_first(2), 2, true);
        viewHook = safetyhook::create_mid(setter.get_first(), [](SafetyHookContext& regs)
        {
            Correct(reinterpret_cast<void*>(regs.ebx));
        });
        if (!viewHook)
            return false;

        auto begin = hook::pattern(game, "83 EC 08 8B 44 24 0C 89 04 24 FF 50 18 83 C4 08 C3");
        auto nativeSetter = hook::pattern(game, "53 8B 5C 24 08 83 EC 08 8B 44 24 14 8B 10 89 53 68 8B 40 04 89 43 6C");
        auto scopeState = hook::pattern(game, "A1 ? ? ? ? 83 EC 64 85 C0 56 57 0F 84 ? ? ? ? 8B 15");
        auto scriptState = hook::pattern(game, "A1 ? ? ? ? 85 C0 74 ? A1 ? ? ? ? 50 68 ? ? ? ? E8 ? ? ? ? 8B 0D");
        auto followState = hook::pattern(game, "A1 ? ? ? ? 85 C0 75 ? A1 ? ? ? ? 85 C0 75 ? A1 ? ? ? ? 85 C0 75 ? A1");
        auto canvas = hook::pattern(game, "8B 44 24 04 53 83 EC 20 8B D8 85 DB 0F 84 ? ? ? ? 89 1C 24 E8 ? ? ? ? 85 C0");
        auto canvasState = hook::pattern(game, "89 1D ? ? ? ? 83 C4 20 C7 05 ? ? ? ? 00 00 00 00 B8 01 00 00 00 5B C3");
        if (auto address = *scopeState.get_first<int*>(1))
            game_scope.SetAddress(address);
        if (auto address = *scriptState.get_first<int*>(1))
            game_scripted.SetAddress(address);
        if (auto address = *followState.get_first<int*>(1))
            game_followBlocked.SetAddress(address);
        if (auto address = *followState.get_first<int*>(19))
            game_controlBlocked.SetAddress(address);
        if (auto address = *canvasState.get_first<void**>(2))
            cameraCanvas.SetAddress(address);
        setCanvasCamera = reinterpret_cast<decltype(setCanvasCamera)>(canvas.get_first());
        setView = reinterpret_cast<decltype(setView)>(nativeSetter.get_first());
        beginHook = safetyhook::create_inline(begin.get_first(), Begin);
        return bool(beginHook);
    }
}
