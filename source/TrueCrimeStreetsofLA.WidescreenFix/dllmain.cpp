#include "stdafx.h"
#include "FixSettings.hpp"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fAspectRatioDiff;
    int Width43;
    float fWidth43;
    float fHudScale;
} Screen;

void __stdcall Ret4(uint32_t)
{
}

SafetyHookInline sub_5EE430_hook{};
void __fastcall sub_5EE430(int _this, void* edx, float a2)
{
    // a2 scales the view frustum extents: larger is wider
    static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
    if (std::isfinite(a2) && Screen.fAspectRatioDiff > 0.0f)
        a2 = a2 / Screen.fAspectRatioDiff * std::clamp(factor, 0.5f, 2.0f);
    return sub_5EE430_hook.unsafe_fastcall<void>(_this, edx, a2);
}

void Init()
{
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    hook::pattern pattern("");

    //ini res crash fix
    pattern = hook::pattern("74 21 8B 4C 24 10 8B 54 24 0C 8B 74 24 08"); //0x62AC16
    if (pattern.size() >= 1)
    {
        ProtectedGameRef<uint8_t>{pattern.count_hint(1).get(0).get<uint8_t>(0)} = 0xEB;
    }
    else
        OutputDebugStringA("True Crime: patch group unavailable; native behavior retained.\n");

    //display crashfix
    pattern = hook::pattern("E8 ? ? ? ? 8B 96 0C 0B 00 00 8D 8E 0C 0B 00 00 6A 00"); //0x58063B
    if (pattern.size() >= 1)
    {
        injector::MakeCALL(pattern.count_hint(1).get(0).get<uint32_t>(0), Ret4, true);
    }
    else
        OutputDebugStringA("True Crime: patch group unavailable; native behavior retained.\n");

    pattern = hook::pattern("89 96 2C 05 00 00 8B 41 18"); //0x62BC08 
    if (pattern.size() > 0)
    {
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *reinterpret_cast<uint32_t*>(regs.esi + 0x528) = Screen.Width;
                *reinterpret_cast<uint32_t*>(regs.esi + 0x52C) = Screen.Height;
            }
        }; injector::MakeInline<SetResHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
    }
    else
    {
        pattern = hook::pattern("89 91 2C 05 00 00 8B 46 18 8B 50 04"); //0x61D630
        if (pattern.size() > 0)
        {
            struct SetResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *reinterpret_cast<uint32_t*>(regs.ecx + 0x528) = Screen.Width;
                    *reinterpret_cast<uint32_t*>(regs.ecx + 0x52C) = Screen.Height;
                }
            }; injector::MakeInline<SetResHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
        }
    }
    pattern = hook::pattern("8D 99 28 05 00 00 8B 49 24 53"); //0x62C1F2
    if (pattern.size() >= 1)
    {
        struct SetResHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ebx = regs.ecx + 0x528;
                *reinterpret_cast<uint32_t*>(regs.ecx + 0x528) = Screen.Width;
                *reinterpret_cast<uint32_t*>(regs.ecx + 0x52C) = Screen.Height;
            }
        }; injector::MakeInline<SetResHook2>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));
    }
    else
        OutputDebugStringA("True Crime: patch group unavailable; native behavior retained.\n");

    pattern = hook::pattern("89 0D ? ? ? ? 8B 56 04 89 15 ? ? ? ? 8B 4E 0C"); //0x5792C4
    if (pattern.size() >= 1)
    {

        **pattern.get_first<uint32_t*>(2) = Screen.Width;
        **pattern.get_first<uint32_t*>(11) = Screen.Height;

        injector::MakeNOP(pattern.count_hint(1).get(0).get<uint32_t>(0), 6, true);
        injector::MakeNOP(pattern.count_hint(1).get(0).get<uint32_t>(9), 6, true);
    }
    else
        OutputDebugStringA("True Crime: patch group unavailable; native behavior retained.\n");

    pattern = hook::pattern("8B D8 53 8D ? 24 10 68 ? ? ? ? ? E8"); //0x62FCD5
    if (pattern.size() >= 3)
    {
        struct SetIniResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ebx = Screen.Width;
                regs.edx = regs.esp + 0x10;
            }
        }; injector::MakeInline<SetIniResHook>(pattern.count_hint(3).get(1).get<uint32_t>(0), pattern.count_hint(3).get(1).get<uint32_t>(7));
        ProtectedGameRef<uint8_t>{pattern.count_hint(3).get(1).get<uint8_t>(5)} = 0x53; //push ebx

        struct SetIniResHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ebx = Screen.Height;
                regs.ecx = regs.esp + 0x10;
            }
        }; injector::MakeInline<SetIniResHook2>(pattern.count_hint(3).get(2).get<uint32_t>(0), pattern.count_hint(3).get(2).get<uint32_t>(7));
        ProtectedGameRef<uint8_t>{pattern.count_hint(3).get(2).get<uint8_t>(5)} = 0x53; //push ebx
    }
    else
        OutputDebugStringA("True Crime: patch group unavailable; native behavior retained.\n");

    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 64 F3 0F 10 75");
    if (pattern.size() >= 1)
    {
        sub_5EE430_hook = safetyhook::create_inline(pattern.get_first(0), sub_5EE430);
    }
    else
        OutputDebugStringA("True Crime: patch group unavailable; native behavior retained.\n");


}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        WindowedModeWrapper::EnableDpiAwareness();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("BF 94 00 00 00 8B C7"));
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