#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <systemctrl.h>

#include "../../includes/psp/log.h"
#include "../../includes/psp/injector.h"
#include "../../includes/psp/patterns.h"
#include "../../includes/psp/inireader.h"
#include "../../includes/psp/gvm.h"
#include "../../includes/psp/mips.h"

#define MODULE_NAME_INTERNAL "GTA3"
#define MODULE_NAME "GTAVCS.PPSSPP.WidescreenFix"
#define LOG_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.log"
#define INI_PATH "ms0:/PSP/PLUGINS/GTAVCS.PPSSPP.WidescreenFix/GTAVCS.PPSSPP.WidescreenFix.ini"

#ifndef __INTELLISENSE__
PSP_MODULE_INFO(MODULE_NAME, PSP_MODULE_USER, 1, 0);
#endif

static const float fPSPResW = 480.0f;
static const float fPSPResH = 272.0f;

enum
{
    EMULATOR_DEVCTL__TOGGLE_FASTFORWARD = 0x30,
    EMULATOR_DEVCTL__GET_ASPECT_RATIO,
    EMULATOR_DEVCTL__GET_SCALE,
    EMULATOR_DEVCTL__GET_AXIS,
    EMULATOR_DEVCTL__GET_VKEY,
};

struct CControllerState
{
    short LEFTSTICKX;
    short LEFTSTICKY;
    short RIGHTSTICKX;
    short RIGHTSTICKY;

    short LEFTSHOULDER1;
    short LEFTSHOULDER2;
    short RIGHTSHOULDER1;
    short RIGHTSHOULDER2;

    short DPADUP;
    short DPADDOWN;
    short DPADLEFT;
    short DPADRIGHT;

    short unk1;
    short unk2;
    short unk3;
    short unk4;

    short START;
    short SELECT;

    short SQUARE;
    short TRIANGLE;
    short CROSS;
    short CIRCLE;

    short LEFTSHOCK;
    short RIGHTSHOCK;
};

struct CPad
{
    char unk[2];
    struct CControllerState NewState;
    char unk2[2];
    struct CControllerState OldState;
    char pad[50];
    int16_t Mode;
    int16_t ShakeDur;
    int16_t DisablePlayerControls;
};

uintptr_t ptr_E2D44, ptr_E2D44_data;
int (*sub_8B1FD70)(int a1);
int (*FindPlayerPed)();
int (*FindPlayerVehicle)();
int dword_8BB3B4C;
char bManualReloadTriggered = 0;
void handleReload(struct CPad* pad)
{
    if (!sub_8B1FD70 || !FindPlayerPed)
        return;

    int v2 = FindPlayerPed();
    if (!v2)
        return;
    uint8_t m_currentWeapon = *(uint8_t*)(v2 + 0x789);
    if (!(m_currentWeapon >= 3 && m_currentWeapon <= 8))
        return;
    v2 += 0x574;
    v2 += m_currentWeapon * 0x1C;

    int32_t m_nAmmoInClip = *(int32_t*)(v2 + 0x0C);
    int32_t m_nAmmoTotal = *(int32_t*)(v2 + 0x10);
    if (m_nAmmoInClip == m_nAmmoTotal || m_nAmmoTotal == 0)
        return;

    if (!bManualReloadTriggered)
    {
        //if (pad->NewState.LEFTSHOULDER1 && pad->NewState.DPADUP && pad->OldState.DPADUP == 0)
        if (pad->NewState.LEFTSHOULDER1 && pad->NewState.SQUARE && pad->OldState.SQUARE == 0)
        {
            bManualReloadTriggered = 1;
            injector.WriteMemory32(v2 + 0x08, 2);
            int m_nShotTimer = *(int*)(injector.GetGP() + dword_8BB3B4C) + *(int*)(sub_8B1FD70(*(int*)((v2 + 4) + 4)) + 16);
            injector.WriteMemory32(v2 + 0x14, m_nShotTimer + 1000); //hack
            pad->NewState.RIGHTSHOULDER1 = 0;
        }
    }
    else
    {
        if (*(int*)(injector.GetGP() + dword_8BB3B4C) > *(int*)(v2 + 0x14))
        {
            injector.WriteMemory32(ptr_E2D44, ptr_E2D44_data); // PlayOneShot enable
            injector.WriteMemory32(v2 + 0x08, 2);
            bManualReloadTriggered = 0;
        }
        else
        {
            //*(int32_t*)(v2 + 0x10) += m_nAmmoInClip;
            //*(int32_t*)(v2 + 0x0C) = 0;
            injector.MakeNOP(ptr_E2D44); // PlayOneShot disable
            pad->NewState.RIGHTSHOULDER1 = 1;
            injector.WriteMemory32(v2 + 0x08, 2);
        }
    }
}

int16_t CPad__GetAccelerate(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.RIGHTSHOULDER1;
}

int16_t CPad__GetAccelerateNormal(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.CROSS;
}

int16_t CPad__GetAccelerateBovver(struct CPad* pad)
{
    if (*(int16_t*)(FindPlayerVehicle() + 0x56) == 198)
        return CPad__GetAccelerate(pad);
    else
        return CPad__GetAccelerateNormal(pad);
}

int16_t CPad__GetBrake(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.LEFTSHOULDER1;
}

int16_t CPad__GetBrakeNormal(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.SQUARE;
}

int16_t CPad__GetBrakeBovver(struct CPad* pad)
{
    if (*(int16_t*)(FindPlayerVehicle() + 0x56) == 198)
        return CPad__GetBrake(pad);
    else
        return CPad__GetBrakeNormal(pad);
}

int16_t CPad__GetRightStickX(struct CPad* pad)
{
    return pad->NewState.RIGHTSTICKX;
}

int16_t CPad__GetRightStickY(struct CPad* pad)
{
    return pad->NewState.RIGHTSTICKY;
}

int16_t aimX(struct CPad* pad) {
    if (pad->NewState.CROSS || pad->NewState.DPADDOWN)
        return pad->NewState.LEFTSTICKX ? pad->NewState.LEFTSTICKX : pad->NewState.RIGHTSTICKX;
    else
        return pad->NewState.LEFTSTICKX ? pad->NewState.LEFTSTICKX * 2 : pad->NewState.RIGHTSTICKX * 2;
}

int16_t aimY(struct CPad* pad) {
    if (pad->NewState.CROSS || pad->NewState.DPADDOWN)
        return pad->NewState.LEFTSTICKY ? pad->NewState.LEFTSTICKY : pad->NewState.RIGHTSTICKY;
    else
        return pad->NewState.LEFTSTICKY ? pad->NewState.LEFTSTICKY * 2 : pad->NewState.RIGHTSTICKY * 2;
}

int16_t CPad__GetTarget(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
    {
        handleReload(pad);
        return pad->NewState.LEFTSHOULDER1 != 0;
    }
}

int16_t CPad__JumpJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.SQUARE)
    {
        if (!pad->OldState.SQUARE)
        {
            if (CPad__GetTarget(pad)) // for reloading
                return 0;
            return 1;
        }
    }
    return 0;
}

int16_t CPad__TargetJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.LEFTSHOULDER1)
        return pad->OldState.LEFTSHOULDER1 == 0;
    return 0;
}

int16_t CPad__GetWeapon(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    return pad->NewState.RIGHTSHOULDER1;
}

int16_t CPad__WeaponJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.RIGHTSHOULDER1)
        return pad->OldState.RIGHTSHOULDER1 == 0;
    return 0;
}

int16_t CPad__GetLookBehindForPed(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    return pad->NewState.CIRCLE != 0;
}

int16_t CPad__EnterFreeAim(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    return pad->NewState.LEFTSHOULDER1 && (pad->NewState.DPADDOWN != 0/* || pad->NewState.RIGHTSTICKX || pad->NewState.RIGHTSTICKY*/);
}

float fAspectRatio = 16.0f / 9.0f;
float fFOVFactor = 1.0f;
float fARDiff;
float adjustRightX(float in, float scale)
{
    float fRightOffset = fPSPResW - in;
    return in + (fRightOffset - (scale / fARDiff * fRightOffset));
}

float adjustTopRightY(float in, float scale)
{
    return in * scale;
}

float adjustBottomRightY(float in, float scale)
{
    float fBottomOffset = fPSPResH - in;
    return in + (fBottomOffset - (scale * fBottomOffset));
}

float AdjustFOV(float f, float ar)
{
    return round((2.0f * atan(((ar) / (16.0f / 9.0f)) * tan(f / 2.0f * ((float)M_PI / 180.0f)))) * (180.0f / (float)M_PI) * 100.0f) / 100.0f;
}

uintptr_t TheCamera;
int RestoreCutsceneFOV = 1;
float sub_171D44(float a1)
{
    if (RestoreCutsceneFOV && *(uint8_t*)(TheCamera + 0x804)) //switch_widescreen opcode
        return a1;
    else
        return a1 * fFOVFactor * (AdjustFOV(70.0f, fAspectRatio) / 70.0f);
}

float TextScaling(int a1)
{
    return *(float*)(a1 + 32) / fARDiff;
}

float TextScalingVertical(int a1)
{
    return *(float*)(a1 + 32);
}

void isLittleWillie(uintptr_t a1)
{
    if (*(float*)(a1 + 1944) == 1.0f && (*(int16_t*)(*(uint32_t*)(a1 + 1984) + 86) == 0xAD))
        *(float*)(a1 + 1944) = *(float*)(a1 + 1944) + 1.0f;
}

void UnthrottleEmuEnable()
{
    sceIoDevctl("kemulator:", EMULATOR_DEVCTL__TOGGLE_FASTFORWARD, (void*)1, 0, NULL, 0);
}

void UnthrottleEmuDisable()
{
    sceIoDevctl("kemulator:", EMULATOR_DEVCTL__TOGGLE_FASTFORWARD, (void*)0, 0, NULL, 0);
}

int sub_2A9B4(int a1)
{
    return *(uint8_t*)(a1 + 0x20);
}

int16_t ptr_1334C4 = 0;
uintptr_t ptr_3C5100 = 0;
uintptr_t sub_471400()
{
    return ptr_3C5100;
}

