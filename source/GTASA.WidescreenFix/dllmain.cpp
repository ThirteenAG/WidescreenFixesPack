#include "stdafx.h"
#include "GTA\common.h"
#include "GTA\global.h"

float fWideScreenWidthScale, fWideScreenHeightScale;
float fWideScreenWidthProperScale = 1.0f;
float fWideScreenHeightProperScale = 480.0f / 448.0f;
float fFrontendWidth[40], fFrontendHeight[40];
float fMiscWidth[30], fMiscHeight[30];
float fRadarWidth[23], fRadarHeight[23];
float fHUDWidth[120], fHUDHeight[120];
float fCameraWidth[2], fCameraHeight[2];
float fDefaultWidth;
float fDefaultCoords;
float fFrontendDefaultWidth;

int(__cdecl* FindPlayerVehicleSA)(int, char);
int FindPlayerVehicle2()
{
    return FindPlayerVehicleSA(-1, 0);
}

void GetMemoryAddresses()
{
    RsGlobal = (RsGlobalType *)0xC17040;
    CDraw::pfScreenAspectRatio = (float*)0xC3EFA4;
    CDraw::pfScreenFieldOfView = (float*)0x8D5038;
    CSprite2dDrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x727B60;
    CSprite2dDrawRect2 = (int(__cdecl *)(CRect const &, CRGBA const &, CRGBA const &, CRGBA const &, CRGBA const &)) 0x727C10;
    bWideScreen = (bool*)0xBA6793; BordersVar1 = (uint32_t*)0xB6F0B8; BordersVar2 = (uint32_t*)0xB6F0CC;
    FindPlayerVehicleSA = (int(__cdecl *)(int playerNum, char a2)) 0x56E0D0;
    FindPlayerVehicle = &FindPlayerVehicle2;
    bIsInCutscene = (bool*)0xB6F065;
}

void OverwriteResolution()
{
    // Unlocked widescreen resolutions by Silent
    injector::WriteMemory(0x745B71, 0x9090687D, true);
    injector::WriteMemory(0x74596C, 0x9090127D, true);
    injector::MakeNOP(0x745970, 2, true);
    injector::MakeNOP(0x745B75, 2, true);
    injector::MakeNOP(0x7459E1, 2, true);

    if (!ResX || !ResY)
        std::tie(ResX, ResY) = GetDesktopRes();
    else if (ResX == -1 || ResY == -1)
        return;

    injector::WriteMemory(0x746362 + 0x1, ResX, true);
    injector::WriteMemory(0x746367 + 0x1, ResY, true);
    injector::WriteMemory(0x74636C + 0x1, 32, true);
    injector::WriteMemory<unsigned short>(0x7462AB, 0xB2E9, true); //jz      loc_746362 > jmp      loc_746362
    injector::WriteMemory(0x7462AB + 2, 0x00, true);
    injector::WriteMemory<unsigned short>(0x7462B3, 0xAAE9, true);
    injector::WriteMemory(0x7462B3 + 2, 0x00, true);
}

DWORD _EAX;
void __declspec(naked) AllowMouseMovement()
{
    _asm
    {
        mov _EAX, eax
        mov eax, dword ptr ds : [0x8D621C]
        cmp eax, 0
        jne label1
        mov eax, _EAX
        ret

        label1 :
        mov eax, _EAX
            mov _EAX, 0x7453F0
            jmp _EAX
    }
}

void UpdateFrontendFixes() {
    fFrontendWidth[0] = 0.0020312499f * fWideScreenWidthScale;
    fFrontendWidth[1] = 0.00065625005f * fWideScreenWidthScale;
    fFrontendWidth[2] = 0.00109375f * fWideScreenWidthScale;
    fFrontendWidth[3] = 0.00109375f * fWideScreenWidthScale;
    fFrontendWidth[4] = 0.00087500003f * fWideScreenWidthScale;
    fFrontendWidth[5] = 0.000546875f * fWideScreenWidthScale;
    fFrontendWidth[6] = 0.00076562498f * fWideScreenWidthScale;
    fFrontendWidth[7] = 0.00076562498f * fWideScreenWidthScale;
    fFrontendWidth[8] = 0.00046875002f * fWideScreenWidthScale;
    fFrontendWidth[9] = 0.00093750004f * fWideScreenWidthScale;
    fFrontendWidth[10] = 0.0625f;
    fFrontendWidth[11] = 0.09375f;
    fFrontendWidth[12] = 0.0015625f;
    fFrontendWidth[13] = 0.0625f;
    fFrontendWidth[14] = 0.0015625f * fWideScreenWidthScale;
    fFrontendWidth[15] = 0.0015625f * fWideScreenWidthScale;
    fFrontendWidth[16] = 0.0015625f * fWideScreenWidthScale;
    fFrontendWidth[17] = 0.0015625f * fWideScreenWidthScale;
    fFrontendWidth[18] = 0.0015625f * fWideScreenWidthScale;
    fFrontendWidth[19] = 0.0015625f * fWideScreenWidthScale;
    fFrontendWidth[20] = 0.0015625f * fWideScreenWidthScale;
    fFrontendWidth[21] = 0.00046875002f * fWideScreenWidthScale;
    fFrontendWidth[22] = 0.78125f * fWideScreenWidthScale;
    fFrontendWidth[23] = 0.78125f * fWideScreenWidthScale;
    fFrontendWidth[24] = 0.78125f * fWideScreenWidthScale;
    fFrontendWidth[25] = 0.78125f * fWideScreenWidthScale;
    fFrontendWidth[26] = 0.78125f * fWideScreenWidthScale;
    fFrontendWidth[27] = 0.78125f * fWideScreenWidthScale;
    fFrontendWidth[28] = 0.0046875002f * fWideScreenWidthScale;
    fFrontendWidth[29] = 0.15625f * fWideScreenWidthScale;
    fFrontendWidth[30] = 0.0046875002f * fWideScreenWidthScale;
    fFrontendWidth[31] = 0.15625f * fWideScreenWidthScale;
    fFrontendWidth[32] = 0.0046875002f * fWideScreenWidthScale;
    fFrontendWidth[33] = 0.15625f * fWideScreenWidthScale;
    fFrontendWidth[34] = 0.0046875002f * fWideScreenWidthScale;
    fFrontendWidth[35] = 0.15625f * fWideScreenWidthScale;
    fFrontendWidth[36] = 0.0046875002f * fWideScreenWidthScale;
    fFrontendWidth[37] = 0.15625f * fWideScreenWidthScale;
    fFrontendWidth[38] = 0.0015625f * fWideScreenWidthScale;
    fFrontendWidth[39] = 0.00087500003f * fWideScreenWidthScale;

    fFrontendHeight[0] = 0.0046874997f * fWideScreenHeightScale;
    fFrontendHeight[1] = 0.0021205356f * fWideScreenHeightScale;
    fFrontendHeight[2] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[3] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[4] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[5] = 0.0021205356f * fWideScreenHeightScale;
    fFrontendHeight[6] = 0.0026785715f * fWideScreenHeightScale;
    fFrontendHeight[7] = 0.0015625f * fWideScreenHeightScale;
    fFrontendHeight[8] = 0.0016741072f * fWideScreenHeightScale;
    fFrontendHeight[9] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[10] = 0.0625f * fWideScreenHeightScale;
    fFrontendHeight[11] = 0.21651785f * fWideScreenHeightScale;
    fFrontendHeight[12] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[13] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[14] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[15] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[16] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[17] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[18] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[19] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[20] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[21] = 0.0012276786f * fWideScreenHeightScale;
    fFrontendHeight[22] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[23] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[24] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[25] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[26] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[27] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[28] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[29] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[30] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[31] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[32] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[33] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[34] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[35] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[36] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[37] = 0.0017857143f * fWideScreenHeightScale;
    fFrontendHeight[38] = 0.002232143f * fWideScreenHeightScale;
    fFrontendHeight[39] = 0.002232143f * fWideScreenHeightScale;
}

