module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <d3d9.h>
#include <xinput.h>

export module Misc;

import ComVars;

// Skip intro, the title logo, the episode 4 fix, the GPU sync workaround and the developer autoclicker.

namespace
{
    int nForceLogo = 0;
    int __cdecl GetLogoRegion()
    {
        return nForceLogo - 1;
    }

    bool bDisableCreateQuery = false;
    void __fastcall GpuCommandBufferSync(IDirect3DDevice9** device, void* edx)
    {
        if (bDisableCreateQuery)
            return;

        IDirect3DQuery9* query = nullptr;
        device[38]->CreateQuery(D3DQUERYTYPE_EVENT, &query);
        if (query)
        {
            query->Issue(D3DISSUE_END);
            while (query->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
                Sleep(0);
            query->Release();
        }
    }
}

namespace Autoclicker
{
    bool bNeedsAutoclick = false;

    injector::hook_back<const char* (__fastcall*)(void*, void*, int, int)> hbGetMessage;
    const char* __fastcall GetMessageHook(void* _this, void* edx, int a3, int a4)
    {
        static constexpr std::string_view Notices[] =
        {
            "Restart game from your last checkpoint.",
            "Load successful.",
            "This game has an autosave feature.\r\nDo not turn off the power when the above icon is displayed.",
        };

        auto ret = hbGetMessage.fun(_this, edx, a3, a4);
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
            if (frames % 10)
            {
                pState->Gamepad.wButtons = 0x0000;
                if (frames >= 2000)
                {
                    frames = 0;
                    bNeedsAutoclick = false;
                }
            }
            frames++;
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

            // Skip the startup logos: the logo loader loads none of them
            if (settings.Bool("MAIN.SkipIntro"))
            {
                auto pattern = hook::pattern("74 20 83 FE 05 74 1B 8B 0D ? ? ? ?");
                if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Misc"))
                {
                    WriteGameValue<uint8_t>(pattern.get_first(0), 0xEB);
                    WriteGameValue<uint16_t>(pattern.get_first(0x29), 0x9090);
                }
            }

            // Episode 4 finale: the controls get stuck before the final boss. The resource loading thread waits
            // 121 ms instead of 1 ms when it has nothing to load.
            auto pattern = hook::pattern("85 FF 75 09 6A 01 FF D5 E9");
            if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Misc"))
                WriteGameValue<uint8_t>(pattern.get_first(5), 0x79);

            // MAIN.ForceLogo: the title logo of a region (the game picks it by the language)
            nForceLogo = std::clamp(settings.Int("MAIN.ForceLogo"), 0, 4);
            if (nForceLogo)
            {
                for (auto ref : {
                    "E8 ? ? ? ? 83 F8 03 77 19 FF 24 85 ? ? ? ? 6A 01 68 ? ? ? ? EB 10 6A 01 68 ? ? ? ? EB 07 6A 01 68 ? ? ? ? 8B 0D ? ? ? ? 68 ? ? ? ? 8B 01 FF 50 30 85 C0",
                    "E8 ? ? ? ? 83 F8 03 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 68 ? ? ? ? 8D 4C 24 10",
                    "E8 ? ? ? ? 83 F8 03 77 19 FF 24 85 ? ? ? ? 6A 01 68 ? ? ? ? EB 10 6A 01 68 ? ? ? ? EB 07 6A 01 68 ? ? ? ? 8B 0D ? ? ? ? 68 ? ? ? ? 8B 01 FF 50 30 8B E8",
                    "E8 ? ? ? ? 83 F8 03 0F 87 ? ? ? ? FF 24 85 ? ? ? ? 68 ? ? ? ? 8D 4C 24 18" })
                {
                    pattern = hook::pattern(ref);
                    if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Misc"))
                        injector::MakeCALL(pattern.get_first(), GetLogoRegion, true);
                }
            }

            // GPU command buffer sync: a performance workaround that skips the wait for the GPU
            bDisableCreateQuery = settings.Bool("MAIN.DisableCreateQuery");
            if (bDisableCreateQuery)
            {
                pattern = hook::pattern("51 80 B9 ? ? ? ? ? 8B 91");
                if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Misc"))
                    injector::MakeJMP(pattern.get_first(), GpuCommandBufferSync, true);
            }

            if (settings.Bool("MAIN.Autoclicker"))
            {
                Autoclicker::pXInputGetState = std::move(IATHook::Replace(GetModuleHandleA(NULL), "XINPUT1_3.dll",
                    std::forward_as_tuple("XInputGetState@2", Autoclicker::XInputGetStateHook)
                )["XInputGetState@2"]);

                pattern = hook::pattern("E8 ? ? ? ? 8B D0 85 D2 75 13 8B 07 8B CF FF 50 60 56 FF 15 ? ? ? ? 5E 5F C2 0C 00 8B CA 53 8D 59 01 8D A4 24");
                if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: Misc"))
                    Autoclicker::hbGetMessage.fun = injector::MakeCALL(pattern.get_first(), Autoclicker::GetMessageHook, true).get();
            }
        };
    }
} Misc;
