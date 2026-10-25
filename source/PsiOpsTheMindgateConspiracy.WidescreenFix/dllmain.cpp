#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"
#include "NativeMenu.hpp"

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
    float fHudPos;
} Screen;

void Init()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    PsiOpsMenu::Init();
    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");
    const bool bFixHUD = settings.Bool("MAIN.FixHUD");

    static const auto& fFOVFactor = settings.Float("MAIN.FOVFactor");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 32767 || Screen.Height > 32767)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    if (wfp::PatchPreflight("Psi-Ops resolution: signatures unavailable; feature disabled.\n",
    {
        { "FF 24 95 ? ? ? ? 8B 45 F8 0F B6 08 85 C9", 1 },
        { "C7 02 ? ? ? ? 8B 45 0C C7 00 ? ? ? ? C6 45 FF 01", 1 },
    }))
    {
        pattern = hook::pattern("FF 24 95 ? ? ? ? 8B 45 F8 0F B6 08 85 C9"); //0x7A3B5E
        injector::MakeNOP(pattern.count_hint(1).get(0).get<uint32_t>(0), 7, true);

        pattern = hook::pattern("C7 02 ? ? ? ? 8B 45 0C C7 00 ? ? ? ? C6 45 FF 01"); //0x7A3B72
        ProtectedGameRef<int32_t>{pattern.count_hint(1).get(0).get<int32_t>(2)} = Screen.Width;
        ProtectedGameRef<int32_t>{pattern.count_hint(1).get(0).get<int32_t>(11)} = Screen.Height;

    }

    //fmv
    Screen.Width43 = static_cast<uint32_t>(Screen.fWidth * (4.0f / 3.0f));
    if (wfp::PatchPreflight("Psi-Ops movie width: signatures unavailable; feature disabled.\n",
    {
        { "66 8B 15 ? ? ? ? 66 89 15 ? ? ? ? 66 A1 ? ? ? ? 66 A3", 1 },
    }))
    {
        pattern = hook::pattern("66 8B 15 ? ? ? ? 66 89 15 ? ? ? ? 66 A1 ? ? ? ? 66 A3"); //0x4BCD2F
        ProtectedGameRef<int32_t*>{pattern.count_hint(1).get(0).get<int32_t*>(3)} = &Screen.Width43;
        //injector::WriteMemory(0x4BCD49 + 3, &Screen.Width43, true);

    }

    if (bFixHUD)
    {
        if (wfp::PatchPreflight("Psi-Ops HUD: signatures unavailable; feature disabled.\n",
        {
            { "68 ? ? ? ? 8D 85 00 FF FF FF 50 E8", 1 },
            { "D9 05 ? ? ? ? D8 75 10 51 D9 1C 24 8D 55 80 52", 1 },
        }))
        {
            Screen.fHudPos = (-1.0f * (4.0f / 3.0f)) / (Screen.fAspectRatio);
            Screen.fHudScale = (2.0f * (4.0f / 3.0f)) / (Screen.fAspectRatio);

            pattern = hook::pattern("68 ? ? ? ? 8D 85 00 FF FF FF 50 E8"); //0x756CF8
            ProtectedGameRef<float>{pattern.count_hint(1).get(0).get<float>(1)} = Screen.fHudPos;

            pattern = hook::pattern("D9 05 ? ? ? ? D8 75 10 51 D9 1C 24 8D 55 80 52"); //0x756D23
            ProtectedGameRef<float*>{pattern.count_hint(1).get(0).get<float*>(2)} = &Screen.fHudScale;
        }


    }

    pattern = hook::pattern("D9 45 18 D8 45 14 D8 4D F8 51 D9 1C 24 6A 00");
    if (pattern.size() == 1)
    {
        static ProtectedGameRef<float> CameraFactor{ pattern.get_first<float>(-4) };
        auto updateFOV = []()
        {
            CameraFactor = -(std::isfinite(fFOVFactor) ? std::clamp(fFOVFactor, 0.5f, 2.0f) : 1.0f);
        };
        updateFOV();
        settings.OnChange("MAIN.FOVFactor", updateFOV);
    }
    else
        OutputDebugStringA("PsiOps: FOV pattern unavailable.\n");

    if (settings.Bool("MAIN.SkipIntro"))
    {
        // The startup logo movies (publisher, GameSpy, Havok, legal) play only when this flag is set; story movies stay
        pattern = hook::pattern("8A 0D ? ? ? ? 51 E8 ? ? ? ? 8B C8 E8 ? ? ? ? C6 05");
        if (pattern.size() == 1)
        {
            static constexpr uint8_t clearFlag[] = { 0x31, 0xC9, 0x90, 0x90, 0x90, 0x90 }; // xor ecx, ecx
            for (size_t i = 0; i < sizeof(clearFlag); ++i)
                ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(i)} = clearFlag[i];
        }
        else
            OutputDebugStringA("PsiOps: intro pattern unavailable.\n");
    }
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            // Real pixels on scaled desktops (desktop resolution, full screen picture not scaled up from a corner)
            WindowedModeWrapper::EnableDpiAwareness();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("64 89 25 00 00 00 00 83 C4 90 53"));
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