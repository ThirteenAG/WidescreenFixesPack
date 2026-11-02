module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <fstream>

export module ButtonIcons;

import ComVars;
import Controller;
import PostFX;

// Pad button icons in the controls menu, drawn from Mafia.FusionFix/buttons.dds
export namespace MafiaIcons
{
    SafetyHookInline nameHook, measureHook, drawHook;
    IDirect3DDevice9* textureDevice = nullptr;
    IDirect3DTexture9* texture = nullptr;
    DWORD lastAttempt = 0;
    constexpr unsigned char marker = 0x1F;

    bool Ready()
    {
        auto device = GetDevice9();
        if (!device) return false;
        if (texture && textureDevice == device) return true;
        const auto now = GetTickCount();
        if (lastAttempt && now - lastAttempt < 1000) return false;
        lastAttempt = now;
        if (texture) texture->Release();
        texture = nullptr;
        textureDevice = nullptr;
        auto path = GetThisModulePath() / L"Mafia.FusionFix" / L"buttons.dds";
        std::ifstream file(path, std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        constexpr uint32_t width = 512, height = 1024;
        if (data.size() != 128 + width * height || std::memcmp(data.data(), "DDS ", 4) ||
            *reinterpret_cast<const uint32_t*>(data.data() + 12) != height ||
            *reinterpret_cast<const uint32_t*>(data.data() + 16) != width ||
            std::memcmp(data.data() + 84, "DXT5", 4)) return false;
        if (FAILED(device->CreateTexture(width, height, 1, 0, D3DFMT_DXT5, D3DPOOL_MANAGED, &texture, nullptr))) return false;
        D3DLOCKED_RECT lock{};
        if (FAILED(texture->LockRect(0, &lock, nullptr, 0)))
        {
            texture->Release(); texture = nullptr;
            return false;
        }
        for (uint32_t row = 0; row < height / 4; ++row)
            std::memcpy(static_cast<uint8_t*>(lock.pBits) + row * lock.Pitch, data.data() + 128 + row * width * 4, width * 4);
        texture->UnlockRect(0);
        textureDevice = device;
        return true;
    }

    struct Label
    {
        std::string plain;
        std::vector<std::pair<size_t, unsigned int>> icons;
    };

    Label Parse(const char* text, const char* end)
    {
        Label result;
        if (!text) return result;
        const auto start = reinterpret_cast<uintptr_t>(text);
        const auto limit = reinterpret_cast<uintptr_t>(end);
        const size_t bound = limit > start ? std::min<size_t>(limit - start, 4096) : 4096;
        const size_t length = strnlen(text, bound);
        result.plain.assign(text, length);
        for (size_t index = 0; index + 2 < length; ++index)
        {
            const unsigned int code = static_cast<unsigned char>(text[index + 1]);
            if (static_cast<unsigned char>(text[index]) != marker || static_cast<unsigned char>(text[index + 2]) != marker || code < 'A' || code >= 'A' + 27) continue;
            const auto icon = code - 'A';
            result.icons.emplace_back(index, icon);
            result.plain.replace(index, 3, 3, ' ');
            index += 2;
        }
        return result;
    }

    const char* __fastcall Name(uintptr_t object, void*, uintptr_t binding)
    {
        if (!binding || !Ready()) return nameHook.unsafe_thiscall<const char*>(object, binding);
        const auto original = binding;
        const auto mapped = MafiaPad::ForBinding(binding);
        if (mapped) binding = reinterpret_cast<uintptr_t>(mapped);
        if (Field<uint16_t>(binding, 4) != 3) return nameHook.unsafe_thiscall<const char*>(object, original);
        const auto button = Field<uint16_t>(binding, 6);
        unsigned int icon = 27;
        if (button >= 0x100 && button < 0x10C)
        {
            constexpr unsigned int buttons[] = { 0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 8, 9 };
            icon = buttons[button - 0x100];
        }
        else if (button >= 0x200)
        {
            switch (button & 0xF)
            {
                case 1: icon = 12; break;
                case 2: icon = 15; break;
                case 4: icon = 13; break;
                case 8: icon = 14; break;
                default: icon = 16; break;
            }
        }
        else
        {
            switch (button)
            {
                case 0: icon = 21; break;
                case 1: icon = 20; break;
                case 3: icon = 26; break;
                case 4: icon = 25; break;
            }
        }
        if (icon >= 27) return nameHook.unsafe_thiscall<const char*>(object, original);
        static const auto labels = []()
        {
            std::array<std::array<char, 4>, 27> values{};
            for (unsigned int index = 0; index < values.size(); ++index)
                values[index] = { static_cast<char>(marker), static_cast<char>('A' + index), static_cast<char>(marker), 0 };
            return values;
        }();
        if (mapped && Field<uint16_t>(original, 4) != 3)
        {
            static std::array<std::string, MafiaPad::ActionCount> combined;
            const auto index = (original - MafiaPad::profileBindings) / sizeof(MafiaPad::Binding);
            const char* native = nameHook.unsafe_thiscall<const char*>(object, original);
            combined[index] = native && *native ? std::string(native).substr(0, 96) + " / " : "";
            combined[index] += labels[icon].data();
            return combined[index].c_str();
        }
        return labels[icon].data();
    }

    double __fastcall Measure(void* fonts, void*, const char* text, float height, unsigned int font, const char* end)
    {
        if (!text || !std::strchr(text, marker))
            return measureHook.unsafe_thiscall<double>(fonts, text, height, font, end);
        auto label = Parse(text, end);
        return measureHook.unsafe_thiscall<double>(fonts, label.plain.c_str(), height, font, nullptr);
    }

    struct Vertex
    {
        float x, y, z, rhw;
        uint32_t color;
        float u, v;
    };

    void DrawIcons(const std::vector<Vertex>& vertices)
    {
        if (vertices.empty() || !Ready()) return;
        IDirect3DStateBlock9* state = nullptr;
        auto device = textureDevice;
        if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state))) return;
        device->SetVertexShader(nullptr);
        device->SetPixelShader(nullptr);
        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetTexture(0, texture);
        for (auto [state, value] : { std::pair{ D3DTSS_COLOROP, DWORD(D3DTOP_SELECTARG1) },
            { D3DTSS_COLORARG1, DWORD(D3DTA_TEXTURE) }, { D3DTSS_COLORARG2, DWORD(D3DTA_DIFFUSE) },
            { D3DTSS_ALPHAOP, DWORD(D3DTOP_MODULATE) }, { D3DTSS_ALPHAARG1, DWORD(D3DTA_TEXTURE) },
            { D3DTSS_ALPHAARG2, DWORD(D3DTA_DIFFUSE) } })
            device->SetTextureStageState(0, state, value);
        device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(vertices.size() / 3), vertices.data(), sizeof(Vertex));
        state->Apply();
        state->Release();
    }

    double __fastcall Draw(float* fonts, void*, const char* text, float x, float y, float width, float height,
        uint32_t color, unsigned char flags, unsigned int font, const char* end)
    {
        if (!text || font >= 4 || !std::strchr(text, marker))
            return drawHook.unsafe_thiscall<double>(fonts, text, x, y, width, height, color, flags, font, end);
        auto label = Parse(text, end);
        const auto result = drawHook.unsafe_thiscall<double>(fonts, label.plain.c_str(), x, y, width, height, color, flags, font, nullptr);
        auto bank = fonts + 1031 * font;
        const float size = height * bank[2];
        const auto measure = [&](const char* string)
        {
            return static_cast<float>(measureHook.unsafe_thiscall<double>(fonts, string, size, font, nullptr));
        };
        const float total = measure(label.plain.c_str());
        float scale = 1.0f;
        if (flags & 2) x += (width - total) * 0.5f;
        else if (flags & 4) x += width - total;
        else
        {
            if (flags & 8) x += height * 0.5f;
            if ((flags & 0x10) && total > 0)
                scale = std::min(1.0f, std::max(0.0f, width - ((flags & 8) ? height : 0.0f)) / total);
        }
        const int family = std::clamp(FixSettings().Int("MAIN.GamepadIcons"), 1, 3) - 1;
        std::vector<Vertex> vertices;
        for (auto [position, icon] : label.icons)
        {
            const float advance = measure(label.plain.substr(0, position).c_str()) - measure("");
            const float cellWidth = size * 0.9f * scale;
            const float side = std::min(size, cellWidth);
            const float left = x + advance * scale + (cellWidth - side) * 0.5f - 0.5f;
            const float top = y - height * bank[6] + (size - side) * 0.5f - 0.5f;
            const float u = static_cast<float>(icon % 8) / 8.0f;
            const float v = static_cast<float>(family * 4 + icon / 8) / 16.0f;
            Vertex tl{ left, top, 0, 1, color, u, v }, tr = tl, bl = tl, br = tl;
            tr.x = br.x = left + side;
            bl.y = br.y = top + side;
            tr.u = br.u = u + 1.0f / 8.0f;
            bl.v = br.v = v + 1.0f / 16.0f;
            for (auto vertex : { tl, tr, bl, tr, br, bl }) vertices.push_back(vertex);
        }
        DrawIcons(vertices);
        return result;
    }

    void Install()
    {
        auto name = hook::pattern("53 55 8B 6C 24 0C 33 C0 56 8B F1 85 ED 0F 84 ? ? ? ? 8D 9E ? ? ? ? 57 B9 08 00 00 00 8B FB F3 AB 66 8B 45 04");
        if (name.empty())
            name = hook::pattern("56 57 8B 7C 24 0C 33 C0 85 FF 0F 84 ? ? ? ? 66 8B 47 04 83 F8 03 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 33 C0 8A 47 06 3D DD 00 00 00");
        auto draw = hook::pattern("B8 E4 38 00 00 E8 ? ? ? ? 53 8B 9C 24 EC 38 00 00 85 DB 89 8C 24 9C 00 00 00");
        auto measure = hook::pattern("56 8B 74 24 08 85 F6 75 0A D9 05 ? ? ? ? 5E C2 10 00 8B 44 24 10 53 8B D0 57 C1 E2 07 03 D0");
        measureHook = safetyhook::create_inline(measure.get_first(), Measure);
        drawHook = safetyhook::create_inline(draw.get_first(), Draw);
        if (measureHook && drawHook) nameHook = safetyhook::create_inline(name.get_first(), Name);
        if (!nameHook)
        {
            drawHook.reset();
            measureHook.reset();
        }
        FixSettings().Availability("MAIN.GamepadIcons", [](std::string_view value)
        {
            return value == "0" || (nameHook && Ready());
        });
    }
}
