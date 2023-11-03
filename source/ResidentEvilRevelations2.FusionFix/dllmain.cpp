#include "stdafx.h"
#include "LEDEffects.h"
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")
#include <vector>

static bool bLogiLedInitialized = false;

constexpr auto defaultAspectRatio = 16.0f / 9.0f;
float fFOVFactor = 1.0f;
int32_t ResX = 0;
int32_t ResY = 0;

enum GUI
{
    uBioGUI__cResourceParam = 0x13BF0D8,
    uBioGUISubtitles = 0x13BBC30,
    uGUIAchieveCutin = 0x13977F0,
    uGUIActionIcon = 0x1391608,
    uGUIActionIcon2 = 0x13919C8,
    uGUIAppraise = 0x1391B70,
    uGUIAppraisePopup = 0x1391CD8,
    uGUIBonusGallery = 0x1392520,
    uGUIBonusOver = 0x13927B0,
    uGUIBonusPoint = 0x1392978,
    uGUIBonusRecord = 0x1392B70,
    uGUIBonusSettings = 0x1392E98,
    uGUIChallengeMedal = 0x1393790,
    uGUICommandBase = 0x139AA40,
    uGUICommandFar = 0x139AB98,
    uGUICommandNear = 0x139ACF8,
    uGUICommonMenu = 0x13950C8,
    uGUIComposePartsRaid__cParts = 0x1395C10,
    uGUIComsoon = 0x1396080,
    uGUIContinueInfo = 0x13961D8,
    uGUICustomCutin = 0x1396390,
    uGUICustomInfo = 0x1396570,
    uGUICustomWeapon__cWeapon = 0x13968B0,
    uGUICustomWeaponRaid = 0x1396BA0,
    uGUICutin = 0x1397950,
    uGUICWRPartsInfo = 0x1396D00,
    uGUICWRPartsList__cParts = 0x1396EA8,
    uGUICWRTipsBase = 0x1397030,
    uGUICWRWeaponInfo = 0x13971A0,
    uGUICWRWeaponList__cWeapon = 0x1397400,
    uGUIDamage = 0x1397DA0,
    uGUIDamage2 = 0x1397EF0,
    uGUIDamageNum = 0x139C028,
    uGUIDead = 0x1398040,
    uGUIDifficultySelect = 0x1398478,
    uGUIEpisode = 0x1398738,
    uGUIEpisodeEnd = 0x1398880,
    uGUIEpisodeEndEx = 0x1398B08,
    uGUIEpisodeHint = 0x1398CE8,
    uGUIEpisodeLogo = 0x1398E48,
    uGUIEpisodeTitle = 0x1398FF0,
    uGUIEquip = 0x139AE78,
    uGUIEquipShortcut = 0x139B008,
    uGUIEquipShortcutRaid = 0x139B1F8,
    uGUIEventCutIn = 0x1397AA0,
    uGUIExMarking = 0x139FB88,
    uGUIFade = 0x1399138,
    uGUIFileList = 0x13998B8,
    uGUIFileText = 0x1399A00,
    uGUIFlash = 0x139B5B0,
    uGUIFood = 0x139B710,
    uGUIGesture = 0x139B860,
    uGUIGetItem = 0x139D460,
    uGUIGuide = 0x1399E68,
    uGUIHeadLineNews = 0x139A8F0,
    uGUIHeal = 0x139B9F0,
    uGUIHealNum = 0x139BB48,
    uGUIIndicatorFar = 0x139C718,
    uGUIIndicatorNear = 0x139C860,
    uGUIInventoryCampaign__cCompose = 0x139CD20,
    uGUIInventoryRaid = 0x139D2E8,
    uGUIKeyItem = 0x139D640,
    uGUILoading = 0x139D830,
    uGUILoadingKafka = 0x139D980,
    uGUILoadingShow = 0x139DC10,
    uGUIManual = 0x139DFE8,
    uGUIMap = 0x139E148,
    uGUIMapDetailBase = 0x139F2B0,
    uGUIMapIcon = 0x139F9A0,
    uGUIMessageBox = 0x1398278,
    uGUIMissionClear = 0x13A0540,
    uGUIMissionFailed = 0x13A06A0,
    uGUIMissionStart = 0x13A0970,
    uGUIMoneyGuide = 0x1399FC8,
    uGUIMouseCursor = 0x13BF3F0,
    uGUIOnlineStore = 0x139FF68,
    uGUIPurpose = 0x139BC88,
    uGUIPuzzle = 0x13A0298,
    uGUIRaidBreakIn = 0x13A0400,
    uGUIRaidBuyCutin = 0x13A0AB8,
    uGUIRaidCutIn = 0x13A0C28,
    uGUIRaidDeparture = 0x13A10C0,
    uGUIRaidEquip = 0x139B460,
    uGUIRaidFormationBase = 0x13A1788,
    uGUIRaidFormationChar = 0x13A1978,
    uGUIRaidFormationGesture = 0x13A1AE0,
    uGUIRaidFormationSkill = 0x13A1CB8,
    uGUIRaidFormationWeapon = 0x13A1E38,
    uGUIRaidGetItemPopup = 0x13A0D78,
    uGUIRaidManualList = 0x13A1F90,
    uGUIRaidReady = 0x13A14E0,
    uGUIRaidReadyNew = 0x13A1620,
    uGUIRaidSkillPopup = 0x13A0EF8,
    uGUIRaidStore = 0x13A2828,
    uGUIRaidTop = 0x13A2A30,
    uGUIRaidVoiceList = 0x13A2598,
    uGUIRanking = 0x13A26D0,
    uGUIRankingCampaign = 0x13E84E8,
    uGUIRankingOption = 0x13A2B60,
    uGUIRankingRaid = 0x13A2CD0,
    uGUIReady = 0x13A2E30,
    uGUIReBanner = 0x13975D8,
    uGUIResult = 0x13A2F78,
    uGUIResultCutin = 0x13A30C8,
    uGUIResultCutinRaid = 0x13A3220,
    uGUIResultRaid = 0x13A34B0,
    uGUIResultRaidEvent = 0x13A3618,
    uGUIReticleBase = 0x13A38C0,
    uGUIReticleNatalia = 0x13A3A68,
    uGUIReticleScope = 0x13A3BC8,
    uGUIReticleThrow = 0x13A3D00,
    uGUISaving = 0x139DD80,
    uGUISearch = 0x13BAB38,
    uGUISessionCreat = 0x13A20D8,
    uGUISessionList = 0x13A2278,
    uGUISessionSearch = 0x13A2430,
    uGUISkill = 0x13BB710,
    uGUISkillCoolDown = 0x139BDF0,
    uGUIStaffRoll = 0x13BB8B8,
    uGUIStoreInfomation = 0x13A0128,
    uGUITextVoice = 0x13BEF88,
    uGUIThanks = 0x13BE7F0,
    uGUIThumbnail = 0x13BE938,
    uGUITimer = 0x13BEA88,
    uGUITitleTop = 0x13BEBD8,
    uGUITutorial = 0x13BEE38,
    uGUIVitalityEnemy = 0x139C178,
    uGUIVitalityMe = 0x139C2E0,
    uGUIVitalityPartner = 0x139C430,
    uGUIVoiceChatName = 0x13BF6E0,
    uGUIWeakPoint = 0x13BF848,
    VirtualScreen = 0x141AC10
};

