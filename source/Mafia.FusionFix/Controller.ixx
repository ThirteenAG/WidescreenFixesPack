module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include <atomic>
#include <mutex>

export module Controller;

import ComVars;
import NativeMenu;
import Menu;

// Modern controller layout on the game's own pad bindings, pad navigation of the native menus
export namespace MafiaPad
{
    struct Binding
    {
        uint32_t direction = 0;
        uint16_t type = 0, code = 0;
        uint32_t device = 0;
    };
    static_assert(sizeof(Binding) == 12);
    struct Vector
    {
        uintptr_t begin, end, capacity;
    };
    static_assert(sizeof(Vector) == 12);
    constexpr size_t ActionCount = 60;
    enum Action : size_t
    {
        Forwards = 0, Backwards = 2, Left = 4, Right = 6, WalkRun = 8,
        Use = 10, Fire = 12, Crouch = 14, Jump = 16, LookX = 18, LookY = 19,
        NextWeapon = 20, PreviousWeapon = 22, Inventory = 24, HideWeapon = 25,
        DropWeapon = 26, ToggleWalk = 27, Reload = 28, SniperAim = 29, Objectives = 30, CityMap = 31,
        Accelerate = 33, Brake = 35, SteerLeft = 37, SteerRight = 39, Handbrake = 41,
        SpeedLimiter = 43, GearUp = 45, GearDown = 47, Horn = 49, LookLeft = 51,
        LookRight = 53, Clutch = 55, ManualGears = 57, ChangeCamera = 58
    };
    constexpr auto modern = []()
    {
        std::array<Binding, ActionCount> bindings{};
        const auto button = [&](size_t action, uint16_t index) { bindings[action] = { 0, 3, static_cast<uint16_t>(0x100 + index), 0 }; };
        const auto axis = [&](size_t action, uint16_t index, uint32_t direction) { bindings[action] = { direction, 3, index, 0 }; };
        const auto hat = [&](size_t action, uint16_t direction) { bindings[action] = { 0, 3, static_cast<uint16_t>(0x200 + direction), 0 }; };
        axis(Forwards, 1, 2); axis(Backwards, 1, 1); axis(Left, 0, 2); axis(Right, 0, 1);
        button(WalkRun, 10); button(Use, 3); button(Fire, 7); button(Crouch, 1); button(Jump, 0);
        axis(LookX, 3, 0); axis(LookY, 4, 0);
        button(NextWeapon, 5); button(PreviousWeapon, 4); button(HideWeapon, 11);
        button(Reload, 2); button(SniperAim, 6); button(CityMap, 8);
        hat(Inventory, 1); hat(DropWeapon, 4); hat(ToggleWalk, 8); hat(Objectives, 2);
        // Analog triggers: RT is also the Z axis and LT the Z rotation (Xidi.ini)
        axis(Accelerate, 2, 1); axis(Brake, 5, 1); axis(SteerLeft, 0, 2); axis(SteerRight, 0, 1);
        button(Handbrake, 0); button(GearUp, 2); button(GearDown, 1); button(Horn, 10); button(Clutch, 4);
        hat(SpeedLimiter, 1); hat(ManualGears, 8); hat(ChangeCamera, 2);
        return bindings;
    }();
    constexpr auto driving = []()
    {
        auto bindings = modern;
        for (auto action : { Forwards, Backwards, Left, Right, WalkRun, Crouch, Jump, NextWeapon, PreviousWeapon,
            Inventory, HideWeapon, DropWeapon, ToggleWalk, Reload, SniperAim, Objectives }) bindings[action] = {};
        // The car uses its own triggers. Drive-by fire/reload remain separate
        // native weapon actions rather than also firing while accelerating.
        bindings[Fire] = { 0, 3, 0x105, 0 };
        bindings[Reload] = { 0, 3, 0x10B, 0 };
        return bindings;
    }();
    uintptr_t profileBindings = 0, labelUpdate = 0;
    struct MenuPad
    {
        XINPUT_STATE state{};
        ULONGLONG nextProbe = 0;
        bool connected = false;
        bool active = false;
    };
    std::array<MenuPad, XUSER_MAX_COUNT> menuPads{};
    DWORD menuSlot = XUSER_MAX_COUNT;
    bool waitForNeutral = true;
    uint32_t previousButtons = 0;
    int previousDirection = 0;
    DWORD repeatTick = 0, pauseTick = 0;
    bool ready = false, pauseRequested = false;
    // The device the player used last: pad, or keyboard/mouse (the mouse only after real movement)
    bool padLastUsed = false;
    SafetyHookMid pauseHook, racePauseHook;
    SafetyHookInline legacyReadKey;
    uintptr_t* legacyStack;
    std::atomic<bool> resetRequested{ false };
    std::atomic<bool> modernLayout{ false }, invertCameraY{ false };
    std::atomic<float> lookSpeed{ 1.0f };
    void RefreshNames()
    {
        if (!ready || !labelUpdate) return;
        auto* page = MafiaMenu::Top();
        if (!page) return;
        for (auto** entry = page->begin; entry < page->end; ++entry)
        {
            auto* control = *entry;
            if (!control || control->table[13] != labelUpdate || control->textKey >= ActionCount) continue;
            reinterpret_cast<void(__thiscall*)(MafiaMenu::Control*)>(labelUpdate)(control);
        }
    }
    void ResetPending()
    {
        if (!resetRequested.exchange(false)) return;
        pauseRequested = false;
        MafiaMenu::padKey = 0;
        previousButtons = UINT32_MAX;
        previousDirection = 0;
        ModernPadActions.fill(false);
        RefreshNames();
    }

