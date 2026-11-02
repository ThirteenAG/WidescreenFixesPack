module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
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

void UpdateHudLayout()
{
    if (Screen.nWidth <= 0 || Screen.nHeight <= 0)
        return;
    Screen.fHudOffsetWide = -CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f);
    auto ratio = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
    if (ratio && std::isfinite(*ratio) && *ratio > 0.0f)
    {
        const auto value = ClampHudAspectRatio(std::clamp(*ratio, 4.0f / 3.0f, 32.0f / 9.0f), Screen.fAspectRatio);
        Screen.fHudOffsetWide = -CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f);
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

static auto idx = 0; // seems like 4 in debug build and 3 in release
static void* stack[6];
SafetyHookInline shsub_47C7C0 = {};
void __fastcall sub_47C7C0(
        void* _this,
        void* edx,
        int a2,
        float a3,
        float a4,
        int a5,
        void* a6,
        int a7,
        float* a8,
        float* a9,
        float a10,
        float* a11,
        int a12,
        int a13,
        float* a14)
{
    std::fill(std::begin(stack), std::end(stack), nullptr);
    CaptureStackBackTrace(0, static_cast<DWORD>(std::size(stack)), stack, nullptr);

    return shsub_47C7C0.unsafe_fastcall(_this, edx, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
}

void InitializeFix()
{
    WindowedModeWrapper::EnableDpiAwareness();
    PsychonautsDisplay::Init();
    PsychonautsRenderer::Init();
    hook::pattern pattern;
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    InstallLogoSkip();
    InstallPressStartSkip();
    PsychonautsUpdates::Init(PsychonautsMenu::Init());
    PsychonautsRenderer::Frame = []()
    {
        static std::once_flag once;
        std::call_once(once, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
        RWGTA::UpdateCheck::Tick();
        PsychonautsMenu::Frame();
    };
    InstallGameplayFov();
    static bool bWidescreenHud = true;
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
            Screen.nWidth43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
            Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
            Screen.fHudScale2 = 640.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
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
            *reinterpret_cast<float*>(regs.ebp - 0x20) = Screen.fHudOffset; //x
            *reinterpret_cast<float*>(regs.ebp - 0x1C) = 0.0f;              //y
            *reinterpret_cast<float*>(regs.ebp - 0x14) = Screen.fHudScale2; //w
            *reinterpret_cast<float*>(regs.ebp - 0x10) = 480.0f;            //h
        }
    }; injector::MakeInline<HudHook>(pattern.get_first(0), pattern.get_first(36)); //0x47D5B8, 0x47D5DC

    // xrefs
    static auto dw_61F2EF = hook::get_pattern("EB ? 8B 85 14 FF FF FF 8B 08 E8", 0);// 0x0061F2EF;
    static auto dw_61EA8F = hook::get_pattern("8B 95 14 FF FF FF 8B 4A 1C E8 ? ? ? ? 8B", 0); // 0x0061EA8F
    static auto dw_616FB7 = hook::get_pattern("8B 45 C4 83 C0 01 89 45 C4 8B 4D C8", 0); // 0x00616FB7
    static auto dw_4FD37D = hook::get_pattern("8B 8D 28 FA FF FF 83 79 18 00 74", 0); // 0x004FD37D
    static auto dw_61EAC4 = hook::get_pattern("8B 8D 14 FF FF FF 8B 51 10 8B 85 14", 0); // 0x0061EAC4
    static auto dw_61EADD = hook::get_pattern("8B 8D 14 FF FF FF 8B 51 0C 8B 85 14", 0); // 0x0061EADD
    static auto dw_61EAF6 = hook::get_pattern("C7 45 D8 00 00 00 00 8B 0D ? ? ? ? 8B 91 8C", 0); // 0x0061EAF6
    static auto dw_61F2FE = hook::get_pattern("8B 85 14 FF FF FF 8B 08 E8 ? ? ? ? 8B E5 5D C3", 13); // 0x0061F2FE
    static auto dw_673413 = hook::get_pattern("D9 41 08 D9 1C 24 8B 4D FC E8 ? ? ? ? 8B E5 5D C3", 14); // 0x00673413
    static auto dw_61F19D = hook::get_pattern("8B 95 14 FF FF FF 8B 4A 28 E8", 0); // 0x0061F19D

    pattern = hook::pattern("BE ? ? ? ? 8D BC 24 00 01 00 00 F3 A5 83 7D 20 00");
    injector::WriteMemory<MemFloat*>(pattern.get_first<MemFloat*>(1), &flt_7933E0, true); //0x47CC71 + 1

    pattern = hook::pattern("E8 ? ? ? ? 8B 85 ? ? ? ? 83 C0 ? EB");
    shsub_47C7C0 = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first(0)).as_int(), sub_47C7C0);

    pattern = hook::pattern("89 8C 24 AC 01 00 00 83 7D 08 00");
    struct HudHook2
    {
        const CRect fs = CRect(0.0f, 0.0f, 640.0f, 480.0f); // fullscreen images
        const CRect fl = CRect(0.0f, 0.0f, 20.0f, 15.0f);   // strike flash
        const CRect flbv = CRect(0.0f, 0.0f, 2.5f, 1.875f);   // strike flash (black velvetopia variant)

        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<uint32_t*>(regs.esp + 0x1AC) = regs.ecx;

            if (HudLayoutChanged.exchange(false, std::memory_order_acquire))
                UpdateHudLayout();

            flt_7933E0.a01 = 1.0f;
            flt_7933E0.a13 = 0.0f;

            auto ptr = *(uint32_t*)(regs.ebp + 0x24);
            if (ptr)
            {
                CRect* r = (CRect*)(ptr - 0x08);

                DBGONLY(KEYPRESS(VK_F1) { spd::log()->info("{0:f} {1:f} {2:f} {3:f} {4:08x} {5:08x} {6:08x} {7:08x}", r->m_fLeft, r->m_fBottom, r->m_fRight, r->m_fTop, *(uint32_t*)&r->m_fLeft, *(uint32_t*)&r->m_fBottom, *(uint32_t*)&r->m_fRight, *(uint32_t*)&r->m_fTop); });

                if (*r == fs || *r == fl || *r == flbv)
                {
                    flt_7933E0.a01 = Screen.fHudScale;
                    flt_7933E0.a13 = -Screen.fHudOffset;
                }
                else
                {
                    if (bWidescreenHud)
                    {
                        auto it = std::find_if(std::begin(stack), std::end(stack), [](void* n) { return n == dw_61F2EF || n == dw_61F2FE; });
                        if (!idx && it != std::end(stack))
                            idx = std::distance(std::begin(stack), it);

                        if (idx)
                        {
                            const auto frame = [](size_t index) { return index < std::size(stack) ? stack[index] : nullptr; };
                            if (frame(static_cast<size_t>(idx)) == dw_61F2EF || frame(static_cast<size_t>(idx) + 1) == dw_61F2EF || frame(static_cast<size_t>(idx)) == dw_61F2FE || frame(static_cast<size_t>(idx) + 1) == dw_61F2FE)
                            {
                                flt_7933E0.a13 += Screen.fHudOffsetWide;
                            }
                            else
                            {
                                //DBGONLY(KEYPRESS(VK_F2) { spd::log()->info("0x{0:08x} 0x{1:08x}", stack[3], stack[4]); });

                                if (frame(static_cast<size_t>(idx)) == dw_61EA8F || frame(static_cast<size_t>(idx)) == dw_616FB7 || frame(static_cast<size_t>(idx)) == dw_4FD37D)
                                {
                                    flt_7933E0.a13 -= Screen.fHudOffsetWide;
                                }
                                else
                                {
                                    if (frame(static_cast<size_t>(idx)) == dw_61EAC4 || frame(static_cast<size_t>(idx)) == dw_61EAF6 || frame(static_cast<size_t>(idx)) == dw_61EADD || frame(static_cast<size_t>(idx)) == dw_673413 || frame(static_cast<size_t>(idx)) == dw_61F19D)
                                    {
                                        flt_7933E0.a13 += Screen.fHudOffsetWide;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }; injector::MakeInline<HudHook2>(pattern.get_first(0), pattern.get_first(7)); //0x47C7CE, 0x47C7CE+7

    InstallMovies();

    // Only the gameplay focus decision is optional; input still requires real focus.
    pattern = hook::pattern("E8 ? ? ? ? 88 45 FB 0F B6 45 FB 85 C0 74 34 83 3D ? ? ? ? 00 74 2B");
    static auto focusHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& context)
    {
        static const auto& pause = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        if (!pause)
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
