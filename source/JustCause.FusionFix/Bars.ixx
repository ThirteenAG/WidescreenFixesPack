module;
#include <stdafx.h>
#include <array>
#include <algorithm>
#include <d3d9.h>

export module Bars;


export namespace JCBars
{
    inline bool Clear(IDirect3DDevice9* device, int width, int height, RECT picture)
    {
        picture.left = std::clamp<LONG>(picture.left, 0, width);
        picture.right = std::clamp<LONG>(picture.right, picture.left, width);
        picture.top = std::clamp<LONG>(picture.top, 0, height);
        picture.bottom = std::clamp<LONG>(picture.bottom, picture.top, height);
        std::array<D3DRECT, 4> rectangles{};
        DWORD count = 0;
        const auto append = [&](LONG left, LONG top, LONG right, LONG bottom)
        {
            if (right > left && bottom > top) rectangles[count++] = { left, top, right, bottom };
        };
        append(0, 0, picture.left, height);
        append(picture.right, 0, width, height);
        append(picture.left, 0, picture.right, picture.top);
        append(picture.left, picture.bottom, picture.right, height);
        if (!count) return true;

        struct State
        {
            IDirect3DDevice9* device;
            std::array<IDirect3DSurface9*, 4> targets{};
            IDirect3DSurface9* backbuffer = nullptr;
            IDirect3DSurface9* depth = nullptr;
            D3DVIEWPORT9 viewport{};
            RECT scissorRect{};
            DWORD scissor = FALSE, targetCount = 0;
            bool changed = false;
            ~State()
            {
                if (changed)
                {
                    device->SetRenderTarget(0, targets[0]);
                    for (DWORD i = 1; i < targetCount; ++i) device->SetRenderTarget(i, targets[i]);
                    device->SetDepthStencilSurface(depth);
                    device->SetViewport(&viewport);
                    device->SetScissorRect(&scissorRect);
                    device->SetRenderState(D3DRS_SCISSORTESTENABLE, scissor);
                }
                if (backbuffer) backbuffer->Release();
                if (depth) depth->Release();
                for (auto target : targets) if (target) target->Release();
            }
        } state{ device };
        D3DCAPS9 caps{};
        if (!device || FAILED(device->GetDeviceCaps(&caps)) || FAILED(device->GetRenderTarget(0, &state.targets[0])) ||
            FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &state.backbuffer)) ||
            FAILED(device->GetViewport(&state.viewport)) || FAILED(device->GetScissorRect(&state.scissorRect)) ||
            FAILED(device->GetRenderState(D3DRS_SCISSORTESTENABLE, &state.scissor)))
            return false;
        state.targetCount = std::min<DWORD>(caps.NumSimultaneousRTs, static_cast<DWORD>(state.targets.size()));
        for (DWORD i = 1; i < state.targetCount; ++i) device->GetRenderTarget(i, &state.targets[i]);
        device->GetDepthStencilSurface(&state.depth);
        state.changed = true;
        for (DWORD i = 1; i < state.targetCount; ++i) device->SetRenderTarget(i, nullptr);
        device->SetDepthStencilSurface(nullptr);
        const D3DVIEWPORT9 output{ 0, 0, static_cast<DWORD>(width), static_cast<DWORD>(height), 0.0f, 1.0f };
        if (FAILED(device->SetRenderTarget(0, state.backbuffer)) || FAILED(device->SetViewport(&output)) ||
            FAILED(device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE))) return false;
        return SUCCEEDED(device->Clear(count, rectangles.data(), D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0));
    }
}
