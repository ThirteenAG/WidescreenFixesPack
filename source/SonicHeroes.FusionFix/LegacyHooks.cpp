#include <stdafx.h>
#include "LegacyHooks.hpp"

import ComVars;
import Rendering;

void __declspec(naked) StretchOnBoot()
{
    if (!bStretchOnBootFlag)
    {
        bStretchOnBootFlag = true;
        // Invalidate it to trigger the window update and device reset.
        _asm
        {
            pushad
            call InvalidateWindowSize
            popad
        }
    }
    _asm
    {
        mov edx, [esp + 0x20]
        mov eax, ptrWindowSizeX
        mov eax, [eax]
        jmp loc_6567BF
    }
}

void __declspec(naked) LensFlareScale()
{
    _asm
    {
        fmul[fLensFlareScalar]
        mov eax, [esi + 0x1C]
        fmul dword ptr[esi + 0x10]
        jmp loc_48F41F
    }
}

void __declspec(naked) RestoreDemos()
{
    _asm
    {
        inc ecx
        cmp cl, 06
        cmovge ecx, ebx
        //cmp ecx, ebx
        //jle thyjle
        //xor ecx, ecx
        //thyjle:
        jmp DemoRestoreExit1
    }
}


#pragma runtime_checks("", off)

namespace AdvertiseWindowFix
{
    void __declspec(naked) hkAdvWindowConstructor()
    {
        _asm
        {
            pushad
            call AdvWindowConstructorHook
            popad
            jmp sub_456AA0
        }
    }
}

namespace AdvStaffRollFix
{
    void __declspec(naked) hkStaffrollConstructor()
    {
        _asm
        {
            call sub_454F10
            pushad
            push esi
            call AdvStaffrollConstructorHook
            popad
            retn
        }
    }
}


#pragma runtime_checks("", restore)
