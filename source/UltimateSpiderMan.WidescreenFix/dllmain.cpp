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
    float fAspectRatioDiff;
    float fFOVFactor;
} Screen;

void* sub_76B4D0_addr;
uint32_t* dword_971F00;
void __cdecl sub_76B6D0(float a1, float a2, float a3, float a4)
{
    auto v4 = *dword_971F00;
    float v12 = 640.0f;
    float v6 = 480.0f;

    if (!(*(uint8_t*)(*(uint32_t*)(v4 + 0x334) + 0x34) & 4))
    {
        v12 = static_cast<float>(*(uint32_t*)(*(uint32_t*)(v4 + 0x334) + 0x18));
        v6 = static_cast<float>(*(uint32_t*)(*(uint32_t*)(v4 + 0x334) + 0x1C));
    }

    if (false) // fixed radar but broken blips
    {
        v12 *= Screen.fAspectRatioDiff;
        a1 += 106.0f;
        a3 += 106.0f;
    }

    float v8 = 1.0f / v12;
    float v10 = 1.0f / v6;
    *(float*)(v4 + 0x364) = a1 * v8 + a1 * v8 - 1.0f;
    *(float*)(v4 + 0x368) = a2 * v10 + a2 * v10 - 1.0f;
    *(float*)(v4 + 0x36C) = (a3 + 1.0f) * v8 + (a3 + 1.0f) * v8 - 1.0f;
    *(float*)(v4 + 0x370) = (a4 + 1.0f) * v10 + (a4 + 1.0f) * v10 - 1.0f;
    *(uint8_t*)(v4 + 0x3E4) = 1;

    static auto _sub_76B4D0 = (void(__cdecl *)(float, float, float, float)) sub_76B4D0_addr;
    _sub_76B4D0(0.0f, 0.0f, 0.0f, 0.0f);
    //_sub_76B4D0(*(float*)(v4 + 0x364), *(float*)(v4 + 0x368), *(float*)(v4 + 0x36C), *(float*)(v4 + 0x370)); // fixed radar but broken blips
}

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 0) != 0;
    Screen.fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    int32_t nFPSLimit = iniReader.ReadInteger("MAIN", "FPSLimit", 0);

    if (!Screen.fFOVFactor)
        Screen.fFOVFactor = 1.0f;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    //Default resolution
    auto pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 35"); //0x5AC3C8
    static std::string defRes = std::to_string(Screen.Width) + "x" + std::to_string(Screen.Height);
    injector::WriteMemory(pattern.get_first(1), &(*defRes.begin()), true);

    pattern = hook::pattern("A3 ? ? ? ? 89 0D ? ? ? ? E8 ? ? ? ? 6A 00"); //0x5AD20D
    static auto nWidth = *pattern.get_first<uint32_t*>(1);
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
            Screen.fAspectRatioDiff = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fFOVFactor = 0.75f * Screen.fAspectRatioDiff * Screen.fFOVFactor;
        }
    }; injector::MakeInline<GetResHook>(pattern.get_first(0), pattern.get_first(11));

    //Aspect ratio
    static injector::hook_back<void(__cdecl*)(float)> hb_540070;
    auto sub_540070 = [](float f) {
        return hb_540070.fun(f * Screen.fAspectRatioDiff);
    };
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 6A 00 56 8D 4C 24 3C"); //0x738A41
    hb_540070.fun = injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)(float)>(sub_540070), true).get();
    pattern = hook::pattern("E8 ? ? ? ? 8B 16 83 C4 04 57 8B CE FF 52 1C"); //0x741C40
    injector::MakeCALL(pattern.get_first(0), static_cast<void(__cdecl*)(float)>(sub_540070), true);

    //FOV
    pattern = hook::pattern("7A 22 D9 44 24 2C D8 0D ? ? ? ? D9 F2 DD D8");
    injector::MakeNOP(pattern.get_first(0), 2, true);
    injector::WriteMemory(pattern.get_first(18), &Screen.fFOVFactor, true);

    if (bFixHUD)
    {
        //Hud scale
        pattern = hook::pattern("C7 44 24 08 CD CC 4C 3B C7 44 24 0C 89 88 88 BB"); //0x640E79
        struct HUDPart1
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x08) = ((1.0f / 320.0f) / Screen.fAspectRatioDiff);
            }
        }; injector::MakeInline<HUDPart1>(pattern.get_first(0), pattern.get_first(8));

        pattern = hook::pattern("C7 44 24 04 00 00 80 BF 8B 4C 24 04"); //0x640EA2
        struct HUDPart2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x04) = (-1.0f / Screen.fAspectRatioDiff);
            }
        }; injector::MakeInline<HUDPart2>(pattern.get_first(0), pattern.get_first(8));

        pattern = hook::pattern("C7 44 24 18 00 00 80 BF C7 44 24 1C 00 00 80 BF"); //0x76A7DC
        struct HUDPart3
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x18) = (-1.0f / Screen.fAspectRatioDiff);
            }
        }; injector::MakeInline<HUDPart3>(pattern.get_first(0), pattern.get_first(8));

        //Hud pos
        pattern = hook::pattern("89 32 8B 71 04 89 72 04 8B 71 08 89 72 08 8B 49 0C 89 4A 0C 8B 4C 24 0C 8B 31 8D 50 10 89 32 8B 71"); //0x4118FB
        struct HUDPart4
        {
            void operator()(injector::reg_pack& regs)
            {
                auto f1 = *(float*)&regs.esi;
                *(float*)(regs.edx + 0x00) = f1;

                if (fabs((1.0f / f1) - 320.0f) < FLT_EPSILON)
                    *(float*)(regs.edx + 0x00) /= Screen.fAspectRatioDiff;

                regs.esi = *(uint32_t*)(regs.ecx + 0x04);
            }
        }; injector::MakeInline<HUDPart4>(pattern.get_first(0));

        //Disable radar
        dword_971F00 = *hook::get_pattern<uint32_t*>("8B 0D ? ? ? ? 8A 81 BA 03 00 00 84 C0", 2);
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
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("81 EC EC 03 00 00 68"));
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