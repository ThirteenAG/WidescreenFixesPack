module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"

export module Hud;

import ComVars;
import Updates;

export
{


struct HudQuad
{
    float x[4];
    float y[4];
    float u[4];
    float v[4];
    uint32_t color[4];
    int32_t texture;
    uint32_t blend;
    uint8_t state[2];
};

// a vertex of the scope and binocular blur quad
struct HudBlur
{
    float x, y, z;
    float blurU, blurV;
    float maskU, maskV;
};

template<size_t index>
struct HudVertexHook
{
    void operator()(injector::reg_pack& regs)
    {
        const auto* quad = reinterpret_cast<const HudQuad*>(regs.eax);
        const auto buffer = *reinterpret_cast<uintptr_t*>(regs.esi + 0x14);
        const auto count = *reinterpret_cast<uint32_t*>(regs.esi);
        auto* vertex = reinterpret_cast<float*>(buffer + count * 0x1C);
        vertex[0] = quad->x[index] - 0.5f;
        vertex[1] = quad->y[index] - 0.5f;
    }
};

int32_t(__cdecl* QueueHudQuad)(HudQuad*);

namespace Textures
{
    enum Type : uint8_t { Other, Menu, Backdrop, Scope, Binoculars };
    Type Types[250]{}; //texture manager capacity
    SafetyHookInline RegisterTexture;

    int32_t __cdecl Register(const char* name, int32_t group, bool* created)
    {
        bool added = false;
        int32_t id = RegisterTexture.unsafe_ccall<int32_t>(name, group, &added);
        if (id < 0 || static_cast<size_t>(id) >= _countof(Types))
            return id;

        if (created)
            *created = added;

        if (added && name)
        {
            static constexpr struct { const char* name; Type type; } textures[] = {
                { "\\Splash\\legal.dds", Backdrop },
                { "\\Splash\\mc2logo.dds", Backdrop },
                { "\\Splash\\namcolog.dds", Backdrop },
                { "\\Splash\\frontscr.dds", Menu },
                { "\\Splash\\backscr.dds", Backdrop },
                { "\\Splash\\oldmenu1.dds", Backdrop },
                { "\\Splash\\profskin.dds", Menu },
                { "\\Splash\\profedit.dds", Menu },
                { "\\Splash\\profsel.dds", Menu },
                { "\\Splash\\Menu25.dds", Menu },
                { "\\HUD\\mauserpc.tga", Scope },
                { "\\HUD\\naganpc.tga", Scope },
                { "\\HUD\\springpc.tga", Scope },
                { "\\HUD\\Binos.tga", Binoculars }
            };
            Types[id] = Other; //handles can get reused after unloading
            for (const auto& texture : textures)
            {
                if (_stricmp(name, texture.name) == 0)
                {
                    Types[id] = texture.type;
                    break;
                }
            }
        }
        return id;
    }
}

namespace Hud
{
    extern float TextWidth, TextOffset;
}

namespace UI
{
    enum DrawMode { Game, Menu, Info };
    DrawMode Mode = Game;
    bool CenteredText = false;
    int32_t FontWidth;
    SafetyHookInline MenuDraw, MenuDrawOverlay, PrintText, PixelClip, DrawLine;
    SafetyHookMid MouseUpdate, MinimapIcons[3], MinimapRect, MinimapCenter;
    uintptr_t CrosshairDraw;
    void(__cdecl* SetViewingArea)(const float*);

    template <DrawMode drawMode, SafetyHookInline& hook, typename Result = int32_t, typename... Args>
    Result __cdecl Draw(Args... args)
    {
        auto mode = std::exchange(Mode, Mode == Info ? Info : drawMode); //preserve info context in nested menus
        auto result = hook.template unsafe_ccall<Result>(args...);
        Mode = mode;
        return result;
    }

    SafetyHookInline LoadingFrame, LoadingText, LoadingProgress, LoadingPercent;
    SafetyHookInline InfoScreen;
    void(__cdecl* FlushQuads)();

    void DrawBackdrop(uint32_t color, bool bordersOnly = false)
    {
        const float width = bordersOnly ? Screen.fHudOffset : Screen.fWidth;
        HudQuad background = { { 0.0f, width, width, 0.0f }, { 0.0f, 0.0f, Screen.fHeight, Screen.fHeight } };
        std::fill(std::begin(background.color), std::end(background.color), color & 0xFF000000);
        background.texture = -1;
        background.blend = 1;
        background.state[0] = background.state[1] = 1;
        QueueHudQuad(&background);
        if (bordersOnly) //cover map icons outside the frame
        {
            background.x[0] = background.x[3] = Screen.fWidth - width;
            background.x[1] = background.x[2] = Screen.fWidth;
            QueueHudQuad(&background);
        }
        FlushQuads(); //backdrop before foreground so the level map can be visible
    }

    void UpdateMouse(injector::reg_pack& regs)
    {
        auto& x = *reinterpret_cast<float*>(regs.esp + sizeof(uintptr_t));
        x -= Screen.fHudOffset;
    }

    void AlignMinimapIcon(injector::reg_pack& regs)
    {
        if (Mode != Game || Screen.fHudOffset <= 0.0f)
            return;

        auto* position = reinterpret_cast<float*>(regs.eax);
        position[0] = (position[0] * Screen.fWidth - Screen.fHudOffset) / Screen.fWidth43; //screen projection to the icon's HUD canvas
    }

    void SquareMinimap(injector::reg_pack& regs)
    {
        if (Mode != Game || Screen.fHudOffset <= 0.0f || *reinterpret_cast<int32_t*>(regs.esi + 0xAC) == 3)
            return;

        auto& width = *reinterpret_cast<float*>(regs.esi + 0x8C);
        const float height = *reinterpret_cast<float*>(regs.esi + 0x90);
        *reinterpret_cast<float*>(regs.esi + 0x94) += width - height; //keep the native right edge
        width = height;
    }

    void CenterMinimap(injector::reg_pack& regs)
    {
        if (Mode != Game || Screen.fHudOffset <= 0.0f)
            return;

        auto& delta = *reinterpret_cast<float*>(regs.esp + 0x14);
        delta *= Screen.fWidth / Screen.fWidth43; //pixel displacement to the map's HUD canvas
    }

    float TextShift = 0.0f; //game text in the HUD box
    int32_t __cdecl DrawMenu();

    char __cdecl Print(const wchar_t* text, const float* clip, float x, float y,
        float spacing, float scale, uint32_t color, uint32_t format);

    float* __cdecl GetPixelClip(float* rect, float* x, float* y, const float* clip, uint32_t format)
    {
        auto width = std::exchange(FontWidth, (Mode != Game || (format & 1)) ? Screen.Width43 : static_cast<int32_t>(Hud::TextWidth));
        auto result = PixelClip.unsafe_ccall<float*>(rect, x, y, clip, format);
        FontWidth = width;
        return result;
    }

    void __cdecl Line(const float* from, const float* to, uint32_t color)
    {
        const float offset = Mode != Game ? Screen.fHudOffset : 0.0f;
        const float a[] = { from[0] + offset, from[1] };
        const float b[] = { to[0] + offset, to[1] };
        DrawLine.unsafe_ccall(a, b, color);
    }

    void __cdecl ProfileViewingArea(const float* area)
    {
        const float ratio = Screen.fWidth43 / Screen.fWidth;
        const float viewport[] = { area[0] * ratio, area[1],
            area[2] * ratio + Screen.fHudOffset / Screen.fWidth, area[3] };
        SetViewingArea(viewport);
    }

    float ActionManagerTextScale = 0.0f, GameplayTextObjectScale = 0.0f;

    float ScaleText()
    {
        const float widthScale = Screen.fWidth / 1024.0f;
        const float heightScale = Screen.fHeight / 768.0f;
        return heightScale / sqrtf(sqrtf((widthScale * widthScale + heightScale * heightScale) * 0.5f));
    }

    void RedirectWidth(const uint8_t* begin, const uint8_t* end, uintptr_t nativeWidth, const int32_t* width)
    {
        hook::range_pattern((uintptr_t)begin, (uintptr_t)end, pattern_str(to_bytes(nativeWidth))).for_each_result(
            [=](hook::pattern_match match) { injector::WriteMemory<const int32_t*>(match.get<const int32_t*>(), width, true); });
    }
}

// The HUD box: Auto puts the side elements at the screen's edges, a ratio keeps them in a centred box of that aspect.
// The HUD tree keeps its 4:3 canvas (sizes and centred elements); the top-level elements at a side move outwards.
namespace Hud
{
    std::atomic<float> Shift{ 0.0f };
    float TextWidth = 0.0f, TextOffset = 0.0f;
    SafetyHookMid Anchor;

    void Update()
    {
        float width = Screen.fWidth;
        const auto ratio = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        if (ratio && std::isfinite(*ratio) && *ratio > 0.0f)
            width = Screen.fHeight * ClampHudAspectRatio(*ratio, Screen.fAspectRatio);
        Shift.store(std::max(0.0f, (width - Screen.fWidth43) / 2.0f), std::memory_order_relaxed);
        TextWidth = width;
        TextOffset = (Screen.fWidth - width) / 2.0f;
    }

    void AnchorElement(SafetyHookContext& context)
    {
        const auto node = context.esi;
        const auto parent = *reinterpret_cast<uintptr_t*>(node + 0x04);
        if (!parent || *reinterpret_cast<uintptr_t*>(parent + 0x04))
            return;
        const auto root = reinterpret_cast<const float*>(parent);
        if (fabsf(root[10] - Screen.fHudOffset) > 1.0f || fabsf(root[12] - Screen.fWidth43) > 1.0f)
            return; //only the HUD's 4:3 canvas
        auto* rect = reinterpret_cast<float*>(node);
        const float width = rect[8], center = rect[6] + width / 2.0f;
        if (width > 0.6f)
            return;
        const float shift = Shift.load(std::memory_order_relaxed);
        const float side = center < 0.45f ? -1.0f : center > 0.55f ? 1.0f : 0.0f;
        rect[10] += side * shift; //the element's pictures; its text moves in Print
    }
}

int32_t __cdecl UI::DrawMenu()
{
    const auto result = Draw<Menu, MenuDraw>();
    const auto mode = std::exchange(Mode, Menu);
    SniperUpdates::Draw();
    Mode = mode;
    return result;
}

char __cdecl UI::Print(const wchar_t* text, const float* clip, float x, float y,
    float spacing, float scale, uint32_t color, uint32_t format)
{
    auto centered = std::exchange(CenteredText, Mode != Game || (format & 1));
    auto width = std::exchange(FontWidth, CenteredText ? Screen.Width43 : static_cast<int32_t>(Hud::TextWidth));
    float side = 0.0f;
    if (Mode == Game && (format & 1)) //HUD text on the 4:3 canvas: with its element at the side it is on
    {
        const float position = x > 1.5f ? x / Screen.fWidth43 : x;
        side = position < 0.45f ? -1.0f : position > 0.55f ? 1.0f : 0.0f;
    }
    auto shift = std::exchange(TextShift, CenteredText ? side * Hud::Shift.load(std::memory_order_relaxed) : Hud::TextOffset);
    auto result = PrintText.unsafe_ccall<char>(text, clip, x, y, spacing, scale, color, format);
    TextShift = shift;
    FontWidth = width;
    CenteredText = centered;
    return result;
}





inline bool Near(float a, float b)
{
    return fabsf(a - b) < 2.0f;
}

int32_t __cdecl sub_48B140Hook(HudQuad* quad)
{
    const auto type = (uint32_t)quad->texture < _countof(Textures::Types) ? Textures::Types[quad->texture] : Textures::Other;
    float scale = (UI::Mode != UI::Game || UI::CenteredText) ? 1.0f : 0.0f;
    if (!scale && LoadingTexture.is_initialized() && quad->texture != -1 && Screen.fHudOffset > 0.0f &&
        Near(quad->x[0], 0.0f) && Near(quad->x[1], Screen.fWidth))
    {
        bool loading = quad->texture == LoadingTexture.get();
        if (loading || type == Textures::Menu || type == Textures::Backdrop)
        {
            if (loading || type == Textures::Backdrop)
                UI::DrawBackdrop(quad->color[0]);

            scale = Screen.fWidth43 / Screen.fWidth;
        }
    }

    if (scale)
    {
        HudQuad centered{};
        memcpy(&centered, quad, 0x5A);
        for (float& x : centered.x)
            x = Screen.fHudOffset + x * scale + (UI::Mode == UI::Game ? UI::TextShift : 0.0f);

        if (UI::Mode == UI::Info &&
            Near(quad->x[0], 0.0f) && Near(quad->x[1], Screen.fWidth43) &&
            Near(quad->y[0], 0.0f) && Near(quad->y[2], Screen.fHeight))
        {
            if (quad->texture == -1)
            {
                centered.x[0] = centered.x[3] = 0.0f;
                centered.x[1] = centered.x[2] = Screen.fWidth; //fullscreen coverage
            }
            else if (Screen.fHudOffset > 0.0f)
                UI::DrawBackdrop(quad->color[0], true);
        }
        return QueueHudQuad(&centered);
    }

    if (!scale && UI::TextShift != 0.0f && UI::Mode == UI::Game)
    {
        HudQuad shifted{};
        memcpy(&shifted, quad, 0x5A);
        for (float& x : shifted.x)
            x += UI::TextShift;
        return QueueHudQuad(&shifted);
    }

    const float right = Screen.fWidth - Screen.fHudOffset;

    if (Screen.fHudOffset > 0.0f && Near(quad->x[0], Screen.fHudOffset) &&
        (type == Textures::Scope || type == Textures::Binoculars))
    {
        HudQuad Strip{};
        memcpy(&Strip, quad, 0x5A);

        const bool binoculars = type == Textures::Binoculars;
        const float uWidth = quad->u[1] - quad->u[0];
        const float scopeBorder = 32.5f / 1024.0f;
        const float scopeInset = uWidth * scopeBorder;

        const float leftU = binoculars ? 0.5f / 512.0f : quad->u[0] + scopeInset;
        const float rightU = binoculars ? 0.5f / 512.0f : quad->u[1] - scopeInset;

        const float borderWidth = binoculars ? 0.0f : (right - Screen.fHudOffset) * scopeBorder;
        const float centerLeft = Screen.fHudOffset + borderWidth;
        const float centerRight = right - borderWidth;

        Strip.x[0] = Strip.x[3] = 0.0f;
        Strip.x[1] = Strip.x[2] = centerLeft;
        Strip.u[0] = Strip.u[1] = Strip.u[2] = Strip.u[3] = leftU;
        QueueHudQuad(&Strip);

        Strip.x[0] = Strip.x[3] = centerRight;
        Strip.x[1] = Strip.x[2] = Screen.fWidth;
        Strip.u[0] = Strip.u[1] = Strip.u[2] = Strip.u[3] = rightU;
        QueueHudQuad(&Strip);

        HudQuad Center{};
        memcpy(&Center, quad, 0x5A);

        Center.x[0] = Center.x[3] = centerLeft;
        Center.x[1] = Center.x[2] = centerRight;
        if (!binoculars)
        {
            Center.u[0] = Center.u[3] = leftU;
            Center.u[1] = Center.u[2] = rightU;
        }
        return QueueHudQuad(&Center);
    }
    return QueueHudQuad(quad);
}

SafetyHookMid ReticleHorizontal[3], ReticleVertical[3];

inline float ReticleX(float x)
{
    return 0.5f + (x - 0.5f) * (Screen.fWidth - 2.0f * Screen.fHudOffset) / Screen.fWidth;
}

void AdjustReticleHorizontal(injector::reg_pack& regs)
{
    auto* args = reinterpret_cast<float*>(regs.esp);
    args[0] = ReticleX(args[0]);
    args[1] = ReticleX(args[1]);
}
void AdjustReticleVertical(injector::reg_pack& regs)
{
    auto& x = *reinterpret_cast<float*>(regs.esp);
    x = ReticleX(x);
}


}
