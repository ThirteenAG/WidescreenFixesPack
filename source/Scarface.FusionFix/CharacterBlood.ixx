module;

#include "stdafx.h"
#include "RTTI.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <wrl/client.h>
#include <fstream>

export module CharacterBlood;

namespace
{
    using Microsoft::WRL::ComPtr;
    SafetyHookInline primitiveHook, hardwareHook;
    uintptr_t characterVtable = 0, textureVtable = 0;
    bool enabled = false;
    HMODULE d3dx = nullptr;
    decltype(&D3DXDisassembleShader) disassemble = nullptr;
    decltype(&D3DXAssembleShader) assemble = nullptr;
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

    void Log(std::string_view message)
    {
        std::ofstream(GetExeModulePath() / "Scarface.CharacterBlood.log", std::ios::app) << message << '\n';
    }

    int __fastcall DrawPrimitiveGroup(void* self, void*)
    {
        static bool reported = false;
        if (!reported) { Log("Entered skinned primitive draw."); reported = true; }
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
        static bool reported = false;
        if (!reported) { Log("Entered hardware skin draw."); reported = true; }
        // d3dExtHardwareSkinning::Draw (0x650E10), slot 6.
        const auto previous = std::exchange(currentMaterial,
            material && Field<uintptr_t>(material, 0) == characterVtable ? material : nullptr);
        const auto result = hardwareHook.thiscall<int>(self, material, buffer);
        currentMaterial = previous;
        return result;
    }

    ComPtr<ID3DXBuffer> Assemble(const std::string& source)
    {
        ComPtr<ID3DXBuffer> code, errors;
        if (FAILED(assemble(source.c_str(), UINT(source.size()), nullptr, nullptr, 0,
                            code.GetAddressOf(), errors.GetAddressOf())))
        {
            Log(errors ? static_cast<const char*>(errors->GetBufferPointer()) : "Shader assembly failed.");
            return {};
        }
        return code;
    }

    std::string BloodVertexSource(const char* original)
    {
        std::istringstream lines(original);
        std::string line, source;
        while (std::getline(lines, line))
        {
            if (auto comment = line.find("//"); comment != std::string::npos) line.resize(comment);
            auto first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos) continue;
            line.erase(0, first);
            if (line.starts_with("vs_1_1")) line = "vs_2_0";
            // VS 2 permits the additional palette at c128 without borrowing lighting/bone constants.
            if (line.starts_with("mov a0.x")) line.replace(0, 3, "mova");
            source += line + '\n';
        }
        // PS2 0x76CC18 selects the first palette bone's front/back damage by normal Z.
        // 0x76EAC8/0x76EB10 supply this value as the second texture pass's alpha.
        source += "mul r0.x, v2.x, c0.z\n"
                  "mova a0.x, r0.x\n"
                  "slt r0.y, v3.z, c0.x\n"
                  "sub r0.z, c0.y, r0.y\n"
                  "mul r0.y, r0.y, c128[a0.x].x\n"
                  "mad oD0.w, r0.z, c128[a0.x].y, r0.y\n";
        return source;
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
                    ComPtr<ID3DXBuffer> text;
                    if (SUCCEEDED(disassemble(bytes.data(), FALSE, nullptr, text.GetAddressOf())))
                        if (auto code = Assemble(BloodVertexSource(static_cast<const char*>(text->GetBufferPointer()))))
                            device->CreateVertexShader(static_cast<const DWORD*>(code->GetBufferPointer()), entry.blood.GetAddressOf());
                    Log(entry.blood ? "Skin vertex shader ready." : "Skin vertex shader unavailable; original rendering retained.");
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
        CIniReader iniReader("");
        const bool bRestoreCharacterBlood = iniReader.ReadInteger("GRAPHICS", "RestoreCharacterBlood", 0) != 0;
        if (!bRestoreCharacterBlood) return false;
        std::ofstream(GetExeModulePath() / "Scarface.CharacterBlood.log") << "Character blood restoration\n";
        d3dx = LoadLibraryW(L"d3dx9_43.dll");
        if (d3dx)
        {
            disassemble = reinterpret_cast<decltype(disassemble)>(GetProcAddress(d3dx, "D3DXDisassembleShader"));
            assemble = reinterpret_cast<decltype(assemble)>(GetProcAddress(d3dx, "D3DXAssembleShader"));
        }
        characterVtable = ScarfaceRTTI::FindVtable(".?AVd3dCharacterShader@pure3d@@");
        textureVtable = ScarfaceRTTI::FindVtable(".?AVd3dTexture@pure3d@@");
        auto primitive = ScarfaceRTTI::FindVtable(".?AVPrimGroupSkinnedOptimized@pure3d@@");
        auto hardware = ScarfaceRTTI::FindVtable(".?AVd3dExtHardwareSkinning@pure3d@@");
        if (assemble && disassemble && characterVtable && textureVtable && primitive && hardware)
        {
            primitiveHook = safetyhook::create_inline(injector::ReadMemory<void*>(primitive + 8 * sizeof(void*), true), DrawPrimitiveGroup);
            hardwareHook = safetyhook::create_inline(injector::ReadMemory<void*>(hardware + 6 * sizeof(void*), true), DrawHardwareSkin);
        }
        enabled = primitiveHook && hardwareHook;
        if (!enabled) { primitiveHook.reset(); hardwareHook.reset(); }
        Log(enabled ? "RTTI hooks installed." : "Initialization failed; original rendering retained.");
        return enabled;
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
            if (auto code = Assemble("ps_1_1\ntex t0\nmul r0, t0, v0\n"))
                device->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()), bloodPixelShader.GetAddressOf());
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
        static bool reported = false;
        if (!reported) { Log("Blood pass submitted using the material's TTEX and front/back joint damage."); reported = true; }
    }

    void Shutdown()
    {
        primitiveHook.reset(); hardwareHook.reset();
        replacements.clear(); bloodPixelShader.Reset(); shaderDevice = nullptr;
        if (d3dx) FreeLibrary(d3dx);
        d3dx = nullptr; assemble = nullptr; disassemble = nullptr; enabled = false;
    }
}
