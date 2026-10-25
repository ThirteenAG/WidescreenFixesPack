#include "stdafx.h"
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <atomic>

ASI_SETTINGS_EXPORT(FixSettings())

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fHudOffset1;
    float fHudOffset2;
    float fFMVOffset1;
    float fFMVOffset2;
    float fFMVOffset3;
    float fFMVOffset4;
    int Width43;
    float fWidth43;
    float f3DHorScale;
} Screen;

static std::atomic<int> MovieMode{ 1 };

void Init()
{
    Screen.Width = FixSettings().Int("MAIN.ResX");
    Screen.Height = FixSettings().Int("MAIN.ResY");
    MovieMode.store(FixSettings().Int("MAIN.FMVWidescreenMode"), std::memory_order_relaxed);
    FixSettings().OnChange("MAIN.FMVWidescreenMode", []()
    {
        MovieMode.store(FixSettings().Int("MAIN.FMVWidescreenMode"), std::memory_order_relaxed);
    });

    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
    if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 32767 || Screen.Height > 32767)
        std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    [&]()
    {
        if (!CheckPatterns("Onimusha 3: resolution", {
            "75 0C 68 ? ? ? ? 68 ? ? ? ? EB 0A",
            "8B 04 C5 ? ? ? ? 89 0D ? ? ? ? A3",
        }))
            return;

        auto pattern = hook::pattern("75 0C 68 ? ? ? ? 68 ? ? ? ? EB 0A"); //406584
        injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 2, true);
        ProtectedGameRef<int32_t>{pattern.count(1).get(0).get<int32_t>(3)} = Screen.Height;
        ProtectedGameRef<int32_t>{pattern.count(1).get(0).get<int32_t>(8)} = Screen.Width;

        pattern = hook::pattern("8B 04 C5 ? ? ? ? 89 0D ? ? ? ? A3"); //4957DC
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = Screen.Width;
                regs.eax = Screen.Height;
            }
        }; injector::MakeInline<SetResHook>(pattern.get_first(-2), pattern.get_first(7));
        ProtectedGameRef<uint16_t>{pattern.get_first<uint16_t>(3)} = 0xD13B;

        ProtectedGameRef<uint32_t>{*pattern.count(1).get(0).get<uint32_t*>(9)} = Screen.Width;
        ProtectedGameRef<uint32_t>{*pattern.count(1).get(0).get<uint32_t*>(14)} = Screen.Height;
    }();

    //clipping fix
    static float fNegResX = -Screen.fWidth;
    static float fNegResY = -Screen.fHeight;
    [&]()
    {
        if (!CheckPatterns("Onimusha 3: clipping", {
            "F3 0F 10 05 ? ? ? ? 8D 54 24 10 8D 4C 24 70",
            "F3 0F 10 05 ? ? ? ? 8D 54 24 10 8D 8C 24 90",
        }))
            return;

        auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8D 54 24 10 8D 4C 24 70"); //479982
        ProtectedGameRef<float*>{pattern.get_first<float*>(4)} = &fNegResX;
        pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8D 54 24 10 8D 8C 24 90"); //479A0C
        ProtectedGameRef<float*>{pattern.get_first<float*>(4)} = &fNegResY;
    }();

    static float f2 = 2.0f;
    Screen.f3DHorScale = 2.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    [&]()
    {
        if (!CheckPatterns("Onimusha 3: projection", {
            "F3 0F 10 0D ? ? ? ? 0F 28 54 24 30",
        }))
            return;

        auto pattern = hook::pattern("F3 0F 10 0D ? ? ? ? 0F 28 54 24 30"); //40B504
        ProtectedGameRef<float*>{pattern.count(1).get(0).get<float*>(4)} = &Screen.f3DHorScale;
    }();

    // The draw state of the 2D quad being set up (the renderer object's fields from +8): the quads that keep the
    // stretched full-width scale are told apart by it
    [&]()
    {
        if (!CheckPatterns("Onimusha 3: 2D scaling", {
            "8B F1 E8 ? ? ? ? C6 46 10 01 B9 ? ? ? ? C6 05",
            "F3 0F 10 2D ? ? ? ? F3 0F 11 64 24 64 F3 0F 2A E1",
        }))
            return;

        auto pattern = hook::pattern("8B F1 E8 ? ? ? ? C6 46 10 01 B9 ? ? ? ? C6 05");
        static auto pDrawState = *pattern.get_first<uint8_t*>(12) + 8;

        pattern = hook::pattern("F3 0F 10 2D ? ? ? ? F3 0F 11 64 24 64 F3 0F 2A E1"); //41104C
        struct Set2DScale
        {
            static uint32_t State(uint32_t offset)
            {
                return *reinterpret_cast<uint32_t*>(pDrawState + offset);
            }

            void operator()(injector::reg_pack& regs)
            {
                if ((State(0x0) == 100 && (State(0x8) == 70) && (State(0x38) == 524288 || State(0x38) == 524296) && (State(0xB8) == 1)) //generic
                    || (State(0x0) == 0 && State(0x4) == 0 && State(0x8) == 6 && (State(0x38) == 524288 || State(0x38) == 524296) && (State(0xB8) == 1 || State(0xB8) == 65536)) //cutscene borders
                    || (State(0x0) == 100 && State(0x8) == 70 && (State(0x38) == 524288 || State(0x38) == 524296) && (State(0xB8) == 65536)) //cutscene fading
                    || (State(0x0) == 68 && State(0x4) == 0 && State(0x8) == 6) //player shadow
                    )
                    regs.xmm5.f32[0] = f2;
                else
                    regs.xmm5.f32[0] = Screen.f3DHorScale;
            }
        }; injector::MakeInline<Set2DScale>(pattern.get_first(0), pattern.get_first(8));
    }();

    Screen.fHudOffset1 = (Screen.fWidth - Screen.fWidth43) / 2.0f;
    Screen.fHudOffset2 = Screen.fHudOffset1 + Screen.fWidth43;
    [&]()
    {
        if (!CheckPatterns("Onimusha 3: backgrounds", {
            "8B 08 8B 91 0C 01 00 00 6A 00 50",
        }))
            return;

        auto pattern = hook::pattern("8B 08 8B 91 0C 01 00 00 6A 00 50"); //412FC7
        struct BackgroundsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *reinterpret_cast<float*>(regs.esp + 0x0C) = Screen.fHudOffset1;
                *reinterpret_cast<float*>(regs.esp + 0x60) = Screen.fHudOffset1;
                *reinterpret_cast<float*>(regs.esp + 0x28) = Screen.fHudOffset2;
                *reinterpret_cast<float*>(regs.esp + 0x44) = Screen.fHudOffset2;

                regs.ecx = *(uintptr_t*)regs.eax;
                regs.edx = *(uintptr_t*)(regs.ecx + 0x10C);
            }
        }; injector::MakeInline<BackgroundsHook>(pattern.get_first(0), pattern.get_first(8));
    }();

    // Each movie quad uses one mode snapshot, preserving its 4:3 aspect unless stretched.
    [&]()
    {
        if (!CheckPatterns("Onimusha 3: movies", {
            "F3 0F 11 64 24 70 F3 0F 11 44 24 74 8B 08 6A 00",
        }))
            return;

        auto pattern = hook::pattern("F3 0F 11 64 24 70 F3 0F 11 44 24 74 8B 08 6A 00"); //412E6F
        struct MoviesHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto mode = MovieMode.load(std::memory_order_relaxed);
                float width = Screen.fWidth;
                float height = Screen.fHeight;
                if (mode != 2)
                {
                    width = mode == 1 ? std::max(width, height * (4.0f / 3.0f)) : std::min(width, height * (4.0f / 3.0f));
                    height = width * (3.0f / 4.0f);
                }
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
    }();

    //disable "Effect" option overlay
    [&]()
    {
        if (!CheckPatterns("Onimusha 3: effect overlay", {
            "0F 84 FC 00 00 00 8D 44 24 60 33 ED",
        }))
            return;

        auto pattern = hook::pattern("0F 84 FC 00 00 00 8D 44 24 60 33 ED"); //5686E4
        ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(-1)} = 10;
        ProtectedGameRef<uint8_t>{pattern.get_first<uint8_t>(1)} = 0x82; //jb
    }();


}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            FixSettings();
            asi_settings::LiveLink::Start();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("81 EC 04 01 00 00 53 55 56"));
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