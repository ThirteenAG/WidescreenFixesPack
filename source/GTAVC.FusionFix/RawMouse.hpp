#pragma once
#include "Shared/RWGTA/RawMouse.hpp"

namespace GtaRawMouse
{
    inline SafetyHookMid MouseHook;

    inline void Install(HWND (*gameWindow)(), bool (*playing)())
    {
        Playing = playing;
        // Replace the native relative packet before its invert-axis handling.
        // Buttons, wheel, menu cursor and the game's sensitivity remain native.
        auto packet = hook::pattern("8B 44 24 0C C6 05 ? ? ? ? 00 0F AF C2 C6 05 ? ? ? ? 00 C6 05");
        static constexpr uint8_t x = 0x0C;
        static constexpr uint8_t y = 0x10;
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
