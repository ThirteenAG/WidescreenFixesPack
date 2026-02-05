module;

#include <stdafx.h>
#include <d3d9.h>
#include <d3dx9effect.h>
#pragma comment(lib, "D3dx9.lib")

export module DistantBlur;

import ComVars;

static IUnknown** pD3D8Device = nullptr;

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
    struct
    {
        D3DXHANDLE InputTex2D;
        D3DXHANDLE DepthTex2D;

        D3DXHANDLE vec2ViewportSize;
        D3DXHANDLE vec4SMAARTMetrics;

        // distance blur
        D3DXHANDLE fNearPlane;
        D3DXHANDLE fFarPlane;
        D3DXHANDLE fDistBlurStrength;

        // bloom
        D3DXHANDLE fBloomIntensity;
        D3DXHANDLE fBloomThreshold;
        D3DXHANDLE fBloomSoftThreshold;
        D3DXHANDLE vec2BloomTexelSize;

        // SMAA
        D3DXHANDLE colorTex2D;
        D3DXHANDLE edgesTex2D;
        D3DXHANDLE blendTex2D;
        D3DXHANDLE areaTex2D;
        D3DXHANDLE searchTex2D;

        // techniques
        D3DXHANDLE DistanceBlurTechnique;
        D3DXHANDLE ConsoleGammaTechnique;
        D3DXHANDLE BloomTechnique;

        D3DXHANDLE SMAAEdgeDetectionTechnique;
        D3DXHANDLE SMAABlendWeightCalculationTechnique;
        D3DXHANDLE SMAAOutputPassTechnique;

        inline void GetHandles(ID3DXEffect* effect)
        {
            InputTex2D = effect->GetParameterByName(nullptr, "InputTex2D");
            DepthTex2D = effect->GetParameterByName(nullptr, "DepthTex2D");
            vec2ViewportSize = effect->GetParameterByName(nullptr, "vec2ViewportSize");
            fNearPlane = effect->GetParameterByName(nullptr, "fNearPlane");
            fFarPlane = effect->GetParameterByName(nullptr, "fFarPlane");
            fDistBlurStrength = effect->GetParameterByName(nullptr, "fDistBlurStrength");
            fBloomIntensity = effect->GetParameterByName(nullptr, "fBloomIntensity");
            fBloomThreshold = effect->GetParameterByName(nullptr, "fBloomThreshold");
            fBloomSoftThreshold = effect->GetParameterByName(nullptr, "fBloomSoftThreshold");
            vec2BloomTexelSize = effect->GetParameterByName(nullptr, "vec2BloomTexelSize");
            colorTex2D = effect->GetParameterByName(nullptr, "colorTex2D");
            edgesTex2D = effect->GetParameterByName(nullptr, "edgesTex2D");
            blendTex2D = effect->GetParameterByName(nullptr, "blendTex2D");
            areaTex2D = effect->GetParameterByName(nullptr, "areaTex2D");
            searchTex2D = effect->GetParameterByName(nullptr, "searchTex2D");
            vec4SMAARTMetrics = effect->GetParameterByName(nullptr, "vec4SMAARTMetrics");

            DistanceBlurTechnique = effect->GetTechniqueByName("DistanceBlur");
            ConsoleGammaTechnique = effect->GetTechniqueByName("ConsoleGamma");
            BloomTechnique = effect->GetTechniqueByName("Bloom");

            SMAAEdgeDetectionTechnique = effect->GetTechniqueByName("SMAAEdgeDetection");
            SMAABlendWeightCalculationTechnique = effect->GetTechniqueByName("SMAABlendWeightCalculation");
            SMAAOutputPassTechnique = effect->GetTechniqueByName("SMAAOutputPass");
        }

    } static inline EffectHandles = {};

    static inline void Initialize(IDirect3DDevice9* device, IDirect3DSurface9* backBuffer)
    {
        if (bInitialized)
            return;
        bInitialized = true;

        D3DSURFACE_DESC desc;
        backBuffer->GetDesc(&desc);
        nScreenWidth = desc.Width;
        nScreenHeight = desc.Height;

        device->CreateTexture(desc.Width, desc.Height, 1, desc.Usage, desc.Format, desc.Pool, &pBackBufferTex, nullptr);
        pBackBufferTex->GetSurfaceLevel(0, &pBackBufferSurf);

        HMODULE hModule;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&Initialize, &hModule);
        ID3DXBuffer* error = nullptr;
        HRESULT hr = D3DXCreateEffectFromResource(device, hModule, MAKEINTRESOURCE(IDR_POSTFX), nullptr, nullptr, 0, nullptr, &pEffect, &error);

        if (FAILED(hr))
        {
            if (error)
            {
                OutputDebugStringA((LPCSTR)error->GetBufferPointer());
                error->Release();
            }
            pEffect = nullptr;
        }

        EffectHandles.GetHandles(pEffect);
    }

    static inline void UpdateBackBuffer(IDirect3DDevice9* device, IDirect3DSurface9* rt0)
    {
        assert(SUCCEEDED(device->StretchRect(rt0, nullptr, pBackBufferSurf, nullptr, D3DTEXF_NONE)));
    }

    static inline void DrawScreenQuad(IDirect3DDevice9* device, float width, float height)
    {
        struct ScreenVertex { float x, y, z, rhw; float u, v; };
        ScreenVertex screenVertices[4] =
        {
            { -0.5f,         -0.5f,          0.0f, 1.0f, 0.0f, 0.0f },
            { -0.5f,          height - 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
            { width - 0.5f, -0.5f,          0.0f, 1.0f, 1.0f, 0.0f },
            { width - 0.5f, height - 0.5f, 0.0f, 1.0f, 1.0f, 1.0f }
        };
        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, screenVertices, sizeof(ScreenVertex));
    }

    static inline void RenderSafe(std::function<void(IDirect3DDevice9* device, IDirect3DSurface9* rt0, IDirect3DTexture9* depthTex)> cb)
    {
        IDirect3DDevice9* d3d9device = nullptr;
        if (FAILED((*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9*), (void**)&d3d9device)))
            return;

        IDirect3DSurface9* depthSurf = nullptr;
        IDirect3DTexture9* depthTex = nullptr;
        IDirect3DSurface9* rt0 = nullptr;
        IDirect3DVertexDeclaration9* oldDecl = nullptr;
        IDirect3DVertexBuffer9* oldVB = nullptr;
        UINT oldOffset = 0;
        UINT oldStride = 0;
        DWORD oldFVF = 0;

        d3d9device->GetFVF(&oldFVF);
        d3d9device->GetVertexDeclaration(&oldDecl);
        d3d9device->GetStreamSource(0, &oldVB, &oldOffset, &oldStride);

        d3d9device->GetDepthStencilSurface(&depthSurf);

        d3d9device->GetRenderTarget(0, &rt0);
        depthSurf->GetContainer(__uuidof(IDirect3DTexture9*), (void**)&depthTex);

        Initialize(d3d9device, rt0);

        d3d9device->SetDepthStencilSurface(nullptr);
        d3d9device->SetStreamSource(0, nullptr, 0, 0);
        d3d9device->SetVertexDeclaration(nullptr);
        d3d9device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1); // fullscreen fvf

        cb(d3d9device, rt0, depthTex);

        d3d9device->SetRenderTarget(0, rt0);
        d3d9device->SetDepthStencilSurface(depthSurf);

        d3d9device->SetFVF(oldFVF);
        d3d9device->SetVertexDeclaration(oldDecl);
        d3d9device->SetStreamSource(0, oldVB, oldOffset, oldStride);

        SAFE_RELEASE(depthSurf);
        SAFE_RELEASE(depthTex);
        SAFE_RELEASE(d3d9device);
        SAFE_RELEASE(rt0);
        SAFE_RELEASE(oldDecl);
        SAFE_RELEASE(oldVB);
    }

    class ConsoleGamma
    {
    public:
        static inline bool bConsoleGammaEnabled = true;

        static inline void Render()
        {
            if (bConsoleGammaEnabled)
            {
                RenderSafe([](IDirect3DDevice9* device, IDirect3DSurface9* rt0, IDirect3DTexture9* depthTex)
                {
                    UpdateBackBuffer(device, rt0);

                    device->SetRenderTarget(0, rt0);

                    pEffect->SetTexture(EffectHandles.InputTex2D, pBackBufferTex);
                    pEffect->SetTechnique(EffectHandles.ConsoleGammaTechnique);
                    pEffect->CommitChanges();

                    UINT passes;
                    pEffect->Begin(&passes, 0);
                    pEffect->BeginPass(0);
                    DrawScreenQuad(device, nScreenWidth, nScreenHeight);
                    pEffect->EndPass();
                    pEffect->End();
                });
            }
        }
    };
    class Bloom
    {
    public:
        static inline bool bBloomEnabled = true;
        static inline bool bBloomInitialized = false;

        static inline UINT BloomWidth = 0;
        static inline UINT BloomHeight = 0;

        static inline IDirect3DTexture9* pBloomTex = nullptr;
        static inline IDirect3DTexture9* pBloomBlurTex = nullptr;

        static inline IDirect3DSurface9* pBloomSurface = nullptr;
        static inline IDirect3DSurface9* pBloomBlurSurface = nullptr;

        static inline float fBloomIntensity = 2.0;
        static inline float fBloomThreshold = 0.8;
        static inline float fBloomSoftThreshold = 0.75;

        static void Initialize(IDirect3DDevice9* device)
        {
            if (bBloomEnabled && !bBloomInitialized)
            {
                bBloomInitialized = true;

                BloomWidth = nScreenWidth / 2;
                BloomHeight = nScreenHeight / 2;

                device->CreateTexture(BloomWidth, BloomHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &pBloomTex, nullptr);
                device->CreateTexture(BloomWidth, BloomHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &pBloomBlurTex, nullptr);
                pBloomTex->GetSurfaceLevel(0, &pBloomSurface);
                pBloomBlurTex->GetSurfaceLevel(0, &pBloomBlurSurface);
            }
        }
        static void Render()
        {
            if (bBloomEnabled)
            {
                RenderSafe([](IDirect3DDevice9* device, IDirect3DSurface9* rt0, IDirect3DTexture9* depthTex)
                {
                    Initialize(device);

                    UpdateBackBuffer(device, rt0);

                    UINT passes;
                    pEffect->SetTechnique(EffectHandles.BloomTechnique);
                    pEffect->CommitChanges();

                    pEffect->Begin(&passes, 0);
                    assert(passes == 3);
                    {
                        pEffect->BeginPass(0); // extract highlights
                        {
                            device->SetRenderTarget(0, pBloomSurface);

                            float ViewportSize[] = { nScreenWidth, nScreenHeight };

                            pEffect->SetFloatArray(EffectHandles.vec2ViewportSize, ViewportSize, 2);
                            pEffect->SetTexture(EffectHandles.InputTex2D, pBackBufferTex);
                            pEffect->SetFloat(EffectHandles.fBloomIntensity, fBloomIntensity);
                            pEffect->SetFloat(EffectHandles.fBloomThreshold, fBloomThreshold);
                            pEffect->SetFloat(EffectHandles.fBloomSoftThreshold, fBloomSoftThreshold);

                            pEffect->CommitChanges();

                            DrawScreenQuad(device, BloomWidth, BloomHeight);
                        }
                        pEffect->EndPass();

                        pEffect->BeginPass(1); // blur
                        {
                            constexpr size_t numBlurPasses = 4;

                            for (size_t i = 0; i < numBlurPasses; ++i)
                            {
                                device->SetRenderTarget(0, pBloomBlurSurface);

                                float blurParamsH[2] = { 1.0f / (float)BloomWidth, 0.0 };
                                pEffect->SetTexture(EffectHandles.InputTex2D, pBloomTex);
                                pEffect->SetFloatArray(EffectHandles.vec2BloomTexelSize, blurParamsH, 2);
                                pEffect->CommitChanges();

                                DrawScreenQuad(device, BloomWidth, BloomHeight);

                                device->SetRenderTarget(0, pBloomSurface);
                                float blurParamsV[2] = { 0.0f, 1.0f / (float)BloomHeight };
                                pEffect->SetTexture(EffectHandles.InputTex2D, pBloomBlurTex);
                                pEffect->SetFloatArray(EffectHandles.vec2BloomTexelSize, blurParamsV, 2);
                                pEffect->CommitChanges();


                                DrawScreenQuad(device, BloomWidth, BloomHeight);
                            }
                        }
                        pEffect->EndPass();

                        pEffect->BeginPass(2); // output
                        {
                            device->SetRenderTarget(0, rt0);
                            pEffect->SetTexture(EffectHandles.InputTex2D, pBloomTex);
                            pEffect->CommitChanges();

                            DrawScreenQuad(device, nScreenWidth, nScreenHeight);
                        }
                        pEffect->EndPass();
                    }
                    pEffect->End();
                });
            }
        }
    };

    class DistanceBlur
    {
    public:
        static inline bool bDOFEnabled = true;

        static inline float fDistBlurStrength = 1.0f;

        static void Render(float nearZ, float farZ)
        {
            if (bDOFEnabled)
            {
                RenderSafe([=](IDirect3DDevice9* device, IDirect3DSurface9* rt0, IDirect3DTexture9* depthTex)
                {
                    UpdateBackBuffer(device, rt0);

                    float ViewportSize[] = { nScreenWidth, nScreenHeight };

                    pEffect->SetTexture(EffectHandles.InputTex2D, pBackBufferTex);
                    pEffect->SetTexture(EffectHandles.DepthTex2D, depthTex);
                    pEffect->SetFloatArray(EffectHandles.vec2ViewportSize, ViewportSize, 2);
                    pEffect->SetFloat(EffectHandles.fNearPlane, nearZ);
                    pEffect->SetFloat(EffectHandles.fFarPlane, farZ);
                    pEffect->SetFloat(EffectHandles.fDistBlurStrength, fDistBlurStrength);
                    pEffect->SetTechnique(EffectHandles.DistanceBlurTechnique);
                    pEffect->CommitChanges();

                    UINT passes;
                    pEffect->Begin(&passes, 0);
                    pEffect->BeginPass(0);
                    DrawScreenQuad(device, nScreenWidth, nScreenHeight);
                    pEffect->EndPass();
                    pEffect->End();
                });
            }
        }
    };

    class AntiAliasing
    {
    public:
        static inline bool bAAEnabled = true;
        static inline bool bAAInitialized = false;

        static inline IDirect3DTexture9* pAreaTex = {};
        static inline IDirect3DTexture9* pSearchTex = {};
        // these are to be rendered into
        static inline IDirect3DTexture9* pEdgeTex = {};
        static inline IDirect3DTexture9* pBlendTex = {};

        static inline IDirect3DSurface9* pEdgeSurface = {};
        static inline IDirect3DSurface9* pBlendSurface = {};

        static void Initialize(IDirect3DDevice9* device)
        {
            if (!bAAEnabled || bAAInitialized)
                return;

            bAAInitialized = true;

            device->CreateTexture(nScreenWidth, nScreenHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pEdgeTex, nullptr);
            device->CreateTexture(nScreenWidth, nScreenHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pBlendTex, nullptr);
            pEdgeTex->GetSurfaceLevel(0, &pEdgeSurface);
            pBlendTex->GetSurfaceLevel(0, &pBlendSurface);

            // Get the module handle (same as in the effect loading code)
            HMODULE hModule;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&Initialize, &hModule);

            D3DXIMAGE_INFO info;

            // Load area texture from resource
            HRESULT hrArea = D3DXGetImageInfoFromResource(hModule, MAKEINTRESOURCE(IDR_AREATEX), &info);
            if (SUCCEEDED(hrArea))
            {
                hrArea = D3DXCreateTextureFromResourceEx(device, hModule, MAKEINTRESOURCE(IDR_AREATEX),
                    info.Width, info.Height, 1, 0, D3DFMT_A8L8, D3DPOOL_DEFAULT,
                    D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, &info, nullptr, &pAreaTex);
                if (FAILED(hrArea))
                {
                    OutputDebugStringA("Failed to load area texture from resource.\n");
                    pAreaTex = nullptr;
                }
            }
            else
            {
                OutputDebugStringA("Failed to get info for area texture resource.\n");
            }

            // Load search texture from resource
            HRESULT hrSearch = D3DXGetImageInfoFromResource(hModule, MAKEINTRESOURCE(IDR_SEARCHTEX), &info);
            if (SUCCEEDED(hrSearch))
            {
                hrSearch = D3DXCreateTextureFromResourceEx(device, hModule, MAKEINTRESOURCE(IDR_SEARCHTEX),
                    info.Width, info.Height, 1, 0, D3DFMT_L8, D3DPOOL_DEFAULT,
                    D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, &info, nullptr, &pSearchTex);
                if (FAILED(hrSearch))
                {
                    OutputDebugStringA("Failed to load search texture from resource.\n");
                    pSearchTex = nullptr;
                }
            }
            else
            {
                OutputDebugStringA("Failed to get info for search texture resource.\n");
            }
        }

        static void Render()
        {
            if (bAAEnabled)
            {
                RenderSafe([](IDirect3DDevice9* device, IDirect3DSurface9* rt0, IDirect3DTexture9* depthTex)
                {
                    Initialize(device);
                    UpdateBackBuffer(device, rt0);

                    float ViewportSize[] = { 1.0 / nScreenWidth, 1.0 / nScreenHeight, nScreenWidth, nScreenHeight };
                    pEffect->SetFloatArray(EffectHandles.vec2ViewportSize, ViewportSize + 2, 2);
                    pEffect->SetFloatArray(EffectHandles.vec4SMAARTMetrics, ViewportSize, 4);

                    device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);

                    D3DXVECTOR2 pixelSize = D3DXVECTOR2(1.0f / float(nScreenWidth), 1.0f / float(nScreenHeight));
                    float quad[4][5] = {
                        { -1.0f - pixelSize.x,  1.0f + pixelSize.y, 0.5f, 0.0f, 0.0f },
                        {  1.0f - pixelSize.x,  1.0f + pixelSize.y, 0.5f, 1.0f, 0.0f },
                        { -1.0f - pixelSize.x, -1.0f + pixelSize.y, 0.5f, 0.0f, 1.0f },
                        {  1.0f - pixelSize.x, -1.0f + pixelSize.y, 0.5f, 1.0f, 1.0f }
                    };


                    auto DrawAAQuad = [&]()
                    {
                        UINT passes;
                        pEffect->Begin(&passes, 0);
                        pEffect->BeginPass(0);

                        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));

                        pEffect->EndPass();
                        pEffect->End();
                    };

                    // edge detection pass
                    device->SetRenderTarget(0, pEdgeSurface);
                    device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

                    pEffect->SetTexture(EffectHandles.colorTex2D, pBackBufferTex);
                    pEffect->SetTechnique(EffectHandles.SMAAEdgeDetectionTechnique);

                    pEffect->CommitChanges();

                    DrawAAQuad();

                    // blending weights calculations pass
                    device->SetRenderTarget(0, pBlendSurface);
                    device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

                    pEffect->SetTexture(EffectHandles.edgesTex2D, pEdgeTex);
                    pEffect->SetTexture(EffectHandles.areaTex2D, pAreaTex);
                    pEffect->SetTexture(EffectHandles.searchTex2D, pSearchTex);
                    pEffect->SetTechnique(EffectHandles.SMAABlendWeightCalculationTechnique);
                    pEffect->CommitChanges();

                    DrawAAQuad();

                    // output pass

                    device->SetRenderTarget(0, rt0);
                    pEffect->SetTexture(EffectHandles.colorTex2D, pBackBufferTex);
                    pEffect->SetTexture(EffectHandles.blendTex2D, pBlendTex);
                    pEffect->SetTechnique(EffectHandles.SMAAOutputPassTechnique);
                    pEffect->CommitChanges();

                    DrawAAQuad();
                });
            }
        }
    };
};

