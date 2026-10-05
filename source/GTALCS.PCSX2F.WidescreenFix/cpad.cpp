#include "../../external/injector/include/ps2/runtime.hpp"
using namespace mips_asm;

extern "C" {
#include "cpad.h"

int* CMenuManager__m_PrefsInvertLook;
int16_t(*pCPad__GetLookBehindForCar)(struct CPad* pad);

void ReplacePadFuncsWithModernControls()
{
    // Disabling DPAD movement
    injector::MakeNOP(0x285764);
    injector::MakeNOP(0x285770);
    injector::MakeNOP(0x2858BC);
    injector::MakeNOP(0x2858C8);

    // CPad::SniperMode
    injector::MakeCALL(0x286938, CPad__GetRightStickX);
    injector::MakeCALL(0x286A40, CPad__GetRightStickY);
    injector::MakeCALL(0x286A6C, CPad__GetRightStickY);

    // Patching look left/right/behind
    injector::WriteMemory<uint16_t>(0x2859F8, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285A20, offsetof(struct CPad, NewState.LEFTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285A40, offsetof(struct CPad, OldState.LEFTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285A50, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285A5C, offsetof(struct CPad, OldState.RIGHTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285A34, offsetof(struct CPad, NewState.LEFTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x2859D0, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285A04, offsetof(struct CPad, NewState.LEFTSHOULDER1));

    // Look right
    injector::WriteMemory<uint16_t>(0x285B40, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285B68, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285B90, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285BB0, offsetof(struct CPad, OldState.RIGHTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285BC0, offsetof(struct CPad, NewState.LEFTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285BCC, offsetof(struct CPad, OldState.LEFTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285BA4, offsetof(struct CPad, NewState.LEFTSHOULDER2));
    injector::WriteMemory<uint16_t>(0x285B74, offsetof(struct CPad, NewState.LEFTSHOULDER1));

    // Look behind
    injector::WriteMemory<uint16_t>(0x285C9C, offsetof(struct CPad, NewState.LEFTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285CBC, offsetof(struct CPad, OldState.LEFTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285CCC, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector::WriteMemory<uint16_t>(0x285CC4, offsetof(struct CPad, NewState.RIGHTSHOULDER2));
    injector::WriteMemory<uint16_t>(0x285CD8, offsetof(struct CPad, OldState.RIGHTSHOULDER2));

    //CPad__GetLookBehindForCar - add possibility to look back with R3
    injector::MakeCALL(0x29486C, CPad__GetLookBehindForCar);
    injector::MakeCALL(0x295080, CPad__GetLookBehindForCar);
    injector::MakeCALL(0x2A2BC0, CPad__GetLookBehindForCar);
    injector::MakeCALL(0x3A7DA4, CPad__GetLookBehindForCar);

    // Other bindings
    injector::MakeJMP(0x285FA0, CPad__GetHandBrake);
    injector::MakeJMP(0x286030, CPad__GetBrake);
    injector::MakeJMP(0x286148, CPad__GetWeapon);
    injector::MakeJMP(0x286180, CPad__WeaponJustDown);
    injector::MakeJMP(0x2861D0, CPad__GetAccelerate);
    injector::MakeJMP(0x286328, CPad__ChangeStationUpJustDown);
    injector::MakeJMP(0x2863E0, CPad__ChangeStationDownJustDown);
    injector::MakeJMP(0x286408, CPad__CycleWeaponLeftJustDown);
    injector::MakeJMP(0x286458, CPad__CycleWeaponRightJustDown);
    injector::MakeJMP(0x2864A8, CPad__GetTarget);
    injector::MakeJMP(0x286500, CPad__TargetJustDown);
    injector::MakeJMP(0x286578, CPad__DuckJustDown);
    injector::MakeJMP(0x286768, CPad__SniperZoomIn);
    injector::MakeJMP(0x2867A8, CPad__SniperZoomOut);
    injector::MakeJMP(0x286808, CPad__ShiftTargetLeftJustDown);
    injector::MakeJMP(0x286850, CPad__ShiftTargetRightJustDown);
    injector::MakeJMP(0x286AF0, CPad__LookAroundLeftRight);
    injector::MakeJMP(0x286B20, CPad__LookAroundUpDown);
    injector::MakeJMP(0x286DE8, CPad__GetOddJobTrigger);
    injector::MakeJMP(0x286E30, CPad__EnterFreeAim);
    injector::MakeJMP(0x287640, CPad__GetLeftStickX);
    injector::MakeJMP(0x287678, CPad__GetLeftStickY);
}

int16_t CPad__GetHandBrake(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.CROSS;
}
int16_t CPad__GetBrake(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.LEFTSHOULDER2;
}
int16_t CPad__GetWeapon(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->Mode >= 4)
        return 0;
    return pad->NewState.RIGHTSHOULDER2;
}
int16_t CPad__WeaponJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->Mode >= 4)
        return 0;
    if (pad->NewState.RIGHTSHOULDER2)
        return pad->OldState.RIGHTSHOULDER2 == 0;
    return 0;
}
int16_t CPad__GetAccelerate(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.RIGHTSHOULDER2;
}
int16_t CPad__ChangeStationUpJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.DPADRIGHT)
        return pad->OldState.DPADRIGHT == 0;
    return 0;
}
int16_t CPad__ChangeStationDownJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.DPADLEFT)
        return pad->OldState.DPADLEFT == 0;
    return 0;
}
int16_t CPad__CycleWeaponLeftJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->Mode >= 4)
        return 0;
    if (pad->NewState.DPADLEFT)
        return pad->OldState.DPADLEFT == 0;
    return 0;
}
int16_t CPad__CycleWeaponRightJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.DPADRIGHT)
        return pad->OldState.DPADRIGHT == 0;
    return 0;
}
int16_t CPad__GetTarget(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.LEFTSHOULDER2 != 0;
}
int16_t CPad__TargetJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.LEFTSHOULDER2)
        return pad->OldState.LEFTSHOULDER2 == 0;
    return 0;
}
int16_t CPad__DuckJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.DPADDOWN)
        return pad->OldState.DPADDOWN == 0;
    return 0;
}
int16_t CPad__SniperZoomIn(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    return pad->NewState.SQUARE != 0 || ((pad->NewState.LEFTSTICKY < -10) ? pad->NewState.LEFTSTICKY : 0);
}
int16_t CPad__SniperZoomOut(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    return pad->NewState.CROSS != 0 || ((pad->NewState.LEFTSTICKY > -10) ? pad->NewState.LEFTSTICKY : 0);;
}
int16_t CPad__ShiftTargetLeftJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.DPADLEFT)
        return pad->OldState.DPADLEFT == 0;
    return 0;
}
int16_t CPad__ShiftTargetRightJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.DPADRIGHT)
        return pad->OldState.DPADRIGHT == 0;
    return 0;
}
int16_t CPad__LookAroundLeftRight(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.RIGHTSTICKX;
}
int16_t CPad__LookAroundUpDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (*CMenuManager__m_PrefsInvertLook)
        return -pad->NewState.RIGHTSTICKY;
    else
        return pad->NewState.RIGHTSTICKY;
}
int16_t CPad__GetOddJobTrigger(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.DPADUP != 0;
}
int16_t CPad__EnterFreeAim(struct CPad* pad)
{
    return pad->NewState.LEFTSHOCK != 0 || pad->NewState.RIGHTSTICKX || pad->NewState.RIGHTSTICKY;
}
int16_t CPad__GetLeftStickX(struct CPad* pad)
{
    return pad->NewState.LEFTSTICKX;
}

