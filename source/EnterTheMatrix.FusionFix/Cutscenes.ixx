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
    std::atomic<bool> Changed{ false };

    // The cutscene picture: the original 16:9 band (4:3 frame, 3/4 of its height), fitted to the screen per mode
    void Apply()
    {
        const int mode = std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
        float width = 1.0f, height = 1.0f;
        const float band = 16.0f / 9.0f;
        if ((mode == 1 || mode == 3) && Screen.fAspectRatio < band)
            height = Screen.fAspectRatio / band;
        if ((mode == 2 || mode == 3) && Screen.fAspectRatio > band)
            width = band / Screen.fAspectRatio;
        const float x = (1.0f - width) / 2.0f, y = (1.0f - height) / 2.0f;
        if (pCameraList)
            (*pfnSetCameraViewport)(pCameraList, x, y, width, height);
        (*pfnSetCameraViewport)(MainCamera.get_ptr(), x, y, width, height);
    }

    void __cdecl Letterbox(uint8_t full)
    {
        LetterboxHook.ccall(full);
        if (bNativeWidescreen)
            return;
        Cinematic = !full;
        if (Cinematic)
            Apply();
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

class CutsceneBorders
{
public:
    CutsceneBorders()
    {
        WFP::onInitEvent() += []()
        {
            FixSettings().OnChange("MAIN.CutsceneBorders", []() { Cutscenes::Changed = true; });

            // Cutscene borders
            auto pattern = hook::pattern("55 8B EC 83 3D ? ? ? ? 00 0F 85 ? ? ? ? 8B 45 08 25 FF 00 00 00 A3");
            bNativeWidescreen.SetAddress(*pattern.get_first<int32_t*>(5));
            pCameraList.SetAddress(*pattern.get_first<void**>(0x2C));
            pfnSetCameraViewport.SetAddress(*pattern.get_first<ViewportFn*>(0x4A));
            MainCamera.SetAddress(*pattern.get_first<CameraViewport*>(0x60));
            Cutscenes::LetterboxHook = safetyhook::create_inline(pattern.get_first(), Cutscenes::Letterbox);
            pattern = hook::pattern("55 8B EC 51 83 3D ? ? ? ? 00 74 78 C7 45 FC");
            pfnSetTexture.SetAddress(*pattern.get_first<TextureFn*>(0x18));
            pfnSetColor.SetAddress(*pattern.get_first<ColorFn*>(0x2E));
            Cutscenes::BarsHook = safetyhook::create_inline(pattern.get_first(), Cutscenes::Bars);
        };

        WFP::onGameProcessEvent() += []()
        {
            if (Cutscenes::Changed.exchange(false) && Cutscenes::Cinematic && !bNativeWidescreen)
                Cutscenes::Apply();
        };
    }
} CutsceneBorders;
