module;

#include <stdafx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <wrl/client.h>
#pragma comment(lib, "d3dx9.lib")

export module PostFXCore;

using Microsoft::WRL::ComPtr;

export class CPostFX
{
public:
    static inline bool bConsoleGammaEnabled = false;
    static inline bool bSmaaEnabled = false;
    static inline bool bBlurEnabled = false;
    static inline float fBlurStrength = 5.0f;
    static inline bool bDisableSMAAWhenMSAA = true;
    static inline bool bRenderToBackBuffer = false;
    static inline bool bUseMWGammaTechnique = false;

private:
    struct BackBufferInfo
    {
        D3DFORMAT format = D3DFMT_UNKNOWN;
        D3DMULTISAMPLE_TYPE multiSampleType = D3DMULTISAMPLE_NONE;
        DWORD multiSampleQuality = 0;
        UINT width = 0;
        UINT height = 0;
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

    // Blur
    static inline ComPtr<IDirect3DTexture9> pBlurIntermediateTex;
    static inline ComPtr<IDirect3DSurface9> pBlurIntermediateSurf;

    static inline bool bCreatedTextures = false;

    // ConsoleGamma
    static inline D3DXHANDLE hInputTex2D = nullptr;
    static inline D3DXHANDLE hGammaTechnique = nullptr;
    static inline D3DXHANDLE hMWGammaTechnique = nullptr;

    // Blur
    static inline D3DXHANDLE hBlurHorizontalTechnique = nullptr;
    static inline D3DXHANDLE hBlurVerticalTechnique = nullptr;
    static inline D3DXHANDLE hTexelSize = nullptr;
    static inline D3DXHANDLE hBlurStrengthParam = nullptr;

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

    static BackBufferInfo GetBackBufferInfo(IDirect3DDevice9* dev)
    {
        BackBufferInfo info{};
        if (!dev)
            return info;

        if (bRenderToBackBuffer)
        {
            // NFS/SCDA: base format and size on the swapchain backbuffer
            ComPtr<IDirect3DSurface9> bb = GetBackBuffer(dev);
            if (bb)
            {
                D3DSURFACE_DESC desc{};
                if (SUCCEEDED(bb->GetDesc(&desc)))
                {
                    info.format = desc.Format;
                    info.multiSampleType = desc.MultiSampleType;
                    info.multiSampleQuality = desc.MultiSampleQuality;
                    info.width = desc.Width;
                    info.height = desc.Height;
                    return info;
                }
            }
        }

        ComPtr<IDirect3DSurface9> rt0;
        if (SUCCEEDED(dev->GetRenderTarget(0, &rt0)) && rt0)
        {
            D3DSURFACE_DESC desc{};
            if (SUCCEEDED(rt0->GetDesc(&desc)))
            {
                info.format = desc.Format;
                info.multiSampleType = desc.MultiSampleType;
                info.multiSampleQuality = desc.MultiSampleQuality;
                info.width = desc.Width;
                info.height = desc.Height;
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
                info.width = pp.BackBufferWidth;
                info.height = pp.BackBufferHeight;
            }
        }

        return info;
    }

    static ComPtr<IDirect3DSurface9> GetBackBuffer(IDirect3DDevice9* dev)
    {
        ComPtr<IDirect3DSurface9> bb;
        if (dev)
            dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
        return bb;
    }

    static bool InitShaderAndStaticResources(IDirect3DDevice9* dev)
    {
        if (pEffect)
            return true;

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
            return false;
        }

        hInputTex2D = pEffect->GetParameterByName(nullptr, "InputTex2D");
        hGammaTechnique = pEffect->GetTechniqueByName("ConsoleGamma");
        hMWGammaTechnique = pEffect->GetTechniqueByName("MWGamma");

        hBlurHorizontalTechnique = pEffect->GetTechniqueByName("BlurHorizontal");
        hBlurVerticalTechnique = pEffect->GetTechniqueByName("BlurVertical");
        hTexelSize = pEffect->GetParameterByName(nullptr, "TexelSize");
        hBlurStrengthParam = pEffect->GetParameterByName(nullptr, "BlurStrength");

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

