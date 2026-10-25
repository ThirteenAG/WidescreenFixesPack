#include "stdafx.h"
#include "FixSettings.hpp"
#include "Camera.hpp"
#include "Movie.hpp"
#include <d3d9.h>
#include "NativeMenu.hpp"
#include "Window.hpp"
#include "Intro.hpp"
#include "Focus.hpp"
#include <array>

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScale;
    float fHudOffset;
    float fTextOffset;
    float fFMVOffset;
    float fInvMenuHeartLineEnd; // On-Screen Range: -0.5 to 0.5
    float fInvMenuHeartLineWidth; // Full Screen Width is 0.002
} Screen;

void Init()
{
    WindowedModeWrapper::EnableDpiAwareness();
    (void)FixSettings();
    CoCWindow::Init();
    CoCIntro::Init();
    CoCFocus::Init();
    asi_settings::LiveLink::Start();

    auto pattern = hook::pattern("8B 4C 24 18 BF 01 00 00 00 C7"); //0x406344
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(int32_t*)(regs.esp + 0x18);
            regs.edi = 1;
            int width = int(regs.ebx), height = int(regs.ecx);
            CoCWindow::Resolution(width, height);
            regs.ebx = width;
            regs.ecx = height;
            *reinterpret_cast<int32_t*>(regs.esp + 0x18) = height;

            Screen.Width = regs.ebx;
            Screen.Height = regs.ecx;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
            Screen.fHudScale = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
            Screen.fHudOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);
            Screen.fTextOffset = -(Screen.fHudOffset / Screen.fHudScale);
            Screen.fFMVOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f);

            static auto aspectPattern = hook::pattern("68 A5 AC A9 3F"); //4352C4 435511 5D7EF6
            auto pattern = aspectPattern;
            for (size_t i = 0; i < pattern.size(); ++i)
            {
                ProtectedGameRef<float>{pattern.get(i).get<float>(1)} = Screen.fAspectRatio; // thanks to nemesis2000
            }

            // InventoryMenu - BlackBox (covers top-right line paper)
            static auto cachedPattern1 = hook::pattern("68 D7 A3 30 3F 6A 00");
            static auto cachedPattern2 = hook::pattern("68 E1 7A 14 3F ? ? ? ? ? ? ? 68 52 B8 9E 3E");
            pattern = cachedPattern1; // 422B1A
            if (!cachedPattern1.empty() && !cachedPattern2.empty())
            {
                const uint32_t codePushZeroPush = 0x68006A; // 6A 00 = push 0x00; 68 = push (next value)
                const float blackBoxWidth = (4.0f / 3.0f) * Screen.fHudOffset / Screen.Width;
                ProtectedGameRef<uint32_t>{pattern.get_first<uint32_t>(0)} = codePushZeroPush; // y = 0
                ProtectedGameRef<float>{pattern.get_first<float>(3)} = 1.0f; // x = 1
                pattern = cachedPattern2; // 422B26
                ProtectedGameRef<float>{pattern.get_first<float>(1)} = blackBoxWidth; // w
                ProtectedGameRef<float>{pattern.get_first<float>(13)} = 1.0f; // h = 1
            }
            else
            {
                static const bool logged = []()
                {
                    OutputDebugStringA("Call of Cthulhu: HUD patch group unavailable.\n");
                    return true;
                }();
            }

            // InventoryMenu - HeartMonitorLine (positioned on line paper)
            // NOTE: HeartMonitorLine ended at 103% ScreenWidth (i.e. 3% off-screen) in OG game.
            static auto cachedPattern3 = hook::pattern("D8 0D ? ? ? ? 85 C9 DB");
            pattern = cachedPattern3; // 424FD8; [????] = 0.53
            if (!pattern.empty())
            {
                Screen.fInvMenuHeartLineEnd = Screen.fHudScale * 0.5f; // 50% instead of 53% so line doesn't go past screen edge
                ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fInvMenuHeartLineEnd;
            }
            else
            {
                static const bool logged = []()
                {
                    OutputDebugStringA("Call of Cthulhu: HUD patch group unavailable.\n");
                    return true;
                }();
            }

            static auto cachedPattern4 = hook::pattern("D8 0D ? ? ? ? 8B 44 24 ? 85 C0");
            pattern = cachedPattern4; // 425DC8; [????] = 0.000775
            if (!pattern.empty())
            {
                Screen.fInvMenuHeartLineWidth = Screen.fHudScale * 0.000715f; // Subtracted 0.00006 (3% + 3%)
                ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fInvMenuHeartLineWidth;
            }
            else
            {
                static const bool logged = []()
                {
                    OutputDebugStringA("Call of Cthulhu: HUD patch group unavailable.\n");
                    return true;
                }();
            }

            // HealMenu - Fix Cursor Boxes
            // NOTE: Cursor was off by one small grid square in the OG game (fixed)
            static auto cachedPattern5 = hook::pattern("D8 0D ? ? ? ? 89 44 24 ? D9 6C 24 ? DB 5C 24 ? 8B 44 24 ? 50");
            pattern = cachedPattern5; // 4247B5
            if (!pattern.empty())
            {
                float* ptr = injector::ReadMemory<float*>(pattern.get_first(2));
                static const std::array<float, 5> originalHealValues = { ptr[0], ptr[1], ptr[2], ptr[3], ptr[4] };
                const float HealMenuBoxWidth = originalHealValues[0]; // 26.087f
                const float HealMenuCursorXMin = originalHealValues[1]; // 0.35f
                const float HealMenuCursorYMax = originalHealValues[2]; // 0.87f
                const float HealMenuCursorYMin = originalHealValues[3]; // 0.635f
                const float HealMenuCursorXMax = originalHealValues[4]; // 0.58f
                const float TinyBoxWidth = (HealMenuCursorXMax - HealMenuCursorXMin) / 24.0f; // 24 tiny boxes wide
                ptr[0] = HealMenuBoxWidth / Screen.fHudScale;
                ptr[1] = ((HealMenuCursorXMin + TinyBoxWidth - 0.5f) * Screen.fHudScale) + 0.5f;
                ptr[2] = HealMenuCursorYMax - TinyBoxWidth;
                ptr[3] = HealMenuCursorYMin - TinyBoxWidth;
                ptr[4] = ((HealMenuCursorXMax + TinyBoxWidth - 0.5f) * Screen.fHudScale) + 0.5f;
            }
            else
            {
                static const bool logged = []()
                {
                    OutputDebugStringA("Call of Cthulhu: HUD patch group unavailable.\n");
                    return true;
                }();
            }

        }
    };
    const bool resolutionAvailable = !pattern.empty();
    if (resolutionAvailable)
        injector::MakeInline<ResHook>(pattern.count_hint(1).get(0).get<uintptr_t>(0), pattern.count_hint(1).get(0).get<uintptr_t>(9));
    else
        OutputDebugStringA("Call of Cthulhu: ResHook unavailable.\n");

    pattern = hook::pattern("D9 05 ? ? ? ? D8 F1 D9 5C 24 38 DD D8 DD D8"); //0x4E1486
    struct HudHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto ptr = *(uintptr_t*)(regs.esp + 0x60);
            float x = (float)(*(int32_t*)(regs.esp + 0x24));

            if (*(float*)(ptr + 0x0) == 0.0f && *(float*)(ptr + 0x8) == Screen.fWidth &&
                FixSettings().Bool("MAIN.StretchFullscreenImages"))
            {
                *(float*)(regs.esp + 0x2C) = 1.0f / (10.0f * x);
                *(float*)(regs.esp + 0x34) = -0.5f / x;
            }
            else
            {
                *(float*)(regs.esp + 0x2C) = Screen.fHudScale / (10.0f * x);
                *(float*)(regs.esp + 0x34) = Screen.fHudOffset / x;
            }

            float fneg05 = -0.5f;
            _asm fld ds : fneg05
        }
    };
    if (resolutionAvailable && !pattern.empty())
        injector::MakeInline<HudHook>(pattern.count_hint(1).get(0).get<uintptr_t>(0), pattern.count_hint(1).get(0).get<uintptr_t>(6));
    else
        OutputDebugStringA("Call of Cthulhu: HudHook unavailable.\n");

    pattern = hook::pattern("89 4C 24 08 8B 48 08 89 54 24 0C 8B 50 0C 8D 44 24 04"); //0x4239B1
    struct ClickableAreaHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.esp + 8) = (*(float*)&regs.ecx * Screen.fHudScale) + Screen.fHudOffset;
            *(float*)&regs.ecx = (*(float*)(regs.eax + 0x8) * Screen.fHudScale);
        }
    };
    if (resolutionAvailable && !pattern.empty())
        injector::MakeInline<ClickableAreaHook>(pattern.count_hint(1).get(0).get<uintptr_t>(0), pattern.count_hint(1).get(0).get<uintptr_t>(7));
    else
        OutputDebugStringA("Call of Cthulhu: ClickableAreaHook unavailable.\n");

    pattern = hook::pattern("D8 25 ? ? ? ? D8 F2 D9 5C 24 2C D9"); //0x4E0BC4
    if (resolutionAvailable && !pattern.empty())
        ProtectedGameRef<float*>{pattern.get_first<float*>(2)} = &Screen.fTextOffset;
    else
        OutputDebugStringA("Call of Cthulhu: text offset patch unavailable.\n");

    pattern = hook::pattern("D8 F2 D9 5C 24 2C D9 44 24 44 D8 25"); //0x4E0BCA
    struct TextHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float temp = 0.0f;
            _asm {fdiv    st, st(2)}
            _asm {fstp    dword ptr[temp]}
            *(float*)(regs.esp + 0x2C) = temp;

            *(float*)(regs.esp + 0x24) *= Screen.fHudScale;
            *(float*)(regs.esp + 0x2C) *= Screen.fHudScale;
        }
    };
    if (resolutionAvailable && !pattern.empty())
        injector::MakeInline<TextHook>(pattern.count_hint(1).get(0).get<uintptr_t>(0), pattern.count_hint(1).get(0).get<uintptr_t>(6));
    else
        OutputDebugStringA("Call of Cthulhu: TextHook unavailable.\n");

    // The decoder publishes one texture per frame. Its metadata is consumed
    // only by the full-viewport movie call, never by the cursor blit below it.
    auto moviePublish = hook::pattern("8B 4E 14 E8 ? ? ? ? 8B 4E 14 E8 ? ? ? ? 57 E8 ? ? ? ? 83 C4 04 5F 32 C0 5E C3");
    auto movieDraw = hook::pattern("6A 01 6A 00 51 52 50 6A 00 6A 00 56 E8 ? ? ? ? 83 C4 20 C7 05 ? ? ? ? 00 00 00 00 5E 83 C4 18 C3");
    if (moviePublish.size() == 1 && movieDraw.size() == 1)
    {
        static SafetyHookMid MoviePublishHook, MovieDrawHook;
        MoviePublishHook = safetyhook::create_mid(moviePublish.get_first(16), [](SafetyHookContext& regs)
        {
            CoCMovie::frame = {};
            auto player = reinterpret_cast<uint8_t*>(regs.esi);
            auto decoder = *reinterpret_cast<uint8_t**>(player + 20);
            CoCMovie::frame = CoCMovie::FromDecoder(decoder, reinterpret_cast<void*>(regs.edi), int(regs.eax));
        });
        MovieDrawHook = safetyhook::create_mid(movieDraw.get_first(12), [](SafetyHookContext& regs)
        {
            auto args = reinterpret_cast<uint32_t*>(regs.esp);
            const auto metadata = CoCMovie::Take(reinterpret_cast<void*>(args[5]));
            const int width = int(args[3]), height = int(args[4]);
            CoCMovie::Rectangle rectangle;
            if (!metadata.texture || args[1] || args[2] ||
                !CoCMovie::Frame(rectangle, width, height, metadata.width, metadata.height, FixSettings().Int("MAIN.FMVWidescreenMode")))
            {
                CthulhuMenu::Log("CoC video: frame metadata or viewport rejected; native playback retained");
                return;
            }
            if (rectangle.width < width || rectangle.height < height)
            {
                auto renderer = reinterpret_cast<uint8_t*>(args[0]);
                auto device = renderer ? *reinterpret_cast<IDirect3DDevice9**>(renderer + 104) : nullptr;
                if (!device || FAILED(device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0)))
                {
                    CthulhuMenu::Log("CoC video: black-bar clear failed; native playback retained");
                    return;
                }
            }
            args[1] = rectangle.left;
            args[2] = rectangle.top;
            args[3] = rectangle.width;
            args[4] = rectangle.height;
        });
        if (!MoviePublishHook || !MovieDrawHook)
        {
            MoviePublishHook.reset();
            MovieDrawHook.reset();
            OutputDebugStringA("Call of Cthulhu: movie hook installation failed; native playback retained.\n");
        }
    }
    else
    {
        OutputDebugStringA("Call of Cthulhu: movie signatures unavailable; native playback retained.\n");
    }

    // The player FOV priority stack reaches the camera setter before it builds
    // both the projection and culling planes. Keep its stored authored FOV intact.
    pattern = hook::pattern("8B 8E C0 00 00 00 51 8B C8 E8 ? ? ? ? 5E 83 C4 08 C3");
    if (pattern.size() == 1)
    {
        static auto GameplayFOVHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
        {
            static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
            auto argument = reinterpret_cast<float*>(regs.esp);
            *argument = CoCCamera::Apply(reinterpret_cast<void*>(regs.esi), *argument, factor);
        });
        if (!GameplayFOVHook)
            OutputDebugStringA("Call of Cthulhu: gameplay FOV hook installation failed.\n");
    }
    else
    {
        OutputDebugStringA("Call of Cthulhu: gameplay FOV signature unavailable; native camera retained.\n");
    }

    CthulhuMenu::Init();
}

ASI_SETTINGS_EXPORT(FixSettings())

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("89 65 E8 8B F4 89 3E 56"));
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
