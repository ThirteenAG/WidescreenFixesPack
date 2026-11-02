module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d11.h>
#include <atomic>

export module Borders;

import ComVars;

namespace HudLayout
{
    struct Rect
    {
        float left, top, right, bottom;
    };

    struct Widget
    {
        const char* name;
        const char* resize;
        int alignment;
    };

    constexpr Widget Widgets[] = {
        { "WidgetHudRewind_01", "HUD.instance1.WidgetHudRewind_01.onResize", 0 },
        { "WidgetHudDistanceToFinish_01", "HUD.instance1.WidgetHudDistanceToFinish_01.onResize", 0 },
        { "WidgetHudPlayerConnect_01", "HUD.instance1.WidgetHudPlayerConnect_01.onResize", 0 },
        { "WidgetHudPlaygroup_01", "HUD.instance1.WidgetHudPlaygroup_01.onResize", 0 },
        { "WidgetHudNextObjective_01", "HUD.instance1.WidgetHudNextObjective_01.onResize", 0 },
        { "WidgetPositionIndicator_01", "HUD.instance1.WidgetPositionIndicator_01.onResize", 0 },
        { "WidgetMinimap_01", "HUD.instance1.WidgetMinimap_01.onResize", 0 },
        { "WidgetMedalTarget_01", "HUD.instance1.WidgetMedalTarget_01.onResize", 2 },
        { "WidgetHudFriendPosition_01", "HUD.instance1.WidgetHudFriendPosition_01.onResize", 2 },
        { "WidgetTimer_01", "HUD.instance1.WidgetTimer_01.onResize", 2 },
        { "WidgetHudSpeedTach_01", "HUD.instance1.WidgetHudSpeedTach_01.onResize", 2 },
        { "WidgetHudNitrous_01", "HUD.instance1.WidgetHudNitrous_01.onResize", 2 },
        { "WidgetHudDraftMeter_01", "HUD.instance1.WidgetHudDraftMeter_01.onResize", 2 },
    };
    constexpr uint32_t AllWidgets = (1u << std::size(Widgets)) - 1;

    std::atomic<void*> Movie = nullptr;
    std::atomic<uint64_t> Revision = 1;
    std::atomic<ULONGLONG> InitializedAt = 0;
    thread_local bool Refreshing = false;
    thread_local float Constraint = 0.0f;
    SafetyHookInline AdvanceHook;

    void Constrain(Rect& rect)
    {
        float width = rect.right - rect.left;
        float height = rect.bottom - rect.top;
        if (!Refreshing || Constraint <= 0.0f || width <= 0.0f || height <= 0.0f)
            return;
        float aspect = std::min(std::clamp(Constraint, 4.0f / 3.0f, 32.0f / 9.0f), width / height);
        float half = height * aspect * 0.5f;
        float center = (rect.left + rect.right) * 0.5f;
        rect.left = center - half;
        rect.right = center + half;
    }

    float __fastcall Advance(void* movie, void*, float time, unsigned int count)
    {
        auto result = AdvanceHook.unsafe_thiscall<float>(movie, time, count);
        if (movie != Movie.load())
            return result;

        auto table = *static_cast<void***>(movie);
        Rect visible{};
        reinterpret_cast<Rect*(__thiscall*)(void*, Rect*)>(table[0x1F])(movie, &visible);
        static void* previousMovie = nullptr;
        static Rect previous{};
        static uint64_t previousRevision = 0;
        static uint32_t pending = AllWidgets;
        static ULONGLONG lastAttempt = 0;
        auto revision = Revision.load();
        if (movie != previousMovie || revision != previousRevision || visible.left != previous.left ||
            visible.top != previous.top || visible.right != previous.right || visible.bottom != previous.bottom)
        {
            previousMovie = movie;
            previous = visible;
            previousRevision = revision;
            pending = AllWidgets;
        }

        auto now = GetTickCount64();
        if (!pending || now - InitializedAt.load() < 500 || now - lastAttempt < 250)
            return result;
        lastAttempt = now;
        auto constraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
        Constraint = constraint && std::isfinite(*constraint) ? *constraint : 0.0f;
        auto available = reinterpret_cast<bool(__thiscall*)(void*, const char*)>(table[0x0A]);
        auto invoke = reinterpret_cast<bool(__cdecl*)(void*, const char*, void*, const char*, ...)>(table[0x16]);
        for (size_t i = 0; i < std::size(Widgets); ++i)
        {
            auto bit = 1u << i;
            if (!(pending & bit) || !available(movie, Widgets[i].resize))
                continue;
            Refreshing = true;
            bool called = invoke(movie, Widgets[i].resize, nullptr, "");
            Refreshing = false;
            if (called)
                pending &= ~bit;
        }
        return result;
    }

