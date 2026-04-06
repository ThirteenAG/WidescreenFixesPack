module;

#include <stdafx.h>
#include <d3d9.h>

export module DebugObjects;

import ComVars;

namespace DebugTags
{
    //by xan1242
    //https://github.com/ThirteenAG/WidescreenFixesPack/issues/643#issuecomment-1083564196
    unsigned int FEngSetInvisible_Addr = 0x000495F70;
    void __stdcall FEngSetInvisible(const char* pkgname, unsigned int hash)
    {
        _asm
        {
            mov edi, hash
            mov esi, pkgname
            call FEngSetInvisible_Addr
        }
    }

    unsigned int FEHashUpper_Addr = 0x004FD230;
    unsigned int __stdcall FEHashUpper(const char* instr)
    {
        unsigned int result = 0;
        _asm
        {
            mov edx, instr
            call FEHashUpper_Addr
            mov result, eax
        }
        return result;
    }

    unsigned int FEngFindObject_Addr = 0x004FFB70;
    unsigned int __stdcall FEngFindObject(const char* pkg, int hash)
    {
        unsigned int result = 0;
        _asm
        {
            mov ecx, hash
            mov edx, pkg
            call FEngFindObject_Addr
            mov result, eax
        }
        return result;
    }

    unsigned int FEPkgMgr_FindPackage_Addr = 0x004F65D0;
    unsigned int __stdcall FEPkgMgr_FindPackage(const char* pkgname)
    {
        unsigned int result = 0;
        _asm
        {
            mov eax, pkgname
            call FEPkgMgr_FindPackage_Addr
            mov result, eax
        }
        return result;
    }

    void __stdcall CorruptAllFEDevObjects(char* pkgname)
    {
        // so in order to hide the silly little side-tags used during FEng development, an attack at its FEObjects is necessary
        // all of them, and I mean each and every one of the tags' text object hashes and name is "ConduitMdITC_TT21i"
        // because of this, the game only references the first one in memory (which is usually next_tag), so FEngSetInvisible doesn't work
        // ...unless we invalidate/corrupt the NameHash on purpose and that's exactly what this function will do

        unsigned int TestObj = 0;

        // this may be a lot and potentially taxing but it's the best we can do without editing all FNGs themselves
        // hide ALL of those red blocks
        FEngSetInvisible(pkgname, 0x8BDC3C7D);	// QUIT_TAG
        FEngSetInvisible(pkgname, 0x3A8A5C3E);	// QUITTAG
        FEngSetInvisible(pkgname, 0xA1D233EB);	// BACK_TAG
        FEngSetInvisible(pkgname, 0x88C81DEC);	// BACKTAG
        FEngSetInvisible(pkgname, 0x1A705339);	// NEXT_TAG
        FEngSetInvisible(pkgname, 0x2F588B7A);	// NEXTTAG
        FEngSetInvisible(pkgname, 0xFF4BB93F);	// DISPLAYMENUBAR
        FEngSetInvisible(pkgname, 0xD0257F7D);	// DISPLAY_MENUBAR_
        FEngSetInvisible(pkgname, 0x3AE43C3E);	// DeleteProfile_tag
        FEngSetInvisible(pkgname, 0x28894A9D);	// CUSTOMIZE_TAG
        FEngSetInvisible(pkgname, 0x10BE265E);	// CUSTOMIZETAG
        FEngSetInvisible(pkgname, 0xA8ED017E);	// STOCK_TAG
        FEngSetInvisible(pkgname, 0x2426051F);	// STOCKTAG
        FEngSetInvisible(pkgname, 0x7BD9401F);	// changecolor_tag
        FEngSetInvisible(pkgname, 0x134473A0);	// changecolortag
        FEngSetInvisible(pkgname, 0x5B393372);	// DecalColor_tag
        FEngSetInvisible(pkgname, 0xDBF9C893);	// DecalColortag
        FEngSetInvisible(pkgname, 0x502AFE6C);	// performance_tag
        FEngSetInvisible(pkgname, 0x50011C4D);	// performancetag
        FEngSetInvisible(pkgname, 0x677392C4);  // defaultcontsettings_tag

        // hide all text
        // there are many duplicates... eww...
        FEngSetInvisible(pkgname, 0x8C4C21FA); // ConduitMdITC_TT21i(88)
        FEngSetInvisible(pkgname, 0x8C4C2533); // ConduitMdITC_TT21i(90)
        FEngSetInvisible(pkgname, 0x8C4C2554); // ConduitMdITC_TT21i(91)
        FEngSetInvisible(pkgname, 0x8C4C2575); // ConduitMdITC_TT21i(92)
        FEngSetInvisible(pkgname, 0x8C4C2596); // ConduitMdITC_TT21i(93)
        FEngSetInvisible(pkgname, 0x8C4C25B7); // ConduitMdITC_TT21i(94)
        FEngSetInvisible(pkgname, 0x15CC69FE); // ConduitMdITC_TT21i(103)
        FEngSetInvisible(pkgname, 0x15CC6A82); // ConduitMdITC_TT21i(107)
        FEngSetInvisible(pkgname, 0x15CC6AA3); // ConduitMdITC_TT21i(108)
        FEngSetInvisible(pkgname, 0x15CC6AC4); // ConduitMdITC_TT21i(109)
        FEngSetInvisible(pkgname, 0x15CC6DDC); // ConduitMdITC_TT21i(110)
        FEngSetInvisible(pkgname, 0x15CC6EC3); // ConduitMdITC_TT21i(117)
        FEngSetInvisible(pkgname, 0x15CC6EE4); // ConduitMdITC_TT21i(118)

        // there are exceptions we have to watch out for (we're just gonna use the game's hashing function to quickly compare strings)
        // anything that breaks goes here
        // hash can be calclulated simply by using bStringHash or this FEHashUpper ripped from the game
        if (FEHashUpper(pkgname) == 0xAB447B38) // PC_Loading.fng -- removes the loading text (if it would exist, e.g. the beta loading screen)
            return;
        if (FEHashUpper(pkgname) == 0x2729D8C3) // LS_Splash_PC.fng -- removes the copyright text
            return;
        if (FEHashUpper(pkgname) == 0x41613BD0) // MU_Online_NetworkLogin.fng -- removes some buttons
            return;
        if (FEHashUpper(pkgname) == 0x630135FF) // MU_QuickRaceOptions_PC.fng -- removes some buttons
            return;

        // we search for every single test object until it's done (FEngFindObject returns 0 if it can't find any)
        while (FEngFindObject((char*)FEPkgMgr_FindPackage(pkgname), FEHashUpper("ConduitMdITC_TT21i")))
        {
            // once found, catch it in a variable
            TestObj = FEngFindObject((char*)FEPkgMgr_FindPackage(pkgname), FEHashUpper("ConduitMdITC_TT21i"));
            // before corrupting, hide it
            FEngSetInvisible(pkgname, FEHashUpper("ConduitMdITC_TT21i"));
            // after hiding, we can corrupt the hash, this is simply done by changing the NameHash at 0x10 of the FEObject (as per its class)
            *(int*)(TestObj + 0x10) = 0xDEADBEEF;
            // rip and tear... until it's done
        }
    }

