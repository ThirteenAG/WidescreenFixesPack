module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>


export module Hud;

import ComVars;
import Window;

export namespace TrueCrimeHud
{
    // The game lays out its HUD between two screen bounds (left/right side anchors) and places a few elements at
    // fixed distances from the screen width. With a HUD box both are moved inward for the duration of the HUD draw.
    inline std::atomic<float> RequestedAspect{ 0.0f };
    inline SafetyHookInline DrawHook, WidthHook;
    inline thread_local int Offset = 0;

    inline int BoxOffset()
    {
        const float requested = RequestedAspect.load(std::memory_order_relaxed);
        const auto width = static_cast<float>(TrueCrimeWindow::Width);
        const auto height = static_cast<float>(TrueCrimeWindow::Height);
        if (requested <= 0.0f || width <= 0.0f || height <= 0.0f)
            return 0;
        return std::max(0, static_cast<int>(std::lround((width - std::min(width, height * requested)) * 0.5f)));
    }

    // Screen width: elements measured from the right edge get the right edge of the box
    inline int __cdecl Width()
    {
        return WidthHook.ccall<int>() - Offset;
    }

    // Centered elements halve the full width
    inline int __cdecl CenterWidth()
    {
        return WidthHook.ccall<int>();
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
        if (pass == 4 && BeforeDraw)
            BeforeDraw();
        const int offset = BoxOffset();
        if (!offset)
            return DrawHook.thiscall<int>(hud, pass, a2, a3, a4);
        const int left = HudLeftBound.get();
        const int right = HudRightBound.get();
        HudLeftBound = left + offset;
        HudRightBound = right - offset;
        Offset = offset;
        const int result = DrawHook.thiscall<int>(hud, pass, a2, a3, a4);
        Offset = 0;
        HudLeftBound = left;
        HudRightBound = right;
        return result;
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

        // HUD bounds, set by the layout code at start
        auto bounds = hook::pattern("8B 44 24 04 8B 4C 24 08 8B 54 24 0C A3 ? ? ? ? 8B 44 24 10 89 0D ? ? ? ? 89 15 ? ? ? ? A3 ? ? ? ? C3");
        HudLeftBound.SetAddress(*bounds.get_first<int*>(13));
        HudRightBound.SetAddress(*bounds.get_first<int*>(29));

        auto width = hook::pattern("8B 44 24 04 A3 ? ? ? ? C3 CC CC CC CC CC CC A1 ? ? ? ? C3");
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