    void Install()
    {
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", []() { ++Revision; });

        auto alignment = hook::pattern("8B 74 24 34 F2 0F 2A 46 28 8D 84 24 A4 00 00 00 50 51 8B 4C 24 58");
        static auto widgetHook = safetyhook::create_mid(alignment.get_first(9), [](SafetyHookContext& regs)
        {
            auto node = reinterpret_cast<uint8_t*>(regs.esi);
            auto screen = *reinterpret_cast<uint8_t**>(regs.esp + 0x274);
            auto screenName = *reinterpret_cast<const char**>(screen + 0x20);
            auto name = *reinterpret_cast<const char**>(node + 0x20);
            if (!screenName || !name || std::string_view(screenName) != "HUD")
                return;
            for (const auto& widget : Widgets)
            {
                if (std::string_view(name) == widget.name)
                {
                    regs.xmm0.f64[0] = static_cast<double>(widget.alignment);
                    Movie = reinterpret_cast<void*>(regs.ebp);
                    InitializedAt = GetTickCount64();
                    ++Revision;
                    break;
                }
            }
        });

        auto stage = hook::pattern("E8 ? ? ? ? 8B 10 8B 52 7C 8D 4C 24 14 51 8B C8 FF D2 8D 44 24 14");
        static auto stageHook = safetyhook::create_mid(stage.get_first(19), [](SafetyHookContext& regs)
        {
            Constrain(*reinterpret_cast<Rect*>(regs.esp + 0x14));
        });

        auto advance = hook::pattern("83 EC 54 56 8B F1 8B 86 64 24 00 00 C1 E8 14 A8 01");
        AdvanceHook = safetyhook::create_inline(advance.get_first(), Advance);
    }
}

// MAIN.CutsceneBorders: the cutscenes are framed for 16:9. The 3D view keeps its vertical field of view at any aspect
// (Hor+), so on wider screens the middle 16:9 of the picture is the original framing: Pillarbox covers the rest with
// black bars. On narrower screens the view loses the sides of the frame: Letterbox widens the cutscene's field of view
// to the whole 16:9 frame and covers the space above and below it. Off leaves the full picture. The menus and the
// subtitles are drawn in a 16:9 box in the middle, so the bars never cover them.
namespace
{
    constexpr float FrameAspect = 16.0f / 9.0f;

    int Mode()
    {
        static auto& mode = FixSettings().Int("MAIN.CutsceneBorders");
        return mode;
    }

    // The level's cinematic camera is the one in use (the front end has its own), and when the game simulates: the
    // camera at the last simulation tick changes
    bool bCinebotCamera = false;
    ULONGLONG SimulatedAt = 0;

    // A cutscene, a scripted moment or a quick-time event: in a level, the player's car without the player's control and
    // the level's camera. Not while the pause menu has stopped the game.
    bool InCutscene()
    {
        bool paused = fb::PauseMenu::OpenedAt > SimulatedAt;
        return bCinebotCamera && !paused && fb::Vehicle::InRaceWithoutControl();
    }

    // The race's pause menu (and its settings pages) is shown: the game stops simulating while it's open
    bool PauseMenuShown()
    {
        return fb::PauseMenu::OpenedAt && fb::PauseMenu::OpenedAt > SimulatedAt;
    }

    float ScreenAspect = FrameAspect;

    // Black, copied over the parts of the back buffer outside the frame
    ID3D11Device* Device = nullptr;
    ID3D11Texture2D* Black = nullptr;
    UINT BlackWidth = 0, BlackHeight = 0;
    DXGI_FORMAT BlackFormat = DXGI_FORMAT_UNKNOWN;

    void ReleaseBlack()
    {
        if (Black)
            Black->Release();
        Black = nullptr;
        BlackWidth = BlackHeight = 0;
    }

    bool MakeBlack(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format)
    {
        if (Black && device == Device && width <= BlackWidth && height <= BlackHeight && format == BlackFormat)
            return true;
        ReleaseBlack();
        Device = device;
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = format;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        std::vector<uint8_t> zeros(size_t(width) * height * 16);
        D3D11_SUBRESOURCE_DATA data{ zeros.data(), width * 16, 0 };
        if (FAILED(device->CreateTexture2D(&desc, &data, &Black)))
        {
            Black = nullptr;
            return false;
        }
        BlackWidth = width;
        BlackHeight = height;
        BlackFormat = format;
        return true;
    }

    // The cutscene's field of view, widened to the 16:9 frame on screens narrower than that
    SafetyHookMid CameraFovHook = {};
    uintptr_t* pCameraManager = nullptr;
}

