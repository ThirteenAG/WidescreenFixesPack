#ifndef H_KEY
#define H_KEY
#include <stdint.h>
#include "cpad.h"

void ReplacePadFuncsWithPCControls();
void HandlePCCheats();
char GetAsyncKeyState(int vKey);

void CPad__UpdateHook(struct CPad* pad, int16_t a2);

int16_t EmulateRightStickXWithMouse(struct CPad* pad);
int16_t EmulateRightStickYWithMouse(struct CPad* pad);

int16_t sub_287470(struct CPad* pad);
int16_t sub_287450(struct CPad* pad);
int16_t sub_287430(struct CPad* pad);
int16_t sub_287410(struct CPad* pad);

int16_t sub_287580(struct CPad* pad);
int16_t sub_287530(struct CPad* pad);
int16_t sub_2874E0(struct CPad* pad);
int16_t sub_287490(struct CPad* pad);
int16_t sub_286D28(struct CPad* pad);
int16_t sub_286D78(struct CPad* pad);

float CPad__KBLookAroundLeftRightF(struct CPad* pad);
float CPad__KBLookAroundUpDownF(struct CPad* pad);
float CPad__KBLookAroundLeftRightNoTimeStamp(struct CPad* pad);
float CPad__KBLookAroundUpDownNoTimeStamp(struct CPad* pad);
int16_t CPad__KBGetSteeringLeftRight(struct CPad* pad);
int16_t CPad__KBGetSteeringUpDown(struct CPad* pad);
int16_t CPad__KBGetCarGunUpDown(struct CPad* pad);
int16_t CPad__KBGetCarGunLeftRight(struct CPad* pad);
int16_t CPad__KBGetPedWalkLeftRight(struct CPad* pad);
int16_t CPad__KBGetPedWalkUpDown(struct CPad* pad);
int16_t CPad__KBGetLookLeft(struct CPad* pad);
int16_t CPad__KBGetLookLeftFly(struct CPad* pad);
int16_t CPad__KBGetLookRight(struct CPad* pad);
int16_t CPad__KBGetLookRightFly(struct CPad* pad);
int16_t CPad__KBGetLookBehindForCar(struct CPad* pad);
int16_t CPad__KBGetLookBehindForPed(struct CPad* pad);
int16_t CPad__KBGetHorn(struct CPad* pad);
int16_t CPad__KBHornJustDown(struct CPad* pad);
int16_t CPad__KBGetCarGunFired(struct CPad* pad);
int16_t CPad__KBCarGunJustDown(struct CPad* pad);
int16_t CPad__KBCarGunJustUp(struct CPad* pad);
int16_t CPad__KBGetHandBrake(struct CPad* pad);
int16_t CPad__KBGetBrake(struct CPad* pad);
int16_t CPad__KBJumpJustDown(struct CPad* pad);
int16_t CPad__KBGetExitVehicle(struct CPad* pad);
int16_t CPad__KBExitVehicleJustDown(struct CPad* pad);
int16_t CPad__KBGetWeapon(struct CPad* pad);
int16_t CPad__KBWeaponJustDown(struct CPad* pad);
int16_t CPad__KBGetAccelerate(struct CPad* pad);
int16_t CPad__KBCycleCameraModeUpJustDown(struct CPad* pad);
int16_t CPad__KBChangeStationUpJustDown(struct CPad* pad);
int16_t CPad__KBChangeStationDownJustDown(struct CPad* pad);
int16_t CPad__KBCycleWeaponLeftJustDown(struct CPad* pad);
int16_t CPad__KBCycleWeaponRightJustDown(struct CPad* pad);
int16_t CPad__KBGetTarget(struct CPad* pad);
int16_t CPad__KBTargetJustDown(struct CPad* pad);
int16_t CPad__KBDuckJustDown(struct CPad* pad);
int16_t CPad__KBGetSprint(struct CPad* pad);
int16_t CPad__KBForceCameraBehindPlayer(struct CPad* pad);
int16_t CPad__KBSniperZoomIn(struct CPad* pad);
int16_t CPad__KBSniperZoomOut(struct CPad* pad);
int16_t CPad__KBShiftTargetLeftJustDown(struct CPad* pad);
int16_t CPad__KBShiftTargetRightJustDown(struct CPad* pad);
int16_t CPad__KBSniperModeLookLeftRight(struct CPad* pad);
int16_t CPad__KBSniperModeLookUpDown(struct CPad* pad);
int16_t CPad__KBLookAroundLeftRight(struct CPad* pad);
int16_t CPad__KBLookAroundUpDown(struct CPad* pad);
int16_t CPad__KBGuiLeft(struct CPad* pad);
int16_t CPad__KBGuiRight(struct CPad* pad);
int16_t CPad__KBGuiUp(struct CPad* pad);
int16_t CPad__KBGuiDown(struct CPad* pad);
int16_t CPad__KBGuiSelect(struct CPad* pad);
int16_t CPad__KBGuiBack(struct CPad* pad);
int16_t CPad__KBGetSkipCutscene(struct CPad* pad);
int16_t CPad__KBGetOddJobTrigger(struct CPad* pad);
int16_t CPad__KBEnterFreeAim(struct CPad* pad);
int16_t CPad__KBGetLeftStickX(struct CPad* pad);
int16_t CPad__KBGetLeftStickY(struct CPad* pad);
int16_t CPad__KBGetRightStickX(struct CPad* pad);
int16_t CPad__KBGetRightStickY(struct CPad* pad);

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

#endif
