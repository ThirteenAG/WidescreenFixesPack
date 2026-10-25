#pragma once
#include <atomic>

namespace FarCryCamera
{
    inline SafetyHookInline ViewHook;
    inline SafetyHookMid ClientHook;
    inline uintptr_t ClientReturn = 0;
    inline std::atomic<bool> Gameplay{ false };
    inline thread_local const void* ExpectedView = nullptr;
    inline thread_local bool ExpectedGameplay = false;

    inline bool PlayerCamera(const void* client, const void* player)
    {
        if (!client || !player)
            return false;
        const auto input = static_cast<const uint8_t*>(client);
        const auto actor = static_cast<const uint8_t*>(player);
#ifdef _WIN64
        const auto override = *reinterpret_cast<const uint8_t* const*>(input + 736);
        return override && !*reinterpret_cast<const uint16_t*>(override + 8) &&
            *reinterpret_cast<const int*>(actor + 168) > 0 && !actor[214];
#else
        const auto override = *reinterpret_cast<const uint8_t* const*>(input + 552);
        return override && !*reinterpret_cast<const uint16_t*>(override + 4) &&
            *reinterpret_cast<const int*>(actor + 160) > 0 && !actor[206];
#endif
    }

    inline void Client(SafetyHookContext& regs)
    {
#ifdef _WIN64
        ExpectedView = reinterpret_cast<const void*>(regs.rsp + 0x500);
        ExpectedGameplay = PlayerCamera(reinterpret_cast<const void*>(regs.rbp), *reinterpret_cast<const void* const*>(regs.rsp + 0x38));
#else
        ExpectedView = reinterpret_cast<const void*>(regs.ebp - 0x240);
        ExpectedGameplay = PlayerCamera(reinterpret_cast<const void*>(regs.esi), *reinterpret_cast<const void* const*>(regs.ebp - 0x30));
#endif
    }

    inline void Submitted(const void* camera, uintptr_t caller)
    {
        Gameplay.store(camera && camera == ExpectedView && caller == ClientReturn && ExpectedGameplay);
        ExpectedView = nullptr;
        ExpectedGameplay = false;
    }

#ifndef _WIN64
    inline uintptr_t __fastcall SetView(void* system, void*, const void* camera)
    {
        Submitted(camera, reinterpret_cast<uintptr_t>(_ReturnAddress()));
        return ViewHook.unsafe_fastcall<uintptr_t>(system, nullptr, camera);
    }
#else
    inline uintptr_t __fastcall SetView(void* system, const void* camera)
    {
        Submitted(camera, reinterpret_cast<uintptr_t>(_ReturnAddress()));
        return ViewHook.unsafe_fastcall<uintptr_t>(system, camera);
    }
#endif

    inline float Factor(float aspectFactor, float userFactor)
    {
        return aspectFactor * (Gameplay.load() ? userFactor : 1.0f);
    }

    inline void Reset()
    {
        ClientHook.reset();
        ViewHook.reset();
        ClientReturn = 0;
        Gameplay.store(false);
        ExpectedView = nullptr;
        ExpectedGameplay = false;
    }

    inline bool Install(HMODULE game, HMODULE system)
    {
        if (ClientHook && ViewHook)
            return true;
        if (!game || !system)
            return false;
#ifdef _WIN64
        auto client = hook::module_pattern(game, "48 8B 45 78 48 8B 48 40 48 8B 01 48 8D 94 24 00 05 00 00 FF 90 A0 01 00 00 80 BD 80 00 00 00 00");
        auto view = hook::module_pattern(system, "48 81 C1 B8 24 00 00 E9 ? ? ? ? CC CC CC CC 48 8D 81 B8 24 00 00 C3");
        auto player = hook::module_pattern(game, "48 8B 00 4C 8D 44 24 38 33 D2 48 8B CF FF 50 50 84 C0 74 ? 48 8B 4C 24 38 44 88 B1 22 01 00 00");
        constexpr ptrdiff_t callOffset = 19, hookOffset = 11;
#else
        auto client = hook::module_pattern(game, "8B 46 44 8B 48 24 8B 01 8D 95 C0 FD FF FF 52 FF 90 D0 00 00 00 80 7E 48 00");
        auto view = hook::module_pattern(system, "81 C1 A8 24 00 00 E9 ? ? ? ? 55 8B EC 8B C1 8B 4D 08 53 56 57");
        auto player = hook::module_pattern(game, "8B 07 8D 4D D0 51 53 8B CF FF 50 28 84 C0 74 ? 8B 45 D0 C6 80 1A 01 00 00 00");
        constexpr ptrdiff_t callOffset = 15, hookOffset = 8;
#endif
        if (client.size() != 1 || view.size() != 1 || player.size() != 1)
        {
            OutputDebugStringA("Far Cry: camera ownership sites unavailable; gameplay FOV multiplier disabled.\n");
            return false;
        }
        const auto source = reinterpret_cast<uintptr_t>(client.get_first());
        const auto actor = reinterpret_cast<uintptr_t>(player.get_first());
        if (actor >= source || source - actor > 0x1000)
        {
            OutputDebugStringA("Far Cry: native camera frame layout unavailable; gameplay FOV multiplier disabled.\n");
            return false;
        }
        const auto call = client.get_first<uint8_t>(callOffset);
        ClientReturn = reinterpret_cast<uintptr_t>(call + 6);
        ViewHook = safetyhook::create_inline(view.get_first(), SetView);
        if (ViewHook)
            ClientHook = safetyhook::create_mid(client.get_first(hookOffset), Client);
        if (!ClientHook || !ViewHook)
        {
            Reset();
            OutputDebugStringA("Far Cry: camera ownership hooks unavailable; gameplay FOV multiplier disabled.\n");
            return false;
        }
        return true;
    }
}
