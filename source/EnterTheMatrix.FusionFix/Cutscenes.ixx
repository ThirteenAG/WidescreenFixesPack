module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Cutscenes;

import ComVars;
import Draw;

export namespace Cutscenes
{
    bool Cinematic = false;
}

namespace Cutscenes
{
    SafetyHookInline LetterboxHook, BarsHook;
    std::atomic<int> Borders{ 3 };
    int AppliedMode = -1;

    struct ViewportSize
    {
        int32_t Width, Height;
    };
    ViewportSize* NativeViewportSize = nullptr;

    void SetViewport(float x, float y, float width, float height)
    {
        if (pCameraList)
            (*pfnSetCameraViewport)(pCameraList, x, y, width, height);
        (*pfnSetCameraViewport)(MainCamera.get_ptr(), x, y, width, height);
    }

    // The cutscene picture: the original 16:9 band (4:3 frame, 3/4 of its height), fitted to the screen per mode
    void Apply(int mode)
    {
        AppliedMode = mode;
        float width = 1.0f, height = 1.0f;
        const float band = 16.0f / 9.0f;
        if ((mode == 1 || mode == 3) && Screen.fAspectRatio < band)
            height = Screen.fAspectRatio / band;
        if ((mode == 2 || mode == 3) && Screen.fAspectRatio > band)
            width = band / Screen.fAspectRatio;
        const float x = (1.0f - width) / 2.0f, y = (1.0f - height) / 2.0f;
        SetViewport(x, y, width, height);
    }

    void __cdecl Letterbox(uint8_t full)
    {
        Cinematic = !full;
        LetterboxHook.ccall(full);
        if (Cinematic)
            Apply(std::clamp(Borders.load(), 0, 3));
    }

    // Black around the cutscene picture
    int __cdecl Bars()
    {
        const float x = static_cast<float>(MainCamera->x), y = static_cast<float>(MainCamera->y);
        const float w = static_cast<float>(MainCamera->w), h = static_cast<float>(MainCamera->h);
        if (w <= 0.0f || h <= 0.0f || (x <= 0.0f && y <= 0.0f && w >= Screen.fWidth && h >= Screen.fHeight))
            return 0;
        Draw::Black(0.0f, 0.0f, Screen.fWidth, y);
        Draw::Black(0.0f, y + h, Screen.fWidth, Screen.fHeight);
        Draw::Black(0.0f, y, x, y + h);
        Draw::Black(x + w, y, Screen.fWidth, y + h);
        return 0;
    }
}

export namespace Cutscenes
{
    void DisplayResized()
    {
        // The engine uses a separate canvas size to convert normalized camera viewports into pixels.
        *NativeViewportSize = { Screen.nWidth, Screen.nHeight };
        if (Cinematic)
            Apply(std::clamp(Borders.load(), 0, 3));
        else
            SetViewport(0.0f, 0.0f, 1.0f, 1.0f);
    }
}

class CutsceneBorders
{
public:
    CutsceneBorders()
    {
        WFP::onInitEvent() += []()
        {
            WatchMatrixNumber("MAIN.CutsceneBorders", Cutscenes::Borders);

            // Cutscene borders
            auto pattern = hook::pattern("55 8B EC 83 3D ? ? ? ? 00 0F 85 ? ? ? ? 8B 45 08 25 FF 00 00 00 A3");
            pCameraList.SetAddress(*pattern.get_first<void**>(0x2C));
            pfnSetCameraViewport.SetAddress(*pattern.get_first<ViewportFn*>(0x4A));
            MainCamera.SetAddress(*pattern.get_first<CameraViewport*>(0x60));
            auto dimensions = hook::pattern("55 8B EC 51 C7 45 FC ? ? ? ? 8B 45 FC DB 40 04 D8 4D 0C D9 5D 0C 8B 4D FC DB 41 08 D8 4D 10");
            Cutscenes::NativeViewportSize = reinterpret_cast<Cutscenes::ViewportSize*>(*dimensions.get_first<uintptr_t>(7) + 4);
            Cutscenes::LetterboxHook = safetyhook::create_inline(pattern.get_first(), Cutscenes::Letterbox);
            pattern = hook::pattern("55 8B EC 51 83 3D ? ? ? ? 00 74 78 C7 45 FC");
            pfnSetTexture.SetAddress(*pattern.get_first<TextureFn*>(0x18));
            pfnSetColor.SetAddress(*pattern.get_first<ColorFn*>(0x2E));
            Cutscenes::BarsHook = safetyhook::create_inline(pattern.get_first(), Cutscenes::Bars);
        };

        WFP::onGameProcessEvent() += []()
        {
            const int mode = std::clamp(Cutscenes::Borders.load(), 0, 3);
            if (Cutscenes::Cinematic && mode != Cutscenes::AppliedMode)
                Cutscenes::Apply(mode);
        };
    }
} CutsceneBorders;