        return true;
    }

    static bool CreateTextures(IDirect3DDevice9* dev)
    {
        if (bCreatedTextures) return true;

        if (bBackBufferInfoDirty || backBufferInfo.format == D3DFMT_UNKNOWN)
        {
            backBufferInfo = GetBackBufferInfo(dev);
            bBackBufferInfoDirty = false;
        }
        if (backBufferInfo.format == D3DFMT_UNKNOWN)
            return false;

        if (FAILED(dev->CreateTexture(backBufferInfo.width, backBufferInfo.height, 1,
            D3DUSAGE_RENDERTARGET, backBufferInfo.format, D3DPOOL_DEFAULT, &pSceneTex, nullptr)))
            return false;

        if (FAILED(pSceneTex->GetSurfaceLevel(0, &pSceneSurf)))
        {
            ReleaseTextures(); return false;
        }

        if (backBufferInfo.multiSampleType != D3DMULTISAMPLE_NONE)
        {
            if (FAILED(dev->CreateRenderTarget(backBufferInfo.width, backBufferInfo.height, backBufferInfo.format,
                D3DMULTISAMPLE_NONE, 0, FALSE, &pResolveSurf, nullptr)))
            {
                ReleaseTextures(); return false;
            }
        }

        if (bSmaaEnabled)
        {
            if (FAILED(dev->CreateTexture(backBufferInfo.width, backBufferInfo.height, 1,
                D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pEdgeTex, nullptr)) ||
                FAILED(dev->CreateTexture(backBufferInfo.width, backBufferInfo.height, 1,
                    D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pBlendTex, nullptr)))
            {
                ReleaseTextures(); return false;
            }

            pEdgeTex->GetSurfaceLevel(0, &pEdgeSurf);
            pBlendTex->GetSurfaceLevel(0, &pBlendSurf);
        }

        if (bBlurEnabled)
        {
            if (FAILED(dev->CreateTexture(backBufferInfo.width, backBufferInfo.height, 1,
                D3DUSAGE_RENDERTARGET, backBufferInfo.format, D3DPOOL_DEFAULT, &pBlurIntermediateTex, nullptr)))
            {
                ReleaseTextures(); return false;
            }

            pBlurIntermediateTex->GetSurfaceLevel(0, &pBlurIntermediateSurf);
        }

        bCreatedTextures = true;
        return true;
    }

    static void ReleaseTextures()
    {
        pSceneSurf.Reset();
        pSceneTex.Reset();
        pResolveSurf.Reset();
        pEdgeSurf.Reset();
        pEdgeTex.Reset();
        pBlendSurf.Reset();
        pBlendTex.Reset();
        pBlurIntermediateSurf.Reset();
        pBlurIntermediateTex.Reset();
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
        float pixelSizeX = 1.0f / (float)backBufferInfo.width;
        float pixelSizeY = 1.0f / (float)backBufferInfo.height;
        float quad[4][5] =
        {
            { -1.0f - pixelSizeX,  1.0f + pixelSizeY, 0.5f, 0.0f, 0.0f },
            {  1.0f - pixelSizeX,  1.0f + pixelSizeY, 0.5f, 1.0f, 0.0f },
            { -1.0f - pixelSizeX, -1.0f + pixelSizeY, 0.5f, 0.0f, 1.0f },
            {  1.0f - pixelSizeX, -1.0f + pixelSizeY, 0.5f, 1.0f, 1.0f }
        };
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    }

