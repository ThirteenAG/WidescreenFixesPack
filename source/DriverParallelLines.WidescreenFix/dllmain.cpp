#include "stdafx.h"

struct Screen
{
    int32_t DesktopResW;
    int32_t DesktopResH;
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fCustomFieldOfView;
    float fAspectRatio;
    float fHudOffset;
    int32_t Width43;
    float fWidth43;
    float fGameAspectRatio;
    float fCenterPos;
    int32_t* FMVStatus;
    bool bFixFMVs;
} Screen;

enum ScreenModes
{
    LETTERBOX = 0,
    UNKNOWN = 1,
    WIDE16BY9 = 2
};

void __fastcall sub_5F3627(uintptr_t _this, uint32_t edx, uintptr_t a2, uintptr_t a3)
{
    size_t j = 0;
    float v4 = 1.0f / Screen.fWidth;
    float v5 = 1.0f / Screen.fHeight;
    uintptr_t v6 = *(uintptr_t*)(a2 + 8);
    if (*(uint32_t*)a2)
    {
        do
        {
            *(float*)(a3 + 00) = *(float*)(v6 + 16);
            *(float*)(a3 + 04) = *(float*)(v6 + 20) + *(float *)(v6 + 28);
            *(float*)(a3 + 16) = *(float*)(v6 + 32);
            *(float*)(a3 + 20) = *(float*)(v6 + 44);
            *(float*)(a3 + 24) = *(float*)(v6 + 16);
            *(float*)(a3 + 28) = *(float*)(v6 + 20);
            *(float*)(a3 + 40) = *(float*)(v6 + 32);
            *(float*)(a3 + 44) = *(float*)(v6 + 36);
            *(float*)(a3 + 48) = *(float*)(v6 + 24) + *(float *)(v6 + 16);
            *(float*)(a3 + 52) = *(float*)(v6 + 20) + *(float *)(v6 + 28);
            *(float*)(a3 + 64) = *(float*)(v6 + 40);
            *(float*)(a3 + 68) = *(float*)(v6 + 44);
            *(float*)(a3 + 72) = *(float*)(v6 + 24) + *(float *)(v6 + 16);
            *(float*)(a3 + 76) = *(float*)(v6 + 20);
            *(float*)(a3 + 88) = *(float*)(v6 + 40);
            *(float*)(a3 + 92) = *(float*)(v6 + 36);

            uintptr_t v8 = a3 + 4;

            struct flt
            {
                float f1;
                float f2;
            };

            flt floats[4];

            for (size_t i = 0; i < 4; i++)
            {
                floats[i].f1 = *(float*)(v8 - 4);
                floats[i].f2 = *(float*)(v8 - 0);
                *(float*)(v8 - 4) = (((*(float*)(v8 - 4) * 2.0f) - 1.0f) - v4);
                *(float*)(v8 + 0) = ((1.0f - (*(float*)v8 * 2.0f)) - (0.0f - v5));
                uint32_t v9 = (uint32_t)(*(float*)(v6 + 8) * 255.0f);
                uint32_t v10 = (uint32_t)(*(float*)(v6 + 4) * 255.0f) & 0xFF | ((((uint32_t)(*(float*)(v6 + 12) * 255.0f) << 8) | (uint32_t)(*(float*)v6 * 255.0) & 0xFF) << 8);
                *(uint32_t*)(v8 + 8) = (uint8_t)v9 | (v10 << 8);
                *(uint32_t*)(v8 + 4) = 0;
                v8 += 24;
            }

            uintptr_t v8a = a3 + 4;
            for (size_t i = 0; i < 4; i++)
            {
                if ((floats[0].f1 == 0.0f && floats[0].f2 == 1.0f) && (floats[1].f1 == 0.0f && floats[1].f2 == 0.0f) &&
                    (floats[2].f1 == 1.0f && floats[2].f2 == 1.0f) && (floats[3].f1 == 1.0f && floats[3].f2 == 0.0f))
                {
                    if (Screen.bFixFMVs && *Screen.FMVStatus == 1)
                        *(float *)(v8a - 4) /= (Screen.fAspectRatio / Screen.fGameAspectRatio);
                }
                else
                {
                    *(float *)(v8a - 4) /= (Screen.fAspectRatio / Screen.fGameAspectRatio);
                    if (Screen.fGameAspectRatio <= ((16.0f / 9.0f) - 0.05f))
                        *(float *)(v8a - 0) *= 0.75f;
                }
                v8a += 24;
            }

            ++j;
            a3 += 96;
            v6 += 48;
        } while (j < *(uint32_t*)a2);
    }
}

