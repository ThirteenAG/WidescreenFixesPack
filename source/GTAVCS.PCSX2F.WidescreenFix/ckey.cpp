#include "../../external/injector/include/ps2/runtime.hpp"
using namespace mips_asm;

extern "C" {
#include "ckey.h"

#include "../../external/injector/include/ps2/log.h"

extern const int16_t BtnMinValue = 0;
extern const int16_t BtnMaxValue = 255;
extern const int16_t AxisMinValue = -128;
extern const int16_t AxisMaxValue = 127;

extern const float fRawMouseSensitivity = 500.0f;
float PCMouseXSens = 2.5f;
float PCMouseYSens = 4.0f;

float TheCamera_m_fMouseAccelHorzntl = 1.0f;
float TheCamera_m_fMouseAccelVertical = 1.0f;

inline int8_t isMouseKeyDown(size_t vKey)
{
    return *(int8_t*)((uintptr_t)(&MouseState[CurrentState]) + vKey);
}

inline int8_t isMouseKeyFirstPressed(size_t vKey)
{
    int8_t previousState = *(int8_t*)((uintptr_t)(&MouseState[PreviousState]) + vKey);
    *(int8_t*)((uintptr_t)(&MouseState[PreviousState]) + vKey) = isMouseKeyDown(vKey);
    return (*(int8_t*)((uintptr_t)(&MouseState[PreviousState] + vKey)) && !previousState);
}

inline int8_t isMouseKeyFirstReleased(size_t vKey)
{
    int8_t previousState = *(int8_t*)((uintptr_t)(&MouseState[PreviousState]) + vKey);
    *(int8_t*)((uintptr_t)(&MouseState[PreviousState]) + vKey) = isMouseKeyDown(vKey);
    return (!*(int8_t*)((uintptr_t)(&MouseState[PreviousState]) + vKey) && previousState);
}

inline int16_t isMouseWheelUp()
{
    if (MouseState[CurrentState].Z > 0.0f)
    {
        //MouseState[CurrentState].Z = 0.0f;
        return AxisMaxValue;
    }
    return 0;
}

inline int16_t isMouseWheelDown()
{
    if (MouseState[CurrentState].Z < 0.0f)
    {
        //MouseState[CurrentState].Z = 0.0f;
        return AxisMinValue;
    }
    return 0;
}

inline char isKeyDown(int vKey)
{
    return KeyboardState[CurrentState][vKey];
}

inline char isKeyFirstPressed(int key)
{
    char prevState = KeyboardState[PreviousState][key];
    KeyboardState[PreviousState][key] = isKeyDown(key);
    return (KeyboardState[PreviousState][key] && !prevState);
}

inline char isKeyFirstReleased(int key)
{
    char prevState = KeyboardState[PreviousState][key];
    KeyboardState[PreviousState][key] = isKeyDown(key);
    return (!KeyboardState[PreviousState][key] && prevState);
}

inline int16_t ClampAxisRange(float flt_value)
{
    int32_t value = (int32_t)flt_value;
    if (value == 0) return 0;
    if (value <= AxisMinValue) return AxisMinValue;
    return (int16_t)(value >= AxisMaxValue ? AxisMaxValue : value);
}

void ReplacePadFuncsWithPCControls()
{
    // First, let's emulate gamepad with keyboard and mouse, assuming it's used only to control GUI
    injector::MakeCALL(0x2843A4, CPad__UpdateHook);

    //Disable Camera Movement with DPAD
    injector::MakeNOP(0x2854F0);

    //Disable Camera Movement when flying
    injector::MakeNOP(0x286704);

    // DPAD movement to WASD
    injector::MakeCALL(0x285764, sub_287470);
    injector::MakeCALL(0x285770, sub_287450);
    injector::MakeCALL(0x2858BC, sub_287430);
    injector::MakeCALL(0x2858C8, sub_287410);

    //Numpad controls when flying
    injector::MakeCALL(0x3ECDC8, CPad__KBGetLookLeftFly);
    injector::MakeCALL(0x3ECEA0, CPad__KBGetLookLeftFly);
    injector::MakeCALL(0x2E71B0, CPad__KBGetLookLeftFly);
    injector::MakeCALL(0x2E71F4, CPad__KBGetLookLeftFly);
    injector::MakeCALL(0x3ECD8C, CPad__KBGetLookRightFly);
    injector::MakeCALL(0x3ECF08, CPad__KBGetLookRightFly);
    injector::MakeCALL(0x2E7170, CPad__KBGetLookRightFly);
    injector::MakeCALL(0x2E7234, CPad__KBGetLookRightFly);

    //DPAD GUI?
    //injector::MakeJMP(0x287580, sub_287580).as_int(); // right
    //injector::MakeJMP(0x287530, sub_287530).as_int(); // left
    //injector::MakeJMP(0x2874E0, sub_2874E0).as_int(); // down
    //injector::MakeJMP(0x287490, sub_287490).as_int(); // up
    //injector::MakeJMP(0x286D28, sub_286D28).as_int(); // enter
    //injector::MakeCALL(0x3B585C, sub_286D78).as_int(); //injector::MakeJMP(0x286D78, sub_286D78).as_int(); // back

    // GUI
    //injector::MakeJMP(0x286C08, CPad__KBGuiLeft).as_int();
    //injector::MakeJMP(0x286C48, CPad__KBGuiRight).as_int();
    //injector::MakeJMP(0x286C88, CPad__KBGuiUp).as_int();
    //injector::MakeJMP(0x286CC8, CPad__KBGuiDown).as_int();
    //injector::MakeJMP(0x286D28, CPad__KBGuiSelect).as_int();
    //injector::MakeJMP(0x286D78, CPad__KBGuiBack).as_int();

    // CPad::SniperMode
    //injector::MakeCALL(0x286938, CPad__KBGetRightStickX).as_int();
    //injector::MakeCALL(0x286A40, CPad__KBGetRightStickY).as_int();
    //injector::MakeCALL(0x286A6C, CPad__KBGetRightStickY).as_int();

    injector::MakeJMP(0x2852C0, CPad__KBGetSteeringLeftRight);
    injector::MakeJMP(0x285460, CPad__KBGetSteeringUpDown);
    injector::MakeJMP(0x285518, CPad__KBGetCarGunUpDown);
    injector::MakeJMP(0x285600, CPad__KBGetCarGunLeftRight);
    injector::MakeJMP(0x285690, CPad__KBGetPedWalkLeftRight);
    injector::MakeJMP(0x2857E8, CPad__KBGetPedWalkUpDown);
    injector::MakeJMP(0x285940, CPad__KBGetLookLeft);
    injector::MakeJMP(0x285AB0, CPad__KBGetLookRight);
    injector::MakeJMP(0x285C20, CPad__KBGetLookBehindForCar);
    injector::MakeJMP(0x285DD0, CPad__KBGetLookBehindForPed);
    injector::MakeJMP(0x285E08, CPad__KBGetHorn);
    injector::MakeJMP(0x285E80, CPad__KBHornJustDown);
    injector::MakeJMP(0x285F20, CPad__KBGetCarGunFired);
    injector::MakeJMP(0x285F40, CPad__KBCarGunJustDown);
    injector::MakeJMP(0x285F70, CPad__KBCarGunJustUp);
    injector::MakeJMP(0x285FA0, CPad__KBGetHandBrake);
    injector::MakeJMP(0x286030, CPad__KBGetBrake);
    injector::MakeJMP(0x286050, CPad__KBJumpJustDown);
    injector::MakeJMP(0x2860B0, CPad__KBGetExitVehicle);
    injector::MakeJMP(0x286100, CPad__KBExitVehicleJustDown);
    injector::MakeJMP(0x286148, CPad__KBGetWeapon);
    injector::MakeJMP(0x286180, CPad__KBWeaponJustDown);
    injector::MakeJMP(0x2861D0, CPad__KBGetAccelerate);
    injector::MakeJMP(0x286220, CPad__KBCycleCameraModeUpJustDown);
    injector::MakeJMP(0x286328, CPad__KBChangeStationUpJustDown);
    injector::MakeJMP(0x2863E0, CPad__KBChangeStationDownJustDown);
    injector::MakeJMP(0x286408, CPad__KBCycleWeaponLeftJustDown);
    injector::MakeJMP(0x286458, CPad__KBCycleWeaponRightJustDown);
    injector::MakeJMP(0x2864A8, CPad__KBGetTarget);
    injector::MakeJMP(0x286500, CPad__KBTargetJustDown);
    injector::MakeJMP(0x286578, CPad__KBDuckJustDown);
    injector::MakeJMP(0x286580, CPad__KBGetSprint);
    injector::MakeJMP(0x286630, CPad__KBForceCameraBehindPlayer);
    injector::MakeJMP(0x286768, CPad__KBSniperZoomIn);
    injector::MakeJMP(0x2867A8, CPad__KBSniperZoomOut);
    injector::MakeJMP(0x286808, CPad__KBShiftTargetLeftJustDown);
    injector::MakeJMP(0x286850, CPad__KBShiftTargetRightJustDown);
    //injector::MakeJMP(0x286928, CPad__KBSniperModeLookLeftRight).as_int();
    //injector::MakeJMP(0x286A20, CPad__KBSniperModeLookUpDown).as_int();
    //injector::MakeJMP(0x286AF0, CPad__KBLookAroundLeftRight).as_int();
    //injector::MakeJMP(0x286B20, CPad__KBLookAroundUpDown).as_int();
    injector::MakeJMP(0x286DC8, CPad__KBGetSkipCutscene);
    injector::MakeJMP(0x286DE8, CPad__KBGetOddJobTrigger);
    injector::MakeJMP(0x286E30, CPad__KBEnterFreeAim);
    //injector::MakeJMP(0x287640, CPad__KBGetLeftStickX).as_int();
    //injector::MakeJMP(0x287678, CPad__KBGetLeftStickY).as_int();

    //injector::MakeJMP(0x286AF0, CPad__KBLookAroundLeftRightF).as_int();
    //injector::MakeJMP(0x286B20, CPad__KBLookAroundUpDownF).as_int();


    //Cam::Process_1stPerson(UNTESTED!!!)
    {
        injector::MakeNOP(0x2955D8, 64);
        injector::MakeInline(0x2955D8,
            move(at, v0),
            jal((uintptr_t)CPad__KBLookAroundLeftRightNoTimeStamp),
            move(a0, at),
            movs(f20, f0),
            jal((uintptr_t)CPad__KBLookAroundUpDownNoTimeStamp),
            move(a0, at),
            movs(f5, f0),
            lui(at, 0x3F80),
            mtc1(at, f9)
        );
        injector::WriteInstr(0x295614, clts(f20, f21)); //f21 = 0

        injector::MakeNOP(0x2956A8); // CTimer::GetTimeStep()
        injector::MakeNOP(0x2a0f0c); // CTimer::GetTimeStep()

        // Negative Mouse Acceleration
        //?
    }
    // CCam::Process_FollowPed
    {
        injector::MakeNOP(0x29EC9C, 44);
        injector::MakeInline(0x29EC9C,
            jal((uintptr_t)CPad__KBLookAroundLeftRightNoTimeStamp),
            lw(a0, sp, 0xBC),
            movs(f20, f0),
            jal((uintptr_t)CPad__KBLookAroundUpDownNoTimeStamp),
            lw(a0, sp, 0xBC),
            movs(f21, f0)
        );
        injector::MakeNOP(0x29ED4C); // CTimer::GetTimeStep()
        injector::MakeNOP(0x29ED54); // CTimer::GetTimeStep()

        // Negative Mouse Acceleration
        injector::MakeNOP(0x29ED78);
        injector::MakeNOP(0x29ED8C);
        injector::MakeNOP(0x29EE6C);
        injector::MakeNOP(0x29EE58);

        // Stick Sensitivity
        //injector::MakeInlineLUIORI(0x29ECE4, 0.000049000002f * 10.0f);
        //injector::MakeInlineLUIORI(0x29ECE4, 0.000049000002f * 10.0f);

        // stick check, is it correct to nop?
        injector::MakeNOP(0x29DC18);
        //injector::MakeNOP(0x29DBEC+4);
        //injector::MakeInline(0x29DBEC,
        //    jal(EmulateRightStickXWithMouse),
        //    nop(),
        //    move(v1, v0),
        //    jal(EmulateRightStickYWithMouse),
        //    nop()
        //    //move(v0, v0)
        //);
    }
    //CCam::Process_FollowCar_SA
    {
        injector::MakeNOP(0x2A0E94, 44);
        injector::MakeInline(0x2A0E94,
            jal((uintptr_t)CPad__KBLookAroundLeftRightNoTimeStamp),
            lw(a0, sp, 0xCC),
            movs(f20, f0),
            jal((uintptr_t)CPad__KBLookAroundUpDownNoTimeStamp),
            lw(a0, sp, 0xCC),
            movs(f21, f0)
        );

        injector::MakeNOP(0x2A0F04); // CTimer::GetTimeStep()
        injector::MakeNOP(0x2956A4); // CTimer::GetTimeStep()

        // Negative Mouse Acceleration
        injector::MakeNOP(0x2A113C);
        injector::MakeNOP(0x2A1128);
        injector::MakeNOP(0x2A12F8);
        injector::MakeNOP(0x2A1308);
    }
    //CCam::Process_M16_1stPerson
    {
        injector::MakeNOP(0x29804C, 52);
        injector::MakeInline(0x29804C,
            move(at, v0),
            jal((uintptr_t)CPad__KBLookAroundLeftRightNoTimeStamp),
            move(a0, at),
            movs(f21, f0),
            jal((uintptr_t)CPad__KBLookAroundUpDownNoTimeStamp),
            move(a0, at),
            movs(f8, f0)
        );

        //HELI Turret Mode (e.g. Farewell To Arms)
        float f80 = 80.0f;
        {
            injector::MakeNOP(0x2980B4, 32);
            injector::MakeInline(0x2980B4, //f2 = FOV
                muls(f20, f21, f2),
                muls(f22, f8, f2),
                lui(at, injector::HighWord(f80)),
                addiu(at, at, injector::LowWord(f80)),
                mtc1(at, f30),
                divs(f20, f20, f30),
                divs(f22, f22, f30)
            );
            injector::MakeNOP(0x2980D4);
            injector::MakeNOP(0x2980DC);
        }

        injector::MakeNOP(0x298164, 40);
        injector::MakeInline(0x298164, //f4 = FOV
            muls(f20, f21, f4),
            muls(f22, f8, f4),
            lui(at, injector::HighWord(f80)),
            addiu(at, at, injector::LowWord(f80)),
            mtc1(at, f30),
            divs(f20, f20, f30),
            divs(f22, f22, f30)
        );
    }

    // Sniper Zoom with Wheel
    {
        float FOVDif = 10.0f;
        //SniperZoomIn
        injector::MakeNOP(0x297F3C, 4 * 4);
        injector::MakeInline(0x297F40,
            lui(v0, injector::HighWord(FOVDif)),
            addiu(v0, v0, injector::LowWord(FOVDif)),
            mtc1(v0, f30),
            subs(f1, f1, f30)
        );

        //SniperZoomOut
        injector::MakeNOP(0x297ED4, 4 * 4);
        injector::MakeInline(0x297ED4,
            lui(v0, injector::HighWord(FOVDif)),
            addiu(v0, v0, injector::LowWord(FOVDif)),
            mtc1(v0, f30),
            adds(f0, f0, f30)
        );
    }

    //CCam::Process_AimWeapon
    {
        injector::MakeNOP(0x29CD04, 56);
        injector::MakeInline(0x29CD04,
            move(at, v0),
            jal((uintptr_t)CPad__KBLookAroundLeftRightNoTimeStamp),
            move(a0, at),
            movs(f20, f0),
            jal((uintptr_t)CPad__KBLookAroundUpDownNoTimeStamp),
            move(a0, at)
            //movs(f0, f0)
        );

        // Remove clamp
        injector::MakeNOP(0x29CC10);
        injector::MakeNOP(0x29CCC0);
    }

    //CPlayerPed::PlayerControlFreeAim
    {
        injector::MakeNOP(0x23442C, 56);
        injector::MakeInline(0x23442C,
            swc1(f20, sp, 0x260),
            lui(at, 0x4248),
            mtc1(at, f23),
            lui(at, 0x42DC),
            mtc1(at, f24),
            jal((uintptr_t)CPad__KBLookAroundLeftRightNoTimeStamp),
            move(a0, s0),
            movs(f22, f0),
            jal((uintptr_t)CPad__KBLookAroundUpDownNoTimeStamp),
            move(a0, s0),
            movs(f25, f0)
        );

        injector::MakeInlineLUIORI(0x234f74, -1.0f);
        injector::MakeInlineLUIORI(0x234BE4, 1.0f);
    }

    //CAutomobile::TankControl
    {
        //injector::MakeNOP(0x150D9C, 20);
        //injector::MakeInline(0x150D9C,
        //    jal(CPad__KBGetCarGunLeftRight),
        //    li(a1, 1)
        //);
        //
        //injector::MakeInlineLUIORI(0x150DB0, -1.0f);
        //injector::MakeNOP(0x150DC4, 8);
        //injector::WriteInstr(0x150DC4, subs(f2, f2, f0));

        injector::WriteMemory<uint16_t>(0x2A1098, 0xFFFF);
    }
}

extern const char cheat_inputs[CheatsNum][9] =
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

void HandlePCCheats()
{
#define _CHEATCMP(str) strncmp(str, CheatString, sizeof(str)-1)
    int32_t cheat_id = -1;

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

    if (cheat_id >= 0)
    {
        struct CPad* (*pCPad__GetPad)(int padNumber) = reinterpret_cast<decltype(pCPad__GetPad)>(0x2852A8);
        void(*pCPad__AddToCheatString)(struct CPad* pad, char a2) = reinterpret_cast<decltype(pCPad__AddToCheatString)>(0x283C60);
        struct CPad* pad = pCPad__GetPad(0);
        for (size_t j = 0; j < 8; j++)
            pCPad__AddToCheatString(pad, cheat_inputs[cheat_id][j]);
        CheatString[0] = 0;
    }

    //if (!_CHEATCMP("SAVESAVESAVE"))
    //{
    //    //void(*SaveSlot)(int unk, int slot) = reinterpret_cast<decltype(SaveSlot)>(0x2778A8);
    //    //SaveSlot(0x510480, 6);
    //
    //    *(char*)0x4818AC = 6;
    //    void(*SaveSlot)() = reinterpret_cast<decltype(SaveSlot)>(0x108128);
    //    SaveSlot();
    //    *(char*)0x4818AC = 0;
    //    CheatString[0] = 0;
    //}
}

void CPad__UpdateHook(struct CPad* pad, int16_t a2)
{
    int16_t(*CPad__Update)(struct CPad* pad, int16_t a2) = reinterpret_cast<decltype(CPad__Update)>(0x2843D8);
    CPad__Update(pad, a2);

    //pad->OldState.LEFTSTICKX = pad->NewState.LEFTSTICKX;
    //pad->OldState.LEFTSTICKY = pad->NewState.LEFTSTICKY;
    ////pad->OldState.RIGHTSTICKX    = pad->NewState.RIGHTSTICKX;
    ////pad->OldState.RIGHTSTICKY    = pad->NewState.RIGHTSTICKY;
    //pad->OldState.LEFTSHOULDER1 = pad->NewState.LEFTSHOULDER1;
    //pad->OldState.LEFTSHOULDER2 = pad->NewState.LEFTSHOULDER2;
    //pad->OldState.RIGHTSHOULDER1 = pad->NewState.RIGHTSHOULDER1;
    //pad->OldState.RIGHTSHOULDER2 = pad->NewState.RIGHTSHOULDER2;
    //pad->OldState.DPADUP = pad->NewState.DPADUP;
    //pad->OldState.DPADDOWN = pad->NewState.DPADDOWN;
    //pad->OldState.DPADLEFT = pad->NewState.DPADLEFT;
    //pad->OldState.DPADRIGHT = pad->NewState.DPADRIGHT;
    //pad->OldState.START = pad->NewState.START;
    //pad->OldState.SELECT = pad->NewState.SELECT;
    //pad->OldState.SQUARE = pad->NewState.SQUARE;
    //pad->OldState.TRIANGLE = pad->NewState.TRIANGLE;
    //pad->OldState.CROSS = pad->NewState.CROSS;
    //pad->OldState.CIRCLE = pad->NewState.CIRCLE;
    //pad->OldState.LEFTSHOCK = pad->NewState.LEFTSHOCK;
    //pad->OldState.RIGHTSHOCK = pad->NewState.RIGHTSHOCK;

    char* curKB = KeyboardState[CurrentState];
    pad->NewState.LEFTSTICKX = curKB[VK_KEY_A] ? AxisMinValue : (curKB[VK_KEY_D] ? AxisMaxValue : 0);
    pad->NewState.LEFTSTICKY = curKB[VK_KEY_W] ? AxisMinValue : (curKB[VK_KEY_S] ? AxisMaxValue : 0);
    //pad->NewState.RIGHTSTICKX = EmulateRightStickXWithMouse(pad); //ClampAxisRange(MouseState[CurrentState].X);
    //pad->NewState.RIGHTSTICKY = EmulateRightStickYWithMouse(pad); //ClampAxisRange(MouseState[CurrentState].Y);
    pad->NewState.LEFTSHOULDER1 = curKB[VK_TAB];
    pad->NewState.LEFTSHOULDER2 = curKB[VK_KEY_1];
    pad->NewState.RIGHTSHOULDER1 = curKB[VK_KEY_2];
    pad->NewState.RIGHTSHOULDER2 = MouseState[CurrentState].rmb;
    pad->NewState.DPADUP = curKB[VK_UP] ? BtnMaxValue : (curKB[VK_KEY_W]);
    pad->NewState.DPADDOWN = curKB[VK_DOWN] ? BtnMaxValue : (curKB[VK_KEY_S]);
    pad->NewState.DPADLEFT = curKB[VK_LEFT] ? BtnMaxValue : (curKB[VK_KEY_A]);
    pad->NewState.DPADRIGHT = curKB[VK_RIGHT] ? BtnMaxValue : (curKB[VK_KEY_D]);
    pad->NewState.START = curKB[VK_OEM_3];
    pad->NewState.SELECT = curKB[VK_KEY_V];
    pad->NewState.SQUARE = curKB[VK_SPACE];
    pad->NewState.TRIANGLE = curKB[VK_BACK];
    pad->NewState.CROSS = curKB[VK_RETURN];
    pad->NewState.CIRCLE = MouseState[CurrentState].lmb;
    pad->NewState.LEFTSHOCK = curKB[VK_KEY_H];
    pad->NewState.RIGHTSHOCK = curKB[VK_ADD];

    if (pad->NewState.RIGHTSTICKY
        || pad->NewState.LEFTSTICKX
        || pad->NewState.LEFTSTICKY
        || pad->NewState.DPADUP
        || pad->NewState.DPADDOWN
        || pad->NewState.DPADRIGHT
        || pad->NewState.TRIANGLE
        || pad->NewState.CIRCLE
        || pad->NewState.SQUARE
        || pad->NewState.START
        || pad->NewState.SELECT
        || pad->NewState.LEFTSHOULDER1
        || pad->NewState.LEFTSHOULDER2
        || pad->NewState.RIGHTSHOULDER2
        || pad->NewState.LEFTSHOCK
        || pad->NewState.RIGHTSHOCK)
    {
        int* CTimer__m_snTimeInMilliseconds = (int*)0x4CD104;
        *(int*)&pad[1].NewState.unk2 = *CTimer__m_snTimeInMilliseconds; //InputHowLongAgo
    }
}

int16_t EmulateRightStickXWithMouse(struct CPad* pad)
{
    float x = ClampAxisRange(MouseState[CurrentState].X) * 200.0f;
    //x = x / 0.000049000002f;
    //x = x / 0.00089285721f;
    MouseState[CurrentState].X = 0.0f;
    return x;
}

int16_t EmulateRightStickYWithMouse(struct CPad* pad)
{
    float y = ClampAxisRange(MouseState[CurrentState].Y) * 200.0f;
    //y = y / 0.000049000002f;
    //y = y / 0.00089285721f;
    MouseState[CurrentState].Y = 0.0f;
    return y;
}

int16_t sub_287470(struct CPad* pad)
{
    if (isKeyDown(VK_KEY_D) || isKeyDown(VK_RIGHT))
        return BtnMaxValue;
    return 0;
}

int16_t sub_287450(struct CPad* pad)
{
    if (isKeyDown(VK_KEY_A) || isKeyDown(VK_LEFT))
        return BtnMaxValue;
    return 0;
}

int16_t sub_287430(struct CPad* pad)
{
    if (isKeyDown(VK_KEY_S) || isKeyDown(VK_DOWN))
        return BtnMaxValue;
    return 0;
}

int16_t sub_287410(struct CPad* pad)
{
    if (isKeyDown(VK_KEY_W) || isKeyDown(VK_UP))
        return BtnMaxValue;
    return 0;
}

int16_t sub_287580(struct CPad* pad)
{
    if (KeyboardState[CurrentState][VK_RIGHT])
        return KeyboardState[PreviousState][VK_RIGHT] == 0;
    if (KeyboardState[CurrentState][VK_KEY_D])
        return KeyboardState[PreviousState][VK_KEY_D] == 0;
    return 0;
}
int16_t sub_287530(struct CPad* pad)
{
    if (KeyboardState[CurrentState][VK_LEFT])
        return KeyboardState[PreviousState][VK_LEFT] == 0;
    if (KeyboardState[CurrentState][VK_KEY_A])
        return KeyboardState[PreviousState][VK_KEY_A] == 0;
    return 0;
}
int16_t sub_2874E0(struct CPad* pad)
{
    if (KeyboardState[CurrentState][VK_DOWN])
        return KeyboardState[PreviousState][VK_DOWN] == 0;
    if (KeyboardState[CurrentState][VK_KEY_S])
        return KeyboardState[PreviousState][VK_KEY_S] == 0;
    return 0;
}
int16_t sub_287490(struct CPad* pad)
{
    if (KeyboardState[CurrentState][VK_UP])
        return KeyboardState[PreviousState][VK_UP] == 0;
    if (KeyboardState[CurrentState][VK_KEY_W])
        return KeyboardState[PreviousState][VK_KEY_W] == 0;
    return 0;
}
int16_t sub_286D28(struct CPad* pad)
{
    if (KeyboardState[CurrentState][VK_RETURN])
        return KeyboardState[PreviousState][VK_RETURN] == 0;
    return 0;
}
int16_t sub_286D78(struct CPad* pad)
{
    //if (isKeyFirstReleased(VK_BACK))
    //    return 1;
    //if (KeyboardState[CurrentState][VK_BACK])
    //{
    //    int ret = KeyboardState[PreviousState][VK_BACK] == 0;
    //    KeyboardState[PreviousState][VK_BACK] = 1;
    //    return ret;
    //}
    //if (KeyboardState[CurrentState][VK_BACK] == 0)
    //    return KeyboardState[PreviousState][VK_BACK] != 0;
    //if (isKeyFirstReleased(VK_BACK))
    //    return 1;
    return 0;
}

int16_t CPad__KBGetRightStickX(struct CPad* pad)
{
    //float x = -(MouseState[CurrentState].X);
    //MouseState[CurrentState].X = 0.0f;
    //return x;
    //return (int32_t)-(MouseState[CurrentState].X);
    return (int16_t)CPad__KBLookAroundLeftRightF(pad) * fRawMouseSensitivity;
}

int16_t CPad__KBGetRightStickY(struct CPad* pad)
{
    //float y = -(MouseState[CurrentState].Y);
    //MouseState[CurrentState].Y = 0.0f;
    //return y;
    //return (int32_t)-(MouseState[CurrentState].Y);
    return (int16_t)CPad__KBLookAroundUpDownF(pad);
}

inline float CPad__KBLookAroundLeftRightF(struct CPad* pad)
{
    float x = -(MouseState[CurrentState].X) * PCMouseXSens;
    //x = x / 0.000049000002f;
    //x = x / 0.00089285721f;
    MouseState[CurrentState].X = 0.0f;
    return x;
}

float CPad__KBLookAroundLeftRightNoTimeStamp(struct CPad* pad)
{
    return CPad__KBLookAroundLeftRightF(pad) / fRawMouseSensitivity;
}

inline float CPad__KBLookAroundUpDownF(struct CPad* pad)
{
    float y = -(MouseState[CurrentState].Y) * PCMouseYSens;
    //y = y / 0.000049000002f;
    //y = y / 0.00089285721f;
    MouseState[CurrentState].Y = 0.0f;
    return y;
}

float CPad__KBLookAroundUpDownNoTimeStamp(struct CPad* pad)
{
    return CPad__KBLookAroundUpDownF(pad) / fRawMouseSensitivity;
}

int16_t CPad__KBGetSteeringLeftRight(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
    {
        if (isKeyDown(VK_KEY_A))
            return AxisMinValue;
        else if (isKeyDown(VK_KEY_D))
            return AxisMaxValue;
        return 0;
    }
}

int16_t CPad__KBGetSteeringUpDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
    {
        if (isKeyDown(VK_NUMPAD8))
            return AxisMinValue;
        else if (isKeyDown(VK_NUMPAD5))
            return AxisMaxValue;
        if (isKeyDown(VK_LSHIFT))
            return AxisMinValue;
        else if (isKeyDown(VK_LCONTROL))
            return AxisMaxValue;
        return 0;
    }
}

