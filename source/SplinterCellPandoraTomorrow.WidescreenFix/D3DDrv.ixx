module;

#include <stdafx.h>
#include "../Shared/SplinterCell/Window.hpp"
#include "FixSettings.hpp"
#include "MovieFrame.hpp"
#include <minidx8/d3d8.h>

export module D3DDrv;

import ComVars;
import Shaders;
import Xidi;

// Rain splash point sprites use pixel-sized D3D8 constants scaled for 640x480, causing them to shrink at higher resolutions
namespace RainSplashFix
{
    bool   bInitialized   = false;
    float* pSizeScaleC73z = nullptr;
    float* pSizeScaleC75y = nullptr;
    float* pMaxSizeC75z   = nullptr;
    float* pMaxSizeC75w   = nullptr;
    float  fOrigC73z = 0.0f, fOrigC75y = 0.0f, fOrigC75z = 0.0f, fOrigC75w = 0.0f;
    uint32_t* pDoneFlag = nullptr; // retail only

    // Retail builds c73 on the stack and lazily stores c74/c75 as immediates
    void InitRetail()
    {
        auto pat = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "F6 05 ? ? ? ? 02 75 ? 83 0D ? ? ? ? 02 C7 05 ? ? ? ? 00 00 00 41 C7 05 ? ? ? ? 00 40 9C 45 C7 05 ? ? ? ? 00 00 C0 41 C7 05 ? ? ? ? 00 00 F0 41");
        auto pat2 = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 0D ? ? ? ? DC 08 D9 05");
        if (pat.empty() || pat2.empty())
            return;

        uint32_t* doneFlag = *pat.get_first<uint32_t*>(2);
        float* c73z = *pat2.get_first<float*>(2);
        if (!doneFlag || !c73z || *reinterpret_cast<uint32_t*>(c73z) != 0x46EA6000u)
            return;

        pDoneFlag      = doneFlag;
        pSizeScaleC73z = c73z;
        pSizeScaleC75y = pat.get_first<float>(32);
        pMaxSizeC75z   = pat.get_first<float>(42);
        pMaxSizeC75w   = pat.get_first<float>(52);

        if (*reinterpret_cast<uint32_t*>(pSizeScaleC75y) != 0x459C4000u ||
            *reinterpret_cast<uint32_t*>(pMaxSizeC75z)   != 0x41C00000u ||
            *reinterpret_cast<uint32_t*>(pMaxSizeC75w)   != 0x41F00000u)
            return;

        fOrigC73z = *pSizeScaleC73z;
        fOrigC75y = *pSizeScaleC75y;
        fOrigC75z = *pMaxSizeC75z;
        fOrigC75w = *pMaxSizeC75w;

        bInitialized = true;
    }

    void Init()
    {
        // Rain splash constant block (c73–c75)
        auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "6A 01 68 ? ? ? ? 6A 4B 8B 80 94 46 00 00");
        if (pattern.empty())
        {
            InitRetail();
            return;
        }

        float* c75 = *pattern.get_first<float*>(3);
        if (!c75)
            return;

        float* c73 = c75 - 8; // contiguous float4 block

        // Verify expected c73–c75 constant block for this build
        static const uint32_t kExpected[12] =
        {
            0x40C00000u, 0x40A00000u, 0x46EA6000u, 0x3F000000u,
            0x41A00000u, 0x41A00000u, 0x40C00000u, 0x3E4CCCCDu,
            0x41000000u, 0x459C4000u, 0x41C00000u, 0x41F00000u,
        };
        for (int i = 0; i < 12; i++)
        {
            if (*reinterpret_cast<uint32_t*>(&c73[i]) != kExpected[i])
                return;
        }

        pSizeScaleC73z = &c73[2];
        pSizeScaleC75y = &c75[1];
        pMaxSizeC75z   = &c75[2];
        pMaxSizeC75w   = &c75[3];

        fOrigC73z = *pSizeScaleC73z;
        fOrigC75y = *pSizeScaleC75y;
        fOrigC75z = *pMaxSizeC75z;
        fOrigC75w = *pMaxSizeC75w;

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

        ProtectedGameRef<float>(reinterpret_cast<float*>(pSizeScaleC73z)) = fOrigC73z * scale;
        ProtectedGameRef<float>(reinterpret_cast<float*>(pSizeScaleC75y)) = fOrigC75y * scale;
        ProtectedGameRef<float>(reinterpret_cast<float*>(pMaxSizeC75z)) = fOrigC75z * scale;
        ProtectedGameRef<float>(reinterpret_cast<float*>(pMaxSizeC75w)) = fOrigC75w * scale;

        if (pDoneFlag) // retail: re-run the patched lazy initialiser
            ProtectedGameRef<uint32_t>(reinterpret_cast<uint32_t*>(pDoneFlag)) = *pDoneFlag & ~2u;
    }
}

