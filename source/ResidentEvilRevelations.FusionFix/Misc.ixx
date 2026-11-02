module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "Shared/PadInput.hpp"

export module Misc;

import ComVars;

// Start-up: logos and the notices that only offer OK. Focus loss.

namespace
{
    const int* pPauseOnFocusLoss = nullptr;
    SafetyHookInline shActivate;

    // Full screen always pauses: the game loses its device there.
    bool Windowed()
    {
        auto device = GetD3DDevice();
        IDirect3DSwapChain9* chain = nullptr;
        if (!device || FAILED(device->GetSwapChain(0, &chain)) || !chain)
            return false;
        D3DPRESENT_PARAMETERS parameters{};
        const auto result = chain->GetPresentParameters(&parameters);
        chain->Release();
        return SUCCEEDED(result) && parameters.Windowed;
    }

    BOOL __stdcall Activate(bool active)
    {
        if (!active && pPauseOnFocusLoss && !*pPauseOnFocusLoss && Windowed())
            return TRUE;
        return shActivate.unsafe_stdcall<BOOL>(active);
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
            // Skip the startup logos: the logo scene (aRevelationsLogo) waits until its logo screen has loaded, then
            // until the logo animation has ended (its state 25) before it goes on to the title (aRevelationsTitle).
            // Without the second wait it goes on as soon as the logo screen has loaded.
            if (settings.Bool("MAIN.SkipIntro"))
            {
                auto pattern = hook::pattern("8B 4E 68 8B 11 8B 82 D4 00 00 00 FF D0 3C 01 75 ? 8B 0D");
                injector::MakeNOP(pattern.get_first(15), 2, true);

                // The autosave notice after Start (OK only): its step goes on once no message box is open.
                pattern = hook::pattern("80 BE C0 05 00 00 00 74 ? 8B 0D ? ? ? ? 6A 00 6A 06 6A 25 E8");
                injector::WriteMemory<uint8_t>(pattern.get_first(7), 0xEB, true);

                // "The RESIDENT EVIL.NET service is not available" (OK only) when the service can't be reached: the
                // sign-in step waits for that message box and goes on to the main menu without it.
                pattern = hook::pattern("8B 0D ? ? ? ? 6A 01 6A 21 6A 13 E8 ? ? ? ? 8B 8C 24");
                injector::MakeNOP(pattern.get_first(), 17, true);
            }

            // The title's ready input check enters the game's normal player and save-selection flow.
            if (settings.Bool("MAIN.SkipPressStart"))
            {
                auto press = hook::pattern("E8 ? ? ? ? 84 C0 74 ? 83 BE 78 02 00 00 00 75 ? 84 DB 75 ? 8B CE E8");
                static SafetyHookMid PressStartHook = safetyhook::create_mid(press.get_first(18), [](SafetyHookContext& regs)
                {
                    static bool skipped = false;
                    if (skipped)
                        return;
                    if (regs.ebx & 0xFF)
                    {
                        skipped = true;
                        return;
                    }
                    XINPUT_STATE state{};
                    DWORD slot = 0;
                    if (!PadInput::Read(state, slot) || slot > 1)
                        slot = 0;
                    if (regs.edi == slot)
                    {
                        skipped = true;
                        regs.ebx |= 1;
                    }
                });
            }

            // MAIN.PauseOnFocusLoss: the window's activation handler pauses the game and its sound while another
            // window is active. Off keeps the game running.
            pPauseOnFocusLoss = &settings.Int("MAIN.PauseOnFocusLoss");
            auto pattern = hook::pattern("80 7C 24 04 00 74 2E A1 ? ? ? ? C6 80 51 25 00 00 01");
            shActivate = safetyhook::create_inline(pattern.get_first(), Activate);
        };
    }
} Misc;
