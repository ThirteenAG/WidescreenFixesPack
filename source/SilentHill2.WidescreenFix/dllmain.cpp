#include "stdafx.h"
#include "GTA\CFileMgr.h"
#include <set>

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

void LoadDatFile(std::string_view str, std::function<void(std::string_view line)>&& cb)
{
    if (FILE* hFile = CFileMgr::OpenFile(str.data(), "r"))
    {
        while (const char* pLine = CFileMgr::LoadLine(hFile))
        {
            if (pLine[0] && pLine[0] != '#')
            {
                cb(pLine);
            }
        }
        CFileMgr::CloseFile(hFile);
    }
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFMVWidescreenMode = iniReader.ReadInteger("MAIN", "FMVWidescreenMode", 1) != 0;
    uint32_t nFMVWidescreenEnhancementPackCompatibility = iniReader.ReadInteger("MAIN", "FMVWidescreenEnhancementPackCompatibility", 0);
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
    Screen.FullscreenOffsetX = static_cast<int32_t>(((Screen.fWidth - (Screen.fWidth * ((4.0f / 3.0f) / (1440.0f / 810.0f)))) / 2.0f) * ((1440.0f / 810.0f) / (4.0f / 3.0f)));
    Screen.FullscreenOffsetY = static_cast<int32_t>(((Screen.fHeight - (Screen.fHeight * ((4.0f / 3.0f) / (1440.0f / 810.0f)))) / 2.0f) * ((1440.0f / 810.0f) / (4.0f / 3.0f)));

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
        auto pattern = hook::pattern(pattern_str(0xDB, 0x05, to_bytes(dword_A37080)));
        for (size_t i = 0; i < pattern.size(); ++i) //http://pastebin.com/ZpkGX9My
        {
            if (i == 318 || i == 315 || i == 314 || i == 313 || i == 310 || i == 312 || i == 311 || i == 320 || i == 178 || i == 177 || i == 176 || i == 175 || i == 174 || i == 327 || i == 2 || i == 309
                || i == 173 || i == 317 || i == 316 || i == 332 || i == 330 || i == 333)
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
        pattern = hook::pattern("8B 0D ? ? ? ? D1 E9 85 C9 89 4C 24 10 DB 44 24 10"); //004DC103 toluca lake light  (i == 330)
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true);
        pattern = hook::pattern("8B 0D ? ? ? ? 51 6A 00 03 D1 52 33 C0"); //00478B74 "screen position" overlay
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true);
        pattern = hook::pattern("8B 0D ? ? ? ? 50 51 33 C0 6A 00 50"); //00478B98 "screen position" overlay
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true);
        pattern = hook::pattern("8B 15 ? ? ? ? 51 52 03 C1 50 33 C0 50"); //00478BB4 "screen position" overlay
        injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width, true);

        pattern = hook::pattern("A3 ? ? ? ? 89 44 24 14 A1 ? ? ? ? 8D 4C 24 0C 51");
        struct SetScaleHook
        {
            void operator()(injector::reg_pack&)
            {
                *dword_A37080 = Screen.Width43;
            }
        }; injector::MakeInline<SetScaleHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5)); //4F5FFD

        pattern = hook::pattern("89 15 ? ? ? ? C6 05 ? ? ? ? 01 33 C0 C3");
        struct SetScaleHook2
        {
            void operator()(injector::reg_pack&)
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
            475,477,479,481,483,485,487,489,491,493,495,497,499,501,503,505,507,509,511,513,515,517,519,521,523,525,527,529,531,533,535,537,539,541,543,545,547,549,551,553,555,
            557,559,561,563,565,567,569,571,574,576,578,580,582,584,586,588,590,592,594,596,598,600,602,604,606,608,628,630,632,634,682,610,612,614,616,618,620,622,624,626 };

        for (size_t i = 0; i < pattern.size(); ++i)
        {
            //logfile << std::dec << i << " " << std::hex << pattern.get(i).get<uint32_t>(2) << std::endl;
#pragma warning(suppress: 4389)
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

                _asm {FSTP DWORD PTR[tempvar1]}
                _asm {FSTP DWORD PTR[tempvar2]}
                _asm {FSTP DWORD PTR[tempvar3]}
                tempvar3 += Screen.TextOffset;
                _asm {FLD  DWORD PTR[tempvar3]}
                _asm {FLD  DWORD PTR[tempvar2]}
                _asm {FLD  DWORD PTR[tempvar1]}
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
            void operator()(injector::reg_pack&)
            {
                _asm {FSTP DWORD PTR[temp]}
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

                _asm {FSTP DWORD PTR[tempvar4]}
                tempvar4 += Screen.TextOffset;
                _asm {FLD  DWORD PTR[tempvar4]}

                *(float*)(regs.esp + 0x6C + 4) += Screen.TextOffset;
            }
        }; injector::MakeInline<TextPosHook3>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(7)); //0x481E70 | sub_481D20+119+


        pattern = hook::pattern("D9 94 24 90 00 00 00 D9 C1 D9 9C 24 94 00 00 00");
        struct TextPosHook4
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm {FSTP DWORD PTR[tempvar5]}
                tempvar5 += Screen.TextOffset;
                _asm {FLD  DWORD PTR[tempvar5]}

                float esp90 = 0.0f;
                _asm {FST DWORD PTR[esp90]}
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
                _asm {FSTP DWORD PTR[tempvar6]}
                tempvar6 += Screen.TextOffset;
                _asm {FLD  DWORD PTR[tempvar6]}
                *(float*)(regs.esp + 0x58) = tempvar6;
                uint32_t esp58 = (regs.esp + 0x58);
                _asm {FST DWORD PTR[esp58]}
                _asm fxch    st(1)
            }
        }; injector::MakeInline<TextPosHook6>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //482117 | sub_481D20+3E3+


        //horizontal lines in text
        pattern = hook::pattern("DB 05 ? ? ? ? 7D ? D8 05 ? ? ? ? D9 5C 24 20 0F BE 15");
        struct TextPosHook7
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm {FSTP DWORD PTR[tempvar7]}
                tempvar7 += Screen.TextOffset;
                _asm {FLD  DWORD PTR[tempvar7]}
                *(float*)(regs.esp + 0x2C) += Screen.TextOffset;
                _asm {FILD DWORD PTR[Screen.Height]}
            }
        }; injector::MakeInline<TextPosHook7>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6)); //482249 | sub_482160+D3 


        pattern = hook::pattern("D9 54 24 40 D9 C1 D9 5C 24 44 D9 CA");
        struct TextPosHook8
        {
            void operator()(injector::reg_pack& regs)
            {
                uint32_t esp40 = (regs.esp + 0x40);
                _asm {FSTP DWORD PTR[tempvar8]}
                tempvar8 += Screen.TextOffset;
                _asm {FLD  DWORD PTR[tempvar8]}
                *(float*)(regs.esp + 0x40) = tempvar8;
                _asm {FST DWORD PTR[esp40]}
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

        //Stretched menu hitboxes #440
        pattern = hook::pattern("DB 05 ? ? ? ? C7 44 24 0C 0A D7 23 3C C7"); //004F6DCC
        injector::WriteMemory(pattern.get_first(2), &Screen.Width43, true);

        pattern = hook::pattern("E8 ? ? ? ? 2D 00 01 00 00 89 44 24 04 DB 44 24 04 D9 1D ? ? ? ? E8 ? ? ? ? 2D F0 00 00 00 89 44 24 04 DB 44 24 04 D9"); //4A2D4E
        auto t = injector::GetBranchDestination(pattern.get_first(0), true);
        static auto dword_944F58 = injector::ReadMemory<uint32_t*>(t.as_int() + 1, true);
        static auto offs = static_cast<int32_t>(((64.0f * Screen.fAspectRatio / (4.0f / 3.0f)) - 64.0f) * 4.0f);

        struct retHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *dword_944F58;
                *(int32_t*)&regs.eax -= offs;
            }
        }; injector::MakeInline<retHook>(t);

        auto sub_45A5E0 = []() -> int32_t
        {
            return *dword_944F58;
        }; injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)()>(sub_45A5E0), true);

        auto x = static_cast<int32_t>(640.0f * (Screen.fAspectRatio / (4.0f / 3.0f)));
        pattern = hook::pattern("81 FE 80 02 00 00 7E 05 BE 80 02 00 00 3B 35 ? ? ? ? EB 17"); //0045A84F
        injector::WriteMemory(pattern.get_first(2), x, true);
        injector::WriteMemory(pattern.get_first(9), x, true);
        
        //FMV Width (Fix 2D)
        auto FMVpattern1 = hook::pattern("A1 ? ? ? ? D9 15 ? ? ? ? D9 C2 89 15 ? ? ? ? D9 1D");
        injector::WriteMemory(FMVpattern1.count(1).get(0).get<uint32_t>(1), &Screen.TextOffset, true); //0043E4D8
        auto FMVpattern2 = hook::pattern("D8 25 ? ? ? ? 8B 0D ? ? ? ? 85 C9 8B 15");
        injector::WriteMemory(FMVpattern2.count(1).get(0).get<uint32_t>(2), &Screen.TextOffset, true); //0043E4C5
        auto FMVpattern3 = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 89 15 ? ? ? ? A3");
        injector::WriteMemory(FMVpattern3.count(1).get(0).get<uint32_t>(2), &Screen.TextOffset, true); //0043E47F
    }

    //solves camera pan inconsistency for different resolutions
    //solves camera tilt inconsistency for different resolutions
    auto pattern = hook::pattern("51 E8 ? ? ? ? 89 44 24 00 DB 44 24 00 D9 1D"); //47CD30
    struct Ret512
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 512;
        }
    };
    struct Ret448
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = 448;
        }
    };
    injector::MakeInline<Ret512>(pattern.get_first(1));
    injector::MakeInline<Ret512>(pattern.get_first(39));
    injector::MakeInline<Ret448>(pattern.get_first(20));
    injector::MakeInline<Ret448>(pattern.get_first(64));

    if (bFMVWidescreenMode || nFMVWidescreenEnhancementPackCompatibility)
    {
        //FMV Width
        auto FMVpattern1 = hook::pattern("A1 ? ? ? ? D9 15 ? ? ? ? D9 C2 89 15 ? ? ? ? D9 1D");
        auto FMVpattern2 = hook::pattern("D8 25 ? ? ? ? 8B 0D ? ? ? ? 85 C9 8B 15");
        auto FMVpattern3 = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 89 15 ? ? ? ? A3");

        static float FMVWidth = Screen.TextOffset - (((Screen.fHeight * (5.0f / 3.0f)) - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);

        if (nFMVWidescreenEnhancementPackCompatibility == 1)
        {
            FMVWidth = Screen.TextOffset - (((Screen.fHeight * (16.0f / 9.0f)) - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
        }

        if (nFMVWidescreenEnhancementPackCompatibility == 2)
        {
            FMVWidth = Screen.TextOffset - (((Screen.fHeight * (16.0f / 9.0f * 1.14f)) - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
        }
        
        if (nFMVWidescreenEnhancementPackCompatibility >= 3)
        {
            FMVWidth = Screen.TextOffset - (((Screen.fHeight * (16.0f / 9.0f * 0.86f)) - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
        }

        injector::WriteMemory(FMVpattern1.count(1).get(0).get<uint32_t>(1), &FMVWidth, true); //0043E4D8
        injector::WriteMemory(FMVpattern2.count(1).get(0).get<uint32_t>(2), &FMVWidth, true); //0043E4C5
        injector::WriteMemory(FMVpattern3.count(1).get(0).get<uint32_t>(2), &FMVWidth, true); //0043E47F

        //FMV Height
        auto FMVpattern4 = hook::pattern("A1 ? ? ? ? 89 15 ? ? ? ? A3 ? ? ? ? C7 05");
        auto FMVpattern5 = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? D9 15");
        auto FMVpattern6 = hook::pattern("D8 25 ? ? ? ? A1 ? ? ? ? 68");

        static float FMVHeight = 0.0f;

        if (nFMVWidescreenEnhancementPackCompatibility == 1)
        {
            FMVHeight = 0.0f;
        }

        if (nFMVWidescreenEnhancementPackCompatibility == 2)
        {
            FMVHeight = ((Screen.fHeight * (4.0f / 3.0f)) / -19.1236f);
        }
        
        if (nFMVWidescreenEnhancementPackCompatibility >= 3)
        {
            FMVHeight = ((Screen.fHeight * (4.0f / 3.0f)) / 19.1236f);
        }
        
        injector::WriteMemory(FMVpattern4.count(1).get(0).get<uint32_t>(1), &FMVHeight, true); //0043E4D8
        injector::WriteMemory(FMVpattern5.count(1).get(0).get<uint32_t>(2), &FMVHeight, true); //0043E4C5
        injector::WriteMemory(FMVpattern6.count(1).get(0).get<uint32_t>(2), &FMVHeight, true); //0043E47F
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
        GetModuleFileNameA(NULL, moduleName, MAX_PATH);
        *(strrchr(moduleName, '\\') + 1) = '\0';
        strcat_s(moduleName, MAX_PATH, "local.fix");

        auto FileExists = [](LPCSTR szPath) -> BOOL
        {
            DWORD dwAttrib = GetFileAttributesA(szPath);
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
        injector::WriteMemory<uint8_t>(pattern.count(2).get(1).get<uint32_t>(3), (uint8_t)nFPSLimit, true); //004F6F53
    }

    if (bPS2CameraSpeed)
    {
        uint32_t* dword_51C262 = hook::pattern("BE ? ? ? ? EB 19 8B 15").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory<float>(*dword_51C262 + 0x08, 4.0f, true); //006C94C0 indoor camera pan speed

        uint32_t* dword_51C276 = hook::pattern("BE ? ? ? ? 74 05 BE ? ? ? ? D9 05").count(1).get(0).get<uint32_t>(1);
        injector::WriteMemory<float>(*dword_51C276 + 0x08, 4.0f, true); //006C94D0 outdoor camera pan speed
        
        pattern = hook::pattern("74 ? D9 05 ? ? ? ? E8 ? ? ? ? 50 E8");
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uint32_t>(0), (uint8_t)0xEBi8, true); //0058C0E3 flesh lips cutscene fix
    }

    if (bGamepadControlsFix)
    {
        pattern = hook::pattern("74 15 39 04 CD ? ? ? ? 75 0C 83 F9 16");
        injector::WriteMemory<uint8_t>(pattern.count(2).get(1).get<uint32_t>(0), 0xEB, true); //5AF936
    }

    static uint8_t* dword_01F7E3C4 = nullptr;
    if (bLightingFix)
    {
        pattern = hook::pattern("8B 10 75 ? 8B 0D");
        auto dword_1F81298 = *pattern.count(2).get(0).get<uintptr_t>(6) - 0x10;
        auto dword_1F8129C = *pattern.count(2).get(1).get<uintptr_t>(6) - 0x10;
        injector::WriteMemory(pattern.count(2).get(0).get<void*>(6), dword_1F81298, true); //50002A solves flashlight bug for NPCs
        injector::WriteMemory(pattern.count(2).get(1).get<void*>(6), dword_1F8129C, true); //5039FB solves flashlight bug for reflective objects

        pattern = hook::pattern("0F B7 05 ? ? ? ? 48 C3");
        static uint16_t* word_9467F0 = *pattern.get_first<uint16_t*>(3);
        dword_01F7E3C4 = *hook::get_pattern<uint8_t*>("A1 ? ? ? ? 83 C4 18 83 C0 F9", 1);
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
        pattern = hook::pattern("D8 0D ? ? ? ? D9 1D ? ? ? ? 0F 87");
        injector::WriteMemory(pattern.get_first(2), &f1472, true); //0059FFBD
        static float f1043 = 1.0437882f / (1.0f / (Screen.fAspectRatio / (4.0f / 3.0f)));
        pattern = hook::pattern("D8 0D ? ? ? ? D9 1D ? ? ? ? E8 ? ? ? ? 53 53 53 53 53");
        injector::WriteMemory(pattern.get_first(2), &f1043, true); //005A18C5
    }

    if (bSteamCrashFix)
    {
        pattern = hook::pattern("57 33 C0 B9 0E 00 00 00"); //45826B
        injector::WriteMemory<uint8_t>(pattern.get_first(-2), (uint8_t)0xEBi8, true);
    }

    if (nIncreaseNoiseEffectRes)
    {
        if (nIncreaseNoiseEffectRes < 128)
        {
            switch (nIncreaseNoiseEffectRes)
            {
            case 2:
                nIncreaseNoiseEffectRes = 640;
                break;
            case 3:
                nIncreaseNoiseEffectRes = 960;
                break;
            case 4:
                nIncreaseNoiseEffectRes = 1440;
                break;
            default:
                nIncreaseNoiseEffectRes = 512;
                break;
            }
        }

        //Eliminates tiling
        pattern = hook::pattern("C7 44 24 2C ? ? ? ? C7 44 24 48 ? ? ? ? C7 44 24 5C ? ? ? ? C7 44 24 60"); //4780E0
        injector::WriteMemory<float>(pattern.get_first(4), 1.0f, true);
        injector::WriteMemory<float>(pattern.get_first(12), 1.0f, true);
        injector::WriteMemory<float>(pattern.get_first(20), 1.0f, true);
        injector::WriteMemory<float>(pattern.get_first(28), 1.0f, true);

        auto x = static_cast<int32_t>((roundf(((float)nIncreaseNoiseEffectRes / ((4.0f / 3.0f) / (Screen.fAspectRatio))) / 4.0f)));
        auto y = static_cast<int32_t>(roundf((float)nIncreaseNoiseEffectRes / 4.0f));

        pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 50 FF 52 50"); //478373 478378
        injector::WriteMemory(pattern.get_first(1), y * 4, true);
        injector::WriteMemory(pattern.get_first(6), x * 4, true);

        pattern = hook::pattern("8B 5C 24 10 55 56 57 BD"); //4783FA
        injector::WriteMemory(pattern.get_first(8), y, true);

        pattern = hook::pattern("8B F3 BF ? ? ? ? E8"); //478402
        injector::WriteMemory(pattern.get_first(3), x, true);
    }

    if (bFullscreenImages && bFix2D)
    {
        static std::set<uint32_t> images;
        auto DataFilePath = iniReader.GetIniPath();
        auto pos = DataFilePath.rfind('.');
        DataFilePath.replace(pos, DataFilePath.length() - pos, ".dat");
        char buf[MAX_PATH];
        GetModuleFileNameA(NULL, buf, MAX_PATH);
        *(strrchr(buf, '\\') + 1) = '\0';
        std::string mPath(buf);
        LoadDatFile(DataFilePath, [&mPath](std::string_view line)
            {
                auto texPath = mPath + std::string(line);
                std::FILE* fp;
                if (fopen_s(&fp, texPath.c_str(), "rb") == 0)
                {
                    uint16_t n;
                    std::fseek(fp, 0x10, SEEK_SET); // seek to start
                    std::fread(&n, sizeof(uint16_t), 1, fp);
                    std::fclose(fp);
                    images.emplace((uint32_t)n);
                }
            });

        static uint32_t* unk_1DBFC50 = nullptr;
        static auto sub_401168 = (uint32_t*(*)())(injector::GetBranchDestination(hook::get_pattern("E8 ? ? ? ? 50 68 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 8B C6 C1", 0)).as_int());

        pattern = hook::pattern("FF 85 00 04 00 00 5F 5E 5D 5B C3 90 90 90 90 90");
        struct PtrHook
        {
            void operator()(injector::reg_pack&)
            {
                unk_1DBFC50 = sub_401168() + 0x4;
            }
        }; injector::MakeInline<PtrHook>(pattern.get_first(10)); //49F282
        injector::MakeRET(pattern.get_first(10 + 5));

        static auto isFullscreenImage = []() -> bool
        {
            return std::any_of(std::begin(images), std::end(images), [](uint32_t i) { return i == *unk_1DBFC50; });
        };

        static auto isFakeFading = []() -> bool
        {
            return (*dword_01F7E3C4 == 0x19 && *unk_1DBFC50 == 656550); //fading for cutscene id 0x19
        };

        pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 81 EC C4 00 00 00 84 C9");
        struct ImagesHook1
        {
            void operator()(injector::reg_pack&)
            {
                int32_t z = static_cast<int32_t>(Screen.fHeight * (1440.0f / 810.0f));

                if (isFakeFading())
                {
                    z = Screen.Width;
                    _asm {fild dword ptr[z]}
                    return;
                }

                if (isFullscreenImage())
                    _asm fild dword ptr[z]
                else
                    _asm fild dword ptr[Screen.Width43]
            }
        }; injector::MakeInline<ImagesHook1>(pattern.get_first(0), pattern.get_first(6)); //0x49F294, 0x49F294+6

        struct ImagesHook2
        {
            void operator()(injector::reg_pack&)
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
                if (isFakeFading())
                {
                    *(int32_t*)&regs.edx = 0 - static_cast<int32_t>(((Screen.fWidth - (Screen.fWidth * ((4.0f / 3.0f) / (Screen.fAspectRatio)))) / 2.0f) * ((Screen.fAspectRatio) / (4.0f / 3.0f)));
                    return;
                }

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

        //mouse boundaries
        static auto f180 = 180.0f;
        static auto fneg180 = -f180;

        pattern = hook::pattern("D9 05 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A 78 D9 05"); //004A2D24
        injector::WriteMemory(pattern.get_first(2), &f180, true);
        pattern = hook::pattern("D9 05 ? ? ? ? D9 1D ? ? ? ? E8 ? ? ? ? 85 C0"); //004A2D37 
        injector::WriteMemory(pattern.get_first(2), &f180, true);
        pattern = hook::pattern("D9 05 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 05 0F 8A ? ? ? ? D9 05 ? ? ? ? D9 1D ? ? ? ? D9 05 ? ? ? ? D8 05"); //004A2FB6 
        injector::WriteMemory(pattern.get_first(2), &f180, true);
        pattern = hook::pattern("D9 05 ? ? ? ? D9 1D ? ? ? ? D9 05 ? ? ? ? D8 05 ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? D9 05"); //004A2FCD 
        injector::WriteMemory(pattern.get_first(2), &f180, true);
        pattern = hook::pattern("D8 05 ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? 83 C4 08 A1 ? ? ? ? 66 81 25"); //004A2FF6 
        injector::WriteMemory(pattern.get_first(2), &f180, true);
        pattern = hook::pattern("2D ? ? ? ? 89 44 24 04 DB 44 24 04 D9 1D ? ? ? ? E9 ? ? ? ? D9 05"); //004A2D6C 
        injector::WriteMemory(pattern.get_first(1), static_cast<int32_t>(f180), true);

        pattern = hook::pattern("D9 05 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 41 0F 85 ? ? ? ? DD D8 "); //004A2DB5 004A3132
        injector::WriteMemory(pattern.count(3).get(0).get<void>(2), &fneg180, true);
        injector::WriteMemory(pattern.count(3).get(2).get<void>(2), &fneg180, true);
        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? E9 ? ? ? ? 8B 15"); //004A314F 
        injector::WriteMemory<float>(pattern.get_first(6), fneg180, true);
        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? E9 ? ? ? ? 6A 00 68 00 00 04 00 6A 00"); //004A2DD2 
        injector::WriteMemory<float>(pattern.get_first(6), fneg180, true);
    }

    // Fixes lying figure cutscene bug; original value 00000005; issue #349
    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C4 04 83 F8 20");
    injector::WriteMemory(pattern.get_first(6), 0x0000002A, true);

    //Loading Text Position #493
    static constexpr auto n640 = 640;
    static constexpr auto n480 = 480;
    pattern = hook::pattern("DB 05 ? ? ? ? 50 A1 ? ? ? ? 85 C0 7D 06 D8 05");
    injector::WriteMemory(pattern.get_first(2), &n640, true); //0044AE96
    pattern = hook::pattern("DB 05 ? ? ? ? 8B 15 ? ? ? ? 85 D2 7D 06 D8 05");
    injector::WriteMemory(pattern.get_first(2), &n480, true); //0044AE6F
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("6A 70 68 ? ? ? ? E8 ? ? ? ? 33 DB 53 8B 3D ? ? ? ? FF D7"));
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
