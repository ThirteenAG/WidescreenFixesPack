module;

#include <stdafx.h>

export module Matrix;

import ComVars;

injector::hook_back<void* (__cdecl*)(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)> hb_5F27E2;
void* __cdecl sub_5F27E2(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    if ((a8 == 642 && a9 == 450) || (a8 == 640 && a9 == 448) || (a8 == 641 && a9 == 58))
        return hb_5F27E2.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9);

    a5 = static_cast<int32_t>(static_cast<float>(a5) / Screen.fHudScale);
    a8 = static_cast<int32_t>(static_cast<float>(a8) / Screen.fHudScale);
    a5 += static_cast<int32_t>(Screen.fHudOffsetReal / (Screen.fWidth / 640.0f));

    return hb_5F27E2.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

injector::hook_back<void* (__cdecl*)(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)> hb_5F28E2;
void* __cdecl sub_5F28E2(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    a5 = static_cast<int32_t>(static_cast<float>(a5) / Screen.fHudScale);
    a6 = static_cast<int32_t>(static_cast<float>(a6) / Screen.fHudScale);
    a5 += static_cast<int32_t>(Screen.fHudOffsetReal / (Screen.fWidth / 640.0f));

    return hb_5F28E2.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

injector::hook_back<void* (__cdecl*)(int a1, int a2, int a3, int a4, int a5, int a6, float a7, void* a8, float a9, int a10)> hb_608BF9;
void* __cdecl sub_608BF9(int a1, int a2, int a3, int a4, int a5, int a6, float a7, void* a8, float a9, int a10)
{
    if (Screen.nWidescreenHudOffset)
    {
        if (a1 == 20)
        {
            a1 -= Screen.nWidescreenHudOffset;
            *(int32_t*)(*(uintptr_t*)a8 + 0x30) -= Screen.nWidescreenHudOffset;
        }
        else if (a1 == 592)
        {
            a1 += Screen.nWidescreenHudOffset + 15;
            *(int32_t*)(*(uintptr_t*)a8 + 0x30) += Screen.nWidescreenHudOffset + 15;
        }
    }

    return hb_608BF9.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

export void InitMatrix()
{
    CIniReader iniReader("");
    bool bFixHud = iniReader.ReadInteger("MAIN", "FixHud", 1) != 0;
    Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(iniReader.ReadString("MAIN", "HudAspectRatioConstraint", ""));

    static int32_t* dwordResX = nullptr;
    static int32_t* dwordResY = nullptr;
    static ptrdiff_t resHookEnd = 0;

    auto resGlobals = find_pattern("A1 ? ? ? ? 89 45 E0 8B 0D ? ? ? ? 89 4D E4 33 D2", "8B ? ? ? ? 00 89 55 E0 A1 ? ? ? ? 89 45 E4 33 C9");
    if (!resGlobals.empty())
    {
        if (*resGlobals.get_first<uint8_t>() == 0xA1) // original release
        {
            dwordResX = *resGlobals.get_first<int32_t*>(1);
            dwordResY = *resGlobals.get_first<int32_t*>(10);
            resHookEnd = 186;
        }
        else // v1.2
        {
            dwordResX = *resGlobals.get_first<int32_t*>(2);
            dwordResY = *resGlobals.get_first<int32_t*>(10);
            resHookEnd = 237;
        }
    }

    auto pattern = hook::pattern("68 ? ? ? ? 6A 00 68 ? ? ? ? 68 ? ? ? ? FF 15 ? ? ? ? 89 85 EC FA FF FF 8B 8D");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            char iniPath[MAX_PATH];
            GetModuleFileNameA(NULL, iniPath, MAX_PATH);
            *strrchr(iniPath, '\\') = '\0';
            strcat_s(iniPath, "\\MatrixConfig.ini");
            CIniReader iniReader(iniPath);
            std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
            char defVal[20];
            sprintf_s(defVal, "%dx%d", Screen.nWidth, Screen.nHeight);
            auto iniRes = iniReader.ReadString("RENDERING", "RESOLUTION", defVal);
            sscanf_s(iniRes.c_str(), "%dx%d", &Screen.nWidth, &Screen.nHeight);

            *dwordResX = Screen.nWidth;
            *dwordResY = Screen.nHeight;

            Screen.fWidth = static_cast<float>(Screen.nWidth);
            Screen.fHeight = static_cast<float>(Screen.nHeight);
            Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
            Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
            Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fFieldOfView = 1.0f * (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fCutOffArea = 0.5f / Screen.fFieldOfView;
            Screen.fFMVScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fFMVOffset = ((Screen.fWidth - Screen.fWidth43) / 2.0f) / (Screen.fWidth / 640.0f);

            Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f);
            if (Screen.fHudAspectRatioConstraint.has_value())
            {
                float value = Screen.fHudAspectRatioConstraint.value();
                if (value < 0.0f || value > (32.0f / 9.0f))
                    Screen.fWidescreenHudOffset = value;
                else
                {
                    value = ClampHudAspectRatio(value, Screen.fAspectRatio);
                    Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f);
                }
            }
            Screen.nWidescreenHudOffset = static_cast<int32_t>(Screen.fWidescreenHudOffset);
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(resHookEnd)); //0x7AD916

    //Aspect Ratio
    pattern = hook::pattern("D9 05 ? ? ? ? D8 35 ? ? ? ? D9 1D ? ? ? ? 5D C3");
    static auto flt_972448 = *pattern.get_first<float*>(8);
    static auto flt_97244C = *pattern.get_first<float*>(14);
    struct ARHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *flt_972448 = 1.0f;
            *flt_97244C = 1.0f;
        }
    }; injector::MakeInline<ARHook>(pattern.get_first(0), pattern.get_first(18)); //0x6E6024, 0x6E6024+18

    //FMVs
    pattern = hook::pattern("C7 45 10 00 00 00 00 C7 45 18 80 02 00 00 8B 45 E4");
    struct FMVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.ebp + 0x10) = 0 + static_cast<int32_t>(Screen.fFMVOffset);
            *(int32_t*)(regs.ebp + 0x18) = 640 - static_cast<int32_t>(Screen.fFMVOffset + Screen.fFMVOffset);
        }
    }; injector::MakeInline<FMVHook>(pattern.get_first(0), pattern.get_first(14)); //0x7B3369

    if (bFixHud)
    {
        pattern = hook::pattern("6A 02 FF 15 ? ? ? ? 83 C4 04 C7 05"); //0x40766C
        hb_5F27E2.fun = injector::MakeJMP(*pattern.get_first<void*>(157), sub_5F27E2, true).get(); // hud
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 1C 83 3D ? ? ? ? 00 75 ? 8B 55 FC"); //0x403882
        hb_5F28E2.fun = injector::MakeJMP(injector::GetBranchDestination(pattern.get_first(0)), sub_5F28E2, true).get(); // text
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 28 8B 15 ? ? ? ? DB 42 1C"); //0x407013
        hb_608BF9.fun = injector::MakeJMP(injector::GetBranchDestination(pattern.get_first(0)), sub_608BF9, true).get();
    }
}