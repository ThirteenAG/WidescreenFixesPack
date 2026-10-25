module;

#include <stdafx.h>
#include <usercall.hpp>

export module DebugObjects;

import ComVars;

using namespace usercall;

namespace DebugTags
{
    //by xan1242
    //https://github.com/ThirteenAG/WidescreenFixesPack/issues/643#issuecomment-1083564196
    void __stdcall CorruptAllFEDevObjects(const char* pkgname)
    {
        // so in order to hide the silly little side-tags used during FEng development, an attack at its FEObjects is necessary
        // all of them, and I mean each and every one of the tags' text object hashes and name is "ConduitMdITC_TT21i"
        // because of this, the game only references the first one in memory (which is usually next_tag), so FEngSetInvisible doesn't work
        // ...unless we invalidate/corrupt the NameHash on purpose and that's exactly what this function will do

        // this may be a lot and potentially taxing but it's the best we can do without editing all FNGs themselves
        // hide ALL of those red blocks
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x8BDC3C7D);	// QUIT_TAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x3A8A5C3E);	// QUITTAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0xA1D233EB);	// BACK_TAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x88C81DEC);	// BACKTAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x1A705339);	// NEXT_TAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x2F588B7A);	// NEXTTAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0xFF4BB93F);	// DISPLAYMENUBAR
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0xD0257F7D);	// DISPLAY_MENUBAR_
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x3AE43C3E);	// DeleteProfile_tag
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x28894A9D);	// CUSTOMIZE_TAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x10BE265E);	// CUSTOMIZETAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0xA8ED017E);	// STOCK_TAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x2426051F);	// STOCKTAG
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x7BD9401F);	// changecolor_tag
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x134473A0);	// changecolortag
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x5B393372);	// DecalColor_tag
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0xDBF9C893);	// DecalColortag
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x502AFE6C);	// performance_tag
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x50011C4D);	// performancetag
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x677392C4);  // defaultcontsettings_tag

        // hide all text
        // there are many duplicates... eww...
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x8C4C21FA); // ConduitMdITC_TT21i(88)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x8C4C2533); // ConduitMdITC_TT21i(90)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x8C4C2554); // ConduitMdITC_TT21i(91)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x8C4C2575); // ConduitMdITC_TT21i(92)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x8C4C2596); // ConduitMdITC_TT21i(93)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x8C4C25B7); // ConduitMdITC_TT21i(94)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x15CC69FE); // ConduitMdITC_TT21i(103)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x15CC6A82); // ConduitMdITC_TT21i(107)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x15CC6AA3); // ConduitMdITC_TT21i(108)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x15CC6AC4); // ConduitMdITC_TT21i(109)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x15CC6DDC); // ConduitMdITC_TT21i(110)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x15CC6EC3); // ConduitMdITC_TT21i(117)
        usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, 0x15CC6EE4); // ConduitMdITC_TT21i(118)

        // there are exceptions we have to watch out for (we're just gonna use the game's hashing function to quickly compare strings)
        // anything that breaks goes here
        // hash can be calclulated simply by using bStringHash or this FEHashUpper ripped from the game
        switch (usercall::CallAndRet<unsigned int, reg::edx>(FEHashUpper, pkgname))
        {
            case 0xAB447B38: // PC_Loading.fng -- removes the loading text (if it would exist, e.g. the beta loading screen)
            case 0x2729D8C3: // LS_Splash_PC.fng -- removes the copyright text
            case 0x41613BD0: // MU_Online_NetworkLogin.fng -- removes some buttons
            case 0x630135FF: // MU_QuickRaceOptions_PC.fng -- removes some buttons
                return;
        }

        unsigned int obj = 0;
        unsigned int targetHash = usercall::CallAndRet<unsigned int, reg::edx>(FEHashUpper, "ConduitMdITC_TT21i");
        char* pkg = (char*)usercall::CallAndRet<unsigned int, reg::eax>(FEPkgMgr_FindPackage, pkgname);

        // we search for every single test object until it's done (FEngFindObject returns 0 if it can't find any)
        while ((obj = usercall::CallAndRet<unsigned int, reg::edx, reg::ecx>(FEngFindObject, pkg, targetHash)) != 0)
        {
            usercall::Call<reg::esi, reg::edi>(FEngSetInvisible, pkgname, targetHash);
            *(int*)(obj + 0x10) = 0xDEADBEEF;
        }
    }
}

class DebugObjects
{
public:
    DebugObjects()
    {
        WFP::onInitEvent() += []()
        {
            // FEPkgMgr_SendMessageToPackage talks to the whole front end, so it's an ideal place to affect it globally
            onMenuScreenBaseNotify() += [](MenuScreen* menu)
            {
                DebugTags::CorruptAllFEDevObjects(menu->PackageFilename);
            };
        };
    }
} DebugObjects;