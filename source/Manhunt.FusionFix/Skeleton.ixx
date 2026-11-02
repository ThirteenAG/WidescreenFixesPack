module;

#include <stdafx.h>
#include <cstdint>
#include "common.h"

export module Skeleton;

export import ComVars;

// RwGlobals: the current camera and the device's render state and immediate mode 2D functions
export namespace RwEngine
{
    constexpr size_t CurCamera = 0x00;
    constexpr size_t RenderStateSet = 0x20;
    constexpr size_t RenderStateGet = 0x24;
    constexpr size_t Im2DRenderLine = 0x28;
    constexpr size_t Im2DRenderTriangle = 0x2C;
    constexpr size_t Im2DRenderPrimitive = 0x30;
    constexpr size_t Im2DRenderIndexedPrimitive = 0x34;

    constexpr int32_t rwRENDERSTATETEXTURERASTER = 1;
    constexpr int32_t rwVIDEOMODEEXCLUSIVE = 1;


    template<typename T>
    T& Field(size_t offset)
    {
        return *reinterpret_cast<T*>(Instance.get() + offset);
    }
}

export struct RwVideoMode
{
    int32_t width;
    int32_t height;
    int32_t depth;
    int32_t flags;
    int32_t refRate;
    int32_t format;
};

export inline int32_t RwRasterGetWidth(void* raster) { return *reinterpret_cast<int32_t*>(static_cast<uint8_t*>(raster) + 0x0C); }
export inline int32_t RwRasterGetHeight(void* raster) { return *reinterpret_cast<int32_t*>(static_cast<uint8_t*>(raster) + 0x10); }
export inline void* RwCameraGetRaster(void* camera) { return *reinterpret_cast<void**>(static_cast<uint8_t*>(camera) + 0x60); }

// The raster the engine renders to now, or nullptr before the engine runs
export void* GetCurrentRaster()
{
    if (!RwEngine::Instance.get())
        return nullptr;
    auto camera = RwEngine::Field<void*>(RwEngine::CurCamera);
    return camera ? RwCameraGetRaster(camera) : nullptr;
}

// The game's resolution and its aspect ratio (0 before a video mode is set)
export float GetScreenAspectRatio()
{
    auto rs = RsGlobal.get_ptr();
    if (rs->width <= 0 || rs->height <= 0)
        return 0.0f;
    return float(rs->width) / float(rs->height);
}

export void InitSkeletonRequiredRefs()
{
    RsGlobal.SetAddress(*hook::pattern("C7 05 ? ? ? ? ? ? ? ? 83 3D ? ? ? ? 00 75 0F").get_first<RsGlobalType*>(6));
    RwEngine::Instance.SetAddress(*hook::pattern("8B 0D ? ? ? ? 8B 19 8B 53 60").get_first<uint8_t**>(2));
}
