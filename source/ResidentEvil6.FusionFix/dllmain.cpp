#include "stdafx.h"
#include "LEDEffects.h"
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")
#include <vector>
#include <map>

static bool bLogiLedInitialized = false;

constexpr auto defaultAspectRatio = 16.0f / 9.0f;
bool bSplitScreenSwapTopBottom = false;
int32_t ResX = 0;
int32_t ResY = 0;

std::map<uintptr_t, uintptr_t> addrTbl;

void FillAddressTable()
{
    addrTbl[0x17CF454] = (uintptr_t)*hook::get_pattern<uint32_t>("8B 0D ? ? ? ? E8 ? ? ? ? 84 C0 75 0E", 2);
    addrTbl[0x186E8BC] = (uintptr_t)*hook::get_pattern<uint32_t>("8B 0D ? ? ? ? 6A 00 E8 ? ? ? ? EB 5E", 2);
    addrTbl[0x186E5D0] = (uintptr_t)*hook::get_pattern<uint32_t>("38 1D ? ? ? ? 74 0A 83 C7 04 57 FF 15 ? ? ? ? A1 ? ? ? ? 8D 70 1C 8B F8 38 1E 75 08 38 1D ? ? ? ? 74 0A 83 C0 04 50 FF 15 ? ? ? ? 89 9F ? ? ? ? 38 1E 75 08 38 1D ? ? ? ? 74 0A 83 C7 04 57 FF 15 ? ? ? ? 5F", 2);
    addrTbl[0x186E23C] = (uintptr_t)*hook::get_pattern<uint32_t>("8B 0D ? ? ? ? 56 E8 ? ? ? ? 3B C5", 2);
    addrTbl[0xE6E800] = (uintptr_t)injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 80 7D 10 00 74 23")).as_int();
    addrTbl[0x97BE91] = (uintptr_t)hook::get_pattern("0F 84 ? ? ? ? 48 74 0C 8B 04 8D");
    addrTbl[0x97BFDA] = (uintptr_t)hook::get_pattern("FF D0 89 87 ? ? ? ? 5F", 8);
    addrTbl[0xD23CFB] = (uintptr_t)hook::get_pattern("0F 8C ? ? ? ? 83 FA 04");
    addrTbl[0x58ED00] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B 54 24 24");
    addrTbl[0x58EDC3] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 66 0F 6E 44 24 ? 8B 54 24 10");
    addrTbl[0x5103E5] = (uintptr_t)hook::get_pattern("D8 0D ? ? ? ? 6A 00 83 EC 08", 2);
    addrTbl[0x55DF21] = (uintptr_t)hook::get_pattern("F3 0F 10 15 ? ? ? ? 2B C2", 4);
    addrTbl[0x58DF02] = (uintptr_t)hook::get_pattern("F3 0F 10 05 ? ? ? ? F3 0F 5E C1 51 F3 0F 11 44 24", 4);
    addrTbl[0x58E118] = (uintptr_t)hook::get_pattern("F3 0F 10 05 ? ? ? ? F3 0F 5E C1 52", 4);
    addrTbl[0x9FC4F3] = (uintptr_t)hook::get_pattern("F3 0F 59 05 ? ? ? ? F3 0F 11 44 24 ? F3 0F 10 05 ? ? ? ? F3 0F 11 44 24", 4);
    addrTbl[0x116A33B] = (uintptr_t)hook::get_pattern("F3 0F 10 0D ? ? ? ? F3 0F 5E C8 0F 2F CB", 4);
    addrTbl[0x116A378] = (uintptr_t)hook::get_pattern("F3 0F 10 05 ? ? ? ? F3 0F 5E C1 F3 0F 11 44 24 ? F3 0F 11 44 24", 4);
    addrTbl[0x50076B] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 8B 44 24 14 89 44 24 24");
    addrTbl[0x50079E] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? C7 86 ? ? ? ? ? ? ? ? C7 86 ? ? ? ? ? ? ? ? 8B 0D");
    addrTbl[0x974C80] = (uintptr_t)hook::get_pattern("51 8B 0D ? ? ? ? E8 ? ? ? ? 83 F8 01 75 1F");
    addrTbl[0x974CD0] = (uintptr_t)hook::get_pattern("8B 0D ? ? ? ? 83 EC 08 E8 ? ? ? ? 83 F8 01 75 2D");
    addrTbl[0x55DB40] = (uintptr_t)hook::get_pattern("51 8B 0D ? ? ? ? E8 ? ? ? ? 83 F8 01 75 21");
    addrTbl[0x55DBA0] = (uintptr_t)hook::get_pattern("8B 0D ? ? ? ? 83 EC 08 E8 ? ? ? ? 83 F8 01 75 2F");
    addrTbl[0x5F816C] = (uintptr_t)hook::get_pattern("8B 74 24 18 8B CE F3 0F 11 04 24", 11);
    addrTbl[0x96B347] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7 74 28 F3 0F 10 0F F3 0F 10 05 ? ? ? ? F3 0F 5E 46 ? F3 0F 5E C8 F3 0F 11 0F F3 0F 10 47 ? F3 0F 59 46 ? F3 0F 11 47 ? 5F 5E 83 C4 10");
    addrTbl[0x104357D] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 80 7D 10 00 74 23");
    addrTbl[0x104866C] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 8B 7D 08 38 9E");
    addrTbl[0x104F1F7] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? F3 0F 10 83 ? ? ? ? F3 0F 10 94 24");
    addrTbl[0x107A275] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 83 BE ? ? ? ? ? 8B C7 74 28 F3 0F 10 0F F3 0F 10 05 ? ? ? ? F3 0F 5E 46 ? F3 0F 5E C8 F3 0F 11 0F F3 0F 10 47 ? F3 0F 59 46 ? F3 0F 11 47 ? 5F 5E C2 04 00");
    addrTbl[0x109B17D] = (uintptr_t)hook::get_pattern("F3 0F 10 41 ? 8B CE F3 0F 11 04 24", 12);
    addrTbl[0x111366D] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? F3 0F 10 44 24 ? F3 0F 10 4C 24 ? F3 0F 10 7C 24 ? F3 0F 10 6C 24 ? F3 0F 10 74 24 ? 0F 28 D8 F3 0F 59 5C 24 ? 0F 28 D1");
    addrTbl[0x117789C] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? F3 0F 10 44 24 ? F3 0F 10 4C 24 ? F3 0F 10 7C 24 ? F3 0F 10 6C 24 ? 0F 28 D1");
    addrTbl[0x1197937] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? F3 0F 10 8C 24 ? ? ? ? F3 0F 10 94 24 ? ? ? ? F3 0F 10 A4 24 ? ? ? ? F3 0F 10 AC 24 ? ? ? ? F3 0F 10 BC 24");
    addrTbl[0x123F3CF] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 8B 7C 24 1C 8B CF");
    addrTbl[0x012915D1] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 89 47 08 0F B7 44 24");
    addrTbl[0x01291614] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? 89 47 08 0F B7 4C 24");
    addrTbl[0x511DAA] = (uintptr_t)hook::get_pattern("8A 85 ? ? ? ? 33 DB");
    addrTbl[0xF3CA40] = (uintptr_t)hook::get_pattern("89 47 24 8B 4D 00");
    addrTbl[0xF3CA8E] = (uintptr_t)hook::get_pattern("89 47 28 8B 4D 00");
    addrTbl[0x98410A] = (uintptr_t)hook::get_pattern("C6 46 38 14 E9 ? ? ? ? F3 0F 10 05");
    addrTbl[0x9842C3] = (uintptr_t)hook::get_pattern("C6 46 38 14 8B 4E 40");
    addrTbl[0x9840F5] = (uintptr_t)hook::get_pattern("C6 46 38 0A 8B 4E 40");
    addrTbl[0x58DE0F] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? E8 ? ? ? ? 89 06 8B 74 24 18");
    addrTbl[0x58E052] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 10 F3 0F 2C 4E");
    addrTbl[0x97A188] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5C 24 24 8B 55 0C");
    addrTbl[0x97A787] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5C 24 3C 8B 45 0C");
    addrTbl[0x55DCC7] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 04 8B 4C 24 14");
    addrTbl[0x55DCE5] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 0C 8B 44 24 1C");
    addrTbl[0x55DD52] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 04 A1");
    addrTbl[0x55DD8F] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 0C A1");
    addrTbl[0x55DE28] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 04 8B 4C 24 18 51 8B CF E8 ? ? ? ? D9 5E 08 8B 54 24 1C 52 8B CF E8 ? ? ? ? D9 5E 0C");
    addrTbl[0x55DE46] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 0C 8B 44 24 20 50");
    addrTbl[0x55DEDC] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 04 8B 54 24 18 52 8B CF E8 ? ? ? ? D9 54 24 30");
    addrTbl[0x55DF52] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 56 0C DA 4C 24 20 8B 4C 24 24");
    addrTbl[0x55DFE8] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 04 8B 4C 24 18 51 8B CF E8 ? ? ? ? D9 5E 08 8B 54 24 1C 52 8B CF E8 ? ? ? ? D9 56 0C");
    addrTbl[0x55E006] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 56 0C DA 4C 24 20 8B 4C 24 28");
    addrTbl[0x55E09C] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 04 8B 54 24 18 52 8B CF E8 ? ? ? ? 83 EC 08");
    addrTbl[0x55E0C8] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 5E 0C 8B 44 24 20 F3 0F 10 05");
    addrTbl[0x97D139] = (uintptr_t)hook::get_pattern("75 0A F3 0F 10 05 ? ? ? ? EB 43");
    addrTbl[0x9A0EE0] = (uintptr_t)hook::get_pattern("E8 ? ? ? ? D9 00 0F 57 C0 D9 5E 40 D9 40 04 5F");
    addrTbl[0x97D0F0] = (uintptr_t)hook::get_pattern("8B 44 24 0C 0F 57 C0 69 C0");
    addrTbl[0x01292227] = (uintptr_t)hook::get_pattern("FF 15 ? ? ? ? 56 68");
    addrTbl[0xF35DF6] = (uintptr_t)hook::get_pattern("FF 15 ? ? ? ? 8B D8 6A 00");
    addrTbl[0xF35EEC] = (uintptr_t)hook::get_pattern("FF 15 ? ? ? ? 89 86 ? ? ? ? EB 0A");
    addrTbl[0xF4228A] = (uintptr_t)hook::get_pattern("FF 15 ? ? ? ? 8B 44 24 20 2B 44 24 18");
    addrTbl[0xF4227E] = (uintptr_t)hook::get_pattern("FF 15 ? ? ? ? 8B 17 55");
    addrTbl[0xF422B1] = (uintptr_t)hook::get_pattern("FF 15 ? ? ? ? 8B 44 24 10 40");
    addrTbl[0x514CDB] = (uintptr_t)hook::get_pattern("F3 0F 10 05 ? ? ? ? 51 F3 0F 11 04 24 68 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? A1");
    addrTbl[0xD583AB] = (uintptr_t)hook::get_pattern("F3 0F 10 05 ? ? ? ? 51 F3 0F 11 04 24 68 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 8B 0D");
    addrTbl[0xEE17C5] = (uintptr_t)hook::get_pattern("D9 05 ? ? ? ? 5F 5E C2 0C 00");
    addrTbl[0x9DBE9D] = (uintptr_t)hook::get_pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 40 ? EB 28");
    addrTbl[0x9DB8E8] = (uintptr_t)hook::get_pattern("F3 0F 5C 05 ? ? ? ? F3 0F 10 0D ? ? ? ? 0F 54 C1");

    hook::pattern("6A 03 53 53 68").for_each_result([](hook::pattern_match match)
    {
        if (std::string_view((const char*)injector::ReadMemory<uint32_t>(*match.get<void*>(5), true)) == "VARIABLE")
        {
            addrTbl[0x016E62D8] = *match.get<uint32_t>(5) + 4;
            return;
        }
    });

#if _DEBUG
    if (std::string_view((const char*)0x016E1608, 9) == "VARIABLE")
    {
        for (auto& it : addrTbl)
        {
            assert(it.first == it.second);
        }
    }
#endif // _DEBUG
}

