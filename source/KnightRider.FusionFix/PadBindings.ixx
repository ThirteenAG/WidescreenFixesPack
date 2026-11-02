module;
#include <stdafx.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0700
#endif
#include <dinput.h>
#include "FixSettings.hpp"

export module PadBindings;


export namespace KnightRiderPadBindings
{
    inline SafetyHookInline BindingHook, MenuHook, RangesHook, DeadzoneHook, NormalizeHook;
    inline void (__thiscall* Range)(void*, int, int*, int*) = nullptr;
    inline void* AcceleratorReturn = nullptr;
    inline bool (__thiscall* Exists)(void*, int, int) = nullptr;
    inline uint8_t* (__thiscall* Device)(void*, int, int) = nullptr;
    inline void* (__thiscall* Property)(void*, uint8_t**, int) = nullptr;
    inline int (__thiscall* Append)(void*, uint8_t*) = nullptr;

    inline int DefaultKey(int action)
    {
        switch (action)
        {
            case 0: return 16; // Pause: Start.
            case 2: return 15; // Camera: Back.
            case 3: return 3;  // Steering: native analog left-stick directions.
            case 4: return 4;
            case 5: return 5;  // Accelerate: positive native Y direction, supplied by RT.
            case 6: return 2;  // Brake: native Z axis, supplied by LT.
            case 7: return 17; // Reset: left-stick button.
            case 11: return 11;
            case 12: return 12;
            case 13: return 7;
            case 14: return 8;
            case 15: return 9;
            case 16: return 18;
            case 17: return 10;
            case 18: return 22;
            case 19: return 21;
            case 22: return 16; // Start/continue a mission.
        }
        return -1;
    }

    inline void __cdecl Binding(void* manager, int action, const char* name, int type, int key, int index)
    {
        BindingHook.ccall<void>(manager, action, name, type, key, index);
        const auto padKey = DefaultKey(action);
        if ((type & 0xFFFF) != 1 || padKey < 0 || !GetModuleHandleW(L"Xidi.32.dll"))
            return;
        // The engine owns all polling, analog values and action combination. These are additional native bindings.
        // Explicit controller bindings in the player's controls page remain authoritative.
        for (int pad = 0; pad < 4; ++pad)
            BindingHook.ccall<void>(manager, action, name, 3, padKey, pad);
    }

    inline void __fastcall Menu(uint8_t* menu, void*, uint8_t* frame)
    {
        MenuHook.thiscall<void>(menu, frame);
        if (!GetModuleHandleW(L"Xidi.32.dll"))
            return;
        const auto manager = *reinterpret_cast<uint8_t**>(frame + 8) + 124;
        const auto state = *reinterpret_cast<uint8_t**>(menu + 40);
        const auto navigation = state ? *reinterpret_cast<uint8_t**>(state + 4) : nullptr;
        if (!navigation)
            return;
        // Xidi supplies the d-pad as four extra native buttons. Its POV hat is not read by this game's input code.
        static constexpr int directions[4]{ 21, 22, 19, 20 };
        for (int pad = 0; pad < 4; ++pad)
        {
            if (!Exists(manager, 3, pad))
                continue;
            const auto device = Device(manager, 3, pad);
            for (size_t direction = 0; direction < std::size(directions); ++direction)
            {
                const auto action = *reinterpret_cast<uint8_t**>(navigation + 168 + direction * 4);
                if (!action)
                    continue;
                uint8_t* property = nullptr;
                Property(device, &property, directions[direction]);
                if (property)
                    Append(action, property); // The native by-value smart pointer transfers this reference.
            }
        }
    }

    inline void __fastcall Ranges(uint8_t* device, void*)
    {
        RangesHook.thiscall<void>(device);
        if (GetModuleHandleW(L"Xidi.32.dll") && *reinterpret_cast<DWORD*>(device + 92) >= 3)
            Range(device, DIJOFS_Z, reinterpret_cast<int*>(device + 232), reinterpret_cast<int*>(device + 236));
    }

    inline bool __fastcall Deadzone(uint8_t* device, void*, float requested)
    {
        if (!GetModuleHandleW(L"Xidi.32.dll"))
            return DeadzoneHook.thiscall<bool>(device, requested);
        const auto result = DeadzoneHook.thiscall<bool>(device, 0.15f);
        // The left stick keeps a deadzone; the accelerator uses the entire trigger travel.
        const auto input = *reinterpret_cast<IDirectInputDevice7A**>(device + 72);
        DIPROPDWORD property{};
        property.diph.dwSize = sizeof(property);
        property.diph.dwHeaderSize = sizeof(property.diph);
        property.diph.dwHow = DIPH_BYOFFSET;
        property.diph.dwObj = DIJOFS_Y;
        if (input)
            input->SetProperty(DIPROP_DEADZONE, &property.diph);
        return result;
    }

