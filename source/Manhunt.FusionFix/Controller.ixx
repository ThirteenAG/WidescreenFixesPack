module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include <atomic>

export module Controller;

import NativeMenuView;


import ComVars;

import NativeMenu;

export namespace MHPad
{
    enum Button : uint32_t
    {
        A = 1 << 0, B = 1 << 1, X = 1 << 2, Y = 1 << 3,
        LB = 1 << 4, RB = 1 << 5, LT = 1 << 6, RT = 1 << 7,
        Back = 1 << 8, Start = 1 << 9, LS = 1 << 10, RS = 1 << 11,
        Up = 1 << 12, Right = 1 << 13, Down = 1 << 14, Left = 1 << 15
    };

    inline SafetyHookInline inputHook, actionHook, nameHook;
    inline SafetyHookMid nativeBindingsHook;
    inline std::array<SafetyHookInline, 6> menuHooks;
    inline uint32_t buttons = 0, previous = 0;
    inline std::array<float, 2> leftStick{};
    inline std::atomic<bool> lastPad{ false };
    inline bool menuWasActive = false;
    inline uint64_t frame = 0, commandFrame = UINT64_MAX;
    inline std::array<bool, 6> commands{}, held{};
    inline std::array<ULONGLONG, 6> repeat{};
    inline const int* modern = nullptr;
    inline const int* family = nullptr;
    inline const float* cameraSpeed = nullptr;
    inline GameRef<int32_t>* inputEnabledRef = nullptr;
    inline GameRef<int32_t>* mouseModeRef = nullptr;
    inline GameRef<uint32_t>* nativePadRef = nullptr;
    inline int(__cdecl* menuBlocked)() = nullptr;
    inline const wchar_t* (*iconLabel)(uint32_t) = nullptr;
    inline const wchar_t* (*movementLabel)() = nullptr;

    inline uint32_t ActionButton(int action)
    {
        switch (action)
        {
            case 0: return A;
            case 1: return RS;
            case 2: return X;
            case 4: return Y;
            case 6: return RT;
            case 8: return LT;
            case 18: return LB;
            case 19: return RB;
            case 20: return Start;
            case 23: return LS;
            case 24: return B;
            case 31: return Back;
            case 36: return Up;
            case 37: return Down;
            default: return 0;
        }
    }

    inline bool Active()
    {
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        return process == GetCurrentProcessId() && inputEnabledRef && inputEnabledRef->get() && modern && *modern;
    }

    inline void UpdateNativeMode()
    {
        if (!modern || !*modern)
            return;
        if (nativePadRef && (!lastPad || !Active()))
            injector::WriteMemory<uint32_t>(nativePadRef->get_ptr(), nativePadRef->get() & ~0xCFF00000u, true);
        if (lastPad && mouseModeRef)
            injector::WriteMemory<int32_t>(mouseModeRef->get_ptr(), 0, true);
    }

    inline void __cdecl Input(int event, void* data)
    {
        if (event == 18)
        {
            previous = buttons;
            buttons = 0;
            leftStick = {};
            ++frame;
        }
        else if (data && event == 2)
        {
            buttons |= static_cast<const uint32_t*>(data)[1];
            if (buttons & ~previous)
                lastPad = true;
        }
        else if (data && (event == 0 || event == 1))
        {
            const auto axes = static_cast<const float*>(data);
            if (std::isfinite(axes[0]) && std::isfinite(axes[1]))
            {
                if (event == 0)
                    leftStick = { axes[0], axes[1] };
                if (std::fabs(axes[0]) > 0.2f || std::fabs(axes[1]) > 0.2f)
                    lastPad = true;
                // A centered native joystick must not hide the keyboard or mouse path.
                if (modern && *modern && (!lastPad || !Active()))
                {
                    UpdateNativeMode();
                    return;
                }
                if (event == 1 && cameraSpeed && Active())
                {
                    const float speed = std::clamp(*cameraSpeed, 0.1f, 5.0f);
                    const float scaled[] = { axes[0] * speed, axes[1] * speed };
                    inputHook.unsafe_ccall(event, scaled);
                    UpdateNativeMode();
                    return;
                }
            }
        }
        else if (event == 16 || event == 3 || event == 5 || event == 7 || event == 9 || event == 11)
            lastPad = false;
        else if (data && event == 13)
        {
            const auto mouse = static_cast<const float*>(data);
            if (mouse[2] != 0.0f || mouse[3] != 0.0f)
                lastPad = false;
        }
        inputHook.unsafe_ccall(event, data);
        UpdateNativeMode();
    }

