#pragma once
#include <d3d9.h>
#include "Video.hpp"

namespace TonyBink9
{
    inline SafetyHookInline DrawHook;
    inline const float* FrameWidth = nullptr;
    inline const float* FrameHeight = nullptr;
    inline const float* MovieWidth = nullptr;
    inline const float* MovieHeight = nullptr;
    inline const int32_t* Mode = nullptr;

    inline int __cdecl Draw(IDirect3DTexture9* texture, float x, float y, float width, float height, float alpha)
    {
        if (!texture || !FrameWidth || !FrameHeight || !MovieWidth || !MovieHeight ||
            !std::isfinite(*FrameWidth) || !std::isfinite(*FrameHeight) || !std::isfinite(*MovieWidth) || !std::isfinite(*MovieHeight) ||
            *FrameWidth <= 0.0f || *FrameHeight <= 0.0f || *MovieWidth <= 0.0f || *MovieHeight <= 0.0f)
        {
            return DrawHook.ccall<int>(texture, x, y, width, height, alpha);
        }
        float targetWidth = *FrameWidth, targetHeight = *FrameHeight;
        IDirect3DDevice9* device = nullptr;
        D3DVIEWPORT9 previous{};
        bool restore = false;
        if (SUCCEEDED(texture->GetDevice(&device)))
        {
            IDirect3DSurface9* target = nullptr;
            if (SUCCEEDED(device->GetRenderTarget(0, &target)))
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
                D3DVIEWPORT9 viewport{ 0, 0, static_cast<DWORD>(targetWidth), static_cast<DWORD>(targetHeight), 0.0f, 1.0f };
                restore = SUCCEEDED(device->SetViewport(&viewport));
                if (restore)
                    device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
            }
        }
        const auto box = TonyVideo::Layout(targetWidth, targetHeight, *MovieWidth, *MovieHeight, Mode ? std::clamp(*Mode, 0, 2) : 1);
        const auto result = DrawHook.ccall<int>(texture, box.x, box.y, box.width / *FrameWidth, box.height / *FrameHeight, alpha);
        if (device)
        {
            if (restore)
                device->SetViewport(&previous);
            device->Release();
        }
        return result;
    }

    inline void Install(const int32_t* mode, const char* drawSignature, const char* dimensionsSignature,
        const std::array<int, 4>& offsets, HMODULE module = GetModuleHandleW(nullptr))
    {
        Mode = mode;
        auto draw = hook::module_pattern(module, drawSignature);
        auto dimensions = hook::module_pattern(module, dimensionsSignature);
        if (draw.size() != 1 || dimensions.size() != 1)
        {
            OutputDebugStringA("Tony Hawk: fullscreen movie patterns unavailable.\n");
            return;
        }
        MovieWidth = *dimensions.get_first<const float*>(offsets[0]);
        MovieHeight = *dimensions.get_first<const float*>(offsets[1]);
        FrameWidth = *dimensions.get_first<const float*>(offsets[2]);
        FrameHeight = *dimensions.get_first<const float*>(offsets[3]);
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);
        if (!DrawHook)
            OutputDebugStringA("Tony Hawk: fullscreen movie hook unavailable.\n");
    }
}
