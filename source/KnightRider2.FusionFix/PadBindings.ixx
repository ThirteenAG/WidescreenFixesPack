module;
#include <stdafx.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0700
#endif
#include <dinput.h>
#include "FixSettings.hpp"

export module PadBindings;


export namespace KnightRider2PadBindings
{
    inline SafetyHookInline BindingHook, InitializeHook, DeadzoneHook, PropertyHook, StateHook;
    inline void (__thiscall* Range)(void*, int, int*, int*) = nullptr;
    inline uintptr_t MenuStart = 0;
    inline uintptr_t GenericMenuStart = 0;
    inline float ZeroGain = 0.0f;

    inline int DefaultKey(int action)
    {
        switch (action)
        {
            case 0: return 16;
            case 1: return 3;
            case 2: return 4;
            case 3: return 5;
            case 4: case 5: return 2;
            case 8: return 7;
            case 9: return 8;
            case 10: return 9;
            case 11: return 10;
            case 12: return 12;
            case 13: return 11;
            case 14: return 13;
            case 15: return 14;
            case 16: return 25;
            case 17: return 26;
            case 18: return 17;
            case 19: case 36: return 24;
            case 21: return 15;
            case 22: return 20;
            case 37: return 19;
            case 38: return 23;
            case 40: return 18;
            case 42: return 16;
        }
        return -1;
    }

    inline void __cdecl Binding(void* manager, int action, const char* name, int type, int key, int index)
    {
        BindingHook.ccall<void>(manager, action, name, type, key, index);
        const int padKey = DefaultKey(action);
        if ((type & 0xFFFF) != 1 || padKey < 0 || !GetModuleHandleW(L"Xidi.32.dll"))
            return;
        for (int pad = 0; pad < 4; ++pad)
            BindingHook.ccall<void>(manager, action, name, 3, padKey, pad);
    }

    inline bool __fastcall Initialize(uint8_t* device, void*, HINSTANCE instance, int window, int type, int slot)
    {
        const bool result = InitializeHook.thiscall<bool>(device, instance, window, type, slot);
        if (result && GetModuleHandleW(L"Xidi.32.dll"))
            Range(device, DIJOFS_Z, reinterpret_cast<int*>(device + 220), reinterpret_cast<int*>(device + 224));
        return result;
    }

    inline bool __fastcall Deadzone(uint8_t* device, void*, float value)
    {
        if (!GetModuleHandleW(L"Xidi.32.dll"))
            return DeadzoneHook.thiscall<bool>(device, value);
        const bool result = DeadzoneHook.thiscall<bool>(device, 0.15f);
        DIPROPDWORD property{};
        property.diph.dwSize = sizeof(property);
        property.diph.dwHeaderSize = sizeof(property.diph);
        property.diph.dwHow = DIPH_BYOFFSET;
        property.diph.dwObj = DIJOFS_Y;
        if (auto input = *reinterpret_cast<IDirectInputDevice7A**>(device + 60))
            input->SetProperty(DIPROP_DEADZONE, &property.diph);
        return result;
    }

