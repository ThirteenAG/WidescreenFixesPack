module;

#include <stdafx.h>
#include "common.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <wrl/client.h>
#pragma comment(lib, "d3dx9.lib")

export module PostFX;

import Skeleton;

using Microsoft::WRL::ComPtr;

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
                auto pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E9 ? ? ? ? 8D 44 20");
                hbRenderMotionBlur.fun = injector::MakeCALL(pattern.get_first(), RenderMotionBlur).get();
            }

            if (bConsoleGamma)
            {
                auto pattern = hook::pattern("A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 59 C3");
                static auto ConsoleGammaHook = safetyhook::create_mid(pattern.get_first(), +[](SafetyHookContext& regs)
                {
                    RenderConsoleGamma();
                });
            }

            if (bConsoleGamma || bSMAA)
            {
                WFP::onBeforeReset() += []()
                {
                    OnDeviceLost();
                    OnDeviceReset();
                };
            }

        };
    }

private:
    static inline bool bConsoleGamma = false;
    static inline bool bSMAA = false;
    static inline bool bDisableSMAAWhenMSAA = true;

    struct BackBufferInfo
    {
        D3DFORMAT format = D3DFMT_UNKNOWN;
        D3DMULTISAMPLE_TYPE multiSampleType = D3DMULTISAMPLE_NONE;
        DWORD multiSampleQuality = 0;
    };

    static inline BackBufferInfo backBufferInfo{};
    static inline bool bBackBufferInfoDirty = true;

    static inline ComPtr<IDirect3DTexture9> pSceneTex;
    static inline ComPtr<IDirect3DSurface9> pSceneSurf;
    static inline ComPtr<IDirect3DSurface9> pResolveSurf;
    static inline ComPtr<ID3DXEffect> pEffect;

    // SMAA
    static inline ComPtr<IDirect3DTexture9> pEdgeTex;
    static inline ComPtr<IDirect3DTexture9> pBlendTex;
    static inline ComPtr<IDirect3DSurface9> pEdgeSurf;
    static inline ComPtr<IDirect3DSurface9> pBlendSurf;
    static inline ComPtr<IDirect3DTexture9> pAreaTex;
    static inline ComPtr<IDirect3DTexture9> pSearchTex;

    static inline UINT nScreenWidth = 0;
    static inline UINT nScreenHeight = 0;
    static inline bool bCreatedTextures = false;

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

    static ComPtr<IDirect3DDevice9> GetDevice9()
    {
        if (!pD3D8Device || !*pD3D8Device) return nullptr;

        ComPtr<IDirect3DDevice9> dev9;
        if (FAILED((*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)dev9.GetAddressOf())))
            return nullptr;

        return dev9;
    }

    static BackBufferInfo GetBackBufferInfo(IDirect3DDevice9* dev)
    {
        BackBufferInfo info{};
        if (!dev)
            return info;

        ComPtr<IDirect3DSurface9> rt0;
        if (SUCCEEDED(dev->GetRenderTarget(0, &rt0)) && rt0)
        {
            D3DSURFACE_DESC desc{};
            if (SUCCEEDED(rt0->GetDesc(&desc)))
            {
                info.format = desc.Format;
                info.multiSampleType = desc.MultiSampleType;
                info.multiSampleQuality = desc.MultiSampleQuality;
                return info;
            }
        }

        ComPtr<IDirect3DSwapChain9> swap;
        if (SUCCEEDED(dev->GetSwapChain(0, &swap)) && swap)
        {
            D3DPRESENT_PARAMETERS pp{};
            if (SUCCEEDED(swap->GetPresentParameters(&pp)))
            {
                info.format = pp.BackBufferFormat;
                info.multiSampleType = pp.MultiSampleType;
                info.multiSampleQuality = pp.MultiSampleQuality;
            }
        }

        return info;
    }

    static void InitShaderAndStaticResources(IDirect3DDevice9* dev)
    {
        if (!dev)
            return;

        HMODULE hModule = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&CreateTextures, &hModule);

        ComPtr<ID3DXBuffer> errors;
        HRESULT hr = D3DXCreateEffectFromResource(dev, hModule, MAKEINTRESOURCE(IDR_POSTFX),
            nullptr, nullptr, 0, nullptr, &pEffect, &errors);

        if (FAILED(hr) || !pEffect)
        {
            if (errors) OutputDebugStringA((const char*)errors->GetBufferPointer());
            OutputDebugStringA("PostFX: Failed to load shader\n");
            return;
        }

        hInputTex2D = pEffect->GetParameterByName(nullptr, "InputTex2D");
        hGammaTechnique = pEffect->GetTechniqueByName("ConsoleGamma");

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
                info.Width, info.Height, 1, 0, D3DFMT_A8L8, D3DPOOL_MANAGED,
                D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, &info, nullptr, &pAreaTex);
        }
        if (FAILED(hrArea))
        {
            OutputDebugStringA("PostFX: Failed to load area texture from resource.\n");
        }

        HRESULT hrSearch = D3DXGetImageInfoFromResource(hModule, MAKEINTRESOURCE(IDR_SEARCHTEX), &info);
        if (SUCCEEDED(hrSearch))
        {
            hrSearch = D3DXCreateTextureFromResourceEx(dev, hModule, MAKEINTRESOURCE(IDR_SEARCHTEX),
                info.Width, info.Height, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED,
                D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, &info, nullptr, &pSearchTex);
        }
        if (FAILED(hrSearch))
        {
            OutputDebugStringA("PostFX: Failed to load search texture from resource.\n");
        }
    }

    static bool EnsureInitialized(IDirect3DDevice9* dev)
    {
        if (pEffect)
            return true;

        InitShaderAndStaticResources(dev);
        return pEffect != nullptr;
    }

    static bool CreateTextures(IDirect3DDevice9* dev)
    {
        if (bCreatedTextures) return true;

        nScreenWidth = RsGlobal->width;
        nScreenHeight = RsGlobal->height;

        if (bBackBufferInfoDirty || backBufferInfo.format == D3DFMT_UNKNOWN)
        {
            backBufferInfo = GetBackBufferInfo(dev);
            bBackBufferInfoDirty = false;
        }
        if (backBufferInfo.format == D3DFMT_UNKNOWN)
            return false;

        if (FAILED(dev->CreateTexture(nScreenWidth, nScreenHeight, 1,
            D3DUSAGE_RENDERTARGET, backBufferInfo.format, D3DPOOL_DEFAULT, &pSceneTex, nullptr)))
            return false;

        if (FAILED(pSceneTex->GetSurfaceLevel(0, &pSceneSurf)))
        {
            ReleaseTextures(); return false;
        }

        if (backBufferInfo.multiSampleType != D3DMULTISAMPLE_NONE)
        {
            if (FAILED(dev->CreateRenderTarget(nScreenWidth, nScreenHeight, backBufferInfo.format,
                D3DMULTISAMPLE_NONE, 0, FALSE, &pResolveSurf, nullptr)))
            {
                ReleaseTextures(); return false;
            }
        }

        if (bSMAA)
        {
            if (FAILED(dev->CreateTexture(nScreenWidth, nScreenHeight, 1,
                D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pEdgeTex, nullptr)) ||
                FAILED(dev->CreateTexture(nScreenWidth, nScreenHeight, 1,
                D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pBlendTex, nullptr)))
            {
                ReleaseTextures(); return false;
            }

            pEdgeTex->GetSurfaceLevel(0, &pEdgeSurf);
            pBlendTex->GetSurfaceLevel(0, &pBlendSurf);
        }

        bCreatedTextures = true;
        return true;
    }

    static void ReleaseTextures()
    {
        pSceneSurf.Reset();
        pSceneTex.Reset();
        pEdgeSurf.Reset();
        pEdgeTex.Reset();
        pBlendSurf.Reset();
        pBlendTex.Reset();
        pResolveSurf.Reset();
        nScreenWidth = 0;
        nScreenHeight = 0;
        bCreatedTextures = false;
    }

    static bool UpdateSceneTex(IDirect3DDevice9* dev, IDirect3DSurface9* currentRT)
    {
        IDirect3DSurface9* pSrcSurf = currentRT;

        if (backBufferInfo.multiSampleType != D3DMULTISAMPLE_NONE)
        {
            if (FAILED(dev->StretchRect(currentRT, nullptr, pResolveSurf.Get(), nullptr, D3DTEXF_LINEAR)))
            {
                return false;
            }
            pSrcSurf = pResolveSurf.Get();
        }

        if (FAILED(dev->StretchRect(pSrcSurf, nullptr, pSceneSurf.Get(), nullptr, D3DTEXF_POINT)))
        {
            return false;
        }

        return true;
    }

    static void DrawAAQuad(IDirect3DDevice9* dev)
    {
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

public:
    static void RenderSMAA()
    {
        if (!bSMAA) return;

        ComPtr<IDirect3DDevice9> dev = GetDevice9();
        if (!dev) return;
        if (!EnsureInitialized(dev.Get())) return;

        ComPtr<IDirect3DSurface9> currentRT;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        if (!CreateTextures(dev.Get())) return;
        if (bDisableSMAAWhenMSAA && backBufferInfo.multiSampleType != D3DMULTISAMPLE_NONE) return;
        if (!UpdateSceneTex(dev.Get(), currentRT.Get())) return;

        ComPtr<IDirect3DVertexBuffer9> oldVB;
        ComPtr<IDirect3DVertexDeclaration9> oldDecl;

        UINT oldOffset, oldStride;
        DWORD oldFVF;

        dev->GetStreamSource(0, &oldVB, &oldOffset, &oldStride);
        dev->GetVertexDeclaration(&oldDecl);
        dev->GetFVF(&oldFVF);

        dev->SetStreamSource(0, nullptr, 0, 0);
        dev->SetVertexDeclaration(nullptr);
        dev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);

        float metrics[] = { 1.0f / (float)nScreenWidth, 1.0f / (float)nScreenHeight, (float)nScreenWidth, (float)nScreenHeight };
        pEffect->SetFloatArray(hSMAARTMetrics, metrics, 4);

        dev->SetRenderTarget(0, pEdgeSurf.Get());
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        pEffect->SetTexture(hColorTex2D, pSceneTex.Get());
        DrawAAPass(dev.Get(), hEdgeDetectionTechnique);

        dev->SetRenderTarget(0, pBlendSurf.Get());
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        pEffect->SetTexture(hEdgesTex2D, pEdgeTex.Get());
        pEffect->SetTexture(hAreaTex2D, pAreaTex.Get());
        pEffect->SetTexture(hSearchTex2D, pSearchTex.Get());
        DrawAAPass(dev.Get(), hBlendWeightTechnique);

        dev->SetRenderTarget(0, currentRT.Get());
        pEffect->SetTexture(hColorTex2D, pSceneTex.Get());
        pEffect->SetTexture(hBlendTex2D, pBlendTex.Get());
        DrawAAPass(dev.Get(), hOutputTechnique);

        dev->SetStreamSource(0, oldVB.Get(), oldOffset, oldStride);
        dev->SetVertexDeclaration(oldDecl.Get());
        dev->SetFVF(oldFVF);
    }

    static void RenderConsoleGamma()
    {
        if (!bConsoleGamma) return;

        ComPtr<IDirect3DDevice9> dev = GetDevice9();
        if (!dev) return;
        if (!EnsureInitialized(dev.Get())) return;

        ComPtr<IDirect3DSurface9> currentRT;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        if (!CreateTextures(dev.Get()) || !UpdateSceneTex(dev.Get(), currentRT.Get())) return;

        ComPtr<IDirect3DVertexBuffer9> oldVB;
        ComPtr<IDirect3DVertexDeclaration9> oldDecl;

        UINT oldOffset, oldStride;
        DWORD oldFVF;

        dev->GetStreamSource(0, &oldVB, &oldOffset, &oldStride);
        dev->GetVertexDeclaration(&oldDecl);
        dev->GetFVF(&oldFVF);

        dev->SetStreamSource(0, nullptr, 0, 0);
        dev->SetVertexDeclaration(nullptr);
        dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

        pEffect->SetTexture(hInputTex2D, pSceneTex.Get());
        pEffect->SetTechnique(hGammaTechnique);
        pEffect->CommitChanges();

        UINT passes = 0;
        if (SUCCEEDED(pEffect->Begin(&passes, 0)))
        {
            if (SUCCEEDED(pEffect->BeginPass(0)))
            {
                struct ScreenVertex { float x, y, z, rhw, u, v; };
                ScreenVertex v[4] =
                {
                    {-0.5f,                      -0.5f,                      0.0f, 1.0f, 0.0f, 0.0f},
                    {-0.5f,                      float(nScreenHeight) - 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
                    {float(nScreenWidth) - 0.5f, -0.5f,                      0.0f, 1.0f, 1.0f, 0.0f},
                    {float(nScreenWidth) - 0.5f, float(nScreenHeight) - 0.5f, 0.0f, 1.0f, 1.0f, 1.0f}
                };

                dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(ScreenVertex));

                pEffect->EndPass();
            }
            pEffect->End();
        }

        dev->SetStreamSource(0, oldVB.Get(), oldOffset, oldStride);
        dev->SetVertexDeclaration(oldDecl.Get());
        dev->SetFVF(oldFVF);
    }

    static void OnDeviceReset()
    {
        bBackBufferInfoDirty = true;

        if (!pEffect)
            return;
        pEffect->OnResetDevice();
    }

    static void OnDeviceLost()
    {
        if (!pEffect)
            return;
        pEffect->OnLostDevice();
        ReleaseTextures();
    }
} PostFX;

void __fastcall RenderMotionBlur(void* camera, void* edx)
{
    PostFX::RenderSMAA();
    hbRenderMotionBlur.fun(camera, edx);
}