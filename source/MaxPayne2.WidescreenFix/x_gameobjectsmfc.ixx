module;

#include <stdafx.h>

export module x_gameobjectsmfc;

import ComVars;
import e2mfc;

namespace X_LevelRuntimeCamera
{
    void(__fastcall* setFOVOriginal)(void* _this, void* edx, float fFOV) = nullptr;

    // The FOV set on the level camera is the 4:3 one, P_Camera::validate widens it for the screen.
    // FOVFactor and the cutscene framing are applied here, so the skybox camera, mirrors and the
    // portal code, which all read this camera's FOV, stay in sync.
    float CalculateFOV(float fFOV)
    {
        float fTan = tanf(fFOV * 0.5f);
        if (fTan <= 0.0f)
            return fFOV;

        float fZoom = Screen.fAspectScaleX;
        if (Screen.fFOVFactor != 1.0f)
        {
            float fWideFOV = std::clamp(2.0f * atanf(fTan * Screen.fAspectScaleX) * Screen.fFOVFactor, 0.01f, 3.1f);
            fZoom = tanf(fWideFOV * 0.5f) / fTan;
        }

        // Cutscenes move to the vanilla framing along with the game's widescreen transition. Camera
        // overlays are 4:3 images with the gameplay view behind them.
        if (!Screen.bDrawBordersForCameraOverlay)
        {
            auto state = Cinematic::GetState(MP_GameMode::pInstance);
            if (state.fProgress > 0.0f)
                fZoom += (Cinematic::GetCutsceneZoom(state.fWideScreenMultiplier) - fZoom) * state.fProgress;
        }

        return 2.0f * atanf(fTan * fZoom / Screen.fAspectScaleX);
    }

    void __fastcall setFOV(void* pCamera, void* edx, float fFOV)
    {
        setFOVOriginal(pCamera, edx, CalculateFOV(fFOV));
    }

    // Portal culling in updateVisibility and the room sub-viewports derive the frustum from the
    // camera FOV, so they need the same scaling as P_Camera::validate. Without it rooms seen through
    // portals get clipped to the 4:3 part of the screen.
    void updateVisibilityHook(SafetyHookContext& regs)
    {
        *(float*)(regs.esp + 0x14) *= Screen.fAspectScaleX; // tan(fov / 2)
        *(float*)(regs.esp + 0x1C) *= Screen.fAspectScaleY; // the same for the viewport height
    }

    void setSubViewportHook(SafetyHookContext& regs)
    {
        *(float*)(regs.esp + 0x08) *= Screen.fAspectScaleX;
        *(float*)(regs.esp + 0x0C) *= Screen.fAspectScaleY;
    }
}

namespace X_Mirror
{
    // Mirrors build projection matrices of their own from the level camera's FOV and the 4:3 viewport
    // ratio, for the screen area the mirror covers and for projecting the mirror image onto it. Both
    // have to match the widened projection the scene and the mirror image are rendered with.
    // The FOV and the viewport ratio are the first two arguments pushed at this point.
    void WidenProjection(SafetyHookContext& regs)
    {
        auto& fFOV = *(float*)(regs.esp + 0x00);
        auto& fViewportRatio = *(float*)(regs.esp + 0x04);
        fFOV = 2.0f * atanf(tanf(fFOV * 0.5f) * Screen.fAspectScaleX);
        fViewportRatio *= Screen.fAspectScaleY / Screen.fAspectScaleX;
    }
}

