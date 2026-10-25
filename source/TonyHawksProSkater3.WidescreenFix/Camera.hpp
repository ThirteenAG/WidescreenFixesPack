#pragma once

namespace THPS3Camera
{
    struct View
    {
        uintptr_t owner = 0;
        std::array<float, 2> native{};
    };

    inline std::map<uintptr_t, View> Cameras;
    inline SafetyHookInline ViewHook, BeginHook;
    inline SafetyHookMid BindHook, DestroyHook;
    inline const float* Factor = nullptr;
    inline float AspectScale = 1.0f;

    inline uintptr_t __cdecl SetView(uintptr_t camera, const float* view)
    {
        std::array<float, 2> corrected{ view[0] * AspectScale, view[1] };
        if (auto it = Cameras.find(camera); it != Cameras.end())
            it->second.native = corrected;
        return ViewHook.ccall<uintptr_t>(camera, corrected.data());
    }

    inline uintptr_t __cdecl Begin(uintptr_t camera)
    {
        if (auto it = Cameras.find(camera); it != Cameras.end())
        {
            const auto& view = it->second;
            const auto owner = view.owner;
            const auto mode = *reinterpret_cast<int*>(owner + 0x1C);
            const float focalLength = *reinterpret_cast<float*>(owner + 0x6C);
            float factor = 1.0f;
            if (mode >= 1 && mode <= 4 && *reinterpret_cast<uintptr_t*>(owner + 0x48) &&
                !*reinterpret_cast<uintptr_t*>(owner + 0x3C) && !*reinterpret_cast<uint8_t*>(owner + 0x11D) &&
                focalLength > 0.000001f && std::abs(view.native[1] - 0.018f / focalLength) < 0.0001f &&
                Factor && std::isfinite(*Factor))
                factor = std::clamp(*Factor, 0.5f, 2.0f);
            auto corrected = view.native;
            if (corrected[0] > 0.0f && corrected[1] > 0.0f &&
                std::isfinite(corrected[0]) && std::isfinite(corrected[1]))
            {
                if (factor != 1.0f)
                {
                    const float halfAngle = std::atan(corrected[0]);
                    const float scale = std::tan(std::clamp(halfAngle * factor, 0.008726646f, 1.56206968f)) / corrected[0];
                    corrected[0] *= scale;
                    corrected[1] *= scale;
                }
                // The native setter refreshes projection reciprocals and marks the frustum dirty.
                ViewHook.ccall<uintptr_t>(camera, corrected.data());
            }
        }
        return BeginHook.ccall<uintptr_t>(camera);
    }

    inline void Install(float aspectScale, const float* factor, HMODULE module = GetModuleHandleW(nullptr))
    {
        AspectScale = aspectScale;
        Factor = factor;
        auto view = hook::module_pattern(module, "8B 44 24 08 56 8B 74 24 08 8B 08 D9 05 ? ? ? ? 89 4E 68");
        if (view.size() != 1)
        {
            OutputDebugStringA("THPS3: camera projection pattern unavailable.\n");
            return;
        }
        ViewHook = safetyhook::create_inline(view.get_first(), SetView);
        if (!ViewHook)
        {
            OutputDebugStringA("THPS3: camera projection hook unavailable.\n");
            return;
        }
        auto bind = hook::module_pattern(module, "8B 44 24 04 56 89 41 44 57 8B 40 10 8D 51 70");
        auto destroy = hook::module_pattern(module, "56 8B F1 8B 46 20 85 C0 74 10 50 E8 ? ? ? ? 83 C4 04 C7 46 20 00 00 00 00 8B 46 3C");
        auto begin = hook::module_pattern(module, "8B 44 24 04 50 FF 50 18 83 C4 04 C3");
        if (bind.size() != 1 || destroy.size() != 1 || begin.size() != 1)
        {
            OutputDebugStringA("THPS3: gameplay camera ownership patterns unavailable.\n");
            return;
        }
        BindHook = safetyhook::create_mid(bind.get_first(), [](SafetyHookContext& regs)
        {
            std::erase_if(Cameras, [&](const auto& entry) { return entry.second.owner == regs.ecx; });
            const auto wrapper = *reinterpret_cast<uintptr_t*>(regs.esp + 4);
            if (wrapper)
            {
                const auto camera = *reinterpret_cast<uintptr_t*>(wrapper + 0x10);
                if (camera)
                    Cameras[camera] = { regs.ecx, { *reinterpret_cast<float*>(camera + 0x68), *reinterpret_cast<float*>(camera + 0x6C) } };
            }
        });
        DestroyHook = safetyhook::create_mid(destroy.get_first(), [](SafetyHookContext& regs)
        {
            std::erase_if(Cameras, [&](const auto& entry) { return entry.second.owner == regs.ecx; });
        });
        BeginHook = safetyhook::create_inline(begin.get_first(), Begin);
        if (!BindHook || !DestroyHook || !BeginHook)
        {
            BeginHook.reset();
            BindHook.reset();
            DestroyHook.reset();
            Cameras.clear();
            OutputDebugStringA("THPS3: gameplay camera hooks unavailable.\n");
        }
    }
}
