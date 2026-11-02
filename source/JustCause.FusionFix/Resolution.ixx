module;
#include <stdafx.h>
export module Resolution;
import ComVars;
import Hud;
import Video;

export void UpdateScreen(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    Screen.nWidth = width;
    Screen.nHeight = height;
    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    JCHud::screenAspect.store(Screen.fAspectRatio);
    JCVideo::outputSize.store((static_cast<uint32_t>(width) << 16) | static_cast<uint32_t>(height));
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.fWidescreenOffsetX = (Screen.fWidth - Screen.fHeight * (16.0f / 9.0f)) / 2.0f;
    Screen.fWidescreenOffsetY = (Screen.fHeight - (Screen.fHeight * ((4.0f / 3.0f) / (16.0f / 9.0f)))) / 2.0f;
}

