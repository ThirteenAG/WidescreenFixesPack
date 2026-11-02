module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"
#include <atomic>


export module ButtonIcons;

import ComVars;
import NativeMenu;

export namespace SH2Icons
{
    inline SafetyHookInline NameHook, MessageHook, GlyphsHook, MeasureHook, LineHook;
    inline IDirect3DTexture8* Texture = nullptr;
    inline std::atomic<bool> Playing = false;
    inline bool ProfileInstalled = false;
    constexpr uint16_t Marker = 0xFAFE;
    constexpr uint16_t FirstIcon = 0xFEC0;
    constexpr int IconCount = 27;
    constexpr int IconWidth = 20;
    struct Icon { float x, y; int image; };
    inline std::vector<Icon> Pending;
    inline thread_local std::vector<int>* MessageIcons = nullptr;
    inline thread_local size_t MessageIcon = 0;

    inline void Release()
    {
        if (Texture)
            Texture->Release();
        Texture = nullptr;
        Pending.clear();
    }

    inline void Load(IDirect3DDevice8* device)
    {
        Release();
        auto path = GetThisModulePath<std::filesystem::path>() / L"SilentHill2.FusionFix" / L"buttons.dds";
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

    inline int __cdecl Name(int action, char* buffer, int language)
    {
        if (!Texture || action < 0 || action >= 22)
            return NameHook.ccall<int>(action, buffer, language);
        int button = ControllerBindings.get()[action * 2];
        // Physical buttons in Xidi.ini's gameplay mapper; unmapped extra buttons keep their native names.
        constexpr int images[] = { 0, 1, 8, 1, 3, 18, 6, 9, 10, 11, 12, 2, 4, 5 };
        if (button < 0 || button >= int(std::size(images)))
            return NameHook.ccall<int>(action, buffer, language);
        return sprintf_s(buffer, 16, "\\h\\x254\\_%d", 192 + images[button]);
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

    inline int __cdecl Measure(int* widths, const uint16_t* source)
    {
        Text text(source);
        return MeasureHook.ccall<int>(widths, text.changed ? text.value.data() : source);
    }

    inline void* __cdecl Message(const uint16_t* table, int index, int x, int y)
    {
        y = SH2Menu::MessageY(y);
        if (!table || index < 0 || index >= table[0])
            return MessageHook.ccall<void*>(table, index, x, y);
        Text text(table + table[index + 1]);
        if (!text.changed)
            return MessageHook.ccall<void*>(table, index, x, y);
        std::vector<uint16_t> copied{ 1, 2 };
        copied.insert(copied.end(), text.value.begin(), text.value.end());
        auto previous = MessageIcons;
        const auto previousIcon = MessageIcon;
        MessageIcons = &text.icons;
        MessageIcon = 0;
        auto result = MessageHook.ccall<void*>(copied.data(), 0, x, y);
        MessageIcons = previous;
        MessageIcon = previousIcon;
        return result;
    }

    inline int __cdecl Line(int x, int width, int y)
    {
        if (MessageIcons && MessageIcon < MessageIcons->size())
        {
            int image = (*MessageIcons)[MessageIcon++];
            if (image >= 0)
            {
                int height = (FontFlags.get() & 0x800) ? GlyphHeight.get() : (FontIndex.get() < 16 ? DefaultHeights.get()[FontIndex.get() * 2] : IconWidth);
                height = std::clamp(height, 8, 128);
                if (Pending.size() < 128)
                    // Match the native GS conversion and centre on the current glyph box.
                    Pending.push_back({ float(int16_t(x)) - 1792.0f, float(int16_t(y)) - 1808.0f + float(height - IconWidth) * 0.5f, image });
                return 0;
            }
        }
        return LineHook.ccall<int>(x, width, y);
    }

    inline int __cdecl Glyphs(const uint16_t* source, int x, int y)
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

    inline void Draw(IDirect3DDevice8* device)
    {
        if (Pending.empty() || !Texture)
            return;
        auto icons = std::move(Pending);
        Pending.clear();
        IDirect3DSurface8* target = nullptr;
        IDirect3DSurface8* back = nullptr;
        device->GetRenderTarget(&target);
        device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back);
        const bool visible = target && target == back;
        if (target) target->Release();
        if (back) back->Release();
        if (!visible)
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
        const float sx = Screen.fHeight * (4.0f / 3.0f) / 512.0f, sy = Screen.fHeight / 480.0f;
        const int chosen = FixSettings().Int("MAIN.GamepadIcons");
        const int family = chosen == 0 ? 0 : std::clamp(chosen - 1, 0, 2);
        struct Vertex { float x, y, z, w; DWORD colour; float u, v; };
        for (auto icon : icons)
        {
            const float side = IconWidth * sy;
            const float left = Screen.TextOffset + icon.x * sx + (IconWidth * sx - side) * 0.5f - 0.5f;
            const float top = icon.y * sy - 0.5f;
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

    inline void Init()
    {
        auto name = hook::pattern("8B 44 24 04 83 F8 16 0F 8D ? ? ? ? 8B 04 C5 ? ? ? ? 83 F8 FF 75 14");
        auto line = hook::pattern("0F BF 05 ? ? ? ? 0F B7 0D ? ? ? ? 3B C1 0F 8D ? ? ? ? 8D 14 80 A1");
        auto glyphs = hook::pattern("8B 44 24 04 85 C0 74 41 8B 44 24 08 85 C0 8B 4C 24 0C 66 A3");
        auto metrics = hook::pattern("A1 ? ? ? ? F6 C4 08 74 ? 33 C0 66 A1 ? ? ? ? 66 D1 E8 EB ? 8B 15 ? ? ? ? 0F BF 04 95");
        if (!SH2Menu::DrawMessage || !SH2Menu::Measure)
            return;
        FontFlags.SetAddress(*metrics.get_first<uint32_t*>(1));
        GlyphHeight.SetAddress(*metrics.get_first<uint16_t*>(14));
        FontIndex.SetAddress(*metrics.get_first<uint32_t*>(25));
        DefaultHeights.SetAddress(*metrics.get_first<int16_t*>(33));
        ControllerBindings.SetAddress(*name.get_first<int*>(16));
        NameHook = safetyhook::create_inline(name.get_first(), &Name);
        MessageHook = safetyhook::create_inline(SH2Menu::DrawMessage, &Message);
        SH2Menu::ExtendPanel();
        GlyphsHook = safetyhook::create_inline(glyphs.get_first(), &Glyphs);
        MeasureHook = safetyhook::create_inline(SH2Menu::Measure, &Measure);
        LineHook = safetyhook::create_inline(line.get_first(), &Line);
        SHDisplay::OnDeviceReady().push_back(&Load);
        SHDisplay::OnBeforeReset().push_back([](IDirect3DDevice8*) { Release(); });
        SHDisplay::OnEndScene().push_back(&Draw);
        SHDisplay::OnPresent().push_back([]()
        {
            Playing = GameMode.is_initialized() && GamePhase.is_initialized() && GameMode.get() == 13 && GamePhase.get() == 4;
            if (!ProfileInstalled)
            {
                auto xidi = GetModuleHandleW(L"Xidi.32.dll");
                using Register = bool(*)(const wchar_t* (*)());
                auto registerProfile = xidi ? reinterpret_cast<Register>(GetProcAddress(xidi, "XidiRegisterProfileCallback")) : nullptr;
                if (registerProfile)
                    ProfileInstalled = registerProfile([]() -> const wchar_t* { return Playing ? L"Main" : L"Menu"; });
            }
        });
    }
}
