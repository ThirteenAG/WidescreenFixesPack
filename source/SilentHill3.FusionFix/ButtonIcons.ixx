module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"

export module ButtonIcons;

// Pad button icons in the game's text
export namespace SH3Icons
{
    SafetyHookInline GlyphsHook, MeasureHook, LineHook;
    IDirect3DTexture8* Texture = nullptr;
    constexpr uint16_t Marker = 0xFAFE;
    constexpr uint16_t FirstIcon = 0xFEC0;
    constexpr int IconCount = 27;
    constexpr int IconWidth = 28;
    struct Icon { float x, y; int image; };
    std::vector<Icon> Pending;
    thread_local std::vector<int>* MessageIcons = nullptr;
    thread_local size_t MessageIcon = 0;

    void Release()
    {
        if (Texture)
            Texture->Release();
        Texture = nullptr;
        Pending.clear();
    }

    void Load(IDirect3DDevice8* device)
    {
        Release();
        auto path = GetThisModulePath<std::filesystem::path>() / L"SilentHill3.FusionFix" / L"buttons.dds";
        std::ifstream file(path, std::ios::binary);
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        constexpr UINT width = 512, height = 1024;
        if (data.size() != 128 + width * height || memcmp(data.data(), "DDS ", 4) ||
            *reinterpret_cast<const uint32_t*>(data.data() + 12) != height ||
            *reinterpret_cast<const uint32_t*>(data.data() + 16) != width || memcmp(data.data() + 84, "DXT5", 4))
            return;
        if (FAILED(device->CreateTexture(width, height, 1, 0, D3DFMT_DXT5, D3DPOOL_MANAGED, &Texture)))
            return;
        D3DLOCKED_RECT locked{};
        if (FAILED(Texture->LockRect(0, &locked, nullptr, 0)))
            return Release();
        for (UINT row = 0; row < height / 4; ++row)
            memcpy(static_cast<uint8_t*>(locked.pBits) + row * locked.Pitch, data.data() + 128 + row * width * 4, width * 4);
        Texture->UnlockRect(0);
    }

    struct Text
    {
        std::vector<uint16_t> value;
        std::vector<int> icons;
        bool changed = false;

        explicit Text(const uint16_t* source)
        {
            if (!source)
                return;
            for (size_t i = 0; i < 2048; ++i)
            {
                uint16_t code = source[i];
                if (code == Marker && i + 1 < 2048 && source[i + 1] >= FirstIcon && source[i + 1] < FirstIcon + IconCount)
                {
                    icons.push_back(source[++i] - FirstIcon);
                    value.push_back(0xFE00 + IconWidth);
                    changed = true;
                }
                else
                {
                    if ((code & 0xFF00) == 0xFE00)
                        icons.push_back(-1);
                    value.push_back(code);
                }
                if (code == 0xFFFF)
                    return;
            }
            value.clear();
            icons.clear();
            changed = false;
        }
    };

    int __cdecl Measure(int* widths, const uint16_t* source)
    {
        Text text(source);
        return MeasureHook.ccall<int>(widths, text.changed ? text.value.data() : source);
    }

    int __cdecl Line(int x, int width, int y)
    {
        if (MessageIcons && MessageIcon < MessageIcons->size())
        {
            int image = (*MessageIcons)[MessageIcon++];
            if (image >= 0)
            {
                // Native glyph vertices use the GS origin at (2048, 2048).
                const float left = float(int16_t(x)) - 2048.0f, top = float(int16_t(y)) - 2048.0f;
                for (auto& icon : Pending)
                {
                    if (icon.image == image && std::abs(icon.x - left) <= 4 && std::abs(icon.y - top) <= 4)
                    {
                        icon.x = left;
                        icon.y = top;
                        return 0;
                    }
                }
                if (Pending.size() < 128)
                    Pending.push_back({ left, top, image });
                return 0;
            }
        }
        return LineHook.ccall<int>(x, width, y);
    }