    bool Enabled()
    {
        return profileBindings && modernLayout.load(std::memory_order_relaxed);
    }
    const Binding* ForBinding(uintptr_t binding)
    {
        if (!Enabled() || binding < profileBindings || binding >= profileBindings + ActionCount * sizeof(Binding) ||
            (binding - profileBindings) % sizeof(Binding)) return nullptr;
        const auto& mapped = (GameplayDriving ? driving : modern)[(binding - profileBindings) / sizeof(Binding)];
        return mapped.type ? &mapped : nullptr;
    }
    // The pad binding of an action in the current layout, or nullptr
    const Binding* PadBinding(size_t action)
    {
        if (!profileBindings || action >= ActionCount) return nullptr;
        if (Enabled()) return modern[action].type ? &modern[action] : nullptr;
        const auto* binding = reinterpret_cast<const Binding*>(profileBindings) + action;
        return binding->type == 3 ? binding : nullptr;
    }
    bool ConsumePause()
    {
        return std::exchange(pauseRequested, false) && ready && GetTickCount() - pauseTick <= 250 &&
            !(MafiaMenu::blocksInput && MafiaMenu::blocksInput());
    }
    int __stdcall LegacyKey(void* driver)
    {
        const int nativeKey = legacyReadKey.stdcall<int>(driver);
        const int padKey = std::exchange(MafiaMenu::padKey, 0);
        if (nativeKey || !legacyStack)
            return nativeKey;
        if (*legacyStack)
            return padKey;
        return ConsumePause() ? 1 : 0;
    }
    void Install()
    {
        auto profile = hook::pattern("8D 0C 40 8D 14 8D ? ? ? ? B9 ? ? ? ? 52 E8 ? ? ? ? 83 C9 FF 85 C0 74 28 8B F8 33 C0 F2 AE F7 D1 2B F9 56 8B F7");
        const auto profileSite = reinterpret_cast<uintptr_t>(profile.get_first());
        profileBindings = Field<uintptr_t>(profileSite, 6);
        auto pause = hook::pattern("80 7C 24 ? 01 0F 85 ? ? ? ? 8B 0D ? ? ? ? 8B 49 24 E8 ? ? ? ? 6A 30");
        if (!pause.empty()) // 1.1/1.2 controller layout
        {
            static uintptr_t resume = reinterpret_cast<uintptr_t>(pause.get_first()) + 11;
            racePauseHook = safetyhook::create_mid(pause.get_first(5), [](SafetyHookContext& context)
            {
                if (ConsumePause()) context.eip = resume;
            });
            auto gameplay = hook::pattern("80 FB 01 0F 85 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 84 C0 74 ? A0 ? ? ? ? 84 C0 0F 85");
            pauseHook = safetyhook::create_mid(gameplay.get_first(), [](SafetyHookContext& context)
            {
                if (ConsumePause()) context.ebx = (context.ebx & ~0xFFu) | 1u;
            });
            auto labels = hook::pattern("53 55 8B E9 8B 45 0C 8D 5D 40 8D 4C 40 7E C6 03 00 66 83 3C 8D ? ? ? ? 03");
            labelUpdate = reinterpret_cast<uintptr_t>(labels.get_first());
        }
        else // 1.0 reads the native scan code directly in its menu and gameplay loops.
        {
            legacyStack = LegacyMenuStack();
            auto key = hook::module_pattern(GetLS3DF(), "F6 05 ? ? ? ? 02 56 74 ? 8B 0D ? ? ? ? A1 ? ? ? ? C7 05");
            legacyReadKey = safetyhook::create_inline(key.get_first(), LegacyKey);
        }
        ready = legacyStack || MafiaMenu::ready;
        const auto update = []()
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            auto& settings = FixSettings();
            const bool useModernLayout = settings.Get("MAIN.ControllerLayout") == "1";
            if (modernLayout.exchange(useModernLayout) != useModernLayout)
                resetRequested.store(true);
            invertCameraY.store(settings.Get("MAIN.InvertGamepadY") == "1");
            float speed;
            lookSpeed.store(asi_settings::detail::ParseFloat(settings.Get("MAIN.GamepadCameraSpeed"), speed) && std::isfinite(speed)
                ? std::clamp(speed, 0.25f, 3.0f) : 1.0f);
        };
        for (const char* id : { "MAIN.ControllerLayout", "MAIN.InvertGamepadY", "MAIN.GamepadCameraSpeed" })
            FixSettings().OnChange(id, update);
        update();
    }
    DWORD ReadMenuPads(decltype(&XInputGetState) getState, ULONGLONG now)
    {
        DWORD pressed = XUSER_MAX_COUNT, idle = XUSER_MAX_COUNT;
        for (DWORD i = 0; i < menuPads.size(); ++i)
        {
            auto& slot = menuPads[i];
            if (!slot.connected && now < slot.nextProbe) continue;
            if (!getState || getState(i, &slot.state) != ERROR_SUCCESS)
            {
                slot.connected = slot.active = false;
                slot.nextProbe = now + 250;
                continue;
            }
            const auto& pad = slot.state.Gamepad;
            const bool active = pad.wButtons || pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
                pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
                std::abs(pad.sThumbLX) >= 19660 || std::abs(pad.sThumbLY) >= 19660 ||
                std::abs(pad.sThumbRX) >= 19660 || std::abs(pad.sThumbRY) >= 19660;
            // A held control on a newly connected or non-owning pad must be released before handoff.
            if (active && slot.connected && !slot.active && pressed == XUSER_MAX_COUNT) pressed = i;
            if (!active && idle == XUSER_MAX_COUNT) idle = i;
            slot.active = active;
            slot.connected = true;
        }
        if (menuSlot < menuPads.size() && menuPads[menuSlot].connected)
            return menuPads[menuSlot].active || pressed == XUSER_MAX_COUNT ? menuSlot : pressed;
        return pressed != XUSER_MAX_COUNT ? pressed : idle;
    }
    void Frame(HWND window)
    {
        ResetPending();
        // Menu navigation and Start pause work with either layout
        if (!ready) return;
        const DWORD active = ReadMenuPads(PadInput::XInput().getState, GetTickCount64());
        if (active != menuSlot)
        {
            menuSlot = active;
            previousButtons = 0;
            previousDirection = 0;
            pauseRequested = false;
            MafiaMenu::padKey = 0;
        }
        if (active == XUSER_MAX_COUNT)
        {
            waitForNeutral = true;
            return;
        }
        const auto& pad = menuPads[active].state.Gamepad;
        const uint32_t buttons = pad.wButtons;
        constexpr int threshold = 19660;
        int direction = 0;
        if (buttons & XINPUT_GAMEPAD_DPAD_UP) direction = 200;
        else if (buttons & XINPUT_GAMEPAD_DPAD_DOWN) direction = 208;
        else if (buttons & XINPUT_GAMEPAD_DPAD_LEFT) direction = 203;
        else if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) direction = 205;
        else if (std::abs(pad.sThumbLY) >= threshold) direction = pad.sThumbLY > 0 ? 200 : 208;
        else if (std::abs(pad.sThumbLX) >= threshold) direction = pad.sThumbLX < 0 ? 203 : 205;
        const uint32_t before = std::exchange(previousButtons, buttons);
        const uint32_t pressed = buttons & ~before;
        if (!window || GetForegroundWindow() != window || (MafiaMenu::blocksInput && MafiaMenu::blocksInput()))
        {
            previousDirection = 0;
            pauseRequested = false;
            MafiaMenu::padKey = 0;
            waitForNeutral = true;
            return;
        }
        if (waitForNeutral)
        {
            waitForNeutral = direction != 0 || (buttons & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_START));
            return;
        }
        if (pressed) padLastUsed = true;
        if (legacyStack ? !*legacyStack : !MafiaMenu::Top())
        {
            if (pressed & XINPUT_GAMEPAD_START) { pauseRequested = true; pauseTick = GetTickCount(); }
            return;
        }
        pauseRequested = false;
        int key = 0;
        if (pressed & (XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_START)) key = 1;
        else if (pressed & XINPUT_GAMEPAD_A) key = 28;
        else if (pressed & XINPUT_GAMEPAD_Y) key = 59;
        const auto now = GetTickCount();
        if (direction && (direction != previousDirection || static_cast<int32_t>(now - repeatTick) >= 0))
        {
            if (!key) key = direction;
            repeatTick = now + (direction == previousDirection ? 100 : 350);
        }
        previousDirection = direction;
        if (key)
        {
            MafiaMenu::padKey = key;
            padLastUsed = true;
        }
    }

    bool Active(size_t action, float value, bool pad)
    {
        if (!std::isfinite(value)) return false;
        const bool look = action == LookX || action == LookY;
        return std::abs(value) > (pad ? 0.3f : look ? 0.1f : 0.5f);
    }
    // Original layout: each action has one binding, so its type tells the device
    void TrackDevice(uintptr_t object)
    {
        const auto& bindings = Field<Vector>(object, InputBindingsOffset);
        const auto& values = Field<Vector>(object, InputBindingsOffset + 16);
        if (!bindings.begin || !values.begin || bindings.end - bindings.begin != ActionCount * sizeof(Binding) ||
            values.end - values.begin != ActionCount * sizeof(float)) return;
        const auto* binding = reinterpret_cast<const Binding*>(bindings.begin);
        const auto* value = reinterpret_cast<const float*>(values.begin);
        for (size_t index = 0; index < ActionCount; ++index)
        {
            if (!binding[index].type) continue;
            const bool pad = binding[index].type == 3;
            if (Active(index, value[index], pad)) padLastUsed = pad;
        }
    }

    template<class Read> uint16_t Update(uintptr_t object, Read&& read)
    {
        ResetPending();
        ModernPadActions.fill(false);
        if (!Enabled())
        {
            const auto result = read();
            TrackDevice(object);
            return result;
        }
        const size_t bindingsOffset = InputBindingsOffset;
        const size_t valuesOffset = bindingsOffset + 16;
        auto& bindings = Field<Vector>(object, bindingsOffset);
        auto& values = Field<Vector>(object, valuesOffset);
        auto& pressed = Field<Vector>(object, valuesOffset + 16);
        const auto valid = [](const Vector& vector, size_t bytes)
        {
            return vector.begin && vector.end >= vector.begin && vector.end - vector.begin == bytes && vector.capacity >= vector.end;
        };
        if (!valid(bindings, ActionCount * sizeof(Binding)) || !valid(values, ActionCount * sizeof(float)) ||
            !valid(pressed, ActionCount * sizeof(float))) return read();
        std::array<Binding, ActionCount * 2> extra{};
        std::array<float, ActionCount * 2> state{}, events{};
        std::copy_n(reinterpret_cast<const Binding*>(bindings.begin), ActionCount, extra.begin());
        // Modern replaces only the native pad side of the live lookup. The
        // player's original table is restored before any profile/UI consumer.
        for (size_t index = 0; index < ActionCount; ++index)
            if (extra[index].type == 3) extra[index] = {};
        const auto& layout = GameplayDriving ? driving : modern;
        std::copy(layout.begin(), layout.end(), extra.begin() + ActionCount);
        const auto savedBindings = bindings, savedValues = values, savedPressed = pressed;
        struct Restore
        {
            Vector& binding; Vector& value; Vector& event;
            Vector oldBinding, oldValue, oldEvent;
            ~Restore() { binding = oldBinding; value = oldValue; event = oldEvent; }
        } restore{ bindings, values, pressed, savedBindings, savedValues, savedPressed };
        const auto temporary = [](auto& array)
        {
            auto first = reinterpret_cast<uintptr_t>(array.data());
            return Vector{ first, first + sizeof(array), first + sizeof(array) };
        };
        bindings = temporary(extra); values = temporary(state); pressed = temporary(events);
        const auto result = read();
        const float cameraSpeed = lookSpeed.load(std::memory_order_relaxed);
        const bool invertY = invertCameraY.load(std::memory_order_relaxed);
        for (size_t index = 0; index < ActionCount; ++index)
        {
            const auto strongest = [](float native, float pad)
            {
                return std::isfinite(pad) && std::abs(pad) > std::abs(native) ? pad : native;
            };
            const bool look = index == LookX || index == LookY;
            const float scale = look ? cameraSpeed * (index == LookY && invertY ? -1.0f : 1.0f) : 1.0f;
            const float padValue = state[index + ActionCount] * scale;
            const float padEvent = events[index + ActionCount] * scale;
            reinterpret_cast<float*>(savedValues.begin)[index] = strongest(state[index], padValue);
            reinterpret_cast<float*>(savedPressed.begin)[index] = strongest(events[index], padEvent);
            ModernPadActions[index] = std::isfinite(padValue) && std::abs(padValue) > 0.001f &&
                std::abs(padValue) >= std::abs(state[index]);
            if (Active(index, state[index + ActionCount], true)) padLastUsed = true;
            else if (Active(index, state[index], false)) padLastUsed = false;
        }
        return result;
    }
}

