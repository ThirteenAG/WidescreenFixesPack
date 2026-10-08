#pragma once

namespace GTA1Rotation { inline void ScreenToWorld(float& x, float& y); }
namespace GTA1Options { inline void Tick(); }

namespace GTA1Input
{
    inline SafetyHookMid GatherHook;
    inline int* LocalPlayer = nullptr;
    inline int* Replay = nullptr;
    inline int* PlayerCount = nullptr;
    inline int* Paused = nullptr;
    inline int* Recording = nullptr;
    inline bool MouseAim = true;
    inline char* PlayerState = nullptr;
    inline int PacketSlot = 12;                      // stack offset of the packet at the gather hook
    inline char* (__cdecl* GetPed)(int16_t) = nullptr;
    inline bool Available()
    {
        return Replay && PlayerCount && Recording && !*Replay && *PlayerCount == 1 && !*Recording;
    }
    inline bool IsPaused() { return Paused && *Paused == 2; }

    inline uint32_t Transform(uint32_t packet)
    {
        using namespace InputDevices;
        Frontend = false;
        BeginInputFrame();
        Poll();
        auto player = *LocalPlayer;
        if (player >= 0 && player < 4 && Available())
        {
            auto state = PlayerState + player * GTA1Build::PlayerStride;
            auto type = *reinterpret_cast<int*>(state);
            auto [x, y] = Movement();
            if (IsPaused()) x = y = 0;
            auto [aimX, aimY] = Stick(true);
            bool aiming = !IsPaused() && std::hypot(aimX, aimY) > 0.15f;
            if (!IsPaused() && !aiming && ModernControls && MouseAim && (Key(VK_RBUTTON) || Key(VK_LBUTTON)))
            {
                POINT cursor{}; RECT rect{};
                if (GetCursorPos(&cursor) && ScreenToClient(Window, &cursor) && GetClientRect(Window, &rect))
                {
                    aimX = float(cursor.x - rect.right / 2);
                    aimY = float(cursor.y - rect.bottom / 2);
                    aiming = std::hypot(aimX, aimY) > 10.0f;
                }
            }
            // Native player modes: 0 vehicle, 1 train, 2 pedestrian.
            bool driving = type != 2;
            if (driving && PadConnected && !IsPaused())
                y += (Pad.Gamepad.bLeftTrigger > 30) - (Pad.Gamepad.bRightTrigger > 30);
            // Send complete movement states, including release on focus loss.
            if (ModernControls || Gamepad)
            {
                static bool controlled = false;
                bool moving = std::hypot(x, y) > 0.15f;
                if (moving || controlled || (!driving && aiming))
                {
                    packet &= ~0x1FE83u;
                    packet |= 1u;
                    if (driving)
                    {
                        auto turn = x < -0.15f ? -7 : x > 0.15f ? 7 : 0;
                        packet |= (turn & 15) << 9;
                        if (y < -0.15f) packet |= 2u | (3u << 15);
                        else if (y > 0.15f) packet |= 0x80u | (3u << 13);
                        else packet |= 0x82u;
                    }
                    else if (moving || aiming)
                    {
                        auto id = *reinterpret_cast<int16_t*>(state + 4);
                        if (id >= 0)
                        {
                            auto ped = GetPed(id);
                            // Screen directions follow the rotating camera.
                            float dirX = moving ? x : aimX, dirY = moving ? y : aimY;
                            GTA1Rotation::ScreenToWorld(dirX, dirY);
                            auto heading = static_cast<int>(std::lround(std::atan2(dirX, dirY) * (1024.0 / (2.0 * M_PI))));
                            heading = (heading + 1024) % 1024;
                            *reinterpret_cast<int16_t*>(ped + GTA1Build::PedHeading) = static_cast<int16_t>(heading);
                            packet |= moving ? 2u | (3u << 15) : 0x82u;
                        }
                    }
                }
                if (!moving && controlled) packet |= 0x82u;
                controlled = moving;
            }
            const bool actions[] = {
                (ModernControls && Key(FireKey)) || Button(XINPUT_GAMEPAD_X) ||
                    (!driving && PadConnected && Pad.Gamepad.bRightTrigger > 30),
                (ModernControls && Key(EnterVehicleKey)) || Button(XINPUT_GAMEPAD_Y),
                (ModernControls && Key(JumpKey)) || Button(XINPUT_GAMEPAD_A),
                (ModernControls && Key(SpecialKey)) || Button(XINPUT_GAMEPAD_B),
                (ModernControls && Key(PreviousWeaponKey)) || Button(XINPUT_GAMEPAD_LEFT_SHOULDER),
                (ModernControls && Key(NextWeaponKey)) || Button(XINPUT_GAMEPAD_RIGHT_SHOULDER)
            };
            static bool held[6]{};
            // Native control slots: fire, jump/handbrake, enter/exit, special, previous and next weapon.
            constexpr uint32_t changed[] = {4, 0, 8, 32, 16, 256};
            constexpr uint32_t value[] = {0x100000, 0x400000, 0x200000, 0x80000, 0x40000, 0x20000};
            for (int i = 0; i < 6; ++i)
            {
                bool action = actions[i] && !IsPaused();
                if (action != held[i])
                {
                    packet |= changed[i];
                    if (action) packet |= value[i];
                    else packet &= ~value[i];
                }
                held[i] = action;
            }
        }
        return packet;
    }