int16_t CPad__GetLeftStickY(struct CPad* pad)
{
    return pad->NewState.LEFTSTICKY;
}
int16_t CPad__GetRightStickX(struct CPad* pad)
{
    return pad->NewState.RIGHTSTICKX;
}

int16_t CPad__GetRightStickY(struct CPad* pad)
{
    return pad->NewState.RIGHTSTICKY;
}

int16_t CPad__GetLookBehindForCar(struct CPad* pad)
{
    int16_t ret = pCPad__GetLookBehindForCar(pad);
    if (ret)
        return ret;
    else
        return pad->NewState.RIGHTSHOCK != 0;
}

//////////// ORIGINAL CODE ////////////////////
int16_t CPad__GetLookBehindForPed(struct CPad* pad)
{
    if (*(int*)0x489F7C)
        return 0;
    if (pad->DisablePlayerControls)
        return 0;
    return pad->NewState.RIGHTSHOCK != 0;
}
int16_t CPad__GetHorn(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->Mode == 1)
        return pad->NewState.LEFTSHOULDER2 != 0;
    if (pad->Mode < 2)
    {
        if (pad->Mode)
            return 0;
        return pad->NewState.RIGHTSHOCK != 0;
    }
    if (pad->Mode == 2)
        return pad->NewState.RIGHTSHOULDER2 != 0;
    if (pad->Mode == 3)
    {
        return pad->NewState.RIGHTSHOCK != 0;
    }
    return 0;
}
int16_t CPad__HornJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->Mode == 1)
    {
        if (!pad->NewState.LEFTSHOULDER2)
            return 0;
        return pad->OldState.LEFTSHOULDER1 == 0;
    }
    if (pad->Mode >= 2)
    {
        if (pad->Mode == 2)
        {
            if (!pad->NewState.RIGHTSHOULDER2)
                return 0;
            return pad->OldState.RIGHTSHOULDER1 == 0;
        }
        if (pad->Mode != 3)
            return 0;
        if (!pad->NewState.RIGHTSHOCK)
            return 0;
        return pad->OldState.LEFTSHOCK == 0;
    }
    if (!pad->Mode)
    {
        if (!pad->NewState.RIGHTSHOCK)
            return 0;
        return pad->OldState.LEFTSHOCK == 0;
    }
    return 0;
}
int16_t CPad__GetCarGunFired(struct CPad* pad)
{
    return !pad->DisablePlayerControls && pad->NewState.CIRCLE != 0;
}
int16_t CPad__CarGunJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.CIRCLE)
        return pad->OldState.CIRCLE == 0;
    return 0;
}
int16_t CPad__CarGunJustUp(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (!pad->NewState.CIRCLE)
        return pad->OldState.CIRCLE != 0;
    return 0;
}
int16_t CPad__JumpJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->Mode >= 4)
        return 0;
    if (pad->NewState.SQUARE)
        return pad->OldState.SQUARE == 0;
    return 0;
}
int16_t CPad__GetExitVehicle(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    else
        return pad->NewState.TRIANGLE;
}
int16_t CPad__ExitVehicleJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.TRIANGLE)
        return pad->OldState.TRIANGLE == 0;
    return 0;
}
int16_t CPad__CycleCameraModeUpJustDown(struct CPad* pad)
{
    if (pad->DisablePlayerControls)
        return 0;
    if (pad->NewState.SELECT)
        return pad->OldState.SELECT == 0;
    return 0;
}

} // extern "C"
