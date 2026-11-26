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
        auto packet = hook::pattern("8B 44 24 ? 0F AF C3 89 04 24 8B 44 24 ? 0F AF C5 DB 04 24 D9 1D");
        static const auto x = *packet.get_first<uint8_t>(3);
        static const auto y = *packet.get_first<uint8_t>(13);
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
