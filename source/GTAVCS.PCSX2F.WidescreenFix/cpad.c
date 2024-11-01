#include "cpad.h"
#include "../../includes/pcsx2/injector.h"

int* CMenuManager__m_PrefsInvertLook;
int16_t(*pCPad__GetLookBehindForCar)(struct CPad* pad);

void ReplacePadFuncsWithModernControls()
{
    // Disabling DPAD movement
    injector.MakeNOP(0x285764);
    injector.MakeNOP(0x285770);
    injector.MakeNOP(0x2858BC);
    injector.MakeNOP(0x2858C8);

    // CPad::SniperMode
    injector.MakeJAL(0x286938, (intptr_t)CPad__GetRightStickX);
    injector.MakeJAL(0x286A40, (intptr_t)CPad__GetRightStickY);
    injector.MakeJAL(0x286A6C, (intptr_t)CPad__GetRightStickY);

    // Patching look left/right/behind
    injector.WriteMemory16(0x2859F8, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285A20, offsetof(struct CPad, NewState.LEFTSHOULDER1));
    injector.WriteMemory16(0x285A40, offsetof(struct CPad, OldState.LEFTSHOULDER1));
    injector.WriteMemory16(0x285A50, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285A5C, offsetof(struct CPad, OldState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285A34, offsetof(struct CPad, NewState.LEFTSHOULDER1));
    injector.WriteMemory16(0x2859D0, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285A04, offsetof(struct CPad, NewState.LEFTSHOULDER1));

    // Look right
    injector.WriteMemory16(0x285B40, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285B68, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285B90, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285BB0, offsetof(struct CPad, OldState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285BC0, offsetof(struct CPad, NewState.LEFTSHOULDER1));
    injector.WriteMemory16(0x285BCC, offsetof(struct CPad, OldState.LEFTSHOULDER1));
    injector.WriteMemory16(0x285BA4, offsetof(struct CPad, NewState.LEFTSHOULDER2));
    injector.WriteMemory16(0x285B74, offsetof(struct CPad, NewState.LEFTSHOULDER1));

    // Look behind
    injector.WriteMemory16(0x285C9C, offsetof(struct CPad, NewState.LEFTSHOULDER1));
    injector.WriteMemory16(0x285CBC, offsetof(struct CPad, OldState.LEFTSHOULDER1));
    injector.WriteMemory16(0x285CCC, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285CC4, offsetof(struct CPad, NewState.RIGHTSHOULDER1));
    injector.WriteMemory16(0x285CD8, offsetof(struct CPad, OldState.RIGHTSHOULDER1));

    //CPad__GetLookBehindForCar - add possibility to look back with R3
    injector.MakeJAL(0x29486C, (intptr_t)CPad__GetLookBehindForCar);
    injector.MakeJAL(0x295080, (intptr_t)CPad__GetLookBehindForCar);
    injector.MakeJAL(0x2A2BC0, (intptr_t)CPad__GetLookBehindForCar);
    injector.MakeJAL(0x3A7DA4, (intptr_t)CPad__GetLookBehindForCar);

    // Other bindings
    injector.MakeJMPwNOP(0x285FA0, (intptr_t)CPad__GetHandBrake);
    injector.MakeJMPwNOP(0x286030, (intptr_t)CPad__GetBrake);
    injector.MakeJMPwNOP(0x286148, (intptr_t)CPad__GetWeapon);
    injector.MakeJMPwNOP(0x286180, (intptr_t)CPad__WeaponJustDown);
    injector.MakeJMPwNOP(0x2861D0, (intptr_t)CPad__GetAccelerate);
    injector.MakeJMPwNOP(0x286328, (intptr_t)CPad__ChangeStationUpJustDown);
    injector.MakeJMPwNOP(0x2863E0, (intptr_t)CPad__ChangeStationDownJustDown);
    injector.MakeJMPwNOP(0x286408, (intptr_t)CPad__CycleWeaponLeftJustDown);
    injector.MakeJMPwNOP(0x286458, (intptr_t)CPad__CycleWeaponRightJustDown);
    injector.MakeJMPwNOP(0x2864A8, (intptr_t)CPad__GetTarget);
    injector.MakeJMPwNOP(0x286500, (intptr_t)CPad__TargetJustDown);
    injector.MakeJMPwNOP(0x286578, (intptr_t)CPad__DuckJustDown);
    injector.MakeJMPwNOP(0x286768, (intptr_t)CPad__SniperZoomIn);
    injector.MakeJMPwNOP(0x2867A8, (intptr_t)CPad__SniperZoomOut);
    injector.MakeJMPwNOP(0x286808, (intptr_t)CPad__ShiftTargetLeftJustDown);
    injector.MakeJMPwNOP(0x286850, (intptr_t)CPad__ShiftTargetRightJustDown);
    injector.MakeJMPwNOP(0x286AF0, (intptr_t)CPad__LookAroundLeftRight);
    injector.MakeJMPwNOP(0x286B20, (intptr_t)CPad__LookAroundUpDown);
    injector.MakeJMPwNOP(0x286DE8, (intptr_t)CPad__GetOddJobTrigger);
    injector.MakeJMPwNOP(0x286E30, (intptr_t)CPad__EnterFreeAim);
    injector.MakeJMPwNOP(0x287640, (intptr_t)CPad__GetLeftStickX);
    injector.MakeJMPwNOP(0x287678, (intptr_t)CPad__GetLeftStickY);
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
