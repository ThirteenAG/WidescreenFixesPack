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
            swscanf(it.second.c_str(), L"%dx%d", &width, &height);
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
    Screen.fFMVoffsetStartX = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fFMVoffsetEndX = Screen.fWidth - Screen.fFMVoffsetStartX;
    Screen.fFMVoffsetStartY = 0.0f - ((Screen.fHeight - ((Screen.fHeight / 1.5f) * ((16.0f / 9.0f) / Screen.fAspectRatio))) / 2.0f);
    Screen.fFMVoffsetEndY = Screen.fHeight - Screen.fFMVoffsetStartY;
    Screen.fWidescreenHudOffset = Screen.fIniHudOffset;
    if (Screen.fAspectRatio < (16.0f / 9.0f))
        Screen.fWidescreenHudOffset = Screen.fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
    if (Screen.fAspectRatio <= (4.0f / 3.0f))
        Screen.fWidescreenHudOffset = 0.0f;

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

    if (pPresentParams->Windowed)
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

export void InitD3DDrv()
{
    pPresentParams = *find_module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? 33 C0 8B D9 C1 E9 02 83 E3 03", "68 ? ? ? ? FF 15 ? ? ? ? 8B 8D").get_first<D3DPRESENT_PARAMETERS*>(1);
    auto pattern = find_module_pattern(GetModuleHandle(L"D3DDrv"), "B8 01 00 00 00 8B 4D F4 64 89 0D 00 00 00 00 5F 5E 5B 8B E5 5D C2 10 00", "B8 ? ? ? ? EB ? 33 C0 89 86");

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

        static auto WaterPreDraw = [](SafetyHookContext& regs, IDirect3DDevice8* pDevice)
        {
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
                            {
                                WaterBlendPS = 0;
                            }
                        }
                    }
                }
            }

            if (WaterBlendPS)
            {
                pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &OriginalAlphaBlendEnable);
                pDevice->GetRenderState(D3DRS_SRCBLEND, &OriginalSrcBlend);
                pDevice->GetRenderState(D3DRS_DESTBLEND, &OriginalDestBlend);

                pDevice->SetPixelShader(WaterBlendPS);
                pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
                pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            }
        };

        static auto WaterPostDraw = [](SafetyHookContext& regs, IDirect3DDevice8* pDevice)
        {
            if (WaterBlendPS)
            {
                pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, OriginalAlphaBlendEnable);
                pDevice->SetRenderState(D3DRS_SRCBLEND, OriginalSrcBlend);
                pDevice->SetRenderState(D3DRS_DESTBLEND, OriginalDestBlend);
            }

            pDevice->SetPixelShader(0);
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
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "D9 1C 24 56 56 FF 15");
    if (!pattern.empty())
    {
        injector::WriteMemory<uint8_t>(pattern.get_first(-4), 0x56, true); //push esi
        struct DisplayVideo_Hook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x04) = static_cast<float>(*(int32_t*)(regs.esp + 0x68 + 0x4));

                if (Screen.nFMVWidescreenMode)
                {
                    *(float*)(regs.esp + 0x00) = Screen.fFMVoffsetStartY;
                    *(float*)(regs.esp + 0x08) = Screen.fFMVoffsetEndY;
                }
                else
                {
                    *(float*)&regs.esi = Screen.fFMVoffsetStartX;
                    *(float*)(regs.esp + 0x04) = Screen.fFMVoffsetEndX;
                }
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
                *(float*)(regs.esp + 0x00) = 0.0f;

                if (Screen.nFMVWidescreenMode)
                {
                    *(float*)(regs.esp + 0x04) = Screen.fFMVoffsetStartY;
                    *(float*)(regs.esp + 0x0C) = Screen.fFMVoffsetEndY;
                }
                else
                {
                    *(float*)(regs.esp + 0x00) = Screen.fFMVoffsetStartX;
                    *(float*)(regs.esp + 0x08) = Screen.fFMVoffsetEndX;
                }
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
}