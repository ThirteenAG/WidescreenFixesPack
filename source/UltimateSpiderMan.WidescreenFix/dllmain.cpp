#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fWidth43;
    float fAspectRatio;
    float fFieldOfView;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
    float fHudPos;
    float fAspectRatioDiff;
    float fScale;
    float fDynamicScreenFieldOfViewScale;
    float fCustomFieldOfView;
} Screen;

void* sub_76B4D0_addr;
void __cdecl sub_76B6D0(float a1, float a2, float a3, float a4)
{
    auto v4 = *(uint32_t*)0x971F00;
    float v12 = 640.0f;
    float v6 = 480.0f;

    if (!(*(uint8_t*)(*(uint32_t*)(v4 + 0x334) + 0x34) & 4))
    {
        v12 = static_cast<float>(*(uint32_t*)(*(uint32_t*)(v4 + 0x334) + 0x18));
        v6 = static_cast<float>(*(uint32_t*)(*(uint32_t*)(v4 + 0x334) + 0x1C));
    }

    float v8 = 1.0f / v12;
    float v10 = 1.0f / v6;
    *(float*)(v4 + 0x364) = a1 * v8 + a1 * v8 - 1.0f;
    *(float*)(v4 + 0x368) = a2 * v10 + a2 * v10 - 1.0f;
    *(float*)(v4 + 0x36C) = (a3 + 1.0f) * v8 + (a3 + 1.0f) * v8 - 1.0f;
    *(float*)(v4 + 0x370) = (a4 + 1.0f) * v10 + (a4 + 1.0f) * v10 - 1.0f;
    *(uint8_t*)(v4 + 0x3E4) = 1;


    //float t = a3 - a1;
    //a1 -= Screen.fHudOffset / (Screen.fWidth / 640.0f);
    //a3 = a1 + (t / (Screen.fAspectRatio / (4.0f / 3.0f)));
    //
    //float v9 = a1 * v8 + a1 * v8 - 1.0f;
    //float v14 = a2 * v10 + a2 * v10 - 1.0f;
    //float v15 = (a3 + 1.0f) * v8 + (a3 + 1.0f) * v8 - 1.0f;
    //float v13 = (a4 + 1.0f) * v10 + (a4 + 1.0f) * v10 - 1.0f;
    
    auto _sub_76B4D0 = (void(__cdecl *)(float, float, float, float)) sub_76B4D0_addr;
    _sub_76B4D0(0.0f, 0.0f, 0.0f, 0.0f); //_sub_76B4D0(v9, v14, v15, v13);
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("81 EC EC 03 00 00 68");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 0) != 0;
    Screen.fCustomFieldOfView = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    int32_t nFPSLimit = iniReader.ReadInteger("MAIN", "FPSLimit", 0);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    //Default resolution
    pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 35"); //0x5AC3C8
    static std::string defRes = std::to_string(Screen.Width) + "x" + std::to_string(Screen.Height);
    injector::WriteMemory(pattern.get_first(1), &(*defRes.begin()), true);

    pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? E8 ? ? ? ? 6A 00"); //0x5AD20D
    static auto nWidth  = *pattern.get_first<uint32_t*>(1);
    static auto nHeight = *pattern.get_first<uint32_t*>(7);
    struct GetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *nWidth = regs.eax;
            *nHeight = regs.ecx;

            Screen.Width = regs.eax;
            Screen.Height = regs.ecx;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.Width43);
            Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fHudPos = (320.0f / ((4.0f / 3.0f) / (Screen.fAspectRatio)));
            Screen.fScale = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio)));
            Screen.fDynamicScreenFieldOfViewScale = (2.25f / ((4.0f / 3.0f) / (Screen.fAspectRatio))) * (Screen.fCustomFieldOfView ? Screen.fCustomFieldOfView : 1.0f);
        }
    }; injector::MakeInline<GetResHook>(pattern.get_first(0), pattern.get_first(11));

    //3D stretching
    pattern = hook::pattern("D8 0D ? ? ? ? E8 ? ? ? ? D9 05 ? ? ? ? D8 25 ? ? ? ? 89 44 24 20 DA 4C 24 38"); //0x53A96C
    injector::WriteMemory(pattern.get_first(2), &Screen.fScale, true);
    pattern = hook::pattern("D8 0D ? ? ? ? D9 55 1C DC C0"); //0x4B3F4B
    injector::WriteMemory(pattern.get_first(2), &Screen.fDynamicScreenFieldOfViewScale, true);
    
    //FMVs
    //pattern = hook::pattern("8D 84 24 14 01 00 00 53 50"); //0x629B1A
    //struct FMVHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        regs.eax = regs.esp + 0x114;
    //        *(float*)&regs.ebx = (Screen.fHudOffset / ((Screen.fAspectRatio) / (4.0f / 3.0f)));
    //        *(float*)(regs.esp + 4) = 640.0f - (Screen.fHudOffset / ((Screen.fAspectRatio) / (4.0f / 3.0f)));
    //    }
    //}; 
    //injector::MakeInline<FMVHook>(pattern.get_first(0), pattern.get_first(7));
    //injector::WriteMemory<uint8_t>(pattern.get_first(5), 0x53i8, true); //push ebx
    //injector::WriteMemory<uint16_t>(pattern.get_first(6), 0xDB33i16, true); //xor ebx, ebx

    if (bFixHUD)
    {
        pattern = hook::pattern("C7 44 24 08 CD CC 4C 3B C7 44 24 0C 89 88 88 BB"); //0x640E79
        struct HUDPart1
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x08) = ((1.0f / 320.0f) * ((4.0f / 3.0f) / (Screen.fAspectRatio)));
            }
        }; injector::MakeInline<HUDPart1>(pattern.get_first(0), pattern.get_first(8));

        pattern = hook::pattern("C7 44 24 04 00 00 80 BF 8B 4C 24 04"); //0x640EA2
        struct HUDPart2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x04) = (-1.0f * ((4.0f / 3.0f) / (Screen.fAspectRatio)));
            }
        }; injector::MakeInline<HUDPart2>(pattern.get_first(0), pattern.get_first(8));

        pattern = hook::pattern("C7 44 24 18 00 00 80 BF C7 44 24 1C 00 00 80 BF"); //0x76A7DC
        struct HUDPart3
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x18) = (-1.0f * ((4.0f / 3.0f) / (Screen.fAspectRatio)));
            }
        }; injector::MakeInline<HUDPart3>(pattern.get_first(0), pattern.get_first(8));

        pattern = hook::pattern("D9 05 ? ? ? ? D9 05 ? ? ? ? EB ? 8B 80"); //0x76A776
        injector::WriteMemory(pattern.get_first(2), &Screen.fHudPos, true); //breaks minimap blips

        //Radar
        pattern = hook::pattern("D9 05 ? ? ? ? BE ? ? ? ? 8D 7C"); //0x53ABBB
        injector::WriteMemory(pattern.get_first(2), &Screen.fScale, true); //blips on global map fix

        sub_76B4D0_addr = hook::get_pattern("D9 44 24 04 83 EC 08 D8 1D ? ? ? ? DF E0 F6 C4 05");
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 10 8D 54 24 0F 52 8D 4C 24 48"); //0x641AA1
        injector::MakeCALL(pattern.get_first(0), sub_76B6D0, true);
    }

    if (nFPSLimit)
    {
        static float fFPSLimit = static_cast<float>(nFPSLimit * 2);
        pattern = hook::pattern("D8 3D ? ? ? ? D9 5C 24 0C"); //0x76E7CD
        injector::WriteMemory(pattern.get_first(2), &fFPSLimit, true);
    }

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