module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <d3d9.h>

export module PostFX;

import PostFXCore;

IDirect3DDevice9* pDevice = nullptr;

bool bPostFXEnabled = false;
void (*BeforeHud)(IDirect3DDevice9*) = nullptr;

export bool InitDrawHook(void (*beforeHud)(IDirect3DDevice9*))
{
    BeforeHud = beforeHud;

    // Found by the code before the hooked call, which other plugins hooking the same place (Xbox Rain Droplets) leave intact
    #ifndef _WIN64
    auto pattern = hook::module_pattern(GetModuleHandle(L"Cry3DEngine"), "33 DB EB 08 6A FF 55 E8");
    constexpr ptrdiff_t DrawOffset = 12;
    #else
    auto pattern = hook::module_pattern(GetModuleHandle(L"Cry3DEngine"), "80 38 00 0F 85 ? ? ? ? BA FF FF FF FF 48 8B CD E8");
    constexpr ptrdiff_t DrawOffset = 22;
    #endif
    static auto DrawHook = safetyhook::create_mid(pattern.get_first(DrawOffset), [](SafetyHookContext& regs)
    {
        if (!pDevice)
            return;

        if (bPostFXEnabled)
        {
            CPostFX::RenderSMAA(pDevice);
            CPostFX::RenderGamma(pDevice);
        }
        if (BeforeHud)
            BeforeHud(pDevice);
    });
    return bool(DrawHook);
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

export bool InitDeviceHook()
{
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
    return bool(GetDevicePtrHook) && bool(shInvalidateDeviceObjects);
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
