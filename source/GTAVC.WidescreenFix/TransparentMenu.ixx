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

injector::hook_back<void(__cdecl*)(CRect*, uint8_t*, uint8_t*, uint8_t*, uint8_t*)> hbDrawRect1;
void __cdecl CSprite2d__DrawRect1(CRect* a1, uint8_t* a2, uint8_t* a3, uint8_t* a4, uint8_t* a5)
{
    if (FrontendMenuManager->m_bGameNotLoaded)
        return hbDrawRect1.fun(a1, a2, a3, a4, a5);
}

injector::hook_back<void(__cdecl*)(CRect*, uint8_t*)> hbDrawRect2;
void __cdecl CSprite2d__DrawRect2(CRect* a1, uint8_t* a2)
{
    if (FrontendMenuManager->m_bGameNotLoaded)
        return hbDrawRect2.fun(a1, a2);
}

injector::hook_back<void(__cdecl*)(float, float, float, float, float, float, float, float, uint8_t*)> hbDraw2DPolygon;
void __cdecl CSprite2d__Draw2DPolygon(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, uint8_t* a9)
{
    //return hbDraw2DPolygon.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9);
}

class TransparentMenu
{
public:
    TransparentMenu()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            static int bTransparentMenu = iniReader.ReadInteger("GRAPHICS", "TransparentMenu", 0);

            if (bTransparentMenu <= 0)
                return;

            if (*pD3D8Device)
            {
                HRESULT hr = (*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), (void**)&pD3D9Device);
                if (FAILED(hr))
                    pD3D9Device = nullptr;
            }

            auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? B0 ? 83 C4 ? 5B");
            hbDrawRect1.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__DrawRect1, true).get();

            pattern = hook::pattern("E8 ? ? ? ? 59 59 8B 86");
            hbDrawRect2.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__DrawRect2, true).get();

            if (bTransparentMenu < 2)
            {
                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 4C 24 ? 68 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? ? ? 8B 2D ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 81 FD ? ? ? ? ? ? 75 ? ? ? EB ? ? ? ? ? ? ? ? ? 89 6C 24 ? ? ? ? ? ? ? A1");
                hbDraw2DPolygon.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__Draw2DPolygon, true).get();

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 4C 24 ? 68 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? ? ? 8B 0D ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 81 F9 ? ? ? ? ? ? 75 ? ? ? EB ? ? ? ? ? ? ? ? ? 89 4C 24 ? ? ? ? ? ? ? 8B 1D");
                hbDraw2DPolygon.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__Draw2DPolygon, true).get();

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 4C 24 ? 68 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? ? ? 8B 2D ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 81 FD ? ? ? ? ? ? 75 ? ? ? EB ? ? ? ? ? ? ? ? ? 89 6C 24 ? ? ? ? ? ? ? 8B 15");
                hbDraw2DPolygon.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__Draw2DPolygon, true).get();

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 4C 24 ? 68 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? ? ? 8B 0D ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 81 F9 ? ? ? ? ? ? 75 ? ? ? EB ? ? ? ? ? ? ? ? ? 89 4C 24 ? ? ? ? ? ? ? A1");
                hbDraw2DPolygon.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__Draw2DPolygon, true).get();

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 8C 24 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? ? ? 8B 15");
                hbDraw2DPolygon.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__Draw2DPolygon, true).get();

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 8D 8C 24 ? ? ? ? 68 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? ? ? 8B 0D");
                hbDraw2DPolygon.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__Draw2DPolygon, true).get();

                pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 80 3D ? ? ? ? ? 75 ? 31 C0 C6 05 ? ? ? ? ? A3 ? ? ? ? 8B 86");
                hbDraw2DPolygon.fun = injector::MakeCALL(pattern.get_first(), CSprite2d__Draw2DPolygon, true).get();

                pattern = hook::pattern("E8 ? ? ? ? 59 59 6A ? 6A ? E8 ? ? ? ? 59 59 6A ? 6A ? E8 ? ? ? ? 59 59 6A ? 6A ? E8 ? ? ? ? 59 59 6A ? 6A ? E8 ? ? ? ? 59 59 6A ? 6A ? E8 ? ? ? ? 59 59 6A ? 6A ? E8 ? ? ? ? 81 BE");
                static auto renderHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
                {
                    if (gTransparentMenuCanRender)
                    {
                        if (pD3D9Device)
                            TransparentMenuDX9::RenderBlur();
                        else
                            TransparentMenuDX8::RenderBlur();
                    }
                });
            }

            pattern = hook::pattern("53 89 CB B9 ? ? ? ? E8");
            static auto captureHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
            {
                if (FrontendMenuManager->m_bGameNotLoaded)
                    return;

                static bool wasMenuActive = false;
                bool isMenuActive = FrontendMenuManager->GetIsMenuActive();

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

                if (bTransparentMenu > 1)
                {
                    if (gTransparentMenuCanRender)
                    {
                        if (pD3D9Device)
                            TransparentMenuDX9::RenderBlur();
                        else
                            TransparentMenuDX8::RenderBlur();
                    }
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