float __stdcall sub_4BAA87(float a1)
{
    if (Screen.fGameAspectRatio <= ((16.0f / 9.0f) - 0.05f))
        return a1 * 0.75f;
    else
        return a1;
}

int __fastcall sub_4C5838(int* _this, void* edx, int* a2)
{
    __try
    {
        int result = 0;
        if (*_this <= 0)
            return -1;
        for (auto i = (int*)(_this[1] + 72); ; i += 21)
        {
            if (*a2 == *(i - 1) && a2[1] == *i && a2[2] == i[1])
            {
                auto v5 = a2[3];
                if (v5 == i[2] || !v5)
                    break;
            }
            if (++result >= *_this)
                return -1;
        }
        return result;
    }
    __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return -1;
    }
}

void Init()
{  
    CIniReader iniReader("");
    auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
    Screen.fCustomFieldOfView = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    Screen.bFixFMVs = iniReader.ReadInteger("MAIN", "FixFMVs", 1) != 0;
    int32_t nMinResX = iniReader.ReadInteger("MAIN", "MinResX", 0);
    int32_t nMinResY = iniReader.ReadInteger("MAIN", "MinResY", 0);

    if (bSkipIntro)
    {
        static auto bLegalScreenShown = *hook::get_pattern<uint8_t*>("38 1D ? ? ? ? 0F 85 ? ? ? ? C7 45", 2);
        auto pattern = hook::pattern("0F 84 ? ? ? ? 48 0F 84 ? ? ? ? 48 48 74 13");

        struct SkipIntroMovie
        {
            void operator()(injector::reg_pack& regs)
            {
                *bLegalScreenShown = 1;
            }
        }; injector::MakeInline<SkipIntroMovie>(pattern.get_first(0), pattern.get_first(6));
    }

    std::tie(Screen.DesktopResW, Screen.DesktopResH) = GetDesktopRes();

    if (!nMinResX && !nMinResY)
    {
        nMinResX = Screen.DesktopResW;
        nMinResY = Screen.DesktopResH;
    }
    else if (nMinResX < 0 && nMinResY < 0)
    {
        nMinResX = 0;
        nMinResY = 0;
    }

    //crash fix?
    auto pattern = hook::pattern("53 8B 19 56 33 C0 85 DB 57"); //0x4C5838
    injector::MakeJMP(pattern.get_first(0), sub_4C5838, true);

    //uncapping resolutions
    pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? FF 75 F8 BF"); //4C5A89
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 0), INT_MAX, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 5), INT_MAX, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 10), nMinResY, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 15), nMinResX, true);

    pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? FF 75 F8 B9"); //5EB9AC
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 0), INT_MAX, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 5), INT_MAX, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 10), nMinResY, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 15), nMinResX, true);

    pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 6A 00 B9"); //5E4B9B
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 0), INT_MAX, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 5), INT_MAX, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 10), nMinResY, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(1 + 15), nMinResX, true);

    //scroll through resolutions
    pattern = hook::pattern("7C D8 83 C8 FF"); //4C5875
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEBi8, true);

    //default to desktop res
    pattern = hook::pattern("C7 46 4C ? ? ? ? C7 46 50 ? ? ? ? C7 46 54 ? ? ? ? C7"); //4C5077
    injector::WriteMemory(pattern.get_first<int32_t*>(3 + 0), Screen.DesktopResW, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(3 + 7), Screen.DesktopResH, true);

    //1024x768 black screen fix
    pattern = hook::pattern("C7 45 E0 ? ? ? ? C7 45 E4 ? ? ? ? C7 45 E8 ? ? ? ? C7 45 EC 16 00 00 00"); //0x5EBA06
    injector::WriteMemory(pattern.get_first<int32_t*>(3 + 0), Screen.DesktopResW, true);
    injector::WriteMemory(pattern.get_first<int32_t*>(3 + 7), Screen.DesktopResH, true);

    static auto pGameHwnd = *hook::get_pattern<HWND*>("A1 ? ? ? ? 85 C0 8D 7E 14 89", 1); //0x933E18
    static auto pAspectRatio = *hook::get_pattern<float*>("D9 05 ? ? ? ? 53 8B 5D 08 8B 03 57 51", 2); //64B238
    static auto pFOV = *hook::get_pattern<float*>("D9 05 ? ? ? ? 8B 03 51 8B CB D9 1C 24", 2); //64B234
    static auto dword_71D4B4 = *hook::get_pattern<uintptr_t*>("A1 ? ? ? ? 8B 08 6A 00 50 FF 91 ? ? ? ? 8D 4E 18", 1);
    static auto pGameResWidth = dword_71D4B4 + 2;
    static auto pGameResHeight = dword_71D4B4 + 3;
    Screen.FMVStatus = *hook::get_pattern<int32_t*>("BE ? ? ? ? 33 C0 B9 20 01 00 00 8B FE F3 AB", 1) + 0x1D; // 0x70C8B8

    pattern = hook::pattern("89 86 6C 01 00 00 8B 45 F4"); //0x5E4C55
    struct SetWindowedResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esi + 0x16C) = Screen.DesktopResW;
            *(uint32_t*)(regs.esi + 0x170) = Screen.DesktopResH;
            SetWindowPos(*pGameHwnd, NULL, 0, 0, Screen.DesktopResW, Screen.DesktopResH, SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }; injector::MakeInline<SetWindowedResHook>(pattern.get_first(0), pattern.get_first(15));

    static auto GetRes = []()
    {
        Screen.Width = *pGameResWidth;
        Screen.Height = *pGameResHeight;
        Screen.fWidth = static_cast<float>(Screen.Width);
        Screen.fHeight = static_cast<float>(Screen.Height);
        Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
        Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
        Screen.fWidth43 = static_cast<float>(Screen.Width43);
        Screen.fCenterPos = ((480.0f * Screen.fAspectRatio) / 2.0f);

        injector::WriteMemory<float>(pAspectRatio, Screen.fAspectRatio, true); //*pAspectRatio = Screen.fAspectRatio;
        injector::WriteMemory<float>(pFOV, AdjustFOV(1.04f, Screen.fAspectRatio) * (Screen.fCustomFieldOfView ? Screen.fCustomFieldOfView : 1.0f), true);

        if (Screen.fAspectRatio >= ((16.0f / 9.0f) - 0.05f))
            Screen.fGameAspectRatio = (16.0f / 9.0f);
        else
            Screen.fGameAspectRatio = (4.0f / 3.0f);

        static tagRECT REKT;
        REKT.left = (LONG)(((float)Screen.DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
        REKT.top = (LONG)(((float)Screen.DesktopResH / 2.0f) - (Screen.fHeight / 2.0f));
        REKT.right = (LONG)Screen.Width;
        REKT.bottom = (LONG)Screen.Height;
        SetWindowPos(*pGameHwnd, NULL, REKT.left, REKT.top, REKT.right, REKT.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
    };

    pattern = hook::pattern("C6 01 01 33 C0 C2 0C 00"); //0x5E4A6A
    struct GetResHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            GetRes();
            *(uint8_t*)regs.ecx = 1;
            regs.eax = 0;
        }
    }; injector::MakeInline<GetResHook1>(pattern.get_first(0));

    pattern = hook::pattern("89 8E 8C 01 00 00"); //0x5E4C73
    struct GetResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esi + 0x18C) = regs.ecx;
            GetRes();
        }
    }; injector::MakeInline<GetResHook2>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("8B 44 24 04 89 41 10 B0 01 C2 04 00"); //0x57B954
    struct MenuAspectRatioSwitchHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ecx + 0x10) = WIDE16BY9; //*(uint32_t*)(regs.esp + 0x04);
        }
    }; injector::MakeInline<MenuAspectRatioSwitchHook>(pattern.get_first(0), pattern.get_first(7));

    //2D
    pattern = hook::pattern("E8 ? ? ? ? 8B 45 08 8B 4D 0C 89 45 E8 8B"); //0x5F75AD
    injector::MakeCALL(pattern.get_first(0), sub_5F3627, true);

    //Radar
    static auto GetRadarPosition = [](injector::reg_pack& regs) -> float
    {
        float fVar1 = (480.0f * Screen.fGameAspectRatio);
        float fVar2 = (480.0f * Screen.fAspectRatio);
        float fOriginalPos = *(float*)(regs.eax + 0x10) + 0.00078125f;
        fOriginalPos *= fVar1; // 692px
        float offset = fOriginalPos - (fVar1 / 2.0f); //265px
        offset += Screen.fCenterPos;
        offset /= fVar2;
        return offset;
    };

    static auto GetRadarScale = [](injector::reg_pack& regs) -> float
    {
        return ((*(float *)(regs.eax + 0x18) - (1.0f / 640.0f / 2.0f)) / (Screen.fAspectRatio / Screen.fGameAspectRatio));
    };

    struct RadarPosHookEBX
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.ebx + 0x744) = GetRadarPosition(regs);
        }
    };

    struct RadarPosHookESI
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esi + 0x744) = GetRadarPosition(regs);
        }
    };

    struct RadarScaleHookEBX
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.ebx + 0x74C) = GetRadarScale(regs);

            if (Screen.fGameAspectRatio <= ((16.0f / 9.0f) - 0.05f))
                *(float*)(regs.ebx + 0x748) *= 0.936f;
        }
    };

    struct RadarScaleHookESI
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esi + 0x74C) = GetRadarScale(regs);

            if (Screen.fGameAspectRatio <= ((16.0f / 9.0f) - 0.05f))
                *(float*)(regs.esi + 0x748) *= 0.936f;
        }
    };

    pattern = hook::pattern("F3 0F 11 ? 44 07 00 00").count(4); //0x4B85EC
    injector::MakeInline<RadarPosHookEBX>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(8));
    injector::MakeInline<RadarPosHookESI>(pattern.get(2).get<uintptr_t>(0), pattern.get(2).get<uintptr_t>(8));
    injector::MakeInline<RadarPosHookEBX>(pattern.get(3).get<uintptr_t>(0), pattern.get(3).get<uintptr_t>(8));

    pattern = hook::pattern("F3 0F 11 ? 4C 07 00 00").count(3); //0x4B877E
    injector::MakeInline<RadarScaleHookESI>(pattern.get(1).get<uintptr_t>(0), pattern.get(1).get<uintptr_t>(8));
    injector::MakeInline<RadarScaleHookEBX>(pattern.get(2).get<uintptr_t>(0), pattern.get(2).get<uintptr_t>(8));
    pattern = hook::pattern("89 8B 4C 07 00 00 D9 40 1C 51").count(1);
    injector::MakeInline<RadarScaleHookEBX>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(6));

    //restoring radar height on 4:3
    pattern = hook::pattern("E8 ? ? ? ? 0F 57 C0 D9 9B 50"); //0x4BAA87
    injector::MakeJMP(injector::GetBranchDestination(pattern.get_first(0)), sub_4BAA87, true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("55 8B EC 83 EC 60 53 56 57"));
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