bool IsSplitScreenActive()
{
    auto ptr = *(uint32_t*)0x157AE00;
    return ptr && *(uint32_t*)(ptr + 0x8F4) == 1;
}

int32_t GetResX()
{
    if (ResX)
        return ResX;
    else
        return *(int32_t*)(*(uint32_t*)0x15DE88C + 0x1E0);
}

int32_t GetResY()
{
    if (ResY)
        return ResY;
    else
        return *(int32_t*)(*(uint32_t*)0x15DE88C + 0x1E4);
}

int32_t GetRelativeResX()
{
    return 1280;
}

int32_t GetRelativeResY()
{
    return 720;
}

int32_t GetCurrentSplitScreenResX()
{
    return *(int32_t*)(*(uint32_t*)0x15DE88C + 0x50);
}

int32_t GetCurrentSplitScreenResY()
{
    return *(int32_t*)(*(uint32_t*)0x15DE88C + 0x54);
}

int32_t GetNativeSplitScreenResX()
{
    return 936 - 136;
}

int32_t GetNativeSplitScreenResY()
{
    return 360;
}

float GetAspectRatio()
{
    return (float)GetResX() / (float)GetResY();
}

float GetDiff()
{
    if (IsSplitScreenActive())
    {
        static constexpr float fDiffSplitScreen = (1280.0f / (936.0f - 136.0f));
        return (GetAspectRatio() / defaultAspectRatio) * fDiffSplitScreen;
    }
    else
    {
        return GetAspectRatio() / defaultAspectRatio;
    }
}

