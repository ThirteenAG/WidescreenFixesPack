module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Focus;

import ComVars;
import Window;

export namespace CoCFocus
{
    inline std::atomic<bool> pause{ true };

    inline void Select(SafetyHookContext& context)
    {
        // Skip only the deactivation pause body. Native active-state and
        // keyboard-hook bookkeeping still receives the original wParam.
        if (!pause.load(std::memory_order_relaxed) && !context.esi)
            context.eflags &= ~uintptr_t(0x40);
    }

    // The game frame only runs while the app is marked active. A windowed device keeps working in the background,
    // so let the frame run there too; a full screen device is lost while away and stays gated.
    inline void Run(SafetyHookContext& context)
    {
        if (pause.load(std::memory_order_relaxed) || (context.eax & 0xFF))
            return;
        if (CoCWindow::hasPresentation && PresentationParameters->Windowed)
            context.eax |= 1;
    }

    inline void Init()
    {
        pause = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
        {
            pause = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        });

        auto branch = hook::pattern("85 F6 75 69 E8 ? ? ? ? 84 C0 75 60 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 74 50");
        static auto FocusHook = safetyhook::create_mid(branch.get_first(2), Select);

        auto active = hook::pattern("83 EC 4C 57 8B F9 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 0F 84");
        static auto ActiveHook = safetyhook::create_mid(active.get_first(18), Run);
    }
}
