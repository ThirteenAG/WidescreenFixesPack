module;

#include <stdafx.h>

export module Camera;

import ComVars;
import Cinematic;

export namespace P_Camera
{
    enum
    {
        VIEWPORT = 0x1C4, // left, right, top and bottom
    };

    void(__fastcall* setFOV)(void* _this, void* edx, float fFOV) = nullptr;

    // What P_Camera::validate turns the horizontal FOV tangent into the vertical one with
    float GetViewportRatio(uint8_t* _this)
    {
        auto pViewport = (float*)(_this + VIEWPORT);
        float fRatio = (pViewport[3] - pViewport[2]) / (pViewport[1] - pViewport[0]);
        return std::isfinite(fRatio) && fRatio > 0.0f ? fRatio : 0.75f;
    }
}

namespace X_LevelRuntimeCamera
{
    // The FOV set on the level camera is the 4:3 one, P_Camera::validate widens it for the screen.
    // FOVFactor and the cutscene framing are applied here, so the skybox camera and the portal code,
    // which both read this camera's FOV, stay in sync.
    float CalculateFOV(float fFOV)
    {
        float fTan = tanf(fFOV * 0.5f);
        if (fTan <= 0.0f)
            return fFOV;

        float fZoom = Screen.fAspectScaleX;
        if (Screen.fFOVFactor != 1.0f)
        {
            float fBaseFOV = 2.0f * atanf(fTan * Screen.fAspectScaleX);
            // A rectilinear projection becomes unstable near 180 degrees; keep the unmodified view
            // and cap only the multiplier's expansion at 150 degrees.
            constexpr float fMaximumFOV = 2.61799388f;
            float fWideFOV = std::clamp(fBaseFOV * Screen.fFOVFactor, 0.01f, std::max(fBaseFOV, fMaximumFOV));
            fZoom = tanf(fWideFOV * 0.5f) / fTan;
        }

        // Cutscenes move to the vanilla framing along with the game's widescreen transition. Camera
        // overlays are 4:3 images with the gameplay view behind them.
        if (!Screen.bDrawBordersForCameraOverlay)
        {
            auto state = Cinematic::GetState(MaxPayne_GameMode::pInstance);
            if (state.fProgress > 0.0f)
                fZoom += (Cinematic::GetCutsceneZoom(state.fWideScreenMultiplier) - fZoom) * state.fProgress;
        }

        return 2.0f * atanf(fTan * fZoom / Screen.fAspectScaleX);
    }

    void __fastcall setFOV(void* pCamera, void* edx, float fFOV)
    {
        P_Camera::setFOV(pCamera, edx, CalculateFOV(fFOV));
    }

    // Portal culling in getFirstSceneToRender and getSceneToRender derives the frustum from the
    // camera FOV, so it needs the same scaling as P_Camera::validate (the mobile release does this
    // too). Without it rooms seen through portals get clipped to the 4:3 part of the screen.
    void getFirstSceneToRenderHook(SafetyHookContext& regs)
    {
        // [ebp-10h] is tan(fov / 2), st(0) is it multiplied by the viewport height
        *(float*)(regs.ebp - 0x10) *= Screen.fAspectScaleX;
        float fScaleY = Screen.fAspectScaleY;
        _asm fmul dword ptr[fScaleY]
    }

    void getSceneToRenderHook(SafetyHookContext& regs)
    {
        // st(1) is tan(fov / 2), st(0) is it multiplied by the viewport height
        float fScaleX = Screen.fAspectScaleX;
        float fScaleY = Screen.fAspectScaleY;
        _asm
        {
            fmul    dword ptr[fScaleY]
            fxch    st(1)
            fmul    dword ptr[fScaleX]
            fxch    st(1)
        }
    }
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            // FOV
            auto pattern = hook::pattern("8B 97 D0 05 00 00 52 8B CE FF 15"); // X_LevelRuntimeCamera camera setup
            P_Camera::setFOV = **pattern.get_first<decltype(P_Camera::setFOV)*>(11);
            injector::MakeCALL(pattern.get_first(9), X_LevelRuntimeCamera::setFOV, true);
            injector::MakeNOP(pattern.get_first(14), 1, true);

            pattern = hook::pattern("D9 87 FA 02 00 00 D8 A7 F6 02 00 00 C7 05");
            static auto getFirstSceneToRenderHook = safetyhook::create_mid(pattern.get_first(), X_LevelRuntimeCamera::getFirstSceneToRenderHook);

            pattern = hook::pattern("D9 86 FA 02 00 00 D8 A6 F6 02 00 00 DE F9 74");
            static auto getSceneToRenderHook = safetyhook::create_mid(pattern.get_first(), X_LevelRuntimeCamera::getSceneToRenderHook);
        };
    }
} Camera;