    inline void Install()
    {
        CIniReader ini("");
        MouseAim = ini.ReadBoolean("INPUT", "MouseAim", true);
        using GTA1Build::Find, GTA1Build::Off;
        // The player state array (from the packet handler), the local player, the replay
        // mode, the ped getter, the player count, the recording flag and the pause state.
        auto input = Find("A1 ? ? ? ? 53 8B 5C 24 08 56 F6 C3 01 74 35", "8B BC C1 ? ? ? ? 8D 04 C1 85 FF",
            "8B 0D ? ? ? ? 8D 04 49 8D 04 81 C1 E0 05 03 C1 8B 88 ? ? ? ? 83 E9 00 74 10 83 E9 02");
        auto local = Find("8B 0D ? ? ? ? 8B 15 ? ? ? ? 33 C0 3B CA 0F 94 C0 C3 90 90 90 90 90 90 90 90 90 90 90 90 0F BE 44 24 04",
            "33 C0 8B 0D ? ? ? ? 3B 0D ? ? ? ? 0F 94 C0 C3", "8B 0D ? ? ? ? 8B 15 ? ? ? ? 33 C0 3B CA 0F 94 C0 C3");
        auto replay = Find("51 A1 ? ? ? ? 53 55 33 ED 2B C5 56 89 6C 24 0C",
            "83 EC 04 A1 ? ? ? ? 85 C0 C7 44 24 00 00 00 00 00 53 56 57 55", "51 A1 ? ? ? ? 56 83 E8 00 C7 44 24 04 00 00 00 00");
        auto ped = Find("0F BF 44 24 04 C1 E0 08 05 ? ? ? ? C3 90 90 57",
            "0F BF 44 24 04 53 8B C8 8D 14 40 8D 04 91 8D 1C C0 8D 84 59 ? ? ? ? 5B C3", "0F BF 4C 24 04 8D 04 49 C1 E0 04 2B C1 8D 84 80 ? ? ? ? C3");
        auto mode = Find("83 3D ? ? ? ? 01 75 09 8B 44 24 04 A3 ? ? ? ? C3",
            "A1 ? ? ? ? 83 F8 01 75 09 8B 44 24 04 A3 ? ? ? ? C3", "83 3D ? ? ? ? 01 75 09 8B 44 24 04 A3 ? ? ? ? C3");
        auto recording = hook::pattern("89 15 ? ? ? ? 8B 54 24 60 A3 ? ? ? ? 8B 44 24 68");
        auto paused = Find("83 3D ? ? ? ? 02 0F 94 C0 C3", "83 3D ? ? ? ? 02 0F 94 C0 C3", "8B 0D ? ? ? ? 33 C0 83 F9 02 0F 94 C0 C3");
        if (input.size() != 1 || local.size() != 1 || replay.size() != 1 || ped.size() != 1 || mode.size() != 1 || recording.size() != 1 || paused.size() != 1)
        {
            char diagnostic[128]{};
            sprintf_s(diagnostic, "GTA1 input signatures: input=%zu local=%zu replay=%zu ped=%zu", input.size(), local.size(), replay.size(), ped.size());
            Log::Write(diagnostic);
            return;
        }
        LocalPlayer = *local.get_first<int*>(Off(8, 4, 8));
        Replay = *replay.get_first<int*>(Off(2, 4, 2));
        PlayerCount = *mode.get_first<int*>(Off(2, 1, 2));
        Paused = *paused.get_first<int*>(2);
        Recording = *recording.get_first<int*>(2);
        PlayerState = *input.get_first<char*>(Off(49, 3, 19));
        GetPed = reinterpret_cast<decltype(GetPed)>(ped.get_first());
        auto gather = Find("8B 44 24 0C 3B C5 74 ? A8 40 8B F0 74", "8B 44 24 10 85 C0 0F 84 ? ? ? ? 8B D0 A8 40 74",
            "8B 44 24 04 85 C0 74 ? 50 E8 ? ? ? ? 83 C4 04 85 C0 74");
        PacketSlot = Off(12, 0x10, 4);
        if (gather.size() != 1) { Log::Write("GTA1 input gathering signature unavailable."); return; }
        // Poll every simulation tick, including ticks with no legacy input. The
        // native recorder and network queue consume the resulting packet afterwards.
        GatherHook = safetyhook::create_mid(gather.get_first(), [](SafetyHookContext& context)
        {
            auto packet = reinterpret_cast<uint32_t*>(context.esp + PacketSlot);
            *packet = Transform(*packet);
            GTA1Options::Tick();
        });
        Log::Write(GatherHook ? "GTA1 input gathering hook installed." : "GTA1 input gathering hook FAILED.");
    }
}