int16_t CPad__KBGetCarGunUpDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
    {
        if (isKeyDown(VK_NUMPAD8))
            return AxisMinValue;
        else if (isKeyDown(VK_NUMPAD5))
            return AxisMaxValue;
        return 0;
    }
}

int16_t CPad__KBGetCarGunLeftRight(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
    {
        if (isKeyDown(VK_NUMPAD4))
            return AxisMinValue;
        else if (isKeyDown(VK_NUMPAD6))
            return AxisMaxValue;
        return 0;
    }
}

int16_t CPad__KBGetPedWalkLeftRight(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
    {
        if (isKeyDown(VK_KEY_A))
            return isKeyDown(VK_LMENU) ? -1 : AxisMinValue;
        else if (isKeyDown(VK_KEY_D))
            return isKeyDown(VK_LMENU) ? 1 : AxisMaxValue;
        return 0;
    }
}

int16_t CPad__KBGetPedWalkUpDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
    {
        if (isKeyDown(VK_KEY_W))
            return isKeyDown(VK_LMENU) ? -1 : AxisMinValue;
        else if (isKeyDown(VK_KEY_S))
            return isKeyDown(VK_LMENU) ? 1 : AxisMaxValue;
        return 0;
    }
}

int16_t CPad__KBGetLookLeft(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_KEY_Q) && !isKeyDown(VK_KEY_E))
        return 1;
    return 0;
}

