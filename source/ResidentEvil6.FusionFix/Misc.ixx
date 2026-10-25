module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <xinput.h>

export module Misc;

import ComVars;

// Skip intro, and the autoclicker that confirms the game's start-up notices with the pad's A button.

namespace Autoclicker
{
    bool bNeedsAutoclick = false;

    injector::hook_back<const char* (__fastcall*)(void*, void*, unsigned int)> hbGetMessage;
    const char* __fastcall GetMessageHook(void* _this, void* edx, unsigned int id)
    {
        static constexpr std::string_view Notices[] =
        {
            "Restart game from last checkpoint.",
            "Load successful.",
            "This game includes an auto-save feature.\r\nPlease do not turn off your computer.",
            "Version",
        };

        auto ret = hbGetMessage.fun(_this, edx, id);
        if (ret && std::find(std::begin(Notices), std::end(Notices), std::string_view(ret)) != std::end(Notices))
            bNeedsAutoclick = true;
        return ret;
    }

    std::future<void*> pXInputGetState;
    injector::hook_back<DWORD(WINAPI*)(DWORD, XINPUT_STATE*)> hbXInputGetState;
    DWORD WINAPI XInputGetStateHook(DWORD dwUserIndex, XINPUT_STATE* pState)
    {
        if (!hbXInputGetState.fun)
            hbXInputGetState.fun = (decltype(hbXInputGetState.fun))pXInputGetState.get();

        auto ret = hbXInputGetState.fun(dwUserIndex, pState);

        if (bNeedsAutoclick)
        {
            pState->Gamepad.wButtons = XINPUT_GAMEPAD_A;

            static auto frames = 0;
            if (frames >= 20)
            {
                pState->Gamepad.wButtons = 0x0000;
                frames = 0;
            }
            frames++;

            bNeedsAutoclick = false;
        }

        return ret;
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();

            // Skip the Capcom and engine logos and the startup notice
            if (settings.Bool("MAIN.SkipIntro"))
            {
                auto logos = hook::pattern("0F 84 ? ? ? ? 48 74 0C 8B 04 8D");
                auto logosEnd = hook::pattern("FF D0 89 87 ? ? ? ? 5F");
                if (PatternAvailable(logos, "ResidentEvil6.FusionFix: Misc") && PatternAvailable(logosEnd, "ResidentEvil6.FusionFix: Misc"))
                {
                    injector::MakeNOP(logos.get_first(), 6);
                    injector::MakeJMP(logos.get_first(), logosEnd.get_first(8));
                }

                auto notice = hook::pattern("0F 8C ? ? ? ? 83 FA 04");
                if (PatternAvailable(notice, "ResidentEvil6.FusionFix: Misc"))
                    WriteGameValue<uint16_t>(notice.get_first(), 0xE990);
            }

            if (settings.Bool("MAIN.Autoclicker"))
            {
                Autoclicker::pXInputGetState = std::move(IATHook::Replace(GetModuleHandleA(NULL), "XINPUT1_3.dll",
                    std::forward_as_tuple("XInputGetState@2", Autoclicker::XInputGetStateHook)
                )["XInputGetState@2"]);

                auto pattern = hook::pattern("E8 ? ? ? ? 8D 4C 24 24 8A 11");
                if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: Misc"))
                    Autoclicker::hbGetMessage.fun = injector::MakeCALL(pattern.get_first(), Autoclicker::GetMessageHook, true).get();
            }
        };
    }
} Misc;
