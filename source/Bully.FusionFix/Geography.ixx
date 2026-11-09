module;

#include <stdafx.h>

export module Geography;

import ComVars;

// Geography class: the map art is stretched over the whole screen, while pins, flags and the cursor sit in sprite
// units scaled by the aspect ratio. Draw the map at its own aspect in the same units so everything lines up.
export namespace BullyGeography
{
    // Width of the map art in sprite units; the 4:3 view shows the middle three quarters of it
    constexpr float MapWidth = 640.0f / 0.75f;
    constexpr float CursorMargin = 30.0f;

    using DrawRectangle = int(__cdecl*)(float, float, float, float, void*, const uint32_t*, const float*, const float*);
    DrawRectangle Draw = nullptr;
    float*(__fastcall* MoveCursor)(void*, void*, float*, float, float, int) = nullptr;

    struct Placement
    {
        float left;
        float right;
        float u0;
        float u1;
    };

    // The game shows the texture range [u0, u1] across the screen, centred on the pins' origin
    Placement Place(float left, float right, float u0, float u1)
    {
        Placement place{ left, right, u0, u1 };
        const float centre = (left + right) * 0.5f;
        const float half = (u1 - u0) * MapWidth * 0.5f * SpriteScaleX;
        if (!std::isfinite(half) || half <= 0.0f)
            return place;
        place.left = centre - half;
        place.right = centre + half;
        // Wider than the screen: crop the sides of the art instead
        if (place.left < left)
        {
            const float texel = (u1 - u0) / (place.right - place.left);
            place.u0 = u0 + (left - place.left) * texel;
            place.u1 = u1 - (place.right - right) * texel;
            place.left = left;
            place.right = right;
        }
        return place;
    }

    int __cdecl DrawMap(float left, float top, float right, float bottom, void* texture, const uint32_t* color, const float* uv0, const float* uv1)
    {
        const auto place = Place(left, right, uv0[0], uv1[0]);
        const float start[2]{ place.u0, uv0[1] };
        const float end[2]{ place.u1, uv1[1] };
        return Draw(place.left, top, place.right, bottom, texture, color, start, end);
    }

    // The main map also fills the space beside it, which would otherwise show the classroom
    int __cdecl DrawMapWithBorders(float left, float top, float right, float bottom, void* texture, const uint32_t* color, const float* uv0, const float* uv1)
    {
        const int result = DrawMap(left, top, right, bottom, texture, color, uv0, uv1);
        const auto place = Place(left, right, uv0[0], uv1[0]);
        if (place.left > left)
        {
            static constexpr uint32_t black = 0xFF000000;
            Draw(left, top, place.left, bottom, nullptr, &black, uv0, uv1);
            Draw(place.right, top, right, bottom, nullptr, &black, uv0, uv1);
        }
        return result;
    }

    // Keep the cursor over the map
    float* __fastcall Move(void* self, void* edx, float* out, float x, float y, int player)
    {
        auto result = MoveCursor(self, edx, out, x, y, player);
        const float limit = MapWidth * 0.5f - CursorMargin;
        result[0] = std::clamp(result[0], -limit, limit);
        return result;
    }

    void Install()
    {
        auto scale = hook::pattern("D9 05 ? ? ? ? 8B 44 24 10 83 F8 01 D8 4C 24 08 D9 5C 24 08 D9 05");
        auto background = hook::pattern("D9 1C 24 E8 ? ? ? ? 83 C4 20 83 C4 10 C3");
        auto transition = hook::pattern("D9 1C 24 E8 ? ? ? ? 83 86 78 06 00 00 FE");
        auto cursor = hook::pattern("8D 54 24 14 52 8B CE E8 ? ? ? ? D9 00 D9 9E CC 04 00 00");

        SpriteScaleX.SetAddress(*scale.get_first<float*>(2));
        Draw = reinterpret_cast<DrawRectangle>(injector::GetBranchDestination(background.get_first(3)).as_int());
        injector::MakeCALL(background.get_first(3), DrawMapWithBorders, true);
        injector::MakeCALL(transition.get_first(3), DrawMap, true);
        MoveCursor = reinterpret_cast<decltype(MoveCursor)>(injector::GetBranchDestination(cursor.get_first(7)).as_int());
        injector::MakeCALL(cursor.get_first(7), Move, true);
    }
}
