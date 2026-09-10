module;

#include <stdafx.h>
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")

export module PostFXCore;

export class CPostFX
{
public:
    static inline bool bConsoleGammaEnabled = false;
    static inline bool bSmaaEnabled = false;
    static inline bool bBlurEnabled = false;
    static inline float fBlurStrength = 5.0f;
    static inline bool bDisableSMAAWhenMSAA = true;
    static inline bool bRenderToBackBuffer = false;

private:
    template <typename T>
    static void SafeRelease(T*& p)
    {
        if (p)
        {
            p->Release();
            p = nullptr;
        }
    }

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

    static inline IDirect3DTexture9* pSceneTex = nullptr;
    static inline IDirect3DSurface9* pSceneSurf = nullptr;
    static inline IDirect3DSurface9* pResolveSurf = nullptr;
    static inline ID3DXEffect* pEffect = nullptr;

    // SMAA
    static inline IDirect3DTexture9* pEdgeTex = nullptr;
    static inline IDirect3DTexture9* pBlendTex = nullptr;
    static inline IDirect3DSurface9* pEdgeSurf = nullptr;
    static inline IDirect3DSurface9* pBlendSurf = nullptr;
    static inline IDirect3DTexture9* pAreaTex = nullptr;
    static inline IDirect3DTexture9* pSearchTex = nullptr;

    // Blur
    static inline IDirect3DTexture9* pBlurIntermediateTex = nullptr;
    static inline IDirect3DSurface9* pBlurIntermediateSurf = nullptr;

    static inline bool bCreatedTextures = false;

