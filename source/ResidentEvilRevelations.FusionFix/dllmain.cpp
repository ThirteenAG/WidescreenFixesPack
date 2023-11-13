#include "stdafx.h"
#include "LEDEffects.h"
#include <d3d9.h>
#include <vector>

constexpr auto defaultAspectRatio = 16.0f / 9.0f;
float fFOVFactor = 1.0f;
int32_t ResX = 0;
int32_t ResY = 0;

enum GUI
{
    uGUI_ActionIcon = 0x10846C0,
    uGUI_ActionIconBlur = 0x10844F8,
    uGUI_CharaSelect = 0x1088DF8,
    uGUI_CheckFingerprint = 0x1089668,
    uGUI_CoopDeriveBase = 0x1092618,
    uGUI_CoopEnemyHP = 0x1084998,
    uGUI_CutInMyBag = 0x1089958,
    uGUI_CutInReNet = 0x10874D0,
    uGUI_CutInRescue = 0x1087638,
    uGUI_CutInRevel = 0x10873A8,
    uGUI_CutInReward = 0x108E7B8,
    uGUI_CutInWarning = 0x1087738,
    uGUI_Damage = 0x1084CF0,
    uGUI_DamageChoke = 0x1084DF8,
    uGUI_DataSlot = 0x108EEF8,
    uGUI_EpisodeEnding = 0x1083E50,
    uGUI_EpisodeLogo = 0x1083FE0,
    uGUI_EpisodeStartTelop = 0x1084260,
    uGUI_EpisodeTitle = 0x1084378,
    uGUI_FloatIcon = 0x1084808,
    uGUI_GadgetScanner = 0x1085148,
    uGUI_GadgetStinger = 0x10852E0,
    uGUI_GadgetTurretGauge = 0x1085418,
    uGUI_GameOverHD = 0x10893C0,
    uGUI_GeneralMenu = 0x10901D8,
    uGUI_GoalHint = 0x1085D78,
    uGUI_GoalHintBlur = 0x1085E88,
    uGUI_Guide = 0x1087878,
    uGUI_Heal = 0x1084FA8,
    uGUI_Info = 0x10904E8,
    uGUI_InGameFile = 0x1085FB8,
    uGUI_ItemCursor = 0x108B3C0,
    uGUI_ItemWin = 0x1086100,
    uGUI_ItemWinBlur = 0x1086210,
    uGUI_ListParts = 0x1089BB8,
    uGUI_MainEquipWin = 0x1085568,
    uGUI_MainEquipWinBlur = 0x10856A0,
    uGUI_Map = 0x1086868,
    uGUI_MapBaseAndHerb = 0x10869D0,
    uGUI_MapBaseAndHerbBlur = 0x1086AE0,
    uGUI_MessageBoxBase = 0x1083AF0,
    uGUI_MyBagHD = 0x108B638,
    uGUI_NowLoadingHD = 0x1083CC8,
    uGUI_OneLineMes = 0x1087A00,
    uGUI_OptionMenu = 0x108D2B8,
    uGUI_OptionTop = 0x108DA98,
    uGUI_ParamParts = 0x1089D30,
    uGUI_ParamParts2 = 0x1089E90,
    uGUI_ParamWp = 0x108A0B0,
    uGUI_PartnerWarning = 0x1084B08,
    uGUI_PauseBlurHD = 0x108DCE8,
    uGUI_PauseHD = 0x108DE18,
    uGUI_PlasmaPuzzle = 0x108E480,
    uGUI_RaidPl = 0x1087B68,
    uGUI_Result = 0x108EB00,
    uGUI_Reticle = 0x10857C0,
    uGUI_SavingForOutGame = 0x108F0A8,
    uGUI_Scope = 0x1085968,
    uGUI_SessionList = 0x10890C8,
    uGUI_Shop = 0x108CA00,
    uGUI_ShopParamItems = 0x108AF18,
    uGUI_StaffRoll = 0x108F1F8,
    uGUI_StageSelect = 0x1089210,
    uGUI_SubEquipWin = 0x1085A80,
    uGUI_SubEquipWinBlur = 0x1085BF0,
    uGUI_SubMenu = 0x1086DE0,
    uGUI_SubMenuBase = 0x1086FE8,
    uGUI_SwitchBoardPuzzle = 0x108E630,
    uGUI_Telop = 0x108F370,
    uGUI_Telop2 = 0x108F460,
    uGUI_Telop2Blur = 0x108F568,
    uGUI_TelopBlur = 0x108F678,
    uGUI_TelopChangeControl = 0x108F788,
    uGUI_TimeCounter = 0x1086388,
    uGUI_TimeUp = 0x1089528,
    uGUI_TitleAdvanceBG = 0x10906D8,
    uGUI_TitleHD = 0x10907F0,
    uGUI_TitleStartHD = 0x1090918,
    uGUI_Transceiver = 0x10865A8,
    uGUI_VIP_RoomDoor = 0x1091398,
    uGUI_WorldMapIcon = 0x1087100,
    uGUIScheduler = 0x10D4C20,
    uGUISubtitlesFix = 0x10924B0,
};