    int __cdecl Glyphs(const uint16_t* source, int x, int y)
    {
        Text text(source);
        if (!text.changed)
            return GlyphsHook.ccall<int>(source, x, y);
        auto previous = MessageIcons;
        const auto previousIcon = MessageIcon;
        MessageIcons = &text.icons;
        MessageIcon = 0;
        int result = GlyphsHook.ccall<int>(text.value.data(), x, y);
        MessageIcons = previous;
        MessageIcon = previousIcon;
        return result;
    }

    void Draw(IDirect3DDevice8* device)
    {
        if (Pending.empty() || !Texture)
            return;
        auto icons = std::move(Pending);
        Pending.clear();
        IDirect3DSurface8* target = nullptr;
        if (FAILED(device->GetRenderTarget(&target)) || !target)
            return;
        D3DSURFACE_DESC surface{};
        const auto result = target->GetDesc(&surface);
        target->Release();
        if (FAILED(result) || !surface.Width || !surface.Height)
            return;
        DWORD saved = 0;
        if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &saved)))
            return;
        IDirect3DVertexBuffer8* stream = nullptr;
        IDirect3DIndexBuffer8* indices = nullptr;
        UINT stride = 0, base = 0;
        device->GetStreamSource(0, &stream, &stride);
        device->GetIndices(&indices, &base);
        device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        device->SetPixelShader(0);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        device->SetTexture(0, Texture);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
        device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
        device->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
        device->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
        device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        const float width = float(surface.Width), height = float(surface.Height);
        const float boxWidth = std::min(width, height * (4.0f / 3.0f));
        const float sx = boxWidth / 512.0f, sy = boxWidth * 0.75f / 448.0f;
        const int chosen = FixSettings().Int("MAIN.GamepadIcons");
        const int family = chosen == 0 ? 0 : std::clamp(chosen - 1, 0, 2);
        struct Vertex { float x, y, z, w; DWORD colour; float u, v; };
        for (auto icon : icons)
        {
            const float side = IconWidth * sy;
            const float left = width * 0.5f + icon.x * sx + (IconWidth * sx - side) * 0.5f - 0.5f;
            const float top = height * 0.5f + icon.y * sy - 0.5f;
            const float u = float(icon.image % 8) / 8.0f, v = float(family * 4 + icon.image / 8) / 16.0f;
            Vertex quad[] =
            {
                { left, top, 0, 1, 0xFFFFFFFF, u, v },
                { left + side, top, 0, 1, 0xFFFFFFFF, u + 1.0f / 8.0f, v },
                { left, top + side, 0, 1, 0xFFFFFFFF, u, v + 1.0f / 16.0f },
                { left + side, top + side, 0, 1, 0xFFFFFFFF, u + 1.0f / 8.0f, v + 1.0f / 16.0f },
            };
            device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));
        }
        device->ApplyStateBlock(saved);
        device->SetStreamSource(0, stream, stride);
        device->SetIndices(indices, base);
        if (stream) stream->Release();
        if (indices) indices->Release();
        device->DeleteStateBlock(saved);
    }

    void Init()
    {
        auto line = hook::pattern("0F BF 05 ? ? ? ? 0F B7 0D ? ? ? ? 3B C1 0F 8D ? ? ? ? 8D 14 80 A1");
        auto glyphs = hook::pattern("8B 44 24 04 85 C0 74 41 8B 44 24 08 85 C0 8B 4C 24 0C 66 A3");
        auto measure = hook::pattern("83 EC 0C 55 8B 6C 24 18 33 D2 3B EA 75 07 33 C0");
        GlyphsHook = safetyhook::create_inline(glyphs.get_first(), &Glyphs);
        MeasureHook = safetyhook::create_inline(measure.get_first(), &Measure);
        LineHook = safetyhook::create_inline(line.get_first(), &Line);
        SHDisplay::OnDeviceReady().push_back(&Load);
        SHDisplay::OnBeforeReset().push_back([](IDirect3DDevice8*) { Release(); });
        SHDisplay::OnEndScene().push_back(&Draw);
    }
}
