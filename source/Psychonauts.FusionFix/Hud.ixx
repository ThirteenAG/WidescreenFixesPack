module;

#include "stdafx.h"
#include "rawinput.hpp"
#include "FixSettings.hpp"
#include <bit>
#include <atomic>

export module Hud;

import ComVars;
import Camera;
import Movies;
import Intro;
import Display;
import Renderer;
import NativeMenu;
import Updates;

std::atomic_bool HudLayoutChanged{ true };
std::atomic_bool PauseOnFocusLoss{ true };

void UpdateHudLayout()
{
    if (Screen.nWidth <= 0 || Screen.nHeight <= 0)
        return;
    Screen.fHudOffsetWide = Screen.fHudOffsetX;
    auto ratio = ParseWidescreenHudOffset(FixSettings().Get("MAIN.HudAspectRatioConstraint"));
    if (ratio && std::isfinite(*ratio) && *ratio > 0.0f)
    {
        const auto value = ClampHudAspectRatio(std::clamp(*ratio, 4.0f / 3.0f, 32.0f / 9.0f), Screen.fAspectRatio);
        Screen.fHudOffsetWide = std::max(0.0f, -CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f));
    }
}

class CRect
{
public:
    float m_fLeft;
    float m_fBottom;
    float m_fRight;
    float m_fTop;

    bool operator==(const CRect& rect) const
    {
        return std::bit_cast<uint32_t>(m_fLeft) == std::bit_cast<uint32_t>(rect.m_fLeft) &&
            std::bit_cast<uint32_t>(m_fBottom) == std::bit_cast<uint32_t>(rect.m_fBottom) &&
            std::bit_cast<uint32_t>(m_fRight) == std::bit_cast<uint32_t>(rect.m_fRight) &&
            std::bit_cast<uint32_t>(m_fTop) == std::bit_cast<uint32_t>(rect.m_fTop);
    }

    inline CRect() {}
    inline CRect(float a, float b, float c, float d)
        : m_fLeft(a), m_fBottom(b), m_fRight(c), m_fTop(d)
    {
    }
    inline CRect(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
        : m_fLeft(std::bit_cast<float>(a)), m_fBottom(std::bit_cast<float>(b)), m_fRight(std::bit_cast<float>(c)), m_fTop(std::bit_cast<float>(d))
    {
    }
};

struct MemFloat
{
    float a01 = 1.0f;
    float a02 = 0.0f;
    float a03 = 0.0f;
    float a04 = 0.0f;
    float a05 = 0.0f;
    float a06 = 1.0f;
    float a07 = 0.0f;
    float a08 = 0.0f;
    float a09 = 0.0f;
    float a10 = 0.0f;
    float a11 = 1.0f;
    float a12 = 0.0f;
    float a13 = 0.0f;
    float a14 = 0.0f;
    float a15 = 0.0f;
    float a16 = 1.0f;
} flt_7933E0;

enum class HudSide { Left = -1, Center, Right };
thread_local HudSide hudSide{ HudSide::Center };

struct HudSideScope
{
    explicit HudSideScope(HudSide side) : previous(std::exchange(hudSide, side))
    {
    }

    ~HudSideScope()
    {
        hudSide = previous;
    }

private:
    HudSide previous;
};

template<HudSide Side, class Result, class... Args>
struct HudDrawCall
{
    using NativeDraw = Result(__thiscall*)(void*, Args...);
    static inline NativeDraw nativeDraw{};

    static Result __fastcall Draw(void* object, void*, Args... args)
    {
        const HudSideScope scope(Side);
        return nativeDraw(object, args...);
    }

