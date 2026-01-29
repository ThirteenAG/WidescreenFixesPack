module;

#include <stdafx.h>
#include "dxsdk\dx8\d3d8.h"

export module DepthStencil;

import ComVars;

IDirect3DDevice8** pDevice = nullptr;

export void InitDepthStencil()
{
    auto pattern = hook::pattern("A1 ? ? ? ? 8B 10 8D 4C 24 ? 51 50");
    pDevice = *pattern.get_first<IDirect3DDevice8**>(1);

    pattern = hook::pattern("8B 6D ? 85 ED 66 89 5E");
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
        (*pDevice)->GetRenderTarget(&rt);
        D3DSURFACE_DESC rtDesc;
        rt->GetDesc(&rtDesc);
        rt->Release();

        // Override Width/Height with render target size
        Width = rtDesc.Width;
        Height = rtDesc.Height;

        //regs.eax = (*pDevice)->CreateDepthStencilSurface(Width, Height, Format, MultiSample, (IDirect3DSurface8**)(regs.ecx));

        IDirect3D8* pD3D8 = nullptr;
        (*pDevice)->GetDirect3D(&pD3D8);

        if (FAILED(pD3D8->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z'))))
        {
            pD3D8->Release();
            return;
        }

        IDirect3DTexture8* texture = nullptr;
        if (FAILED((*pDevice)->CreateTexture(Width, Height, 1, D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z'), D3DPOOL_DEFAULT, &texture)))
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