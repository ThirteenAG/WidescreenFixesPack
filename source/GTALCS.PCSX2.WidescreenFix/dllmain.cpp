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

    static auto ps2 = PCSX2({ 0x7EA439F5 }, NULL, [](PCSX2& ps2)
    {
        float intResX = ps2.GV({ 640.0f, 512.0f });
        float intResY = ps2.GV({ 480.0f, 384.0f });

        Screen.nWidth = ps2.GetWindowWidth();
        Screen.nHeight = ps2.GetWindowHeight();
        Screen.fAspectRatio = ps2.GetAspectRatio();
        Screen.fWidth = static_cast<float>(Screen.nWidth);
        Screen.fHeight = static_cast<float>(Screen.nHeight);
        Screen.fHudScale = (((intResX / intResY)) / (Screen.fAspectRatio));
        Screen.fHudOffset = (((intResY * Screen.fAspectRatio) - intResX) / 2.0f) * Screen.fHudScale;
    });

    while (!ps2.isCRCValid())
        std::this_thread::yield();

    ps2.EnableCallback();

    ps2.vecPatches.push_back(PCSX2Memory(L"gametitle=Grand Theft Auto: Liberty City Stories " + ps2.GV({ std::wstring(L"(SLUS_21423)") })));
    ps2.vecPatches.push_back(PCSX2Memory(L"comment=Widescreen Fix by ThirteenAG https://thirteenag.github.io/wfp#gtalcs"));
    ps2.vecPatches.push_back(PCSX2Memory(L""));
    ps2.vecPatches.push_back(PCSX2Memory(L"// Selected Resolution: " + std::to_wstring(Screen.nWidth) + L"x" + std::to_wstring(Screen.nHeight) + L", Aspect Ratio: " + std::to_wstring(Screen.fAspectRatio)));
    ps2.vecPatches.push_back(PCSX2Memory(L""));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x1F35EC }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        nop(buf);
    }),
    L"// Skip Intro"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x291994 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        li2(buf, t9, AdjustFOV(70.0f, Screen.fAspectRatio) / 70.0f);
        mtc1(buf, t9, f30);
        muls(buf, f12, f12, f30);

        swc1(buf, f12, gp, -0x1AE8);
    }),
    L"// FOV Scaling"));

    struct ws
    {
        enum _instr
        {
            ASPECT_RATIO,
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
        {0x2083C4, ws::ASPECT_RATIO, (16.0f / 9.0f)},
    };

    for (auto& w : lui_ori_addr_array)
    {
        switch (w.instr)
        {
        case ws::ASPECT_RATIO:
            ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ w.ptr + 4 }), WORD_T, MAKE_INLINE, mips_asm([&w](oss& buf)
            {
                li2(buf, at, Screen.fAspectRatio);
            }),
            L"// ASPECT_RATIO"));
            break;
        default:
            break;
        }
    }

    ps2.WritePnach();
    ps2.WriteMemoryLoop();
}