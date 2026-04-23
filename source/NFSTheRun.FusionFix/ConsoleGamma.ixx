module;

#include <stdafx.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cassert>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

export module consolegamma;

import ComVars;

bool bConsoleGammaEnabled = false;

class ConsoleGammaDX11
{
    static inline void SafeRelease(auto*& ptr)
    {
        if (ptr) { ptr->Release(); ptr = nullptr; }
    }

public:
    static bool Initialize(IDXGISwapChain* swapChain)
    {
        if (s_initialized || !swapChain)
            return s_initialized;

        HRESULT hr = swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&s_device);
        if (FAILED(hr)) return false;
        s_device->GetImmediateContext(&s_context);

        static const char* shaderSource = R"(
struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 Texcoord : TEXCOORD0;
};

VS_OUTPUT FullscreenVS(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    output.Texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.Position = float4(output.Texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

Texture2D InputTexture : register(t0);
SamplerState LinearClamp : register(s0);

float X360GammaApprox(float x)
{
    float A = 0.541901f;
    float B = 1.13465f;
    float C = 13.53054f;
    float D = 6.56649f;
    float E = 0.311465f;
    x = max(0.0f, x);
    float f1 = A * x;
    float f2 = pow(x, B) * (1.0f - exp2(-C * x));
    float f3 = saturate(x * D + E);
    return lerp(f1, f2, f3);
}

float4 ConsoleGammaPS(VS_OUTPUT input) : SV_TARGET
{
    float3 color = InputTexture.Sample(LinearClamp, input.Texcoord).rgb;
    float3 outColor = float3(
        X360GammaApprox(color.r),
        X360GammaApprox(color.g),
        X360GammaApprox(color.b)
    );
    return float4(outColor, 1.0f);
}
)";

        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        ID3DBlob* errBlob = nullptr;

        hr = D3DCompile(shaderSource, strlen(shaderSource), "ConsoleGamma", nullptr, nullptr,
            "FullscreenVS", "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vsBlob, &errBlob);
        if (FAILED(hr))
        {
            if (errBlob) { OutputDebugStringA((char*)errBlob->GetBufferPointer()); errBlob->Release(); }
            return false;
        }

        hr = D3DCompile(shaderSource, strlen(shaderSource), "ConsoleGamma", nullptr, nullptr,
            "ConsoleGammaPS", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &psBlob, &errBlob);
        if (FAILED(hr))
        {
            if (errBlob) { OutputDebugStringA((char*)errBlob->GetBufferPointer()); errBlob->Release(); }
            SafeRelease(vsBlob);
            return false;
        }

        hr = s_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &s_vs);
        SafeRelease(vsBlob);
        if (FAILED(hr)) { SafeRelease(psBlob); return false; }

        hr = s_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &s_ps);
        SafeRelease(psBlob);
        if (FAILED(hr)) return false;

        // ---- Sampler (linear, clamp) ----
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        hr = s_device->CreateSamplerState(&sampDesc, &s_sampler);
        if (FAILED(hr)) return false;

        // ---- Blend state (no blending) ----
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = s_device->CreateBlendState(&blendDesc, &s_blendState);
        if (FAILED(hr)) return false;

        // ---- Depth-stencil state (depth disabled) ----
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = FALSE;
        dsDesc.StencilEnable = FALSE;
        hr = s_device->CreateDepthStencilState(&dsDesc, &s_dsState);
        if (FAILED(hr)) return false;

        // ---- Rasterizer state (no culling, no scissor) ----
        D3D11_RASTERIZER_DESC rsDesc = {};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_NONE;
        rsDesc.ScissorEnable = FALSE;
        rsDesc.DepthClipEnable = TRUE;
        hr = s_device->CreateRasterizerState(&rsDesc, &s_rsState);
        if (FAILED(hr)) return false;

        s_initialized = true;
        return true;
    }

    static void Render(IDXGISwapChain* swapChain)
    {
        if (!bConsoleGammaEnabled || !swapChain)
            return;

        if (!s_initialized && !Initialize(swapChain))
            return;

        ID3D11Texture2D* backBuffer = nullptr;
        HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        if (FAILED(hr)) return;

        D3D11_TEXTURE2D_DESC bbDesc;
        backBuffer->GetDesc(&bbDesc);

        // ---- (Re)create copy texture if resolution changed ----
        if (!s_copyTex || bbDesc.Width != s_width || bbDesc.Height != s_height)
        {
            s_width = bbDesc.Width;
            s_height = bbDesc.Height;

            SafeRelease(s_copySRV);
            SafeRelease(s_copyTex);

            D3D11_TEXTURE2D_DESC copyDesc = bbDesc;
            copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            copyDesc.MiscFlags = 0;
            hr = s_device->CreateTexture2D(&copyDesc, nullptr, &s_copyTex);
            if (FAILED(hr)) { SafeRelease(backBuffer); return; }

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = bbDesc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            hr = s_device->CreateShaderResourceView(s_copyTex, &srvDesc, &s_copySRV);
            if (FAILED(hr)) { SafeRelease(backBuffer); return; }
        }

        // ---- Create RTV fresh every frame from current back buffer ----
        ID3D11RenderTargetView* frameRTV = nullptr;
        hr = s_device->CreateRenderTargetView(backBuffer, nullptr, &frameRTV);
        if (FAILED(hr)) { SafeRelease(backBuffer); return; }

        // ---- Save pipeline state ----
        PipelineState old = {};
        SaveState(old);

        s_context->CopyResource(s_copyTex, backBuffer);
        SafeRelease(backBuffer);

        D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)s_width, (float)s_height, 0.0f, 1.0f };
        s_context->RSSetViewports(1, &vp);
        s_context->RSSetState(s_rsState);

        s_context->OMSetRenderTargets(1, &frameRTV, nullptr);
        s_context->OMSetBlendState(s_blendState, nullptr, 0xFFFFFFFF);
        s_context->OMSetDepthStencilState(s_dsState, 0);

        s_context->IASetInputLayout(nullptr);
        s_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        s_context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

        s_context->VSSetShader(s_vs, nullptr, 0);
        s_context->PSSetShader(s_ps, nullptr, 0);
        s_context->PSSetShaderResources(0, 1, &s_copySRV);
        s_context->PSSetSamplers(0, 1, &s_sampler);

        s_context->Draw(3, 0);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        s_context->PSSetShaderResources(0, 1, &nullSRV);

        SafeRelease(frameRTV);

        RestoreState(old);
    }

    static void OnResize()
    {
        SafeRelease(s_copySRV);
        SafeRelease(s_copyTex);
        s_width = 0;
        s_height = 0;
    }

