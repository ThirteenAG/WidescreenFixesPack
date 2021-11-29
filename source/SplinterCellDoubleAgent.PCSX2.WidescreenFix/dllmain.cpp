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
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////
//////////////////////Experimental PCSX2 support///////////////////////
///////////////////////////////////////////////////////////////////////

void PCSX2Thread()
{
    using namespace mips;

    static auto ps2 = PCSX2({ 0xC0498D24, 0xABE2FDE9 }, NULL, [](PCSX2& ps2)
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

    ps2.vecPatches.push_back(PCSX2Memory(L"gametitle=Tom Clancy's Splinter Cell Double Agent " + ps2.GV({ std::wstring(L"(SLUS-21356)"), std::wstring(L"(SLES-53827)") })));
    ps2.vecPatches.push_back(PCSX2Memory(L"comment=Widescreen Fix by ThirteenAG https://thirteenag.github.io/wfp#scdaps2"));
    ps2.vecPatches.push_back(PCSX2Memory(L""));
    ps2.vecPatches.push_back(PCSX2Memory(L"// Selected Resolution: " + std::to_wstring(Screen.nWidth) + L"x" + std::to_wstring(Screen.nHeight) + L", Aspect Ratio: " + std::to_wstring(Screen.fAspectRatio)));
    ps2.vecPatches.push_back(PCSX2Memory(L""));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x25F5A0, 0x25F610 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        lwc1(buf, f12, v0, 0xBC);
        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f31);
        muls(buf, f12, f12, f31);
    }),
    L"// 3D Scaling: " + std::to_wstring(Screen.fHudScale)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x2EACE4, 0x2EAD30 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        //filtering out 0-640 textures (fading etc)
        move(buf, k0, v0); //x
        move(buf, k1, v1); //w
        li2(buf, s6, 640);
        lh(buf, v0, sp, ps2.GV({ 0x12E, 0x13E }));
    }),
    L"// Hud Scaling Checks"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x2EACE8, 0x2EAD34 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        muls(buf, f0, f0, ps2.GV({ f21, f25 }));

        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        li2(buf, t9, Screen.fHudOffset);
        mtc1(buf, t9, f31);

        beq(buf, k0, zero, 2);
        nop(buf);
        b(buf, 1); //-->
        beq(buf, k1, s6, 3);
        nop(buf);  //<--
        muls(buf, f0, f0, f30);
        adds(buf, f0, f0, f31);
    }),
    L"// Hud Scaling 1 X1 = X1 / " + std::to_wstring(Screen.fHudScale) + L" + " + std::to_wstring(Screen.fHudOffset)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x2EAD10, 0x2EAD5C }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        muls(buf, f0, f0, ps2.GV({ f21, f25 }));

        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        li2(buf, t9, Screen.fHudOffset);
        mtc1(buf, t9, f31);

        beq(buf, k0, zero, 2);
        nop(buf);
        b(buf, 1); //-->
        beq(buf, k1, s6, 3);
        nop(buf);  //<--
        muls(buf, f0, f0, f30);
        adds(buf, f0, f0, f31);
    }),
    L"// Hud Scaling 1 X2 = X2 / " + std::to_wstring(Screen.fHudScale) + L" + " + std::to_wstring(Screen.fHudOffset)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x2EADA0, 0x2EAE10 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        mtc1(buf, ps2.GV({ t2, a0 }), f0);

        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        li2(buf, t9, Screen.fHudOffset);
        mtc1(buf, t9, f31);

        //coop goggles overlay check (FColor, animation still broken)
        li2(buf, t9, 0x96DAFAEC);
        beq(buf, a1, t9, 11);
        nop(buf);
        li2(buf, t9, 0x96C3B081);
        beq(buf, a1, t9, 7);
        nop(buf);
        li2(buf, t9, 0x00C3B081);
        beq(buf, a1, t9, 3);
        nop(buf);

        muls(buf, f12, f12, f30);
        adds(buf, f12, f12, f31);
    }),
    L"// Hud Scaling 2 X1 = X1 / " + std::to_wstring(Screen.fHudScale) + L" + " + std::to_wstring(Screen.fHudOffset)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x2EADD0, 0x2EAE20 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        li2(buf, t9, Screen.fHudOffset);
        mtc1(buf, t9, f31);

        //coop goggles overlay check (FColor, animation still broken)
        li2(buf, t9, 0x96DAFAEC);
        beq(buf, a1, t9, 11);
        nop(buf);
        li2(buf, t9, 0x96C3B081);
        beq(buf, a1, t9, 7);
        nop(buf);
        li2(buf, t9, 0x00C3B081);
        beq(buf, a1, t9, 3);
        nop(buf);

        muls(buf, f16, f16, f30);
        adds(buf, f16, f16, f31);

        swc1(buf, f16, sp, 0);
    }),
    L"// Hud Scaling 2 X2 = X2 / " + std::to_wstring(Screen.fHudScale) + L" + " + std::to_wstring(Screen.fHudOffset)));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0, 0x2EAFD8 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        //only for EU version
        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        li2(buf, t9, Screen.fHudOffset);
        mtc1(buf, t9, f31);

        muls(buf, f12, f12, f30);
        adds(buf, f12, f12, f31);

        muls(buf, f16, f16, f30);
        adds(buf, f16, f16, f31);

        lw(buf, t9, sp, 0x00);
        mtc1(buf, t9, f7);
        muls(buf, f7, f7, f30);
        adds(buf, f7, f7, f31);
        swc1(buf, f7, sp, 0x00);

        lw(buf, t9, sp, 0x20);
        mtc1(buf, t9, f7);
        muls(buf, f7, f7, f30);
        adds(buf, f7, f7, f31);
        swc1(buf, f7, sp, 0x20);

        sw(buf, zero, sp, 0x48);
    }),
    L"// Hud Scaling 3"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x2EC52C, 0x2EC71C }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        cvtsw(buf, f1, f1);

        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        divs(buf, f1, f1, f30);
    }),
    L"// Text Scaling"));

    ps2.vecPatches.push_back(PCSX2Memory(CONT, EE, ps2.GV({ 0x2EC830, 0x2ECAF8 }), WORD_T, MAKE_INLINE, mips_asm([](oss& buf)
    {
        li2(buf, t9, Screen.fHudScale);
        mtc1(buf, t9, f30);
        //li2(buf, t9, Screen.fHudOffset);
        //mtc1(buf, t9, f31);

        //muls(buf, f26, f26, f30);
        //muls(buf, f27, f27, f30);
        adds(buf, f26, f26, f31);
        adds(buf, f27, f27, f31);

        swc1(buf, f26, sp, 0x1A48);
        swc1(buf, f27, sp, 0x1A40);
    }),
    L"// Text Scaling = TS / " + std::to_wstring(Screen.fHudScale) + L" + " + std::to_wstring(Screen.fHudOffset)));

    //this is ugly and needs to be rewritten somehow
    []()
    {
        ps2.FindHostMemoryMapEEmem();
        auto SC4_OFF_ELF_CHECK = (uint32_t*)(ps2.GV({ 0x25ECE0, 0x25ED50 }) + EEStart);
        while (true)
        {
            __try
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100ms);
                if (ps2.isCRCValid())
                {
                    if (*SC4_OFF_ELF_CHECK == 0x27BDF4E0)
                    {
                        static bool bOnce = false;
                        if (!bOnce)
                        {
                            ps2.WritePnach();
                            bOnce = true;
                        }

                        if (!ps2.GetWidescreenPatchesOption())
                            ps2.WriteMemory();
                    }
                }
            }
            __except (true)
            {
            }
        }
    }();
}