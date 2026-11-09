module;

#include <stdafx.h>
#include "Shared/SplinterCell/Window.hpp"
#include "FixSettings.hpp"
#include <minidx8/d3d8.h>
#include <stdexcept>

export module D3DDrv;

import ComVars;
import Shaders;
import Xidi;
import CutsceneFrame;
import MovieFrame;

// Rain splash point sprites use pixel-sized D3D8 constants scaled for 640x480, causing them to shrink at higher resolutions
namespace RainSplashFix
{
    bool   bInitialized   = false;
    float  fOrigC73z = 0.0f, fOrigC75y = 0.0f, fOrigC75z = 0.0f, fOrigC75w = 0.0f;

    // Retail builds c73 on the stack and lazily stores c74/c75 as immediates
    void InitRetail()
    {
        auto pat = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F6 05 ? ? ? ? 02 75 ? 83 0D ? ? ? ? 02 C7 05 ? ? ? ? 00 00 00 41 C7 05 ? ? ? ? 00 40 9C 45 C7 05 ? ? ? ? 00 00 C0 41 C7 05 ? ? ? ? 00 00 F0 41");
        auto pat2 = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 0D ? ? ? ? DC 08 D9 05");
        uint32_t* doneFlag = *pat.get_first<uint32_t*>(2);
        float* c73z = *pat2.get_first<float*>(2);
        RainSplashInitFlags.SetAddress(doneFlag);
        RainSplashSizeScaleC73z.SetAddress(c73z);
        RainSplashSizeScaleC75y.SetAddress(pat.get_first<float>(32));
        RainSplashMaxSizeC75z.SetAddress(pat.get_first<float>(42));
        RainSplashMaxSizeC75w.SetAddress(pat.get_first<float>(52));

        fOrigC73z = RainSplashSizeScaleC73z;
        fOrigC75y = RainSplashSizeScaleC75y;
        fOrigC75z = RainSplashMaxSizeC75z;
        fOrigC75w = RainSplashMaxSizeC75w;

        bInitialized = true;
    }

    void Init()
    {
        // Rain splash constant block (c73–c75)
        auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "6A 01 68 ? ? ? ? 6A 4B 8B 80 94 46 00 00");
        if (pattern.empty()) // retail
        {
            InitRetail();
            return;
        }

        float* c75 = *pattern.get_first<float*>(3);
        float* c73 = c75 - 8; // contiguous float4 block

        RainSplashSizeScaleC73z.SetAddress(&c73[2]);
        RainSplashSizeScaleC75y.SetAddress(&c75[1]);
        RainSplashMaxSizeC75z.SetAddress(&c75[2]);
        RainSplashMaxSizeC75w.SetAddress(&c75[3]);

        fOrigC73z = RainSplashSizeScaleC73z;
        fOrigC75y = RainSplashSizeScaleC75y;
        fOrigC75z = RainSplashMaxSizeC75z;
        fOrigC75w = RainSplashMaxSizeC75w;

        bInitialized = true;
    }

    void Apply()
    {
        if (!bInitialized)
            return;

        // Maintain original 480p vertical scale
        float scale = static_cast<float>(Screen.Height) / 480.0f;
        if (!(scale > 0.0f))
            scale = 1.0f;

        RainSplashSizeScaleC73z = fOrigC73z * scale;
        RainSplashSizeScaleC75y = fOrigC75y * scale;
        RainSplashMaxSizeC75z = fOrigC75z * scale;
        RainSplashMaxSizeC75w = fOrigC75w * scale;

        if (RainSplashInitFlags.is_initialized()) // retail: re-run the patched lazy initialiser
            RainSplashInitFlags &= ~2u;
    }
}

