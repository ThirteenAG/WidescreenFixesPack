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
    // Subtitle boxes keep the scale they were laid out with
    inline std::atomic<float> SubtitleLayout{ 1.0f };
    inline SafetyHookInline DrawHook, WidthHook, HeightHook, SubtitleHook, SubtitleDrawHook;
    inline thread_local int Offset = 0;

    // Active virtual screen and its factors to the real screen; zero size while nothing is scaled
    inline thread_local int VirtualWidth = 0, VirtualHeight = 0;
    inline thread_local float FactorX = 1.0f, FactorY = 1.0f;

    struct VirtualScreen
    {
        int width, height;
        float x, y;

        explicit VirtualScreen(float scale) : width(VirtualWidth), height(VirtualHeight), x(FactorX), y(FactorY)
        {
            VirtualWidth = 0;
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

        ~VirtualScreen()
        {
            VirtualWidth = width;
            VirtualHeight = height;
            FactorX = x;
            FactorY = y;
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
            vertex[0] = (vertex[0] + 0.5f) * FactorX - 0.5f;
            vertex[1] = (vertex[1] + 0.5f) * FactorY - 0.5f;
        }
    }

    inline float Setting(const char* id)
    {
        const float value = FixSettings().Float(id);
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
        const int result = DrawHook.thiscall<int>(hud, pass, a2, a3, a4);
        Offset = 0;
        HudLeftBound = left;
        HudRightBound = right;
        HudBottomBound = bottom;
        return result;
    }

    // Subtitle boxes: their position and line width come from the screen size when the text of a voice line loads
    inline int __fastcall Subtitle(void* box, void*, const char* path, int font)
    {
        const float scale = SubtitleScale.load(std::memory_order_relaxed);
        SubtitleLayout.store(scale, std::memory_order_relaxed);
        VirtualScreen screen(scale);
        return SubtitleHook.thiscall<int>(box, path, font);
    }

    inline int __fastcall SubtitleDraw(void* box, void*, int pass, int a2, int a3, int a4)
    {
        if (pass != 4)
            return SubtitleDrawHook.thiscall<int>(box, pass, a2, a3, a4);
        VirtualScreen screen(SubtitleLayout.load(std::memory_order_relaxed));
        return SubtitleDrawHook.thiscall<int>(box, pass, a2, a3, a4);
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
            const auto value = ParseWidescreenHudOffset(FixSettings().Get("MAIN.HudAspectRatioConstraint"));
            RequestedAspect.store(value && std::isfinite(*value) && *value > 0
                ? std::clamp(*value, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f, std::memory_order_relaxed);
        };
        changed();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", changed);
        auto scales = []()
        {
            HudScale.store(Setting("MAIN.HudScale"), std::memory_order_relaxed);
            SubtitleScale.store(Setting("MAIN.SubtitleScale"), std::memory_order_relaxed);
        };
        scales();
        FixSettings().OnChange("MAIN.HudScale", scales);
        FixSettings().OnChange("MAIN.SubtitleScale", scales);

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

        // Subtitles: the box set up for a voice line and its drawing
        SubtitleHook = safetyhook::create_inline(hook::pattern("83 EC 0C 56 57 8D 44 24 08 8B F9 8B 4C 24 18 50 51 E8 ? ? ? ? 8B F0 83 C4 08 85 F6 0F 84 ? ? ? ? E8").get_first(), Subtitle);
        auto subtitleDraw = hook::pattern("51 83 7C 24 08 04 56 8B F1 0F 85 ? ? ? ? A0");
        if (subtitleDraw.empty()) // TrueCrimeMB.exe
            subtitleDraw = hook::pattern("8B 44 24 04 83 EC 08 83 F8 04 56 8B F1 0F 85 ? ? ? ? A0");
        SubtitleDrawHook = safetyhook::create_inline(subtitleDraw.get_first(), SubtitleDraw);

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
