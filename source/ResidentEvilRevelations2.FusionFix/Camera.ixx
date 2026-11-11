module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>

export module Camera;

import ComVars;

namespace Rev2CameraFOV
{
    inline float Scale(float fov, float factor)
    {
        if (!std::isfinite(fov) || fov <= 0.0f || fov >= 180.0f ||
            !std::isfinite(factor) || factor <= 0.0f || factor == 1.0f)
            return fov;
        auto result = std::min(fov * std::clamp(factor, 0.5f, 2.0f), std::max(fov, 170.0f));
        return std::isfinite(result) && result > 0.0f ? result : fov;
    }
}

namespace
{
    const float* pFOVFactor{};
    const int* pCutsceneBorders{};
    SafetyHookInline shSetPerspective;
    SafetyHookInline shEventProjection;
    SafetyHookInline shCinemaScope;
    SafetyHookMid shGameplayFOV;
    SafetyHookMid shGameplayNear;
    thread_local bool bFittingEventProjection{};

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

    bool IsEventActive()
    {
        if (!pCameraSystem.is_initialized() || !pCameraSystem.get())
            return false;
        for (size_t i = 0; i < 8; ++i)
        {
            auto camera = Field<uint8_t*>(pCameraSystem.get(), 52 + i * 400);
            if (camera && HasType(camera, "uEventMotionCamera", 448))
                return true;
        }
        return false;
    }

    bool IsActiveCamera(const uint8_t* camera)
    {
        if (!pCameraSystem.is_initialized() || !pCameraSystem.get())
            return false;
        for (size_t i = 0; i < 8; ++i)
            if (Field<uint8_t*>(pCameraSystem.get(), 52 + i * 400) == camera)
                return true;
        return false;
    }

    float* __fastcall EventProjection(uint8_t* camera, void*, float* matrix)
    {
        auto& aspect = *reinterpret_cast<float*>(camera + 56);
        const auto original = aspect;
        const auto physical = GetAspectRatio();
        const bool fit = !IsSplitScreenActive() && IsActiveCamera(camera) &&
            std::isfinite(physical) && physical > 0.0f && physical < defaultAspectRatio &&
            std::abs(original - defaultAspectRatio) < 0.001f;
        const auto previous = bFittingEventProjection;
        if (fit)
            aspect = physical;
        bFittingEventProjection = fit;
        auto result = shEventProjection.unsafe_thiscall<float*>(camera, matrix);
        bFittingEventProjection = previous;
        aspect = original;
        return result;
    }

