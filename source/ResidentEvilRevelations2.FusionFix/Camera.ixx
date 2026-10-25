module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include "CameraFOV.hpp"

export module Camera;

import ComVars;

namespace
{
    const float* pFOVFactor{};
    uint8_t** pCameraSystem{};
    SafetyHookInline shSetPerspective;
    SafetyHookMid shGameplayFOV;
    SafetyHookMid shGameplayNear;

    template<typename T>
    const T& Field(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    const uint8_t* TypeInfo(const void* object)
    {
        if (!object)
            return nullptr;
        auto vtable = *static_cast<const uintptr_t* const*>(object);
        if (!vtable || !vtable[4])
            return nullptr;
        return reinterpret_cast<const uint8_t* (__thiscall*)(const void*)>(vtable[4])(object);
    }

    bool HasType(const void* object, std::string_view name, size_t size)
    {
        auto type = TypeInfo(object);
        if (!type || (Field<uint32_t>(type, 24) & 0x7FFFFF) != size / 4)
            return false;
        auto typeName = Field<const char*>(type, 4);
        return typeName && name == typeName;
    }

    bool IsPlayer(const void* object)
    {
        auto type = TypeInfo(object);
        if (!type || (Field<uint32_t>(type, 24) & 0x7FFFFF) < 0x7E90 / 4)
            return false;
        for (size_t depth = 0; type && depth < 16; ++depth)
        {
            auto name = Field<const char*>(type, 4);
            if (name && std::string_view(name) == "uPlayer")
                return true;
            type = Field<const uint8_t*>(type, 16);
        }
        return false;
    }

    bool IsGameplayController(uint8_t* controller, float fov)
    {
        if (!pCameraSystem || !*pCameraSystem || !HasType(*pCameraSystem, "sBioCamera", 5728) ||
            !HasType(controller, "uCameraCtrl", 24896))
            return false;
        auto modeManager = pGameMode.get();
        if (!modeManager)
            return false;
        auto mode = Field<uint32_t>(reinterpret_cast<void*>(modeManager), 0x8F4);
        if (mode != 0 && mode != 1 && mode != 4)
            return false;
        auto system = *pCameraSystem;
        if (Field<uint32_t>(system, 3296) != mode || Field<uint32_t>(system, 3336))
            return false;
        bool visible{};
        for (size_t i = 0; i < 8; ++i)
            visible |= Field<uint8_t*>(system, 52 + i * 400) == controller;
        if (!visible || Field<uint8_t>(controller, 23984) || Field<uint32_t>(controller, 24024))
            return false;
        auto camera = controller + 464;
        if (Field<uint8_t*>(controller, 452) != camera || Field<uint8_t*>(controller, 456) != camera ||
            Field<uint8_t*>(controller, 460) != camera || !HasType(camera, "uCameraQFPS", 9856))
            return false;
        auto player = Field<uint8_t*>(controller, 144);
        if (!player || player != Field<uint8_t*>(camera, 144) || !IsPlayer(player))
            return false;
        auto state = Field<uint32_t>(player, 12) & 7;
        auto input = Field<uint8_t*>(player, 0x5940);
        if ((state != 1 && state != 2) || !input || Field<uint32_t>(input, 20) == 2)
            return false;
        // Both parameter banks retain their native aiming state across camera interpolation.
        if (Field<uint8_t>(camera, 8448) || Field<uint8_t>(camera, 8500) ||
            Field<uint32_t>(camera, 8932) != 3 || Field<float>(camera, 9220) != 1.0f ||
            !Field<uintptr_t>(camera, 8656) ||
            ((Field<uint32_t>(camera, 9708) | Field<uint32_t>(camera, 9712)) & 0xD))
            return false;
        auto aspect = Field<float>(controller, 56);
        auto nativeFOV = Field<float>(camera, 60);
        auto nearClip = Field<float>(controller, 52);
        auto farClip = Field<float>(controller, 48);
        return std::isfinite(aspect) && aspect > 0.0f && std::isfinite(nearClip) && nearClip > 0.0f &&
            std::isfinite(farClip) && farClip > nearClip && std::isfinite(nativeFOV) &&
            std::isfinite(fov) && std::abs(fov - nativeFOV) <= 0.001f;
    }

    void ApplyGameplayFOV(SafetyHookContext& regs)
    {
        if (!pFOVFactor || *pFOVFactor == 1.0f || !std::isfinite(*pFOVFactor) || *pFOVFactor <= 0.0f)
            return;
        auto& fov = *reinterpret_cast<float*>(regs.esp + 12);
        if (IsGameplayController(reinterpret_cast<uint8_t*>(regs.edi), fov))
            fov = Rev2CameraFOV::Scale(fov, *pFOVFactor);
    }

    void ApplyGameplayNear(SafetyHookContext& regs)
    {
        if (!pFOVFactor || *pFOVFactor == 1.0f || !std::isfinite(*pFOVFactor) || *pFOVFactor <= 0.0f)
            return;
        auto controller = reinterpret_cast<uint8_t*>(regs.ecx);
        if (!HasType(controller, "uCameraCtrl", 24896))
            return;
        auto originalFOV = Field<float>(controller + 464, 60);
        if (!IsGameplayController(controller, originalFOV) || originalFOV <= 0.0f)
            return;
        auto fov = Field<float>(controller, 60);
        auto expected = Rev2CameraFOV::Scale(originalFOV, *pFOVFactor);
        if (fov == originalFOV || !std::isfinite(fov) || std::abs(fov - expected) > 0.001f)
            return;
        auto nearClip = regs.xmm0.f32[0] / (fov / originalFOV);
        if (std::isfinite(nearClip) && nearClip > 0.0f && nearClip < Field<float>(controller, 48))
            regs.xmm0.f32[0] = nearClip;
    }

    void __fastcall SetPerspective(float* matrix, void* edx, float zNear, float zFar, float fov, float aspect)
    {
        if (std::isfinite(aspect) && std::abs(aspect - 1.0f) > 0.01f)
        {
            auto diff = GetDiff();
            if (std::isfinite(diff) && diff > 0.0f)
                zNear /= diff;
        }
        shSetPerspective.unsafe_fastcall(matrix, edx, zNear, zFar, fov, aspect);
    }
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            pFOVFactor = &FixSettings().Float("MAIN.FOVFactor");
            auto perspective = hook::pattern("E8 ? ? ? ? 8B 44 24 04 C2 04 00 CC CC CC CC CC CC CC CC CC CC 8D 41 50");
            if (PatternUnique(perspective, "ResidentEvilRevelations2.FusionFix: Camera"))
                shSetPerspective = safetyhook::create_inline(injector::GetBranchDestination(perspective.get_first()).as_int(), SetPerspective);
            if (!shSetPerspective)
                OutputDebugStringA("Resident Evil Revelations 2 Fusion Fix: near-clip projection hook unavailable.\n");

            auto output = hook::pattern("C7 87 D8 5D 00 00 00 00 00 00 8B 87 C4 01 00 00 F3 0F 10 44 24 70 F3 0F 11 80 20 01 00 00");
            auto nearClip = hook::pattern("F3 0F 10 41 34 8B 4C 24 14 F3 0F 11 04 24 E8 ? ? ? ? 8B 44 24 04 C2 04 00");
            auto root = hook::pattern("A1 ? ? ? ? 32 D2 33 C9 88 54 24 0B 83 C0 34 8D 49 00 39 38 74 ? 41 05 90 01 00 00");
            if (output.size() != 1 || nearClip.size() != 1 || root.size() != 1)
            {
                OutputDebugStringA("Resident Evil Revelations 2 Fusion Fix: unsupported gameplay camera layout.\n");
                return;
            }
            pCameraSystem = *root.get_first<uint8_t**>(1);
            // ECX still identifies the camera and XMM0 contains its freshly loaded near plane.
            shGameplayNear = safetyhook::create_mid(nearClip.get_first(5), ApplyGameplayNear);
            if (!shGameplayNear)
            {
                OutputDebugStringA("Resident Evil Revelations 2 Fusion Fix: gameplay near-clip hook unavailable.\n");
                return;
            }
            // Change the final local FOV before native publication and culling, while override flags are still intact.
            shGameplayFOV = safetyhook::create_mid(output.get_first(), ApplyGameplayFOV);
            if (!shGameplayFOV)
            {
                shGameplayNear.reset();
                OutputDebugStringA("Resident Evil Revelations 2 Fusion Fix: gameplay camera hook unavailable.\n");
            }
        };
    }
} Camera;
