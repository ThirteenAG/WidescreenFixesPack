module;

#include <stdafx.h>
#include <dinput.h>

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

    bool IsController(const DIDEVICEINSTANCEA& instance)
    {
        const auto type = GET_DIDEVICE_TYPE(instance.dwDevType);
        return type >= DI8DEVTYPE_JOYSTICK && type <= DI8DEVTYPE_1STPERSON;
    }

    void AcceptMappedController(SafetyHookContext& regs)
    {
        if (!IsController(*reinterpret_cast<const DIDEVICEINSTANCEA*>(regs.esi)))
            return;

        // The game has one player and three device slots. Leave room for the keyboard and mouse.
        const auto mapper = reinterpret_cast<const uint8_t*>(regs.ebp);
        const auto devices = *reinterpret_cast<const uint8_t* const*>(mapper + 24);
        const auto count = *reinterpret_cast<const uint32_t*>(mapper + 28);
        for (uint32_t i = 0; i < count; ++i)
        {
            if (IsController(*reinterpret_cast<const DIDEVICEINSTANCEA*>(devices + 604 * i + 24)))
            {
                regs.eax &= ~DIEDBS_MAPPEDPRI1;
                return;
            }
        }

        // Generic controller actions can arrive as secondary semantic mappings.
        if (regs.eax & DIEDBS_MAPPEDPRI2)
            regs.eax |= DIEDBS_MAPPEDPRI1;
    }
}

class Gamepad
{
public:
    Gamepad()
    {
        WFP::onInitEvent() += []()
        {
            auto mapped = hook::pattern("8B 84 24 44 01 00 00 83 E0 01 89 44 24 08 0F 84 ? ? ? ? 8B 45 1C 3B 45 20");
            static auto acceptMappedController = safetyhook::create_mid(mapped.get_first(7), TCGamepad::AcceptMappedController);

            // The game's three action formats (on foot, driving, menus), set up before the input devices are created
            aActionFormats.SetAddress(*hook::pattern("8B 86 ? ? ? ? 5F C7 82 ? ? ? ? 28 00 00 00").get_first<DIActionFormat*>(2));
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
