module;

#include <stdafx.h>
#include "Shared/ResidentEvil/InputDrain.hpp"
#include <functional>

export module InputDrain;

export namespace REInput
{
    using namespace REInputDrain;

    inline SafetyHookInline keyboardHook, padHook, mouseHook;
    inline std::function<bool()> overlayBlocksInput;
    inline std::array<HeldControls, 8> keyboardHeld{};
    inline HeldControls mouseHeld{};
    inline std::array<HeldControls, 4> padHeld{};

    inline InputAccess Access()
    {
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        if (process != GetCurrentProcessId()) return InputAccess::Unfocused;
        if (overlayBlocksInput && overlayBlocksInput()) return InputAccess::Blocked;
        return InputAccess::Allowed;
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
        auto currentKey = hook::pattern("83 EC 08 53 55 56 57 8B F9 8B 87 20 01 00 00 8D 9F 20 01 00 00 C7 03 00 00 00 00 89 87 40 01 00 00");
        auto oldKey = hook::pattern("83 EC 14 53 55 56 8D A9 20 01 00 00 57 89 4C 24 10 89 6C 24 20 8B D5 BE 08 00 00 00");
        auto currentPad = hook::pattern("83 EC 24 53 55 8B E9 56 57 89 6C 24 20 80 BD 75 0B 00 00 00 74 17 C6 85 75 0B 00 00 00");
        auto oldPad = hook::pattern("53 55 8B E9 56 80 BD 75 0B 00 00 00 57 74 17 C6 85 75 0B 00 00 00 8B 0D");
        auto currentMouse = hook::pattern("83 EC 1C 53 8B D9 55 56 8B 35 ? ? ? ? 8B 43 38 89 43 3C 8D 44 24 18 57 C7 43 38 00 00 00 00 8D 7B 20");
        auto oldMouse = hook::pattern("83 EC 20 53 55 8B E9 56 8B 45 38 8B 35 ? ? ? ? 89 45 3C 57 8D 44 24 20 C7 45 38 00 00 00 00");
        keyboardHook = safetyhook::create_inline(currentKey.size() ? currentKey.get_first() : oldKey.get_first(), Keyboard);
        padHook = safetyhook::create_inline(currentPad.size() ? currentPad.get_first() : oldPad.get_first(), Pad);
        mouseHook = safetyhook::create_inline(currentMouse.size() ? currentMouse.get_first() : oldMouse.get_first(), Mouse);
    }
}
