module;

#include <stdafx.h>
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

export module TransparentMenuDX9;

import Skeleton;

export IDirect3DDevice9* pD3D9Device = nullptr;

template <typename T>
static inline void SafeRelease(T*& p)
{
    if (p) { p->Release(); p = nullptr; }
}

export class TransparentMenuDX9
{
private:
    static inline IDirect3DTexture9* pSceneTex = nullptr;
    static inline IDirect3DSurface9* pSceneSurf = nullptr;
    static inline IDirect3DTexture9* pBlurTex = nullptr;
    static inline IDirect3DSurface9* pBlurSurf = nullptr;
    static inline IDirect3DVertexDeclaration9* pQuadDecl = nullptr;
    static inline ID3DXEffect* pBlurEffect = nullptr;

    static inline UINT nScreenWidth = 0;
    static inline UINT nScreenHeight = 0;
    static inline bool bInitialized = false;
    static inline bool captureReady = false;

    struct Handles
    {
        D3DXHANDLE InputTex2D = nullptr;
        D3DXHANDLE TexelSize = nullptr;
        D3DXHANDLE BlurStrengthParam = nullptr;
        D3DXHANDLE BlurHorizontalTechnique = nullptr;
        D3DXHANDLE BlurVerticalTechnique = nullptr;
        D3DXHANDLE DarknessParam = nullptr;
        D3DXHANDLE BlurVerticalDarkenTechnique = nullptr;

        void GetHandles(ID3DXEffect* effect)
        {
            InputTex2D = effect->GetParameterByName(nullptr, "InputTex2D");
            TexelSize = effect->GetParameterByName(nullptr, "TexelSize");
            BlurStrengthParam = effect->GetParameterByName(nullptr, "BlurStrength");
            DarknessParam = effect->GetParameterByName(nullptr, "Darkness");
            BlurHorizontalTechnique = effect->GetTechniqueByName("BlurHorizontal");
            BlurVerticalTechnique = effect->GetTechniqueByName("BlurVertical");
            BlurVerticalDarkenTechnique = effect->GetTechniqueByName("BlurVerticalDarken");
        }
    } static inline handles;

    static bool IsDeviceReady()
    {
        if (!pD3D9Device) return false;
        const HRESULT hr = pD3D9Device->TestCooperativeLevel();
        return hr == D3D_OK;
    }

    static bool Initialize();
    static void DrawScreenQuad(IDirect3DDevice9* dev, UINT w, UINT h);
    static void ReleaseResources();

public:
    static bool CaptureFrame();
    static void RenderBlur();
    static void OnDeviceReset();
};

void TransparentMenuDX9::ReleaseResources()
{
    captureReady = false;
    bInitialized = false;
    nScreenWidth = 0;
    nScreenHeight = 0;

    SafeRelease(pBlurEffect);
    SafeRelease(pQuadDecl);
    SafeRelease(pBlurSurf);
    SafeRelease(pBlurTex);
    SafeRelease(pSceneSurf);
    SafeRelease(pSceneTex);
}

void TransparentMenuDX9::OnDeviceReset()
{
    ReleaseResources();
    SafeRelease(pD3D9Device);
}

bool TransparentMenuDX9::Initialize()
{
    if (bInitialized) return true;
    if (!IsDeviceReady()) return false;

    IDirect3DSurface9* bb = nullptr;
    if (FAILED(pD3D9Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)) || !bb)
        return false;

    D3DSURFACE_DESC desc{};
    bb->GetDesc(&desc);
    bb->Release();

    if (!desc.Width || !desc.Height)
        return false;

    nScreenWidth = desc.Width;
    nScreenHeight = desc.Height;

    HRESULT hr = pD3D9Device->CreateTexture(nScreenWidth, nScreenHeight, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &pSceneTex, nullptr);
    if (FAILED(hr) || !pSceneTex) { ReleaseResources(); return false; }

    hr = pSceneTex->GetSurfaceLevel(0, &pSceneSurf);
    if (FAILED(hr) || !pSceneSurf) { ReleaseResources(); return false; }

    hr = pD3D9Device->CreateTexture(nScreenWidth, nScreenHeight, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &pBlurTex, nullptr);
    if (FAILED(hr) || !pBlurTex) { ReleaseResources(); return false; }

    hr = pBlurTex->GetSurfaceLevel(0, &pBlurSurf);
    if (FAILED(hr) || !pBlurSurf) { ReleaseResources(); return false; }

    D3DVERTEXELEMENT9 decl[] = {
        {0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0},
        {0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0},
        D3DDECL_END()
    };

    hr = pD3D9Device->CreateVertexDeclaration(decl, &pQuadDecl);
    if (FAILED(hr) || !pQuadDecl) { ReleaseResources(); return false; }

    HMODULE hModule = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&Initialize, &hModule);

    ID3DXBuffer* errors = nullptr;
    hr = D3DXCreateEffectFromResource(pD3D9Device, hModule, MAKEINTRESOURCE(IDR_POSTFX),
        nullptr, nullptr, 0, nullptr, &pBlurEffect, &errors);

    if (FAILED(hr) || !pBlurEffect)
    {
        if (errors)
        {
            OutputDebugStringA((const char*)errors->GetBufferPointer());
        }
        SafeRelease(errors);
        OutputDebugStringA("TransparentMenuDX9: Failed to load shader resource\n");
        ReleaseResources();
        return false;
    }

    SafeRelease(errors);
    handles.GetHandles(pBlurEffect);
    bInitialized = true;
    return true;
}

