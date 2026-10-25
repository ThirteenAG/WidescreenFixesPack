module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "PlayerControls.hpp"
#include <array>

export module Controls;

namespace
{
    using namespace RevPlayer;
    constexpr float Pi{ 3.14159265358979323846f };
    uintptr_t* pPad{};
    uintptr_t* pCamera{};
    bool (__thiscall* QuickTurn)(uintptr_t, int){};
    SafetyHookInline InputHook;
    SafetyHookInline ViewHook;
    SafetyHookMid LegsHook;
    SafetyHookInline ReloadHook;

    struct ViewState
    {
        uintptr_t ch{};
        uint32_t serial{};
        std::array<float, 3> position{};
        float yaw{};
        float pitch{};
        float lookX{};
        float lookY{};
        bool aiming{};
        bool directionLocked{};
        float lockedAngle{};
        float lockedYaw{};
        double time{ -10.0 };
        double inputTime{};
        double viewTime{};
    } view;

    float Delta(double& previous)
    {
        auto now = Now();
        auto result = previous > 0.0 ? static_cast<float>(std::clamp(now - previous, 0.0, 0.1)) : 0.0f;
        previous = now;
        return result;
    }

    float Wrap(float angle)
    {
        return std::remainder(angle, 2.0f * Pi);
    }

    float Yaw(uintptr_t ch)
    {
        auto x = Field<float>(ch, 0x50);
        auto y = Field<float>(ch, 0x54);
        auto z = Field<float>(ch, 0x58);
        auto w = Field<float>(ch, 0x5C);
        return std::atan2(2.0f * (w * y + x * z), 1.0f - 2.0f * (y * y + x * x));
    }

    void SetYaw(uintptr_t ch, float yaw)
    {
        Write(ch, 0x50, 0.0f);
        Write(ch, 0x54, std::sin(yaw * 0.5f));
        Write(ch, 0x58, 0.0f);
        Write(ch, 0x5C, std::cos(yaw * 0.5f));
    }

    uintptr_t Manager(uintptr_t ch)
    {
        auto root = pCamera ? *pCamera : 0;
        auto manager = root ? Field<uintptr_t>(root, 0xCE0) : 0;
        if (!manager || Field<uintptr_t>(manager, 0x78) != ch || Field<uintptr_t>(manager, 0x74) != ch)
            return 0;
        auto index = Field<uint32_t>(manager, 0xCC);
        auto count = Field<uint32_t>(manager, 0xD8);
        auto cameras = Field<uintptr_t*>(manager, 0xE4);
        if (!cameras || count > 64 || index >= count || index != Field<uint32_t>(manager, 0xD0) ||
            index != Field<uint32_t>(manager, 0xE8))
            return 0;
        auto camera = cameras[index];
        auto name = TypeName(camera);
        if (!name || std::string_view(name) != "cTPSCamera" || Field<uintptr_t>(camera, 0x50) != manager ||
            !Field<uintptr_t>(camera, 0xC4) || Field<uint8_t>(camera, 0xDC) || Field<uint32_t>(camera, 0xE4))
            return 0;
        return manager;
    }

    bool Ordinary(uintptr_t ch)
    {
        auto state = Lower(ch);
        return state >= 1 && state <= 5 && Field<int>(ch, 3848) != 4 &&
            !(Field<uint32_t>(ch, 0xF3C) & 0x100000);
    }

    void Sync(uintptr_t ch, uintptr_t manager)
    {
        auto now = Now();
        auto position = Field<std::array<float, 3>>(ch, 0x40);
        auto relocated = view.ch == ch && std::hypot(position[0] - view.position[0],
            position[1] - view.position[1], position[2] - view.position[2]) > 1000.0f;
        view.position = position;
        if (relocated || view.ch != ch || view.serial != Field<uint32_t>(ch, 0xE2C) || now - view.time > 0.25)
        {
            if (relocated || view.ch != ch || view.serial != Field<uint32_t>(ch, 0xE2C) || now - view.time > 2.0)
                view.directionLocked = false;
            view.ch = ch;
            view.serial = Field<uint32_t>(ch, 0xE2C);
            view.yaw = Wrap(Yaw(ch) + (relocated ? 0.0f : Field<float>(manager, 0x154)));
            view.pitch = Field<float>(manager, 0x150);
            view.aiming = false;
            view.lookX = view.lookY = 0.0f;
            view.inputTime = view.viewTime = now;
        }
        view.time = now;
    }

    bool Dodge(uintptr_t ch, uintptr_t pad)
    {
        if (!Field<uint8_t>(ch, 4433))
            return false;
        view.directionLocked = false;
        auto slot = Field<uint32_t>(pad, 0x870);
        if (slot >= 2)
            return false;
        auto control = pad + 0x628 + slot * 0x90;
        auto pressed = Field<int>(control, 16) == 6 ? QuickTurn(pad, 0) :
            (Field<uint32_t>(pad, slot * 716 + 416) & 0x4000) != 0;
        if (!pressed)
            return false;
        auto x = Field<float>(control, 40);
        auto y = Field<float>(control, 44);
        int action{ -1 };
        if (std::abs(x) > 0.4f && std::abs(x) >= std::abs(y))
            action = x < 0.0f ? 17 : 18;
        else if (y < -0.4f)
            action = 19;
        return action >= 0 && ChangeState(Field<uintptr_t>(ch, 0xE80), action, 0, nullptr);
    }

