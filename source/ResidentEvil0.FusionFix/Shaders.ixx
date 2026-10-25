module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module Shaders;

// The game's post effects. The TV noise filter (grain, scanlines and the rolling blank bar) is drawn in screen
// pixels: one noise texel and one scanline mask texel per pixel, the bar's height and speed in pixels. At 1440p and
// above the grain and scanlines shrank to a fraction of what they are at 1080p and the bar got short and slow; they
// are now scaled by whole multiples of the screen height over 1080, so 1080p and below stay as they were.
// Same engine build as Resident Evil HD: the filter and its draw code are identical.
// The noise, colour correction and fog passes can be turned off (MAIN.DisableNoise, MAIN.DisableColorCorrection,
// MAIN.DisableFog): the game's pixel shaders are swapped for the versions in NoNoise.ps, NoColorCorrection.ps and
// NoFog.ps, assembled at build time and embedded in the plugin.

namespace
{
    bool bDisableNoise = false;
    bool bDisableColorCorrection = false;
    bool bDisableFog = false;
    uint8_t** ppRender = nullptr; // sRender, its device at +0x98

    struct Replacement
    {
        uint32_t crc;
        const char* resource;
        bool* enabled;
    };

    Replacement Replacements[] =
    {
        { 0x59FA1317, "NOCOLORCORRECTION", &bDisableColorCorrection },
        { 0x2C3893DD, "NONOISE", &bDisableNoise },
        { 0xE8C35AD8, "NOFOG", &bDisableFog },
    };

    const DWORD* LoadShader(const char* name)
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&LoadShader), &module);
        HRSRC resource = FindResourceA(module, name, MAKEINTRESOURCEA(10));
        HGLOBAL handle = resource ? LoadResource(module, resource) : nullptr;
        return handle ? static_cast<const DWORD*>(LockResource(handle)) : nullptr;
    }

    // Replaces the game's call that creates a pixel shader from its shader package entry (bytecode at entry[2])
    IDirect3DPixelShader9* __stdcall CreatePixelShaderHook(const DWORD** entry)
    {
        if (!entry || !entry[2] || !ppRender || !*ppRender)
            return nullptr;

        auto pDevice = *reinterpret_cast<IDirect3DDevice9**>(*ppRender + 0x98);
        if (!pDevice)
            return nullptr;
        IDirect3DPixelShader9* pShader = nullptr;
        pDevice->CreatePixelShader(entry[2], &pShader);
        if (!pShader)
            return nullptr;

        static std::vector<uint8_t> pbFunc;
        UINT len = 0;
        if (FAILED(pShader->GetFunction(nullptr, &len)) || !len)
            return pShader;
        if (pbFunc.size() < len)
            pbFunc.resize(len);
        if (FAILED(pShader->GetFunction(pbFunc.data(), &len)))
            return pShader;

        auto crc = crc32(0, pbFunc.data(), len);
        for (auto& it : Replacements)
        {
            if (it.crc != crc || !*it.enabled)
                continue;

            IDirect3DPixelShader9* replacement = nullptr;
            auto code = LoadShader(it.resource);
            if (code && SUCCEEDED(pDevice->CreatePixelShader(code, &replacement)) && replacement)
            {
                pShader->Release();
                return replacement;
            }
            break;
        }
        return pShader;
    }

    // Whole multiples of 1080p, at least 1
    float NoiseScale = 1.0f;
    void UpdateNoiseScale(int32_t height)
    {
        NoiseScale = float(std::max(1, (height + 540) / 1080));
    }
}

export namespace Shaders
{
    void Init()
    {
        bDisableNoise = FixSettings().Bool("MAIN.DisableNoise") != 0;
        bDisableColorCorrection = FixSettings().Bool("MAIN.DisableColorCorrection") != 0;
        bDisableFog = FixSettings().Bool("MAIN.DisableFog") != 0;

        if (bDisableNoise || bDisableColorCorrection || bDisableFog)
        {
            auto device = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 0F B6 C0 F7 D8");
            auto create = hook::pattern("E8 ? ? ? ? 89 46 08 66 8B 4C 24 ? 0F B7 C1 43 83 C5 0C 3B D8 72 C7 33 C0");
            if (PatternAvailable(device, "ResidentEvil0.FusionFix: Shaders") && PatternAvailable(create, "ResidentEvil0.FusionFix: Shaders"))
            {
                ppRender = *device.get_first<uint8_t**>(2);
                injector::MakeCALL(create.get_first(), CreatePixelShaderHook, true);
            }
        }

        // TV noise filter: the texture coordinates of the noise and scanline mask are the viewport size over the
        // mask texture size (arguments 7 and 8 of the quad call), the viewport is at +188 of the draw context (esi)
        auto noiseQuad = hook::pattern("56 E8 ? ? ? ? 83 C4 20 80 7F 74 00");
        auto blankSize = hook::pattern("80 7F 74 00 0F 84 ? ? ? ? F3 0F 10 ? 84 00 00 00 F3 0F 59");
        auto blankSpeed = hook::pattern("F3 0F 10 46 78 F3 0F 5C 46 7C");
        if (PatternUnique(noiseQuad, "ResidentEvil0.FusionFix: noise scaling") &&
            PatternUnique(blankSize, "ResidentEvil0.FusionFix: noise scaling") && PatternUnique(blankSpeed, "ResidentEvil0.FusionFix: noise scaling"))
        {
            static SafetyHookMid QuadHook = safetyhook::create_mid(noiseQuad.get_first(), [](SafetyHookContext& regs)
            {
                auto viewport = reinterpret_cast<int32_t*>(regs.esi + 188);
                UpdateNoiseScale(viewport[3] - viewport[1]);
                *reinterpret_cast<float*>(regs.esp + 0x14) /= NoiseScale;
                *reinterpret_cast<float*>(regs.esp + 0x18) /= NoiseScale;
            });

            // The blank bar's height (+0x84), loaded into the register named by the movss before the multiply
            static uint8_t SizeRegister = (*blankSize.get_first<uint8_t>(13) >> 3) & 7;
            static SafetyHookMid SizeHook = safetyhook::create_mid(blankSize.get_first(18), [](SafetyHookContext& regs)
            {
                (&regs.xmm0)[SizeRegister].f32[0] *= NoiseScale;
            });

            // The bar's position moves by its speed (+0x7C) in every update
            static SafetyHookMid SpeedHook = safetyhook::create_mid(blankSpeed.get_first(5), [](SafetyHookContext& regs)
            {
                regs.xmm0.f32[0] -= *reinterpret_cast<float*>(regs.esi + 0x7C) * (NoiseScale - 1.0f);
            });
        }
    }
}
