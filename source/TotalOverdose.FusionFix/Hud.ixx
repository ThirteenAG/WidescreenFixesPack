module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Hud;

import ComVars;

// Widescreen layout: the engine has its own widescreen layout for the HUD (the consoles' widescreen mode); its flag is
// otherwise set only by the "widescreen_emulation" development setting, which also forces a resolution. The game reads it
// when it lays out the HUD.
// The HUD is laid out in a virtual screen (853x480 in the widescreen mode) scaled by the screen height; the engine
// assumes a 16:9 television and squeezes or widens it at other aspects, so the fix keeps its units square.
// HUD elements have fixed places in that virtual screen. On wider screens (and for MAIN.HudAspectRatioConstraint) each
// element on the left or right moves as a whole by the extra width, elements in the middle stay, and bands as wide as the
// virtual screen span the whole width.
export namespace Hud
{

    void UpdateWideScreen()
    {
        if (wideScreen.is_initialized())
            wideScreen = Screen.fAspectRatio > 4.0f / 3.0f;
    }

    // Text marked "scale if widescreen" is drawn 4:3 taller in the widescreen mode; the factor shares its constant with
    // the camera's view area, which the fix changes, so the text gets its own
    float textHeight = 4.0f / 3.0f;

    SafetyHookInline ScreenSizeHook;
    int __fastcall SetScreenSize(void* view, void*, float width, float height, float pixelAspect, float zoom)
    {
        if (wideScreen.is_initialized() && wideScreen)
            pixelAspect = 1.0f;
        return ScreenSizeHook.unsafe_thiscall<int>(view, width, height, pixelAspect, zoom);
    }

