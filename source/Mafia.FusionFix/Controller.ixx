module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>

export module Controller;

import ComVars;
import NativeMenu;

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
        button(Accelerate, 7); button(Brake, 6); axis(SteerLeft, 0, 2); axis(SteerRight, 0, 1);
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
    uintptr_t profileBindings = 0, inputObject = 0, cacheGetter = 0, labelUpdate = 0;
    uint32_t previousButtons = 0;
    int previousDirection = 0;
    DWORD repeatTick = 0, pauseTick = 0;
    bool ready = false, pauseRequested = false, padActive = false;
    SafetyHookMid pauseHook;
    std::atomic<bool> resetRequested{ false };
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
        return ready && FixSettings().Int("MAIN.ControllerLayout") == 1;
    }
    const Binding* ForBinding(uintptr_t binding)
    {
        if (!Enabled() || binding < profileBindings || binding >= profileBindings + ActionCount * sizeof(Binding) ||
            (binding - profileBindings) % sizeof(Binding)) return nullptr;
        const auto& mapped = (GameplayDriving ? driving : modern)[(binding - profileBindings) / sizeof(Binding)];
        return mapped.type ? &mapped : nullptr;
    }
    void Install()
    {
        auto profile = hook::pattern("8D 0C 40 8D 14 8D ? ? ? ? B9 ? ? ? ? 52 E8 ? ? ? ? 83 C9 FF 85 C0 74 28 8B F8 33 C0 F2 AE F7 D1 2B F9 56 8B F7");
        const auto profileSite = reinterpret_cast<uintptr_t>(profile.get_first());
        profileBindings = Field<uintptr_t>(profileSite, 6);
        inputObject = Field<uintptr_t>(profileSite, 11);
        auto pause = hook::pattern("80 7C 24 ? 01 0F 85 ? ? ? ? 8B 0D ? ? ? ? 8B 49 24 E8 ? ? ? ? 6A 30");
        if (!pause.empty()) // 1.1/1.2 controller layout
        {
            static uintptr_t resume = reinterpret_cast<uintptr_t>(pause.get_first()) + 11;
            pauseHook = safetyhook::create_mid(pause.get_first(5), [](SafetyHookContext& context)
            {
                const bool requested = std::exchange(pauseRequested, false);
                const bool blocked = MafiaMenu::blocksInput && MafiaMenu::blocksInput();
                if (Enabled() && !blocked && requested && GetTickCount() - pauseTick <= 250) context.eip = resume;
            });
            auto labels = hook::pattern("53 55 8B E9 8B 45 0C 8D 5D 40 8D 4C 40 7E C6 03 00 66 83 3C 8D ? ? ? ? 03");
            labelUpdate = reinterpret_cast<uintptr_t>(labels.get_first());
        }
        else // 1.0 has no pause or label routine
        {
            auto legacy = hook::pattern("F7 85 A4 90 00 00 00 00 00 02 74 ? 39 BD 4C 94 00 00");
            legacy.get_first();
        }
        ready = MafiaMenu::ready && profileBindings && inputObject && static_cast<bool>(pauseHook);
        if (!ready) OutputDebugStringA("Mafia: modern native controller contracts unavailable.\n");
        FixSettings().Availability("MAIN.ControllerLayout", [](std::string_view value) { return value == "0" || ready; });
        FixSettings().OnChange("MAIN.ControllerLayout", []() { resetRequested.store(true); });
    }
    void InitJoy()
    {
        auto getter = hook::module_pattern(GetModuleHandleW(L"IJoy.dll"),
            "57 8B 7C 24 08 85 FF 74 ? 56 8D B1 30 01 00 00 B9 11 00 00 00 B0 01 F3 A5 5E 5F C2 04 00");
        cacheGetter = reinterpret_cast<uintptr_t>(getter.get_first());
    }
    void Frame(HWND window)
    {
        ResetPending();
        if (!Enabled() || !cacheGetter || !inputObject) return;
        const auto first = Field<uintptr_t>(inputObject, 8), last = Field<uintptr_t>(inputObject, 12);
        if (!first || last < first + 140) return;
        const auto device = Field<uintptr_t>(first, 0);
        if (!device) return;
        const auto table = Field<uintptr_t>(device, 0);
        if (Field<uintptr_t>(table, 80) != cacheGetter) return;
        const auto state = device + 0x130;
        const uint32_t buttons = Field<uint32_t>(state, 52);
        const uint32_t pressed = buttons & ~std::exchange(previousButtons, buttons);
        if (!window || GetForegroundWindow() != window || (MafiaMenu::blocksInput && MafiaMenu::blocksInput()))
        {
            previousDirection = 0;
            pauseRequested = false;
            MafiaMenu::padKey = 0;
            return;
        }
        auto* page = MafiaMenu::Top();
        if (!page)
        {
            if (pressed & (1u << 9)) { pauseRequested = true; pauseTick = GetTickCount(); }
            return;
        }
        pauseRequested = false;
        int key = 0;
        if (pressed & ((1u << 1) | (1u << 9))) key = 1;
        else if (pressed & 1u) key = 28;
        else if (pressed & (1u << 3)) key = 59;
        const float x = Field<float>(state, 0), y = Field<float>(state, 4);
        int direction = std::abs(y) >= 0.6f ? (y < 0 ? 200 : 208) : std::abs(x) >= 0.6f ? (x < 0 ? 203 : 205) : 0;
        if (!Field<uint8_t>(state, 48))
        {
            const float angle = Field<float>(state, 32);
            if (std::isfinite(angle))
            {
                const int hat = static_cast<int>(std::lround(angle * (4.0 / 3.14159265358979323846))) & 7;
                direction = hat <= 1 || hat == 7 ? 200 : hat <= 3 ? 205 : hat <= 5 ? 208 : 203;
            }
        }
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
            padActive = true;
        }
    }

    template<class Read> uint16_t Update(uintptr_t object, Read&& read)
    {
        ResetPending();
        ModernPadActions.fill(false);
        if (!Enabled()) return read();
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
        const float requestedSpeed = FixSettings().Float("MAIN.GamepadCameraSpeed");
        const float cameraSpeed = std::isfinite(requestedSpeed) ? std::clamp(requestedSpeed, 0.25f, 3.0f) : 1.0f;
        const bool invertY = FixSettings().Bool("MAIN.InvertGamepadY");
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
        }
        return result;
    }
}

// Stick deadzones in IJoy.dll
export void InitIJoy()
{
    MafiaPad::InitJoy();
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