int sub_218770(uintptr_t Camera)
{
    float fadeAlpha = *(float*)(Camera + 0xA54); //m_fFadeAlpha
    if (fadeAlpha == 0.0f)
        return 0;
    if (fadeAlpha == 255.0f)
        return 1;
    return 2;
}

uintptr_t ptr_1C5584;
uintptr_t ptr_1C547C;
float LODDistMultiplier = 2.0f;
float sub_8A1E990(uintptr_t a1)
{
    //uintptr_t dword_8BAEF30 = injector.GetGP() + *(int16_t*)ptr_1C5584;
    //uintptr_t MaxNumberOfCarsInUse = injector.GetGP() + *(int16_t*)ptr_1C547C;
    //*(int32_t*)(0x08BAEF30 + 0) = 25 * (int)LODDistMultiplier; // MaxNumberOfPedsInUse interior
    //*(int32_t*)(dword_8BAEF30 + 4) = 30 * (int)LODDistMultiplier; // MaxNumberOfPedsInUse exterior
    //*(int32_t*)(MaxNumberOfCarsInUse) = 16 * (int)LODDistMultiplier; // 08BB32BC

    *(float*)(a1 + 0x7A0) = *(float*)(a1 + 0x7A8) * LODDistMultiplier;
    return LODDistMultiplier;
}

enum CheatTable
{
    Weapon1,
    Weapon2,
    Weapon3,
    Money,
    Armour,
    Health,
    WantedUp,
    NeverWanted,
    SunnyWeather,
    ExtraSunny,
    CloudyWeather,
    RainyWeather,
    FoggyWeather,
    Tank,
    FastWeather,
    BlowUpCars,
    Mayhem,
    PedsAttackVic,
    WeaponsForAll,
    FastTime,
    SlowTime,
    StrongGrip,
    Suicide,
    TrafficLights,
    MadCars,
    BlackCars,
    Trashmaster,
    PickUpChicks,
    FannyMagnet,
    GlassCars,
    TopsyTurvy,

    CheatsNum
};

const char
#ifndef __INTELLISENSE__
__attribute__((aligned(16)))
#endif
cheat_inputs[CheatsNum][9] =
{
    "LRXUDSLR",
    "LRSUDTLR",
    "LRTUDCLR",
    "UDLRXX12",
    "UDLRSS12",
    "UDLRCC12",
    "URSSDLCC",
    "URTTDLXX",
    "LD21RULC",
    "LD21RULX",
    "LD12RULS",
    "LD12RULT",
    "LDTXRUL1",
    "U1D2L1R2",
    "211DUXD1",
    "122LRSD2",
    "211DLCD1",
    "DTUX1212",
    "U1D2LCRT",
    "LL22UTDX",
    "LLCCDUTX",
    "DLU12TCX",
    "RRCC12DX",
    "UDTX12LC",
    "UURLTCCS",
    "1212LCUX",
    "DURT1T1T",
    "DUR11SU1",
    "R1D1CU1S",
    "RULDTT12",
    "SSS112LR",
};

int Enable60FPS;
uintptr_t ptr_2030C8;
uint32_t ptr_2030C8_data;
uintptr_t* dword_8BAF748;
uintptr_t ptr_14F7E8;
int (*sub_89F6390)(int a1, char* a2);
int UnthrottleEmuDuringLoading = 0;
int once = 0;
int PCCheats;
char cheatString[20];
int32_t cheat_id = -1;
struct CPad* (*pCPad__GetPad)(int padNumber);
void(*pCPad__AddToCheatString)(struct CPad* pad, char a2);
void GameLoopStuff()
{
    if (once != 1)
    {
        if (UnthrottleEmuDuringLoading)
            UnthrottleEmuDisable();
        once = 1;
    }

    if (UnthrottleEmuDuringLoading)
    {
        uint32_t gMenuActivated = sub_2A9B4(sub_471400()); //inlined on psp
        float gBlackScreenTime = *(float*)(injector.GetGP() + ptr_1334C4);
        int ScreenFadeStatus = sub_218770(TheCamera);
            
        if ((gBlackScreenTime || ScreenFadeStatus == UnthrottleEmuDuringLoading) && !gMenuActivated)
            UnthrottleEmuEnable();
        else
            UnthrottleEmuDisable();
    }

    if (Enable60FPS == 2)
    {
        uintptr_t str = sub_89F6390(*dword_8BAF748, "REN7_O9");
        if (IsPlayerOnAMission() && *(uint16_t*)str)
            injector.WriteMemory32(ptr_2030C8, ptr_2030C8_data);
        else
        {
            Enable60FPS = 1;
            injector.MakeNOP(ptr_2030C8);
        }
    }

    if (cheat_id >= 0)
    {
        struct CPad* pad = pCPad__GetPad(0);
        for (size_t j = 0; j < 8; j++)
            pCPad__AddToCheatString(pad, cheat_inputs[cheat_id][j]);
        cheatString[0] = 0;
        cheat_id = -1;
    }
}

uintptr_t* ScriptSpace;
uintptr_t* OnAMissionFlag;
uint32_t MainScriptSize;
uint32_t LargestMissionScriptSize;
int IsPlayerOnAMission()
{
    if (OnAMissionFlag)
        return *(uint32_t*)(*ScriptSpace + *OnAMissionFlag) == 1;
    return 0;
}

uintptr_t ptr_2B7F48;
uintptr_t ptr_8BB3CA0 = 0;
int applyMissionFixes()
{
    struct MisFix
    {
        char* pattern;
        int32_t offset;
    };

    struct MisFix mission_fixes[] =
    {
        { "08 00 ? ? ? 8F C2 75 3C", 5 }, // Boomshine Blowout
        { "0A 00 ? ? ? 8F C2 75 3D", 5 }, // The Exchange
        { "0A 00 ? ? ? 0A D7 23 3D", 5 }, // Hose the Hoes 1
        { "0A 00 ? ? ? 8F C2 F5 3C", 5 }, // Hose the Hoes 2
        { "0A 00 ? ? ? 0A D7 23 3C", 5 }, // Hose the Hoes 3
        { "08 00 ? ? ? 0A D7 A3 3B", 5 }, // Balls / Farewell To Arms
        { "08 00 ? ? ? 09 04 40 3F", 5 }, // In The Air Tonight
    };

    if (!ScriptSpace)
        ScriptSpace = (injector.GetGP() + *(int16_t*)(ptr_2B7F48 + 0));
    if (!OnAMissionFlag)
        OnAMissionFlag = (injector.GetGP() + *(int16_t*)(ptr_2B7F48 + 8));
    if (!MainScriptSize)
        MainScriptSize = *(uint32_t*)(injector.GetGP() + ptr_8BB3CA0 + 4);
    if (!LargestMissionScriptSize)
        LargestMissionScriptSize = *(uint32_t*)(injector.GetGP() + ptr_8BB3CA0 + 8);
    if (!dword_8BAF748)
        dword_8BAF748 = (injector.GetGP() + *(int16_t*)(ptr_14F7E8));

    for (size_t i = 0; i < sizeof(mission_fixes) / sizeof(mission_fixes[0]); i++)
    {
        uintptr_t addr = range_pattern.get_first(*ScriptSpace + MainScriptSize, LargestMissionScriptSize, mission_fixes[i].pattern, mission_fixes[i].offset);
        if (addr)
        {
            injector.WriteMemoryFloat(addr, *(float*)addr / (60.0f / 30.0f));
    
            if (i == 6 && Enable60FPS) // In The Air Tonight
                Enable60FPS = 2;
        }
    }
    
    //Farewell To Arms (truck health)
    uintptr_t addr = range_pattern.get_first(*ScriptSpace + MainScriptSize, LargestMissionScriptSize, "2F 04 11 08 58 1B 4C 01 11 08 58 1B", 0);
    if (addr)
    {
        injector.WriteMemory16(addr + 4, 7000 * (int)(60.0f / 30.0f));
        injector.WriteMemory16(addr + 10, 7000 * (int)(60.0f / 30.0f));
    }

    //Light My Pyre (Lance's health)
    addr = range_pattern.get_first(*ScriptSpace + MainScriptSize, LargestMissionScriptSize, "0B 00 ? ? ? 0B", 0);
    if (addr)
    {
        injector.WriteMemory8(addr + 5, 11 * (int)(60.0f / 15.0f)); // adding extra health here
    }

    return 0;
}

uintptr_t GetAbsoluteAddress(uintptr_t at, int32_t offs_hi, int32_t offs_lo)
{
    return (uintptr_t)((uint32_t)(*(uint16_t*)(at + offs_hi)) << 16) + *(int16_t*)(at + offs_lo);
}

int NKCODE_A = 29;
int NKCODE_Z = 54;
int module_thread(SceSize args, void* argp)
{
    int cheatStringSize = sizeof(cheatString) / sizeof(cheatString[0]);
    while (1)
    {
        sceKernelDelayThread(1000);

        for (size_t i = NKCODE_A; i <= NKCODE_Z; i++)
        {
            char key = 0;
            sceIoDevctl("kemulator:", EMULATOR_DEVCTL__GET_VKEY, (void*)i, 0, &key, sizeof(key));
            if (key)
            {
                while (key)
                {
                    sceKernelDelayThread(1000);
                    sceIoDevctl("kemulator:", EMULATOR_DEVCTL__GET_VKEY, (void*)i, 0, &key, sizeof(key));
                }
                char cheatString2[20];
                memcpy(&cheatString2[0], &cheatString[0], cheatStringSize);
                memcpy(&cheatString[1], &cheatString2[0], cheatStringSize - 1);
                cheatString[0] = i + 36;
                cheatString[cheatStringSize - 1] = 0;
                HandlePCCheats();
                break;
            }
        }
    }
    sceKernelExitDeleteThread(0);
    return 0;
}

