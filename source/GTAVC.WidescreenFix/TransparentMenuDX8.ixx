module;

#include <stdafx.h>
#include "common.h"
#include <minidx8/d3d8.h>

export module TransparentMenuDX8;

import Skeleton;
import Menu;

struct ScreenVertex
{
    float x, y, z, rhw;
    DWORD color;
    float u, v;
};

#define D3DFVF_SCREENVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

export class TransparentMenuDX8
{
private:
    struct Resources
    {
        IDirect3DSurface8* captureSurf = nullptr; // D3DPOOL_SYSTEMMEM — safe to CopyRects into
        IDirect3DTexture8* renderTex = nullptr; // D3DPOOL_MANAGED   — safe for SetTexture, no RT state side-effects
        int fbWidth = 0;
        int fbHeight = 0;
        size_t rowBytes = 0;

        bool initialised = false;
        bool captureReady = false;
    };

    static inline Resources s_res;

    static IDirect3DDevice8* GetDevice8()
    {
        return *reinterpret_cast<IDirect3DDevice8**>(pD3D8Device);
    }

    static bool IsDeviceReady()
    {
        auto* dev = GetDevice8();
        if (!dev) return false;
        return dev->TestCooperativeLevel() == D3D_OK;
    }

    static void InitResources(IDirect3DDevice8* dev);

public:
    static bool CaptureFrame();
    static void RenderBlur();
    static void OnDeviceReset();
};

void TransparentMenuDX8::InitResources(IDirect3DDevice8* dev)
{
    if (s_res.initialised) return;

    IDirect3DSurface8* backBuffer = nullptr;
    if (FAILED(dev->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer))) return;

    D3DSURFACE_DESC desc{};
    const HRESULT descResult = backBuffer->GetDesc(&desc);
    backBuffer->Release();
    if (FAILED(descResult) || !desc.Width || !desc.Height) return;

    size_t bytesPerPixel = 0;
    switch (desc.Format)
    {
    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
        bytesPerPixel = 2;
        break;
    case D3DFMT_R8G8B8:
        bytesPerPixel = 3;
        break;
    case D3DFMT_X8R8G8B8:
    case D3DFMT_A8R8G8B8:
        bytesPerPixel = 4;
        break;
    default:
        return; // Unsupported capture format; do not guess its layout.
    }
    if (desc.Width > SIZE_MAX / bytesPerPixel) return;

    // System-memory surface — CopyRects from back buffer is valid here,
    // and does NOT touch any render-target or transform internal state.
    if (FAILED(dev->CreateImageSurface(desc.Width, desc.Height, desc.Format, &s_res.captureSurf)))
        return;

    // Managed texture — no D3DUSAGE_RENDERTARGET, so the driver never
    // re-points its internal RT / transform matrix pointers on creation.
    if (FAILED(dev->CreateTexture(desc.Width, desc.Height, 1,
        0, desc.Format, D3DPOOL_MANAGED, &s_res.renderTex)))
    {
        s_res.captureSurf->Release();
        s_res.captureSurf = nullptr;
        return;
    }

    s_res.rowBytes = static_cast<size_t>(desc.Width) * bytesPerPixel;
    s_res.fbWidth = desc.Width;
    s_res.fbHeight = desc.Height;
    s_res.initialised = true;
}

bool TransparentMenuDX8::CaptureFrame()
{
    s_res.captureReady = false;
    if (!IsDeviceReady()) return false;
    auto* dev = GetDevice8();
    if (!dev) return false;

    InitResources(dev);
    if (!s_res.captureSurf || !s_res.renderTex) return false;

    IDirect3DSurface8* backBuffer = nullptr;
    if (FAILED(dev->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer))) return false;

    // Copy back buffer → system-memory surface (always legal in DX8).
    HRESULT hr = dev->CopyRects(backBuffer, nullptr, 0, s_res.captureSurf, nullptr);
    backBuffer->Release();
    if (FAILED(hr)) return false;

    // Upload system-memory surface → managed texture via lock/blit.
    IDirect3DSurface8* texSurf = nullptr;
    if (FAILED(s_res.renderTex->GetSurfaceLevel(0, &texSurf)) || !texSurf) return false;

    D3DLOCKED_RECT srcLock{}, dstLock{};
    if (SUCCEEDED(s_res.captureSurf->LockRect(&srcLock, nullptr, D3DLOCK_READONLY)))
    {
        if (SUCCEEDED(texSurf->LockRect(&dstLock, nullptr, 0)))
        {
            if (srcLock.pBits && dstLock.pBits && srcLock.Pitch > 0 && dstLock.Pitch > 0 &&
                s_res.rowBytes <= static_cast<size_t>(srcLock.Pitch) &&
                s_res.rowBytes <= static_cast<size_t>(dstLock.Pitch))
            {
                const auto* src = static_cast<const uint8_t*>(srcLock.pBits);
                auto* dst = static_cast<uint8_t*>(dstLock.pBits);
                for (int y = 0; y < s_res.fbHeight; ++y)
                    memcpy(dst + static_cast<size_t>(y) * dstLock.Pitch,
                        src + static_cast<size_t>(y) * srcLock.Pitch, s_res.rowBytes);
                s_res.captureReady = true;
            }
            texSurf->UnlockRect();
        }
        s_res.captureSurf->UnlockRect();
    }
    texSurf->Release();

    return s_res.captureReady;
}

