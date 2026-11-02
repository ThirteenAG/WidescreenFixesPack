module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;

// The projection. The game builds every projection with one function (matrix, near, far, vertical FOV in radians,
// aspect), so the 3D view is already Hor+ at any aspect.
// - Near clip: the near plane is moved closer (divided by 1.6) at every caller, so the camera doesn't see through walls
//   when it gets close to them, in co-op too.
// - MAIN.FOVFactor (live) scales the gameplay projection tangent, up to 150 degrees across. The weapon view and the
//   look camera while aiming or looking at the partner keep their framing; cutscenes (uMotionCamera) don't use this
//   projection.

namespace
{
    injector::memory_pointer_raw pSetPerspective;
    void __fastcall SetPerspective(float* _this, void* edx, float zNear, float zFar, float fovY, float aspect)
    {
        zNear /= 1280.0f / 800.0f;
        return injector::fastcall<void(float*, void*, float, float, float, float)>::call(pSetPerspective, _this, edx, zNear, zFar, fovY, aspect);
    }

    enum class CameraKind
    {
        Gameplay,
        Player,
        Weapon
    };

    // The camera's class from its DTI (vtable slot 4 returns it; the name is at +4): the weapon view (uCameraWep) keeps
    // its framing, the player's look camera (uCameraQFPS) only while it aims or looks at the partner
    bool IsGameplayCamera(uintptr_t camera)
    {
        static std::unordered_map<uintptr_t, CameraKind> cache;
        const auto vtable = *reinterpret_cast<uintptr_t*>(camera);
        auto it = cache.find(vtable);
        if (it == cache.end())
        {
            CameraKind kind = CameraKind::Gameplay;
            auto getDTI = *reinterpret_cast<uintptr_t*(__fastcall**)(uintptr_t, void*)>(vtable + 4 * 4);
            if (const auto dti = getDTI(camera, nullptr))
            {
                if (const auto name = reinterpret_cast<const char*>(dti[1]))
                {
                    const std::string_view type{ name };
                    if (type == "uCameraQFPS") kind = CameraKind::Player;
                    else if (type == "uCameraWep") kind = CameraKind::Weapon;
                }
            }
            it = cache.emplace(vtable, kind).first;
        }
        if (it->second == CameraKind::Player)
            return !*reinterpret_cast<const uint8_t*>(camera + 0x1B1) &&
                !*reinterpret_cast<const uint8_t*>(camera + 0x1B3);
        return it->second == CameraKind::Gameplay;
    }

    float ScaleFOV(float fov, float aspect, float factor)
    {
        constexpr float pi = 3.14159265358979323846f;
        if (factor == 1.0f || !std::isfinite(fov) || !std::isfinite(aspect) || !std::isfinite(factor) ||
            fov <= 0.0f || fov >= pi || aspect <= 0.0f || factor <= 0.0f)
            return fov;
        const auto tangent = std::tan(fov * 0.5f) * std::clamp(factor, 0.5f, 2.0f);
        const auto maximum = std::tan(75.0f * pi / 180.0f) / aspect;
        return 2.0f * std::atan(std::min(tangent, maximum));
    }

}

export namespace Camera
{
    void Init()
    {
        [&]()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7");

            pSetPerspective = injector::GetBranchDestination(pattern.get_first());
            injector::MakeCALL(pattern.get_first(), SetPerspective, true);
            injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 55"), SetPerspective, true);
            injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 80 BF ? ? ? ? ? 74 0E"), SetPerspective, true);
            injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? F3 0F 10 AC 24 ? ? ? ? F3 0F 10 84 24 ? ? ? ? F3 0F 10 A4 24"), SetPerspective, true);
            injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8D 97 ? ? ? ? 52 8D 87"), SetPerspective, true);
            injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 83 EC 28"), SetPerspective, true);
            injector::MakeCALL(hook::get_pattern("E8 ? ? ? ? F3 0F 10 44 24 ? F3 0F 10 4C 24 ? F3 0F 10 7C 24 ? 0F 28 D1"), SetPerspective, true);
        }();

        // uCameraBase's projection: the FOV in degrees (+0x24) times pi/180 is in xmm0, the camera in ecx
        auto pattern = hook::pattern("D9 41 20 F3 0F 10 41 24 F3 0F 59 05 ? ? ? ? 83 EC 0C 56 8B 75 08 83 EC 10 D9 5C 24 0C F3 0F 11 44 24 08 D9 41 18 D9 5C 24 04 D9 41 1C 8B CE D9 1C 24 E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 83 EC 28");
        static auto& fFOVFactor = FixSettings().Float("MAIN.FOVFactor");
        static SafetyHookMid FOVHook = safetyhook::create_mid(pattern.get_first(16), [](SafetyHookContext& regs)
        {
            auto factor = std::clamp(fFOVFactor, 0.5f, 2.0f);
            if (factor == 1.0f || !IsGameplayCamera(regs.ecx))
                return;
            const auto aspect = *reinterpret_cast<const float*>(regs.ecx + 0x20);
            regs.xmm0.f32[0] = ScaleFOV(regs.xmm0.f32[0], aspect, factor);
        });
    }
}
