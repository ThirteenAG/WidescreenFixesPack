module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

export module Shadows;

import ComVars;
import Resolution;

// The dynamic shadows' map. The game renders the shadow casters' depth into a 1024x1024 depth texture (with a colour
// render target of the same size). The NVIDIA path samples it with the hardware's depth compare (sharp edges), the
// ATI path reads the depth itself (DF16) and filters it (soft edges). The game picks the path at startup by the GPU's
// vendor and gives other vendors (Intel and the rest) no dynamic shadows. The path is picked here by the setting
// instead, on every GPU that has the depth texture format: D24S8 for sharp shadows, INTZ for soft ones.
namespace ShadowMap
{
    enum Path : uint32_t
    {
        PathNone = 0,
        PathHardwareCompare = 1,
        PathDepthTexture = 2,
    };

    GameRef<uint32_t> CurrentPath;
    GameRef<IDirect3DDevice9*> Device;
    GameRef<IDirect3DTexture9*> ColorTexture;
    GameRef<IDirect3DSurface9*> ColorSurface;
    GameRef<IDirect3DTexture9*> DepthTexture;
    GameRef<IDirect3DSurface9*> DepthSurface;

    // The size functions' "mov eax, 1024", for the viewport and the texel offsets
    ProtectedGameRef<uint32_t> MapWidth;
    ProtectedGameRef<uint32_t> MapHeight;

    uint32_t Size = 2048;
    bool bSoft = false;

    // The game's Direct3D object and adapter, for the format check
    GameRef<IDirect3D9*> Direct3D;
    GameRef<uint32_t> Adapter;

    constexpr D3DFORMAT INTZ = static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z'));

    bool DepthTextureSupported(D3DFORMAT format)
    {
        IDirect3D9* d3d = Direct3D;
        return d3d && SUCCEEDED(d3d->CheckDeviceFormat(Adapter, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, format));
    }

    // Soft shadows fall back to sharp ones when INTZ isn't there
    void PickPath()
    {
        if (bSoft && DepthTextureSupported(INTZ))
            CurrentPath = PathDepthTexture;
        else if (DepthTextureSupported(D3DFMT_D24S8))
            CurrentPath = PathHardwareCompare;
        else
            CurrentPath = PathNone;
    }

    // The height has the chosen size; the width grows with the screen's aspect ratio, as the shadows cover the
    // width of the view
    void UpdateSize()
    {
        float widthScale = std::max(1.0f, GetAspectRatio() / (4.0f / 3.0f));
        MapWidth = std::min(static_cast<uint32_t>(Size * widthScale), 16384u);
        MapHeight = Size;
    }

    // Replaces the game's creation of the shadow map textures, with the size and the depth format of the settings
    HRESULT __cdecl Create()
    {
        UpdateSize();
        IDirect3DDevice9* device = Device;
        if (FAILED(device->CreateTexture(MapWidth, MapHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &ColorTexture.get(), nullptr)))
            return E_FAIL;

        D3DFORMAT depthFormat = D3DFMT_UNKNOWN;
        if (CurrentPath == PathHardwareCompare)
            depthFormat = D3DFMT_D24S8;
        else if (CurrentPath == PathDepthTexture)
            depthFormat = INTZ;
        if (depthFormat != D3DFMT_UNKNOWN && FAILED(device->CreateTexture(MapWidth, MapHeight, 1, D3DUSAGE_DEPTHSTENCIL, depthFormat, D3DPOOL_DEFAULT, &DepthTexture.get(), nullptr)))
            return E_FAIL;

        if (FAILED(ColorTexture->GetSurfaceLevel(0, &ColorSurface.get())) || !DepthTexture || FAILED(DepthTexture->GetSurfaceLevel(0, &DepthSurface.get())) || !DepthSurface)
            return E_FAIL;
        return S_OK;
    }
}

