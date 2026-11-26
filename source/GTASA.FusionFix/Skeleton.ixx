module;

#include <stdafx.h>
#include <chrono>
#include <cstdint>
#include "common.h"
#include <d3d9.h>

export module Skeleton;

import Entity;

enum RsInputDeviceType : int
{
    rsKEYBOARD,
    rsMOUSE,
    rsPAD
};

enum RsEventStatus : int
{
    rsEVENTERROR,
    rsEVENTPROCESSED,
    rsEVENTNOTPROCESSED
};

enum RsEvent : int
{
    rsCAMERASIZE,
    rsCOMMANDLINE,
    rsFILELOAD,
    rsINITDEBUG,
    rsINPUTDEVICEATTACH,
    rsLEFTBUTTONDOWN,
    rsLEFTBUTTONUP,
    rsMOUSEMOVE,
    rsMOUSEWHEELMOVE,
    rsPLUGINATTACH,
    rsREGISTERIMAGELOADER,
    rsRIGHTBUTTONDOWN,
    rsRIGHTBUTTONUP,
    _rs_13,
    _rs_14,
    _rs_15,
    _rs_16,
    _rs_17,
    _rs_18,
    _rs_19,
    _rs_20,
    rsRWINITIALIZE,
    rsRWTERMINATE,
    rsSELECTDEVICE,
    rsINITIALIZE,
    rsTERMINATE,
    rsIDLE,
    rsFRONTENDIDLE,
    rsKEYDOWN,
    rsKEYUP,
    rsQUITAPP,
    rsPADBUTTONDOWN,
    rsPADBUTTONUP,
    rsPADANALOGUELEFT,
    rsPADANALOGUELEFTRESET,
    rsPADANALOGUERIGHT,
    rsPADANALOGUERIGHTRESET,
    rsPREINITCOMMANDLINE,
    rsACTIVATE,
};

using RsInputEventHandler = RsEventStatus(*)(RsEvent event, void* param);

struct RsInputDevice
{
    RsInputDeviceType inputDeviceType;
    bool              used;
    RsInputEventHandler inputEventHandler;
};

export struct RsGlobalType
{
    const char* appName;
    union
    {
        int32_t maximumWidth;
        int32_t width;
    };
    union
    {
        int32_t maximumHeight;
        int32_t height;
    };
    int32_t maxFPS;
    bool  quit;
    void* ps;
    RsInputDevice keyboard;
    RsInputDevice mouse;
    RsInputDevice pad;
};

export GameRef<RsGlobalType> RsGlobal([]() -> RsGlobalType*
{
    auto pattern = hook::pattern("A1 ? ? ? ? 51 2B 54 24");
    return *pattern.get_first<RsGlobalType*>(1);
});

export namespace CGeneral
{
    unsigned int __cdecl GetRandomNumber()
    {
        return rand();
    }
}

export CEntity* (__cdecl* FindPlayerPed)(int number) = nullptr;
export CEntity* (__cdecl* FindPlayerVehicle)(int playerNum, bool bIncludeRemote) = nullptr;
export void (__cdecl* RwCameraSetNearClipPlane)(RwCamera* camera, float nearClip) = nullptr;

export GameRef<IDirect3DDevice9*> pD3D9Device;

export GameRef<RwGlobals*> RwEngineInstance;

export void RwRenderStateSet(RwRenderState nState, void* pParam)
{
    RwEngineInstance->dOpenDevice.fpRenderStateSet(nState, pParam);
}