int16_t CPad__KBGetLookRight(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_KEY_E) && !isKeyDown(VK_KEY_Q))
        return 1;
    return 0;
}

int16_t CPad__KBGetLookLeftFly(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_KEY_Q) && !isKeyDown(VK_KEY_E))
        return 1;
    if (isKeyDown(VK_NUMPAD4) && !isKeyDown(VK_NUMPAD6))
        return 1;
    return 0;
}

int16_t CPad__KBGetLookRightFly(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_KEY_E) && !isKeyDown(VK_KEY_Q))
        return 1;
    if (isKeyDown(VK_NUMPAD6) && !isKeyDown(VK_NUMPAD4))
        return 1;
    return 0;
}

int16_t CPad__KBGetLookBehindForCar(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if ((isKeyDown(VK_KEY_Q) && isKeyDown(VK_KEY_E)) || isKeyDown(VK_KEY_C) || isMouseKeyDown(offsetof(struct CMouseControllerState, mmb)))
        return 1;
    return 0;
}

int16_t CPad__KBGetLookBehindForPed(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_KEY_C) || isMouseKeyDown(offsetof(struct CMouseControllerState, mmb)))
        return 1;
    return 0;
}

int16_t CPad__KBGetHorn(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_KEY_H))
        return 1;
    return 0;
}