void UpdateMiscFixes() {
    // Aim point.
    fCameraWidth[0] = 0.01403292f;
    fCameraHeight[0] = 0.0f;

    fMiscWidth[0] = 0.0015625f * fWideScreenWidthScale; // StretchX
    fMiscWidth[1] = 0.0546875f * fWideScreenWidthScale;
    fMiscWidth[2] = 0.0015625f;
    fMiscWidth[3] = 0.09375f * fWideScreenWidthScale;
    fMiscWidth[4] = 0.0015625f;
    fMiscWidth[5] = 0.078125f * fWideScreenWidthScale;
    fMiscWidth[6] = 0.0703125f * fWideScreenWidthScale;
    fMiscWidth[7] = 0.0859375f * fWideScreenWidthScale;
    fMiscWidth[8] = 0.078125f * fWideScreenWidthScale;
    fMiscWidth[9] = 0.078125f * fWideScreenWidthScale;
    fMiscWidth[10] = 0.0703125f * fWideScreenWidthScale;
    fMiscWidth[11] = 0.0859375f * fWideScreenWidthScale;
    fMiscWidth[12] = 0.078125f * fWideScreenWidthScale;
    fMiscWidth[13] = 0.625f;
    fMiscWidth[14] = 0.0015625f * fWideScreenWidthScale;
    fMiscWidth[15] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[16] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[17] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[18] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[19] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[20] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[21] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[22] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[23] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[24] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[25] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[26] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[27] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fMiscWidth[28] = 0.0015625f * fWideScreenWidthScale;

    fMiscHeight[0] = 0.002232143f * fWideScreenHeightScale; // StretchY
    fMiscHeight[1] = 0.078125f * fWideScreenHeightScale;
    fMiscHeight[2] = 0.66964287f * fWideScreenHeightScale;
    fMiscHeight[3] = 0.13392857f * fWideScreenHeightScale;
    fMiscHeight[4] = 0.6473214f * fWideScreenHeightScale;
    fMiscHeight[5] = 0.22321428f * fWideScreenHeightScale;
    fMiscHeight[6] = 0.24553572f * fWideScreenHeightScale;
    fMiscHeight[7] = 0.24553572f * fWideScreenHeightScale;
    fMiscHeight[8] = 0.22321428f * fWideScreenHeightScale;
    fMiscHeight[9] = 0.77678573f * fWideScreenHeightScale;
    fMiscHeight[10] = 0.75446427f * fWideScreenHeightScale;
    fMiscHeight[11] = 0.75446427f * fWideScreenHeightScale;
    fMiscHeight[12] = 0.77678573f * fWideScreenHeightScale;
    fMiscHeight[13] = 0.037946429f * fWideScreenHeightScale;
    fMiscHeight[14] = 0.002232143f * fWideScreenHeightScale;
    fMiscHeight[15] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[16] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[17] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[18] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[19] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[20] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[21] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[22] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[23] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[24] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[25] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[26] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[27] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fMiscHeight[28] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
}

void UpdateHUDFixes() {
    fRadarWidth[0] = 0.0015625f * fWideScreenWidthScale * fRadarWidthScale;
    fRadarWidth[1] = 0.0015625f * fWideScreenWidthScale * fRadarWidthScale;
    fRadarWidth[2] = 0.0015625f * fWideScreenWidthScale * fRadarWidthScale;
    fRadarWidth[3] = 0.0015625f * fWideScreenWidthScale * fRadarWidthScale;
    fRadarWidth[4] = 0.0015625f * fWideScreenWidthScale * fRadarWidthScale;
    fRadarWidth[5] = 0.0015625f * fWideScreenWidthScale * fRadarWidthScale;
    fRadarWidth[6] = 0.0015625f * fWideScreenWidthScale * fRadarWidthScale;
    fRadarWidth[7] = 0.0015625f * fWideScreenWidthScale * fRadarWidthScale;
    fRadarWidth[8] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[9] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[10] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[11] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[12] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[13] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[14] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[15] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[16] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[17] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[18] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[19] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[20] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[21] = 0.0015625f * fWideScreenWidthScale;
    fRadarWidth[22] = 0.0015625f * fWideScreenWidthScale;

    fRadarHeight[0] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[1] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[2] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[3] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[4] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[5] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[6] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[7] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[8] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[9] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[10] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[11] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[12] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[13] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[14] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[15] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[16] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[17] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[18] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[19] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[20] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[21] = 0.002232143f * fWideScreenHeightScale;
    fRadarHeight[22] = 0.002232143f * fWideScreenHeightScale;

    fHUDWidth[0] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[1] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[2] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[3] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[4] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[5] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[6] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[7] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[8] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[9] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[10] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[11] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[12] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[13] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[14] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[15] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[16] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[17] = 0.17343046f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[18] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[19] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[20] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[21] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[22] = 0.17343046f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[23] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[24] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[25] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[26] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[27] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[28] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[29] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[30] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[31] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[32] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[33] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[34] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[35] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale * fSubtitlesScale;
    fHUDWidth[36] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale * fSubtitlesScale;
    fHUDWidth[37] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale * fSubtitlesScale;
    fHUDWidth[38] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[39] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[40] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[41] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[42] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[43] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[44] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[45] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[46] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[47] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[48] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[49] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[50] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[51] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[52] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[53] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[54] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[55] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[56] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[57] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[58] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[59] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[60] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[61] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[62] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[63] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[64] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[65] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[66] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[67] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[68] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[69] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[70] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[71] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[72] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[73] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[74] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[75] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[76] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[77] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[78] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[79] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[80] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[81] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[82] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[83] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[84] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[85] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[86] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[87] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[88] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[89] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[90] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[91] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[92] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[93] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[94] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[95] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[96] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[97] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[98] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[99] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[100] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[101] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[102] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[103] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[104] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;
    fHUDWidth[105] = 0.0015625f * fWideScreenWidthScale * fHudWidthScale;


    fHUDWidth[110] = RsGlobal->MaximumWidth * 0.17343046f * fWideScreenWidthScale * fHudWidthScale;

    fHUDHeight[0] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[1] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[2] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[3] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[4] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[5] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[6] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[7] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[8] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[9] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[10] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[11] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[12] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[13] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[14] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[15] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[16] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[17] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[18] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[19] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[20] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[21] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[22] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[23] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[24] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[25] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[26] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[27] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[28] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[29] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[30] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[31] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[32] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[33] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[34] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[35] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale * fSubtitlesScale;
    fHUDHeight[36] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale * fSubtitlesScale;
    fHUDHeight[37] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale * fSubtitlesScale;
    fHUDHeight[38] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[39] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[40] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[41] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[42] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[43] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[44] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[45] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[46] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[47] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[48] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[49] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[50] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[51] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[52] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[53] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[54] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[55] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[56] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[57] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[58] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[59] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[60] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[61] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[62] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[63] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[64] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[65] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[66] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[67] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[68] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[69] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[70] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[71] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[72] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[73] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[74] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[75] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[76] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[77] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[78] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[79] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[80] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[81] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[82] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[83] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[84] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[85] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[86] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[87] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[88] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[89] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[90] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[91] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[92] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[93] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[94] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[95] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[96] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[97] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[98] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[99] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[100] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[101] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[102] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[103] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[104] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
    fHUDHeight[105] = 0.002232143f * fWideScreenHeightScale * fHudHeightScale;
}

void UpdateScriptFixes() {
    float w;
    if (*CDraw::pfScreenAspectRatio < fDefaultWidth)
        w = static_cast<float>(RsGlobal->MaximumWidth);
    else
        w = RsGlobal->MaximumWidth * fDefaultWidth / *CDraw::pfScreenAspectRatio;

    fDefaultCoords = 0.5f * (RsGlobal->MaximumWidth - w);
}

template<uintptr_t addr>
void updateScreenAspectRatioWrapper()
{
    using func_hook = injector::function_hooker<addr, void()>;
    injector::make_static_hook<func_hook>([](func_hook::func_type updateScreenAspectRatio) {
        updateScreenAspectRatio();

        fWideScreenWidthScale = 640.0f / (*CDraw::pfScreenAspectRatio * 448.0f);
        fWideScreenHeightScale = 448.0 / 448.0f;
        fWideScreenWidthProperScale = static_cast<float>(RsGlobal->MaximumWidth) / (*CDraw::pfScreenAspectRatio * 448.0f);
        fWideScreenHeightProperScale = static_cast<float>(RsGlobal->MaximumHeight) / 448.0f;
        fDefaultWidth = (4.0f / 3.0f);

        UpdateFrontendFixes();
        UpdateMiscFixes();
        UpdateHUDFixes();
        UpdateScriptFixes();
        });
}

