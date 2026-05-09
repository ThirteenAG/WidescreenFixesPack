module;

#include <stdafx.h>
#include "common.h"
#include <d3d9.h>

export module TransparentMenu;

import Skeleton;
import Menu;
import TransparentMenuDX8;
import TransparentMenuDX9;

export bool gTransparentMenuCanRender = false;
export bool gTransparentMenuWaitForGameplay = false;

class TransparentMenu
{
public:
    TransparentMenu()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            auto bTransparentMenu = iniReader.ReadInteger("GRAPHICS", "TransparentMenu", 0) != 0;

            if (!bTransparentMenu)
                return;

            if (*pD3D8Device)
            {
                HRESULT hr = (*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&pD3D9Device);
                if (FAILED(hr))
                    pD3D9Device = nullptr;
            }

            auto pattern = hook::pattern("E8 ? ? ? ? 83 7C 24 ? ? 59 75 ? 83 C4");
            static auto captureHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                static bool wasMenuActive = false;
                bool isMenuActive = CMenuManager::m_bIsActive;

                // After reset: ignore menu frames until gameplay appears.
                if (gTransparentMenuWaitForGameplay)
                {
                    if (isMenuActive)
                    {
                        gTransparentMenuCanRender = false;
                        wasMenuActive = true; // keep transition stable
                        return;
                    }

                    // First gameplay frame after reset
                    gTransparentMenuCanRender = false;
                    wasMenuActive = false;
                    gTransparentMenuWaitForGameplay = false;
                    return;
                }

                if (!isMenuActive)
                {
                    wasMenuActive = false;
                    gTransparentMenuCanRender = false;
                    return;
                }

                if (!wasMenuActive || !gTransparentMenuCanRender)
                {
                    if (pD3D9Device)
                        gTransparentMenuCanRender = TransparentMenuDX9::CaptureFrame();
                    else
                        gTransparentMenuCanRender = TransparentMenuDX8::CaptureFrame();
                }

                if (gTransparentMenuCanRender)
                {
                    if (pD3D9Device)
                        TransparentMenuDX9::RenderBlur();
                    else
                        TransparentMenuDX8::RenderBlur();
                }

                wasMenuActive = true;
            });

            WFP::onBeforeReset() += []()
            {
                gTransparentMenuCanRender = false;
                gTransparentMenuWaitForGameplay = true;

                TransparentMenuDX8::OnDeviceReset();
                TransparentMenuDX9::OnDeviceReset();

                if (pD3D9Device)
                {
                    pD3D9Device->Release();
                    pD3D9Device = nullptr;
                }

                if (!pD3D8Device || !*pD3D8Device)
                    return;

                IDirect3DDevice9* dev9 = nullptr;
                HRESULT hr = (*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&dev9);
                if (SUCCEEDED(hr) && dev9)
                    pD3D9Device = dev9;
            };
        };
    }
} TransparentMenu;