#include "stdafx.h"
#include <d3d9.h>
#include <vector>

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

IDirect3DVertexShader9* shader_4F0EE939 = nullptr;
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
            shader_4F0EE939 = pShader;
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

        struct SetVertexShaderHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (IsSplitScreenActive() || GetDiff() > 1.0f)
                {
                    auto pShader = (IDirect3DVertexShader9*)regs.ecx;
                    if (pShader == shader_4F0EE939)
                    {
                        regs.ecx = 0;
                    }
                }
                *(uint32_t*)(regs.ebx + 0x24) = regs.ecx;
                regs.eax = *(uint32_t*)(regs.esi + 0x0);
            }
        }; injector::MakeInline<SetVertexShaderHook>(0xCCD0A4);
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
        injector::MakeNOP(0xC74C0C, 6, true);
        injector::MakeCALL(0xC74C0C, WindowedModeWrapper::CreateWindowExA_Hook, true);
        injector::MakeNOP(0xC75D67, 6, true);
        injector::MakeCALL(0xC75D67, WindowedModeWrapper::CreateWindowExW_Hook, true);
        injector::MakeNOP(0xC75EE5, 6, true);
        injector::MakeCALL(0xC75EE5, WindowedModeWrapper::CreateWindowExW_Hook, true);
        injector::MakeNOP(0xCCB096, 6, true);
        injector::MakeCALL(0xCCB096, WindowedModeWrapper::SetWindowLongA_Hook, true);
        injector::MakeNOP(0xCCB08B, 6, true);
        injector::MakeCALL(0xCCB08B, WindowedModeWrapper::AdjustWindowRect_Hook, true);
        //0xCCB0BA: needs WindowedModeWrapper::SetWindowPos_Hook here
    }

    //Episode 4 finale stuck controls fix
    {
        injector::WriteMemory<uint8_t>(0xC21007 + 1, 0x79, true);
    }

    {
        injector::WriteMemory(0xA97206 + 4, 1000, true); //max fps
        injector::WriteMemory(0xA9796C + 4, 1000, true); //max fps
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
    return TRUE;
}

uint32_t crc32_tab[] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
  0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
  0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
  0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
  0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
  0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
  0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
  0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
  0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
  0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
  0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
  0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
  0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
  0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
  0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
  0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t crc32(uint32_t crc, const void* buf, size_t size)
{
    const uint8_t* p;

    p = (uint8_t*)buf;
    crc = crc ^ ~0U;

    while (size--)
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

    return crc ^ ~0U;
}