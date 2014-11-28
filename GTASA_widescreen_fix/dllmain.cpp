#include "stdafx.h"
#include "game.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
#include <string>

HWND hWnd;
void Init();
void AllowMouseMovement();
void WINAPI InstallAllHooks();
float fHudWidthScale, fHudHeightScale;
float fCustomWideScreenWidthScaleDown;
float fCustomWideScreenHeightScaleDown;
float fCustomRadarWidthScaleDown;
float fRadarWidthScale, fCustomRadarWidthScale, fSubtitlesScale;
float fSubtitlesScaleX, fSubtitlesScaleY;
int RestoreCutsceneFOV;
float fCarSpeedDependantFOV;
int DontTouchFOV;
char *szForceAspectRatio;
char *szFOVControl;
int FOVControl;
int AspectRatioWidth, AspectRatioHeight;
int HideAABug, SmartCutsceneBorders;
float fCustomDynamicScreenFieldOfViewScale;
DWORD DynamicScreenFieldOfViewScale;
float fDefaultFOV = 1.0f;
float fRadarScaling;
int DisableWhiteCrosshairDot;

float **const fWideScreenWidthScaleDown = (float**)0x00573F95;
float **const fWideScreenHeightScaleDown = (float**)0x00573F7F;

HMODULE hwshps;

