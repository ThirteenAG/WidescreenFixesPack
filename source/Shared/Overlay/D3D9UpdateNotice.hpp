#pragma once

#include "asi_settings/asi_settings.hpp"
#include <d3d9.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <string>
#include <vector>

namespace asi_overlay
{
    // An in-game update line: the text drawn with GDI into a texture, shown as a box near the bottom of the screen.
    // No input handling.
    class D3D9UpdateNotice
    {
    public:
        static std::wstring Format(asi_settings::Plugin& settings, std::string_view fallbackLanguage, const std::string& latest, bool hub)
        {
            auto texts = settings.GetTexts();
            const auto language = settings.Get("MAIN.TextLanguage");
            texts.Select(language.empty() || language == "auto" ? fallbackLanguage : std::string_view(language));
            auto line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
            const std::pair<std::string_view, std::string> replacements[] =
            {
                { "{name}", texts.Resolve(settings.GetDescription().name) },
                { "{version}", latest },
            };
            for (const auto& [key, value] : replacements)
            {
                const auto start = line.find(key);
                if (start != std::string::npos)
                    line.replace(start, key.size(), value);
            }
            return asi_settings::text::Widen(line);
        }

        // From any thread (the update check's)
        void Show(const std::wstring& text)
        {
            std::lock_guard lock(mutex);
            shownText = text;
            noticeUntil = GetTickCount64() + 12000;
        }

        // Before the device's Reset: the texture lives in the default pool
        void BeforeReset()
        {
            Release();
        }