bool IsSplitScreenActive()
{
    auto ptr = *(uint32_t*)addrTbl[0x17CF454];
    return ptr && *(uint32_t*)(ptr + 0x86C) == 1;
}

int32_t GetResX()
{
    return *(int32_t*)(*(uint32_t*)addrTbl[0x186E8BC] + 0x120);
}

int32_t GetResY()
{
    return *(int32_t*)(*(uint32_t*)addrTbl[0x186E8BC] + 0x124);
}

int32_t GetRelativeResX()
{
    return *(int32_t*)(*(uint32_t*)addrTbl[0x186E8BC] + 0x128);
}

int32_t GetRelativeResY()
{
    return *(int32_t*)(*(uint32_t*)addrTbl[0x186E8BC] + 0x12C);
}

int32_t GetCurrentSplitScreenResX(int32_t val = 0)
{
    static int32_t CurrentSplitScreenResX = GetResX();
    if (val)
        CurrentSplitScreenResX = val;
    return CurrentSplitScreenResX;
}

int32_t GetCurrentSplitScreenResY(int32_t val = 0)
{
    static int32_t CurrentSplitScreenResY = GetResY() / 2;
    if (val)
        CurrentSplitScreenResY = val;
    return CurrentSplitScreenResY;
}

int32_t GetNativeSplitScreenResX(int32_t val = 0)
{
    static int32_t NativeSplitScreenResX = GetResX();
    if (val)
        NativeSplitScreenResX = val;
    return NativeSplitScreenResX;
}

