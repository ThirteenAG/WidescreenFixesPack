#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fFMVScale;
} Screen;

injector::hook_back<void(__cdecl*)(float, float, float, float, float, float, float, float, int, int)> hb_563BF0;
void __cdecl sub_563BF0_hook(float x, float y, float w, float h, float a5, float a6, float a7, float a8, int a9, int a10)
{
    x -= Screen.fHudOffset;
    w += Screen.fHudOffset;
    return hb_563BF0.fun(x, y, w, h, a5, a6, a7, a8, a9, a10);
}

void __cdecl sub_563BF0_hook2(float x, float y, float w, float h, float a5, float a6, float a7, float a8, int a9, int a10)
{
    x -= Screen.fHudOffset;
    return hb_563BF0.fun(x, y, w, h, a5, a6, a7, a8, a9, a10);
}

void __cdecl FMVHook(float X1, float Y1, float X2, float Y2, float a5, float a6, float a7, float a8, int a9, int a10)
{
    Y1 -= Screen.fHudOffset / (4.0f / 3.0f);
    Y2 += Screen.fHudOffset / (4.0f / 3.0f);
    X1 -= Screen.fHudOffset;
    X2 += Screen.fHudOffset;
    return hb_563BF0.fun(X1, Y1, X2, Y2, a5, a6, a7, a8, a9, a10);
}