uintptr_t SetEdgeAddr = 0x719590;
void __declspec(naked) SetDropShadowPosition() {
    _asm {
        mov eax, [esp + 4]
        cmp ReplaceTextShadowWithOutline, 2
        jae label1
        test eax, eax
        jz label1
        dec eax
        mov[esp + 4], eax

        label1 :
        jmp SetEdgeAddr
    }
}

constexpr char* main_scm_tex[] = { "10ls", "10ls2", "10ls3", "10ls4", "10ls5", "10og", "10weed", "11dice", "11dice2", "11ggift", "11grov2", "11grov3", "11grove",
    "11jail", "12angel", "12bndit", "12cross", "12dager", "12maybr", "12myfac", "4rip", "4spider", "4weed", "5cross", "5cross2", "5cross3", "5gun", "6africa", "6aztec",
    "6clown", "6crown", "7cross", "7cross2", "7cross3", "7mary", "8gun", "8poker", "8sa", "8sa2", "8sa3", "8santos", "8westsd", "9bullt", "9crown", "9gun", "9gun2", "9homby",
    "9rasta", "addcoin", "AirLogo", "arrow", "back2", "back3", "back4", "back5", "back6", "back7", "back8", "back8_right", "back8_top", "backbet", "backcyan", "backgnd",
    "backred", "badchat", "ball", "bar1_o", "bar2_o", "bckgrnd", "bee1", "bee2", "beea", "bell", "black", "blkdot", "blue", "brboat", "brfly", "bride1", "bride2", "bride3",
    "bride4", "bride5", "bride6", "bride7", "bride8", "bronze", "bstars", "bum1", "bum2", "bumble", "bushes", "butnA", "butnAo", "butnB", "butnBo", "butnC", "cbarl", "cbarm",
    "cbarr", "cd10c", "cd10d", "cd10h", "cd10s", "cd11c", "cd11d", "cd11h", "cd11s", "cd12c", "cd12d", "cd12h", "cd12s", "cd13c", "cd13d", "cd13h", "cd13s", "cd1c", "cd1d",
    "cd1h", "cd1s", "cd2c", "cd2d", "cd2h", "cd2s", "cd3c", "cd3d", "cd3h", "cd3s", "cd4c", "cd4d", "cd4h", "cd4s", "cd5c", "cd5d", "cd5h", "cd5s", "cd6c", "cd6d", "cd6h",
    "cd6s", "cd7c", "cd7d", "cd7h", "cd7s", "cd8c", "cd8d", "cd8h", "cd8s", "cd9c", "cd9d", "cd9h", "cd9s", "cdback", "cherry", "chit", "circle", "cring", "cross", "crosshair",
    "dark", "deal", "down", "downl", "downr", "dpad_64", "dpad_lr", "DUALITY", "eax", "ex1", "ex2", "ex3", "ex4", "exitw", "exity", "explm01", "explm02", "explm03", "explm04",
    "explm05", "explm06", "explm07", "explm08", "explm09", "explm10", "explm11", "explm12", "fen", "fire", "flwr", "flwra", "force", "fstar", "ghost", "goboat", "gold", "golfly",
    "goodcha", "grapes", "gride1", "gride2", "gride3", "gride4", "gride5", "gride6", "gride7", "gride8", "hbarl", "hbarm", "hbarr", "Health", "hiscorew", "hiscorey", "hive",
    "hi_a", "hi_b", "hi_c", "holdmid", "holdoff", "holdon", "hon", "hrs1", "hrs2", "hrs3", "hrs4", "hrs5", "hrs6", "hrs7", "hrs8", "intro1", "intro2", "intro3", "intro4", "kami",
    "layer", "leaf", "left", "light", "load0uk", "loadsc0", "loadsc1", "loadsc10", "loadsc11", "loadsc12", "loadsc13", "loadsc14", "loadsc2", "loadsc3", "loadsc4", "loadsc5",
    "loadsc6", "loadsc7", "loadsc8", "loadsc9", "loadscuk", "map", "mouse", "naward", "nawtxt", "nib", "nmef", "nvidia", "outro", "pa", "playw", "playy", "pm2", "pm3", "pole2",
    "power", "pride1", "pride2", "pride3", "pride4", "pride5", "pride6", "pride7", "pride8", "ps1", "ps2", "ps3", "race00", "race01", "race02", "race03", "race04", "race05", "race06",
    "race07", "race08", "race09", "race10", "race11", "race12", "race13", "race14", "race15", "race16", "race17", "race18", "race19", "race20", "race21", "race22", "race23", "race24",
    "radio_bounce", "radio_csr", "radio_KDST", "radio_kjah", "radio_krose", "radio_mastersounds", "radio_playback", "radio_RADIOX", "radio_RLS", "radio_SFUR", "radio_TPLAYER",
    "radio_WCTR", "ribb", "ribbw", "Ric1", "Ric2", "Ric3", "Ric4", "Ric5", "right", "rockshp", "roulbla", "roulgre", "roulred", "rride1", "rride2", "rride3", "rride4", "rride5",
    "rride6", "rride7", "rride8", "r_69", "ship", "ship2", "ship3", "shoot", "shpnorm", "shpwarp", "silboat", "silfly", "silver", "sky", "splash1", "splash2", "splsh", "square",
    "thorn", "thrust", "thrustG", "thumbdn", "thumbup", "timer", "title", "title_pc_EU", "title_pc_US", "trees", "triang", "tvbase", "tvcorn", "tvl", "tvr", "ufo", "un_a", "un_b",
    "un_c", "up", "upl", "upr", "warp", "white", "yride1", "yride2", "yride3", "yride4", "yride5", "yride6", "yride7", "yride8" };

injector::hook_back<void(__cdecl*)(CRect const&, CRGBA const&)> hbDrawRect;
void __cdecl DrawRectHook(CRect const& rect, CRGBA  const& color)
{
    hbDrawRect.fun(CRect(fDefaultCoords + rect.m_fLeft * fDefaultWidth / *CDraw::pfScreenAspectRatio, rect.m_fBottom, fDefaultCoords + rect.m_fRight * fDefaultWidth / *CDraw::pfScreenAspectRatio, rect.m_fTop), color);
}

injector::hook_back<void(__fastcall*)(CSprite2d const&, int, CRect const&, CRGBA const&)> hbDraw;
void __fastcall DrawSpriteHook(CSprite2d const& sprite, int, CRect const& rect, CRGBA const& color)
{
    auto pTexture = *(uintptr_t*)&sprite;

    if (pTexture && std::find(std::begin(main_scm_tex), std::end(main_scm_tex), std::string_view((const char*)(pTexture + 0x10))) != std::end(main_scm_tex))
    {
        hbDraw.fun(sprite, -1, CRect(fDefaultCoords + rect.m_fLeft * fDefaultWidth / *CDraw::pfScreenAspectRatio, rect.m_fBottom, fDefaultCoords + rect.m_fRight * fDefaultWidth / *CDraw::pfScreenAspectRatio, rect.m_fTop), color);

        if (rect.m_fLeft <= 0.0f && rect.m_fRight >= (float)RsGlobal->MaximumWidth * 320.0f * 0.0015625) {
            CSprite2dDrawRect(CRect(0.0f, (float)RsGlobal->MaximumHeight, fDefaultCoords, 0.0f), CRGBA(0, 0, 0, 255));

            CSprite2dDrawRect(CRect(fDefaultCoords + (float)RsGlobal->MaximumWidth * fDefaultWidth / *CDraw::pfScreenAspectRatio, (float)RsGlobal->MaximumHeight, (float)RsGlobal->MaximumWidth, 0.0f), CRGBA(0, 0, 0, 255));
        }
    }
    else
    {
        hbDraw.fun(sprite, -1, rect, color);
    }
}

injector::hook_back<void(__fastcall*)(CSprite2d const&, int, float, float, float, float, float, float, float, float, CRGBA const&)> hbDraw2;
void __fastcall DrawSpriteHook2(CSprite2d const& sprite, int, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, CRGBA const& color)
{
    hbDraw2.fun(sprite, 0, fDefaultCoords + x1 * fDefaultWidth / *CDraw::pfScreenAspectRatio, y1, fDefaultCoords + x2 * fDefaultWidth / *CDraw::pfScreenAspectRatio, y2,
        fDefaultCoords + x3 * fDefaultWidth / *CDraw::pfScreenAspectRatio, y3, fDefaultCoords + x4 * fDefaultWidth / *CDraw::pfScreenAspectRatio, y4, color);
}

