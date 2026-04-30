module;

#include <stdafx.h>
#include "common.h"

export module Misc;

import Skeleton;
import Camera;
import Entity;
import Vehicle;
import Timer;
import Draw;

int ReplaceTextShadowWithOutline = 0;

enum BlackCRGBA
{
    CDarkel__DrawMessages_0,
    CDarkel__DrawMessages_1,
    CGameLogic__Update_2,
    CGameLogic__Update_3,
    CGarages__PrintMessages_4,
    CRunningScript__ProcessCommands800To899_5,
    CCamera__DrawBordersForWideScreen_6,
    CCamera__DrawBordersForWideScreen_7,
    CMenuManager__DrawFrontEndNormal_8,
    CMenuManager__Draw_9,
    CMenuManager__Draw_10,
    CMenuManager__DrawPlayerSetupScreen_11,
    CMenuManager__DrawPlayerSetupScreen_12,
    CMenuManager__DrawPlayerSetupScreen_13,
    CMenuManager__DrawControllerSetupScreen_14,
    CMenuManager__DrawControllerSetupScreen_15,
    CMenuManager__PrintStats_16,
    sub_483990_17,
    CMenuManager__DisplaySlider_18,
    CMenuManager__FilterOutColorMarkersFromString_19,
    CMenuManager__DrawControllerBound_20,
    CMenuManager__DrawControllerBound_21,
    CMenuManager__MessageScreen_22,
    CGame__Process_23,
    DoFade_24,
    LoadingScreen_25,
    LoadingIslandScreen_26,
    Render2dStuff_27,
    CPlayerInfo__Process_28,
    CPlayerInfo__Process_29,
    CRadar__ShowRadarTrace_30,
    CFont__ParseToken_31,
    CHud__Draw_32,
    CHud__Draw_33,
    CHud__Draw_34,
    CHud__Draw_35,
    CHud__Draw_36,
    CHud__Draw_37,
    CHud__Draw_38,
    CHud__Draw_39,
    CHud__Draw_40,
    CHud__Draw_41,
    CHud__Draw_42,
    CHud__Draw_43,
    CHud__Draw_44,
    CHud__Draw_45,
    CHud__Draw_46,
    CHud__Draw_47,
    CHud__Draw_48,
    CHud__Draw_49,
    CHud__Draw_50,
    CHud__DrawAfterFade_51,
    CHud__DrawAfterFade_52,
    CHud__DrawAfterFade_53,
    CHud__DrawAfterFade_54,
    CPlane__ProcessControl_55,
    CPlane__ProcessControl_56,
    cMusicManager__DisplayRadioStationName_57,
    CSceneEdit__Draw_58,
    CSceneEdit__Draw_59,
    CSceneEdit__Draw_60,
    CSceneEdit__Draw_61,
    ConvertingTexturesScreen_62,
    sub_5C4817_63,
    rxD3D8SubmitNode_64,
    rxD3D8SubmitNode_65,
};

struct __declspec(align(4)) CFontDetails
{
    CRGBA m_Color;
    CVector2D m_vecScale;
    float m_fSlant;
    CVector2D m_vecSlantRefPoint;
    bool m_bJustify;
    bool m_bCentre;
    bool m_bRightJustify;
    bool m_bBackground;
    bool m_bBackGroundOnlyText;
    bool m_bProp;
    char _pad1E[2];
    float m_fAlphaFade;
    CRGBA m_BackgroundColor;
    float m_fWrapX;
    float m_fCentreSize;
    float m_fRightJustifyWrap;
    int16_t m_nStyle;
    char _pad36[2];
    int m_nBank;
    int16_t m_nDropShadowPosition;
    CRGBA m_DropColor;
    char _pad42[2];
};

namespace CFont
{
    void (__cdecl* SetColor)(CRGBA* color) = nullptr;

    export GameRef<CFontDetails> Details([]() -> CFontDetails*
    {
        auto pattern = hook::pattern("68 ? ? ? ? 52 0F BF 05");
        if (!pattern.empty())
            return *pattern.get_first<CFontDetails*>(1);
        return nullptr;
    });

    SafetyHookInline shPrintString = {};
    void __cdecl PrintString(float x, float y, wchar_t* str)
    {
        if (!str)
            return shPrintString.unsafe_ccall(x, y, str);

        if (Details->m_bBackground)
            return shPrintString.unsafe_ccall(x, y, str);

        CRGBA originalColor = Details->m_Color;

        if (originalColor.red == 1 && originalColor.green == 1 && originalColor.blue == 1)
            return;

        if (originalColor.red == 0x20 && originalColor.green == 0xA2 && originalColor.blue == 0x42) //pager
            return shPrintString.unsafe_ccall(x, y, str);

        float outlineStrength = 0.5f;
        CRGBA outlineColor;
        bool isVeryDark = (originalColor.red <= 0x14 && originalColor.green <= 0x14 && originalColor.blue <= 0x14);

        if (isVeryDark)
            outlineColor = CRGBA(255, 255, 255, originalColor.alpha / 10);
        else
            outlineColor = CRGBA(0, 0, 0, originalColor.alpha);

        static CVector2D offsets[] = {
            { 1.f,  1.f}, { 1.f, -1.f},
            {-1.f,  1.f}, {-1.f, -1.f},
            { 2.f,  0.f}, {-2.f,  0.f},
            { 0.f,  2.f}, { 0.f, -2.f}
        };

        int offsetCount = (ReplaceTextShadowWithOutline > 1) ? 8 : 4;

        SetColor(&outlineColor);

        for (int i = 0; i < offsetCount; ++i)
        {
            CVector2D& offset = offsets[i];
            shPrintString.unsafe_ccall(x + SCREEN_SCALE_X(offset.x * outlineStrength), y + SCREEN_SCALE_Y(offset.y * outlineStrength), str);
        }

        SetColor(&originalColor);
        shPrintString.unsafe_ccall(x, y, str);
    }

