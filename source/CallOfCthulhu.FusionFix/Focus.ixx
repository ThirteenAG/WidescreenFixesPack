module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Focus;


export namespace CoCFocus
{
    inline void Select(SafetyHookContext& context, bool pause)
    {
        // Skip only the deactivation pause body. Native active-state and
        // keyboard-hook bookkeeping still receives the original wParam.
        if (!pause && !context.esi)
            context.eflags &= ~uintptr_t(0x40);
    }

    inline void Init()
    {
        auto branch = hook::pattern("85 F6 75 69 E8 ? ? ? ? 84 C0 75 60 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 74 50");
        static auto FocusHook = safetyhook::create_mid(branch.get_first(2), [](SafetyHookContext& context)
        {
            Select(context, FixSettings().Bool("MAIN.PauseOnFocusLoss"));
        });
        if (!FocusHook)
            OutputDebugStringA("Call of Cthulhu: focus pause hook installation failed.\n");
    }
}