injector::hook_back<void(__cdecl*)(float, float, const char*)> hbPrintString;
void __cdecl PrintStringHook(float x, float y, char* str)
{
    hbPrintString.fun(fDefaultCoords + x * fDefaultWidth / *CDraw::pfScreenAspectRatio, y, str);
}

injector::hook_back<void(__cdecl*)(float, float)> hbSetScale;
void __cdecl SetScaleHook(float w, float h)
{
    hbSetScale.fun(w * fDefaultWidth / *CDraw::pfScreenAspectRatio, h);
}

injector::hook_back<void(__cdecl*)(float)> hbSetWrapx;
void __cdecl SetWrapxHook(float fWrap)
{
    hbSetWrapx.fun(fDefaultCoords + fWrap * fDefaultWidth / *CDraw::pfScreenAspectRatio);
}

injector::hook_back<void(__stdcall*)(const CRect&, const char*, unsigned char, CRGBA, bool, bool)> hbDrawWindow;
void __stdcall DrawWindowHook(CRect *rect, char *titleKey, char fadeState, CRGBA color, int a5, char bDrawBox)
{
    hbDrawWindow.fun(CRect(fDefaultCoords + rect->m_fLeft * fDefaultWidth / *CDraw::pfScreenAspectRatio, rect->m_fBottom, fDefaultCoords + rect->m_fRight * fDefaultWidth / *CDraw::pfScreenAspectRatio, rect->m_fTop), titleKey, fadeState, color, a5, bDrawBox);
}

void InstallSCMDrawingFixes() {
    hbDrawRect.fun = injector::MakeCALL(0x464A53, DrawRectHook).get();
    injector::MakeCALL(0x464A53, DrawRectHook);

    hbDraw.fun = injector::MakeCALL(0x464A90, DrawSpriteHook).get();
    injector::MakeCALL(0x464A90, DrawSpriteHook);

    hbDraw2.fun = injector::MakeCALL(0x464B7F, DrawSpriteHook2).get();
    injector::MakeCALL(0x464B7F, DrawSpriteHook2);

    hbPrintString.fun = injector::MakeCALL(0x58C229, PrintStringHook).get();
    injector::MakeCALL(0x58C229, PrintStringHook);

    hbSetScale.fun = injector::MakeCALL(0x58C0E8, SetScaleHook).get();
    injector::MakeCALL(0x58C0E8, SetScaleHook);

    hbSetWrapx.fun = injector::MakeCALL(0x58C137, SetWrapxHook).get();
    injector::MakeCALL(0x58C137, SetWrapxHook);

    hbDrawWindow.fun = injector::MakeCALL(0x464A24, DrawWindowHook).get();
    injector::MakeCALL(0x464A24, DrawWindowHook);
}

void InstallAspectRatioFixes()
{
    // Unlock resolutions
    injector::MakeJMP(0x745B5B, (void*)0x745BCB, true);
    injector::WriteMemory<BYTE>(0x745BD1, 0x7D, true);
    injector::WriteMemory<BYTE>(0x745BD9, 0x7C, true);

    // Proportional coronas
    injector::MakeNOP(0x6FB2C9, 4);
    injector::WriteMemory<BYTE>(0x6FB2BD, 0x6C);
    injector::WriteMemory<BYTE>(0x6FB2DC, 0x78);
    injector::WriteMemory<BYTE>(0x713BE5, 0x20);
    injector::WriteMemory<BYTE>(0x713B6D, 0x38);
    injector::WriteMemory<BYTE>(0x713CFB, 0x38);
    injector::WriteMemory<BYTE>(0x713EFC, 0x30);
    injector::WriteMemory<BYTE>(0x714004, 0x38);

    injector::MakeJMP(0x6FF420, CDraw::CalculateAspectRatio, true);

    updateScreenAspectRatioWrapper<(0x0053D695 - 0x1)>();
    updateScreenAspectRatioWrapper<(0x0053D7B2 - 0x1)>();
    updateScreenAspectRatioWrapper<(0x0053D967 - 0x1)>();
    updateScreenAspectRatioWrapper<(0x0053E771 - 0x1)>();
    updateScreenAspectRatioWrapper<(0x0053EB1A - 0x1)>();
}

void __declspec(naked) CalculateAimingPoint()
{
    _asm {
        fstp	st
        mov		edx, [CDraw::pfScreenAspectRatio]
        fmul[edx]
        mov		edi, [esp + 1Ch + 14h]
        mov     edx, edi
        mov		ebx, 5149A6h
        jmp		ebx
    }
}

void InstallFieldOfViewFixes() {
    injector::MakeJMP(0x6FF410, CDraw::SetFOV, true);

    // Fix sky multitude
    static const float fSkyMultFix = 10.0f;
    injector::WriteMemory<const void*>(0x714843, &fSkyMultFix, true);
    injector::WriteMemory<const void*>(0x714860, &fSkyMultFix, true);

    // Set vehicle max FOV.
    static const float fVehMaxFov = *CDraw::pfScreenFieldOfView + 30.0f;
    injector::WriteMemory<const void*>(0x524BB4, &fVehMaxFov, true);
    injector::WriteMemory<float>(0x524BC5, *CDraw::pfScreenFieldOfView + 30.0f, true);

    // Aiming point
    injector::MakeJMP(0x51499E, CalculateAimingPoint, true);
    injector::MakeNOP(0x50AD79, 6, true);
    injector::WriteMemory<const void*>(0x50AD59 + 0x2, &fCameraWidth[0]);
    injector::WriteMemory<const void*>(0x51498D + 0x2, &fCameraWidth[0]);
}

float __stdcall StretchXHook(float fValue)
{
    if (RsGlobal->MaximumWidth == 640)
        return fValue;
    else
        return (float)RsGlobal->MaximumWidth * fValue * 0.0015625f;
}

float __stdcall StretchYHook(float fValue)
{
    if (RsGlobal->MaximumHeight == 448)
        return fValue;
    else
        return (float)RsGlobal->MaximumHeight * fValue * 0.002232143f;
}

injector::hook_back<void(__stdcall*)(float, float, float, float, float, float, signed int)> hbDisplaySlider;
void __stdcall DisplaySliderHook(float x, float y, float unk0, float unk1, float width, float progress, signed int unk)
{
    hbDisplaySlider.fun(fWideScreenWidthProperScale * (-100.0f) + (float)RsGlobal->MaximumWidth - (float)RsGlobal->MaximumWidth * 0.0625f, y, unk0, unk1, width, progress, unk);
}

injector::hook_back<void(__cdecl*)(float, float, unsigned short, unsigned int, float, int, bool, bool, CRGBA const& color, CRGBA const&)> hbDrawBarChart;
void __cdecl DrawBarChartHook(float posX, float posY, unsigned short width, unsigned int height, float progress, int progressAdded, bool drawPercentage, bool drawBlackBorder, CRGBA const& color, CRGBA const& progressAddedColor)
{
    hbDrawBarChart.fun(fWideScreenWidthProperScale * (-50.0f) + (float)RsGlobal->MaximumWidth * 0.703125f, posY, width, height, progress, progressAdded, drawPercentage, drawBlackBorder, color, progressAddedColor);
}