    SafetyHookInline shSetDropColor = {};
    void __cdecl SetDropColor(CRGBA* color)
    {
        Details->m_DropColor = { 0, 0, 0, 0 };
    }
}

injector::hook_back<void(__fastcall*)(CCamera*, void*)> hbCalculateDerivedValues;
void __fastcall CalculateDerivedValues(CCamera* camera, void* edx)
{
    CCamera::UpdatePlayerVehicleSpeedBlur(camera);
    return hbCalculateDerivedValues.fun(camera, edx);
}

class Misc
{
public:
    Misc()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            ReplaceTextShadowWithOutline = iniReader.ReadInteger("MISC", "ReplaceTextShadowWithOutline", 0);
            auto bCarSpeedDependantFOV = iniReader.ReadInteger("MISC", "CarSpeedDependantFOV", 1) != 0;
            auto bVCSCamShake = iniReader.ReadInteger("MISC", "VCSCamShake", 0) != 0;

            if (ReplaceTextShadowWithOutline)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 83 3D ? ? ? ? ? 59 0F 84");
                CFont::SetColor = (decltype(CFont::SetColor))injector::GetBranchDestination(pattern.get_first(0)).as_int();

                pattern = hook::pattern("E8 ? ? ? ? 8D 44 24 ? 50 E8 ? ? ? ? 59 6A ? E8 ? ? ? ? 83 3D ? ? ? ? ? 59 0F 84");
                auto CRGBACtor = injector::GetBranchDestination(pattern.get_first()).as_int();

                //Hiding text shadow
                std::vector<uintptr_t> CRGBACtorRefs;
                auto MenuPattern3 = hook::pattern("6A 00 6A 00 6A 00 E8"); // CRGBA::CRGBA(..., 0, 0, 0, ...);
                MenuPattern3.for_each_result([&](const hook::pattern_match& match)
                {
                    CRGBACtorRefs.push_back((uintptr_t)match.get<void*>(0));
                });

