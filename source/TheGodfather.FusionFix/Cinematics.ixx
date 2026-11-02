module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "CinematicMath.hpp"
#include <atomic>
#include <stdexcept>

export module Cinematics;

import Window;

import ComVars;

export namespace GodfatherCinematics
{
    SafetyHookInline drawHook;
    bool rootBound = false;
    std::atomic<int> borderMode{ 3 };
    std::atomic<bool> animated{ true };
    bool drawn = false;

    template <typename T> T Read(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    void* Root()
    {
        return rootBound ? CinematicRoot.get() : nullptr;
    }

    bool Active()
    {
        const auto root = Root();
        return root && Read<uint8_t>(root, 32) != 0;
    }

    float Progress()
    {
        const auto root = Root();
        return root ? Animation(Read<uint8_t>(root, 32) != 0, Read<int>(root, 36) != 0,
            Read<float>(root, 60), Read<float>(root, 40), animated.load(std::memory_order_relaxed)) : 0.0f;
    }

    float Aspect(float actual)
    {
        return Active() ? ProjectionAspect(actual, borderMode.load(std::memory_order_relaxed), Progress()) : actual;
    }

    void __fastcall Draw(void* self, void*, void* camera)
    {
        const auto raster = camera ? Read<void*>(camera, 96) : nullptr;
        if (!GodfatherWindow::device || GetCurrentThreadId() != GodfatherWindow::renderThread
            || !raster || Read<int>(raster, 12) != GodfatherWindow::width
            || Read<int>(raster, 16) != GodfatherWindow::height)
        {
            drawHook.unsafe_thiscall<void>(self, camera);
            return;
        }
        drawn = true;
    }

    void Render(IDirect3DDevice9* device)
    {
        if (!std::exchange(drawn, false) || !Active())
            return;
        const int mode = borderMode.load(std::memory_order_relaxed);
        if (!mode)
            return;
        const auto rectangles = Borders(GodfatherWindow::width, GodfatherWindow::height, mode, Progress());
        D3DRECT clear[4]{};
        DWORD count = 0;
        for (const auto& rectangle : rectangles)
            if (rectangle.right > rectangle.left && rectangle.bottom > rectangle.top)
                clear[count++] = { rectangle.left, rectangle.top, rectangle.right, rectangle.bottom };
        if (!count)
            return;
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
        if (matches)
        {
            const auto root = Root();
            const auto color = root ? D3DCOLOR_XRGB(Read<uint8_t>(root, 48), Read<uint8_t>(root, 49), Read<uint8_t>(root, 50))
                : D3DCOLOR_XRGB(0, 0, 0);
            D3DVIEWPORT9 previous{};
            if (SUCCEEDED(device->GetViewport(&previous)))
            {
                const D3DVIEWPORT9 viewport{ 0, 0, DWORD(GodfatherWindow::width), DWORD(GodfatherWindow::height), 0.0f, 1.0f };
                if (SUCCEEDED(device->SetViewport(&viewport)))
                {
                    device->Clear(count, clear, D3DCLEAR_TARGET, color, 1.0f, 0);
                    device->SetViewport(&previous);
                }
            }
        }
        if (backbuffer)
            backbuffer->Release();
        if (target)
            target->Release();
    }

    void Init()
    {
        auto root = hook::pattern("89 0D ? ? ? ? C7 06 ? ? ? ? C7 46 08 ? ? ? ? 88 46 0C 88 46 14 89 46 18 89 46 1C 88 46 20 88 5E 21 89 46 24 88 46 34 89 46 38 89 46 3C C7 46 10 ? ? ? ?");
        if (!GodfatherWindow::createD3DHook)
            throw std::runtime_error("Cinematic rendering dependency unavailable");
        CinematicRoot.SetAddress(*root.get_first<void**>(2));
        rootBound = true;
        borderMode.store(FixSettings().Int("MAIN.CutsceneBorders"), std::memory_order_relaxed);
        animated.store(FixSettings().Bool("MAIN.AnimatedCutsceneBorders"), std::memory_order_relaxed);
        FixSettings().OnChange("MAIN.CutsceneBorders", []()
        {
            borderMode.store(FixSettings().Int("MAIN.CutsceneBorders"), std::memory_order_relaxed);
        });
        FixSettings().OnChange("MAIN.AnimatedCutsceneBorders", []()
        {
            animated.store(FixSettings().Bool("MAIN.AnimatedCutsceneBorders"), std::memory_order_relaxed);
        });
        const auto table = *root.get_first<void**>(52);
        const auto draw = table ? *table : nullptr;
        drawHook = draw ? safetyhook::create_inline(draw, Draw) : SafetyHookInline{};
        if (!drawHook)
        {
            rootBound = false;
            throw std::runtime_error("Cinematic rendering hook unavailable");
        }
        const auto frame = GodfatherWindow::frame;
        GodfatherWindow::frame = [frame](IDirect3DDevice9* device)
        {
            if (frame)
                frame(device);
            Render(device);
        };
    }
}
