module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <charconv>
#include <array>
#include <cmath>
#include <utility>
#include <format>
#include <string>
#include <algorithm>

export module Hud;


// The HUD is laid out in an interface rectangle (left, top, right, bottom as parts of the screen) that the game fits
// to a 16:9 design box. It covers the whole screen instead (MAIN.HudAspectRatioConstraint Auto), or a centred box
// of the chosen aspect.
export namespace CondemnedHud
{
    inline std::atomic<float> constraint{ 0.0f };
    inline SafetyHookInline rectangleHook;

    inline float Number(std::string_view text)
    {
        while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
        while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
        float value = 0.0f;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        if (text.empty() || parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || !std::isfinite(value) || value <= 0.0f)
            return 0.0f;
        return value;
    }

    // "Auto" (or anything unreadable) = 0, the whole screen; "W:H" or a decimal, clamped to 4:3 to 32:9
    inline float ParseAspect(std::string_view text)
    {
        const auto colon = text.find(':');
        float value = Number(colon == std::string_view::npos ? text : text.substr(0, colon));
        if (colon != std::string_view::npos)
        {
            const auto height = Number(text.substr(colon + 1));
            value = height > 0.0f ? value / height : 0.0f;
        }
        return std::isfinite(value) && value > 0.0f ? std::clamp(value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f;
    }

    inline float* __stdcall Rectangle(float* rectangle, float width, float height)
    {
        rectangle[0] = rectangle[1] = 0.0f;
        rectangle[2] = rectangle[3] = 1.0f;
        const float box = constraint;
        if (!(width > 0.0f) || !(height > 0.0f) || box <= 0.0f)
            return rectangle;
        const float screen = width / height;
        if (box < screen)
        {
            rectangle[0] = (1.0f - box / screen) * 0.5f;
            rectangle[2] = 1.0f - rectangle[0];
        }
        else
        {
            rectangle[1] = (1.0f - screen / box) * 0.5f;
            rectangle[3] = 1.0f - rectangle[1];
        }
        return rectangle;
    }

    // A changed setting lays the HUD out again on the game's thread, as after a resolution change
    inline std::atomic<bool> relayout{ false };
    inline uint8_t** interfaceManager = nullptr;

    inline void Frame()
    {
        if (!relayout.exchange(false) || !interfaceManager || !*interfaceManager)
            return;
        const auto hud = *interfaceManager + 0x3970;
        using UpdateLayout = void(__thiscall*)(void*);
        reinterpret_cast<UpdateLayout>((*reinterpret_cast<void***>(hud))[6])(hud);
    }

    // Menus: their layout is made for 16:9 and was stretched to the screen's width on wider screens. While the screen
    // manager draws, the screen-space 2D transform puts everything in a centred 16:9 box, and the mouse positions it
    // passes to the menus are mapped into that box.
    inline thread_local int menuDepth = 0;
    inline float screenWidth = 0.0f, screenHeight = 0.0f;

    // Width of the 16:9 box as a part of the screen width, 1 on screens up to 16:9
    inline float MenuBoxScale()
    {
        if (!(screenWidth > 0.0f) || !(screenHeight > 0.0f))
            return 1.0f;
        return (std::min)(1.0f, screenHeight * (16.0f / 9.0f) / screenWidth);
    }

    // The screen-space transform's x row (scale, offset) of a draw inside the menus
    inline void MenuTransform(float width, float height, float& scaleX, float& offsetX)
    {
        screenWidth = width;
        screenHeight = height;
        if (menuDepth <= 0)
            return;
        const float box = MenuBoxScale();
        scaleX *= box;
        offsetX = -box;
    }

    inline int MenuX(int x)
    {
        const float box = MenuBoxScale();
        if (box >= 1.0f)
            return x;
        const float left = screenWidth * (1.0f - box) * 0.5f;
        return int(std::lround((float(x) - left) / box));
    }

    using ScreenRender = char(__thiscall*)(void*);
    using ScreenMouse = char(__thiscall*)(void*, int, int);
    inline ScreenRender RenderOriginal = nullptr;
    inline std::array<std::array<ScreenMouse, 7>, 3> MouseOriginal{};
    inline SafetyHookInline MenuRenderHook;

    // The screen manager (front end, pause and loading screens) and the menu managers (in-game menus) draw
    inline char __fastcall Render(void* manager, void*)
    {
        ++menuDepth;
        const char result = RenderOriginal(manager);
        --menuDepth;
        return result;
    }

    inline char __fastcall MenuRender(void* manager, void*)
    {
        ++menuDepth;
        const char result = MenuRenderHook.unsafe_thiscall<char>(manager);
        --menuDepth;
        return result;
    }

    // A menu's own drawing (panel and items), also when something else than a menu manager draws it
    inline SafetyHookInline MenuPanelHook;

    inline char __fastcall MenuPanel(void* menu, void*)
    {
        ++menuDepth;
        const char result = MenuPanelHook.unsafe_thiscall<char>(menu);
        --menuDepth;
        return result;
    }

    // Untextured quads (position, colour): one over the whole screen (the darkening behind the in-game menus, fades)
    // keeps covering it
    using DrawFlatQuads = int(__thiscall*)(void*, float*, int);
    inline DrawFlatQuads DrawFlatQuadsOriginal = nullptr;

    inline int __fastcall FlatQuads(void* drawPrim, void*, float* quads, int count)
    {
        const float box = MenuBoxScale();
        if (menuDepth > 0 && box < 1.0f && quads && count > 0)
        {
            constexpr int stride = 4;
            const float left = screenWidth * (1.0f - box) * 0.5f;
            for (int q = 0; q < count; ++q)
            {
                float* quad = quads + q * 4 * stride;
                float minX = quad[0], maxX = quad[0];
                for (int v = 1; v < 4; ++v)
                {
                    minX = (std::min)(minX, quad[v * stride]);
                    maxX = (std::max)(maxX, quad[v * stride]);
                }
                if (minX <= 0.5f && maxX >= screenWidth - 0.5f)
                    for (int v = 0; v < 4; ++v)
                        quad[v * stride] = (quad[v * stride] - left) / box;
            }
        }
        return DrawFlatQuadsOriginal(drawPrim, quads, count);
    }

    template<size_t Table, size_t I>
    char __fastcall Mouse(void* manager, void*, int x, int y)
    {
        return MouseOriginal[Table][I](manager, MenuX(x), y);
    }

    // Seven mouse button and move handlers (x, y) from the given slot on
    template<size_t Table>
    void HookMouse(void** table, size_t first)
    {
        [&]<size_t... I>(std::index_sequence<I...>)
        {
            ((MouseOriginal[Table][I] = reinterpret_cast<ScreenMouse>(table[first + I]),
                injector::WriteMemory(&table[first + I], &Mouse<Table, I>, true)), ...);
        }(std::make_index_sequence<7>{});
    }

    // Text in the menus keeps its glyph widths (fonts are sized by height): each string is placed in the box at its
    // left edge, and its quads are widened by the inverse of the box scale around that edge before the transform.
    inline thread_local int textDepth = 0;
    inline thread_local bool textAnchored = false;
    inline thread_local float textAnchor = 0.0f;

    template<int Slot>
    struct TextHook
    {
        static inline SafetyHookInline hook;
    };

    template<int Tag, typename... Args>
    int __stdcall Text(Args... args)
    {
        ++textDepth;
        textAnchored = false;
        const int result = TextHook<Tag>::hook.template unsafe_stdcall<int>(args...);
        --textDepth;
        return result;
    }

    using DrawQuads = int(__thiscall*)(void*, float*, int);
    inline DrawQuads DrawQuadsOriginal = nullptr;

    inline int __fastcall Quads(void* drawPrim, void*, float* quads, int count)
    {
        const float box = MenuBoxScale();
        constexpr int stride = 6;   // per vertex: position (3), colour, u, v
        if (textDepth <= 0 && menuDepth > 0 && box < 1.0f && quads && count > 0)
        {
            // Untextured quads over the whole screen (fades) keep covering it
            const float left = screenWidth * (1.0f - box) * 0.5f;
            for (int q = 0; q < count; ++q)
            {
                float* quad = quads + q * 4 * stride;
                float minX = quad[0], maxX = quad[0];
                bool flat = true;
                for (int v = 1; v < 4; ++v)
                {
                    minX = (std::min)(minX, quad[v * stride]);
                    maxX = (std::max)(maxX, quad[v * stride]);
                    flat = flat && quad[v * stride + 4] == quad[4] && quad[v * stride + 5] == quad[5];
                }
                if (flat && minX <= 0.5f && maxX >= screenWidth - 0.5f)
                    for (int v = 0; v < 4; ++v)
                        quad[v * stride] = (quad[v * stride] - left) / box;
            }
        }
        if (textDepth > 0 && menuDepth > 0 && box < 1.0f && quads && count > 0)
        {
            const int vertices = count * 4;
            if (!textAnchored)
            {
                textAnchor = quads[0];
                for (int i = 1; i < vertices; ++i)
                    textAnchor = (std::min)(textAnchor, quads[i * stride]);
                textAnchored = true;
            }
            for (int i = 0; i < vertices; ++i)
                quads[i * stride] = textAnchor + (quads[i * stride] - textAnchor) / box;
        }
        return DrawQuadsOriginal(drawPrim, quads, count);
    }

    // An engine interface instance by its registered name ("ILTDrawPrim.Default", ...)
    inline void** EngineInterface(const char* name)
    {
        const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        const auto pe = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
        const auto sections = IMAGE_FIRST_SECTION(pe);
        const auto data = std::find_if(sections, sections + pe->FileHeader.NumberOfSections,
            [](const IMAGE_SECTION_HEADER& section) { return std::memcmp(section.Name, ".rdata", 6) == 0; });
        std::string bytes;
        for (const char* c = name; ; ++c)
        {
            bytes += std::format("{:02X} ", uint8_t(*c));
            if (!*c)
                break;
        }
        const auto start = base + data->VirtualAddress;
        const auto text = reinterpret_cast<uintptr_t>(hook::range_pattern(start, start + data->Misc.VirtualSize, bytes).get_first());
        const auto registration = std::format("6A 00 68 {:02X} {:02X} {:02X} {:02X} 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 C3",
            text & 0xFF, (text >> 8) & 0xFF, (text >> 16) & 0xFF, (text >> 24) & 0xFF);
        return *hook::pattern(registration).get_first<void**>(8);
    }

    inline void InitMenus(HMODULE gameClient)
    {
        // Text strings: the four drawing methods of the engine's texture strings
        {
            auto table = *reinterpret_cast<void***>(EngineInterface("ILTTextureString.Default"));
            TextHook<18>::hook = safetyhook::create_inline(table[18], &Text<18, void*, void*, void*, int, int, int, int, int, int>);
            TextHook<19>::hook = safetyhook::create_inline(table[19], &Text<19, void*, void*, void*, int, int, int, int>);
            TextHook<20>::hook = safetyhook::create_inline(table[20], &Text<20, void*, void*, void*, int, int, int, int, int>);
            TextHook<21>::hook = safetyhook::create_inline(table[21], &Text<21, void*, void*, void*, int, int, int>);
            auto drawPrim = *reinterpret_cast<void***>(EngineInterface("ILTDrawPrim.Default"));
            DrawQuadsOriginal = reinterpret_cast<DrawQuads>(drawPrim[17]);
            injector::WriteMemory(&drawPrim[17], &Quads, true);
            DrawFlatQuadsOriginal = reinterpret_cast<DrawFlatQuads>(drawPrim[16]);
            injector::WriteMemory(&drawPrim[16], &FlatQuads, true);
        }

        // The screen manager's table (from its constructor): mouse handlers in slots 6 to 12, drawing in slot 23
        auto pattern = hook::pattern(gameClient, "C7 06 ? ? ? ? 33 DB 57 8D 4E 74 89 5E 68");
        const auto screens = *pattern.get_first<void**>(2);
        RenderOriginal = reinterpret_cast<ScreenRender>(screens[23]);
        injector::WriteMemory(&screens[23], &Render, true);
        HookMouse<0>(screens, 6);

        // The in-game menu managers (player and system menus): mouse handlers in slots 7 to 13, and their drawing
        pattern = hook::pattern(gameClient, "8D 4F 34 C7 07 ? ? ? ? E8 ? ? ? ? 8D BE 38 13 00 00");
        HookMouse<1>(*pattern.get_first<void**>(5), 7);
        pattern = hook::pattern(gameClient, "8D 4F 34 C7 07 ? ? ? ? E8 ? ? ? ? 8D 8E B0 20 00 00");
        HookMouse<2>(*pattern.get_first<void**>(5), 7);
        pattern = hook::pattern(gameClient, "8A 41 30 84 C0 74 0E 8B 41 08 85 C0 74 07 8B C8 8B 01 FF 60 10 C3");
        MenuRenderHook = safetyhook::create_inline(pattern.get_first(), &MenuRender);
        pattern = hook::pattern(gameClient, "56 8B F1 8A 46 42 84 C0 74 44 8A 86 34 0D 00 00 84 C0");
        MenuPanelHook = safetyhook::create_inline(pattern.get_first(), &MenuPanel);
    }

    inline void Init(HMODULE gameClient)
    {
        InitMenus(gameClient);
        auto pattern = hook::pattern(gameClient, "83 EC 08 8B 0D ? ? ? ? 85 C9 C7 44 24 04 00 00 10 3F C7 04 24 39 8E E3 3F");
        const auto refresh = []() { constraint = ParseAspect(FixSettings().String("MAIN.HudAspectRatioConstraint")); };
        refresh();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", [refresh]()
        {
            refresh();
            relayout = true;
        });
        rectangleHook = safetyhook::create_inline(pattern.get_first(), Rectangle); //0x1006D5C0

        pattern = hook::pattern(gameClient, "88 9E 95 01 00 00 E8 ? ? ? ? 8B 0D ? ? ? ? 8B 91 70 39 00 00 81 C1 70 39 00 00");
        interfaceManager = *pattern.get_first<uint8_t**>(13);
    }
}
