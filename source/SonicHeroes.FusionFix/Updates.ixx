module;

#include <stdafx.h>
#include <bit>
#include <wrl/client.h>
#include <minidx8/d3d8.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

#pragma comment(lib, "gdi32.lib")

export module Updates;

import ComVars;

export
{
namespace Updates
{
    using Microsoft::WRL::ComPtr;

    SafetyHookInline PresentHook;
    IDirect3DDevice8* ActiveDevice = nullptr;
    ComPtr<IDirect3DTexture8> Texture;
    std::wstring Notice;
    ULONGLONG Until = 0;
    DWORD CanvasWidth = 0, CanvasHeight = 0, TextureWidth = 0, TextureHeight = 0;
    int TextWidth = 0, TextHeight = 0;

    bool InFrontend()
    {
        const auto state = UpdateGameState.get();
        return state && *reinterpret_cast<int32_t*>(state + 0x38) == 1;
    }

    bool CreateNoticeTexture(IDirect3DDevice8* device, DWORD width, DWORD height)
    {
        const auto dc = CreateCompatibleDC(nullptr);
        if (!dc)
            return false;
        const auto font = CreateFontW(-std::clamp(int(height / 39), 13, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        const auto previousFont = font ? SelectObject(dc, font) : nullptr;
        RECT text{ 0, 0, LONG(width * 3 / 4), 0 };
        constexpr UINT layout = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX;
        DrawTextW(dc, Notice.c_str(), -1, &text, layout | DT_CALCRECT);
        const int drawnWidth = text.right + 20;
        const int drawnHeight = text.bottom + 14;
        const auto textureWidth = std::bit_ceil(DWORD(drawnWidth));
        const auto textureHeight = std::bit_ceil(DWORD(drawnHeight));

        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = drawnWidth;
        info.bmiHeader.biHeight = -drawnHeight;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* pixels = nullptr;
        const auto bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        bool uploaded = false;
        if (bitmap && pixels)
        {
            const auto previousBitmap = SelectObject(dc, bitmap);
            SetTextColor(dc, RGB(255, 255, 255));
            SetBkMode(dc, TRANSPARENT);
            OffsetRect(&text, 10, 7);
            std::memset(pixels, 0, size_t(drawnWidth) * drawnHeight * sizeof(DWORD));
            DrawTextW(dc, Notice.c_str(), -1, &text, layout);
            GdiFlush();

            ComPtr<IDirect3DTexture8> generated;
            D3DLOCKED_RECT locked{};
            if (SUCCEEDED(device->CreateTexture(textureWidth, textureHeight, 1, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_MANAGED, generated.GetAddressOf())) && SUCCEEDED(generated->LockRect(0, &locked, nullptr, 0)))
            {
                const auto src = static_cast<const DWORD*>(pixels);
                for (DWORD y = 0; y < textureHeight; ++y)
                {
                    const auto target = reinterpret_cast<DWORD*>(static_cast<uint8_t*>(locked.pBits) + y * locked.Pitch);
                    std::fill_n(target, textureWidth, 0);
                    if (y >= DWORD(drawnHeight))
                        continue;
                    for (int x = 0; x < drawnWidth; ++x)
                    {
                        const DWORD ink = src[y * drawnWidth + x] & 255;
                        target[x] = ((205 + ink * 50 / 255) << 24) | (ink * 0x010101u);
                    }
                }
                uploaded = SUCCEEDED(generated->UnlockRect(0));
                if (uploaded)
                {
                    Texture = std::move(generated);
                    CanvasWidth = width;
                    CanvasHeight = height;
                    TextureWidth = textureWidth;
                    TextureHeight = textureHeight;
                    TextWidth = drawnWidth;
                    TextHeight = drawnHeight;
                }
            }
            SelectObject(dc, previousBitmap);
            DeleteObject(bitmap);
        }
        if (font)
        {
            SelectObject(dc, previousFont);
            DeleteObject(font);
        }
        DeleteDC(dc);
        return uploaded;
    }

    void DrawNotice(IDirect3DDevice8* device)
    {
        if (Notice.empty())
            return;
        if (GetTickCount64() >= Until)
        {
            Notice.clear();
            Texture.Reset();
            return;
        }
        if (!InFrontend())
            return;

        ComPtr<IDirect3DSurface8> backbuffer, target, depth;
        D3DSURFACE_DESC desc{};
        D3DVIEWPORT8 original{};
        if (FAILED(device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, backbuffer.GetAddressOf())) ||
            FAILED(backbuffer->GetDesc(&desc)) || FAILED(device->GetRenderTarget(target.GetAddressOf())) ||
            FAILED(device->GetViewport(&original)))
            return;
        if ((!Texture || CanvasWidth != desc.Width || CanvasHeight != desc.Height) && !CreateNoticeTexture(device, desc.Width, desc.Height))
            return;

        DWORD state = 0;
        if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state)))
            return;
        if (SUCCEEDED(device->CaptureStateBlock(state)) && SUCCEEDED(device->BeginScene()))
        {
            const bool otherTarget = target.Get() != backbuffer.Get();
            if (otherTarget)
                device->GetDepthStencilSurface(depth.GetAddressOf());
            if (!otherTarget || SUCCEEDED(device->SetRenderTarget(backbuffer.Get(), nullptr)))
            {
                const D3DVIEWPORT8 viewport{ 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
                device->SetViewport(&viewport);
                device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
                device->SetPixelShader(0);
                device->SetRenderState(D3DRS_ZENABLE, FALSE);
                device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
                device->SetRenderState(D3DRS_LIGHTING, FALSE);
                device->SetRenderState(D3DRS_FOGENABLE, FALSE);
                device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
                device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
                device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
                device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
                device->SetTexture(0, Texture.Get());
                device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
                device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
                device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
                device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
                device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

                struct Vertex { float x, y, z, rhw, u, v; };
                const float left = (float(desc.Width) - TextWidth) * 0.5f - 0.5f;
                const float top = std::max(0.0f, float(desc.Height) * 0.20f - TextHeight * 0.5f) - 0.5f;
                const float right = left + TextWidth, bottom = top + TextHeight;
                const float u = float(TextWidth) / TextureWidth, v = float(TextHeight) / TextureHeight;
                const Vertex vertices[]{
                    { left, top, 0.0f, 1.0f, 0.0f, 0.0f }, { right, top, 0.0f, 1.0f, u, 0.0f },
                    { left, bottom, 0.0f, 1.0f, 0.0f, v }, { right, bottom, 0.0f, 1.0f, u, v },
                };
                device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
                if (otherTarget)
                    device->SetRenderTarget(target.Get(), depth.Get());
            }
            device->EndScene();
            device->ApplyStateBlock(state);
            device->SetViewport(&original);
        }
        device->DeleteStateBlock(state);
    }

    HRESULT WINAPI Present(IDirect3DDevice8* device, const RECT* source, const RECT* destination, HWND window, const RGNDATA* dirty)
    {
        if (device == ActiveDevice)
            DrawNotice(device);
        return PresentHook.unsafe_stdcall<HRESULT>(device, source, destination, window, dirty);
    }

    void Frame()
    {
        RWGTA::UpdateCheck::Tick();
        if (Notice.empty() || !HudDevice.is_initialized())
            return;
        const auto device = HudDevice.get();
        if (device && ActiveDevice != device)
        {
            Texture.Reset();
            PresentHook.reset();
            const auto methods = *reinterpret_cast<void***>(device);
            PresentHook = safetyhook::create_inline(methods[15], Present);
            ActiveDevice = device;
        }
    }

    void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }
}