int32_t GetResX()
{
    if (ResX)
        return ResX;
    else
        return *(int32_t*)(*(uint32_t*)0x11E7B9C + 0xB8);
}

int32_t GetResY()
{
    if (ResY)
        return ResY;
    else
        return *(int32_t*)(*(uint32_t*)0x11E7B9C + 0xBC);
}

int32_t GetRelativeResX()
{
    return 1280;
}

int32_t GetRelativeResY()
{
    return 720;
}

float GetAspectRatio()
{
    return (float)GetResX() / (float)GetResY();
}

float GetDiff()
{
    if (GetAspectRatio() >= defaultAspectRatio)
    {
        return GetAspectRatio() / defaultAspectRatio;
    }
    else
    {
        return 1.0f;
    }
}

enum
{
    RESCALE = 0xAAAAEEEE,
    OFFSET = 0xAAAFFFFF,
};

void __fastcall sub_BA1050(int _this, int edx, int a2)
{
    char* v2; // esi
    int v3; // ebx
    float v5; // xmm0_4
    int v6; // ecx
    float v7; // xmm5_4
    float v8; // xmm6_4
    int v9; // ebp
    int v10; // eax
    uint32_t* v11; // ecx
    unsigned int v12; // ebp
    int v13; // eax
    int v14; // ecx
    float* v15; // eax
    float v16; // xmm0_4
    float v17; // xmm1_4
    int v18; // ebp
    double v19; // st7
    int v20; // [esp+10h] [ebp-34h]
    float v21; // [esp+10h] [ebp-34h]
    float v22; // [esp+14h] [ebp-30h]
    float v23; // [esp+18h] [ebp-2Ch]
    float v24; // [esp+1Ch] [ebp-28h]
    float v25; // [esp+20h] [ebp-24h]
    int v26; // [esp+24h] [ebp-20h]
    int v27; // [esp+28h] [ebp-1Ch]
    int v28; // [esp+30h] [ebp-14h]
    int v29; // [esp+34h] [ebp-10h]
    int v30; // [esp+38h] [ebp-Ch]

    auto sub_B83530 = (int(__fastcall*) (char*, int, int))0xB83530;
    auto sub_B838F0 = (float(__fastcall*) (char*, int))0xB838F0;
    auto sub_B838E0 = (float(__fastcall*) (char*, int))0xB838E0;
    auto sub_B98830 = (void(__fastcall*) (uint32_t*, int, int))0xB98830;

    v2 = *(char**)(a2 + 4);
    v3 = *((uint32_t*)v2 + 49);
    v5 = *(float*)(_this + 64);
    v6 = *((uint32_t*)v2 + 50);
    v7 = *(float*)(_this + 96);
    v8 = *(float*)(_this + 100);
    v27 = *((uint32_t*)v2 + 48);
    v30 = v27;
    v24 = v5;
    v26 = *((uint32_t*)v2 + 47);
    v29 = v26;
    v9 = 2;
    v28 = v6;
    v10 = v3;
    v25 = *(float*)(_this + 68);
    v22 = v7;
    v23 = v8;
    if (*(uint32_t*)(_this + 240))
    {
        v11 = *(uint32_t**)(_this + 240);
        v12 = v11[28];
        v10 = v11[30];
        v6 = v11[31];
        v9 = (v12 >> 1) & 3;
        v29 = 0;
        v30 = 0;
    }
    if ((*(uint32_t*)(_this + 328) & 4) != 0)
    {
        v7 = v7 * (float)((float)(v3 - v26) / (float)(v10 - v29));
        v8 = v8 * (float)((float)(v28 - v27) / (float)(v6 - v30));
        v22 = v7;
        v23 = v8;
    }
    *((uint32_t*)v2 + 19) |= 2u;
    if (v2[76] < 0)
    {
        *(uint32_t*)&v2[12 * *((uint32_t*)v2 + 23) + 199836] = *(uint32_t*)&v2[12 * *((uint32_t*)v2 + 23) + 199836] & 0x80000000 | 0x2BE;
        *(uint32_t*)&v2[12 * *((uint32_t*)v2 + 23) + 199836] &= ~0x80000000;
        v13 = 3 * *((uint32_t*)v2 + 23);
        *(uint32_t*)&v2[4 * v13 + 199840] = *((uint32_t*)v2 + 1528);
        *(uint32_t*)&v2[4 * v13 + 199844] = *((uint32_t*)v2 + 1529);
        ++*((uint32_t*)v2 + 23);
    }
    v14 = (4 * *(unsigned __int16*)(*(uint32_t*)(*((uint32_t*)v2 + 2) + 2808) + 24) + 15) & 0xFFFFFFF0;
    v20 = v14;
    if ((unsigned int)(v14 + *((uint32_t*)v2 + 5)) > *((uint32_t*)v2 + 6))
    {
        sub_B83530(v2, edx, v14);
        v8 = v23;
        v7 = v22;
        v14 = v20;
    }
    v15 = (float*)*((uint32_t*)v2 + 5);
    *(char**)((uint32_t*)v2 + 5) = (char*)v15 + v14;
    *(float**)((uint32_t*)v2 + 1528) = v15;
    *((uint32_t*)v2 + 81) |= 1u;
    if (v15)
    {
        v16 = 2.0 / (float)(v3 - v26);
        v17 = -2.0 / (float)(v28 - v27);
        v15[0] = v16 * v7;
        v15[1] = v17 * v8;
        v15[2] = (float)(v16 * v24) - 1.0;
        v15[3] = (float)(v17 * v25) + 1.0;
        *((uint32_t*)v2 + 19) &= ~2u;

        switch (edx)
        {
        case RESCALE:
        {
            v16 = (2.0 / GetDiff()) / (float)(v3 - v26);
            v17 = -2.0 / (float)(v28 - v27);
            v15[0] = v16 * v7;
            v15[1] = v17 * v8;
            v15[2] = (float)(v16 * v24) - (1.0f / GetDiff());
            v15[3] = (float)(v17 * v25) + 1.0;
        }
        break;
        case OFFSET:
        {
            v16 = 2.0 / (float)(v3 - v26);
            v17 = -2.0 / (float)(v28 - v27);
            v15[0] = v16 * v7;
            v15[1] = v17 * v8;
            v15[2] = (float)(v16 * v24) - (1.0f / GetDiff());
            v15[3] = (float)(v17 * v25) + 1.0;
        }
        break;
        default:
            break;
        }
    }
    v18 = v9 - 1;
    if (v18)
    {
        if (v18 != 1)
        {
            v21 = 0.0;
            goto LABEL_17;
        }
        v19 = sub_B838F0(v2, edx);
    }
    else
    {
        v19 = sub_B838E0(v2, edx);
    }
    v21 = v19;
LABEL_17:
    *(float*)(a2 + 96) = v21;
    if ((*(uint8_t*)(_this + 328) & 1) != 0)
    {
        if (*(uint32_t*)(_this + 244))
        {
            *((uint32_t*)v2 + 85) = *((uint32_t*)v2 + 85) & 0x1F | (32
                * (*(uint32_t*)(_this + 332)
                    + ((*(uint32_t*)(_this + 336) & 0x7F) << 20)));
            sub_B98830(*(uint32_t**)(_this + 244), edx, a2);
        }
    }
}

