module;

#include <stdafx.h>
#include "FloatingPoint.hpp"
#include <d3d9.h>
#include "MoviePixels.hpp"
#include "MovieGeometry.hpp"

export module MovieRenderer;

import MovieInterfaces;

import MovieSurfaces;
import Window;

export namespace GodfatherMovies
{
    std::atomic<int> mode{ 1 };

    class DeviceState
    {
        IDirect3DDevice9* device = nullptr;
        IDirect3DStateBlock9* state = nullptr;
        IDirect3DVertexBuffer9* stream = nullptr;
        UINT offset = 0, stride = 0;
        bool valid = false, streamValid = false;
    public:
        explicit DeviceState(IDirect3DDevice9* target) : device(target)
        {
            if (SUCCEEDED(device->CreateStateBlock(D3DSBT_ALL, &state)))
                valid = SUCCEEDED(state->Capture());
            if (valid)
                streamValid = SUCCEEDED(device->GetStreamSource(0, &stream, &offset, &stride));
        }
        ~DeviceState()
        {
            if (valid)
            {
                state->Apply();
                if (streamValid)
                    device->SetStreamSource(0, stream, offset, stride);
            }
            if (stream)
                stream->Release();
            if (state)
                state->Release();
        }
        bool Valid() const { return valid && streamValid; }
    };

    bool Upload(Surface& surface, IDirect3DDevice9* device)
    {
        if (surface.textureDevice != device)
        {
            if (surface.texture)
                surface.texture->Release();
            surface.texture = nullptr;
            surface.textureDevice = device;
            surface.uploaded = 0;
        }
        if (!surface.texture && FAILED(device->CreateTexture(surface.width, surface.height, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_MANAGED, &surface.texture, nullptr)))
            return false;
        if (surface.uploaded == surface.version)
            return true;
        D3DLOCKED_RECT locked{};
        if (FAILED(surface.texture->LockRect(0, &locked, nullptr, 0)))
            return false;
        const bool converted = Convert(surface.pixels.data(), surface.pixels.size(), surface.pitch,
            static_cast<uint8_t*>(locked.pBits), locked.Pitch, surface.width, surface.height);
        const auto unlocked = surface.texture->UnlockRect(0);
        if (!converted || FAILED(unlocked))
            return false;
        surface.uploaded = surface.version;
        return true;
    }

