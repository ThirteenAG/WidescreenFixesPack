module;

#include <stdafx.h>
#include <atomic>
#include "FixSettings.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"

export module Quicksave;

import ComVars;

// MISC.QuicksaveKey: saves the game outside missions
export namespace GTA2Quicksave
{
    int sampledKey = -1;
    bool held = false;
    bool ignoreUntilRelease = false;

    bool Released(int key, bool down)
    {
        if (key != sampledKey)
        {
            sampledKey = key;
            ignoreUntilRelease = down;
            held = false;
        }
        if (ignoreUntilRelease)
        {
            ignoreUntilRelease = down;
            return false;
        }
        return std::exchange(held, down) && !down;
    }

    std::atomic<bool> PadRequest = false;       // Back + Start on a pad

    uint32_t dword_45E510 = 0;                  // the save
    uint32_t dword_5EC070 = 0;
    uint32_t dword_47EF40 = 0;
    uint32_t dword_4C6750 = 0;                  // the text display

    struct QuicksaveHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = *(uint32_t*)(regs.ebx + 4);
            regs.edi = 0;

            const bool padRequest = PadRequest.exchange(false);
            if (!InputDevices::Focused())
            {
                Released(-1, false);
                return;
            }
            const int key = nQuicksaveKey;
            const bool down = key && (GetAsyncKeyState(key) & 0x8000);
            if (Released(key, down) || padRequest)
            {
                uint32_t missionFlag = **(uint32_t**)(dword_6644BC + 0x344);
                uint32_t isMP = nMultiplayer;
                if (!missionFlag && !isMP)
                {
                    //injector::thiscall<int(int, int)>::call(0x4105B0, 0x5D85A0, 0x3D); //sfx

                    auto i = injector::thiscall<uint32_t(uint32_t)>::call(dword_45E510, dword_5EC070); //save
                    injector::thiscall<uint32_t(uint32_t, uint32_t)>::call(dword_47EF40, uint32_t(dword_6644BC), i);

                    injector::thiscall<uint32_t(uint32_t, uint32_t, const char*)>::call(dword_4C6750, dword_672F40 + 0xE4, 1, "svdone"); // text display
                }
            }

        }
    };

    void Install()
    {
        //injector::WriteMemory(0x47FEDC, 0, true);
        //injector::WriteMemory(0x47FEF5, 500, true);

        dword_45E510 = (uint32_t)hook::get_pattern("8D 81 00 03 00 00 C3", 0);
        dword_5EC070 = *(uint32_t*)hook::get_pattern("B9 ? ? ? ? E8 ? ? ? ? 66 0F B6", 1);
        dword_47EF40 = (uint32_t)hook::get_pattern("83 EC 18 53 8B 5C 24 20 55 8B", 0);
        dword_6644BC.SetAddress(*(uint32_t*)hook::get_pattern("8B 15 ? ? ? ? 8B 82 38 03 00", 2));
        dword_4C6750 = (uint32_t)hook::get_pattern("8B 44 24 08 8B 54 24 04 6A FF 50 52", 0);
        dword_672F40.SetAddress(*(uint32_t*)hook::get_pattern("8B 0D ? ? ? ? 56 68 ? ? ? ? 6A 01", 2));
        nMultiplayer.SetAddress(*(uint32_t*)hook::get_pattern("A1 ? ? ? ? 85 C0 75 ? 8A 41 30", 1));

        auto pattern = hook::pattern("8B 73 04 33 FF 3B F7 66 89 BB E8"); //0x481380
        injector::MakeInline<QuicksaveHook>(pattern.get_first(0));
    }
}
