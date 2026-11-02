module;

#include <stdafx.h>
#include <d3d9.h>
#include <mutex>
#include "FixSettings.hpp"

export module Cinematics;

import ComVars;
import Splitscreen;
import CinematicFrame;

namespace
{
    struct Shot
    {
        ULONGLONG updated = 0;
        RECT area{};
        uintptr_t controller = 0;
    };
    std::mutex ShotsMutex;
    std::array<Shot, 2> Shots;
}

export namespace Cinematics
{
    void Camera(uintptr_t camera, bool cinematic)
    {
        const auto viewport = GetCameraViewport(camera);
        if (viewport.player < 0 || viewport.player >= int(Shots.size()))
            return;
        {
            std::lock_guard lock(ShotsMutex);
            Shots[viewport.player] = { cinematic ? GetTickCount64() : 0, viewport.area,
                *reinterpret_cast<const uintptr_t*>(camera + 0x20) };
        }
    }

    float Projection(uintptr_t viewport, float angle, int width, int height)
    {
        if (!viewport || width <= 0 || height <= 0)
            return angle;
        const auto controller = *reinterpret_cast<const uintptr_t*>(viewport + 0x30);
        if (!controller)
            return angle;
        const auto now = GetTickCount64();
        std::lock_guard lock(ShotsMutex);
        for (const auto& shot : Shots)
        {
            if (shot.updated && now - shot.updated <= 250 && shot.controller == controller &&
                shot.area.right - shot.area.left == width && shot.area.bottom - shot.area.top == height)
                return SCCinematicFrame::FieldOfView(angle, float(width) / height, FixSettings().Int("MAIN.CutsceneBorders"));
        }
        return angle;
    }

    void Video()
    {
        std::lock_guard lock(ShotsMutex);
        for (auto& shot : Shots)
            shot.updated = 0;
    }

    float SubtitleInset(int player, int width, int height)
    {
        if (player < 0 || player >= int(Shots.size()))
            return 0.0f;
        std::lock_guard lock(ShotsMutex);
        const auto updated = Shots[player].updated;
        if (!updated || GetTickCount64() - updated > 250)
            return 0.0f;
        return SCCinematicFrame::Fit(float(width), float(height), FixSettings().Int("MAIN.CutsceneBorders")).top;
    }

    void Present(IDirect3DDevice9* device)
    {
        const int borders = FixSettings().Int("MAIN.CutsceneBorders");
        if (!borders)
            return;
        std::array<Shot, 2> shots;
        {
            std::lock_guard lock(ShotsMutex);
            shots = Shots;
        }
        const auto now = GetTickCount64();
        if (std::none_of(shots.begin(), shots.end(), [&](const Shot& shot) { return shot.updated && now - shot.updated <= 250; }))
            return;
        IDirect3DSurface9* backbuffer = nullptr;
        IDirect3DSurface9* target = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer)))
            return;
        const bool current = SUCCEEDED(device->GetRenderTarget(0, &target)) && target == backbuffer;
        D3DSURFACE_DESC desc{};
        const bool valid = SUCCEEDED(backbuffer->GetDesc(&desc));
        if (target)
            target->Release();
        backbuffer->Release();
        if (!current || !valid)
            return;
        std::array<D3DRECT, 4> bars{};
        DWORD count = 0;
        for (const auto& shot : shots)
        {
            const auto& area = shot.area;
            if (!shot.updated || now - shot.updated > 250 || area.left < 0 || area.top < 0 ||
                area.right > LONG(desc.Width) || area.bottom > LONG(desc.Height))
                continue;
            const auto frame = SCCinematicFrame::Fit(float(area.right - area.left), float(area.bottom - area.top), borders);
            if (frame.left > 0)
            {
                const auto edge = LONG(std::lround(frame.left));
                bars[count++] = { area.left, area.top, area.left + edge, area.bottom };
                bars[count++] = { area.right - edge, area.top, area.right, area.bottom };
            }
            else if (frame.top > 0)
            {
                const auto edge = LONG(std::lround(frame.top));
                bars[count++] = { area.left, area.top, area.right, area.top + edge };
                bars[count++] = { area.left, area.bottom - edge, area.right, area.bottom };
            }
        }
        if (!count)
            return;
        D3DVIEWPORT9 previous{};
        if (FAILED(device->GetViewport(&previous)))
            return;
        const D3DVIEWPORT9 full{ 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
        if (SUCCEEDED(device->SetViewport(&full)))
        {
            device->Clear(count, bars.data(), D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            device->SetViewport(&previous);
        }
    }
}
