module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module ClassicKeys;

// MAIN.ClassicKeyLinks (live): keyboard keys linked as in the original game. The fire and check keys work as one key
// (fire while aiming, check otherwise), in menus they confirm and the run key cancels. The keys stay the ones set in
// the game's keyboard options. A key that is also the run or quick turn key isn't linked in play, so running doesn't
// check; in menus it confirms.

namespace
{
    // sBhdKeyboard: the player's keys and the key bit arrays (two sets: held, pressed, repeat)
    constexpr uint32_t KeyRun = 0xA50;
    constexpr uint32_t KeyQuickTurn = 0xA54;
    constexpr uint32_t KeyFire = 0xA5C;
    constexpr uint32_t KeyCheck = 0xA64;
    constexpr uint32_t KeysHeld = 0xA80;
    constexpr uint32_t KeysPressed = 0xAC0;
    constexpr uint32_t KeysRepeat = 0xB20;
    constexpr uint32_t OtherKeysHeld = 0x120;
    constexpr uint32_t OtherKeysPressed = 0x160;
    constexpr uint32_t OtherKeysRepeat = 0x1C0;

    // sBhdPad: the menu's confirm and cancel buttons
    constexpr uint32_t PadDecide = 0xB6C;
    constexpr uint32_t PadCancel = 0xB70;

    // The player's command bits from the keyboard
    constexpr uint32_t CmdCheck = 0x1000;
    constexpr uint32_t CmdFire = 0x2000;

    GameRef<uintptr_t> Keyboard;
    GameRef<uintptr_t> Pad;

    bool Linked()
    {
        static auto& linked = FixSettings().Int("MAIN.ClassicKeyLinks");
        return linked != 0;
    }

    uint32_t Key(uintptr_t keyboard, uint32_t offset)
    {
        return *reinterpret_cast<uint32_t*>(keyboard + offset);
    }

    bool Down(uintptr_t keyboard, uint32_t keys, uint32_t key)
    {
        return key < 256 && ((*reinterpret_cast<uint32_t*>(keyboard + keys + (key >> 5) * 4) >> (key & 31)) & 1);
    }

    // A key that also runs or turns isn't linked in play
    bool Shared(uintptr_t keyboard, uint32_t key)
    {
        return key == Key(keyboard, KeyRun) || key == Key(keyboard, KeyQuickTurn);
    }

    uint32_t MenuButtons(uint32_t keys)
    {
        uintptr_t keyboard = Keyboard;
        uintptr_t pad = Pad;
        if (!Linked() || !keyboard || !pad)
            return 0;
        if (Down(keyboard, keys, Key(keyboard, KeyFire)) || Down(keyboard, keys, Key(keyboard, KeyCheck)))
            return *reinterpret_cast<uint32_t*>(pad + PadDecide);
        if (Down(keyboard, keys, Key(keyboard, KeyRun)))
            return *reinterpret_cast<uint32_t*>(pad + PadCancel);
        return 0;
    }

    // Menu readers (thiscall: menu, which key array set; the pressed one also a flag that reads a menu ignoring input)
    SafetyHookInline MenuHeld, MenuRepeat, MenuPressed;

    template<SafetyHookInline* Hook, uint32_t Keys, uint32_t OtherKeys>
    uint32_t __fastcall MenuReader(uint8_t* menu, void*, int other)
    {
        const auto buttons = Hook->thiscall<uint32_t>(menu, other);
        return menu[0x420] == 1 ? buttons : buttons | MenuButtons(other ? OtherKeys : Keys);
    }

    uint32_t __fastcall MenuReaderPressed(uint8_t* menu, void*, int other, char forced)
    {
        const auto buttons = MenuPressed.thiscall<uint32_t>(menu, other, forced);
        return menu[0x420] == 1 && !forced ? buttons : buttons | MenuButtons(other ? OtherKeysPressed : KeysPressed);
    }

    // The player's keyboard commands
    SafetyHookInline CommandsHeld, CommandsPressed;

    template<SafetyHookInline* Hook, uint32_t Keys>
    uint32_t __fastcall Commands(void* player, void*)
    {
        auto bits = Hook->thiscall<uint32_t>(player);
        uintptr_t keyboard = Keyboard;
        if (!Linked() || !keyboard)
            return bits;
        const auto fire = Key(keyboard, KeyFire);
        const auto check = Key(keyboard, KeyCheck);
        if (!Shared(keyboard, fire) && Down(keyboard, Keys, fire))
            bits |= CmdCheck;
        if (!Shared(keyboard, check) && Down(keyboard, Keys, check))
            bits |= CmdFire;
        return bits;
    }
}

export namespace REClassicKeys
{
    void Install()
    {
        auto held = hook::pattern("56 57 8B 3D ? ? ? ? 8B D1 8B 8F 54 0A 00 00 8B C1 C1 E8 05 83 E1 1F 8B B4 87 80 0A 00 00");
        auto pressed = hook::pattern("56 57 8B 3D ? ? ? ? 8B D1 8B 8F 54 0A 00 00 8B C1 C1 E8 05 83 E1 1F 8B B4 87 C0 0A 00 00");
        auto menuHeld = hook::pattern("80 B9 20 04 00 00 01 75 05 33 C0 C2 04 00 53 8B 5C 24 08 55 8B 2D ? ? ? ? 85 DB 75 ? 8B 85 D8 0D 00 00 8D 95 FC 0C 00 00 83 F8 04 73 05 6B C0 2C 03 D0 8B 12");
        auto menuRepeat = hook::pattern("80 B9 20 04 00 00 01 75 05 33 C0 C2 04 00 53 8B 5C 24 08 55 8B 2D ? ? ? ? 85 DB 75 ? 8B 85 D8 0D 00 00 8D 95 FC 0C 00 00 83 F8 04 73 05 6B C0 2C 03 D0 8B 52 14");
        auto menuPressed = hook::pattern("80 B9 20 04 00 00 01 75 0C 80 7C 24 08 00 75 05 33 C0 C2 08 00");
        Keyboard.SetAddress(*held.get_first<uintptr_t*>(4));
        Pad.SetAddress(*menuHeld.get_first<uintptr_t*>(22));

        CommandsHeld = safetyhook::create_inline(held.get_first(), Commands<&CommandsHeld, KeysHeld>);
        CommandsPressed = safetyhook::create_inline(pressed.get_first(), Commands<&CommandsPressed, KeysPressed>);
        MenuHeld = safetyhook::create_inline(menuHeld.get_first(), MenuReader<&MenuHeld, KeysHeld, OtherKeysHeld>);
        MenuRepeat = safetyhook::create_inline(menuRepeat.get_first(), MenuReader<&MenuRepeat, KeysRepeat, OtherKeysRepeat>);
        MenuPressed = safetyhook::create_inline(menuPressed.get_first(), MenuReaderPressed);
    }
}