int32_t GetNativeSplitScreenResY(int32_t val = 0)
{
    static int32_t NativeSplitScreenResY = GetResY() / 2;
    if (val)
        NativeSplitScreenResY = val;
    return NativeSplitScreenResY;
}

float GetAspectRatio()
{
    return (float)GetResX() / (float)GetResY();
}

float GetDiff()
{
    return GetAspectRatio() / defaultAspectRatio;
}

float GetSplitScreenDiff()
{
    return (float)GetCurrentSplitScreenResX() / (float)GetNativeSplitScreenResX();
}

int32_t GetRelativeSplitScreenResX()
{
    return (int32_t)((float)GetRelativeResX() / ((float)GetResX() / (float)GetCurrentSplitScreenResX())); //800
}

int32_t GetRelativeSplitScreenResY()
{
    return int32_t((float)GetRelativeResY() / 2.0f); //360
}

int32_t GetHudOffset()
{
    if (IsSplitScreenActive())
        return (int32_t)((((float)GetCurrentSplitScreenResX() - ((float)GetNativeSplitScreenResY() * ((float)GetNativeSplitScreenResX() / (float)GetNativeSplitScreenResY())))) / 2.0f);
    else
        return (int32_t)((GetResX() - (GetResY() * defaultAspectRatio)) / 2.0f);
}

void __fastcall sub_4F8C60(int _this, int edx, int a2, int32_t* a3)
{
    if (*(uint8_t*)(_this + 28) || *(uint8_t*)addrTbl[0x186E5D0])
        EnterCriticalSection((LPCRITICAL_SECTION)(_this + 4));
    *(uint8_t*)(400 * a2 + _this + 67) = 1;
    auto v4 = (int32_t*)(_this + 400 * a2);
    v4[18] = a3[0];
    v4[19] = a3[1];
    v4[20] = a3[2];
    v4[21] = a3[3];

    if (bSplitScreenSwapTopBottom)
    {
        if (a2 == 0)
            a2 = 1;
        else
            a2 = 0;
    }

    if (a2 == 0)
    {
        v4[18] = 0;
        v4[19] = 0;
        v4[20] = GetResX();
        v4[21] = GetResY() / 2;
    }
    else if (a2 == 1)
    {
        v4[18] = 0;
        v4[19] = GetResY() / 2;
        v4[20] = GetResX();
        v4[21] = GetResY();
    }

    GetNativeSplitScreenResX(a3[2] - a3[0]);
    GetNativeSplitScreenResY(a3[3] - a3[1]);
    GetCurrentSplitScreenResX(v4[20] - v4[18]);
    GetCurrentSplitScreenResY(v4[21] - v4[19]);

    if (*(uint8_t*)(_this + 28) || *(uint8_t*)addrTbl[0x186E5D0])
        LeaveCriticalSection((LPCRITICAL_SECTION)(_this + 4));
}

float __cdecl sub_974C80(int a1)
{
    if (IsSplitScreenActive())
        return (float)(*(uint32_t*)(*(uint32_t*)addrTbl[0x186E23C] + 80) - *(uint32_t*)(*(uint32_t*)addrTbl[0x186E23C] + 72)) * (float)a1 * (1.0f / (GetRelativeSplitScreenResX() * GetDiff()));
    else
        return (float)(a1 * *(uint32_t*)(*(uint32_t*)addrTbl[0x186E8BC] + 288)) * ((1.0f / (GetRelativeResX() * GetDiff())));
}

float __cdecl sub_974CD0(int a1)
{
    if (IsSplitScreenActive())
        return (float)(*(uint32_t*)(*(uint32_t*)addrTbl[0x186E23C] + 80) - *(uint32_t*)(*(uint32_t*)addrTbl[0x186E23C] + 72))
            * GetRelativeSplitScreenResY()
            * (1.0f / (GetRelativeSplitScreenResX() * GetDiff()))
            * (float)a1
            * (1.0f / GetRelativeSplitScreenResY());
    else
        return (float)(a1 * *(uint32_t*)(*(uint32_t*)addrTbl[0x186E8BC] + 292)) * (1.0f / GetRelativeResY());
}

float __stdcall sub_55DB40(int a1)
{
    return sub_974C80(a1);
}

float __stdcall sub_55DBA0(int a1)
{
    return sub_974CD0(a1);
}

float __cdecl sub_974C80_center(int a1)
{
    return sub_974C80(a1) + GetHudOffset();
}

float __stdcall sub_55DB40_center(int a1)
{
    return sub_974C80(a1) + GetHudOffset();
}

float __stdcall sub_55DB40_stretch(int a1)
{
    return sub_974C80(a1) + (GetHudOffset() * 2.0f);
}