void __fastcall sub_96C410(int _this, int edx, int a2, int a3)
{
    int32_t v4 = 0, v5 = 0, v6 = 0, v7 = 0;
    auto dword_15DE88C = *(uint32_t*)0x15DE88C;

    if (*(uint8_t*)(_this + 144))
    {
        v4 = *(int32_t*)(dword_15DE88C + 3248);
        v5 = *(int32_t*)(dword_15DE88C + 3252);
        v6 = *(int32_t*)(dword_15DE88C + 3256);
        v7 = *(int32_t*)(dword_15DE88C + 3260);
    }
    else
    {
        auto v8 = 400 * *(uint32_t*)(_this + 140);
        v4 = *(int32_t*)(v8 + dword_15DE88C + 72);
        v5 = *(int32_t*)(v8 + dword_15DE88C + 76);
        v6 = *(int32_t*)(v8 + dword_15DE88C + 80);
        v7 = *(int32_t*)(v8 + dword_15DE88C + 84);
    }

    auto v9 = (float)(v7 - v5) * 0.0013888889f;
    auto v10 = (float)(v6 - v4) * 0.00078125001f;
    if (v10 <= v9)
        v9 = v10;

    if (IsSplitScreenActive())
        a2 += (int32_t)(720.0f * GetAspectRatio()) - (1280.0f / (1280.0f / GetNativeSplitScreenResX()));
    else
        a2 += (int32_t)(((720.0f * GetAspectRatio()) - 1280.0f) / 2.0f);

    *(int32_t*)(_this + 164) = (int32_t)(((float)a2 * v9) + (float)v4);
    *(int32_t*)(_this + 168) = (int32_t)(((float)a3 * v9) + (float)v5);
}

enum
{
    RESCALE = 0xAAAAEEEE,
    STRETCH = 0xBBBBFFFF,
    OFFSET = 0xCCCCDDDD
};