    float hudAspect = 0.0f;
    std::atomic<uint32_t> frame = 1, layout = 1;
    void UpdateHudAspect()
    {
        const auto parsed = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        hudAspect = parsed ? std::clamp(*parsed, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
        ++layout;
    }

    // The 2D element being drawn (sprite or text node). An element is classified by the area its parent's elements
    // covered in the previous frame, so the pieces of one box or bar move together.
    uintptr_t element = 0;

    struct Area
    {
        float left = FLT_MAX, right = -FLT_MAX;
        void Add(const Area& other)
        {
            left = std::min(left, other.left);
            right = std::max(right, other.right);
        }
        bool Valid() const { return left <= right; }
    };

    struct Group
    {
        uint32_t frame = 0;
        Area current, last;
    };
    std::unordered_map<uintptr_t, Group> groups;

    using WorldMatrixFn = float*(__thiscall*)(uintptr_t node, float* matrix);
    WorldMatrixFn WorldMatrix = nullptr;

    // The element's area in the virtual screen, from its world matrix (translation plus half its size, as drawn)
    Area Bounds(uintptr_t node)
    {
        float matrix[16];
        WorldMatrix(node, matrix);
        const float width = *reinterpret_cast<const float*>(node + 128), height = *reinterpret_cast<const float*>(node + 132);
        const float center = matrix[12] + width * 0.5f;
        const float half = (std::abs(matrix[0]) * width + std::abs(matrix[4]) * height) * 0.5f;
        return { center - half, center + half };
    }

    uintptr_t Parent(uintptr_t node)
    {
        return *reinterpret_cast<const uintptr_t*>(node + 64);
    }

    bool Virtual(uintptr_t node)
    {
        return *reinterpret_cast<const uint32_t*>(node + 156) & 0x40;
    }

    void Record(uintptr_t node)
    {
        const auto parent = Parent(node);
        if (!parent)
            return;
        if (groups.size() > 4096)
            groups.clear();
        auto& group = groups[parent];
        const uint32_t now = frame;
        if (group.frame != now)
        {
            if (group.current.Valid())
                group.last = group.current;
            group.current = {};
            group.frame = now;
        }
        group.current.Add(Bounds(node));
    }

    Area Classified(uintptr_t node)
    {
        if (const auto parent = Parent(node))
        {
            auto found = groups.find(parent);
            if (found != groups.end() && found->second.last.Valid())
                return found->second.last;
        }
        return Bounds(node);
    }

    // Extra width (virtual units) on each side of the virtual screen, for the screen or the HUD box
    float Extra(const float* view)
    {
        const float width = view[0], scale = view[11];
        if (!wideScreen.is_initialized() || !wideScreen || width <= 0.0f || scale <= 0.0f)
            return 0.0f;
        const float screen = view[4] / scale;
        const float box = hudAspect > 0.0f ? std::min(screen, hudAspect * view[5] / scale) : screen;
        const float extra = (box - width) * 0.5f;
        return std::abs(extra) < 0.5f ? 0.0f : extra;
    }

    const float* view = nullptr;
    bool text = false;

    float Anchor(const float* screen, float x)
    {
        if (!element || !Virtual(element))
            return x;
        const float extra = Extra(screen);
        if (extra == 0.0f)
            return x;
        const float width = screen[0];
        const auto area = Classified(element);
        if (area.right - area.left >= width * 0.9f)
            return text ? x : (x - width * 0.5f) * (width + extra * 2.0f) / width + width * 0.5f;
        const float center = (area.left + area.right) * 0.5f;
        if (center < width * 0.4f)
            return x - extra;
        if (center > width * 0.6f)
            return x + extra;
        return x;
    }

    SafetyHookInline ToScreenHook;
    float* __fastcall ToScreen(float* screen, void*, float* out, float x, float y)
    {
        view = screen;
        return ToScreenHook.unsafe_thiscall<float*>(screen, out, Anchor(screen, x), y);
    }

    SafetyHookInline SpriteHook, SpriteNodeHook, TextNodeHook;
    template <SafetyHookInline& Hook, typename R, bool Text>
    R __fastcall Draw(uintptr_t node, void*)
    {
        const auto previous = element;
        const bool previousText = text;
        element = node;
        text = Text;
        if (Virtual(node) && view)
        {
            Record(node);
            // Elements keep their drawn corners until they change; they are placed anew while the HUD is moved and once
            // after the HUD box changes
            static std::unordered_map<uintptr_t, uint32_t> placed;
            if (placed.size() > 8192)
                placed.clear();
            auto& last = placed[node];
            if (Extra(view) != 0.0f || last != layout)
                *reinterpret_cast<uint32_t*>(node + 156) |= 0x1000000;
            last = layout;
        }
        if constexpr (std::is_void_v<R>)
        {
            Hook.unsafe_thiscall<void>(node);
            element = previous;
            text = previousText;
        }
        else
        {
            const R result = Hook.unsafe_thiscall<R>(node);
            element = previous;
            text = previousText;
            return result;
        }
    }

    // Full-screen pictures (title, menu backgrounds) are 4:3 art drawn across the whole width; they get a centered 4:3
    // area at full height instead
    using DrawQuadFn = int(__thiscall*)(void* renderer, int texture, float* position, float* size, float* color);
    DrawQuadFn DrawQuad = nullptr;
    SafetyHookInline FullScreenHook;
    int __fastcall DrawFullScreen(int* renderer, void*, int texture)
    {
        const float width = static_cast<float>(renderer[202]), height = static_cast<float>(renderer[203]);
        if (std::abs(width * 3.0f - height * 4.0f) < 4.0f)
            return FullScreenHook.unsafe_thiscall<int>(renderer, texture);
        float size[2] = { std::min(width, height * 4.0f / 3.0f), std::min(height, width * 0.75f) };
        float position[2] = { (width - size[0]) * 0.5f, (height - size[1]) * 0.5f };
        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        return DrawQuad(renderer, texture, position, size, color);
    }
}

export void InstallHud()
{
    auto isWide = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 4C 24 04 0F B6 C0 89 01 C2 04 00");
    auto getter = injector::GetBranchDestination(isWide.get_first(6)).get<uint8_t>(); //0x420120: mov al, [flag]; ret
    if (auto address = *reinterpret_cast<uint8_t**>(getter + 1))
        wideScreen.SetAddress(address); //0xA39F12
    Hud::UpdateWideScreen();

    auto screenSize = hook::pattern("8B 44 24 04 8B 54 24 08 F3 0F 10 44 24 0C 89 41 10 8A 41 20 84 C0 89 51 14");
    Hud::ScreenSizeHook = safetyhook::create_inline(screenSize.get_first(), Hud::SetScreenSize); //0x420D90

    // The widescreen mode's 3D projection is fixed at 16:9
    auto projection = hook::pattern("8D BE 64 55 00 00 51 74 07 68 39 8E E3 3F EB");
    injector::WriteMemory<float>(projection.get_first<float>(10), Screen.fAspectRatio, true); //0x44E632

    auto textHeight = hook::pattern("F3 0F 59 15 ? ? ? ? F3 0F 5C D8 5F");
    injector::WriteMemory<float*>(textHeight.get_first<float*>(4), &Hud::textHeight, true); //0x41AA51

    Hud::UpdateHudAspect();
    FixSettings().OnChange("MAIN.HudAspectRatioConstraint", Hud::UpdateHudAspect);
    auto toScreen = hook::pattern("F3 0F 10 51 2C F3 0F 10 44 24 08 F3 0F 5C 41 24 F3 0F 10 4C 24 0C F3 0F");
    auto sprite = hook::pattern("81 EC 04 01 00 00 56 8B F1 8B 86 9C 00 00 00 8B 8E 94 00 00 00 C1 E8 11");
    auto spriteNode = hook::pattern("81 EC 08 01 00 00 55 56 8B F1 8B AE 88 00 00 00 85 ED 0F 84");
    auto textNode = hook::pattern("55 8B EC 83 E4 F8 B8 48 11 00 00 E8 ? ? ? ? 53 55 56 8B F1 8B 8E 9C");
    Hud::WorldMatrix = reinterpret_cast<Hud::WorldMatrixFn>(injector::GetBranchDestination(sprite.get_first(0x28)).as_int()); //0x8F2810
    Hud::ToScreenHook = safetyhook::create_inline(toScreen.get_first(), Hud::ToScreen);                                       //0x420280
    Hud::SpriteHook = safetyhook::create_inline(sprite.get_first(), Hud::Draw<Hud::SpriteHook, int, false>);                  //0x8F4C40
    Hud::SpriteNodeHook = safetyhook::create_inline(spriteNode.get_first(), Hud::Draw<Hud::SpriteNodeHook, void, false>);    //0x8F6CC0
    Hud::TextNodeHook = safetyhook::create_inline(textNode.get_first(), Hud::Draw<Hud::TextNodeHook, void, true>);           //0x8FE280
}

export void InstallFullScreenPictures()
{
    auto fullScreen = hook::pattern("A0 ? ? ? ? F3 0F 2A 81 28 03 00 00 83 EC 20 84 C0 0F 28 C8 F3 0F 59 0D");
    Hud::DrawQuad = reinterpret_cast<Hud::DrawQuadFn>(injector::GetBranchDestination(fullScreen.get_first(0xD0)).as_int()); //0x45D4A0
    Hud::FullScreenHook = safetyhook::create_inline(fullScreen.get_first(), Hud::DrawFullScreen);                     //0x45D940
}

