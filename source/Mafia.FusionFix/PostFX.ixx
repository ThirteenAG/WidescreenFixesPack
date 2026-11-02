module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "callargs.h"

export module PostFX;

import ComVars;
import PostFXCore;

export void InitDevicePointer()
{
    if (pD3D8Device.is_initialized()) return;

    auto hLS3DF = GetLS3DF();
    if (!hLS3DF) return;

    static auto pattern = hook::module_pattern(hLS3DF, "A1 ? ? ? ? 53 6A ? 89 1D ? ? ? ? ? ? 50 FF 91 ? ? ? ? 8B 4C 24");
    pD3D8Device.SetAddress(*pattern.get_first<IUnknown**>(1));
}

export IDirect3DDevice9* GetDevice9()
{
    if (!pD3D8Device.is_initialized() || !pD3D8Device) return nullptr;

    IDirect3DDevice9* dev9 = nullptr;
    if (FAILED(pD3D8Device->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&dev9)))
        return nullptr;

    dev9->Release();
    return dev9;
}

export void InitPostFX()
{
    CPostFX::bConsoleGammaEnabled = FixSettings().Bool("GRAPHICS.ConsoleGamma") != 0;
    CPostFX::bSmaaEnabled = FixSettings().Bool("GRAPHICS.AntiAliasing") != 0;

    InitDevicePointer();

    if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bSmaaEnabled)
        return;

    auto hLS3DF = GetLS3DF();
    if (!hLS3DF || !pD3D8Device.is_initialized())
        return;

    auto pattern = hook::module_pattern(hLS3DF, "E8 ? ? ? ? 33 C9 ? ? ? ? ? ? ? E8 ? ? ? ? 6A ? 6A ? 6A ? FF 96 ? ? ? ? C6 05");
    {
        static auto RenderHook = safetyhook::create_mid(pattern.get_first(), +[](SafetyHookContext& regs)
        {
            auto dev = GetDevice9();
            CPostFX::RenderSMAA(dev);
            CPostFX::RenderGamma(dev);
        });
    }

    pattern = hook::module_pattern(hLS3DF, "56 8A D9 74 ? 8B 0D");
    {
        static auto ResetHook = safetyhook::create_mid(pattern.get_first(), +[](SafetyHookContext& regs)
        {
            CPostFX::Shutdown();
            CPostFX::OnDeviceReset();
        });
    }
}

// Shadows Z-Fighting
export void InitShadowZBias()
{
    static std::vector<SafetyHookMid> zbiasHooks;

    std::vector<std::pair<std::string, int>> patterns = {
        { "FF 92 ? ? ? ? A1 ? ? ? ? ? ? 6A ? 6A ? 50 FF 91 ? ? ? ? F6 86", 0 },
        { "FF 92 ? ? ? ? A1 ? ? ? ? ? ? 6A ? 6A ? 50 FF 91 ? ? ? ? 8B 96", 0 },
        { "FF 92 ? ? ? ? ? ? ? ? 56", 0 },
        { "51 6A ? 89 0D ? ? ? ? ? ? 50 FF 92 ? ? ? ? C3", 12 },
        { "FF A2 ? ? ? ? 8B 44 24 ? 39 05 ? ? ? ? 0F 84 ? ? ? ? 8B 0D ? ? ? ? 50 6A ? 6A ? A3 ? ? ? ? ? ? 51 FF 92 ? ? ? ? 5B C2 ? ? 8B 44 24 ? 39 05 ? ? ? ? 0F 84 ? ? ? ? 8B 0D ? ? ? ? 50 6A ? 6A ? A3 ? ? ? ? ? ? 51 FF 92 ? ? ? ? 5B C2 ? ? 8B 44 24 ? 8B 0D", 0 },
    };

    const bool oldEngine = hook::module_pattern(GetLS3DF(), patterns[4].first).empty(); // 1.0
    for (size_t i = 0; i < patterns.size(); ++i)
    {
        const auto& p = patterns[i];
        auto pattern = hook::module_pattern(GetLS3DF(), p.first);
        if (i == 4 && oldEngine) // 1.0 has no equivalent site
            continue;
        const size_t sites = i == 2 && oldEngine ? 2 : 1; // duplicated in 1.0
        for (size_t site = 0; site < sites; ++site)
            zbiasHooks.emplace_back(safetyhook::create_mid(pattern.get(site).get<void>(p.second), [](SafetyHookContext& regs)
        {
            constexpr auto D3DRS_ZBIAS = 47;
            constexpr auto D3DRS_ZVISIBLE = 30;
            using SetRenderState = HRESULT(__stdcall*)(void* pDevice, DWORD State, DWORD Value);
            auto [pDevice, State, Value] = deduce_args<SetRenderState>(regs);

            if (State != D3DRS_ZBIAS)
                return;

            if (Value)
                Value = 16;

            // On real D3D8 the driver biases slope dependently, so ZBIAS alone fixes the flickering.
            // d3d8to9 only turns it into a constant D3DRS_DEPTHBIAS, so set the bias on the D3D9 device
            // ourselves and hand d3d8to9 a state it ignores (ZVISIBLE returns D3D_OK at once), otherwise
            // its own conversion would overwrite the values set here.
            auto pDevice9 = GetDevice9();
            if (!pDevice9)
                return;

            constexpr float fDepthBiasUnit = -1.0f / ((1 << 20) - 1); // 24-bit depth, same as CalcDepthBias
            const float fDepthBias = Value * fDepthBiasUnit;
            const float fSlopeDepthBias = -0.125f * static_cast<float>(Value); // per ZBIAS unit, -2.0 at 16
            pDevice9->SetRenderState(D3DRS_DEPTHBIAS, *reinterpret_cast<const DWORD*>(&fDepthBias));
            pDevice9->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *reinterpret_cast<const DWORD*>(&fSlopeDepthBias));

            State = D3DRS_ZVISIBLE;
        }));
    }
}
