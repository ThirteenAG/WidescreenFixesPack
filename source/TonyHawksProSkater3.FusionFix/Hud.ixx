module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/Console/Viewport.hpp"

export module Hud;

import ComVars;

export namespace THPS3Hud
{
    inline SafetyHookInline PanelHook, ImageHook, TextHook;
    inline uintptr_t Panel = 0;
    inline int Anchor = 0;
    inline float Width = 640.0f, Height = 480.0f, FrameWidth = 640.0f;
    inline const std::string* Constraint = nullptr;

    inline float Offset()
    {
        const float aspect = Constraint ? console::parse_hud_aspect(Constraint->c_str()) : 0.0f;
        const float boxWidth = aspect > 0.0f ? std::min(Width, Height * aspect) : Width;
        return Anchor * (boxWidth - FrameWidth) * 0.5f;
    }

    inline int Classify(uintptr_t object, bool text)
    {
        if (!Panel || *reinterpret_cast<uint16_t*>(Panel + 8012) != 0)
            return 0;
        if (text)
            return object == *reinterpret_cast<uintptr_t*>(Panel + 8048) ? -1 : 0;
        for (auto offset : { 8524, 8528, 8532 })
        {
            if (object == *reinterpret_cast<uintptr_t*>(Panel + offset))
                return -1;
        }
        for (auto offset : { 8196, 8200 })
        {
            if (object == *reinterpret_cast<uintptr_t*>(Panel + offset))
                return 1;
        }
        return 0;
    }

    inline uint8_t __fastcall DrawPanel(uintptr_t panel, void*)
    {
        const auto previous = std::exchange(Panel, panel);
        const auto result = PanelHook.thiscall<uint8_t>(panel);
        Panel = previous;
        return result;
    }

    inline int __fastcall DrawImage(uintptr_t image, void*)
    {
        const auto previous = std::exchange(Anchor, Classify(image, false));
        const auto result = ImageHook.thiscall<int>(image);
        Anchor = previous;
        return result;
    }

    inline void __fastcall DrawText(uintptr_t text, void*)
    {
        const auto previous = std::exchange(Anchor, Classify(text, true));
        TextHook.thiscall<void>(text);
        Anchor = previous;
    }

    inline void Install(float width, float height, float frameWidth, const std::string* constraint, HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        FrameWidth = frameWidth;
        Constraint = constraint;
        auto panel = hook::module_pattern(module, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 4C 53 55 56 8B F1 57 8A 86 50 1F 00 00");
        auto image = hook::module_pattern(module, "56 8B F1 E8 ? ? ? ? 84 C0 75 49 8B CE E8 ? ? ? ? 85 C0 74 3E");
        auto text = hook::module_pattern(module, "83 EC 30 53 55 56 8B F1 57 80 3E 00 74 07 8B 46 3C C6 40 01 01");
        PanelHook = safetyhook::create_inline(panel.get_first(), DrawPanel);
        ImageHook = safetyhook::create_inline(image.get_first(), DrawImage);
        TextHook = safetyhook::create_inline(text.get_first(), DrawText);
    }
}

export void InstallHud()
{
    auto& settings = FixSettings();
    hook::pattern pattern("");
    //HUD
    THPS3Hud::Install(Screen.fWidth, Screen.fHeight, 640.0f * Screen.fUnit, &settings.String("MAIN.HudAspectRatioConstraint"));
    pattern = hook::pattern("D8 0D ? ? ? ? 8B CF 89 5C 24 50 D8"); //0x58DDC8
    injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fHUDScaleX, true);
    if (Screen.fHudOffsetY > 0.0f)
    {
        pattern = hook::pattern("DB 81 B0 00 00 00 D8 0D ? ? ? ? D8 49 58 D9 58 04"); //0x58DDC4
        injector::WriteMemory<float>(*pattern.get_first<float*>(8), Screen.fHUDScaleY, true);
    }

    pattern = hook::pattern("0B F2 89 70 64 A1 8C 67 90"); //0x4F65B1
    struct HUDHook //sub_4F62A0
    {
        void operator()(injector::reg_pack& regs)
        {
            if (*(float*)(regs.eax + 0x00) == 0.0f && *(float*)(regs.eax + 0x1C) == 0.0f
                && (int32_t)(*(float*)(regs.eax + 0x38)) == Screen.Width43
                && (int32_t)(*(float*)(regs.eax + 0x54)) == Screen.Width43)
            {
                // full-screen layers (blood overlay, fades) cover the whole screen
                *(float*)(regs.eax + 0x38) += Screen.fHudOffsetReal * 2.0f;
                *(float*)(regs.eax + 0x54) += Screen.fHudOffsetReal * 2.0f;
                for (auto vertexOffset : { 0x04, 0x20, 0x3C, 0x58 })
                    *reinterpret_cast<float*>(regs.eax + vertexOffset) *= Screen.fHeight / (480.0f * Screen.fUnit);
            }
            else if (std::ranges::all_of(std::array{ 0x00, 0x1C, 0x38, 0x54 }, [&](int vertex) { return *(float*)(regs.eax + vertex) >= Screen.Width43; })
                || std::ranges::all_of(std::array{ 0x00, 0x1C, 0x38, 0x54 }, [&](int vertex) { return *(float*)(regs.eax + vertex) <= 0.0f; }))
            {
                // Menu panels parked beside the 640x480 frame are hidden there; keep them out of the side areas
                for (auto vertexOffset : { 0x00, 0x1C, 0x38, 0x54 })
                    *reinterpret_cast<float*>(regs.eax + vertexOffset) = -1.0f;
            }
            else
            {
                const float offset = Screen.fHudOffsetReal + THPS3Hud::Offset();
                for (auto vertexOffset : { 0x00, 0x1C, 0x38, 0x54 })
                {
                    *reinterpret_cast<float*>(regs.eax + vertexOffset) += offset;
                    *reinterpret_cast<float*>(regs.eax + vertexOffset + 4) += Screen.fHudOffsetY;
                }
            }

            regs.esi |= regs.edx;
            *(uint32_t*)(regs.eax + 0x64) = regs.esi;
        }
    }; injector::MakeInline<HUDHook>(pattern.get_first(0));

    pattern = hook::pattern("8B 81 A4 00 00 00 89 46"); //0x4F66F0
    struct HUDHook2 //sub_4F65E0
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<float*>(regs.esi - 0x04) += Screen.fHudOffsetReal + THPS3Hud::Offset();
            *reinterpret_cast<float*>(regs.esi) += Screen.fHudOffsetY;
            regs.eax = *(uint32_t*)(regs.ecx + 0xA4);
        }
    }; injector::MakeInline<HUDHook2>(pattern.get_first(0), pattern.get_first(6));

    // Level-select cassettes take 2D positions in the 640x480 frame
    pattern = hook::pattern("8B 44 24 10 56 85 C0 8B F1 74 ? 8B 4C 24 08 8B 54 24 0C 89 8E F8 03 00 00"); //0x4861B0
    static auto cassetteHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        *reinterpret_cast<float*>(regs.esp + 4) += Screen.fHudOffsetReal;
        *reinterpret_cast<float*>(regs.esp + 8) += Screen.fHudOffsetY;
    });
}
