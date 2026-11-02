module;

#include <stdafx.h>

export module Gamepad;

import ComVars;

// The menus' gamepad actions use the DirectInput browser genre, which only a genre-aware mapper resolves; XInput
// controllers through Xidi offer the generic buttons and axes. The menu actions get the generic ones of the game's own
// gamepad layout: A select, B back, Start exit, X and Y set and clear the map waypoint, left stick moves the cursor.
namespace TCGamepad
{
    constexpr DWORD BrowserGenre = 0x28000000;
    constexpr std::pair<DWORD, DWORD> MenuSemantics[] =
    {
        { 0x28000401, 0xFF004400 },     // select: button 1 (A)
        { 0x28004407, 0xFF004401 },     // back: button 2 (B)
        { 0x28004408, 0xFF004407 },     // exit: button 8 (Start)
        { 0x28004404, 0xFF004402 },     // set waypoint: button 3 (X)
        { 0x28000402, 0xFF004403 },     // clear waypoint: button 4 (Y)
        { 0x28010202, 0xFF014201 },     // cursor up and down: Y axis
        { 0x28008201, 0xFF00C201 },     // cursor left and right: X axis
    };
}

class Gamepad
{
public:
    Gamepad()
    {
        WFP::onInitEvent() += []()
        {
            // The game's three action formats (on foot, driving, menus), set up before the input devices are created
            aActionFormats.SetAddress(*hook::pattern("68 84 01 00 00 E8 ? ? ? ? 68 85 01 00 00 A3").get_first<DIActionFormat*>(16));
            auto& formats = aActionFormats.get();
            for (int i = 0; i < 3; ++i)
            {
                if (formats[i].genre != TCGamepad::BrowserGenre)
                    continue;
                for (DWORD a = 0; a < formats[i].count; ++a)
                {
                    auto& action = formats[i].actions[a];
                    for (const auto& [browser, generic] : TCGamepad::MenuSemantics)
                    {
                        if (action.semantic == browser)
                            injector::WriteMemory<DWORD>(&action.semantic, generic, true);
                    }
                }
            }
        };
    }
} Gamepad;