    int __fastcall UpdateInput(uintptr_t ch, void*)
    {
        auto result = InputHook.unsafe_thiscall<int>(ch);
        if (!Offline(ch) || !Ordinary(ch))
            return result;
        auto manager = Manager(ch);
        auto pad = pPad ? *pPad : 0;
        if (pad && Field<uint8_t>(pad, 0x882) && Dodge(ch, pad))
            return result;
        if (!manager || !pad || !Field<uint8_t>(pad, 0x882))
            return result;
        Sync(ch, manager);
        auto slot = Field<uint32_t>(pad, 0x870);
        if (slot >= 2)
            return result;
        auto control = pad + 0x628 + slot * 0x90;
        auto dt = Delta(view.inputTime);
        auto x = Field<float>(control, 40);
        auto y = Field<float>(control, 44);
        auto aiming = Field<uint8_t>(ch, 4433) != 0;
        // Native aim entry transfers the TPS offsets into the player's facing and pitch.
        if (aiming && !view.aiming)
            view.directionLocked = false;
        if (!aiming && view.aiming)
            view.yaw = Wrap(Yaw(ch) + Field<float>(manager, 0x154));
        view.aiming = aiming;
        auto type = Field<int>(control, 16);
        if (type == 6 && !aiming)
        {
            view.lookX += Field<float>(ch, 4416) * dt;
            view.lookY += Field<float>(ch, 4420) * dt;
        }
        else
            view.lookX = view.lookY = 0.0f;
        auto pressed = type == 6 ? QuickTurn(pad, 0) : (Field<uint32_t>(pad, slot * 716 + 416) & 0x4000) != 0;
        if (!aiming && pressed && y < -0.4f && std::abs(y) > std::abs(x))
        {
            auto target = Wrap(Yaw(ch) + Pi);
            if (ChangeState(Field<uintptr_t>(ch, 0xE80), 6, 0, nullptr))
            {
                view.directionLocked = true;
                view.lockedAngle = std::atan2(-x, y);
                view.lockedYaw = target;
                return result;
            }
        }
        if (!aiming)
        {
            auto magnitude = (std::min)(std::hypot(x, y), 1.0f);
            if (!std::isfinite(magnitude))
                return result;
            if (magnitude > 0.01f)
            {
                auto angle = std::atan2(-x, y);
                auto target = Wrap(view.yaw + angle);
                if (view.directionLocked)
                {
                    if (std::abs(Wrap(angle - view.lockedAngle)) < Pi / 6.0f)
                        target = view.lockedYaw;
                    else
                        view.directionLocked = false;
                }
                auto yaw = Yaw(ch);
                auto turn = std::clamp(Wrap(target - yaw), -3.0f * Pi * dt, 3.0f * Pi * dt);
                SetYaw(ch, Wrap(yaw + turn));
            }
            else
                view.directionLocked = false;
            Write(ch, 4408, 0.0f);
            Write(ch, 4412, magnitude);
            Write(ch, 4416, 0.0f);
            Write(ch, 4420, 0.0f);
        }
        return result;
    }

    void __fastcall UpdateView(uintptr_t camera, void*, int arg)
    {
        auto manager = Field<uintptr_t>(camera, 0x50);
        auto ch = manager ? Field<uintptr_t>(manager, 0x78) : 0;
        if (!Offline(ch) || !Ordinary(ch) || Manager(ch) != manager || Field<uint8_t>(ch, 4433))
        {
            ViewHook.unsafe_thiscall(camera, arg);
            if (ch && Local(ch) && manager)
            {
                view.yaw = Wrap(Yaw(ch) + Field<float>(manager, 0x154));
                view.pitch = Field<float>(manager, 0x150);
                view.viewTime = Now();
                view.lookX = view.lookY = 0.0f;
            }
            return;
        }
        Sync(ch, manager);
        auto pad = pPad ? *pPad : 0;
        if (!pad || !Field<uint8_t>(pad, 0x882))
        {
            ViewHook.unsafe_thiscall(camera, arg);
            return;
        }
        auto slot = Field<uint32_t>(pad, 0x870);
        if (slot >= 2)
        {
            ViewHook.unsafe_thiscall(camera, arg);
            return;
        }
        auto control = pad + 0x628 + slot * 0x90;
        auto dt = Delta(view.viewTime);
        auto keyboard = Field<int>(control, 16) == 6;
        auto x = view.lookX;
        auto y = view.lookY;
        view.lookX = view.lookY = 0.0f;
        if (!keyboard)
        {
            auto axis = [](int value)
            {
                auto normalized = std::clamp(value / 32767.0f, -1.0f, 1.0f);
                return std::copysign((std::max)(std::abs(normalized) - 0.2f, 0.0f) / 0.8f, normalized);
            };
            auto speed = 0.6f + 0.2f * std::clamp(Field<int>(control, 24), 0, 4);
            x = axis(Field<int>(pad, slot * 716 + 432)) * 3.0f * speed;
            y = axis(Field<int>(pad, slot * 716 + 436)) * 2.0f * speed;
        }
        if (!keyboard)
        {
            auto invert = Field<uint32_t>(control, 32);
            if (invert & 2)
                x = -x;
            if (invert & 1)
                y = -y;
        }
        if (!std::isfinite(x) || !std::isfinite(y))
            return;
        auto scale = keyboard ? 1.0f : (Now() - view.inputTime < 0.1 ? dt : 0.0f);
        view.yaw = Wrap(view.yaw - x * scale);
        view.pitch = std::clamp(view.pitch - y * scale, -Pi * 0.42f, Pi * 0.42f);
        auto relative = Wrap(view.yaw - Yaw(ch));
        Write(camera, 200, view.pitch);
        Write(camera, 204, relative);
        Write(manager, 0x150, view.pitch);
        Write(manager, 0x154, relative);
    }

