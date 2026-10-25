#pragma once

namespace DeerIntro
{
    inline SafetyHookMid decisionHook;

    inline bool Init(bool skip, HMODULE game = GetModuleHandleW(nullptr))
    {
        if (!skip)
            return true;
        auto decision = hook::pattern(game, "A1 ? ? ? ? 83 C4 0C 85 C0 75 37 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 50 E8");
        if (decision.size() != 1)
            return false;

        // Skip the startup videos without changing the game's saved preference.
        decisionHook = safetyhook::create_mid(decision.get_first(5), [](SafetyHookContext& context)
        {
            context.eax = 1;
        });
        return bool(decisionHook);
    }
}
