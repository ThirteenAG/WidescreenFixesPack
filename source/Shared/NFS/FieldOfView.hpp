#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace NFSFieldOfView
{
    inline uint32_t ScaleDrivingAngle(uint32_t value, float factor, float maximum)
    {
        factor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
        if (factor == 1.0f)
            return value;
        const auto angle = static_cast<float>(value & 0xFFFF) * factor;
        return (value & 0xFFFF0000) | static_cast<uint32_t>(std::clamp(angle, 0.0f, maximum));
    }

    template<class Pattern>
    inline bool InstallDrivingHooks(Pattern& pattern, ptrdiff_t offset, size_t expected, std::vector<SafetyHookMid>& hooks,
        void(*callback)(SafetyHookContext&))
    {
        if (!hooks.empty())
            return true;
        if (!expected || pattern.size() != expected)
        {
            OutputDebugStringA("NFS: driving FOV sites unavailable.\n");
            return false;
        }
        std::vector<SafetyHookMid> pending;
        pending.reserve(expected);
        for (size_t index = 0; index < expected; ++index)
        {
            auto hook = safetyhook::create_mid(pattern.get(index).template get<void>(offset), callback);
            if (!hook)
            {
                OutputDebugStringA("NFS: driving FOV hook unavailable.\n");
                return false;
            }
            pending.push_back(std::move(hook));
        }
        hooks = std::move(pending);
        return true;
    }
}
