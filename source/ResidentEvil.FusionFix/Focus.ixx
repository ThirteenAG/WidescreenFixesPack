module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <functional>

export module Focus;

export namespace REFocus
{
    inline std::atomic<bool> pause{ true };
    inline SafetyHookMid updateGate;
    inline bool originalCompiler = false;
    inline std::function<bool()> backgroundInputReady;

    inline uint32_t UpdateFlags(uint32_t flags, bool pauseOnFocusLoss, bool active)
    {
        return (flags & ~0x40u) | (pauseOnFocusLoss && !active ? 0x40u : 0);
    }

    inline void Install()
    {
        pause.store(FixSettings().Bool("MAIN.PauseOnFocusLoss"), std::memory_order_relaxed);
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
        {
            pause.store(FixSettings().Bool("MAIN.PauseOnFocusLoss"), std::memory_order_relaxed);
        });
        auto current = hook::pattern("80 78 20 00 75 0F 80 B8 51 25 00 00 00 75 06 6A 01 FF D6");
        auto original = hook::pattern("80 79 20 00 75 0F 80 B9 51 25 00 00 00 75 06 6A 01 FF D6");
        originalCompiler = original.size() == 1;
        auto site = originalCompiler ? original.get_first<uint8_t>(4) : current.get_first<uint8_t>(4);
        updateGate = safetyhook::create_mid(site, [](SafetyHookContext& context)
        {
            // The native always-update flag also runs Options in the background. Apply our pause rule before it.
            auto system = reinterpret_cast<const uint8_t*>(originalCompiler ? context.ecx : context.eax);
            const bool shouldPause = pause.load(std::memory_order_relaxed) || (backgroundInputReady && !backgroundInputReady());
            context.eflags = UpdateFlags(context.eflags, shouldPause, system[9553] != 0);
        });
        FixSettings().Availability("MAIN.PauseOnFocusLoss", [](std::string_view value)
        {
            return updateGate && (value == "1" || !backgroundInputReady || backgroundInputReady());
        });
    }
}