int16_t CPad__KBHornJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyFirstPressed(VK_KEY_H))
        return 1;
    return 0;
}

int16_t CPad__KBGetCarGunFired(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_NUMPAD0) || isKeyDown(VK_LMENU) || isMouseKeyDown(offsetof(struct CMouseControllerState, lmb)))
        return 1;
    return 0;
}

int16_t CPad__KBCarGunJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyFirstPressed(VK_NUMPAD0) || isMouseKeyFirstPressed(offsetof(struct CMouseControllerState, lmb)))
        return 1;
    return 0;
}

int16_t CPad__KBCarGunJustUp(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isMouseKeyFirstReleased(VK_NUMPAD0) || isMouseKeyFirstReleased(offsetof(struct CMouseControllerState, lmb)))
        return 1;
    return 0;
}

int16_t CPad__KBGetHandBrake(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_SPACE))
        return 1;
    return 0;
}

int16_t CPad__KBGetBrake(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_KEY_S))
        return BtnMaxValue;
    return 0;
}

int16_t CPad__KBJumpJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyFirstPressed(VK_SPACE))
        return 1;
    return 0;
}

int16_t CPad__KBGetExitVehicle(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_KEY_F))
        return 1;
    return 0;
}

int16_t CPad__KBExitVehicleJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyFirstPressed(VK_KEY_F))
        return 1;
    return 0;
}