float fAspectRatioInv = (1.0f / (16.0f / 9.0f));
void __stdcall sub_58DDF0(uint32_t * a1, int* a2, int a3, uint16_t a4)
{
    int v4; // eax
    float v5; // st7
    int v6; // esi
    int v7; // ecx
    float v8; // st7
    int v9; // eax
    float v10; // st7
    float v11; // st7
    int v12; // [esp-20h] [ebp-2Ch]
    int v13; // [esp-1Ch] [ebp-28h]
    int v14; // [esp-18h] [ebp-24h]
    int v15; // [esp-18h] [ebp-24h]
    int v16; // [esp-14h] [ebp-20h]
    int v17; // [esp-10h] [ebp-1Ch]
    float v18; // [esp+4h] [ebp-8h]

    fAspectRatioInv = 1.0f / GetAspectRatio();

    if (IsSplitScreenActive())
    {
        *a1 = (int)sub_974C80(*a1) + GetHudOffset();
        *a2 = (int)sub_974CD0(*a2);
        *(float*)(a3 + 16) = sub_974C80((int)*(float*)(a3 + 16));
        *(float*)(a3 + 20) = sub_974CD0((int)*(float*)(a3 + 20));
        v4 = (int)sub_974C80(*(uint32_t*)(a3 + 8));
        v14 = *(uint32_t*)(a3 + 12);
        *(uint32_t*)(a3 + 8) = (int32_t)((float)v4);
        *(uint32_t*)(a3 + 12) = (int32_t)(sub_974CD0(v14));
        v5 = sub_974C80(*(uint32_t*)(a3 + 32));
        v12 = *(uint32_t*)(a3 + 36);
        *(uint32_t*)(a3 + 32) = (int32_t)v5;
        *(uint32_t*)(a3 + 36) = (int32_t)sub_974CD0(v12);
    }
    else
    {
        v6 = *(uint32_t*)(*(uint32_t*)addrTbl[0x186E8BC] + 292);
        v18 = fAspectRatioInv / (float)((float)v6 / (float)*(int*)(*(uint32_t*)addrTbl[0x186E8BC] + 288));
        *a1 = (int)sub_974C80(*a1) + GetHudOffset();
        v7 = (int)sub_974CD0(*a2);
        *a2 = v7;
        if (a4)
            *a2 = (int)(float)((float)((float)(v7 - v6 / 2) * v18) + (float)(v6 / 2));
        *(float*)(a3 + 16) = sub_974C80((int)*(float*)(a3 + 16));
        v8 = sub_974CD0((int)*(float*)(a3 + 20));
        v17 = *(uint32_t*)(a3 + 8);
        *(float*)(a3 + 20) = v8 * v18;
        v9 = (int)sub_974C80(v17);
        v16 = *(uint32_t*)(a3 + 12);
        *(int32_t*)(a3 + 8) = (int32_t)((float)v9);
        v10 = sub_974CD0(v16);
        v15 = *(uint32_t*)(a3 + 32);
        *(uint32_t*)(a3 + 12) = (int)(float)((float)(int)v10 * v18);
        v11 = sub_974C80(v15);
        v13 = *(uint32_t*)(a3 + 36);
        *(uint32_t*)(a3 + 32) = (int32_t)v11;
        *(uint32_t*)(a3 + 36) = (int32_t)sub_974CD0(v13);
    }
}

void __fastcall sub_E6E800(float* _this, void* edx, float a2, float a3, float a4, float a5)
{
    if (IsSplitScreenActive())
        a2 /= GetSplitScreenDiff();
    else
        a2 /= GetDiff();
    return injector::fastcall<void(float*, void*, float, float, float, float)>::call(addrTbl[0xE6E800], _this, edx, a2, a3, a4, a5);
}

IDirect3DVertexShader9* shader_4F0EE939 = nullptr;
IDirect3DVertexShader9* __stdcall CreateVertexShaderHook(const DWORD** a1)
{
    if (!a1)
        return nullptr;

    auto pDevice = (IDirect3DDevice9*)*(uint32_t*)(*(uint32_t*)addrTbl[0x186E8BC] + 256);
    IDirect3DVertexShader9* pShader = nullptr;
    pDevice->CreateVertexShader(a1[2], &pShader);

    if (pShader != nullptr)
    {
        static std::vector<uint8_t> pbFunc;
        UINT len;
        pShader->GetFunction(nullptr, &len);
        if (pbFunc.size() < len)
            pbFunc.resize(len);

        pShader->GetFunction(pbFunc.data(), &len);

        uint32_t crc32(uint32_t crc, const void* buf, size_t size);
        auto crc = crc32(0, pbFunc.data(), len);

        // various overlays (low health, waiting for partner, pause text, maybe more)
        if (crc == 0x4F0EE939)
        {
            const char* shader_text =
                "vs_3_0\n"
                "def c0, 32768, -128, 0.00390625, 0.25\n"
                "def c4, 1, 0, -128, 4\n"
                "def c5, 0.000244140654, 0.5, 6.28318548, -3.14159274\n"
                "def c6, 2, -1, 1, 9.99999997e-007\n"
                "dcl_position v0\n"
                "dcl_normal v1\n"
                "dcl_tangent v2\n"
                "dcl_binormal v3\n"
                "dcl_texcoord v4\n"
                "dcl_position o0\n"
                "dcl_texcoord o1\n"
                "dcl_texcoord1 o2\n"
                "mov r0.x, v3.x\n"
                "add o0.z, r0.x, v0.z\n"
                "add r0.xyz, c0.x, v3.zwyw\n"
                "mul r0.xz, r0, c0.z\n"
                "mad r0.y, r0.y, c5.x, c5.y\n"
                "frc r0.y, r0.y\n"
                "mad r0.y, r0.y, c5.z, c5.w\n"
                "sincos r1.xy, r0.y\n"
                "mul o1.xyz, r0.z, v1\n"
                "mul o2.xy, c3, v2\n"
                "add r0.yz, c0.y, v4.xxyw\n"
                "mul r0.xy, r0.x, r0.yzzw\n"
                "mad r0.zw, v4.z, c4.xyxy, c4\n"
                "mul r0.xy, r0.zwzw, r0\n"
                "mul r0.x, r0.x, c0.w\n"
                "mul r0.yz, r1.xyxw, r0.y\n"
                "mad r2.x, r0.x, r1.x, -r0.y\n"
                "mad r2.y, r0.x, r1.y, r0.z\n"
                "add r0.xy, r2, v0\n"
                "mul r0.xy, r0, c2\n"
                "mad r0.xy, r0, c6.x, c6.yzzw\n"
                "slt r0.z, v1.w, c6.w\n"
                "add r0.z, -r0.z, c4.x\n"
                "mul r0.x, r0.x, c230.x\n"
                "mad o0.x, c1.x, -r0.z, r0.x\n"
                "mad o0.y, c1.y, r0.z, r0.y\n"
                "mov o0.w, r0.z\n"
                "mov o1.w, v1.w\n"
                "mov o2.zw, c4.y\n";

            LPD3DXBUFFER pCode;
            LPD3DXBUFFER pErrorMsgs;
            LPDWORD pShaderData;
            auto result = D3DXAssembleShader(shader_text, strlen(shader_text), NULL, NULL, 0, &pCode, &pErrorMsgs);
            if (SUCCEEDED(result))
            {
                pShaderData = (DWORD*)pCode->GetBufferPointer();
                IDirect3DVertexShader9* shader = nullptr;
                result = pDevice->CreateVertexShader(pShaderData, &shader);
                if (FAILED(result)) {
                    return pShader;
                }
                else
                {
                    shader_4F0EE939 = shader;
                    pShader->Release();
                    return shader;
                }
            }
        }
        else if (crc == 0xF09EC5E9) // low health rotating blurry rectangle
        {
            const char* shader_text =
                "vs_3_0\n"
                "def c0, 32768, 1, 3.90624991e-005, 0\n"
                "def c10, 0.000244140625, 0, 0, 0\n"
                "def c11, 2, 0, 0, 0\n"
                "dcl_position v0\n"
                "dcl_normal v1\n"
                "dcl_tangent v2\n"
                "dcl_binormal v3\n"
                "dcl_position o0\n"
                "dcl_texcoord o1\n"
                "dcl_texcoord1 o2\n"
                "dcl_texcoord2 o3\n"
                "mov r0.x, c1.w\n"
                "mov r0.y, c2.w\n"
                "mov r0.z, c3.w\n"
                "mov r0.w, c4.w\n"
                "mov r1.xyz, v0\n"
                "mad r1.xyz, c7, -v3.x, r1\n"
                "mov r1.w, c0.y\n"
                "dp4 r0.x, r1, r0\n"
                "rcp r0.x, r0.x\n"
                "mul r0.yzw, c2.xxyw, v0.y\n"
                "mad r0.yzw, v0.x, c1.xxyw, r0\n"
                "mad r0.yzw, v0.z, c3.xxyw, r0\n"
                "add r0.yzw, r0, c4.xxyw\n"
                "mul r2.x, r0.w, c1.z\n"
                "mul r2.y, r0.w, c2.z\n"
                "mul r2.z, r0.w, c3.z\n"
                "mul r2.w, r0.w, c4.z\n"
                "mul r2, r0.x, r2\n"
                "dp4 o0.z, r1, r2\n"
                "add r0.x, c0.x, v3.y\n"
                "mul r0.x, r0.x, c9.z\n"
                "mul o2.w, r0.x, c0.z\n"
                "mul r0.y, r0.y, c11.x\n"
                "mad o0.x, c8.x, -r0.w, r0.y\n"
                "mad o0.y, c8.y, r0.w, r0.z\n"
                "mov o0.w, r0.w\n"
                "mov o1, v1\n"
                "mov o2.xyz, c0.ywww\n"
                "mul o3, c10.xxyy, v2.xyxx\n";

            LPD3DXBUFFER pCode;
            LPD3DXBUFFER pErrorMsgs;
            LPDWORD pShaderData;
            auto result = D3DXAssembleShader(shader_text, strlen(shader_text), NULL, NULL, 0, &pCode, &pErrorMsgs);
            if (SUCCEEDED(result))
            {
                pShaderData = (DWORD*)pCode->GetBufferPointer();
                IDirect3DVertexShader9* shader = nullptr;
                result = pDevice->CreateVertexShader(pShaderData, &shader);
                if (FAILED(result)) {
                    return pShader;
                }
                else
                {
                    pShader->Release();
                    return shader;
                }
            }
        }
    }

    return pShader;
}

