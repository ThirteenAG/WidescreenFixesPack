#include "stdafx.h"
#include "FixSettings.hpp"
#include "Video.hpp"
#include "Hud.hpp"
#include "Window.hpp"
#include "FrameRate.hpp"
#include "NativeMenu.hpp"

ASI_SETTINGS_EXPORT(FixSettings())
#include <random>

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fFieldOfView;
    float fFOVFactor;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

namespace THPS4Camera
{
    uintptr_t SkaterType = 0;
    const int* Layout = nullptr;
    const float* NativeZoomScale = nullptr;

    float Correct(uintptr_t camera, float fov, float value)
    {
        float factor = 1.0f;
        if (SkaterType && *reinterpret_cast<uintptr_t*>(camera) == SkaterType && Layout && NativeZoomScale &&
            std::abs(*NativeZoomScale - 1.0f) < 0.001f)
        {
            const int mode = *reinterpret_cast<int*>(camera + 0xBC);
            const auto skater = *reinterpret_cast<uintptr_t*>(camera + 0xC4);
            if (mode >= 1 && mode <= 4 && skater && *reinterpret_cast<uint8_t*>(skater + 13624))
            {
                const float baseline = *reinterpret_cast<float*>(camera + 0xE8) * (*Layout == 1 ? 0.5f : 1.0f);
                const float nativeFov = *reinterpret_cast<float*>(camera + 0xA0);
                const auto animations = *reinterpret_cast<uintptr_t*>(skater + 14300);
                const auto first = animations ? *reinterpret_cast<uintptr_t*>(animations + 0x4C) : 0;
                const bool animated = first && *reinterpret_cast<int*>(first + 4) != -1;
                if (!animated && std::abs(nativeFov - baseline) < 0.001f && std::isfinite(value))
                    factor = std::clamp(value, 0.5f, 2.0f);
            }
        }
        return std::clamp(AdjustFOV(fov, Screen.fAspectRatio) * factor, 1.0f, 179.0f);
    }

    void Init()
    {
        auto type = hook::pattern("C7 06 ? ? ? ? 89 9E BC 00 00 00 39 1D");
        auto layout = hook::pattern("8B F9 E8 ? ? ? ? 48 6A 00 74 ? 48 74 ? 68 ? ? ? ? EB ? 68 ? ? ? ? EB ? 68 ? ? ? ? E8");
        auto scale = hook::pattern("E8 ? ? ? ? D9 44 24 04 D9 F2 DD D8 D9 54 24 04 DE C9 D9 E8 D9 F3");
        auto animation = hook::pattern("51 53 8B D9 55 56 8B 43 4C 57 83 CF FF 33 C9 39 78 04 74 ? 85 C0 74 ? 8B 40 0C 41");
        if (type.size() != 1 || layout.size() != 1 || scale.empty() || animation.size() != 1)
        {
            OutputDebugStringA("THPS4: gameplay camera patterns unavailable.\n");
            return;
        }
        auto layoutGetter = injector::GetBranchDestination(layout.get_first(2)).as_int();
        auto scaleGetter = injector::GetBranchDestination(scale.get_first()).as_int();
        for (size_t i = 1; i < scale.size(); ++i)
        {
            if (injector::GetBranchDestination(scale.get(i).get<void>()).as_int() != scaleGetter)
            {
                OutputDebugStringA("THPS4: camera zoom scale is ambiguous.\n");
                return;
            }
        }
        if (*reinterpret_cast<uint8_t*>(layoutGetter) != 0xA1 || *reinterpret_cast<uint8_t*>(layoutGetter + 5) != 0xC3 ||
            *reinterpret_cast<uint16_t*>(scaleGetter) != 0x05D9 || *reinterpret_cast<uint8_t*>(scaleGetter + 6) != 0xC3)
        {
            OutputDebugStringA("THPS4: camera state getters unavailable.\n");
            return;
        }
        Layout = *reinterpret_cast<const int**>(layoutGetter + 1);
        NativeZoomScale = *reinterpret_cast<const float**>(scaleGetter + 2);
        SkaterType = *type.get_first<uintptr_t>(2);
    }
}

