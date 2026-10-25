#pragma once
#include "../Shared/Console/Viewport.hpp"

namespace THPS3Hud
{
    inline SafetyHookInline PanelHook, ImageHook, TextHook;
    inline uintptr_t Panel = 0;
    inline int Anchor = 0;
    inline float Width = 640.0f, Height = 480.0f;
    inline const std::string* Constraint = nullptr;

    inline float Offset()
    {
        const float aspect = Constraint ? console::parse_hud_aspect(Constraint->c_str()) : 0.0f;
        const float boxWidth = aspect > 0.0f ? std::min(Width, Height * aspect) : Width;
        return Anchor * (boxWidth - Height * (4.0f / 3.0f)) * 0.5f;
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

    inline void Install(float width, float height, const std::string* constraint, HMODULE module = GetModuleHandleW(nullptr))
    {
        Width = width;
        Height = height;
        Constraint = constraint;
        auto panel = hook::module_pattern(module, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 4C 53 55 56 8B F1 57 8A 86 50 1F 00 00");
        auto image = hook::module_pattern(module, "56 8B F1 E8 ? ? ? ? 84 C0 75 49 8B CE E8 ? ? ? ? 85 C0 74 3E");
        auto text = hook::module_pattern(module, "83 EC 30 53 55 56 8B F1 57 80 3E 00 74 07 8B 46 3C C6 40 01 01");
        if (panel.size() != 1 || image.size() != 1 || text.size() != 1)
        {
            OutputDebugStringA("THPS3: HUD group patterns unavailable.\n");
            return;
        }
        PanelHook = safetyhook::create_inline(panel.get_first(), DrawPanel);
        ImageHook = safetyhook::create_inline(image.get_first(), DrawImage);
        TextHook = safetyhook::create_inline(text.get_first(), DrawText);
        if (!PanelHook || !ImageHook || !TextHook)
        {
            PanelHook.reset();
            ImageHook.reset();
            TextHook.reset();
            OutputDebugStringA("THPS3: HUD group hooks unavailable.\n");
        }
    }
}
