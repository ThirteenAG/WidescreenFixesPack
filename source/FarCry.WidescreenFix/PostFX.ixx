module;

#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"
#include <d3d9.h>

export module PostFX;

import PostFXCore;

IDirect3DDevice9* pDevice = nullptr;

bool bPostFXEnabled = false;

export void InitDrawHook()
{
    if (!bPostFXEnabled)
        return;

    auto pattern = find_module_pattern(GetModuleHandle(L"Cry3DEngine"), "55 E8 ? ? ? ? 8B 0D ? ? ? ? ? ? FF 90", "48 8B CD E8 ? ? ? ? 48 8B 0D ? ? ? ? ? ? ? FF 90");
    if (pattern.empty())
    {
        OutputDebugStringA("Far Cry: post-processing draw signature unavailable.\n");
        return;
    }
    static auto DrawHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (!pDevice)
            return;

        CPostFX::RenderSMAA(pDevice);
        CPostFX::RenderGamma(pDevice);
    });
}

#ifndef _WIN64
SafetyHookInline shInvalidateDeviceObjects = {};
HRESULT __fastcall InvalidateDeviceObjects(void* a1, void* edx)
{
    CPostFX::Shutdown();
    CPostFX::OnDeviceReset();
    return shInvalidateDeviceObjects.unsafe_fastcall<HRESULT>(a1, edx);
}
#else
SafetyHookInline shInvalidateDeviceObjects = {};
HRESULT __fastcall InvalidateDeviceObjects(void* a1)
{
    CPostFX::Shutdown();
    CPostFX::OnDeviceReset();
    return shInvalidateDeviceObjects.unsafe_fastcall<HRESULT>(a1);
}
#endif

export void InitDeviceHook()
{
    if (!bPostFXEnabled)
        return;
#ifndef _WIN64
    if (!wfp::PatchPreflight(GetModuleHandleW(L"XRenderD3D9"), "Far Cry post-processing device: signatures unavailable; feature disabled.\n",
    {
        { "F6 86 ? ? ? ? ? 0F 85", 1 },
        { "E8 ? ? ? ? 8A 86 ? ? ? ? 84 C0 74 ? 8B CE", 1 },
    }))
        return;
#else
    if (!wfp::PatchPreflight(GetModuleHandleW(L"XRenderD3D9"), "Far Cry post-processing device: signatures unavailable; feature disabled.\n",
    {
        { "41 F6 84 24 ? ? ? ? ? 75 ? 48 8B 4D", 1 },
        { "E8 ? ? ? ? 80 BB ? ? ? ? ? 74", 1 },
    }))
        return;
#endif

    if (!bPostFXEnabled)
        return;

    #ifndef _WIN64
    auto pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "F6 86 ? ? ? ? ? 0F 85");
    static auto GetDevicePtrHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        pDevice = *(IDirect3DDevice9**)(regs.esi + 0x1C698);
    });

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "E8 ? ? ? ? 8A 86 ? ? ? ? 84 C0 74 ? 8B CE");
    shInvalidateDeviceObjects = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), InvalidateDeviceObjects);
    #else
    auto pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "41 F6 84 24 ? ? ? ? ? 75 ? 48 8B 4D");
    static auto GetDevicePtrHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        pDevice = *(IDirect3DDevice9**)(regs.r12 + 0x20E58);
    });

    pattern = hook::module_pattern(GetModuleHandle(L"XRenderD3D9"), "E8 ? ? ? ? 80 BB ? ? ? ? ? 74");
    shInvalidateDeviceObjects = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), InvalidateDeviceObjects);
    #endif
}

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            CPostFX::bConsoleGammaEnabled = settings.Bool("GRAPHICS.ConsoleGamma");
            CPostFX::bSmaaEnabled = settings.Int("GRAPHICS.AntiAliasing");

            if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
                return;

            bPostFXEnabled = true;
        };

        WFP::onShutdownEvent() += []()
        {
            CPostFX::Shutdown();
            pDevice = nullptr;
        };
    }
} PostFX;