D3DPRESENT_PARAMETERS* pPresentParams = nullptr;
SafetyHookInline shUD3DRenderDeviceSetRes = {};
int __fastcall UD3DRenderDeviceSetRes(void* UD3DRenderDevice, void* edx, void* UViewport, int width, int height, int a5)
{
    static const int mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    static bool first = true;
    if (std::exchange(first, false))
        a5 = mode == 0;
    const auto nativeResolution = RequestedNativeResolution;
    if (nativeResolution)
    {
        width = nativeResolution->first;
        height = nativeResolution->second;
    }
    if (!a5 && mode == 2)
    {
        auto area = SCWindow::Monitor(pPresentParams ? pPresentParams->hDeviceWindow : nullptr).rcMonitor;
        width = area.right - area.left;
        height = area.bottom - area.top;
    }

    bool bInvalidRes = !nativeResolution;
    std::wstring resStr = std::to_wstring(width) + L"x" + std::to_wstring(height);
    for (const auto& it : ResList)
    {
        if (!nativeResolution && resStr == it.first)
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
    if (!ret || !pPresentParams)
        return ret;

    Screen.Width = pPresentParams->BackBufferWidth;
    Screen.Height = pPresentParams->BackBufferHeight;
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
    constexpr auto HUD_OFFSET_X = 24.0f; // Default offset used in original scripts

    auto computeHudOffset = [&](float width, float height) -> float
    {
        if (Screen.nHudWidescreenMode > 1)
            return CalculateWidescreenOffset(width, height, 640.0f, 480.0f, HUD_OFFSET_X);
        else
            return HUD_OFFSET_X - CalculateWidescreenOffset(width, height, 640.0f, 480.0f, 0.0f);
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

    float targetAspect = 4.0f / 3.0f;
    if (Screen.nFMVWidescreenMode && Screen.fAspectRatio > targetAspect)
    {
        // Video has black bars on top and bottom, zoom in to hide them
        float scale = 480.0f / 384.0f;

        float contentHeight = Screen.fHeight * scale;
        float contentWidth = contentHeight * targetAspect;

        // Center the scaled content
        Screen.fFMVoffsetStartX = (Screen.fWidth - contentWidth) / 2.0f;
        Screen.fFMVoffsetEndX = Screen.fFMVoffsetStartX + contentWidth;
        Screen.fFMVoffsetStartY = (Screen.fHeight - contentHeight) / 2.0f;
        Screen.fFMVoffsetEndY = Screen.fFMVoffsetStartY + contentHeight;
    }
    else
    {
        if (Screen.fAspectRatio > targetAspect)
        {
            // Pillarbox: Fit video to screen height, center horizontally
            float targetWidth = Screen.fHeight * targetAspect;
            Screen.fFMVoffsetStartX = (Screen.fWidth - targetWidth) / 2.0f;
            Screen.fFMVoffsetEndX = Screen.fFMVoffsetStartX + targetWidth;
            Screen.fFMVoffsetStartY = 0.0f;
            Screen.fFMVoffsetEndY = Screen.fHeight;
        }
        else if (Screen.fAspectRatio < targetAspect)
        {
            // Letterbox: Fit video to screen width, center vertically
            float targetHeight = Screen.fWidth / targetAspect;
            Screen.fFMVoffsetStartY = (Screen.fHeight - targetHeight) / 2.0f;
            Screen.fFMVoffsetEndY = Screen.fFMVoffsetStartY + targetHeight;
            Screen.fFMVoffsetStartX = 0.0f;
            Screen.fFMVoffsetEndX = Screen.fWidth;
        }
        else
        {
            // Exact 4:3: No bars needed
            Screen.fFMVoffsetStartX = 0.0f;
            Screen.fFMVoffsetEndX = Screen.fWidth;
            Screen.fFMVoffsetStartY = 0.0f;
            Screen.fFMVoffsetEndY = Screen.fHeight;
        }
    }

    SCWindow::Track(pPresentParams->hDeviceWindow, mode, pPresentParams->Windowed != FALSE, Screen.Width, Screen.Height);
    SCWindow::SquareCorners(pPresentParams->hDeviceWindow);

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

    bIsWindowed = pPresentParams->Windowed ? true : false;

    if (bIsWindowed)
    {
        SCWindow::Place(pPresentParams->hDeviceWindow, Screen.Width, Screen.Height, mode == 2);
        SetForegroundWindow(pPresentParams->hDeviceWindow);
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

        const auto bink = *reinterpret_cast<const int**>(reinterpret_cast<uintptr_t>(UCanvas) + 0x98);
        if (bink)
        {
            const auto frame = PTMovie::Layout(Screen.Width, Screen.Height, bink[0], bink[1],
                FixSettings().Int("MAIN.FMVWidescreenMode"));
            if (frame.width > 0.0f && frame.height > 0.0f)
            {
                Screen.fFMVoffsetStartX = frame.x;
                Screen.fFMVoffsetEndX = frame.x + frame.width;
                Screen.fFMVoffsetStartY = frame.y;
                Screen.fFMVoffsetEndY = frame.y + frame.height;
            }
        }
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
    pPresentParams = *pattern.get_first<D3DPRESENT_PARAMETERS*>(1);

    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 81 EC ? ? ? ? 53 56 57 8B 7D ? 8B F1", "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 ? 56 57 50 8D 45 ? 64 A3 ? ? ? ? 8B F1 89 75");
    shUD3DRenderDeviceSetRes = safetyhook::create_inline(pattern.get_first(), UD3DRenderDeviceSetRes);

    bool bRetailVersion = GetProcAddress(GetModuleHandle(L"D3DDrv"), "?Exec@UD3DRenderDevice@@UAEHPBGAAVFOutputDevice@@@Z") != nullptr;

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

        if (!bRetailVersion)
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
            if (!pattern.empty())
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
            if (!pattern.empty())
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
        if (!pattern.empty())
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
        ProtectedGameRef<uint8_t>(reinterpret_cast<uint8_t*>(pattern.get_first(-4))) = 0x56; //push esi
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
        if (Screen.nPostProcessFixedScale == 1)
            Screen.nPostProcessFixedScale = Screen.Width;

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 00 02 00 00 68 00 02 00 00");
        for (size_t i = 0; i < pattern.size(); i++)
        {
            if (pattern.size() == 8 && i == 2)
                continue;

            ProtectedGameRef<decltype(Screen.nPostProcessFixedScale)>(reinterpret_cast<decltype(Screen.nPostProcessFixedScale)*>(pattern.get(i).get<uint32_t>(1))) = Screen.nPostProcessFixedScale; //affects glass reflections
            ProtectedGameRef<decltype(Screen.nPostProcessFixedScale)>(reinterpret_cast<decltype(Screen.nPostProcessFixedScale)*>(pattern.get(i).get<uint32_t>(6))) = Screen.nPostProcessFixedScale;
        }

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "B8 00 02 00 00");
        if (!pattern.empty())
            ProtectedGameRef<decltype(Screen.nPostProcessFixedScale)>(reinterpret_cast<decltype(Screen.nPostProcessFixedScale)*>(pattern.count(1).get(0).get<uint32_t>(1))) = Screen.nPostProcessFixedScale;
        else
        {
            pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 80 ? ? ? ? ? ? ? ? 56");
            ProtectedGameRef<decltype(Screen.nPostProcessFixedScale)>(reinterpret_cast<decltype(Screen.nPostProcessFixedScale)*>(pattern.get_first(6))) = Screen.nPostProcessFixedScale;

            pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "C7 80 ? ? ? ? ? ? ? ? 8B 45 ? 6A");
            ProtectedGameRef<decltype(Screen.nPostProcessFixedScale)>(reinterpret_cast<decltype(Screen.nPostProcessFixedScale)*>(pattern.get_first(6))) = Screen.nPostProcessFixedScale;
        }
    }

    if (Screen.nShadowMapResolution > 0)
    {
        if (Screen.nShadowMapResolution == 1)
            Screen.nShadowMapResolution = std::clamp(Screen.Width, 0, bRetailVersion ? 2048 : 3072);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "83 EC ? 53 55 56 8B F1 8B 86", "55 8B EC 83 EC ? 53 56 8B F1 57");
        auto rpattern = hook::range_pattern((uint32_t)pattern.get_first(), (uint32_t)pattern.get_first() + 0x488, "68 ? ? ? ? 68");
        for (size_t i = 0; i < rpattern.size(); i++)
        {
            ProtectedGameRef<decltype(Screen.nShadowMapResolution)>(reinterpret_cast<decltype(Screen.nShadowMapResolution)*>(rpattern.get(i).get<uint32_t>(1))) = Screen.nShadowMapResolution;
            ProtectedGameRef<decltype(Screen.nShadowMapResolution)>(reinterpret_cast<decltype(Screen.nShadowMapResolution)*>(rpattern.get(i).get<uint32_t>(6))) = Screen.nShadowMapResolution;
        }
    }

    if (Screen.nReflectionsResolution > 0)
    {
        if (Screen.nReflectionsResolution == 1)
            Screen.nReflectionsResolution = std::clamp(Screen.Width, 0, bRetailVersion ? 2048 : 3072);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? C7 04 81 ? ? ? ? 8B 07 8B 10 68 ? ? ? ? 50 FF 52 ? 6A ? 6A ? 8B CE FF 15 ? ? ? ? 8B 0E 68");
        if (!pattern.empty())
        {
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(pattern.get_first(1))) = Screen.nReflectionsResolution;
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(pattern.get_first(17))) = Screen.nReflectionsResolution;

            pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? C7 04 81 ? ? ? ? 8B 07 8B 10 68 ? ? ? ? 50 FF 52 ? 6A ? 6A ? 8B CE FF 15 ? ? ? ? 8B 0E C7 04 81");
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(pattern.get_first(1))) = Screen.nReflectionsResolution;
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(pattern.get_first(17))) = Screen.nReflectionsResolution;
        }
        else
        {
            auto rpattern = hook::range_pattern(shUD3DRenderDeviceSetRes.target_address(), shUD3DRenderDeviceSetRes.target_address() + 0x1AFB, "68 ? ? ? ? 68");
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(rpattern.get(10).get<uint32_t>(1))) = Screen.nReflectionsResolution;
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(rpattern.get(10).get<uint32_t>(6))) = Screen.nReflectionsResolution;
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(rpattern.get(11).get<uint32_t>(1))) = Screen.nReflectionsResolution;
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(rpattern.get(11).get<uint32_t>(6))) = Screen.nReflectionsResolution;
        }

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 8B CF", "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? FF 50 ? 8B 07");
        if (!pattern.empty())
        {
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(pattern.get_first(1))) = Screen.nReflectionsResolution;
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(pattern.get_first(6))) = Screen.nReflectionsResolution;
        }

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 8B CB FF 50", "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 8B CB FF 52 ? 68 ? ? ? ? 68 ? ? ? ? 53 8D 4C 24 ? FF 15 ? ? ? ? 8B 45 ? 85 C0");
        if (!pattern.empty())
        {
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(pattern.get_first(1))) = Screen.nReflectionsResolution;
            ProtectedGameRef<decltype(Screen.nReflectionsResolution)>(reinterpret_cast<decltype(Screen.nReflectionsResolution)*>(pattern.get_first(6))) = Screen.nReflectionsResolution;
        }
    }

    if (Screen.nBloomResolutionMultiplier > 0)
    {
        auto rpattern = hook::range_pattern(shUD3DRenderDeviceSetRes.target_address(), shUD3DRenderDeviceSetRes.target_address() + 0x1AFB, "68 ? ? ? ? 68");
        ProtectedGameRef<decltype(*rpattern.get(1).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier)>(reinterpret_cast<decltype(*rpattern.get(1).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier)*>(rpattern.get(1).get<uint32_t>(1))) = *rpattern.get(1).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier;
        ProtectedGameRef<decltype(*rpattern.get(1).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier)>(reinterpret_cast<decltype(*rpattern.get(1).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier)*>(rpattern.get(1).get<uint32_t>(6))) = *rpattern.get(1).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier;
        ProtectedGameRef<decltype(*rpattern.get(2).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier)>(reinterpret_cast<decltype(*rpattern.get(2).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier)*>(rpattern.get(2).get<uint32_t>(1))) = *rpattern.get(2).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier;
        ProtectedGameRef<decltype(*rpattern.get(2).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier)>(reinterpret_cast<decltype(*rpattern.get(2).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier)*>(rpattern.get(2).get<uint32_t>(6))) = *rpattern.get(2).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier;
        ProtectedGameRef<decltype(*rpattern.get(3).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier)>(reinterpret_cast<decltype(*rpattern.get(3).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier)*>(rpattern.get(3).get<uint32_t>(1))) = *rpattern.get(3).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier;
        ProtectedGameRef<decltype(*rpattern.get(3).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier)>(reinterpret_cast<decltype(*rpattern.get(3).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier)*>(rpattern.get(3).get<uint32_t>(6))) = *rpattern.get(3).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier;
        ProtectedGameRef<decltype(*rpattern.get(3).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier)>(reinterpret_cast<decltype(*rpattern.get(3).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier)*>(rpattern.get(4).get<uint32_t>(1))) = *rpattern.get(3).get<uint32_t>(1) * Screen.nBloomResolutionMultiplier;
        ProtectedGameRef<decltype(*rpattern.get(3).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier)>(reinterpret_cast<decltype(*rpattern.get(3).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier)*>(rpattern.get(4).get<uint32_t>(6))) = *rpattern.get(3).get<uint32_t>(6) * Screen.nBloomResolutionMultiplier;

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
        if (!pattern.empty())
            shsub_1009D270 = safetyhook::create_inline(pattern.get_first(), sub_1009D270);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "55 8B EC 83 EC ? A1 ? ? ? ? 33 C5 89 45 ? 8B 45 ? 8D 55", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 24 8B 45 10 53 8B 5D 0C 56 57 8B 7D 08 8B F1");
        shsub_100A7850 = safetyhook::create_inline(pattern.get_first(), sub_100A7850);
    }

    // Flash afterimage is captured into a hardcoded 256x256 buffer and stretched over the screen. It shares the main
    // depth stencil, so it has to match the backbuffer exactly or SetRenderTarget fails and the effect never draws.
    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "6A 17 8B 00 6A 01 6A 01 68 00 01 00 00 8B 08 68 00 01 00 00", "6A 17 6A 01 6A 01 68 00 01 00 00 68 00 01 00 00");
    if (!pattern.empty())
    {
        bool bDigitalLayout = *pattern.get_first<uint8_t>(2) == 0x8B;
        ProtectedGameRef<decltype(Screen.Height)>(reinterpret_cast<decltype(Screen.Height)*>(pattern.get_first(bDigitalLayout ? 9 : 7))) = Screen.Height;
        ProtectedGameRef<decltype(Screen.Width)>(reinterpret_cast<decltype(Screen.Width)*>(pattern.get_first(bDigitalLayout ? 16 : 12))) = Screen.Width;
    }

    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B 10 50 FF 92 ? ? ? ? 8B 9E 28 58 00 00", "8B 08 FF 91 ? ? ? ? FF 83 2C 58 00 00");
    static auto EndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        UpdateXidiProfile();

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
    if (!pattern.empty())
        ProtectedGameRef<uint8_t>(reinterpret_cast<uint8_t*>(pattern.get_first(5))) = 0xEB; // ja -> jmp

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
        if (!pattern.empty())
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