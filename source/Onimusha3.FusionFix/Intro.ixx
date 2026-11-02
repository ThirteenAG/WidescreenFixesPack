module;
#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Intro;
import ComVars;
import Window;


export namespace OnimushaIntro
{
    inline SafetyHookInline Splash, Shot;

    inline int __fastcall ShortSplash(uint8_t* state, void*)
    {
        if (state[1] == 2)
            *reinterpret_cast<uint32_t*>(state + 136) = 121;
        return Splash.unsafe_thiscall<int>(state);
    }

    inline int __fastcall ShortShot(uint8_t* state, void*)
    {
        if (state[1] == 2)
            *reinterpret_cast<uint32_t*>(state + 136) = 131;
        return Shot.unsafe_thiscall<int>(state);
    }

    inline void Draw(IDirect3DDevice9* device)
    {
        if (Startup != 2 && Startup != 4)
            return;
        IDirect3DSurface9* buffer = nullptr;
        if (SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &buffer)))
        {
            device->ColorFill(buffer, nullptr, D3DCOLOR_XRGB(0, 0, 0));
            buffer->Release();
        }
    }

    inline void Init()
    {
        if (FixSettings().Bool("MAIN.SkipPressStart"))
        {
            auto start = hook::pattern("F6 05 ? ? ? ? 08 74 2A 38 1D ? ? ? ? 53 53 53 53 53 53 0F 95 C0");
            injector::MakeNOP(start.get_first(7), 2, true);
        }
        if (FixSettings().Bool("MAIN.SkipIntro"))
        {
            auto splash = hook::pattern("56 8B F1 0F B6 46 01 83 F8 03 77 4F FF 24 85 ? ? ? ? 6A 0C 6A 01");
            auto shot = hook::pattern("56 8B F1 8A 46 01 0F B6 C8 83 F9 03 0F 87 ? ? ? ? FF 24 8D ? ? ? ? 04 01 57 8B 7E 0C");
            auto startup = hook::pattern("53 32 DB 88 1D ? ? ? ? 88 1D ? ? ? ? 88 1D ? ? ? ? 88 1D");
            Startup.SetAddress(*startup.get_first<uint8_t*>(5));
            // Keep both stages' resource, viewport and camera setup and their normal teardown. Only their waits
            // and visible splash frames are removed; save-device choices remain native.
            Splash = safetyhook::create_inline(splash.get_first(), ShortSplash);
            Shot = safetyhook::create_inline(shot.get_first(), ShortShot);
            OnimushaWindow::Draw = Draw;
        }
    }
}