void __fastcall sub_E18040(int _this, int edx, int a2)
{
    char* v2; // esi
    float v4; // xmm5_4
    float v5; // xmm6_4
    float v6; // xmm7_4
    float v7; // xmm0_4
    int v8; // ecx
    uint32_t* v9; // eax
    int v10; // edi
    int v11; // ecx
    int v12; // ecx
    float* v13; // edx
    float v14; // xmm4_4
    float v15; // xmm3_4
    int v16; // eax
    int v17; // edi
    float v18; // xmm0_4
    int v19; // [esp+8h] [ebp-3Ch]
    float v20; // [esp+8h] [ebp-3Ch]
    int v21; // [esp+Ch] [ebp-38h]
    int v22; // [esp+10h] [ebp-34h]
    int v23; // [esp+14h] [ebp-30h]
    int v24; // [esp+18h] [ebp-2Ch]
    int v25; // [esp+18h] [ebp-2Ch]
    float v26; // [esp+1Ch] [ebp-28h]
    float v27; // [esp+28h] [ebp-1Ch]
    int v28; // [esp+2Ch] [ebp-18h]
    int v29; // [esp+30h] [ebp-14h]

    auto sub_D7A340 = (int(__fastcall*) (char*, int, int))0xD7A340;
    auto sub_D79650 = (float(__fastcall*) (char*, int))0xD79650;
    auto sub_D79630 = (float(__fastcall*) (char*, int))0xD79630;
    auto sub_E67E20 = (void(__fastcall*) (uint32_t*, int, int))0xE67E20;

    v2 = *(char**)(a2 + 4);
    v19 = *(uint32_t*)0x15DDFD8;
    v21 = *(uint32_t*)0x15DDFDC + *((uint32_t*)v2 + 50) - *((uint32_t*)v2 + 48);
    v29 = *(uint32_t*)0x15DDFDC;
    v24 = *(uint32_t*)0x15DDFDC;
    v28 = *(uint32_t*)0x15DDFD8 + *((uint32_t*)v2 + 49) - *((uint32_t*)v2 + 47);
    v4 = *(float*)(_this + 0x60) * *(float*)&*(uint32_t*)0x153C628;
    v5 = *(float*)(_this + 100) * *(float*)&*(uint32_t*)0x153C62C;
    v6 = *(float*)(_this + 64);
    v7 = *(float*)(_this + 68);
    v22 = *(uint32_t*)0x15DDFD8;
    v8 = v28;
    v23 = v21;
    v9 = *(uint32_t**)(_this + 240);
    v10 = 2;
    v26 = v4;
    v27 = v5;
    if (v9)
    {
        v8 = v9[30];
        v10 = (v9[28] >> 1) & 3;
        v23 = v9[31];
        v22 = 0;
        v24 = 0;
    }
    if ((*(uint32_t*)(_this + 328) & 4) != 0)
    {
        v4 = (float)((float)(*((uint32_t*)v2 + 49) - *((uint32_t*)v2 + 47)) / (float)(v8 - v22)) * v4;
        v26 = v4;
        v27 = (float)((float)(*((uint32_t*)v2 + 50) - *((uint32_t*)v2 + 48)) / (float)(v23 - v24)) * v5;
        v5 = v27;
    }
    *((uint32_t*)v2 + 19) |= 2u;
    if (v2[76] < 0)
    {
        *(uint32_t*)&v2[12 * *((uint32_t*)v2 + 23) + 199884] = *(uint32_t*)&v2[12 * *((uint32_t*)v2 + 23) + 199884] & 0x80000000 | 0x2C4;
        *(uint32_t*)&v2[12 * *((uint32_t*)v2 + 23) + 199884] &= ~0x80000000;
        v11 = 3 * *((uint32_t*)v2 + 23);
        *(uint32_t*)&v2[4 * v11 + 199888] = *((uint32_t*)v2 + 1542);
        *(uint32_t*)&v2[4 * v11 + 199892] = *((uint32_t*)v2 + 1543);
        ++*((uint32_t*)v2 + 23);
    }
    v12 = (4 * *(uint16_t*)(*(uint32_t*)(*((uint32_t*)v2 + 2) + 2832) + 24) + 15) & 0xFFFFFFF0;
    v25 = v12;
    if ((unsigned int)(v12 + *((uint32_t*)v2 + 5)) > *((uint32_t*)v2 + 6))
    {
        sub_D7A340(v2, edx, v12);
        v4 = v26;
        v5 = v27;
        v12 = v25;
    }
    v13 = (float*)*((uint32_t*)v2 + 5);
    *(char**)((uint32_t*)v2 + 5) = (char*)v13 + v12;
    *(float**)((uint32_t*)v2 + 1542) = v13;
    *((uint32_t*)v2 + 81) |= 1u;
    if (v13)
    {
        v14 = 2.0 / (float)(v28 - v19);
        v15 = -2.0 / (float)(v21 - v29);
        v13[0] = v14 * v4;
        v13[1] = v15 * v5;
        v13[2] = (float)(v14 * v6) - 1.0;
        v13[3] = (float)(v15 * v7) + 1.0;
        *((uint32_t*)v2 + 19) &= ~2u;

        switch (edx)
        {
        case RESCALE:
        {
            if (IsSplitScreenActive())
            {
                if (v21 == GetCurrentSplitScreenResY() || v21 == (GetCurrentSplitScreenResY() + 1))
                {
                    v14 = 2.0 / (float)(v28 - v19);
                    v15 = -2.0 / (float)(v21 - v29);
                    v13[0] = v14 * v4;
                    v13[1] = v15 * v5;
                    v13[2] = (float)(v14 * v6) - (1.0f / GetDiff());
                    v13[3] = (float)(v15 * v7) + 1.0;
                }
                else if (v21 == GetResY())
                {
                    v14 = 2.0 / (float)(v28 - v19);
                    v15 = -2.0 / (float)(v21 - v29);
                    v13[0] = v14 * v4;
                    v13[1] = v15 * v5;
                    v13[2] = (float)(v14 * v6) - (1.0f / (GetAspectRatio() / defaultAspectRatio));
                    v13[3] = (float)(v15 * v7) + 1.0;
                }
            }
            else
            {
                v14 = 2.0 / (float)(v28 - v19);
                v15 = -2.0 / (float)(v21 - v29);
                v13[0] = v14 * v4;
                v13[1] = v15 * v5;
                v13[2] = (float)(v14 * v6) - (1.0f / GetDiff());
                v13[3] = (float)(v15 * v7) + 1.0;
            }
        }
        break;
        case STRETCH:
        {
            v14 = (2.0 * GetDiff()) / (float)(v28 - v19);
            v15 = -2.0 / (float)(v21 - v29);
            v13[0] = v14 * v4;
            v13[1] = v15 * v5;
            v13[2] = -1.0f; //(float)(v14 * v6) - fDiffInv;
            v13[3] = (float)(v15 * v7) + 1.0;
        }
        break;
        case OFFSET:
        {
            if (IsSplitScreenActive())
            {
                if (v21 == GetCurrentSplitScreenResY() || v21 == (GetCurrentSplitScreenResY() + 1))
                {
                    v14 = 2.0 / (float)(v28 - v19);
                    v15 = -2.0 / (float)(v21 - v29);
                    v13[0] = v14 * v4;
                    v13[1] = v15 * v5;
                    v13[2] = (float)(v14 * v6) - (1.0f / GetDiff());
                    v13[3] = (float)(v15 * v7) + 1.0;
                }
                else if (v21 == GetResY())
                {
                    v14 = 2.0 / (float)(v28 - v19);
                    v15 = -2.0 / (float)(v21 - v29);
                    v13[0] = v14 * v4;
                    v13[1] = v15 * v5;
                    v13[2] = (float)(v14 * v6) - (1.0f / (GetAspectRatio() / defaultAspectRatio));
                    v13[3] = (float)(v15 * v7) + 1.0;
                }
            }
            else
            {
                if (v21 == GetResY())
                {
                    v14 = 2.0 / (float)(v28 - v19);
                    v15 = -2.0 / (float)(v21 - v29);
                    v13[0] = v14 * v4;
                    v13[1] = v15 * v5;
                    v13[2] = (float)(v14 * v6) - (1.0f / GetDiff());
                    v13[3] = (float)(v15 * v7) + 1.0;
                }
            }
        }
        break;
        default:
            break;
        }
    }
    v16 = a2;
    *(float*)(a2 + 184) = v6;
    *(float*)(a2 + 188) = v7;
    *(float*)(a2 + 192) = v4;
    *(float*)(a2 + 196) = v5;
    v17 = v10 - 1;
    if (!v17)
    {
        v20 = sub_D79650(v2, edx);
    LABEL_18:
        v18 = v20;
        goto LABEL_19;
    }
    if (v17 != 1)
    {
        v18 = 0.0;
        goto LABEL_20;
    }
    v20 = sub_D79630(v2, edx);
    if ((uint8_t) * ((uint32_t*)v2 + 86) < 8u
        && !*(uint32_t*)(400 * (uint8_t) * ((uint32_t*)v2 + 86) + *(uint32_t*)0x15DE88C + 52))
    {
        goto LABEL_18;
    }
    v18 = v20 + 0.25;
LABEL_19:
    v16 = a2;
LABEL_20:
    *(float*)(v16 + 96) = v18;
    if ((*(uint8_t*)(_this + 328) & 1) != 0)
    {
        if (*(uint32_t*)(_this + 244))
        {
            *((uint32_t*)v2 + 85) = *((uint32_t*)v2 + 85) & 0x1F ^ (32
                * (*(uint32_t*)(_this + 332) + (*(uint32_t*)(_this + 336) << 20)));
            sub_E67E20(*(uint32_t**)(_this + 244), edx, a2);
        }
    }
}