//IDirect3DPixelShader9* shader_dummy = nullptr;
//IDirect3DPixelShader9* shader_498080AC = nullptr;
//IDirect3DPixelShader9* shader_FD473559 = nullptr;
//IDirect3DPixelShader9* __stdcall CreatePixelShaderHook(const DWORD** a1)
//{
//    if (!a1)
//        return nullptr;
//
//    auto pDevice = (IDirect3DDevice9*)*(uint32_t*)(*(uint32_t*)addrTbl[0x186E8BC] + 256);
//    IDirect3DPixelShader9* pShader = nullptr;
//    pDevice->CreatePixelShader(a1[2], &pShader);
//
//    if (pShader != nullptr)
//    {
//        static std::vector<uint8_t> pbFunc;
//        UINT len;
//        pShader->GetFunction(nullptr, &len);
//        if (pbFunc.size() < len)
//            pbFunc.resize(len);
//
//        pShader->GetFunction(pbFunc.data(), &len);
//
//        uint32_t crc32(uint32_t crc, const void* buf, size_t size);
//        auto crc = crc32(0, pbFunc.data(), len);
//
//        if (crc == 0x498080AC)      // low health rotating blurry rectangle
//        {
//            const char* shader_text =
//                "ps_3_0\n"
//                "dcl_texcoord v0.xy\n"
//                "dcl_2d s0\n"
//                "dcl_2d s1\n"
//                "texld r0, v0, s0\n"
//                "mul r0.xyz, r0, r0\n"
//                "mov oC0.w, r0.w\n"
//                "mul r1.xyz, r0.y, c2\n"
//                "mad r1.xyz, r0.x, c1, r1\n"
//                "mad r1.xyz, r0.z, c3, r1\n"
//                "add r1.xyz, r1, c4\n"
//                "texld r2, r1.x, s1\n"
//                "mul r2.x, r2.x, r2.x\n"
//                "texld r3, r1.y, s1\n"
//                "texld r1, r1.z, s1\n"
//                "mul r2.z, r1.z, r1.z\n"
//                "mul r2.y, r3.y, r3.y\n"
//                "rsq r0.x, r0.x\n"
//                "rcp oC0.x, r0.x\n"
//                "rsq r0.x, r0.y\n"
//                "rsq r0.y, r0.z\n"
//                "rcp oC0.z, r0.y\n"
//                "rcp oC0.y, r0.x\n";
//
//            LPD3DXBUFFER pCode;
//            LPD3DXBUFFER pErrorMsgs;
//            LPDWORD shader_data;
//            auto result = D3DXAssembleShader(shader_text, strlen(shader_text), NULL, NULL, 0, &pCode, &pErrorMsgs);
//            if (SUCCEEDED(result))
//            {
//                shader_data = (DWORD*)pCode->GetBufferPointer();
//                IDirect3DPixelShader9* shader = nullptr;
//                result = pDevice->CreatePixelShader(shader_data, &shader);
//                if (FAILED(result)) {
//                    return pShader;
//                }
//                else
//                {
//                    pShader->Release();
//                    return shader;
//                }
//            }
//        }
//
//        //else if (crc == 0xFD473559) // waiting for partner overlay
//        //    shader_FD473559 = pShader;
//        //else if (crc == 0x793BE067) // injured overlay
//        //    shader_793BE067 = pShader;
//    }
//
//    return pShader;
//}

