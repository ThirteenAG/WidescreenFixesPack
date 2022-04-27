#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../../includes/pcsx2/pcsx2f_api.h"
#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/injector.h"

int CompatibleCRCList[] = { 0xC0498D24, 0xABE2FDE9 };
char ElfPattern[] = "10 00 BF FF 00 00 B0 7F 30 00 A4 AF 40 00 A5 AF";
int PCSX2Data[PCSX2Data_Size] = { 1 };
char OSDText[OSDStringNum][OSDStringSize] = { {1} };

struct ScreenX
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float f3DScale;
    float fHudScale;
    float fHudOffset;
} Screen;

void __attribute__((naked)) sub_197100()
{
    asm volatile ("" :: "r"(Screen.f3DScale));
    asm volatile ("mtc1    $v0, $f31\n");
    asm volatile ("mul.s   $f12, $f12, $f31\n");

    //original code
    asm volatile ("swc1    $f12, 0($a0)\n");
    asm volatile ("move    $v0, $a0\n");
    asm volatile ("swc1    $f13, 4($a0)\n");
    asm volatile ("swc1    $f14, 8($a0)\n");
    asm volatile ("swc1    $f15, 0xC($a0)\n");
    asm volatile ("jr      $ra\n");
    asm volatile ("nop\n");
}

