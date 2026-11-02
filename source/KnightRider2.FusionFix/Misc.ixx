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
import Updates;
import DeveloperOverlay;
import PadBindings;
import PadLabels;

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

SafetyHookInline MovieOpenHook;
std::atomic<bool> LetterboxedMovie{ false };

char __fastcall OpenMovie(void* player, void*, const char** filename, void* target)
{
    const auto path = *filename + 6; // The game's string payload follows its six-byte header.
    const auto slash = std::strrchr(path, '/');
    const auto backslash = std::strrchr(path, '\\');
    const auto name = (slash && backslash ? std::max(slash, backslash) : slash ? slash : backslash);
    const std::string_view base = name ? name + 1 : path;
    const auto stem = base.substr(0, base.find('.'));
    // The intro and new game movies are 640x480 files with black bars baked in above and below the picture.
    const auto equals = [&](std::string_view other)
    {
        return stem.size() == other.size() && _strnicmp(stem.data(), other.data(), other.size()) == 0;
    };
    const bool letterboxed = equals("intro") || equals("newgame");
    const auto result = MovieOpenHook.thiscall<char>(player, filename, target);
    LetterboxedMovie.store(result && letterboxed, std::memory_order_relaxed);
    return result;
}

// DirectShow decodes into a DirectDraw surface; only the movie blit uses this path.
IDirectDrawSurface* WINAPI BltMovieFrame(IDirectDrawSurface* destination, RECT* destinationRect,
    IDirectDrawSurface* source, RECT* sourceRect, DWORD flags, DDBLTFX* effects)
{
    DDSURFACEDESC description{ sizeof(description) };
    if (SUCCEEDED(destination->GetSurfaceDesc(&description)) && description.dwWidth && description.dwHeight)
    {
        const auto width = static_cast<LONG>(description.dwWidth);
        const auto height = static_cast<LONG>(description.dwHeight);
        DDSURFACEDESC video{ sizeof(video) };
        const bool original = std::clamp(FixSettings().Int("MAIN.FMVWidescreenMode"), 0, 1) == 0;
        RECT visible{};
        if (!original && LetterboxedMovie.load(std::memory_order_relaxed) &&
            SUCCEEDED(source->GetSurfaceDesc(&video)) && video.dwWidth == 640 && video.dwHeight == 480 &&
            (!sourceRect || (sourceRect->left == 0 && sourceRect->top == 0 && sourceRect->right == 640 && sourceRect->bottom == 480)))
        {
            // The picture stays within rows 72-408 in both movies; keep every column.
            visible = { 0, 72, 640, 408 };
            sourceRect = &visible;
        }
        float aspect = 4.0f / 3.0f;
        if (sourceRect && sourceRect->right > sourceRect->left && sourceRect->bottom > sourceRect->top)
            aspect = static_cast<float>(sourceRect->right - sourceRect->left) / (sourceRect->bottom - sourceRect->top);
        else if (SUCCEEDED(source->GetSurfaceDesc(&video)) && video.dwWidth && video.dwHeight)
            aspect = static_cast<float>(video.dwWidth) / video.dwHeight;
        const float boxWidth = original ? std::min(static_cast<float>(width), height * (4.0f / 3.0f)) : static_cast<float>(width);
        const float boxHeight = original ? std::min(static_cast<float>(height), width / (4.0f / 3.0f)) : static_cast<float>(height);
        const LONG movieWidth = std::max(1L, static_cast<LONG>(std::lround(std::min(boxWidth, boxHeight * aspect))));
        const LONG movieHeight = std::max(1L, static_cast<LONG>(std::lround(movieWidth / aspect)));

        RECT movieRect{ (width - movieWidth) / 2, (height - movieHeight) / 2, 0, 0 };
        movieRect.right = movieRect.left + movieWidth;
        movieRect.bottom = movieRect.top + movieHeight;
        DDBLTFX clear{ sizeof(clear) };
        destination->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &clear);
        destination->Blt(&movieRect, source, sourceRect, flags, effects);
    }
    else
    {
        destination->Blt(destinationRect, source, sourceRect, flags, effects);
    }
    return destination;
}

