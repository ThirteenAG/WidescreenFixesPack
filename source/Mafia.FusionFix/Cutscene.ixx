module;

#include <stdafx.h>
#include <cmath>

export module Cutscene;

import ComVars;

namespace Cutscene
{
    enum BordersMode { Off, Letterbox, Pillarbox, Both };
    uintptr_t hud, drawEnd;
    size_t flagsOffset, targetOffset, fadeOffset;
    void** graphics;
    bool enabled;
    float aspect = 0.540540516376495f;
    float bordersMult;
    DWORD lastTick;

    bool IsActive()
    {
        return enabled && (Field<uint32_t>(hud, flagsOffset) & 0x4000) && Field<uint8_t>(hud, targetOffset);
    }

    struct ContentRect
    {
        float left, top, right, bottom, fovScale;
    };

    ContentRect GetContentRect()
    {
        // Native 800x600 bars use (601 - 801 * aspect) / 2 and a half-pixel
        // downward center offset. Fit that visible frustum, including the offset,
        // rather than fitting the entire 4:3 frame or changing the recorded FOV.
        // The extra pixel in the height limit keeps the bottom edge on-screen.
        float height = 801.0f * aspect;
        float scale = std::min(Screen.fWidth / 800.0f, Screen.fHeight / (height + 1.0f));
        float width = 800.0f * scale;
        float center = Screen.fHeight * 0.5f + scale * 0.5f;
        return { (Screen.fWidth - width) * 0.5f, center - height * scale * 0.5f,
            (Screen.fWidth + width) * 0.5f, center + height * scale * 0.5f, Screen.fWidth / width };
    }

    void TickBorderAnim(DWORD now, bool active, float fade)
    {
        float dt = lastTick ? std::min((now - lastTick) / 1000.0f, 0.1f) : 0.0f;
        lastTick = now;
        if (bNoCutsceneBorderAnimation)
            bordersMult = active ? 1.0f : 0.0f;
        else if (fade > 50.0f / 255.0f)
            bordersMult = 0.0f;
        else if (active && fade <= 0.0f)
            bordersMult = std::min(bordersMult + dt / 0.35f, 1.0f);
        else
            bordersMult = std::max(bordersMult - dt / 0.35f, 0.0f);
    }

    struct Vertex
    {
        float x, y, z, rhw;
        uint32_t color, specular;
        float u, v;
    };
    static_assert(sizeof(Vertex) == 32);

    void DrawBorders(SafetyHookContext& regs)
    {
        // This site runs even after the native 250 ms alpha fade has cleared
        // its flag, allowing the 350 ms slide to finish when returning to play.
        regs.eip = drawEnd;
        TickBorderAnim(GetTickCount(), IsActive(), Field<float>(hud, fadeOffset));
        if (nCutsceneBorders == Off || bordersMult <= 0.0f || !*graphics)
            return;

        auto rect = GetContentRect();
        std::array<Vertex, 24> vertices;
        size_t count = 0;
        auto addRect = [&vertices, &count](float left, float top, float right, float bottom)
        {
            if (right <= left || bottom <= top) return;
            Vertex tl{left, top, 0.0f, 1.0f, 0xFF000000, 0, 0, 0};
            Vertex tr = tl, bl = tl, br = tl;
            tr.x = br.x = right;
            bl.y = br.y = bottom;
            for (auto vertex : {tl, tr, bl, tr, br, bl})
                vertices[count++] = vertex;
        };
        if (nCutsceneBorders == Letterbox || nCutsceneBorders == Both)
        {
            if (rect.top > 0.0f)
                addRect(-2.0f, -2.0f, Screen.fWidth + 2.0f, rect.top * bordersMult);
            if (rect.bottom < Screen.fHeight)
                addRect(-2.0f, Screen.fHeight - (Screen.fHeight - rect.bottom) * bordersMult,
                    Screen.fWidth + 2.0f, Screen.fHeight + 2.0f);
        }
        if (nCutsceneBorders == Pillarbox || nCutsceneBorders == Both)
        {
            if (rect.left > 0.0f)
                addRect(-2.0f, -2.0f, rect.left * bordersMult, Screen.fHeight + 2.0f);
            if (rect.right < Screen.fWidth)
                addRect(Screen.fWidth - (Screen.fWidth - rect.right) * bordersMult, -2.0f,
                    Screen.fWidth + 2.0f, Screen.fHeight + 2.0f);
        }
        if (!count) return;
        auto graph = *graphics;
        auto table = *reinterpret_cast<uintptr_t**>(graph);
        using SetTexture = void(__stdcall*)(void*, void*);
        using Draw = void(__stdcall*)(void*, int, int, const Vertex*, uint32_t);
        reinterpret_cast<SetTexture>(table[12 / sizeof(uintptr_t)])(graph, nullptr);
        reinterpret_cast<Draw>(table[88 / sizeof(uintptr_t)])(graph, 3, static_cast<int>(count / 3), vertices.data(), 1);
    }
}

export float GetCameraFovScale()
{
    // As in the GTA fixes, border preferences and their animation never animate
    // the camera zoom. Off reveals the surrounding image at the same shot FOV.
    if (Cutscene::IsActive())
        return Cutscene::GetContentRect().fovScale;
    return Screen.fAspectRatio / (4.0f / 3.0f);
}

export void InitCutscenes()
{
    auto pStart = find_pattern<2>("68 DD 60 0A 3F B9 ? ? ? ? E8 ? ? ? ? 68 FA 00 00 00 6A 01 B9 ? ? ? ? E8 ? ? ? ?");
    auto pDraw = find_pattern<2>("8B 85 ? ? ? ? F6 C4 40 74 ? A1 ? ? ? ? 57 50 8B 10 FF 52 0C A1 ? ? ? ? 6A 01 68 ? ? ? ? 6A 04 8B 08 6A 03 50 FF 51 58 F7 85");
    if (pStart.size() != 1 || pDraw.size() != 1) return;
    auto start = reinterpret_cast<uintptr_t>(pStart.get_first());
    auto draw = reinterpret_cast<uintptr_t>(pDraw.get_first());
    Cutscene::flagsOffset = Field<uint32_t>(draw, 2);
    bool old = Cutscene::flagsOffset == 0x90A4;
    if (!old && Cutscene::flagsOffset != 0x40A4) return;
    Cutscene::targetOffset = Cutscene::flagsOffset + (old ? 960 : 968);
    Cutscene::fadeOffset = Cutscene::flagsOffset + (old ? 984 : 992);
    Cutscene::hud = Field<uintptr_t>(start, 6);
    Cutscene::graphics = *pDraw.get_first<void**>(12);
    Cutscene::drawEnd = draw + 11 + Field<int8_t>(draw, 10);
    auto aspect = start + 15 + Field<int32_t>(start, 11);
    static auto AspectHook = safetyhook::create_mid(aspect, [](SafetyHookContext& regs)
    {
        if (regs.ecx != Cutscene::hud) return;
        float ratio = Field<float>(regs.esp, 4);
        if (std::isfinite(ratio) && ratio > 0.0f && ratio < 0.75f)
            Cutscene::aspect = ratio;
    });
    if (!AspectHook) return;
    static auto BordersHook = safetyhook::create_mid(draw, Cutscene::DrawBorders);
    if (!BordersHook)
    {
        AspectHook.reset();
        return;
    }
    Cutscene::enabled = true;
}