    inline int __cdecl Action(int action, int continuous)
    {
        const int native = actionHook.unsafe_ccall<int>(action, continuous);
        if (native || !Active())
            return native;
        const uint32_t mask = ActionButton(action);
        return ((continuous ? buttons : buttons & ~previous) & mask) != 0;
    }

    inline void BuildCommands()
    {
        if (!Active() || (menuBlocked && menuBlocked()))
        {
            commands = {};
            held = {};
            repeat = {};
            menuWasActive = false;
            commandFrame = UINT64_MAX;
            return;
        }
        if (commandFrame == frame)
            return;
        commandFrame = frame;
        commands = {};
        const bool now[] =
        {
            (buttons & Up) != 0 || leftStick[1] < -0.5f,
            (buttons & Down) != 0 || leftStick[1] > 0.5f,
            (buttons & Left) != 0 || leftStick[0] < -0.5f,
            (buttons & Right) != 0 || leftStick[0] > 0.5f,
            (buttons & A) != 0,
            (buttons & B) != 0
        };
        const auto time = GetTickCount64();
        if (!menuWasActive)
        {
            std::copy(std::begin(now), std::end(now), held.begin());
            repeat.fill(time + 350);
            menuWasActive = true;
            return;
        }
        for (size_t i = 0; i < commands.size(); ++i)
        {
            if (now[i] && !held[i])
            {
                commands[i] = true;
                repeat[i] = time + 350;
            }
            else if (now[i] && i < 4 && time >= repeat[i])
            {
                commands[i] = true;
                repeat[i] = time + 100;
            }
            held[i] = now[i];
        }
    }

    template<size_t Index> int __cdecl Menu()
    {
        const int native = menuHooks[Index].unsafe_ccall<int>();
        BuildCommands();
        return native || commands[Index];
    }

    inline const wchar_t* Label(uint32_t button)
    {
        const int selected = family ? *family : 0;
        switch (button)
        {
            case A: return selected == 2 ? L"[CROSS]" : selected == 3 ? L"[B]" : L"[A]";
            case B: return selected == 2 ? L"[CIRCLE]" : selected == 3 ? L"[A]" : L"[B]";
            case X: return selected == 2 ? L"[SQUARE]" : selected == 3 ? L"[Y]" : L"[X]";
            case Y: return selected == 2 ? L"[TRIANGLE]" : selected == 3 ? L"[X]" : L"[Y]";
            case LB: return selected == 2 ? L"[L1]" : selected == 3 ? L"[L]" : L"[LB]";
            case RB: return selected == 2 ? L"[R1]" : selected == 3 ? L"[R]" : L"[RB]";
            case LT: return selected == 2 ? L"[L2]" : selected == 3 ? L"[ZL]" : L"[LT]";
            case RT: return selected == 2 ? L"[R2]" : selected == 3 ? L"[ZR]" : L"[RT]";
            case Back: return selected == 2 ? L"[SHARE]" : selected == 3 ? L"[-]" : L"[VIEW]";
            case Start: return selected == 2 ? L"[OPTIONS]" : selected == 3 ? L"[+]" : L"[MENU]";
            case LS: return selected == 2 ? L"[L3]" : L"[LS]";
            case RS: return selected == 2 ? L"[R3]" : L"[RS]";
            case Up: return L"[UP]";
            case Down: return L"[DOWN]";
            default: return nullptr;
        }
    }

