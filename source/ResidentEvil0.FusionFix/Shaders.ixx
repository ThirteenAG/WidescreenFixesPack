module;

#include <stdafx.h>
#include <d3d9.h>
#include <cstring>
#include <string_view>
#include "FixSettings.hpp"

export module Shaders;

import ComVars;

// The game's post effects. The TV noise filter (grain, scanlines and the rolling blank bar) is drawn in screen
// pixels: one noise texel and one scanline mask texel per pixel, the bar's height and speed in pixels. At 1440p and
// above the grain and scanlines shrank to a fraction of what they are at 1080p and the bar got short and slow; they
// are now scaled by whole multiples of the screen height over 1080, so 1080p and below stay as they were.
// Same engine build as Resident Evil HD: the filter and its draw code are identical.
// Noise and colour correction use replacement shaders. Fog is suppressed in the known composite shader while the
// game's depth-copy shader remains intact for effects that use depth.

namespace
{
    bool bDisableNoise = false;
    bool bDisableColorCorrection = false;
    bool bDisableFog = false;

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
    };

    bool SuppressMaterialFog(std::vector<DWORD>& code, size_t length)
    {
        if (length < 8 || length % sizeof(DWORD) || code[0] != 0xFFFF0300)
            return false;
        const size_t words = length / sizeof(DWORD);
        int fogRegister = -1;
        size_t candidate = 0;
        size_t matches = 0;
        DWORD zero = 0;
        bool haveZero = false;
        size_t pending = 0;
        DWORD factorSource = 0, factorMask = 0;
        for (size_t i = 1; i < words;)
        {
            const DWORD token = code[i];
            const DWORD opcode = token & 0xFFFF;
            if (opcode == 0xFFFF)
                break;
            if (opcode == 0xFFFE)
            {
                const size_t count = (token >> 16) & 0x7FFF;
                if (count > words - i - 1)
                    return false;
                if (count >= 8 && code[i + 1] == 0x42415443)
                {
                    const auto* base = reinterpret_cast<const uint8_t*>(&code[i + 2]);
                    const size_t size = (count - 1) * sizeof(DWORD);
                    auto u32 = [base](size_t offset) { DWORD value; std::memcpy(&value, base + offset, sizeof(value)); return value; };
                    auto u16 = [base](size_t offset) { uint16_t value; std::memcpy(&value, base + offset, sizeof(value)); return value; };
                    const size_t entries = u32(12), table = u32(16);
                    if (table > size || entries > (size - table) / 20)
                        return false;
                    for (size_t j = 0; j < entries; ++j)
                    {
                        const size_t entry = table + j * 20, name = u32(entry);
                        if (name >= size)
                            return false;
                        const auto end = static_cast<const uint8_t*>(std::memchr(base + name, 0, size - name));
                        if (!end)
                            return false;
                        if (std::string_view(reinterpret_cast<const char*>(base + name), end - (base + name)) == "CBFog__packed0" && u16(entry + 4) == 2 && u16(entry + 8) == 1)
                            fogRegister = u16(entry + 6);
                    }
                }
                i += count + 1;
                continue;
            }
            const size_t count = (token >> 24) & 15;
            if (count > words - i - 1)
                return false;
            if (!haveZero && token == 0x05000051 && count == 5)
            {
                for (DWORD component = 0; component < 4; ++component)
                {
                    if ((code[i + 2 + component] & 0x7FFFFFFF) == 0)
                    {
                        zero = 0xA0000000 | (component * 0x55 << 16) | (code[i + 1] & 0x7FF);
                        haveZero = true;
                        break;
                    }
                }
            }
            if (pending)
            {
                // Fog either blends colour or fades the material's alpha by (1 - fog).
                if ((token == 0x04000012 && count == 4 && code[i + 2] == factorSource &&
                    (code[i + 3] & 0xFFFFE000) == 0xA0E40000 && (code[i + 3] & 0x7FF) == (code[pending + 3] & 0x7FF)) ||
                    (token == 0x04000004 && count == 4 && code[i + 2] == factorSource &&
                    (code[i + 1] == 0x80080800 || code[i + 1] == code[pending + 1]) &&
                    (code[i + 3] ^ code[i + 4]) == 0x01000000 && (code[i + 3] & 0x01000000)))
                {
                    candidate = pending;
                    ++matches;
                    pending = 0;
                }
                else
                {
                    // Only straight-line arithmetic and texture reads with no writes to the fog-factor component are safe.
                    switch (opcode)
                    {
                        case 1: case 2: case 4: case 5: case 6: case 7: case 8: case 9:
                        case 11: case 14: case 15: case 18: case 32: case 36: case 66: case 88:
                            if (!count || ((code[i + 1] & 0xF00007FF) == (0x80000000 | (code[pending + 1] & 0x7FF)) && (code[i + 1] & factorMask)))
                                pending = 0;
                            break;
                        default:
                            pending = 0;
                            break;
                    }
                }
            }
            if (token == 0x03000005 && count == 3 && fogRegister >= 0 &&
                (code[i + 1] & 0xFFF0F800) == 0x80000000 &&
                ((code[i + 1] & 0x000F0000) == 0x00010000 || (code[i + 1] & 0x000F0000) == 0x00020000 ||
                    (code[i + 1] & 0x000F0000) == 0x00040000 || (code[i + 1] & 0x000F0000) == 0x00080000) &&
                (code[i + 3] & 0xFFFFE000) == 0xA0FF0000 && (code[i + 3] & 0x7FF) == DWORD(fogRegister))
            {
                const DWORD mask = code[i + 1] & 0x000F0000;
                const DWORD component = mask == 0x00010000 ? 0 : mask == 0x00020000 ? 1 : mask == 0x00040000 ? 2 : 3;
                const DWORD source = 0x80000000 | (component * 0x55 << 16) | (code[i + 1] & 0x7FF);
                if (code[i + 2] == source)
                {
                    pending = i;
                    factorSource = source;
                    factorMask = code[i + 1] & 0x000F0000;
                }
            }
            i += count + 1;
        }
        if (matches != 1 || fogRegister < 0 || (code[candidate + 3] & 0x7FF) != DWORD(fogRegister))
            return false;
        if (haveZero)
            code[candidate + 3] = zero;
        else
        {
            // The saturated fog factor cancels itself when no zero constant is available.
            if (candidate < 4 || code[candidate - 4] != 0x03000005 ||
                code[candidate - 3] != (code[candidate + 1] | 0x00100000) || code[candidate - 2] != code[candidate + 2])
                return false;
            code[candidate] = 0x03000002;
            code[candidate + 3] = code[candidate + 2] | 0x01000000;
        }
        return true;
    }

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
        if (!entry || !entry[2] || !sRender.is_initialized() || !sRender)
            return nullptr;

        auto pDevice = *reinterpret_cast<IDirect3DDevice9**>(sRender + 0x98);
        if (!pDevice)
            return nullptr;
        IDirect3DPixelShader9* pShader = nullptr;
        pDevice->CreatePixelShader(entry[2], &pShader);
        if (!pShader)
            return nullptr;

        UINT len = 0;
        if (FAILED(pShader->GetFunction(nullptr, &len)) || !len)
            return pShader;
        std::vector<DWORD> pbFunc((len + sizeof(DWORD) - 1) / sizeof(DWORD));
        if (FAILED(pShader->GetFunction(pbFunc.data(), &len)))
            return pShader;

        auto crc = crc32(0, reinterpret_cast<const uint8_t*>(pbFunc.data()), len);
        if (bDisableFog && SuppressMaterialFog(pbFunc, len))
        {
            IDirect3DPixelShader9* replacement = nullptr;
            if (SUCCEEDED(pDevice->CreatePixelShader(pbFunc.data(), &replacement)) && replacement)
            {
                pShader->Release();
                return replacement;
            }
            return pShader;
        }
        if (bDisableFog && crc == 0x2213B2AC && len == 736)
        {
            // c0.w is zero in this shader. It replaces the fog factor without altering the depth copy.
            pbFunc[0x230 / sizeof(DWORD)] = 0xA0FF0000;
            IDirect3DPixelShader9* replacement = nullptr;
            if (SUCCEEDED(pDevice->CreatePixelShader(pbFunc.data(), &replacement)) && replacement)
            {
                pShader->Release();
                return replacement;
            }
            return pShader;
        }
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

