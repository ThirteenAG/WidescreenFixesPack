module;
#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Misc;
import ComVars;
import Window;
import Borders;
import Camera;
import Hud;
import Intro;
import Movies;
import Focus;
import Timing;
import Storage;
import DeveloperOverlay;
import NativeMenuText;
import NativeMenuView;
import Resolution;
import NativeMenu;
import Pad;
import Updates;

static std::atomic<int> MovieMode{ 1 };

bool FitFrontend()
{
    return Screen.fAspectRatio < (4.0f / 3.0f) &&
        (GameState != 2 || GameState.get_ptr()[39612] != 0 || OnimushaBorders::Active());
}

void UpdateScreen(int width, int height)
{
    Screen.Width = width;
    Screen.Height = height;
    Screen.fWidth = float(width);
    Screen.fHeight = float(height);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.fWidth43 = std::min(Screen.fWidth, Screen.fHeight * (4.0f / 3.0f));
    Screen.Width43 = int(Screen.fWidth43);
    Screen.f3DHorScale = 2.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHudOffset1 = (Screen.fWidth - Screen.fWidth43) * 0.5f;
    Screen.fHudOffset2 = Screen.fHudOffset1 + Screen.fWidth43;
    Screen.fNegResX = -Screen.fWidth;
    Screen.fNegResY = -Screen.fHeight;
    if (OnimushaWindow::NativeWidth.is_initialized())
    {
        OnimushaWindow::NativeWidth = width;
        OnimushaWindow::NativeHeight = height;
    }
}

