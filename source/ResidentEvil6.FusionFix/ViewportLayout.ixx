module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>

export module ViewportLayout;

import ComVars;
import Cutscenes;

namespace
{
    std::atomic<bool> pending{ false };
    SafetyHookInline update;

    void __fastcall Update(void* object, void*)
    {
        const bool changed = pending.exchange(false);
        if ((changed || IsCutsceneBorderTransitionActive()) && reinterpret_cast<uintptr_t>(object) == pViewports.get())
            injector::WriteMemory<uint8_t>(static_cast<uint8_t*>(object) + 3340, 1, true);
        update.unsafe_thiscall(object);
    }
}

class ViewportLayout
{
public:
    ViewportLayout()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("83 EC 34 56 8B F1 80 7E 1C 00 75 ? 80 3D ? ? ? ? 00 74 ? 8D 46 04 50 FF 15 ? ? ? ? 8B 86 04 0D 00 00");
            update = safetyhook::create_inline(pattern.get_first(), Update);
            if (!update) return;
            for (const auto id : { "MAIN.CutsceneBorders", "MAIN.SplitScreenVertical", "MAIN.SplitScreenSwapTopBottom" })
                FixSettings().OnChange(id, []() { pending = true; });
        };
    }
} ViewportLayout;
