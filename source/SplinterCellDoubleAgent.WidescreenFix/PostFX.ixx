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

    static inline IDirect3DTexture9* pBackBufferTex = nullptr;
    static inline IDirect3DSurface9* pBackBufferSurf = nullptr;

    static inline UINT nScreenWidth = 0;
    static inline UINT nScreenHeight = 0;

    static inline ID3DXEffect* pEffect = nullptr;

    static inline IDirect3DVertexDeclaration9* pQuadVertexDecl = nullptr;

    struct
    {
        D3DXHANDLE InputTex2D;
        D3DXHANDLE ConsoleGammaTechnique;

        inline void GetHandles(ID3DXEffect* effect)
        {
            InputTex2D = effect->GetParameterByName(nullptr, "InputTex2D");
            ConsoleGammaTechnique = effect->GetTechniqueByName("ConsoleGamma");
        }

    } static inline EffectHandles = {};

    static HRESULT CreateQuadVertexDeclaration(IDirect3DDevice9* device)
    {
        if (pQuadVertexDecl) return S_OK;

        D3DVERTEXELEMENT9 decl[] =
        {
            { 0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },  // x,y,z,rhw
            { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },  // u,v
            D3DDECL_END()
        };

        return device->CreateVertexDeclaration(decl, &pQuadVertexDecl);
    }

    static inline void Initialize(IDirect3DDevice9* device, IDirect3DSurface9* backBuffer)
    {
        if (bInitialized) return;
        bInitialized = true;

        D3DSURFACE_DESC desc;
        backBuffer->GetDesc(&desc);
        nScreenWidth = desc.Width;
        nScreenHeight = desc.Height;

        HRESULT hr = device->CreateTexture(desc.Width, desc.Height, 1,
                                           desc.Usage, desc.Format, desc.Pool,
                                           &pBackBufferTex, nullptr);
        if (SUCCEEDED(hr))
            pBackBufferTex->GetSurfaceLevel(0, &pBackBufferSurf);

        CreateQuadVertexDeclaration(device);

        HMODULE hModule = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&Initialize, &hModule);

        ID3DXBuffer* error = nullptr;
        hr = D3DXCreateEffectFromResource(device, hModule, MAKEINTRESOURCE(IDR_POSTFX),
                                          nullptr, nullptr, 0, nullptr, &pEffect, &error);

        if (FAILED(hr))
        {
            if (error)
            {
                OutputDebugStringA((LPCSTR)error->GetBufferPointer());
                error->Release();
            }
            pEffect = nullptr;
            return;
        }

        EffectHandles.GetHandles(pEffect);
    }

    static inline void UpdateBackBuffer(IDirect3DDevice9* device, IDirect3DSurface9* rt0)
    {
        device->StretchRect(rt0, nullptr, pBackBufferSurf, nullptr, D3DTEXF_NONE);
    }

    static inline void DrawScreenQuad(IDirect3DDevice9* device, UINT width, UINT height)
    {
        struct ScreenVertex
        {
            float x, y, z, rhw;
            float u, v;
        };

        ScreenVertex verts[4] =
        {
            { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
            { -0.5f, (float)height - 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
            { (float)width - 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f },
            { (float)width - 0.5f, (float)height - 0.5f, 0.0f, 1.0f, 1.0f, 1.0f }
        };

        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(ScreenVertex));
    }

    static inline void RenderSafe(IDirect3DDevice9* d3d9device, std::function<void(IDirect3DDevice9*, IDirect3DSurface9*)> cb)
    {
        IDirect3DSurface9* rt0 = nullptr;
        IDirect3DVertexDeclaration9* oldDecl = nullptr;
        IDirect3DVertexBuffer9* oldVB = nullptr;
        UINT oldOffset = 0, oldStride = 0;

        d3d9device->GetRenderTarget(0, &rt0);
        d3d9device->GetVertexDeclaration(&oldDecl);
        d3d9device->GetStreamSource(0, &oldVB, &oldOffset, &oldStride);

        Initialize(d3d9device, rt0);

        if (pQuadVertexDecl)
            d3d9device->SetVertexDeclaration(pQuadVertexDecl);

        d3d9device->SetStreamSource(0, nullptr, 0, 0);

        cb(d3d9device, rt0);

        d3d9device->SetRenderTarget(0, rt0);
        d3d9device->SetVertexDeclaration(oldDecl);
        d3d9device->SetStreamSource(0, oldVB, oldOffset, oldStride);

        SAFE_RELEASE(rt0);
        SAFE_RELEASE(oldDecl);
        SAFE_RELEASE(oldVB);
    }

    class ConsoleGamma
    {
    public:
        static inline bool bConsoleGammaEnabled = true;

        static inline void Render(IDirect3DDevice9* device)
        {
            if (!bConsoleGammaEnabled) return;

            RenderSafe(device, [](IDirect3DDevice9* device, IDirect3DSurface9* rt0)
            {
                UpdateBackBuffer(device, rt0);
                device->SetRenderTarget(0, rt0);

                pEffect->SetTexture(EffectHandles.InputTex2D, pBackBufferTex);
                pEffect->SetTechnique(EffectHandles.ConsoleGammaTechnique);
                pEffect->CommitChanges();

                UINT passes = 0;
                pEffect->Begin(&passes, 0);
                pEffect->BeginPass(0);
                DrawScreenQuad(device, nScreenWidth, nScreenHeight);
                pEffect->EndPass();
                pEffect->End();
            });
        }
    };

    static void Shutdown()
    {
        SAFE_RELEASE(pQuadVertexDecl);
        SAFE_RELEASE(pEffect);
        SAFE_RELEASE(pBackBufferTex);
        SAFE_RELEASE(pBackBufferSurf);
        bInitialized = false;
    }
};

export void InitPostFX()
{
    CIniReader iniReader("");
    CPostFX::ConsoleGamma::bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 1) != 0;

    if (!CPostFX::ConsoleGamma::bConsoleGammaEnabled)
        return;

    auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 11 53 50 8D 45");
    static auto UD3DRenderDevicePresentHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto device = (IDirect3DDevice9*)regs.ecx;
        CPostFX::ConsoleGamma::Render(device);
    });

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 10 56 53 50");
    static auto UD3DRenderDevicePresentHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto device = (IDirect3DDevice9*)regs.eax;
        CPostFX::ConsoleGamma::Render(device);
    });

    static auto DeviceResetHook = safetyhook::create_mid(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?resetDevice@UD3DRenderDevice@@QAEXAAU_D3DPRESENT_PARAMETERS_@@@Z"), [](SafetyHookContext& regs)
    {
        CPostFX::Shutdown();
    });
}