    bool __fastcall ReloadPressed(uintptr_t control, void*, int slot)
    {
        auto root = pCamera ? *pCamera : 0;
        auto manager = root ? Field<uintptr_t>(root, 0xCE0) : 0;
        auto ch = manager ? Field<uintptr_t>(manager, 0x78) : 0;
        auto pad = pPad ? *pPad : 0;
        if (!Offline(ch) || !Ordinary(ch) || !pad || slot < 0 || slot >= 2 || Field<int>(control, 16) == 6)
            return ReloadHook.unsafe_thiscall<bool>(control, slot);
        auto active = Field<uint32_t>(pad, 0x870);
        return active < 2 &&
            (Field<uint32_t>(pad, active * 716 + 416) & 0x8000) != 0;
    }

    void AllowLegs(SafetyHookContext& regs)
    {
        auto ch = static_cast<uintptr_t>(regs.esi);
        if (!Offline(ch) || !Ordinary(ch))
            return;
        auto upper = Upper(ch);
        if (upper == 8 || upper == 9 || upper == 10)
            Write(static_cast<uintptr_t>(regs.esp), 0x12, uint8_t{ 1 });
    }
}

class Controls
{
public:
    Controls()
    {
        WFP::onInitEvent() += []()
        {
            if (!RevPlayer::Init())
                return;
            auto input = find_pattern("55 8B EC 83 E4 F8 83 EC 34 53 56 8B F1 33 DB 57 39 9E 80 0E 00 00");
            auto camera = find_pattern("55 8B EC 83 E4 F8 83 EC 1C 53 56 8B F1 8B 4E 50 8B 49 78 33 C0 57");
            auto pad = find_pattern("8B 0D ? ? ? ? E8 ? ? ? ? D9 00 D9 9E 38 11 00 00 53 D9 40 04 D9 9E 3C 11 00 00");
            auto root = find_pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 66 89 5C 24 0C 88 5C 24 0E 8B 44 24 0C");
            auto quick = find_pattern("8B CE E8 ? ? ? ? 50 8B CF E8 ? ? ? ? 84 C0 74 ? 6A 06 8B CD E8");
            auto reload = find_pattern("51 53 56 8B F1 8B 46 10 32 DB 83 F8 06 0F 87 ? ? ? ? 57 FF 24 85 ? ? ? ? 8B 44 24 14 50 E8");
            auto legs = find_pattern("8B 8E 3C 0F 00 00 C1 E9 14 F6 C1 01 0F 85 ? ? ? ? 8B 3D ? ? ? ? 8B CE E8 ? ? ? ? 50 8B CF E8");
            if (input.size() != 1 || camera.size() != 1 || pad.size() != 1 || root.size() != 1 || quick.size() != 1 || legs.size() != 1 || reload.size() != 1)
            {
                OutputDebugStringA("Resident Evil Revelations Fusion Fix: modern controls layout unavailable.\n");
                return;
            }
            pPad = *pad.get_first<uintptr_t*>(2);
            pCamera = *root.get_first<uintptr_t*>(2);
            QuickTurn = reinterpret_cast<decltype(QuickTurn)>(injector::GetBranchDestination(quick.get_first(10)).as_int());
            ReloadHook = safetyhook::create_inline(reload.get_first(), ReloadPressed, SafetyHookInline::StartDisabled);
            InputHook = safetyhook::create_inline(input.get_first(), UpdateInput, SafetyHookInline::StartDisabled);
            ViewHook = safetyhook::create_inline(camera.get_first(), UpdateView, SafetyHookInline::StartDisabled);
            LegsHook = safetyhook::create_mid(legs.get_first(), AllowLegs, SafetyHookMid::StartDisabled);
            if (!InputHook || !ViewHook || !LegsHook || !ReloadHook)
                return;
            auto apply = []()
            {
                auto enabled = FixSettings().Int("MAIN.ModernControls") != 0;
                SetEnabled(InputHook, enabled);
                SetEnabled(ViewHook, enabled);
                SetEnabled(LegsHook, enabled);
                SetEnabled(ReloadHook, enabled);
            };
            FixSettings().OnChange("MAIN.ModernControls", apply);
            apply();
        };
    }
} Controls;
