module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <LEDEffects.h>
#include <d3dx9.h>
#include <atomic>
#include <stdexcept>

export module D3DDrv;

import ComVars;
import GUI;
import WidescreenHUD;
import Xidi;
import DeveloperOverlay;
import Movie;

std::atomic<float> gVisibility{ 1.0f };
export int32_t gBlacklistIndicators = 0;
export FLTColor gColor = { 0 };
export uint32_t bLightSyncRGB;
float* __cdecl FGetHSV(float* dest, uint8_t H, uint8_t S, uint8_t V, const uint8_t* light)
{
    bool bChangeColor = false;
    if ((H == 0x41 && S == 0xC8) || (H == 0x2C && S == 0xCC) || (H == 0x00 && S == 0xFF && V == 0xFF))
    {
        const auto id = *reinterpret_cast<const uint32_t*>(light + 20);
        bChangeColor = id == 862 || id == 879 || id == 881 || id == 875;
        if (bChangeColor && !gColor.empty())
        {
            dest[0] = gColor.R;
            dest[1] = gColor.G;
            dest[2] = gColor.B;
            dest[3] = 1.0f;
            if (gBlacklistIndicators)
            {
                const float visibility = gVisibility.load(std::memory_order_relaxed);
                dest[0] *= visibility;
                dest[1] *= visibility;
                dest[2] *= visibility;
                dest[3] *= visibility;
            }
            return dest;
        }
    }

    float r, g, b, a = 1.0f;
    float v14 = static_cast<float>(H) * 6.0f * 0.00390625f;
    float v4 = floor(v14);
    float v5 = static_cast<float>(255 - S) * 0.0039215689f;
    float v6 = static_cast<float>(V) * 0.0039215689f;
    float v16 = (1.0f - v5) * v6;
    float v10 = (1.0f - (v5 * (v14 - v4))) * v6;
    float v7 = (1.0f - (v14 - v4)) * v5;
    float v15 = static_cast<float>(V) * 0.0039215689f;
    float v17 = (1.0f - v7) * v6;

    switch (static_cast<uint32_t>(v4))
    {
        case 0:
            r = v15;
            g = v17;
            b = v16;
            break;
        case 1:
            r = v10;
            g = v15;
            b = v16;
            break;
        case 2:
            r = v16;
            g = v15;
            b = v17;
            break;
        case 3:
            r = v16;
            g = v10;
            b = v15;
            break;
        case 4:
            r = v17;
            g = v16;
            b = v15;
            break;
        case 5:
            r = v15;
            g = v16;
            b = v10;
            break;
        default:
            break;
    }

    dest[0] = r;
    dest[1] = g;
    dest[2] = b;
    dest[3] = a;

    if (bChangeColor && gBlacklistIndicators)
    {
        const float visibility = gVisibility.load(std::memory_order_relaxed);
        dest[0] *= visibility;
        dest[1] *= visibility;
        dest[2] *= visibility;
        dest[3] *= visibility;
    }

    return dest;
}

void ApplyLightHsv(SafetyHookContext& regs, uintptr_t light, uintptr_t resume)
{
    const auto args = reinterpret_cast<const uint32_t*>(regs.esp);
    auto dest = reinterpret_cast<float*>(args[0]);
    regs.eax = reinterpret_cast<uintptr_t>(FGetHSV(dest, static_cast<uint8_t>(args[1]), static_cast<uint8_t>(args[2]),
        static_cast<uint8_t>(args[3]), reinterpret_cast<const uint8_t*>(light)));
    regs.eip = resume;
}

SafetyHookInline shsub_10C86280 = {};
int __fastcall sub_10C86280(int _this, void* edx, void* a2, int a3)
{
    static bool bOnce = false;

    if (!bOnce)
    {
        auto pDevice = *(IDirect3DDevice9**)(_this + 0x4D3C);
        IDirect3DSwapChain9* pSwapChain = nullptr;
        if (SUCCEEDED(pDevice->GetSwapChain(0, &pSwapChain)))
        {
            D3DPRESENT_PARAMETERS pp;
            if (SUCCEEDED(pSwapChain->GetPresentParameters(&pp)))
            {
                bIsWindowed = pp.Windowed;
                hGameWindow = pp.hDeviceWindow;
            }
            pSwapChain->Release();
        }
        bOnce = true;
    }

    //crashes
    //if (bIsWindowed)
    //{
    //    MSG msg;
    //    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
    //    {
    //        TranslateMessage(&msg);
    //        DispatchMessageA(&msg);
    //    }
    //}

    bPlayingVideo = true;
    auto ret = shsub_10C86280.unsafe_fastcall<int>(_this, edx, a2, a3);
    bPlayingVideo = false;
    return ret;
}

