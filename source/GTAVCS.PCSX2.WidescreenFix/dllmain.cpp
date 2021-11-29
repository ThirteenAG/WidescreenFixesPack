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
        CallbackHandler::RegisterCallback(L"WINSTA.dll", []() {
            if (!PCSX2::pcsx2_crc_pattern.empty())
            {
                void PCSX2Thread();
                std::thread t(PCSX2Thread);
                t.detach();
            }
        });
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
        Screen.fHudOffset = (((480.0f * (Screen.fWidth / Screen.fHeight)) - 853.0f)) / ((Screen.fWidth / Screen.fHeight) / (640.0f / 480.0f));
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

    //to do
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

    
    //////////////////////////////////////

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x37ADD4 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        li2(buf, t9, AdjustFOV(70.0f, Screen.fAspectRatio) / 70.0f);
        mtc1(buf, t9, f30);
        muls(buf, f12, f12, f30);
    
        lui(buf, v0, 0x48);
    
        swc1(buf, f12, v0, 0x7484);
    }),
    L"// FOV Scaling"));

    struct ws
    {
        enum _instr
        {
            LUI_ORI,
            LUI_LUI,
            LUI_SUB,
            LUI_ADD,
            ASPECT_RATIO,
            LI_V1,
            LI_A0,
            LI_A1,
            MOVN_S1_V1_V0,
            ADDIU_A1_S1,
            ADDIU_A0_S0,
            ADDIU_S4_S4
        };

        uintptr_t ptr;
        _instr instr;
        union
        {
            float f;
            int32_t i;
        } value;
    };

    static std::vector<ws> lui_ori_addr_array
    {
        //{0x1147A8, ws::LUI_ORI, 0.56f},
        //{0x114838, ws::LUI_ORI, 0.56f},
        //{0x1147B8, ws::LUI_ORI, -0.42561f},
        //{0x1149B0, ws::LUI_ORI, 0.56f},
        
        {0x115878, ws::LUI_ORI, 0.85f},
        {0x118454, ws::LUI_LUI, 3.0f},
        {0x118950, ws::LUI_LUI, 12.0f},
        {0x11896C, ws::LUI_LUI, 16.0f},
        {0x1189C8, ws::LUI_LUI, 12.0f},
        {0x118A40, ws::LUI_LUI, 12.0f},
        {0x118A9C, ws::LUI_LUI, 12.0f},
        {0x118B14, ws::LUI_LUI, 12.0f},
        {0x118B70, ws::LUI_LUI, 12.0f},
        {0x118BE8, ws::LUI_LUI, 12.0f},
        {0x118C44, ws::LUI_LUI, 12.0f},
        {0x118CB8, ws::LUI_LUI, 12.0f},
        {0x118D3C, ws::LUI_LUI, 4.0f},
        {0x118D54, ws::LUI_LUI, 12.0f},
        {0x118DBC, ws::LUI_LUI, 12.0f},
        {0x118E40, ws::LUI_LUI, 4.0f},
        {0x118E58, ws::LUI_LUI, 12.0f},
        {0x118EC0, ws::LUI_LUI, 12.0f},
        {0x118F44, ws::LUI_LUI, 4.0f},
        {0x118F5C, ws::LUI_LUI, 12.0f},
        {0x118FC4, ws::LUI_LUI, 12.0f},
        {0x119048, ws::LUI_LUI, 4.0f},
        {0x2625AC, ws::LUI_LUI, 0.75f}, //game text
        {0x26BB68, ws::LUI_LUI, 1.0f},
        {0x2AF900, ws::LUI_ORI, 0.68f},
        {0x2AF9F4, ws::LUI_ORI, 0.68f},
        {0x2AFAA4, ws::LUI_ORI, 0.68f},
        {0x3193DC, ws::LUI_LUI, 10.5f},
        {0x31DCC8, ws::LUI_ORI, 0.68f},
        {0x31DCFC, ws::LUI_ORI, 375.0f},
        {0x31DD30, ws::LUI_ORI, 375.0f},
        {0x31DDE4, ws::LUI_ORI, 375.0f},
        {0x31DE18, ws::LUI_ORI, 375.0f},
        {0x31DE50, ws::LUI_ORI, 43.52f},
        {0x31DF20, ws::LUI_ORI, 375.0f},
        {0x31DF54, ws::LUI_ORI, 375.0f},
        {0x31DF8C, ws::LUI_ORI, 43.52f},
        {0x31E274, ws::LUI_ORI, 375.0f},
        {0x31E2A8, ws::LUI_ORI, 439.0f},
        {0x31E678, ws::LUI_ORI, 0.68f},
        {0x31E6AC, ws::LUI_ORI, 375.0f},
        {0x31E6E0, ws::LUI_ORI, 375.0f},
        {0x31E794, ws::LUI_ORI, 375.0f},
        {0x31E7C8, ws::LUI_ORI, 375.0f},
        {0x31E800, ws::LUI_ORI, 43.52f},
        {0x31E8D0, ws::LUI_ORI, 375.0f},
        {0x31E904, ws::LUI_ORI, 375.0f},
        {0x31E93C, ws::LUI_ORI, 43.52f},
        {0x31EC78, ws::LUI_LUI, 386.0f},
        {0x31ECD0, ws::LUI_LUI, 386.0f},
        {0x31EEB4, ws::LUI_LUI, 386.0f},
        {0x31EFB0, ws::LUI_LUI, 8.25f}, //clock
        {0x31EFC8, ws::LUI_LUI, 9.0f},  //and
        {0x31F000, ws::LUI_LUI, 12.0f}, //money
        //{0x31F29C, ws::LUI_LUI, 48.0f}, //radar
        {0x31F5CC, ws::LUI_ORI, 465.0f}, //weapon icon pos
        {0x31F620, ws::LUI_LUI, 45.5f},//weapon icon scale
        {0x31F658, ws::LUI_LUI, 40.5f},
        {0x31FFA4, ws::LUI_LUI, 11.0f},
        {0x320B5C, ws::LUI_ORI, 415.0f},
        {0x321184, ws::LUI_LUI, 35.0f},
        {0x3211B8, ws::LUI_ORI, 425.0f},
        {0x32122C, ws::LUI_LUI, 426.0f},
        {0x3212A4, ws::LUI_LUI, 426.0f},
        {0x3212F0, ws::LUI_LUI, 426.0f},
        {0x321320, ws::LUI_LUI, 29.0f},
        {0x3220CC, ws::LUI_LUI, 193.0f},
        {0x322DF8, ws::LUI_LUI, 122.0f},
        {0x322F88, ws::LUI_LUI, 113.0f},
        {0x323284, ws::LUI_LUI, 208.0f},
        {0x3232B8, ws::LUI_LUI, 272.0f},
        {0x323368, ws::LUI_LUI, 208.0f},
        {0x32339C, ws::LUI_LUI, 272.0f},
        {0x3234AC, ws::LUI_ORI, 217.6f},
        {0x3234E4, ws::LUI_ORI, 262.4f},
        {0x323688, ws::LUI_LUI, 210.0f},
        {0x323718, ws::LUI_ORI, 178.5f},
        {0x32378C, ws::LUI_ORI, 180.75f},
        {0x323BD8, ws::LUI_LUI, 0.75f},

        //idk
        {0x119060, ws::LUI_LUI, 12.0f},
        {0x322E38, ws::LUI_ORI, 34.0f},
        
        //int stuff
        {0x21F6A8, ws::LI_V1, 0xAA},
        {0x31E0D0, ws::LI_A1, 0x181},
        {0x31E120, ws::LI_A1, 0x185},
        {0x31EA80, ws::LI_A1, 0x181},
        {0x31EAD0, ws::LI_A1, 0x185},
        {0x320E6C, ws::LI_A0, 0x19f},
        {0x321424, ws::LI_A0, 0x19f},
        
        //??
        {0x003b8660, ws::ADDIU_A1_S1, 0x6},
        {0x003b5914, ws::ADDIU_A0_S0, 0x6},
        {0x00118820, ws::ADDIU_S4_S4, 0x1},
        {0x001187b8, ws::ADDIU_S4_S4, 0x1},

        {0x265448, ws::ASPECT_RATIO, (16.0f / 9.0f)},

        //experimental
        {0x116cdc, ws::LUI_LUI, 6.0f}, //DrawRadarSprite
    };

    for (auto& w : lui_ori_addr_array)
    {
        switch (w.instr)
        {
        case ws::LUI_ORI:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr + 4 }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                li2(buf, at, w.value.f * Screen.fHudScale);
            }),
            L"// LUI_ORI"));
            break;
        case ws::LUI_LUI:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                li2(buf, at, w.value.f * Screen.fHudScale);
            }),
            L"// LUI_LUI"));
            break;
        case ws::ASPECT_RATIO:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr + 4 }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                li2(buf, at, Screen.fAspectRatio);
            }),
            L"// ASPECT_RATIO"));
            break;
        case ws::LI_A0:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                li(buf, a0, int32_t((float)w.value.i * Screen.fHudScale));
            }),
            L"// LI_A0"));
            break;
        case ws::LI_A1:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                li(buf, a1, int32_t((float)w.value.i * Screen.fHudScale));
            }),
            L"// LI_A1"));
            break;
        case ws::LI_V1:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                li(buf, v1, int32_t((float)w.value.i * Screen.fHudScale));
            }),
            L"// LI_V1"));
            break;
        case ws::ADDIU_A1_S1:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                addiu(buf, a1, s1, int16_t((float)w.value.i * Screen.fHudScale));
            }),
            L"// ADDIU_A1_S1"));
            break;
        case ws::ADDIU_A0_S0:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                addiu(buf, a0, s0, int16_t((float)w.value.i * Screen.fHudScale));
            }),
            L"// ADDIU_A0_S0"));
            break;
        case ws::ADDIU_S4_S4:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                addiu(buf, s4, s4, int16_t((float)w.value.i * Screen.fHudScale));
            }),
            L"// ADDIU_S4_S4"));
            break;
        default:
            break;
        }
    }

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x31F800 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHudOffset);
            mtc1(buf, t9, f30);
            adds(buf, f1, f1, f30);
            adds(buf, f12, f12, f30);

            swc1(buf, f12, sp, 0);
            swc1(buf, f1, sp, 0x8);
        }),
        L"// weapon icon pos"));


    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x3e057c }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHudOffset);
            mtc1(buf, t9, f30);
            adds(buf, f4, f4, f30);
            adds(buf, f3, f3, f30);

            cvtws(buf, f2, f3);
        }),
        L"// hud pos"));


    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x116BE4 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, t9, Screen.fHudScale / 2.0f);
            mtc1(buf, t9, f30);

            cvtsw(buf, f3, f3);
            muls(buf, f3, f3, f30);

        }),
        L"// blip rotating hook"));

    if (false)
    {
        ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x31f2c0 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, at, 48.0f / Screen.fHudScale);
            mtc1(buf, at, f0);
        }),
        L"// GetRadarSizeY"));


        ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x31f2e0 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, at, ((204.0f + 48.0f) - (48.0f / Screen.fHudScale)));
            mtc1(buf, at, f0);
        }),
        L"// GetRadarPosY"));
    }
    else
    {
        //breaks for some reason
        ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x31f2a0 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, at, 48.0f * Screen.fHudScale);
            mtc1(buf, at, f0);
        }),
        L"// GetRadarSizeX"));

        ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x31f2d0 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
        {
            li2(buf, at, 28.0f);
            mtc1(buf, at, f0);
        }),
        L"// GetRadarPosX"));
    }
    
    ps2.WritePnach();
    ps2.WriteMemoryLoop();
}