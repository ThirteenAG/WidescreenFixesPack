#pragma once
#include "Shared/RWGTA/RawMouse.hpp"

namespace GtaRawMouse
{
    inline SafetyHookMid MouseHook;

    inline void Install(HWND (*gameWindow)(), void (*mouseActivity)())
    {
        MouseActivity = mouseActivity;
        // Replace the native relative packet before its invert-axis handling.
        // Buttons, wheel, menu cursor and the game's sensitivity remain native.
        auto packet = hook::pattern("8B 54 24 08 0F AF D0 8B 44 24 0C 0F AF C1 8A 4C 24 14 89 54 24 04 8A 54 24 15");
        static constexpr uint8_t x = 0x08;
        static constexpr uint8_t y = 0x0C;
        GetGameWindow = gameWindow;
        {
            std::lock_guard registration(RegistrationMutex);
            RefreshGameWindow();
        }
        MouseHook = safetyhook::create_mid(packet.get_first(), [](SafetyHookContext& context)
        {
            Sample(*reinterpret_cast<int32_t*>(context.esp + x), *reinterpret_cast<int32_t*>(context.esp + y));
        });
    }
}
