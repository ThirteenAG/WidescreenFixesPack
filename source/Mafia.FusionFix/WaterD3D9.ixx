module;

#include <stdafx.h>
#include <d3d9.h>

export module WaterD3D9;

import WaterCommon;

// The Direct3D 9 side of the water, used when the game runs through d3d8to9: the reflection target and the water
// drawn with shader model 2 shaders straight on the Direct3D 9 device
namespace WaterD3D9
{
    IDirect3DTexture9* reflection = nullptr;
    IDirect3DSurface9* reflectionSurface = nullptr;
    IDirect3DSurface9* reflectionDepth = nullptr;
    IDirect3DSurface9* savedTarget = nullptr;
    IDirect3DSurface9* savedDepth = nullptr;
    IDirect3DTexture9* waves = nullptr;
    IDirect3DVertexShader9* vertexShader = nullptr;
    IDirect3DPixelShader9* pixelShader = nullptr;
    std::vector<std::pair<WaterLayout, IDirect3DVertexDeclaration9*>> declarations;
    int supported = -1;

    std::span<const DWORD> Resource(int id)
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Resource), &module);
        auto info = FindResourceW(module, MAKEINTRESOURCEW(id), RT_RCDATA);
        auto data = static_cast<const DWORD*>(LockResource(LoadResource(module, info)));
        return { data, SizeofResource(module, info) / sizeof(DWORD) };
    }

    // The Direct3D 9 device behind the game's Direct3D 8 one, when d3d8to9 is in use
    export void* Device(IUnknown* device8)
    {
        IDirect3DDevice9* device = nullptr;
        if (!device8 || FAILED(device8->QueryInterface(__uuidof(IDirect3DDevice9), reinterpret_cast<void**>(&device))) || !device)
            return nullptr;
        device->Release();
        return device;
    }

    export bool Supported(void* device)
    {
        if (supported < 0)
        {
            D3DCAPS9 caps{};
            static_cast<IDirect3DDevice9*>(device)->GetDeviceCaps(&caps);
            supported = caps.PixelShaderVersion >= D3DPS_VERSION(2, 0) && caps.VertexShaderVersion >= D3DVS_VERSION(2, 0);
        }
        return supported > 0;
    }

    // The device is gone with everything made on it
    export void Forget()
    {
        reflection = nullptr;
        reflectionSurface = reflectionDepth = savedTarget = savedDepth = nullptr;
        waves = nullptr;
        vertexShader = nullptr;
        pixelShader = nullptr;
        declarations.clear();
        supported = -1;
    }

    export void Release()
    {
        for (auto** surface : { &reflectionSurface, &reflectionDepth })
        {
            if (*surface)
                (*surface)->Release();
            *surface = nullptr;
        }
        if (reflection)
            reflection->Release();
        reflection = nullptr;
    }

    bool Target(IDirect3DDevice9* device, UINT width, UINT height)
    {
        if (reflection)
        {
            D3DSURFACE_DESC description{};
            reflectionSurface->GetDesc(&description);
            if (description.Width == width && description.Height == height)
                return true;
            Release();
        }
        if (FAILED(device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &reflection, nullptr)))
        {
            reflection = nullptr;
            return false;
        }
        reflection->GetSurfaceLevel(0, &reflectionSurface);
        if (FAILED(device->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &reflectionDepth, nullptr)))
        {
            reflectionDepth = nullptr;
            Release();
            return false;
        }
        return true;
    }

    export bool BeginReflection(void* device, UINT width, UINT height, DWORD clear)
    {
        auto d = static_cast<IDirect3DDevice9*>(device);
        if (!Target(d, width, height))
            return false;
        d->GetRenderTarget(0, &savedTarget);
        d->GetDepthStencilSurface(&savedDepth);
        d->SetRenderTarget(0, reflectionSurface);
        d->SetDepthStencilSurface(reflectionDepth);
        d->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, clear, 1.0f, 0);
        return true;
    }

    // The Direct3D 8 viewport has the same layout as the Direct3D 9 one
    export void EndReflection(void* device, const void* viewport)
    {
        auto d = static_cast<IDirect3DDevice9*>(device);
        d->SetRenderTarget(0, savedTarget);
        d->SetDepthStencilSurface(savedDepth);
        d->SetViewport(static_cast<const D3DVIEWPORT9*>(viewport));
        for (auto** surface : { &savedTarget, &savedDepth })
        {
            if (*surface)
                (*surface)->Release();
            *surface = nullptr;
        }
    }

    bool Waves(IDirect3DDevice9* device)
    {
        if (waves)
            return true;
        if (FAILED(device->CreateTexture(WaveNormalSize, WaveNormalSize, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &waves, nullptr)))
        {
            waves = nullptr;
            return false;
        }
        for (UINT level = 0; level < waves->GetLevelCount(); ++level)
        {
            const auto texels = WaveNormals(WaveNormalSize, level);
            const UINT size = std::max(1u, WaveNormalSize >> level);
            D3DLOCKED_RECT locked{};
            if (FAILED(waves->LockRect(level, &locked, nullptr, 0)))
                continue;
            for (UINT y = 0; y < size; ++y)
                memcpy(static_cast<BYTE*>(locked.pBits) + y * locked.Pitch, &texels[y * size], size * sizeof(uint32_t));
            waves->UnlockRect(level);
        }
        return true;
    }

    IDirect3DVertexDeclaration9* Declaration(IDirect3DDevice9* device, const WaterLayout& layout)
    {
        for (const auto& [key, declaration] : declarations)
            if (key == layout)
                return declaration;
        const D3DVERTEXELEMENT9 elements[] =
        {
            { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
            { 0, static_cast<WORD>(layout.color), D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
            { 0, static_cast<WORD>(layout.uv), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END()
        };
        IDirect3DVertexDeclaration9* declaration = nullptr;
        if (FAILED(device->CreateVertexDeclaration(elements, &declaration)))
            declaration = nullptr;
        declarations.emplace_back(layout, declaration);
        return declaration;
    }

    export bool Draw(void* device, const WaterDraw& water, const WaterLayout& layout, const std::function<HRESULT()>& draw)
    {
        auto d = static_cast<IDirect3DDevice9*>(device);
        if (!reflection || !Waves(d))
            return false;
        if (!vertexShader && FAILED(d->CreateVertexShader(Resource(IDR_WATER_VS9).data(), &vertexShader)))
            vertexShader = nullptr;
        if (!pixelShader && FAILED(d->CreatePixelShader(Resource(IDR_WATER_PS9).data(), &pixelShader)))
            pixelShader = nullptr;
        auto declaration = Declaration(d, layout);
        if (!vertexShader || !pixelShader || !declaration)
            return false;

        constexpr D3DSAMPLERSTATETYPE states[] = { D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV, D3DSAMP_MAGFILTER, D3DSAMP_MINFILTER, D3DSAMP_MIPFILTER };
        std::array<std::array<DWORD, std::size(states)>, 4> savedStates{};
        std::array<IDirect3DBaseTexture9*, 4> savedTextures{};
        for (DWORD stage = 0; stage < 4; ++stage)
        {
            d->GetTexture(stage, &savedTextures[stage]);
            for (size_t i = 0; i < std::size(states); ++i)
                d->GetSamplerState(stage, states[i], &savedStates[stage][i]);
        }
        IDirect3DVertexShader9* savedVertexShader = nullptr;
        IDirect3DPixelShader9* savedPixelShader = nullptr;
        IDirect3DVertexDeclaration9* savedDeclaration = nullptr;
        DWORD savedFVF = 0;
        d->GetVertexShader(&savedVertexShader);
        d->GetPixelShader(&savedPixelShader);
        d->GetVertexDeclaration(&savedDeclaration);
        d->GetFVF(&savedFVF);

        d->SetTexture(3, savedTextures[0]);
        d->SetTexture(0, reflection);
        d->SetTexture(1, waves);
        for (DWORD stage = 0; stage < 4; ++stage)
        {
            const bool wrap = stage != 0;
            d->SetSamplerState(stage, D3DSAMP_ADDRESSU, wrap ? (stage == 3 ? savedStates[0][0] : D3DTADDRESS_WRAP) : D3DTADDRESS_CLAMP);
            d->SetSamplerState(stage, D3DSAMP_ADDRESSV, wrap ? (stage == 3 ? savedStates[0][1] : D3DTADDRESS_WRAP) : D3DTADDRESS_CLAMP);
            d->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            d->SetSamplerState(stage, D3DSAMP_MINFILTER, stage == 1 ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR);
            d->SetSamplerState(stage, D3DSAMP_MIPFILTER, stage == 0 ? D3DTEXF_NONE : D3DTEXF_LINEAR);
        }
        DWORD savedAnisotropy = 1;
        d->GetSamplerState(1, D3DSAMP_MAXANISOTROPY, &savedAnisotropy);
        d->SetSamplerState(1, D3DSAMP_MAXANISOTROPY, 8);

        // vs: c0-c3 clip, c4-c6 world, c8-c11 reflection, c12-c17 waves, c18 fog
        d->SetVertexShaderConstantF(0, &water.worldViewProj[0][0], 4);
        d->SetVertexShaderConstantF(4, &water.world[0][0], 3);
        d->SetVertexShaderConstantF(8, &water.reflection[0][0], 4);
        d->SetVertexShaderConstantF(12, &water.waves[0][0], 6);
        d->SetVertexShaderConstantF(18, water.fog, 1);
        // ps: c0 camera, c1 sun, c2 sun color, c3 tint, c4 parameters
        d->SetPixelShaderConstantF(0, water.camera, 1);
        d->SetPixelShaderConstantF(1, water.sun, 1);
        d->SetPixelShaderConstantF(2, water.sunColor, 1);
        d->SetPixelShaderConstantF(3, water.tint, 1);
        d->SetPixelShaderConstantF(4, water.params, 1);

        d->SetVertexDeclaration(declaration);
        d->SetVertexShader(vertexShader);
        d->SetPixelShader(pixelShader);
        const HRESULT result = draw();

        d->SetVertexShader(savedVertexShader);
        d->SetPixelShader(savedPixelShader);
        if (savedFVF)
            d->SetFVF(savedFVF);
        else
            d->SetVertexDeclaration(savedDeclaration);
        for (auto* object : std::initializer_list<IUnknown*>{ savedVertexShader, savedPixelShader, savedDeclaration })
            if (object)
                object->Release();
        d->SetSamplerState(1, D3DSAMP_MAXANISOTROPY, savedAnisotropy);
        for (DWORD stage = 0; stage < 4; ++stage)
        {
            d->SetTexture(stage, savedTextures[stage]);
            if (savedTextures[stage])
                savedTextures[stage]->Release();
            for (size_t i = 0; i < std::size(states); ++i)
                d->SetSamplerState(stage, states[i], savedStates[stage][i]);
        }
        return SUCCEEDED(result);
    }
}
