#pragma once
#include <minidx8/d3d8.h>
#include "../Shared/TonyHawk/Video.hpp"

namespace THPS4Video
{
    inline SafetyHookInline DrawHook;
    inline const float* FrameWidth = nullptr;
    inline const float* FrameHeight = nullptr;
    inline const float* MovieWidth = nullptr;
    inline const float* MovieHeight = nullptr;
    inline const int32_t* Mode = nullptr;

    inline void __cdecl Draw(IDirect3DTexture8* texture, float x, float y, float width, float height, float alpha)
    {
        if (!texture || !FrameWidth || !FrameHeight || !MovieWidth || !MovieHeight ||
            !std::isfinite(*FrameWidth) || !std::isfinite(*FrameHeight) || !std::isfinite(*MovieWidth) || !std::isfinite(*MovieHeight) ||
            *FrameWidth <= 0.0f || *FrameHeight <= 0.0f || *MovieWidth <= 0.0f || *MovieHeight <= 0.0f)
        {
            DrawHook.ccall<void>(texture, x, y, width, height, alpha);
            return;
        }
        float targetWidth = *FrameWidth, targetHeight = *FrameHeight;
        IDirect3DDevice8* device = nullptr;
        D3DVIEWPORT8 previous{};
        bool restore = false;
        if (SUCCEEDED(texture->GetDevice(&device)))
        {
            IDirect3DSurface8* target = nullptr;
            if (SUCCEEDED(device->GetRenderTarget(&target)))
            {
                D3DSURFACE_DESC description{};
                if (SUCCEEDED(target->GetDesc(&description)))
                {
                    targetWidth = static_cast<float>(description.Width);
                    targetHeight = static_cast<float>(description.Height);
                }
                target->Release();
            }
            if (SUCCEEDED(device->GetViewport(&previous)))
            {
                D3DVIEWPORT8 viewport{ 0, 0, static_cast<DWORD>(targetWidth), static_cast<DWORD>(targetHeight), 0.0f, 1.0f };
                restore = SUCCEEDED(device->SetViewport(&viewport));
                if (restore)
                    device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
            }
        }
        const auto box = TonyVideo::Layout(targetWidth, targetHeight, *MovieWidth, *MovieHeight, Mode ? std::clamp(*Mode, 0, 2) : 1);
        DrawHook.ccall<void>(texture, box.x, box.y, box.width / *FrameWidth, box.height / *FrameHeight, alpha);
        if (device)
        {
            if (restore)
                device->SetViewport(&previous);
            device->Release();
        }
    }

    inline void Install(const int32_t* mode, HMODULE module = GetModuleHandleW(nullptr))
    {
        Mode = mode;
        auto draw = hook::module_pattern(module, "83 EC 70 56 8B 74 24 78 85 F6 0F 84 ? ? ? ? 57 68 01 00 01 00 6A 04 E8");
        auto dimensions = hook::module_pattern(module, "D9 1D ? ? ? ? DF 6C 24 ? 51 6A 01 6A 16 53 D9 1D ? ? ? ? DB 05 ? ? ? ? 6A 01 56 57 50 D9 1D ? ? ? ? DB 05 ? ? ? ? D9 1D");
        if (draw.size() != 1 || dimensions.size() != 1)
        {
            OutputDebugStringA("THPS4: fullscreen movie patterns unavailable.\n");
            return;
        }
        MovieWidth = *dimensions.get_first<const float*>(2);
        MovieHeight = *dimensions.get_first<const float*>(18);
        FrameWidth = *dimensions.get_first<const float*>(35);
        FrameHeight = *dimensions.get_first<const float*>(47);
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        if (!DrawHook)
            OutputDebugStringA("THPS4: fullscreen movie hook unavailable.\n");
    }
}