    inline void* __fastcall Property(void* device, void*, void* output, int key)
    {
        const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
        const bool genericMenu = caller >= GenericMenuStart && caller < GenericMenuStart + 0x300;
        const bool menu = genericMenu || (caller >= MenuStart && caller < MenuStart + 0x700);
        if (GetModuleHandleW(L"Xidi.32.dll") && *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(device) + 8) == 3)
        {
            if (menu && key >= 3 && key <= 6)
            {
                static constexpr int directions[]{ 25, 26, 23, 24 };
                key = directions[key - 3];
            }
            else if (genericMenu && key >= 7 && key <= 9)
            {
                key = 7;
            }
        }
        return PropertyHook.thiscall<void*>(device, output, key);
    }

    inline int __fastcall State(uint8_t* device, void*)
    {
        const auto result = StateHook.thiscall<int>(device);
        if (GetModuleHandleW(L"Xidi.32.dll"))
        {
            // Complete the native POV path using its existing four spare DirectInput button properties.
            // The engine still polls the device and dispatches every menu/gameplay action.
            const auto angle = *reinterpret_cast<DWORD*>(device + 156);
            const bool active = LOWORD(angle) != 0xFFFF;
            auto buttons = device + 188;
            buttons[0] = active && (angle >= 31500 || angle <= 4500) ? 0x80 : 0;
            buttons[1] = active && angle >= 13500 && angle <= 22500 ? 0x80 : 0;
            buttons[2] = active && angle >= 22500 && angle <= 31500 ? 0x80 : 0;
            buttons[3] = active && angle >= 4500 && angle <= 13500 ? 0x80 : 0;
        }
        return result;
    }

    inline void Init()
    {
        auto binding = hook::pattern("51 53 8B 5C 24 0C 56 57 8B 7C 24 18 8D 44 24 18 57 50 8B CB C7 44 24 14 00 00 00 00");
        BindingHook = safetyhook::create_inline(binding.get_first(), Binding);
        auto initialize = hook::pattern("A1 ? ? ? ? 83 EC 58 85 C0 53 8B D9 75 48 8B 44 24 60 6A 00 68 ? ? ? ?");
        InitializeHook = safetyhook::create_inline(initialize.get_first(), Initialize);
        auto deadzone = hook::pattern("83 EC 18 D9 44 24 1C 56 D8 0D ? ? ? ? 57 8B F1 C7 44 24 0C 14 00 00 00");
        DeadzoneHook = safetyhook::create_inline(deadzone.get_first(), Deadzone);
        auto range = hook::pattern("83 EC 20 8B 44 24 24 53 56 8B F1 89 44 24 18 8B 46 3C 85 C0 BB 10 00 00 00");
        Range = reinterpret_cast<decltype(Range)>(range.get_first());
        auto property = hook::pattern("51 53 55 56 8B F1 0F B7 5E 20 33 ED 85 DB 57 7E 69 39 5E 24 7C 07 C7 46 24 00 00 00 00");
        PropertyHook = safetyhook::create_inline(property.get_first(), Property);
        auto menu = hook::pattern("83 EC 20 8D 44 24 0C 53 55 56 57 8B F9 50 8B 4F 24 E8 ? ? ? ? 8B 4C 24 1C");
        MenuStart = reinterpret_cast<uintptr_t>(menu.get_first());
        auto genericMenu = hook::pattern("53 55 56 57 8B F1 E8 ? ? ? ? 8B D8 33 C0 66 A1 ? ? ? ? 6A 00 83 C3 7C");
        GenericMenuStart = reinterpret_cast<uintptr_t>(genericMenu.get_first());
        auto state = hook::pattern("51 56 8B F1 8B 46 40 85 C0 57 75 33 68 ? ? ? ? 8D 4C 24 0C E8 ? ? ? ? 8D 44 24 08 50 68 18 01 00 00");
        StateHook = safetyhook::create_inline(state.get_first(), State);
        // Keep native steering sensitivity, while RT uses its complete analog travel.
        auto gain = hook::pattern("D9 05 ? ? ? ? D8 8E 00 01 00 00 D8 05 ? ? ? ? D8 4C 24 10 D8 15 ? ? ? ?");
        injector::WriteMemory<float*>(gain.get_first<float*>(2), &ZeroGain, true);
        auto accept = hook::pattern("51 8B CE 8B C4 6A 09 50 E8 ? ? ? ? 8D 4C 24 30 51");
        injector::WriteMemory<uint8_t>(accept.get_first<uint8_t>(6), 7, true);
        auto back = hook::pattern("51 8B CE 8B C4 6A 07 50 E8 ? ? ? ? 8D 4C 24 18 E8");
        injector::WriteMemory<uint8_t>(back.get_first<uint8_t>(6), 8, true);
        auto slots = hook::pattern("45 83 FD 02 0F 8C ? ? ? ? 8B 4C 24 18 85 C9");
        injector::WriteMemory<uint8_t>(slots.get_first<uint8_t>(3), 4, true);
    }
}
