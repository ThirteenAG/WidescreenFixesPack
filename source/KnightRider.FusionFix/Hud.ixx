module;
#include <stdafx.h>
#include <ddraw.h>
#include <d3d.h>
#include "FixSettings.hpp"

export module Hud;
import ComVars;
import Window;
import Camera;


export namespace KnightRiderHud
{
    inline std::atomic<float> Constraint{ 0.0f };
    inline thread_local int Anchor = 0;
    inline thread_local bool InfoScene = false;
    inline SafetyHookInline EventHook, MapHook, TimeHook, InfoHook, InspectHook, DialogueHook, SceneHook;

    struct Scope
    {
        int Previous;
        explicit Scope(int anchor) : Previous(Anchor) { Anchor = anchor; }
        ~Scope() { Anchor = Previous; }
    };

    inline float BoxWidth(float width, float height)
    {
        if (CockpitActive || (KnightRiderCamera::Cinematic && KnightRiderCamera::Borders.load(std::memory_order_relaxed) != 0))
            return std::min(width, height * (4.0f / 3.0f));
        const auto ratio = Constraint.load(std::memory_order_relaxed);
        return ratio > 0.0f ? std::min(width, height * ratio) : width;
    }

    inline float Offset(float width, float height, int anchor)
    {
        const float side = std::round(width * 0.5f - (BoxWidth(width, height) - std::min(width, height * (4.0f / 3.0f))) * 0.5f);
        return anchor * (width * 0.5f - side);
    }

    inline int __fastcall Event(void* object, void*, int frame, int pass)
    {
        const Scope scope{ -1 };
        return EventHook.thiscall<int>(object, frame, pass);
    }

    inline int __fastcall Map(void* object, void*, int frame, int pass)
    {
        const Scope scope{ 1 };
        return MapHook.thiscall<int>(object, frame, pass);
    }

    inline int __fastcall Time(void* object, void*, int frame, int pass)
    {
        const Scope scope{ 1 };
        return TimeHook.thiscall<int>(object, frame, pass);
    }

    inline int __fastcall Inspect(void* object, void*, int frame, int pass)
    {
        const Scope scope{ -1 };
        return InspectHook.thiscall<int>(object, frame, pass);
    }

    inline int __fastcall Dialogue(void* object, void*, int frame, int pass)
    {
        const Scope scope{ -1 };
        return DialogueHook.thiscall<int>(object, frame, pass);
    }

    inline int __fastcall Info(uint8_t* property, void*, int frame, int pass)
    {
        // The cockpit uses its own three-dimensional dashboard and camera.
        const int anchor = *reinterpret_cast<int*>(property + 20) == 0 ? 2 : 3;
        if (pass == 1)
            CockpitActive = anchor == 3;
        const Scope scope{ anchor };
        return InfoHook.thiscall<int>(property, frame, pass);
    }

    struct Group
    {
        uint8_t* Object;
        uint16_t Flags;
        int Anchor;
    };

