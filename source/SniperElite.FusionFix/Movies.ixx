module;

#include <stdafx.h>

export module Movies;

import ComVars;
import Hud;

export namespace FMV
{
    std::atomic<int> Mode{ 1 };
    SafetyHookMid ScreenRect;

    void FitToScreen(injector::reg_pack& regs)
    {
        if (MovieHandle < 0 || !MovieStreams.get())
            return;

        const auto* stream = MovieStreams.get()[MovieHandle.get()];
        if (!stream || !stream->Video || !stream->Video->Width || !stream->Video->Height)
            return;

        auto* rect = reinterpret_cast<float*>(regs.esp); //x, y, width, height before SetRect
        // The whole movie at its own aspect: Original inside the game's centred 4:3 area, Fill inside the whole screen
        const bool original = Mode.load(std::memory_order_relaxed) == 0;
        const float x = (original ? std::min(Screen.fWidth, Screen.fHeight * 4.0f / 3.0f) : Screen.fWidth) / stream->Video->Width;
        const float y = (original ? std::min(Screen.fHeight, Screen.fWidth * 3.0f / 4.0f) : Screen.fHeight) / stream->Video->Height;
        const float scale = std::min(x, y);
        rect[2] = stream->Video->Width * scale;
        rect[3] = stream->Video->Height * scale;
        rect[0] = (Screen.fWidth - rect[2]) / 2.0f;
        rect[1] = (Screen.fHeight - rect[3]) / 2.0f;
        UI::DrawBackdrop(0xFF000000); //clear the pillarbox or letterbox area every frame
    }
}
