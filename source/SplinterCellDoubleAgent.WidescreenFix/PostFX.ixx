module;

#include <stdafx.h>
#include <d3d9.h>

export module PostFX;

import ComVars;
import PostFXCore;

namespace UD3DRenderDevice
{
    ptrdiff_t pDeviceOffset = 0;
    SafetyHookInline shPresent = {};
    void __fastcall Present(void* UD3DRenderDevice, void* edx, void* UViewport, int a3)
    {
        auto device = *(IDirect3DDevice9**)((uintptr_t)UD3DRenderDevice + pDeviceOffset);
        //CPostFX::RenderSMAA(device);
        CPostFX::RenderGamma(device);
        return shPresent.unsafe_fastcall(UD3DRenderDevice, edx, UViewport, a3);
    }
}

export void InitPostFX()
{
    CIniReader iniReader("");
    CPostFX::bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 1) != 0;
    CPostFX::bSmaaEnabled = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;
    CPostFX::bBlurEnabled = iniReader.ReadInteger("GRAPHICS", "BlurEnabled", 1) != 0;
    CPostFX::fBlurStrength = iniReader.ReadFloat("GRAPHICS", "BlurStrength", 5.0f);
    CPostFX::bRenderToBackBuffer = true;

    if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bBlurEnabled && !CPostFX::bSmaaEnabled)
        return;

    auto pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B 8F ? ? ? ? ? ? 53");
    UD3DRenderDevice::pDeviceOffset = *pattern.get_first<int32_t>(2);

    UD3DRenderDevice::shPresent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?Present@UD3DRenderDevice@@UAEXPAVUViewport@@H@Z"), UD3DRenderDevice::Present);

    static auto resetDeviceHook = safetyhook::create_mid(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?resetDevice@UD3DRenderDevice@@QAEXAAU_D3DPRESENT_PARAMETERS_@@@Z"), [](SafetyHookContext& regs)
    {
        CPostFX::Shutdown();
        CPostFX::OnDeviceReset();
    });

    static void* UD3DRenderDevice = nullptr;
    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "0F 85 ? ? ? ? 8B AF");
    static auto RenderBlurHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UD3DRenderDevice = (void*)regs.edi;
    });

    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B 8C 24 ? ? ? ? 5F 5E 5D 5B 64 89 0D ? ? ? ? 81 C4 8C 00 00 00");
    static auto RenderBlurHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto device = *(IDirect3DDevice9**)((uintptr_t)UD3DRenderDevice + UD3DRenderDevice::pDeviceOffset);
        CPostFX::RenderBlur(device);
    });
}
