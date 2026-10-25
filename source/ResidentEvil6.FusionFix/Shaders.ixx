module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <d3d9.h>

export module Shaders;

import ComVars;

// Shaders of the full-screen overlays. The game lays out its overlays (low health, partner waiting, pause texts) and
// the low health frame for the 16:9 middle of the screen. Replacement vertex shaders (the shaders folder, assembled at
// build time) widen them to the whole screen; at 16:9 and narrower they draw exactly as the game's own.

namespace
{
    struct ShaderResource
    {
        uint32_t crc;
        int resourceId;
    };

    const ShaderResource VertexShaders[] =
    {
        { 0x4F0EE939, IDR_SHADER_OVERLAY_VS },          // 2D overlays: x scaled by c230.x
        { 0xF09EC5E9, IDR_SHADER_DAMAGEOVERLAY_VS },    // low health: the rotating blurred frame
    };

    IDirect3DVertexShader9* pOverlayShader = nullptr;
    IDirect3DVertexShader9* pDamageOverlayShader = nullptr;

    uint32_t GetShaderCrc(IUnknown* shader, bool vertex)
    {
        static std::vector<uint8_t> function;
        UINT len = 0;
        HRESULT hr = vertex ? ((IDirect3DVertexShader9*)shader)->GetFunction(nullptr, &len) : ((IDirect3DPixelShader9*)shader)->GetFunction(nullptr, &len);
        if (FAILED(hr) || !len)
            return 0;
        if (function.size() < len)
            function.resize(len);
        hr = vertex ? ((IDirect3DVertexShader9*)shader)->GetFunction(function.data(), &len) : ((IDirect3DPixelShader9*)shader)->GetFunction(function.data(), &len);
        if (FAILED(hr))
            return 0;

        return crc32(0, function.data(), len);
    }

    const DWORD* LoadShaderResource(int id)
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&LoadShaderResource, &module);
        auto res = FindResourceW(module, MAKEINTRESOURCEW(id), RT_RCDATA);
        auto data = res ? LoadResource(module, res) : nullptr;
        return data ? (const DWORD*)LockResource(data) : nullptr;
    }

    // The game's CreateVertexShader(shader resource): the bytecode at +8
    SafetyHookInline shCreateVertexShader = {};
    IDirect3DVertexShader9* __stdcall CreateVertexShader(const DWORD** resource)
    {
        auto shader = shCreateVertexShader.unsafe_stdcall<IDirect3DVertexShader9*>(resource);
        if (!shader)
            return shader;

        auto crc = GetShaderCrc(shader, true);
        for (auto& replacement : VertexShaders)
        {
            if (crc != replacement.crc)
                continue;

            auto device = GetD3DDevice();
            auto bytecode = LoadShaderResource(replacement.resourceId);
            IDirect3DVertexShader9* newShader = nullptr;
            if (device && bytecode && SUCCEEDED(device->CreateVertexShader(bytecode, &newShader)))
            {
                shader->Release();
                shader = newShader;
                if (crc == 0x4F0EE939)
                    pOverlayShader = shader;
                else
                    pDamageOverlayShader = shader;
            }
            break;
        }
        return shader;
    }
}

class Shaders
{
public:
    Shaders()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("E8 ? ? ? ? 89 47 08 0F B7 44 24");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Shaders"))
                shCreateVertexShader = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CreateVertexShader);

            // MAIN.DisableDamageOverlay / MAIN.DisableDBNOEffects: the player's condition skips the low health (20) and
            // the dying (10) states, whose screen effects then aren't shown
            auto& settings = FixSettings();
            if (settings.Bool("MAIN.DisableDamageOverlay"))
            {
                for (auto ref : { "C6 46 38 14 E9 ? ? ? ? F3 0F 10 05", "C6 46 38 14 8B 4E 40" })
                {
                    auto state = hook::pattern(ref);
                    if (PatternAvailable(state, "ResidentEvil6.FusionFix: Shaders"))
                        WriteGameValue<uint8_t>(state.get_first(3), 0x16);
                }
            }
            if (settings.Bool("MAIN.DisableDBNOEffects"))
            {
                auto state = hook::pattern("C6 46 38 0A 8B 4E 40");
                if (PatternAvailable(state, "ResidentEvil6.FusionFix: Shaders"))
                    WriteGameValue<uint8_t>(state.get_first(3), 0x16);
            }

            // Film grain (uTVNoiseFilter): the noise is tiled one texel per screen pixel and the rolling band is sized
            // and moved in pixels, both made for 720p: at higher resolutions the grain got finer and the band thinner
            // and slower. The fix keeps them at their 720p size relative to the screen height.
            pattern = hook::pattern("56 E8 ? ? ? ? F3 0F 10 05 ? ? ? ? 83 C4 20");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Shaders"))
            {
                // The noise's texture coordinate scale (viewport size / mask texture size) at esp+0x14 and esp+0x18
                static auto GrainScaleHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    const int32_t height = *(int32_t*)(regs.esi + 200) - *(int32_t*)(regs.esi + 192);
                    if (height <= 0)
                        return;
                    const float scale = 720.0f / (float)height;
                    *(float*)(regs.esp + 0x14) *= scale;
                    *(float*)(regs.esp + 0x18) *= scale;
                });
            }

            // The rolling band: its position, size and speed are in 720p pixels (the screen height at esp+0x1C)
            pattern = hook::pattern("F3 0F 10 8A 94 00 00 00 F3 0F 59 0D ? ? ? ?");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Shaders"))
            {
                static auto GrainBandHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    *(float*)(regs.esp + 0x1C) = 720.0f;
                });
            }

            // The renderer sets a new vertex shader (eax): the overlay shader's horizontal scale
            pattern = hook::pattern("89 47 24 8B 4D 00");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Shaders"))
            {
                static auto SetVertexShaderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto shader = (IDirect3DVertexShader9*)regs.eax;
                    if (!shader || (shader != pOverlayShader && shader != pDamageOverlayShader))
                        return;
                    float scale[4] = { IsSplitScreenActive() ? SplitScreen::GetDiff() : GetDiff(), 0.0f, 0.0f, 0.0f };
                    if (scale[0] < 1.0f)
                        scale[0] = 1.0f;
                    if (auto device = GetD3DDevice())
                        device->SetVertexShaderConstantF(230, scale, 1);
                });
            }
        };
    }
} Shaders;