void InstallFrontendFixes()
{
    // Font Scales
    int m_dwFrontendWidth[] = { 0x5795CF, // 0 Menu text
                                    0x579981, // 1 Menu text
                                    0x579A01, // 2 Menu text
                                    0x57A36E, // 3 Menu text
                                    0x57A319, // 4 Menu text
                                    0x57A2A0, // 5 Menu text	
                                    0x579718, // 6 Menu text
                                    0x5763C0, // 7 Menu text
                                    0x5749A5, // 8 Menu text
                                    0x57500F, // 9 Menu text
                                    0x579665, // 10 Menu text
                                    0x579863, // 11 Menu text
                                    0x57A1E3, // 12 Menu text
                                    0x57A3EB, // 13 Menu text
                                    0x582DDB, // 14 Legend
                                    0x582E0D, // 15 Legend
                                    0x582EB0, // 16 Legend
                                    0x583019, // 17 Legend
                                    0x58309C, // 18 Legend
                                    0x583128, // 19 Legend
                                    0x5831AD, // 20 Legend	
                                    0x57613B, // 21 Legend
                                    0x575EDF, // 22 Map zones
                                    0x57A88D, // 23 Brightness slider posn
                                    0x57AA81, // 24 Radio slider posn
                                    0x57ACBE, // 25 Sfx slider posn
                                    0x57AEB0, // 26 Draw slider posn
                                    0x57B0E4, // 27 Mouse slider posn
                                    0x57A807, // 28 Brightness slider
                                    0x57A811, // 29 Brightness slider
                                    0x57A9FE, // 30 Radio slider
                                    0x57AA08, // 31 Radio slider
                                    0x57AC2F, // 32 Sfx slider
                                    0x57AC39, // 33 Sfx slider
                                    0x57AE36, // 34 Draw distance slider
                                    0x57AE40, // 35 Draw distance slider
                                    0x57B064, // 36 Mouse acc slider
                                    0x57B06E, // 37 Mouse acc slider
                                    0x584A12, // 38 DrawYouAreHereSprite
                                    0x5740BC, // 39 Message screen
    };

    int m_dwFrontendHeight[] = { 0x5795AD, // 0 Menu text
                                    0x579958, // 1 Menu text								
                                    0x5799D5, // 2 Menu text
                                    0x57A345, // 3 Menu text
                                    0x57A2DB, // 4 Menu text
                                    0x57A25F, // 5 Menu text
                                    0x5796F6, // 6 Menu text
                                    0x576398, // 7 Menu text
                                    0x57497D, // 8 Menu text
                                    0x574FED, // 9 Menu text
                                    0x579643, // 10 Menu text
                                    0x579841, // 11 Menu text
                                    0x57A1AE, // 12 Menu text
                                    0x57A3C5, // 13 Menu text
                                    0x582DC1, // 14 Legend
                                    0x582ED3, // 15 Legend
                                    0x582FA3, // 16 Legend
                                    0x582FF5, // 17 Legend
                                    0x583079, // 18 Legend
                                    0x5830F6, // 19 Legend
                                    0x58317B, // 20 Legend
                                    0x576119, // 21 Legend
                                    0x575EB7, // 22 Map zones
                                    NULL,	  // 23
                                    NULL,	  // 24
                                    NULL,	  // 25
                                    NULL,	  // 26
                                    NULL,	  // 27
                                    NULL,	  // 28
                                    NULL,	  // 29
                                    NULL,	  // 30
                                    NULL,	  // 31
                                    NULL,	  // 32
                                    NULL,	  // 33
                                    NULL,	  // 34
                                    NULL,	  // 35
                                    NULL,	  // 36
                                    NULL,	  // 37
                                    0x5849FA, // 38 DrawYouAreHereSprite
                                    0x574096, // 39 Message screen
    };

    for (int i = 0; i < sizeof(m_dwFrontendWidth) / sizeof(const void*); i++)
    {
        if (m_dwFrontendWidth[i] != NULL)
            injector::WriteMemory<const void*>(m_dwFrontendWidth[i] + 0x2, &fFrontendWidth[i], true);
    }

    for (int i = 0; i < sizeof(m_dwFrontendHeight) / sizeof(const void*); i++)
    {
        if (m_dwFrontendHeight[i] != NULL)
            injector::WriteMemory<const void*>(m_dwFrontendHeight[i] + 0x2, &fFrontendHeight[i], true);
    }

    // StretchXY Restoration
    int m_dwCallWidth[] = { 0x57E3A5,
                                0x577CB7,
                                0x577CD0,
                                0x577D6F,
                                0x577D99,
                                0x577DAB,
                                0x577FA6,
                                0x578031,
                                0x578052,
                                0x5780E1,
                                0x5788DF,
                                0x5788FB,
    };

    int m_dwCallHeight[] = { 0x57E391,
                                0x577C7B,
                                0x577C94,
                                0x577DEC,
                                0x577F6F,
                                0x577F81,
                                0x578106,
                                0x578199,
                                0x5781BA,
                                0x57824D,
                                0x5788ED,


    };

    for (int i = 0; i < sizeof(m_dwCallWidth) / sizeof(const void*); i++)
        injector::MakeCALL(m_dwCallWidth[i], StretchXHook, true);

    for (int i = 0; i < sizeof(m_dwCallHeight) / sizeof(const void*); i++)
        injector::MakeCALL(m_dwCallHeight[i], StretchYHook, true);

    // Fix sliders
    hbDisplaySlider.fun = injector::MakeCALL(0x57A8D1, DisplaySliderHook).get();
    injector::MakeCALL(0x57A8D1, DisplaySliderHook);

    hbDisplaySlider.fun = injector::MakeCALL(0x57AACE, DisplaySliderHook).get();
    injector::MakeCALL(0x57AACE, DisplaySliderHook);

    hbDisplaySlider.fun = injector::MakeCALL(0x57AD0B, DisplaySliderHook).get();
    injector::MakeCALL(0x57AD0B, DisplaySliderHook);

    hbDisplaySlider.fun = injector::MakeCALL(0x57AEEE, DisplaySliderHook).get();
    injector::MakeCALL(0x57AEEE, DisplaySliderHook);

    hbDisplaySlider.fun = injector::MakeCALL(0x57B122, DisplaySliderHook).get();
    injector::MakeCALL(0x57B122, DisplaySliderHook);

    // Fix Stats bar chart.
    hbDrawBarChart.fun = injector::MakeCALL(0x574F54, DrawBarChartHook).get();
    injector::MakeCALL(0x574F54, DrawBarChartHook);
}

void InstallMiscFixes()
{
    // Misc
    int m_dwMiscWidth[] = { 0x5733FD, // 0 StretchX
                                0x574761, // 1 Radio Icons
                                0x5747A6, // 2 Radio Icons
                                0x574857, // 3 Radio Icons
                                0x5748AA, // 4 Radio Icons
                                0x5765C0, // 5 2d Brief triangles
                                0x576603, // 6 2d Brief triangles
                                0x576646, // 7 2d Brief triangles
                                0x576689, // 8 2d Brief triangles
                                0x57672F, // 9 2d Brief triangles
                                0x576772, // 10 2d Brief triangles
                                0x5767B5, // 11 2d Brief triangles
                                0x5767F8, // 12 2d Brief triangles
                                0x574ECC, // 13 Stats bars
                                0x574F30, // 14 Stats bars
                                0x719D2D, // 15 Font fixes
                                0x719D94, // 16 Font fixes
                                0x719DD1, // 17 Font fixes
                                0x719E0E, // 18 Font fixes
                                0x719E4B, // 19 Font fixes
                                0x719E6F, // 20 Font fixes
                                0x719E97, // 21 Font fixes
                                0x719C0D, // 22 Font fixes
                                0x719C6E, // 23 Font fixes
                                0x7288F5, // 24 BarChart fixes
                                0x728941, // 25 BarChart fixes
                                0x573F93, // 26 DrawWindow header
                                0x573FF0, // 27 DrawWindow header
    };

    int m_dwMiscHeight[] = { 0x57342D, // 0 StretchY
                                0x57473B, // 1 Radio Icons
                                0x574783, // 2 Radio Icons
                                0x57482F, // 3 Radio Icons
                                0x574879, // 4 Radio Icons
                                0x57659A, // 5 2d Brief triangles
                                0x5765E2, // 6 2d Brief triangles
                                0x576625, // 7 2d Brief triangles
                                0x576668, // 8 2d Brief triangles
                                0x576709, // 9 2d Brief triangles
                                0x576751, // 10 2d Brief triangles
                                0x576794, // 11 2d Brief triangles
                                0x5767D7, // 12 2d Brief triangles
                                0x574EAA, // 13 Stats bars
                                0x574F0C, // 14 Stats bars
                                0x719D47, // 15 Font fixes
                                0x719D7C, // 16 Font fixes
                                0x719DB5, // 17 Font fixes
                                0x719DF2, // 18 Font fixes
                                0x719E2F, // 19 Font fixes
                                0x719EBF, // 20 Font fixes
                                0x719EE3, // 21 Font fixes
                                0x719C27, // 22 Font fixes
                                0x719C58, // 23 Font fixes
                                0x728864, // 24 BarChart fixes
                                0x7288A9, // 25 BarChart fixes
                                0x573F7D, // 26 DrawWindow header
                                0x573FD6, // 27 DrawWindow header
    };

    for (int i = 0; i < sizeof(m_dwMiscWidth) / sizeof(const void*); i++)
    {
        if (m_dwMiscWidth[i] != NULL)
            injector::WriteMemory<const void*>(m_dwMiscWidth[i] + 0x2, &fMiscWidth[i], true);
    }

    for (int i = 0; i < sizeof(m_dwMiscHeight) / sizeof(const void*); i++)
    {
        if (m_dwMiscHeight[i] != NULL)
            injector::WriteMemory<const void*>(m_dwMiscHeight[i] + 0x2, &fMiscHeight[i], true);
    }
}

