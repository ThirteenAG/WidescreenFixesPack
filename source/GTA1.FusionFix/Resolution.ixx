module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"

export module Resolution;

import ComVars;
import Modes;
import Options;

// The configured resolution in the display mode switch, and crash fixes of the larger resolutions
export namespace GTA1Resolution
{
    // Runtime resolution change from the options screen, between frontend frames.
    bool ApplyResolution(int width, int height)
    {
        Screen.Width = width; Screen.Height = height;
        Screen.Width43 = static_cast<int32_t>(height * (4.0f / 3.0f));
        GTA1Modes::Select(width, height);
        if (InputDevices::Frontend)
        {
            if (!GTA1Modes::Reload)
                return false;
            GTA1Modes::Reload(-1);
            return true;
        }
        if (!GTA1Modes::ApplyGameplay())
            return false;
        if (GTA1Options::Redraw)
            GTA1Options::Redraw();
        return true;
    }

    // The mode switch stores the configured size
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            nWidth = Screen.Width;
            dword_787370 = Screen.Width;
            regs.eax = Screen.Height;
            auto dc = reinterpret_cast<int*>(regs.ebx);
            dc[108] = Screen.Width - 1; dc[109] = Screen.Height - 1;
            dc[91] = dc[144] = Screen.Width;
            dc[92] = dc[145] = Screen.Height;
        }
    };

    struct SetResHookX1
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_504CC0 = Screen.Width;
        }
    };

    struct SetResHookX2
    {
        void operator()(injector::reg_pack& regs)
        {
            nScreenWidth = Screen.Width;
        }
    };

    void FrontendStride(SafetyHookContext& context)
    {
        int* dc = pOffscreenDC;
        if (dc && !GTA1Options::OverlayStride) context.ebx = dc[114] / ((dc[110] + 7) / 8);
    }

    // Fix crash in FMV (cut scenes) due to the resolution change
    int n480 = 480;

    void InitGtaClassicsSteam()
    {
        // Patches:
        // - GTA1 Rockstar Classics & Steam: SHA256=6A5C313D9C0D87B4EFA527BAAC7097DD4A149B9B2DA5261ABB224083F5CDC1C2 (774144 bytes)
        // - GTA London 1969 / 1961 Rockstar Classics (WINO, 880640 bytes) use this same code.

        //wndmode crashfix (caused by the ASI loader)
        auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 B8 01 00 00 00 5D 5F 5E 5B 81 C4 70 05 00 00"); //0x49225B
        injector::MakeNOP(pattern.get_first(0), 5, true);

        //Player_a bugfix (only in the Rockstar Classics / Steam version)
        //see technical details here: https://misc.daniel-marschall.de/spiele/gta1/player_a_bugfix.txt
        pattern = hook::pattern("3B C2 0F 9D C2 FE CA 83 E2 06");
        {
            struct patch_player_a_new
            {
                void operator()(injector::reg_pack& regs)
                {
                    // mov     al, [ecx] ; ecx is a pointer to the saved resolution (values 0..5)
                    // cmp     eax, edx  ; <-- Patch: mov edx, eax
                    // setnl   dl        ; <-- Patch: nop
                    // dec     dl        ; <-- Patch: nop
                    // and     edx, 6    ; <-- Patch: nop (Bug is in this line: Resolution "6" does not exist!)
                    // mov     byte ptr byte_00503224, dl  ; In byte_00503224, the to-be-applied resolution will be saved
                    regs.edx = regs.eax;
                }
            };
            injector::MakeInline<patch_player_a_new>(pattern.get_first(0), pattern.get_first(10));
        }

        //Res change
        pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? 8B 83 B4 01 00 00 40"); //0x491E4C
        nWidth.SetAddress(*pattern.get_first<uint32_t*>(1));
        dword_787370.SetAddress(*pattern.get_first<uint32_t*>(6));
        injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(17));

        pattern = hook::pattern("89 B1 ? ? ? ? D1 F8 89 81"); //0x46453B
        static auto dword_74F168 = *pattern.get_first<uint32_t>(2);
        struct SetResHook43
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = Screen.Width43;
                *(uint32_t*)(regs.ecx + dword_74F168) = regs.esi;
            }
        }; injector::MakeInline<SetResHook43>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 83 C4 08 40 83"); //0x414FF7
        dword_504CC0.SetAddress(*pattern.get_first<uint32_t*>(1));
        injector::MakeInline<SetResHookX1>(pattern.get_first(0));

        pattern = hook::pattern("89 15 ? ? ? ? 8B 40 0C A3 ? ? ? ? C3"); //0x43B7CF
        injector::MakeInline<SetResHookX2>(pattern.get_first(0), pattern.get_first(6));

        //Menu fix
        pattern = hook::pattern("BB ? ? ? ? 2B 5D 0C D1 E3 89 5D FC"); //0x4898C3
        static auto frontendStrideHook = safetyhook::create_mid(pattern.get_first(5), FrontendStride);

        pattern = hook::pattern("A1 ? ? ? ? 74 56 85 C0 0F");
        injector::WriteMemory<int*>(pattern.get_first<int*>(1), &n480, true); //42D83A
        pattern = hook::pattern("3B 15 ? ? ? ? 7C B7 5B 5F 5E C3");
        injector::WriteMemory<int*>(pattern.get_first<int*>(2), &n480, true); //42D88B
        pattern = hook::pattern("3B 15 ? ? ? ? 7C C7 5F 5E C3");
        injector::WriteMemory<int*>(pattern.get_first<int*>(2), &n480, true); //42D8CC
    }

    // TODO: GTA 1 Retail, London 1969 and London 1961:
    //       The game crashes if a cut scene is replayed (space bar), most likely because of a timer issue:
    //       They are heavily accelerated (like the menu).
    //       The cut scenes do work when they are triggered via mission complete.

    void InitLondonRetail()
    {
        // Patches:
        // - GTA London 1969 Retail (CD): SHA256=1FA3934F330B0097A0CB8160E3968D78E93CB4A808703AD4854BDB1F87BAE39A (1231360 bytes)
        // - GTA London 1961 (Download):  SHA256=549020E49AAD17D1EF26BC5766A8374503C5A5936251B793BF8BE2A1D90700C5 (1235968 bytes)
        // - GTA London 1969 no-CD:       SHA256=5B976387530EB2FC2BC0225521E268BC5A736E3D06D3F471FF0F9874573DE324 (1232384 bytes)
        // - GTA London 1961 no-CD:       SHA256=6FA6727CDB931FA7CA782ECD0E9676D3E5882C075E2F442343BF8A8285FE5D2D (1236992 bytes)
        // - GTA London 1961:             SHA256=AD39E8EBD1385D20B692C913C3913D74530FBA62A3B4803936F7FE6870D9B6DD (1235968 bytes)
        // SafeDisc and other wrapped retail executables are not supported.

        //wndmode crashfix (caused by the ASI loader)
        auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 B8 01 00 00 00 5D 5F 5E 5B 81 C4 70 05 00 00");
        injector::MakeNOP(pattern.get_first(0), 5, true);

        //Res change
        pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? 8B 83 B4 01 00 00 40");
        nWidth.SetAddress(*pattern.get_first<uint32_t*>(1));
        dword_787370.SetAddress(*pattern.get_first<uint32_t*>(6));
        injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(17));

        pattern = hook::pattern("8B 81 ? ? ? ? 99 2B C2 D1 F8");
        static auto dword_80CF48 = *pattern.get_first<uint32_t>(2);
        struct SetResHook43
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.ecx + dword_80CF48);
                regs.esi = Screen.Width43;
                *(uint32_t*)(regs.ecx + dword_80CF48) = regs.esi;
            }
        }; injector::MakeInline<SetResHook43>(pattern.get_first(0), pattern.get_first(6));

        pattern = hook::pattern("A1 ? ? ? ? 83 C4 08 40 83");
        dword_504CC0.SetAddress(*pattern.get_first<uint32_t*>(1));
        injector::MakeInline<SetResHookX1>(pattern.get_first(0));

        pattern = hook::pattern("89 15 ? ? ? ? 8B 40 0C A3 ? ? ? ? C3");
        injector::MakeInline<SetResHookX2>(pattern.get_first(0), pattern.get_first(6));

        //Menu fix
        pattern = hook::pattern("BB ? ? ? ? 2B 5D 0C D1 E3 89 5D FC");
        static auto frontendStrideHook = safetyhook::create_mid(pattern.get_first(5), FrontendStride);

        pattern = hook::pattern("A1 ? ? ? ? 33 D2 85 C0 0F ? ? ? ?");
        injector::WriteMemory<int*>(pattern.get_first<int*>(1), &n480, true);
        pattern = hook::pattern("42 F3 A4 3B 15 ? ? ? ? 7C B7 5F 5E");
        injector::WriteMemory<int*>(pattern.get_first<int*>(5), &n480, true);
        pattern = hook::pattern("A1 ? ? ? ? 33 D2 85 C0 7E 39 8B FA");
        injector::WriteMemory<int*>(pattern.get_first<int*>(1), &n480, true);
        pattern = hook::pattern("42 F3 AA 3B 15 ? ? ? ? 7C C7 5F 5E");
        injector::WriteMemory<int*>(pattern.get_first<int*>(5), &n480, true);
    }

    void InitGtaRetail()
    {
        // Patches:
        // - GTA1 Retail (CD): SHA256=5AF3C1D90A85F694B184F3FC242E5F8F13B01BFE9BC706C655E97B7BC21599C8 (909824 bytes)

        //wndmode crashfix (caused by the ASI loader)
        auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 B8 01 00 00 00 5D 5F 5E 5B 81 C4 70 05 00 00");
        injector::MakeNOP(pattern.get_first(0), 5, true);

        //Res change
        pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? 8B 83 B4 01 00 00 40");
        nWidth.SetAddress(*pattern.get_first<uint32_t*>(1));
        dword_787370.SetAddress(*pattern.get_first<uint32_t*>(6));
        injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(17));

        pattern = hook::pattern("8B 86 ? ? ? ? 99 2B C2 D1 F8 8D 0C 49");
        static auto dword_6575E8 = *pattern.get_first<uint32_t>(2);
        struct SetResHook43
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.esi + dword_6575E8);
                regs.ecx = Screen.Width43;
                *(uint32_t*)(regs.esi + dword_6575E8) = regs.ecx;
            }
        }; injector::MakeInline<SetResHook43>(pattern.get_first(0), pattern.get_first(6));

        // The width store of the mode switch (MGL max x + 1).
        pattern = hook::pattern("A3 ? ? ? ? 51 E8 ? ? ? ? 83 C4 04 40 A3");
        dword_504CC0.SetAddress(*pattern.get_first<uint32_t*>(1));
        injector::MakeInline<SetResHookX1>(pattern.get_first(0));

        pattern = hook::pattern("89 15 ? ? ? ? 8B 40 0C A3 ? ? ? ? C3");
        injector::MakeInline<SetResHookX2>(pattern.get_first(0), pattern.get_first(6));

        //Menu fix
        pattern = hook::pattern("BB ? ? ? ? 2B 5D 0C D1 E3 89 5D FC");
        static auto frontendStrideHook = safetyhook::create_mid(pattern.get_first(5), FrontendStride);

        pattern = hook::pattern("A1 ? ? ? ? 89 54 24 10 3B C2 0F 8E 95 00 00 00");
        injector::WriteMemory<int*>(pattern.get_first<int*>(1), &n480, true);
        pattern = hook::pattern("42 03 F3 F3 A5 3B 15 ? ? ? ? 7C B1");
        injector::WriteMemory<int*>(pattern.get_first<int*>(7), &n480, true);
        pattern = hook::pattern("33 D2 A1 ? ? ? ? 3B C2 7E 33");
        injector::WriteMemory<int*>(pattern.get_first<int*>(3), &n480, true);
        pattern = hook::pattern("F3 AB 3B 15 ? ? ? ? 7C CD");
        injector::WriteMemory<int*>(pattern.get_first<int*>(4), &n480, true);
    }

    // The executable's own patches
    void Install()
    {
        switch (GTA1Build::Current)
        {
            case GTA1Build::Kind::London: InitLondonRetail(); break;
            case GTA1Build::Kind::Retail: InitGtaRetail(); break;
            default: InitGtaClassicsSteam(); break;
        }
    }
}
