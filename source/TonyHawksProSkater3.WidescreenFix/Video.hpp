#pragma once
#include <minidx8/d3d8.h>
#include <bit>
#include "../Shared/TonyHawk/Video.hpp"

namespace THPS3Video
{
    using Callback = int(__cdecl*)(const uint8_t*, int);
    inline SafetyHookInline SetupHook, CleanupHook;
    inline Callback* NativeCallback = nullptr;
    inline Callback PreviousCallback = nullptr;
    inline IDirect3DDevice8* (__cdecl* GetDevice)() = nullptr;
    inline IDirect3DDevice8* Device = nullptr;
    inline IDirect3DTexture8* Texture = nullptr;
    inline const int32_t* Mode = nullptr;
    inline uint32_t SourceWidth = 0, SourceHeight = 0, SourcePitch = 0, TextureWidth = 0, TextureHeight = 0;
    inline uint16_t Bits = 0;
    inline bool TopDown = false;
    inline std::array<uint32_t, 3> Masks{};

    inline void Release()
    {
        if (Texture)
            Texture->Release();
        Texture = nullptr;
        Device = nullptr;
    }

    inline bool Prepare(IDirect3DDevice8* device, const BITMAPINFOHEADER* info)
    {
        Release();
        if (!device || !info || info->biWidth <= 0 || info->biWidth > 16384 || !info->biHeight ||
            info->biHeight < -16384 || info->biHeight > 16384 ||
            (info->biBitCount != 16 && info->biBitCount != 24 && info->biBitCount != 32))
            return false;
        SourceWidth = info->biWidth;
        SourceHeight = std::abs(info->biHeight);
        TopDown = info->biHeight < 0;
        Bits = info->biBitCount;
        SourcePitch = ((SourceWidth * Bits + 31) / 32) * 4;
        Masks = { 0x7C00, 0x03E0, 0x001F };
        if (Bits == 16 && info->biCompression == BI_BITFIELDS)
            std::copy_n(reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(info) + 40), Masks.size(), Masks.begin());
        if (Bits == 16)
        {
            for (auto mask : Masks)
            {
                if (!mask || mask > 0xFFFF || !std::has_single_bit((mask >> std::countr_zero(mask)) + 1))
                    return false;
            }
        }
        D3DCAPS8 caps{};
        if (FAILED(device->GetDeviceCaps(&caps)))
            return false;
        TextureWidth = caps.TextureCaps & D3DPTEXTURECAPS_POW2 ? std::bit_ceil(SourceWidth) : SourceWidth;
        TextureHeight = caps.TextureCaps & D3DPTEXTURECAPS_POW2 ? std::bit_ceil(SourceHeight) : SourceHeight;
        if (caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
            TextureWidth = TextureHeight = std::max(TextureWidth, TextureHeight);
        if (TextureWidth > caps.MaxTextureWidth || TextureHeight > caps.MaxTextureHeight)
            return false;
        if (FAILED(device->CreateTexture(TextureWidth, TextureHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &Texture)))
            return false;
        Device = device;
        return true;
    }

    inline uint32_t Pixel(const uint8_t* pixel)
    {
        if (Bits != 16)
            return 0xFF000000u | (pixel[2] << 16) | (pixel[1] << 8) | pixel[0];
        const uint16_t value = pixel[0] | (pixel[1] << 8);
        uint32_t color = 0xFF000000u;
        for (size_t i = 0; i < Masks.size(); ++i)
        {
            const auto shift = std::countr_zero(Masks[i]);
            const auto maximum = Masks[i] >> shift;
            const auto channel = ((value & Masks[i]) >> shift) * 255 / maximum;
            color |= channel << (16 - i * 8);
        }
        return color;
    }

