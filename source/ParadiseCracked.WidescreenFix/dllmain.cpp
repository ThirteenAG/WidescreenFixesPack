#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
} Screen;

void Init()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 32767 || Screen.Height > 32767)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    if (wfp::PatchPreflight("Paradise Cracked resolution: signatures unavailable; feature disabled.\n",
    {
        { "68 58 02 00 00 68 20 03 00 00", 1 },
        { "BE 20 03 00 00", 0 },
        { "BF 58 02 00 00", 0 },
        { "74 22 48 75 25 68", 1 },
        { "83 7D F0 02 75 ? 8B 45 08", 1 },
        { "C7 45 D0 20 03 00 00", 0 },
        { "C7 45 D4 58 02 00 00", 0 },
        { "C7 85 5C FF FF FF 20 03 00 00", 1 },
        { "C7 85 60 FF FF FF 58 02 00 00", 1 },
        { "D9 05 ? ? ? ? 83 EC 10 D9 5C 24 0C", 1 },
    }))
    {
        pattern = hook::pattern("68 58 02 00 00 68 20 03 00 00");
        ProtectedGameRef<uint32_t>{pattern.count_hint(1).get(0).get<uint32_t>(1)} = Screen.Height;
        ProtectedGameRef<uint32_t>{pattern.count_hint(1).get(0).get<uint32_t>(6)} = Screen.Width;

        pattern = hook::pattern("BE 20 03 00 00");
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            ProtectedGameRef<uint32_t>{pattern.get(i).get<uint32_t>(1)} = Screen.Width;
        }
        pattern = hook::pattern("BF 58 02 00 00");
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            ProtectedGameRef<uint32_t>{pattern.get(i).get<uint32_t>(1)} = Screen.Height;
        }

        pattern = hook::pattern("74 22 48 75 25 68");
        ProtectedGameRef<uint8_t>{pattern.count_hint(1).get(0).get<uint8_t>(0)} = 0xEB;

        pattern = hook::pattern("83 7D F0 02 75 ? 8B 45 08");
        ProtectedGameRef<uint8_t>{pattern.count_hint(1).get(0).get<uint8_t>(4)} = 0xEB;


        pattern = hook::pattern("C7 45 D0 20 03 00 00");
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            ProtectedGameRef<uint32_t>{pattern.get(i).get<uint32_t>(3)} = Screen.Width;
        }

        pattern = hook::pattern("C7 45 D4 58 02 00 00");
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            ProtectedGameRef<uint32_t>{pattern.get(i).get<uint32_t>(3)} = Screen.Height;
        }

        pattern = hook::pattern("C7 85 5C FF FF FF 20 03 00 00");
        ProtectedGameRef<uint32_t>{pattern.count_hint(1).get(0).get<uint32_t>(6)} = Screen.Width;
        pattern = hook::pattern("C7 85 60 FF FF FF 58 02 00 00");
        ProtectedGameRef<uint32_t>{pattern.count_hint(1).get(0).get<uint32_t>(6)} = Screen.Height;

        const float inverseAspectRatio = Screen.fHeight / Screen.fWidth;
        pattern = hook::pattern("D9 05 ? ? ? ? 83 EC 10 D9 5C 24 0C");
        ProtectedGameRef<float>{*pattern.count_hint(1).get(0).get<float*>(2)} = inverseAspectRatio;
    }


}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            // Real pixels on scaled desktops (desktop resolution, full screen picture not scaled up from a corner)
            WindowedModeWrapper::EnableDpiAwareness();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("83 EC 58 53 56 57 89 65 E8"));
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