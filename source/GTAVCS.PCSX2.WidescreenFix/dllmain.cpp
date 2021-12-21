#include "stdafx.h"
#include "pcsx2/pcsx2.h"

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScale;
    float fHudOffset;
} Screen;

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (!PCSX2::pcsx2_crc_pattern.empty())
        {
            void PCSX2Thread();
            std::thread t(PCSX2Thread);
            t.detach();
        }
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent())
        {
            InitializeASI();
        }
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////
//////////////////////Experimental PCSX2 support///////////////////////
///////////////////////////////////////////////////////////////////////

void PCSX2Thread()
{
    using namespace mips;

    static auto ps2 = PCSX2({ 0x4F32A11F }, NULL, [](PCSX2& ps2)
    {
        float intResX = ps2.GV({ 640.0f, 640.0f });
        float intResY = ps2.GV({ 480.0f, 480.0f });

        Screen.nWidth = ps2.GetWindowWidth();
        Screen.nHeight = ps2.GetWindowHeight();
        Screen.fAspectRatio = ps2.GetAspectRatio();
        Screen.fWidth = static_cast<float>(Screen.nWidth);
        Screen.fHeight = static_cast<float>(Screen.nHeight);
        Screen.fHudScale = (((16.0f / 9.0f)) / (Screen.fAspectRatio));
        Screen.fHudOffset = ((((480.0f * (Screen.fAspectRatio)) - (480.0f * (16.0f / 9.0f)))) / 2.0f) / (Screen.fAspectRatio);
    });

    while (!ps2.isCRCValid())
        std::this_thread::yield();

    ps2.EnableCallback();

    ps2.vecPatches.push_back(PCSX2Memory(L"gametitle=Grand Theft Auto: Vice City Stories " + ps2.GV({ std::wstring(L"(SLUS_21590)") })));
    ps2.vecPatches.push_back(PCSX2Memory(L"comment=Widescreen Fix by ThirteenAG https://thirteenag.github.io/wfp#gtavcs"));
    ps2.vecPatches.push_back(PCSX2Memory(L""));
    ps2.vecPatches.push_back(PCSX2Memory(L"// Selected Resolution: " + std::to_wstring(Screen.nWidth) + L"x" + std::to_wstring(Screen.nHeight) + L", Aspect Ratio: " + std::to_wstring(Screen.fAspectRatio)));
    ps2.vecPatches.push_back(PCSX2Memory(L""));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x21C8F0 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        nop(buf);
    }),
    L"// Skip Intro"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x265448 }), WORD_T, LUI_ORI, &Screen.fAspectRatio,
        std::wstring(L"// Aspect Ratio: ") + std::to_wstring(Screen.fAspectRatio)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x37ADD4 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        li2(buf, t9, AdjustFOV(70.0f, Screen.fAspectRatio) / 70.0f);
        mtc1(buf, t9, f30);
        muls(buf, f12, f12, f30);
    
        lui(buf, v0, 0x48);
    
        swc1(buf, f12, v0, 0x7484);
    }),
    L"// FOV Scaling"));

    ///////////////////////////////////////
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x21CA34 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            addiu(buf, a1, zero, 1);
            sw(buf, a1, a0, 0x30);
        }),
        L"// Force Widescreen"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x2AF06C }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            addiu(buf, a1, zero, 1);
            sw(buf, a1, a0, 0x30);
        }),
        L"// Force Widescreen"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x3AD0F8 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            addiu(buf, a1, zero, 1);
            sw(buf, a1, a0, 0x30);
        }),
        L"// Force Widescreen"));

    
    static uint32_t xrefs[] = {
        //0x104C40, 0x104DE4, 0x106610, 0x1066F8, 0x107788, 0x1125A0, 0x112940, 0x1129B4, 0x1165B0, 0x1166C4, 0x1167D8, 
        //0x116D50, radar blips
        0x117040, 0x117084, 0x117A18, 0x1185D0, 0x11864C, //radar
        //0x1186F0, 0x1189E4, 0x118AB8, 0x118B8C, 0x118C60, 0x118D64, 0x118E68, 0x118F6C, 0x11907C, 0x119278, 0x119868, 0x1198B8, 0x156B18, 0x156D58, 0x1576E0,
        //0x157C88, 0x1679E0, 0x167CE4, 0x18C268, 0x18C29C, 0x18C2F0, 0x18E03C, 0x18E0A8, 0x1A9778, 0x1AAF0C, 0x1AAFBC, 0x21CE30, 0x21D278, 0x21D2E8, 0x21D6A0, 0x21D6D8,
        //0x21D740, 0x21D794, 0x21D8B0, 0x21DFFC, 0x21E068, 0x21E0B0, 0x21E100, 0x21E234, 0x21E338, 0x21E628, 0x21E660, 0x21F51C, 0x21F594, 0x21F5C8, 0x21F608, 0x21F63C,
        //0x21F6D8, 0x21F710, 0x21F8D4, 0x25CE50, 0x2617D4, 0x26315C, 0x263190, 0x2631B8, 0x2631E4, 0x26BB90, 0x26DD4C, 0x26DDBC, 0x26DFDC, 0x26E044, 0x26E3D8, 0x26E50C,
        //0x272F6C, 0x27334C, 0x28052C, 0x280578, 0x286FF0, 0x28708C, 0x2870E4, 0x2A999C, 0x2A99E8, 0x2A9A28, 0x2A9CE4, 0x2A9D38, 0x2A9DB0, 0x2ABB94, 0x2ABBBC, 0x2ABC28,
        //0x2AF97C, 0x2AFA18, 0x2AFAC8, 0x2F0B98, 0x31CBDC, 0x31CC48, 0x31CC74, 0x31CD78, 0x31CDBC, 0x31CF1C, 0x31CF54, 0x31CFB4, 0x31D05C, 0x31D0A4, 0x31D148, 0x31D170,
        //0x31D23C, 0x31D270, 0x31D81C, 
        0x31DD5C, 0x31DE7C, 0x31DFB8, // top...
        0x31E2CC, 0x31E70C, 0x31E82C, // ...right...
        0x31E968, 0x31F12C, 0x31F67C, // ...hud
        0x31FB64, 0x31FBB8, 0x31FC04, 0x31FC8C, //ammo text
        //0x3206B4, 0x320994, 0x320B84, 0x320D58, 0x320D80, 0x320EB8, 0x321090, 0x3210B8, 0x3211D0, 0x321248, 0x321340, 0x321478, 
         0x321650, //radardisc
        //0x3218A0, 0x32194C, 0x321980,
        //0x321AF4, 0x321B60, 0x321B8C, 0x321C54, 0x321C90, 0x3220EC, 0x322714, 0x322E40, 0x322FBC, 0x3232D4, 0x3233B8, 0x32350C, 0x3237CC, 0x323864, 0x3238D0, 0x32393C,
        //0x323C98, 0x323D30, 0x323D90, 0x323EB4, 0x3240BC, 0x324110, 0x32415C, 0x3241AC, 0x324208, 0x324264, 0x3242B0, 0x324300, 0x376CEC, 0x376D3C, 0x3A2B5C, 0x3B55D0,
        //0x3B56F8, 0x3B69C4, 0x3B69FC, 0x3B6A60, 0x3B8F24, 0x3B93D0, 0x3B946C, 0x3B94BC, 0x3B950C, 0x3B955C, 0x3BA4C0, 0x3D8270, 0x3DFEA0, 0x3F3C78, 0x3F3F28, 0x3F4594,
        //0x411DF0, 0x411F78, 0x41D3B0, 0x42E2D4, 0x42E3D4, 0x42E5CC, 0x42E628,
    };

    for (auto& i : xrefs)
    {
        ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ i }), WORD_T, MAKE_INLINE, mips_asm([&](oss& buf)
        {
            move(buf, v0, a0);

            li2(buf, t9, Screen.fHudScale);
            mtc1(buf, t9, f30);
            muls(buf, f12, f12, f30);
            muls(buf, f14, f14, f30);

            li2(buf, t9, Screen.fHudOffset);
            mtc1(buf, t9, f30);
            adds(buf, f12, f12, f30);
            adds(buf, f14, f14, f30);

            swc1(buf, f12, a0, 0);
            daddu(buf, v0, a0, zero);
            swc1(buf, f13, a0, 0xC);
            swc1(buf, f14, a0, 8); //
            swc1(buf, f15, a0, 4);
        }),
        L"// HUD"));
    }

    //ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x31F8D8 }), WORD_T, LUI_ORI, Screen.fHudScale * 0.2392578f, L"Ammo Font Scale"));
    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x2625AC }), WORD_T, LUI_ORI, Screen.fHudScale * 0.75f, L"Game Text Scale"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x1148d8 }), WORD_T, MAKE_INLINE, mips_asm([&](oss& buf)
    {
        adds(buf, f2, f2, f1);

        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        muls(buf, f2, f2, f30);
        muls(buf, f20, f20, f30);

        li2(buf, t9, Screen.fHudOffset);
        mtc1(buf, t9, f30);
        adds(buf, f2, f2, f30);
        adds(buf, f20, f20, f30);
    }),
    L"// Radar Scaling"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x116BE4 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        li2(buf, t9, Screen.fHudScale / 2.0f);
        mtc1(buf, t9, f30);

        cvtsw(buf, f3, f3);
        muls(buf, f3, f3, f30);

    }),
    L"// DrawRotatingRadarSprite"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x116d14 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        muls(buf, f30, f0, f30);

        subs(buf, f12, f12, f30);
    }),
    L"// DrawRadarSprite"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x116cf8 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        muls(buf, f30, f14, f30);

        adds(buf, f14, f12, f30);

    }),
    L"// DrawRadarSprite"));

    //
    //injector.MakeInlineLUIORI(ptr_1C1744, adjustRightX(422.0f, fHudScale)); // Single clip ammo number Left X
    //injector.MakeInlineLUIORI(ptr_1C1644, adjustRightX(425.0f, fHudScale)); // Ammo Left X
    //injector.MakeInlineLUIORI(ptr_1C162C, adjustRightX(449.0f, fHudScale)); // '-' Left X
    //injector.MakeInlineLUIORI(ptr_1C1688, adjustRightX(455.0f, fHudScale)); // Clip ammo Left X
    //injector.MakeInlineLUIORI(ptr_1C14DC, adjustRightX(473.0f, fHudScale)); // Clip ammo Right X
    //injector.MakeInlineLUIORI(ptr_1C13B8, fHudScale * 0.415f); // Font scale
    
    //ps2.WritePnach(); //broken, generated too early, some memory is still zeroes
    ps2.WriteMemoryLoop();
}