                pattern = hook::pattern("6A ? 6A ? 6A ? E8 ? ? ? ? 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? 8B 3D ? ? ? ? 8D 84 24");
                if (CRGBACtorRefs.size() >= CSceneEdit__Draw_61 && CRGBACtorRefs[CSceneEdit__Draw_61] == (uintptr_t)pattern.get_first())
                {
                    std::vector<int> HideTextShadowsIndexArray = {
                        CDarkel__DrawMessages_0,                   // 0x00420B66
                        CDarkel__DrawMessages_1,                   // 0x00420C72
                        CGarages__PrintMessages_4,                 //0x00426387
                        //CMenuManager__Draw_9,                    // 0x0047AEB4
                        CMenuManager__Draw_10,                     // 0x0047C5AF
                        //CMenuManager__DrawPlayerSetupScreen_11,    // 0x0047F34B
                        CMenuManager__DrawPlayerSetupScreen_12,    // 0x00480F75
                        CMenuManager__DrawPlayerSetupScreen_13,    // 0x004810A8
                        //CMenuManager__DrawControllerSetupScreen_14,// 0x004812D1
                        //CMenuManager__DrawControllerSetupScreen_15,// 0x00481FD5
                        //CMenuManager__PrintStats_16,               // 0x004826EC
                        //CMenuManager__DrawControllerBound_20,      // 0x00489CFE
                        //CMenuManager__DrawControllerBound_21,      // 0x00489F2E
                        CMenuManager__MessageScreen_22,            // 0x0048B7FC
                        CHud__Draw_32,                             // 0x00505F1A
                        CHud__Draw_34,                             // 0x00506572
                        CHud__Draw_35,                             // 0x005068E3
                        CHud__Draw_37,                             // 0x00507084
                        CHud__Draw_38,                             // 0x00507522
                        CHud__Draw_39,                             // 0x005076EC
                        CHud__Draw_40,                             // 0x005078DC
                        CHud__Draw_41,                             // 0x00507A21
                        CHud__Draw_42,                             // 0x00507C88
                        CHud__Draw_43,                             // 0x00507F83
                        CHud__Draw_48,                             // 0x005087B1
                        CHud__Draw_49,                             // 0x00508871
                        CHud__Draw_50,                             // 0x00508E9A
                        CHud__DrawAfterFade_52,                    // 0x00509862
                        CHud__DrawAfterFade_53,                    // 0x005099E3
                        CHud__DrawAfterFade_54,                    // 0x00509D4C
                        cMusicManager__DisplayRadioStationName_57, // 0x0057E9A1
                        CSceneEdit__Draw_58,                       // 0x005856FB
                        CSceneEdit__Draw_59,                       // 0x0058585A
                        CSceneEdit__Draw_60,                       // 0x005859B2
                        CSceneEdit__Draw_61,                       // 0x00585A24
                    };

                    for (size_t i = 0; i < CRGBACtorRefs.size(); ++i)
                    {
                        if (std::find(HideTextShadowsIndexArray.begin(), HideTextShadowsIndexArray.end(), i) != HideTextShadowsIndexArray.end())
                        {
                            auto dest = injector::GetBranchDestination(CRGBACtorRefs[i] + 6).as_int();
                            if (dest == CRGBACtor)
                            {
                                injector::WriteMemory<uint8_t>(CRGBACtorRefs[i] + 1, 0x01, true);
                                injector::WriteMemory<uint8_t>(CRGBACtorRefs[i] + 3, 0x01, true);
                                injector::WriteMemory<uint8_t>(CRGBACtorRefs[i] + 5, 0x01, true);
                            }
                        }
                    }
                }

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? FF 35 ? ? ? ? FF 35");
                CFont::shPrintString = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), CFont::PrintString);

                pattern = hook::pattern("E8 ? ? ? ? 59 8D 8C 24 ? ? ? ? 68 ? ? ? ? 68");
                CFont::shSetDropColor = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), CFont::SetDropColor);

                // cursor shadow alpha
                pattern = hook::pattern("6A 32 6A 64 6A 64 6A 64"); //0x47ACB7
                pattern.for_each_result([&](const hook::pattern_match& match)
                {
                    injector::WriteMemory<uint8_t>(match.get<void*>(1), 0x00, true);
                });

                //mission text shadow color
                pattern = hook::pattern("6A ? 6A ? 6A ? ? ? E8 ? ? ? ? 8D 44 24");//50A0C2
                injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x01, true);
                injector::WriteMemory<uint8_t>(pattern.get_first(3), 0x01, true);
                injector::WriteMemory<uint8_t>(pattern.get_first(5), 0x01, true);

                //mission failed text shadow color
                pattern = hook::pattern("6A ? 6A ? 6A ? ? ? E8 ? ? ? ? 8D 84 24 ? ? ? ? 50");//508BFA
                injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x01, true);
                injector::WriteMemory<uint8_t>(pattern.get_first(3), 0x01, true);
                injector::WriteMemory<uint8_t>(pattern.get_first(5), 0x01, true);

                //pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? D9 05 ? ? ? ? D8 4C 24 04 D9 3C 24");
                //injector::WriteMemory(pattern.get(0).get<uint32_t>(1), 0xFFFF885F, true); //0x50948D text box background disable
                //
                //pager
                //pattern = hook::pattern("6A ? 68 ? ? ? ? 6A ? E8 ? ? ? ? 8D 84 24 ? ? ? ? 50 E8 ? ? ? ? 59");
                //injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x00, true);
                //injector::WriteMemory<uint32_t>(pattern.get_first(3), 0x00, true);
                //injector::WriteMemory<uint8_t>(pattern.get_first(8), 0x00, true);
            }

            if (bCarSpeedDependantFOV)
            {
                auto pattern = hook::pattern("C7 83 ? ? ? ? ? ? ? ? 80 7B ? ? 74");
                injector::MakeNOP(pattern.get_first(), 10, true);
                static auto Process_Cam_On_A_String_Hook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    constexpr auto DefaultFOV = 70.0f;
                    auto cam = (CCam*)regs.ebx;

                    if (cam->ResetStatics)
                    {
                        cam->FOV = DefaultFOV;
                    }
                    else if (cam->CamTargetEntity)
                    {
                        CVehicle* car = (CVehicle*)cam->CamTargetEntity;

                        float forwardSpeed = DotProduct(car->GetForward(), car->m_vecMoveSpeed);

                        if (forwardSpeed > 0.4f)
                            cam->FOV += (forwardSpeed - 0.4f) * CTimer::ms_fTimeStep;

                        if (cam->FOV > DefaultFOV)
                            cam->FOV = pow(0.98f, CTimer::ms_fTimeStep) * (cam->FOV - DefaultFOV) + DefaultFOV;

                        if (cam->FOV > DefaultFOV + 30.0f)
                            cam->FOV = DefaultFOV + 30.0f;
                    }
                });
            }

            if (bVCSCamShake)
            {
                auto pattern = hook::pattern("E8 ? ? ? ? FF B4 24 ? ? ? ? E8 ? ? ? ? 80 7B");
                hbCalculateDerivedValues.fun = injector::MakeCALL(pattern.get_first(), CalculateDerivedValues, true).get();
            }
        };
    }
} Misc;