module;

#include <stdafx.h>
#include <random>

export module Misc;

import ComVars;

// The game reads pads through DirectInput (the bundled Xidi). Its controller list knows only the pads of its time, so
// the Xidi pad gets the layout of the console game, and it is the primary controller until the player picks another.
namespace THPS3Pad
{
    struct Device
    {
        DWORD size;
        GUID instance, product;
        DWORD type;
    };

    SafetyHookInline MappingHook, EnumerateHook;
    SafetyHookMid SkaterHook, PreferencesHook;
    int FirstPad = -1;

    bool IsXidi(const GUID& product)
    {
        return (product.Data1 & 0xFFFFu) == 0xFFFFu && product.Data2 == 0 && product.Data3 == 0 &&
               product.Data4[0] == 'X' && product.Data4[1] == 'I' && product.Data4[2] == 'D' &&
               product.Data4[3] == 'I' && product.Data4[4] == 'V' && product.Data4[5] == 'C';
    }

    // One controller record: product GUID, name, profile count, current profile, 8 profiles of 136 bytes
    int __fastcall LoadMapping(uint8_t* record, void*)
    {
        const int result = MappingHook.thiscall<int>(record);
        if (result || !IsXidi(*reinterpret_cast<const GUID*>(record)))
            return result;
        std::memset(record + 16, 0, 32);
        std::memcpy(record + 16, "Xbox Controller", 16);
        *reinterpret_cast<int32_t*>(record + 48) = 1;
        *reinterpret_cast<int32_t*>(record + 52) = 0;
        auto* profile = record + 56;
        std::memset(profile, 0, 136);
        std::memcpy(profile, "Default", 8);
        // Left stick X/Y, right stick Z/RotZ
        constexpr int32_t sticks[10]{ 0, 65535, 0, 4, 0, 0, 65535, 8, 20, 0 };
        std::memcpy(profile + 32, sticks, sizeof(sticks));
        // Start, Select, L3, R3, Triangle, Circle, Cross, Square, L2, R2, L1, R1 on the pad's buttons (A 0, B 1, X 2,
        // Y 3, LB 4, RB 5, LT 6, RT 7, Back 8, Start 9, LS 10, RS 11); the D-pad is read as the POV
        constexpr int16_t buttons[12]{ 9, 8, 10, 11, 3, 1, 0, 2, 6, 7, 4, 5 };
        auto* entries = reinterpret_cast<int16_t*>(profile + 72);
        for (size_t i = 0; i < std::size(buttons); ++i)
        {
            entries[i * 2] = 1;
            entries[i * 2 + 1] = buttons[i];
        }
        *reinterpret_cast<int32_t*>(record + 1144) = 1;
        return 1;
    }

    BOOL __stdcall Enumerate(const Device* device, void* context)
    {
        const int index = DeviceCount.get();
        const auto result = EnumerateHook.stdcall<BOOL>(device, context);
        if (FirstPad < 0 && DeviceCount.get() != index && IsXidi(device->product))
        {
            FirstPad = index;
            // The skater profile may exist already, still on the keyboard (controller 0) with no controller chosen
            if (auto* skater = Skater.get(); skater && *reinterpret_cast<int32_t*>(skater + 0x34) == 0 &&
                *reinterpret_cast<int32_t*>(skater + 0x40) == -1)
                *reinterpret_cast<int32_t*>(skater + 0x34) = FirstPad;
        }
        return result;
    }

    void Install()
    {
        auto mapping = hook::pattern("83 EC 34 53 55 56 57 8B E9 E8 ? ? ? ? 8B F0");
        auto enumerate = hook::pattern("8B 44 24 04 8B 0D ? ? ? ? 83 C0 04 50 51 8B 4C 24 10 E8");
        auto profile = hook::pattern("89 5E 34 89 5E 38 C7 46 40 FF FF FF FF");
        auto preferences = hook::pattern("8D 46 3C 6A 00 50 68 ? ? ? ? E8 ? ? ? ? 57 8B CE E8");
        auto skater = hook::pattern("A1 ? ? ? ? 85 C0 75 3D 8A 4C 24 04 84 C9 74 35 6A 01 50 68 BC 04 00 00");
        DeviceCount.SetAddress(*enumerate.get_first<int32_t*>(6));
        Skater.SetAddress(*skater.get_first<uint8_t**>(1));
        MappingHook = safetyhook::create_inline(mapping.get_first(), LoadMapping);
        EnumerateHook = safetyhook::create_inline(enumerate.get_first(), Enumerate);
        // A new skater profile starts with the keyboard (controller 0) as its primary controller
        SkaterHook = safetyhook::create_mid(profile.get_first(6), [](SafetyHookContext& regs)
        {
            if (FirstPad > 0)
                *reinterpret_cast<int32_t*>(regs.esi + 0x34) = FirstPad;
        });
        // A save without a chosen controller keeps the pad as well
        PreferencesHook = safetyhook::create_mid(preferences.get_first(16), [](SafetyHookContext& regs)
        {
            if (FirstPad > 0 && regs.edi == 0 && *reinterpret_cast<int32_t*>(regs.esi + 0x40) == -1)
                regs.edi = FirstPad;
        });
    }
}

export void InstallMisc()
{
    hook::pattern pattern("");
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B 96 E8");
        struct RandomHook
        {
            void operator()(injector::reg_pack& regs)
            {
                std::mt19937 r{ std::random_device{}() };
                std::uniform_int_distribution<uint32_t> uid(0, regs.eax);
                regs.eax = uid(r);
            }
        }; injector::MakeInline<RandomHook>(pattern.get_first(0));
    }
    THPS3Pad::Install();
}
