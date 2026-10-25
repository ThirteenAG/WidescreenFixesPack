module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <d3d9.h>

export module Shaders;

import ComVars;

// Shader replacements (the shaders folder, compiled at build time):
// - 2D overlays (vertex shader 4F0EE939): widened to the whole screen.
// - Film grain: kept at its 720p size at any resolution.
// - Movies: the game stretches the 16:9 video over the whole screen. A second screen vertex shader, used only while
//   the movie pixel shader draws, keeps its aspect (MAIN.FMVWidescreenMode: 0 = the whole video with black bars,
//   1 = filling the screen, cropped; 2 = stretched as the game does).

namespace
{
    IDirect3DVertexShader9* pScreenVertexShader = nullptr;   // the game's full-screen quad
    IDirect3DPixelShader9* pMoviePixelShader = nullptr;      // the WMV YUV decoder
    IDirect3DVertexShader9* pMovieVertexShader = nullptr;    // the fix's scaled quad
    const int* pFMVWidescreenMode = nullptr;
    bool bMovieVertexShaderBound = false;                    // set for one draw, restored right after it

    uint32_t GetShaderCrc(IUnknown* shader, bool vertex)
    {
        std::vector<uint8_t> function;
        UINT len = 0;
        HRESULT hr = vertex ? ((IDirect3DVertexShader9*)shader)->GetFunction(nullptr, &len) : ((IDirect3DPixelShader9*)shader)->GetFunction(nullptr, &len);
        if (FAILED(hr) || !len)
            return 0;
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

    // The game's CreateVertexShader(shader resource) and CreatePixelShader: the bytecode at +8
    SafetyHookInline shCreateVertexShader = {};
    IDirect3DVertexShader9* __stdcall CreateVertexShader(const DWORD** resource)
    {
        auto shader = shCreateVertexShader.unsafe_stdcall<IDirect3DVertexShader9*>(resource);
        if (!shader)
            return shader;

        IDirect3DDevice9* device = nullptr;
        if (FAILED(shader->GetDevice(&device)) || !device)
            return shader;
        device->Release();

        switch (GetShaderCrc(shader, true))
        {
        case 0x4F0EE939:
            if (auto bytecode = LoadShaderResource(IDR_SHADER_OVERLAY_VS))
            {
                IDirect3DVertexShader9* replacement = nullptr;
                if (SUCCEEDED(device->CreateVertexShader(bytecode, &replacement)))
                {
                    shader->Release();
                    shader = replacement;
                }
            }
            break;
        case 0x1287841D:
            pScreenVertexShader = shader;
            if (!pMovieVertexShader)
            {
                if (auto bytecode = LoadShaderResource(IDR_SHADER_MOVIE_VS))
                    device->CreateVertexShader(bytecode, &pMovieVertexShader);
            }
            break;
        }
        return shader;
    }

    SafetyHookInline shCreatePixelShader = {};
    IDirect3DPixelShader9* __stdcall CreatePixelShader(const DWORD** resource)
    {
        auto shader = shCreatePixelShader.unsafe_stdcall<IDirect3DPixelShader9*>(resource);
        if (shader && GetShaderCrc(shader, false) == 0x9D190FC7)
            pMoviePixelShader = shader;
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
            pFMVWidescreenMode = &FixSettings().Int("MAIN.FMVWidescreenMode");

            // Film grain (uTVNoiseFilter): the noise is tiled one texel per screen pixel and the rolling band is sized
            // and moved in pixels, both made for 720p: at higher resolutions the grain got finer and the band thinner
            // and slower. The fix keeps them at their 720p size relative to the screen height.
            if (FixSettings().Bool("MAIN.DisableFilmGrain"))
            {
                // Its noise and mask textures aren't found, so it isn't drawn
                for (auto name : { R"(system\filter\TVNoise_LIN)", R"(system\filter\TVMask_LIN)" })
                {
                    std::string bytes;
                    for (auto c = name; *c; ++c)
                        bytes += std::format("{:02X} ", (uint8_t)*c);
                    bytes += "00";
                    auto pattern = hook::pattern(bytes);
                    if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Shaders"))
                        WriteGameValue<uint8_t>(pattern.get_first(), 'r');
                }
            }
            else
            {
                // The noise's texture coordinate scale (xmm1, xmm2 = viewport size / mask texture size), esi = the context
                auto pattern = hook::pattern("A1 ? ? ? ? 83 EC 14 8B CC F3 0F 11 54 24 10");
                if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Shaders"))
                {
                    static auto GrainScaleHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                    {
                        const int32_t height = *(int32_t*)(regs.esi + 200) - *(int32_t*)(regs.esi + 192);
                        if (height <= 0)
                            return;
                        const float scale = 720.0f / (float)height;
                        regs.xmm1.f32[0] *= scale;
                        regs.xmm2.f32[0] *= scale;
                    });
                }

                // The rolling band: its position, size and speed are in 720p pixels (xmm1 = the screen height)
                pattern = hook::pattern("F3 0F 10 4C 24 20 0F 28 C3 F3 0F 58 C3");
                if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Shaders"))
                {
                    static auto GrainBandHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                    {
                        regs.xmm1.f32[0] = 720.0f;
                    });
                }
            }

            auto pattern = hook::pattern("E8 ? ? ? ? 89 47 08 66 8B 4C 24 30");
            if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Shaders"))
                shCreateVertexShader = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CreateVertexShader);

            pattern = hook::pattern("E8 ? ? ? ? 89 47 08 66 8B 4C 24 32");
            if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Shaders"))
                shCreatePixelShader = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CreatePixelShader);

            // DrawPrimitive of the screen quads (edi = the device)
            pattern = hook::pattern("57 FF 90 44 01 00 00 8B 9C 24 60 0E 00 00");
            if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Shaders"))
            {
                static auto DrawPrimitiveHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    auto device = (IDirect3DDevice9*)regs.edi;
                    if (!device || !pMovieVertexShader || !pMoviePixelShader)
                        return;

                    IDirect3DPixelShader9* ps = nullptr;
                    if (FAILED(device->GetPixelShader(&ps)))
                        return;
                    if (ps)
                        ps->Release();
                    if (ps != pMoviePixelShader)
                        return;

                    IDirect3DVertexShader9* vs = nullptr;
                    if (FAILED(device->GetVertexShader(&vs)))
                        return;
                    if (vs)
                        vs->Release();
                    if (vs != pScreenVertexShader)
                        return;

                    const float ratio = GetAspectRatio() / defaultAspectRatio;  // > 1: the screen is wider than the video
                    float scale[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
                    switch (*pFMVWidescreenMode)
                    {
                    case 0: // the whole video
                        if (ratio > 1.0f) scale[0] = 1.0f / ratio; else scale[1] = ratio;
                        break;
                    case 1: // filling the screen
                        if (ratio > 1.0f) scale[1] = ratio; else scale[0] = 1.0f / ratio;
                        break;
                    default:
                        return;
                    }
                    device->SetVertexShader(pMovieVertexShader);
                    device->SetVertexShaderConstantF(20, scale, 1);
                    bMovieVertexShaderBound = true;
                });

                // After the draw: give the game its own shader back, it may skip setting a shader it thinks is bound
                static auto DrawPrimitiveDoneHook = safetyhook::create_mid(pattern.get_first(7), [](SafetyHookContext& regs)
                {
                    if (!bMovieVertexShaderBound)
                        return;
                    bMovieVertexShaderBound = false;
                    if (auto device = (IDirect3DDevice9*)regs.edi)
                        device->SetVertexShader(pScreenVertexShader);
                });
            }
        };
    }
} Shaders;
