module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include "d3d9.h"

export module Hud;
import ComVars;
import Window;
import Camera;


export namespace LARushHud
{
    inline SafetyHookInline MinimapHook;
    inline SafetyHookMid SpeedometerBegin, SpeedometerEnd, MeterBegin, MeterEnd;
    inline SafetyHookMid MoneyBegin1, MoneyEnd1, MoneyBegin2, MoneyEnd2;
    inline SafetyHookMid RaceBegin1, RaceEnd1, RaceBegin2, RaceEnd2;
    inline SafetyHookMid LowriderBegin, LowriderEnd;
    inline SafetyHookInline BatchHook;
    inline SafetyHookInline GlyphHook;
    inline thread_local bool DrawingMinimap = false;
    inline thread_local bool DrawingSpeedometer = false;
    inline thread_local bool DrawingMeter = false;
    inline thread_local bool BuildingMoney = false;
    inline thread_local bool BuildingRace = false;
    inline thread_local bool BuildingLowrider = false;
    inline thread_local bool DrawingLowrider = false;
    inline thread_local std::array<uintptr_t, 128> MoneyGlyphs{};
    inline thread_local size_t MoneyGlyphCount = 0;
    inline thread_local std::array<uintptr_t, 256> RaceGlyphs{};
    inline thread_local size_t RaceGlyphCount = 0;
    inline thread_local std::array<uintptr_t, 256> LowriderGlyphs{};
    inline thread_local size_t LowriderGlyphCount = 0;
    inline std::atomic<float> Constraint{ 0.0f };

    inline int __cdecl SetGlyph(int object, int x, int y, char layer, int width, int height)
    {
        const int glyph = GlyphHook.ccall<int>(object, x, y, layer, width, height);
        if (BuildingMoney && glyph != 0 && glyph != -2 && MoneyGlyphCount < MoneyGlyphs.size())
            MoneyGlyphs[MoneyGlyphCount++] = static_cast<uintptr_t>(glyph);
        if (BuildingRace && LARushWindow::GameState == 56 && glyph != 0 && glyph != -2 && RaceGlyphCount < RaceGlyphs.size())
            RaceGlyphs[RaceGlyphCount++] = static_cast<uintptr_t>(glyph);
        if (BuildingLowrider && LARushWindow::GameState == 56 && glyph != 0 && glyph != -2 && LowriderGlyphCount < LowriderGlyphs.size())
            LowriderGlyphs[LowriderGlyphCount++] = static_cast<uintptr_t>(glyph);
        return glyph;
    }

    inline bool IsMoney(uintptr_t glyph)
    {
        return std::find(MoneyGlyphs.begin(), MoneyGlyphs.begin() + MoneyGlyphCount, glyph) != MoneyGlyphs.begin() + MoneyGlyphCount;
    }

    inline bool IsRace(uintptr_t glyph)
    {
        return std::find(RaceGlyphs.begin(), RaceGlyphs.begin() + RaceGlyphCount, glyph) != RaceGlyphs.begin() + RaceGlyphCount;
    }

    inline bool IsLowrider(uintptr_t glyph)
    {
        return std::find(LowriderGlyphs.begin(), LowriderGlyphs.begin() + LowriderGlyphCount, glyph) != LowriderGlyphs.begin() + LowriderGlyphCount;
    }

    inline float Scale()
    {
        return std::min(Screen.fHeight, Screen.fWidth * 0.75f) / 480.0f;
    }

    inline float BoxLeft()
    {
        const auto ratio = Constraint.load(std::memory_order_relaxed);
        const float box = ratio > 0.0f ? std::min(Screen.fWidth, Screen.fHeight * ratio) : Screen.fWidth;
        return (Screen.fWidth - box) * 0.5f;
    }

    inline bool LowriderViewport(D3DVIEWPORT9& viewport)
    {
        auto device = LARushWindow::Device.get();
        if (!LowriderGlyphCount || LARushWindow::GameState != 56 || !device || FAILED(device->GetViewport(&viewport)))
            return false;
        const int width = std::min(Screen.nWidth, static_cast<int>(std::lround(Screen.fHeight * (4.0f / 3.0f))));
        const int height = std::min(Screen.nHeight, static_cast<int>(std::lround(Screen.fWidth * (3.0f / 4.0f))));
        return width != Screen.nWidth &&
            viewport.X == static_cast<DWORD>((Screen.nWidth - width) / 2) && viewport.Y == static_cast<DWORD>((Screen.nHeight - height) / 2) &&
            viewport.Width == static_cast<DWORD>(width) && viewport.Height == static_cast<DWORD>(height);
    }

