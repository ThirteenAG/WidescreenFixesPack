module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "RTTI.h"
#include <d3d9.h>
#include <wrl/client.h>

export module CharacterBlood;

namespace
{
    using Microsoft::WRL::ComPtr;
    SafetyHookInline primitiveHook, hardwareHook;
    uintptr_t characterVtable = 0, textureVtable = 0;
    bool enabled = false;
    ComPtr<IDirect3DPixelShader9> bloodPixelShader;
    IDirect3DDevice9* shaderDevice = nullptr;
    struct VertexReplacement
    {
        ComPtr<IDirect3DVertexShader9> original, blood;
    };
    std::vector<VertexReplacement> replacements;
    constexpr UINT damageRegister = 128, damageRegisters = 32 * 3;
    struct Palette
    {
        // Three registers per joint match the original skin shader's index stride.
        std::array<float, damageRegisters * 4> constants{};
        bool damaged = false;
    };
    thread_local const Palette* currentPalette = nullptr;
    thread_local void* currentMaterial = nullptr;
    thread_local bool rendering = false;

    template<class T> T Field(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    int __fastcall DrawPrimitiveGroup(void* self, void*)
    {
        // PrimGroupSkinnedOptimized::Display (0x6A4E00), slot 8.
        // Read the source bytes, not the averaged m[15] produced by 0x6A45A0.
        Palette palette;
        auto state = Field<void*>(self, 80);
        auto bones = Field<void*>(self, 84);
        if (state && bones)
        {
            auto begin = Field<const uint32_t*>(bones, 8);
            auto end = Field<const uint32_t*>(bones, 12);
            const auto count = (reinterpret_cast<uintptr_t>(end) - reinterpret_cast<uintptr_t>(begin)) / sizeof(uint32_t);
            auto front = Field<const uint8_t*>(state, 20);
            auto back = Field<const uint8_t*>(state, 24);
            const auto total = Field<uint32_t>(state, 28);
            if (begin && end >= begin && count <= 32 && front && back)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    if (begin[i] >= total) continue;
                    palette.constants[i * 12] = front[begin[i]] / 255.0f;
                    palette.constants[i * 12 + 1] = back[begin[i]] / 255.0f;
                    palette.damaged |= front[begin[i]] != 0 || back[begin[i]] != 0;
                }
            }
        }
        const auto previous = std::exchange(currentPalette, &palette);
        const auto result = primitiveHook.thiscall<int>(self);
        currentPalette = previous;
        return result;
    }

    int __fastcall DrawHardwareSkin(void* self, void*, void* material, void* buffer)
    {
        // d3dExtHardwareSkinning::Draw (0x650E10), slot 6.
        const auto previous = std::exchange(currentMaterial,
            material && Field<uintptr_t>(material, 0) == characterVtable ? material : nullptr);
        const auto result = hardwareHook.thiscall<int>(self, material, buffer);
        currentMaterial = previous;
        return result;
    }

    // ps_1_1: tex t0; mul r0, t0, v0
    constexpr DWORD bloodPixelCode[] = { D3DPS_VERSION(1, 1), 0x00000042, 0xB00F0000,
        0x00000005, 0x800F0000, 0xB0E40000, 0x90E40000, D3DSIO_END };

    int SourceCount(DWORD opcode)
    {
        switch (opcode)
        {
        case D3DSIO_MOV: case D3DSIO_RCP: case D3DSIO_RSQ: case D3DSIO_EXP: case D3DSIO_LOG:
        case D3DSIO_LIT: case D3DSIO_FRC: case D3DSIO_EXPP: case D3DSIO_LOGP:
            return 1;
        case D3DSIO_ADD: case D3DSIO_SUB: case D3DSIO_MUL: case D3DSIO_DP3: case D3DSIO_DP4:
        case D3DSIO_MIN: case D3DSIO_MAX: case D3DSIO_SLT: case D3DSIO_SGE: case D3DSIO_DST:
        case D3DSIO_M4x4: case D3DSIO_M4x3: case D3DSIO_M3x4: case D3DSIO_M3x3: case D3DSIO_M3x2:
            return 2;
        case D3DSIO_MAD:
            return 3;
        default:
            return -1;
        }
    }

    std::vector<DWORD> BloodVertexCode(const DWORD* code, size_t count)
    {
        // Rewrites the vs_1_1 skin program as vs_2_0 token by token, so the palette at c128
        // fits without borrowing lighting/bone constants, and no shader compiler is needed.
        constexpr DWORD addressX = 0x80000000 | D3DSPR_ADDR << D3DSP_REGTYPE_SHIFT;
        // PS2 0x76CC18 selects the first palette bone's front/back damage by normal Z.
        // 0x76EAC8/0x76EB10 supply this value as the second texture pass's alpha.
        static constexpr DWORD damage[] =
        {
            0x03000005, 0x80010000, 0x90000002, 0xA0AA0000,                     // mul r0.x, v2.x, c0.z
            0x0200002E, 0xB0010000, 0x80000000,                                 // mova a0.x, r0.x
            0x0300000C, 0x80020000, 0x90AA0003, 0xA0000000,                     // slt r0.y, v3.z, c0.x
            0x03000002, 0x80040000, 0xA0550000, 0x81550000,                     // add r0.z, c0.y, -r0.y
            0x04000005, 0x80020000, 0x80550000, 0xA0002080, addressX,           // mul r0.y, r0.y, c128[a0.x].x
            0x05000004, 0xD0080000, 0x80AA0000, 0xA0552080, addressX, 0x80550000, // mad oD0.w, r0.z, c128[a0.x].y, r0.y
        };
        if (count < 2 || code[0] != D3DVS_VERSION(1, 1)) return {};
        std::vector<DWORD> out{ D3DVS_VERSION(2, 0) };
        for (size_t i = 1; i < count;)
        {
            const DWORD token = code[i];
            const DWORD opcode = token & D3DSI_OPCODE_MASK;
            if (token == D3DSIO_END)
            {
                out.insert(out.end(), std::begin(damage), std::end(damage));
                out.push_back(D3DSIO_END);
                return out;
            }
            if (opcode == D3DSIO_COMMENT)
            {
                i += 1 + ((token & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT);
                continue;
            }
            const size_t at = out.size();
            out.push_back(opcode);
            if (opcode == D3DSIO_DCL || opcode == D3DSIO_DEF)
            {
                const size_t size = opcode == D3DSIO_DCL ? 2 : 5;
                if (i + 1 + size > count) return {};
                out.insert(out.end(), code + i + 1, code + i + 1 + size);
                i += 1 + size;
            }
            else if (opcode == D3DSIO_NOP)
            {
                ++i;
            }
            else
            {
                const int sources = SourceCount(opcode);
                if (sources < 0 || i + 2 + sources > count) return {};
                const DWORD dest = code[i + 1];
                const DWORD type = (dest & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT |
                                   (dest & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2;
                // vs_2_0 loads a0 with mova and names a0.x after every relative source.
                if (opcode == D3DSIO_MOV && type == D3DSPR_ADDR) out[at] = D3DSIO_MOVA;
                out.push_back(dest);
                for (int s = 0; s < sources; ++s)
                {
                    const DWORD source = code[i + 2 + s];
                    out.push_back(source);
                    if (source & D3DVS_ADDRMODE_RELATIVE) out.push_back(addressX);
                }
                i += 2 + sources;
            }
            // vs_2_0 instructions carry their length.
            out[at] |= DWORD(out.size() - at - 1) << D3DSI_INSTLENGTH_SHIFT;
        }
        return {};
    }

    IDirect3DVertexShader9* GetBloodVertex(IDirect3DDevice9* device, IDirect3DVertexShader9* shader)
    {
        for (auto& entry : replacements)
            if (entry.original.Get() == shader) return entry.blood.Get();
        // Only the two verified skin programs. Retain their COM identities to avoid pointer reuse.
        if (!shader || replacements.size() >= 16) return nullptr;
        VertexReplacement entry;
        entry.original = shader;
        UINT size = 0;
        if (SUCCEEDED(shader->GetFunction(nullptr, &size)) && size && size <= 65536)
        {
            std::vector<DWORD> bytes((size + 3) / 4);
            if (SUCCEEDED(shader->GetFunction(bytes.data(), &size)))
            {
                auto hash = crc32(0, bytes.data(), size);
                if (hash == 0xD41E262E || hash == 0xDF075E41)
                {
                    const auto code = BloodVertexCode(bytes.data(), size / sizeof(DWORD));
                    if (!code.empty()) device->CreateVertexShader(code.data(), entry.blood.GetAddressOf());
                }
            }
        }
        replacements.push_back(std::move(entry));
        return replacements.back().blood.Get();
    }

    struct SavedState
    {
        IDirect3DDevice9* device;
        ComPtr<IDirect3DVertexShader9> vertex;
        ComPtr<IDirect3DPixelShader9> pixel;
        ComPtr<IDirect3DBaseTexture9> texture;
        std::array<float, damageRegisters * 4> constants;
        static constexpr D3DRENDERSTATETYPE states[] = { D3DRS_ALPHABLENDENABLE, D3DRS_SRCBLEND,
            D3DRS_DESTBLEND, D3DRS_BLENDOP, D3DRS_ZWRITEENABLE, D3DRS_ZFUNC, D3DRS_ALPHATESTENABLE,
            D3DRS_COLORWRITEENABLE };
        std::array<DWORD, std::size(states)> values{};
        DWORD addressU = 0, addressV = 0;
        bool valid = false;
        explicit SavedState(IDirect3DDevice9* d) : device(d)
        {
            if (FAILED(d->GetVertexShader(vertex.GetAddressOf())) || FAILED(d->GetPixelShader(pixel.GetAddressOf())) ||
                FAILED(d->GetTexture(0, texture.GetAddressOf())) ||
                FAILED(d->GetVertexShaderConstantF(damageRegister, constants.data(), damageRegisters)) ||
                FAILED(d->GetSamplerState(0, D3DSAMP_ADDRESSU, &addressU)) ||
                FAILED(d->GetSamplerState(0, D3DSAMP_ADDRESSV, &addressV))) return;
            for (size_t i = 0; i < values.size(); ++i)
                if (FAILED(d->GetRenderState(states[i], &values[i]))) return;
            valid = true;
        }
        ~SavedState()
        {
            if (!valid) return;
            device->SetVertexShader(vertex.Get());
            device->SetPixelShader(pixel.Get());
            device->SetTexture(0, texture.Get());
            device->SetVertexShaderConstantF(damageRegister, constants.data(), damageRegisters);
            for (size_t i = 0; i < values.size(); ++i) device->SetRenderState(states[i], values[i]);
            device->SetSamplerState(0, D3DSAMP_ADDRESSU, addressU);
            device->SetSamplerState(0, D3DSAMP_ADDRESSV, addressV);
        }
    };
}

export namespace CharacterBlood
{
    bool IsEnabled() { return enabled; }

    bool Initialize()
    {
        auto& settings = FixSettings();
        const bool bRestoreCharacterBlood = settings.Bool("GRAPHICS.RestoreCharacterBlood");
        if (!bRestoreCharacterBlood) return false;
        characterVtable = ScarfaceRTTI::FindVtable(".?AVd3dCharacterShader@pure3d@@");
        textureVtable = ScarfaceRTTI::FindVtable(".?AVd3dTexture@pure3d@@");
        auto primitive = ScarfaceRTTI::FindVtable(".?AVPrimGroupSkinnedOptimized@pure3d@@");
        auto hardware = ScarfaceRTTI::FindVtable(".?AVd3dExtHardwareSkinning@pure3d@@");
        primitiveHook = safetyhook::create_inline(*reinterpret_cast<void**>(primitive + 8 * sizeof(void*)), DrawPrimitiveGroup);
        hardwareHook = safetyhook::create_inline(*reinterpret_cast<void**>(hardware + 6 * sizeof(void*)), DrawHardwareSkin);
        enabled = true;
        return true;
    }

    void Render(IDirect3DDevice9* device, const std::function<void()>& draw)
    {
        if (!enabled || rendering || !currentPalette || !currentPalette->damaged || !currentMaterial) return;
        // d3dCharacterShader's TTEX setter (0x705440) retains the game's blood_spat.tga at +132.
        auto texture = Field<void*>(currentMaterial, 132);
        if (!texture || Field<uintptr_t>(texture, 0) != textureVtable) return;
        // Same preference order as d3dTexture::SetTexture (0x6561E0).
        auto bloodTexture = Field<IDirect3DBaseTexture9*>(texture, 20);
        if (!bloodTexture) bloodTexture = Field<IDirect3DBaseTexture9*>(texture, 24);
        if (!bloodTexture) bloodTexture = Field<IDirect3DBaseTexture9*>(texture, 16);
        if (!bloodTexture) return;
        if (shaderDevice != device)
        {
            replacements.clear(); bloodPixelShader.Reset(); shaderDevice = device;
            D3DCAPS9 caps{};
            if (FAILED(device->GetDeviceCaps(&caps)) || caps.VertexShaderVersion < D3DVS_VERSION(2, 0) ||
                caps.MaxVertexShaderConst < damageRegister + damageRegisters) return;
            device->CreatePixelShader(bloodPixelCode, bloodPixelShader.GetAddressOf());
        }
        if (!bloodPixelShader) return;
        SavedState saved(device);
        if (!saved.valid) return;
        auto vertex = GetBloodVertex(device, saved.vertex.Get());
        if (!vertex) return;
        rendering = true;
        struct Guard { ~Guard() { rendering = false; } } guard;
        auto damage = currentPalette->constants;
        const float opacity = 1.0f - std::clamp(Field<uint32_t>(currentMaterial, 56) / 255.0f, 0.0f, 1.0f);
        for (auto& value : damage) value *= opacity;
        if (FAILED(device->SetVertexShader(vertex)) || FAILED(device->SetPixelShader(bloodPixelShader.Get())) ||
            FAILED(device->SetTexture(0, bloodTexture)) ||
            FAILED(device->SetVertexShaderConstantF(damageRegister, damage.data(), damageRegisters))) return;
        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetRenderState(D3DRS_COLORWRITEENABLE, saved.values[7] & 7); // Preserve target alpha.
        draw();
    }

    void Shutdown()
    {
        primitiveHook.reset(); hardwareHook.reset();
        replacements.clear(); bloodPixelShader.Reset(); shaderDevice = nullptr;
        enabled = false;
    }
}
