#pragma once

namespace THPS2Camera
{
    inline SafetyHookMid CopyHook, ProjectionHook, DestroyHook;
    inline SafetyHookInline WorldHook;
    inline std::array<bool, 2> Gameplay{};
    inline const int* NativeIndex = nullptr;
    inline const int16_t* ReplayPlayers = nullptr;
    inline const uint8_t* Demo = nullptr;
    inline const float* Factor = nullptr;
    inline const int32_t* AspectFov = nullptr;
    inline int DrawingPlayer = -1;

    inline uint32_t Numerator(uint32_t original, float factor, int32_t aspectFov)
    {
        if (!std::isfinite(factor) || aspectFov <= 0 || factor == 1.0f || !original)
            return original;
        const float tangent = static_cast<float>(original) / (4096.0f * aspectFov);
        const float halfAngle = std::atan(tangent);
        const float corrected = std::tan(std::clamp(halfAngle * std::clamp(factor, 0.5f, 2.0f), 0.008726646f, 1.56206968f));
        return static_cast<uint32_t>(std::clamp(std::round(corrected * aspectFov), 1.0f, 65535.0f)) << 12;
    }

    inline int __cdecl DrawWorld(uint16_t* viewport, int player)
    {
        const auto previous = std::exchange(DrawingPlayer, player);
        const auto result = WorldHook.ccall<int>(viewport, player);
        DrawingPlayer = previous;
        return result;
    }

    inline void Install(const float* factor, const int32_t* aspectFov, HMODULE module = GetModuleHandleW(nullptr))
    {
        Factor = factor;
        AspectFov = aspectFov;
        auto copy = hook::module_pattern(module, "8B 81 A4 03 00 00 56 85 C0 57 0F 84 ? ? ? ? 83 B8 B8 30 00 00 05 75 ? A1");
        auto world = hook::module_pattern(module, "83 EC 54 53 55 6A 09 E8 ? ? ? ? 83 C4 04 BD 01 00 00 00 84 C0");
        auto replay = hook::module_pattern(module, "66 83 3D ? ? ? ? 00 7E 0F A0 ? ? ? ? 84 C0 75 06 B8 01 00 00 00 C3 33 C0 C3");
        auto projection = hook::module_pattern(module, "F7 3D ? ? ? ? 33 C9 66 8B 4E");
        auto destroy = hook::module_pattern(module, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 8B F1 89 74 24 04 C7 06 ? ? ? ? 68 ? ? ? ? C7 44 24 14 0D 00 00 00 E8");
        if (copy.size() != 1 || world.size() != 1 || replay.size() != 1 || projection.size() != 1 || destroy.size() != 1)
        {
            OutputDebugStringA("THPS2: gameplay camera patterns unavailable.\n");
            return;
        }
        NativeIndex = *copy.get_first<const int*>(26);
        ReplayPlayers = *replay.get_first<const int16_t*>(3);
        Demo = *replay.get_first<const uint8_t*>(11);
        CopyHook = safetyhook::create_mid(copy.get_first(), [](SafetyHookContext& regs)
        {
            const auto player = *NativeIndex;
            if (player >= 0 && player < static_cast<int>(Gameplay.size()))
            {
                const auto camera = regs.ecx;
                const auto mode = *reinterpret_cast<int*>(camera + 1284);
                Gameplay[player] = (mode == 1 || mode == 2) && *reinterpret_cast<uintptr_t*>(camera + 932) &&
                    *reinterpret_cast<int*>(camera + 1036) == 12 && !*reinterpret_cast<uint16_t*>(camera + 1040);
            }
        });
        ProjectionHook = safetyhook::create_mid(projection.get_first(), [](SafetyHookContext& regs)
        {
            if (DrawingPlayer >= 0 && DrawingPlayer < static_cast<int>(Gameplay.size()) && Gameplay[DrawingPlayer] &&
                *ReplayPlayers <= 0 && !*Demo && Factor && AspectFov)
            {
                regs.eax = Numerator(regs.eax, *Factor, *AspectFov);
                regs.edx = 0;
            }
        });
        DestroyHook = safetyhook::create_mid(destroy.get_first(), [](SafetyHookContext&)
        {
            Gameplay.fill(false);
        });
        WorldHook = safetyhook::create_inline(world.get_first(), DrawWorld);
        if (!CopyHook || !ProjectionHook || !DestroyHook || !WorldHook)
        {
            CopyHook.reset();
            ProjectionHook.reset();
            DestroyHook.reset();
            WorldHook.reset();
            Gameplay.fill(false);
            OutputDebugStringA("THPS2: gameplay camera hooks unavailable.\n");
        }
    }
}