// UWindowsViewport::ResizeViewport set the resolution, in fullscreen or in a window
void OnResolutionChange(int32_t width, int32_t height, bool bFullscreen)
{
    bIsWindowed = !bFullscreen;
    Screen.Width = width;
    Screen.Height = height;
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    Screen.fHUDScaleX = (1.0f / Screen.fWidth * (Screen.fHeight / 480.0f)) * 2.0f;
    dword_1120B6BC = Screen.fHUDScaleX;
    Screen.fTextScaleX = ((4.0f / 3.0f) / Screen.fAspectRatio) * 2.0f;
    Screen.fHudOffset = Screen.fTextScaleX / 2.0f;
    Screen.nHudOffsetReal = static_cast<int32_t>(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
    Screen.nScopeScale = static_cast<int32_t>(Screen.fWidth * (Screen.fAspectRatio / (4.0f / 3.0f)));

    RawInputHandler<>::OnResChange();

    // The resolution is kept for the next start, a borderless window always has the monitor's
    if (bFullscreen || !bBorderless)
    {
        auto& settings = FixSettings();
        auto [DesktopResW, DesktopResH] = GetDesktopRes();
        if (Screen.Width != DesktopResW || Screen.Height != DesktopResH)
        {
            settings.Set("MAIN.ResX", std::to_string(Screen.Width));
            settings.Set("MAIN.ResY", std::to_string(Screen.Height));
        }
        else
        {
            settings.Set("MAIN.ResX", "0");
            settings.Set("MAIN.ResY", "0");
        }

        settings.Save();
    }

    Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f);
    if (Screen.fHudAspectRatioConstraint.has_value())
    {
        float value = Screen.fHudAspectRatioConstraint.value();
        if (value < 0.0f || value > (32.0f / 9.0f))
            Screen.fWidescreenHudOffset = value;
        else
        {
            value = ClampHudAspectRatio(value, Screen.fAspectRatio);
            Screen.fWidescreenHudOffset = -CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f);
        }
    }
}

void BeforePresent(SafetyHookContext& regs)
{
    UpdateXidiProfile();

    if ((CMenusManager::IsMainMenuDisplayed() && !CMenusManager::IsMenuDisplayed(Page::P_Briefing) &&
        !CMenusManager::IsMenuDisplayed(Page::P_LoadOut) && !CMenusManager::IsMenuDisplayed(Page::P_LoadOut2) &&
        !CMenusManager::IsMenuDisplayed(Page::P_CamControl) && !CMenusManager::IsMenuDisplayed(Page::P_Camera) &&
        !CMenusManager::IsMenuDisplayed(Page::P_opticcable) && !CMenusManager::IsMenuDisplayed(Page::P_MenuGameOver) &&
        (bIsInMenu && *bIsInMenu == 0)) || bQuickLoadingShown || bLoadingBarShown)
    {
        IDirect3DDevice9* pD3DDevice = (IDirect3DDevice9*)(regs.eax);

        const int W = Screen.Width;
        const int H = Screen.Height;

        constexpr float TARGET_ASPECT = 4.0f / 3.0f;

        float render_w = (float)W;
        float render_h = (float)H;

        // Fit 4:3 inside current resolution (letterbox or pillarbox)
        if ((float)W / (float)H > TARGET_ASPECT)
        {
            // Current screen is wider, pillarboxes on sides
            render_w = (float)H * TARGET_ASPECT;
        }
        else
        {
            // Current screen is taller or same, letterboxes top/bottom
            render_h = (float)W / TARGET_ASPECT;
        }

        // Center the render area
        const float x = ((float)W - render_w) * 0.5f;
        const float y = ((float)H - render_h) * 0.5f;

        const LONG left = (LONG)floor(x);
        const LONG right = (LONG)ceil(x + render_w);
        const LONG top = (LONG)floor(y);
        const LONG bottom = (LONG)ceil(y + render_h);

        const D3DCOLOR BLACK = D3DCOLOR_XRGB(0, 0, 0);

        // Left pillar
        if (left > 0)
        {
            D3DRECT r{ 0, 0, left, H };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }

        // Right pillar
        if (right < W)
        {
            D3DRECT r{ right, 0, W, H };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }

        // Top letterbox
        if (top > 0)
        {
            D3DRECT r{ 0, 0, W, top };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }

        // Bottom letterbox
        if (bottom < H)
        {
            D3DRECT r{ 0, bottom, W, H };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }

        // One-pixel safety overlap on right/bottom edges
        if (right < W)
        {
            D3DRECT r{ right - 1, 0, right, H };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }
        if (bottom < H)
        {
            D3DRECT r{ 0, bottom - 1, W, bottom };
            pD3DDevice->Clear(1, &r, D3DCLEAR_TARGET, BLACK, 1.0f, 0);
        }
    }
    DeveloperOverlay::Present(reinterpret_cast<IDirect3DDevice9*>(regs.eax));
}