void InstallHUDFixes() {
    int m_dwCrosshairWidth[] = { 0x58E7CE,
                                    0x58E7F8,
                                    0x58E2FA,
                                    0x58E4ED,
                                    0x58E75B,
                                    0x58E28B,
                                    0x58E2AC,
                                    0x58E2BA,
                                    0x53E472,
                                    0x53E4AE,
    };

    int m_dwCrosshairHeight[] = { 0x58E7E4,
                                    0x58E80E,
                                    0x58E319,
                                    0x58E527,
                                    0x58E2C8,
                                    0x53E3E7,
                                    0x53E409,
                                    NULL,
                                    NULL,
                                    NULL,
    };

    for (int i = 0; i < sizeof(m_dwCrosshairWidth) / sizeof(const void*); i++)
    {
        if (m_dwCrosshairWidth[i] != NULL)
            injector::WriteMemory<const void*>(m_dwCrosshairWidth[i] + 0x2, &fMiscWidth[0], true);
    }

    for (int i = 0; i < sizeof(m_dwCrosshairHeight) / sizeof(const void*); i++)
    {
        if (m_dwCrosshairHeight[i] != NULL)
            injector::WriteMemory<const void*>(m_dwCrosshairHeight[i] + 0x2, &fMiscHeight[0], true);
    }

    int m_dwRadarWidth[] = { 0x58A441, // Radar plane
                                    0x58A791, // Radar disc
                                    0x58A82E, // Radar disc
                                    0x58A8DF, // Radar disc
                                    0x58A982, // Radar disc
                                    0x58A5D8,
                                    0x58A6DE,
                                    0x5834BA, // Radar point
                                    0x58603F, // Radar point
                                    0x5886CC, // Radar centre
                                    0x58439C, // Radar Trace 0
                                    0x584434, // Radar Trace 0
                                    0x58410B, // Radar Trace 2
                                    0x584190, // Radar Trace 2
                                    0x584249, // Radar Trace 1
                                    0x5842E6, // Radar Trace 1
                                    0x5876D4,
                                    0x58774B,
                                    0x58780A,
                                    0x58788F,
                                    0x58792E,
                                    0x587A1A,
                                    0x587AAA,
    };

    int m_dwRadarHeight[] = { 0x58A473, // Radar plane
                                    0x58A600, // Radar disc
                                    0x58A69E, // Radar disc
                                    0x58A704, // Radar disc
                                    0x58A7B9, // Radar disc
                                    0x58A85A,
                                    0x58A909,
                                    0x58A9BD, // Radar point
                                    0x5834EC, // Radar point
                                    0x586058, // Radar centre
                                    0x584346, // Radar Trace 0
                                    0x58440C, // Radar Trace 0
                                    0x58412B, // Radar Trace 2
                                    0x5841B0, // Radar Trace 2
                                    0x584207, // Radar Trace 1
                                    0x5842C6, // Radar Trace 1
                                    0x5876BC,
                                    0x587733,
                                    0x587916,
                                    0x587A02,
                                    0x587A92,
                                    NULL,
                                    NULL,
    };

    for (int i = 0; i < sizeof(m_dwRadarWidth) / sizeof(const void*); i++)
    {
        if (m_dwRadarWidth[i] != NULL)
            injector::WriteMemory<const void*>(m_dwRadarWidth[i] + 0x2, &fRadarWidth[i], true);
    }

    for (int i = 0; i < sizeof(m_dwRadarHeight) / sizeof(const void*); i++)
    {
        if (m_dwRadarHeight[i] != NULL)
            injector::WriteMemory<const void*>(m_dwRadarHeight[i] + 0x2, &fRadarHeight[i], true);
    }

    int m_dwHUDWidth[] = { 0x58EB3F, // 0 Clock 
                                0x58EC0C, // 1 Clock
                                0x58F55C, // 2 Money
                                0x58F5F4, // 3 Money
                                0x5892CA, // 4 Info bars
                                0x58937E, // 5 Info bars
                                0x58EE7E, // 6 Info bars
                                0x58EEF4, // 7 Info bars
                                0x589155, // 8 Info bars
                                0x58EF50, // 9 Info bars
                                0x58EFC5, // 10 Info bars
                                0x58922D, // 11 Info bars
                                0x58F116, // 12 Info bars
                                0x58F194, // 13 Info bars
                                0x58D92D, // 14 Weapon icons
                                0x58D8C3, // 15 Weapon icons
                                0x58F91C, // 16 Weapon icons
                                0x58F92D, // 17 Weapon icons
                                0x5894C5, // 18 Ammo
                                0x5894E9, // 19 Ammo
                                0x58F9D0, // 20 Ammo
                                0x58FA5D, // 21 Ammo
                                0x58F9F5, // 22 Ammo
                                0x58FA8E, // 23 Ammo
                                0x58DCB8, // 24 Wanted
                                0x58DD00, // 25 Wanted
                                0x58DD7E, // 26 Wanted
                                0x58DF71, // 27 Wanted
                                0x58DFE5, // 28 Wanted
                                NULL,	  // 29
                                0x58B09F, // 30 Vehicle names
                                0x58B13F, // 31 Vehicle names
                                0x58AD3A, // 32 Area names
                                0x58AD65, // 33 Area names
                                0x58AE4A, // 34 Area names
                                0x58C395, // 35 Subs
                                0x58C41D, // 36 Subs
                                0x58C4DC, // 37 Subs
                                0x5896D8, // 38	Stats box
                                0x589703, // 39	Stats box
                                0x58990C, // 40 Stats box
                                0x58986D, // 41 Stats box
                                0x5897C3, // 42 Stats box
                                0x589A16, // 43 Stats box
                                0x589B2D, // 44 Stats box
                                0x589C73, // 45 Stats box
                                0x589D61, // 46 Stats box
                                0x589E49, // 47 Stats box
                                0x589F31, // 48 Stats box
                                0x58A013, // 49 Stats box
                                0x58A090, // 50 Stats box
                                0x58A134, // 51 Stats box
                                NULL,	  // 52
                                NULL,	  // 53
                                NULL,	  // 54
                                NULL,	  // 55
                                NULL,	  // 56
                                NULL,	  // 57
                                NULL,	  // 58
                                NULL,	  // 59
                                NULL,	  // 60
                                NULL,	  // 61
                                0x58C863, // 62 SuccessFailed text
                                0x58D2DB, // 63 MissionTitle text
                                0x58D459, // 64 MissionTitle text
                                0x58CBC1, // 65 WastedBusted text
                                0x58B273, // 66 Timers
                                0x58B2A4, // 67 Timers
                                0x58B3AF, // 68 Timers
                                0x58B3FC, // 69 Timers
                                0x58B56A, // 70 Timers
                                0x58B5EE, // 71 Timers
                                0x58B67E, // 72 Timers
                                0x58B76F, // 73 Helptext
                                0x58B7D6, // 74 Helptext
                                0x58BA62, // 75 Helptext
                                0x58BAC6, // 76 Helptext
                                0x58BBDB, // 77 Helptext
                                0x58BCB0, // 78 Helptext
                                0x58BD58, // 79 Helptext
                                0x58BE8D, // 80 Helptext
                                0x58BF7E, // 81 Helptext
                                0x58BFFC, // 82 Helptext
                                0x580F16, // 83 Menu system
                                0x580F95, // 84
                                0x5810EF, // 85
                                0x581158, // 86
                                0x5811CD, // 87
                                0x58148A, // 88
                                0x5814F7, // 89
                                0x5815B1, // 90
                                0x5815EB, // 91
                                0x581633, // 92			
                                0x47AD2A, // 93
                                0x5818CF, // 94
                                0x58CCDB, // 95 OddJob
                                0x58CDE6, // 96 OddJob
                                0x58CEE2, // 97 OddJob
                                0x58D15C, // 98 OddJob
                                0x58A178, // 99 TripSkip
                                0x58A21D, // 100 TripSkip
                                0x58A2C0, // 101 TripSkip
                                0x4E9F30, // 102 RadioStation
                                0x43CF57, // 103 CDarkel
                                0x4477CD, // 104 CGarages
                                0x4477F7, // 105 CGarages
    };

    int m_dwHUDHeight[] = { 0x58EB29, // 0 Clock
                                0x58EBF9, // 1 Clock
                                0x58F546, // 2 Money
                                0x58F5CE, // 3 Money
                                0x589346, // 4 Info bars
                                0x58EE60, // 5 Info bars
                                0x588B9C, // 6 Info bars
                                0x58EEC8, // 7 Info bars
                                0x58913E, // 8 Info bars
                                0x58EF32, // 9 Info bars
                                0x58EF99, // 10 Info bars
                                0x589216, // 11 Info bars
                                0x58F0F8, // 12 Info bars
                                0x58F168, // 13 Info bars
                                0x58D945, // 14 Weapon icons
                                0x58D882, // 15 Weapon icons
                                0x58F90B, // 16 Weapon icons
                                NULL,	  // 17
                                0x5894AF, // 18 Ammo
                                NULL,	  // 19
                                0x58F9C0, // 20 Ammo
                                0x58FA4A, // 21 Ammo
                                NULL,	  // 22
                                NULL,	  // 23
                                0x58DCA2, // 24 Wanted
                                0x58DD68, // 25 Wanted
                                0x58DDF4, // 26 Wanted
                                0x58DF55, // 27 Wanted
                                0x58DF9B, // 28 Wanted
                                0x58DEE4, // 29 Wanted
                                0x58B089, // 30 Vehicle names
                                0x58B12D, // 31 Vehicle names
                                0x58AD24, // 32 Area names
                                0x58AE0D, // 33 Area names
                                NULL,	  // 34
                                0x58C37F, // 35 Subs
                                0x58C407, // 36 Subs
                                0x58C4C6, // 37 Subs
                                //0x58C53B, // 38 Subs
                                //0x58C611, // 39 Subs
                                0x5898F6, // 40 Stats box text
                                //0x58C46E, // 41 Subs
                                NULL,	  // 42
                                0x589735, // 43 Stats box
                                0x58978B, // 44 Stats box
                                0x589813, // 45 Stats box
                                0x58983F, // 46 Stats box
                                0x5898BD, // 47 Stats box
                                0x5899FF, // 48 Stats box
                                0x589A4B, // 49 Stats box
                                0x589B16, // 50 Stats box
                                0x589C5C, // 51 Stats box
                                0x589CA8, // 52 Stats box
                                0x589D4A, // 53 Stats box
                                0x589D92, // 54 Stats box
                                0x589E32, // 55 Stats box
                                0x589E7A, // 56 Stats box
                                0x589F1A, // 57 Stats box
                                0x589F62, // 58 Stats box
                                0x589FFC, // 59 Stats box
                                0x58A040, // 60 Stats box
                                0x58A07A, // 61 Stats box
                                0x58C84D, // 62 SuccessFailed text
                                0x58D2C5, // 63 MissionTitle text
                                NULL, // 0x58D447, // 64 MissionTitle text
                                0x58CBAB, // 65 WastedBusted text
                                NULL, // 0x58B1B7, // 66 Timers
                                0x58B263, // 67 Timers
                                NULL, //0x58B435, // 68 Timers
                                NULL, //0x58B536, // 69 Timers
                                0x58B5DE, // 70 Timers		
                                NULL,	  // 71
                                NULL,	  // 72
                                0x58B7BD, // 73 Help text
                                0x58BA4C, // 74 Help text
                                0x58BBA7, // 75 Help text
                                0x58BD19, // 76 Help text
                                0x58BE2B, // 77 Help text
                                0x58BF1C, // 78 Help text
                                0x58BFCB, // 79 Help text		
                                NULL,	  // 80
                                NULL,	  // 81
                                NULL,	  // 82
                                0x580E11, // 83 Menu system
                                0x580F85, // 84
                                0x5810CC, // 85
                                0x581132, // 86
                                0x5811A1, // 87
                                0x58147A, // 88
                                0x5814E7, // 89
                                0x581699, // 90
                                NULL,	  // 91
                                NULL,	  // 92
                                NULL,	  // 93
                                0x581889, // 94
                                0x58CCC5, // 95 OddJob
                                0x58CDD0, // 96 OddJob
                                0x58CECC, // 97 OddJob
                                0x58D146, // 98 OddJob
                                NULL, //0x58A199, // 99 TripSkip
                                NULL, //0x58A207, // 100 TripSkip
                                NULL, //0x58A2B0, // 101 TripSkip
                                0x4E9F1A, // 102 RadioStation
                                0x43CF47, // 103 CDarkel
                                0x4477B7, // 104 CGarages
                                0x4478AC, // 105 CGarages							
    };

    for (int i = 0; i < sizeof(m_dwHUDWidth) / sizeof(const void*); i++)
    {
        if (m_dwHUDWidth[i] != NULL)
            injector::WriteMemory<const void*>(m_dwHUDWidth[i] + 0x2, &fHUDWidth[i], true);
    }

    for (int i = 0; i < sizeof(m_dwHUDHeight) / sizeof(const void*); i++)
    {
        if (m_dwHUDHeight[i] != NULL)
            injector::WriteMemory<const void*>(m_dwHUDHeight[i] + 0x2, &fHUDHeight[i], true);
    }

    // Help text bar chart offset
    static float fBarChartOffsetY = 160.0f;
    injector::WriteMemory<const void*>(0x58BE9F + 0x2, &fBarChartOffsetY);

    // Lock Subtitles Width
    static float fSubtitlesMult = 1.0f;
    injector::WriteMemory<const void*>(0x58C4E8 + 0x2, &fSubtitlesMult);

    // Second player fix.
    injector::WriteMemory<const void*>(0x58F9A0 + 0x2, &fHUDWidth[110]); // Weapon icon X
    injector::WriteMemory<const void*>(0x58F993 + 0x2, &fHUDWidth[16]); // Weapon icon X 
    injector::WriteMemory<const void*>(0x58F972 + 0x2, &fHUDHeight[16]); // Weapon icon Y
    injector::WriteMemory<const void*>(0x58FA8E + 0x2, &fHUDWidth[17]); // Ammo x
}

