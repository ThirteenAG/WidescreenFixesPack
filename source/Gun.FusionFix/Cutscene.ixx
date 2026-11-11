module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <atomic>

export module Cutscene;

import ComVars;

// Cutscenes switch the engine to its letterbox mode: the 3D view and 2D layer shrink to the middle three quarters of
// the screen, a 16:9 frame on the original 4:3 screen. On wider screens the fix keeps that frame at full height
// instead and covers the sides.
export namespace GunCutscene
{
    constexpr float Aspect = 16.0f / 9.0f;
    enum Borders { Off, Letterbox, Pillarbox, Both };
    inline SafetyHookInline Hook;
    inline std::atomic_bool Requested{ false };
    inline std::atomic<float> ScreenAspect{ 4.0f / 3.0f };

    inline int Mode()
    {
        static const auto& mode = FixSettings().Int("MAIN.CutsceneBorders");
        return std::clamp(mode, 0, 3);
    }

    inline bool Pillarboxed(float aspect)
    {
        return (Mode() & Pillarbox) && aspect >= Aspect * 0.999f;
    }

    // The game's letterbox stays only where it gives the original frame or the player chose it.
    inline bool Letterboxed(float aspect)
    {
        return Mode() == Letterbox || (Mode() == Both && !Pillarboxed(aspect));
    }

    inline bool Active()
    {
        return Hook && Requested.load(std::memory_order_relaxed);
    }

    // fov is the native angle for the 4:3 screen.
    inline float FieldOfView(float fov, float aspect)
    {
        if (Pillarboxed(aspect))
            return AdjustFOV(fov, aspect, Aspect);
        return AdjustFOV(fov, aspect);
    }

    inline char __cdecl SetLetterbox(char on)
    {
        Requested.store(on != 0, std::memory_order_relaxed);
        return Hook.ccall<char>(static_cast<char>(on && Letterboxed(ScreenAspect.load(std::memory_order_relaxed))));
    }

    // Every frame on the game thread: follows option and resolution changes during a cutscene.
    inline void Tick()
    {
        if (!Active() || !LetterboxFlag.is_initialized())
            return;
        const bool wanted = Letterboxed(ScreenAspect.load(std::memory_order_relaxed));
        if ((LetterboxFlag != 0) != wanted)
            Hook.ccall<char>(static_cast<char>(wanted));
    }

    inline void Draw(IDirect3DDevice9* device)
    {
        if (!Active() || !device)
            return;
        IDirect3DSurface9* surface = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
            return;
        D3DSURFACE_DESC desc{};
        surface->GetDesc(&desc);
        surface->Release();
        if (!desc.Height || !Pillarboxed(static_cast<float>(desc.Width) / desc.Height))
            return;
        const auto bar = static_cast<LONG>((desc.Width - desc.Height * Aspect) * 0.5f);
        if (bar <= 0)
            return;
        const D3DRECT bars[] =
        {
            { 0, 0, bar, static_cast<LONG>(desc.Height) },
            { static_cast<LONG>(desc.Width) - bar, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height) },
        };
        D3DVIEWPORT9 viewport{}, full{ 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
        device->GetViewport(&viewport);
        device->SetViewport(&full);
        device->Clear(2, bars, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        device->SetViewport(&viewport);
    }

    inline void Init(float aspect)
    {
        ScreenAspect.store(aspect, std::memory_order_relaxed);
        auto letterbox = hook::pattern("8A 44 24 04 84 C0 A0 ? ? ? ? 74 27 84 C0 75 63 A1 ? ? ? ? D9 05 ? ? ? ? D8 0D");
        LetterboxFlag.SetAddress(*letterbox.get_first<uint8_t*>(7));
        Hook = safetyhook::create_inline(letterbox.get_first(), SetLetterbox);
    }
}
