#pragma once

namespace TonyCamera
{
    struct Bindings
    {
        const char* name;
        const char* registration;
        size_t registrationOffset;
        size_t componentCameraOffset;
        const char* construction;
        const char* update;
        const char* zoomScale;
        size_t nativeFovOffset;
        size_t cameraZoomOffset;
        size_t skaterOffset;
        size_t localPlayerOffset;
        size_t animationOffset;
        size_t animationSkipOffset;
    };

    struct Owner
    {
        uintptr_t component = 0;
        float nativeFov = 0.0f;
        bool gameplay = false;
    };

    inline Bindings Native{};
    inline std::map<uintptr_t, Owner> Cameras;
    inline const float* Factor = nullptr;
    inline const float* NativeZoomScale = nullptr;
    inline bool FixHorPlus = true;
    inline SafetyHookMid RegisterHook, ConstructHook, UpdateHook, DestroyHook;

    inline void Log(const char* message)
    {
        OutputDebugStringA((std::string(Native.name) + ": " + message + "\n").c_str());
    }

    inline float Correct(uintptr_t camera, float fov, float aspect)
    {
        float factor = 1.0f;
        if (auto it = Cameras.find(camera); it != Cameras.end() && it->second.gameplay)
        {
            const float nativeFov = *reinterpret_cast<float*>(camera + Native.nativeFovOffset);
            const bool zoomed = Native.cameraZoomOffset &&
                std::abs(*reinterpret_cast<float*>(camera + Native.cameraZoomOffset) - 1.0f) >= 0.001f;
            if (std::abs(nativeFov - it->second.nativeFov) < 0.001f && !zoomed &&
                NativeZoomScale && std::abs(*NativeZoomScale - 1.0f) < 0.001f && Factor && std::isfinite(*Factor))
                factor = std::clamp(*Factor, 0.5f, 2.0f);
        }
        return std::clamp((FixHorPlus ? AdjustFOV(fov, aspect) : fov) * factor, 1.0f, 179.0f);
    }

    inline void Install(const Bindings& bindings, const float* factor, bool fixHorPlus, HMODULE module = GetModuleHandleW(nullptr))
    {
        Native = bindings;
        Factor = factor;
        FixHorPlus = fixHorPlus;
        auto registration = hook::module_pattern(module, Native.registration);
        auto construction = hook::module_pattern(module, Native.construction);
        auto update = hook::module_pattern(module, Native.update);
        auto zoomScale = hook::module_pattern(module, Native.zoomScale);
        if (registration.size() != 1 || construction.size() != 1 || update.size() != 1 || zoomScale.empty())
        {
            Log("gameplay camera ownership patterns unavailable.");
            return;
        }
        auto getter = injector::GetBranchDestination(zoomScale.get_first()).as_int();
        for (size_t i = 1; i < zoomScale.size(); ++i)
        {
            if (injector::GetBranchDestination(zoomScale.get(i).get<void>()).as_int() != getter)
            {
                Log("camera zoom scale is ambiguous.");
                return;
            }
        }
        if (*reinterpret_cast<uint16_t*>(getter) != 0x05D9 || *reinterpret_cast<uint8_t*>(getter + 6) != 0xC3)
        {
            Log("camera zoom scale unavailable.");
            return;
        }
        NativeZoomScale = *reinterpret_cast<const float**>(getter + 2);
        auto table = *construction.get_first<uintptr_t*>(2);
        RegisterHook = safetyhook::create_mid(registration.get_first(Native.registrationOffset), [](SafetyHookContext& regs)
        {
            auto camera = Native.componentCameraOffset ?
                *reinterpret_cast<uintptr_t*>(regs.ecx + Native.componentCameraOffset) : regs.ecx;
            if (camera)
                Cameras[camera] = { regs.edi, *reinterpret_cast<float*>(regs.esp), false };
        });
        ConstructHook = safetyhook::create_mid(construction.get_first(), [](SafetyHookContext& regs)
        {
            Cameras.erase(regs.esi);
        });
        UpdateHook = safetyhook::create_mid(update.get_first(), [](SafetyHookContext& regs)
        {
            const auto component = regs.ecx;
            const auto mode = *reinterpret_cast<int*>(component + 0x24);
            const auto skater = *reinterpret_cast<uintptr_t*>(component + Native.skaterOffset);
            const auto animation = *reinterpret_cast<uintptr_t*>(component + Native.animationOffset);
            bool gameplay = mode >= 1 && mode <= 4 && skater && *reinterpret_cast<uint8_t*>(skater + Native.localPlayerOffset);
            if (animation)
            {
                const auto player = *reinterpret_cast<uintptr_t*>(animation + 0x2C);
                gameplay &= !*reinterpret_cast<uint8_t*>(animation + 0x18) && player &&
                    !*reinterpret_cast<uint8_t*>(player + Native.animationSkipOffset);
            }
            for (auto& [camera, owner] : Cameras)
            {
                if (owner.component == component)
                    owner.gameplay = gameplay;
            }
        });
        DestroyHook = safetyhook::create_mid(reinterpret_cast<void*>(*table), [](SafetyHookContext& regs)
        {
            Cameras.erase(regs.ecx);
        });
        if (!RegisterHook || !ConstructHook || !UpdateHook || !DestroyHook)
        {
            RegisterHook.reset();
            ConstructHook.reset();
            UpdateHook.reset();
            DestroyHook.reset();
            Cameras.clear();
            Log("gameplay camera hooks unavailable.");
        }
    }
}