void HandlePCCheats()
{
#define _CHEATCMP(str) strncmp(str, cheatString, sizeof(str)-1)

    if (!_CHEATCMP("SLOOTSGUHT") || !_CHEATCMP("NUGDEKAN")) // THUGSTOOLS || NAKEDGUN                                          
        cheat_id = Weapon1;
    else if (!_CHEATCMP("SLOOTLANOISSEFORP") || !_CHEATCMP("NOPAEWLAHTEL")) // PROFESSIONALTOOLS || LETHALWEAPON                                  
        cheat_id = Weapon2;
    else if (!_CHEATCMP("SLOOTRETTUN") || !_CHEATCMP("NUGPOT")) // NUTTERTOOLS || TOPGUN                                                          
        cheat_id = Weapon3;
    else if (!_CHEATCMP("PLEHEMOSDEENI") || !_CHEATCMP("HSACYRRACSYAWLA")) // INEEDSOMEHELP|| ALWAYSCARRYCASH                                     
        cheat_id = Money;
    else if (!_CHEATCMP("NOITCETORPSUOICERP") || !_CHEATCMP("RALVEK")) // PRECIOUSPROTECTION || KEVLAR                              
        cheat_id = Armour;
    else if (!_CHEATCMP("ENIRIPSA") || !_CHEATCMP("ENIEDOC")) // ASPIRINE || CODEINE                                                 
        cheat_id = Health;
    else if (!_CHEATCMP("EVILAEMEKATTNOWUOY") || !_CHEATCMP("EMTAEMOC")) // YOUWONTTAKEMEALIVE || COMEATME                          
        cheat_id = WantedUp;
    else if (!_CHEATCMP("ENOLAEMEVAEL") || !_CHEATCMP("EMFFOYAL")) // LEAVEMEALONE || LAYOFFME                                      
        cheat_id = NeverWanted;
    else if (!_CHEATCMP("YADYLEVOLA") || !_CHEATCMP("TAEHECIV")) // ALOVELYDAY || VICEHEAT                                          
        cheat_id = SunnyWeather;
    else if (!_CHEATCMP("TOHNMADOOT") || !_CHEATCMP("YNNUSARTXE")) // TOODAMNHOT || EXTRASUNNY                                      
        cheat_id = ExtraSunny;
    else if (!_CHEATCMP("YADLLUDLLUD") || !_CHEATCMP("REHTAEWYDUOLC")) // DULLDULLDAY|| CLOUDYWEATHER                                             
        cheat_id = CloudyWeather;
    else if (!_CHEATCMP("VTHCTAWDNANIYATS") || !_CHEATCMP("RETSEHCNAMNI")) // STAYINANDWATCHTV || INMANCHESTER                      
        cheat_id = RainyWeather;
    else if (!_CHEATCMP("GNIHTAEESTNAC") || !_CHEATCMP("TSIMAES")) // CANTSEEATHING|| SEAMIST                                                     
        cheat_id = FoggyWeather;
    else if (!_CHEATCMP("REZNAP") || !_CHEATCMP("UOYKNAT")) // PANZER || TANKYOU                                                     
        cheat_id = Tank;
    else if (!_CHEATCMP("YBSEILFTSUJEMIT") || !_CHEATCMP("DESREVERYTIRALOP")) // TIMEJUSTFLIESBY|| POLARITYREVERSED                              
        cheat_id = FastWeather;
    else if (!_CHEATCMP("GNABGIB") || !_CHEATCMP("HSALFLANIF")) // BIGBANG|| FINALFLASH                                                          
        cheat_id = BlowUpCars;
    else if (!_CHEATCMP("SROODNIYATSRETTEB") || !_CHEATCMP("MEHYAM")) // BETTERSTAYINDOORS || MAYHEM                                              
        cheat_id = Mayhem;
    else if (!_CHEATCMP("DOOHRUOBHGIENHGUOR") || !_CHEATCMP("1YMENECILBUP")) // ROUGHNEIGHBOURHOOD || PUBLICENEMY1                  
        cheat_id = PedsAttackVic;
    else if (!_CHEATCMP("SRETTUNYBDEDNUORRUS") || !_CHEATCMP("ELASREPUSUMMA")) // SURROUNDEDBYNUTTERS || AMMUSUPERSALE                             
        cheat_id = WeaponsForAll;
    else if (!_CHEATCMP("PUTIDEEPS") || !_CHEATCMP("HSURAEKIL")) // SPEEDITUP|| LIKEARUSH                                                         
        cheat_id = FastTime;
    else if (!_CHEATCMP("NWODTIWOLS") || !_CHEATCMP("ENILANERDAEKIL")) // SLOWITDOWN || LIKEADRENALINE                              
        cheat_id = SlowTime;
    else if (!_CHEATCMP("GNIHTYREVESIPIRG") || !_CHEATCMP("PIRGGNORTS")) // GRIPISEVERYTHING || STRONGGRIP                          
        cheat_id = StrongGrip;
    else if (!_CHEATCMP("DLROWLEURCEYBDOOG") || !_CHEATCMP("DAEDYDAERLAERAUOY")) // GOODBYECRUELWORLD || YOUAREALREADYDEAD                         
        cheat_id = Suicide;
    else if (!_CHEATCMP("EMPOTSDNAYRTTNOD") || !_CHEATCMP("POTSOTEMITON")) // DONTTRYANDSTOPME || NOTIMETOSTOP                      
        cheat_id = TrafficLights;
    else if (!_CHEATCMP("SLANIMIRCERASREVIRDLLA") || !_CHEATCMP("ROIRRAWDAOR")) // ALLDRIVERSARECRIMINALS || ROADWARRIOR             
        cheat_id = MadCars;
    else if (!_CHEATCMP("KCALBDETNIAPTITNAWI") || !_CHEATCMP("SREDIRTHGIN")) // IWANTITPAINTEDBLACK || NIGHTRIDERS                                 
        cheat_id = BlackCars;
    else if (!_CHEATCMP("RACHSIBBUR") || !_CHEATCMP("KROWTANEM")) // RUBBISHCAR || MENATWORK                                         
        cheat_id = Trashmaster;
    else if (!_CHEATCMP("LLASREUQNOCEVOL") || !_CHEATCMP("ECIVRESTROCSE")) // LOVECONQUERSALL || ESCORTSERVICE                                     
        cheat_id = PickUpChicks;
    else if (!_CHEATCMP("TENGAMYNNAF") || !_CHEATCMP("48FOSSALC")) // FANNYMAGNET || CLASSOF84                                                     
        cheat_id = FannyMagnet;
    else if (!_CHEATCMP("EMORHCOSYNIHSOS")) // SOSHINYSOCHROME || SOSHINYSOCHROME                                 
        cheat_id = GlassCars;
    else if (!_CHEATCMP("YVRUTYSPOT")) // TOPSYTURVY || TOPSYTURVY                                                       
        cheat_id = TopsyTurvy;
    else
        cheat_id = -1;
}

float* CRect__CRect(float* dest, float f12, float f13, float f14, float f15)
{
    if (f12 == 0.0f && f14 == 480.0f)
    {
        dest[0] = f12;
        dest[3] = f13;
        dest[2] = f14;
        dest[1] = f15;
        return dest;
    }

    float fHudOffset = (((480.0f * fARDiff) - 480.0f) / 2.0f) / fARDiff;
    dest[0] = (f12 / fARDiff) + fHudOffset;
    dest[3] = f13;
    dest[2] = (f14 / fARDiff) + fHudOffset;
    dest[1] = f15;
    return dest;
}

float* CRect__CRect2(float* dest, float f12, float f13, float f14, float f15)
{
    if (f12 == 0.0f && f14 == 480.0f)
    {
        dest[0] = f12;
        dest[3] = f13;
        dest[2] = f14;
        dest[1] = f15;
        return dest;
    }

    float fHudOffset = ((((f14 - f12) * fARDiff) - (f14 - f12)) / 2.0f) / fARDiff;
    dest[0] = f12;
    dest[3] = f13;
    dest[2] = f14 - fHudOffset;
    dest[1] = f15;
    return dest;
}

