module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"

export module Input;

import ComVars;
import Rotation;
import Log;

export namespace GTA1Input
{
    SafetyHookMid GatherHook;
    bool MouseAim = true;
    int PacketSlot = 12;                      // stack offset of the packet at the gather hook

    // Vibration (the Classics build): the player loses a life or armour, or the player's car is damaged.
    // Lives and armour are bytes of the player state (from its getters), the car's damage a byte of its record.
    int LivesOffset = -1, ArmourOffset = -1;
    uint8_t* CarDamage = nullptr;
    void Damage(char* state)
    {
        using InputDevices::Rumble;
        static char* lastState = nullptr;
        static int lives = -1, armour = -1, carId = -1, carDamage = -1;
        int type = *reinterpret_cast<int*>(state);
        int id = *reinterpret_cast<int16_t*>(state + 4);
        int nowLives = LivesOffset >= 0 ? *reinterpret_cast<int8_t*>(state + LivesOffset) : -1;
        int nowArmour = ArmourOffset >= 0 ? *reinterpret_cast<int16_t*>(state + ArmourOffset) : -1;
        int nowCar = type == 0 && CarDamage && id >= 0 ? id : -1;
        int nowDamage = nowCar >= 0 ? CarDamage[size_t(nowCar) * GTA1Build::CarStride] : -1;
        if (state == lastState)
        {
            if (nowLives >= 0 && lives > 0 && nowLives < lives) Rumble(1.0f, 700);
            else if (nowArmour >= 0 && armour > 0 && nowArmour < armour) Rumble(0.6f, 300);
            if (nowCar >= 0 && nowCar == carId && nowDamage > carDamage)
                Rumble(std::clamp(0.3f + (nowDamage - carDamage) / 20.0f, 0.3f, 1.0f), 200 + 20 * ULONGLONG(nowDamage - carDamage));
        }
        lastState = state;
        lives = nowLives; armour = nowArmour; carId = nowCar; carDamage = nowDamage;
    }

    uint32_t Transform(uint32_t packet)
    {
        using namespace InputDevices;
        Frontend = false;
        BeginInputFrame();
        Poll();
        CursorConfinement::Frame(Window);
        static bool controlled = false;
        static bool held[6]{};
        if (QuitPromptActive())
        {
            UpdateRumble(true);
            controlled = false;
            std::fill(std::begin(held), std::end(held), false);
            // Bit 6 and bits 23-31 carry the native menu key. Keep them while
            // releasing gameplay actions, so Enter/Escape still reach the dialog.
            return (packet & 0xFF800040u) | 1u | 0x82u | 4u | 8u | 16u | 32u | 256u;
        }
        auto player = nLocalPlayer;
        if (player >= 0 && player < 4 && Available())
        {
            auto state = PlayerStates + player * GTA1Build::PlayerStride;
            auto type = *reinterpret_cast<int*>(state);
            if (!IsPaused() && Focused())
                Damage(state);
            UpdateRumble(IsPaused() || !Focused());
            auto [x, y] = Movement(type != 2);
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
            if (driving && PadConnected && !StickThrottle && !IsPaused())
                y += (Pad.Gamepad.bLeftTrigger > 30) - (Pad.Gamepad.bRightTrigger > 30);
            // Send complete movement states, including release on focus loss.
            if (ModernControls || Gamepad)
            {
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

    void Install()
    {
        SettingsReader ini("");
        MouseAim = ini.ReadBoolean("INPUT", "MouseAim", true);
        FixSettings().OnChange("INPUT.MouseAim", [] { MouseAim = FixSettings().Bool("INPUT.MouseAim"); });
        if (GTA1Build::Current == GTA1Build::Kind::Classics)
        {
            // Lives, armour depletion and a car's damage (Classics code, including London).
            auto lives = hook::pattern("0F BE 44 24 04 8D 0C C0 8D 04 88 8D 0C 40 8A 04 8D ? ? ? ? 3C 63");
            auto armour = hook::pattern("0F BF 44 24 04 8D 0C C0 8D 04 88 8D 04 40 66 FF 0C 85 ? ? ? ? 66 8B 0C 85 ? ? ? ? 66 85 C9");
            auto damage = hook::pattern("0F BF 4C 24 04 8D 04 CD 00 00 00 00 2B C1 8D 04 40 8D 0C 41 C1 E1 04 8A 81 ? ? ? ? C3");
            LivesOffset = int(*lives.get_first<char*>(17) - PlayerStates);
            ArmourOffset = int(*armour.get_first<char*>(18) - PlayerStates);
            CarDamage = *damage.get_first<uint8_t*>(25);
            if (LivesOffset < 0 || LivesOffset >= GTA1Build::PlayerStride) LivesOffset = -1;
            if (ArmourOffset < 0 || ArmourOffset >= GTA1Build::PlayerStride) ArmourOffset = -1;
        }
        auto gather = GTA1Build::Find("8B 44 24 0C 3B C5 74 ? A8 40 8B F0 74", "8B 44 24 10 85 C0 0F 84 ? ? ? ? 8B D0 A8 40 74",
            "8B 44 24 04 85 C0 74 ? 50 E8 ? ? ? ? 83 C4 04 85 C0 74");
        PacketSlot = GTA1Build::Off(12, 0x10, 4);

        // Poll every simulation tick, including ticks with no legacy input. The
        // native recorder and network queue consume the resulting packet afterwards.
        GatherHook = safetyhook::create_mid(gather.get_first(), [](SafetyHookContext& context)
        {
            auto packet = reinterpret_cast<uint32_t*>(context.esp + PacketSlot);
            *packet = Transform(*packet);
            WFP::onGameProcessEvent().executeAll();
        });
        Log::Write("GTA1 input gathering hook installed.");
    }
}
