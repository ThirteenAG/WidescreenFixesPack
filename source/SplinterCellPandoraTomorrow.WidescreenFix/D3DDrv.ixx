module;

#include <stdafx.h>
#include <dxsdk/dx8/d3d8.h>

export module D3DDrv;

import ComVars;

D3DPRESENT_PARAMETERS* pPresentParams = nullptr;
SafetyHookInline shUD3DRenderDeviceSetRes = {};
int __fastcall UD3DRenderDeviceSetRes(void* UD3DRenderDevice, void* edx, void* UViewport, int width, int height, int a5)
{
    bool bInvalidRes = true;
    std::wstring resStr = std::to_wstring(width) + L"x" + std::to_wstring(height);
    for (const auto& it : ResList)
    {
        if (resStr == it.first)
        {
            swscanf_s(it.second.c_str(), L"%dx%d", &width, &height);
            bInvalidRes = false;
            break;
        }
    }

    if (bInvalidRes)
    {
        width = Screen.Width;
        height = Screen.Height;
        bInvalidRes = false;
    }

    auto ret = shUD3DRenderDeviceSetRes.unsafe_fastcall<int>(UD3DRenderDevice, edx, UViewport, width, height, a5);

    Screen.Width = pPresentParams->BackBufferWidth;
    Screen.Height = pPresentParams->BackBufferHeight;
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.dHUDScaleX = static_cast<double>(Screen.fHUDScaleX);
    Screen.dHUDScaleXInv = 1.0 / Screen.dHUDScaleX;
    Screen.fWidescreenHudOffset = Screen.fIniHudOffset;
    if (Screen.fAspectRatio < (16.0f / 9.0f))
        Screen.fWidescreenHudOffset = Screen.fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
    if (Screen.fAspectRatio <= (4.0f / 3.0f))
        Screen.fWidescreenHudOffset = 0.0f;

    float targetAspect = 4.0f / 3.0f;
    if (Screen.nFMVWidescreenMode && Screen.fAspectRatio > targetAspect)
    {
        // Video has black bars on top and bottom, zoom in to hide them
        float barsHeight = 96.0f + 40.0f;
        float scale = Screen.fHeight / (Screen.fHeight - 2.0f * barsHeight);

        float contentHeight = Screen.fHeight * scale;
        float contentWidth = contentHeight * targetAspect;

        // Center the scaled content
        Screen.fFMVoffsetStartX = (Screen.fWidth - contentWidth) / 2.0f;
        Screen.fFMVoffsetEndX = Screen.fFMVoffsetStartX + contentWidth;
        Screen.fFMVoffsetStartY = -barsHeight * scale;
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

    if (Screen.Width < 640 || Screen.Height < 480)
        return ret;

    CIniReader iniReader("");
    auto [DesktopResW, DesktopResH] = GetDesktopRes();
    if (Screen.Width != DesktopResW || Screen.Height != DesktopResH)
    {
        iniReader.WriteInteger("MAIN", "ResX", Screen.Width);
        iniReader.WriteInteger("MAIN", "ResY", Screen.Height);
    }
    else
    {
        iniReader.WriteInteger("MAIN", "ResX", 0);
        iniReader.WriteInteger("MAIN", "ResY", 0);
    }

    bIsWindowed = pPresentParams->Windowed ? true : false;

    if (bIsWindowed)
    {
        tagRECT rect;
        rect.left = (LONG)(((float)DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
        rect.top = (LONG)(((float)DesktopResH / 2.0f) - (Screen.fHeight / 2.0f));
        rect.right = (LONG)Screen.Width;
        rect.bottom = (LONG)Screen.Height;
        SetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE, GetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(pPresentParams->hDeviceWindow, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
        SetWindowPos(pPresentParams->hDeviceWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(pPresentParams->hDeviceWindow);
        SetCursor(NULL);
    }

    return ret;
}

uint32_t xRefIndex = -1;
SafetyHookInline shsub_1009D270 = {};
void __fastcall sub_1009D270(void* FRenderInterface, void* edx, int a2, int a3, float a4, float a5, float a6, float a7, int* a8)
{
    a4 *= Screen.nBloomResolution;
    a5 *= Screen.nBloomResolution;
    a6 *= Screen.nBloomResolution;
    a7 *= Screen.nBloomResolution;

    if (xRefIndex == 1)
    {
        a6 += 40.0f;
        a7 += 40.0f;
    }
    else if (xRefIndex == 2)
    {
        // I don't know what to put here
        a4 += 10000.0f;
        a5 += 10000.0f;
        a6 += 10000.0f;
        a7 += 10000.0f;
    }

    return shsub_1009D270.unsafe_fastcall(FRenderInterface, edx, a2, a3, a4, a5, a6, a7, a8);
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

        shDisplayVideo.unsafe_fastcall(UD3DRenderDevice, edx, UCanvas, a3);

        IDirect3DDevice8* pD3DDevice = *(IDirect3DDevice8**)((uintptr_t)UD3DRenderDevice + 0x4694);

        int W = Screen.Width;
        int H = Screen.Height;

        float targetX = Screen.fFMVoffsetStartX;
        float targetY = Screen.fFMVoffsetStartY;
        float targetW = Screen.fFMVoffsetEndX - Screen.fFMVoffsetStartX;
        float targetH = Screen.fFMVoffsetEndY - Screen.fFMVoffsetStartY;

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

    bool bRetailVersion = false;

    // Water Blend Fix
    static DWORD OriginalAlphaBlendEnable = 0;
    static DWORD OriginalSrcBlend = 0;
    static DWORD OriginalDestBlend = 0;

    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "FF B5 ? ? ? ? 8B 07 6A ? FF B5");
    if (!pattern.empty())
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

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 50 8B 08 FF 51 ? 8B 07");
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
    else
        bRetailVersion = true;

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
        if (Screen.nPostProcessFixedScale == 1)
            Screen.nPostProcessFixedScale = Screen.Width;

        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "68 00 02 00 00 68 00 02 00 00");
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
            Screen.nShadowMapResolution = std::clamp(Screen.Width, 0, bRetailVersion ? 2048 : 3072);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "83 EC ? 53 55 56 8B F1 8B 86", "55 8B EC 83 EC ? 53 56 8B F1 57");
        auto rpattern = hook::range_pattern((uint32_t)pattern.get_first(), (uint32_t)pattern.get_first() + 0x488, "68 ? ? ? ? 68");
        for (size_t i = 0; i < rpattern.size(); i++)
        {
            injector::WriteMemory(rpattern.get(i).get<uint32_t>(1), Screen.nShadowMapResolution, true);
            injector::WriteMemory(rpattern.get(i).get<uint32_t>(6), Screen.nShadowMapResolution, true);
        }
    }

    if (Screen.nReflectionsResolution > 0)
    {
        if (Screen.nReflectionsResolution == 1)
            Screen.nReflectionsResolution = std::clamp(Screen.Width, 0, bRetailVersion ? 2048 : 3072);

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
            auto rpattern = hook::range_pattern(shUD3DRenderDeviceSetRes.target_address(), shUD3DRenderDeviceSetRes.target_address() + 0x1AFB, "68 ? ? ? ? 68");
            injector::WriteMemory(rpattern.get(10).get<uint32_t>(1), Screen.nReflectionsResolution, true);
            injector::WriteMemory(rpattern.get(10).get<uint32_t>(6), Screen.nReflectionsResolution, true);
            injector::WriteMemory(rpattern.get(11).get<uint32_t>(1), Screen.nReflectionsResolution, true);
            injector::WriteMemory(rpattern.get(11).get<uint32_t>(6), Screen.nReflectionsResolution, true);
        }

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 8B CF", "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? FF 50 ? 8B 07");
        injector::WriteMemory(pattern.get_first(1), Screen.nReflectionsResolution, true);
        injector::WriteMemory(pattern.get_first(6), Screen.nReflectionsResolution, true);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 8B CB");
        injector::WriteMemory(pattern.get_first(1), Screen.nReflectionsResolution, true);
        injector::WriteMemory(pattern.get_first(6), Screen.nReflectionsResolution, true);
    }

    if (Screen.nBloomResolution > 0)
    {
        constexpr auto defaultBloomRes = 256u;
        static auto newBloomRes = defaultBloomRes * std::clamp(Screen.nBloomResolution, 0u, Screen.Height / defaultBloomRes);
        auto rpattern = hook::range_pattern(shUD3DRenderDeviceSetRes.target_address(), shUD3DRenderDeviceSetRes.target_address() + 0x1AFB, "68 ? ? ? ? 68");
        injector::WriteMemory(rpattern.get(1).get<uint32_t>(1), newBloomRes, true);
        injector::WriteMemory(rpattern.get(1).get<uint32_t>(6), newBloomRes, true);
        injector::WriteMemory(rpattern.get(2).get<uint32_t>(1), newBloomRes / 2, true);
        injector::WriteMemory(rpattern.get(2).get<uint32_t>(6), newBloomRes / 2, true);
        injector::WriteMemory(rpattern.get(3).get<uint32_t>(1), newBloomRes, true);
        injector::WriteMemory(rpattern.get(3).get<uint32_t>(6), newBloomRes, true);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "50 FF 51 ? 8B 45 ? 81 C6 ? ? ? ? 56 6A ? 6A");
        static auto ResHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esp + 0x0) = newBloomRes / 4;
            *(uint32_t*)(regs.esp + 0x4) = newBloomRes / 4;
        });

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "50 FF 51 ? 8B 4D ? 6A");
        static auto ResHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            *(uint32_t*)(regs.esp + 0x0) = newBloomRes / 8;
            *(uint32_t*)(regs.esp + 0x4) = newBloomRes / 8;
        });

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "C7 83 ? ? ? ? ? ? ? ? F3 A5");
        static auto ResHook3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            xRefIndex = 1;
        });

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 04 24 ? ? ? ? FF D7 8D 8D ? ? ? ? FF 15 ? ? ? ? 8B 46 ? 6A ? 8B 80");
        static auto ResHook4 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            xRefIndex = 2;
        });

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B CB C7 44 24");
        static auto ResHook5 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            xRefIndex = 3;
        });

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "55 8B EC 6A ? 68 ? ? ? ? 64 A1 ? ? ? ? 50 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 ? 53 56 57 50 8D 45 ? 64 A3 ? ? ? ? 8B D9 89 9D ? ? ? ? F3 0F 10 45");
        shsub_1009D270 = safetyhook::create_inline(pattern.get_first(), sub_1009D270);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? FF 50 ? 83 EC ? 8D 8D ? ? ? ? C7 44 24 ? ? ? ? ? C7 04 24 ? ? ? ? 53");
        injector::WriteMemory(pattern.get_first(1), newBloomRes, true);
        injector::WriteMemory(pattern.get_first(6), newBloomRes, true);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? FF 50 ? 8B 46 ? 6A ? 6A");
        injector::WriteMemory(pattern.get_first(1), newBloomRes, true);
        injector::WriteMemory(pattern.get_first(6), newBloomRes, true);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? FF 50 ? 83 EC ? 8D 8D ? ? ? ? C7 44 24 ? ? ? ? ? C7 04 24 ? ? ? ? 56");
        injector::WriteMemory(pattern.get_first(1), newBloomRes, true);
        injector::WriteMemory(pattern.get_first(6), newBloomRes, true);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "68 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? FF 50 ? 8B 46 ? 6A ? 8B 80");
        injector::WriteMemory(pattern.get_first(1), newBloomRes, true);
        injector::WriteMemory(pattern.get_first(6), newBloomRes, true);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? 8B 88 ? ? ? ? 85 C9 74 ? 83 BE");
        injector::WriteMemory(pattern.get_first(3), newBloomRes, true);
        injector::WriteMemory(pattern.get_first(10), newBloomRes, true);

        pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? C7 45 ? ? ? ? ? 8B 88 ? ? ? ? 85 C9 74 ? 83 BF");
        injector::WriteMemory(pattern.get_first(3), newBloomRes, true);
        injector::WriteMemory(pattern.get_first(10), newBloomRes, true);
    }

    pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "8B 10 50 FF 92 ? ? ? ? 8B 9E 28 58 00 00", "8B 08 FF 91 ? ? ? ? FF 83 2C 58 00 00");
    static auto EndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
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
                D3DRECT leftRect = { 0, 0, (LONG)targetX, H };
                pD3DDevice->Clear(1, &leftRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            }

            // Clear right pillar area
            if (targetX + targetW < (float)W)
            {
                D3DRECT rightRect = { (LONG)(targetX + targetW), 0, W, H };
                pD3DDevice->Clear(1, &rightRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            }

            // Clear top letterbox area
            if (targetY > 0.0f)
            {
                D3DRECT topRect = { 0, 0, W, (LONG)targetY };
                pD3DDevice->Clear(1, &topRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            }

            // Clear bottom letterbox area
            if (targetY + targetH < (float)H)
            {
                D3DRECT bottomRect = { 0, (LONG)(targetY + targetH), W, H };
                pD3DDevice->Clear(1, &bottomRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
            }
        }
    });
}