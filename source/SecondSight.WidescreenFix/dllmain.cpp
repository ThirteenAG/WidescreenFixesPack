#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
} Screen;

void Init()
{
    hook::pattern pattern;
    Screen.Width = FixSettings().Int("MAIN.ResX");
    Screen.Height = FixSettings().Int("MAIN.ResY");
    auto updateFOV = []()
    {
        const float factor = FixSettings().Float("MAIN.FOVFactor");
        Screen.fFieldOfView = 0.4f * (std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f);
    };
    updateFOV();
    FixSettings().OnChange("MAIN.FOVFactor", updateFOV);

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 32767 || Screen.Height > 32767)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    if (wfp::PatchPreflight("Second Sight resolution and projection: signatures unavailable; feature disabled.\n",
    {
        { "A3 ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 83 C4 44", 1 },
        { "D8 0D ? ? ? ? 51 DB 05 ? ? ? ? 8B", 2 },
        { "D8 0D ? ? ? ? 33 C9 D9 44 24 14 89 48 04 D8 64 24 10", 1 },
        { "68 ? ? ? ? 52 E8 ? ? ? ? 8D 44 24 60 83", 1 },
        { "D8 0D ? ? ? ? D9 9C 24 88 00 00 00 E8", 1 },
    }))
    {
        pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 83 C4 44"); //0x40F2CD
        static auto pResY = *pattern.count_hint(1).get(0).get<uint32_t*>(1);
        struct ResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(pResY - 1) = Screen.Width;
                *pResY = Screen.Height;
            }
        }; injector::MakeInline<ResHook>(pattern.count_hint(1).get(0).get<uint32_t>(0));

        static float fARFactor = 0.25f * Screen.fAspectRatio;
        pattern = hook::pattern("D8 0D ? ? ? ? 51 DB 05 ? ? ? ? 8B");
        ProtectedGameRef<float*>{pattern.count_hint(2).get(1).get<float*>(2)} = &fARFactor; //0x43D26B

        pattern = hook::pattern("D8 0D ? ? ? ? 33 C9 D9 44 24 14 89 48 04 D8 64 24 10");
        ProtectedGameRef<float*>{pattern.count_hint(1).get(0).get<float*>(2)} = &Screen.fFieldOfView; //0x5E4B08

        pattern = hook::pattern("68 ? ? ? ? 52 E8 ? ? ? ? 8D 44 24 60 83");
        ProtectedGameRef<float>{pattern.count_hint(1).get(0).get<float>(1)} = Screen.fAspectRatio; //0x55BEB0
        pattern = hook::pattern("D8 0D ? ? ? ? D9 9C 24 88 00 00 00 E8");
        ProtectedGameRef<float>{*pattern.count_hint(1).get(0).get<float*>(2)} = Screen.fAspectRatio; //0x71C714
    }


}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            // Real pixels on scaled desktops (desktop resolution, full screen picture not scaled up from a corner)
            WindowedModeWrapper::EnableDpiAwareness();
            FixSettings();
            asi_settings::LiveLink::Start();
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