SafetyHookInline shUD3DRenderDeviceSetRes = {};
int __fastcall UD3DRenderDeviceSetRes(void* UD3DRenderDevice, void* edx, void* UViewport, int width, int height, int a5)
{
    static const int mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    static bool first = true;
    const bool startup = std::exchange(first, false);
    if (startup)
        a5 = mode == 0;
    const auto nativeResolution = RequestedNativeResolution;
    const bool startupResolution = startup && !nativeResolution && mode != 2;
    if (nativeResolution)
    {
        width = nativeResolution->first;
        height = nativeResolution->second;
    }
    else if (startupResolution)
    {
        width = Screen.Width;
        height = Screen.Height;
    }
    if (!a5 && mode == 2)
    {
        auto area = SCWindow::Monitor(PresentParams->hDeviceWindow).rcMonitor;
        width = area.right - area.left;
        height = area.bottom - area.top;
    }

    bool bInvalidRes = !nativeResolution && !startupResolution;
    std::wstring resStr = std::to_wstring(width) + L"x" + std::to_wstring(height);
    for (const auto& it : ResList)
    {
        if (!nativeResolution && !startupResolution && resStr == it.first)
        {
            swscanf_s(it.second.c_str(), L"%dx%d", &width, &height);
            bInvalidRes = false;
            break;
        }
    }

    if (bInvalidRes && (a5 || mode != 2))
    {
        width = Screen.Width;
        height = Screen.Height;
        bInvalidRes = false;
    }

    auto ret = shUD3DRenderDeviceSetRes.unsafe_fastcall<int>(UD3DRenderDevice, edx, UViewport, width, height, a5);
    if (!ret)
        return ret;

    Screen.Width = PresentParams->BackBufferWidth;
    Screen.Height = PresentParams->BackBufferHeight;
    if (nativeResolution && UViewport)
    {
        const int actualWidth = *reinterpret_cast<int*>(static_cast<uint8_t*>(UViewport) + 0x80);
        const int actualHeight = *reinterpret_cast<int*>(static_cast<uint8_t*>(UViewport) + 0x84);
        if (actualWidth > 0 && actualHeight > 0 && actualWidth <= 16384 && actualHeight <= 16384)
        {
            Screen.Width = actualWidth;
            Screen.Height = actualHeight;
        }
        FixSettings().Set("MAIN.ResX", std::to_string(Screen.Width), true);
        FixSettings().Set("MAIN.ResY", std::to_string(Screen.Height), true);
    }
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    // Rescale the rain splash point sprites for the new resolution
    RainSplashFix::Apply();

    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.dHUDScaleX = static_cast<double>(Screen.fHUDScaleX);
    Screen.dHUDScaleXInv = 1.0 / Screen.dHUDScaleX;

    // How far the HUD's side columns move out from the 4:3 middle to the edges of the HUD box
    auto computeHudOffset = [&](float width, float height) -> float
    {
        return -CalculateWidescreenOffset(width, height, 640.0f, 480.0f);
    };

    Screen.fWidescreenHudOffset = computeHudOffset(Screen.fWidth, Screen.fHeight);

    if (Screen.fHudAspectRatioConstraint.has_value())
    {
        float value = Screen.fHudAspectRatioConstraint.value();
        if (value < 0.0f || value > (32.0f / 9.0f))
        {
            Screen.fWidescreenHudOffset = value;
        }
        else
        {
            value = ClampHudAspectRatio(value, Screen.fAspectRatio);
            Screen.fWidescreenHudOffset = computeHudOffset(Screen.fHeight * value, Screen.fHeight);
        }
    }

    constexpr float targetAspect = 4.0f / 3.0f;
    const float videoWidth = std::min(Screen.fWidth, Screen.fHeight * targetAspect);
    const float videoHeight = std::min(Screen.fHeight, Screen.fWidth / targetAspect);
    Screen.fFMVoffsetStartX = (Screen.fWidth - videoWidth) * 0.5f;
    Screen.fFMVoffsetEndX = Screen.fFMVoffsetStartX + videoWidth;
    Screen.fFMVoffsetStartY = (Screen.fHeight - videoHeight) * 0.5f;
    Screen.fFMVoffsetEndY = Screen.fFMVoffsetStartY + videoHeight;

    SCWindow::Track(PresentParams->hDeviceWindow, mode, PresentParams->Windowed != FALSE, Screen.Width, Screen.Height);
    SCWindow::SquareCorners(PresentParams->hDeviceWindow);

    if (Screen.Width < 640 || Screen.Height < 480)
        return ret;

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

    bIsWindowed = PresentParams->Windowed ? true : false;

    if (bIsWindowed)
    {
        SCWindow::Place(PresentParams->hDeviceWindow, Screen.Width, Screen.Height, mode == 2);
        SetForegroundWindow(PresentParams->hDeviceWindow);
        SetCursor(NULL);
    }

    return ret;
}

SafetyHookInline shsub_1009D270 = {};
void __fastcall sub_1009D270(void* FRenderInterface, void* edx, int a2, int a3, float a4, float a5, float a6, float a7, int* a8)
{
    a4 *= Screen.nBloomResolutionMultiplier;
    a5 *= Screen.nBloomResolutionMultiplier;
    a6 *= Screen.nBloomResolutionMultiplier;
    a7 *= Screen.nBloomResolutionMultiplier;
    return shsub_1009D270.unsafe_fastcall(FRenderInterface, edx, a2, a3, a4, a5, a6, a7, a8);
}

SafetyHookInline shsub_100A7850 = {};
void __fastcall sub_100A7850(void* FRenderInterface, void* edx, int a2, int a3, int a4, int a5)
{
    a4 *= Screen.nBloomResolutionMultiplier;
    a5 *= Screen.nBloomResolutionMultiplier;
    return shsub_100A7850.unsafe_fastcall(FRenderInterface, edx, a2, a3, a4, a5);
}