int16_t CPad__KBGetWeapon(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isMouseKeyDown(offsetof(struct CMouseControllerState, lmb)))
        return 1;
    if (pad->NewState.CIRCLE)
        return 1;
    return 0;
}

int16_t CPad__KBWeaponJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isMouseKeyFirstPressed(offsetof(struct CMouseControllerState, lmb)))
        return 1;
    if (pad->NewState.CIRCLE)
        return pad->OldState.CIRCLE == 0;
    return 0;
}

int16_t CPad__KBGetAccelerate(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_KEY_W))
        return BtnMaxValue;
    return 0;
}

int16_t CPad__KBCycleCameraModeUpJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyFirstPressed(VK_KEY_V))
        return 1;
    return 0;
}

int16_t lastStationState = 0;
int16_t CPad__KBChangeStationUpJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (lastStationState == 1)
    {
        lastStationState = 0;
        return 1;
    }
    if (isMouseWheelUp())
    {
        lastStationState = 1;
        MouseState[CurrentState].Z = 0.0f;
        return 1;
    }
    return 0;
}

int16_t CPad__KBChangeStationDownJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (lastStationState == 2)
        lastStationState = 0;
    if (isMouseWheelDown())
    {
        lastStationState = 2;
        MouseState[CurrentState].Z = 0.0f;
        return 1;
    }
    return 0;
}

int16_t CPad__KBCycleWeaponLeftJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isMouseWheelUp())
    {
        if (!CPad__KBGetTarget(pad))
            MouseState[CurrentState].Z = 0.0f;
        return 1;
    }
    return 0;
}

int16_t CPad__KBCycleWeaponRightJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isMouseWheelDown())
    {
        if (!CPad__KBGetTarget(pad))
            MouseState[CurrentState].Z = 0.0f;
        return 1;
    }
    return 0;
}

int16_t CPad__KBGetTarget(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isMouseKeyDown(offsetof(struct CMouseControllerState, rmb)))
        return 1;
    return 0;
}

int16_t CPad__KBTargetJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isMouseKeyDown(offsetof(struct CMouseControllerState, rmb)))
        return 1;
    return 0;
}

int16_t CPad__KBDuckJustDown(struct CPad* pad)
{
    //if (pad->DisablePlayerControls)
    //    return 0;
    //if (isKeyFirstPressed(VK_LCONTROL))
    //    return BtnMaxValue;
    return 0;
}

int16_t CPad__KBGetSprint(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_LSHIFT) || isKeyDown(VK_RSHIFT))
        return 1;
    return 0;
}

int16_t CPad__KBForceCameraBehindPlayer(struct CPad* pad)
{
    return 0;
}