int OnModuleStart() {
    sceKernelDelayThread(100000);

    int SkipIntro = inireader.ReadInteger("MAIN", "SkipIntro", 1);

    if (SkipIntro)
    {
        uintptr_t ptr = pattern.get_first("00 00 00 00 ? ? ? ? 00 00 00 00 ? ? 04 3C 25 28 00 00", -4);
        injector.MakeNOP(ptr);
    }

    UnthrottleEmuDuringLoading = inireader.ReadInteger("MAIN", "UnthrottleEmuDuringLoading", 1);

    if (UnthrottleEmuDuringLoading)
    {
        UnthrottleEmuEnable();
    }

    char szForceAspectRatio[100];
    int DualAnalogPatch = inireader.ReadInteger("MAIN", "DualAnalogPatch", 1);
    char* ForceAspectRatio = inireader.ReadString("MAIN", "ForceAspectRatio", "auto", szForceAspectRatio, sizeof(szForceAspectRatio));
    Enable60FPS = inireader.ReadInteger("MAIN", "Enable60FPS", 0);

    int ModernControlScheme = inireader.ReadInteger("CONTROLS", "ModernControlScheme", 0);
    PCCheats = inireader.ReadInteger("CONTROLS", "PCCheats", 0);

    float fHudScale = inireader.ReadFloat("HUD", "HudScale", 1.0f);

    float fRadarScale = inireader.ReadFloat("RADAR", "RadarScale", 1.0f);
    float fRadarPosX = inireader.ReadFloat("RADAR", "RadarPosX", 12.0f);
    float fRadarPosY = inireader.ReadFloat("RADAR", "RadarPosY", 196.0f);

    float fMPRadarScale = inireader.ReadFloat("MPRADAR", "RadarScale", 1.0f);
    float fMPRadarPosY = inireader.ReadFloat("MPRADAR", "RadarPosY", 196.0f);

    fFOVFactor = inireader.ReadFloat("FOV", "FOVFactor", 0.0f);
    if (fFOVFactor <= 0.0f)
        fFOVFactor = 1.0f;
    RestoreCutsceneFOV = inireader.ReadInteger("FOV", "RestoreCutsceneFOV", 1);

    LODDistMultiplier = inireader.ReadFloat("MISC", "LODDistMultiplier", 0.0f);

    if (DualAnalogPatch)
    {
        // Implement right analog stick
        uintptr_t ptr = pattern.get_first("06 00 00 10 03 00 A7 A3", 0);
        injector.WriteInstr(ptr + 0x24,
            sh(a1, sp, 0)
        );
        injector.WriteInstr(ptr + 0x1C,
            lhu(a1, sp, 0xE)
        );

        // Redirect camera movement
        ptr = pattern.get_first("36 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 16 00 40 10", 0);
        injector.MakeNOP(ptr + 0x00);
        injector.WriteMemory8(ptr + 0x10 + 0x2, 0x00);  // beqz -> b
        injector.MakeJAL(ptr + 0x8C, (intptr_t)CPad__GetRightStickX);
        injector.MakeNOP(ptr + 0x108);
        injector.WriteMemory8(ptr + 0x118 + 0x2, 0x00); // beqz -> b
        injector.MakeJAL(ptr + 0x144, (intptr_t)CPad__GetRightStickY);

        // Redirect gun aim movement
        ptr = pattern.get_first("40 00 80 04 ? ? ? ? 3E 00 80 10", 0);
        injector.MakeJAL(ptr + 0x50, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x7C, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x8C, (intptr_t)aimX);
        injector.MakeJAL(ptr + 0x158, (intptr_t)aimY);
        injector.MakeJAL(ptr + 0x1BC, (intptr_t)aimY);

        // Allow using L trigger when walking
        ptr = pattern.get_first("0E 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 08 00 80 10 ? ? ? ? ? ? ? ? 03 00 80 04", 0);
        injector.MakeNOP(ptr + 0x10);
        injector.MakeNOP(ptr + 0x9C);

        // Force L trigger value in the L+camera movement function
        //ptr = pattern.get_first("0A 00 C7 84", 0);
        //injector.WriteInstr(ptr + 0x00,
        //    li(a3, -0x1)
        //);
        ptr = pattern.get(0, "02 00 04 34 25 30 80 00", -4);
        injector.MakeNOP(ptr + 0x00);
    }

    if (ModernControlScheme)
    {
        // Swap R trigger and cross button
        uintptr_t ptr_1892D8 = pattern.get_first("9A 00 85 94 2B 28 05 00 FF 00 A5 30 03 00 A0 10 00 00 00 00 02 00 00 10 25 10 00 00 2A 00 82 84", 0);
        injector.MakeJMPwNOP(ptr_1892D8, (intptr_t)CPad__GetAccelerate);

        // Use normal button for flying plane
        uintptr_t ptr_1D5E14 = pattern.get_first("80 07 0E C6 02 63 0D 46 42 73 0D 46", 0);
        injector.MakeJAL(ptr_1D5E14 + 0x1C, (intptr_t)CPad__GetAccelerateBovver); //bovver64 and maybe more?
        injector.MakeJAL(ptr_1D5E14 + 0x3D0, (intptr_t)CPad__GetAccelerateNormal);

        // Use normal button for flying helicoper
        uintptr_t ptr_1EB29C = pattern.get_first("18 00 40 16 ? ? ? ? ? ? ? ? 25 20 20 02", 0);
        injector.MakeJAL(ptr_1EB29C + 0x14, (intptr_t)CPad__GetAccelerateNormal);

        uintptr_t ptr_18906C = pattern.get_first("0C 00 80 14 ? ? ? ? ? ? ? ? ? ? ? ? 0A 00 A0 50", 0);
        injector.WriteMemory16(ptr_18906C + 0x20, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector.WriteMemory16(ptr_18906C + 0x68, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector.WriteMemory16(ptr_18906C + 0x80, offsetof(struct CPad, NewState.CROSS));

        // Swap L trigger and square button
        uintptr_t ptr_187918 = pattern.get(0, "26 00 05 86 ? ? ? ? 00 00 04 34", 0);
        uintptr_t ptr_18886C = pattern.get(0, "0A 00 04 86 ? ? ? ? 00 00 00 00 ? ? ? ? 25 10 00 00", 0); // count = 3
        uintptr_t ptr_188AC0 = pattern.get(1, "0A 00 04 86 ? ? ? ? 00 00 00 00 ? ? ? ? 25 10 00 00", 0);
        uintptr_t ptr_188DAC = pattern.get(0, "0A 00 04 86 ? ? ? ? 00 00 00 00 ? ? ? ? 25 20 00 02", 0);
        uintptr_t ptr_189140 = pattern.get(4, "9A 00 85 94 2B 28 05 00 FF 00 A5 30 ? ? ? ? 00 00 00 00", 0); // count = 15
        uintptr_t ptr_189BB0 = pattern.get(2, "0A 00 04 86 ? ? ? ? 00 00 00 00 ? ? ? ? 25 10 00 00", 0);
        uintptr_t ptr_189D60 = pattern.get(0, "0A 00 04 86 ? ? ? ? 00 00 11 34", 0);
        uintptr_t ptr_18AAAC = pattern.get(0, "26 00 04 86 ? ? ? ? 00 00 00 00", 0);
        uintptr_t ptr_18B560 = pattern.get(2, "0A 00 04 86 ? ? ? ? ? ? ? ? ? ? ? ? 00 00 00 00", 0); // count = 3

        uintptr_t ptr_1EB2BC = pattern.get(0, "25 20 20 02 23 20 42 02", -4);
        uintptr_t ptr_188904 = pattern.get(0, "25 20 00 02 F6 FF 42 28 00 00 B0 8F", -4);
        uintptr_t ptr_188B58 = pattern.get(0, "25 20 00 02 0B 00 42 28 01 00 42 38", -4);
        uintptr_t ptr_188DFC = pattern.get(0, "01 00 42 38 25 10 00 00 04 00 B0 8F 08 00 B1 8F 0C 00 BF 8F 08 00 E0 03 10 00 BD 27", 4);
        uintptr_t ptr_1D5AE4 = pattern.get(0, "80 FF BD 27 54 00 B4 E7 58 00 B6 E7 5C 00 B8 E7", 0);

        uintptr_t ptr_5EE58 = pattern.get(0, "2C 00 82 84 ? ? ? ? 2E 00 82 84", 0);
        uintptr_t ptr_14CCA8 = pattern.get(2, "04 00 84 8C 27 00 05 34", 36);
        uintptr_t ptr_14D3F4 = pattern.get(1, "0A 00 64 86 ? ? ? ? 00 00 00 00", 0); // count = 2
        uintptr_t ptr_188118 = pattern.get(1, "02 00 07 34 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00", 12); // count = 2
        uintptr_t ptr_188078 = pattern.get(1, "2C 00 85 84 ? ? ? ? 00 00 06 34 5E 00 85 84", 0); // count = 2
        uintptr_t ptr_188084 = pattern.get(1, "5E 00 85 84 ? ? ? ? 28 00 87 84 01 00 06 34 28 00 87 84", 0); // count = 2
        uintptr_t ptr_188E14 = pattern.get(0, "EC FB 85 8F ? ? ? ? 00 00 00 00 9A 00 85 94", 0);
        uintptr_t ptr_18922C = pattern.get(6, "9A 00 85 94 2B 28 05 00 FF 00 A5 30 ? ? ? ? 00 00 00 00", 0); // count = 15
        uintptr_t ptr_189270 = pattern.get(7, "9A 00 85 94 2B 28 05 00 FF 00 A5 30 ? ? ? ? 00 00 00 00", 0); // count = 15
        uintptr_t ptr_189560 = pattern.get(7, "F0 FF BD 27 9A 00 85 94 00 00 B0 AF 25 80 80 00 2B 20 05 00", 0); // count = 11
        uintptr_t ptr_1895BC = pattern.get(8, "F0 FF BD 27 9A 00 85 94 00 00 B0 AF 25 80 80 00 2B 20 05 00", 0); // count = 11
        uintptr_t ptr_236F58 = pattern.get(0, "2C 00 44 86 ? ? ? ? 2C 00 45 86 2C 00 45 86 ? ? ? ? 00 00 04 34", 0);
        uintptr_t ptr_1898BC = pattern.get(0, "FF 00 A5 30 ? ? ? ? ? ? ? ? 00 00 00 00 0E 00 85 84 ? ? ? ? 00 00 00 00", 16); // count = 2
        uintptr_t ptr_189914 = pattern.get(1, "FF 00 A5 30 ? ? ? ? ? ? ? ? 00 00 00 00 0E 00 85 84 ? ? ? ? 00 00 00 00", 16); // count = 2

        injector.MakeJMPwNOP(ptr_189140, (intptr_t)CPad__GetBrake);
        injector.WriteMemory16(ptr_189D60, offsetof(struct CPad, NewState.SQUARE));
        //driveby
        uintptr_t ptr_18890C = pattern.get(0, "FF 00 82 30 ? ? ? ? 25 20 00 02 ? ? ? ? 00 00 B0 8F", 12);
        injector.WriteMemory16(ptr_18890C, -110); // look left threshold
        uintptr_t ptr_188B60 = pattern.get(0, "0B 00 42 28 01 00 42 38", -0);
        injector.WriteMemory16(ptr_188B60, 110);  // look right threshold

        injector.MakeNOP(ptr_18886C+4);
        MakeInlineWrapper(ptr_18886C,
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(ptr_188904),
            nop()
        );
        injector.MakeNOP(ptr_188AC0+4);
        MakeInlineWrapper(ptr_188AC0,
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)),
            _or(a0, a0, k0),
            beq(a0, zero, 2),
            nop(),
            j(ptr_188B58),
            nop()
        );
        injector.MakeNOP(ptr_188DAC+4);
        MakeInlineWrapper(ptr_188DAC,
            lh(a0, s0, offsetof(struct CPad, NewState.SQUARE)),
            //lh(k0, s0, offsetof(struct CPad, NewState.CIRCLE)), // fix for firetruck cannon
            _or(a0, a0, k0),
            bne(a0, zero, 2),
            nop(),
            j(ptr_188DFC),
            nop()
        );
        injector.WriteMemory16(ptr_189BB0, offsetof(struct CPad, NewState.SQUARE));
        injector.WriteMemory16(ptr_18B560, offsetof(struct CPad, NewState.SQUARE));

        injector.WriteMemory16(ptr_187918, offsetof(struct CPad, NewState.LEFTSHOULDER1));
        injector.WriteMemory16(ptr_18AAAC, offsetof(struct CPad, NewState.LEFTSHOULDER1));

        // Use normal button for flying plane
        injector.MakeJAL(ptr_1D5AE4 + 0x360, (intptr_t)CPad__GetBrakeBovver); //bovver64
        injector.MakeJAL(ptr_1D5AE4 + 0x628, (intptr_t)CPad__GetBrakeNormal);
        injector.MakeJAL(ptr_1D5AE4 + 0x660, (intptr_t)CPad__GetBrakeNormal);
        injector.MakeJAL(ptr_1D5AE4 + 0x690, (intptr_t)CPad__GetBrakeNormal);
        injector.MakeJAL(ptr_1D5AE4 + 0x6E4, (intptr_t)CPad__GetBrakeNormal);

        // Use normal button for flying helicoper
        injector.MakeJAL(ptr_1EB2BC, (intptr_t)CPad__GetBrakeNormal);

        //Shooting with triggers
        injector.MakeJMPwNOP(ptr_189560, (intptr_t)CPad__GetTarget);
        injector.MakeJMPwNOP(ptr_1895BC, (intptr_t)CPad__TargetJustDown);
        injector.MakeJMPwNOP(ptr_18922C, (intptr_t)CPad__GetWeapon);
        injector.MakeJMPwNOP(ptr_189270, (intptr_t)CPad__WeaponJustDown);
        injector.MakeJMPwNOP(ptr_188E14, (intptr_t)CPad__GetLookBehindForPed);
        //injector.WriteMemory16(ptr_188078, RIGHTSHOULDER1 * 2);
        injector.WriteMemory16(ptr_188084, offsetof(struct CPad, OldState.RIGHTSHOULDER1));
        injector.WriteMemory16(ptr_5EE58, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector.WriteMemory16(ptr_188118 + 2, 0x1000);
        injector.WriteMemory16(ptr_14CCA8 + 2, 0x1000);
        injector.MakeNOP(ptr_14D3F4);
        //injector.MakeNOP(0x1880F8);
        //ShiftTargetJustDown
        injector.WriteMemory16(ptr_1898BC, offsetof(struct CPad, OldState.LEFTSHOULDER1));
        injector.WriteMemory16(ptr_189914, offsetof(struct CPad, OldState.LEFTSHOULDER1));

        //melee
        injector.WriteMemory16(ptr_236F58 + 0, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector.WriteMemory16(ptr_236F58 + 8, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
        injector.WriteMemory16(ptr_236F58 + 12, offsetof(struct CPad, NewState.RIGHTSHOULDER1));

        //reloading
        ptr_E2D44 = pattern.get(0, "39 00 06 34 ? ? ? ? 00 00 00 00 28 00 8C C6", -4);
        ptr_E2D44_data = *(uint32_t*)ptr_E2D44;
        sub_8B1FD70 = (void*)pattern.get(0, "00 21 04 00 ? ? ? ? ? ? ? ? 23 20 E4 00 00 00 A2 8C", -8);
        FindPlayerPed = (void*)pattern.get(0, "00 32 04 00 21 20 85 00 40 21 04 00 ? ? ? ? 21 20 C4 00 ? ? ? ? 21 20 85 00 08 00 E0 03 00 00 82 8C", -8);
        FindPlayerVehicle = (void*)pattern.get(0, "21 28 84 00 00 32 04 00 21 20 85 00 00 00 B0 AF", -8);
        dword_8BB3B4C = *(int16_t*)pattern.get(0, "25 B8 40 00 ? ? ? ? 70 00 05 8E 23 20 85 00", 4);
        uintptr_t ptr_14AF54 = pattern.get(0, "25 20 20 02 ? ? ? ? 00 00 00 00 B4 08 04 8E", -4);
        injector.MakeJAL(ptr_14AF54, (intptr_t)CPad__JumpJustDown);

        //allow camera move while reloading
        uintptr_t ptr_1477DC = pattern.get(1, "01 00 05 34 ? ? ? ? 00 00 00 00 ? ? ? ? 20 00 84 30 ? ? ? ? 00 00 00 00", 0);
        injector.MakeNOP(ptr_1477DC + 4);
        injector.MakeNOP(ptr_1477DC + 20);
        injector.MakeNOP(ptr_1477DC + 32);

        // Free aim
        uintptr_t ptr_1880E8 = pattern.get(0, "96 00 86 94 ? ? ? ? ? ? ? ? 0A 00 85 84", -4);
        injector.MakeJMPwNOP(ptr_1880E8, (intptr_t)CPad__EnterFreeAim);
    }

    if (strcmp(ForceAspectRatio, "auto") != 0)
    {
        char* ch;
        ch = strtok(ForceAspectRatio, ":");
        int x = str2int(ch, 10);
        ch = strtok(NULL, ":");
        int y = str2int(ch, 10);
        fAspectRatio = (float)x / (float)y;
    }
    else
    {
        float ar = 0.0f;
        sceIoDevctl("kemulator:", EMULATOR_DEVCTL__GET_ASPECT_RATIO, NULL, 0, &ar, sizeof(ar));
        if (ar)
            fAspectRatio = ar;
    }
    
    uintptr_t ptr_130C4C = pattern.get(0, "E3 3F 05 3C 39 8E A5 34 00 68 85 44 25 28 00 00", 0);
    injector.MakeInlineLUIORI(ptr_130C4C, fAspectRatio);

    //if (strcmp(ForceAspectRatio, "auto") != 0 || fFOVFactor)
    {
        uintptr_t ptr_21FE98 = pattern.get(0, "00 63 00 46 ? ? ? ? ? ? ? ? 25 88 40 00", -4);
        MakeInlineWrapper(ptr_21FE98,
            jal((intptr_t)sub_171D44),
            nop(),
            swc1(f0, gp, -0x4250)
        );
    }

    if (Enable60FPS)
    {
        //60 fps
        ptr_2030C8 = pattern.get(0, "02 00 84 2C ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00", 20);
        ptr_2030C8_data = *(uint32_t*)ptr_2030C8;
        injector.MakeNOP(ptr_2030C8);

        ptr_2B7F48 = pattern.get(0, "23 20 44 00 ? ? ? ? 25 10 00 00", -4);
        ptr_8BB3CA0 = *(int16_t*)pattern.get(0, "25 20 00 00 ? ? ? ? ? ? ? ? 00 00 10 34 00 00 40 AE", 8);

        sub_89F6390 = (void*)pattern.get(0, "04 00 B0 AF 08 00 B1 AF 25 80 A0 00 25 88 80 00 00 00 A0 A3", -4);
        ptr_14F7E8 = pattern.get(0, "80 20 04 00 21 20 A4 00 00 00 90 8C ? ? ? ? 58 01 10 26", -8);

        //mission fixes
        uintptr_t ptr_2B8180 = pattern.get(0, "25 28 00 00 ? ? ? ? 25 20 00 02 25 10 00 00", 16);
        MakeInlineWrapper(ptr_2B8180,
            jal((intptr_t)applyMissionFixes),
            nop(),
            lw(s0, sp, 0)
        );
    }

    fARDiff = fAspectRatio / (16.0f / 9.0f);

    if (fHudScale > 0.0f)
    {
        /* Patterns */
        uintptr_t ptr_1B74C8 = pattern.get(0, "B8 43 05 3C", 0);
        uintptr_t ptr_1B7570 = pattern.get(0, "E0 40 07 3C 06 A3 00 46", 0);
        uintptr_t ptr_1B7594 = pattern.get(0, "70 42 07 3C 06 A3 00 46", 0);
        uintptr_t ptr_1B7C98 = pattern.get(0, "8B 3F 04 3C 80 42 04 3C 06 D6 00 46", 0);
        uintptr_t ptr_1B7CA8 = pattern.get(0, "8B 3F 04 3C 1F 85 84 34 00 60 84 44", 0); // count = 2
        uintptr_t ptr_1B7CB8 = pattern.get(0, "B0 43 04 3C ? ? ? ? 28 42 05 3C", 0);
        uintptr_t ptr_1B7CC0 = pattern.get(0, "28 42 05 3C 68 42 06 3C", 0);
        uintptr_t ptr_1B7CC4 = pattern.get(0, "68 42 06 3C", 0);
        uintptr_t ptr_1B7D40 = pattern.get(0, "D2 43 05 3C 06 F3 00 46 48 E1 A5 34 46 A3 00 46", 0);
        uintptr_t ptr_1B7EEC = pattern.get(0, "A0 3F 04 3C ? ? ? ? 00 60 84 44 25 20 00 02 6A 01 05 34", 0); // count = 2
        uintptr_t ptr_1B7EFC = pattern.get(0, "6A 01 05 34 ? ? ? ? 23 00 06 34", 0);
        uintptr_t ptr_1B7F04 = pattern.get(0, "23 00 06 34 ? ? ? ? ? ? ? ? 40 3F 04 3C", 0);
        uintptr_t ptr_1B7F10 = pattern.get(0, "40 3F 04 3C ? ? ? ? 00 60 84 44 25 20 00 02 6E 01 05 34", 0); // count = 2
        uintptr_t ptr_1B7F20 = pattern.get(0, "6E 01 05 34 ? ? ? ? 28 00 06 34", 0);
        uintptr_t ptr_1B7F28 = pattern.get(0, "6E 01 05 34 ? ? ? ? 28 00 06 34", 8);
        uintptr_t ptr_1B8A18 = pattern.get(0, "D8 41 05 3C", 0);
        uintptr_t ptr_1B8A1C = pattern.get(0, "A0 42 06 3C ? ? ? ? D0 41 07 3C", 0);
        uintptr_t ptr_1B8A24 = pattern.get(0, "D0 41 07 3C 3A 43 08 3C", 0);
        uintptr_t ptr_1B8A28 = pattern.get(0, "3A 43 08 3C", 0);
        uintptr_t ptr_1B8AF0 = pattern.get(0, "4C 3F 04 3C 02 63 18 46", 0);
        uintptr_t ptr_1B8B5C = pattern.get(0, "24 42 05 3C 02 63 18 46", 0);
        uintptr_t ptr_1B8B6C = pattern.get(0, "20 42 06 3C 48 00 04 34", 0);
        uintptr_t ptr_1B8B90 = pattern.get(0, "F8 41 07 3C", 0);
        uintptr_t ptr_1B8B94 = pattern.get(0, "29 43 08 3C", 0);
        uintptr_t ptr_1B8B98 = pattern.get(0, "50 42 09 3C", 0);
        uintptr_t ptr_1B8BB4 = pattern.get(0, "1C 42 05 3C", 0);
        uintptr_t ptr_1B96F8 = pattern.get(0, "B8 43 07 3C", 0);
        uintptr_t ptr_1B9708 = pattern.get(0, "00 41 07 3C 25 28 A0 03", 0);
        uintptr_t ptr_1B97A8 = pattern.get(0, "50 41 04 3C ? ? ? ? 00 B0 84 44", 0);
        uintptr_t ptr_1B992C = pattern.get(0, "80 41 04 3C 00 78 84 44 25 20 A0 02", 0);
        uintptr_t ptr_1B99B4 = pattern.get(0, "80 40 04 3C 00 60 84 44 01 A5 0C 46", 0);
        uintptr_t ptr_1B9B00 = pattern.get(0, "E2 43 04 3C", 0);
        uintptr_t ptr_1B9B0C = pattern.get(0, "9A 42 04 3C", 0);
        uintptr_t ptr_1B9B18 = pattern.get(0, "88 41 04 3C 00 A0 84 44", 0);
        uintptr_t ptr_1B9DB8 = pattern.get(0, "B0 43 05 3C ? ? ? ? ? ? ? ? B0 43 05 3C 04 00 05 34", 0);
        uintptr_t ptr_1B9DC4 = pattern.get(0, "B0 43 05 3C 04 00 05 34", 0);
        uintptr_t ptr_1B9DD0 = pattern.get(0, "B0 43 05 3C 02 00 04 34", 0);
        uintptr_t ptr_1B9DF8 = pattern.get(0, "B0 43 05 3C 03 00 04 34", 0);
        uintptr_t ptr_1B9E04 = pattern.get(0, "B0 43 05 3C 00 60 85 44", 0);
        uintptr_t ptr_1B9E10 = pattern.get(0, "24 42 05 3C 00 68 85 44", 0);
        uintptr_t ptr_1B9E18 = pattern.get(0, "D0 43 05 3C", 0);
        uintptr_t ptr_1B9E20 = pattern.get(0, "C8 41 05 3C ? ? ? ? 00 78 85 44", 0);
        uintptr_t ptr_1BB728 = pattern.get(0, "8B 3F 04 3C 80 42 04 3C 86 A6 00 46", 0);
        uintptr_t ptr_1BB738 = pattern.get(1, "8B 3F 04 3C 1F 85 84 34 00 60 84 44", 0);
        uintptr_t ptr_1BB748 = pattern.get(0, "B0 43 04 3C ? ? ? ? C8 41 05 3C", 0);
        uintptr_t ptr_1BB750 = pattern.get(0, "C8 41 05 3C 24 42 06 3C", 0);
        uintptr_t ptr_1BB754 = pattern.get(0, "24 42 06 3C", 0);
        uintptr_t ptr_1BB7D0 = pattern.get(0, "D2 43 05 3C 06 F3 00 46 48 E1 A5 34 46 B3 00 46", 0);
        uintptr_t ptr_1BB97C = pattern.get(1, "A0 3F 04 3C ? ? ? ? 00 60 84 44 25 20 00 02 6A 01 05 34", 0);
        uintptr_t ptr_1BB98C = pattern.get(0, "6A 01 05 34 ? ? ? ? 12 00 06 34", 0);
        uintptr_t ptr_1BB994 = pattern.get(0, "12 00 06 34 ? ? ? ? ? ? ? ? 40 3F 04 3C", 0);
        uintptr_t ptr_1BB9A0 = pattern.get(1, "40 3F 04 3C ? ? ? ? 00 60 84 44 25 20 00 02 6E 01 05 34", 0);
        uintptr_t ptr_1BB9B0 = pattern.get(0, "6E 01 05 34 ? ? ? ? 17 00 06 34", 0);
        uintptr_t ptr_1BB9B8 = pattern.get(0, "6E 01 05 34 ? ? ? ? 17 00 06 34", 8);
        uintptr_t ptr_1C0EF8 = pattern.get(0, "99 3F 04 3C 9A 99 84 34 ? ? ? ? 00 60 84 44 FF 00 04 34", 0);
        uintptr_t ptr_1C0F24 = pattern.get(0, "ED 43 04 3C 00 80 84 34 00 B0 84 44 52 43 04 3C", 0);
        uintptr_t ptr_1C0F30 = pattern.get(0, "52 43 04 3C ? ? ? ? 00 C0 84 44", 0);
        uintptr_t ptr_1C1274 = pattern.get(0, "E3 43 04 3C", 0);
        uintptr_t ptr_1C1280 = pattern.get(0, "04 42 05 3C", 0);
        uintptr_t ptr_1C1284 = pattern.get(0, "1C 42 04 3C", 0);
        uintptr_t ptr_1C128C = pattern.get(0, "00 42 06 3C 00 68 84 44", 0);
        uintptr_t ptr_1C1304 = pattern.get(0, "D3 43 05 3C 00 60 85 44", 0);
        uintptr_t ptr_1C1310 = pattern.get(0, "E0 40 05 3C 00 68 85 44 25 20 A0 02", 0);
        uintptr_t ptr_1C131C = pattern.get(0, "F4 43 05 3C", 0);
        uintptr_t ptr_1C1324 = pattern.get(0, "8E 42 05 3C", 0);
        uintptr_t ptr_1C13B8 = pattern.get(0, "D4 3E 04 3C", 0);
        uintptr_t ptr_1C14DC = pattern.get(0, "EC 43 05 3C", 0);
        uintptr_t ptr_1C14E8 = pattern.get(0, "34 42 06 3C 6C 42 05 3C", 0);
        uintptr_t ptr_1C14EC = pattern.get(0, "6C 42 05 3C", 0);
        uintptr_t ptr_1C162C = pattern.get(0, "E0 43 05 3C", 0);
        uintptr_t ptr_1C1644 = pattern.get(0, "D4 43 05 3C 86 D3 00 46", 0);
        uintptr_t ptr_1C1688 = pattern.get(0, "E3 43 05 3C 06 D3 00 46", 0);
        uintptr_t ptr_1C1744 = pattern.get(0, "D3 43 05 3C 46 A3 00 46", 0);
        uintptr_t ptr_1C1C30 = pattern.get(3, "99 3F 04 3C 9A 99 84 34 ? ? ? ? 00 60 84 44", 0); // count = 4
        uintptr_t ptr_1C1C7C = pattern.get(0, "ED 43 04 3C 00 80 84 34 00 A0 84 44", 0);
        uintptr_t ptr_1C1C88 = pattern.get(0, "6A 43 04 3C", 0);
        uintptr_t ptr_2ABCBC = pattern.get(0, "8B 3F 04 3C 1F 85 84 34 ? ? ? ? 02 E3 0C 46", 0);
        uintptr_t ptr_1B9924 = pattern.get(0, "80 A3 0C 46 1C 00 AD C7", 0);
        uintptr_t ptr_17013C = pattern.get(0, "08 00 E0 03 20 00 80 C4 08 00 E0 03 24 00 85 A0 08 00 E0 03 24 00 82 90", 0);
        uintptr_t ptr_16F9B0 = pattern.get(0, "06 00 24 96 00 68 84 44", -8);
        uintptr_t ptr_170DD8 = pattern.get(0, "25 20 20 02 06 06 00 46 ? ? ? ? 25 20 20 02", -4); // count = 2
        uintptr_t ptr_170FF4 = pattern.get(1, "25 20 20 02 06 06 00 46 ? ? ? ? 25 20 20 02", -4); // count = 2
        uintptr_t ptr_9804 = pattern.get(0, "00 78 85 44 80 63 0F 46", -4);
        uintptr_t ptr_B6DC = pattern.get(0, "02 00 0E 46 00 60 00 46 04 00 00 E5", 0);
        uintptr_t ptr_C9CC = pattern.get(0, "06 E4 00 46 86 B4 00 46", 0);
        uintptr_t ptr_214638 = pattern.get(0, "00 78 89 44 00 60 8A 44 00 68 8B 44", 0);
        uintptr_t ptr_2A43F0 = pattern.get(0, "F0 43 04 3C 02 63 0D 46", 0);
        uintptr_t ptr_1B9438 = pattern.get(0, "80 40 04 3C 00 A0 84 44 25 20 00 02", 0);
        uintptr_t ptr_1B97C0 = pattern.get(0, "01 B3 0C 46 24 00 A4 AF", 0);

        /* Health bar */
        injector.MakeInlineLUIORI(ptr_1B7CB8, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B7CC0, adjustTopRightY(42.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1B7D40, adjustRightX(421.76f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1B7CC4, adjustTopRightY(58.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_1B7CA8, fHudScale * 1.09f / fARDiff); // 1%
        injector.MakeInlineLUIORI(ptr_1B7C98, fHudScale * 1.085938f / fARDiff); // 1%

        injector.MakeInlineLI(ptr_1B7F20, (int32_t)(adjustRightX(366.0f, fHudScale / fARDiff)));
        injector.MakeInlineLI(ptr_1B7F28, (int32_t)(adjustTopRightY(40.0f, fHudScale)));
        injector.MakeInlineLUIORI(ptr_1B7F10, fHudScale * 0.75f); // Small "+" Font scale

        injector.MakeInlineLI(ptr_1B7EFC, (int32_t)(adjustRightX(362.0f, fHudScale)));
        injector.MakeInlineLI(ptr_1B7F04, (int32_t)(adjustTopRightY(35.0f, fHudScale)));
        injector.MakeInlineLUIORI(ptr_1B7EEC, fHudScale * 1.25f); // Large "+" Font scale

        /* Armor bar */
        injector.MakeInlineLUIORI(ptr_1BB748, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1BB750, adjustTopRightY(25.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1BB7D0, adjustRightX(421.76f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1BB754, adjustTopRightY(41.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_1BB738, fHudScale * 1.09f / fARDiff); // 1%
        injector.MakeInlineLUIORI(ptr_1BB728, fHudScale * 1.085938f / fARDiff); // 1%

        injector.MakeInlineLI(ptr_1BB9B0, (int32_t)(adjustRightX(366.0f, fHudScale)));
        injector.MakeInlineLI(ptr_1BB9B8, (int32_t)(adjustTopRightY(23.0f, fHudScale)));
        injector.MakeInlineLUIORI(ptr_1BB9A0, fHudScale * 0.75f); // Small "+" Font scale

        injector.MakeInlineLI(ptr_1BB98C, (int32_t)(adjustRightX(362.0f, fHudScale)));
        injector.MakeInlineLI(ptr_1BB994, (int32_t)(adjustTopRightY(18.0f, fHudScale)));
        injector.MakeInlineLUIORI(ptr_1BB97C, fHudScale * 1.25f); // Large "+" Font scale

        /* Oxygen bar */
        injector.MakeInlineLUIORI(ptr_1B9DB8, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9DC4, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9DD0, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9DF8, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9E04, adjustRightX(352.0f, fHudScale)); // Left X
        injector.MakeInlineLUIORI(ptr_1B9E20, adjustTopRightY(25.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1B9E18, adjustRightX(352.0f, fHudScale) + 64.0f); // Right X
        injector.MakeInlineLUIORI(ptr_1B9E10, adjustTopRightY(41.0f, fHudScale)); // Bottom Y
        injector.MakeInlineLUIORI(ptr_2ABCBC, fHudScale * 1.09f / fARDiff); // 1%

        /* Time, cash numbers, wanted stars */
        MakeInlineWrapper(ptr_1B97C0,
            lui(t9, HIWORD(fARDiff)),
            ori(t9, t9, LOWORD(fARDiff)),
            mtc1(t9, f30),
            divs(f22, f22, f30),
            divs(f12, f12, f30),
            lui(t9, HIWORD(fHudScale)),
            ori(t9, t9, LOWORD(fHudScale)),
            mtc1(t9, f30),
            muls(f22, f22, f30),
            subs(f12, f22, f12)
        );
        injector.MakeInlineLUIORI(ptr_1B992C, fHudScale * 16.0f); // Height
        injector.MakeInlineLUIORI(ptr_1B99B4, 4.0f / fARDiff);

        injector.MakeInlineLUIORI(ptr_1B96F8, adjustRightX(369.0f, fHudScale)); // Time Left X
        injector.MakeInlineLUIORI(ptr_1B9708, adjustTopRightY(8.0f, fHudScale)); // Time Top Y

        injector.MakeInlineLUIORI(ptr_1B74C8, adjustRightX(369.0f, fHudScale)); // Cash Left X
        injector.MakeInlineLUIORI(ptr_1B7570, adjustTopRightY(7.0f, fHudScale)); // Cash Top Y
        injector.MakeInlineLUIORI(ptr_1B7594, adjustTopRightY(60.0f, fHudScale)); // Cash Top Y (2)

        injector.MakeInlineLUIORI(ptr_1B9B00, adjustRightX(452.0f, fHudScale)); // Wanted stars Right X
        injector.MakeInlineLUIORI(ptr_1B9B0C, adjustTopRightY(77.0f, fHudScale)); // Wanted stars Top Y
        injector.MakeInlineLUIORI(ptr_1B9B18, fHudScale * 17.0f / fARDiff); // Wanted stars Width

        /* Weapon icon & ammo numbers */
        injector.MakeInlineLUIORI(ptr_1C1304, adjustRightX(422.0f, fHudScale)); // Fist icon Left X
        injector.MakeInlineLUIORI(ptr_1C1310, adjustTopRightY(7.0f, fHudScale)); // Fist icon Top Y
        injector.MakeInlineLUIORI(ptr_1C131C, adjustRightX(488.0f, fHudScale)); // Fist icon Right X
        injector.MakeInlineLUIORI(ptr_1C1324, adjustTopRightY(71.0f, fHudScale)); // Fist icon Bottom Y

        injector.MakeInlineLUIORI(ptr_1C1274, adjustRightX(455.0f, fHudScale)); // Weapon icon Center X
        injector.MakeInlineLUIORI(ptr_1C1284, adjustTopRightY(39.0f, fHudScale));  // Weapon icon Center Y
        injector.MakeInlineLUIORI(ptr_1C1280, fHudScale * 33.0f / fARDiff); // Weapon icon Width
        injector.MakeInlineLUIORI(ptr_1C128C, fHudScale * 32.0f); // Weapon icon Height
        injector.MakeInlineLUIORI(ptr_1C14E8, adjustTopRightY(45.0f, fHudScale)); // Ammo Top Y
        injector.MakeInlineLUIORI(ptr_1C14EC, adjustTopRightY(59.0f, fHudScale)); // Ammo Bottom Y
        injector.MakeInlineLUIORI(ptr_1C1744, adjustRightX(422.0f, fHudScale)); // Single clip ammo number Left X
        injector.MakeInlineLUIORI(ptr_1C1644, adjustRightX(425.0f, fHudScale)); // Ammo Left X
        injector.MakeInlineLUIORI(ptr_1C162C, adjustRightX(449.0f, fHudScale)); // '-' Left X
        injector.MakeInlineLUIORI(ptr_1C1688, adjustRightX(455.0f, fHudScale)); // Clip ammo Left X
        injector.MakeInlineLUIORI(ptr_1C14DC, adjustRightX(473.0f, fHudScale)); // Clip ammo Right X
        injector.MakeInlineLUIORI(ptr_1C13B8, fHudScale * 0.415f); // Font scale

        /* Zone name */
        injector.MakeInlineLUIORI(ptr_1C1C7C, adjustRightX(475.0f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1C1C88, adjustBottomRightY(234.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1C1C30, fHudScale * 1.2f); // Font scale

        /* Vehicle name */
        injector.MakeInlineLUIORI(ptr_1C0F24, adjustRightX(475.0f, fHudScale)); // Right X
        injector.MakeInlineLUIORI(ptr_1C0F30, adjustBottomRightY(210.0f, fHudScale)); // Top Y
        injector.MakeInlineLUIORI(ptr_1C0EF8, fHudScale * 1.2f); // Font scale

        /* Pager */
        //injector.MakeInlineLUIORI(ptr_1B8A24, fHudScale * 26.0f); // Left X
        //injector.MakeInlineLUIORI(ptr_1B8A18, fHudScale * 27.0f); // Top Y
        //injector.MakeInlineLUIORI(ptr_1B8A28, fHudScale * 186.0f); // Right X
        //injector.MakeInlineLUIORI(ptr_1B8A1C, fHudScale * 80.0f); // Height
        //injector.MakeInlineLUIORI(ptr_1B8B6C, fHudScale * 40.0f); // Text rectangle Left X
        //injector.MakeInlineLUIORI(ptr_1B8B5C, fHudScale * 41.0f); // Text rectangle Top Y
        //injector.MakeInlineLUIORI(ptr_1B8BB4, fHudScale * 39.0f); // Text rectangle Left X
        //injector.MakeInlineLUIORI(ptr_1B8B90, fHudScale * 31.0f); // Text rectangle Top Y
        //injector.MakeInlineLUIORI(ptr_1B8B94, fHudScale * 169.0f); // Text rectangle Right X
        //injector.MakeInlineLUIORI(ptr_1B8B98, fHudScale * 52.0f); // Text rectangle Bottom Y
        //injector.MakeInlineLUIORI(ptr_1B8AF0, fHudScale * 0.8f); // Font scale

        /* Text */
        injector.MakeJMPwNOP(ptr_17013C, (intptr_t)TextScaling);
        injector.MakeJAL(ptr_16F9B0, (intptr_t)TextScalingVertical);
        injector.MakeJAL(ptr_170FF4, (intptr_t)TextScalingVertical);
        injector.MakeJAL(ptr_170DD8, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16F8F4, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16F91C, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16F958, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16F980, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16FBF8, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x16FC38, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170264, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x17029C, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x17032C, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170364, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170548, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170580, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170628, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170660, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170BAC, (intptr_t)TextScalingVertical);
        //injector.MakeJAL(0x170C20, (intptr_t)TextScalingVertical);

        /*Radar Blips*/
        float f6 = 6.0f;
        float f6_pequeno = f6 / fARDiff;
        if (fRadarScale)
        {
            f6 = f6 * fRadarScale;
            f6_pequeno = f6_pequeno * fRadarScale;
        }
        MakeInlineWrapper(ptr_9804,
            lui(a1, HIWORD(f6_pequeno)),
            ori(a1, a1, LOWORD(f6_pequeno)),
            mtc1(a1, f30),
            lui(a1, HIWORD(f6)),
            ori(a1, a1, LOWORD(f6))
        );
        injector.WriteInstr(ptr_9804 + 8, adds(f14, f12, f30));
        injector.WriteInstr(ptr_9804 + 0x24, subs(f12, f12, f30));

        //CRadar::DrawRotatingRadarSprite
        MakeInlineWrapper(ptr_B6DC,
            lui(t2, HIWORD(f6_pequeno)),
            ori(t2, t2, LOWORD(f6_pequeno)),
            mtc1(t2, f14),
            muls(f0, f0, f14),
            lui(t2, HIWORD(f6)),
            ori(t2, t2, LOWORD(f6)),
            mtc1(t2, f14)
        );

        //Crosshair
        float fCross = fARDiff * 1.125f;
        MakeInlineWrapper(ptr_C9CC,
            movs(f16, f28),
            lui(t9, HIWORD(fCross)),
            ori(t9, t9, LOWORD(fCross)),
            mtc1(t9, f29),
            divs(f28, f28, f29)
        );
        MakeInlineWrapper(ptr_214638,
            mtc1(t1, f15),
            mtc1(t1, f16),
            lui(t1, HIWORD(fCross)),
            ori(t1, t1, LOWORD(fCross)),
            mtc1(t1, f29),
            divs(f15, f15, f29)
        );
        injector.MakeNOP(ptr_214638 + 0x10);

        //Sprites
        injector.MakeInlineLUIORI(ptr_2A43F0, 480.0f * fARDiff);

        // Radar Disc
        float f4 = 4.0f;
        float f4_new = 4.0f / fARDiff;
        MakeInlineWrapper(ptr_1B9438,
            lui(a0, HIWORD(f4_new)),
            ori(a0, a0, LOWORD(f4_new)),
            mtc1(a0, f30),
            lui(a0, HIWORD(f4)),
            ori(a0, a0, LOWORD(f4))
        );
        injector.WriteInstr(ptr_1B9438 + 0x10, subs(f22, f0, f30));
        injector.WriteInstr(ptr_1B9438 + 0x38, adds(f28, f12, f30));

        // Controller icons in menu
        uintptr_t ptr_89043E8 = pattern.get(0, "E0 7B 80 46 48 00 04 26", -4);
        injector.MakeJAL(ptr_89043E8, (intptr_t)CRect__CRect2);
        
        //Map Legend Text
        uintptr_t ptr_88116CC = pattern.get(0, "E0 7B 80 46 04 00 A6 27", -4);
        injector.MakeJAL(ptr_88116CC, (intptr_t)CRect__CRect2);

        //ControllerMenu
        uintptr_t ptr_8B3FD50 = pattern.get(0, "E0 7B 80 46 25 20 A0 03 FF 00 05 34 FF 00 06 34 FF 00 07 34 ? ? ? ? FF 00 08 34 25 20 00 02 ? ? ? ? 25 28 40 00 11 01 44 92", -4);
        injector.MakeJAL(ptr_8B3FD50, (intptr_t)CRect__CRect);
        uintptr_t ptr_8B3FE38 = pattern.get(0, "80 63 0E 46 25 20 A0 03", -4);
        injector.MakeJAL(ptr_8B3FE38, (intptr_t)CRect__CRect);
        uintptr_t ptr_8B3FF0C = pattern.get(0, "E0 7B 80 46 25 20 A0 03 FF 00 05 34 FF 00 06 34 FF 00 07 34 ? ? ? ? FF 00 08 34 25 20 00 02 ? ? ? ? 25 28 40 00 08 01 44 8E", -4);
        injector.MakeJAL(ptr_8B3FF0C, (intptr_t)CRect__CRect);
    }

    /* Radar */
    {
        uintptr_t ptr_1B6A8C = pattern.get(0, "40 41 05 3C ? ? ? ? 00 00 85 44", 0);
        uintptr_t ptr_1B6AB4 = pattern.get(0, "2A 43 04 3C ? ? ? ? 00 00 84 44", 0);
        uintptr_t ptr_1B6AC0 = pattern.get(0, "44 43 04 3C 00 00 84 44", 0);
        uintptr_t ptr_1B6ADC = pattern.get(0, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0); // count = 2
        uintptr_t ptr_1B6AE8 = pattern.get(0, "83 42 04 3C 9A 99 84 34 00 00 84 44", 0); // count = 2
        uintptr_t ptr_1B6B08 = pattern.get(1, "B8 42 04 3C ? ? ? ? 00 00 84 44", 0);
        uintptr_t ptr_1B6B14 = pattern.get(1, "83 42 04 3C 9A 99 84 34 00 00 84 44", 0);

        if (fRadarScale > 0.0f)
        {
            if (fRadarPosX)
                injector.MakeInlineLUIORI(ptr_1B6A8C, fRadarScale * fRadarPosX);  // Left X
            else
                injector.MakeInlineLUIORI(ptr_1B6A8C, fRadarScale * 12.0f);  // Left X

            if (fRadarPosY)
                injector.MakeInlineLUIORI(ptr_1B6AC0, adjustBottomRightY(fRadarPosY, fRadarScale)); // Top Y
            else
                injector.MakeInlineLUIORI(ptr_1B6AC0, adjustBottomRightY(196.0f, fRadarScale)); // Top Y

            injector.MakeInlineLUIORI(ptr_1B6AE8, fRadarScale * 65.8f / fARDiff);  // Size X
            injector.MakeInlineLUIORI(ptr_1B6B14, fRadarScale * 65.8f);  // Size Y
        }

        if (fMPRadarScale > 0.0f)
        {
            if (fMPRadarPosY)
                injector.MakeInlineLUIORI(ptr_1B6AB4, adjustBottomRightY(fMPRadarPosY, (fMPRadarScale ? fMPRadarScale : 1.0f))); // Top Y (Multiplayer)
            injector.MakeInlineLUIORI(ptr_1B6ADC, fMPRadarScale * 92.0f / fARDiff);  // Size X (Multiplayer)
            injector.MakeInlineLUIORI(ptr_1B6B08, fMPRadarScale * 92.0f);  // Size Y (Multiplayer)
        }
    }

    uintptr_t ptr_3CF00 = pattern.get(0, "25 20 20 02 30 00 64 26 00 00 80 D8 00 00 41 D8", -16);
    TheCamera = (uintptr_t)((uint32_t)(*(uint16_t*)(ptr_3CF00 + 0)) << 16) + *(int16_t*)(ptr_3CF00 + 4);

    {
        ptr_1334C4 = *(int16_t*)pattern.get(0, "00 60 80 44 ? ? ? ? 25 20 80 02 ? ? ? ? 25 28 00 00 ? ? ? ? ? ? ? ? 25 20 80 02", 4);
        uintptr_t ptr_1336C4 = pattern.get(0, "00 00 04 34 00 00 05 34 ? ? ? ? 00 00 04 34 ? ? ? ? FF 00 05 34", 0);
        ptr_3C5100 = GetAbsoluteAddress(ptr_1336C4, -12, -4);
        
        uintptr_t ptr_133A5C = pattern.get(0, "1C 00 BF AF ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 01 00 15 34", 16);
        injector.MakeJAL(ptr_133A5C, (intptr_t)GameLoopStuff);
    }

    //Little Willie Cam Fix
    {
        uintptr_t ptr_225130 = pattern.get(0, "A0 40 04 3C 00 68 84 44 3E 60 0D 46 00 00 00 00 ? ? ? ? 00 00 00 00 00 60 80 44", 0);
        MakeInlineWrapper(ptr_225130,
            move(a0, s1),
            jal(isLittleWillie),
            nop(),
            lui(a0, 0x40A0)
        );
    }

    // Cars dissapearing from garages fix
    {
        uintptr_t ptr_2D0EF4 = pattern.get(0, "25 20 00 02 00 00 12 34 25 20 20 02", -4);
        injector.MakeNOP(ptr_2D0EF4);
    }

    if (LODDistMultiplier)
    {
        uintptr_t ptr_220130 = pattern.get(0, "00 00 00 00 A8 07 0C C6", -4);
        injector.MakeJAL(ptr_220130, (intptr_t)sub_8A1E990);
        injector.WriteInstr(ptr_220130 + 4, move(a0, s0));

        //ptr_1C5584 = pattern.get(0, "00 00 00 00 ? ? ? ? 00 00 00 00 80 3F 04 3C 00 F0 84 44", -16);
        //ptr_1C547C = pattern.get(0, "2A 20 85 00 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 01 00 14 34", -4);

        uintptr_t ptr_341AC0 = pattern.get(0, "70 42 04 3C 00 60 84 44 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 66 BF 04 3C", -0);
        injector.MakeInlineLUIORI(ptr_341AC0, 60.0f * LODDistMultiplier); //vehicle offscr despawn range
        uintptr_t ptr_1C73A0 = pattern.get(0, "4C 42 04 3C 00 E0 84 44", 0);
        injector.MakeInlineLUIORI(ptr_1C73A0 + 0, 51.0f * LODDistMultiplier);
        injector.MakeInlineLUIORI(ptr_1C73A0 + 8, 25.0f * LODDistMultiplier);
        injector.MakeInlineLUIORI(ptr_1C73A0 + 16, 80.0f * LODDistMultiplier);
    }

    if (PCCheats)
    {
        uintptr_t ptr_18742C = pattern.get(0, "23 20 A4 00 C0 20 04 00", -4);
        pCPad__GetPad = (struct CPad* (*)(int))ptr_18742C;
        uintptr_t ptr_18ABC0 = pattern.get(0, "04 00 B1 AF 00 2E 05 00", -4);
        pCPad__AddToCheatString = (void(*)(struct CPad*, char))ptr_18ABC0;

        SceUID thid = sceKernelCreateThread(MODULE_NAME, module_thread, 0x18, 0x10000, THREAD_ATTR_USER, NULL);
        if (thid >= 0)
            sceKernelStartThread(thid, 0, 0);
    }

    sceKernelDcacheWritebackAll();
    sceKernelIcacheClearAll();

    return 0;
}

int module_start(SceSize args, void* argp) {
    if (sceIoDevctl("kemulator:", 0x00000003, NULL, 0, NULL, 0) == 0) {
        SceUID modules[10];
        int count = 0;
        int result = 0;
        if (sceKernelGetModuleIdList(modules, sizeof(modules), &count) >= 0) {
            int i;
            SceKernelModuleInfo info;
            for (i = 0; i < count; ++i) {
                info.size = sizeof(SceKernelModuleInfo);
                if (sceKernelQueryModuleInfo(modules[i], &info) < 0) {
                    continue;
                }

                if (strcmp(info.name, MODULE_NAME_INTERNAL) == 0)
                {
                    injector.SetGameBaseAddress(info.text_addr, info.text_size);
                    pattern.SetGameBaseAddress(info.text_addr, info.text_size);
                    inireader.SetIniPath(INI_PATH);
                    logger.SetPath(LOG_PATH);
                    result = 1;
                }
                else if (strcmp(info.name, MODULE_NAME) == 0)
                {
                    injector.SetModuleBaseAddress(info.text_addr, info.text_size);
                }
            }

            if (result)
                OnModuleStart();
        }
    }
    return 0;
}