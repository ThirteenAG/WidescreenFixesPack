module;

#include "stdafx.h"

export module CursorClip;

import ComVars;

namespace
{
    bool bClipped = false;

    void ReleaseCursor()
    {
        if (bClipped)
        {
            ClipCursor(nullptr);
            bClipped = false;
        }
    }

    void UpdateCursorClip()
    {
        HWND window = hWnd;
        RECT rect{};
        POINT origin{};
        if (!window || GetForegroundWindow() != window || IsIconic(window) ||
            !GetClientRect(window, &rect) || !ClientToScreen(window, &origin) ||
            rect.right <= rect.left || rect.bottom <= rect.top)
        {
            ReleaseCursor();
            return;
        }

        OffsetRect(&rect, origin.x, origin.y);
        if (ClipCursor(&rect))
            bClipped = true;
    }
}

class CursorClip
{
public:
    CursorClip()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            if (iniReader.ReadInteger("MAIN", "ClipCursor", 1) == 0)
                return;

            WFP::onGameProcessEvent() += UpdateCursorClip;
            WFP::onActivateApp() += [](bool active)
            {
                if (!active)
                    ReleaseCursor();
            };
            WFP::onShutdownEvent() += ReleaseCursor;
        };
    }
} CursorClip;
