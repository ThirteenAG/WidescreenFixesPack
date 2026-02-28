module;

#include <stdafx.h>
#include <d3d9.h>
#include <d3dx9effect.h>
#pragma comment(lib, "D3dx9.lib")

export module PostFX;

import ComVars;

inline void SAFE_RELEASE(auto*& ptr)
{
    if (ptr) { ptr->Release(); ptr = nullptr; }
}

class CPostFX
{
public:
    static inline bool bInitialized = false;
    static inline bool bConsoleGammaEnabled = true;
    static inline bool bBlurEnabled = false;
    static inline float fBlurStrength = 5.0f;

    static inline UINT nScreenWidth = 0;
    static inline UINT nScreenHeight = 0;

    static inline IDirect3DTexture9* pSceneTex = nullptr;
    static inline IDirect3DSurface9* pSceneSurf = nullptr;

    static inline IDirect3DTexture9* pBlurIntermediateTex = nullptr;
    static inline IDirect3DSurface9* pBlurIntermediateSurf = nullptr;

    static inline ID3DXEffect* pEffect = nullptr;
    static inline IDirect3DVertexDeclaration9* pQuadVertexDecl = nullptr;

    struct
    {
        D3DXHANDLE InputTex2D;
        D3DXHANDLE ConsoleGammaTechnique;
        D3DXHANDLE BlurHorizontalTechnique;
        D3DXHANDLE BlurVerticalTechnique;
        D3DXHANDLE TexelSize;
        D3DXHANDLE BlurStrengthParam;

        inline void GetHandles(ID3DXEffect* effect)
        {
            InputTex2D = effect->GetParameterByName(nullptr, "InputTex2D");
            ConsoleGammaTechnique = effect->GetTechniqueByName("ConsoleGamma");
            BlurHorizontalTechnique = effect->GetTechniqueByName("BlurHorizontal");
            BlurVerticalTechnique = effect->GetTechniqueByName("BlurVertical");
            TexelSize = effect->GetParameterByName(nullptr, "TexelSize");
            BlurStrengthParam = effect->GetParameterByName(nullptr, "BlurStrength");
        }
    } static inline EffectHandles = {};

    static void RenderGamma(IDirect3DDevice9* device)
    {
        if (!bConsoleGammaEnabled)
            return;

        if (!bInitialized)
            Initialize(device);

        if (!pEffect || !pSceneTex || !pSceneSurf || !pQuadVertexDecl)
            return;

        IDirect3DSurface9* pRealBB = GetRealBackBuffer(device);
        if (!pRealBB)
            return;

        IDirect3DSurface9* pCurrentRT = nullptr;
        device->GetRenderTarget(0, &pCurrentRT);
        if (pCurrentRT)
        {
            device->StretchRect(pCurrentRT, nullptr, pSceneSurf, nullptr, D3DTEXF_POINT);
            SAFE_RELEASE(pCurrentRT);
        }

        IDirect3DStateBlock9* pState = nullptr;
        device->CreateStateBlock(D3DSBT_ALL, &pState);
        if (pState) pState->Capture();

        device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        device->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000F);

        for (int i = 0; i < 8; ++i)
            device->SetTexture(i, nullptr);

        device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

        device->SetVertexDeclaration(pQuadVertexDecl);

        device->SetRenderTarget(0, pRealBB);

        pEffect->SetTexture(EffectHandles.InputTex2D, pSceneTex);
        pEffect->SetTechnique(EffectHandles.ConsoleGammaTechnique);
        pEffect->CommitChanges();

        UINT passes = 0;
        pEffect->Begin(&passes, 0);
        pEffect->BeginPass(0);
        DrawScreenQuad(device, nScreenWidth, nScreenHeight);
        pEffect->EndPass();
        pEffect->End();

        if (pState)
        {
            pState->Apply();
            pState->Release();
        }

