#pragma once

namespace GTA2Options { inline void Process(char* menu); }
#include <atomic>

namespace GTA2Input
{
    inline SafetyHookInline PlayerInputHook, MenuInputHook;
    inline SafetyHookMid GatherHook;
    inline void* CachedPlayer = nullptr;
    inline uint32_t CachedNativeButtons = 0;
    inline void* (__fastcall* GetPed)(void*, int) = nullptr;
    inline void (__fastcall* ProcessKey)(void*, int, int) = nullptr;
    inline void (__fastcall* TeleportPed)(void*, int, int, int) = nullptr;
    inline uint32_t* Multiplayer = nullptr;
    inline void** Game = nullptr;
    inline void* Replay = nullptr;
    inline bool (__fastcall* IsReplay)(void*, int) = nullptr;
    inline bool MouseAim = true;
    inline int RecoveryKey = VK_F8;
    inline std::atomic<uint32_t> MenuKeys = 0;
    inline std::atomic<unsigned> TextKey = 0;
    inline void QueueMenuKey(WPARAM key)
    {
        if (!InputDevices::ModernControls) return;
        TextKey = MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC) & 0xFFu;
        switch (key)
        {
        case VK_LEFT: case 'A': MenuKeys.fetch_or(1); break;
        case VK_RIGHT: case 'D': MenuKeys.fetch_or(2); break;
        case VK_UP: case 'W': MenuKeys.fetch_or(4); break;
        case VK_DOWN: case 'S': MenuKeys.fetch_or(8); break;
        case VK_RETURN: MenuKeys.fetch_or(16); break;
        case VK_ESCAPE: MenuKeys.fetch_or(32); break;
        case VK_DELETE: case VK_BACK: MenuKeys.fetch_or(64); break;
        }
    }

    template<class T> T& Field(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<char*>(object) + offset);
    }

    inline int __fastcall PlayerInput(void* player, int)
    {
        using namespace InputDevices;
        Frontend = false;
        MenuKeys = 0;
        BeginInputFrame();
        Poll();
        auto nativeButtons = Field<uint32_t>(player, 4);
        if (CachedPlayer != player) { CachedPlayer = player; CachedNativeButtons = 0; }
        if (Field<uint8_t>(player, 142)) CachedNativeButtons = nativeButtons & 0xFFFu;
        else Field<uint32_t>(player, 4) = CachedNativeButtons;
        // Only the local player; leave network input and replays to the engine.
        auto ped = GetPed(player, 0);
        bool replay = IsReplay(Replay, 0) || Field<int>(Replay, 0x38) == 2;
        bool quitPrompt = Field<uint8_t>(player, 1930) != 0;
        // Keyboard Escape/Enter already reach the native quit prompt; inject only controller buttons.
        static bool confirmHeld = false;
        bool confirm = quitPrompt && Button(XINPUT_GAMEPAD_A);
        if (Focused() && !replay && confirm && !confirmHeld) ProcessKey(player, 0, 28);
        confirmHeld = confirm;
        static bool escapeHeld = false;
        bool escape = quitPrompt && Button(XINPUT_GAMEPAD_B);
        if (Focused() && !replay && escape && !escapeHeld) ProcessKey(player, 0, 1);
        escapeHeld = escape;
        static bool pauseHeld = false;
        bool pause = Button(XINPUT_GAMEPAD_START) || (ModernControls && Key(PauseKey));
        if (Focused() && !replay && pause && !pauseHeld) ProcessKey(player, 0, 0x40);
        pauseHeld = pause;
        bool paused = *Game && Field<int>(*Game, 0) == 2;
        static unsigned inputSamples = 0;
        if (Log::Enabled && Key('W') && inputSamples++ < 8)
        {
            char message[192]{};
            sprintf_s(message, "GTA2 input: ped=%p multiplayer=%u replay=%d paused=%d mode=%d active=%u", ped,
                *Multiplayer, replay, paused, Field<int>(Replay, 0x38), unsigned(Field<uint8_t>(player, 142)));
            Log::Write(message);
        }
        // Byte 142 denotes a new packet, rather than an active player.
        if (Focused() && ped && !*Multiplayer && !replay && !paused && !quitPrompt)
        {
            auto car = Field<void*>(ped, 0x16C);
            auto object = Field<void*>(ped, 0x168);
            auto [x, y] = Movement();
            uint32_t buttons = 0;
            if (car)
            {
                if (y < -0.15f || (PadConnected && Pad.Gamepad.bRightTrigger > 30)) buttons |= 1;
                if (y > 0.15f || (PadConnected && Pad.Gamepad.bLeftTrigger > 30)) buttons |= 2;
                if (x < -0.15f) buttons |= 4;
                if (x > 0.15f) buttons |= 8;
            }
            else if (object)
            {
                auto [aimX, aimY] = Stick(true);
                bool moving = std::hypot(x, y) > 0.15f;
                bool aiming = std::hypot(aimX, aimY) > 0.15f;
                if (!moving && !aiming && ModernControls && MouseAim && (Key(VK_RBUTTON) || Key(VK_LBUTTON)))
                {
                    POINT cursor{}; RECT rect{};
                    if (GetCursorPos(&cursor) && ScreenToClient(Window, &cursor) && GetClientRect(Window, &rect))
                    {
                        aimX = static_cast<float>(cursor.x - rect.right / 2);
                        aimY = static_cast<float>(cursor.y - rect.bottom / 2);
                        aiming = std::hypot(aimX, aimY) > 10.0f;
                    }
                }
                if (moving || aiming)
                {
                    static unsigned samples = 0;
                    if (Log::Enabled && Field<void*>(object, 0x80) && samples++ < 32)
                    {
                        char message[192]{};
                        sprintf_s(message, "GTA2 movement: %.2f,%.2f heading=%d position=%d,%d pad=%d", x, y,
                            Field<int16_t>(object, 0x40), Field<int>(Field<void*>(object, 0x80), 0x14),
                            Field<int>(Field<void*>(object, 0x80), 0x18), PadConnected);
                        Log::Write(message);
                    }
                    // GTA2 headings: 1440 units per turn, zero pointing down the screen,
                    // a heading h facing (sin h, cos h) in screen axes (x right, y down).
                    // Screen directions are relative to the rotated camera view.
                    float dirX = moving ? x : aimX, dirY = moving ? y : aimY;
                    GTA2Camera::ScreenToWorldDirection(dirX, dirY);
                    auto angle = std::atan2(dirX, dirY);
                    auto heading = static_cast<int>(std::lround(angle * (1440.0 / (2.0 * M_PI))));
                    Field<int16_t>(object, 0x40) = static_cast<int16_t>((heading + 1440) % 1440);
                    // Suppress native turn inputs while supplying a world direction.
                    Field<uint32_t>(player, 4) &= ~15u;
                    if (moving) buttons |= 1;
                }
            }
            if (ModernControls && Key(FireKey)) buttons |= 16;
            if (ModernControls && Key(EnterVehicleKey)) buttons |= 32;
            if (ModernControls && Key(JumpKey)) buttons |= 64;
            if (ModernControls && Key(PreviousWeaponKey)) buttons |= 128;
            if (ModernControls && Key(NextWeaponKey)) buttons |= 256;
            if (ModernControls && Key(SpecialKey)) buttons |= 512;
            if (Button(XINPUT_GAMEPAD_X) || (!car && PadConnected && Pad.Gamepad.bRightTrigger > 30)) buttons |= 16;
            if (Button(XINPUT_GAMEPAD_Y)) buttons |= 32;
            if (Button(XINPUT_GAMEPAD_A)) buttons |= 64;
            if (Button(XINPUT_GAMEPAD_LEFT_SHOULDER)) buttons |= 128;
            if (Button(XINPUT_GAMEPAD_RIGHT_SHOULDER)) buttons |= 256;
            if (Button(XINPUT_GAMEPAD_B) || Button(XINPUT_GAMEPAD_LEFT_THUMB)) buttons |= 512;
            Field<uint32_t>(player, 4) |= buttons;

            static bool messageHeld = false, recoveryHeld = false;
            bool message = Button(XINPUT_GAMEPAD_BACK);
            if (message && !messageHeld) ProcessKey(player, 0, 0x41);
            messageHeld = message;
            bool recovery = Key(RecoveryKey);
            if (recovery && !recoveryHeld && (car || object))
            {
                // Ask the engine for ground height and teleport through its native
                // object/vehicle path, without enabling unrelated debug cheats.
                auto physics = car ? car : Field<void*>(object, 0x80);
                if (physics) TeleportPed(ped, 0, Field<int>(physics, 0x14), Field<int>(physics, 0x18));
            }
            recoveryHeld = recovery;
        }
        // The engine retains its input packet between key transitions. Inject
        // only while decoding it, so a released modern key cannot remain latched.
        if (!Focused()) { Field<uint32_t>(player, 4) = 0; CachedNativeButtons = 0; }
        auto result = PlayerInputHook.thiscall<int>(player);
        Field<uint32_t>(player, 4) = nativeButtons;
        return result;
    }

    inline void __fastcall MenuInput(void* menu, int)
    {
        using namespace InputDevices;
        // The engine polls DirectInput here; merge controller transitions afterwards.
        MenuInputHook.thiscall<void>(menu);
        BeginInputFrame();
        Poll();
        auto queued = MenuKeys.exchange(0);
        auto textKey = TextKey.exchange(0);
        auto page = Field<uint16_t>(menu, 286);
        // Modes 2/3 are native text editing; leave its raw scancodes alone.
        if (Field<int>(menu, 272) != 1)
        {
            queued = 0;
            if (Focused() && Field<int>(menu, 272) == 3 && textKey)
                Field<uint8_t>(menu, 8 + textKey) = 0x80;
        }
        if (Focused() && (page == 8 || page == 15) &&
            (queued || Button(XINPUT_GAMEPAD_A) || Button(XINPUT_GAMEPAD_B) || Button(XINPUT_GAMEPAD_START)))
            // Movie playback checks the raw DirectInput array instead of the
            // menu action flags. Preserve quick WM_KEYDOWN taps there too.
            Field<uint8_t>(menu, 8) = 0x80; // Unassigned scan code; cannot also quit the next menu.
        if (Focused() && page != 8 && page != 15)
            for (int i = 0; i < 7; ++i)
                if ((queued >> i) & 1)
                {
                    Field<uint8_t>(menu, 51640 + i) = 1;
                    // A WM tap can arrive a frame before DirectInput notices
                    // the same press. Mark it held to prevent a second activation.
                    Field<uint8_t>(menu, 51647 + i) = 0x80;
                }
        static bool held[7]{};
        static DWORD repeat[4]{};
        if (Focused() && PadConnected && Field<int>(menu, 272) == 1)
        {
            auto [x, y] = Stick();
            const bool down[] = {
                x < -0.35f || Button(XINPUT_GAMEPAD_DPAD_LEFT),
                x > 0.35f || Button(XINPUT_GAMEPAD_DPAD_RIGHT),
                y < -0.35f || Button(XINPUT_GAMEPAD_DPAD_UP),
                y > 0.35f || Button(XINPUT_GAMEPAD_DPAD_DOWN),
                Button(XINPUT_GAMEPAD_A), Button(XINPUT_GAMEPAD_B), Button(XINPUT_GAMEPAD_X)
            };
            auto now = GetTickCount();
            for (int i = 0; i < 7; ++i)
            {
                bool pressed = down[i] && !held[i];
                if (i < 4)
                {
                    if (pressed) repeat[i] = now + 400;
                    else if (down[i] && static_cast<int32_t>(now - repeat[i]) >= 0)
                    {
                        pressed = true;
                        repeat[i] = now + 120;
                    }
                }
                Field<uint8_t>(menu, 51640 + i) |= static_cast<uint8_t>(pressed);
                Field<uint8_t>(menu, 51647 + i) |= static_cast<uint8_t>(down[i]);
                held[i] = down[i];
            }
        }
        else std::fill(std::begin(held), std::end(held), false);
        GTA2Options::Process(static_cast<char*>(menu));
    }

    inline void Install(CIniReader& ini)
    {
        MouseAim = ini.ReadBoolean("INPUT", "MouseAim", true);
        RecoveryKey = ini.ReadInteger("MISC", "RecoverPlayerKey", VK_F8);
        GetPed = reinterpret_cast<decltype(GetPed)>(hook::get_pattern("8B 41 68 83 F8 02 74 0C 83 F8 03 74 07 8B 81 C4 02 00 00 C3"));
        ProcessKey = reinterpret_cast<decltype(ProcessKey)>(hook::get_pattern("53 8B 5C 24 08 56 57 8B FB 8B F1 8B 0D ? ? ? ? 81 E7 FF FF 00 00"));
        TeleportPed = reinterpret_cast<decltype(TeleportPed)>(hook::get_pattern("53 8B 5C 24 08 56 57 8B 7C 24 14 8D 44 24 14 57 8B F1 8B 0D"));
        Multiplayer = *hook::pattern("A1 ? ? ? ? 85 C0 75 ? 8A 41 30").get_first<uint32_t*>(1);
        Game = *hook::pattern("8B 0D ? ? ? ? 6A 02 6A 01 E8 ? ? ? ? 5F 5E 5B C2 04 00").get_first<void**>(2);
        Replay = *hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 84 C0 74 22 B9 ? ? ? ?").get_first<void*>(1);
        IsReplay = reinterpret_cast<decltype(IsReplay)>(hook::get_pattern("8B 41 38 83 F8 01 74 08 83 F8 03 74"));
        PlayerInputHook = safetyhook::create_inline(hook::get_pattern("56 8B 71 04 8A 51 78 8B C6 24 01 3C 01 0F 94 C0"), PlayerInput);
        GatherHook = safetyhook::create_mid(hook::get_pattern("84 C0 74 05 E8 ? ? ? ? 33 C0 47 8A 43 23"), [](SafetyHookContext& context)
        {
            using namespace InputDevices;
            // Decode every simulation tick, including ticks without a legacy
            // keyboard packet, so modern input and releases are always polled.
            if ((ModernControls || Gamepad) && !*Multiplayer && !IsReplay(Replay, 0) && Field<int>(Replay, 0x38) != 2)
                context.eax = (context.eax & ~255u) | 1u;
        });
        MenuInputHook = safetyhook::create_inline(hook::get_pattern("53 56 8B F1 32 DB 38 9E 0D 01 00 00 0F 84"), MenuInput);
    }
}
