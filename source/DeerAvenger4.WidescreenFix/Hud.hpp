#pragma once

#include "FixSettings.hpp"
#include "NativeMenuContracts.hpp"
#include "HudLayout.hpp"
#include "Hooking.Patterns.h"
#include "safetyhook.hpp"
#include <atomic>
#include <optional>

namespace DeerHud
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

    inline std::optional<Anchor> Sprite(void* path, void* brush)
    {
        for (int i = 0; i < 15; ++i)
        {
            const uintptr_t sprite = 0x4F48D8 + i * 124;
            if (DeerMenu::Global<void*>(sprite + 96) != path || DeerMenu::Global<void*>(sprite + 92) != brush)
                continue;
            if (i < 4 || i >= 11)
                return Anchor::Left;
            if (i < 6 || i == 9 || i == 10)
                return Anchor::Right;
            return Anchor::Centre;
        }
        return std::nullopt;
    }

    inline bool Gameplay(void*& camera, float& aspect, float& span)
    {
        using DeerMenu::Global;
        if (Global<int>(0x532B40) != 1 || Global<int>(0x532B1C) != 0 ||
            Global<int>(0x532CA0) == 0 || Global<int>(0x533228) != 0 ||
            Global<int>(0x52F99C) != 0 || Global<int>(0x533DBC) != 0 ||
            Global<int>(0x532B14) != 0 ||
            Global<int>(0x532300) != 0 || Global<int>(0x532304) != 0)
            return false;
        camera = Global<void*>(0x54699C);
        const auto engine = Global<void*>(0x554934);
        if (!camera || camera != Global<void*>(0x5AD250) || !DeerMenu::Readable(camera, 112) ||
            !DeerMenu::Readable(engine, sizeof(void*)) || *static_cast<void**>(engine) != camera)
            return false;
        const auto address = reinterpret_cast<uintptr_t>(camera);
        if (*reinterpret_cast<const int*>(address + 20) != 1)
            return false;
        const auto raster = *reinterpret_cast<void* const*>(address + 96);
        if (!DeerMenu::Readable(raster, 20))
            return false;
        const int width = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(raster) + 12);
        const int height = *reinterpret_cast<const int*>(static_cast<const uint8_t*>(raster) + 16);
        const float x = Global<float>(0x5AD154);
        const float y = Global<float>(0x5AD158);
        const float cameraX = *reinterpret_cast<const float*>(address + 104);
        const float cameraY = *reinterpret_cast<const float*>(address + 108);
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
            !std::isfinite(x) || !std::isfinite(y) || x <= 0.0f || y <= 0.0f ||
            !std::isfinite(cameraX) || !std::isfinite(cameraY) || cameraX <= 0.0f || cameraY <= 0.0f ||
            Global<float>(0x5AD15C) != float(width) || Global<float>(0x5AD160) != float(height) ||
            std::abs(x - 2.0f * cameraX) > 0.0001f || std::abs(y - 2.0f * cameraY) > 0.0001f ||
            Global<float>(0x5AD240) != 0.0f || Global<float>(0x5AD244) != 0.0f ||
            std::abs(Global<float>(0x5AD248) - 4.0f / 3.0f) > 0.0001f ||
            Global<float>(0x5AD24C) != 1.0f)
            return false;
        aspect = float(width) / float(height);
        span = x / std::min(x, y);
        return true;
    }

    inline void* __cdecl Draw(void* path, void* brush)
    {
        if (drawing || !path || !brush)
            return pathHook.ccall<void*>(path, brush);
        void* camera = nullptr;
        float aspect = 0.0f, span = 0.0f;
        if (!Gameplay(camera, aspect, span))
            return pathHook.ccall<void*>(path, brush);
        const auto anchor = Sprite(path, brush);
        const int index = DeerMenu::Global<int>(0x5AD150);
        if (!anchor || index < 0 || index >= 32)
            return pathHook.ccall<void*>(path, brush);
        auto matrix = DeerMenu::Global<Matrix*>(0x5AD0D0 + index * 4);
        if (!DeerMenu::Readable(matrix, sizeof(*matrix)))
            return pathHook.ccall<void*>(path, brush);
        const float wanted = constraintSetting;
        const float constraint = wanted > 0.0f ? wanted : aspect;
        Transform transform;
        if (!Transform::Build(aspect, constraint, span, *anchor, transform))
            return pathHook.ccall<void*>(path, brush);
        struct Drawing
        {
            Drawing() { drawing = true; }
            ~Drawing() { drawing = false; }
        } active;
        MatrixScope scope(matrix, transform, reinterpret_cast<int*>(0x5AD224), reinterpret_cast<int*>(0x5AD238));
        return pathHook.ccall<void*>(path, brush);
    }

    inline bool Init(bool enabled)
    {
        bool available = false;
        if (enabled && DeerMenu::Contracts::Supported())
        {
            auto path = hook::pattern("53 8B 5C 24 08 83 EC 08 89 1C 24 8B 44 24 14 89 44 24 04 E8 ? ? ? ? 83 C4 08 8B C3 5B C3");
            if (path.size() == 1 && path.get_first() == reinterpret_cast<void*>(0x4AA240))
            {
                UpdateConstraint();
                FixSettings().OnChange("MAIN.HudAspectRatioConstraint", [] { UpdateConstraint(); });
                pathHook = safetyhook::create_inline(path.get_first(), Draw);
                available = bool(pathHook);
            }
        }
        FixSettings().Availability("MAIN.HudAspectRatioConstraint", [available](std::string_view) { return available; });
        return !enabled || available;
    }
}