    // ConsoleGamma
    static inline D3DXHANDLE hInputTex2D = nullptr;
    static inline D3DXHANDLE hGammaTechnique = nullptr;

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
            IDirect3DSurface9* bb = GetBackBuffer(dev);
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
                    bb->Release();
                    return info;
                }
                bb->Release();
            }
        }

        IDirect3DSurface9* rt0 = nullptr;
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
                rt0->Release();
                return info;
            }
            rt0->Release();
        }

        IDirect3DSwapChain9* swap = nullptr;
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
            swap->Release();
        }

        return info;
    }

    static IDirect3DSurface9* GetBackBuffer(IDirect3DDevice9* dev)
    {
        IDirect3DSurface9* bb = nullptr;
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

        ID3DXBuffer* errors = nullptr;
        HRESULT hr = D3DXCreateEffectFromResource(dev, hModule, MAKEINTRESOURCE(IDR_POSTFX),
            nullptr, nullptr, 0, nullptr, &pEffect, &errors);

        if (FAILED(hr) || !pEffect)
        {
            if (errors)
            {
                OutputDebugStringA((const char*)errors->GetBufferPointer());
                errors->Release();
            }
            OutputDebugStringA("PostFX: Failed to load shader\n");
            return false;
        }

        hInputTex2D = pEffect->GetParameterByName(nullptr, "InputTex2D");
        hGammaTechnique = pEffect->GetTechniqueByName("ConsoleGamma");

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

        if (!pAreaTex)
        {
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
        }

        if (!pSearchTex)
        {
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
        SafeRelease(pSceneSurf);
        SafeRelease(pSceneTex);
        SafeRelease(pResolveSurf);
        SafeRelease(pEdgeSurf);
        SafeRelease(pEdgeTex);
        SafeRelease(pBlendSurf);
        SafeRelease(pBlendTex);
        SafeRelease(pBlurIntermediateSurf);
        SafeRelease(pBlurIntermediateTex);
        bCreatedTextures = false;
    }

    static bool UpdateSceneTex(IDirect3DDevice9* dev, IDirect3DSurface9* currentRT)
    {
        IDirect3DSurface9* pSrcSurf = currentRT;

        if (backBufferInfo.multiSampleType != D3DMULTISAMPLE_NONE)
        {
            if (FAILED(dev->StretchRect(currentRT, nullptr, pResolveSurf, nullptr, D3DTEXF_LINEAR)))
            {
                return false;
            }
            pSrcSurf = pResolveSurf;
        }

        if (FAILED(dev->StretchRect(pSrcSurf, nullptr, pSceneSurf, nullptr, D3DTEXF_POINT)))
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
                                IDirect3DVertexBuffer9*& oldVB,
                                IDirect3DVertexDeclaration9*& oldDecl,
                                UINT& oldOffset, UINT& oldStride, DWORD& oldFVF)
    {
        oldVB = nullptr;
        oldDecl = nullptr;
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
                                               IDirect3DSurface9** ppBB)
    {
        if (!bRenderToBackBuffer)
            return currentRT;

        *ppBB = GetBackBuffer(dev);
        return *ppBB;
    }

public:
    static void RenderSMAA(IDirect3DDevice9* dev)
    {
        if (!bSmaaEnabled || !dev) return;
        if (!InitShaderAndStaticResources(dev)) return;
        if (!CreateTextures(dev)) return;
        if (bDisableSMAAWhenMSAA && backBufferInfo.multiSampleType != D3DMULTISAMPLE_NONE) return;

        IDirect3DSurface9* currentRT = nullptr;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        if (!UpdateSceneTex(dev, currentRT))
        {
            currentRT->Release();
            return;
        }

        IDirect3DSurface9* pBB = nullptr;
        IDirect3DSurface9* pOutput = GetOutputSurface(dev, currentRT, &pBB);
        if (!pOutput)
        {
            SafeRelease(pBB);
            currentRT->Release();
            return;
        }

        IDirect3DVertexBuffer9* oldVB = nullptr;
        IDirect3DVertexDeclaration9* oldDecl = nullptr;
        UINT oldOffset = 0, oldStride = 0;
        DWORD oldFVF = 0;

        SaveVertexState(dev, oldVB, oldDecl, oldOffset, oldStride, oldFVF);
        dev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);

        float metrics[] = { 1.0f / (float)backBufferInfo.width, 1.0f / (float)backBufferInfo.height, (float)backBufferInfo.width, (float)backBufferInfo.height };
        pEffect->SetFloatArray(hSMAARTMetrics, metrics, 4);

        dev->SetRenderTarget(0, pEdgeSurf);
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        pEffect->SetTexture(hColorTex2D, pSceneTex);
        DrawAAPass(dev, hEdgeDetectionTechnique);

        dev->SetRenderTarget(0, pBlendSurf);
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        pEffect->SetTexture(hEdgesTex2D, pEdgeTex);
        pEffect->SetTexture(hAreaTex2D, pAreaTex);
        pEffect->SetTexture(hSearchTex2D, pSearchTex);
        DrawAAPass(dev, hBlendWeightTechnique);

        dev->SetRenderTarget(0, pOutput);
        pEffect->SetTexture(hColorTex2D, pSceneTex);
        pEffect->SetTexture(hBlendTex2D, pBlendTex);
        DrawAAPass(dev, hOutputTechnique);

        if (pOutput != currentRT)
            dev->SetRenderTarget(0, currentRT);

        RestoreVertexState(dev, oldVB, oldDecl, oldOffset, oldStride, oldFVF);

        SafeRelease(oldVB);
        SafeRelease(oldDecl);
        SafeRelease(pBB);
        currentRT->Release();
    }

    // szTechniqueOverride: optional alternative gamma technique name, resolved
    // against the loaded effect (used by NFS Most Wanted for its own X360 preset).
    static void RenderGamma(IDirect3DDevice9* dev, const char* szTechniqueOverride = nullptr)
    {
        if (!bConsoleGammaEnabled || !dev) return;
        if (!InitShaderAndStaticResources(dev)) return;
        if (!CreateTextures(dev)) return;

        IDirect3DSurface9* currentRT = nullptr;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        if (!UpdateSceneTex(dev, currentRT))
        {
            currentRT->Release();
            return;
        }

        IDirect3DSurface9* pBB = nullptr;
        IDirect3DSurface9* pOutput = GetOutputSurface(dev, currentRT, &pBB);
        if (!pOutput)
        {
            SafeRelease(pBB);
            currentRT->Release();
            return;
        }

        IDirect3DVertexBuffer9* oldVB = nullptr;
        IDirect3DVertexDeclaration9* oldDecl = nullptr;
        UINT oldOffset = 0, oldStride = 0;
        DWORD oldFVF = 0;

        SaveVertexState(dev, oldVB, oldDecl, oldOffset, oldStride, oldFVF);
        dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

        if (pOutput != currentRT)
            dev->SetRenderTarget(0, pOutput);

        pEffect->SetTexture(hInputTex2D, pSceneTex);
        D3DXHANDLE hTech = hGammaTechnique;
        if (szTechniqueOverride)
        {
            D3DXHANDLE hCustom = pEffect->GetTechniqueByName(szTechniqueOverride);
            if (hCustom)
                hTech = hCustom;
        }
        pEffect->SetTechnique(hTech);
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

        if (pOutput != currentRT)
            dev->SetRenderTarget(0, currentRT);

        RestoreVertexState(dev, oldVB, oldDecl, oldOffset, oldStride, oldFVF);

        SafeRelease(oldVB);
        SafeRelease(oldDecl);
        SafeRelease(pBB);
        currentRT->Release();
    }

    static void RenderBlur(IDirect3DDevice9* dev)
    {
        if (!bBlurEnabled || fBlurStrength <= 0.001f || !dev) return;
        if (!InitShaderAndStaticResources(dev)) return;
        if (!CreateTextures(dev)) return;

        IDirect3DSurface9* currentRT = nullptr;
        if (FAILED(dev->GetRenderTarget(0, &currentRT)) || !currentRT) return;

        if (!UpdateSceneTex(dev, currentRT))
        {
            currentRT->Release();
            return;
        }

        IDirect3DSurface9* pBB = nullptr;
        IDirect3DSurface9* pOutput = GetOutputSurface(dev, currentRT, &pBB);
        if (!pOutput)
        {
            SafeRelease(pBB);
            currentRT->Release();
            return;
        }

        IDirect3DVertexBuffer9* oldVB = nullptr;
        IDirect3DVertexDeclaration9* oldDecl = nullptr;
        UINT oldOffset = 0, oldStride = 0;
        DWORD oldFVF = 0;

        SaveVertexState(dev, oldVB, oldDecl, oldOffset, oldStride, oldFVF);
        dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

        float texelSize[2] = { 1.0f / (float)backBufferInfo.width, 1.0f / (float)backBufferInfo.height };

        UINT passes = 0;

        dev->SetRenderTarget(0, pBlurIntermediateSurf);

        pEffect->SetTexture(hInputTex2D, pSceneTex);
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

        pEffect->SetTexture(hInputTex2D, pBlurIntermediateTex);
        pEffect->SetTechnique(hBlurVerticalTechnique);
        pEffect->CommitChanges();

        pEffect->Begin(&passes, 0);
        pEffect->BeginPass(0);
        DrawScreenQuad(dev);
        pEffect->EndPass();
        pEffect->End();

        if (pOutput != currentRT)
            dev->SetRenderTarget(0, currentRT);

        RestoreVertexState(dev, oldVB, oldDecl, oldOffset, oldStride, oldFVF);

        SafeRelease(oldVB);
        SafeRelease(oldDecl);
        SafeRelease(pBB);
        currentRT->Release();
    }

    static void Shutdown()
    {
        // Release the effect completely instead of just calling OnLostDevice.
        // A live ID3DXEffect keeps device objects alive (its internal state
        // manager state block, shaders), which makes the game's
        // IDirect3DDevice9::Reset fail with D3DERR_INVALIDCALL and retry
        // forever (NFS: infinite reset loop inside d3d9.dll). The effect is
        // recreated lazily on the first render after the reset in
        // InitShaderAndStaticResources, like the original per-game modules did.
        SafeRelease(pEffect);
        ReleaseTextures();
        bBackBufferInfoDirty = true;
    }

    static void OnDeviceReset()
    {
        // The effect was released in Shutdown() and is recreated lazily after
        // the reset, so there is nothing to reset here.
        bBackBufferInfoDirty = true;
    }
};