struct ListNode
{
    struct ListNode* next;
    uint32_t _reserved;
    uint32_t data;
    int (__cdecl* cb)(uint32_t data, float* arg);
    float value;
};

ListNode** g_listHead = nullptr;
float* g_currentVal = nullptr;
int (__cdecl* sub_52F7D0)(uint32_t data, float* arg) = nullptr;

void sub_533AA0(float* screenData)
{
    ListNode* node = *g_listHead;
    ListNode* next = node ? node->next : NULL;

    if (!next)
    {
        *g_currentVal = 0.0f;
        return;
    }

    for (;;)
    {
        *g_currentVal = node->value;

        if (node->cb != NULL && node->cb(node->data, screenData))
        {
            return;
        }
        else
        {
            if (node->cb == sub_52F7D0)
            {
                float zn = screenData[7];
                float zf = screenData[8];
                CPostFX::DistanceBlur::Render(zn, zf);
                CPostFX::Bloom::Render();
                CPostFX::AntiAliasing::Render();
                CPostFX::ConsoleGamma::Render();
            }
        }

        node = next;
        next = node->next;

        if (!next)
            break;
    }

    *g_currentVal = 0.0f;
}

export void InitDistantBlur()
{
    auto pattern = hook::pattern("A1 ? ? ? ? 8B 10 8D 4C 24 ? 51 50");
    pD3D8Device = *pattern.get_first<IUnknown**>(1);

    pattern = hook::pattern("A1 ? ? ? ? 56 8B 30 85 F6 57 74 ? 8B 7C 24 ? F3 0F 10 40");
    g_listHead = *pattern.get_first<ListNode**>(1);

    pattern = hook::pattern("0F 2F 05 ? ? ? ? 72 ? 33 C0 8B 4C 24 ? 64 89 0D ? ? ? ? 5E");
    g_currentVal = *pattern.get_first<float*>(3);

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