    bool Render(Surface& surface, IDirect3DDevice9* device)
    {
        std::lock_guard guard(surface.pixelsMutex);
        if (!surface.visible)
            return true;
        if (!surface.ready)
            return false;
        IDirect3DSurface9* target = nullptr;
        IDirect3DSurface9* backbuffer = nullptr;
        bool matches = SUCCEEDED(device->GetRenderTarget(0, &target))
            && SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer)) && target == backbuffer;
        for (DWORD index = 1; matches && index < 4; ++index)
        {
            IDirect3DSurface9* additional = nullptr;
            if (SUCCEEDED(device->GetRenderTarget(index, &additional)) && additional)
            {
                matches = false;
                additional->Release();
            }
        }
        if (backbuffer)
            backbuffer->Release();
        if (target)
            target->Release();
        if (!matches || !Upload(surface, device))
            return false;

        const int width = GodfatherWindow::width, height = GodfatherWindow::height;
        const RECT clip{ std::clamp(surface.destination.left, 0L, LONG(width)), std::clamp(surface.destination.top, 0L, LONG(height)),
            std::clamp(surface.destination.right, 0L, LONG(width)), std::clamp(surface.destination.bottom, 0L, LONG(height)) };
        if (clip.right <= clip.left || clip.bottom <= clip.top)
            return true;
        const float sourceWidth = float(surface.source.right - surface.source.left);
        const float sourceHeight = float(surface.source.bottom - surface.source.top);
        const auto frame = Fit(float(surface.destination.left), float(surface.destination.top), float(surface.destination.right),
            float(surface.destination.bottom), sourceWidth / sourceHeight, mode.load(std::memory_order_relaxed));
        if (frame.right <= frame.left || frame.bottom <= frame.top)
            return true;
        DeviceState saved(device);
        if (!saved.Valid())
            return false;
        const bool begin = !GodfatherWindow::sceneActive;
        if (begin && FAILED(device->BeginScene()))
            return false;
        struct EndScene
        {
            IDirect3DDevice9* device;
            bool owned;
            ~EndScene() { if (owned) device->EndScene(); }
        } scene{ device, begin };
        D3DVIEWPORT9 viewport{ 0, 0, DWORD(width), DWORD(height), 0.0f, 1.0f };
        device->SetViewport(&viewport);
        device->SetScissorRect(&clip);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        D3DRECT clear[4]{};
        DWORD count = 0;
        const auto add = [&](LONG left, LONG top, LONG right, LONG bottom)
        {
            left = std::clamp(left, clip.left, clip.right);
            right = std::clamp(right, clip.left, clip.right);
            top = std::clamp(top, clip.top, clip.bottom);
            bottom = std::clamp(bottom, clip.top, clip.bottom);
            if (right > left && bottom > top)
                clear[count++] = { left, top, right, bottom };
        };
        add(clip.left, clip.top, clip.right, LONG(std::lround(frame.top)));
        add(clip.left, LONG(std::lround(frame.bottom)), clip.right, clip.bottom);
        add(clip.left, LONG(std::lround(frame.top)), LONG(std::lround(frame.left)), LONG(std::lround(frame.bottom)));
        add(LONG(std::lround(frame.right)), LONG(std::lround(frame.top)), clip.right, LONG(std::lround(frame.bottom)));
        if (count)
            device->Clear(count, clear, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        device->SetVertexShader(nullptr);
        device->SetPixelShader(nullptr);
        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        device->SetRenderState(D3DRS_LIGHTING, FALSE);
        device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        device->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
        device->SetRenderState(D3DRS_WRAP0, 0);
        device->SetRenderState(D3DRS_MULTISAMPLEMASK, 0xFFFFFFFFu);
        device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
        device->SetTexture(0, surface.texture);
        device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
        device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        device->SetTextureStageState(0, D3DTSS_RESULTARG, D3DTA_CURRENT);
        device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        const float u0 = (surface.source.left + frame.u0 * sourceWidth) / surface.width;
        const float u1 = (surface.source.left + frame.u1 * sourceWidth) / surface.width;
        const float v0 = (surface.source.top + frame.v0 * sourceHeight) / surface.height;
        const float v1 = (surface.source.top + frame.v1 * sourceHeight) / surface.height;
        struct Vertex { float x, y, z, rhw; D3DCOLOR color; float u, v; };
        const Vertex vertices[]{
            { frame.left - 0.5f, frame.top - 0.5f, 0.0f, 1.0f, 0xFFFFFFFFu, u0, v0 },
            { frame.right - 0.5f, frame.top - 0.5f, 0.0f, 1.0f, 0xFFFFFFFFu, u1, v0 },
            { frame.left - 0.5f, frame.bottom - 0.5f, 0.0f, 1.0f, 0xFFFFFFFFu, u0, v1 },
            { frame.right - 0.5f, frame.bottom - 0.5f, 0.0f, 1.0f, 0xFFFFFFFFu, u1, v1 } };
        return SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex)));
    }

    void RenderPending(IDirect3DDevice9* device)
    {
        if (!hasFrames.load(std::memory_order_acquire) || !device || GetCurrentThreadId() != GodfatherWindow::renderThread
            || !GodfatherWindow::beginSceneHook || !GodfatherWindow::endSceneHook)
            return;
        Godfather::FloatingPointScope floatingPoint;
        thread_local std::vector<Surface*> frames;
        frames.clear();
        {
            std::lock_guard guard(queueMutex);
            frames = pending;
            for (auto surface : frames)
                surface->AddRef();
        }
        for (auto surface : frames)
        {
            Render(*surface, device);
            surface->Release();
        }
        frames.clear();
    }
}