        SAFE_RELEASE(pRealBB);
    }

    static void RenderBlur(IDirect3DDevice9* device)
    {
        if (!bBlurEnabled || fBlurStrength <= 0.001f)
            return;

        if (!bInitialized)
            Initialize(device);

        if (!pEffect || !pSceneTex || !pSceneSurf || !pQuadVertexDecl ||
            !pBlurIntermediateTex || !pBlurIntermediateSurf)
            return;

        IDirect3DSurface9* pRealBB = GetRealBackBuffer(device);
        if (!pRealBB)
            return;

        IDirect3DSurface9* pCurrentRT = nullptr;
        device->GetRenderTarget(0, &pCurrentRT);
        if (pCurrentRT)
        {
            device->StretchRect(pCurrentRT, nullptr, pSceneSurf, nullptr, D3DTEXF_POINT);
            SAFE_RELEASE(pCurrentRT);
        }

        IDirect3DStateBlock9* pState = nullptr;
        device->CreateStateBlock(D3DSBT_ALL, &pState);
        if (pState) pState->Capture();

        device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        device->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000F);

        for (int i = 0; i < 8; ++i)
            device->SetTexture(i, nullptr);

        device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

        device->SetVertexDeclaration(pQuadVertexDecl);

        float texelSize[2] = { 1.0f / (float)nScreenWidth, 1.0f / (float)nScreenHeight };

        UINT passes = 0;

        device->SetRenderTarget(0, pBlurIntermediateSurf);

        pEffect->SetTexture(EffectHandles.InputTex2D, pSceneTex);
        pEffect->SetFloatArray(EffectHandles.TexelSize, texelSize, 2);
        pEffect->SetFloat(EffectHandles.BlurStrengthParam, fBlurStrength);
        pEffect->SetTechnique(EffectHandles.BlurHorizontalTechnique);
        pEffect->CommitChanges();

        pEffect->Begin(&passes, 0);
        pEffect->BeginPass(0);
        DrawScreenQuad(device, nScreenWidth, nScreenHeight);
        pEffect->EndPass();
        pEffect->End();

        device->SetRenderTarget(0, pRealBB);

        pEffect->SetTexture(EffectHandles.InputTex2D, pBlurIntermediateTex);
        pEffect->SetTechnique(EffectHandles.BlurVerticalTechnique);
        pEffect->CommitChanges();

        pEffect->Begin(&passes, 0);
        pEffect->BeginPass(0);
        DrawScreenQuad(device, nScreenWidth, nScreenHeight);
        pEffect->EndPass();
        pEffect->End();

        if (pState)
        {
            pState->Apply();
            pState->Release();
        }

        SAFE_RELEASE(pRealBB);
    }

    static void Initialize(IDirect3DDevice9* device)
    {
        if (bInitialized) return;
        bInitialized = true;

        IDirect3DSurface9* pBB = GetRealBackBuffer(device);
        if (!pBB)
        {
            OutputDebugStringA("PostFX: Failed to get real backbuffer\n");
            bInitialized = false;
            return;
        }

        D3DSURFACE_DESC desc{};
        pBB->GetDesc(&desc);
        nScreenWidth = desc.Width;
        nScreenHeight = desc.Height;
        SAFE_RELEASE(pBB);

        if (FAILED(device->CreateTexture(nScreenWidth, nScreenHeight, 1,
            D3DUSAGE_RENDERTARGET, desc.Format,
            D3DPOOL_DEFAULT, &pSceneTex, nullptr)))
        {
            OutputDebugStringA("PostFX: Failed to create scene texture\n");
            bInitialized = false;
            return;
        }

        pSceneTex->GetSurfaceLevel(0, &pSceneSurf);

        if (bBlurEnabled)
        {
            if (FAILED(device->CreateTexture(nScreenWidth, nScreenHeight, 1,
                D3DUSAGE_RENDERTARGET, desc.Format,
                D3DPOOL_DEFAULT, &pBlurIntermediateTex, nullptr)))
            {
                OutputDebugStringA("PostFX: Failed to create blur intermediate texture\n");
                bInitialized = false;
                return;
            }
            pBlurIntermediateTex->GetSurfaceLevel(0, &pBlurIntermediateSurf);
        }

        CreateQuadVertexDeclaration(device);

        HMODULE hMod = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&Initialize, &hMod);

        ID3DXBuffer* err = nullptr;
        HRESULT hr = D3DXCreateEffectFromResource(device, hMod, MAKEINTRESOURCE(IDR_POSTFX),
                                                  nullptr, nullptr, 0, nullptr, &pEffect, &err);

        if (FAILED(hr))
        {
            if (err) { OutputDebugStringA((LPCSTR)err->GetBufferPointer()); err->Release(); }
            pEffect = nullptr;
            bInitialized = false;
            return;
        }

        EffectHandles.GetHandles(pEffect);
    }

    static IDirect3DSurface9* GetRealBackBuffer(IDirect3DDevice9* device)
    {
        IDirect3DSwapChain9* pSwap = nullptr;
        IDirect3DSurface9* pBB = nullptr;
        if (SUCCEEDED(device->GetSwapChain(0, &pSwap)) && pSwap)
        {
            pSwap->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBB);
            pSwap->Release();
        }
        return pBB;
    }

    static void DrawScreenQuad(IDirect3DDevice9* device, UINT w, UINT h)
    {
        struct { float x, y, z, rhw, u, v; } verts[4] = {
            {-0.5f, -0.5f, 0,1, 0,0},
            {-0.5f, (float)h - 0.5f, 0,1, 0,1},
            {(float)w - 0.5f, -0.5f, 0,1, 1,0},
            {(float)w - 0.5f, (float)h - 0.5f, 0,1, 1,1}
        };
        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(verts[0]));
    }

    static HRESULT CreateQuadVertexDeclaration(IDirect3DDevice9* device)
    {
        if (pQuadVertexDecl) return S_OK;
        D3DVERTEXELEMENT9 decl[] =
        {
            { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
            { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END()
        };
        return device->CreateVertexDeclaration(decl, &pQuadVertexDecl);
    }

    static void Shutdown()
    {
        SAFE_RELEASE(pQuadVertexDecl);
        SAFE_RELEASE(pEffect);
        SAFE_RELEASE(pSceneTex);
        SAFE_RELEASE(pSceneSurf);
        SAFE_RELEASE(pBlurIntermediateTex);
        SAFE_RELEASE(pBlurIntermediateSurf);
        bInitialized = false;
    }
};

namespace UD3DRenderDevice
{
    ptrdiff_t pDeviceOffset = 0;
    SafetyHookInline shPresent = {};
    void __fastcall Present(void* UD3DRenderDevice, void* edx, void* UViewport, int a3)
    {
        auto device = *(IDirect3DDevice9**)((uintptr_t)UD3DRenderDevice + pDeviceOffset);
        CPostFX::RenderGamma(device);
        return shPresent.unsafe_fastcall(UD3DRenderDevice, edx, UViewport, a3);
    }
}

export void InitPostFX()
{
    CIniReader iniReader("");
    CPostFX::bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 1) != 0;
    CPostFX::bBlurEnabled = iniReader.ReadInteger("GRAPHICS", "BlurEnabled", 1) != 0;
    CPostFX::fBlurStrength = iniReader.ReadFloat("GRAPHICS", "BlurStrength", 5.0f);

    if (!CPostFX::bConsoleGammaEnabled && !CPostFX::bBlurEnabled)
        return;

    auto pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B 8F ? ? ? ? ? ? 53");
    UD3DRenderDevice::pDeviceOffset = *pattern.get_first<int32_t>(2);

    UD3DRenderDevice::shPresent = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?Present@UD3DRenderDevice@@UAEXPAVUViewport@@H@Z"), UD3DRenderDevice::Present);

    static auto resetDeviceHook = safetyhook::create_mid(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?resetDevice@UD3DRenderDevice@@QAEXAAU_D3DPRESENT_PARAMETERS_@@@Z"), [](SafetyHookContext& regs)
    {
        CPostFX::Shutdown();
    });

    static void* UD3DRenderDevice = nullptr;
    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "0F 85 ? ? ? ? 8B AF");
    static auto RenderBlurHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UD3DRenderDevice = (void*)regs.edi;
    });

    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B 8C 24 ? ? ? ? 5F 5E 5D 5B 64 89 0D ? ? ? ? 81 C4 8C 00 00 00");
    static auto RenderBlurHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto device = *(IDirect3DDevice9**)((uintptr_t)UD3DRenderDevice + UD3DRenderDevice::pDeviceOffset);
        CPostFX::RenderBlur(device);
    });
}