module;

#include <stdafx.h>
#include "common.h"
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

export module PostFX;

import Skeleton;

template <typename T>
static inline void SafeRelease(T*& p)
{
    if (p) { p->Release(); p = nullptr; }
}

injector::hook_back<void(__fastcall*)(void*, void*)> hbRenderMotionBlur;
void __fastcall RenderMotionBlur(void* camera, void* edx);

class PostFX
{
public:
    PostFX()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            bConsoleGamma = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 0) != 0;
            bSMAA = iniReader.ReadInteger("GRAPHICS", "SMAA", 0) != 0;

            if (bSMAA)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? EB ? E8");
                hbRenderMotionBlur.fun = injector::MakeCALL(pattern.get_first(), RenderMotionBlur).get();
            }

            if (bConsoleGamma)
            {
                WFP::onEndScene() += []()
                {
                    Render();
                };
            }

            if (bConsoleGamma || bSMAA)
            {
                WFP::onBeforeReset() += []()
                {
                    OnDeviceReset();
                };
            }
        };
    }

private:
    static inline bool bConsoleGamma = false;
    static inline bool bSMAA = false;

    static inline IDirect3DTexture9* pSceneTex = nullptr;
    static inline IDirect3DSurface9* pSceneSurf = nullptr;
    static inline IDirect3DVertexDeclaration9* pQuadDecl = nullptr;
    static inline ID3DXEffect* pEffect = nullptr;

    // SMAA
    static inline IDirect3DTexture9* pEdgeTex = nullptr;
    static inline IDirect3DTexture9* pBlendTex = nullptr;
    static inline IDirect3DSurface9* pEdgeSurf = nullptr;
    static inline IDirect3DSurface9* pBlendSurf = nullptr;
    static inline IDirect3DTexture9* pAreaTex = nullptr;
    static inline IDirect3DTexture9* pSearchTex = nullptr;

    static inline UINT nScreenWidth = 0;
    static inline UINT nScreenHeight = 0;
    static inline bool bInitialized = false;

    // ConsoleGamma
    static inline D3DXHANDLE hInputTex2D = nullptr;
    static inline D3DXHANDLE hGammaTechnique = nullptr;

    // SMAA
    static inline D3DXHANDLE hColorTex2D = nullptr;
    static inline D3DXHANDLE hEdgesTex2D = nullptr;
    static inline D3DXHANDLE hBlendTex2D = nullptr;
    static inline D3DXHANDLE hAreaTex2D = nullptr;
    static inline D3DXHANDLE hSearchTex2D = nullptr;
    static inline D3DXHANDLE hSMAARTMetrics = nullptr;
    static inline D3DXHANDLE hEdgeDetectionTechnique = nullptr;
    static inline D3DXHANDLE hBlendWeightTechnique = nullptr;
    static inline D3DXHANDLE hOutputTechnique = nullptr;

    static IDirect3DDevice9* GetDevice9()
    {
        if (!pD3D9Device || !*pD3D9Device) return nullptr;
        return *pD3D9Device;
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

        if (bSMAA)
        {
            if (FAILED(dev->CreateTexture(nScreenWidth, nScreenHeight, 1,
                D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pEdgeTex, nullptr)) ||
                FAILED(dev->CreateTexture(nScreenWidth, nScreenHeight, 1,
                    D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pBlendTex, nullptr)))
            {
                ReleaseResources(); return false;
            }

            pEdgeTex->GetSurfaceLevel(0, &pEdgeSurf);
            pBlendTex->GetSurfaceLevel(0, &pBlendSurf);
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
            OutputDebugStringA("PostFX: Failed to load shader\n");
            ReleaseResources();
            return false;
        }
        SafeRelease(errors);

        hInputTex2D = pEffect->GetParameterByName(nullptr, "InputTex2D");
        hGammaTechnique = pEffect->GetTechniqueByName("ConsoleGamma");

        if (bSMAA)
        {
            hColorTex2D = pEffect->GetParameterByName(nullptr, "colorTex2D");
            hEdgesTex2D = pEffect->GetParameterByName(nullptr, "edgesTex2D");
            hBlendTex2D = pEffect->GetParameterByName(nullptr, "blendTex2D");
            hAreaTex2D = pEffect->GetParameterByName(nullptr, "areaTex2D");
            hSearchTex2D = pEffect->GetParameterByName(nullptr, "searchTex2D");
            hSMAARTMetrics = pEffect->GetParameterByName(nullptr, "vec4SMAARTMetrics");

            hEdgeDetectionTechnique = pEffect->GetTechniqueByName("SMAAEdgeDetection");
            hBlendWeightTechnique = pEffect->GetTechniqueByName("SMAABlendWeightCalculation");
            hOutputTechnique = pEffect->GetTechniqueByName("SMAAOutputPass");

            D3DXIMAGE_INFO info{};

            HRESULT hrArea = D3DXGetImageInfoFromResource(hModule, MAKEINTRESOURCE(IDR_AREATEX), &info);
            if (SUCCEEDED(hrArea))
            {
                hrArea = D3DXCreateTextureFromResourceEx(dev, hModule, MAKEINTRESOURCE(IDR_AREATEX),
                    info.Width, info.Height, 1, 0, D3DFMT_A8L8, D3DPOOL_DEFAULT,
                    D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, &info, nullptr, &pAreaTex);
            }
            if (FAILED(hrArea))
            {
                OutputDebugStringA("PostFX: Failed to load area texture from resource.\n");
                ReleaseResources();
                return false;
            }

            HRESULT hrSearch = D3DXGetImageInfoFromResource(hModule, MAKEINTRESOURCE(IDR_SEARCHTEX), &info);
            if (SUCCEEDED(hrSearch))
            {
                hrSearch = D3DXCreateTextureFromResourceEx(dev, hModule, MAKEINTRESOURCE(IDR_SEARCHTEX),
                    info.Width, info.Height, 1, 0, D3DFMT_L8, D3DPOOL_DEFAULT,
                    D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, &info, nullptr, &pSearchTex);
            }
            if (FAILED(hrSearch))
            {
                OutputDebugStringA("PostFX: Failed to load search texture from resource.\n");
                ReleaseResources();
                return false;
            }
        }

        bInitialized = true;
        return true;
    }

    static void DrawGammaQuad(IDirect3DDevice9* dev)
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

    static void DrawAAQuad(IDirect3DDevice9* dev)
    {
        dev->SetVertexDeclaration(nullptr);
        dev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);

        float pixelSizeX = 1.0f / (float)nScreenWidth;
        float pixelSizeY = 1.0f / (float)nScreenHeight;
        float quad[4][5] =
        {
            { -1.0f - pixelSizeX,  1.0f + pixelSizeY, 0.5f, 0.0f, 0.0f },
            {  1.0f - pixelSizeX,  1.0f + pixelSizeY, 0.5f, 1.0f, 0.0f },
            { -1.0f - pixelSizeX, -1.0f + pixelSizeY, 0.5f, 0.0f, 1.0f },
            {  1.0f - pixelSizeX, -1.0f + pixelSizeY, 0.5f, 1.0f, 1.0f }
        };
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    }

    static void DrawAAPass(IDirect3DDevice9* dev, D3DXHANDLE technique)
    {
        pEffect->SetTechnique(technique);
        pEffect->CommitChanges();

        UINT passes = 0;
        if (SUCCEEDED(pEffect->Begin(&passes, 0)))
        {
            if (SUCCEEDED(pEffect->BeginPass(0)))
            {
                DrawAAQuad(dev);
                pEffect->EndPass();
            }
            pEffect->End();
        }
    }

    static void RenderSMAAPasses(IDirect3DDevice9* dev, IDirect3DSurface9* currentRT)
    {
        float metrics[] = { 1.0f / (float)nScreenWidth, 1.0f / (float)nScreenHeight, (float)nScreenWidth, (float)nScreenHeight };
        pEffect->SetFloatArray(hSMAARTMetrics, metrics, 4);

        // pass 1: edge detection
        dev->SetRenderTarget(0, pEdgeSurf);
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        pEffect->SetTexture(hColorTex2D, pSceneTex);
        DrawAAPass(dev, hEdgeDetectionTechnique);

        // pass 2: blending weights calculation
        dev->SetRenderTarget(0, pBlendSurf);
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        pEffect->SetTexture(hEdgesTex2D, pEdgeTex);
        pEffect->SetTexture(hAreaTex2D, pAreaTex);
        pEffect->SetTexture(hSearchTex2D, pSearchTex);
        DrawAAPass(dev, hBlendWeightTechnique);

        // pass 3: neighborhood blending (output)
        dev->SetRenderTarget(0, currentRT);
        pEffect->SetTexture(hColorTex2D, pSceneTex);
        pEffect->SetTexture(hBlendTex2D, pBlendTex);
        DrawAAPass(dev, hOutputTechnique);
    }

    static void RenderGamma(IDirect3DDevice9* dev)
    {
        pEffect->SetTexture(hInputTex2D, pSceneTex);
        pEffect->SetTechnique(hGammaTechnique);
        pEffect->CommitChanges();

        UINT passes = 0;
        if (SUCCEEDED(pEffect->Begin(&passes, 0)))
        {
            if (SUCCEEDED(pEffect->BeginPass(0)))
            {
                DrawGammaQuad(dev);
                pEffect->EndPass();
            }
            pEffect->End();
        }
    }

