module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>

export module Camera;

import ComVars;

namespace RevCameraFOV
{
    inline bool Scale(float& fov, float nearClip, float farClip, float factor, float aspect)
    {
        if (!std::isfinite(fov) || fov <= 0.0f || fov >= 180.0f ||
            !std::isfinite(nearClip) || nearClip <= 0.0f || !std::isfinite(farClip) || farClip <= nearClip ||
            !std::isfinite(factor) || factor <= 0.0f || !std::isfinite(aspect) || aspect <= 0.0f)
            return false;
        constexpr double Pi = 3.14159265358979323846;
        const auto tangent = std::tan(double(fov) * Pi / 360.0);
        const auto maximum = std::tan(150.0 * Pi / 360.0) / double(aspect);
        if (factor == 1.0f && tangent <= maximum)
            return false;
        const auto scaled = std::min(tangent * double(std::clamp(factor, 0.5f, 2.0f)), maximum);
        const auto adjusted = float(2.0 * std::atan(scaled) * 180.0 / Pi);
        // The near plane stays as authored: the HUD and screen effects depend on it.
        if (!std::isfinite(adjusted) || adjusted <= 0.0f || adjusted >= 180.0f)
            return false;
        fov = adjusted;
        return true;
    }
}

namespace
{
    const float* pFOVFactor{};
    SafetyHookMid shGameplayFOV;
    SafetyHookInline shMotionCamera;
    std::atomic<ULONGLONG> nMotionCameraTick{ 0 };

    // uMotionCamera::move: +0xC0 = its keyframe data, +0x0C flag 0x4000 = playing
    void __fastcall MotionCamera(uint8_t* camera, void* edx)
    {
        shMotionCamera.unsafe_fastcall(camera, edx);
        if (*reinterpret_cast<uintptr_t*>(camera + 0xC0) && (*reinterpret_cast<uint32_t*>(camera + 0x0C) & 0x4000))
            nMotionCameraTick = GetTickCount64();
    }

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
        if (!type || (Field<uint32_t>(type, 24) & 0x7FFFFF) != size)
            return false;
        auto typeName = Field<const char*>(type, 4);
        return typeName && name == typeName;
    }

    bool IsPlayer(const void* object)
    {
        auto type = TypeInfo(object);
        if (!type || (Field<uint32_t>(type, 24) & 0x7FFFFF) < 0x1152)
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

    bool IsGameplayManager(uint8_t* manager)
    {
        if (!pGameCamera.is_initialized() || !pGameCamera.get() || !manager ||
            !HasType(pGameCamera.get(), "sGameCamera", 3344) ||
            Field<uint8_t*>(pGameCamera.get(), 0xCE0) != manager || !HasType(manager, "uCameraManage", 544))
            return false;
        auto aspect = Field<float>(manager, 0x38);
        if (!std::isfinite(aspect) || aspect <= 0.0f)
            return false;
        auto critical = reinterpret_cast<CRITICAL_SECTION*>(manager + 0x200);
        EnterCriticalSection(critical);
        struct Lock
        {
            CRITICAL_SECTION* critical;
            ~Lock()
            {
                LeaveCriticalSection(critical);
            }
        } lock{ critical };
        auto index = Field<uint32_t>(manager, 0xCC);
        auto count = Field<uint32_t>(manager, 0xD8);
        auto cameras = Field<uint8_t**>(manager, 0xE4);
        if (!cameras || count > 64 || index >= count || index != Field<uint32_t>(manager, 0xE8) ||
            index != Field<uint32_t>(manager, 0xD0))
            return false;
        auto camera = cameras[index];
        if (!HasType(camera, "cTPSCamera", 416) || Field<uint8_t*>(camera, 0x50) != manager ||
            !Field<uintptr_t>(camera, 0xC0) || !Field<uintptr_t>(camera, 0xD4) ||
            Field<uint8_t>(camera, 0xDC) || Field<uint32_t>(camera, 0xE4) ||
            Field<int32_t>(camera, 0xA0) < 0 || Field<int32_t>(camera, 0xA0) != Field<int32_t>(camera, 0xA4))
            return false;
        auto player = Field<uint8_t*>(manager, 0x78);
        if (!player || player != Field<uint8_t*>(manager, 0x74) || !IsPlayer(player))
            return false;
        auto state = Field<uint32_t>(player, 12) & 7;
        if ((state != 1 && state != 2) || Field<uint8_t>(player, 0x1151))
            return false;
        return true;
    }

    void ApplyGameplayFOV(SafetyHookContext& regs)
    {
        if (!pFOVFactor)
            return;
        auto manager = reinterpret_cast<uint8_t*>(regs.esi);
        if (!IsGameplayManager(manager))
            return;
        auto fov = Field<float>(manager, 0x3C);
        if (RevCameraFOV::Scale(fov, Field<float>(manager, 0x34), Field<float>(manager, 0x30), *pFOVFactor, Field<float>(manager, 0x38)))
        {
            // Change the published camera output, retaining the controller's unscaled state.
            *reinterpret_cast<float*>(manager + 0x3C) = fov;
        }
    }
}

// A cutscene is playing: a cutscene camera (uMotionCamera) plays its keyframes. It updates every frame while it plays.
export bool IsCutsceneCamera()
{
    return GetTickCount64() - nMotionCameraTick.load() < 250;
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            pFOVFactor = &FixSettings().Float("MAIN.FOVFactor");

            auto motion = hook::pattern("83 EC 18 53 56 8B F1 8B 86 C4 00 00 00 57 85 C0 74 1A 8B 40 0C 83 E0 07 83 F8 02 74 0F 83 F8 01 74 0A C7 86 C4 00 00 00 00 00 00 00 8B 86 C0 00 00 00");
            shMotionCamera = safetyhook::create_inline(motion.get_first(), MotionCamera);
            auto output = hook::pattern("D9 5E 34 8B 03 8B 50 48 8B CB FF D2 D9 5E 30 D9 86 8C 00 00 00 0F 57 C0 D9 5E 3C 8B CB D9 86 90 00 00 00");
            auto root = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 66 89 5C 24 0C 88 5C 24 0E 8B 44 24 0C");
            pGameCamera.SetAddress(*root.get_first<uint8_t**>(2));
            // The native x87 store completes before the callback; projection consumes the published camera output.
            shGameplayFOV = safetyhook::create_mid(output.get_first(27), ApplyGameplayFOV);
            if (!shGameplayFOV)
                OutputDebugStringA("Resident Evil Revelations Fusion Fix: gameplay camera hook unavailable.\n");
        };
    }
} Camera;
