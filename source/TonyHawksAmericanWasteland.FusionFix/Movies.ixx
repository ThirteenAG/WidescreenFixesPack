module;

#include <stdafx.h>
#include <d3d9.h>
#include "Shared/TonyHawk/Video.hpp"

export module Movies;

export namespace THAWMovies
{
    struct Vertex { float x, y, z, rhw, u, v; };
    static_assert(sizeof(Vertex) == 24);
    inline SafetyHookMid BeginMovie, EndMovie, BeginDraw, EndDraw;
    inline const int32_t* Mode = nullptr;
    inline thread_local const uint32_t* Movie = nullptr;
    inline thread_local Vertex Vertices[4]{};
    inline thread_local IDirect3DDevice9* Device = nullptr;
    inline thread_local D3DVIEWPORT9 Previous{};
    inline thread_local bool RestoreViewport = false;

    inline void Install(const int32_t* mode)
    {
        Mode = mode;
        auto movie = hook::pattern("8B 15 ? ? ? ? 52 E8 ? ? ? ? A1 ? ? ? ? 8B 48 08 8B 50 0C 83 C4 04");
        auto draw = hook::pattern("6A 18 8D 54 24 30 52 6A 02 6A 05 50 FF 91 4C 01 00 00 8B 54 24 20");
        BeginMovie = safetyhook::create_mid(movie.get_first(7), [](SafetyHookContext& regs)
        {
            Movie = *reinterpret_cast<const uint32_t**>(regs.esp);
        });
        EndMovie = safetyhook::create_mid(movie.get_first(12), [](SafetyHookContext&)
        {
            Movie = nullptr;
        });
        BeginDraw = safetyhook::create_mid(draw.get_first(12), [](SafetyHookContext& regs)
        {
            if (!Movie || !Movie[0] || !Movie[1]) return;
            auto* args = reinterpret_cast<uint32_t*>(regs.esp);
            auto* device = reinterpret_cast<IDirect3DDevice9*>(args[0]);
            auto* source = reinterpret_cast<const Vertex*>(args[3]);
            if (!device || !source) return;
            // The frame goes through an off-screen target first; only the final quad on the back buffer is placed
            IDirect3DSurface9* target = nullptr;
            if (FAILED(device->GetRenderTarget(0, &target)) || !target) return;
            IDirect3DSurface9* backBuffer = nullptr;
            const bool final = SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) && backBuffer == target;
            if (backBuffer) backBuffer->Release();
            D3DSURFACE_DESC description{};
            const auto result = target->GetDesc(&description);
            target->Release();
            if (!final || FAILED(result) || !description.Width || !description.Height) return;
            const float width = static_cast<float>(description.Width), height = static_cast<float>(description.Height);
            const auto box = TonyVideo::Layout(width, height, static_cast<float>(Movie[0]), static_cast<float>(Movie[1]), std::clamp(*Mode, 0, 1));
            // The game places its quad in the 2D frame (squeezed into a wide strip); the whole picture goes into the box
            float left = source[0].x, right = source[0].x, top = source[0].y, bottom = source[0].y;
            for (size_t i = 1; i < 4; ++i)
            {
                left = std::min(left, source[i].x);
                right = std::max(right, source[i].x);
                top = std::min(top, source[i].y);
                bottom = std::max(bottom, source[i].y);
            }
            if (right - left < 1.0f || bottom - top < 1.0f) return;
            for (size_t i = 0; i < 4; ++i)
            {
                Vertices[i] = source[i];
                Vertices[i].x = box.x + (source[i].x - left) * box.width / (right - left);
                Vertices[i].y = box.y + (source[i].y - top) * box.height / (bottom - top);
            }
            Device = device;
            RestoreViewport = SUCCEEDED(device->GetViewport(&Previous));
            if (RestoreViewport)
            {
                D3DVIEWPORT9 viewport{ 0, 0, description.Width, description.Height, 0.0f, 1.0f };
                RestoreViewport = SUCCEEDED(device->SetViewport(&viewport));
            }
            device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0);
            args[3] = reinterpret_cast<uint32_t>(Vertices);
        });
        EndDraw = safetyhook::create_mid(draw.get_first(18), [](SafetyHookContext&)
        {
            if (RestoreViewport && Device) Device->SetViewport(&Previous);
            RestoreViewport = false;
            Device = nullptr;
        });
    }
}
