module;
#include <stdafx.h>
#include <dinput.h>
#include <ddraw.h>
#include <d3d.h>
#include "FixSettings.hpp"

export module Misc;
import ComVars;
import Window;
import Camera;
import Hud;
import Intro;
import Frame;
import OverlayD3D7;
import DeveloperOverlay;
import PadBindings;
import PadLabels;
import Updates;

void RaiseD3DIMRenderTargetLimit()
{
    // d3dim700.dll refuses render targets wider or taller than 2048 (CreateDevice and SetRenderTarget return an error),
    // so the renderer is never created above that; loaded here first so ddraw picks up the patched module later
    auto hD3DIM = LoadLibraryExW(L"d3dim700.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!hD3DIM)
        return;

    auto pattern = hook::module_pattern(hD3DIM, "B8 00 08 00 00 39 45 ? 0F 87 ? ? ? ? 39 45 ? 0F 87"); //DIRECT3DDEVICEI::Init
    injector::WriteMemory<uint32_t>(pattern.get_first<uint32_t>(1), 16384, true);

    pattern = hook::module_pattern(hD3DIM, "B8 00 08 00 00 39 85 ? ? ? ? 0F 87 ? ? ? ? 39 85 ? ? ? ? 0F 87"); //DIRECT3DDEVICEI::SetRenderTarget
    injector::WriteMemory<uint32_t>(pattern.get_first<uint32_t>(1), 16384, true);
}

void InitializeGame()
{
    hook::pattern pattern;
    Screen.nWidth = FixSettings().Int("MAIN.ResX");
    Screen.nHeight = FixSettings().Int("MAIN.ResY");

    if (std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2) == 2
        || Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 16384 || Screen.nHeight > 16384)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
    if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 32767 || Screen.nHeight > 32767)
        std::tie(Screen.nWidth, Screen.nHeight) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;

    RaiseD3DIMRenderTargetLimit();

    //Default res
    pattern = hook::pattern("68 E0 01 00 00 8D 44 24 1C 68 80 02 00 00 8D 4C 24 1C 50 51 6A 03 8B CE");
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1), Screen.nHeight, true);
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(10), Screen.nWidth, true);

    //Menu res
    pattern = hook::pattern("8B 44 24 28 89 30 8B 55 04 8B 6D 08");
    struct ForceResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esp + 0x24);
            *reinterpret_cast<uint32_t*>(regs.eax) = Screen.nWidth;
            regs.eax = *(uint32_t*)(regs.esp + 0x28);
            *reinterpret_cast<uint32_t*>(regs.eax) = Screen.nHeight;
        }
    }; injector::MakeInline<ForceResHook>(pattern.get_first(0), pattern.get_first(6));

    // Aspect ratio
    pattern = hook::pattern("D9 5E 4C 8B 46 28 C7 46 50 ? ? ? ? C7 06 ? ? ? ? 89 6C 24 30");
    injector::WriteMemory<float>(pattern.get_first(9), Screen.fAspectRatio, true);

    pattern = hook::pattern("A1 ? ? ? ? 83 C4 0C D8 C9 50 51");
    injector::WriteMemory<float>(*pattern.get_first<float*>(1), Screen.fAspectRatio, true);

    //2D
    static auto dword_56F775 = hook::get_pattern("8B 8E B0 01 00 00 8B 01 FF 90 B4 00 00 00 8B 8E 7C 01 00 00 85 C9", 0);
    static auto dword_4AF6E1 = hook::get_pattern("85 F6 74 11 8B 46 04 48 89 46 04 75 08 8B 06 6A 01 8B CE FF 10 5E C2 08 00", 0);
    // The two strip cameras precede the separate caption and speaker draws.
    KnightRiderCamera::SubtitleStripCallers = {
        hook::get_pattern("8B 4E 2C E8 ? ? ? ? 8B 4E 1C E8 ? ? ? ? 8B 4E 20 E8 ? ? ? ? 8A 46 58 84 C0", 0),
        hook::get_pattern("8B 4E 1C E8 ? ? ? ? 8B 4E 20 E8 ? ? ? ? 8A 46 58 84 C0 74 10 8B 46 14 8B 4E", 0)
    };
    static auto dword_4BD9A5 = hook::get_pattern("8B 96 C4 00 00 00 33 C9 8B 52 08 8B 82 EC 00 00 00 85 C0", 0);
    static auto dword_4081EF = hook::pattern("83 7C 24 08 01 75 08 8B 49 10 E8 ? ? ? ? C2 08 00").count(2).get(1).get<void>(15); //objective marker
    static auto dword_404D92 = hook::get_pattern("8B 4B 20 E8 ? ? ? ? 5F 5E", 0); //night vision 1
    static auto dword_404D9A = hook::get_pattern("5F 5E 5B C2 08 00 81 EC C0 00 00 00", 0); //night vision 2
    static auto dword_4B514B = hook::get_pattern("C6 86 B4 00 00 00 00 8B 43 0C 8B 48 4C 89 8E B8 00 00 00", 0);

    pattern = hook::pattern("C7 44 24 14 00 00 00 00 DA 4C 24 10 E8 ? ? ? ? 89 44 24 7C");
    struct HUDHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<uint32_t*>(regs.esp + 0x14) = 0;

            auto ret1 = *(void**)(regs.esp + 0xC8);
            auto ret2 = *(void**)(regs.esp + 0xF0);
            auto ret3 = *(void**)(regs.esp + 0xF4);

            if (KnightRiderCamera::DrawingMain)
                return;
            if (KnightRiderCamera::Caller)
            {
                ret2 = KnightRiderCamera::Caller;
                ret3 = KnightRiderCamera::CallerNext;
            }

            if (ret1 == dword_56F775 && ret2 != dword_4AF6E1 && !KnightRiderCamera::IsSubtitleStripCall(ret2) && ret2 != dword_4BD9A5 && ret2 != dword_4081EF && ret2 != dword_404D92 && ret2 != dword_404D9A && ret3 != dword_4B514B)
            {
                if (KnightRiderHud::InfoScene)
                {
                    const auto viewport = reinterpret_cast<const float*>(regs.edi);
                    *reinterpret_cast<int32_t*>(regs.esp + 0x70) = static_cast<int32_t>(std::lround(viewport[0] * Screen.fWidth));
                    *reinterpret_cast<int32_t*>(regs.esp + 0x74) = static_cast<int32_t>(std::lround((1.0f - viewport[2]) * Screen.fHeight));
                    *reinterpret_cast<int32_t*>(regs.esp + 0x78) = static_cast<int32_t>(std::lround((viewport[1] - viewport[0]) * Screen.fWidth));
                    return;
                }
                int anchor = KnightRiderHud::Anchor == 3 ? 0 : KnightRiderHud::Anchor;
                if (anchor == 2)
                {
                    const float center = (*reinterpret_cast<int32_t*>(regs.esp + 0x70)
                        + *reinterpret_cast<int32_t*>(regs.esp + 0x78) * 0.5f) / Screen.fWidth;
                    anchor = center < 0.4f ? -1 : center > 0.6f ? 1 : 0;
                }
                const float scale = std::min(1.0f, Screen.fAspectRatio / (4.0f / 3.0f));
                const float width = std::min(Screen.fWidth, Screen.fHeight * (4.0f / 3.0f));
                const float offset = (Screen.fWidth - width) * 0.5f
                    + KnightRiderHud::Offset(Screen.fWidth, Screen.fHeight, anchor);
                auto& x = *reinterpret_cast<int32_t*>(regs.esp + 0x70);
                auto& y = *reinterpret_cast<int32_t*>(regs.esp + 0x74);
                auto& w = *reinterpret_cast<int32_t*>(regs.esp + 0x78);
                x = static_cast<int32_t>(std::lround(x * width / Screen.fWidth + offset));
                w = static_cast<int32_t>(std::lround(w * width / Screen.fWidth));
                y = static_cast<int32_t>(std::lround(y * scale + (Screen.fHeight * (1.0f - scale)) * 0.5f));
                *reinterpret_cast<int32_t*>(regs.esp + 0x10) = static_cast<int32_t>(std::lround(Screen.fHeight * scale));
            }
        }
    }; injector::MakeInline<HUDHook>(pattern.get_first(0), pattern.get_first(8)); //0x5C0EB1

    // Clear depth at the same position as the adjusted cockpit and speaker viewports.
    static const auto cameraClearReturn = hook::get_pattern("8B 81 B0 01 00 00 85 C0 74 ? 8B 10 56 8B 74 24 08 81 C1 6C 01 00 00 56 51 8B C8 FF 52 44 5E C2 04 00", 30);
    static const auto speakerClearReturn = hook::get_pattern("8B 4E 28 85 C9 74 ? 6A 04 E8 ? ? ? ? 8B 4E 28 E8 ? ? ? ? 5E C2 08 00", 14);
    pattern = hook::pattern("8B 4D 00 89 8E 48 02 00 00 8B 10 8B 46 28 89 96 4C 02 00 00");
    static auto HudDepthClearHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& context)
    {
        const auto stack = reinterpret_cast<void* const*>(context.esp);
        const bool speaker = stack[0x28 / sizeof(void*)] == cameraClearReturn
            && stack[0x38 / sizeof(void*)] == speakerClearReturn;
        if (!speaker && (!KnightRiderHud::CockpitActive || (KnightRiderHud::Anchor != -1 && KnightRiderHud::Anchor != 1)))
            return;

        const float designWidth = std::min(Screen.fWidth, Screen.fHeight * KnightRiderCamera::NativeAspect);
        const float offset = (Screen.fWidth - designWidth) * 0.5f
            + (speaker ? KnightRiderHud::Offset(Screen.fWidth, Screen.fHeight, -1) : 0.0f);
        auto rect = reinterpret_cast<int32_t*>(context.esp + 0x18);
        rect[0] = static_cast<int32_t>(std::lround(rect[0] * designWidth / Screen.fWidth + offset));
        rect[2] = static_cast<int32_t>(std::lround(rect[2] * designWidth / Screen.fWidth + offset));
        if (speaker && Screen.fAspectRatio < KnightRiderCamera::NativeAspect)
        {
            const float scale = Screen.fAspectRatio / KnightRiderCamera::NativeAspect;
            const float top = Screen.fHeight * (1.0f - scale) * 0.5f;
            rect[1] = static_cast<int32_t>(std::lround(rect[1] * scale + top));
            rect[3] = static_cast<int32_t>(std::lround(rect[3] * scale + top));
        }
    });

    pattern = hook::pattern("D8 0D ? ? ? ? 53 8B D9 56 57 D9 F2 8B 53 08");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float t = 0.0087266462f;
            float f = 0.0f;
            _asm {fstp[f]}
            const auto factor = KnightRiderCamera::FOVFactor.load(std::memory_order_relaxed);
            KnightRiderCamera::NativeFOV = f;
            f = AdjustFOV(std::min(f * factor, 170.0f), Screen.fAspectRatio);
            _asm {fld[f]}
            _asm {fmul[t]}
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6)); //0x485717

    // A failed Miles provider switch must leave the listener closed.
    pattern = hook::pattern("8B 86 30 02 00 00 50 FF 15 ? ? ? ? 8B 8E 20 02 00 00 51 FF 15");
    static auto ClearClosedListener = safetyhook::create_mid(pattern.get_first(13), [](SafetyHookContext& context)
    {
        *reinterpret_cast<void**>(context.esi + 0x230) = nullptr;
    });

    //Mouse cooperative level: some systems refuse non-exclusive mouse access, which makes the game abort at start-up
    pattern = hook::pattern("8B 10 6A 06 57 50 FF 52 34 8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B 08 50 FF 51 1C");
    struct MouseCooperativeLevelHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto device = reinterpret_cast<IDirectInputDevice7A*>(regs.eax);
            auto window = reinterpret_cast<HWND>(regs.edi);
            auto result = device->SetCooperativeLevel(window, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
            if (FAILED(result))
                result = device->SetCooperativeLevel(window, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
            regs.eax = static_cast<uint32_t>(result);
        }
    }; injector::MakeInline<MouseCooperativeLevelHook>(pattern.get_first(2), pattern.get_first(9)); //0x4C92EE

    MessagePumpGuard::Install(GetModuleHandleW(nullptr));
    KnightRiderWindow::Init(Screen.nWidth, Screen.nHeight);
    KnightRiderIntro::Init();
    KnightRiderHud::Init();
    KnightRiderCamera::Init();
    KnightRiderFrame::Init();
    KnightRiderUpdates::Init();
    KnightRiderOverlay::Init();
    KnightRiderPadLabels::Init();
    KnightRiderPadBindings::Init();
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
