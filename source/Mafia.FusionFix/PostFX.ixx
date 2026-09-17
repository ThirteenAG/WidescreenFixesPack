module;

#include <stdafx.h>
#include <d3d9.h>

export module PostFX;

import PostFXCore;

// LS3DF.dll is the D3D8 renderer of the game. The first operand of the pattern
// below is the global that holds the IDirect3DDevice8 pointer, it is only set
// once the device was created (i.e. after this code), hence the dereference in
// GetDevice9 instead of here.
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

// Returns the D3D9 device when the game renders through a D3D8 to D3D9 wrapper
// (thirteenag's d3d8.dll with UseD3D8to9=1), which forwards the query to the real
// D3D9 device. A native D3D8 device does not answer this query, so a null return
// means the game runs on real D3D8 (which needs no translation at all).
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

    // resolved unconditionally, other modules (the Z-bias fix in dllmain.cpp) need it too
    InitDevicePointer();

    if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
        return;

    auto hLS3DF = GetModuleHandle(L"LS3DF.dll");
    if (!hLS3DF || !pD3D8Device)
        return;

    // All rendering happens in I3D_driver::Render() (BeginScene ... EndScene).
    // This call is the last one before IDirect3DDevice8::EndScene, the 3D scene
    // is fully rendered into the render target at this point. The call itself
    // renders the remaining 2D pass (HUD/menus), which is intentionally left
    // untouched by the post processing. rendering.
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

    // This function calls IDirect3DDevice8::Reset, so all default pool resources
    // must be released before it runs.
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