export void InitD3DDrv()
{
    dword_1120B6BC.SetAddress(*hook::pattern("D9 1D ? ? ? ? 0F BF").count(1).get(0).get<float*>(2));
    dword_1120B6B0.SetAddress(*hook::pattern("8B 0D ? ? ? ? 85 C9 74 ? 8B 54 24 04 8B 82").count(1).get(0).get<uint32_t*>(2));
    auto pattern = hook::pattern("89 6B 58 89 7B 5C EB 0E");
    struct GetRes
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<uint32_t*>(regs.ebx + 0x58) = regs.ebp;
            *reinterpret_cast<uint32_t*>(regs.ebx + 0x5C) = regs.edi;
            OnResolutionChange(regs.ebp, regs.edi, true);
        }
    }; injector::MakeInline<GetRes>(pattern.get_first(0), pattern.get_first(6)); //<0x10CC622C, 0x10CC6232>

    pattern = hook::pattern("89 6B 50 89 7B 54 6A 00 68 00 10 00 00");
    struct GetResWindowed
    {
        void operator()(injector::reg_pack& regs)
        {
            *reinterpret_cast<uint32_t*>(regs.ebx + 0x50) = regs.ebp;
            *reinterpret_cast<uint32_t*>(regs.ebx + 0x54) = regs.edi;
            OnResolutionChange(regs.ebp, regs.edi, false);
        }
    }; injector::MakeInline<GetResWindowed>(pattern.get_first(0), pattern.get_first(6)); //<0x10CC623C, 0x10CC6242>

    // The movie quad receives pixel bounds and texture coordinates separately.
    pattern = hook::pattern("52 50 57 E8 ? ? ? ? 68 00 00 80 3F 68 00 00 80 3F 68 00 00 80 3F 68 00 00 80 3F 6A 2E 8B C8");
    static auto MovieFrameHook = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
    {
        auto movie = reinterpret_cast<const uint32_t*>(regs.esi);
        auto renderer = reinterpret_cast<const uint8_t*>(regs.edi);
        auto args = reinterpret_cast<float*>(regs.esp + sizeof(uint32_t));
        CTMovie::Rectangle rectangle{ args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7] };
        static const auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
        if (!CTMovie::Frame(rectangle, float(*reinterpret_cast<const int*>(renderer + 0x4898)),
            float(*reinterpret_cast<const int*>(renderer + 0x489C)), float(movie[0]), float(movie[1]), mode))
            return;

        if (rectangle.left > 0.0f || rectangle.top > 0.0f)
        {
            auto device = *reinterpret_cast<IDirect3DDevice9* const*>(renderer + 0x4D3C);
            if (device)
                device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        }
        args[0] = rectangle.left;
        args[1] = rectangle.top;
        args[2] = rectangle.right;
        args[3] = rectangle.bottom;
        args[4] = rectangle.u0;
        args[5] = rectangle.v0;
        args[6] = rectangle.u1;
        args[7] = rectangle.v1;
    });

    pattern = hook::pattern("8B 44 24 04 81 EC 84 00 00 00 56");
    shsub_10C86280 = safetyhook::create_inline(pattern.get_first(0), sub_10C86280);

    // The game uses a 2048 shadow map by default.
    if (nShadowMapResolution && nShadowMapResolution != 2048)
    {
        auto pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 8B CE FF 92 ? ? ? ? 5F 5E C7 43 ? ? ? ? ? 5B C2 04 00 C7 87");
        injector::WriteMemory(pattern.get_first(1), nShadowMapResolution, true);
        injector::WriteMemory(pattern.get_first(6), nShadowMapResolution, true);

        pattern = hook::pattern("B9 00 01 00 00 89 94 24 DC 00 00 00");
        injector::WriteMemory(pattern.get_first(1), nShadowMapResolution, true);
    }

    if (!bEnableShadowFiltering)
    {
        // Disable angle-dependent filtering. Soft shadows remain less soft at wide resolutions.
        auto pattern = hook::pattern("B9 ? ? ? ? 89 94 24 DC 00 00 00");
        injector::WriteMemory(pattern.get_first(1), 0x10000, true);

        pattern = hook::pattern("BB 9A 99 99 3F");
        injector::WriteMemory<float>(pattern.get_first(1), 0.25f, true);

        pattern = hook::pattern("C7 86 C8 07 00 00 9A 99 99 BE");
        injector::WriteMemory<float>(pattern.get_first(6), -0.15f, true);
    }

    //Goggles Light Color
    if (!gColor.empty() || gBlacklistIndicators || bLightSyncRGB)
    {
        pattern = hook::pattern("33 D2 8A 53 62 8D 4C 24 ? 50 33 C0 8A 43 61 52 50 51 E8 ? ? ? ? 8B 8E ? ? ? ? 8B 11 83 C4 10 6A 01 50 6A 14 51");
        auto first = pattern.get_first<uint8_t>(18);
        static const auto firstResume = reinterpret_cast<uintptr_t>(first) + 5;
        static auto firstHook = safetyhook::create_mid(first, [](SafetyHookContext& regs)
        {
            ApplyLightHsv(regs, regs.ebx, firstResume);
        });
        pattern = hook::pattern("33 D2 8A 55 62 33 C0 8A 45 61 68 FF 00 00 00 8D 4C 24 ? 52 50 51 E8 ? ? ? ? 8B 45 48 83 C4 10");
        auto second = pattern.get_first<uint8_t>(22);
        static const auto secondResume = reinterpret_cast<uintptr_t>(second) + 5;
        static auto secondHook = safetyhook::create_mid(second, [](SafetyHookContext& regs)
        {
            ApplyLightHsv(regs, regs.ebp, secondResume);
        });
        if (!firstHook || !secondHook)
            throw std::runtime_error("Light color hooks unavailable");

        pattern = hook::pattern("66 89 44 24 ? 8B 17");
        struct BlacklistIndicatorsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto ax = *reinterpret_cast<int16_t*>(&regs.eax);
                *reinterpret_cast<int16_t*>(regs.esp + 0xC) = ax;
                auto v = ax - 446;
                if (v <= 10) v = 10;
                const float visibility = static_cast<float>(v) / 134.0f;
                gVisibility.store(gBlacklistIndicators == 2 ? visibility : 1.0f - visibility, std::memory_order_relaxed);
            }
        }; injector::MakeInline<BlacklistIndicatorsHook>(pattern.get_first()); //0x10B66B97

        if (bLightSyncRGB)
        {
            LEDEffects::Inject([]()
            {
                static float fPlayerVisibility = gVisibility.load(std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                auto gVisCmp = static_cast<float>(static_cast<int>(gVisibility.load(std::memory_order_relaxed) * 10.0f)) / 10.0f;
                auto fPlVisCmp = static_cast<float>(static_cast<int>(fPlayerVisibility * 10.0f)) / 10.0f;

                if (fPlVisCmp > gVisCmp)
                    fPlayerVisibility -= 0.05f;
                else if (fPlVisCmp < gVisCmp)
                    fPlayerVisibility += 0.05f;

                fPlayerVisibility = std::clamp(fPlayerVisibility, 0.0f, 1.0f);

                auto [R, G, B] = LEDEffects::RGBtoPercent(1, 255, 1, fPlayerVisibility);

                LEDEffects::SetLighting(R, G, B, false, false, true);
            });
        }
    }

    pattern = hook::pattern("8B 10 55 55 55 55");
    static auto FullscreenPresentHook = safetyhook::create_mid(pattern.get_first(), BeforePresent);

    pattern = hook::pattern("8B 86 3C 4D 00 00 8B 08 55 53 74 ? 8D 54 24 18 52 52 50 FF 51 44");
    static auto WindowedPresentHook = safetyhook::create_mid(pattern.get_first(6), BeforePresent);

}