public:
    static void RenderSMAA()
    {
        if (!bSMAA) return;

        IDirect3DDevice9* dev = GetDevice9();
        if (!dev) return;

        if (!Initialize(dev)) return;

        IDirect3DSurface9* currentRT = nullptr;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        // Fails when the render target is multisampled (in-game MSAA), skip the frame.
        if (FAILED(dev->StretchRect(currentRT, nullptr, pSceneSurf, nullptr, D3DTEXF_POINT)))
        {
            currentRT->Release();
            return;
        }

        IDirect3DStateBlock9* state = nullptr;
        if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &state)) || !state)
        {
            currentRT->Release(); return;
        }

        RenderSMAAPasses(dev, currentRT);

        state->Apply();
        state->Release();
        currentRT->Release();
    }

    static void Render()
    {
        if (!bConsoleGamma) return;

        IDirect3DDevice9* dev = GetDevice9();
        if (!dev) return;

        if (!Initialize(dev)) return;

        IDirect3DSurface9* currentRT = nullptr;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        // Fails when the render target is multisampled (in-game MSAA), skip the frame.
        if (FAILED(dev->StretchRect(currentRT, nullptr, pSceneSurf, nullptr, D3DTEXF_POINT)))
        {
            currentRT->Release();
            return;
        }

        IDirect3DStateBlock9* state = nullptr;
        if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &state)) || !state)
        {
            currentRT->Release(); return;
        }

        RenderGamma(dev);

        state->Apply();
        state->Release();
        currentRT->Release();
    }

    static void OnDeviceReset()
    {
        ReleaseResources();
    }

    static void ReleaseResources()
    {
        SafeRelease(pEffect);
        SafeRelease(pQuadDecl);
        SafeRelease(pSceneSurf);
        SafeRelease(pSceneTex);
        SafeRelease(pEdgeSurf);
        SafeRelease(pEdgeTex);
        SafeRelease(pBlendSurf);
        SafeRelease(pBlendTex);
        SafeRelease(pAreaTex);
        SafeRelease(pSearchTex);
        nScreenWidth = 0;
        nScreenHeight = 0;
        bInitialized = false;
    }
} PostFX;

void __fastcall RenderMotionBlur(void* camera, void* edx)
{
    PostFX::RenderSMAA();
    hbRenderMotionBlur.fun(camera, edx);
}