namespace
{
    void InstallRasterResetFix()
    {
        const auto intervalReset = hook::pattern("C7 05 ? ? ? ? 00 00 00 80 E8 ? ? ? ? A1 ? ? ? ? 68 ? ? ? ? "
            "50 8B 10 FF 52 40 A1 ? ? ? ?").get_first(31);
        const auto lostReset = hook::pattern("81 FB 69 08 76 88 0F 85 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 68 ? ? ? ? "
            "50 8B 10 FF 52 40 A1 ? ? ? ?").get_first(33);
        static const auto returnFromRasterShow = reinterpret_cast<uintptr_t>(hook::pattern("A1 ? ? ? ? 85 C0 74 02 FF D0 E8 ? ? ? ? "
            "33 C0 5F 85 DB 5E 5B 0F 9D C0 C3").get_first(16));
        static const auto guard = [](SafetyHookContext& regs)
        {
            if (FAILED(static_cast<HRESULT>(regs.eax)))
            {
                // The device is still lost. Leave surface lookup and resource restoration to the next reset.
                regs.ebx = regs.eax;
                regs.eip = returnFromRasterShow;
            }
        };
        static SafetyHookMid intervalGuard = safetyhook::create_mid(intervalReset, guard);
        static SafetyHookMid lostGuard = safetyhook::create_mid(lostReset, guard);
    }

    struct ShadowCamera
    {
        RwCamera* camera;
        RwTexture* texture;
    };

    struct RealTimeShadow
    {
        uint8_t beforeCamera[8];
        ShadowCamera camera;
        uint8_t beforeBlurCamera[4];
        ShadowCamera blurCamera;
    };

    struct RealTimeShadowManager
    {
        static constexpr size_t shadowCount = 16;

        bool initialized;
        bool needsReinit;
        RealTimeShadow* shadows[shadowCount];
        ShadowCamera blurCamera;
        ShadowCamera gradientCamera;
    };

    static_assert(offsetof(RealTimeShadow, blurCamera) == 0x14);
    static_assert(offsetof(RealTimeShadowManager, shadows) == 0x04);
    static_assert(offsetof(RealTimeShadowManager, gradientCamera) == 0x4C);
    static_assert(offsetof(RwCamera, frameBuffer) == 0x60);

    RwRaster* (__cdecl* CreateShadowRaster)(int width, int height, int depth, int flags);
    int (__cdecl* DestroyShadowRaster)(RwRaster* raster);
    RwTexture* (__cdecl* SetShadowTextureRaster)(RwTexture* texture, RwRaster* raster);
    RwRaster* (__thiscall* RenderShadowGradient)(ShadowCamera* camera);
    void (__thiscall* UpdateRealTimeShadows)(RealTimeShadowManager* manager);

    bool RefreshShadowRaster(ShadowCamera& shadow)
    {
        if (!shadow.camera || !shadow.camera->frameBuffer || !shadow.texture)
            return false;

        constexpr int cameraTexture = 5;
        auto* oldRaster = shadow.camera->frameBuffer;
        auto* raster = CreateShadowRaster(oldRaster->width, oldRaster->height, 0, cameraTexture);
        if (!raster)
            return false;

        if (!SetShadowTextureRaster(shadow.texture, raster))
        {
            DestroyShadowRaster(raster);
            return false;
        }

        shadow.camera->frameBuffer = raster;
        DestroyShadowRaster(oldRaster);
        return true;
    }

    class ShadowReset
    {
        static constexpr size_t pedRasterCount = RealTimeShadowManager::shadowCount * 2;
        static constexpr size_t rasterCount = pedRasterCount + 2;
        static constexpr auto retryInterval = std::chrono::milliseconds(100);

        size_t nextRaster = 0;
        bool pending = false;
        std::chrono::steady_clock::time_point retryAfter{};

        static ShadowCamera* CameraAt(RealTimeShadowManager& manager, size_t index)
        {
            if (index == pedRasterCount)
                return &manager.blurCamera;
            if (index == pedRasterCount + 1)
                return &manager.gradientCamera;

            auto* shadow = manager.shadows[index / 2];
            if (!shadow)
                return nullptr;
            return index % 2 ? &shadow->blurCamera : &shadow->camera;
        }

