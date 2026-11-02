module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "Hooking.Patterns.h"
#include "safetyhook.hpp"
#include <atomic>
#include <optional>

export module Hud;

import Frontend;
import ComVars;

export namespace DeerHud
{
    enum class Anchor { Left, Centre, Right };

    struct Matrix
    {
        float right[3];
        uint32_t flags;
        float up[3];
        uint32_t pad1;
        float at[3];
        uint32_t pad2;
        float position[3];
        uint32_t pad3;
    };
    static_assert(sizeof(Matrix) == 64);

    struct Transform
    {
        float scale = 1.0f;
        float shift = 0.0f;
        float scaleY = 1.0f;
        float shiftY = 0.0f;

        static bool Build(float aspect, float constraint, float span, Anchor anchor, Transform& result)
        {
            if (!std::isfinite(aspect) || !std::isfinite(constraint) || !std::isfinite(span) ||
                aspect <= 0.0f || constraint <= 0.0f || span <= 0.0f)
                return false;
            const float box = std::clamp(constraint, std::min(aspect, 4.0f / 3.0f), aspect);
            const float scale = (4.0f / 3.0f) / aspect;
            float origin = (1.0f - scale) * 0.5f;
            if (anchor == Anchor::Left)
                origin = (1.0f - box / aspect) * 0.5f;
            else if (anchor == Anchor::Right)
                origin = (1.0f + box / aspect) * 0.5f - scale;
            result = { scale, span * origin };
            return true;
        }

        static bool Fit(float aspect, float spanX, float spanY, Transform& result)
        {
            if (!std::isfinite(aspect) || !std::isfinite(spanX) || !std::isfinite(spanY) ||
                aspect <= 0.0f || spanX <= 0.0f || spanY <= 0.0f)
                return false;
            constexpr float designAspect = 4.0f / 3.0f;
            const float x = std::min(1.0f, designAspect / aspect);
            const float y = std::min(1.0f, aspect / designAspect);
            result = { x, spanX * (1.0f - x) * 0.5f, y, spanY * (1.0f - y) * 0.5f };
            return true;
        }
    };

    class MatrixScope
    {
        Matrix* matrix = nullptr;
        Matrix saved{};
        int* world = nullptr;
        int* derived = nullptr;

    public:
        MatrixScope(Matrix* target, const Transform& transform, int* worldCache, int* derivedCache)
        {
            if (!target || !worldCache || !derivedCache ||
                (transform.scale == 1.0f && transform.shift == 0.0f &&
                    transform.scaleY == 1.0f && transform.shiftY == 0.0f))
                return;
            matrix = target;
            world = worldCache;
            derived = derivedCache;
            std::memcpy(&saved, matrix, sizeof(saved));
            matrix->right[0] *= transform.scale;
            matrix->up[0] *= transform.scale;
            matrix->at[0] *= transform.scale;
            matrix->position[0] = matrix->position[0] * transform.scale + transform.shift;
            matrix->right[1] *= transform.scaleY;
            matrix->up[1] *= transform.scaleY;
            matrix->at[1] *= transform.scaleY;
            matrix->position[1] = matrix->position[1] * transform.scaleY + transform.shiftY;
            matrix->flags &= ~0x20003u;
            *world = 0;
            *derived = 0;
        }

        MatrixScope(const MatrixScope&) = delete;
        MatrixScope& operator=(const MatrixScope&) = delete;

        ~MatrixScope()
        {
            if (!matrix)
                return;
            std::memcpy(matrix, &saved, sizeof(saved));
            *world = 0;
            *derived = 0;
        }
    };
}

export namespace DeerHud
{
    inline SafetyHookInline pathHook;
    inline thread_local bool drawing = false;
    // Parsed MAIN.HudAspectRatioConstraint; 0 = follow the screen.
    inline std::atomic<float> constraintSetting = 0.0f;

