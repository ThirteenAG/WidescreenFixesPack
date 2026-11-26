module;

#include <stdafx.h>
#include "Shared/ResidentEvil/InputDrain.hpp"
#include <functional>

export module InputDrain;

export namespace RE0Input
{
    using namespace REInputDrain;

    inline SafetyHookInline keyboardHook, padHook, mouseHook;
    inline std::function<bool()> overlayBlocksInput;
    inline std::array<HeldControls, 8> keyboardHeld{};
    inline HeldControls mouseHeld{};
    inline std::array<HeldControls, 4> padHeld{};
    inline bool ready = false;

    inline InputAccess Access()
    {
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        if (process != GetCurrentProcessId()) return InputAccess::Unfocused;
        if (overlayBlocksInput && overlayBlocksInput()) return InputAccess::Blocked;
        return InputAccess::Allowed;
    }

    inline bool Blocked()
    {
        return Access() != InputAccess::Allowed;
    }

    inline int __fastcall Keyboard(void* keyboard, void*)
    {
        const auto result = keyboardHook.thiscall<int>(keyboard);
        FilterKeyboard(keyboard, Access(), keyboardHeld);
        return result;
    }

    inline int __fastcall Mouse(void* mouse, void*)
    {
        const auto result = mouseHook.thiscall<int>(mouse);
        FilterMouse(mouse, Access(), mouseHeld);
        return result;
    }

    inline int __fastcall Pad(void* manager, void*)
    {
        const auto result = padHook.thiscall<int>(manager);
        FilterPad(manager, Access(), padHeld);
        return result;
    }

    inline void Install()
    {
        auto keyboard = hook::pattern("83 EC 14 53 55 56 8D A9 20 01 00 00 57 89 4C 24 10 89 6C 24 20 8B D5 BE 08 00 00 00");
        auto pad = hook::pattern("53 55 8B E9 56 80 BD 75 0B 00 00 00 57 74 17 C6 85 75 0B 00 00 00 8B 0D");
        auto mouse = hook::pattern("83 EC 20 53 55 8B E9 56 8B 45 38 57 89 45 3C C7 45 38 00 00 00 00 C7 45 2C 00 00 00 00");
        keyboardHook = safetyhook::create_inline(keyboard.get_first(), Keyboard);
        padHook = safetyhook::create_inline(pad.get_first(), Pad);
        mouseHook = safetyhook::create_inline(mouse.get_first(), Mouse);
        ready = true;
    }
}