export void InitX_GameObjectsMFC()
{
    auto X_GameObjectsMFC = GetModuleHandle(L"X_GameObjectsMFC");

    // FOV
    auto pattern = hook::module_pattern(X_GameObjectsMFC, "8B 97 38 05 00 00 52 8B CE FF 15"); // X_LevelRuntimeCamera::getCamera
    X_LevelRuntimeCamera::setFOVOriginal = **pattern.get_first<decltype(X_LevelRuntimeCamera::setFOVOriginal)*>(11);
    injector::MakeCALL(pattern.get_first(9), X_LevelRuntimeCamera::setFOV, true); //0x10004292
    injector::MakeNOP(pattern.get_first(14), 1, true);

    pattern = hook::module_pattern(X_GameObjectsMFC, "8B 8D 28 05 00 00 D9 05 ? ? ? ? 89 4C 24 18"); // X_LevelRuntimeCamera::updateVisibility
    static auto updateVisibilityHook = safetyhook::create_mid(pattern.get_first(), X_LevelRuntimeCamera::updateVisibilityHook); //0x1000AD4F

    pattern = hook::module_pattern(X_GameObjectsMFC, "8B 87 9C 05 00 00 8B 4C 24 14 8B 49 5C"); // sub-viewport of a room seen through a portal
    static auto setSubViewportHook = safetyhook::create_mid(pattern.get_first(), X_LevelRuntimeCamera::setSubViewportHook); //0x10005FE0

    // Mirrors
    pattern = hook::module_pattern(X_GameObjectsMFC, "68 00 00 40 3F 56 8D 44 24 ? 50 8B CF E8"); // screen area of the mirror
    static auto X_MirrorScreenAreaHook = safetyhook::create_mid(pattern.get_first(6), X_Mirror::WidenProjection); //0x101020BE
    pattern = hook::module_pattern(X_GameObjectsMFC, "68 00 00 40 3F 50 8D 4C 24 ? 51 8B CB E8"); // texture matrix, X_Mirror::update
    static auto X_MirrorTextureMatrixHook = safetyhook::create_mid(pattern.get_first(6), X_Mirror::WidenProjection); //0x10104DBB

    // Dynamic character shadows render into square textures with their own cameras
    pattern = hook::module_pattern(X_GameObjectsMFC, "8B 15 ? ? ? ? 8B 0D ? ? ? ? 52 FF 15 ? ? ? ? 32 C9 FF D7 8B 0D ? ? ? ? E8");
    static auto ppShadowEdgeFader = *pattern.get_first<uint8_t**>(25);
    static auto X_DynamicCharacterShadowRenderHook = safetyhook::create_mid(pattern.get_first(12), [](SafetyHookContext& regs) //0x10073D30
    {
        KeepOriginalProjection((void*)regs.edx);
        if (*ppShadowEdgeFader)
            KeepOriginalProjection(*(void**)(*ppShadowEdgeFader + 0x10));
    });

    pattern = hook::module_pattern(X_GameObjectsMFC, "A0 ? ? ? ? 84 C0 0F 85"); //byte_101A7AA0
    X_Crosshair::sm_bCameraPathRunning.SetAddress(*pattern.get_first<bool*>(1));

    pattern = hook::module_pattern(X_GameObjectsMFC, "B1 01 88 46 65 E8 ? ? ? ? 5E C2 08 00");
    struct CameraOverlayHook
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.bDrawBordersForCameraOverlay = false;
            *((BYTE*)&(regs.ecx)) = 1;
            *(uint8_t*)(regs.esi + 0x65) = LOBYTE(regs.eax);

            auto x = *(uint32_t*)(regs.esi + 0x48);
            auto y = *(uint32_t*)(regs.esi + 0x4C);
            auto z = *(uint32_t*)(regs.esi + 0x50);

            //what happens here is check for some camera coordinates
            if ((x == 0x40d9d740 && y == 0x40d9d95a && z == 0xc24f706a) || (x == 0x405b016c && y == 0x40d69c24 && z == 0xc1a50336) || (x == 0xc0a3f326 && y == 0x40ee9c24 && z == 0xc2101fe9) || //https://i.imgur.com/Kn7lHIc.png
                (x == 0xc1564e4e && y == 0x406865f0 && z == 0xc253cb06) ||																														 // https://i.imgur.com/7Z0aaKz.png
                (x == 0xc01d03ff && y == 0x40e39c24 && z == 0x42ce75c2) || (x == 0xc10a916a && y == 0x412ee03e && z == 0x42cc4f35) || (x == 0x4117f993 && y == 0x418ee709 && z == 0x424c1fe9)    //https://i.imgur.com/7aw4nNh.png
                )
            {
                Screen.bDrawBordersForCameraOverlay = true;
            }
        }
    }; injector::MakeInline<CameraOverlayHook>(pattern.get_first(0)); // 100E19B7
}
