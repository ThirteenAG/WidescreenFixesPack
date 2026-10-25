#pragma once
#include <cstdint>
#include <utility>

// The scene camera's depth-stencil as an INTZ texture, for temporal anti-aliasing (AntiAliasing.hpp) in the Direct3D 8
// games, III and Vice City. Include it in a file of its own with the Direct3D 8 headers (minidx8/d3d8.h): they can't
// be in one file with Direct3D 9's. The texture is made through the game's Direct3D 8 device, so that d3d8to9 knows it
// and RenderWare can set it as a Direct3D 8 surface; the Direct3D 9 side finds it as the device's depth-stencil.
//
// The camera's Z raster holds the device's depth-stencil at the start of its RenderWare extension and sets it with the
// render target in every _rwD3D8CameraBeginUpdate. It gets the INTZ surface with a reference of its own (RenderWare
// releases what a raster holds unless it's the device's own depth-stencil), and its original back before a reset.
namespace RWGTA::Depth8
{
    namespace detail
    {
        inline IDirect3DTexture8* IntzTexture = nullptr;
        inline IDirect3DSurface8* IntzSurface = nullptr;
        inline IDirect3DSurface8* OriginalDepth = nullptr;
        inline void* ReplacedRaster = nullptr;
        inline UINT IntzWidth = 0, IntzHeight = 0;

        inline void ReleaseIntz()
        {
            if (IntzSurface)
                IntzSurface->Release();
            if (IntzTexture)
                IntzTexture->Release();
            IntzSurface = nullptr;
            IntzTexture = nullptr;
            IntzWidth = IntzHeight = 0;
        }

        // RwCamera::zBuffer
        inline void* ZRaster(void* camera)
        {
            return camera ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(camera) + 0x64) : nullptr;
        }

        inline IDirect3DSurface8*& RasterSurface(void* raster, int32_t extOffset)
        {
            return *reinterpret_cast<IDirect3DSurface8**>(reinterpret_cast<uint8_t*>(raster) + extOffset);
        }
    }

    // Each frame from the scene camera's begin update: gives its Z raster the INTZ surface. False when it can't be
    // (MSAA, no INTZ, no Z raster). created: the INTZ texture was made now.
    inline bool Replace(void* device8, void* camera, int32_t extOffset, bool& created)
    {
        using namespace detail;
        created = false;
        auto device = static_cast<IDirect3DDevice8*>(device8);
        auto raster = ZRaster(camera);
        if (!device || !raster)
            return false;
        auto& surface = RasterSurface(raster, extOffset);
        if (!surface)
            return false;
        if (surface == IntzSurface && raster == ReplacedRaster)
            return true;

        D3DSURFACE_DESC desc{};
        if (FAILED(surface->GetDesc(&desc)) || desc.MultiSampleType != D3DMULTISAMPLE_NONE)
            return false;
        if (!IntzSurface || IntzWidth != desc.Width || IntzHeight != desc.Height)
        {
            ReleaseIntz();
            if (FAILED(device->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z'), D3DPOOL_DEFAULT, &IntzTexture)) ||
                !IntzTexture || FAILED(IntzTexture->GetSurfaceLevel(0, &IntzSurface)) || !IntzSurface)
            {
                ReleaseIntz();
                return false;
            }
            IntzWidth = desc.Width;
            IntzHeight = desc.Height;
            created = true;
        }
        OriginalDepth = surface;
        ReplacedRaster = raster;
        IntzSurface->AddRef();
        surface = IntzSurface;
        return true;
    }

    // Before a reset: the raster gets its depth-stencil back, the device too; the INTZ texture is released
    inline void Restore(void* device8, void* camera, int32_t extOffset)
    {
        using namespace detail;
        auto device = static_cast<IDirect3DDevice8*>(device8);
        if (ReplacedRaster && ZRaster(camera) == ReplacedRaster && RasterSurface(ReplacedRaster, extOffset) == IntzSurface)
        {
            RasterSurface(ReplacedRaster, extOffset) = OriginalDepth;
            if (device)
                device->SetRenderTarget(nullptr, OriginalDepth);
            IntzSurface->Release();
        }
        ReplacedRaster = nullptr;
        OriginalDepth = nullptr;
        ReleaseIntz();
    }

    inline std::pair<uint32_t, uint32_t> Size()
    {
        return { detail::IntzWidth, detail::IntzHeight };
    }
}