injector::hook_back<void(__cdecl*)(CRect&, CRGBA const&, CRGBA const&, CRGBA const&, CRGBA const&)> hbSetVertices;
void __cdecl SetVerticesHook(CRect& a1, CRGBA const& a2, CRGBA const& a3, CRGBA const& a4, CRGBA const& a5)
{
    uint32_t pTexture = 0;
    _asm mov pTexture, esi

    if (static_cast<int>(a1.m_fLeft) <= 0 && static_cast<int>(a1.m_fTop) <= 0 && static_cast<int>(a1.m_fRight) >= RsGlobal->MaximumWidth && static_cast<int>(a1.m_fBottom) >= RsGlobal->MaximumHeight)
    {
        if (pTexture && std::find(std::begin(main_scm_tex), std::end(main_scm_tex), std::string_view((const char*)(*(uintptr_t*)pTexture + 0x10))) != std::end(main_scm_tex))
        {
            float fMiddleScrCoord = (float)RsGlobal->MaximumWidth / 2.0f;

            float w = 16.0f;
            float h = 9.0f;

            if (FrontendAspectRatioWidth && FrontendAspectRatioHeight)
            {
                w = (float)FrontendAspectRatioWidth;
                h = (float)FrontendAspectRatioHeight;
            }
            else
            {
                if (pTexture)
                {
                    if (*(uint32_t*)pTexture)
                    {
                        pTexture = **(uint32_t**)pTexture;
                        w = (float)(*(uint32_t*)(pTexture + 0x28));
                        h = (float)(*(uint32_t*)(pTexture + 0x2C));
                        if (w == h && w > 0 && h > 0)
                        {
                            w = 4.0f;
                            h = 3.0f;
                        }
                    }
                }
            }

            fFrontendDefaultWidth = ((((float)RsGlobal->MaximumHeight * (w / h))));

            a1.m_fTop = 0.0f;
            a1.m_fLeft = fMiddleScrCoord - ((((float)RsGlobal->MaximumHeight * (w / h))) / 2.0f);
            a1.m_fBottom = (float)RsGlobal->MaximumHeight;
            a1.m_fRight = fMiddleScrCoord + ((((float)RsGlobal->MaximumHeight * (w / h))) / 2.0f);

            CRGBA RectColor = { 0, 0, 0, a2.alpha };
            CSprite2dDrawRect2(CRect(-5.0f, a1.m_fBottom, a1.m_fLeft, -5.0f), RectColor, RectColor, RectColor, RectColor);
            CSprite2dDrawRect2(CRect((float)RsGlobal->MaximumWidth, a1.m_fBottom, a1.m_fRight, -5.0f), RectColor, RectColor, RectColor, RectColor);
            CSprite2dDrawRect2(CRect(-5.0f, (float)RsGlobal->MaximumHeight + 5.0f, (float)RsGlobal->MaximumWidth + 5.0f, -5.0f), RectColor, RectColor, RectColor, RectColor);
        }
    }
    return hbSetVertices.fun(a1, a2, a3, a4, a5);
}

