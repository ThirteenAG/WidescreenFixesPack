module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Draw;

import ComVars;
import Layout;

export namespace Draw
{
    using GlyphFn = void* (__cdecl*)(int u, int v, int tw, int th, int x, int w, int h);
    using PositionFn = void* (__cdecl*)(int x, int y, int z);
    using QuadFn = void* (__cdecl*)(float* points, float* uv);
    using BarFn = void* (__cdecl*)(int x, int y, int w, int h, int alpha, int texture, float fill, int text, float a9, int a10);

    SpriteFn SpriteOriginal = nullptr;
    GlyphFn GlyphOriginal = nullptr;
    PositionFn PositionOriginal = nullptr;
    QuadFn QuadOriginal = nullptr;
    TextFn TextOriginal = nullptr;
    BarFn BarOriginal = nullptr;

    void* __cdecl Sprite(int u, int v, int tw, int th, int x, int y, int z, int w, int h)
    {
        // Full-screen fades, flashes and backdrops keep covering the whole screen; markers projected from the 3D view
        // are placed over the whole screen already
        if (Layout::Raw || (w == 642 && h == 450) || (w == 640 && h == 448) || (w == 641 && h == 58) ||
            Layout::Current == Layout::Context::World)
        {
            ++Layout::Raw;
            auto result = SpriteOriginal(u, v, tw, th, x, y, z, w, h);
            --Layout::Raw;
            return result;
        }

        const float shift = Layout::Group ? Layout::Shift : Layout::Classify(x + w * 0.5f);
        float left = Layout::PixelX(static_cast<float>(x), shift);
        float right = Layout::PixelX(static_cast<float>(x + w), shift);
        if (Layout::Current == Layout::Context::Mask && tw == 0 && th == 0)
        {
            // The black sides of scope masks reach the screen edges
            if (std::min(x, x + w) <= 0)
                (w >= 0 ? left : right) = 0.0f;
            if (std::max(x, x + w) >= 640)
                (w >= 0 ? right : left) = Screen.fWidth;
        }
        const int32_t newX = Layout::ToX(left);
        const int32_t newW = Layout::ToX(right) - newX;
        ++Layout::Raw;
        auto result = SpriteOriginal(u, v, tw, th, newX, Layout::Y(static_cast<float>(y)), z, newW, Layout::Height(static_cast<float>(h)));
        --Layout::Raw;
        return result;
    }

    // x is the glyph's left edge; in vertical text it is its top and the line's x comes from Position
    void* __cdecl Glyph(int u, int v, int tw, int th, int x, int w, int h)
    {
        if (Layout::Raw)
            return GlyphOriginal(u, v, tw, th, x, w, h);
        if (bVerticalText)
            return GlyphOriginal(u, v, tw, th, Layout::Y(static_cast<float>(x)), Layout::Width(static_cast<float>(w)), Layout::Height(static_cast<float>(h)));
        const int32_t newX = Layout::X(static_cast<float>(x), Layout::Shift);
        const int32_t newW = Layout::X(static_cast<float>(x + w), Layout::Shift) - newX;
        return GlyphOriginal(u, v, tw, th, newX, newW, Layout::Height(static_cast<float>(h)));
    }

    void* __cdecl Position(int x, int y, int z)
    {
        if (Layout::Raw)
            return PositionOriginal(x, y, z);
        auto result = PositionOriginal(Layout::X(static_cast<float>(x), Layout::Shift), Layout::Y(static_cast<float>(y)), z);
        // The text cursor stays in the game's units: its glyphs are moved one by one
        nTextCursorX = x;
        nTextCursorY = y;
        return result;
    }

    void* __cdecl Quad(float* points, float* uv)
    {
        if (Layout::Raw || !points || Layout::Current == Layout::Context::World)
            return QuadOriginal(points, uv);
        float moved[12];
        std::copy_n(points, 12, moved);
        const float center = (points[0] + points[3] + points[6] + points[9]) * 0.25f;
        const float shift = Layout::Group ? Layout::Shift : Layout::Classify(center);
        for (int i = 0; i < 4; ++i)
        {
            moved[i * 3 + 0] = static_cast<float>(Layout::X(points[i * 3 + 0], shift));
            moved[i * 3 + 1] = static_cast<float>(Layout::Y(points[i * 3 + 1]));
        }
        return QuadOriginal(moved, uv);
    }

    // One string is one element: measured first, its glyphs and lines share the shift
    int __cdecl Text(int** format, void* a2, int measure, int* width, void* a5)
    {
        if (measure || Layout::Raw || !format || !*format)
            return TextOriginal(format, a2, measure, width, a5);
        const float previous = Layout::Shift;
        if (!Layout::Group)
        {
            int textWidth = 0;
            if (Layout::InHud && Layout::Current == Layout::Context::Normal && Screen.fHudShift > 0.0f)
                TextOriginal(format, nullptr, 1, &textWidth, nullptr);
            Layout::Shift = Layout::Classify((*format)[12] + textWidth * 0.5f);
        }
        ++Layout::Group;
        const int result = TextOriginal(format, a2, measure, width, a5);
        --Layout::Group;
        Layout::Shift = previous;
        return result;
    }