    static void Install(uint8_t* call)
    {
        nativeDraw = reinterpret_cast<NativeDraw>(injector::GetBranchDestination(call).as_int());
        injector::MakeCALL(call, Draw, true);
    }
};

template<HudSide Side>
using HudSpriteCall = HudDrawCall<Side, void, int, float, float, int, void*, int,
    float*, float*, float, float*, int, int, float*>;

void __fastcall DrawRightWidget(void* widget, void*)
{
    const HudSideScope scope(HudSide::Right);
    const auto draw = reinterpret_cast<void(__thiscall*)(void*)>((*static_cast<void***>(widget))[1]);
    draw(widget);
}

void InstallHudAnchors()
{
    // The lives panel and its digits, plus the two lives sprites drawn by the main HUD.
    HudDrawCall<HudSide::Left, void>::Install(hook::pattern(
        "8B 8D 14 FF FF FF 8B 49 04 E8 ? ? ? ? 8B 95 14 FF FF FF 8B 4A 1C E8").get_first<uint8_t>(9));
    HudDrawCall<HudSide::Left, void, float, float, float, int, float, int, float, float>::Install(hook::pattern(
        "D9 1C 24 8B 8D 4C FF FF FF E8 ? ? ? ? 8B 45 C4 83 C0 01 89 45 C4 8B 4D C8").get_first<uint8_t>(9));
    HudSpriteCall<HudSide::Left>::Install(hook::pattern(
        "8B 82 A8 00 00 00 50 8B 0D ? ? ? ? 8B 89 A4 01 00 00 E8 ? ? ? ? D9 05 ? ? ? ? D9 5D AC").get_first<uint8_t>(19));
    HudSpriteCall<HudSide::Left>::Install(hook::pattern(
        "8B 44 95 B4 50 8B 0D ? ? ? ? 8B 89 A4 01 00 00 E8 ? ? ? ? 8B 95 14 FF FF FF D9 82 D8 02 00 00").get_first<uint8_t>(17));

    // The inventory and the other widgets drawn against its edge.
    auto inventory = hook::pattern(
        "8B 95 14 FF FF FF 8B 0A E8 ? ? ? ? EB ? 8B 85 14 FF FF FF 8B 08 E8 ? ? ? ? 8B E5 5D C3");
    HudDrawCall<HudSide::Right, int>::Install(inventory.get_first<uint8_t>(8));
    HudDrawCall<HudSide::Right, int>::Install(inventory.get_first<uint8_t>(23));
    HudDrawCall<HudSide::Right, void, float>::Install(hook::pattern(
        "D9 41 08 D9 1C 24 8B 4D FC E8 ? ? ? ? 8B E5 5D C3").get_first<uint8_t>(9));
    // Only the icon in the goal notification belongs at the edge.
    HudSpriteCall<HudSide::Right>::Install(hook::pattern(
        "8B 91 D4 89 00 00 52 A1 ? ? ? ? 8B 88 A4 01 00 00 E8 ? ? ? ? 8B 4D A8 0F B6 51 20").get_first<uint8_t>(18));

    // Each virtual draw loads slot one and calls it in five bytes.
    for (const auto* pattern : {
        "8B 48 08 8B 12 8B 42 04 FF D0 8B 8D 14 FF FF FF 8B 51 10",
        "8B 48 10 8B 12 8B 42 04 FF D0 8B 8D 14 FF FF FF 8B 51 0C",
        "8B 48 0C 8B 12 8B 42 04 FF D0 C7 45 D8 00 00 00 00" })
        injector::MakeCALL(hook::pattern(pattern).get_first<uint8_t>(5), DrawRightWidget, true);
}

void InitializeFix()
{
    WindowedModeWrapper::EnableDpiAwareness();
    MessagePumpGuard::Install(GetModuleHandleW(nullptr));
    PsychonautsDisplay::Init();
    PsychonautsRenderer::Init();
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    InstallLogoSkip();
    InstallPressStartSkip();
    PsychonautsMenu::Init();
    PsychonautsUpdates::Init();
    PsychonautsRenderer::Frame = []()
    {
        static std::once_flag once;
        std::call_once(once, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
        PsychonautsUpdates::Tick();
        PsychonautsMenu::Frame();
    };
    InstallGameplayFov();
    settings.OnChange("MAIN.HudAspectRatioConstraint", []()
    {
        HudLayoutChanged.store(true, std::memory_order_release);
    });

    pattern = hook::pattern("8B 82 C8 02 00 00 89 81 94 05 00 00 8B 4D F8");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *reinterpret_cast<uint32_t*>(regs.edx + 0x2C8);
            *reinterpret_cast<int32_t*>(regs.ecx + 0x594) = static_cast<int32_t>(regs.eax);
            Screen.nWidth = *(int32_t*)(regs.ecx + 0x590);
            Screen.nHeight = *(int32_t*)(regs.ecx + 0x594);

            if (Screen.nWidth <= 0 || Screen.nHeight <= 0)
            {
                Screen.nWidth = 640;
                Screen.nHeight = 480;
            }

            Screen.fWidth = static_cast<float>(Screen.nWidth);
            Screen.fHeight = static_cast<float>(Screen.nHeight);
            Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
            Screen.fHudCanvasWidth = std::max(640.0f, 480.0f * Screen.fAspectRatio);
            Screen.fHudCanvasHeight = std::max(480.0f, 640.0f / Screen.fAspectRatio);
            Screen.fHudOffsetX = (Screen.fHudCanvasWidth - 640.0f) * 0.5f;
            Screen.fHudOffsetY = (Screen.fHudCanvasHeight - 480.0f) * 0.5f;
            UpdateHudLayout();
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(12)); //0x67F958

    //Default resolution
    pattern = hook::pattern("A1 ? ? ? ? 50 8B 4D 84 E8 ? ? ? ? 6A 00 68");
    int32_t defX = 0, defY = 0;
    std::tie(defX, defY) = GetDesktopRes();
    if (defX <= 0 || defY <= 0)
    {
        defX = 640;
        defY = 480;
    }
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(-4), defX, true); //497822
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(17), defY, true);

    //Hud
    pattern = hook::pattern("8B 0D ? ? ? ? 89 4D E0 8B 15 ? ? ? ? 89 55 E4 D9 05");
    struct HudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<float*>(regs.ebp - 0x20) = Screen.fHudOffsetX;
            *reinterpret_cast<float*>(regs.ebp - 0x1C) = Screen.fHudOffsetY;
            *reinterpret_cast<float*>(regs.ebp - 0x14) = Screen.fHudCanvasWidth;
            *reinterpret_cast<float*>(regs.ebp - 0x10) = Screen.fHudCanvasHeight;
        }
    }; injector::MakeInline<HudHook>(pattern.get_first(0), pattern.get_first(36)); //0x47D5B8, 0x47D5DC

    pattern = hook::pattern("BE ? ? ? ? 8D BC 24 00 01 00 00 F3 A5 83 7D 20 00");
    injector::WriteMemory<MemFloat*>(pattern.get_first<MemFloat*>(1), &flt_7933E0, true); //0x47CC71 + 1
    InstallHudAnchors();

    pattern = hook::pattern("89 8C 24 AC 01 00 00 83 7D 08 00");
    struct HudHook2
    {
        const CRect fs = CRect(0.0f, 0.0f, 640.0f, 480.0f); // fullscreen images
        const CRect fl = CRect(0.0f, 0.0f, 20.0f, 15.0f);   // strike flash
        const CRect flbv = CRect(0.0f, 0.0f, 2.5f, 1.875f);   // strike flash (black velvetopia variant)

        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<uint32_t*>(regs.esp + 0x1AC) = regs.ecx;

            if (HudLayoutChanged.load(std::memory_order_relaxed) &&
                HudLayoutChanged.exchange(false, std::memory_order_acquire))
                UpdateHudLayout();

            flt_7933E0.a01 = 1.0f;
            flt_7933E0.a06 = 1.0f;
            flt_7933E0.a13 = 0.0f;
            flt_7933E0.a14 = 0.0f;

            const auto center = *reinterpret_cast<const uint8_t**>(regs.ebp + 0x24);
            if (!center)
                return;

            const auto& rect = *reinterpret_cast<const CRect*>(center - 8);
            if (rect == fs || rect == fl || rect == flbv)
            {
                flt_7933E0.a01 = Screen.fHudCanvasWidth / 640.0f;
                flt_7933E0.a06 = Screen.fHudCanvasHeight / 480.0f;
                flt_7933E0.a13 = -Screen.fHudOffsetX;
                flt_7933E0.a14 = -Screen.fHudOffsetY;
            }
            else
                flt_7933E0.a13 = static_cast<int>(hudSide) * Screen.fHudOffsetWide;
        }
    }; injector::MakeInline<HudHook2>(pattern.get_first(0), pattern.get_first(7)); //0x47C7CE, 0x47C7CE+7

    InstallMovies();

    // Only the gameplay focus decision is optional; input still requires real focus.
    const auto updatePauseOnFocusLoss = []()
    {
        PauseOnFocusLoss.store(FixSettings().Get("MAIN.PauseOnFocusLoss") == "1", std::memory_order_relaxed);
    };
    updatePauseOnFocusLoss();
    settings.OnChange("MAIN.PauseOnFocusLoss", updatePauseOnFocusLoss);
    pattern = hook::pattern("E8 ? ? ? ? 88 45 FB 0F B6 45 FB 85 C0 74 34 83 3D ? ? ? ? 00 74 2B");
    static auto focusHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& context)
    {
        if (!PauseOnFocusLoss.load(std::memory_order_relaxed))
            context.eax = (context.eax & ~0xFFu) | 1u;
    });
}

class Hud
{
public:
    Hud()
    {
        WFP::onInitEvent() += InitializeFix;
    }
} Hud;