    // FEPkgMgr_SendMessageToPackage talks to ALL of FrontEnd, so it's an ideal place to globally affect it
    unsigned int FEPkgMgr_SendMessageToPackage_Cave_Exit = 0x004F7D18;
    unsigned int FEPkgMgr_SendMessageToPackage_EAX = 0;
    unsigned int FEPkgMgr_SendMessageToPackage_ECX = 0;
    unsigned int FEPkgMgr_SendMessageToPackage_ESI = 0;
    void __declspec(naked) FEPkgMgr_SendMessageToPackage_Cave()
    {
        _asm
        {
            mov eax, [esp + 0x50]
            mov ecx, [esp + 0x4C]
            mov FEPkgMgr_SendMessageToPackage_EAX, eax
            mov FEPkgMgr_SendMessageToPackage_ECX, ecx
            mov FEPkgMgr_SendMessageToPackage_ESI, esi
        }
        CorruptAllFEDevObjects(*(char**)(FEPkgMgr_SendMessageToPackage_ESI + 0xC));
        _asm
        {
            mov eax, FEPkgMgr_SendMessageToPackage_EAX
            mov ecx, FEPkgMgr_SendMessageToPackage_ECX
            mov esi, FEPkgMgr_SendMessageToPackage_ESI
            jmp FEPkgMgr_SendMessageToPackage_Cave_Exit
        }
    }
}

void DrawRect(LPDIRECT3DDEVICE9 pDevice, int32_t x, int32_t y, int32_t w, int32_t h, D3DCOLOR color = D3DCOLOR_XRGB(0, 0, 0))
{
    D3DRECT BarRect = { x, y, x + w, y + h };
    pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 0, 0);
}

export void InitDebugObjects()
{
    CIniReader iniReader("");
    int nHideDebugObjects = iniReader.ReadInteger("MISC", "HideDebugObjects", 0);

    if (nHideDebugObjects)
    {
        if (nHideDebugObjects == 2)
        {
            //D3D9 EndScene Hook
            auto pattern = hook::pattern("A1 ? ? ? ? 8B 08 83 C4 04 50");
            static LPDIRECT3DDEVICE9* pDevice = *pattern.get_first<LPDIRECT3DDEVICE9*>(1);
            struct EndSceneHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.eax = *(uint32_t*)pDevice;
                    regs.ecx = *(uint32_t*)(regs.eax);

                    if (nGameState == 3)
                    {
                        DrawRect(*pDevice, 0, 0, static_cast<int32_t>(Screen.fHudOffsetReal), Screen.nHeight);
                        DrawRect(*pDevice, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal), 0, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal + Screen.fHudOffsetReal), Screen.nHeight);
                    }
                }
            }; injector::MakeInline<EndSceneHook>(pattern.get_first(0), pattern.get_first(7));
        }
        else
        {
            using namespace DebugTags;

            FEngSetInvisible_Addr = static_cast<decltype(FEngSetInvisible_Addr)>(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8A 43 61"))); //0x000495F70;
            FEHashUpper_Addr = static_cast<decltype(FEHashUpper_Addr)>(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8B 75 6C"))); //0x004FD230;
            FEngFindObject_Addr = static_cast<decltype(FEngFindObject_Addr)>(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 3B C6 75 16"))); //0x004FFB70;
            FEPkgMgr_FindPackage_Addr = static_cast<decltype(FEPkgMgr_FindPackage_Addr)>(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 8B 50 1C"))); //0x004F65D0;

            auto pattern = hook::pattern("8B 4C 24 4C 8B 16");
            FEPkgMgr_SendMessageToPackage_Cave_Exit = reinterpret_cast<decltype(FEPkgMgr_SendMessageToPackage_Cave_Exit)>(pattern.get_first(4)); //0x004F7D18;
            injector::MakeJMP(pattern.get_first(-4), FEPkgMgr_SendMessageToPackage_Cave, true);
        }
    }
}