auto CSprite2d__DrawRect = (int(__cdecl *)(CRect const &, CRGBA const &)) 0x727B60;
void _1pxBugFix()
{
    CSprite2d__DrawRect(CRect(0.0f, -5.0f, (float)CLASS_pclRsGlobal->m_iScreenWidth, 0.5f), CRGBA(0, 0, 0, 255));
    CSprite2d__DrawRect(CRect(-5.0f, -1.0f, 0.5f, (float)CLASS_pclRsGlobal->m_iScreenHeight), CRGBA(0, 0, 0, 255));
    if (HideAABug == 2)
    {
        CSprite2d__DrawRect(CRect(0.0f, (float)CLASS_pclRsGlobal->m_iScreenHeight - 1.5f, (float)CLASS_pclRsGlobal->m_iScreenWidth, (float)CLASS_pclRsGlobal->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
        CSprite2d__DrawRect(CRect((float)CLASS_pclRsGlobal->m_iScreenWidth - 1.0f, 0.0f, (float)CLASS_pclRsGlobal->m_iScreenWidth + 5.0f, (float)CLASS_pclRsGlobal->m_iScreenHeight), CRGBA(0, 0, 0, 255));
    }
}

void GetScreenRect(CRect& rect)
{
    float			fScreenRatio = *VAR_pfScreenAspectRatio;
    float			dScreenHeightWeWannaCut = ((-9.0f / 16.0f) * fScreenRatio + 1.0f);
    float			dBorderProportionsFix = ((-144643.0f / 50000.0f) * fScreenRatio * fScreenRatio) + ((807321.0f / 100000.0f) * fScreenRatio) - (551143.0f / 100000.0f);

    if (dBorderProportionsFix < 0.0)
        dBorderProportionsFix = 0.0;

    if (dScreenHeightWeWannaCut > 0.0)
    {
        // Letterbox
        rect.y1 = ((float)CLASS_pclRsGlobal->m_iScreenHeight / 2) * (dScreenHeightWeWannaCut - dBorderProportionsFix);
        rect.y2 = (float)CLASS_pclRsGlobal->m_iScreenHeight - (((float)CLASS_pclRsGlobal->m_iScreenHeight / 2) * (dScreenHeightWeWannaCut + dBorderProportionsFix));
    }
    else
    {
        // Pillarbox
        dScreenHeightWeWannaCut = -dScreenHeightWeWannaCut;

        rect.x1 = ((float)CLASS_pclRsGlobal->m_iScreenWidth / 4) * dScreenHeightWeWannaCut;
        rect.x2 = (float)CLASS_pclRsGlobal->m_iScreenWidth - ((float)CLASS_pclRsGlobal->m_iScreenWidth / 4) * dScreenHeightWeWannaCut;
    }
}

int CCamera__DrawBordersForWideScreen(int)
{
    if (!*(BYTE*)0xBA6793)
        return 0;

    CRect		ScreenRect;

    ScreenRect.x1 = -1000.0f;
    ScreenRect.y1 = -1000.0f;
    ScreenRect.x2 = -1000.0f;
    ScreenRect.y2 = -1000.0f;

    GetScreenRect(ScreenRect);

    unsigned char bTextBoxOffset = static_cast<unsigned char>(ScreenRect.y1);
    CPatch::SetUChar(0x58BB93, bTextBoxOffset == 24 ? 0 : bTextBoxOffset);

    // Letterbox
    if (ScreenRect.y1 > 0.0 && ScreenRect.y2 > 0.0)
    {
        CSprite2d__DrawRect(CRect(-5.0f, -5.0f, (float)CLASS_pclRsGlobal->m_iScreenWidth + 5.0f, ScreenRect.y1), CRGBA(0, 0, 0, 255));
        CSprite2d__DrawRect(CRect(-5.0f, ScreenRect.y2, (float)CLASS_pclRsGlobal->m_iScreenWidth + 5.0f, (float)CLASS_pclRsGlobal->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
    }
    //Pillarbox
    else if (ScreenRect.x1 > 0.0 && ScreenRect.x2 > 0.0)
    {
        CSprite2d__DrawRect(CRect(-5.0f, -5.0f, ScreenRect.x1, (float)CLASS_pclRsGlobal->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
        CSprite2d__DrawRect(CRect(ScreenRect.x2, -5.0f, (float)CLASS_pclRsGlobal->m_iScreenWidth + 5.0f, (float)CLASS_pclRsGlobal->m_iScreenHeight + 5.0f), CRGBA(0, 0, 0, 255));
    }
    return 0;

}

auto FindPlayerVehicle = (int(__cdecl *)(int playerNum, char a2)) 0x56E0D0;
inline float getDynamicScreenFieldOfView(float fFactor)
{
    fCustomWideScreenWidthScaleDown = **fWideScreenWidthScaleDown * fHudWidthScale;
    fCustomWideScreenHeightScaleDown = **fWideScreenHeightScaleDown * fHudHeightScale;
    fCustomRadarWidthScaleDown = **fWideScreenWidthScaleDown * fRadarWidthScale;
    fSubtitlesScaleX = **fWideScreenWidthScaleDown * fHudWidthScale * fSubtitlesScale;
    fSubtitlesScaleY = **fWideScreenHeightScaleDown * fHudHeightScale * fSubtitlesScale;

    if ((!(char)*(DWORD*)0xB6F065 == 0 && RestoreCutsceneFOV) || DontTouchFOV)
    {
        return fFactor * (*(float*)FOVControl);
    }

    if (fCarSpeedDependantFOV)
    {
        if (FindPlayerVehicle(-1, 0))
        {
            return (fFactor * (*(float*)FOVControl) * *(float*)DynamicScreenFieldOfViewScale) + ((fRadarScaling > 350.0f ? 350.0f : fRadarScaling) / fCarSpeedDependantFOV);
        }

    }

    return fFactor * (*(float*)FOVControl) * *(float*)DynamicScreenFieldOfViewScale;
}

void setScreenFieldOfView(float fFactor) {
    *VAR_pfScreenFieldOfView = /*!CLASS_pclCamera->m_bWidescreen ? */getDynamicScreenFieldOfView(fFactor)/* : fFactor*/;
}

DWORD jmpAddr;
void __declspec(naked)RadarScaling_compat_patch()
{
    _asm
    {
        //fstp dword ptr ds : [0x0BA8314]
        mov jmpAddr, ebx
        fstp    ds : fRadarScaling
        mov ebx, fRadarScaling
        mov ds : [0xBA8314], ebx
    }
    fRadarScaling -= 180.0f;
    _asm
    {
        mov ebx, jmpAddr
        mov jmpAddr, 0x586C70
        jmp jmpAddr
    }
}

DWORD WINAPI Load(LPVOID)
{
        CIniReader iniReader("");
        fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.62221788786f);
        fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.66666670937f);
        fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.80354591724f);
        fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 1.0f);

        RestoreCutsceneFOV = iniReader.ReadInteger("MAIN", "RestoreCutsceneFOV", 1);

        fCarSpeedDependantFOV = iniReader.ReadFloat("MAIN", "CarSpeedDependantFOV", 0.0f);
        CPatch::RedirectJump(0x586C6A, RadarScaling_compat_patch);

        DontTouchFOV = iniReader.ReadInteger("MAIN", "DontTouchFOV", 0);
        DisableWhiteCrosshairDot = iniReader.ReadInteger("MAIN", "DisableWhiteCrosshairDot", 1);

        szForceAspectRatio = iniReader.ReadString("MAIN", "ForceAspectRatio", "auto");
        if (strncmp(szForceAspectRatio, "auto", 4) != 0)
        {
            AspectRatioWidth = std::stoi(szForceAspectRatio);
            AspectRatioHeight = std::stoi(strchr(szForceAspectRatio, ':') + 1);
        }

        // Unlocked widescreen resolutions by Silent
        CPatch::SetUInt(0x745B71, 0x9090687D);
        CPatch::SetUInt(0x74596C, 0x9090127D);
        CPatch::Nop(0x745970, 2);
        CPatch::Nop(0x745B75, 2);
        CPatch::Nop(0x7459E1, 2);

        if (iniReader.ReadInteger("MAIN", "AllowAltTabbingWithoutPausing", 0))
        {
            //Windowed mode fix (from MTA sources)
            if ((GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP) == 0)
            {
                // Disable MENU AFTER alt + tab
                //0053BC72   C605 7B67BA00 01 MOV BYTE PTR DS:[BA677B],1    
                CPatch::SetUChar(0x53BC78, 0x00);

                // ALLOW ALT+TABBING WITHOUT PAUSING
                CPatch::Nop(0x748A8D, 6);

                CPatch::RedirectJump(0x6194A0, AllowMouseMovement);
            }
        }

        static int i;
        do
        {

            Sleep(100);
            i++;
            if (i > 10000)
                break;
            hwshps = GetModuleHandle("wshps.asi");

        } while (hwshps == NULL);

            if (AspectRatioWidth && AspectRatioHeight)
            {
                CPatch::SetPointer((DWORD)hwshps + 0x8616, &AspectRatioWidth);
                CPatch::SetPointer((DWORD)hwshps + 0x8616 + 6, &AspectRatioHeight);
            }


        szFOVControl = iniReader.ReadString("MAIN", "FOVControl", "0x6FF41B");
        sscanf_s(szFOVControl, "%X", &FOVControl);
        CPatch::SetFloat(FOVControl, 1.0f);

        HideAABug = iniReader.ReadInteger("MAIN", "HideAABug", 1);

        SmartCutsceneBorders = iniReader.ReadInteger("MAIN", "SmartCutsceneBorders", 1);

        if (!fHudWidthScale || !fHudHeightScale) { fHudWidthScale = 0.82962955833f; fHudHeightScale = 0.76923079438f; }
        if (!fRadarWidthScale) { fRadarWidthScale = 0.82962955833f; }
        if (!fSubtitlesScale) { fSubtitlesScale = 1.0f; }

        // Set screen Field Of View (FOV).
        CPatch::RedirectCall(0x00460306 - 1, setScreenFieldOfView);
        CPatch::RedirectCall(0x0052C977 - 1, setScreenFieldOfView);
        CPatch::RedirectCall(0x0053BD7B - 1, setScreenFieldOfView);
        CPatch::RedirectCall(0x005BA225 - 1, setScreenFieldOfView);

        DynamicScreenFieldOfViewScale = *(DWORD*)((DWORD)hwshps + 0x85F5);

        InstallAllHooks();		

        if (HideAABug)
        {
            CPatch::RedirectJump(0x53E90E, _1pxBugFix);
        }

        CPatch::SetUChar(0x53E2AD, 0x74); //Reverse g_MenuManager.widescreenOn to make widescreen off equal to borders off
        CPatch::SetUChar(0x58BB90, 0x74); //for borders and text boxes.
        if (SmartCutsceneBorders)
        {
            CPatch::RedirectCall(0x53E2B4, CCamera__DrawBordersForWideScreen);
            CPatch::RedirectCall(0x5AF8C0, CCamera__DrawBordersForWideScreen);
        }

        if (DisableWhiteCrosshairDot)
        {
            CPatch::Nop(0x58E2DD, 5); // disable white crosshair dot drawing
        }
    return 0;
}

// COMMAND_GET_HEIGHT_PERCENT_IN_WORLD_LEVEL
float getHeightPercentInWorldLevel(float fPosZ) {
    float fHeightPerc;

    if (fPosZ <= -100.0f) {
        fHeightPerc = 100.0f;
    }
    else if (fPosZ <= 0.0f) {
        fHeightPerc = -fPosZ;
    }
    else if (fPosZ <= 200.0f) {
        fHeightPerc = fPosZ * (100.0f / 200.0f);
    }
    else if (fPosZ <= 950.0f) {
        fHeightPerc = (fPosZ - 200.0f) * (100.0f / (950.0f - 200.0f));
    }
    else if (fPosZ < 1500.0f) {
        fHeightPerc = (fPosZ - 950.0f) * (100.0f / (1500.0f - 950.0f));
    }
    else {
        fHeightPerc = 100.0f;
    }

    return fHeightPerc;
}

void __declspec(naked) HOOK_PTR_0058A68F_drawMap() {
    static const float fPosX = 15.0f;
    static const float fPosY = 28.0f;
    static const float fWidth = 20.0f;
    static const float fHeight = 2.0f;
    static const float fScrollHeight = 74.0f;
    static const float fPercent = 0.01f;

    __asm {
        fild	ds : [ADDR_clRsGlobal]CRsGlobal.m_iScreenHeight
            fld		st
            fmul	fCustomWideScreenHeightScaleDown
            mov		ecx, [eax]SRwV3d.m_fZ
            push	ecx
            call	getHeightPercentInWorldLevel
            add		esp, 4
            fmul	fPercent
            fmul	fScrollHeight
            fadd	fPosY
            fmul	st, st(1)
            fsubp	st(2), st
            fmul	fHeight
            fsubr	st, st(1)
            fstp	dword ptr[esp + 34h - 14h]
            fstp	dword ptr[esp + 34h - 1Ch]
            fild	ds : [ADDR_clRsGlobal]CRsGlobal.m_iScreenWidth
            fmul	fCustomWideScreenWidthScaleDown
            fld		fPosX
            fadd	fWidth
            fmul	st, st(1)
            fxch	st(1)
            fmul	fPosX
            mov		edx, ADDR_0058A73B_drawMap
            jmp		edx
    }
}

void WINAPI InstallAllHooks() {
    // Rampage (Kill Frenzy) counter.
    CPatch::SetPointer(0x0043CF49, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x0043CF59, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    CPatch::SetPointer(0x0043D10B, &fCustomWideScreenWidthScaleDown);							// Time X position scale.
    CPatch::SetPointer(0x0043D163, &fCustomWideScreenWidthScaleDown);							// Kills X position scale.
    // Collectable text.
    CPatch::SetPointer(0x004477B9, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x004477CF, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    CPatch::SetPointer(0x004477F9, &fCustomWideScreenWidthScaleDown);							// Text centre size scale.
    CPatch::SetPointer(0x004478AE, &fCustomWideScreenHeightScaleDown);							// 2-numbered text Y position scale.
    CPatch::SetPointer(0x00447918, &fCustomWideScreenHeightScaleDown);							// 1-numbered text Y position scale.
    CPatch::SetPointer(0x0044795D, &fCustomWideScreenHeightScaleDown);							// 0-numbered text Y position scale.
    // Replay text.
    CPatch::SetPointer(0x0045C241, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x0045C257, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    // Radar map zoomed coordinates.
    CPatch::SetPointer(0x005834BC, &fCustomRadarWidthScaleDown);	                						// Map width scale.
    //CPatch::SetPointer(0x005834EE, &fCustomWideScreenHeightScaleDown);						// Map height scale.
    // Radar blip.
    CPatch::SetPointer(0x0058410D, &fCustomWideScreenWidthScaleDown);							// Squared radar blip border width scale.
    CPatch::SetPointer(0x0058412D, &fCustomWideScreenHeightScaleDown);							// Squared radar blip border height scale.
    CPatch::SetPointer(0x00584192, &fCustomWideScreenWidthScaleDown);							// Squared radar blip width scale.
    CPatch::SetPointer(0x005841B2, &fCustomWideScreenHeightScaleDown);							// Squared radar blip height scale.
    CPatch::SetPointer(0x00584209, &fCustomWideScreenHeightScaleDown);							// Fipped triangular radar blip border height scale.
    CPatch::SetPointer(0x0058424B, &fCustomWideScreenWidthScaleDown);							// Fipped triangular radar blip border width scale.
    CPatch::SetPointer(0x005842C8, &fCustomWideScreenHeightScaleDown);							// Fipped triangular radar blip height scale.
    CPatch::SetPointer(0x005842E8, &fCustomWideScreenWidthScaleDown);							// Fipped triangular radar blip width scale.
    CPatch::SetPointer(0x00584348, &fCustomWideScreenHeightScaleDown);							// Triangular radar blip border height scale.
    CPatch::SetPointer(0x0058439E, &fCustomWideScreenWidthScaleDown);							// Triangular radar blip border width scale.
    CPatch::SetPointer(0x0058440E, &fCustomWideScreenHeightScaleDown);							// Triangular radar blip height scale.
    CPatch::SetPointer(0x00584436, &fCustomWideScreenWidthScaleDown);							// Triangular radar blip width scale.
    // Radar position.
    CPatch::SetPointer(0x005849FC, &fCustomWideScreenHeightScaleDown);							// Sprite height scale.
    CPatch::SetPointer(0x00584A14, &fCustomWideScreenWidthScaleDown);							// Sprite width scale.
    // Radar sprite.
    CPatch::SetPointer(0x00586041, &fCustomWideScreenWidthScaleDown);							// Sprite width scale.
    CPatch::SetPointer(0x0058605A, &fCustomWideScreenHeightScaleDown);							// Sprite height scale.
    // Radar blip for entity.
    CPatch::SetPointer(0x005876BE, &fCustomWideScreenHeightScaleDown);							// Radar centre sprite height scale.
    CPatch::SetPointer(0x005876D6, &fCustomWideScreenWidthScaleDown);							// Radar centre sprite width scale.
    CPatch::SetPointer(0x00587735, &fCustomWideScreenHeightScaleDown);							// Radar centre sprite height scale on camera locked behind the player.
    CPatch::SetPointer(0x0058774D, &fCustomWideScreenWidthScaleDown);							// Radar centre sprite width scale on camera locked behind the player.
    CPatch::SetPointer(0x0058780C, &fCustomWideScreenWidthScaleDown);							// Radar searchlight sprite X position scale.
    CPatch::SetPointer(0x00587891, &fCustomWideScreenWidthScaleDown);							// Radar searchlight sprite width scale.
    CPatch::SetPointer(0x00587918, &fCustomWideScreenHeightScaleDown);							// Radar moving light sprite height scale.
    CPatch::SetPointer(0x00587930, &fCustomWideScreenWidthScaleDown);							// Radar moving light sprite width scale.
    CPatch::SetPointer(0x00587A04, &fCustomWideScreenHeightScaleDown);							// Radar runway lights sprite height scale in menu.
    CPatch::SetPointer(0x00587A1C, &fCustomWideScreenWidthScaleDown);							// Radar runway lights sprite width scale in menu.
    CPatch::SetPointer(0x00587A94, &fCustomWideScreenHeightScaleDown);							// Radar runway lights sprite height scale.
    CPatch::SetPointer(0x00587AAC, &fCustomWideScreenWidthScaleDown);							// Radar runway lights sprite width scale.
    // HUD component Y position computation.
    CPatch::SetPointer(0x00588B9E, &fCustomWideScreenHeightScaleDown);							// Component relative Y position scale.
    // Armour meter.
    CPatch::SetPointer(0x00589140, &fCustomWideScreenHeightScaleDown);							// Meter height scale.
    CPatch::SetPointer(0x00589157, &fCustomWideScreenWidthScaleDown);							// Meter width scale.
    // Breath meter.
    CPatch::SetPointer(0x00589218, &fCustomWideScreenHeightScaleDown);							// Meter height scale.
    CPatch::SetPointer(0x0058922F, &fCustomWideScreenWidthScaleDown);							// Meter width scale.
    // Health meter.
    CPatch::SetPointer(0x005892CC, &fCustomWideScreenWidthScaleDown);							// Meter width scale.
    CPatch::SetPointer(0x00589348, &fCustomWideScreenHeightScaleDown);							// Meter height scale.
    CPatch::SetPointer(0x00589380, &fCustomWideScreenWidthScaleDown);							// Meter X position scale.
    // Weapon ammo.
    CPatch::SetPointer(0x005894B1, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x005894C7, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    // Statistics.
    CPatch::SetPointer(0x00589705, &fCustomWideScreenWidthScaleDown);							// Texts and meters X position scale.
    CPatch::SetPointer(0x00589737, &fCustomWideScreenHeightScaleDown);							// Respect meter relative Y position scale.
    CPatch::SetPointer(0x0058978D, &fCustomWideScreenHeightScaleDown);							// 5-meters box height scale.
    CPatch::SetPointer(0x005897C5, &fCustomWideScreenWidthScaleDown);							// 5-meters box width scale.
    CPatch::SetPointer(0x00589815, &fCustomWideScreenHeightScaleDown);							// 5-meters box content relative Y position scale.
    CPatch::SetPointer(0x00589841, &fCustomWideScreenHeightScaleDown);							// 6-meters box height scale.
    CPatch::SetPointer(0x0058986F, &fCustomWideScreenWidthScaleDown);							// 6-meters box width scale.
    CPatch::SetPointer(0x005898BF, &fCustomWideScreenHeightScaleDown);							// 6-meters box content relative Y position scale.
    CPatch::SetPointer(0x005898F8, &fCustomWideScreenHeightScaleDown);							// Skill text height scale.
    CPatch::SetPointer(0x0058990E, &fCustomWideScreenWidthScaleDown);							// Skill text width scale.
    CPatch::SetPointer(0x00589A01, &fCustomWideScreenHeightScaleDown);							// Respect text and meter height scale.
    CPatch::SetPointer(0x00589A18, &fCustomWideScreenWidthScaleDown);							// Respect text and meter width scale.
    CPatch::SetPointer(0x00589A01, &fCustomWideScreenHeightScaleDown);							// Lung capacity/Weapon skill text and meter relative Y position scale.
    CPatch::SetPointer(0x00589B18, &fCustomWideScreenHeightScaleDown);							// Lung capacity text and meter height scale.
    CPatch::SetPointer(0x00589B2F, &fCustomWideScreenWidthScaleDown);							// Lung capacity text and meter width scale.
    CPatch::SetPointer(0x00589C5E, &fCustomWideScreenHeightScaleDown);							// Weapon skill text and meter height scale.
    CPatch::SetPointer(0x00589C75, &fCustomWideScreenWidthScaleDown);							// Weapon skill text and meter width scale.
    CPatch::SetPointer(0x00589CAA, &fCustomWideScreenHeightScaleDown);							// Stamina text and meter relative Y position scale.
    CPatch::SetPointer(0x00589D4C, &fCustomWideScreenHeightScaleDown);							// Stamina text and meter height scale.
    CPatch::SetPointer(0x00589D63, &fCustomWideScreenWidthScaleDown);							// Stamina text and meter width scale.
    CPatch::SetPointer(0x00589D94, &fCustomWideScreenHeightScaleDown);							// Muscle text and meter relative Y position scale.
    CPatch::SetPointer(0x00589E34, &fCustomWideScreenHeightScaleDown);							// Muscle text and meter height scale.
    CPatch::SetPointer(0x00589E4B, &fCustomWideScreenWidthScaleDown);							// Muscle text and meter width scale.
    CPatch::SetPointer(0x00589E7C, &fCustomWideScreenHeightScaleDown);							// Fat text and meter relative Y position scale.
    CPatch::SetPointer(0x00589F1C, &fCustomWideScreenHeightScaleDown);							// Fat text and meter height scale.
    CPatch::SetPointer(0x00589F33, &fCustomWideScreenWidthScaleDown);							// Fat text and meter width scale.
    CPatch::SetPointer(0x00589F64, &fCustomWideScreenHeightScaleDown);							// Sex appeal text and meter relative Y position scale.
    CPatch::SetPointer(0x00589FFE, &fCustomWideScreenHeightScaleDown);							// Sex appeal text and meter height scale.
    CPatch::SetPointer(0x0058A015, &fCustomWideScreenWidthScaleDown);							// Sex appeal text and meter width scale.
    CPatch::SetPointer(0x0058A042, &fCustomWideScreenHeightScaleDown);							// Weekday text relative Y position scale.
    CPatch::SetPointer(0x0058A07C, &fCustomWideScreenHeightScaleDown);							// Weekday text height scale.
    CPatch::SetPointer(0x0058A092, &fCustomWideScreenWidthScaleDown);							// Weekday text width scale.
    CPatch::SetPointer(0x0058A136, &fCustomWideScreenWidthScaleDown);							// Weekday text X position scale.
    // Trip skip.
    CPatch::SetPointer(0x0058A17A, &fCustomWideScreenWidthScaleDown);							// Sprite X position and width scale.
    CPatch::SetPointer(0x0058A19B, &fCustomWideScreenHeightScaleDown);							// Sprite Y position and height scale.
    CPatch::SetPointer(0x0058A209, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x0058A21F, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    CPatch::SetPointer(0x0058A2B2, &fCustomWideScreenHeightScaleDown);							// Text Y position scale.
    CPatch::SetPointer(0x0058A2C2, &fCustomWideScreenWidthScaleDown);							// Text X position scale.
    // Map.
    CPatch::SetPointer(0x0058A443, &fCustomRadarWidthScaleDown);						        // Radar ring plane sprite X position and width scale.
    //CPatch::SetPointer(0x0058A475, &fCustomWideScreenHeightScaleDown);						// Radar ring plane sprite Y position and height scale.
    CPatch::SetPointer(0x0058A5DA, &fCustomRadarWidthScaleDown);						        // Altimeter vertical bar X position and width scale.
    CPatch::SetPointer(0x0058A602, &fCustomRadarWidthScaleDown);						        // Altimeter vertical bar Y position and height scale.
    CPatch::RedirectJump(0x0058A68F, HOOK_PTR_0058A68F_drawMap);								// Altimeter horizontal bar position and scale.
    CPatch::SetPointer(0x0058A793, &fCustomRadarWidthScaleDown);						        // Upper-left radar disc X position and width scale.
    //CPatch::SetPointer(0x0058A7BB, &fCustomWideScreenHeightScaleDown);						// Upper-left radar disc Y position and height scale.
    CPatch::SetPointer(0x0058A830, &fCustomRadarWidthScaleDown);						        // Upper-right radar disc X position and width scale.
    //CPatch::SetPointer(0x0058A85C, &fCustomWideScreenHeightScaleDown);						// Upper-right radar disc Y position and height scale.
    CPatch::SetPointer(0x0058A8E1, &fCustomRadarWidthScaleDown);							    // Lower-left radar disc X position and width scale.
    //CPatch::SetPointer(0x0058A90B, &fCustomWideScreenHeightScaleDown);						// Lower-left radar disc Y position and height scale.
    CPatch::SetPointer(0x0058A984, &fCustomRadarWidthScaleDown);						        // Lower-right radar disc X position and width scale.
    //CPatch::SetPointer(0x0058A9BF, &fCustomWideScreenHeightScaleDown);						// Lower-right radar disc Y position and height scale.
    // Zone name.
    CPatch::SetPointer(0x0058AD26, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x0058AD3C, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    CPatch::SetPointer(0x0058AD67, &fCustomWideScreenWidthScaleDown);							// Text centre X wrap scale.
    CPatch::SetPointer(0x0058AE0F, &fCustomWideScreenHeightScaleDown);							// Text Y position scale.
    CPatch::SetPointer(0x0058AE4C, &fCustomWideScreenWidthScaleDown);							// Text X position scale.
    // Vehicle name.
    CPatch::SetPointer(0x0058B08B, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x0058B0A1, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    CPatch::SetPointer(0x0058B12F, &fCustomWideScreenHeightScaleDown);							// Text Y position scale.
    CPatch::SetPointer(0x0058B141, &fCustomWideScreenWidthScaleDown);							// Text X position scale.
    // On-screen displays.
    CPatch::SetPointer(0x0058B265, &fCustomWideScreenHeightScaleDown);							// Texts height scale.
    CPatch::SetPointer(0x0058B275, &fCustomWideScreenWidthScaleDown);							// Texts width scale.
    CPatch::SetPointer(0x0058B3B1, &fCustomWideScreenWidthScaleDown);							// Timer text X position scale.
    CPatch::SetPointer(0x0058B3FE, &fCustomWideScreenWidthScaleDown);							// Timer value text X position scale.
    CPatch::SetPointer(0x0058B538, &fCustomWideScreenHeightScaleDown);							// Counter relative Y position.
    CPatch::SetPointer(0x0058B56C, &fCustomWideScreenWidthScaleDown);							// Counter value text X position scale.
    CPatch::SetPointer(0x0058B5E0, &fCustomWideScreenHeightScaleDown);							// Counter meter relative Y position scale.
    CPatch::SetPointer(0x0058B5F0, &fCustomWideScreenWidthScaleDown);							// Counter meter X position scale.
    CPatch::SetPointer(0x0058B680, &fCustomWideScreenWidthScaleDown);							// Counter text X position scale.
    // Message box.
    CPatch::SetPointer(0x0058B771, &fCustomWideScreenWidthScaleDown);							// Text X wrap scale.
    CPatch::SetPointer(0x0058B7BF, &fCustomWideScreenHeightScaleDown);							// Text Y position scale for the line count.
    CPatch::SetPointer(0x0058B7D8, &fCustomWideScreenWidthScaleDown);							// Text X position scale for the line count.
    CPatch::SetPointer(0x0058BA4E, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x0058BA64, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    CPatch::SetPointer(0x0058BCB2, &fCustomWideScreenWidthScaleDown);							// Text width scale for the line width.
    CPatch::SetPointer(0x0058BD1B, &fCustomWideScreenHeightScaleDown);							// Statistic text Y position scale.
    CPatch::SetPointer(0x0058BD5A, &fCustomWideScreenWidthScaleDown);							// Statistic text X position scale.
    CPatch::SetPointer(0x0058BF1E, &fCustomWideScreenHeightScaleDown);							// Statistic increment/decrement foreground Y position and height scale.
    CPatch::SetPointer(0x0058BF80, &fCustomWideScreenWidthScaleDown);							// Statistic increment/decrement foreground X position and width scale.
    CPatch::SetPointer(0x0058BFCD, &fCustomWideScreenHeightScaleDown);							// Statistic plus/minus text Y position scale.
    CPatch::SetPointer(0x0058BFFE, &fCustomWideScreenWidthScaleDown);							// Statistic plus/minus text X position scale.
    CPatch::SetPointer(0x0058BAC8, &fCustomWideScreenWidthScaleDown);							// Message text X wrap scale.
    CPatch::SetPointer(0x0058BBA9, &fCustomWideScreenHeightScaleDown);							// Message text Y position scale.
    CPatch::SetPointer(0x0058BBDD, &fCustomWideScreenWidthScaleDown);							// Message text X position scale.
    // Subtitle text.
    CPatch::SetPointer(0x0058C361, &fCustomWideScreenWidthScaleDown);							// Cutscene text centre size scale.
    CPatch::SetPointer(0x0058C381, &fCustomWideScreenHeightScaleDown);							// Cutscene text height scale.
    CPatch::SetPointer(0x0058C397, &fCustomWideScreenWidthScaleDown);							// Cutscene text width scale.
    CPatch::SetPointer(0x0058C3B6, &fCustomWideScreenHeightScaleDown);							// Cutscene text Y position scale.
    CPatch::SetPointer(0x0058C409, &fCustomWideScreenHeightScaleDown);							// Positioning/Normal text height scale.
    CPatch::SetPointer(0x0058C41F, &fCustomWideScreenWidthScaleDown);							// Positioning/Normal text width scale.
    CPatch::SetPointer(0x0058C451, &fCustomWideScreenWidthScaleDown);							// Positioning text centre size scale.
    CPatch::SetPointer(0x0058C470, &fCustomWideScreenHeightScaleDown);							// Positioning text Y position scale.
    CPatch::SetPointer(0x0058C4C8, &fCustomWideScreenHeightScaleDown);							// Stat box dependent text height scale.
    CPatch::SetPointer(0x0058C4DE, &fCustomWideScreenWidthScaleDown);							// Stat box dependent text width scale.
    CPatch::SetPointer(0x0058C501, &fCustomWideScreenWidthScaleDown);							// Stat box dependent text centre size scale.
    CPatch::SetPointer(0x0058C546, &fCustomWideScreenHeightScaleDown);							// Stat box dependent text Y position scale.
    CPatch::SetPointer(0x0058C5D4, &fCustomWideScreenWidthScaleDown);							// Normal text centre size scale.
    CPatch::SetPointer(0x0058C613, &fCustomWideScreenHeightScaleDown);							// Normal text Y position scale.
    // Mission text.
    CPatch::SetPointer(0x0058C6CE, &fCustomWideScreenHeightScaleDown);							// Text initial relative Y position scale.
    CPatch::SetPointer(0x0058C76D, &fCustomWideScreenHeightScaleDown);							// Monoline text relative Y position scale.
    CPatch::SetPointer(0x0058C7AB, &fCustomWideScreenHeightScaleDown);							// Multiline text relative Y position scale.
    CPatch::SetPointer(0x0058C7F2, &fCustomWideScreenHeightScaleDown);							// Text relative Y position scale when the title text is printed.
    CPatch::SetPointer(0x0058C813, &fCustomWideScreenHeightScaleDown);							// Text relative Y position scale when the odd-job and reward texts (A) are printed.
    CPatch::SetPointer(0x0058C84F, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x0058C865, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    CPatch::SetPointer(0x0058C897, &fCustomWideScreenWidthScaleDown);							// Text centre size scale.
    // Title text.
    CPatch::SetPointer(0x0058CA8B, &fCustomWideScreenHeightScaleDown);							// Text initial relative Y position scale.
    CPatch::SetPointer(0x0058CB0E, &fCustomWideScreenHeightScaleDown);							// Text relative Y position scale.
    CPatch::SetPointer(0x0058CB36, &fCustomWideScreenHeightScaleDown);							// Text relative Y position scale when the mission text is printed.
    CPatch::SetPointer(0x0058CBAD, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x0058CBC3, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    // Secondary mission text.
    CPatch::SetPointer(0x0058CCC7, &fCustomWideScreenHeightScaleDown);							// Odd-job text (B) height scale.
    CPatch::SetPointer(0x0058CCDD, &fCustomWideScreenWidthScaleDown);							// Odd-job text (B) width scale.
    CPatch::SetPointer(0x0058CD08, &fCustomWideScreenWidthScaleDown);							// Odd-job text (B) centre size scale.
    CPatch::SetPointer(0x0058CDD2, &fCustomWideScreenHeightScaleDown);							// Reward text (B) height scale.
    CPatch::SetPointer(0x0058CDE8, &fCustomWideScreenWidthScaleDown);							// Reward text (B) width scale.
    CPatch::SetPointer(0x0058CE0D, &fCustomWideScreenWidthScaleDown);							// Reward text (B) centre size scale.
    CPatch::SetPointer(0x0058CECE, &fCustomWideScreenHeightScaleDown);							// Odd-job text (A) height scale.
    CPatch::SetPointer(0x0058CEE4, &fCustomWideScreenWidthScaleDown);							// Odd-job text (A) width scale.
    CPatch::SetPointer(0x0058CF0F, &fCustomWideScreenWidthScaleDown);							// Odd-job text (A) centre size scale.
    CPatch::SetPointer(0x0058D148, &fCustomWideScreenHeightScaleDown);							// Reward text (A) height scale.
    CPatch::SetPointer(0x0058D15E, &fCustomWideScreenWidthScaleDown);							// Reward text (A) width scale.
    CPatch::SetPointer(0x0058D189, &fCustomWideScreenWidthScaleDown);							// Reward text (A) centre size scale.
    // Load mission text.
    CPatch::SetPointer(0x0058D2C7, &fCustomWideScreenHeightScaleDown);							// Text height scale.
    CPatch::SetPointer(0x0058D2DD, &fCustomWideScreenWidthScaleDown);							// Text width scale.
    CPatch::SetPointer(0x0058D449, &fCustomWideScreenHeightScaleDown);							// Text Y position scale.
    CPatch::SetPointer(0x0058D45B, &fCustomWideScreenWidthScaleDown);							// Text X position scale.
    // Weapon icon.
    CPatch::SetPointer(0x0058D884, &fCustomWideScreenHeightScaleDown);							// Gun sprite height scale.
    CPatch::SetPointer(0x0058D8C5, &fCustomWideScreenWidthScaleDown);							// Gun sprite width scale.
    CPatch::SetPointer(0x0058D92F, &fCustomWideScreenWidthScaleDown);							// Fist sprite width scale.
    CPatch::SetPointer(0x0058D947, &fCustomWideScreenHeightScaleDown);							// Fist sprite height scale.
    // Wanted level.
    CPatch::SetPointer(0x0058DD02, &fCustomWideScreenWidthScaleDown);							// Rightmost star X position scale.
    CPatch::SetPointer(0x0058DD6A, &fCustomWideScreenHeightScaleDown);							// Stars foreground text height scale.
    CPatch::SetPointer(0x0058DD80, &fCustomWideScreenWidthScaleDown);							// Stars foreground text width scale.
    CPatch::SetPointer(0x0058DF57, &fCustomWideScreenHeightScaleDown);							// Stars background text height scale.
    CPatch::SetPointer(0x0058DF73, &fCustomWideScreenWidthScaleDown);							// Stars background text width scale.
    CPatch::SetPointer(0x0058DF9D, &fCustomWideScreenHeightScaleDown);							// Stars background Y position scale.
    CPatch::SetPointer(0x0058DFE7, &fCustomWideScreenWidthScaleDown);							// Stars relative X position scale.
    CPatch::SetPointer(0x0058DEE6, &fCustomWideScreenHeightScaleDown);							// Stars foreground Y position scale.
    // HUD interface.
    CPatch::SetPointer(0x0058EB2B, &fCustomWideScreenHeightScaleDown);							// Day time text height scale.
    CPatch::SetPointer(0x0058EB41, &fCustomWideScreenWidthScaleDown);							// Day time text width scale.
    CPatch::SetPointer(0x0058EBFB, &fCustomWideScreenHeightScaleDown);							// Day time text Y position scale.
    CPatch::SetPointer(0x0058EC0E, &fCustomWideScreenWidthScaleDown);							// Day time text X position scale.
    CPatch::SetPointer(0x0058EE62, &fCustomWideScreenHeightScaleDown);							// First player health meter Y position scale.
    CPatch::SetPointer(0x0058EE80, &fCustomWideScreenWidthScaleDown);							// First player health meter X position scale.
    CPatch::SetPointer(0x0058EECA, &fCustomWideScreenHeightScaleDown);							// Second player health meter Y position scale.
    CPatch::SetPointer(0x0058EEF6, &fCustomWideScreenWidthScaleDown);							// Second player health meter X position scale.
    CPatch::SetPointer(0x0058EF34, &fCustomWideScreenHeightScaleDown);							// First player armour meter Y position scale.
    CPatch::SetPointer(0x0058EF52, &fCustomWideScreenWidthScaleDown);							// First player armour meter X position scale.
    CPatch::SetPointer(0x0058EF9B, &fCustomWideScreenHeightScaleDown);							// Second player armour meter Y position scale.
    CPatch::SetPointer(0x0058EFC7, &fCustomWideScreenWidthScaleDown);							// Second player armour meter X position scale.
    CPatch::SetPointer(0x0058F0FA, &fCustomWideScreenHeightScaleDown);							// First player breath meter Y position scale.
    CPatch::SetPointer(0x0058F118, &fCustomWideScreenWidthScaleDown);							// First player breath meter X position scale.
    CPatch::SetPointer(0x0058F16A, &fCustomWideScreenHeightScaleDown);							// Second player breath meter Y position scale.
    CPatch::SetPointer(0x0058F196, &fCustomWideScreenWidthScaleDown);							// Second player breath meter X position scale.
    CPatch::SetPointer(0x0058F548, &fCustomWideScreenHeightScaleDown);							// Money text height scale.
    CPatch::SetPointer(0x0058F55E, &fCustomWideScreenWidthScaleDown);							// Money text width scale.
    CPatch::SetPointer(0x0058F5D0, &fCustomWideScreenHeightScaleDown);							// Money text Y position scale.
    CPatch::SetPointer(0x0058F5F6, &fCustomWideScreenWidthScaleDown);							// Money text X position scale.
    CPatch::SetPointer(0x0058F90D, &fCustomWideScreenHeightScaleDown);							// First player weapon icon Y position scale.
    CPatch::SetPointer(0x0058F91E, &fCustomWideScreenWidthScaleDown);							// First player weapon icon X position scale.
    CPatch::SetPointer(0x0058F974, &fCustomWideScreenHeightScaleDown);							// Second player weapon icon Y position scale.
    CPatch::SetPointer(0x0058F995, &fCustomWideScreenWidthScaleDown);							// Second player weapon icon X position scale.
    CPatch::SetPointer(0x0058F9C2, &fCustomWideScreenHeightScaleDown);							// First player weapon ammo Y position scale.
    CPatch::SetPointer(0x0058F9D2, &fCustomWideScreenWidthScaleDown);							// First player weapon ammo X position scale.
    CPatch::SetPointer(0x0058FA4C, &fCustomWideScreenHeightScaleDown);							// Second player weapon ammo Y position scale.
    CPatch::SetPointer(0x0058FA5F, &fCustomWideScreenWidthScaleDown);							// Second player weapon ammo X position scale.
    // Meter.
    /*CPatch::SetPointer(0x00728866, &fCustomWideScreenHeightScaleDown);							// Top border height scale.
    CPatch::SetPointer(0x007288AB, &fCustomWideScreenHeightScaleDown);							// Bottom border height scale.
    CPatch::SetPointer(0x007288F7, &fCustomWideScreenWidthScaleDown);							// Left border width scale.
    CPatch::SetPointer(0x00728943, &fCustomWideScreenWidthScaleDown);							// Right border width scale.*/
    
    CPatch::SetPointer(0x0058C397, &fSubtitlesScaleX);								// Cutscene text height scale.
    CPatch::SetPointer(0x0058C381, &fSubtitlesScaleY);								// Cutscene text width scale.
    CPatch::SetPointer(0x0058C409, &fSubtitlesScaleY);								// Positioning/Normal text height scale.
    CPatch::SetPointer(0x0058C41F, &fSubtitlesScaleX);								// Positioning/Normal text width scale.

    CPatch::SetPointer((DWORD)hwshps + 0x85F5, &fCustomDynamicScreenFieldOfViewScale);

    //crash fixes
    CPatch::SetUChar(0x57F884 + 1, 0x08);
    CPatch::SetUChar(0x57FAE9, 0x14);
    CPatch::SetUInt(0x57FAF3, 0x456A006A);
    CPatch::SetUChar(0x57FAF7, 0x8B);
    CPatch::SetUShort(0x57FD02, 0xFF68);

    //high speed bug
    CPatch::SetUChar(0x574252, 0x3B);
    CPatch::SetUInt(0x574253, 0xD90875C7);

    //Frame limiter bug
    CPatch::SetUChar(0x58BB87, 0x0C);
    CPatch::SetUChar(0x58BB88, 0x8A);
}

DWORD _EAX;
void __declspec(naked) AllowMouseMovement()
{
    _asm
    {
        mov _EAX, eax
        mov eax, dword ptr ds: [0x8D621C]
        cmp eax, 0
        jne label1
        mov eax, _EAX
        ret
    label1:
        mov eax, _EAX
        mov _EAX, 0x7453F0
        jmp _EAX
    }
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Load, NULL, 0, NULL);
    }
    return TRUE;
}