        void Present(IDirect3DDevice9* device, HMODULE, bool playable)
        {
            {
                std::lock_guard lock(mutex);
                if (shownText.empty() && noticeText.empty()) return;
                if (GetTickCount64() >= noticeUntil)
                {
                    shownText.clear();
                    noticeText.clear();
                    Release();
                    return;
                }
                if (shownText != noticeText)
                {
                    noticeText = shownText;
                    Release();
                }
            }
            if (!playable || !device) return;
            if (noticeDevice && device != noticeDevice) Release();

            using Microsoft::WRL::ComPtr;
            ComPtr<IDirect3DSurface9> backbuffer, target, depth;
            ComPtr<IDirect3DStateBlock9> state;
            D3DSURFACE_DESC desc{};
            if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, backbuffer.GetAddressOf())) ||
                FAILED(backbuffer->GetDesc(&desc)) || FAILED(device->GetRenderTarget(0, target.GetAddressOf())))
                return;
            if (!texture || desc.Height != textureForHeight)
            {
                Release();
                if (!Create(device, desc.Width, desc.Height)) return;
                noticeDevice = device;
            }
            if (FAILED(device->CreateStateBlock(D3DSBT_ALL, state.GetAddressOf())) || FAILED(state->Capture()))
                return;
            if (FAILED(device->BeginScene())) return;
            if (backbuffer.Get() != target.Get())
            {
                device->GetDepthStencilSurface(depth.GetAddressOf());
                device->SetDepthStencilSurface(nullptr);
                device->SetRenderTarget(0, backbuffer.Get());
            }

            const D3DVIEWPORT9 viewport{ 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
            device->SetViewport(&viewport);
            device->SetVertexShader(nullptr);
            device->SetPixelShader(nullptr);
            device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
            device->SetRenderState(D3DRS_ZENABLE, FALSE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            device->SetRenderState(D3DRS_LIGHTING, FALSE);
            device->SetRenderState(D3DRS_FOGENABLE, FALSE);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
            device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
            device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
            device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
            device->SetTexture(0, texture.Get());
            device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
            device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
            device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
            device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);

            // The box's texels map 1:1 to pixels: D3D9 samples at pixel centres, so the quad goes half a pixel up-left
            const float x = std::floor((float(desc.Width) - float(boxWidth)) * 0.5f) - 0.5f;
            const float y = std::floor(std::max(0.0f, float(desc.Height) * 0.93f - float(boxHeight))) - 0.5f;
            const float w = float(boxWidth), h = float(boxHeight);
            const float u = w / float(textureWidth), v = h / float(textureHeight);
            struct Vertex { float x, y, z, rhw, u, v; };
            const Vertex quad[] =
            {
                { x, y, 0.0f, 1.0f, 0.0f, 0.0f },
                { x + w, y, 0.0f, 1.0f, u, 0.0f },
                { x, y + h, 0.0f, 1.0f, 0.0f, v },
                { x + w, y + h, 0.0f, 1.0f, u, v },
            };
            device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));

            device->EndScene();
            if (backbuffer.Get() != target.Get())
            {
                device->SetRenderTarget(0, target.Get());
                device->SetDepthStencilSurface(depth.Get());
            }
            state->Apply();
        }

    private:
        Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
        IDirect3DDevice9* noticeDevice = nullptr;
        std::mutex mutex;
        std::wstring shownText;     // the last Show's text
        std::wstring noticeText;    // the text in the texture
        ULONGLONG noticeUntil = 0;
        UINT textureForHeight = 0, textureWidth = 0, textureHeight = 0, boxWidth = 0, boxHeight = 0;

        void Release()
        {
            texture.Reset();
            noticeDevice = nullptr;
            textureForHeight = 0;
        }

        // The line in Segoe UI, white on a translucent black box, wrapped at 76% of the screen's width
        bool Create(IDirect3DDevice9* device, UINT width, UINT height)
        {
            const int fontSize = std::max(14, int(height / 42));
            const int padX = std::max(12, fontSize / 2), padY = std::max(7, fontSize / 4);
            const int wrap = std::max(1, int(width * 0.76f));
            HDC dc = CreateCompatibleDC(nullptr);
            if (!dc) return false;
            HFONT font = CreateFontW(-fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            HGDIOBJ oldFont = font ? SelectObject(dc, font) : nullptr;
            RECT bounds{ 0, 0, wrap, 0 };
            DrawTextW(dc, noticeText.c_str(), int(noticeText.size()), &bounds, DT_CALCRECT | DT_WORDBREAK | DT_CENTER | DT_NOPREFIX);
            const int textWidth = std::min<int>(bounds.right - bounds.left, wrap), textHeight = bounds.bottom - bounds.top;
            boxWidth = UINT(std::min<int>(textWidth + padX * 2, int(width)));
            boxHeight = UINT(std::min<int>(textHeight + padY * 2, int(height)));

            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(info.bmiHeader);
            info.bmiHeader.biWidth = int(boxWidth);
            info.bmiHeader.biHeight = -int(boxHeight);
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            void* bits = nullptr;
            HBITMAP bitmap = boxWidth && boxHeight ? CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
            bool created = false;
            if (bitmap && bits)
            {
                HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
                memset(bits, 0, size_t(boxWidth) * boxHeight * 4);
                SetBkMode(dc, TRANSPARENT);
                SetTextColor(dc, RGB(255, 255, 255));
                RECT text{ padX, padY, int(boxWidth) - padX, int(boxHeight) - padY };
                DrawTextW(dc, noticeText.c_str(), int(noticeText.size()), &text, DT_WORDBREAK | DT_CENTER | DT_NOPREFIX);
                GdiFlush();
                created = Upload(device, static_cast<const uint32_t*>(bits));
                SelectObject(dc, oldBitmap);
            }
            if (bitmap) DeleteObject(bitmap);
            if (oldFont) SelectObject(dc, oldFont);
            if (font) DeleteObject(font);
            DeleteDC(dc);
            if (created) textureForHeight = height;
            return created;
        }

        // White text over a black box of 80% opacity, as one layer for SRCALPHA blending
        bool Upload(IDirect3DDevice9* device, const uint32_t* pixels)
        {
            D3DCAPS9 caps{};
            device->GetDeviceCaps(&caps);
            textureWidth = boxWidth;
            textureHeight = boxHeight;
            if ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) && !(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
            {
                textureWidth = 1;
                while (textureWidth < boxWidth) textureWidth <<= 1;
                textureHeight = 1;
                while (textureHeight < boxHeight) textureHeight <<= 1;
            }
            if (FAILED(device->CreateTexture(textureWidth, textureHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, texture.ReleaseAndGetAddressOf(), nullptr)))
                return false;
            D3DLOCKED_RECT locked{};
            if (FAILED(texture->LockRect(0, &locked, nullptr, D3DLOCK_DISCARD)))
            {
                texture.Reset();
                return false;
            }
            constexpr float box = 205.0f / 255.0f;
            for (UINT row = 0; row < textureHeight; ++row)
            {
                auto out = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(locked.pBits) + size_t(row) * locked.Pitch);
                for (UINT column = 0; column < textureWidth; ++column)
                {
                    if (row >= boxHeight || column >= boxWidth)
                    {
                        out[column] = 0;
                        continue;
                    }
                    const uint32_t p = pixels[size_t(row) * boxWidth + column];
                    const float coverage = float(std::max({ (p >> 16) & 0xFF, (p >> 8) & 0xFF, p & 0xFF })) / 255.0f;
                    const float alpha = coverage + box * (1.0f - coverage);
                    const uint32_t gray = uint32_t(std::clamp(coverage / alpha, 0.0f, 1.0f) * 255.0f + 0.5f);
                    out[column] = (uint32_t(alpha * 255.0f + 0.5f) << 24) | (gray << 16) | (gray << 8) | gray;
                }
            }
            texture->UnlockRect(0);
            return true;
        }
    };
}
