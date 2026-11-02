module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Cinematics;


// Cinematic bars: the game draws its letterbox as two black quads that fade in and out while a cinematic runs.
// MAIN.CutsceneBorders keeps them (Letterbox), removes them (Off) and adds side bars that keep the picture at 16:9
// (Pillarbox); MAIN.AnimatedCutsceneBorders off shows and hides the bars at once instead of fading them.
export namespace CondemnedCinematics
{
    struct Vertex
    {
        float x, y, z;
        uint32_t color;
    };
    using DrawQuads = void(__thiscall*)(void* draw, Vertex* quads, uint32_t count);

    inline std::atomic<int> borders{ 3 };
    inline std::atomic<bool> animated{ true };
    inline SafetyHookMid topHook, bottomHook;

    inline void Side(void* draw, const Vertex* quad, float left, float right, float height)
    {
        Vertex side[4]
        {
            { left, 0.0f, quad[0].z, quad[0].color },
            { right, 0.0f, quad[1].z, quad[0].color },
            { right, height, quad[2].z, quad[0].color },
            { left, height, quad[3].z, quad[0].color },
        };
        reinterpret_cast<DrawQuads>((*reinterpret_cast<void***>(draw))[16])(draw, side, 1);
    }

    // Before the native draw call's arguments: eax is the draw interface, the quad is on the stack next to the
    // screen height, edi is the interface screen with its "cinematic active" byte.
    inline void Bar(SafetyHookContext& context, bool top)
    {
        auto quad = reinterpret_cast<Vertex*>(context.esp + 0x24);
        const auto draw = reinterpret_cast<void*>(context.eax);
        if (!draw)
            return;
        const int mode = borders;
        if (!animated)
        {
            const bool active = *reinterpret_cast<const uint8_t*>(context.edi + 0x2962) != 0;
            for (int i = 0; i < 4; ++i)
                quad[i].color = active ? 0xFF000000 : 0;
        }
        if (top && (mode == 2 || mode == 3))
        {
            const auto height = static_cast<float>(*reinterpret_cast<const uint32_t*>(context.esp + 0x18));
            const float width = quad[1].x;
            const float side = (width - height * 16.0f / 9.0f) / 2.0f;
            if (side >= 1.0f && (quad[0].color >> 24))
            {
                Side(draw, quad, 0.0f, side, height);
                Side(draw, quad, width - side, width, height);
            }
        }
        if (mode == 0 || mode == 2)
        {
            // An empty bar: the far edge on the near one
            quad[2].y = quad[1].y;
            quad[3].y = quad[0].y;
        }
    }

    inline void Init(HMODULE gameClient)
    {
        auto top = hook::pattern(gameClient, "6A 01 8D 4C 24 28 51 8B C8 FF 52 40 D9 44 24 1C 8B 54 24 14");
        auto bottom = hook::pattern(gameClient, "6A 01 8D 4C 24 28 51 8B C8 FF 52 40 5F 5E 5D 5B 83 C4 54 C3");
        const auto refresh = []()
        {
            borders = std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
            animated = FixSettings().Bool("MAIN.AnimatedCutsceneBorders");
        };
        refresh();
        FixSettings().OnChange("MAIN.CutsceneBorders", refresh);
        FixSettings().OnChange("MAIN.AnimatedCutsceneBorders", refresh);
        topHook = safetyhook::create_mid(top.get_first(), [](SafetyHookContext& context) { Bar(context, true); });
        bottomHook = safetyhook::create_mid(bottom.get_first(), [](SafetyHookContext& context) { Bar(context, false); });
    }
}
