module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>

export module Misc;

import ComVars;

// Skip intro, the title logo, the episode 4 fix and the GPU sync workaround.

namespace
{
    int nForceLogo = 0;
    int __cdecl GetLogoRegion()
    {
        return nForceLogo - 1;
    }

    GameRef<uintptr_t> TitleInputSystem;
    bool bDisableCreateQuery = false;
    uintptr_t pOnlineNoticeDone = 0;
    SafetyHookInline shOnlineEntry;
    void* pInitialOnlineEntry = nullptr;
    bool bInitialOnlineComplete = false;
    thread_local bool bInInitialOnlineEntry = false;

    void __fastcall OnlineEntry(void* object, void*)
    {
        if (!pInitialOnlineEntry && !bInitialOnlineComplete)
            pInitialOnlineEntry = object;
        const bool initial = !bInitialOnlineComplete && pInitialOnlineEntry == object;
        const bool previous = bInInitialOnlineEntry;
        bInInitialOnlineEntry = initial;
        shOnlineEntry.unsafe_thiscall(object);
        bInInitialOnlineEntry = previous;
        if (initial && *(static_cast<uint8_t*>(object) + 4) == 8)
        {
            bInitialOnlineComplete = true;
            pInitialOnlineEntry = nullptr;
        }
    }
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
                injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true);
                injector::WriteMemory<uint16_t>(pattern.get_first(0x29), 0x9090, true);

                // The startup autosave notice has only an acknowledgement. Follow the game's existing path for
                // continuing without that notice; the save and profile selection remains untouched.
                auto notice = hook::pattern("A1 ? ? ? ? 8A 80 CC 0C 00 00 84 C0 0F 85 ? ? ? ? A1 ? ? ? ? 8A 40 20 84 C0 74 ? B9 ? ? ? ?");
                auto continuation = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 75 15 A1 ? ? ? ? 8A 40 20 84 C0 74 09 C6 46 05 08");
                injector::MakeJMP(notice.get_first(), continuation.get_first(), true);

                // The initial RE.NET service-unavailable message has no choice. Follow the existing completion path
                // once, before the game creates that message; later online messages keep their normal behavior.
                auto onlineNotice = hook::pattern("5F 5E C3 83 FF 02 75 47 8B 0D ? ? ? ? E8 ? ? ? ? 8B F8 85 FF 74 E0 81 4F 0C 00 20 00 00 6A 06 6A 0B");
                auto epilogue = onlineNotice.get_first<uint8_t>();
                pOnlineNoticeDone = reinterpret_cast<uintptr_t>(epilogue);
                auto onlineEntry = hook::pattern("56 8B F1 E8 ? ? ? ? 8B 46 04 25 FF 00 00 00 83 F8 08 77 ? FF 24 85 ? ? ? ?");
                shOnlineEntry = safetyhook::create_inline(onlineEntry.get_first(), OnlineEntry);
                static auto skipOnlineNotice = safetyhook::create_mid(epilogue + 3, [](SafetyHookContext& regs)
                {
                    if (bInInitialOnlineEntry && regs.edi == 2)
                    {
                        *reinterpret_cast<uint8_t*>(regs.esi + 4) = 8;
                        regs.eip = pOnlineNoticeDone;
                    }
                });
            }

            // The title prompt's first input check advances to the normal profile and menu flow.
            if (settings.Bool("MAIN.SkipPressStart"))
            {
                auto input = hook::pattern("A1 ? ? ? ? 83 B8 F0 08 00 00 01 75 ? 8B 81 AC 02 00 00 C3");
                TitleInputSystem.SetAddress(*input.get_first<uintptr_t*>(1));
                auto press = hook::pattern("8B 03 FF 90 E0 00 00 00 A9 08 F0 00 00 75 ? 80 BB D0 02 00 00 00 0F 84");
                static SafetyHookMid PressStartHook = safetyhook::create_mid(press.get_first(8), [](SafetyHookContext& regs)
                {
                    static bool skipped = false;
                    const uintptr_t inputSystem = TitleInputSystem;
                    if (!skipped && inputSystem && *reinterpret_cast<const uint32_t*>(inputSystem + 0x8F0) == 1)
                    {
                        skipped = true;
                        regs.eax |= 8;
                    }
                });
            }

            // Episode 4 finale: the controls get stuck before the final boss. The resource loading thread waits
            // 121 ms instead of 1 ms when it has nothing to load.
            auto pattern = hook::pattern("85 FF 75 09 6A 01 FF D5 E9");
            injector::WriteMemory<uint8_t>(pattern.get_first(5), 0x79, true);

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
                    injector::MakeCALL(pattern.get_first(), GetLogoRegion, true);
                }
            }

            // GPU command buffer sync: a performance workaround that skips the wait for the GPU
            bDisableCreateQuery = settings.Bool("MAIN.DisableCreateQuery");
            if (bDisableCreateQuery)
            {
                pattern = hook::pattern("51 80 B9 ? ? ? ? ? 8B 91");
                injector::MakeJMP(pattern.get_first(), GpuCommandBufferSync, true);
            }


        };
    }
} Misc;