void __fastcall sub_E18040_nop(int _this, int edx, int a2)
{
    return;
}

void __fastcall sub_E18040_rescale(int _this, int edx, int a2)
{
    return sub_E18040(_this, RESCALE, a2);
}

void __fastcall sub_E18040_stretch(int _this, int edx, int a2)
{
    return sub_E18040(_this, STRETCH, a2);
}

void __fastcall sub_E18040_offset(int _this, int edx, int a2)
{
    return sub_E18040(_this, OFFSET, a2);
}

void __fastcall sub_B82960(void* _this, void* edx, float a2, float a3, float a4, float a5)
{
    a4 *= fFOVFactor;
    a2 /= fFOVFactor;
    a2 /= GetDiff();
    return injector::fastcall<void(void*, void*, float, float, float, float)>::call(0xB82960, _this, edx, a2, a3, a4, a5);
}

//IDirect3DVertexShader9* shader_4F0EE939 = nullptr;
IDirect3DVertexShader9* __stdcall CreateVertexShaderHook(const DWORD** a1)
{
    if (!a1)
        return nullptr;

    auto pDevice = (IDirect3DDevice9*)*((uint32_t*)*(uint32_t*)0x15E0388 + 0x26);
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
                "def c7, 1.8, 0.28125, 0, 0\n" // 1.8 instead of 1.6 to cover the gaps in ultra wide
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
                "mov r0.xy, v0\n"
                "mul r0.xy, r0, c2\n"
                "mad r0.xy, r0, c6.x, c6.yzzw\n"
                "slt r0.z, v1.w, c6.w\n"
                "add r0.z, -r0.z, c4.x\n"
                "rcp r10.x, c2.x\n"
                "mul r10.x, c2.y, r10.x\n"
                "mul r10.x, r10.x, c7.y\n"
                "mul r0.x, r0.x, r10.x\n"
                "mul o0.x, r0.x, c7.x\n"
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
                    pShader->Release();
                    return shader;
                }
            }
        }
    }

    return pShader;
}