    static void DrawScreenQuad(IDirect3DDevice9* dev)
    {
        struct ScreenVertex { float x, y, z, rhw, u, v; };
        ScreenVertex v[4] =
        {
            {-0.5f,                       -0.5f,                       0.0f, 1.0f, 0.0f, 0.0f},
            {-0.5f,                       float(backBufferInfo.height) - 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
            {float(backBufferInfo.width) - 0.5f, -0.5f,                0.0f, 1.0f, 1.0f, 0.0f},
            {float(backBufferInfo.width) - 0.5f, float(backBufferInfo.height) - 0.5f, 0.0f, 1.0f, 1.0f, 1.0f}
        };
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(ScreenVertex));
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

    static void SaveVertexState(IDirect3DDevice9* dev,
                                ComPtr<IDirect3DVertexBuffer9>& oldVB,
                                ComPtr<IDirect3DVertexDeclaration9>& oldDecl,
                                UINT& oldOffset, UINT& oldStride, DWORD& oldFVF)
    {
        dev->GetStreamSource(0, &oldVB, &oldOffset, &oldStride);
        dev->GetVertexDeclaration(&oldDecl);
        dev->GetFVF(&oldFVF);

        dev->SetStreamSource(0, nullptr, 0, 0);
        dev->SetVertexDeclaration(nullptr);
    }

    static void RestoreVertexState(IDirect3DDevice9* dev,
                                   IDirect3DVertexBuffer9* oldVB,
                                   IDirect3DVertexDeclaration9* oldDecl,
                                   UINT oldOffset, UINT oldStride, DWORD oldFVF)
    {
        dev->SetStreamSource(0, oldVB, oldOffset, oldStride);
        dev->SetVertexDeclaration(oldDecl);
        dev->SetFVF(oldFVF);
    }

    static IDirect3DSurface9* GetOutputSurface(IDirect3DDevice9* dev,
                                               IDirect3DSurface9* currentRT,
                                               ComPtr<IDirect3DSurface9>& bb)
    {
        if (!bRenderToBackBuffer)
            return currentRT;

        bb = GetBackBuffer(dev);
        if (!bb)
            return nullptr;

        return bb.Get();
    }

public:
    static void RenderSMAA(IDirect3DDevice9* dev)
    {
        if (!bSmaaEnabled || !dev) return;
        if (!InitShaderAndStaticResources(dev)) return;

        ComPtr<IDirect3DSurface9> currentRT;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        if (!CreateTextures(dev)) return;
        if (bDisableSMAAWhenMSAA && backBufferInfo.multiSampleType != D3DMULTISAMPLE_NONE) return;
        if (!UpdateSceneTex(dev, currentRT.Get())) return;

        ComPtr<IDirect3DSurface9> bb;
        IDirect3DSurface9* pOutput = GetOutputSurface(dev, currentRT.Get(), bb);
        if (!pOutput) return;

        ComPtr<IDirect3DVertexBuffer9> oldVB;
        ComPtr<IDirect3DVertexDeclaration9> oldDecl;

        UINT oldOffset = 0, oldStride = 0;
        DWORD oldFVF = 0;

        SaveVertexState(dev, oldVB, oldDecl, oldOffset, oldStride, oldFVF);
        dev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);

        float metrics[] = { 1.0f / (float)backBufferInfo.width, 1.0f / (float)backBufferInfo.height, (float)backBufferInfo.width, (float)backBufferInfo.height };
        pEffect->SetFloatArray(hSMAARTMetrics, metrics, 4);

        dev->SetRenderTarget(0, pEdgeSurf.Get());
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        pEffect->SetTexture(hColorTex2D, pSceneTex.Get());
        DrawAAPass(dev, hEdgeDetectionTechnique);

        dev->SetRenderTarget(0, pBlendSurf.Get());
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        pEffect->SetTexture(hEdgesTex2D, pEdgeTex.Get());
        pEffect->SetTexture(hAreaTex2D, pAreaTex.Get());
        pEffect->SetTexture(hSearchTex2D, pSearchTex.Get());
        DrawAAPass(dev, hBlendWeightTechnique);

        dev->SetRenderTarget(0, pOutput);
        pEffect->SetTexture(hColorTex2D, pSceneTex.Get());
        pEffect->SetTexture(hBlendTex2D, pBlendTex.Get());
        DrawAAPass(dev, hOutputTechnique);

        if (pOutput != currentRT.Get())
            dev->SetRenderTarget(0, currentRT.Get());

        RestoreVertexState(dev, oldVB.Get(), oldDecl.Get(), oldOffset, oldStride, oldFVF);
    }

