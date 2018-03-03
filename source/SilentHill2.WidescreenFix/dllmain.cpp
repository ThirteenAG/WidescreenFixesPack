#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fHudOffset;
    int32_t Width43;
    float TextOffset;
    int32_t FullscreenOffsetX;
    int32_t FullscreenOffsetY;
} Screen;

/*
uint32_t nImageId; uint32_t* unk_1DBFC50;
injector::hook_back<uint32_t*(__cdecl*)(uintptr_t*, int32_t)> hbsub_457B40;
uint32_t* __cdecl sub_457B40Hook(uintptr_t* a1, int32_t a2)
{
    auto pImageId = hbsub_457B40.fun(a1, a2);
    if (unk_1DBFC50 == pImageId)
        nImageId = *pImageId;
    else
        nImageId = 0;
    return pImageId;
}
*/

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("6A 70 68 ? ? ? ? E8 ? ? ? ? 33 DB 53 8B 3D ? ? ? ? FF D7");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) != 0;
    bool bFMVWidescreenEnhancementPackCompatibility = iniReader.ReadInteger("MAIN", "FMVWidescreenEnhancementPackCompatibility", 0) != 0;
    bool bFix2D = iniReader.ReadInteger("MAIN", "Fix2D", 1) != 0;
    bool bDisableCutsceneBorders = iniReader.ReadInteger("MISC", "DisableCutsceneBorders", 1) != 0;
    bool bSingleCoreAffinity = iniReader.ReadInteger("MISC", "SingleCoreAffinity", 1) != 0;
    bool bDisableSafeMode = iniReader.ReadInteger("MISC", "DisableSafeMode", 1) != 0;
    bool bFastTransitions = iniReader.ReadInteger("MISC", "FastTransitions", 1) != 0;
    bool bCreateLocalFix = iniReader.ReadInteger("MISC", "CreateLocalFix", 1) != 0;
    uint32_t nFPSLimit = iniReader.ReadInteger("MISC", "FPSLimit", 30);
    bool bPS2CameraSpeed = iniReader.ReadInteger("MISC", "PS2CameraSpeed", 0) != 0;
    bool bGamepadControlsFix = iniReader.ReadInteger("MISC", "GamepadControlsFix", 1) != 0;
    bool bLightingFix = iniReader.ReadInteger("MISC", "LightingFix", 1) != 0;
    bool bReduceCutsceneFOV = iniReader.ReadInteger("MISC", "ReduceCutsceneFOV", 0) != 0;
    bool bSteamCrashFix = iniReader.ReadInteger("MISC", "SteamCrashFix", 0) != 0;
    auto nIncreaseNoiseEffectRes = iniReader.ReadInteger("MISC", "IncreaseNoiseEffectRes", 1);
    bool bFullscreenImages = iniReader.ReadInteger("MISC", "FullscreenImages", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudOffset = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio)));
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.TextOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.FullscreenOffsetX = static_cast<int32_t>(((Screen.fWidth - (Screen.fWidth * ((4.0f / 3.0f) / (1440.0f / 870.0f)))) / 2.0f) * ((1440.0f / 870.0f) / (4.0f / 3.0f)));
    Screen.FullscreenOffsetY = static_cast<int32_t>(((Screen.fHeight - (Screen.fHeight * ((4.0f / 3.0f) / (1440.0f / 870.0f)))) / 2.0f) * ((1440.0f / 870.0f) / (4.0f / 3.0f)));

    auto ResList = *hook::pattern("3B 93 ? ? ? ? 75 ? 8B 44 24 14 3B 83").count(1).get(0).get<uint32_t*>(2); //4F60A8
    for (uint32_t i = (uint32_t)ResList; i <= (uint32_t)ResList + 0x2C; i += 4)
    {
        injector::WriteMemory(i, Screen.Width, true);
        i += 4;
        injector::WriteMemory(i, Screen.Height, true);
    }

    if (bFix2D)
    {
        //2D
        static auto dword_A37080 = *hook::pattern("DB 05 ? ? ? ? 8B 0D ? ? ? ? 85 C9 50").count(1).get(0).get<uint32_t*>(2);
        pattern = hook::pattern(pattern_str(0xDB, 0x05, to_bytes(dword_A37080)));
        for (size_t i = 0; i < pattern.size(); ++i) //http://pastebin.com/ZpkGX9My
        {
            if (i == 318 || i == 315 || i == 314 || i == 313 || i == 310 || i == 312 || i == 311 || i == 320 || i == 178 || i == 177 || i == 176 || i == 175 || i == 174 || i == 327 || i == 2 || i == 309
                || i == 173 || i == 317 || i == 316 || i == 332 || i == 330)
            {
                injector::WriteMemory(pattern.get(i).get<uint32_t>(2), &Screen.Width, true);
            }
        }
        pattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 48 89 44 24 0C 89"); //00478A11
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), &Screen.Width, true);
        pattern = hook::pattern("8B 15 ? ? ? ? 50 52 6A 00 6A 00 33 C0 "); //00478A7F
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true);
        pattern = hook::pattern("8B 0D ? ? ? ? 50 51 56 6A 00 33 C0"); //00478ACE
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true);
        pattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 48 51 50 53 50 33 C0"); //00477EBA
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), &Screen.Width, true);
        pattern = hook::pattern("8B 15 ? ? ? ? 52 C7 44 24 14 00 00 00 00"); //00477313
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true);
        pattern = hook::pattern("8B 15 ? ? ? ? D1 EA 85 D2 89 54 24 04"); //00512067 flashlight
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true);
        pattern = hook::pattern("D9 05 ? ? ? ? D8 0D ? ? ? ? D9 1D ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ? D9 1D ? ? ? ? D9 05 ? ? ? ? D8 35"); //005106DE flashlight
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.fWidth, true);
        pattern = hook::pattern("DB 05 ? ? ? ? 85 C0 7D 06 D8 05 ? ? ? ? 8B 0D ? ? ? ? D9 1D ? ? ? ? DB 05 ? ? ? ?"); //00510681 flashlight
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width43, true);
        pattern = hook::pattern("8B 0D ? ? ? ? D1 E9 85 C9 89 4C 24 10 DB 44 24 10"); //004DC103 The light from the lighthouse  (i == 330)
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true);

        pattern = hook::pattern("A3 ? ? ? ? 89 44 24 14 A1 ? ? ? ? 8D 4C 24 0C 51");
        struct SetScaleHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_A37080 = Screen.Width43;
            }
        }; injector::MakeInline<SetScaleHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5)); //4F5FFD

        pattern = hook::pattern("89 15 ? ? ? ? C6 05 ? ? ? ? 01 33 C0 C3");
        struct SetScaleHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_A37080 = Screen.Width43;
            }
        }; injector::MakeInline<SetScaleHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //4F661E

        pattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? BD 01 00 00 00");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), &Screen.Width, true); //0x4F6672 + 1
        pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 85 C0 7D 06");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true); //0x479BC2 + 2 stretching

        auto flt_630DEC = *hook::pattern("D8 0D ? ? ? ? DE C1 DA 44 24 08 59 C3").count(1).get(0).get<uint32_t*>(2);
        pattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(flt_630DEC))); //0x630DEC fmul

        //http://pastebin.com/Lqg7hYsW
        int f05Indices[] = { 29,31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,63,65,67,75,77,79,81,83,84,85,86,87,88,89,91,93,95,97,99,
            101,103,105,107,109,111,113,120,122,124,126,128,130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168,
            170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,224,226,228,230,232,234,
            236,238,240,242,244,246,248,250,252,254,256,258,260,262,264,266,268,270,272,274,276,278,280,282,284,286,288,290,292,294,296,298,300,302,
            304,306,308,310,312,314,316,318,320,322,324,326,328,330,332,334,336,338,340,342,344,346,348,350,352,354,356,370,372,386,388,390,392,394,396,
            398,400,402,404,406,408,410,412,414,416,418,420,422,424,426,428,430,432,435,437,439,441,443,445,447,449,451,453,455,457,459,461,463,469,471,473,
            475,477,479,481,483,485,487,489,491,493,495,497,499,501,503,505,507,509,511,513,515,517,519,521,523,525,527,529,531,533,535,541,543,549,551,553,555,
            557,559,561,563,565,567,569,571,574,576,578,580,582,584,586,588,590,592,594,596,598,600,602,604,606,608,628,630,632,634,682, 610, 612 ,614, 616, 618, 620, 622, 624, 626 };

        for (size_t i = 0; i < pattern.size(); ++i)
        {
            //logfile << std::dec << i << " " << std::hex << pattern.get(i).get<uint32_t>(2) << std::endl;
            if (!(std::end(f05Indices) == std::find(std::begin(f05Indices), std::end(f05Indices), i)))
            {
                injector::WriteMemory(pattern.get(i).get<uint32_t>(2), &Screen.fHudOffset, true);
            }
        }
        pattern = hook::pattern("D9 05 ? ? ? ? 0F BF 46 0C D8 C9 89 54 24 00");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.fHudOffset, true); //49F2C0

        //2D
        pattern = hook::pattern("D8 0D ? ? ? ? D8 44 24 0C D8 44 24 18 D8 25 ? ? ? ? D9 54 24 1C");
        float offset = (((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f) / (2.0f / (Screen.fAspectRatio / (4.0f / 3.0f)))) / (Screen.fWidth);
        static float f_05 = -0.5f + offset;
        static float f05 = 0.5f + offset;
        injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(2), &f_05, true); //0x049F5B8
        pattern = hook::pattern("D9 05 ? ? ? ? 0F BE 15 ? ? ? ? D8 C9 A1 ? ? ? ? 85 C0");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &f05, true);  //0x049F592

        //Text pos
        static float tempvar1, tempvar2, tempvar3, tempvar4, tempvar5, tempvar6, tempvar7, tempvar8, tempvar9;
        pattern = hook::pattern("89 54 24 04 0F B7 50 ? D8 0D ? ? ? ? 42 D8 C2");
        struct TextPosHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uintptr_t*)(regs.esp + 0x4) = regs.edx;
                regs.edx = *(uint16_t*)(regs.eax + 0xA);

                _asm FSTP DWORD PTR[tempvar1]
                _asm FSTP DWORD PTR[tempvar2]
                _asm FSTP DWORD PTR[tempvar3]
                tempvar3 += Screen.TextOffset;
                _asm FLD  DWORD PTR[tempvar3]
                _asm FLD  DWORD PTR[tempvar2]
                _asm FLD  DWORD PTR[tempvar1]
            }
        }; injector::MakeInline<TextPosHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8)); //0x4819C7, 0x4819C7+8 | sub_4818D0+75 +

        pattern = hook::pattern("D9 1D ? ? ? ? 8B 08 FF 91");
        static auto flt_807498 = *hook::pattern("D9 15 ? ? ? ? DB 05 ? ? ? ? 7D 06 D8 05").count(1).get(0).get<uint32_t*>(2);
        static auto flt_8074A8 = (uint32_t)flt_807498 + 0x10;
        static auto flt_8074B8 = (uint32_t)flt_807498 + 0x20;
        static auto flt_8074C8 = (uint32_t)flt_807498 + 0x30;
        static auto flt_8074D8 = (uint32_t)flt_807498 + 0x40;
        static auto flt_8074E8 = (uint32_t)flt_807498 + 0x50;
        static float temp = 0.0f;
        struct TextPosHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm FSTP DWORD PTR[temp]
                *(float*)flt_807498 += Screen.TextOffset;
                *(float*)flt_8074C8 += Screen.TextOffset;
                *(float*)flt_8074E8 = temp + Screen.TextOffset;
                *(float*)flt_8074A8 += Screen.TextOffset;
                *(float*)flt_8074B8 += Screen.TextOffset;
                *(float*)flt_8074D8 += Screen.TextOffset;
            }
        }; injector::MakeInline<TextPosHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //481343 | sub_481210+41 | sub_481210+B3 +

        pattern = hook::pattern("89 94 24 A8 00 00 00 DB 44 24 18 89 94 24 D0");
        struct TextPosHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uintptr_t*)(regs.esp + 0xA8) = regs.edx;

                _asm FSTP DWORD PTR[tempvar4]
                tempvar4 += Screen.TextOffset;
                _asm FLD  DWORD PTR[tempvar4]

                *(float*)(regs.esp + 0x6C + 4) += Screen.TextOffset;
            }
        }; injector::MakeInline<TextPosHook3>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(7)); //0x481E70 | sub_481D20+119+


        pattern = hook::pattern("D9 94 24 90 00 00 00 D9 C1 D9 9C 24 94 00 00 00");
        struct TextPosHook4
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm FSTP DWORD PTR[tempvar5]
                tempvar5 += Screen.TextOffset;
                _asm FLD  DWORD PTR[tempvar5]

                float esp90 = 0.0f;
                _asm FST DWORD PTR[esp90]
                *(float*)(regs.esp + 0x90) = esp90;
            }
        }; injector::MakeInline<TextPosHook4>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(7)); //481F72 | sub_481D20+23E+


        //censor boxes
        pattern = hook::pattern("DB 05 ? ? ? ? 7D 06 D8 05 ? ? ? ? 0F BE 0D ? ? ? ? 8B");
        struct TextPosHook5
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x30) += Screen.TextOffset;

                _asm FILD DWORD PTR[Screen.Height] //A37084
            }
        }; injector::MakeInline<TextPosHook5>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //482054 | sub_481D20+31C+


        pattern = hook::pattern("D9 54 24 58 D9 C9 D9 5C 24 5C D9 5C 24 6C DB 44 24 24");
        struct TextPosHook6
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm FSTP DWORD PTR[tempvar6]
                tempvar6 += Screen.TextOffset;
                _asm FLD  DWORD PTR[tempvar6]
                *(float*)(regs.esp + 0x58) = tempvar6;
                uint32_t esp58 = (regs.esp + 0x58);
                _asm FST DWORD PTR[esp58]
                _asm fxch    st(1)
            }
        }; injector::MakeInline<TextPosHook6>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //482117 | sub_481D20+3E3+


        //horizontal lines in text
        pattern = hook::pattern("DB 05 ? ? ? ? 7D ? D8 05 ? ? ? ? D9 5C 24 20 0F BE 15");
        struct TextPosHook7
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm FSTP DWORD PTR[tempvar7]
                tempvar7 += Screen.TextOffset;
                _asm FLD  DWORD PTR[tempvar7]
                *(float*)(regs.esp + 0x2C) += Screen.TextOffset;
                _asm FILD DWORD PTR[Screen.Height]
            }
        }; injector::MakeInline<TextPosHook7>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //482249 | sub_482160+D3 


        pattern = hook::pattern("D9 54 24 40 D9 C1 D9 5C 24 44 D9 CA");
        struct TextPosHook8
        {
            void operator()(injector::reg_pack& regs)
            {
                uint32_t esp40 = (regs.esp + 0x40);
                _asm FSTP DWORD PTR[tempvar8]
                tempvar8 += Screen.TextOffset;
                _asm FLD  DWORD PTR[tempvar8]
                *(float*)(regs.esp + 0x40) = tempvar8;
                _asm FST DWORD PTR[esp40]
                _asm fld     st(1)			
            }
        }; injector::MakeInline<TextPosHook8>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //482348 | sub_482160+1D6


        pattern = hook::pattern("A1 ? ? ? ? 8B 10 6A 14 8D 4C 24 30 51 6A 03 6A 02 50");
        static auto dword_A36494 = *pattern.count(1).get(0).get<uint32_t*>(1);
        struct TextPosHook9
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *dword_A36494;
                *(float*)(regs.esp + 0x7C) += Screen.TextOffset;
                *(float*)(regs.esp + 0x90) += Screen.TextOffset;
            }
        }; injector::MakeInline<TextPosHook9>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5)); //4823D9 | sub_482160+230 | sub_482160+257

        //Cursor
        pattern = hook::pattern("D8 0D ? ? ? ? C7 44 24 3C 0A D7 23 3C");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<uint32_t*>(2), 0.05859375f * (1.0f / (Screen.fAspectRatio / (4.0f / 3.0f))), true);
    }

    //solves camera pan inconsistency for different resolutions
    //solves camera tilt inconsistency for different resolutions
    pattern = hook::pattern("51 E8 ? ? ? ? 89 44 24 00 DB 44 24 00 D9 1D"); //47CD30
    struct Ret640
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 640;
        }
    };
    struct Ret480
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 480;
        }
    };
    injector::MakeInline<Ret640>(pattern.get_first(1));
    injector::MakeInline<Ret640>(pattern.get_first(39));
    injector::MakeInline<Ret480>(pattern.get_first(20));
    injector::MakeInline<Ret480>(pattern.get_first(64));

    //FMV
    auto FMVpattern1 = hook::pattern("A1 ? ? ? ? D9 15 ? ? ? ? D9 C2 89 15 ? ? ? ? D9 1D");
    injector::WriteMemory(FMVpattern1.count(1).get(0).get<uint32_t>(1), &Screen.TextOffset, true); //0x0043E4D8
    auto FMVpattern2 = hook::pattern("D8 25 ? ? ? ? 8B 0D ? ? ? ? 85 C9 8B 15");
    injector::WriteMemory(FMVpattern2.count(1).get(0).get<uint32_t>(2), &Screen.TextOffset, true); //0x0043E4C5
    auto FMVpattern3 = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 89 15 ? ? ? ? A3");
    injector::WriteMemory(FMVpattern3.count(1).get(0).get<uint32_t>(2), &Screen.TextOffset, true); //0x0043E47F

    if (bFMVWidescreenMode || bFMVWidescreenEnhancementPackCompatibility)
    {
        pattern = hook::pattern("E9 ? ? ? ? A1 ? ? ? ? 3D ? ? ? ? 77 05 B8 ? ? ? ?");
        injector::WriteMemory<uint16_t>(pattern.count(1).get(0).get<uint32_t>(0), 0x850F, true); //0x0043E47F
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), 0x00000088, true); //0x0043E47F+2
        injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(2 + 4), 4, true);

        static float fFMVOffset1 = (((480.0f * (16.0f / 9.0f)) / (640.0f / 384.0f)) / (4.0f / 3.0f));
        pattern = hook::pattern("D8 0D ? ? ? ? D9 55 28 8B 0D ? ? ? ? DB 05 ? ? ? ? 85 C9 E9");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(-4), &fFMVOffset1, true); //43DBC5
        static float TextOffsetWS = Screen.TextOffset - (((Screen.fHeight * (640.0f / 384.0f)) - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
        if (bFMVWidescreenEnhancementPackCompatibility)
            TextOffsetWS = Screen.TextOffset - (((Screen.fHeight * (1280.0f / 720.0f)) - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
        injector::WriteMemory(FMVpattern1.count(1).get(0).get<uint32_t>(1), &TextOffsetWS, true); //0x0043E4D8
        injector::WriteMemory(FMVpattern2.count(1).get(0).get<uint32_t>(2), &TextOffsetWS, true); //0x0043E4C5
        injector::WriteMemory(FMVpattern3.count(1).get(0).get<uint32_t>(2), &TextOffsetWS, true); //0x0043E47F

        static uint32_t fFMVOffset2 = static_cast<uint32_t>(fFMVOffset1 * (290.0f / 384.0f)); //?
        if (bFMVWidescreenEnhancementPackCompatibility)
            fFMVOffset2 = static_cast<uint32_t>(fFMVOffset1 * (480.0f / 384.0f));
        pattern = hook::pattern("A1 ? ? ? ? 3D ? ? ? ? 77 05 B8 ? ? ? ? 2B 45 10");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), &fFMVOffset2, true); //43DC1E
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(10), 0xEB, true); //43DC28
        injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(30), 8, true);
    }

    if (bDisableCutsceneBorders)
    {
        static float f0 = 0.0f;
        static float f1 = 1.0f;
        pattern = hook::pattern("D8 0D ? ? ? ? DE C1 DA 44 24 ? E8");
        injector::WriteMemory(pattern.count(2).get(0).get<uint32_t>(2), &f0, true); //00478A6E
        injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(2), &f1, true); //00478ABD
    }

    if (bSingleCoreAffinity)
    {
        SetProcessAffinityMask(GetCurrentProcess(), 1);
    }

    if (bDisableSafeMode)
    {
        pattern = hook::pattern("74 05 E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 08");
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0xEB, true);

        pattern = hook::pattern("8B 44 24 04 56 68 ? ? ? ? 50 FF 15 ? ? ? ? 8B F0 83 C4 08 85 F6 74 16");
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0xC3, true);
    }

    if (bFastTransitions)
    {
        pattern = hook::pattern("C7 44 24 08 9A 99 99 3F");
        injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(-2), 2, true);
        injector::WriteMemory<float>(pattern.count(1).get(0).get<float*>(4), 0.5f, true);
    }

    if (bCreateLocalFix)
    {
        char moduleName[MAX_PATH];
        GetModuleFileName(NULL, moduleName, MAX_PATH);
        *(strrchr(moduleName, '\\') + 1) = '\0';
        strcat(moduleName, "local.fix");

        auto FileExists = [](LPCTSTR szPath) -> BOOL 
        {
            DWORD dwAttrib = GetFileAttributes(szPath);
            return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
        };

        if (!FileExists(moduleName))
        {
            std::ofstream local_fix;
            local_fix.open(moduleName);
            local_fix << "HACK DX_CONFIG_DRIVER_AA_NOT_BROKEN 1";
            local_fix.close();
        }
    }

    if (nFPSLimit)
    {
        pattern = hook::pattern("6A 00 6A ? 50 51");
        injector::WriteMemory<uint8_t>(pattern.count(2).get(1).get<uint32_t>(3), nFPSLimit, true); //004F6F53
    }

    if (bPS2CameraSpeed)
    {
        static float f15 = 1.5f; // new value 1.5; restores speed of tilting camera action
        static float f009 = 0.09f; // new value 0.09; increases speed of all camera actions
        static float f25 = 2.5f; // new value 2.5; restores speed of auto-panning camera action
        static float f1 = 1.0f; // new value 1.00; reduces speed of auto-tilting camera action

        pattern = hook::pattern("D8 0D ? ? ? ? 8B 6C 24 5C D9 5C 24 38 D9 43 04 D8 0D ? ? ? ? D9 5C 24 3C E8");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &f15, true); //0051CF36 
        
        struct CameraHook
        {
            void operator()(injector::reg_pack&)
            {
                _asm fld dword ptr[f009]
            }
        }; injector::MakeInline<CameraHook>(pattern.count(1).get(0).get<uint32_t>(27)); //0051CF51 

        pattern = hook::pattern("D9 05 ? ? ? ? D8 C1 D9 1D ? ? ? ? DD D8 F6 47 08 01");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &f25, true); //0051C38C

        pattern = hook::pattern("D8 05 ? ? ? ? D9 1D ? ? ? ? D9 44 24 14 D8 4E 04 D8 0D");
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &f1, true); //0051C302 

        pattern = hook::pattern("74 ? D9 05 ? ? ? ? E8 ? ? ? ? 50 E8");
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), 0xEBi8, true); //0058C0E3
    }

    if (bGamepadControlsFix)
    {
        pattern = hook::pattern("74 15 39 04 CD ? ? ? ? 75 0C 83 F9 16");
        injector::WriteMemory<uint8_t>(pattern.count(2).get(1).get<uint32_t>(0), 0xEB, true); //5AF936
    }

    if (bLightingFix)
    {
        pattern = hook::pattern("8B 10 75 ? 8B 0D");
        auto dword_1F81298 = *pattern.count(2).get(0).get<uintptr_t>(6) - 0x10;
        auto dword_1F8129C = *pattern.count(2).get(1).get<uintptr_t>(6) - 0x10;
        injector::WriteMemory(pattern.count(2).get(0).get<void*>(6), dword_1F81298, true); //50002A solves flashlight bug for NPCs
        injector::WriteMemory(pattern.count(2).get(1).get<void*>(6), dword_1F8129C, true); //5039FB solves flashlight bug for reflective objects

        pattern = hook::pattern("0F B7 05 ? ? ? ? 48 C3");
        static uint16_t* word_9467F0 = *pattern.get_first<uint16_t*>(3);
        static uint8_t* dword_01F7E3C4 = *hook::get_pattern<uint8_t*>("A1 ? ? ? ? 83 C4 18 83 C0 F9", 1);
        struct LightingFixHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *word_9467F0 - 1;

                if (regs.eax == 1 && *dword_01F7E3C4 == 0x5C)
                    regs.eax = 0;
            }
        }; injector::MakeInline<LightingFixHook>(pattern.get_first(0), pattern.get_first(8)); //47AFE0
    }

    if (bReduceCutsceneFOV)
    {
        static float f1472 = 1.14702f / (1.0f / (Screen.fAspectRatio / (4.0f / 3.0f)));
        pattern = hook::pattern("D8 0D ? ? ? ? D9 1D ? ? ? ? E8 ? ? ? ? 6A 00 6A 00 6A 00 6A 00");
        injector::WriteMemory(pattern.count(2).get(0).get<uint32_t>(2), &f1472, true); //4A0E13
        injector::WriteMemory(pattern.count(2).get(1).get<uint32_t>(2), &f1472, true); //4A1A61
    }

    if (bSteamCrashFix)
    {
        pattern = hook::pattern("57 33 C0 B9 0E 00 00 00"); //45826B
        injector::WriteMemory<uint8_t>(pattern.get_first(-2), 0xEBi8, true);
    }

    if (nIncreaseNoiseEffectRes)
    {
        //Eliminates tiling
        pattern = hook::pattern("C7 44 24 2C ? ? ? ? C7 44 24 48 ? ? ? ? C7 44 24 5C ? ? ? ? C7 44 24 60"); //4780E0
        injector::WriteMemory<float>(pattern.get_first(4),  1.0f, true);
        injector::WriteMemory<float>(pattern.get_first(12), 1.0f, true);
        injector::WriteMemory<float>(pattern.get_first(20), 1.0f, true);
        injector::WriteMemory<float>(pattern.get_first(28), 1.0f, true);

        auto a = 0;
        auto b = 0;

        switch (nIncreaseNoiseEffectRes)
        {
        case 2:
            a = 960;
            b = 240;
            break;
        case 3:
            a = 1280;
            b = 320;
            break;
        case 4:
            a = 1920;
            b = 480;
            break;
        default:
            a = 640;
            b = 160;
            break;
        }

        pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 50 FF 52 50"); //478373 478378
        injector::WriteMemory(pattern.get_first(1), a, true);
        injector::WriteMemory(pattern.get_first(6), a, true);

        pattern = hook::pattern("8B 5C 24 10 55 56 57 BD"); //4783FA
        injector::WriteMemory(pattern.get_first(8), b, true);

        pattern = hook::pattern("8B F3 BF ? ? ? ? E8"); //478402
        injector::WriteMemory(pattern.get_first(3), b, true);
    }

    if (bFullscreenImages)
    {
        static auto unk_1DBFC50 = (uint32_t*)(*hook::get_pattern<uint32_t>("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 80 00 00 00", 1) + 0x10);
        //pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 3B C5 74 ? 8B 08");
        //hbsub_457B40.fun = injector::MakeCALL(pattern.get_first(0), sub_457B40Hook).get(); //0x49FCA5

        static uint32_t images[] = { 
            0x00000004, 0x00000008, 0x0000000A, 0x0000000C, 0x0000000E, 0x00000010, 0x00000014, 0x00000016, 0x00000018, 0x0000001E, 0x0000001C, 0x00000020,
            0x00000022, 0x00000024, 0x00000026, 0x00000028, 0x0000002A, 0x0000002C, 0x0000002E, 0x00000030, 0x00000032, 0x00000036, 0x0000003A, 0x0000003C,
            0x0000003E, 0x00000042, 0x00000046, 0x00000048, 0x00000074, 0x00000076, 0x0000007A, 0x0000007C, 0x0000007E, 0x00000082, 0x00000086, 0x00000088,
            0x0000008A, 0x0000008E, 0x00000094, 0x00000096, 0x0000009A, 0x0000009C, 0x000000A6, 0x000000A8, 0x000000B0, 0x000000B4, 0x000000B6, 0x000000B8,
            0x000000BA, 0x000000BE, 0x000000C2, 0x000000C4, 0x000000C8, 0x000000CA, 0x00000130, 0x00000134, 0x00000138, 0x0000013A, 0x0000013C, 0x0000013E,
            0x00000140, 0x00000142, 0x0000014E, 0x00000150, 0x00000152, 0x00000154, 0x00000156,
            0x00000002, 0x00000006, 0x00000012, 0x0000001A, 0x00000072, 0x00000078, 0x00000080, 0x00000084, 0x0000008C, 0x00000090, 0x00000092, 0x00000098,
            0x000000A4, 0x00000038, 0x00000034, 0x00000040, 0x00000044, 0x000000AA, 0x000000AC, 0x000000B2, 0x000000BC, 0x000000AE, 0x000000CC, 0x00000170,
            0x00000174, 0x00000172, 0x00000176, 0x00000184, 0x00000186, 0x00000178, 0x00000188, 0x0000018A, 0x0000018C, 0x0000018E, 0x00000194, 0x0000017A,
            0x00000190, 0x00000192, 0x0000017C, 0x0000017E, 0x00000180, 0x00000182, 0x00000196, 0x00000198, 0x0000019E, 0x000001A0, 0x000001A2, 0x000001A4,
            0x000001A6, 0x0000012E, 0x00000130, 0x00000144
        };

        static auto isFullscreenImage = []() -> bool
        {
            return std::any_of(std::begin(images), std::end(images), [](uint32_t i) { return i == *unk_1DBFC50; });
        };

        pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 81 EC C4 00 00 00 84 C9");
        struct ImagesHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                int32_t z = static_cast<int32_t>(Screen.fHeight * (1440.0f / 870.0f));
                if (isFullscreenImage())
                    _asm fild dword ptr[z]
                else
                    _asm fild dword ptr[Screen.Width43]
            }
        }; injector::MakeInline<ImagesHook1>(pattern.get_first(0), pattern.get_first(6)); //0x49F294, 0x49F294+6

        struct ImagesHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                int32_t z = Screen.Height + Screen.FullscreenOffsetY + Screen.FullscreenOffsetY;
                if (isFullscreenImage())
                    _asm fild dword ptr[z]
                else
                    _asm fild dword ptr[Screen.Height]
            }
        };
        pattern = hook::pattern("DB 05 ? ? ? ? 7D ? D8 05 ? ? ? ? D9 54 24 10 0F BE");
        injector::MakeInline<ImagesHook2>(pattern.get_first(0), pattern.get_first(6)); //0x49F2F8, 0x49F2F8 + 6
        pattern = hook::pattern("DB 05 ? ? ? ? 7D ? D8 05 ? ? ? ? 0F BE 05 ? ? ? ? D9 54 24 10 D8");
        injector::MakeInline<ImagesHook2>(pattern.get_first(0), pattern.get_first(6)); //0x49F4CA, 0x49F4CA + 6
        pattern = hook::pattern("DB 05 ? ? ? ? 7D ? D8 05 ? ? ? ? 0F BE 15 ? ? ? ? D9 54 24 10");
        injector::MakeInline<ImagesHook2>(pattern.get_first(0), pattern.get_first(6)); //0x49F5CE, 0x49F5CE + 6

        struct ImagesHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                if (isFullscreenImage())
                    *(int32_t*)&regs.edx = 0 - Screen.FullscreenOffsetX;
                else
                    *(int32_t*)&regs.edx = 0;
            }
        };
        pattern = hook::pattern("0F BE 15 ? ? ? ? D9 05 ? ? ? ? 0F BF 46 0C D8 C9");
        injector::MakeInline<ImagesHook3>(pattern.get_first(0), pattern.get_first(7)); //0x49F2B7, 0x49F2B7 + 7
        pattern = hook::pattern("0F BE 15 ? ? ? ? D9 54 24 00 D8 0D");
        injector::MakeInline<ImagesHook3>(pattern.get_first(0), pattern.get_first(7)); //0x49F479, 0x49F479 + 7
        pattern = hook::pattern("0F BE 15 ? ? ? ? D8 C9 A1 ? ? ? ? 85 C0 89");
        injector::MakeInline<ImagesHook3>(pattern.get_first(0), pattern.get_first(7)); //0x49F596, 0x49F596 + 7

        pattern = hook::pattern("0F BE 15 ? ? ? ? D9 54 24 10 D8 0D");
        struct ImagesHook4
        {
            void operator()(injector::reg_pack& regs)
            {
                if (isFullscreenImage())
                    *(int32_t*)&regs.edx = 0 - Screen.FullscreenOffsetY;
                else
                    *(int32_t*)&regs.edx = 0;
            }
        };
        injector::MakeInline<ImagesHook4>(pattern.get_first(0), pattern.get_first(7)); //0x49F5DC, 0x49F5DC + 7

        struct ImagesHook5
        {
            void operator()(injector::reg_pack& regs)
            {
                if (isFullscreenImage())
                    *(int32_t*)&regs.eax = 0 - Screen.FullscreenOffsetY;
                else
                    *(int32_t*)&regs.eax = 0;
            }
        };
        pattern = hook::pattern("0F BE 05 ? ? ? ? D8 0D ? ? ? ? 0F BF");
        injector::MakeInline<ImagesHook5>(pattern.get_first(0), pattern.get_first(7)); //0x49F30A, 0x49F30A + 7
        pattern = hook::pattern("0F BE 05 ? ? ? ? D9 54 24 10 D8 0D ? ? ? ? 0F BF 56 0E");
        injector::MakeInline<ImagesHook5>(pattern.get_first(0), pattern.get_first(7)); //0x49F4D8, 0x49F4D8 + 7
    }

    // Fixes lying figure cutscene bug; original value 00000005; issue #349
    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C4 04 83 F8 20");
    injector::WriteMemory(pattern.get_first(6), 0x0000002A, true);

    return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
    }
    return TRUE;
}
