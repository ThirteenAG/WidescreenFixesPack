module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "CameraMath.hpp"
#include "RuntimeBridge.hpp"
#include <atomic>

export module FOV;

import Bindings;

import ComVars;

static std::atomic<float> FOVFactor{ 1.0f };
static SafetyHookInline SetFOVHook;
static GameRef<uintptr_t>* Renderer = nullptr;
static GameRef<uintptr_t>* GameCamera = nullptr;
static GameRef<int>* CameraCount = nullptr;
static GameRef<uintptr_t>* Players = nullptr;
static uintptr_t TrackingUpdate = 0;

static bool GameplayLens()
{
    if (CameraCount->get() <= 0 || !Players->get())
        return false;
    const auto camera = GameCamera->get();
    if (!camera || *reinterpret_cast<int*>(camera + 44) != 0 || *reinterpret_cast<int*>(camera + 2772) != 1)
        return false;
    const auto index = *reinterpret_cast<int*>(camera + 2768);
    if (index < 0 || index >= 4)
        return false;
    const auto transition = *reinterpret_cast<uintptr_t*>(camera + 592 + index * 544 + 528);
    if (!transition || (*reinterpret_cast<uintptr_t**>(transition))[2] != TrackingUpdate)
        return false;
    const auto settings = transition ? *reinterpret_cast<uintptr_t*>(transition + 400) : 0;
    const auto blueprint = settings ? *reinterpret_cast<uintptr_t*>(settings + 8) : 0;
    if (!blueprint || !(*reinterpret_cast<uint8_t*>(blueprint + 260) & 2))
        return false;
    for (const auto action : { 23, 24 })
    {
        const auto state = Players->get() + 6824 + action * 20;
        if (std::abs(*reinterpret_cast<float*>(state)) > 0.001f || *reinterpret_cast<uint32_t*>(state + 12))
            return false;
    }
    const auto getFOV = reinterpret_cast<double(__thiscall*)(uintptr_t)>((*reinterpret_cast<uintptr_t**>(settings))[19]);
    const auto intended = getFOV(settings);
    const auto actual = *reinterpret_cast<float*>(camera + 176);
    return std::isfinite(intended) && intended > 0.0 && std::abs(intended - actual) < 0.001;
}

struct OdinCamera
{
    bool m_bOrthographic;
    PblMatrix4x4 m_ViewMatrix;
    PblMatrix4x4 m_ViewMatrixInverse;
    PblMatrix4x4 m_ProjMatrix;
    PblMatrix4x4 m_ProjMatrixInverse;
    unsigned int m_Dirty;
    float m_NearPlane;
    float m_FarPlane;
    float m_TanHalfFOVWidth;
    float m_TanHalfFOVHeight;

    static void __fastcall SetFOV(OdinCamera* camera, void* edx, float a_FovX, float a_AspectRatio)
    {
        const auto renderer = Renderer->get();
        const auto mainCamera = renderer ? *reinterpret_cast<OdinCamera**>(renderer + 2980) : nullptr;
        if (camera != mainCamera)
        {
            SetFOVHook.unsafe_fastcall(camera, edx, a_FovX, a_AspectRatio);
            return;
        }
        const bool cinematic = SaboteurRuntime::Cinematic();
        const auto factor = !cinematic && GameplayLens() ? FOVFactor.load(std::memory_order_relaxed) : 1.0f;
        const auto projection = cinematic ? SaboteurCamera::Cinematic(a_FovX, a_AspectRatio, SaboteurRuntime::CutsceneBorders()) :
            SaboteurCamera::Widescreen(a_FovX, a_AspectRatio, factor);
        camera->m_Dirty |= 3u;
        camera->m_bOrthographic = false;
        camera->m_TanHalfFOVWidth = projection.Width;
        camera->m_TanHalfFOVHeight = projection.Height;
    }
};
static_assert(offsetof(OdinCamera, m_TanHalfFOVWidth) == 272);

class FOV
{
public:
    FOV()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            FOVFactor.store(settings.Float("MAIN.FOVFactor"), std::memory_order_relaxed);
            settings.OnChange("MAIN.FOVFactor", []()
            {
                FOVFactor.store(FixSettings().Float("MAIN.FOVFactor"), std::memory_order_relaxed);
            });
            auto pattern = hook::pattern("D9 44 24 04 56 DC 0D ? ? ? ? 8B F1 D9 5C 24 08");
            Renderer = SaboteurBindings::Renderer();
            auto cameras = hook::pattern("83 EC 50 56 57 8B 7C 24 5C 3B 3D ? ? ? ? 8B F1 7D 10 8B C7 69 C0 30 19 00 00 03 05 ? ? ? ? EB 02 33 C0 D9 40 50");
            Players = SaboteurBindings::Players();
            auto tracking = hook::pattern("53 D9 44 24 0C 56 57 8B 7C 24 10 0F B6 87 78 01 00 00 0F B6 D0 D0 EA 8B F1 8D 8F 68 01 00 00");
            CameraCount = BindGameRef(fovCameraCount, *cameras.get_first<int*>(11));
            GameCamera = BindGameRef(fovGameCamera, *cameras.get_first<uintptr_t*>(29));
            TrackingUpdate = reinterpret_cast<uintptr_t>(tracking.get_first());
            SetFOVHook = safetyhook::create_inline(pattern.get_first(), OdinCamera::SetFOV, safetyhook::InlineHook::StartDisabled);
            (void)SetFOVHook.enable();
        };
    }
} FOV;