    inline void ConvertTextViewport(float* position, const D3DVIEWPORT9& viewport, bool lowrider)
    {
        const float ratio = viewport.Width / Screen.fWidth;
        const float center = (2.0f * viewport.X + viewport.Width) / Screen.fWidth - 1.0f;
        const float local = position[0] - (lowrider ? 1.0f - 1.0f / Screen.fHudScale : 0.0f);
        const float edge = std::max(0.0f, 1.0f - 1.0f / Screen.fHudScale - 2.0f * BoxLeft() / Screen.fWidth);
        position[0] = center + local * ratio + (lowrider ? (local < 0.0f ? -edge : edge) : 0.0f);
        position[1] *= ratio;
        const float verticalRatio = viewport.Height / Screen.fHeight;
        position[2] = 1.0f - (2.0f * viewport.Y + viewport.Height) / Screen.fHeight + position[2] * verticalRatio;
        position[3] *= verticalRatio;
    }

    inline int __fastcall FlushLowriderBatch(int* renderer, void*)
    {
        D3DVIEWPORT9 previous{};
        auto device = LARushWindow::Device.get();
        const bool scoped = LowriderViewport(previous);
        bool changed = false;
        if (scoped)
        {
            D3DVIEWPORT9 full{ 0, 0, static_cast<DWORD>(Screen.nWidth), static_cast<DWORD>(Screen.nHeight), previous.MinZ, previous.MaxZ };
            changed = SUCCEEDED(device->SetViewport(&full));
        }
        const auto result = BatchHook.thiscall<int>(renderer);
        if (changed)
            device->SetViewport(&previous);
        return result;
    }

    inline char __cdecl DrawMinimap()
    {
        const int index = MinimapCamera;
        if (index < 0 || index >= 26)
            return MinimapHook.ccall<char>();
        const auto camera = LARushCamera::Cameras.get_ptr() + index * 832;
        auto viewport = reinterpret_cast<int32_t*>(camera + 400);
        const int32_t original[4]{ viewport[0], viewport[1], viewport[2], viewport[3] };
        const float scale = Scale();
        const float designHeight = scale * 480.0f;
        const float bottom = (Screen.fHeight + designHeight) * 0.5f;
        viewport[0] = static_cast<int32_t>(std::lround(BoxLeft() + original[0] * scale));
        viewport[1] = static_cast<int32_t>(std::lround(bottom - (Screen.fHeight - original[1]) * scale));
        viewport[2] = std::max(1, static_cast<int32_t>(std::lround(original[2] * scale)));
        viewport[3] = std::max(1, static_cast<int32_t>(std::lround(original[3] * scale)));
        *reinterpret_cast<int32_t*>(camera + 808) = 1;
        const bool previous = DrawingMinimap;
        DrawingMinimap = true;
        const auto result = MinimapHook.ccall<char>();
        DrawingMinimap = previous;
        std::copy(std::begin(original), std::end(original), viewport);
        *reinterpret_cast<int32_t*>(camera + 808) = 1;
        return result;
    }

    inline void TransformMinimapText(float* position)
    {
        const float scale = Scale();
        position[0] = (position[0] + 1.0f) * scale - 1.0f;
        position[1] *= scale;
        position[2] = 1.0f - (1.0f - position[2]) * scale;
        position[3] *= scale;
    }

    inline void TransformSpeedometerText(float* position, bool bottomAnchored)
    {
        const float scale = Scale();
        position[0] = 1.0f - 2.0f * BoxLeft() / Screen.fWidth - (1.0f - position[0]) * scale;
        position[1] *= scale;
        position[2] = bottomAnchored ? 1.0f - (1.0f - position[2]) * scale : -1.0f + (position[2] + 1.0f) * scale;
        position[3] *= scale;
    }