    static void RenderGamma(IDirect3DDevice9* dev)
    {
        if (!bConsoleGammaEnabled || !dev) return;
        if (!InitShaderAndStaticResources(dev)) return;

        ComPtr<IDirect3DSurface9> currentRT;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        if (!CreateTextures(dev)) return;
        if (!UpdateSceneTex(dev, currentRT.Get())) return;

        ComPtr<IDirect3DSurface9> bb;
        IDirect3DSurface9* pOutput = GetOutputSurface(dev, currentRT.Get(), bb);
        if (!pOutput) return;

        ComPtr<IDirect3DVertexBuffer9> oldVB;
        ComPtr<IDirect3DVertexDeclaration9> oldDecl;

        UINT oldOffset = 0, oldStride = 0;
        DWORD oldFVF = 0;

        SaveVertexState(dev, oldVB, oldDecl, oldOffset, oldStride, oldFVF);
        dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

        if (pOutput != currentRT.Get())
            dev->SetRenderTarget(0, pOutput);

        pEffect->SetTexture(hInputTex2D, pSceneTex.Get());
        pEffect->SetTechnique(bUseMWGammaTechnique && hMWGammaTechnique ? hMWGammaTechnique : hGammaTechnique);
        pEffect->CommitChanges();

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

        if (pOutput != currentRT.Get())
            dev->SetRenderTarget(0, currentRT.Get());

        RestoreVertexState(dev, oldVB.Get(), oldDecl.Get(), oldOffset, oldStride, oldFVF);
    }

    static void RenderBlur(IDirect3DDevice9* dev)
    {
        if (!bBlurEnabled || fBlurStrength <= 0.001f || !dev) return;
        if (!InitShaderAndStaticResources(dev)) return;

        ComPtr<IDirect3DSurface9> currentRT;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        if (!CreateTextures(dev)) return;
        if (!UpdateSceneTex(dev, currentRT.Get())) return;

        ComPtr<IDirect3DSurface9> bb;
        IDirect3DSurface9* pOutput = GetOutputSurface(dev, currentRT.Get(), bb);
        if (!pOutput) return;

        ComPtr<IDirect3DVertexBuffer9> oldVB;
        ComPtr<IDirect3DVertexDeclaration9> oldDecl;

        UINT oldOffset = 0, oldStride = 0;
        DWORD oldFVF = 0;

        SaveVertexState(dev, oldVB, oldDecl, oldOffset, oldStride, oldFVF);
        dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

        float texelSize[2] = { 1.0f / (float)backBufferInfo.width, 1.0f / (float)backBufferInfo.height };

        UINT passes = 0;

        dev->SetRenderTarget(0, pBlurIntermediateSurf.Get());

        pEffect->SetTexture(hInputTex2D, pSceneTex.Get());
        pEffect->SetFloatArray(hTexelSize, texelSize, 2);
        pEffect->SetFloat(hBlurStrengthParam, fBlurStrength);
        pEffect->SetTechnique(hBlurHorizontalTechnique);
        pEffect->CommitChanges();

        pEffect->Begin(&passes, 0);
        pEffect->BeginPass(0);
        DrawScreenQuad(dev);
        pEffect->EndPass();
        pEffect->End();

        dev->SetRenderTarget(0, pOutput);

        pEffect->SetTexture(hInputTex2D, pBlurIntermediateTex.Get());
        pEffect->SetTechnique(hBlurVerticalTechnique);
        pEffect->CommitChanges();

        pEffect->Begin(&passes, 0);
        pEffect->BeginPass(0);
        DrawScreenQuad(dev);
        pEffect->EndPass();
        pEffect->End();

        if (pOutput != currentRT.Get())
            dev->SetRenderTarget(0, currentRT.Get());

        RestoreVertexState(dev, oldVB.Get(), oldDecl.Get(), oldOffset, oldStride, oldFVF);
    }

    static void Shutdown()
    {
        if (pEffect)
            pEffect->OnLostDevice();
        ReleaseTextures();
    }

    static void OnDeviceReset()
    {
        bBackBufferInfoDirty = true;

        if (!pEffect)
            return;
        pEffect->OnResetDevice();
    }
};
