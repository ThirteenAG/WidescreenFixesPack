module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>


export module Hud;

import ComVars;
import Window;

export namespace TrueCrimeHud
{
    // The game lays out its HUD and subtitles in screen pixels at a fixed size made for 640x480. While they draw, the
    // game's screen size getters report a screen 480 units high (divided by the scale settings) and their 2D vertices
    // are scaled up to the real screen, so both keep their 640x480 proportions at any resolution.
    // The HUD is laid out between screen bounds (side anchors) and places a few elements at fixed distances from the
    // screen width. With a HUD box both are moved inward for the duration of the HUD draw.
    inline std::atomic<float> RequestedAspect{ 0.0f };
    inline std::atomic<float> HudScale{ 1.0f }, SubtitleScale{ 1.0f };
    // The subtitle box keeps the scale its current line was laid out with
    inline std::atomic<float> SubtitleLayout{ 1.0f };
    inline SafetyHookInline DrawHook, WidthHook, HeightHook, SubtitleLineHook, SubtitleDrawHook;
    // Subtitle box: length of the current line's text and the time the next line starts (they differ per build)
    inline ptrdiff_t SubtitleLength = 28, SubtitleTimer = 40;
    inline thread_local int Offset = 0;
    inline thread_local bool Drawing = false;
    inline uint32_t HudFrame = 0;

    // Active virtual screen, its factors to the real screen and where it starts there; zero size while nothing is
    // scaled
    inline thread_local int VirtualWidth = 0, VirtualHeight = 0;
    inline thread_local float FactorX = 1.0f, FactorY = 1.0f, OriginX = 0.0f, OriginY = 0.0f;

    // Keeps the active virtual screen and brings it back at the end of the scope
    struct ScreenScope
    {
        int width, height;
        float x, y, originX, originY;

        ScreenScope() : width(VirtualWidth), height(VirtualHeight), x(FactorX), y(FactorY), originX(OriginX), originY(OriginY)
        {
        }

        ~ScreenScope()
        {
            VirtualWidth = width;
            VirtualHeight = height;
            FactorX = x;
            FactorY = y;
            OriginX = originX;
            OriginY = originY;
        }

        ScreenScope(const ScreenScope&) = delete;
        ScreenScope& operator=(const ScreenScope&) = delete;
    };

    // A screen 480 units high (divided by the scale) across the whole real screen
    struct VirtualScreen : ScreenScope
    {
        explicit VirtualScreen(float scale)
        {
            VirtualWidth = 0;
            OriginX = OriginY = 0.0f;
            const int realWidth = WidthHook.ccall<int>();
            const int realHeight = HeightHook.ccall<int>();
            const float factor = static_cast<float>(realHeight) / 480.0f * scale;
            if (realWidth <= 0 || realHeight <= 0 || !(factor > 0.0f))
                return;
            VirtualWidth = std::max(1, static_cast<int>(std::lround(realWidth / factor)));
            VirtualHeight = std::max(1, static_cast<int>(std::lround(realHeight / factor)));
            FactorX = static_cast<float>(realWidth) / static_cast<float>(VirtualWidth);
            FactorY = static_cast<float>(realHeight) / static_cast<float>(VirtualHeight);
        }
    };

    // A fixed-size screen drawn at a factor from a point of the real screen
    struct FixedScreen : ScreenScope
    {
        FixedScreen(int width, int height, float factor, float left, float top)
        {
            VirtualWidth = width;
            VirtualHeight = height;
            FactorX = FactorY = factor;
            OriginX = left;
            OriginY = top;
        }
    };

    // Code that works with the real screen inside a scaled scope
    struct RealScreen : ScreenScope
    {
        RealScreen()
        {
            VirtualWidth = 0;
        }
    };

    inline bool Scaling()
    {
        return VirtualWidth > 0;
    }