injector::hook_back<void(__cdecl*)(float, float, unsigned short, unsigned int, float, int, bool, bool, CRGBA const&, CRGBA const&)> hbDrawLoadingBar;
void __cdecl DrawLoadingBarHook(float x, float y, unsigned int w, unsigned int h, float progress, int progressAdded, bool drawPercentage, bool drawBlackBorder, CRGBA const& color, CRGBA const& progressAddedColor) {
    if (FrontendAspectRatioWidth && FrontendAspectRatioHeight)
    {
        x = ((float)RsGlobal->MaximumWidth * 0.5f - fFrontendDefaultWidth * 0.5f + fFrontendDefaultWidth * 0.079f);
        w = static_cast<unsigned int>(fFrontendDefaultWidth * 0.279f);
    }
    else
    {
        x = ((float)RsGlobal->MaximumWidth * 0.5f - fFrontendDefaultWidth * 0.5f + x * (4.0f / 3.0f) / *CDraw::pfScreenAspectRatio);
        w = static_cast<unsigned int>((float)w * (4.0f / 3.0f) / *CDraw::pfScreenAspectRatio);
    }

    hbDrawLoadingBar.fun(x, y, w, h, progress, progressAdded, drawPercentage, drawBlackBorder, color, progressAddedColor);
}

void Install2dSpriteFixes()
{
    CIniReader iniReader("");
    szForceAspectRatio = iniReader.ReadString("MAIN", "FrontendAspectRatio", "auto");
    if (strncmp(szForceAspectRatio.c_str(), "auto", 4) != 0)
    {
        FrontendAspectRatioWidth = std::stoi(szForceAspectRatio.c_str());
        FrontendAspectRatioHeight = std::stoi(strchr(szForceAspectRatio.c_str(), ':') + 1);
    }
    else
    {
        FrontendAspectRatioWidth = 0;
        FrontendAspectRatioHeight = 0;
    }

    hbSetVertices.fun = injector::MakeCALL(0x728360, SetVerticesHook).get();
    injector::MakeCALL(0x728360, SetVerticesHook);

    // Loading bar fix
    hbDrawLoadingBar.fun = injector::MakeCALL(0x590480, DrawLoadingBarHook).get();
    injector::MakeCALL(0x590480, DrawLoadingBarHook);
}

void ApplyIniOptions()
{
    CIniReader iniReader("");
    ResX = iniReader.ReadInteger("MAIN", "ResX", -1);
    ResY = iniReader.ReadInteger("MAIN", "ResY", -1);
    fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.8f);
    fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.8f);
    fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.82f);
    fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 1.0f);
    bRestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 1) != 0;
    fCarSpeedDependantFOV = iniReader.ReadFloat("MAIN", "CarSpeedDependantFOV", 0.0f);
    bDontTouchFOV = iniReader.ReadInteger("MAIN", "DontTouchFOV", 0) != 0;
    bool DisableWhiteCrosshairDot = iniReader.ReadInteger("MAIN", "DisableWhiteCrosshairDot", 1) != 0;
    szForceAspectRatio = iniReader.ReadString("MAIN", "ForceAspectRatio", "auto");
    nHideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 1);
    bSmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1) != 0;
    ReplaceTextShadowWithOutline = iniReader.ReadInteger("MAIN", "ReplaceTextShadowWithOutline", 0);
    bool bAltTab = iniReader.ReadInteger("MAIN", "AllowAltTabbingWithoutPausing", 0) != 0;

    if (bAltTab)
    {
        //Windowed mode fix (from MTA sources)
        if ((GetWindowLong((HWND)RsGlobal->ps, GWL_STYLE) & WS_POPUP) == 0) {
            // Disable MENU AFTER alt + tab
            //0053BC72   C605 7B67BA00 01 MOV BYTE PTR DS:[BA677B],1    
            injector::WriteMemory<uint8_t>(0x53BC78, 0x00);

            // ALLOW ALT+TABBING WITHOUT PAUSING
            injector::MakeNOP(0x748A8D, 6, true);
            injector::MakeJMP(0x6194A0, AllowMouseMovement, true);
        }
    }

    if (strncmp(szForceAspectRatio.c_str(), "auto", 4) != 0)
    {
        AspectRatioWidth = std::stoi(szForceAspectRatio.c_str());
        AspectRatioHeight = std::stoi(strchr(szForceAspectRatio.c_str(), ':') + 1);
        fCustomAspectRatioHor = static_cast<float>(AspectRatioWidth);
        fCustomAspectRatioVer = static_cast<float>(AspectRatioHeight);
    }

    bFOVControl = iniReader.ReadInteger("MAIN", "FOVControl", 1) != 0;
    FOVControl = (uint32_t*)0x6FF41B;
    injector::WriteMemory<float>(FOVControl, 1.0f, true);

    if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.8f; fHudHeightScale = 0.8f; }
    if (!fRadarWidthScale) { fRadarWidthScale = 0.82f; }
    if (!fSubtitlesScale) { fSubtitlesScale = 1.0f; }

    if (DisableWhiteCrosshairDot)
    {
        injector::MakeNOP(0x58E2DD, 5, true);
    }

    if (ReplaceTextShadowWithOutline)
    {
        //CFont::SetDropShadowPosition -> CFont::SetEdge
        injector::MakeJMP(0x719570, SetDropShadowPosition, true);
    }

    if (nHideAABug)
    {
        injector::MakeJMP(0x53E90E, Hide1pxAABug, true);
    }

    injector::WriteMemory<uint8_t>(0x53E2AD, 0x74, true); // Reverse g_MenuManager.widescreenOn to make widescreen off equal to borders off
    injector::WriteMemory<uint8_t>(0x58BB90, 0x74, true); // for borders and text boxes.

    if (bSmartCutsceneBorders)
    {
        injector::MakeCALL(0x53E2B4, CCamera::DrawBordersForWideScreen, true);
        injector::MakeCALL(0x5AF8C0, CCamera::DrawBordersForWideScreen, true);
        injector::WriteMemory<uint8_t>(0x58BB93, 0x00, true); // Text box offset in cutscenes
    }
}

void Init()
{
    GetMemoryAddresses();
    OverwriteResolution();
    ApplyIniOptions();
    InstallAspectRatioFixes();
    InstallFieldOfViewFixes();
    InstallFrontendFixes();
    InstallMiscFixes();
    InstallHUDFixes();
    InstallSCMDrawingFixes();
    Install2dSpriteFixes();
}

void CompatWarning()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileName(GetModuleHandle(L"wshps.asi"), buffer, sizeof(buffer));
    auto s = std::wstring(buffer);

    auto result = MessageBox(NULL, L"wshps.asi is no longer required for the widescreen fix to work.\n\
     Click Yes to remove it, restart the game after.\n\
     Click No to continue, but issues may occur (not recommended).\n\
     You can remove wshps.asi manually later.", L"GTASA.WidescreenFix.asi", MB_YESNO);

    switch (result)
    {
    case IDYES:
        MoveFileW(s.c_str(), std::wstring(s + L".deleteonnextlaunch").c_str());
        ExitProcess(0);
        break;
    default:
        break;
    }
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init();
        CallbackHandler::RegisterCallback(L"SilentPatchSA.asi", OverwriteResolution);
        CallbackHandler::RegisterCallback(L"wshps.asi", CompatWarning);
    }
    return TRUE;
}