injector::hook_back<void(__cdecl*)(float, float, float, float, float, float, float, float, int, int)> hbOverlays;
void __cdecl OverlaysHook(float x, float y, float w, float h, float a5, float a6, float a7, float a8, int a9, int a10)
{
    if ((x >= -2.0f && x <= 2.0f) && (y >= -2.0f && y <= 2.0f) && (w >= 638.0f && w <= 642.0f) && (h >= 478.0f && h <= 482.0f) || (w == 512.0f && h == 448.0f))
        return hbOverlays.fun(x - Screen.fHudOffset, y, w + (Screen.fHudOffset * 2.0f), h, a5, a6, a7, a8, a9, a10);
    else
        return hbOverlays.fun(x, y, w, h, a5, a6, a7, a8, a9, a10);
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFix2D = iniReader.ReadInteger("MAIN", "Fix2D", 1) != 0;
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    bool bDisableCutsceneBorders = iniReader.ReadInteger("MAIN", "DisableCutsceneBorders", 1) != 0;
    bool bIncreaseBackgroundRes = iniReader.ReadInteger("MAIN", "IncreaseBackgroundRes", 1) != 0;
    bool bCutsceneFrameRateFix = iniReader.ReadInteger("MAIN", "CutsceneFrameRateFix", 1) != 0;
    bool bDisableCheckSpec = iniReader.ReadInteger("MAIN", "DisableCheckSpec", 1) != 0;
    bool bDisableRegistryDependency = iniReader.ReadInteger("MISC", "DisableRegistryDependency", 1) != 0;
    bool bDisableSafeMode = iniReader.ReadInteger("MISC", "DisableSafeMode", 1) != 0;
    bool bSkipIntro = iniReader.ReadInteger("MISC", "SkipIntro", 1) != 0;
    bool bBrightnessFix = iniReader.ReadInteger("MISC", "BrightnessFix", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fFieldOfView = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
    Screen.fHudScale = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    //Resolution
    static auto nWidthPtr = *hook::pattern("8B 0D ? ? ? ? 6A 00 50 51").count(1).get(0).get<uint32_t*>(2);
    auto test = pattern_str(0xC7, 0x05, to_bytes(nWidthPtr));
    auto pattern = hook::pattern(pattern_str(0xC7, 0x05, to_bytes(nWidthPtr))); //0x4141E3

    for (size_t i = 0; i < pattern.size(); i++)
    {
        injector::WriteMemory(pattern.get(i).get<uint32_t>(6), Screen.Width, true);
    }

    static auto nHeightPtr = *hook::pattern("8B 0D ? ? ? ? 89 44 24 1C 39").count(1).get(0).get<uint32_t*>(2);
    pattern = hook::pattern(pattern_str(0xC7, 0x05, to_bytes(nHeightPtr))); //0x4141ED

    for (size_t i = 0; i < pattern.size(); i++)
    {
        injector::WriteMemory(pattern.get(i).get<uint32_t>(6), Screen.Height, true);
    }

    struct SetResHook
    {
        void operator()(injector::reg_pack&)
        {
            *nWidthPtr = Screen.Width;
            *nHeightPtr = Screen.Height;
        }
    };

    pattern = hook::pattern("00 5F 89 35 ? ? ? ? 33 C0 5E C3"); //0x4149C8 GOG
    if(pattern.empty())
        pattern = hook::pattern("89 35 ? ? ? ? 33 C0 5E C3"); //0x414A38

    injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uintptr_t>(0), pattern.count(1).get(0).get<uintptr_t>(6));   

    if (bFixFOV)
    {
        pattern = hook::pattern("83 C4 14 68 ? ? ? ? E8 ? ? ? ? E9"); //0x55BDA4 GOG
        if (pattern.empty()) {
            pattern = hook::pattern("83 C4 14 68 ? ? ? ? E8 ? ? ? ? 68"); //0x55BC74 
            injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(8), 5, true);
            injector::WriteMemory<float>(pattern.count(1).get(0).get<uint32_t>(14), Screen.fFieldOfView, true);
        }
        else {
            injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(8), 5, true);
            //GOG version has a jmp to 0x5A9360 for FOV
            pattern = hook::pattern("B8 00 00 80 3F 80 3D F0  1A 09 01 02 72 05 B8");
            injector::WriteMemory<float>(pattern.count(1).get(0).get<uint32_t>(1), Screen.fFieldOfView, true);
        }

        pattern = hook::pattern("C7 46 48 55 55 95"); //0x41D182 GOG
        if (pattern.empty())
            pattern = hook::pattern("C7 46 48 ? ? ? ? C7 46 44"); //0x41D295
        injector::WriteMemory<float>(pattern.count(1).get(0).get<uint32_t>(3), 0.78125f * Screen.fHudScale, true);
    }

    if (bFix2D)
    {
        //Menu scale and disabling of some overlays
        static float fMenuScale = 0.00390625f * Screen.fHudScale;
        pattern = hook::pattern("D8 0D ? ? ? ? 6A 01 8D 54 24 1C 52 D9 5C 24 10 6A 5E"); //0x404E4C + 2
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fMenuScale, true); // Menu Width

        //in game overlay
        auto ret_512 = []() -> int32_t
        {
            return static_cast<int32_t>(512.0f * Screen.fHudScale);
        };
        pattern = hook::pattern("E8 ? ? ? ? 8B 4C 24 1C 99 2B C2 8B 51 10 D1 F8"); //433C4D
        injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)()>(ret_512), true);
        pattern = hook::pattern("E8 ? ? ? ? 8B 7C 24 50 99 2B C2 8B 57 10 D1 F8"); //433CCF
        injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)()>(ret_512), true);
        pattern = hook::pattern("DB 44 24 20 D9 5C 24 20 E8 ? ? ? ? 8B 4C 24 1C 99 2B C2 8B 51 14 D1 F8"); //433C69
        struct OverlayWidthHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                float f = 0.0f;
                int32_t n = *(int32_t*)(regs.esp + 0x20);
                _asm {fild dword ptr[n]}
                _asm {fdiv dword ptr[Screen.fHudScale]}
                _asm {fstp dword ptr[f]}
                *(float*)(regs.esp + 0x20) = f;
            }
        }; injector::MakeInline<OverlayWidthHook1>(pattern.get_first(0), pattern.get_first(8));

        pattern = hook::pattern("DB 44 24 50 D9 5C 24 38 E8 ? ? ? ? 8B 4F 14 99 2B C2 D1 F8"); //433CEB
        struct OverlayWidthHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                float f = 0.0f;
                int32_t n = *(int32_t*)(regs.esp + 0x50);
                _asm {fild dword ptr[n]}
                _asm {fdiv dword ptr[Screen.fHudScale]}
                _asm {fstp dword ptr[f]}
                *(float*)(regs.esp + 0x38) = f;
            }
        }; injector::MakeInline<OverlayWidthHook2>(pattern.get_first(0), pattern.get_first(8));

        pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 64 D9 45 18"); //0x42EAC0
        injector::MakeRET(pattern.get_first());

        pattern = hook::pattern("E8 ? ? ? ? 83 C4 5C C6 05 ? ? ? ? 00 B8"); //0x42E719
        injector::MakeNOP(pattern.get_first(), 5, true); // Cutscene Wall Darkness Effect

        pattern = hook::pattern("E8 ? ? ? ? 83 C4 5C C6 05 ? ? ? ? 00 A1"); //0x45EBD2
        injector::MakeNOP(pattern.get_first(), 5, true); //Cutscene Wall Grunge Effect

        // Text fix
        pattern = hook::pattern("C7 44 24 34 00 00 80 BF D9 5C"); //0x404EBB
        injector::WriteMemory<float>(pattern.count(1).get(0).get<uint32_t>(4), -1.0f * Screen.fHudScale, true); // Text X Pos
        pattern = hook::pattern("C7 44 24 24 CD CC 4C 3B D9 44 24 14"); //0x404E91
        injector::WriteMemory<float>(pattern.count(1).get(0).get<uint32_t>(4), 0.003125f * Screen.fHudScale, true); // Text Width

        //FMV
        pattern = hook::pattern("E8 ? ? ? ? 8B 14 B5 ? ? ? ? A1"); //0x412EAF
        hb_563BF0.fun = injector::MakeCALL(pattern.count(1).get(0).get<uint32_t>(0), FMVHook, true).get();

        //Width
        //sub_563BF0
        auto sub_563BF0 = (uint32_t)hook::pattern("8B 44 24 04 53 8B 5C 24 18 55 8B").count(1).get(0).get<uint32_t>(0);
        pattern = hook::pattern("E8 ? ? ? ?");
        for (size_t i = 0, j = 1; i < pattern.size(); ++i)
        {
            auto addr = pattern.get(i).get<uint32_t>(0);
            auto dest = injector::GetBranchDestination(addr, true).as_int();
            if (dest == sub_563BF0)
            {
                j++;

                if (j == 33 || j == 71 || j == 66 || j == 70) //menu and save menu backgrounds, 1 -> 0x4053C8... http://pastebin.com/Hv6TdTLh
                    continue;

                if (
                    (j >= 44 && j <= 49) || //intro overlay
                    (j >= 56 && j <= 65 && j != 64) //DOF/blur overlay
                    )
                {
                    hb_563BF0.fun = injector::MakeCALL(pattern.get(i).get<uint32_t>(0), sub_563BF0_hook, true).get();
                    continue;
                }

                if (j >= 64) //blur
                {
                    hb_563BF0.fun = injector::MakeCALL(pattern.get(i).get<uint32_t>(0), sub_563BF0_hook2, true).get();
                    continue;
                }

                hbOverlays.fun = injector::MakeCALL(pattern.get(i).get<uint32_t>(0), OverlaysHook, true).get();
            }
        }

        //Cutscene Borders
        static float fBorderWidth = 4096.0f;
        pattern = hook::pattern("DB 05 ? ? ? ? D9 5C 24 08 DB 05 ? ? ? ? D9 5C 24 04 DB 05 ? ? ? ? D9 1C 24"); //0x4F4926
        injector::WriteMemory(pattern.count(2).get(0).get<uint32_t>(2), &fBorderWidth, true);
        injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(2), &fBorderWidth, true);
        static float fBorderPosX = -2048.0f;
        injector::WriteMemory(pattern.count(2).get(0).get<uint32_t>(22), &fBorderPosX, true);
        injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(22), &fBorderPosX, true);

        //Map Zoom
        static float fMapZoom = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio)));
        pattern = hook::pattern("D8 0D ? ? ? ? 51 52 6A 00 D9 5C"); //0x0051D6DB
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fMapZoom, true);

        //Film Effect (loading animation & bathroom hole)
        static float fFilmEffectWidth = 0.1f * ((4.0f / 3.0f) / (Screen.fAspectRatio));
        pattern = hook::pattern("D8 0D ? ? ? ? D8 44 24 14 D9 1C 24 E8 ? ? ? ? 89 44 24 30"); //0x0056807F
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fFilmEffectWidth, true);
        pattern = hook::pattern("D8 0D ? ? ? ? 8B 44 24 38 89 44 24 34 83 C4 28"); //0x005680F3 
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fFilmEffectWidth, true);
        pattern = hook::pattern("D8 0D ? ? ? ? D8 44 24 14 D9 1C 24 E8 ? ? ? ? 89 44 24 38"); //0x00568193 
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fFilmEffectWidth, true);
        pattern = hook::pattern("D8 0D ? ? ? ? 8B 4C 24 38 83 C4 28 89 4C 24 0C D8 44 24 04"); //0x00568207 
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &fFilmEffectWidth, true);

        static float fFilmEffectPos = 0.0f - ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
        pattern = hook::pattern("C7 44 24 0C 00 00 00 00 C7 44 24 04 00 00 00 00 DF E0"); //0x00567FF8 
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(4), &fFilmEffectPos, true);
        //injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(12), &fFilmEffectPos, true);

        //blood stains
        pattern = hook::pattern("50 51 52 C6 05 ? ? ? ? 01 E8"); //0x565AEC
        injector::MakeNOP(pattern.get_first(10), 5, true);

        //Blur Restoration
        static int32_t n640 = static_cast<int32_t>(round(((640.0f - (640.0f / Screen.fHudScale)) * 8.0f) + (640.0f / Screen.fHudScale)));
        pattern = hook::pattern("B8 80 02 00 00 DB 44 24 34 99 F7 FE"); //0x566A79
        injector::WriteMemory(pattern.get_first(1), n640, true);
    }

    if (bDisableCutsceneBorders)
    {
        static uint32_t nOff = 0;
        pattern = hook::pattern("A1 ? ? ? ? 85 C0 74 ? 6A 02 68"); //0x4F4A0A
        injector::WriteMemory(pattern.count(2).get(0).get<uint32_t>(1), &nOff, true);
        injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(1), &nOff, true);
        //injector::WriteMemory(0x565053+1, 2048 - 48, true); //borders pos
    }

    if (bIncreaseBackgroundRes)
    {
        pattern = hook::pattern("B8 ? ? ? ? 89 4C 24 28 8B 0D ? ? ? ? 89 44 24 2C"); //0x403320
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), Screen.Width, true);
        pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 51 E8 ? ? ? ? 8B 15 ? ? ? ? 83 C4 50"); //0x40362E
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), Screen.Width, true);
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(6), Screen.Width, true);

        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 89 35"); //0055A699
        injector::WriteMemory<float>(pattern.count(6).get(3).get<uint32_t>(0x06), Screen.fWidth - 0.5f, true);
        injector::WriteMemory<float>(pattern.count(6).get(3).get<uint32_t>(0x52), Screen.fWidth - 0.5f, true);
        injector::WriteMemory<float>(pattern.count(6).get(3).get<uint32_t>(0x8A), Screen.fWidth - 0.5f, true);
        injector::WriteMemory<float>(pattern.count(6).get(3).get<uint32_t>(0x94), Screen.fWidth - 0.5f, true);
    }

    if (bCutsceneFrameRateFix)
    {
        pattern = hook::pattern("0F 94 C0 A3 ? ? ? ? EB ? C3"); //0x004EFA85
        if(!pattern.empty())// Already patched on GOG version
            injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(1), 0x95, true);
    }

    if (bDisableCheckSpec)
    {
        pattern = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 89 17 66"); //0x00414688 
        injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 6, true);
    }

    if (bDisableRegistryDependency)
    {
        auto RegIATpat = hook::pattern("FF 15 ? ? ? ? 8B 44 24 00 50");
        if (RegIATpat.size() > 0)
        {
            try {
                RegistryWrapper("KONAMI", "");
                RegistryWrapper::AddPathWriter("Install Path", "Movie Install");
                uintptr_t* RegIAT = *RegIATpat.count(1).get(0).get<uintptr_t*>(2); //0x413D67
                injector::WriteMemory(&RegIAT[0], RegQueryValueExA, true);
                injector::WriteMemory(&RegIAT[1], RegOpenKeyA, true);
                injector::WriteMemory(&RegIAT[2], RegSetValueExA, true);
                injector::WriteMemory(&RegIAT[3], RegDeleteValueA, true);
                injector::WriteMemory(&RegIAT[4], RegCloseKey, true);
            }
            catch (std::exception&)
            {
                //Throws error on GOG version
            }
        }
    }

    if (bDisableSafeMode)
    {
        pattern = hook::pattern("83 C8 FF 83 C4 10 C3"); //00413E53
        injector::MakeNOP(pattern.get_first(-2), 2, true);
    }

    if (bSkipIntro)
    {
        pattern = hook::pattern("A1 ? ? ? ? 3B C6 0F 84 ? ? ? ? 83 F8 01"); //415180
        static auto dword_107BFE0 = *pattern.get_first<uint32_t*>(1);
        struct GameStateHook
        {
            void operator()(injector::reg_pack& regs)
            {
                static bool once = false;
                if (!once)
                    *dword_107BFE0 = 3;
                regs.eax = *dword_107BFE0;
                once = true;
            }
        }; injector::MakeInline<GameStateHook>(pattern.get_first(0));
    }

    if (bBrightnessFix)
    {
        static constexpr auto fContrastScale = 0.0075f;
        static constexpr auto fBrightnessScale = 0.003875f;
        static constexpr auto gamma = 0x000A0A0A;

        pattern = hook::pattern("D8 0D ? ? ? ? DB 05 ? ? ? ? D8 0D ? ? ? ? 74 10 DD D8 DD D8");
        injector::WriteMemory(pattern.get_first(2), &fContrastScale, true);    // 55A17E
        injector::WriteMemory(pattern.get_first(14), &fBrightnessScale, true); // 55A18A

        pattern = hook::pattern("68 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 44 24 1C");
        injector::WriteMemory(pattern.get_first(1), gamma, true);  // 5226A9
        injector::WriteMemory(pattern.get_first(11), gamma, true); // 5226AE

        pattern = hook::pattern("BE ? ? ? ? 8D 7C 24 0C F3 A5");
        injector::WriteMemory(*pattern.get_first<uint32_t>(1) + 0x1C, gamma, true); // 5C5B10

        pattern = hook::pattern("83 FF 08 7D 2A 47 EB 1A");
        injector::WriteMemory<uint8_t>(pattern.get_first<uint32_t>(2), 0x05, true); // 5222D2
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("8B 0D ? ? ? ? 6A 00 50 51"));
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