class Shaders
{
public:
    Shaders()
    {
        WFP::onInitEvent() += []()
        {
            bDisableNoise = FixSettings().Bool("MAIN.DisableNoise") != 0;
            bDisableColorCorrection = FixSettings().Bool("MAIN.DisableColorCorrection") != 0;
            bDisableFog = FixSettings().Bool("MAIN.DisableFog") != 0;

            if (bDisableNoise || bDisableColorCorrection || bDisableFog)
            {
                auto device = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 0F B6 C0 F7 D8");
                auto create = hook::pattern("E8 ? ? ? ? 89 46 08 66 8B 4C 24 ? 0F B7 C1 43 83 C5 0C 3B D8 72 C7 33 C0");
                sRender.SetAddress(*device.get_first<uint8_t**>(2));
                injector::MakeCALL(create.get_first(), CreatePixelShaderHook, true);
            }

            // Depth of field in the scenes, as on the consoles: the scenes place the game's depth of field filter
            // (uGameDOFFilter, and the bokeh filter built on it), but this build creates it switched off (+0xA5) and
            // never switches it on
            if (FixSettings().Bool("MAIN.DepthOfField"))
            {
                auto dof = hook::pattern("89 86 AC 00 00 00 C6 86 A5 00 00 00 01");
                injector::WriteMemory<uint8_t>(dof.get_first(12), 0, true);
            }

            // TV noise filter: the texture coordinates of the noise and scanline mask are the viewport size over the
            // mask texture size (arguments 7 and 8 of the quad call), the viewport is at +188 of the draw context (esi)
            auto noiseQuad = hook::pattern("56 E8 ? ? ? ? 83 C4 20 80 7F 74 00");
            auto blankSize = hook::pattern("80 7F 74 00 0F 84 ? ? ? ? F3 0F 10 9F 84 00 00 00 F3 0F 59");
            auto blankSpeed = hook::pattern("F3 0F 10 46 78 F3 0F 5C 46 7C");
            static SafetyHookMid QuadHook = safetyhook::create_mid(noiseQuad.get_first(), [](SafetyHookContext& regs)
            {
                auto viewport = reinterpret_cast<int32_t*>(regs.esi + 188);
                UpdateNoiseScale(viewport[3] - viewport[1]);
                *reinterpret_cast<float*>(regs.esp + 0x14) /= NoiseScale;
                *reinterpret_cast<float*>(regs.esp + 0x18) /= NoiseScale;
            });

            // The blank bar's height (+0x84) is loaded before the multiply.
            static SafetyHookMid SizeHook = safetyhook::create_mid(blankSize.get_first(18), [](SafetyHookContext& regs)
            {
                regs.xmm3.f32[0] *= NoiseScale;
            });

            // The bar's position moves by its speed (+0x7C) in every update
            static SafetyHookMid SpeedHook = safetyhook::create_mid(blankSpeed.get_first(5), [](SafetyHookContext& regs)
            {
                regs.xmm0.f32[0] -= *reinterpret_cast<float*>(regs.esi + 0x7C) * (NoiseScale - 1.0f);
            });
        };
    }
} Shaders;