void init()
{
    logger.SetBuffer(OSDText, sizeof(OSDText) / sizeof(OSDText[0]), sizeof(OSDText[0]));
    logger.Write("Loading SplinterCellDoubleAgent.PCSX2F.WidescreenFix...");

    uint32_t DesktopSizeX = PCSX2Data[PCSX2Data_DesktopSizeX];
    uint32_t DesktopSizeY = PCSX2Data[PCSX2Data_DesktopSizeY];
    Screen.nWidth = PCSX2Data[PCSX2Data_WindowSizeX];
    Screen.nHeight = PCSX2Data[PCSX2Data_WindowSizeY];
    Screen.fWidth = (float)Screen.nWidth;
    Screen.fHeight = (float)Screen.nHeight;
    uint32_t IsFullscreen = PCSX2Data[PCSX2Data_IsFullscreen];
    uint32_t AspectRatioSetting = PCSX2Data[PCSX2Data_AspectRatioSetting];

    if (IsFullscreen || !Screen.nWidth || !Screen.nHeight)
    {
        Screen.nWidth = DesktopSizeX;
        Screen.nHeight = DesktopSizeY;
    }

    switch (AspectRatioSetting)
    {
    case RAuto4_3_3_2: //not implemented
        //if (GSgetDisplayMode() == GSVideoMode::SDTV_480P)
        //    AspectRatio = 3.0f / 2.0f;
        //else
        Screen.fAspectRatio = 4.0f / 3.0f;
        break;
    case R4_3:
        Screen.fAspectRatio = 4.0f / 3.0f;
        break;
    case R16_9:
        Screen.fAspectRatio = 16.0f / 9.0f;
        break;
    case Stretch:
    default:
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        break;
    }

    logger.WriteF("Resolution: %dx%d", Screen.nWidth, Screen.nHeight);
    logger.WriteF("Aspect Ratio: %f", Screen.fAspectRatio);

    enum GameVersion
    {
        SLUS21356,
        SLES53827
    };

    int gv = SLUS21356;
    if (injector.ReadMemory32(0x25F55C) != 0x3C023F80)
        gv = SLES53827;

    float intResX = (gv == SLUS21356 ? 640.0f : 512.0f);
    float intResY = (gv == SLUS21356 ? 480.0f : 384.0f);
    Screen.fHudScale = (((intResX / intResY)) / (Screen.fAspectRatio));
    Screen.fHudOffset = (((intResY * Screen.fAspectRatio) - intResX) / 2.0f) * Screen.fHudScale;
    Screen.f3DScale = (((intResX / intResY)) / (Screen.fAspectRatio));

    //3D Scaling
    injector.MakeJAL(gv == SLUS21356 ? 0x25F5A4 : 0x25F614, (intptr_t)sub_197100);

    //Hud Scaling 1 X1
    uint32_t i640 = 640;
    injector.MakeNOP(gv == SLUS21356 ? 0x2EACE8 : 0x2EAD34);
    MakeInlineWrapper(gv == SLUS21356 ? 0x2EACE4 : 0x2EAD30,
        //filtering out 0-640 textures (fading etc)
        move(k0, v0), //x
        move(k1, v1), //w
        lui(s6, HIWORD(i640)),
        addiu(s6, s6, LOWORD(i640)),
        lh(v0, sp, (gv == SLUS21356 ? 0x12E : 0x13E)),
        muls(f0, f0, (gv == SLUS21356 ? f21 : f25)),
        lui(t9, HIWORD(Screen.fHudScale)),
        addiu(t9, t9, LOWORD(Screen.fHudScale)),
        mtc1(t9, f30),
        lui(t9, HIWORD(Screen.fHudOffset)),
        addiu(t9, t9, LOWORD(Screen.fHudOffset)),
        mtc1(t9, f31),
        beq(k0, zero, 2),
        nop(),
        b(3), //-->
        nop(),
        beq(k1, s6, 3),
        nop(),
        muls(f0, f0, f30), //<--
        adds(f0, f0, f31)
    );

    //Hud Scaling 1 X2 = X2
    MakeInlineWrapper(gv == SLUS21356 ? 0x2EAD10 : 0x2EAD5C,
        muls(f0, f0, (gv == SLUS21356 ? f21 : f25)),
        lui(t9, HIWORD(Screen.fHudScale)),
        addiu(t9, t9, LOWORD(Screen.fHudScale)),
        mtc1(t9, f30),
        lui(t9, HIWORD(Screen.fHudOffset)),
        addiu(t9, t9, LOWORD(Screen.fHudOffset)),
        mtc1(t9, f31),
        beq(k0, zero, 2),
        nop(),
        b(3), //-->
        nop(),
        beq(k1, s6, 3),
        nop(),  
        muls(f0, f0, f30), //<--
        adds(f0, f0, f31)
    );

    //Hud Scaling 2 X1
    uint32_t i96DAFAEC = 0x96DAFAEC;
    uint32_t i96C3B081 = 0x96C3B081;
    uint32_t i00C3B081 = 0x00C3B081;
    MakeInlineWrapper(gv == SLUS21356 ? 0x2EADA0 : 0x2EAE10,
        mtc1((gv == SLUS21356 ? t2 : a0), f0),
        lui(t9, HIWORD(Screen.fHudScale)),
        addiu(t9, t9, LOWORD(Screen.fHudScale)),
        mtc1(t9, f30),
        lui(t9, HIWORD(Screen.fHudOffset)),
        addiu(t9, t9, LOWORD(Screen.fHudOffset)),
        mtc1(t9, f31),
        //coop goggles overlay check (FColor, animation still broken)
        lui(t9, HIWORD(i96DAFAEC)),
        addiu(t9, t9, LOWORD(i96DAFAEC)),
        beq(a1, t9, 11),
        nop(),
        lui(t9, HIWORD(i96C3B081)),
        addiu(t9, t9, LOWORD(i96C3B081)),
        beq(a1, t9, 7),
        nop(),
        lui(t9, HIWORD(i00C3B081)),
        addiu(t9, t9, LOWORD(i00C3B081)),
        beq(a1, t9, 3),
        nop(),

        muls(f12, f12, f30),
        adds(f12, f12, f31)
    );

    // Hud Scaling 2 X2 = X2
    MakeInlineWrapper(gv == SLUS21356 ? 0x2EADD0 : 0x2EAE20,
        lui(t9, HIWORD(Screen.fHudScale)),
        addiu(t9, t9, LOWORD(Screen.fHudScale)),
        mtc1(t9, f30),
        lui(t9, HIWORD(Screen.fHudOffset)),
        addiu(t9, t9, LOWORD(Screen.fHudOffset)),
        mtc1(t9, f31),

        //coop goggles overlay check (FColor, animation still broken)
        lui(t9, HIWORD(i96DAFAEC)),
        addiu(t9, t9, LOWORD(i96DAFAEC)),
        beq(a1, t9, 11),
        nop(),
        lui(t9, HIWORD(i96C3B081)),
        addiu(t9, t9, LOWORD(i96C3B081)),
        beq(a1, t9, 7),
        nop(),
        lui(t9, HIWORD(i00C3B081)),
        addiu(t9, t9, LOWORD(i00C3B081)),
        beq(a1, t9, 3),
        nop(),

        muls(f16, f16, f30),
        adds(f16, f16, f31),

        swc1(f16, sp, 0)
    );

    //Hud Scaling 3
    if (gv == SLES53827) //only for EU version
    {
        MakeInlineWrapper(0x2EAFD8,
            lui(t9, HIWORD(Screen.fHudScale)),
            addiu(t9, t9, LOWORD(Screen.fHudScale)),
            mtc1(t9, f30),
            lui(t9, HIWORD(Screen.fHudOffset)),
            addiu(t9, t9, LOWORD(Screen.fHudOffset)),
            mtc1(t9, f31),

            muls(f12, f12, f30),
            adds(f12, f12, f31),

            muls(f16, f16, f30),
            adds(f16, f16, f31),

            lw(t9, sp, 0x00),
            mtc1(t9, f7),
            muls(f7, f7, f30),
            adds(f7, f7, f31),
            swc1(f7, sp, 0x00),

            lw(t9, sp, 0x20),
            mtc1(t9, f7),
            muls(f7, f7, f30),
            adds(f7, f7, f31),
            swc1(f7, sp, 0x20),

            sw(zero, sp, 0x48)
        );
    }

    // Text Scaling
    MakeInlineWrapper(gv == SLUS21356 ? 0x2EC52C : 0x2EC71C,
        cvtsw(f1, f1),
        lui(t9, HIWORD(Screen.fHudScale)),
        addiu(t9, t9, LOWORD(Screen.fHudScale)),
        mtc1(t9, f30),
        divs(f1, f1, f30)
    );

    // Text Scaling 2
    MakeInlineWrapper(gv == SLUS21356 ? 0x2EC830 : 0x2ECAF8,
        lui(t9, HIWORD(Screen.fHudScale)),
        addiu(t9, t9, LOWORD(Screen.fHudScale)),
        mtc1(t9, f30),
        //li2(buf, t9, Screen.fHudOffset);
        //mtc1(buf, t9, f31);

        //muls(buf, f26, f26, f30);
        //muls(buf, f27, f27, f30);
        adds(f26, f26, f31),
        adds(f27, f27, f31),

        swc1(f26, sp, 0x1A48),
        swc1(f27, sp, 0x1A40)
    );

    logger.Write("SplinterCellDoubleAgent.PCSX2F.WidescreenFix loaded");
    logger.ClearLog();
}

int main()
{
    return 0;
}