void InitUpdates()
{
    UpdateGameState.SetAddress(*hook::pattern("8B 46 04 53 55 33 ED 3B C5 57 89 2E BF 01 00 00 00 75 16").get_first<uintptr_t*>(0x14));
    RWGTA::UpdateCheck::Config config;
    config.pluginId = "SonicHeroes.FusionFix";
    config.fileVersion = rsc_FileVersion;
    config.settings = &FixSettings();
    config.playable = []()
    {
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        return Updates::InFrontend() && process == GetCurrentProcessId();
    };
    config.format = [](const std::string& latest, bool hub)
    {
        // Automatic: the Windows display language when the texts have it, else English
        auto language = FixSettings().Get("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
        {
            switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
            {
                case LANG_FRENCH: language = "fr"; break;
                case LANG_GERMAN: language = "de"; break;
                case LANG_ITALIAN: language = "it"; break;
                case LANG_SPANISH: language = "es"; break;
                default: language = "en"; break;
            }
        }
        auto texts = FixSettings().GetTexts();
        texts.Select(language);
        auto line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
        for (const auto& [key, value] : { std::pair<std::string, std::string>{ "{name}", texts.Resolve("@plugin.name") }, { "{version}", latest } })
            if (const auto at = line.find(key); at != std::string::npos)
                line.replace(at, key.size(), value);
        return asi_settings::text::Widen(line);
    };
    config.show = [](const std::wstring& line)
    {
        Updates::Texture.Reset();
        Updates::Notice = line;
        Updates::Until = GetTickCount64() + 12000;
    };
    RWGTA::UpdateCheck::Init(std::move(config));
    WFP::onGameProcessEvent() += Updates::Frame;
    IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Updates::Exit));
}
}