    // Pretransformed 2D vertices (x and y first) drawn on the virtual screen go to the real screen
    inline void Transform(float* vertices, uint32_t count, uint32_t stride)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            auto vertex = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(vertices) + i * stride);
            vertex[0] = (vertex[0] + 0.5f) * FactorX + OriginX - 0.5f;
            vertex[1] = (vertex[1] + 0.5f) * FactorY + OriginY - 0.5f;
        }
    }

    inline float Setting(const char* id)
    {
        const float value = ReadNumericSetting(id, 1.0f);
        return std::isfinite(value) ? std::clamp(value, 0.5f, 2.0f) : 1.0f;
    }

    inline int BoxOffset(int width, int height)
    {
        const float requested = RequestedAspect.load(std::memory_order_relaxed);
        if (requested <= 0.0f || width <= 0 || height <= 0)
            return 0;
        const auto w = static_cast<float>(width);
        return std::max(0, static_cast<int>(std::lround((w - std::min(w, static_cast<float>(height) * requested)) * 0.5f)));
    }

    // Screen width: elements measured from the right edge get the right edge of the box
    inline int __cdecl Width()
    {
        return (Scaling() ? VirtualWidth : WidthHook.ccall<int>()) - Offset;
    }

    // Centered elements halve the full width
    inline int __cdecl CenterWidth()
    {
        return Scaling() ? VirtualWidth : WidthHook.ccall<int>();
    }

    inline int __cdecl Height()
    {
        return Scaling() ? VirtualHeight : HeightHook.ccall<int>();
    }

    inline std::vector<SafetyHookMid> Fixed;

    inline void Shift(SafetyHookContext& context, uintptr_t slot)
    {
        auto& x = *reinterpret_cast<int16_t*>(context.esp + slot);
        x = static_cast<int16_t>(x + Offset);
    }

    // Rectangle passed by pointer as a stack argument
    inline void ShiftArgument(SafetyHookContext& context, uintptr_t slot)
    {
        *reinterpret_cast<int16_t*>(*reinterpret_cast<uintptr_t*>(context.esp + slot)) += static_cast<int16_t>(Offset);
    }

    // Called once per frame before the HUD's 2D pass (post effects)
    inline void (*BeforeDraw)() = nullptr;

    inline int __fastcall Draw(uintptr_t hud, void*, int pass, int a2, int a3, int a4)
    {
        if (pass != 4)
            return DrawHook.thiscall<int>(hud, pass, a2, a3, a4);
        if (BeforeDraw)
            BeforeDraw();
        ++HudFrame;
        // The bounds keep their margins from the screen edges
        const int realWidth = WidthHook.ccall<int>();
        const int realHeight = HeightHook.ccall<int>();
        const int left = HudLeftBound.get();
        const int right = HudRightBound.get();
        const int bottom = HudBottomBound.get();
        VirtualScreen screen(HudScale.load(std::memory_order_relaxed));
        const int offset = BoxOffset(VirtualWidth, VirtualHeight);
        HudLeftBound = left + offset;
        HudRightBound = VirtualWidth - (realWidth - right) - offset;
        HudBottomBound = VirtualHeight - (realHeight - bottom);
        Offset = offset;
        Drawing = true;
        const int result = DrawHook.thiscall<int>(hud, pass, a2, a3, a4);
        Drawing = false;
        Offset = 0;
        HudLeftBound = left;
        HudRightBound = right;
        HudBottomBound = bottom;
        return result;
    }

    // Subtitle box: position (x, y) and line width, set from the screen size when a voice line's text loads. A
    // cutscene loads all of its lines at once, so the box is placed again for every line it lays out.
    inline void PlaceSubtitles(uintptr_t box)
    {
        const float unit = static_cast<float>(VirtualWidth) * 0.0015625f;
        *reinterpret_cast<int16_t*>(box + 4) = static_cast<int16_t>(unit * 64.0f);
        *reinterpret_cast<int16_t*>(box + 6) = static_cast<int16_t>(VirtualHeight - 10);
        *reinterpret_cast<int16_t*>(box + 8) = static_cast<int16_t>(unit * 512.0f);
    }

    // Lays out the next line: word wrap, centering and its vertical position
    inline int __fastcall SubtitleLine(uintptr_t box, void*)
    {
        const float scale = SubtitleScale.load(std::memory_order_relaxed);
        VirtualScreen screen(scale);
        if (Scaling())
            PlaceSubtitles(box);
        SubtitleLayout.store(scale, std::memory_order_relaxed);
        return SubtitleLineHook.thiscall<int>(box);
    }

    // Lays out the line on screen again for a changed scale. The text pointer sits after the space that ends the
    // line's timestamp; wraps the game put in (newlines in place of spaces) become spaces again.
    inline void RelayoutSubtitles(uintptr_t box)
    {
        auto& text = *reinterpret_cast<char**>(box + 20);
        auto& length = *reinterpret_cast<uint32_t*>(box + SubtitleLength);
        const auto end = *reinterpret_cast<char**>(box + 16);
        if (!text || !end || !length || text[-1] != ' ')
            return;
        for (char* c = text + 1; c < text + length; ++c)
        {
            if (*c == '\n' && c[-1] != '\r')
                *c = ' ';
        }
        auto& timer = *reinterpret_cast<int*>(box + SubtitleTimer);
        const int next = timer;
        --text;
        length = 0;
        SubtitleLine(box, nullptr);
        timer = next;
    }

    inline int __fastcall SubtitleDraw(uintptr_t box, void*, int pass, int a2, int a3, int a4)
    {
        if (pass != 4)
            return SubtitleDrawHook.thiscall<int>(box, pass, a2, a3, a4);
        if (SubtitleScale.load(std::memory_order_relaxed) != SubtitleLayout.load(std::memory_order_relaxed))
            RelayoutSubtitles(box);
        VirtualScreen screen(SubtitleLayout.load(std::memory_order_relaxed));
        return SubtitleDrawHook.thiscall<int>(box, pass, a2, a3, a4);
    }

    // HUD text boxes (hints) wrap to a fixed width made for 640x480 and sit at a distance from the left bound. On a
    // virtual screen narrower than that (large HUD scale) a box that would run past the right bound moves left. Texts
    // sharing an anchor (a hint's title and its text) move together, by the widest of them in this or the last frame.
    inline int (__fastcall* MeasureText)(void* font, void*, const char* text) = nullptr;
    inline ptrdiff_t TextFont = 0x100;

    struct AnchorWidth
    {
        uint32_t key = 0, frame = 0;
        int last = 0, current = 0;
    };
    inline std::array<AnchorWidth, 8> AnchorWidths{};

    inline int SharedWidth(uint32_t key, int width)
    {
        AnchorWidth* slot = nullptr;
        for (auto& entry : AnchorWidths)
        {
            if (entry.key == key && entry.frame)
            {
                slot = &entry;
                break;
            }
            if (!slot || entry.frame < slot->frame)
                slot = &entry;
        }
        if (slot->key != key || slot->frame + 1 < HudFrame)
            *slot = { key, HudFrame, 0, 0 };
        else if (slot->frame != HudFrame)
            *slot = { key, HudFrame, slot->current, 0 };
        slot->current = std::max(slot->current, width);
        return std::max(slot->last, slot->current);
    }

    inline void FitText(SafetyHookContext& context)
    {
        // Text element in ebx (font and flags after its text), anchor flags in esi, its x in ax; on the stack the text
        // scale and the anchor's own x
        if (!Drawing || !Scaling() || !(context.esi & 1))
            return;
        const auto element = context.ebx;
        if (*reinterpret_cast<uint32_t*>(element + TextFont + 4) & 0x100) // centered
            return;
        const auto font = *reinterpret_cast<void**>(element + TextFont);
        int widest = 0;
        for (auto line = reinterpret_cast<const char*>(element); line; line = std::strchr(line, '\n'))
        {
            if (*line == '\n')
                ++line;
            widest = std::max(widest, MeasureText(font, nullptr, line));
        }
        const uint32_t anchor = (*reinterpret_cast<uint32_t*>(context.esp + 0x38) << 8) | (context.esi & 0xFF);
        const int width = SharedWidth(anchor, static_cast<int>(std::lround(widest * *reinterpret_cast<float*>(context.esp + 0x30))));
        const int x = static_cast<int16_t>(context.eax);
        const int right = HudRightBound.get();
        if (x + width > right)
        {
            const int fitted = std::max(HudLeftBound.get(), right - width);
            context.eax = (context.eax & 0xFFFF0000) | static_cast<uint16_t>(fitted);
        }
    }

    template<class Action> void Add(void* address, Action action)
    {
        Fixed.push_back(safetyhook::create_mid(address, [](SafetyHookContext& context)
        {
            if (Offset)
                Action{}(context);
        }));
    }

    template<class Action> void Add(const char* pattern, ptrdiff_t offset, Action action)
    {
        Add(hook::pattern(pattern).get_first(offset), action);
    }

    inline void Init()
    {
        auto changed = []()
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            const auto value = ParseWidescreenHudOffset(FixSettings().Get("MAIN.HudAspectRatioConstraint"));
            RequestedAspect.store(value && std::isfinite(*value) && *value > 0
                ? std::clamp(*value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f, std::memory_order_relaxed);
        };
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", changed);
        changed();
        auto scales = []()
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            HudScale.store(Setting("MAIN.HudScale"), std::memory_order_relaxed);
            SubtitleScale.store(Setting("MAIN.SubtitleScale"), std::memory_order_relaxed);
        };
        FixSettings().OnChange("MAIN.HudScale", scales);
        FixSettings().OnChange("MAIN.SubtitleScale", scales);
        scales();

        // HUD bounds, set by the layout code at start: left, top, right, bottom
        auto bounds = hook::pattern("8B 44 24 04 8B 4C 24 08 8B 54 24 0C A3 ? ? ? ? 8B 44 24 10 89 0D ? ? ? ? 89 15 ? ? ? ? A3 ? ? ? ? C3");
        HudLeftBound.SetAddress(*bounds.get_first<int*>(13));
        HudRightBound.SetAddress(*bounds.get_first<int*>(29));
        HudBottomBound.SetAddress(*bounds.get_first<int*>(34));

        // Screen width and height getters
        auto width = hook::pattern("8B 44 24 04 A3 ? ? ? ? C3 CC CC CC CC CC CC A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC A1 ? ? ? ? C3");
        const auto getter = reinterpret_cast<uintptr_t>(width.get_first(16));
        // Calls that halve the width right away (cdq follows the call) center their element
        auto centered = hook::pattern("E8 ? ? ? ? 99");
        size_t centeredCalls = 0;
        centered.for_each_result([getter, &centeredCalls](hook::pattern_match match)
        {
            auto call = match.get<void>(0);
            if (injector::GetBranchDestination(call).as_int() == getter)
            {
                injector::MakeCALL(call, CenterWidth, true);
                ++centeredCalls;
            }
        });
        if (!centeredCalls)
        {
            centered.get_first();
            std::abort();
        }
        WidthHook = safetyhook::create_inline(getter, Width);
        HeightHook = safetyhook::create_inline(width.get_first(32), Height);

        // Subtitles: line layout and drawing
        auto subtitleLine = hook::pattern("83 EC 0C 56 8B F1 8B 46 14 57 33 FF 3B C7 0F 84 ? ? ? ? 8B 4E 1C 03 C8 8B C1 89 4E 14 89 7E 1C 80 38 0A");
        auto subtitleDraw = hook::pattern("51 83 7C 24 08 04 56 8B F1 0F 85 ? ? ? ? A0");
        auto measure = hook::pattern("83 EC 0C 8B 41 28 53 8A 58 15 8A 40 14 55");
        if (subtitleDraw.empty()) // TrueCrimeMB.exe
        {
            subtitleLine = hook::pattern("83 EC 0C 53 56 8B F1 8B 46 14 33 DB 3B C3 0F 84 ? ? ? ? 8B 4E 20 03 C8 8B C1 89 4E 14 89 5E 20 89 5E 1C 80 38 0A");
            subtitleDraw = hook::pattern("8B 44 24 04 83 EC 08 83 F8 04 56 8B F1 0F 85 ? ? ? ? A0");
            measure = hook::pattern("83 EC 14 8B 41 10 8A 50 14 84 D2 55 0F B6 68 15");
            SubtitleLength = 32;
            SubtitleTimer = 44;
            TextFont = 0x200;
        }
        SubtitleLineHook = safetyhook::create_inline(subtitleLine.get_first(), SubtitleLine);
        SubtitleDrawHook = safetyhook::create_inline(subtitleDraw.get_first(), SubtitleDraw);

        // HUD text boxes: their x, right after the anchor is resolved
        MeasureText = reinterpret_cast<decltype(MeasureText)>(measure.get_first());
        Fixed.push_back(safetyhook::create_mid(hook::pattern("8B 54 24 30 56 52 E8 ? ? ? ? 66 8B F8 8B 44 24 3C 56 50 E8").get_first(11), FitText));

        // Left-side elements with fixed screen positions: radar, compass, portrait, health ring, gauge and meters
        auto draw = hook::pattern("55 8B EC 83 E4 F0 81 EC 64 04 00 00 F3 0F 10 05");
        if (!draw.empty()) // TrueCrime_.exe
        {
            // Radar frame and compass rectangles
            Add("66 C7 44 24 60 0F 00 66 89 4C 24 62", 12, [](SafetyHookContext& context) { Shift(context, 0x20); Shift(context, 0x60); });
            // North marker: its rectangle and the center it rotates around
            auto north = hook::pattern("66 C7 44 24 18 16 00 E8 ? ? ? ? 8B C8 0F BF C3 99 2B C2 D1 F8 83 C0 16");
            Add(north.get_first(7), [](SafetyHookContext& context) { Shift(context, 0x18); });
            Add(north.get_first(25), [](SafetyHookContext& context) { context.eax += Offset; });
            // Left meter rectangle, second argument of the call
            Add("66 C7 44 24 68 20 00 66 C7 44 24 6A 10 00 E8", 14, [](SafetyHookContext& context) { ShiftArgument(context, 4); });
            // Distance gauge and its marker: left and right edges are the first and third float arguments
            constexpr auto quad = [](SafetyHookContext& context)
            {
                *reinterpret_cast<float*>(context.esp) += static_cast<float>(Offset);
                *reinterpret_cast<float*>(context.esp + 8) += static_cast<float>(Offset);
            };
            Add("68 00 00 A8 42 68 00 00 30 42 8B CF E8", 12, quad);
            Add("68 00 00 1C 42 E8", 5, quad);
            // Health ring around the portrait, rectangle in edi
            Add("55 8B EC 83 E4 F0 83 EC 4C 0F BF 0F", 0, [](SafetyHookContext& context) { *reinterpret_cast<int16_t*>(context.edi) += static_cast<int16_t>(Offset); });
            // Player arrow: radar center
            Add("99 0F 57 D2 2B C2 D1 F8 83 C0 0F F3 0F 2A C0", 11, [](SafetyHookContext& context) { context.eax += Offset; });
            // Blips outside the radar range sit on the radar's edge
            Add("83 C1 0F 83 F8 02 F3 0F 5E CA", 3, [](SafetyHookContext& context) { context.ecx += Offset; });
        }
        else // TrueCrimeMB.exe
        {
            draw = hook::pattern("55 8B EC 83 E4 F0 81 EC 84 04 00 00 A1 ? ? ? ? F3 0F 10 05");
            Add("66 C7 44 24 40 0F 00 66 C7 84 24 ? ? ? ? 14 00", 7, [](SafetyHookContext& context) { Shift(context, 0x20); Shift(context, 0x40); });
            auto north = hook::pattern("66 C7 44 24 08 16 00 E8 ? ? ? ? 8B C8 0F BF 44 24 ? 99 2B C2 D1 F8 83 C0 16");
            Add(north.get_first(7), [](SafetyHookContext& context) { Shift(context, 0x08); });
            Add(north.get_first(27), [](SafetyHookContext& context) { context.eax += Offset; });
            Add("66 C7 84 24 ? ? ? ? 20 00 66 C7 84 24 ? ? ? ? 10 00 E8", 20, [](SafetyHookContext& context) { ShiftArgument(context, 4); });
            // Distance gauge and its marker: rectangle pointer, first argument
            Add("68 ? ? ? ? 8D 94 24 ? ? ? ? 89 4F 04 52 8B CF E8", 18, [](SafetyHookContext& context) { ShiftArgument(context, 0); });
            Add("8D 84 24 ? ? ? ? 50 8B CF 66 89 94 24 ? ? ? ? E8", 18, [](SafetyHookContext& context) { ShiftArgument(context, 0); });
            Add("55 8B EC 83 E4 F0 83 EC 5C 0F BF 0F", 0, [](SafetyHookContext& context) { *reinterpret_cast<int16_t*>(context.edi) += static_cast<int16_t>(Offset); });
            Add("99 2B C2 D1 F8 83 C0 0F 89 44 24", 8, [](SafetyHookContext& context) { context.eax += Offset; });
            Add("83 C1 0F 89 4C 24 38 DB 44 24 38", 3, [](SafetyHookContext& context) { context.ecx += Offset; });
        }
        // Heading strip below the radar
        Add("66 C7 44 24 38 20 00 E8", 7, [](SafetyHookContext& context) { Shift(context, 0x38); });
        // Portrait rectangle, first argument of the call, and its hit flash
        Add("66 C7 44 24 18 20 00 66 C7 44 24 1A 10 00 E8", 14, [](SafetyHookContext& context) { ShiftArgument(context, 0); });
        Add("66 C7 44 24 14 20 00 66 C7 44 24 16 18 00 E8", 14, [](SafetyHookContext& context) { Shift(context, 0x14); });
        // Blip center inside the radar
        Add("89 44 24 ? 83 C0 0F", 7, [](SafetyHookContext& context) { context.eax += Offset; });
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
    }
}