void __cdecl sub_79BB50(int a1)
{
    uint32_t* v1; // esi
    int v2; // ecx
    int v3; // edx
    int v4; // ebx
    int v5; // ebp
    int v6; // eax
    int v7; // eax
    unsigned int v8; // edi
    float* v9; // eax
    float v10; // xmm0_4
    float v11; // xmm1_4
    int v12; // [esp+Ch] [ebp-10h]
    int v13; // [esp+10h] [ebp-Ch]
    
    auto sub_B83530 = (int(__fastcall*) (char*, int, int))0xB83530;
    auto sub_B838E0 = (float(__fastcall*) (char*, int))0xB838E0;
    
    v1 = *(uint32_t**)(a1 + 4);
    v2 = v1[47];
    v3 = v1[48];
    v4 = v1[49];
    v5 = v1[50];
    v1[19] |= 2u;
    v12 = v2;
    v13 = v3;
    if ((v1[19] & 0x80u) != 0)
    {
        v1[3 * v1[23] + 49959] = v1[3 * v1[23] + 49959] & 0x80000000 | 0x2BE;
        v1[3 * v1[23] + 49959] &= ~0x80000000;
        v6 = 3 * v1[23];
        v1[v6 + 49960] = v1[1528];
        v1[v6 + 49961] = v1[1529];
        ++v1[23];
    }
    v7 = *(uint32_t*)(v1[2] + 2808);
    v8 = (4 * *(uint16_t*)(v7 + 24) + 15) & 0xFFFFFFF0;
    if (v8 + v1[5] > v1[6])
        sub_B83530((char*)v1, 0, (4 * *(uint16_t*)(v7 + 24) + 15) & 0xFFFFFFF0);
    v9 = (float*)v1[5];
    v1[5] = (uint32_t)((char*)v9 + v8);
    v1[1528] = (uint32_t)v9;
    v1[81] |= 1u;
    if (v9)
    {
        v10 = 2.0 / (float)(v4 - v12);
        v11 = -2.0 / (float)(v5 - v13);
        *v9 = v10;
        v9[1] = v11;
        v9[2] = (float)(v10 * 0.0) - 1.0;
        v9[3] = (float)(v11 * 0.0) + 1.0;
        v1[19] &= ~2u;
    
        if (true) //RESCALE
        {
            v10 = (2.0 / GetDiff()) / (float)(v4 - v12);
            v11 = -2.0 / (float)(v5 - v13);
            v9[0] = v10;
            v9[1] = v11;
            v9[2] = (float)(v10 * 0.0) - (1.0f / GetDiff());
            v9[3] = (float)(v11 * 0.0) + 1.0;
        }
    }
    *(float*)(a1 + 96) = sub_B838E0((char*)v1, 0);
}

