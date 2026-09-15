module;

#include <stdafx.h>

export module CursorClip;

import ComVars;

class CursorClip
{
public:
    CursorClip()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            auto bClipCursor = iniReader.ReadInteger("MAIN", "ClipCursor", 1) != 0;

            if (bClipCursor)
            {
                WFP::onGameProcessEvent() += []()
                {
                    POINT pt;
                    RECT rec;

                    GetClientRect(hWnd, &rec);
                    MapWindowPoints(hWnd, nullptr, (LPPOINT)&rec, 2);

                    if (hWnd == GetFocus())
                        ClipCursor(&rec);
                    else
                        ClipCursor(NULL);

                    GetCursorPos(&pt);
                    ScreenToClient(hWnd, &pt);
                };

                //TODO: add onActivateApp event to dllmain
                WFP::onActivateApp() += [](bool wParam)
                {
                    if (!wParam)
                    {
                        ClipCursor(NULL);
                    }
                };
            }
        };
    }
} CursorClip;