    inline void InstallSpeedometer()
    {
        auto begin = hook::pattern("83 EC 6C 55 56 8B F0 E8 ? ? ? ? D9 46 08 D8 0D");
        auto end = hook::pattern("83 C4 40 5E 5D 83 C4 6C C3");
        SpeedometerBegin = safetyhook::create_mid(begin.get_first(), [](SafetyHookContext&) { DrawingSpeedometer = true; });
        SpeedometerEnd = safetyhook::create_mid(end.get_first(), [](SafetyHookContext&) { DrawingSpeedometer = false; });
        auto meter = hook::pattern("55 57 E8 ? ? ? ? 8B 46 38 8B 0C 85 ? ? ? ? 8B 15");
        auto meterEnd = hook::pattern("5F 5D 5B 5E 83 C4 3C C2 08 00");
        MeterBegin = safetyhook::create_mid(meter.get_first(), [](SafetyHookContext&) { DrawingSpeedometer = true; DrawingMeter = true; });
        MeterEnd = safetyhook::create_mid(meterEnd.get_first(), [](SafetyHookContext&) { DrawingMeter = false; DrawingSpeedometer = false; });
        auto glyph = hook::pattern("8B 4C 24 04 85 C9 74 ? 8B 41 64 8B 49 40 F3 0F 10 44 24 08");
        auto money1 = hook::pattern("E8 ? ? ? ? 8B 54 24 38 52 8D 44 24 24 57 50");
        auto money2 = hook::pattern("E8 ? ? ? ? 83 C4 08 5F 8B C3 5E 83 C4 20 C2 08 00");
        auto first = money1.get_first<uint8_t>();
        auto second = money2.get_first<uint8_t>();
        GlyphHook = safetyhook::create_inline(glyph.get_first(), SetGlyph);
        MoneyBegin1 = safetyhook::create_mid(first, [](SafetyHookContext&) { BuildingMoney = true; });
        MoneyEnd1 = safetyhook::create_mid(first + 5, [](SafetyHookContext&) { BuildingMoney = false; });
        MoneyBegin2 = safetyhook::create_mid(second, [](SafetyHookContext&) { BuildingMoney = true; });
        MoneyEnd2 = safetyhook::create_mid(second + 5, [](SafetyHookContext&) { BuildingMoney = false; });
        auto race1 = hook::pattern("51 8B 4C 24 34 03 D1 52 E8 ? ? ? ? 8B 46 04 85 C0 74 17");
        auto race2 = hook::pattern("8B 56 48 8B 4C 24 14 52 8B 54 24 2C 03 DD 53 03 CA 51 E8 ? ? ? ? 8B 46 08");
        auto firstRace = race1.get_first<uint8_t>(8);
        auto secondRace = race2.get_first<uint8_t>(18);
        RaceBegin1 = safetyhook::create_mid(firstRace, [](SafetyHookContext&) { BuildingRace = true; });
        RaceEnd1 = safetyhook::create_mid(firstRace + 5, [](SafetyHookContext&) { BuildingRace = false; });
        RaceBegin2 = safetyhook::create_mid(secondRace, [](SafetyHookContext&) { BuildingRace = true; });
        RaceEnd2 = safetyhook::create_mid(secondRace + 5, [](SafetyHookContext&) { BuildingRace = false; });
        auto lowrider = hook::pattern("8B 47 10 85 C0 74 0A F3 0F 10 49 48 E8 ? ? ? ? 8B 47 18 85 C0");
        auto lowriderCall = lowrider.get_first<uint8_t>(12);
        LowriderBegin = safetyhook::create_mid(lowriderCall, [](SafetyHookContext&) { BuildingLowrider = DrawingLowrider = true; });
        LowriderEnd = safetyhook::create_mid(lowriderCall + 5, [](SafetyHookContext&) { BuildingLowrider = DrawingLowrider = false; });
        auto batch = hook::pattern("A1 ? ? ? ? 81 EC 80 00 00 00 53 56 57 6A 03 6A 01 6A 00 8B F1 8B 08 50 FF 91");
        BatchHook = safetyhook::create_inline(batch.get_first(), FlushLowriderBatch);
        LARushWindow::IsLowrider = []() { return LowriderGlyphCount != 0; };
        LARushWindow::HudFrame = []() { MoneyGlyphCount = 0; RaceGlyphCount = 0; LowriderGlyphCount = 0; };
    }

    inline void Init()
    {
        const auto update = []()
        {
            const auto ratio = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            Constraint.store(ratio ? std::clamp(*ratio, 4.0f / 3.0f, 32.0f / 9.0f) : 0.0f,
                std::memory_order_relaxed);
        };
        update();
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", update);
        auto index = hook::pattern("A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 85 C0");
        MinimapCamera.SetAddress(*index.get_first<int32_t*>(1));
        auto draw = hook::pattern("A1 ? ? ? ? 81 EC 88 00 00 00 85 C0 0F 85 ? ? ? ? A1 ? ? ? ? 69 C0 68 03 00 00");
        MinimapHook = safetyhook::create_inline(draw.get_first(), DrawMinimap);
    }
}