int16_t CPad__KBSniperZoomIn(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isMouseWheelUp())
    {
        if (CPad__KBGetTarget(pad))
            MouseState[CurrentState].Z = 0.0f;
        return BtnMaxValue;
    }
    return 0;
}

int16_t CPad__KBSniperZoomOut(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isMouseWheelDown())
    {
        if (CPad__KBGetTarget(pad))
            MouseState[CurrentState].Z = 0.0f;
        return BtnMaxValue;
    }
    return 0;
}

int16_t CPad__KBShiftTargetLeftJustDown(struct CPad* pad)
{
    return 0;
}

int16_t CPad__KBShiftTargetRightJustDown(struct CPad* pad)
{
    return 0;
}

int16_t CPad__KBSniperModeLookLeftRight(struct CPad* pad)
{
    return 0;
}

int16_t CPad__KBSniperModeLookUpDown(struct CPad* pad)
{
    return 0;
}

int16_t CPad__KBLookAroundLeftRight(struct CPad* pad)
{
    return 0;
}

int16_t CPad__KBLookAroundUpDown(struct CPad* pad)
{
    return 0;
}

int16_t CPad__KBGuiLeft(struct CPad* pad)
{
    if (isKeyFirstPressed(VK_LEFT) || isKeyFirstPressed(VK_KEY_A))
        return 1;
    return 0;
}