void __stdcall SplitScreenSetupTop(void* a1, int32_t* a2)
{
    a2[0] = 0;
    a2[1] = 0;
    a2[2] = (int32_t)(720.0f * GetAspectRatio());
    a2[3] = (int32_t)(720.0f / 2.0f);
    return injector::stdcall<void(void*, int32_t*)>::call(0x4AC310, a1, a2);
}

void __stdcall SplitScreenSetupBottom(void* a1, int32_t* a2)
{
    a2[0] = 0;
    a2[1] = (int32_t)(720.0f / 2.0f);
    a2[2] = (int32_t)(720.0f * GetAspectRatio());
    a2[3] = (int32_t)(720.0f);
    return injector::stdcall<void(void*, int32_t*)>::call(0x4AC310, a1, a2);
}

void Init()
{
    CIniReader iniReader("");
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
    auto bDisableDamageOverlay = iniReader.ReadInteger("MAIN", "DisableDamageOverlay", 1) != 0;
    auto bDisableFilmGrain = iniReader.ReadInteger("MAIN", "DisableFilmGrain", 1) != 0;
    auto bDisableFade = iniReader.ReadInteger("MAIN", "DisableFade", 0) != 0;
    auto bDisableGUICommandFar = iniReader.ReadInteger("MAIN", "DisableGUICommandFar", 0) != 0;
    fFOVFactor= iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    if (fFOVFactor <= 0.0f) fFOVFactor = 1.0f;
    
    if (bSkipIntro)
    {
        injector::WriteMemory<uint8_t>(0xA62A74, 0xEB, true);
        injector::WriteMemory<uint16_t>(0xA62A9D, 0x9090, true);
    }

    // unrestrict resolutons
    injector::MakeNOP(0xCC63CA, 2);
    injector::MakeNOP(0xCC63D1, 2);

    // overwriting aspect ratio
    hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 8E").for_each_result([](hook::pattern_match match)
    {
        struct hook_ecx_edx { void operator()(injector::reg_pack& regs) { 
            ResX = regs.ecx;
            ResY = regs.edx;

            if (((float)ResX / (float)ResY) < defaultAspectRatio)
            {
                ResY = 9 * ResX / 16;
                regs.edx = ResY;
            }
        } };
        injector::MakeInline<hook_ecx_edx>(match.get<void>(0), match.get<void>(12));
    });

    hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 9E").for_each_result([](hook::pattern_match match)
    {
        struct hook_ebx_edi { void operator()(injector::reg_pack& regs) {
            ResX = regs.ebx;
            ResY = regs.edi;

            if (((float)ResX / (float)ResY) < defaultAspectRatio)
            {
                ResY = 9 * ResX / 16;
                regs.edi = ResY;
            }
        } };
        injector::MakeInline<hook_ebx_edi>(match.get<void>(0), match.get<void>(12));
    });

    // movies fix for ultra wide
    // ?

    // split screen windows dimensions
    injector::MakeCALL(0x4AE6E8, SplitScreenSetupTop, true);
    injector::MakeCALL(0x4AE732, SplitScreenSetupBottom, true);

    // GUI
    injector::MakeJMP(0xE18040, sub_E18040_rescale, true);

    injector::WriteMemory(uGUIFade, sub_E18040_stretch, true);
    injector::WriteMemory(uGUICommandBase, sub_E18040, true);
    injector::WriteMemory(uGUICommandFar, sub_E18040_offset, true);
    injector::WriteMemory(uGUICommandNear, sub_E18040, true);

    if (bDisableDamageOverlay)
    {
        injector::WriteMemory(uGUIDamage, sub_E18040_nop, true);
        injector::WriteMemory(uGUIDamage2, sub_E18040_nop, true);
    }
    else
    {
        injector::WriteMemory(uGUIDamage, sub_E18040_stretch, true);
        injector::WriteMemory(uGUIDamage2, sub_E18040_stretch, true);
    }

    //Camera near clip fix
    injector::MakeCALL(0x4B17CA,  sub_B82960, true);
    injector::MakeCALL(0x4B6F67,  sub_B82960, true);
    injector::MakeCALL(0x9D543D,  sub_B82960, true);
    injector::MakeCALL(0xC89DE9,  sub_B82960, true);
    injector::MakeCALL(0xCA97D5,  sub_B82960, true);
    injector::MakeCALL(0xDFEA3A,  sub_B82960, true);
    injector::MakeCALL(0xE0C9ED,  sub_B82960, true);
    injector::MakeCALL(0xE720D2,  sub_B82960, true);
    injector::MakeCALL(0xF0A652,  sub_B82960, true);
    injector::MakeCALL(0xF28D7B,  sub_B82960, true);
    injector::MakeCALL(0xF4E190,  sub_B82960, true);
    injector::MakeCALL(0x102F9D9, sub_B82960, true);

    //3d items in the inventory
    injector::MakeCALL(0x8931DE, sub_96C410, true);
    injector::MakeCALL(0x8C1B3E, sub_96C410, true);
    injector::MakeCALL(0x8CEB99, sub_96C410, true);
    injector::MakeCALL(0x8F7A29, sub_96C410, true);
    injector::MakeCALL(0x8FA254, sub_96C410, true);
    injector::MakeCALL(0x8FA2E5, sub_96C410, true);
    injector::MakeCALL(0x93D63C, sub_96C410, true);

    //disable shader overlays (don't scale to fullscreen)
    {
        injector::MakeCALL(0xFFB9E2, CreateVertexShaderHook, true);

        //struct SetVertexShaderHook
        //{
        //    void operator()(injector::reg_pack& regs)
        //    {
        //        if (IsSplitScreenActive() || GetDiff() > 1.0f)
        //        {
        //            auto pShader = (IDirect3DVertexShader9*)regs.ecx;
        //            if (pShader == shader_4F0EE939)
        //            {
        //                regs.ecx = 0;
        //            }
        //        }
        //        *(uint32_t*)(regs.ebx + 0x24) = regs.ecx;
        //        regs.eax = *(uint32_t*)(regs.esi + 0x0);
        //    }
        //}; injector::MakeInline<SetVertexShaderHook>(0xCCD0A4);
    }

    if (bDisableFilmGrain)
    {
        injector::WriteMemory<uint8_t>(0x0141664C, 'r', true);
        injector::WriteMemory<uint8_t>(0x01416668, 'r', true);
    }

    if (bDisableFade)
    {
        injector::WriteMemory(uGUIFade, sub_E18040_nop, true);
    }

    if (bDisableGUICommandFar)
    {
        injector::WriteMemory(uGUICommandFar, sub_E18040_nop, true);
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

    //Episode 4 finale stuck controls fix
    {
        injector::WriteMemory<uint8_t>(0xC21007 + 1, 0x79, true);
    }

    {
        injector::WriteMemory(0xA97206 + 4, 1000, true); //max fps
        injector::WriteMemory(0xA9796C + 4, 1000, true); //max fps
    }

    {
        bLogiLedInitialized = LogiLedInit();

        if (bLogiLedInitialized)
        {
            static auto sPlayerPtr = *hook::get_pattern<void*>("8B 0D ? ? ? ? 56 E8 ? ? ? ? 85 C0 74 7B 8B C8", 2);

            std::thread t([]()
            {
                while (true)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    if (bLogiLedInitialized)
                    {
                        if (sPlayerPtr)
                        {
                            static auto sub_6E6A70 = [](int* _this) -> void*
                            {
                                if (!_this)
                                    return nullptr;

                                auto j = 0;
                                for (auto i = _this + 8; !*i || *(DWORD*)(*i + 0x7920); ++i)
                                {
                                    if (++j >= 8)
                                        return nullptr;
                                }
                                return (void*)_this[j + 8];
                            };
                            auto pPlayerPtr = sub_6E6A70(*(int**)sPlayerPtr);

                            if (pPlayerPtr)
                            {
                                auto Player1Health = PtrWalkthrough<int32_t>(&pPlayerPtr, 0x1A08);
                                auto Player2Health = PtrWalkthrough<int32_t>(&pPlayerPtr, 0x1A0C);

                                if (Player1Health && Player2Health)
                                {
                                    auto health1 = *Player1Health;
                                    auto health2 = *Player2Health;
                                    if (health1 > 1)
                                    {
                                        if (health1 <= 250) {
                                            LEDEffects::SetLightingLeftSide(26, 4, 4, true, false); //red
                                            LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                        }
                                        else if (health1 <= 350) {
                                            LEDEffects::SetLightingLeftSide(50, 30, 4, true, false); //orange
                                            LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                                        }
                                        else {
                                            LEDEffects::SetLightingLeftSide(10, 30, 4, true, false);  //green
                                            LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                                        }
                                    }
                                    else
                                    {
                                        LEDEffects::SetLightingLeftSide(26, 4, 4, false, true); //red
                                        LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                                    }

                                    if (health2 > 1)
                                    {
                                        if (health2 <= 250) {
                                            LEDEffects::SetLightingRightSide(26, 4, 4, true, false); //red
                                            LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0); //red
                                        }
                                        else if (health2 <= 350) {
                                            LEDEffects::SetLightingRightSide(50, 30, 4, true, false); //orange
                                            LEDEffects::DrawCardiogramNumpad(67, 0, 0, 0, 0, 0); //orange
                                        }
                                        else {
                                            LEDEffects::SetLightingRightSide(10, 30, 4, true, false);  //green
                                            LEDEffects::DrawCardiogramNumpad(0, 100, 0, 0, 0, 0); //green
                                        }
                                    }
                                    else
                                    {
                                        LEDEffects::SetLightingRightSide(26, 4, 4, false, true); //red
                                        LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0, true);
                                    }
                                }
                                else
                                {
                                    LogiLedStopEffects();
                                    LEDEffects::SetLighting(90, 36, 3);
                                }
                            }
                            else
                            {
                                LogiLedStopEffects();
                                LEDEffects::SetLighting(90, 36, 3);
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
        CallbackHandler::RegisterCallback(Init, hook::pattern("F3 0F 5C 15 ? ? ? ? F3 0F 59 E5"));
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