namespace THPS4Display
{
    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        WindowedModeWrapper::EnableDpiAwareness();
        TonyDisplay::Window(0, width, height);
        THPS4Window::Install(mode);
        if (mode != 0)
        {
            auto pattern = hook::pattern("89 2D ? ? ? ? 89 0D ? ? ? ? 89 1D ? ? ? ? A3 ? ? ? ? 89 1D ? ? ? ? 89 2D ? ? ? ?");
            auto interval = hook::pattern("C7 05 ? ? ? ? 00 00 00 80 C7 05 ? ? ? ? 16 00 00 00 C7 05 ? ? ? ? 4D 00 00 00 8B 10");
            if (pattern.size() == 1 && interval.size() == 1)
            {
                static int* NativeWindowed = *pattern.get_first<int*>(2);
                static auto windowedHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext&)
                {
                    *NativeWindowed = 1;
                });
                static int* PresentationInterval = *interval.get_first<int*>(2);
                static auto intervalHook = safetyhook::create_mid(interval.get_first(30), [](SafetyHookContext&)
                {
                    *PresentationInterval = 0;
                });
                TonyDisplay::Window(mode, width, height);
            }
            else
                OutputDebugStringA("THPS4: native window mode pattern unavailable.\n");
        }
        if (settings.Bool("MAIN.SkipIntro"))
            TonyDisplay::Intro("8B 4C 24 04 8D 44 24 04 6A 01 50 6A 00 E8 ? ? ? ? 84 C0 74 10 8B 4C 24 04 51 E8 ? ? ? ? 83 C4 04 B0 01 C3 32 C0 C3", 27, 32);
    }
}

void Init()
{
    WindowCursor::Initialize(FixSettings());
    InitFrameRateLimit();
    Screen.Width = FixSettings().Int("MAIN.ResX");
    Screen.Height = FixSettings().Int("MAIN.ResY");
    bool bFixHUD = FixSettings().Bool("MAIN.FixHUD");
    bool bRandomSongOrderFix = FixSettings().Bool("MAIN.RandomSongOrderFix");
    static const float& fFOVFactor = FixSettings().Float("MAIN.FOVFactor");

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    if (FixSettings().Int("MAIN.WindowMode") == 2)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
    THPS4Display::Init(Screen.Width, Screen.Height);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    THPS4Video::Install(&FixSettings().Int("MAIN.FMVWidescreenMode"));

    [&]()
    {
        hook::pattern pattern("");
        //Resolution
        pattern = hook::pattern("8B 4C B4 20 89 15");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksProSkater4.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        static int32_t* dword_5A0280 = *pattern.get_first<int32_t*>(6);
        static int32_t* dword_5A0284 = *pattern.get_first<int32_t*>(12);
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_5A0280 = Screen.Width;
                *dword_5A0284 = Screen.Height;
            }
        };
        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_5A0280)));
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_5A0284)));
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));

        pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_5A0284)));
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksProSkater4.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));
    }();

    [&]()
    {
        hook::pattern pattern("");
        //Aspect Ratio
        pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 33 FF 57 68"); //4616E2
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksProSkater4.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        ProtectedGameRef<float>(pattern.get_first<float>(1)) = Screen.fAspectRatio;
    }();

    [&]()
    {
        hook::pattern pattern("");
        THPS4Camera::Init();
        //FOV
        pattern = hook::pattern("D9 96 A4 00 00 00 5E 59 C3");
        if (pattern.size() < 1)
        {
            OutputDebugStringA("TonyHawksProSkater4.WidescreenFix: feature pattern unavailable; native behavior retained.\n");
            return;
        }
        struct FovHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float fov = 0.0f;
                _asm { fstp dword ptr[fov] }
                fov = THPS4Camera::Correct(regs.esi, fov, fFOVFactor);
                *reinterpret_cast<float*>(regs.esi + 0xA4) = fov;
                _asm { fld dword ptr[fov] }
            }
        };
        if (pattern.size() == 1)
            injector::MakeInline<FovHook>(pattern.get_first(0), pattern.get_first(6));
        else
            OutputDebugStringA("THPS4: camera FOV pattern unavailable.\n");
    }();

    [&]()
    {
        hook::pattern pattern("");
        //HUD
        if (bFixHUD && THPS4Hud::Install(Screen.fWidth, Screen.fHeight, &FixSettings().String("MAIN.HudAspectRatioConstraint")))
        {
            pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? 83 E2 10 89 2D");
            if (pattern.size() == 1)
                ProtectedGameRef<int32_t>(pattern.get_first<int32_t>(6)) = static_cast<int32_t>(Screen.fHudOffsetReal) + 32;
            else
                OutputDebugStringA("THPS4: centered canvas offset pattern unavailable.\n");
        }
    }();

    if (bRandomSongOrderFix)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 8B E8 8B 87 D0");
        if (pattern.empty())
        {
            OutputDebugStringA("THPS4: soundtrack pattern unavailable.\n");
            return;
        }
        struct RandomHook
        {
            void operator()(injector::reg_pack& regs)
            {
                std::mt19937 r{ std::random_device{}() };
                std::uniform_int_distribution<uint32_t> uid(0, regs.eax);
                regs.eax = uid(r);
            }
        }; injector::MakeInline<RandomHook>(pattern.get_first(0));
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            FixSettings();
            asi_settings::LiveLink::Start();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([]() { Init(); THPS4Menu::Init(); }, hook::pattern("8B 4C B4 20 89 15"));
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        WindowCursor::Shutdown();
    }
    return TRUE;
}