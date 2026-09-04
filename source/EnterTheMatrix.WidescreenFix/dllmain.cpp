#include "stdafx.h"

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fCustomAspectRatioHor;
    float fCustomAspectRatioVer;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fCutOffArea;
    float fFMVScale;
    float fFMVOffset;
    float fWidescreenHudOffset;
    int32_t nWidescreenHudOffset;
} Screen;

injector::hook_back<void*(__cdecl*)(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)> hb_5F34C0;
void* __cdecl sub_5F34C0(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    if ((a8 == 642 && a9 == 450) || (a8 == 640 && a9 == 448) || (a8 == 641 && a9 == 58))
        return hb_5F34C0.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9);

    a5 = static_cast<int32_t>(static_cast<float>(a5) / Screen.fHudScale);
    a8 = static_cast<int32_t>(static_cast<float>(a8) / Screen.fHudScale);
    a5 += static_cast<int32_t>(Screen.fHudOffsetReal / (Screen.fWidth / 640.0f));

    return hb_5F34C0.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

injector::hook_back<void*(__cdecl*)(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)> hb_5F35C0;
void* __cdecl sub_5F35C0(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    a5 = static_cast<int32_t>(static_cast<float>(a5) / Screen.fHudScale);
    a6 = static_cast<int32_t>(static_cast<float>(a6) / Screen.fHudScale);
    a5 += static_cast<int32_t>(Screen.fHudOffsetReal / (Screen.fWidth / 640.0f));

    return hb_5F35C0.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

injector::hook_back<void*(__cdecl*)(int a1, int a2, int a3, int a4, int a5, int a6, float a7, void* a8, float a9, int a10)> hb_60AC93;
void* __cdecl sub_60AC93(int a1, int a2, int a3, int a4, int a5, int a6, float a7, void* a8, float a9, int a10)
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

    return hb_60AC93.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}

void InitCDSettings()
{
    static std::vector<std::string> list;
    GetResolutionsList(list);

    static auto dword_4280CC = *hook::get_pattern<uint32_t*>("8B 0D ? ? ? ? 8D 54 24 0C 51", 2);
    auto pattern = hook::pattern("8B 45 04 85 C0 74");
    struct ResListHook
    {
        void operator()(injector::reg_pack& regs)
        {
            char iniPath[MAX_PATH];
            GetModuleFileNameA(NULL, iniPath, MAX_PATH);
            *strrchr(iniPath, '\\') = '\0';
            strcat_s(iniPath, "\\MatrixConfig.ini");
            CIniReader iniReader(iniPath);
            int32_t iniResX = 0, iniResY = 0;
            std::tie(iniResX, iniResY) = GetDesktopRes();
            char defVal[20];
            sprintf_s(defVal, "%dx%d", iniResX, iniResY);
            auto iniRes = iniReader.ReadString("RENDERING", "RESOLUTION", defVal);
            //sscanf_s(iniRes, "%dx%d", &iniResX, &iniResY);
            int32_t i = 0, mode = 0;
            for (auto &res : list)
            {
                SendMessageA(*((HWND*)(regs.esi + 0x13E8)), 0x143u, 0, (LPARAM)(res.c_str()));

                if (res == iniRes)
                    mode = i;

                ++i;
            }
            SendMessageA(*((HWND*)(regs.esi + 0x13E8)), 0x14Eu, mode, 0);
            *dword_4280CC = mode;
        }
    }; injector::MakeInline<ResListHook>(pattern.get_first(0), pattern.get_first(126)); //0x40600D, 0x40608B

    pattern = hook::pattern("68 ? ? ? ? 50 68 ? ? ? ? 68 ? ? ? ? FF D6 8D 4C 24 0C 51");
    struct MatrixOptionsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            char iniPath[MAX_PATH];
            GetModuleFileNameA(NULL, iniPath, MAX_PATH);
            *strrchr(iniPath, '\\') = '\0';
            strcat_s(iniPath, "\\MatrixConfig.ini");
            CIniReader iniReader(iniPath);
            iniReader.WriteString("RENDERING", "RESOLUTION", (char*)list[*dword_4280CC].c_str());
        }
    }; injector::MakeInline<MatrixOptionsHook>(pattern.get_first(0), pattern.get_first(18)); //0x406B9F, 0x406BB1
}