    // Health and Focus meters with their labels
    void* __cdecl Bar(int x, int y, int w, int h, int alpha, int texture, float fill, int text, float a9, int a10)
    {
        const float previous = Layout::Shift;
        if (!Layout::Group)
            Layout::Shift = Layout::Classify(x + w * 0.5f);
        ++Layout::Group;
        auto result = BarOriginal(x, y, w, h, alpha, texture, fill, text, a9, a10);
        --Layout::Group;
        Layout::Shift = previous;
        return result;
    }

    // A black rect in screen pixels
    void Black(float left, float top, float right, float bottom)
    {
        if (right <= left || bottom <= top)
            return;
        (*pfnSetTexture)(0);
        (*pfnSetColor)(0, 0, 0, 255, 0);
        ++Layout::Raw;
        const int32_t x = Layout::ToX(left), y = Layout::ToY(top);
        (*pfnDrawSprite)(0, 0, 0, 0, x, y, 10, Layout::ToX(right) - x + 1, Layout::ToY(bottom) - y + 1);
        --Layout::Raw;
    }

    // A line of text in the 640x448 frame
    void Line(int32_t font, const std::wstring& text, int32_t x, int32_t y)
    {
        int32_t format[32]{};
        format[0] = font;
        format[12] = x;
        format[13] = y;
        format[15] = 16;
        format[16] = 16;
        format[17] = 1;
        format[18] = 3;
        format[19] = 220;
        format[20] = 220;
        format[21] = 220;
        format[22] = 255;
        format[23] = 600;
        int32_t* parts[2]{ format, reinterpret_cast<int32_t*>(const_cast<wchar_t*>(text.c_str())) };
        (*pfnDrawText)(reinterpret_cast<int**>(parts), nullptr, 0, nullptr, nullptr);
    }
}

// The update notice, drawn with the game's text just before its 2D commands go out
export namespace Notice
{
    std::wstring Text;
    ULONGLONG Until = 0;
}

namespace Notice
{
    SafetyHookInline FlushHook;

    void __cdecl Flush()
    {
        if (Until && GetTickCount64() < Until && nFrontendFont)
            Draw::Line(nFrontendFont, Text, 20, 412);
        FlushHook.ccall();
    }
}

class Draw2D
{
public:
    Draw2D()
    {
        WFP::onInitEvent() += []()
        {
            FixSettings().OnChange("MAIN.HudAspectRatioConstraint", []() { if (Screen.nWidth > 0) Layout::Update(); });

            // 2D commands
            auto pattern = hook::pattern("6A 02 FF 15 ? ? ? ? 83 C4 04 C7 05"); //0x40766C
            pfnDrawSprite.SetAddress(*pattern.get_first<SpriteFn*>(153));
            pfnDrawText.SetAddress(*pattern.get_first<TextFn*>(173));
            Draw::SpriteOriginal = injector::MakeJMP(*pattern.get_first<void*>(157), Draw::Sprite, true).get();
            Draw::TextOriginal = injector::MakeJMP(*pattern.get_first<void*>(177), Draw::Text, true).get();
            Draw::PositionOriginal = injector::MakeJMP(*pattern.get_first<void*>(217), Draw::Position, true).get();
            pattern = hook::pattern("E9 83 00 00 00 8B 45 08 A3 ? ? ? ? 8B 4D 0C 89 0D");
            nTextCursorX.SetAddress(*pattern.get_first<int32_t*>(9));
            nTextCursorY.SetAddress(*pattern.get_first<int32_t*>(9) + 1);
            pattern = hook::pattern("E8 ? ? ? ? 83 C4 1C 83 3D ? ? ? ? 00 75 ? 8B 55 FC"); //0x403882
            bVerticalText.SetAddress(*hook::pattern("C6 01 0C 8B 55 FC 83 C2 01 89 55 FC A1 ? ? ? ? 83 C0 01 A3 ? ? ? ? 8B 4D FC 8A 15").get_first<uint8_t*>(30));
            Draw::GlyphOriginal = injector::MakeJMP(injector::GetBranchDestination(pattern.get_first(0)), Draw::Glyph, true).get();
            pattern = hook::pattern("C7 80 24 02 00 00 ? ? ? ? 8B 4D FC C7 81 28 02 00 00");
            Draw::QuadOriginal = injector::MakeJMP(*pattern.get_first<void*>(6), Draw::Quad, true).get();
            pattern = hook::pattern("E8 ? ? ? ? 83 C4 28 8B 15 ? ? ? ? DB 42 1C"); //0x407013
            Draw::BarOriginal = injector::MakeJMP(injector::GetBranchDestination(pattern.get_first(0)), Draw::Bar, true).get();

            // Update notice
            pattern = hook::pattern("55 8B EC 81 EC ? ? 00 00 83 3D ? ? ? ? 00 75 05 E9 ? ? ? ? 83 3D ? ? ? ? 00 74 0A E8");
            Notice::FlushHook = safetyhook::create_inline(pattern.get_first(), Notice::Flush);
        };
    }
} Draw2D;