    inline int __cdecl Render(const uint8_t* bytes, int length)
    {
        if (!Device || !Texture || !bytes || length < 0 || static_cast<uint64_t>(SourcePitch) * SourceHeight > static_cast<uint32_t>(length))
            return 0;
        D3DLOCKED_RECT locked{};
        const auto uploaded = Texture->LockRect(0, &locked, nullptr, 0);
        if (FAILED(uploaded))
            return uploaded;
        for (uint32_t y = 0; y < TextureHeight; ++y)
        {
            auto row = static_cast<uint8_t*>(locked.pBits) + y * locked.Pitch;
            auto sourceRow = std::min(y, SourceHeight - 1);
            if (!TopDown)
                sourceRow = SourceHeight - sourceRow - 1;
            const auto source = bytes + sourceRow * SourcePitch;
            auto output = reinterpret_cast<uint32_t*>(row);
            for (uint32_t x = 0; x < SourceWidth; ++x)
                output[x] = Pixel(source + x * (Bits / 8));
            std::fill(output + SourceWidth, output + TextureWidth, output[SourceWidth - 1]);
        }
        Texture->UnlockRect(0);
        IDirect3DSurface8* target = nullptr;
        if (FAILED(Device->GetRenderTarget(&target)))
            return 0;
        D3DSURFACE_DESC description{};
        const auto described = target->GetDesc(&description);
        target->Release();
        if (FAILED(described) || !description.Width || !description.Height)
            return 0;
        D3DVIEWPORT8 previousViewport{};
        DWORD state = 0;
        if (FAILED(Device->GetViewport(&previousViewport)) || FAILED(Device->CreateStateBlock(D3DSBT_ALL, &state)))
            return 0;
        D3DVIEWPORT8 viewport{ 0, 0, description.Width, description.Height, 0.0f, 1.0f };
        Device->SetViewport(&viewport);
        Device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
        const auto begun = Device->BeginScene();
        auto drawn = begun;
        if (SUCCEEDED(begun))
        {
            const auto box = TonyVideo::Layout(static_cast<float>(description.Width), static_cast<float>(description.Height),
                static_cast<float>(SourceWidth), static_cast<float>(SourceHeight), Mode ? std::clamp(*Mode, 0, 2) : 1);
            struct Vertex
            {
                float x, y, z, rhw, u, v;
            };
            const float left = box.x - 0.5f, top = box.y - 0.5f;
            const float right = left + box.width, bottom = top + box.height;
            const float u = static_cast<float>(SourceWidth) / TextureWidth;
            const float v = static_cast<float>(SourceHeight) / TextureHeight;
            const Vertex vertices[] =
            {
                { left, top, 0.0f, 1.0f, 0.0f, 0.0f },
                { right, top, 0.0f, 1.0f, u, 0.0f },
                { left, bottom, 0.0f, 1.0f, 0.0f, v },
                { right, bottom, 0.0f, 1.0f, u, v }
            };
            Device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
            Device->SetPixelShader(0);
            Device->SetRenderState(D3DRS_ZENABLE, FALSE);
            Device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            Device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            Device->SetRenderState(D3DRS_FOGENABLE, FALSE);
            Device->SetRenderState(D3DRS_LIGHTING, FALSE);
            Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            Device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            Device->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
            Device->SetTexture(0, Texture);
            Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            Device->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
            Device->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
            Device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
            Device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
            Device->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
            Device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            Device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            Device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            drawn = Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
            Device->SetTexture(0, nullptr);
            Device->EndScene();
        }
        Device->ApplyStateBlock(state);
        Device->DeleteStateBlock(state);
        Device->SetViewport(&previousViewport);
        const auto presented = Device->Present(nullptr, nullptr, nullptr, nullptr);
        return FAILED(drawn) ? drawn : presented;
    }

    inline int __cdecl Setup(const BITMAPINFOHEADER* info, int width, int height, HWND window)
    {
        if (Prepare(GetDevice(), info))
        {
            if (*NativeCallback != Render)
                PreviousCallback = *NativeCallback;
            ProtectedGameRef<Callback>{ NativeCallback } = Render;
            return 0;
        }
        if (*NativeCallback == Render)
            ProtectedGameRef<Callback>{ NativeCallback } = PreviousCallback;
        OutputDebugStringA("THPS3: movie texture renderer unavailable; using native playback.\n");
        return SetupHook.ccall<int>(info, width, height, window);
    }

    inline int __cdecl Cleanup()
    {
        if (NativeCallback && *NativeCallback == Render)
            ProtectedGameRef<Callback>{ NativeCallback } = PreviousCallback;
        Release();
        return CleanupHook.ccall<int>();
    }

    inline void Install(const int32_t* mode, HMODULE module = GetModuleHandleW(nullptr))
    {
        Mode = mode;
        auto setup = hook::module_pattern(module, "83 EC 2C 8B 44 24 3C 56 57 50 FF 15 ? ? ? ? DB 44 24 3C A3");
        auto cleanup = hook::module_pattern(module, "A1 ? ? ? ? 56 33 F6 3B C6 74 0D 50 FF 15 ? ? ? ? 89 35 ? ? ? ? A1");
        auto callback = hook::module_pattern(module, "8B 44 24 14 8B 4C 24 10 50 51 FF 15 ? ? ? ? 83 C4 08 33 C0 C2 14 00");
        auto getter = hook::module_pattern(module, "E8 ? ? ? ? 8B 4C 24 3C 33 D2 3B CE A3 ? ? ? ? 7E 0E");
        if (setup.size() != 1 || cleanup.size() != 1 || callback.size() != 1 || getter.size() != 1)
        {
            OutputDebugStringA("THPS3: fullscreen movie patterns unavailable.\n");
            return;
        }
        NativeCallback = *callback.get_first<Callback*>(12);
        GetDevice = reinterpret_cast<decltype(GetDevice)>(injector::GetBranchDestination(getter.get_first()).as_int());
        SetupHook = safetyhook::create_inline(setup.get_first(), Setup);
        CleanupHook = safetyhook::create_inline(cleanup.get_first(), Cleanup);
        if (!SetupHook || !CleanupHook)
        {
            SetupHook.reset();
            CleanupHook.reset();
            OutputDebugStringA("THPS3: fullscreen movie hooks unavailable.\n");
        }
    }
}