bool bDisableObjectiveIndicator = false;
float* __stdcall ObjectiveIndicator(float* a1, float* a2, int a3)
{
    if (bDisableObjectiveIndicator)
        return a1;


    // needs proper fix
    auto p186E23C = *(uint32_t*)addrTbl[0x186E23C];

    auto v12 = 0;
    auto v13 = 0;
    auto v14 = 0;
    auto v15 = 0;

    auto v3 = 1.0f;
    auto v4 = *(int32_t*)(400 * a3 + p186E23C + 72);
    auto v5 = *(int32_t*)(400 * a3 + p186E23C + 76);
    auto v6 = *(int32_t*)(400 * a3 + p186E23C + 80);
    auto v8 = *(int32_t*)(400 * a3 + p186E23C + 84);
    auto gameMode = *(uint32_t*)(p186E23C + 3296);
    auto v9 = 1.0f;

    auto additionalScaler = 1.0f;
    auto scrRes = GetResX() * GetResY();
    if (scrRes > 1920 * 1080)
        additionalScaler = ((float)scrRes / (1920.0f * 1080.0f)) / 2.0f;

    if (gameMode == 2)
    {
        auto v10 = 2.0f / additionalScaler;
        v12 = (int)((float)v4 * v10);
        v13 = (int)((float)v5 * v10);
        v14 = (int)((float)v6 * v10);
        v15 = (int)((float)v8 * v10);
        a1[0] = (float)(((float)(v15 - v13) * defaultAspectRatio) * a2[0]) * (v9 / GetSplitScreenDiff());
        a1[1] = (float)((float)(v15 - v13) * a2[1]) * v3;
        a1[2] = -2650.0;
        a1[3] = 0.0;
        return a1;
    }
    if (gameMode == 1)
    {
        auto v11 = (float)(v6 - v4) / (float)(v8 - v5);
        if (v11 > defaultAspectRatio)
            v9 = (float)(v11 - defaultAspectRatio) * 1.25f;
        auto v10 = 3.049072f / additionalScaler;
        v12 = (int)(float)((float)v4 * v10);
        v13 = (int)(float)((float)v5 * v10);
        v14 = (int)(float)((float)v6 * v10);
        v15 = (int)(float)((float)v8 * v10);
        a1[0] = (float)(((float)(v15 - v13) * defaultAspectRatio) * a2[0]) * (v9 / GetSplitScreenDiff());
        a1[1] = (float)((float)(v15 - v13) * a2[1]) * v3;
        a1[2] = -2650.0f;
        a1[3] = 0.0f;
        return a1;
    }

    auto v16 = (float)(v6 - v4) / (float)(v8 - v5);
    if (v16 < defaultAspectRatio)
        v3 = (float)((float)(defaultAspectRatio - v16) * 1.25f) + 1.0f;
    auto v10 = 1.524536f / additionalScaler;
    v12 = (int)(float)((float)v4 * v10);
    v13 = (int)(float)((float)v5 * v10);
    v14 = 1951;
    v15 = 1097;
    a1[0] = (float)(((float)(v15 - v13) * defaultAspectRatio) * a2[0]) * (v9 / GetSplitScreenDiff());
    a1[1] = (float)((float)(v15 - v13) * a2[1]) * v3;
    a1[2] = -2650.0f;
    a1[3] = 0.0f;
    return a1;
}

