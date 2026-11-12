module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module ClassicKeys;

// MAIN.ClassicKeyLinks (live): keyboard keys linked as in the original games. The attack and examine keys work as one
// key (attack while aiming, examine otherwise), in menus they confirm and the run key cancels. The keys stay the ones
// set in the game's keyboard options. A key that is also the run or quick turn key isn't linked in play, so running
// doesn't examine; in menus it confirms.

namespace
{
    // Keyboard map (player keys from +4, in the game's KC_ order)
    enum Key : uint32_t
    {
        Run = 4,
        QuickTurn = 5,
        Attack = 9,
        Action = 10,
    };

    // sGamePad: the menu's confirm and cancel buttons, the active keyboard map
    constexpr uint32_t PadDecide = 0xB6C;
    constexpr uint32_t PadCancel = 0xB70;
    constexpr uint32_t PadKeyboardMap = 0xBBC;

    // sKeyboard key bit arrays
    constexpr uint32_t KeysHeld = 0x120;
    constexpr uint32_t KeysPressed = 0x160;
    constexpr uint32_t KeysRepeat = 0x1C0;

    GameRef<uintptr_t> GamePad;
    GameRef<uintptr_t> Keyboard;

    bool Linked()
    {
        static auto& linked = FixSettings().Int("MAIN.ClassicKeyLinks");
        return linked != 0;
    }

    uint32_t MapKey(uintptr_t map, uint32_t key)
    {
        return *reinterpret_cast<uint32_t*>(map + 4 + key * 4);
    }

    // A key that also runs or turns isn't linked in play
    bool Shared(uintptr_t map, uint32_t key)
    {
        return key == MapKey(map, Run) || key == MapKey(map, QuickTurn);
    }

    bool Down(uintptr_t keyboard, uint32_t keys, uint32_t key)
    {
        return key < 256 && ((*reinterpret_cast<uint32_t*>(keyboard + keys + (key >> 5) * 4) >> (key & 31)) & 1);
    }

    // Menu buttons from the linked keys, read from the same key array as the menu reader
    uint32_t MenuButtons(uint32_t keys)
    {
        if (!Linked())
            return 0;
        uintptr_t pad = GamePad;
        uintptr_t keyboard = Keyboard;
        const auto map = pad ? *reinterpret_cast<uintptr_t*>(pad + PadKeyboardMap) : 0;
        if (!map || !keyboard)
            return 0;
        if (Down(keyboard, keys, MapKey(map, Attack)) || Down(keyboard, keys, MapKey(map, Action)))
            return *reinterpret_cast<uint32_t*>(pad + PadDecide);
        if (Down(keyboard, keys, MapKey(map, Run)))
            return *reinterpret_cast<uint32_t*>(pad + PadCancel);
        return 0;
    }

    template<SafetyHookInline* Hook, uint32_t Keys>
    uint32_t __cdecl MenuReader()
    {
        return Hook->ccall<uint32_t>() | MenuButtons(Keys);
    }

    SafetyHookInline MenuHeld, MenuPressed, MenuRepeat, MenuPressedAny, MenuRepeatAny;

    // Keyboard map queries (thiscall): asked again with the linked key in place of the queried one
    using Query = bool(__fastcall*)(uintptr_t, void*);

    template<Query* Original, uint32_t Queried, uint32_t With>
    bool __fastcall LinkedQuery(uintptr_t map, void* edx)
    {
        if ((*Original)(map, edx))
            return true;
        if (!Linked())
            return false;
        auto& key = *reinterpret_cast<uint32_t*>(map + 4 + Queried * 4);
        const auto other = MapKey(map, With);
        if (other == key || Shared(map, other))
            return false;
        const auto saved = key;
        key = other;
        const bool result = (*Original)(map, edx);
        key = saved;
        return result;
    }

    Query ActionHeld, ActionPressed, AttackHeld, AttackPressed;
}

export namespace RE0ClassicKeys
{
    void Install()
    {
        // The menu readers: keyboard (ecx) and sGamePad (esi) first
        auto held = hook::pattern("8B 0D ? ? ? ? 56 8B 35 ? ? ? ? 8B 91 20 01 00 00 8B 86");
        auto pressed = hook::pattern("8B 0D ? ? ? ? 56 8B 35 ? ? ? ? 8B 91 60 01 00 00 8B 86");
        auto repeat = hook::pattern("8B 0D ? ? ? ? 56 8B 35 ? ? ? ? 8B 91 C0 01 00 00 8B 86");
        // The same for the first pad with input
        auto repeatAny = hook::pattern("56 8B 35 ? ? ? ? 33 C0 8D 8E AC 01 00 00");
        auto pressedAny = hook::pattern("56 8B 35 ? ? ? ? 33 C0 8D 8E A0 01 00 00");
        Keyboard.SetAddress(*held.get_first<uintptr_t*>(2));
        GamePad.SetAddress(*held.get_first<uintptr_t*>(9));

        MenuHeld = safetyhook::create_inline(held.get_first(), MenuReader<&MenuHeld, KeysHeld>);
        MenuPressed = safetyhook::create_inline(pressed.get_first(), MenuReader<&MenuPressed, KeysPressed>);
        MenuRepeat = safetyhook::create_inline(repeat.get_first(), MenuReader<&MenuRepeat, KeysRepeat>);
        MenuRepeatAny = safetyhook::create_inline(repeatAny.get_first(), MenuReader<&MenuRepeatAny, KeysRepeat>);
        MenuPressedAny = safetyhook::create_inline(pressedAny.get_first(), MenuReader<&MenuPressedAny, KeysPressed>);

        // The player's keyboard map (the partner's map is built right after it with the same code)
        auto map = hook::pattern("53 8B D9 56 8B 74 24 0C 57 C7 03 ? ? ? ? 8D 7B 04 B9 15 00 00 00 F3 A5 8D 4B 60 E8");
        auto vtable = *map.count(2).get(0).get<Query*>(11);
        auto replace = [&](size_t slot, Query& original, Query wrapper)
        {
            original = vtable[slot];
            injector::WriteMemory(&vtable[slot], wrapper, true);
        };
        replace(17, ActionHeld, LinkedQuery<&ActionHeld, Action, Attack>);
        replace(18, ActionPressed, LinkedQuery<&ActionPressed, Action, Attack>);
        replace(28, AttackPressed, LinkedQuery<&AttackPressed, Attack, Action>);
        replace(29, AttackHeld, LinkedQuery<&AttackHeld, Attack, Action>);
    }
}