class Shadows
{
public:
    Shadows()
    {
        WFP::onInitEvent() += []()
        {
            ShadowMap::Size = std::clamp(FixSettings().Int("GRAPHICS.ShadowResolution"), 1024, 16384);
            ShadowMap::bSoft = FixSettings().Bool("GRAPHICS.SoftShadows");

            {
                //dynamic shadow fix that stops them from disappearing when going into tunnels, under bridges, etc.
                uint32_t* dword_6DE377 = hook::pattern("75 3B C7 05 ? ? ? ? 00 00 80 3F").count(1).get(0).get<uint32_t>(0);
                injector::MakeNOP(dword_6DE377, 2, true);
                injector::WriteMemory((uint32_t)dword_6DE377 + 8, 0, true);

                //car shadow opacity
                uint32_t* dword_8A0E50 = *hook::pattern("D9 05 ? ? ? ? 8B 54 24 70 D9 1A E9 D1").count(1).get(0).get<uint32_t*>(2);
                injector::WriteMemory(dword_8A0E50, 60.0f, true);

            }

            // The shadow map textures
            {
                auto create = hook::pattern("A1 ? ? ? ? 8B 08 6A 00 68 ? ? ? ? 6A 00 6A 15 6A 01 6A 01 68");
                auto path = hook::pattern("85 C0 7C ? A1 ? ? ? ? 83 F8 01 75 ? A1");
                auto surfaces = hook::pattern("A1 ? ? ? ? 8B 10 68 ? ? ? ? 6A 00 50 FF 52 48 85 C0 7C ? A1 ? ? ? ? 8B 08 68 ? ? ? ? 6A 00 50 FF 51 48");
                ShadowMap::Device.SetAddress(*create.get_first<IDirect3DDevice9**>(1));
                ShadowMap::ColorTexture.SetAddress(*create.get_first<IDirect3DTexture9**>(10));
                ShadowMap::CurrentPath.SetAddress(*path.get_first<uint32_t*>(5));
                ShadowMap::ColorSurface.SetAddress(*surfaces.get_first<IDirect3DSurface9**>(8));
                ShadowMap::DepthTexture.SetAddress(*surfaces.get_first<IDirect3DTexture9**>(23));
                ShadowMap::DepthSurface.SetAddress(*surfaces.get_first<IDirect3DSurface9**>(30));
                static auto CreateHook = safetyhook::create_inline(create.get_first(), ShadowMap::Create);

                auto sizes = hook::pattern("E8 ? ? ? ? 89 44 24 08 DB 44 24 08 D8 3D ? ? ? ? D8 05 ? ? ? ? D9 5C 24 0C E8 ? ? ? ? 89 44 24 08");
                ShadowMap::MapWidth.SetAddress(reinterpret_cast<uint32_t*>(injector::GetBranchDestination(sizes.get_first(0)).as_int() + 1));
                ShadowMap::MapHeight.SetAddress(reinterpret_cast<uint32_t*>(injector::GetBranchDestination(sizes.get_first(29)).as_int() + 1));
                ShadowMap::UpdateSize();

                // The path, right after the game's own vendor-based pick at startup (the adapter's formats are
                // checked there), before the video options read it
                auto formatCheck = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 8B 08 6A 4B 6A 03 6A 02 6A 16");
                ShadowMap::Adapter.SetAddress(*formatCheck.get_first<uint32_t*>(2));
                ShadowMap::Direct3D.SetAddress(*formatCheck.get_first<IDirect3D9**>(7));
                auto picked = hook::pattern("83 C4 10 E8 ? ? ? ? 39 1D ? ? ? ? 75 05 E8");
                static auto PathHook = safetyhook::create_mid(picked.get_first(8), [](SafetyHookContext&) { ShadowMap::PickPath(); });

                // The slope-scaled depth bias is in depth per shadow map texel, and a texel shrinks with the map's
                // size; it's scaled so that it stays the same in the world as with the original map
                static GameRef<float> SlopeScaleDepthBias;
                SlopeScaleDepthBias.SetAddress(*hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 8B 08 52 68 AF 00 00 00").get_first<float*>(2));
                SlopeScaleDepthBias = SlopeScaleDepthBias * (static_cast<float>(ShadowMap::Size) / 1024.0f);
            }

            {
                uint32_t* dword_6E5174 = hook::pattern("68 ? ? ? ? EB ? A1 ? ? ? ? 0D").count(1).get(0).get<uint32_t>(1);
                injector::WriteMemory(dword_6E5174, 0x00000000, true);
                uint32_t* dword_6BFFA2 = hook::pattern("68 ? ? ? ? 50 41 68").count(2).get(1).get<uint32_t>(1);
                injector::WriteMemory(dword_6BFFA2, 0x00006102, true);
            }
        };
    }
} Shadows;