export namespace CutsceneBorders
{
    // After the game's frame, before the overlays
    void Present(IDXGISwapChain* swapChain)
    {
        if (!swapChain)
            return;
        ID3D11Texture2D* backBuffer = nullptr;
        if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer))))
            return;
        D3D11_TEXTURE2D_DESC desc{};
        backBuffer->GetDesc(&desc);
        ScreenAspect = desc.Height ? float(desc.Width) / float(desc.Height) : FrameAspect;

        // The pause menu is drawn in the 16:9 box with its dimmed backdrop: the sides are black like the front end's
        int mode = Mode();
        bool wider = ScreenAspect > FrameAspect + 0.01f;
        bool cutscene = InCutscene();
        bool pillarbox = wider && (((mode & 2) && cutscene) || PauseMenuShown());
        bool letterbox = (mode & 1) && ScreenAspect < FrameAspect - 0.01f && cutscene;
        if ((pillarbox || letterbox) && desc.SampleDesc.Count == 1)
        {
            ID3D11Device* device = nullptr;
            backBuffer->GetDevice(&device);
            ID3D11DeviceContext* context = nullptr;
            device->GetImmediateContext(&context);
            if (pillarbox)
            {
                UINT frame = UINT(std::lround(desc.Height * FrameAspect));
                UINT bar = (desc.Width - std::min(frame, desc.Width)) / 2;
                if (bar && MakeBlack(device, bar, desc.Height, desc.Format))
                {
                    D3D11_BOX box{ 0, 0, 0, bar, desc.Height, 1 };
                    context->CopySubresourceRegion(backBuffer, 0, 0, 0, 0, Black, 0, &box);
                    context->CopySubresourceRegion(backBuffer, 0, desc.Width - bar, 0, 0, Black, 0, &box);
                }
            }
            else
            {
                UINT frame = UINT(std::lround(desc.Width / FrameAspect));
                UINT bar = (desc.Height - std::min(frame, desc.Height)) / 2;
                if (bar && MakeBlack(device, desc.Width, bar, desc.Format))
                {
                    D3D11_BOX box{ 0, 0, 0, desc.Width, bar, 1 };
                    context->CopySubresourceRegion(backBuffer, 0, 0, 0, 0, Black, 0, &box);
                    context->CopySubresourceRegion(backBuffer, 0, 0, desc.Height - bar, 0, Black, 0, &box);
                }
            }
            context->Release();
            device->Release();
        }
        backBuffer->Release();
    }

    void Shutdown()
    {
        ReleaseBlack();
    }
}

class Borders
{
public:
    Borders()
    {
        WFP::onInitEvent() += []()
        {
            HudLayout::Install();
            // After ClientCameraManager::update: the view's vertical field of view (radians), the cutscene's own
            // (FrameRate hooks the instructions right after the update call, so they're left out of both patterns)
            auto pattern = hook::pattern("E8 ? ? ? ? 8B 03 8B 38 8B 68 04 F3 0F 10 43 04 83 C4 04");
            // mov ecx, the camera manager: the update call's 'this', 0x15 bytes before
            auto loadAddress = reinterpret_cast<uintptr_t>(pattern.get_first(-0x15));
            auto load = hook::range_pattern(loadAddress, loadAddress + 6, "8B 0D ? ? ? ?").get_first<uint8_t>();
            pCameraManager = *reinterpret_cast<uintptr_t**>(load + 2);
            CameraFovHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                auto manager = *pCameraManager;
                if (!manager)
                    return;
                // The camera in use and its type
                static uintptr_t lastCamera = 0;
                auto camera = (*(uintptr_t(__thiscall**)(uintptr_t))(*(uintptr_t*)manager + 0x14))(manager);
                if (camera != lastCamera)
                {
                    lastCamera = camera;
                    auto typeInfo = camera ? (*(uintptr_t(__thiscall**)(uintptr_t))(*(uintptr_t*)camera))(camera) : 0;
                    auto name = typeInfo ? **(const char***)typeInfo : nullptr;
                    bCinebotCamera = name && !strcmp(name, "CinebotCamera");
                }
                static float lastTick[16] = {};
                if (memcmp(lastTick, (void*)(manager + 0xEF0), sizeof(lastTick)) != 0)
                {
                    memcpy(lastTick, (void*)(manager + 0xEF0), sizeof(lastTick));
                    SimulatedAt = GetTickCount64();
                }

                if (!(Mode() & 1) || ScreenAspect >= FrameAspect - 0.01f || !InCutscene())
                    return;
                auto view = manager + 0xA50;
                auto fov = (float*)(view + 0x48);
                // Once for each value the game sets
                static float widened = 0.0f;
                if (!(*fov > 0.0f && *fov < 3.0f) || *fov == widened)
                    return;
                widened = 2.0f * std::atan(std::tan(*fov * 0.5f) * FrameAspect / ScreenAspect);
                *fov = widened;
                *(uint32_t*)(view + 0xA0) |= 0xD;   // the projection needs updating
            });

            WFP::onShutdownEvent() += []()
            {
                CutsceneBorders::Shutdown();
            };
        };
    }
} Borders;