bool TransparentMenuDX9::CaptureFrame()
{
    if (!IsDeviceReady()) return false;
    if (!Initialize()) return false;
    if (!pSceneSurf) return false;

    IDirect3DSurface9* currentRT = nullptr;
    if (FAILED(pD3D9Device->GetRenderTarget(0, &currentRT)) || !currentRT)
        return false;

    const HRESULT hr = pD3D9Device->StretchRect(currentRT, nullptr, pSceneSurf, nullptr, D3DTEXF_POINT);
    currentRT->Release();

    captureReady = SUCCEEDED(hr);
    return captureReady;
}

void TransparentMenuDX9::DrawScreenQuad(IDirect3DDevice9* dev, UINT w, UINT h)
{
    struct ScreenQuadVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    ScreenQuadVertex v[4] =
    {
        {-0.5f,           -0.5f,           0.0f, 1.0f, 0.0f, 0.0f},
        {-0.5f,           float(h) - 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
        {float(w) - 0.5f, -0.5f,           0.0f, 1.0f, 1.0f, 0.0f},
        {float(w) - 0.5f, float(h) - 0.5f, 0.0f, 1.0f, 1.0f, 1.0f}
    };

    dev->SetVertexShader(nullptr);
    dev->SetVertexDeclaration(pQuadDecl);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(ScreenQuadVertex));
}

void TransparentMenuDX9::RenderBlur()
{
    if (!IsDeviceReady()) return;
    if (!Initialize()) return;

    if (!captureReady || !pBlurEffect || !pSceneTex || !pBlurTex || !pBlurSurf || !pQuadDecl)
        return;

    if (!nScreenWidth || !nScreenHeight)
        return;

    constexpr float darkness = 0.0f;
    constexpr float blurStrength = 5.0f;

    IDirect3DStateBlock9* state = nullptr;
    if (FAILED(pD3D9Device->CreateStateBlock(D3DSBT_ALL, &state)) || !state)
        return;

    IDirect3DSurface9* originalRT = nullptr;
    if (FAILED(pD3D9Device->GetRenderTarget(0, &originalRT)) || !originalRT)
    {
        state->Apply();
        state->Release();
        return;
    }

    float texel[2] = { 1.0f / nScreenWidth, 1.0f / nScreenHeight };
    UINT passes = 0;
    HRESULT hr = S_OK;

    hr = pD3D9Device->SetRenderTarget(0, pBlurSurf);
    if (SUCCEEDED(hr)) hr = pBlurEffect->SetTexture(handles.InputTex2D, pSceneTex);
    if (SUCCEEDED(hr)) hr = pBlurEffect->SetFloatArray(handles.TexelSize, texel, 2);
    if (SUCCEEDED(hr)) hr = pBlurEffect->SetFloat(handles.BlurStrengthParam, blurStrength);
    if (SUCCEEDED(hr)) hr = pBlurEffect->SetTechnique(handles.BlurHorizontalTechnique);
    if (SUCCEEDED(hr)) hr = pBlurEffect->Begin(&passes, 0);
    if (SUCCEEDED(hr))
    {
        hr = pBlurEffect->BeginPass(0);
        if (SUCCEEDED(hr))
        {
            DrawScreenQuad(pD3D9Device, nScreenWidth, nScreenHeight);
            pBlurEffect->EndPass();
        }
        pBlurEffect->End();
    }

    if (SUCCEEDED(hr)) hr = pD3D9Device->SetRenderTarget(0, originalRT);
    if (SUCCEEDED(hr)) hr = pBlurEffect->SetTexture(handles.InputTex2D, pBlurTex);
    if (SUCCEEDED(hr)) hr = pBlurEffect->SetFloat(handles.DarknessParam, darkness);
    if (SUCCEEDED(hr)) hr = pBlurEffect->SetTechnique(handles.BlurVerticalDarkenTechnique);
    if (SUCCEEDED(hr)) hr = pBlurEffect->Begin(&passes, 0);
    if (SUCCEEDED(hr))
    {
        hr = pBlurEffect->BeginPass(0);
        if (SUCCEEDED(hr))
        {
            DrawScreenQuad(pD3D9Device, nScreenWidth, nScreenHeight);
            pBlurEffect->EndPass();
        }
        pBlurEffect->End();
    }

    state->Apply();
    state->Release();
    originalRT->Release();
}