void InitDVDSettings()
{
    static std::vector<std::string> list;
    GetResolutionsList(list);

    static auto dword_4270D8 = *hook::get_pattern<uint32_t*>("8B 0D ? ? ? ? 8D 54 24 0C 51", 2);
    auto pattern = hook::pattern("8B 45 04 85 C0 74");
    struct ResListHook
    {
        void operator()(injector::reg_pack& regs)
        {
            char iniPath[MAX_PATH];
            GetModuleFileNameA(NULL, iniPath, MAX_PATH);
            *strrchr(iniPath, '\\') = '\0';
            strcat_s(iniPath, "\\MatrixConfig.ini");
            CIniReader iniReader(iniPath);
            int32_t iniResX = 0, iniResY = 0;
            std::tie(iniResX, iniResY) = GetDesktopRes();
            char defVal[20];
            sprintf_s(defVal, "%dx%d", iniResX, iniResY);
            auto iniRes = iniReader.ReadString("RENDERING", "RESOLUTION", defVal);
            //sscanf_s(iniRes, "%dx%d", &iniResX, &iniResY);
            int32_t i = 0, mode = 0;
            for (auto &res : list)
            {
                SendMessageA(*((HWND*)(regs.esi + 0x1460)), 0x143u, 0, (LPARAM)(res.c_str()));

                if (res == iniRes)
                    mode = i;

                ++i;
            }
            SendMessageA(*((HWND*)(regs.esi + 0x1460)), 0x14Eu, mode, 0);
            *dword_4270D8 = mode;
        }
    }; injector::MakeInline<ResListHook>(pattern.get_first(0), pattern.get_first(125)); //0x406055, 0x4060D2

    pattern = hook::pattern("68 ? ? ? ? 50 68 ? ? ? ? 68 ? ? ? ? FF D6 8D 4C 24 0C 51");
    struct MatrixOptionsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            char iniPath[MAX_PATH];
            GetModuleFileNameA(NULL, iniPath, MAX_PATH);
            *strrchr(iniPath, '\\') = '\0';
            strcat_s(iniPath, "\\MatrixConfig.ini");
            CIniReader iniReader(iniPath);
            iniReader.WriteString("RENDERING", "RESOLUTION", (char*)list[*dword_4270D8].c_str());
        }
    }; injector::MakeInline<MatrixOptionsHook>(pattern.get_first(0), pattern.get_first(18)); //0x406C0F, 0x406C21
}

void Init()
{
    CIniReader iniReader("");
    bool bFixHud = iniReader.ReadInteger("MAIN", "FixHud", 1) != 0;
    Screen.fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 100.0f);

    auto pattern = hook::pattern("68 ? ? ? ? 6A 00 68 ? ? ? ? 68 ? ? ? ? FF 15 ? ? ? ? 89 85 EC FA FF FF 8B 8D");
    static auto dword_947F60 = *hook::get_pattern<int32_t*>("8B ? ? ? ? 00 89 55 E0 A1 ? ? ? ? 89 45 E4 33 C9", 2);
    static auto dword_947F64 = *hook::get_pattern<int32_t*>("8B ? ? ? ? 00 89 55 E0 A1 ? ? ? ? 89 45 E4 33 C9", 10);
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

            *dword_947F60 = Screen.nWidth;
            *dword_947F64 = Screen.nHeight;

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

            if (Screen.fWidescreenHudOffset)
            {
                if (Screen.fAspectRatio < (16.0f / 9.0f))
                    Screen.fWidescreenHudOffset = Screen.fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
                Screen.nWidescreenHudOffset = static_cast<int32_t>(Screen.fWidescreenHudOffset);
            }
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(237)); //0x7B8AF0, 0x7B8BDD

    //Aspect Ratio
    pattern = hook::pattern("D9 05 ? ? ? ? D8 35 ? ? ? ? D9 1D ? ? ? ? 5D C3");
    static auto flt_97EDA8 = *pattern.get_first<float*>(8);
    static auto flt_97EDAC = *pattern.get_first<float*>(14);
    struct ARHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *flt_97EDA8 = 1.0f;
            *flt_97EDAC = 1.0f;
        }
    }; injector::MakeInline<ARHook>(pattern.get_first(0), pattern.get_first(18)); //0x6EAE96, 0x6EAE96+18

    //FMVs
    pattern = hook::pattern("C7 45 10 00 00 00 00 C7 45 18 80 02 00 00 8B 45 E4");
    struct FMVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.ebp + 0x10) = 0 + static_cast<int32_t>(Screen.fFMVOffset);
            *(int32_t*)(regs.ebp + 0x18) = 640 - static_cast<int32_t>(Screen.fFMVOffset + Screen.fFMVOffset);
        }
    }; injector::MakeInline<FMVHook>(pattern.get_first(0), pattern.get_first(14)); //0x7BE620

    if (bFixHud)
    {
        pattern = hook::pattern("6A 02 FF 15 ? ? ? ? 83 C4 04 C7 05"); //0x4076CB
        hb_5F34C0.fun = injector::MakeJMP(*pattern.get_first<void*>(157), sub_5F34C0, true).get(); // hud
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 1C 83 3D ? ? ? ? 00 75 ? 8B 55 FC"); //0x403887
        hb_5F35C0.fun = injector::MakeJMP(injector::GetBranchDestination(pattern.get_first(0)), sub_5F35C0, true).get(); // text
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 28 8B 15 ? ? ? ? DB 42 1C"); //0x407068
        hb_60AC93.fun = injector::MakeJMP(injector::GetBranchDestination(pattern.get_first(0)), sub_60AC93, true).get();
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("55 8B EC 83 EC 34 56 6A 00 6A FF"));
            CallbackHandler::RegisterCallback(InitCDSettings, hook::pattern("8B 45 04 85 C0 74 6B 8B 15"));
            CallbackHandler::RegisterCallback(InitDVDSettings, hook::pattern("8B 45 04 85 C0 74 6A A1 F8"));
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