    public:
        bool Ready(RealTimeShadowManager& manager)
        {
            if (!manager.initialized)
            {
                pending = false;
                return true;
            }

            // A fresh device reset invalidates any work completed for the previous one.
            if (std::exchange(manager.needsReinit, false))
            {
                nextRaster = 0;
                retryAfter = {};
                pending = true;
            }

            if (!pending)
                return true;
            if (std::chrono::steady_clock::now() < retryAfter)
                return false;

            // Keep the successful rasters and restart at the first one still needing work.
            for (; nextRaster < rasterCount; ++nextRaster)
            {
                auto* camera = CameraAt(manager, nextRaster);
                if (!camera || !RefreshShadowRaster(*camera))
                {
                    retryAfter = std::chrono::steady_clock::now() + retryInterval;
                    return false;
                }
            }

            if (!RenderShadowGradient(&manager.gradientCamera))
            {
                retryAfter = std::chrono::steady_clock::now() + retryInterval;
                return false;
            }

            pending = false;
            return true;
        }
    };

    void __fastcall UpdateShadows(RealTimeShadowManager* manager, void*)
    {
        // A failed reset leaves the device's resources released until a later one succeeds. The shadow
        // cameras would draw into rasters without a texture; they run again after the reset.
        if (auto* device = *pD3D9Device; device && device->TestCooperativeLevel() != D3D_OK)
            return;

        static ShadowReset reset;
        if (reset.Ready(*manager))
            UpdateRealTimeShadows(manager);
    }

    void InstallShadowResetFix()
    {
        const auto reinit = hook::pattern("8B 46 4C 8B 48 60 8B 79 0C 83 C6 4C 51 89 68 60 E8 ? ? ? ? "
            "6A 05 55 57 57 E8 ? ? ? ? 8B 0E 89 41 60 8B 56 04 50 52 E8 ? ? ? ? 83 C4 38 5F 8B CE 5E 5D 5B 83 C4 08 E9")
            .get_first<uint8_t>();
        DestroyShadowRaster = reinterpret_cast<decltype(DestroyShadowRaster)>(injector::GetBranchDestination(reinit + 16).as_int());
        CreateShadowRaster = reinterpret_cast<decltype(CreateShadowRaster)>(injector::GetBranchDestination(reinit + 26).as_int());
        SetShadowTextureRaster = reinterpret_cast<decltype(SetShadowTextureRaster)>(injector::GetBranchDestination(reinit + 41).as_int());
        RenderShadowGradient = reinterpret_cast<decltype(RenderShadowGradient)>(injector::GetBranchDestination(reinit + 58).as_int());

        auto update = hook::pattern("E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 ? B8 FF 00").get_first(10);
        UpdateRealTimeShadows = reinterpret_cast<decltype(UpdateRealTimeShadows)>(injector::GetBranchDestination(update).as_int());
        injector::MakeCALL(update, UpdateShadows, true);
    }
}

class Skeleton
{
public:
    Skeleton()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("A1 ? ? ? ? 8B 10 6A 00 8D 7B");
            pD3D9Device.SetAddress(*pattern.get_first<IDirect3DDevice9**>(1));

            pattern = find_pattern("E8 ? ? ? ? 8B 80 ? ? ? ? 83 C4 ? 85 C0 74 ? ? ? EB ? 33 C0 8B 40 ? 83 F8 ? 0F 8E");
            FindPlayerPed = (decltype(FindPlayerPed))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("E8 ? ? ? ? 83 C4 ? 85 C0 74 ? 83 C0 ? ? ? 8B 50 ? 8B 40 ? 89 4C 24 ? ? ? ? ? ? ? ? ? ? ? 89 54 24");
            FindPlayerVehicle = (decltype(FindPlayerVehicle))injector::GetBranchDestination(pattern.get_first()).as_int();

            pattern = find_pattern("8B 0D ? ? ? ? 56 6A ? FF 51 ? 83 C4");
            RwEngineInstance.SetAddress(*pattern.get_first<RwGlobals**>(2));

            InstallRasterResetFix();
            InstallShadowResetFix();
        };
    }
} Skeleton;