    int __fastcall DrawCinemaScope(void* scope, void*, void* context)
    {
        if (pCutsceneBorders && !(std::clamp(*pCutsceneBorders, 0, 3) & 1) && IsEventActive())
            return 0;
        return shCinemaScope.unsafe_thiscall<int>(scope, context);
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

    bool IsGameplayController(uint8_t* controller, float fov, bool allowAim = false)
    {
        if (!pCameraSystem.is_initialized() || !pCameraSystem.get() || !HasType(pCameraSystem.get(), "sBioCamera", 5728) ||
            !HasType(controller, "uCameraCtrl", 24896))
            return false;
        auto modeManager = pGameMode.get();
        if (!modeManager)
            return false;
        auto mode = Field<uint32_t>(reinterpret_cast<void*>(modeManager), 0x8F4);
        if (mode != 0 && mode != 1 && mode != 4)
            return false;
        auto system = pCameraSystem.get();
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
        if ((!allowAim && (Field<uint32_t>(camera, 8440) || Field<uint32_t>(camera, 8444))) ||
            Field<uint8_t>(camera, 8448) || Field<uint8_t>(camera, 8500) ||
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
        auto controller = reinterpret_cast<uint8_t*>(regs.edi);
        auto& fov = *reinterpret_cast<float*>(regs.esp + 12);
        // The game can keep a 16:9 camera in narrower display modes. Publish the actual aspect
        // before its projection and culling are built, including while the player is aiming.
        if (!IsSplitScreenActive() && IsGameplayController(controller, fov, true))
        {
            const auto aspect = GetAspectRatio();
            auto& cameraAspect = *reinterpret_cast<float*>(controller + 56);
            if (std::isfinite(aspect) && aspect > 0.0f && aspect < defaultAspectRatio && cameraAspect > aspect)
                cameraAspect = aspect;
        }
        if (pFOVFactor && *pFOVFactor != 1.0f && std::isfinite(*pFOVFactor) && *pFOVFactor > 0.0f &&
            IsGameplayController(controller, fov))
            fov = Rev2CameraFOV::Scale(fov, *pFOVFactor);
    }

    void ApplyGameplayNear(SafetyHookContext& regs)
    {
        auto controller = reinterpret_cast<uint8_t*>(regs.ecx);
        if (!HasType(controller, "uCameraCtrl", 24896))
            return;
        auto originalFOV = Field<float>(controller + 464, 60);
        if (!IsGameplayController(controller, originalFOV, true) || originalFOV <= 0.0f)
            return;
        // The game can republish its logical aspect after the camera update. Correct the
        // already-prepared projection argument too, without changing scripted cameras.
        if (!IsSplitScreenActive())
        {
            const auto aspect = GetAspectRatio();
            auto& projectionAspect = *reinterpret_cast<float*>(regs.esp + 12);
            if (std::isfinite(aspect) && aspect > 0.0f && aspect < defaultAspectRatio && projectionAspect > aspect)
                projectionAspect = aspect;
        }
        if (!pFOVFactor || *pFOVFactor == 1.0f || !std::isfinite(*pFOVFactor) || *pFOVFactor <= 0.0f ||
            !IsGameplayController(controller, originalFOV))
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
        // At narrow aspects, fit the original 16:9 camera vertically inside
        // the screen when its framing is requested. The side-border mode
        // covers the added vertical area at the end of the frame.
        if (bFittingEventProjection && pCutsceneBorders && (std::clamp(*pCutsceneBorders, 0, 3) & 2) &&
            std::isfinite(aspect) && aspect > 0.0f && fov > 0.0f && fov < 3.14f)
            fov = 2.0f * std::atan(std::tan(fov * 0.5f) * defaultAspectRatio / aspect);
        if (std::isfinite(aspect) && std::abs(aspect - 1.0f) > 0.01f)
        {
            auto diff = GetDiff();
            if (!IsSplitScreenActive())
                diff = std::max(diff, 1.0f);
            if (std::isfinite(diff) && diff > 0.0f)
                zNear /= diff;
        }
        shSetPerspective.unsafe_fastcall(matrix, edx, zNear, zFar, fov, aspect);
    }
}

// Scripted scenes publish the event camera directly in the camera system's
// active slots; the ordinary player camera is a uCameraCtrl instead.
export bool IsCutsceneCamera()
{
    return IsEventActive();
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            pFOVFactor = &FixSettings().Float("MAIN.FOVFactor");
            pCutsceneBorders = &FixSettings().Int("MAIN.CutsceneBorders");
            auto eventProjection = hook::pattern("83 EC 10 56 8B F1 57 80 BE BC 01 00 00 00 F3 0F 10 86 C0 00 00 00");
            shEventProjection = safetyhook::create_inline(eventProjection.get_first(), EventProjection);
            auto cinemaScope = hook::pattern("55 8B EC 83 E4 F0 81 EC 58 01 00 00 8B 55 08 56 0F B7 82 5A 01 00 00");
            shCinemaScope = safetyhook::create_inline(cinemaScope.get_first(), DrawCinemaScope);
            auto perspective = hook::pattern("E8 ? ? ? ? 8B 44 24 04 C2 04 00 CC CC CC CC CC CC CC CC CC CC 8D 41 50");
            shSetPerspective = safetyhook::create_inline(injector::GetBranchDestination(perspective.get_first()).as_int(), SetPerspective);

            auto output = hook::pattern("C7 87 D8 5D 00 00 00 00 00 00 8B 87 C4 01 00 00 F3 0F 10 44 24 70 F3 0F 11 80 20 01 00 00");
            auto nearClip = hook::pattern("F3 0F 10 41 34 8B 4C 24 14 F3 0F 11 04 24 E8 ? ? ? ? 8B 44 24 04 C2 04 00");
            auto root = hook::pattern("A1 ? ? ? ? 32 D2 33 C9 88 54 24 0B 83 C0 34 8D 49 00 39 38 74 ? 41 05 90 01 00 00");
            pCameraSystem.SetAddress(*root.get_first<uint8_t**>(1));
            // ECX still identifies the camera and XMM0 contains its freshly loaded near plane.
            shGameplayNear = safetyhook::create_mid(nearClip.get_first(5), ApplyGameplayNear);
            // Both the override and ordinary camera paths converge after the reset instruction.
            // Change the local FOV before native publication and culling.
            shGameplayFOV = safetyhook::create_mid(output.get_first(10), ApplyGameplayFOV);
        };
    }
} Camera;