void __fastcall sub_BA1050_nop(int _this, int edx, int a2)
{
    return;
}

void __fastcall sub_BA1050_rescale(int _this, int edx, int a2)
{
    return sub_BA1050(_this, RESCALE, a2);
}

void __fastcall sub_BA1050_offset(int _this, int edx, int a2)
{
    return sub_BA1050(_this, OFFSET, a2);
}

void __fastcall sub_8F2230(void* _this, void* edx, float a2, float a3, float a4, float a5)
{
    a4 *= fFOVFactor;
    a2 /= GetDiff();
    return injector::fastcall<void(void*, void*, float, float, float, float)>::call(0x8F2230, _this, edx, a2, a3, a4, a5);
}

void __fastcall sub_BA4440(void* _this, int edx, int* a2)
{
    if (*(uint32_t*)(*(uint32_t*)_this + 88) == 0xBA1050)
    {
        DWORD out;
        injector::UnprotectMemory((*(DWORD*)_this + 88), 4, out);
        *(uint32_t*)(*(DWORD*)_this + 88) = (uint32_t)&sub_BA1050_offset;
    }
    return injector::fastcall<void(void*, int, int*)>::call(0xBA4440, _this, edx, a2);
}

bool bDisableCreateQuery = false;
void __fastcall gpuCommandBufferSync(IDirect3DDevice9** m_ppD3DDevice, void* edx)
{
    if (bDisableCreateQuery)
        return;

    IDirect3DQuery9* pEventQuery = NULL;
    m_ppD3DDevice[38]->CreateQuery(D3DQUERYTYPE_EVENT, &pEventQuery);

    if (pEventQuery)
    {
        pEventQuery->Issue(D3DISSUE_END);
        while (pEventQuery->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
            Sleep(0);
        pEventQuery->Release();
    }
}

void Init()
{
    CIniReader iniReader("");
    auto bBorderlessWindowed = iniReader.ReadInteger("MAIN", "BorderlessWindowed", 1) != 0;
    auto bDisableDamageOverlay = iniReader.ReadInteger("MAIN", "DisableDamageOverlay", 1) != 0;
    fFOVFactor= iniReader.ReadFloat("MAIN", "FOVFactor", 1.2f);
    if (fFOVFactor <= 0.0f) fFOVFactor = 1.0f;
    bDisableCreateQuery = iniReader.ReadInteger("MAIN", "DisableCreateQuery", 0) != 0;

    // unrestrict resolutons
    struct ResList
    {
        uint32_t stringPtr;
        uint32_t index;
    };
    static std::vector<std::string> strList;
    static std::vector<ResList> resList;
    GetResolutionsList(strList);
    for (auto i = 0; i < strList.size(); i++)
    {
        uint32_t w = 0, h = 0;
        if (sscanf_s(strList[i].c_str(), "%dx%d", &w, &h) == 2 && w >= 640 && h >= 480)
            resList.emplace_back((uint32_t)strList[i].data(), i);
    }
    injector::WriteMemory(0x8A1346 + 4, resList.data(), true);
    injector::WriteMemory(0x8A13FE + 4, resList.data(), true);
    injector::WriteMemory(0x8A134E + 4, resList.size(), true);
    injector::WriteMemory(0x8A1406 + 4, resList.size(), true);

    // overwriting aspect ratio
    hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 8E").for_each_result([](hook::pattern_match match)
    {
        struct hook_ecx_edx { void operator()(injector::reg_pack& regs) { ResX = regs.ecx; ResY = regs.edx; } };
        injector::MakeInline<hook_ecx_edx>(match.get<void>(0), match.get<void>(12));
    });

    hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 96").for_each_result([](hook::pattern_match match)
    {
        struct hook_ecx_edx { void operator()(injector::reg_pack& regs) { ResX = regs.ecx; ResY = regs.edx; } };
        injector::MakeInline<hook_ecx_edx>(match.get<void>(0), match.get<void>(12));
    });

    hook::pattern("0F 84 ? ? ? ? 48 ? ? 48 ? ? 89 AE").for_each_result([](hook::pattern_match match)
    {
        struct hook_ebp_edi { void operator()(injector::reg_pack& regs) { ResX = regs.ebp; ResY = regs.edi; } };
        injector::MakeInline<hook_ebp_edi>(match.get<void>(0), match.get<void>(12));
    });

    // movies fix for ultra wide
    struct MoviesWorkaround
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = (int32_t)((float)(*(int32_t*)(regs.edi + 0xB8)) / GetDiff());
        }
    }; injector::MakeInline<MoviesWorkaround>(0xBABE12, 0xBABE12 + 6);

    // GUI
    injector::MakeJMP(0xBA1050, sub_BA1050_rescale, true);
    injector::MakeCALL(0x7BB17A, sub_79BB50, true); // radar player blip fix

    injector::WriteMemory(0x10923AC, sub_BA4440, true);
    //injector::WriteMemory(0x10D4A04, sub_BA4440, true); // loading tips
    injector::WriteMemory(0x10D4B14, sub_BA4440, true); // main menu bg
    injector::WriteMemory(0x10D4BF4, sub_BA4440, true); // probably you're dead screen

    injector::WriteMemory(uGUI_PauseBlurHD, sub_BA1050, true);

    if (bDisableDamageOverlay)
    {
        injector::WriteMemory(uGUI_Damage, sub_BA1050_nop, true);
        injector::WriteMemory(uGUI_DamageChoke, sub_BA1050_nop, true);
    }
    else
    {
        injector::WriteMemory(uGUI_Damage, sub_BA1050, true);
        injector::WriteMemory(uGUI_DamageChoke, sub_BA1050, true);
    }

    //Camera near clip fix
    injector::MakeCALL(0x47249D, sub_8F2230, true);
    injector::MakeCALL(0x480F4E, sub_8F2230, true);
    injector::MakeCALL(0x4962E8, sub_8F2230, true);
    injector::MakeCALL(0x498AE5, sub_8F2230, true);
    injector::MakeCALL(0x5ED652, sub_8F2230, true);
    injector::MakeCALL(0x8CFFF3, sub_8F2230, true);
    injector::MakeCALL(0x9EE595, sub_8F2230, true);
    injector::MakeCALL(0xA1025D, sub_8F2230, true);
    injector::MakeCALL(0xAA52EA, sub_8F2230, true);
    injector::MakeCALL(0xBC62D4, sub_8F2230, true);
    injector::MakeCALL(0xBCC44E, sub_8F2230, true);
    injector::MakeCALL(0xC0346A, sub_8F2230, true);
    injector::MakeCALL(0xC52BF4, sub_8F2230, true);
    injector::MakeCALL(0xCDC9D9, sub_8F2230, true);
    injector::MakeCALL(0xD00F71, sub_8F2230, true);
    injector::MakeCALL(0xE98FAF, sub_8F2230, true);
    
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
        injector::WriteMemory(0x0107FF78, 1000, true); //max fps
    }

    if (bDisableCreateQuery)
    {
        auto pattern = hook::pattern("51 8B 81 ? ? ? ? 56 8B 35 ? ? ? ? 68 ? ? ? ? 8D 54 24 08");
        injector::MakeJMP(pattern.get_first(), gpuCommandBufferSync, true);
    }

    {
        static auto sPlayerPtr = *hook::get_pattern<void*>("8B 0D ? ? ? ? 85 C9 75 14 68 ? ? ? ? 6A 3E 68 ? ? ? ? E8 ? ? ? ? 83 C4 0C 8B 0D ? ? ? ? 8D 54 24 0A", 2);
        static auto dword_116F398 = *hook::get_pattern<void*>("A1 ? ? ? ? 81 C6", 1);

        LEDEffects::Inject([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (sPlayerPtr)
            {
                static auto sub_40EEE0 = [](int* _this) -> void*
                {
                    auto sub_87F4E0 = [](uint32_t* _this) -> int
                    {
                        if ((_this[61] - 1) > 1)
                            return 0;
                        else
                            return _this[62];
                    };
            
                    if (!_this)
                        return nullptr;
            
                    uint32_t* v3; // esi
                    auto v2 = _this[12];
                    if (v2 == _this[13])
                        return 0;
                    while (true)
                    {
                        v3 = *(uint32_t**)(v2 + 16);
                        if (sub_87F4E0((uint32_t*)dword_116F398) == v3[907])
                        {
                            auto v5 = v3[3] & 7;
                            if (v5 == 2 || v5 == 1)
                                break;
                        }
                        v2 = *(uint32_t*)(v2 + 8);
                        if (v2 == _this[13])
                            return 0;
                    }
                    return v3;
            
                };
                auto pPlayerPtr = sub_40EEE0(*(int**)sPlayerPtr);
            
                if (pPlayerPtr)
                {
                    auto PlayerHealth = PtrWalkthrough<float>(&pPlayerPtr, 0xE38);
            
                    if (PlayerHealth)
                    {
                        auto Health = *PlayerHealth;
                        if (Health > 0)
                        {
                            if (Health <= 400) {
                                LEDEffects::SetLighting(26, 4, 4, true); //red
                                LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                            }
                            else if (Health <= 600) {
                                LEDEffects::SetLighting(50, 30, 4, true); //orange
                                LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                            }
                            else {
                                LEDEffects::SetLighting(10, 30, 4, true);  //green
                                LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                            }
                        }
                        else
                        {
                            LEDEffects::SetLighting(26, 4, 4, false, true); //red
                            LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                        }
                    }
                    else
                    {
                        LogiLedStopEffects();
                        LEDEffects::SetLighting(76, 12, 18); //logo red
                    }
                }
                else
                {
                    LogiLedStopEffects();
                    LEDEffects::SetLighting(90, 36, 3);
                }
            }
        });
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("8B 4E 68 85 C9 75 47"));
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

    }
    return TRUE;
}
