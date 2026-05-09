module;

#include <stdafx.h>
#include "common.h"
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

export module ConsoleGamma;

import Skeleton;

template <typename T>
static inline void SafeRelease(T*& p)
{
    if (p) { p->Release(); p = nullptr; }
}

export class ConsoleGamma
{
public:
    ConsoleGamma()
    {
        CIniReader iniReader("");
        auto bConsoleGamma = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 0) != 0;

        if (bConsoleGamma)
        {
            WFP::onEndScene() += []()
            {
                Render();
            };

            WFP::onBeforeReset() += []()
            {
                OnDeviceReset();
            };
        }
    }

private:
    static inline IDirect3DTexture9* pSceneTex = nullptr;
    static inline IDirect3DSurface9* pSceneSurf = nullptr;
    static inline IDirect3DVertexDeclaration9* pQuadDecl = nullptr;
    static inline ID3DXEffect* pEffect = nullptr;

    static inline UINT nScreenWidth = 0;
    static inline UINT nScreenHeight = 0;
    static inline bool bInitialized = false;

    static inline D3DXHANDLE hInputTex2D = nullptr;
    static inline D3DXHANDLE hGammaTechnique = nullptr;

    static IDirect3DDevice9* GetDevice9()
    {
        if (!pD3D8Device || !*pD3D8Device) return nullptr;

        IDirect3DDevice9* dev9 = nullptr;
        if (FAILED((*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&dev9)))
            return nullptr;

        dev9->Release();
        return dev9;
    }

    static bool Initialize(IDirect3DDevice9* dev)
    {
        if (bInitialized) return true;

        IDirect3DSurface9* bb = nullptr;
        if (FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)) || !bb)
            return false;

        D3DSURFACE_DESC desc{};
        bb->GetDesc(&desc);
        bb->Release();

        if (!desc.Width || !desc.Height) return false;

        nScreenWidth = desc.Width;
        nScreenHeight = desc.Height;

        if (FAILED(dev->CreateTexture(nScreenWidth, nScreenHeight, 1,
            D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &pSceneTex, nullptr)))
            return false;

        if (FAILED(pSceneTex->GetSurfaceLevel(0, &pSceneSurf)))
        {
            SafeRelease(pSceneTex); return false;
        }

        D3DVERTEXELEMENT9 decl[] = {
            {0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0},
            {0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0},
            D3DDECL_END()
        };
        if (FAILED(dev->CreateVertexDeclaration(decl, &pQuadDecl)))
        {
            ReleaseResources(); return false;
        }

        HMODULE hModule = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&Initialize, &hModule);

        ID3DXBuffer* errors = nullptr;
        HRESULT hr = D3DXCreateEffectFromResource(dev, hModule, MAKEINTRESOURCE(IDR_POSTFX),
            nullptr, nullptr, 0, nullptr, &pEffect, &errors);

        if (FAILED(hr) || !pEffect)
        {
            if (errors) { OutputDebugStringA((const char*)errors->GetBufferPointer()); errors->Release(); }
            OutputDebugStringA("ConsoleGammaDX9: Failed to load shader\n");
            ReleaseResources();
            return false;
        }
        SafeRelease(errors);

        hInputTex2D = pEffect->GetParameterByName(nullptr, "InputTex2D");
        hGammaTechnique = pEffect->GetTechniqueByName("ConsoleGamma");

        bInitialized = true;
        return true;
    }

    static void DrawScreenQuad(IDirect3DDevice9* dev)
    {
        struct Vert { float x, y, z, rhw, u, v; };
        Vert v[4] =
        {
            {-0.5f,                      -0.5f,                      0.0f, 1.0f, 0.0f, 0.0f},
            {-0.5f,                      float(nScreenHeight) - 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
            {float(nScreenWidth) - 0.5f, -0.5f,                      0.0f, 1.0f, 1.0f, 0.0f},
            {float(nScreenWidth) - 0.5f, float(nScreenHeight) - 0.5f, 0.0f, 1.0f, 1.0f, 1.0f}
        };
        dev->SetVertexShader(nullptr);
        dev->SetVertexDeclaration(pQuadDecl);
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(Vert));
    }

    static void ReleaseResources()
    {
        SafeRelease(pEffect);
        SafeRelease(pQuadDecl);
        SafeRelease(pSceneSurf);
        SafeRelease(pSceneTex);
        nScreenWidth = 0;
        nScreenHeight = 0;
        bInitialized = false;
    }

public:
    static void Render()
    {
        IDirect3DDevice9* dev = GetDevice9();
        if (!dev) return;

        if (!Initialize(dev)) return;

        IDirect3DSurface9* currentRT = nullptr;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        dev->StretchRect(currentRT, nullptr, pSceneSurf, nullptr, D3DTEXF_POINT);

        IDirect3DStateBlock9* state = nullptr;
        if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &state)) || !state)
        {
            currentRT->Release(); return;
        }

        pEffect->SetTexture(hInputTex2D, pSceneTex);
        pEffect->SetTechnique(hGammaTechnique);

        UINT passes = 0;
        if (SUCCEEDED(pEffect->Begin(&passes, 0)))
        {
            if (SUCCEEDED(pEffect->BeginPass(0)))
            {
                DrawScreenQuad(dev);
                pEffect->EndPass();
            }
            pEffect->End();
        }

        state->Apply();
        state->Release();
        currentRT->Release();
    }

    static void OnDeviceReset()
    {
        ReleaseResources();
    }
} ConsoleGamma;