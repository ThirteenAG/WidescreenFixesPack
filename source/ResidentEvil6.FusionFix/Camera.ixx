module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <unordered_map>
#include "CameraFOV.hpp"

export module Camera;

import ComVars;

// The game's projection setup (near, far, vertical FOV, aspect) is used by every camera: gameplay, cutscenes,
// shadows and light shafts. The game is Hor+ by itself.
// - Near clip: at wider aspects the near plane of the screen's cameras is moved closer, so the wider view doesn't cut
//   into walls next to the camera. Square projections (shadow maps of spot lights) are left alone.
// - MAIN.FOVFactor scales the normal and vehicle gameplay cameras; scripted, weapon and unknown camera types keep
//   their native projection.

namespace
{
    const float* pFOVFactor = nullptr;

    SafetyHookInline shSetPerspective = {};
    void __fastcall SetPerspective(float* matrix, void* edx, float zNear, float zFar, float fov, float aspect)
    {
        if (std::abs(aspect - 1.0f) > 0.01f)
        {
            if (IsSplitScreenActive())
                zNear /= SplitScreen::GetDiff();
            else
                zNear /= GetDiff();
        }
        shSetPerspective.unsafe_fastcall(matrix, edx, zNear, zFar, fov, aspect);
    }

    // The class name of an MT Framework object: vtable slot 4 returns its type info, which holds the name at +4
    const char* GetClassName(void* object)
    {
        if (!object)
            return nullptr;
        auto vtable = *(uintptr_t**)object;
        if (!vtable || !vtable[4])
            return nullptr;
        auto getDTI = (void* (__fastcall*)(void*))vtable[4];
        auto dti = (uintptr_t)getDTI(object);
        return dti ? *(const char**)(dti + 4) : nullptr;
    }

    bool IsGameplayCamera(void* camera)
    {
        static std::unordered_map<uintptr_t, bool> known;
        if (!camera)
            return false;
        auto vtable = *(uintptr_t*)camera;
        auto it = known.find(vtable);
        if (it != known.end())
            return it->second;

        auto name = GetClassName(camera);
        if (!name)
            return false;
        bool gameplay = CameraFOV::IsGameplayType(name);
        known.emplace(vtable, gameplay);
        return gameplay;
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

            auto pattern = hook::pattern("E8 ? ? ? ? 80 7D 10 00 74 23");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Camera"))
                shSetPerspective = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), SetPerspective);

            // uCameraBase's projection: ecx still holds the camera before the native mov ecx, esi; FOV is at esp+8
            pattern = hook::pattern("8B 74 24 18 8B CE F3 0F 11 04 24");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Camera"))
            {
                static auto CameraFOVHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    const float factor = *pFOVFactor;
                    if (factor == 1.0f || !std::isfinite(factor) || factor <= 0.0f || IsCutsceneViewActive())
                        return;
                    if (!IsGameplayCamera((void*)regs.ecx))
                        return;
                    float& fov = *reinterpret_cast<float*>(regs.esp + 8);
                    fov = CameraFOV::Scale(fov, *reinterpret_cast<float*>(regs.esp + 12), factor);
                });
            }
        };
    }
} Camera;