    inline void UpdateConstraint()
    {
        const auto parsed = ParseWidescreenHudOffset(FixSettings().Wanted("MAIN.HudAspectRatioConstraint"));
        constraintSetting = parsed && *parsed > 0.0f && *parsed <= 32.0f / 9.0f ? *parsed : 0.0f;
    }

    // The game's sprite table: F1-F4 and health on the left; F7/F8, compass, F5/F6 and the tracker
    // devices on the right; crosshair, scope and prompts centred.
    inline std::optional<Anchor> Sprite(void* path, void* brush)
    {
        for (int i = 0; i < 262; ++i)
        {
            const uintptr_t sprite = DeerMenu::Game().sprites + i * 124;
            if (DeerMenu::Global<void*>(sprite + 96) != path || DeerMenu::Global<void*>(sprite + 92) != brush)
                continue;
            if (i < 4 || (i >= 11 && i < 15))
                return Anchor::Left;
            if ((i >= 4 && i < 6) || i == 9 || i == 10 || (i >= 45 && i <= 76))
                return Anchor::Right;
            return Anchor::Centre;
        }
        return std::nullopt;
    }

    inline bool Gameplay(void*& camera, float& aspect, float& span, float& spanY, bool pause)
    {
        using DeerMenu::Global;
        using DeerMenu::Game;
        if ((DeerMenu::Game(), game_worldState.get()) != 1 || (DeerMenu::Game(), game_demoState.get()) != 0 ||
            (DeerMenu::Game(), game_levelState.get()) == 0 || (DeerMenu::Game(), game_transitionState.get()) != 0 ||
            (DeerMenu::Game(), game_consoleState.get()) != 0 || (DeerMenu::Game(), game_hudBlock1.get()) != 0 ||
            (DeerMenu::Game(), game_hudBlock2.get()) != 0 ||
            (!pause && ((DeerMenu::Game(), game_menuOpen.get()) != 0 || (DeerMenu::Game(), game_menuActive.get()) != 0)))
            return false;
        camera = (DeerMenu::Game(), game_camera.get());
        const auto engine = (DeerMenu::Game(), game_engine.get());
        if (!camera || camera != (DeerMenu::Game(), game_viewportCamera.get()) || !engine || *static_cast<void**>(engine) != camera)
            return false;
        const auto address = reinterpret_cast<uintptr_t>(camera);
        if (*reinterpret_cast<const int*>(address + 20) != 1)
            return false;
        const auto raster = *reinterpret_cast<void* const*>(address + 96);
        if (!raster)
            return false;
        const int width = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(raster) + 12);
        const int height = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(raster) + 16);
        const float x = (DeerMenu::Game(), game_viewportX.get());
        const float y = (DeerMenu::Game(), game_viewportY.get());
        const float cameraX = *reinterpret_cast<const float*>(address + 104);
        const float cameraY = *reinterpret_cast<const float*>(address + 108);
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
            !std::isfinite(x) || !std::isfinite(y) || x <= 0.0f || y <= 0.0f ||
            !std::isfinite(cameraX) || !std::isfinite(cameraY) || cameraX <= 0.0f || cameraY <= 0.0f ||
            (DeerMenu::Game(), game_viewportWidth.get()) != float(width) || (DeerMenu::Game(), game_viewportHeight.get()) != float(height) ||
            std::abs(x - 2.0f * cameraX) > 0.0001f || std::abs(y - 2.0f * cameraY) > 0.0001f ||
            (DeerMenu::Game(), game_viewOffsetX.get()) != 0.0f || (DeerMenu::Game(), game_viewOffsetY.get()) != 0.0f ||
            std::abs((DeerMenu::Game(), game_viewAspect.get()) - 4.0f / 3.0f) > 0.0001f ||
            (DeerMenu::Game(), game_viewScale.get()) != 1.0f)
            return false;
        aspect = float(width) / float(height);
        span = x / std::min(x, y);
        spanY = y / std::min(x, y);
        return true;
    }

    // The 2D canvas outside gameplay (loading pictures, hub and result screens).
    inline bool Screen(float& aspect, float& span, float& spanY)
    {
        using DeerMenu::Global;
        using DeerMenu::Game;
        const auto camera = (DeerMenu::Game(), game_viewportCamera.get());
        if (!camera)
            return false;
        const auto raster = *reinterpret_cast<void* const*>(reinterpret_cast<uintptr_t>(camera) + 96);
        if (!raster)
            return false;
        const int width = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(raster) + 12);
        const int height = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(raster) + 16);
        const float x = (DeerMenu::Game(), game_viewportX.get());
        const float y = (DeerMenu::Game(), game_viewportY.get());
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
            !std::isfinite(x) || !std::isfinite(y) || x <= 0.0f || y <= 0.0f ||
            (DeerMenu::Game(), game_viewportWidth.get()) != float(width) || (DeerMenu::Game(), game_viewportHeight.get()) != float(height) ||
            (DeerMenu::Game(), game_viewOffsetX.get()) != 0.0f || (DeerMenu::Game(), game_viewOffsetY.get()) != 0.0f ||
            std::abs((DeerMenu::Game(), game_viewAspect.get()) - 4.0f / 3.0f) > 0.0001f ||
            (DeerMenu::Game(), game_viewScale.get()) != 1.0f)
            return false;
        aspect = float(width) / float(height);
        span = x / std::min(x, y);
        spanY = y / std::min(x, y);
        return true;
    }

    inline void* __cdecl Draw(void* path, void* brush)
    {
        if (drawing || !path || !brush)
            return pathHook.ccall<void*>(path, brush);
        void* camera = nullptr;
        float aspect = 0.0f, span = 0.0f, spanY = 0.0f;
        bool fit = DeerMenu::pauseSpriteDepth != 0;
        std::optional<Anchor> anchor;
        if (Gameplay(camera, aspect, span, spanY, fit))
            anchor = fit ? std::optional<Anchor>{ Anchor::Centre } : Sprite(path, brush);
        else if (Sprite(path, brush) && Screen(aspect, span, spanY))
        {
            // Every other screen keeps its 4:3 design in a centred box.
            anchor = Anchor::Centre;
            fit = true;
        }
        const int index = (DeerMenu::Game(), game_matrixIndex.get());
        if (!anchor || index < 0 || index >= 32)
            return pathHook.ccall<void*>(path, brush);
        auto matrix = DeerMenu::Global<Matrix*>((DeerMenu::Game(), reinterpret_cast<uintptr_t>(game_matrixStack.get_ptr())) + index * 4);
        if (!matrix)
            return pathHook.ccall<void*>(path, brush);
        const float wanted = constraintSetting;
        const float constraint = wanted > 0.0f ? wanted : aspect;
        Transform transform;
        if (!(fit ? Transform::Fit(aspect, span, spanY, transform) :
            Transform::Build(aspect, constraint, span, *anchor, transform)))
            return pathHook.ccall<void*>(path, brush);
        struct Drawing
        {
            Drawing() { drawing = true; }
            ~Drawing() { drawing = false; }
        } active;
        MatrixScope scope(matrix, transform, reinterpret_cast<int*>((DeerMenu::Game(), reinterpret_cast<uintptr_t>(game_matrixDirty.get_ptr()))),
            reinterpret_cast<int*>((DeerMenu::Game(), reinterpret_cast<uintptr_t>(game_matrixUpdate.get_ptr()))));
        return pathHook.ccall<void*>(path, brush);
    }

    inline bool Init()
    {
        // The sprite path wrapper; an identical wrapper elsewhere calls a different function
        auto path = hook::pattern("53 8B 5C 24 08 83 EC 08 89 1C 24 8B 44 24 14 89 44 24 04 E8 08 00 00 00 83 C4 08 8B C3 5B C3");
        UpdateConstraint();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", [] { UpdateConstraint(); });
        pathHook = safetyhook::create_inline(path.get_first(), Draw);
        const bool available = bool(pathHook);
        FixSettings().Availability("MAIN.HudAspectRatioConstraint", [available](std::string_view) { return available; });
        return available;
    }
}