int16_t CPad__KBGuiRight(struct CPad* pad)
{
    if (isKeyFirstPressed(VK_RIGHT) || isKeyFirstPressed(VK_KEY_D))
        return 1;
    return 0;
}

int16_t CPad__KBGuiUp(struct CPad* pad)
{
    if (isKeyFirstPressed(VK_UP) || isKeyFirstPressed(VK_KEY_W))
        return 1;
    return 0;
}

int16_t CPad__KBGuiDown(struct CPad* pad)
{
    if (isKeyFirstPressed(VK_DOWN) || isKeyFirstPressed(VK_KEY_S))
        return 1;
    return 0;
}

int16_t CPad__KBGuiSelect(struct CPad* pad)
{
    if (isKeyFirstPressed(VK_RETURN))
        return 1;
    return 0;
}

int16_t CPad__KBGuiBack(struct CPad* pad)
{
    if (isKeyFirstPressed(VK_BACK))
        return 1;
    return 0;
}

int16_t CPad__KBGetSkipCutscene(struct CPad* pad)
{
    if (isKeyFirstPressed(VK_RETURN) || isKeyFirstPressed(VK_SPACE))
        return 1;
    return 0;
}

int16_t CPad__KBGetOddJobTrigger(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (isKeyDown(VK_ADD) || isKeyDown(VK_KEY_2))
        return 1;
    return 0;
}

int16_t CPad__KBEnterFreeAim(struct CPad* pad)
{
    return 1;
}

} // extern "C"
