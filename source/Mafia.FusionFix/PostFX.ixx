module;

#include <stdafx.h>
#include <d3d9.h>

export module PostFX;

import PostFXCore;

static IUnknown** pD3D8Device = nullptr;

static void InitDevicePointer()
{
    if (pD3D8Device) return;

    auto hLS3DF = GetModuleHandle(L"LS3DF.dll");
    if (!hLS3DF) return;

    auto pattern = hook::module_pattern(hLS3DF, "A1 ? ? ? ? 53 6A ? 89 1D ? ? ? ? ? ? 50 FF 91 ? ? ? ? 8B 4C 24");
    if (!pattern.empty())
        pD3D8Device = *pattern.get_first<IUnknown**>(1);
}

export IDirect3DDevice9* GetDevice9()
{
    InitDevicePointer();

    if (!pD3D8Device || !*pD3D8Device) return nullptr;

    IDirect3DDevice9* dev9 = nullptr;
    if (FAILED((*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&dev9)))
        return nullptr;

    dev9->Release();
    return dev9;
}

export void InitPostFX()
{
    CIniReader iniReader("");
    CPostFX::bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 0) != 0;
    CPostFX::bSmaaEnabled = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;

    InitDevicePointer();

    if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
        return;

    auto hLS3DF = GetModuleHandle(L"LS3DF.dll");
    if (!hLS3DF || !pD3D8Device)
        return;

    auto pattern = hook::module_pattern(hLS3DF, "E8 ? ? ? ? 33 C9 ? ? ? ? ? ? ? E8 ? ? ? ? 6A ? 6A ? 6A ? FF 96 ? ? ? ? C6 05");
    if (!pattern.empty())
    {
        static auto RenderHook = safetyhook::create_mid(pattern.get_first(), +[](SafetyHookContext& regs)
        {
            auto dev = GetDevice9();
            CPostFX::RenderSMAA(dev);
            CPostFX::RenderGamma(dev);
        });
    }

    pattern = hook::module_pattern(hLS3DF, "56 8A D9 74 ? 8B 0D");
    if (!pattern.empty())
    {
        static auto ResetHook = safetyhook::create_mid(pattern.get_first(), +[](SafetyHookContext& regs)
        {
            CPostFX::Shutdown();
            CPostFX::OnDeviceReset();
        });
    }
}