    inline const wchar_t* __cdecl BindingName(int action, int slot)
    {
        if (modern && *modern && lastPad && slot == 1)
        {
            if (action >= 14 && action <= 17)
            {
                if (movementLabel)
                    if (auto icon = movementLabel())
                        return icon;
                return L"[LS]";
            }
            if (action >= 32 && action <= 35)
                return L"-";
            if (iconLabel)
                if (auto icon = iconLabel(ActionButton(action)))
                    return icon;
            if (auto name = Label(ActionButton(action)))
                return name;
        }
        return nameHook.unsafe_ccall<const wchar_t*>(action, slot);
    }

    inline void Install()
    {
        auto input = hook::pattern("53 83 EC 04 83 3D ? ? ? ? 00 8B 5C 24 10 74 05 E8 ? ? ? ? 8B 44 24 0C 83 F8 12");
        auto action = hook::pattern("53 56 57 55 83 EC 2C 31 ED 83 3D ? ? ? ? 00 0F 94 C1 8B 5C 24 40");
        auto name = hook::pattern("53 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 8B 54 24 0C 6B D2 26 03 54 24 08 8B 1C 95");
        auto bindings = hook::pattern("8B 34 9D ? ? ? ? 85 F6 74 50 83 7C 24 44 00");
        auto blocked = hook::pattern("83 3D ? ? ? ? 00 75 1F 83 3D ? ? ? ? 1E 7C 04 31 C9 EB 05");
        auto mouse = hook::pattern("A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 83 3D ? ? ? ? 00 74 17 E8");
        auto analog = hook::pattern("81 0D ? ? ? ? 00 00 00 40 D9 05 ? ? ? ? D8 1D");
        if (!MHMenu::Storage() || !MHMenu::Storage()->enabled)
        {
            FixSettings().Availability("MAIN.ControllerLayout", [](std::string_view value) { return value == "0"; });
            return;
        }
        inputEnabledRef = BindGameRef(inputEnabled, *input.get_first<int32_t*>(6));
        mouseModeRef = BindGameRef(mouseMode, *mouse.get_first<int32_t*>(1));
        nativePadRef = BindGameRef(nativePad, *analog.get_first<uint32_t*>(2));
        menuBlocked = reinterpret_cast<int(__cdecl*)()>(blocked.get_first());
        modern = &FixSettings().Int("MAIN.ControllerLayout");
        family = &FixSettings().Int("MAIN.GamepadIcons");
        cameraSpeed = &FixSettings().Float("MAIN.GamepadCameraSpeed");
        nativeBindingsHook = safetyhook::create_mid(bindings.get_first(7), [](SafetyHookContext& regs)
        {
            // Modern replaces only controller assignments in the native action lookup.
            if (modern && *modern)
                regs.esi = 0;
        });
        inputHook = safetyhook::create_inline(input.get_first(), Input);
        actionHook = safetyhook::create_inline(action.get_first(), Action);
        nameHook = safetyhook::create_inline(name.get_first(), BindingName);
        constexpr std::array<std::string_view, 6> signatures
        {
            "53 31 DB E8 A8 03 00 00 85 C0 74 04 31 C0 5B C3 E8 3B 03 00 00 85 C0 74",
            "53 31 DB E8 28 03 00 00 85 C0 74 04 31 C0 5B C3 E8 AB 02 00 00 85 C0 74",
            "53 31 DB E8 08 05 00 00 85 C0 74 04 31 C0 5B C3 E8 BB 04 00 00 85 C0 74",
            "53 31 DB E8 58 04 00 00 85 C0 74 04 31 C0 5B C3 E8 FB 03 00 00 85 C0 74",
            "53 56 55 31 DB E8 ? ? ? ? 85 C0 74 06 31 C0 5D 5E 5B C3 83 3D",
            "53 55 31 ED 31 DB E8 ? ? ? ? 85 C0 74 05 31 C0 5D 5B C3 83 3D"
        };
        constexpr std::array callbacks{ &Menu<0>, &Menu<1>, &Menu<2>, &Menu<3>, &Menu<4>, &Menu<5> };
        for (size_t i = 0; i < signatures.size(); ++i)
        {
            menuHooks[i] = safetyhook::create_inline(hook::get_pattern(signatures[i]), callbacks[i]);
        }
    }
}