// Stick deadzones in IJoy.dll
export void InitIJoy()
{
    static float fLeftStickDeadzone = FixSettings().Float("MAIN.LeftStickDeadzone") / 100.0f;
    static float fRightStickDeadzone = FixSettings().Float("MAIN.RightStickDeadzone") / 100.0f;
    static constexpr auto pad_max = 10000.0f; //[ -10000 0 10000 ]
    fLeftStickDeadzone = std::isfinite(fLeftStickDeadzone) ? std::clamp(fLeftStickDeadzone, 0.0f, 1.0f) : 0.1f;
    fRightStickDeadzone = std::isfinite(fRightStickDeadzone) ? std::clamp(fRightStickDeadzone, 0.0f, 1.0f) : 0.1f;

    if (fLeftStickDeadzone)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"IJoy"), "DB 06 F6 C1 04 D8 85");
        struct LSHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float x = static_cast<float>(*(int32_t*)(regs.esi + 0x00)) / pad_max;
                float y = static_cast<float>(*(int32_t*)(regs.esi + 0x04)) / pad_max;
                float f = *(float*)(regs.ebp + 0xF38);

                auto len = sqrtf(x * x + y * y);
                if (fLeftStickDeadzone >= 1.0f || len <= fLeftStickDeadzone)
                {
                    x = 0.0f;
                    y = 0.0f;
                }
                else
                {
                    auto scale = std::min(1.0f, (len - fLeftStickDeadzone) / (1.0f - fLeftStickDeadzone)) / len;
                    x *= scale * pad_max;
                    y *= scale * pad_max;
                };

                *(int32_t*)(regs.esi + 0x00) = static_cast<int32_t>(x);
                *(int32_t*)(regs.esi + 0x04) = static_cast<int32_t>(y);

                _asm {fld  dword ptr[x]}
                _asm {fadd dword ptr[f]}
            }
        }; injector::MakeInline<LSHook>(pattern.get_first(0), pattern.get_first(11)); //0x10002F71
        injector::WriteMemory(pattern.get_first(5), 0x9004C1F6, true); //test cl, 4
    }

    if (fRightStickDeadzone)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"IJoy"), "DB 45 2C F6 C1 04 D8 85");
        struct RSX
        {
            void operator()(injector::reg_pack& regs)
            {
                float x = static_cast<float>(*(int32_t*)(regs.ebp + 0x2C)) / pad_max;
                float y = static_cast<float>(*(int32_t*)(regs.ebp + 0x30)) / pad_max;
                float f = *(float*)(regs.ebp + 0xF38);

                auto len = sqrtf(x * x + y * y);
                if (fRightStickDeadzone >= 1.0f || len <= fRightStickDeadzone)
                {
                    x = 0.0f;
                    y = 0.0f;
                }
                else
                {
                    auto scale = std::min(1.0f, (len - fRightStickDeadzone) / (1.0f - fRightStickDeadzone)) / len;
                    x *= scale * pad_max;
                    y *= scale * pad_max;
                };

                *(int32_t*)(regs.ebp + 0x2C) = static_cast<int32_t>(x);
                *(int32_t*)(regs.ebp + 0x30) = static_cast<int32_t>(y);

                _asm {fld  dword ptr[x]}
                _asm {fadd dword ptr[f]}
            }
        }; injector::MakeInline<RSX>(pattern.get_first(0), pattern.get_first(12));
        injector::WriteMemory(pattern.get_first(5), 0x9004C1F6, true); //test cl, 4
    }
}
