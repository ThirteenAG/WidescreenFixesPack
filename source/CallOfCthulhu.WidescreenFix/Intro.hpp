#pragma once

namespace CoCIntro
{
    inline void Init()
    {
        if (!FixSettings().Bool("MAIN.SkipIntro"))
            return;
        auto branch = hook::pattern("38 98 71 06 00 00 53 53 75 76 68 ? ? ? ? E8 ? ? ? ? 8B C8 E8");
        if (branch.size() != 1)
        {
            OutputDebugStringA("Call of Cthulhu: startup video signature unavailable; native playlist retained.\n");
            return;
        }
        static auto IntroHook = safetyhook::create_mid(branch.get_first(6), [](SafetyHookContext& context)
        {
            // Clear ZF so the following jnz takes the skip branch.
            constexpr uintptr_t ZeroFlag = 0x40;
            context.eflags &= ~ZeroFlag;
        });
        if (!IntroHook)
            OutputDebugStringA("Call of Cthulhu: startup video hook installation failed.\n");
    }
}