void InitializeGame()
{

    Screen.Width = FixSettings().Int("MAIN.ResX");
    Screen.Height = FixSettings().Int("MAIN.ResY");
    MovieMode.store(std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1), std::memory_order_relaxed);
    FixSettings().OnChange("MAIN.FMVWidescreenMode", []()
    {
        MovieMode.store(std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1), std::memory_order_relaxed);
    });

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 32767 || Screen.Height > 32767)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    UpdateScreen(Screen.Width, Screen.Height);
    GameState.SetAddress(*hook::pattern("33 C0 A2 ? ? ? ? 66 A3 ? ? ? ? 66 A3 ? ? ? ? EB 0B").get_first<uint8_t*>(3));

    OnimushaWindow::Init(Screen.Width, Screen.Height, UpdateScreen);

    OnimushaIntro::Init();

    OnimushaStorage::Init();

    OnimushaBorders::Init([]() { return Screen.fAspectRatio; });

    OnimushaCamera::Init([]() { return Screen.fAspectRatio; });

    OnimushaHud::Init([]() { return Screen.fAspectRatio; });

    OnimushaMovies::Init();

    OnimushaFocus::Init();

    OnimushaTiming::Init();

    OnimushaOverlay::Init();

    OnimushaMenu::Init();

    OnimushaPad::Init();
    OnimushaUpdates::Init();


    {
        auto pattern = hook::pattern("75 0C 68 ? ? ? ? 68 ? ? ? ? EB 0A 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? B9");
        injector::MakeNOP(pattern.get_first<uint32_t>(0), 2, true);
        injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(3), Screen.Height, true);
        injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(8), Screen.Width, true);

        pattern = hook::pattern("8B 04 C5 ? ? ? ? 89 0D ? ? ? ? A3"); //4957DC
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = Screen.Width;
                regs.eax = Screen.Height;
            }
        }; injector::MakeInline<SetResHook>(pattern.get_first(-2), pattern.get_first(7));
        injector::WriteMemory<uint16_t>(pattern.get_first<uint16_t>(3), 0xD13B, true);

        injector::WriteMemory<uint32_t>(*pattern.get_first<uint32_t*>(9), Screen.Width, true);
        injector::WriteMemory<uint32_t>(*pattern.get_first<uint32_t*>(14), Screen.Height, true);
    }

    //clipping fix
    {
        auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8D 54 24 10 8D 4C 24 70"); //479982
        injector::WriteMemory<float*>(pattern.get_first<float*>(4), &Screen.fNegResX, true);
        pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8D 54 24 10 8D 8C 24 90"); //479A0C
        injector::WriteMemory<float*>(pattern.get_first<float*>(4), &Screen.fNegResY, true);
    }

    static float f2 = 2.0f;
    Screen.f3DHorScale = 2.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    {
        auto pattern = hook::pattern("F3 0F 10 0D ? ? ? ? 0F 28 54 24 30"); //40B504
        injector::WriteMemory<float*>(pattern.get_first<float*>(4), &Screen.f3DHorScale, true);
    }

    // The draw state of the 2D quad being set up (the renderer object's fields from +8): the quads that keep the
    // stretched full-width scale are told apart by it
    {
        auto pattern = hook::pattern("8B F1 E8 ? ? ? ? C6 46 10 01 B9 ? ? ? ? C6 05");
        static auto pDrawState = *pattern.get_first<uint8_t*>(12) + 8;
        static const auto fullScreen = []()
        {
            const auto state = [](uint32_t offset) { return *reinterpret_cast<uint32_t*>(pDrawState + offset); };
            return (state(0) == 100 && state(8) == 70 && (state(0x38) == 524288 || state(0x38) == 524296) && (state(0xB8) == 1 || state(0xB8) == 65536)) ||
                (state(0) == 0 && state(4) == 0 && state(8) == 6 && (state(0x38) == 524288 || state(0x38) == 524296) && (state(0xB8) == 1 || state(0xB8) == 65536)) ||
                (state(0) == 68 && state(4) == 0 && state(8) == 6);
        };
        pattern = hook::pattern("F3 0F 10 2D ? ? ? ? F3 0F 11 64 24 64 F3 0F 2A E1"); //41104C
        struct Set2DScale
        {
            void operator()(injector::reg_pack& regs)
            {
                if (fullScreen() || FitFrontend())
                    regs.xmm5.f32[0] = f2;
                else
                    regs.xmm5.f32[0] = Screen.f3DHorScale;
            }
        }; injector::MakeInline<Set2DScale>(pattern.get_first(0), pattern.get_first(8));
        pattern = hook::pattern("F3 0F 10 2D ? ? ? ? F3 0F 2A E6 F3 0F 5E EC");
        struct Set2DVertical
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.xmm5.f32[0] = FitFrontend() && !fullScreen() ? -2.0f * Screen.fAspectRatio / (4.0f / 3.0f) : -2.0f;
            }
        }; injector::MakeInline<Set2DVertical>(pattern.get_first(0), pattern.get_first(8));
    }

    {
        auto pattern = hook::pattern("8B 08 8B 91 0C 01 00 00 6A 00 50"); //412FC7
        struct BackgroundsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *reinterpret_cast<float*>(regs.esp + 0x0C) = Screen.fHudOffset1;
                *reinterpret_cast<float*>(regs.esp + 0x60) = Screen.fHudOffset1;
                *reinterpret_cast<float*>(regs.esp + 0x28) = Screen.fHudOffset2;
                *reinterpret_cast<float*>(regs.esp + 0x44) = Screen.fHudOffset2;
                const float height = Screen.fWidth43 * (3.0f / 4.0f);
                const float top = (Screen.fHeight - height) * 0.5f;
                *reinterpret_cast<float*>(regs.esp + 0x10) = top;
                *reinterpret_cast<float*>(regs.esp + 0x2C) = top;
                *reinterpret_cast<float*>(regs.esp + 0x48) = top + height;
                *reinterpret_cast<float*>(regs.esp + 0x64) = top + height;

                regs.ecx = *(uintptr_t*)regs.eax;
                regs.edx = *(uintptr_t*)(regs.ecx + 0x10C);
            }
        }; injector::MakeInline<BackgroundsHook>(pattern.get_first(0), pattern.get_first(8));
    }

    // Keep the complete file in Original. Fill fits its picture, enlarging only through baked black rows.
    {
        auto pattern = hook::pattern("F3 0F 11 64 24 70 F3 0F 11 44 24 74 8B 08 6A 00"); //412E6F
        struct MoviesHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto mode = MovieMode.load(std::memory_order_relaxed);
                const float content = OnimushaMovies::ContentAspect.load(std::memory_order_relaxed);
                const float width = std::min(Screen.fWidth, Screen.fHeight * (mode != 0 ? content : 4.0f / 3.0f));
                const float height = width * (3.0f / 4.0f);
                const auto left = (Screen.fWidth - width) * 0.5f;
                const auto right = left + width;
                const auto top = (Screen.fHeight - height) * 0.5f;
                const auto bottom = top + height;
                *reinterpret_cast<float*>(regs.esp + 0x08) = left;
                *reinterpret_cast<float*>(regs.esp + 0x5C) = left;
                *reinterpret_cast<float*>(regs.esp + 0x24) = right;
                *reinterpret_cast<float*>(regs.esp + 0x40) = right;

                *reinterpret_cast<float*>(regs.esp + 0x0C) = top;
                *reinterpret_cast<float*>(regs.esp + 0x28) = top;
                *reinterpret_cast<float*>(regs.esp + 0x60) = bottom;
                *reinterpret_cast<float*>(regs.esp + 0x44) = bottom;

                *reinterpret_cast<float*>(regs.esp + 0x70) = 0.0f;
                *reinterpret_cast<float*>(regs.esp + 0x74) = 1.0f;
            }
        }; injector::MakeInline<MoviesHook>(pattern.get_first(0), pattern.get_first(12));
    }

    //disable "Effect" option overlay
    {
        auto pattern = hook::pattern("0F 84 FC 00 00 00 8D 44 24 60 33 ED"); //5686E4
        injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(-1), 10, true);
        injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(1), 0x82, true); //jb
    }


}


class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            InitializeGame();
        };
    }
} Misc;