private:
    static inline bool s_initialized = false;
    static inline UINT s_width = 0;
    static inline UINT s_height = 0;

    static inline ID3D11Device* s_device = nullptr;
    static inline ID3D11DeviceContext* s_context = nullptr;
    static inline ID3D11VertexShader* s_vs = nullptr;
    static inline ID3D11PixelShader* s_ps = nullptr;
    static inline ID3D11SamplerState* s_sampler = nullptr;
    static inline ID3D11BlendState* s_blendState = nullptr;
    static inline ID3D11DepthStencilState* s_dsState = nullptr;
    static inline ID3D11RasterizerState* s_rsState = nullptr;
    static inline ID3D11Texture2D* s_copyTex = nullptr;
    static inline ID3D11ShaderResourceView* s_copySRV = nullptr;
    static inline ID3D11RenderTargetView* s_rtv = nullptr;

    struct PipelineState
    {
        ID3D11RenderTargetView* rtv = nullptr;
        ID3D11DepthStencilView* dsv = nullptr;
        ID3D11BlendState* blendState = nullptr;
        FLOAT blendFactor[4] = {};
        UINT sampleMask = 0;
        ID3D11DepthStencilState* dsState = nullptr;
        UINT stencilRef = 0;
        ID3D11RasterizerState* rsState = nullptr;
        ID3D11VertexShader* vs = nullptr;
        ID3D11PixelShader* ps = nullptr;
        ID3D11InputLayout* inputLayout = nullptr;
        D3D11_PRIMITIVE_TOPOLOGY topology = {};
        ID3D11ShaderResourceView* psrv = nullptr;
        ID3D11SamplerState* psampler = nullptr;
        UINT numViewports = 0;
        D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    };

    static void SaveState(PipelineState& st)
    {
        s_context->OMGetRenderTargets(1, &st.rtv, &st.dsv);
        s_context->OMGetBlendState(&st.blendState, st.blendFactor, &st.sampleMask);
        s_context->OMGetDepthStencilState(&st.dsState, &st.stencilRef);
        s_context->RSGetState(&st.rsState);
        s_context->VSGetShader(&st.vs, nullptr, nullptr);
        s_context->PSGetShader(&st.ps, nullptr, nullptr);
        s_context->IAGetInputLayout(&st.inputLayout);
        s_context->IAGetPrimitiveTopology(&st.topology);
        s_context->PSGetShaderResources(0, 1, &st.psrv);
        s_context->PSGetSamplers(0, 1, &st.psampler);
        st.numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        s_context->RSGetViewports(&st.numViewports, st.viewports);
    }

    static void RestoreState(PipelineState& st)
    {
        s_context->OMSetRenderTargets(1, &st.rtv, st.dsv);
        s_context->OMSetBlendState(st.blendState, st.blendFactor, st.sampleMask);
        s_context->OMSetDepthStencilState(st.dsState, st.stencilRef);
        s_context->RSSetState(st.rsState);
        s_context->VSSetShader(st.vs, nullptr, 0);
        s_context->PSSetShader(st.ps, nullptr, 0);
        s_context->IASetInputLayout(st.inputLayout);
        s_context->IASetPrimitiveTopology(st.topology);
        s_context->PSSetShaderResources(0, 1, &st.psrv);
        s_context->PSSetSamplers(0, 1, &st.psampler);
        s_context->RSSetViewports(st.numViewports, st.viewports);

        // The OMGet/RSGet/etc. calls AddRef, so release after restoring
        SafeRelease(st.rtv);
        SafeRelease(st.dsv);
        SafeRelease(st.blendState);
        SafeRelease(st.dsState);
        SafeRelease(st.rsState);
        SafeRelease(st.vs);
        SafeRelease(st.ps);
        SafeRelease(st.inputLayout);
        SafeRelease(st.psrv);
        SafeRelease(st.psampler);
    }
};

class ConsoleGamma
{
public:
    ConsoleGamma()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            bConsoleGammaEnabled = iniReader.ReadInteger("GRAPHICS", "ConsoleGamma", 1) != 0;

            if (!bConsoleGammaEnabled)
                return;

            auto pattern = hook::pattern("57 ? ? 52 51 50 8B 47");
            static auto DX11PresentHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                IDXGISwapChain* pSwapChain = (IDXGISwapChain*)regs.eax;
                ConsoleGammaDX11::Render(pSwapChain);
            });
        };
    }
} ConsoleGamma;