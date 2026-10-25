#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"


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
} Screen;

void Init()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();

    Screen.Width = settings.Int("MAIN.ResX");
    Screen.Height = settings.Int("MAIN.ResY");
    const bool bFixFOV = settings.Bool("MAIN.FixFOV");
    const bool bFixHUD = settings.Bool("MAIN.FixHUD");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    if (wfp::PatchPreflight("Street Racing Syndicate resolution: signatures unavailable; feature disabled.\n",
    {
        { "C7 47 14 ? ? ? ? C7 47 18", 1 },
        { "89 4F 14 D8 0D ? ? ? ? D9 5F 18", 1 },
        { "84 DB 74 09 DB 87 ? ? ? ? D9 5F 14", 1 },
        { "D8 0D ? ? ? ? D9 5C 24 04 D9 46", 1 },
        { "C7 05 ? ? ? ? 00 00 20 44", 0 },
        { "68 00 00 F0 43 68 00 00 20 44", 0 },
    }))
    {
        pattern = hook::pattern("C7 47 14 ? ? ? ? C7 47 18"); //0x4C81A3 0x4C81AA
        ProtectedGameRef<float>{pattern.count_hint(1).get(0).get<float>(3)} = Screen.fWidth;
        ProtectedGameRef<float>{pattern.count_hint(1).get(0).get<float>(10)} = Screen.fHeight;

        pattern = hook::pattern("89 4F 14 D8 0D ? ? ? ? D9 5F 18"); //0x4C8235
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float temp = 0.0f;
                _asm {fstp    dword ptr[temp]}

                *(float*)(regs.edi + 0x14) = Screen.fWidth;
                *(float*)(regs.edi + 0x18) = Screen.fHeight;
            }
        }; injector::MakeInline<SetResHook>(pattern.count_hint(1).get(0).get<uint32_t>(3), pattern.count_hint(1).get(0).get<uint32_t>(12));

        pattern = hook::pattern("84 DB 74 09 DB 87 ? ? ? ? D9 5F 14"); //0x4C8241
        injector::MakeNOP(pattern.count_hint(1).get(0).get<uint32_t>(0), 13, true);

        //3D
        static float f3DScale = 1.0f / (480.0f * Screen.fAspectRatio);
        pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 04 D9 46"); //0x4BC66B
        ProtectedGameRef<float*>{pattern.count_hint(1).get(0).get<float*>(2)} = &f3DScale;

        pattern = hook::pattern("C7 05 ? ? ? ? 00 00 20 44"); //0x553DFC //mov
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            ProtectedGameRef<float>{pattern.get(i).get<float>(6)} = 480.0f * Screen.fAspectRatio;
        }

        pattern = hook::pattern("68 00 00 F0 43 68 00 00 20 44"); //0x4CA7E6 //push
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            ProtectedGameRef<float>{pattern.get(i).get<float>(6)} = 480.0f * Screen.fAspectRatio;
        }

    }

    if (bFixHUD)
    {
        if (wfp::PatchPreflight("Street Racing Syndicate HUD: signatures unavailable; feature disabled.\n",
        {
            { "8B 81 ? ? ? ? D8 40 6C 5E D9 58 6C", 1 },
            { "DB 44 24 08 8D 86 A8 00 00 00 8D", 1 },
        }))
        {
            Screen.fHudScale = Screen.fAspectRatio / (4.0f / 3.0f);
            pattern = hook::pattern("8B 81 ? ? ? ? D8 40 6C 5E D9 58 6C"); //0x4CB3C5
            struct HudScaleHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto* hud = *reinterpret_cast<float**>(regs.ecx + 0x0A859C);
                    if (hud && hud[2] != 90.0f && *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(hud) + 0x18) != 235587392) //fading check
                        hud[0] /= Screen.fHudScale;

                    regs.eax = *(uintptr_t*)(regs.ecx + 0x0C3B40);
                }
            }; injector::MakeInline<HudScaleHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(6));

            pattern = hook::pattern("DB 44 24 08 8D 86 A8 00 00 00 8D"); //0x5288D2
            struct RearviewMirrorHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    int32_t fMirrorOffset = *(int32_t*)(regs.esp + 0x8) + (int32_t)(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
                    _asm {fild    dword ptr[fMirrorOffset]}
                    regs.eax = regs.esi + 0xA8;
                }
            }; injector::MakeInline<RearviewMirrorHook>(pattern.count_hint(1).get(0).get<uint32_t>(0), pattern.count_hint(1).get(0).get<uint32_t>(10));
        }


    }

    // Gameplay projection scale: Hor+ when FixFOV is on, times MAIN.FOVFactor (live)
    if (wfp::PatchPreflight("Street Racing Syndicate FOV: signatures unavailable; feature disabled.\n",
    {
        { "D8 0D ? ? ? ? 8D B2 ? ? ? ? 33 C0 8B FE D9 F2", 1 },
    }))
    {
        pattern = hook::pattern("D8 0D ? ? ? ? 8D B2 ? ? ? ? 33 C0 8B FE D9 F2"); //0x4BC4BB
        static float fBaseFOV = bFixFOV ? GetFOV2(0.55f, Screen.fAspectRatio) : **pattern.count_hint(1).get(0).get<float*>(2);
        static auto updateFOV = []()
        {
            Screen.fFieldOfView = std::min(fBaseFOV * std::clamp(FixSettings().Float("MAIN.FOVFactor"), 0.5f, 2.0f), 1.45f);
        };
        updateFOV();
        settings.OnChange("MAIN.FOVFactor", [] { updateFOV(); });
        ProtectedGameRef<float*>{pattern.count_hint(1).get(0).get<float*>(2)} = &Screen.fFieldOfView;
    }


}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            WindowedModeWrapper::EnableDpiAwareness();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 47 14 ? ? ? ? C7 47 18"));
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