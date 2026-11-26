module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <mutex>

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
    std::atomic<float> fovFactor{ 1.0f };
    std::atomic<bool> pillarboxRequested{ true };
    SafetyHookMid shGameplayFOV;
    SafetyHookInline shMotionCamera;
    SafetyHookInline shReleaseMotionCamera;
    SafetyHookInline shDestroyMotionCamera;
    std::mutex motionCameraMutex;
    uintptr_t activeMotionCamera{};
    std::atomic<ULONGLONG> motionCameraFrame{ 0 };

    void ClearMotionCamera(uintptr_t id)
    {
        const std::lock_guard lock(motionCameraMutex);
        if (activeMotionCamera == id)
        {
            activeMotionCamera = 0;
            motionCameraFrame.store(0, std::memory_order_relaxed);
        }
    }

    // uMotionCamera::move: +0xC0 = its keyframe data, +0x0C flag 0x4000 = playing
    void __fastcall MotionCamera(uint8_t* camera, void* edx)
    {
        shMotionCamera.unsafe_fastcall(camera, edx);
        const auto id = reinterpret_cast<uintptr_t>(camera);
        if (!*reinterpret_cast<uintptr_t*>(camera + 0xC0) || !(*reinterpret_cast<uint32_t*>(camera + 0x0C) & 0x4000))
        {
            ClearMotionCamera(id);
            return;
        }

        const std::lock_guard lock(motionCameraMutex);
        activeMotionCamera = id;
        // Present uses the border decision from this camera update.
        const auto mask = static_cast<ULONGLONG>(pillarboxRequested.load(std::memory_order_relaxed));
        motionCameraFrame.store((GetTickCount64() << 1) | mask, std::memory_order_relaxed);
    }

    int __fastcall ReleaseMotionCamera(uint8_t* camera, void*)
    {
        ClearMotionCamera(reinterpret_cast<uintptr_t>(camera));
        return shReleaseMotionCamera.unsafe_thiscall<int>(camera);
    }

    uint8_t* __fastcall DestroyMotionCamera(uint8_t* camera, void*, uint32_t flags)
    {
        ClearMotionCamera(reinterpret_cast<uintptr_t>(camera));
        return shDestroyMotionCamera.unsafe_thiscall<uint8_t*>(camera, flags);
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
        auto manager = reinterpret_cast<uint8_t*>(regs.esi);
        if (!IsGameplayManager(manager))
            return;
        auto fov = Field<float>(manager, 0x3C);
        if (RevCameraFOV::Scale(fov, Field<float>(manager, 0x34), Field<float>(manager, 0x30),
            fovFactor.load(std::memory_order_relaxed), Field<float>(manager, 0x38)))
        {
            // Change the published camera output, retaining the controller's unscaled state.
            *reinterpret_cast<float*>(manager + 0x3C) = fov;
        }
    }
}

export bool ShouldPillarboxCutscene()
{
    const auto frame = motionCameraFrame.load(std::memory_order_relaxed);
    return (frame & 1) && GetTickCount64() - (frame >> 1) < 250;
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            const auto update = []()
            {
                static std::mutex writers;
                const std::lock_guard lock(writers);
                const auto value = FixSettings().Get("MAIN.FOVFactor");
                float factor = 1.0f;
                const auto end = value.data() + value.size();
                const auto parsed = std::from_chars(value.data(), end, factor);
                if (parsed.ec != std::errc() || parsed.ptr != end)
                    factor = 1.0f;
                fovFactor.store(factor, std::memory_order_relaxed);
                pillarboxRequested.store(FixSettings().Get("MAIN.CutsceneBorders") == "2", std::memory_order_relaxed);
            };
            settings.OnChange("MAIN.FOVFactor", update);
            settings.OnChange("MAIN.CutsceneBorders", update);
            update();

            auto motion = hook::pattern("83 EC 18 53 56 8B F1 8B 86 C4 00 00 00 57 85 C0 74 1A 8B 40 0C 83 E0 07 83 F8 02 74 0F 83 F8 01 74 0A C7 86 C4 00 00 00 00 00 00 00 8B 86 C0 00 00 00");
            shMotionCamera = safetyhook::create_inline(motion.get_first(), MotionCamera);
            // uMotionCamera can be destroyed directly or through its scalar deleting destructor.
            auto release = hook::pattern("56 8B F1 8B 8E C0 00 00 00 C7 06 ? ? ? ? 85 C9 74 0F E8 ? ? ? ? C7 86 C0 00 00 00 00 00 00 00 C7 06 ? ? ? ? 8B CE 5E E9 ? ? ? ? 83 B9 C4 00 00 00 00");
            shReleaseMotionCamera = safetyhook::create_inline(release.get_first(), ReleaseMotionCamera);
            auto construct = hook::pattern("F3 0F 10 0D ? ? ? ? C7 06 ? ? ? ? F3 0F 11 46 7C F3 0F 11 86 8C 00 00 00 F3 0F 11 86 9C 00 00 00 F3 0F 11 86 AC 00 00 00 F3 0F 11 8E B0 00 00 00");
            const auto vtable = *construct.get_first<uint8_t**>(10);
            shDestroyMotionCamera = safetyhook::create_inline(vtable[0], DestroyMotionCamera);
            auto output = hook::pattern("D9 5E 34 8B 03 8B 50 48 8B CB FF D2 D9 5E 30 D9 86 8C 00 00 00 0F 57 C0 D9 5E 3C 8B CB D9 86 90 00 00 00");
            auto root = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 66 89 5C 24 0C 88 5C 24 0E 8B 44 24 0C");
            pGameCamera.SetAddress(*root.get_first<uint8_t**>(2));
            // The native x87 store completes before the callback; projection consumes the published camera output.
            shGameplayFOV = safetyhook::create_mid(output.get_first(27), ApplyGameplayFOV);
        };
    }
} Camera;
