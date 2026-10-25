module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module Camera;

// The projection. The game builds every projection with one function (matrix, near, far, vertical FOV in radians,
// aspect), so the 3D view is already Hor+ at any aspect.
// - Near clip: the near plane is moved closer (divided by 1.6) at every caller, so the camera doesn't see through walls
//   when it gets close to them, in co-op too.
// - MAIN.FOVFactor (live) multiplies the vertical FOV of the gameplay camera (uCameraBase and its kinds), not of aiming
//   (uCameraWep, uCameraQFPS) and not of cutscenes (uMotionCamera).

namespace
{
    injector::memory_pointer_raw pSetPerspective;
    void __fastcall SetPerspective(float* _this, void* edx, float zNear, float zFar, float fovY, float aspect)
    {
        zNear /= 1280.0f / 800.0f;
        return injector::fastcall<void(float*, void*, float, float, float, float)>::call(pSetPerspective, _this, edx, zNear, zFar, fovY, aspect);
    }

    // The camera's class name, from its DTI (vtable slot 4 returns it; the name is at +4)
    bool IsAimCamera(uintptr_t camera)
    {
        static std::unordered_map<uintptr_t, bool> cache;
        auto vtable = *reinterpret_cast<uintptr_t*>(camera);
        auto it = cache.find(vtable);
        if (it != cache.end())
            return it->second;

        bool aim = false;
        auto getDTI = *reinterpret_cast<uintptr_t*(__fastcall**)(uintptr_t, void*)>(vtable + 4 * 4);
        if (auto dti = getDTI(camera, nullptr))
        {
            auto name = reinterpret_cast<const char*>(dti[1]);
            aim = name && (std::string_view(name) == "uCameraWep" || std::string_view(name) == "uCameraQFPS");
        }
        cache.emplace(vtable, aim);
        return aim;
    }
}

export namespace Camera
{
    void Init()
    {
        [&]()
        {
            if (!CheckPatterns("Resident Evil 5: near clipping", {
                "E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7",
                "E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 55",
                "E8 ? ? ? ? 80 BF ? ? ? ? ? 74 0E",
                "E8 ? ? ? ? F3 0F 10 AC 24 ? ? ? ? F3 0F 10 84 24 ? ? ? ? F3 0F 10 A4 24",
                "E8 ? ? ? ? 8D 97 ? ? ? ? 52 8D 87",
                "E8 ? ? ? ? 8B C6 5E 8B E5 5D C2 04 00 CC CC CC CC CC CC CC CC CC 83 EC 28",
                "E8 ? ? ? ? F3 0F 10 44 24 ? F3 0F 10 4C 24 ? F3 0F 10 7C 24 ? 0F 28 D1",
            }))
                return;

            auto pattern = hook::pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7");
            if (!PatternAvailable(pattern, "ResidentEvil5.FusionFix: Camera"))
                return;

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
        if (PatternUnique(pattern, "ResidentEvil5.FusionFix: Camera"))
        {
            static auto& fFOVFactor = FixSettings().Float("MAIN.FOVFactor");
            static SafetyHookMid FOVHook = safetyhook::create_mid(pattern.get_first(16), [](SafetyHookContext& regs)
            {
                auto factor = std::clamp(fFOVFactor, 0.5f, 2.0f);
                if (factor == 1.0f || IsAimCamera(regs.ecx))
                    return;
                regs.xmm0.f32[0] = std::min(regs.xmm0.f32[0] * factor, 170.0f * 0.017453292f);
            });
        }
    }
}
