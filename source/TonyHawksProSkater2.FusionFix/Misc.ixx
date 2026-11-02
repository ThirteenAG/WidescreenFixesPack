module;

#include <stdafx.h>

export module Misc;

import ComVars;

export namespace THPS2Controller
{
    inline SafetyHookMid BindingsHook;

    inline void BindUnassigned(int* bindings, size_t count, int kick, int& binding, int button)
    {
        if (binding == -1 && kick != button && std::find(bindings, bindings + count, button) == bindings + count)
            binding = button;
    }

    inline void Install()
    {
        auto camera = hook::pattern("A1 ? ? ? ? 83 F8 FF 0F 8C ? ? ? ? 83 F8 20 0F 8F ? ? ? ? A1 ? ? ? ? C7 44 24 20 00 00 00 00");
        auto kick = hook::pattern("68 ? ? ? ? 8D 8C 24 8C 00 00 00 68 ? ? ? ? 51 E8");
        LastBinding.SetAddress(*camera.get_first<int*>(1));
        KickBinding.SetAddress(*kick.get_first<int*>(1));
        // The native reader has validated the ten action bindings here.
        // Add Start/Back only when those buttons and actions are unassigned.
        BindingsHook = safetyhook::create_mid(camera.get_first(23), [](SafetyHookContext&)
        {
            auto firstBinding = LastBinding.get_ptr() - 8;
            BindUnassigned(firstBinding, 9, KickBinding.get(), LastBinding.get_ptr()[-1], 9);
            BindUnassigned(firstBinding, 9, KickBinding.get(), LastBinding.get(), 8);
        });
    }
}

export namespace THPS2Startup
{
    inline void Install(bool skip, void* module = GetModuleHandleW(nullptr))
    {
        if (!skip) return;
        auto first = hook::module_pattern(module, "E8 ? ? ? ? 6A FF C6 05 ? ? ? ? 00 C6 05 ? ? ? ? 00 C6 05 ? ? ? ? 00 E8 ? ? ? ? A0 ? ? ? ? 83 C4 04 84 C0");
        auto second = hook::module_pattern(module, "6A FE E8 ? ? ? ? 83 C4 04 68 ? ? ? ? E8 ? ? ? ? A0 ? ? ? ? 83 C4 04");
        auto intro = hook::module_pattern(module, "6A FD E8 ? ? ? ? 59 C3");
        injector::MakeNOP(first.get_first(28), 5, true);
        injector::MakeNOP(second.get_first(2), 5, true);
        injector::MakeNOP(second.get_first(15), 5, true);
        // Only the startup caller skips the skating intro. Other movie requests stay native.
        injector::MakeNOP(intro.get_first(2), 5, true);
        injector::WriteMemory<uint16_t>(intro.get_first(2), 0xC031, true);
    }
}