namespace UD3DRenderDevice
{
    SafetyHookInline shDisplayVideo = {};
    void __fastcall DisplayVideo(void* UD3DRenderDevice, void* edx, void* UCanvas, void* a3)
    {
        if (bIsWindowed)
        {
            MSG msg;
            while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }

        auto frame = PTMovie::Layout(Screen.Width, Screen.Height, 4, 3, 0);
        const auto bink = *reinterpret_cast<const int**>(reinterpret_cast<uintptr_t>(UCanvas) + 0x98);
        if (bink)
        {
            const auto decoded = PTMovie::Layout(Screen.Width, Screen.Height, bink[0], bink[1],
                FixSettings().Int("MAIN.FMVWidescreenMode"));
            if (decoded.width > 0.0f && decoded.height > 0.0f)
                frame = decoded;
        }
        Screen.fFMVoffsetStartX = frame.x;
        Screen.fFMVoffsetEndX = frame.x + frame.width;
        Screen.fFMVoffsetStartY = frame.y;
        Screen.fFMVoffsetEndY = frame.y + frame.height;
        shDisplayVideo.unsafe_fastcall(UD3DRenderDevice, edx, UCanvas, a3);

        IDirect3DDevice8* pD3DDevice = *(IDirect3DDevice8**)((uintptr_t)UD3DRenderDevice + 0x4694);
        if (!pD3DDevice)
            return;
        const int W = Screen.Width;
        const int H = Screen.Height;
        const float targetX = Screen.fFMVoffsetStartX;
        const float targetY = Screen.fFMVoffsetStartY;
        const float targetW = Screen.fFMVoffsetEndX - Screen.fFMVoffsetStartX;
        const float targetH = Screen.fFMVoffsetEndY - Screen.fFMVoffsetStartY;

        // Clear pillar/letterbox areas
        if (targetX > 0.0f)
        {
            D3DRECT leftRect = { 0, 0, (LONG)targetX, H };
            pD3DDevice->Clear(1, &leftRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        }
        if (targetX + targetW < (float)W)
        {
            D3DRECT rightRect = { (LONG)(targetX + targetW), 0, W, H };
            pD3DDevice->Clear(1, &rightRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        }
        if (targetY > 0.0f)
        {
            D3DRECT topRect = { 0, 0, W, (LONG)targetY };
            pD3DDevice->Clear(1, &topRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        }
        if (targetY + targetH < (float)H)
        {
            D3DRECT bottomRect = { 0, (LONG)(targetY + targetH), W, H };
            pD3DDevice->Clear(1, &bottomRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        }
    }
}

export void InitD3DDrv()
{
    auto pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? 33 C0 8B D9 C1 E9 02 83 E3 03", "68 ? ? ? ? FF 15 ? ? ? ? 8B 8D");
    PresentParams.SetAddress(*pattern.get_first<D3DPRESENT_PARAMETERS*>(1));

    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 81 EC ? ? ? ? 53 56 57 8B 7D ? 8B F1", "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 ? 56 57 50 8D 45 ? 64 A3 ? ? ? ? 8B F1 89 75");
    shUD3DRenderDeviceSetRes = safetyhook::create_inline(pattern.get_first(), UD3DRenderDeviceSetRes);

    // UseVSync only sets the fullscreen present interval; a window copies without waiting, so give it the vsync copy
    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "83 BE ? ? ? ? 00 0F 45 C1", "8B 96 ? ? ? ? F7 DA 1B D2 81 E2 01 00 00 80");
    static const uint32_t nUseVSyncOffset = *pattern.get_first<uint32_t>(2);
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 86 90 46 00 00 6A 16 6A 01 6A 01 57 6A 01");
    static auto SetResVSyncHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        const bool bUseVSync = *reinterpret_cast<const uint32_t*>(regs.esi + nUseVSyncOffset) != 0;
        if (PresentParams->Windowed && bUseVSync)
            PresentParams->SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;
    });

    auto rendererLayout = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "83 EC ? 53 55 56 8B F1 8B 86");
    bool retailLayout = true;
    if (!rendererLayout.empty()) // retail
        rendererLayout.get_first();
    else // digital
    {
        rendererLayout = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "55 8B EC 83 EC ? 53 56 8B F1 57");
        rendererLayout.get_first();
        retailLayout = false;
    }

    // Water Blend Fix
    static DWORD OriginalAlphaBlendEnable = 0;
    static DWORD OriginalSrcBlend = 0;
    static DWORD OriginalDestBlend = 0;

    {
        static DWORD WaterBlendPS = 0;
        static thread_local bool bWaterFixApplied = false;
        auto CheckWaterBlendState = [](IDirect3DDevice8* dev) -> bool
        {
            DWORD ttf0 = 0, ttf1 = 0, ttf2 = 0;
            DWORD c0 = 0, ca1_0 = 0, ca2_0 = 0, a0 = 0, aa1_0 = 0, aa2_0 = 0;
            DWORD c1 = 0, ca1_1 = 0, ca2_1 = 0, a1 = 0, aa1_1 = 0, aa2_1 = 0;
            DWORD c2 = 0, ca1_2 = 0, ca2_2 = 0, a2 = 0, aa1_2 = 0, aa2_2 = 0;

            dev->GetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, &ttf0);
            dev->GetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, &ttf1);
            dev->GetTextureStageState(2, D3DTSS_TEXTURETRANSFORMFLAGS, &ttf2);

            dev->GetTextureStageState(0, D3DTSS_COLOROP, &c0);
            dev->GetTextureStageState(0, D3DTSS_COLORARG1, &ca1_0);
            dev->GetTextureStageState(0, D3DTSS_COLORARG2, &ca2_0);
            dev->GetTextureStageState(0, D3DTSS_ALPHAOP, &a0);
            dev->GetTextureStageState(0, D3DTSS_ALPHAARG1, &aa1_0);
            dev->GetTextureStageState(0, D3DTSS_ALPHAARG2, &aa2_0);

            dev->GetTextureStageState(1, D3DTSS_COLOROP, &c1);
            dev->GetTextureStageState(1, D3DTSS_COLORARG1, &ca1_1);
            dev->GetTextureStageState(1, D3DTSS_COLORARG2, &ca2_1);
            dev->GetTextureStageState(1, D3DTSS_ALPHAOP, &a1);
            dev->GetTextureStageState(1, D3DTSS_ALPHAARG1, &aa1_1);
            dev->GetTextureStageState(1, D3DTSS_ALPHAARG2, &aa2_1);

            dev->GetTextureStageState(2, D3DTSS_COLOROP, &c2);
            dev->GetTextureStageState(2, D3DTSS_COLORARG1, &ca1_2);
            dev->GetTextureStageState(2, D3DTSS_COLORARG2, &ca2_2);
            dev->GetTextureStageState(2, D3DTSS_ALPHAOP, &a2);
            dev->GetTextureStageState(2, D3DTSS_ALPHAARG1, &aa1_2);
            dev->GetTextureStageState(2, D3DTSS_ALPHAARG2, &aa2_2);

            const bool s0 =
                (ttf0 == D3DTTFF_DISABLE) &&
                (c0 == D3DTOP_SELECTARG1) && (a0 == D3DTOP_SELECTARG1) &&
                (ca1_0 == D3DTA_TEXTURE) && (aa1_0 == D3DTA_TEXTURE);

            const bool s1 =
                (ttf1 & D3DTTFF_PROJECTED) &&
                (c1 == D3DTOP_BLENDCURRENTALPHA) && (a1 == D3DTOP_SELECTARG1) &&
                (ca1_1 == D3DTA_TEXTURE) && (ca2_1 == D3DTA_CURRENT) &&
                (aa1_1 == D3DTA_CURRENT);

            const bool s2 =
                (ttf2 == D3DTTFF_DISABLE) &&
                (c2 == D3DTOP_SELECTARG1) && (a2 == D3DTOP_SELECTARG1) &&
                (ca1_2 == D3DTA_CURRENT) && (aa1_2 == D3DTA_TEXTURE);

            if (!(s0 && s1 && s2))
                return false;

            return true;
        };

        static auto WaterPreDraw = [&](SafetyHookContext& regs, IDirect3DDevice8* pDevice)
        {
            bWaterFixApplied = false;

            if (!WaterBlendPS)
            {
                HMODULE hModule;
                GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&InitD3DDrv, &hModule);
                HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR_WATER_BLEND), RT_RCDATA);
                if (hResource)
                {
                    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
                    if (hLoadedResource)
                    {
                        LPVOID pLockedResource = LockResource(hLoadedResource);
                        DWORD dwResourceSize = SizeofResource(hModule, hResource);
                        if (pLockedResource && dwResourceSize > 0)
                        {
                            if (FAILED(pDevice->CreatePixelShader((DWORD*)pLockedResource, &WaterBlendPS)))
                                WaterBlendPS = 0;
                        }
                    }
                }
            }

            if (!WaterBlendPS)
                return;

            // Skip if a real pixel shader is already bound
            DWORD currentPS = 0;
            pDevice->GetPixelShader(&currentPS);
            if (currentPS)
                return;

            if (!CheckWaterBlendState(pDevice))
                return;

            pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &OriginalAlphaBlendEnable);
            pDevice->GetRenderState(D3DRS_SRCBLEND, &OriginalSrcBlend);
            pDevice->GetRenderState(D3DRS_DESTBLEND, &OriginalDestBlend);

            pDevice->SetPixelShader(WaterBlendPS);
            pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

            bWaterFixApplied = true;
        };

        static auto WaterPostDraw = [](SafetyHookContext& regs, IDirect3DDevice8* pDevice)
        {
            if (bWaterFixApplied)
            {
                pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, OriginalAlphaBlendEnable);
                pDevice->SetRenderState(D3DRS_SRCBLEND, OriginalSrcBlend);
                pDevice->SetRenderState(D3DRS_DESTBLEND, OriginalDestBlend);
                pDevice->SetPixelShader(0);
                bWaterFixApplied = false;
            }
        };

        if (!retailLayout)
        {
            pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "FF B5 ? ? ? ? 8B 07 6A ? FF B5");
            static auto WaterPreDrawPrimitiveHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                IDirect3DDevice8* pDevice = (IDirect3DDevice8*)(regs.edi);
                WaterPreDraw(regs, pDevice);
            });

            pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "F3 0F 10 9D ? ? ? ? 8B 95 ? ? ? ? F3 0F 10 25");
            static auto WaterPostDrawPrimitiveHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                IDirect3DDevice8* pDevice = (IDirect3DDevice8*)(regs.edi);
                WaterPostDraw(regs, pDevice);
            });

            pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "FF B5 ? ? ? ? 8B 06 6A ? FF B5 ? ? ? ? 6A ? 6A ? 56 FF 90 ? ? ? ? F3 0F 10 9D");
            static auto WaterPreDrawPrimitiveHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                IDirect3DDevice8* pDevice = (IDirect3DDevice8*)(regs.esi);
                WaterPreDraw(regs, pDevice);
            });

            pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "F3 0F 10 9D ? ? ? ? 8B 95 ? ? ? ? 8B 8F");
            static auto WaterPostDrawPrimitiveHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                IDirect3DDevice8* pDevice = (IDirect3DDevice8*)(regs.esi);
                WaterPostDraw(regs, pDevice);
            });
        }
        else
        {
            // retail: device is in EBP at all four sites
            pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B 4C 24 ? 8B 45 00 51 6A 00 57 6A 00 6A 05 55 FF 90 1C 01 00 00 8B 83 EC 03 00 00");
            {
                static auto WaterPreDrawPrimitiveHook1Retail = safetyhook::create_mid(pattern.get_first(7), [](SafetyHookContext& regs)
                {
                    IDirect3DDevice8* pDevice = (IDirect3DDevice8*)(regs.ebp);
                    WaterPreDraw(regs, pDevice);
                });

                static auto WaterPostDrawPrimitiveHook1Retail = safetyhook::create_mid(pattern.get_first(22), [](SafetyHookContext& regs)
                {
                    IDirect3DDevice8* pDevice = (IDirect3DDevice8*)(regs.ebp);
                    WaterPostDraw(regs, pDevice);
                });
            }

            pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B 44 24 ? 8B 4C 24 ? 8B 55 00 50 6A 00 51 6A 00 6A 05 55 FF 92 1C 01 00 00 8B 8B EC 03 00 00");
            {
                static auto WaterPreDrawPrimitiveHook2Retail = safetyhook::create_mid(pattern.get_first(11), [](SafetyHookContext& regs)
                {
                    IDirect3DDevice8* pDevice = (IDirect3DDevice8*)(regs.ebp);
                    WaterPreDraw(regs, pDevice);
                });

                static auto WaterPostDrawPrimitiveHook2Retail = safetyhook::create_mid(pattern.get_first(26), [](SafetyHookContext& regs)
                {
                    IDirect3DDevice8* pDevice = (IDirect3DDevice8*)(regs.ebp);
                    WaterPostDraw(regs, pDevice);
                });
            }
        }

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 50 8B 08 FF 51 ? 8B 07", "68 ? ? ? ? 50 FF 51 38 33 C9 3B C1");
        {
            static auto DeviceResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                IDirect3DDevice8* pDevice = *(IDirect3DDevice8**)(regs.edi);
                if (WaterBlendPS)
                {
                    pDevice->DeletePixelShader(WaterBlendPS);
                    WaterBlendPS = 0;
                }
            });
        }
    }

    //FMV
    UD3DRenderDevice::shDisplayVideo = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?DisplayVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@PAX@Z"), UD3DRenderDevice::DisplayVideo);

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "D9 1C 24 56 56 FF 15");
    if (!pattern.empty())
    {
        injector::WriteMemory<uint8_t>(pattern.get_first(-4), 0x56, true); //push esi
        struct DisplayVideo_Hook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x04) = static_cast<float>(*(int32_t*)(regs.esp + 0x68 + 0x4));

                *(float*)&regs.esi = Screen.fFMVoffsetStartX;
                *(float*)(regs.esp + 0x04) = Screen.fFMVoffsetEndX;
                *(float*)(regs.esp + 0x00) = Screen.fFMVoffsetStartY;
                *(float*)(regs.esp + 0x08) = Screen.fFMVoffsetEndY;
            }
        }; injector::MakeInline<DisplayVideo_Hook>(pattern.get_first(-3), pattern.get_first(4)); //pfDisplayVideo + 0x37E
    }
    else
    {
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 04 24 ? ? ? ? FF 15 ? ? ? ? 8D 8D ? ? ? ? FF 15 ? ? ? ? 8B 8E");
        struct DisplayVideo_Hook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x00) = Screen.fFMVoffsetStartX;
                *(float*)(regs.esp + 0x08) = Screen.fFMVoffsetEndX;
                *(float*)(regs.esp + 0x04) = Screen.fFMVoffsetStartY;
                *(float*)(regs.esp + 0x0C) = Screen.fFMVoffsetEndY;
            }
        }; injector::MakeInline<DisplayVideo_Hook>(pattern.get_first(0), pattern.get_first(7));
    }

    if (Screen.nPostProcessFixedScale > 0)
    {
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 00 02 00 00 68 00 02 00 00");
        if (pattern.size() != (retailLayout ? 7 : 8))
            throw std::runtime_error("Post-process signatures not found");
        for (size_t i = 0; i < pattern.size(); i++)
        {
            if (pattern.size() == 8 && i == 2)
                continue;

            injector::WriteMemory(pattern.get(i).get<uint32_t>(1), Screen.nPostProcessFixedScale, true); //affects glass reflections
            injector::WriteMemory(pattern.get(i).get<uint32_t>(6), Screen.nPostProcessFixedScale, true);
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 00 02 00 00");
        if (!pattern.empty())
            injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(1), Screen.nPostProcessFixedScale, true);
        else
        {
            pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 80 ? ? ? ? ? ? ? ? 56");
            injector::WriteMemory(pattern.get_first(6), Screen.nPostProcessFixedScale, true);

            pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 80 ? ? ? ? ? ? ? ? 8B 45 ? 6A");
            injector::WriteMemory(pattern.get_first(6), Screen.nPostProcessFixedScale, true);
        }
    }

    if (Screen.nShadowMapResolution > 0)
    {
        if (Screen.nShadowMapResolution == 1)
            Screen.nShadowMapResolution = std::clamp(Screen.Width, 0, retailLayout ? 2048 : 3072);

        auto rpattern = hook::range_pattern((uint32_t)rendererLayout.get_first(), (uint32_t)rendererLayout.get_first() + 0x488, "6A ? 68 ? ? ? ? 68 ? ? ? ?");
        if (rpattern.size() != 9)
            throw std::runtime_error("Shadow-map signatures not found");
        for (size_t i = 0; i < rpattern.size(); i++)
        {
            injector::WriteMemory(rpattern.get(i).get<uint32_t>(3), Screen.nShadowMapResolution, true);
            injector::WriteMemory(rpattern.get(i).get<uint32_t>(8), Screen.nShadowMapResolution, true);
        }
        if (!retailLayout)
        {
            auto last = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "57 6A 00 6A 50 8B 01 68 ? ? ? ? 68 ? ? ? ? 51 FF 50 68 85 C0 78 14");
            injector::WriteMemory(last.get_first(8), Screen.nShadowMapResolution, true);
            injector::WriteMemory(last.get_first(13), Screen.nShadowMapResolution, true);
        }
    }

    if (Screen.nReflectionsResolution > 0)
    {
        if (Screen.nReflectionsResolution == 1)
            Screen.nReflectionsResolution = std::clamp(Screen.Width, 0, retailLayout ? 2048 : 3072);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? C7 04 81 ? ? ? ? 8B 07 8B 10 68 ? ? ? ? 50 FF 52 ? 6A ? 6A ? 8B CE FF 15 ? ? ? ? 8B 0E 68");
        if (!pattern.empty())
        {
            injector::WriteMemory(pattern.get_first(1), Screen.nReflectionsResolution, true);
            injector::WriteMemory(pattern.get_first(17), Screen.nReflectionsResolution, true);

            pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? C7 04 81 ? ? ? ? 8B 07 8B 10 68 ? ? ? ? 50 FF 52 ? 6A ? 6A ? 8B CE FF 15 ? ? ? ? 8B 0E C7 04 81");
            injector::WriteMemory(pattern.get_first(1), Screen.nReflectionsResolution, true);
            injector::WriteMemory(pattern.get_first(17), Screen.nReflectionsResolution, true);
        }
        else
        {
            auto first = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 04 81 ? ? ? ? 8B 06 6A 01 68 ? ? ? ? 68 ? ? ? ? 8B 08 50 FF 51 50 6A 04");
            auto second = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 04 81 ? ? ? ? 8B 06 68 ? ? ? ? 68 ? ? ? ? 50 8B 08 FF 51 68 6A 04");
            injector::WriteMemory(first.get_first(12), Screen.nReflectionsResolution, true);
            injector::WriteMemory(first.get_first(17), Screen.nReflectionsResolution, true);
            injector::WriteMemory(second.get_first(10), Screen.nReflectionsResolution, true);
            injector::WriteMemory(second.get_first(15), Screen.nReflectionsResolution, true);
        }

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 8B CF", "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? FF 50 ? 8B 07");
        {
            injector::WriteMemory(pattern.get_first(1), Screen.nReflectionsResolution, true);
            injector::WriteMemory(pattern.get_first(6), Screen.nReflectionsResolution, true);
        }

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 8B CB FF 50", "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 8B CB FF 52 ? 68 ? ? ? ? 68 ? ? ? ? 53 8D 4C 24 ? FF 15 ? ? ? ? 8B 45 ? 85 C0");
        {
            injector::WriteMemory(pattern.get_first(1), Screen.nReflectionsResolution, true);
            injector::WriteMemory(pattern.get_first(6), Screen.nReflectionsResolution, true);
        }
    }

    if (Screen.nBloomResolutionMultiplier > 0)
    {
        struct BloomSite
        {
            const char* signature;
            int offset;
            int size;
        };
        const BloomSite digital[] =
        {
            { "8D 96 E8 58 00 00 8B 86 94 46 00 00 52 6A 00 6A 15 8B 08 6A 01 6A 01 68 ? ? ? ? 68 ? ? ? ? 50 FF 51 50", 23, 256 },
            { "8D 96 EC 58 00 00 52 6A 00 6A 15 8B 08 6A 01 6A 01 68 ? ? ? ? 68 ? ? ? ? 50 FF 51 50", 17, 128 },
            { "8D BA 7C 58 00 00 57 6A 00 6A 15 8B 08 6A 01 6A 01 68 ? ? ? ? 68 ? ? ? ? 50 FF 51 50", 17, 256 },
            { "81 C6 80 58 00 00 56 6A 00 6A 4B 8B 00 68 ? ? ? ? 68 ? ? ? ? 50 8B 08 FF 51 68", 13, 256 },
        };
        const BloomSite retail[] =
        {
            { "8D 8B E4 58 00 00 51 6A 00 6A 15 6A 01 6A 01 68 ? ? ? ? 68 ? ? ? ? 50 FF 52 50", 15, 256 },
            { "8D 8B E8 58 00 00 51 6A 00 6A 15 6A 01 6A 01 68 ? ? ? ? 68 ? ? ? ? 50 FF 52 50", 15, 128 },
            { "8D 8B 78 58 00 00 51 6A 00 6A 15 6A 01 6A 01 68 ? ? ? ? 68 ? ? ? ? 50 FF 52 50", 15, 256 },
            { "8D 8B 7C 58 00 00 51 6A 00 6A 4B 68 ? ? ? ? 68 ? ? ? ? 50 FF 52 68", 11, 256 },
        };
        auto first = hook::module_pattern(GetModuleHandle(L"D3DDrv"), digital[0].signature);
        const auto& sites = first.empty() ? retail : digital; // retail / digital
        for (const auto& site : sites)
        {
            auto bloom = hook::module_pattern(GetModuleHandle(L"D3DDrv"), site.signature);
            const int size = site.size * Screen.nBloomResolutionMultiplier;
            injector::WriteMemory(bloom.get_first(site.offset + 1), size, true);
            injector::WriteMemory(bloom.get_first(site.offset + 6), size, true);
        }

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "50 FF 51 ? 8B 45 ? 81 C6 ? ? ? ? 56 6A ? 6A", "50 FF 52 ? 8B 07 8B 10 8D 8B ? ? ? ? 51 6A ? 6A ? 6A ? 6A ? 6A 20 6A 20 50");
        static auto ResHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esp + 0x0) *= Screen.nBloomResolutionMultiplier;
            *(uint32_t*)(regs.esp + 0x4) *= Screen.nBloomResolutionMultiplier;
        });

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "50 FF 51 ? 8B 4D ? 6A", "50 FF 52 50 6A ? 6A ? 8B CE FF 15 ? ? ? ? 8B 16 8D 8B E4 58");
        static auto ResHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esp + 0x0) *= Screen.nBloomResolutionMultiplier;
            *(uint32_t*)(regs.esp + 0x4) *= Screen.nBloomResolutionMultiplier;
        });

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 ? 53 56 57 50 8D 45 ? 64 A3 ? ? ? ? 8B D9 89 9D ? ? ? ? F3 0F 10 45", "6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 81 EC ? ? ? ? 53 55 56 8B B4 24 ? ? ? ? 33 DB 3B F3 8B E9 57");
        shsub_1009D270 = safetyhook::create_inline(pattern.get_first(), sub_1009D270);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "55 8B EC 83 EC ? A1 ? ? ? ? 33 C5 89 45 ? 8B 45 ? 8D 55", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 24 8B 45 10 53 8B 5D 0C 56 57 8B 7D 08 8B F1");
        shsub_100A7850 = safetyhook::create_inline(pattern.get_first(), sub_100A7850);
    }

    // Flash afterimage is captured into a hardcoded 256x256 buffer and stretched over the screen. It shares the main
    // depth stencil, so it has to match the backbuffer exactly or SetRenderTarget fails and the effect never draws.
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "6A 17 8B 00 6A 01 6A 01 68 00 01 00 00 8B 08 68 00 01 00 00");
    const bool bDigitalLayout = !pattern.empty();
    if (!bDigitalLayout) // retail
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "6A 17 6A 01 6A 01 68 00 01 00 00 68 00 01 00 00");
    {
        injector::WriteMemory(pattern.get_first(bDigitalLayout ? 9 : 7), Screen.Height, true);
        injector::WriteMemory(pattern.get_first(bDigitalLayout ? 16 : 12), Screen.Width, true);
    }

    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B 10 50 FF 92 ? ? ? ? 8B 9E 28 58 00 00", "8B 08 FF 91 ? ? ? ? FF 83 2C 58 00 00");
    static auto EndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UpdateXidiProfile();

        // The cinematics' side bars (see CutsceneFrame.hpp); the menus are a HUD state of their own
        if (!bPlayingVideo && !bDisplayingBackground && UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic")
        {
            if (const int bar = PTCutscene::PillarboxWidth(Screen.Width, Screen.Height))
            {
                auto device = reinterpret_cast<IDirect3DDevice8*>(regs.eax);
                D3DVIEWPORT8 saved{};
                if (device && SUCCEEDED(device->GetViewport(&saved)))
                {
                    D3DVIEWPORT8 full{ 0, 0, DWORD(Screen.Width), DWORD(Screen.Height), 0.0f, 1.0f };
                    D3DRECT bars[2] = { { 0, 0, bar, Screen.Height }, { Screen.Width - bar, 0, Screen.Width, Screen.Height } };
                    device->SetViewport(&full);
                    device->Clear(2, bars, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                    device->SetViewport(&saved);
                }
            }
        }

        if (bDisplayingBackground)
        {
            IDirect3DDevice8* pD3DDevice = (IDirect3DDevice8*)(regs.eax);

            int W = Screen.Width;
            int H = Screen.Height;

            // Calculate pillar/letterbox dimensions based on 4:3 aspect ratio
            float expectedWidth = (float)H * (4.0f / 3.0f);
            float expectedHeight = (float)W * (3.0f / 4.0f);

            // Determine which bars to clear
            float targetW, targetH, targetX, targetY;

            if ((float)W / (float)H > (4.0f / 3.0f))
            {
                // Pillarbox (wider than 4:3)
                targetH = (float)H;
                targetW = expectedWidth;
                targetX = ((float)W - targetW) / 2.0f;
                targetY = 0.0f;
            }
            else
            {
                // Letterbox (taller than 4:3)
                targetW = (float)W;
                targetH = expectedHeight;
                targetX = 0.0f;
                targetY = ((float)H - targetH) / 2.0f;
            }

            // Clear left pillar area
            if (targetX > 0.0f)
            {
                D3DRECT leftRect = { 0, 0, (LONG)ceil(targetX), H };
                pD3DDevice->Clear(1, &leftRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            }

            // Clear right pillar area
            if (targetX + targetW < (float)W)
            {
                D3DRECT rightRect = { (LONG)floor(targetX + targetW), 0, W, H };
                pD3DDevice->Clear(1, &rightRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

                // Clear additional 1px
                D3DRECT rightEdge = { (LONG)(targetX + targetW) - 1, 0, (LONG)(targetX + targetW), H };
                pD3DDevice->Clear(1, &rightEdge, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            }

            // Clear top letterbox area
            if (targetY > 0.0f)
            {
                D3DRECT topRect = { 0, 0, W, (LONG)ceil(targetY) };
                pD3DDevice->Clear(1, &topRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            }

            // Clear bottom letterbox area
            if (targetY + targetH < (float)H)
            {
                D3DRECT bottomRect = { 0, (LONG)floor(targetY + targetH), W, H };
                pD3DDevice->Clear(1, &bottomRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            }
        }
    });

    // EOceanInfo "Ocean2" culling fix
    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "3D 2C 01 00 00 77 43 85 C0 0F 84 ? ? ? ? 6A 08 6A 01", "F9 2C 01 00 00 77 ? 85 C9 0F 86");
    injector::WriteMemory<uint8_t>(pattern.get_first(5), 0xEB, true); // ja -> jmp

    RainSplashFix::Init();
    RainSplashFix::Apply();

    // Fix night vision grain scaling at high resolutions
    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "F3 0F 5E C8 F3 0F 11 8D 18 FA FF FF FF D0 66 0F 6E 8D 14 FA FF FF 0F 5B C9 66 0F 6E C0 0F 5B C0 F3 0F 5E C8 F3 0F 11 8D 04 FA FF FF");
    if (!pattern.empty())
    {
        static auto NightVisionGrainHook = safetyhook::create_mid(pattern.get_first(0x2C), [](SafetyHookContext& regs)
            {
                if (Screen.fGrainScale == 0.0f)
                    return;

                float* pGrainScaleU = reinterpret_cast<float*>(regs.ebp - 0x5E8);
                float* pGrainScaleV = reinterpret_cast<float*>(regs.ebp - 0x5FC);
                float  renderWidth = static_cast<float>(*reinterpret_cast<int32_t*>(regs.ebp - 0x5F0));
                float  renderHeight = static_cast<float>(*reinterpret_cast<int32_t*>(regs.ebp - 0x5EC));
                if (renderWidth > 0.0f && renderHeight > 0.0f)
                {
                    *pGrainScaleU *= 1.0f + (640.0f / renderWidth  - 1.0f) * Screen.fGrainScale;
                    *pGrainScaleV *= 1.0f + (480.0f / renderHeight - 1.0f) * Screen.fGrainScale;
                }
            });
    }
    else
    {
        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "DB 45 08 8B 4C 24 1C 8B 11 D9 5C 24 14 FF 52 68 89 44 24 18 DB 44 24 18 8B 4C 24 1C 8B 01 D8 7C 24 14 D9 5C 24 18 DB 45 0C D9 5C 24 20 FF 50 6C 89 44 24 1C DB 44 24 1C D8 7C 24 20 D9 5C 24 1C");
        {
            static auto NightVisionGrainHookRetail = safetyhook::create_mid(pattern.get_first(0x40), [](SafetyHookContext& regs)
                {
                    if (Screen.fGrainScale == 0.0f)
                        return;

                    float* pGrainScaleU = reinterpret_cast<float*>(regs.esp + 0x18);
                    float* pGrainScaleV = reinterpret_cast<float*>(regs.esp + 0x1C);
                    float  renderWidth = static_cast<float>(*reinterpret_cast<int32_t*>(regs.ebp + 0x8));
                    float  renderHeight = static_cast<float>(*reinterpret_cast<int32_t*>(regs.ebp + 0xC));
                    if (renderWidth > 0.0f && renderHeight > 0.0f)
                    {
                        *pGrainScaleU *= 1.0f + (640.0f / renderWidth  - 1.0f) * Screen.fGrainScale;
                        *pGrainScaleV *= 1.0f + (480.0f / renderHeight - 1.0f) * Screen.fGrainScale;
                    }
                });
        }
    }

    InitShaders();
}