    inline float __fastcall Normalize(void* device, void*, float value)
    {
        if (_ReturnAddress() == AcceleratorReturn && GetModuleHandleW(L"Xidi.32.dll"))
            return std::clamp(value, -1.0f, 1.0f);
        return NormalizeHook.thiscall<float>(device, value);
    }

    inline void Init()
    {
        auto ranges = hook::pattern("83 EC 2C 53 8B D9 56 57 8D 73 50 B9 0B 00 00 00 8D 7C 24 0C F3 A5");
        RangesHook = safetyhook::create_inline(ranges.get_first(), Ranges);
        auto range = hook::pattern("83 EC 18 56 57 8B 7C 24 24 8B F1 C7 44 24 08 18 00 00 00");
        Range = reinterpret_cast<decltype(Range)>(range.get_first());
        auto deadzone = hook::pattern("83 EC 14 D9 44 24 18 D8 0D ? ? ? ? 56 8B F1 57 C7 44 24 08 14 00 00 00");
        DeadzoneHook = safetyhook::create_inline(deadzone.get_first(), Deadzone);
        auto normalize = hook::pattern("D9 05 ? ? ? ? D8 89 0C 01 00 00 D8 05 ? ? ? ? D8 4C 24 04");
        NormalizeHook = safetyhook::create_inline(normalize.get_first(), Normalize);
        auto accelerator = hook::pattern("8B 8E F4 00 00 00 D9 9E F0 00 00 00 51 8B CE E8 ? ? ? ? 8B 96 F0 00 00 00");
        AcceleratorReturn = accelerator.get_first(20);
        auto binding = hook::pattern("51 53 8B 5C 24 0C 56 57 8B 7C 24 18 8D 44 24 18 57 50 8B CB C7 44 24 14 00 00 00 00");
        BindingHook = safetyhook::create_inline(binding.get_first(), Binding);
        auto menu = hook::pattern("83 EC 20 8D 44 24 0C 53 55 56 57 8B F9 50 8B 4F 28 E8");
        MenuHook = safetyhook::create_inline(menu.get_first(), Menu);
        auto exists = hook::pattern("8B 44 24 08 8B 54 24 04 50 52 E8 ? ? ? ? 33 C9 83 F8 FF 0F 95 C1");
        Exists = reinterpret_cast<decltype(Exists)>(exists.get_first());
        auto device = hook::pattern("8B 44 24 08 56 8B F1 50 8B 4C 24 0C 51 8B CE E8 ? ? ? ? 8B 56 10");
        Device = reinterpret_cast<decltype(Device)>(device.get_first());
        auto property = hook::pattern("8B 51 20 53 55 56 33 F6 57 85 D2 7E 27 8B 79 1C 8B 5C 24 18 39 51 28 7C 07 C7 41 28 00 00 00 00 8B 41 28 8B 2C 87 39 5D 08 74");
        Property = reinterpret_cast<decltype(Property)>(property.get_first());
        auto append = hook::pattern("83 EC 0C 53 56 8B F1 57 8D 4C 24 0C E8 ? ? ? ? 8D 44 24 1C");
        Append = reinterpret_cast<decltype(Append)>(append.get_first());
        auto accept = hook::pattern("51 8B CE 8B C4 6A 09 50 E8 ? ? ? ? 8D 4C 24 30 51");
        injector::WriteMemory<uint8_t>(accept.get_first<uint8_t>(6), 7, true);
        auto back = hook::pattern("51 8B CE 8B C4 6A 07 50 E8 ? ? ? ? 8D 4C 24 18 E8");
        injector::WriteMemory<uint8_t>(back.get_first<uint8_t>(6), 8, true);
        auto alternateBack = hook::pattern("51 8B CC 6A 0A 51 8B CE E8 ? ? ? ? 8D 4C 24 14 E8");
        injector::WriteMemory<uint8_t>(alternateBack.get_first<uint8_t>(4), 8, true);
        auto slots = hook::pattern("45 83 FD 02 0F 8C ? ? ? ? 8B 4C 24 18 85 C9");
        injector::WriteMemory<uint8_t>(slots.get_first<uint8_t>(3), 4, true);
    }
}
