module;

#include <stdafx.h>
#include <d3d9.h>
#include <d3dx9effect.h>
#pragma comment(lib, "D3dx9.lib")

export module DistantBlur;

import ComVars;

static IUnknown** pDevice = nullptr;

constexpr inline void SAFE_RELEASE(auto*& ptr)
{
    if (ptr) { ptr->Release(); ptr = nullptr; }
}

void DrawDOF(float* screenData)
{
    static ID3DXEffect* effect = nullptr;

    float zn = screenData[7];
    float zf = screenData[8];

    IDirect3DDevice9* pD3D9Device = nullptr;
    if (FAILED((*pDevice)->QueryInterface(__uuidof(IDirect3DDevice9*), (void**)&pD3D9Device)))
        return;

    static bool bOnce = false;
    if (!bOnce && !effect)
    {
        HMODULE hModule;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&DrawDOF, &hModule);
        ID3DXBuffer* error = nullptr;
        HRESULT hr = D3DXCreateEffectFromResource(pD3D9Device, hModule, MAKEINTRESOURCE(IDR_DISTANTBLUR), nullptr, nullptr, 0, nullptr, &effect, &error);

        if (FAILED(hr))
        {
            if (error)
            {
                OutputDebugStringA((LPCSTR)error->GetBufferPointer());
                error->Release();
            }
            effect = nullptr;
        }
        bOnce = true;
    }

    if (!effect)
        return;

    static IDirect3DTexture9* backBufferTex = nullptr;
    static IDirect3DSurface9* backBufferCopySurf = nullptr;

    IDirect3DSurface9* depthSurf = nullptr;
    IDirect3DTexture9* depthTex = nullptr;
    IDirect3DSurface9* rt0 = nullptr;
    IDirect3DVertexDeclaration9* oldDecl = nullptr;
    IDirect3DVertexBuffer9* oldVB = nullptr;
    UINT oldOffset = 0;
    UINT oldStride = 0;
    DWORD oldFVF = 0;

    HRESULT hr;

    pD3D9Device->GetFVF(&oldFVF);
    pD3D9Device->GetVertexDeclaration(&oldDecl);
    pD3D9Device->GetStreamSource(0, &oldVB, &oldOffset, &oldStride);

    pD3D9Device->GetDepthStencilSurface(&depthSurf);
    pD3D9Device->GetRenderTarget(0, &rt0);
    depthSurf->GetContainer(__uuidof(IDirect3DTexture9*), (void**)&depthTex);

    D3DSURFACE_DESC desc;
    rt0->GetDesc(&desc);
    struct ScreenVertex { float x, y, z, rhw; float u, v; };

    if (!backBufferTex)
    {
        pD3D9Device->CreateTexture(desc.Width, desc.Height, 1, desc.Usage, desc.Format, desc.Pool, &backBufferTex, nullptr);
        backBufferTex->GetSurfaceLevel(0, &backBufferCopySurf);
    }

    float width = static_cast<float>(desc.Width);
    float height = static_cast<float>(desc.Height);

    ScreenVertex screenVertices[4] =
    {
        { -0.5f + 0.5f / width,  -0.5f + 0.5f / height,  0.0f, 1.0f,  0.0f, 0.0f },  // top-left
        { -0.5f + 0.5f / width,   height - 0.5f + 0.5f / height,  0.0f, 1.0f,  0.0f, 1.0f },  // bottom-left
        {  width - 0.5f + 0.5f / width,  -0.5f + 0.5f / height,  0.0f, 1.0f,  1.0f, 0.0f },  // top-right
        {  width - 0.5f + 0.5f / width,   height - 0.5f + 0.5f / height,  0.0f, 1.0f,  1.0f, 1.0f }   // bottom-right
    };

    pD3D9Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    pD3D9Device->SetDepthStencilSurface(nullptr);

    UINT passes;

    float texelSize[4] = { 1.0f / width, 1.0f / height };

    effect->SetTexture("colorTex2D", backBufferTex);
    effect->SetTexture("depthTex2D", depthTex);
    effect->SetFloat("nearPlane", zn);
    effect->SetFloat("farPlane", zf);
    effect->SetFloatArray("texelSize", texelSize, 2);

    hr = pD3D9Device->StretchRect(rt0, nullptr, backBufferCopySurf, nullptr, D3DTEXF_LINEAR);
    assert(SUCCEEDED(hr));

    effect->CommitChanges();

    effect->Begin(&passes, {});
    for (UINT pass = 0; pass < passes; ++pass)
    {
        effect->BeginPass(pass);

        hr = pD3D9Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, screenVertices, sizeof(ScreenVertex));
        assert(SUCCEEDED(hr));

        effect->EndPass();
    }
    effect->End();

    pD3D9Device->SetDepthStencilSurface(depthSurf);

    depthSurf->Release();
    depthTex->Release();
    pD3D9Device->Release();
    rt0->Release();

    pD3D9Device->SetFVF(oldFVF);
    pD3D9Device->SetVertexDeclaration(oldDecl);
    pD3D9Device->SetStreamSource(0, oldVB, oldOffset, oldStride);

    SAFE_RELEASE(oldDecl);
    SAFE_RELEASE(oldVB);
}

struct HandlerNode
{
    HandlerNode* next;
    int          field_4;
    int          user_context;
    int          (__cdecl* callback)(int context, float* arg);
    int          tag;
};

HandlerNode** g_handler_list = nullptr;
int* g_last_tag = nullptr;
int (__cdecl* sub_52F7D0)(int context, float* arg) = nullptr;

void __cdecl sub_533AA0(float* screenData)
{
    *g_last_tag = 0;

    for (auto* node = *g_handler_list; node; node = node->next)
    {
        *g_last_tag = node->tag;

        if (node->callback)
        {
            auto ret = node->callback(node->user_context, screenData);

            if (reinterpret_cast<uintptr_t>(node->callback) == reinterpret_cast<uintptr_t>(sub_52F7D0))
                DrawDOF(screenData);

            if (ret)
                return;
        }
    }
}

export void InitDistantBlur()
{
    auto pattern = hook::pattern("A1 ? ? ? ? 8B 10 8D 4C 24 ? 51 50");
    pDevice = *pattern.get_first<IUnknown**>(1);

    pattern = hook::pattern("A1 ? ? ? ? 56 8B 30 85 F6 57 74 ? 8B 7C 24 ? F3 0F 10 40");
    g_handler_list = *pattern.get_first<HandlerNode**>(1);

    pattern = hook::pattern("0F 2F 05 ? ? ? ? 72 ? 33 C0 8B 4C 24 ? 64 89 0D ? ? ? ? 5E");
    g_last_tag = *pattern.get_first<int*>(3);

    pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? 80 B8 ? ? ? ? ? 75 ? 56");
    sub_52F7D0 = (decltype(sub_52F7D0))pattern.get_first();

    pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 8B CF E8 ? ? ? ? 5F");
    injector::MakeCALL(pattern.get_first(), sub_533AA0);
}

//53e480
//53d4a0 Dyn.Shadow / Stat.Shadow
//5384e0 Ground / Decals / Roads / Junctions
//5ac870
//538540
//53d4b0 Reflections
//538550 Cutouts
//538570 Inst
//538580 Dynamic
//538560
//52f800
//53e470
//538530 LoRes
//52f7d0
//52f820 Trans
//52f830 Particles
//52f7b0 Rain
//538590
//52f880
//52f8a0 Dof / Bloom
//5385a0
//471af0
//471b00
//5385b0
//5de410
//44fcc0
//44fcd0
//471ae0