    inline void __fastcall Scene(uint8_t* owner, void*)
    {
        if (Anchor != 2 && Anchor != 3)
            return SceneHook.thiscall<void>(owner);
        const auto camera = *reinterpret_cast<uint8_t**>(owner + 16);
        const auto root = camera ? *reinterpret_cast<uint8_t**>(camera + 380) : nullptr;
        if (!root)
            return SceneHook.thiscall<void>(owner);
        const float width = static_cast<float>(KnightRiderWindow::Width);
        const float height = static_cast<float>(KnightRiderWindow::Height);
        const float designHeight = std::min(height, width / (4.0f / 3.0f));
        const float designWidth = designHeight * (4.0f / 3.0f);
        const float extra = (BoxWidth(width, height) - designWidth) * 0.5f / width;
        auto frustum = reinterpret_cast<float*>(camera + 340);
        auto viewport = reinterpret_cast<float*>(camera + 364);
        const float originalLeft = frustum[0], originalRight = frustum[1];
        const float originalViewport[4]{ viewport[0], viewport[1], viewport[2], viewport[3] };
        viewport[2] = 0.5f + designHeight / height * 0.5f;
        viewport[3] = 1.0f - viewport[2];
        const float center = (originalLeft + originalRight) * 0.5f;
        const float span = (originalRight - originalLeft) * width / designWidth;
        if (Anchor == 3)
        {
            // The original cockpit mesh ends near its 4:3 frame. Keep its complete geometry and
            // use black side areas instead of reflecting the pillars or exposing the world behind it.
            const int marginX = static_cast<int>(std::lround((width - designWidth) * 0.5f));
            const int marginY = static_cast<int>(std::lround((height - designHeight) * 0.5f));
            KnightRiderCamera::ClearBorders(camera, marginX, marginY);
            viewport[0] = static_cast<float>(marginX) / width;
            viewport[1] = 1.0f - viewport[0];
            auto& planes = *reinterpret_cast<uint32_t*>(camera + 584);
            const auto previousPlanes = planes;
            planes = 0;
            InfoScene = true;
            SceneHook.thiscall<void>(owner);
            InfoScene = false;
            planes = previousPlanes;
            std::copy(std::begin(originalViewport), std::end(originalViewport), viewport);
            return;
        }
        const auto horizontal = reinterpret_cast<const float*>(camera + 328);
        const auto direction = reinterpret_cast<const float*>(camera + 304);
        const auto position = reinterpret_cast<const float*>(camera + 132);
        const auto children = *reinterpret_cast<uint8_t***>(root + 228);
        const auto count = *reinterpret_cast<uint32_t*>(root + 236);
        std::vector<Group> groups;
        groups.reserve(count);
        for (uint32_t i = 0; i < count; ++i)
        {
            const auto object = children[i];
            if (!object)
                continue;
            const auto name = *reinterpret_cast<const char**>(object + 8);
            // This mesh contains two disconnected panels, one on either side of the screen.
            int anchor = name && std::strcmp(name, "BGR window") == 0 ? 2
                : name && std::strcmp(name, "BGR minimap_speed indicator") == 0 ? 3 : 0;
            if (anchor < 2 && !(name && std::strcmp(name, "ARROW") == 0))
            {
                const auto point = reinterpret_cast<const float*>(object +
                    (*reinterpret_cast<float*>(object + 40) > 0.0f ? 28 : 132));
                float x = 0.0f, depth = 0.0f;
                for (int axis = 0; axis < 3; ++axis)
                {
                    x += (point[axis] - position[axis]) * horizontal[axis];
                    depth += (point[axis] - position[axis]) * direction[axis];
                }
                if (depth > 0.0f)
                {
                    const float projected = x * frustum[4] / depth;
                    const float threshold = (originalRight - originalLeft) * 0.15f;
                    anchor = projected < center - threshold ? -1 : projected > center + threshold ? 1 : 0;
                }
            }
            groups.push_back({ object, *reinterpret_cast<uint16_t*>(object + 20), anchor });
        }

        const auto draw = [&](int anchor, float left, float right, float start, float end)
        {
            for (const auto& group : groups)
            {
                auto& flags = *reinterpret_cast<uint16_t*>(group.Object + 20);
                const bool visible = group.Anchor == anchor || group.Anchor == 3 || (group.Anchor == 2 && anchor != 0);
                flags = static_cast<uint16_t>((flags & ~uint16_t{ 1 }) | (visible ? group.Flags & 1 : 1));
            }
            frustum[0] = left;
            frustum[1] = right;
            viewport[0] = start;
            viewport[1] = end;
            SceneHook.thiscall<void>(owner);
        };
        auto& planes = *reinterpret_cast<uint32_t*>(camera + 584);
        const auto previousPlanes = planes;
        planes = 0;
        InfoScene = true;
        if (extra == 0.0f)
        {
            frustum[0] = center - span * 0.5f;
            frustum[1] = center + span * 0.5f;
            viewport[0] = 0.0f;
            viewport[1] = 1.0f;
            SceneHook.thiscall<void>(owner);
        }
        else
        {
            // Matching viewport and frustum fractions move each group by a constant number of pixels, including 3D art.
            // The shared two-panel background is clipped in its empty middle and drawn once with each side.
            const float side = std::round(width * (0.5f - extra)) / width;
            const float middle = std::round(designWidth * 0.15f) / width;
            draw(-1, center - span * side, center - span * middle, 0.0f, side - middle);
            draw(1, center + span * middle, center + span * side, 1.0f - side + middle, 1.0f);
            draw(0, center - span * middle, center + span * middle, 0.5f - middle, 0.5f + middle);
        }
        InfoScene = false;
        planes = previousPlanes;
        frustum[0] = originalLeft;
        frustum[1] = originalRight;
        std::copy(std::begin(originalViewport), std::end(originalViewport), viewport);
        for (const auto& group : groups)
        {
            auto& flags = *reinterpret_cast<uint16_t*>(group.Object + 20);
            flags = static_cast<uint16_t>((flags & ~uint16_t{ 1 }) | (group.Flags & 1));
        }
    }

    inline void Init()
    {
        const auto read = []()
        {
            const auto ratio = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            Constraint.store(ratio ? std::clamp(*ratio, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f,
                std::memory_order_relaxed);
        };
        read();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", read);
        auto event = hook::pattern("8B 44 24 08 56 83 F8 01 8B F1 75 10 8B 4E 10 E8 ? ? ? ? 8B 4E 14 E8 ? ? ? ? 5E C2 08 00 8B 41 04");
        EventHook = safetyhook::create_inline(event.get_first(), Event);
        auto map = hook::pattern("8B 44 24 08 56 83 F8 01 8B F1 75 12 8B 4E 48 6A 04 E8 ? ? ? ? 8B 4E 48 E8");
        MapHook = safetyhook::create_inline(map.get_first(), Map);
        auto time = hook::pattern("8B 44 24 08 56 83 F8 01 8B F1 75 10 8B 4E 10 E8 ? ? ? ? 8B 4E 14 E8 ? ? ? ? 5E C2 08 00 83 EC 0C 53");
        TimeHook = safetyhook::create_inline(time.get_first(), Time);
        auto inspect = hook::pattern("8B 44 24 08 56 83 F8 01 8B F1 75 2A 8B 4E 50 6A 04 E8 ? ? ? ? 8B 4E 50 E8");
        InspectHook = safetyhook::create_inline(inspect.get_first(), Inspect);
        auto dialogue = hook::pattern("56 8B F1 8A 46 50 84 C0 74 54 83 7C 24 0C 01 75 4D");
        DialogueHook = safetyhook::create_inline(dialogue.get_first(), Dialogue);
        auto info = hook::pattern("8B 44 24 08 56 83 F8 01 8B F1 75 17 8B 4E 18 8B 54 24 08 6A 01");
        InfoHook = safetyhook::create_inline(info.get_first(), Info);
        auto scene = hook::pattern("56 8B F1 6A 04 8B 4E 10 E8 ? ? ? ? 8B 4E 10 E8 ? ? ? ? 5E C3");
        SceneHook = safetyhook::create_inline(scene.get_first(), Scene);
    }
}
