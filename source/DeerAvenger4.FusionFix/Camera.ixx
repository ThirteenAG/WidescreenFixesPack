module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include "Hooking.Patterns.h"
#include "gameref.hpp"
#include "safetyhook.hpp"
#include <ddraw.h>

export module Camera;

import Frontend;
import ComVars;
import Movie;

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

    inline void** overlayCameras[2]{};
    inline View overlayViews[2];
    inline SafetyHookInline scopeDrawHook, aimDrawHook;

    inline void OverlayBars(void* camera)
    {
        const auto raster = DeerMenu::Read<void*>(reinterpret_cast<uintptr_t>(camera) + 96);
        if (!raster)
            return;
        const auto address = reinterpret_cast<uintptr_t>(raster);
        const int width = DeerMenu::Read<int>(address + 12);
        const int height = DeerMenu::Read<int>(address + 16);
        DeerMovie::Layout layout;
        if (!DeerMovie::Geometry(4, 3, width, height, layout))
            return;
        const auto& box = layout.destination;
        if (box.left == 0 && box.right == width && box.top == 0 && box.bottom == height)
            return;
        const auto surface = DeerMovie::Surface(raster);
        if (!surface)
            return;
        DDBLTFX fill{};
        fill.dwSize = sizeof(fill);
        for (auto rect : { RECT{ 0, 0, box.left, height }, RECT{ box.right, 0, width, height },
            RECT{ 0, 0, width, box.top }, RECT{ 0, box.bottom, width, height } })
        {
            if (rect.right > rect.left && rect.bottom > rect.top)
                surface->Blt(&rect, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fill);
        }
    }

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

    // The mask has its own camera. Apply the current raster aspect when drawing it, including after live resize.
    inline void FitOverlay(void* camera, View& overlay)
    {
        const auto address = reinterpret_cast<uintptr_t>(camera);
        const float x = DeerMenu::Read<float>(address + 104), y = DeerMenu::Read<float>(address + 108);
        const float aspect = Aspect(camera);
        if (!Extents(x, y) || aspect <= 0.0f)
            return;
        if (overlay.camera != camera || x != overlay.lastX || y != overlay.lastY)
            overlay = { camera, x, y, x, y };

        constexpr float design = 4.0f / 3.0f;
        float extent[]{ overlay.x * std::max(1.0f, aspect / design), overlay.y * std::max(1.0f, design / aspect) };
        if (x != extent[0] || y != extent[1])
            setView(camera, extent);
        overlay.lastX = extent[0];
        overlay.lastY = extent[1];
    }

    inline void __cdecl ScopeDraw()
    {
        FitOverlay(*overlayCameras[0], overlayViews[0]);
        scopeDrawHook.ccall();
    }

    inline void __cdecl AimDraw()
    {
        if (game_aim.get() != 0)
            FitOverlay(*overlayCameras[1], overlayViews[1]);
        aimDrawHook.ccall();
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
        if (view.adjusted && (currentX != view.lastX || currentY != view.lastY))
        {
            view = {};
            return;
        }
        const bool normalExtent = std::abs(view.x / view.y - 4.0f / 3.0f) <= 0.001f;
        const float factor = normalExtent && Gameplay() && std::isfinite(wanted) ? std::clamp(wanted, 0.5f, 2.0f) : 1.0f;
        float extent[2]{ normalExtent ? view.y * aspect : view.x, view.y };
        if (factor != 1.0f)
        {
            // Scale the native vertical angle before deriving the horizontal extent for this raster.
            constexpr float maxHalfAngle = 1.56206968f;
            extent[1] = std::tan(std::min(std::atan(view.y) * factor, maxHalfAngle));
            extent[0] = extent[1] * aspect;
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

    inline void* __cdecl ResumeAfterOverlay(void* camera)
    {
        // The main camera owns the current back buffer, which can alternate between raster objects.
        // Fill the bars after the overlay ends and before starting another scene on that buffer.
        OverlayBars(camera);
        return Begin(camera);
    }

    inline void Init(HMODULE game = GetModuleHandleW(nullptr))
    {
        auto owner = hook::pattern(game, "A1 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 34 EB ? D9 05");
        auto setter = hook::pattern(game, "D9 05 ? ? ? ? D8 73 68 D9 05 ? ? ? ? D8 73 6C D9 C9");
        cameraPrimary.SetAddress(*owner.get_first<void**>(1));
        // A window resize rebuilds the camera with the window's own extent (fixed width, Vert-);
        // give it the 4:3 extent fullscreen uses, so the setter below widens it like at startup.
        auto resize = hook::pattern(game, "A8 01 74 16 D9 44 24 34 D8 74 24 38");
        injector::MakeNOP(resize.get_first(2), 2, true);
        viewHook = safetyhook::create_mid(setter.get_first(), [](SafetyHookContext& regs)
        {
            Correct(reinterpret_cast<void*>(regs.ebx));
        });
        auto begin = hook::pattern(game, "83 EC 08 8B 44 24 0C 89 04 24 FF 50 18 83 C4 08 C3");
        auto nativeSetter = hook::pattern(game, "53 8B 5C 24 08 83 EC 08 8B 44 24 14 8B 10 89 53 68 8B 40 04 89 43 6C");
        auto scopeState = hook::pattern(game, "A1 ? ? ? ? 83 EC 64 85 C0 56 57 0F 84 ? ? ? ? 8B 15");
        auto scriptState = hook::pattern(game, "A1 ? ? ? ? 85 C0 74 ? A1 ? ? ? ? 50 68 ? ? ? ? E8 ? ? ? ? 8B 0D");
        auto followState = hook::pattern(game, "A1 ? ? ? ? 85 C0 75 ? A1 ? ? ? ? 85 C0 75 ? A1 ? ? ? ? 85 C0 75 ? A1");
        auto canvas = hook::pattern(game, "8B 44 24 04 53 83 EC 20 8B D8 85 DB 0F 84 ? ? ? ? 89 1C 24 E8 ? ? ? ? 85 C0");
        auto canvasState = hook::pattern(game, "89 1D ? ? ? ? 83 C4 20 C7 05 ? ? ? ? 00 00 00 00 B8 01 00 00 00 5B C3");
        game_scope.SetAddress(*scopeState.get_first<int*>(1));
        game_scripted.SetAddress(*scriptState.get_first<int*>(1));
        game_followBlocked.SetAddress(*followState.get_first<int*>(1));
        game_controlBlocked.SetAddress(*followState.get_first<int*>(19));
        cameraCanvas.SetAddress(*canvasState.get_first<void**>(2));
        setCanvasCamera = reinterpret_cast<decltype(setCanvasCamera)>(canvas.get_first());
        setView = reinterpret_cast<decltype(setView)>(nativeSetter.get_first());
        beginHook = safetyhook::create_inline(begin.get_first(), Begin);

        auto scope = hook::pattern(game, "6A 00 6A 06 E8 ? ? ? ? 6A 00 6A 08 E8 ? ? ? ? A1 ? ? ? ? 50 FF 50 48 A1 ? ? ? ? 83 C4 1C 83 F8 01");
        auto aim = hook::pattern(game, "A1 ? ? ? ? 85 C0 74 ? A1 ? ? ? ? 50 E8 ? ? ? ? 8B 0D ? ? ? ? 51 E8 ? ? ? ? 6A 00 6A 06");
        auto scopeDraw = scope.get_first<uint8_t>(-0x17);
        overlayCameras[0] = *reinterpret_cast<void***>(scopeDraw + 0x0D);
        overlayCameras[1] = *aim.get_first<void**>(0x16);
        game_aim.SetAddress(*aim.get_first<int*>(1));
        scopeDrawHook = safetyhook::create_inline(scopeDraw, ScopeDraw);
        aimDrawHook = safetyhook::create_inline(aim.get_first(), AimDraw);
        auto scopeResume = hook::pattern(game, "8B 15 ? ? ? ? 52 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 18 C3");
        auto aimResume = hook::pattern(game, "8B 15 ? ? ? ? 52 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 34 C3");
        injector::MakeCALL(scopeResume.get_first(18), ResumeAfterOverlay, true);
        injector::MakeCALL(aimResume.get_first(18), ResumeAfterOverlay, true);
    }
}
