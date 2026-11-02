module;

#include <stdafx.h>
#include "minidx8/d3d8.h"

export module DepthStencil;

import ComVars;

// The game's device (pD3DDevice) as the Direct3D 8 interface
IDirect3DDevice8* Device()
{
    return reinterpret_cast<IDirect3DDevice8*>(pD3DDevice.get());
}

void InitDepthStencil()
{
    auto pattern = hook::pattern("8B 6D ? 85 ED 66 89 5E");
    static auto loc_64079F = (uintptr_t)pattern.get_first();

    pattern = hook::pattern("8B 15 ? ? ? ? 8B 3A 8B 5C 24");
    static auto CreateDepthStencilSurfaceHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UINT& Width = *(UINT*)(regs.esp + 0x38);
        UINT& Height = *(UINT*)(regs.esp + 0x3C);
        D3DFORMAT& Format = *(D3DFORMAT*)(regs.esp + 0x10);
        D3DMULTISAMPLE_TYPE MultiSample = (D3DMULTISAMPLE_TYPE)(regs.eax);

        // Get render target size to match depth texture size
        IDirect3DSurface8* rt = nullptr;
        if (FAILED(Device()->GetRenderTarget(&rt)) || !rt)
            return;
        D3DSURFACE_DESC rtDesc{};
        const HRESULT result = rt->GetDesc(&rtDesc);
        rt->Release();
        if (FAILED(result))
            return;

        // Override Width/Height with render target size
        Width = rtDesc.Width;
        Height = rtDesc.Height;

        //regs.eax = Device()->CreateDepthStencilSurface(Width, Height, Format, MultiSample, (IDirect3DSurface8**)(regs.ecx));

        IDirect3D8* pD3D8 = nullptr;
        if (FAILED(Device()->GetDirect3D(&pD3D8)) || !pD3D8)
            return;

        if (FAILED(pD3D8->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z'))))
        {
            pD3D8->Release();
            return;
        }

        IDirect3DTexture8* texture = nullptr;
        if (FAILED(Device()->CreateTexture(Width, Height, 1, D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z'), D3DPOOL_DEFAULT, &texture)))
        {
            pD3D8->Release();
            return;
        }

        if (FAILED(texture->GetSurfaceLevel(0, (IDirect3DSurface8**)(regs.ecx))))
        {
            texture->Release();
            pD3D8->Release();
            return;
        }

        texture->Release();
        pD3D8->Release();

        return_to(loc_64079F);
    });
}

class DepthStencil
{
public:
    DepthStencil()
    {
        WFP::onInitEvent() += []()
        {
            InitDepthStencil();
        };
    }
} DepthStencil;