void Init()
{
    CIniReader iniReader("");
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
    bSplitScreenSwapTopBottom = iniReader.ReadInteger("MAIN", "SplitScreenSwapTopBottom", 0) != 0;
    auto bDisableDamageOverlay = iniReader.ReadInteger("MAIN", "DisableDamageOverlay", 1) != 0;
    auto bDisableDBNOEffects = iniReader.ReadInteger("MAIN", "DisableDBNOEffects", 0) != 0;
    bDisableObjectiveIndicator = iniReader.ReadInteger("MAIN", "DisableObjectiveIndicator", 0) != 0;

    FillAddressTable();
    
    if (bSkipIntro)
    {
        injector::MakeNOP(addrTbl[0x97BE91], 6);
        injector::MakeJMP(addrTbl[0x97BE91], addrTbl[0x97BFDA]);

        injector::WriteMemory<uint16_t>(addrTbl[0xD23CFB], 0xE990, true);
    }

    // overwriting aspect ratio
    hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 8E").for_each_result([](hook::pattern_match match)
    {
        struct hook_ecx_edx { void operator()(injector::reg_pack& regs) { ResX = regs.ecx; ResY = regs.edx; fAspectRatioInv = 1.0f / GetAspectRatio(); } };
        injector::MakeInline<hook_ecx_edx>(match.get<void>(0), match.get<void>(12));
    });

    hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 AE").for_each_result([](hook::pattern_match match)
    {
        struct hook_ebp_edi { void operator()(injector::reg_pack& regs) { ResX = regs.ebp; ResY = regs.edi; fAspectRatioInv = 1.0f / GetAspectRatio(); } };
        injector::MakeInline<hook_ebp_edi>(match.get<void>(0), match.get<void>(12));
    });

    // interface scaling
    injector::MakeCALL(addrTbl[0x58ED00], sub_58DDF0, true);
    injector::MakeCALL(addrTbl[0x58EDC3], sub_58DDF0, true);
    injector::WriteMemory(addrTbl[0x5103E5],  &fAspectRatioInv, true);  // fmul    ds : dword_151B5D8
    injector::WriteMemory(addrTbl[0x55DF21],  &fAspectRatioInv, true);  // movss   xmm2, ds : dword_151B5D8
    injector::WriteMemory(addrTbl[0x58DF02],  &fAspectRatioInv, true);  // movss   xmm0, ds : dword_151B5D8
    injector::WriteMemory(addrTbl[0x58E118],  &fAspectRatioInv, true);  // movss   xmm0, ds : dword_151B5D8
    injector::WriteMemory(addrTbl[0x9FC4F3],  &fAspectRatioInv, true);  // mulss   xmm0, ds : dword_151B5D8
    injector::WriteMemory(addrTbl[0x116A33B], &fAspectRatioInv, true);  // movss   xmm1, ds : dword_151B5D8
    injector::WriteMemory(addrTbl[0x116A378], &fAspectRatioInv, true);  // movss   xmm0, ds : dword_151B5D8

    // split screen setup
    injector::MakeCALL(addrTbl[0x50076B], sub_4F8C60, true);
    injector::MakeCALL(addrTbl[0x50079E], sub_4F8C60, true);

    // hud fix
    injector::MakeJMP(addrTbl[0x974C80], sub_974C80, true);
    injector::MakeJMP(addrTbl[0x974CD0], sub_974CD0, true);

    injector::MakeJMP(addrTbl[0x55DB40], sub_55DB40, true);
    injector::MakeJMP(addrTbl[0x55DBA0], sub_55DBA0, true);

    // Camera near clip fix
    injector::MakeCALL(addrTbl[0x5F816C],  sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x96B347],  sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x104357D], sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x104866C], sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x104F1F7], sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x107A275], sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x109B17D], sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x111366D], sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x117789C], sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x1197937], sub_E6E800, true);
    injector::MakeCALL(addrTbl[0x123F3CF], sub_E6E800, true);

    // shader overlays (scale to fullscreen)
    {
        injector::MakeCALL(addrTbl[0x012915D1], CreateVertexShaderHook, true);
        //injector::MakeCALL(addrTbl[0x01291614], CreatePixelShaderHook, true);
        
        //static bool bIsPaused = false;
        //struct GameStateHook
        //{
        //    void operator()(injector::reg_pack& regs)
        //    {
        //        regs.eax = *(uint8_t*)(regs.ebp + 0x104A9);
        //        bIsPaused = !!regs.eax;
        //    }
        //}; injector::MakeInline<GameStateHook>(addrTbl[0x511DAA], addrTbl[0x511DAA] + 6);

        struct SetVertexShaderHook
        {
            void operator()(injector::reg_pack& regs)
            {
                //if ((IsSplitScreenActive() || GetDiff() > 1.0f)/* && !bIsPaused*/)
                {
                    auto pShader = (IDirect3DVertexShader9*)regs.eax;
                    //if (pShader == shader_4F0EE939)
                    {
                        //regs.eax = 0;
                        IDirect3DDevice9* pDevice = nullptr;
                        pShader->GetDevice(&pDevice);
                        static float arr[4];
                        arr[0] = IsSplitScreenActive() ? GetSplitScreenDiff() : GetDiff();
                        arr[1] = 0.0f;
                        arr[2] = 0.0f;
                        arr[3] = 0.0f;
                        if (arr[0] < 1.0f)
                            arr[0] = 1.0f;
                        pDevice->SetVertexShaderConstantF(230, &arr[0], 1);
                    }
                }
                *(uint32_t*)(regs.edi + 0x24) = regs.eax;
                regs.ecx = *(uint32_t*)(regs.ebp + 0x0);
            }
        }; injector::MakeInline<SetVertexShaderHook>(addrTbl[0xF3CA40], addrTbl[0xF3CA40] + 6);

        //struct SetPixelShaderHook
        //{
        //    void operator()(injector::reg_pack& regs)
        //    {
        //        if ((IsSplitScreenActive() || GetDiff() > 1.0f) && !bIsPaused)
        //        {
        //            auto pShader = (IDirect3DPixelShader9*)regs.eax;
        //            if (pShader == shader_498080AC || pShader == shader_FD473559)
        //            {
        //                regs.eax = (uint32_t)shader_dummy;
        //            }
        //        }
        //        *(uint32_t*)(regs.edi + 0x28) = regs.eax;
        //        regs.ecx = *(uint32_t*)(regs.ebp + 0x0);
        //    }
        //}; injector::MakeInline<SetPixelShaderHook>(addrTbl[0xF3CA8E], addrTbl[0xF3CA8E] + 6);

        // disabling injured overlay
        if (bDisableDamageOverlay)
        {
            injector::WriteMemory<uint8_t>(addrTbl[0x98410A] + 3, 0x16, true);
            injector::WriteMemory<uint8_t>(addrTbl[0x9842C3] + 3, 0x16, true);
        }

        if (bDisableDBNOEffects)
            injector::WriteMemory<uint8_t>(addrTbl[0x9840F5] + 3, 0x16, true);
    }

    {
        //injector::MakeCALL(0x5103C3, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        injector::MakeCALL(addrTbl[0x58DE0F], sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DE38, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DE52, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DE74, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DF11, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DF6D, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DF8B, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58DFBE, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        injector::MakeCALL(addrTbl[0x58E052], sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E06C, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E08E, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E127, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E145, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x58E178, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x96CB66, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80 //press ok to continue
        //injector::MakeCALL(0x96CBB4, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        injector::MakeCALL(addrTbl[0x97A188], sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80 // skills background
        //injector::MakeCALL(0x97A1A2, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        injector::MakeCALL(addrTbl[0x97A787], sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x97A7A1, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x98B263, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x98B898, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x990887, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ADB0B, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ADD6B, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ADE29, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9AE18F, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9BDF69, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9BF6EF, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9C86F4, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9C94C3, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9CF095, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9CF1F9, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9D9325, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9E8123, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ECFEB, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ED413, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0x9ED9E0, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0xA115FA, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0xA116F0, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0xA1611E, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80
        //injector::MakeCALL(0xA1A84A, sub_974C80_center, true); //0x974C80 + 0x0->call    sub_974C80

        injector::MakeCALL(addrTbl[0x55DCC7], sub_55DB40, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(addrTbl[0x55DCE5], sub_55DB40_stretch, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(addrTbl[0x55DD52], sub_55DB40, true); // 0x55DB40 + 0x0->call    sub_55DB40 // pause background
        injector::MakeCALL(addrTbl[0x55DD8F], sub_55DB40_stretch, true); // 0x55DB40 + 0x0->call    sub_55DB40

        injector::MakeCALL(addrTbl[0x55DE28], sub_55DB40_center, true); // 0x55DB40 + 0x0->call    sub_55DB40 weapon name bg
        injector::MakeCALL(addrTbl[0x55DE46], sub_55DB40, true); // 0x55DB40 + 0x0->call    sub_55DB40

        injector::MakeCALL(addrTbl[0x55DEDC], sub_55DB40, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(addrTbl[0x55DF52], sub_55DB40_stretch, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(addrTbl[0x55DFE8], sub_55DB40, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(addrTbl[0x55E006], sub_55DB40_stretch, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(addrTbl[0x55E09C], sub_55DB40, true); // 0x55DB40 + 0x0->call    sub_55DB40
        injector::MakeCALL(addrTbl[0x55E0C8], sub_55DB40_stretch, true); // 0x55DB40 + 0x0->call    sub_55DB40
    }
    
    //3d blips fix
    {
        injector::MakeCALL(addrTbl[0x9A0EE0], ObjectiveIndicator, true);
        //injector::MakeNOP(addrTbl[0x97D139], 2);
    }

    if (bBorderlessWindowed)
    {
        IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
            std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
            std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
            std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
            std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
            std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
            std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook)
        );
    }

    {
        static float fps = 1000.0f;
        injector::WriteMemory(addrTbl[0x514CDB] + 4, &fps, true);
        injector::WriteMemory(addrTbl[0xD583AB] + 4, &fps, true);
        injector::WriteMemory(addrTbl[0xEE17C5] + 2, &fps, true);
        injector::WriteMemory(addrTbl[0x9DBE9D] + 4, &fps, true);
        injector::WriteMemory(addrTbl[0x9DB8E8] + 4, &fps, true);
        injector::WriteMemory(addrTbl[0x016E62D8], (int)fps, true);
    }

    {
        bLogiLedInitialized = LogiLedInit();

        if (bLogiLedInitialized)
        {
            static auto sPlayerPtr = *hook::get_pattern<void*>("8B 0D ? ? ? ? 85 C9 74 07 6A 01 E8 ? ? ? ? FE 86", 2);

            std::thread t([]()
            {
                while (true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    if (bLogiLedInitialized)
                    {
                        if (sPlayerPtr)
                        {
                            auto LeonHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x24, 0xF10);
                            auto HelenaHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x28, 0xF10);
                            auto ChrisHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x2C, 0xF10);
                            auto PierceHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x30, 0xF10);
                            auto JakeHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x34, 0xF10);
                            auto SherryHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x38, 0xF10);
                            auto AdaHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x3C, 0xF10);
                            auto AgentHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x40, 0xF10);

                            int FineR = 0x84, FineG = 0xDE, FineB = 0xFF;
                            int DangerR = 0x89, DangerG = 0x13, DangerB = 0x1D;

                            if ((LeonHealth && HelenaHealth) || (ChrisHealth && PierceHealth) || (JakeHealth && SherryHealth) || AdaHealth)
                            {
                                int16_t health1 = 0;
                                int16_t health2 = 0;

                                if (LeonHealth && HelenaHealth) {
                                    health1 = *LeonHealth;
                                    health2 = *HelenaHealth;
                                }
                                else if (ChrisHealth && PierceHealth) {
                                    health1 = *ChrisHealth;
                                    health2 = *PierceHealth;

                                    FineR = 0xA4, FineG = 0xB8, FineB = 0x39;
                                    DangerR = 0x89, DangerG = 0x13, DangerB = 0x1D;
                                }
                                else if (JakeHealth && SherryHealth) {
                                    health1 = *JakeHealth;
                                    health2 = *SherryHealth;

                                    FineR = 0xC1, FineG = 0xD5, FineB = 0x42;
                                    DangerR = 0x89, DangerG = 0x13, DangerB = 0x1D;
                                }
                                else
                                {
                                    health1 = *AdaHealth;
                                    health2 = AgentHealth ? *AgentHealth : -1;
                                }

                                auto [FineRP, FineGP, FineBP] = LEDEffects::RGBtoPercent(FineR, FineG, FineB);
                                auto [FineRPDimmed, FineGPDimmed, FineBPDimmed] = LEDEffects::RGBtoPercent(FineR, FineG, FineB, 0.5f);
                                auto [DangerRP, DangerGP, DangerBP] = LEDEffects::RGBtoPercent(DangerR, DangerG, DangerB);
                                auto [DangerRPDimmed, DangerGPDimmed, DangerBPDimmed] = LEDEffects::RGBtoPercent(DangerR, DangerG, DangerB, 0.5f);

                                if (health1 >= 1)
                                {
                                    if (health1 <= 150) {
                                        LEDEffects::SetLightingLeftSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, true, false); //red
                                        LEDEffects::DrawCardiogram(DangerRP, DangerGP, DangerBP, 0, 0, 0); //red
                                    }
                                    else {
                                        LEDEffects::SetLightingLeftSide(FineRPDimmed, FineGPDimmed, FineBPDimmed, true, false);  //green
                                        LEDEffects::DrawCardiogram(FineRP, FineGP, FineBP, 0, 0, 0); //green
                                    }
                                }
                                else
                                {
                                    LEDEffects::SetLightingLeftSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, false, true); //red
                                    LEDEffects::DrawCardiogram(DangerRP, DangerGP, DangerBP, 0, 0, 0, true);
                                }

                                if (health2 >= 1)
                                {
                                    if (health2 <= 150) {
                                        LEDEffects::SetLightingRightSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, true, false); //red
                                        LEDEffects::DrawCardiogramNumpad(DangerRP, DangerGP, DangerBP, 0, 0, 0); //red
                                    }
                                    else {
                                        LEDEffects::SetLightingRightSide(FineRPDimmed, FineGPDimmed, FineBPDimmed, true, false);  //green
                                        LEDEffects::DrawCardiogramNumpad(FineRP, FineGP, FineBP, 0, 0, 0); //green
                                    }                                       
                                }
                                else if (health2 == 0)
                                {
                                    LEDEffects::SetLightingRightSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, false, true); //red
                                    LEDEffects::DrawCardiogramNumpad(DangerRP, DangerGP, DangerBP, 0, 0, 0, true);
                                }
                                else
                                {
                                    if (health1 <= 150)
                                        LEDEffects::SetLightingRightSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, false, false); //red
                                    else
                                        LEDEffects::SetLightingRightSide(FineRPDimmed, FineGPDimmed, FineBPDimmed, false, false);  //green
                                }
                            }
                            else
                            {
                                LogiLedStopEffects();
                                LogiLedSetLighting(31, 25, 70); //logo purple
                            }
                        }
                    }
                    else
                        break;
                }
            });

            t.detach();
        }
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("85 C0 74 CB 8B 0D"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        if (bLogiLedInitialized) {
            LogiLedShutdown();
            bLogiLedInitialized = false;
        }
    }
    return TRUE;
}
