#pragma once

namespace ScarfaceVehicleCamera
{
    inline SafetyHookMid BlendHook;
    inline uint32_t FlowTimeOffset = 0;

    inline float BlendWeight(float seconds, float original)
    {
        if (!std::isfinite(seconds) || seconds < 0.0f)
            return original;
        if (seconds == 1.0f / 30.0f)
            return original;
        return static_cast<float>(-std::expm1(std::log1p(-static_cast<double>(original)) * seconds * 30.0));
    }

    inline void UpdateBlend(SafetyHookContext& regs)
    {
        const auto time = *reinterpret_cast<const uint8_t**>(regs.esp + FlowTimeOffset);
        if (!time || reinterpret_cast<uintptr_t>(time) == 0xDDDDDDDD)
            return;
        auto weight = reinterpret_cast<float*>(regs.esp + 3 * sizeof(void*));
        *weight = BlendWeight(*reinterpret_cast<const float*>(time + 0x0C), *weight);
    }

    inline bool Install(HMODULE module = GetModuleHandle(nullptr))
    {
        if (BlendHook)
            return true;
        const auto base = reinterpret_cast<uintptr_t>(module);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const auto begin = base + nt->OptionalHeader.SizeOfHeaders;
        const auto end = base + nt->OptionalHeader.SizeOfImage;
        auto weight = hook::range_pattern(begin, end, "68 CD CC 4C 3E 81 C7 B8 01 00 00 57 8D B5 0C 01 00 00");
        auto blend = hook::range_pattern(begin, end, "E8 ? ? ? ? 8B 84 24 ? ? ? ? 8B 48 0C 89 4C 24");
        if (weight.size() != 1 || blend.size() != 1 || weight.get_first<uint8_t>() + 31 != blend.get_first<uint8_t>())
        {
            OutputDebugStringA("Scarface: vehicle camera smoothing patterns unavailable.\n");
            return false;
        }
        FlowTimeOffset = *blend.get_first<uint32_t>(8);
        if (FlowTimeOffset < 0x10 || FlowTimeOffset > 0x1000)
        {
            OutputDebugStringA("Scarface: vehicle camera FlowTime offset unavailable.\n");
            return false;
        }
        // Scale only this velocity blend; the native acceleration and camera update follow unchanged.
        BlendHook = safetyhook::create_mid(blend.get_first(), UpdateBlend);
        if (!BlendHook)
            OutputDebugStringA("Scarface: vehicle camera smoothing hook unavailable.\n");
        return bool(BlendHook);
    }
}