void TransparentMenuDX8::RenderBlur()
{
    if (!IsDeviceReady()) return;
    if (!s_res.captureReady || !s_res.initialised || !s_res.renderTex) return;

    auto* dev = GetDevice8();
    if (!dev) return;

    DWORD stateBlock = 0;
    if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &stateBlock)) || !stateBlock)
        return;
    dev->CaptureStateBlock(stateBlock);

    dev->SetTexture(0, s_res.renderTex);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);

    dev->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 0.0f, 0);

    const float texelX = 1.0f / static_cast<float>(s_res.fbWidth);
    const float texelY = 1.0f / static_cast<float>(s_res.fbHeight);

    constexpr int   kernelSize = 2;
    constexpr float blurStrength = 5.0f;
    constexpr float darkness = 0.2f;

    const float offsetX = texelX * blurStrength;
    const float offsetY = texelY * blurStrength;

    constexpr int  numSamples = (2 * kernelSize + 1) * (2 * kernelSize + 1);
    const BYTE     weight = static_cast<BYTE>(255 / numSamples);
    const D3DCOLOR sampleColor = D3DCOLOR_ARGB(255, weight, weight, weight);

    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    dev->SetVertexShader(D3DFVF_SCREENVERTEX);

    for (int oy = -kernelSize; oy <= kernelSize; ++oy)
    {
        for (int ox = -kernelSize; ox <= kernelSize; ++ox)
        {
            float shiftU = static_cast<float>(ox) * offsetX;
            float shiftV = static_cast<float>(oy) * offsetY;

            ScreenVertex quad[4] =
            {
                {0.0f,                              0.0f,                               0.0f, 1.0f, sampleColor, 0.0f + shiftU, 0.0f + shiftV},
                {static_cast<float>(s_res.fbWidth), 0.0f,                               0.0f, 1.0f, sampleColor, 1.0f + shiftU, 0.0f + shiftV},
                {static_cast<float>(s_res.fbWidth), static_cast<float>(s_res.fbHeight), 0.0f, 1.0f, sampleColor, 1.0f + shiftU, 1.0f + shiftV},
                {0.0f,                              static_cast<float>(s_res.fbHeight), 0.0f, 1.0f, sampleColor, 0.0f + shiftU, 1.0f + shiftV}
            };

            dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, sizeof(ScreenVertex));
        }
    }

    dev->SetTexture(0, nullptr);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    D3DCOLOR darkColor = D3DCOLOR_ARGB(static_cast<BYTE>(darkness * 255), 0, 0, 0);

    ScreenVertex darkQuad[4] =
    {
        {0.0f,                              0.0f,                               0.0f, 1.0f, darkColor, 0.0f, 0.0f},
        {static_cast<float>(s_res.fbWidth), 0.0f,                               0.0f, 1.0f, darkColor, 0.0f, 0.0f},
        {static_cast<float>(s_res.fbWidth), static_cast<float>(s_res.fbHeight), 0.0f, 1.0f, darkColor, 0.0f, 0.0f},
        {0.0f,                              static_cast<float>(s_res.fbHeight), 0.0f, 1.0f, darkColor, 0.0f, 0.0f}
    };

    dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, darkQuad, sizeof(ScreenVertex));

    dev->ApplyStateBlock(stateBlock);
    dev->DeleteStateBlock(stateBlock);
}

void TransparentMenuDX8::OnDeviceReset()
{
    s_res.captureReady = false;
    s_res.initialised = false;

    if (s_res.captureSurf)
    {
        s_res.captureSurf->Release();
        s_res.captureSurf = nullptr;
    }

    if (s_res.renderTex)
    {
        s_res.renderTex->Release();
        s_res.renderTex = nullptr;
    }

    s_res.fbWidth = 0;
    s_res.fbHeight = 0;
    s_res.rowBytes = 0;
}