void InitializeGame()
{
    auto movieOpen = hook::pattern("8B 44 24 04 81 EC 94 06 00 00 53 55 8B E9 8B 08 56 83 C1 06 57 51 8D 4C 24 20 E8 ? ? ? ?");
    MovieOpenHook = safetyhook::create_inline(movieOpen.get_first(), OpenMovie);
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.nWidth = settings.Int("MAIN.ResX");
    Screen.nHeight = settings.Int("MAIN.ResY");

    if (std::clamp(settings.Int("MAIN.WindowMode"), 0, 2) == 2 || Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 16384 || Screen.nHeight > 16384)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
    if (Screen.nWidth <= 0 || Screen.nHeight <= 0)
        std::tie(Screen.nWidth, Screen.nHeight) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;

    RaiseD3DIMRenderTargetLimit();

    //Default res
    auto pattern = hook::pattern("68 E0 01 00 00 8D 44 24 18 68 80 02 00 00 8D 4C 24 18 50 51 6A 04");
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(1), Screen.nHeight, true);
    injector::WriteMemory<int32_t>(pattern.get_first<int32_t>(10), Screen.nWidth, true);

    //Menu res
    pattern = hook::pattern("8B 5C 24 30 89 03 8B 45 04 8B 75 08");
    struct ForceResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = *(uint32_t*)(regs.esp + 0x2C);
            *(uint32_t*)(regs.ebx) = Screen.nWidth;
            regs.ebx = *(uint32_t*)(regs.esp + 0x30);
            *(uint32_t*)(regs.ebx) = Screen.nHeight;
        }
    }; injector::MakeInline<ForceResHook>(pattern.get_first(0), pattern.get_first(6)); //0x5E183D

    //2D
    static auto dword_7E2446 = hook::get_pattern("83 C4 0C 5D C3 55 8B CD E8 ? ? ? ? 5D C3", 13);
    static auto dword_63AFDC = hook::get_pattern("8B 44 24 04 85 C0 74 17 8B 70 04 8B C8 83 C0 04 4E", 0);
    static auto dword_65E46E = hook::get_pattern("80 88 CC 00 00 00 01 8B 41 68 8B 48 08 E8 ? ? ? ? 5E C2 08 00", 18);
    static auto dword_65E540 = hook::pattern("8B 56 64 8B 4A 08 33 D2 66 39 91 D6 00 00 00").count(2).get(1).get<void>(0);
    static auto dword_532FF8 = hook::get_pattern("8B 4E 44 E8 ? ? ? ? 8A 46 79 84 C0 0F 84 ? ? ? ? 8B 46 2C", 0);
    static auto dword_5369D3 = hook::get_pattern("83 C4 10 8D 4C 24 04 E8 ? ? ? ? 8B 4E 0C E8 ? ? ? ? 5E 59 C2 08 00", 20);
    static auto dword_52E81A = hook::get_pattern("8B 4E 30 E8 ? ? ? ? 5E C2 08 00", 8);
    static auto dword_5D4B43 = hook::get_pattern("83 C6 04 4F 75 E7 5F 5E 5B C2 04 00", 0);
    static auto dword_538F62 = hook::get_pattern("8B 4F 7C E8 ? ? ? ? 5F 5E 5D C2 04 00", 8);
    static auto dword_52A984 = hook::get_pattern("8B 4E 28 E8 ? ? ? ? 5E 59 C2 08 00", 0); //night vision 1
    static auto dword_52A98C = hook::get_pattern("8B 4E 28 E8 ? ? ? ? 5E 59 C2 08 00", 8); //night vision 2

    pattern = hook::pattern("C7 44 24 14 00 00 00 00 DA 4C 24 10 E8 ? ? ? ? 89 44 24 7C 8B 46 30");
    struct HUDHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esp + 0x14) = 0;

            auto ret2 = *(void**)(regs.esp + 0xF4);
            auto ret3 = *(void**)(regs.esp + 0x100);

            if (KnightRider2Camera::DrawingMain)
                return;
            if (KnightRider2Camera::Caller)
                ret3 = KnightRider2Camera::Caller;

            if (ret2 == dword_7E2446 && ret3 != dword_63AFDC && ret3 != dword_65E46E && ret3 != dword_65E540 && ret3 != dword_532FF8 && ret3 != dword_5369D3 &&
                ret3 != dword_52E81A && ret3 != dword_5D4B43 && ret3 != dword_538F62 && ret3 != dword_52A984 && ret3 != dword_52A98C)
            {
                if (KnightRider2Hud::InfoScene)
                {
                    const auto viewport = reinterpret_cast<const float*>(regs.edi);
                    *reinterpret_cast<int32_t*>(regs.esp + 0x70) = static_cast<int32_t>(std::lround(viewport[0] * Screen.fWidth));
                    *reinterpret_cast<int32_t*>(regs.esp + 0x74) = static_cast<int32_t>(std::lround((1.0f - viewport[2]) * Screen.fHeight));
                    *reinterpret_cast<int32_t*>(regs.esp + 0x78) = static_cast<int32_t>(std::lround((viewport[1] - viewport[0]) * Screen.fWidth));
                    return;
                }
                int anchor = KnightRider2Hud::Anchor == 3 ? 0 : KnightRider2Hud::Anchor;
                if (anchor == 2)
                {
                    const float center = (*reinterpret_cast<int32_t*>(regs.esp + 0x70)
                        + *reinterpret_cast<int32_t*>(regs.esp + 0x78) * 0.5f) / Screen.fWidth;
                    anchor = center < 0.4f ? -1 : center > 0.6f ? 1 : 0;
                }
                const float scale = std::min(1.0f, Screen.fAspectRatio / (4.0f / 3.0f));
                const float width = std::min(Screen.fWidth, Screen.fHeight * (4.0f / 3.0f));
                const float offset = (Screen.fWidth - width) * 0.5f
                    + KnightRider2Hud::Offset(Screen.fWidth, Screen.fHeight, anchor);
                auto& x = *reinterpret_cast<int32_t*>(regs.esp + 0x70);
                auto& y = *reinterpret_cast<int32_t*>(regs.esp + 0x74);
                auto& w = *reinterpret_cast<int32_t*>(regs.esp + 0x78);
                x = static_cast<int32_t>(std::lround(x * width / Screen.fWidth + offset));
                w = static_cast<int32_t>(std::lround(w * width / Screen.fWidth));
                y = static_cast<int32_t>(std::lround(y * scale + (Screen.fHeight * (1.0f - scale)) * 0.5f));
                *reinterpret_cast<int32_t*>(regs.esp + 0x10) = static_cast<int32_t>(std::lround(Screen.fHeight * scale));
            }
        }
    }; injector::MakeInline<HUDHook>(pattern.get_first(0), pattern.get_first(8)); //0x

    //Mouse cooperative level: some systems refuse non-exclusive mouse access, which makes the game abort at start-up
    pattern = hook::pattern("8B 4C 24 20 8B 46 3C 8B 10 6A 06 51 50 FF 52 34");
    struct MouseCooperativeLevelHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto device = reinterpret_cast<IDirectInputDevice7A*>(regs.eax);
            auto window = reinterpret_cast<HWND>(regs.ecx);
            auto result = device->SetCooperativeLevel(window, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
            if (FAILED(result))
                result = device->SetCooperativeLevel(window, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
            regs.eax = static_cast<uint32_t>(result);
        }
    }; injector::MakeInline<MouseCooperativeLevelHook>(pattern.get_first(9), pattern.get_first(16)); //0x67D499

    KnightRider2Window::Init(Screen.nWidth, Screen.nHeight);
    KnightRider2Intro::Init();
    KnightRider2Hud::Init();
    KnightRider2Camera::Init();
    KnightRider2Frame::Init();
    KnightRider2Updates::Init();
    KnightRider2Overlay::Init();
    KnightRider2PadBindings::Init();
    KnightRider2PadLabels::Init();

    pattern = hook::pattern("6A 00 68 00 00 00 01 8D 4C 24 18 51 8B 4C 24 18 51 6A 00 50 FF 52 14 8B 44 24 24 8B 10 50 FF 52 08");
    // Replace Blt and its discarded result with a call returning the surface for Release.
    auto blit = pattern.get_first<uint8_t>(20);
    injector::MakeCALL(blit, BltMovieFrame);
    injector::MakeNOP(blit + 5, 2);
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
