#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "../../includes/pcsx2/pcsx2f_api.h"

//#define NANOPRINTF_IMPLEMENTATION
//#include "../../includes/pcsx2/nanoprintf.h"
//#include "../../includes/pcsx2/log.h"
#include "../../includes/pcsx2/memalloc.h"
#include "../../includes/pcsx2/patterns.h"
#include "../../includes/pcsx2/mips.h"
#include "../../includes/pcsx2/injector.h"
#include "../../includes/pcsx2/inireader.h"

#include "cpad.h"
#include "ckey.h"

int CompatibleCRCList[] = { 0x4F32A11F };
char PluginData[MaxIniSize] = { 1 };
int PCSX2Data[PCSX2Data_Size] = { 1 };
char KeyboardState[StateNum][StateSize] = { {1} };
struct CMouseControllerState MouseState[StateNum] = { {1} };
char CheatString[CheatStringLen] = { 1 };
char OSDText[OSDStringNum][OSDStringSize] = { {1} };

struct ScreenX
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fDefaultAR;
    float fHudScale;
    float fHudOffset;
} Screen;

float round_f(float num)
{
    return (float)(int)(num < 0 ? num - 0.5f : num + 0.5f);
}

float BlipsScale = 0.0f;
float __f6 = 6.0f;
void __attribute__((naked)) BlipsScaling(float a1, float a2, float a3, float a4)
{
    {
        asm volatile ("lw      $v1,  %[x]" ::[x] "m" (__f6));
        asm volatile ("mtc1    $v1, $f21");
        asm volatile ("add.s   $f12, $f12, $f21");
        asm volatile ("add.s   $f13, $f13, $f21");
        asm volatile ("sub.s   $f14, $f14, $f21");
        asm volatile ("sub.s   $f15, $f15, $f21");

        asm volatile ("lw      $v1,  %[x]" ::[x] "m" (BlipsScale));
        asm volatile ("mtc1    $v1,  $f31");
        asm volatile ("sub.s   $f13, $f13, $f21");
        asm volatile ("add.s   $f15, $f15, $f21");
        asm volatile ("mul.s   $f21, $f21, $f31");
        asm volatile ("sub.s   $f12, $f12, $f21");
        asm volatile ("add.s   $f14, $f14, $f21");
    }

    /*
    a1 += 6.0f;
    a2 += 6.0f;
    a3 -= 6.0f;
    a4 -= 6.0f;

    a1 -= 6.0f * Screen.fHudScale;
    a2 -= 6.0f * Screen.fHudScale;
    a3 += 6.0f * Screen.fHudScale;
    a4 += 6.0f * Screen.fHudScale;
    */

    //original code
    asm volatile ("move    $v0,  $a0");
    asm volatile ("swc1    $f12, 0x0($a0)");
    asm volatile ("swc1    $f13, 0xC($a0)");
    asm volatile ("swc1    $f14, 0x8($a0)");
    asm volatile ("swc1    $f15, 0x4($a0)");

    asm volatile ("jr      $ra");
    asm volatile ("nop");
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

float (*game_atan2f)(float a1, float a2);
float AdjustFOV(float f, float ar, float t)
{
    return ((2.0f * game_atan2f(((ar) / (4.0f / 3.0f)) * t, 1.0f)) * (180.0f / (float)M_PI) * 100.0f) / 100.0f;
}

const float fDefaultFOV = 70.0f;
float temp_t = 0.0f;
float* flt_487484 = NULL;
void sub_2653E0(float a1)
{
    *flt_487484 = a1 * (AdjustFOV(fDefaultFOV, Screen.fAspectRatio, temp_t) / fDefaultFOV);
}

float* flt_487488 = NULL;
float sub_2653F0(int a1)
{
    *flt_487488 = Screen.fAspectRatio;
    return *flt_487488;
}

int PCControlScheme = 0;
int log_cleared = 0;
void GameLoopStuff()
{
    if (log_cleared != 1)
    {
        //logger.ClearLog();
        log_cleared = 1;
    }

    // Cam Pos OSD
    {
        //extern CVector* GetCamPos();
        //npf_snprintf(OSDText[0], 255, "Cam Pos: %f %f %f", GetCamPos()->x, GetCamPos()->y, GetCamPos()->z);
    }

    // PC Cheats
    if (PCControlScheme != 0)
        HandlePCCheats();
}

void isLittleWillie(uintptr_t a1)
{
    if (*(float*)(a1 + 1992) == 1.0f && (*(int16_t*)(*(uint32_t*)(a1 + 2040) + 86) == 0xAD))
        *(float*)(a1 + 1992) = *(float*)(a1 + 1992) + 1.0f;
}

void test(int a1, struct CPad* a2)
{
    void(*PlayerControlFreeAim)(int a1, struct CPad* pad) = (void*)0x2343E0;
    void(*PlayerControlZelda)(int a1, struct CPad* pad) = (void*)0x232DE8;

    injector.MakeNOP(0x234654);
    injector.MakeNOP(0x23465C);
    injector.MakeNOP(0x23464C);
    injector.MakeNOP(0x234BD8);
    injector.MakeNOP(0x234dfc);

    PlayerControlZelda(a1, a2);
    PlayerControlFreeAim(a1, a2);
}

void init()
{
    //logger.SetBuffer(OSDText, sizeof(OSDText) / sizeof(OSDText[0]), sizeof(OSDText[0]));
    //logger.Write("Loading GTAVCS.PCSX2F.WidescreenFix...");

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

    //logger.WriteF("Resolution: %dx%d", Screen.nWidth, Screen.nHeight);
    //logger.WriteF("Aspect Ratio: %f", Screen.fAspectRatio);

    Screen.fDefaultAR = (4.0f / 3.0f); //Screen.fAspectRatio >= (16.0f / 9.0f) ? (16.0f / 9.0f) : (4.0f / 3.0f);
    Screen.fHudScale = 1.0f / ((Screen.fAspectRatio) / (Screen.fDefaultAR));
    Screen.fHudOffset = ((((480.0f * (Screen.fAspectRatio)) - (480.0f * Screen.fDefaultAR))) / 2.0f) / (Screen.fAspectRatio);
    BlipsScale = 1.0f / ((Screen.fAspectRatio) / (16.0f / 9.0f));

    inireader.SetIniPath((char*)PluginData + sizeof(uint32_t), *(uint32_t*)PluginData);

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);

    if (SkipIntro)
    {
        //logger.Write("Skipping intro...");
        uintptr_t ptr = pattern.get_first("00 00 00 00 ? ? ? ? 2D 20 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 2D 10 00", -4);
        injector.MakeNOP(ptr);
    }

    int ImprovedWidescreenSupport = inireader.ReadInteger("MAIN", "ImprovedWidescreenSupport", 0);
    int Enable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);

    int ModernControlScheme = inireader.ReadInteger("CONTROLS", "ModernControlScheme", 1);
    PCControlScheme = inireader.ReadInteger("CONTROLS", "PCControlScheme", 1);

    if (ImprovedWidescreenSupport)
    {
        //logger.Write("Enabling Improved Widescreen Support...");

        //Proper FOV Scaling
        {
            //logger.Write("Fixing FOV...");
            uintptr_t ptr_44A648 = pattern.get(0, "90 FF BD 27 58 00 B5 E7 50 00 B4 E7 46 6D 00 46", 0);
            game_atan2f = (float (*)(float))ptr_44A648;
            uintptr_t ptr_21CD68 = pattern.get(0, "03 84 10 00 03 8C 11 00 03 94 12 00 03 9C 13 00 03 A4 14 00 03 AC 15 00", 0);
            flt_487484 = (float*)GetAbsoluteAddress(ptr_21CD68, 32, 48);
            temp_t = tan(fDefaultFOV / 2.0f * ((float)M_PI / 180.0f));
            injector.MakeJMPwNOP(0x2653E0, (intptr_t)sub_2653E0);
        }

        //Current res aspect ratio
        {
            //logger.Write("Overriding aspect ratio...");
            flt_487488 = (float*)0x487488;
            injector.MakeJAL(0x21CD80, (intptr_t)sub_2653F0);
            injector.MakeJAL(0x21CF10, (intptr_t)sub_2653F0);
            injector.MakeJAL(0x21EC34, (intptr_t)sub_2653F0);
        }

        // Force Widescreen Option to Off
        {
            injector.WriteInstr(0x21CA38, lui(a1, 0));
            injector.WriteInstr(0x2AF070, lui(a1, 0));
            injector.WriteInstr(0x3AD0FC, lui(a1, 0));
            
            injector.WriteInstr(0x2AF65C, _xor(v0, v0, v0));
            injector.WriteInstr(0x3B8660, lui(v0, 1)); //menu pad icons
            injector.WriteInstr(0x2804FC, lui(v0, 1)); //cutscene borders
            injector.WriteInstr(0x3B5A84, lui(v0, 1)); //menu
        }

        //ammo font
        //injector.WriteMemory8(0x31F8C4, 0x1);

        //static uint32_t xrefs[] = {
        //   //0x1147E0, 0x1149E8, 0x11589C, 0x118598, 0x118624, 0x1186A0, 0x118784, 0x1187EC, 0x118938, 0x118998, 0x118A10,
        //   //0x118A6C, 0x118AE4, 0x118B40, 0x118BB8, 0x118C14, 0x118C8C, 0x118CE4, 0x118D20, 0x118D90, 0x118DE8, 0x118E24,
        //   //0x118E94, 0x118EEC, 0x118F28, 0x118F98, 0x118FF0, 
        //   //0x11902C, 0x11914C, 0x21F69C, 
        //   //0x2625A4, 
        //   //0x265420,
        //   //0x26BB4C,
        //   ///*0x2804FC,*/ 0x29D9D4, 0x29F76C, 0x2A1A08, /*0x2AEC9C,*/ 0x2AF8E4, 0x2AF9D8, 0x2AFA88, 0x3193C4, 0x31DCAC, 0x31DCEC,
        //   //0x31DD18, 0x31DDCC, 0x31DE08, 0x31DE34, 0x31DF10, 0x31DF44, 0x31DF70, 0x31E0C4, 0x31E114, 0x31E264, 0x31E290,
        //   //0x31E65C, 0x31E69C, 0x31E6C8, 0x31E77C, 0x31E7B8, 0x31E7E4, 0x31E8C0, 0x31E8F4, 0x31E920, 0x31EA74, 0x31EAC4,
        //   //0x31EC5C, 0x31ECB4, 0x31EE98, 0x31EF90, 0x31EFB8, 0x31EFF0, 0x31F294, 0x31F5BC, 0x31F5E8, 0x31F608, 0x31F640,
        //   //0x31FF8C, 0x320B4C, 0x320E64, 0x321174, 0x32119C, 0x321214, 0x321294, 0x3212E0, 0x321308, 0x32141C, 0x3220B4,
        //   //0x322DE8, 0x322F38, 0x322F78, 0x323274, 0x32329C, 0x323358, 0x323380, 0x32349C, 0x3234C8, 0x323678, 0x323708,
        //   //0x32377C, 0x323BC8, 0x371E90, 
        //   ///*0x3ACC2C,*/ 0x3B5908, 
        //   0x3B5A84, /*0x3B8660,*/
        //};
        //
        //for (size_t i = 0; i < sizeof(xrefs) / sizeof(xrefs[0]); i++)
        //{
        //    injector.WriteInstr(xrefs[i], lui(v0, 0));
        //}

        if (Screen.fAspectRatio > (4.0f / 3.0f))
        {
            MakeInlineWrapper(0x114800, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x114A20, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x1158B4, mtc1(at, f12), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f12, f12, f31));
            MakeInlineWrapper(0x118464, mtc1(at, f28), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f28, f28, f31));
            MakeInlineWrapper(0x1187c0, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118944, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x1189b0, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118a28, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118a88, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118B00, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118B5C, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118BD4, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118c30, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118ca8, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118d00, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            injector.MakeInlineLUIORI(0x118d28, ((5.333333f * Screen.fHudScale)));
            MakeInlineWrapper(0x118dac, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118e04, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            injector.MakeInlineLUIORI(0x118e34, ((5.333333f * Screen.fHudScale)));
            MakeInlineWrapper(0x118eb0, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x118f08, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            injector.MakeInlineLUIORI(0x118f30, ((5.333333f * Screen.fHudScale)));
            MakeInlineWrapper(0x118fb4, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            MakeInlineWrapper(0x11900c, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            injector.MakeInlineLUIORI(0x119034, ((5.333333f * Screen.fHudScale)));
            MakeInlineWrapper(0x119174, mtc1(at, f0), lui(at, HIWORD(Screen.fHudScale)), addiu(at, at, LOWORD(Screen.fHudScale)), mtc1(at, f31), muls(f0, f0, f31));
            injector.WriteInstr(0x21f654, (li(s1, (int16_t)round_f(210.0f * Screen.fHudScale))));
            injector.MakeInlineLUIORI(0x2625BC, Screen.fHudScale); // game text
            //injector.MakeInlineLUIORI(0x265460, Screen.fAspectRatio); //ar
            //0x2AEC9C todo menu or script

            const float f1_25 = (((4.0f / 3.0f) / (16.0f / 9.0f)) * (10.0f / 6.0f));
            float ARDiff = (16.0f / 9.0f) / (Screen.fAspectRatio);
            float bar_left_edge_offset_from_right = 11.0f * ARDiff;
            const float f15 = 15.0f;
            float fWeaponIconPos = (360.0f + f15 + 64.0f + (26.0f * ARDiff)); //465.0f
            float fWeaponIconSize = 45.5f * ARDiff;
            float fBarsPos = fWeaponIconPos - ((64.0f + (26.0f)) * ARDiff); // 360.0f + 15.0f

            injector.MakeInlineLUIORI(0x26bb54, 1.0f * ARDiff); //menu map
            injector.MakeInlineLUIORI(0x2af8ec, (0.85f / f1_25) * ARDiff);
            injector.MakeInlineLUIORI(0x2af9e0, (0.85f / f1_25) * ARDiff);
            injector.MakeInlineLUIORI(0x2afa90, (0.85f / f1_25) * ARDiff);
            injector.MakeInlineLUIORI(0x31dcb4, (0.85f / f1_25) * ARDiff);
            injector.MakeInlineLUIORI(0x31e664, (0.85f / f1_25) * ARDiff);
            injector.MakeInlineLUIORI(0x31de3c, (54.4f / f1_25) * ARDiff);
            injector.MakeInlineLUIORI(0x31df78, (54.4f / f1_25) * ARDiff);
            injector.MakeInlineLUIORI(0x31e7ec, (54.4f / f1_25) * ARDiff);
            injector.MakeInlineLUIORI(0x31e928, (54.4f / f1_25) * ARDiff);

            injector.MakeInlineLUIORI(0x31dce4, fBarsPos);
            injector.MakeInlineLUIORI(0x31dd20, fBarsPos);
            injector.MakeInlineLUIORI(0x31ddd4, fBarsPos);
            injector.MakeInlineLUIORI(0x31de00, fBarsPos);
            injector.MakeInlineLUIORI(0x31df08, fBarsPos);
            injector.MakeInlineLUIORI(0x31df3c, fBarsPos);
            injector.MakeInlineLUIORI(0x31e25c, fBarsPos);
            injector.MakeInlineLUIORI(0x31e298, fBarsPos + 64.0f);
            injector.MakeInlineLUIORI(0x31e694, fBarsPos);
            injector.MakeInlineLUIORI(0x31e6d0, fBarsPos);
            injector.MakeInlineLUIORI(0x31e784, fBarsPos);
            injector.MakeInlineLUIORI(0x31e7b0, fBarsPos);
            injector.MakeInlineLUIORI(0x31e8b8, fBarsPos);
            injector.MakeInlineLUIORI(0x31e8ec, fBarsPos);

            injector.WriteInstr(0x31ea7c, (li(v1, (int16_t)(fBarsPos + bar_left_edge_offset_from_right))));
            injector.WriteInstr(0x31e0cc, (li(v1, (int16_t)(fBarsPos + bar_left_edge_offset_from_right))));
            injector.WriteInstr(0x31eacc, (li(v1, (int16_t)(fBarsPos + bar_left_edge_offset_from_right))));
            injector.WriteInstr(0x31e11c, (li(v1, (int16_t)(fBarsPos + bar_left_edge_offset_from_right))));
            injector.MakeInlineLUIORI(0x31ec64, (fBarsPos + bar_left_edge_offset_from_right)); //money and time pos
            injector.MakeInlineLUIORI(0x31ecbc, (fBarsPos + bar_left_edge_offset_from_right)); //money and time pos
            injector.MakeInlineLUIORI(0x31eea0, (fBarsPos + bar_left_edge_offset_from_right)); //money and time pos
            injector.MakeInlineLUIORI(0x31efe8, 9.0f * ARDiff); //money and time scale
            injector.MakeInlineLUIORI(0x31f1d4, 2.0f * ARDiff); // money and time spacing, not adjusted by ws option originally
            injector.MakeInlineLUIORI(0x31f5b4, fWeaponIconPos); //weapon icon pos
            injector.MakeInlineLUIORI(0x31f648, fWeaponIconSize); // weapon icon size
            injector.MakeInlineLUIORI(0x31ff94, 13.5f - 2.5f);
            injector.MakeInlineLUIORI(0x320b40, (405.0f + 10.0f));
            injector.MakeInlineLUIORI(0x31f2ac, (float)round_f((64.0f * Screen.fHudScale))); //radar scale
            injector.MakeInlineLUIORI(0x32116c, 35.0f); //script bars, taxi tip, ped health etc
            injector.MakeInlineLUIORI(0x321310, 29.0f);
            injector.MakeInlineLUIORI(0x3211a4, 425.0f); //413 / 425
            injector.MakeInlineLUIORI(0x32121c, 426.0f); //414 / 426
            injector.MakeInlineLUIORI(0x32128c, 426.0f); //414 / 426
            injector.MakeInlineLUIORI(0x3212d8, 426.0f); //414 / 426
            injector.WriteInstr(0x320e70, (li(v1, (int16_t)(415.0f))));
            injector.WriteInstr(0x321428, (li(v1, (int16_t)(415.0f))));
            injector.MakeInlineLUIORI(0x3220bc, (248.0f - 55.0f));
            injector.MakeInlineLUIORI(0x322e08, (154.0f - 32.0f));
            injector.MakeInlineLUIORI(0x322f54, (38.0f - 4.0f));
            injector.MakeInlineLUIORI(0x322f98, (142.0f - 29.0f));
            injector.MakeInlineLUIORI(0x323268, (198.0f + 10.0f));
            injector.MakeInlineLUIORI(0x3232a4, (282.0f - 10.0f));
            injector.MakeInlineLUIORI(0x32334c, (198.0f + 10.0f));
            injector.MakeInlineLUIORI(0x323388, (282.667f - 10.667f));
            injector.MakeInlineLUIORI(0x323490, (210.133f + 7.467f));
            injector.MakeInlineLUIORI(0x3234d0, (269.867f - 7.467f));
            injector.MakeInlineLUIORI(0x323670, (280.0f - 70.0f));
            injector.MakeInlineLUIORI(0x323700, (238.0f - 59.5f));
            injector.MakeInlineLUIORI(0x323774, (241.0f - 60.25f));
            injector.MakeNOP(0x323bd0); injector.MakeInlineLUIORI(0x323bd8, (1.0f * Screen.fHudScale));
            injector.MakeNOP(0x3b8670); //map pad icons

            injector.MakeJAL(0x116D50, (intptr_t)BlipsScaling);

            // Crosshair
            injector.MakeInlineLUIORI(0x3193CC, (float)round_f((14.0f / (Screen.fAspectRatio / (4.0f / 3.0f)))));
            injector.MakeInlineLUIORI(0x3193DC, (float)round_f((14.0f / (Screen.fAspectRatio / (4.0f / 3.0f)))));
            //injector.MakeInlineLUIORI(0x319400, (float)round_f((14.0f * Screen.fHudScale)));

            // Radar Disc
            injector.MakeInlineLUIORI(0x3215E8, (4.0f * Screen.fHudScale));

            // Clock Text Thingies
            injector.MakeInlineLUIORI(0x31F1D4, (2.0f * ARDiff));
            //injector.MakeInlineLUIORI(0x31F1EC, (4.0f * ARDiff));
            injector.MakeInlineLUIORI(0x31F10C, (3.0f * ARDiff));
        }
    }

    if (Enable60FPS)
    {
        //logger.Write("Enabling 60fps...");
        uintptr_t ptr_370314 = pattern.get(0, "6F 00 03 3C 01 00 02 24 68 42 62 AC", -8);
        if (ptr_370314)
            injector.MakeNOP(ptr_370314);
    }

    CMenuManager__m_PrefsInvertLook = (int*)0x487A58;
    pCPad__GetLookBehindForCar = (void*)0x285C20;
    if (!PCControlScheme && ModernControlScheme)
    {
        //logger.Write("Enabling Modern Control Scheme...");
        ReplacePadFuncsWithModernControls();
    }

    if (PCControlScheme)
    {
        //logger.Write("Enabling PC Control Scheme, Gamepad controls will stop working!");
        ReplacePadFuncsWithPCControls();
    }

    {
        injector.MakeJAL(0x21E988, (intptr_t)GameLoopStuff);
    }

    //Little Willie Cam Fix
    {
        MakeInlineWrapper(0x37281C,
            lui(at, 0x40A0),
            move(a0, s0),
            jal(isLittleWillie),
            nop()
        );
    }

    // Allow camera movement durin reloading
    //{
    //    injector.MakeNOP(0x235304);
    //    injector.MakeNOP(0x235404);
    //    injector.MakeNOP(0x2352E4);
    //    injector.MakeNOP(0x2355D8);
    //    injector.MakeNOP(0x2355a8);
    //    injector.MakeNOP(0x234674);
    //    injector.MakeNOP(0x23446C);
    //    injector.MakeNOP(0x235650);
    //
    //    injector.MakeNOP(0x235318);
    //    injector.MakeNOP(0x2355E8);
    //
    //    injector.WriteMemory16(0x235660 + 2, 0x1000); // beq -> b
    //}

    //test
    //{
    //    injector.WriteMemory16(0x232E60+2, 0x1000); // beq -> b
    //    injector.MakeJAL(0x23152C, test);
    //}

    //logger.Write("GTAVCS.PCSX2F.WidescreenFix loaded");
}

int main()
{
    return 0;
}
