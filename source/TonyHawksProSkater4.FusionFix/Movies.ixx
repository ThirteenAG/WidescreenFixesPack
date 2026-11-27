module;

#include <stdafx.h>
#include <minidx8/d3d8.h>
#include <span>
#include "Shared/TonyHawk/Video.hpp"

export module Movies;

import ComVars;

export namespace THPS4Video
{
    inline SafetyHookInline DrawHook;
    inline SafetyHookMid LoadingPictureHook;

    inline void __cdecl Draw(IDirect3DTexture8* texture, float x, float y, float width, float height, float alpha)
    {
        if (!texture || !std::isfinite(MovieFrameWidth.get()) || !std::isfinite(MovieFrameHeight.get()) || MovieFrameWidth.get() <= 0.0f || MovieFrameHeight.get() <= 0.0f)
        {
            DrawHook.ccall<void>(texture, x, y, width, height, alpha);
            return;
        }
        float targetWidth = MovieFrameWidth.get(), targetHeight = MovieFrameHeight.get();
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
        // Every movie is a 4:3 picture; the 640x448 and 320x224 ones are anamorphic
        const auto box = TonyVideo::Layout(targetWidth, targetHeight, 4.0f, 3.0f, 1);
        DrawHook.ccall<void>(texture, box.x, box.y, box.width / MovieFrameWidth.get(), box.height / MovieFrameHeight.get(), alpha);
        if (device)
        {
            if (restore)
                device->SetViewport(&previous);
            device->Release();
        }
    }

    inline void Install(HMODULE module = GetModuleHandleW(nullptr))
    {
        auto draw = hook::module_pattern(module, "83 EC 70 56 8B 74 24 78 85 F6 0F 84 ? ? ? ? 57 68 01 00 01 00 6A 04 E8");
        auto dimensions = hook::module_pattern(module, "D9 1D ? ? ? ? DF 6C 24 ? 51 6A 01 6A 16 53 D9 1D ? ? ? ? DB 05 ? ? ? ? 6A 01 56 57 50 D9 1D ? ? ? ? DB 05 ? ? ? ? D9 1D");
        MovieFrameWidth.SetAddress(*dimensions.get_first<float*>(35));
        MovieFrameHeight.SetAddress(*dimensions.get_first<float*>(47));
        DrawHook = safetyhook::create_inline(draw.get_first(), Draw);

        auto loadingPicture = hook::module_pattern(module,
            "A1 ? ? ? ? 8D 4C 24 0C 6A 18 51 8B 10 6A 02 6A 05 50 FF 92 20 01 00 00 8B 4C 24 08");
        LoadingPictureHook = safetyhook::create_mid(loadingPicture.get_first(19), [](SafetyHookContext& regs)
        {
            const auto box = TonyVideo::Layout(Screen.fWidth, Screen.fHeight, 4.0f, 3.0f, 1);
            if (box.x == 0.0f && box.y == 0.0f)
                return;

            // The loading picture is drawn directly as a transformed quad. The progress bar has a separate draw.
            struct Vertex
            {
                float x, y, z, rhw, u, v;
            };
            auto* device = *reinterpret_cast<IDirect3DDevice8**>(regs.esp);
            auto* vertices = *reinterpret_cast<Vertex**>(regs.esp + 12);
            device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
            for (auto& vertex : std::span<Vertex, 4>(vertices, 4))
            {
                vertex.x = box.x + (vertex.x < 0.0f ? -0.5f : box.width);
                vertex.y = box.y + (vertex.y < 0.0f ? -0.5f : box.height);
            }
        });
    }
}
