module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "d3d9.h"

export module MSAA;

import ComVars;

class MSAA
{
    static bool IsMSAAEnabled(IDirect3DDevice9* device)
    {
        if (!device)
            return false;

        IDirect3DSurface9* pRT = nullptr;
        if (SUCCEEDED(device->GetRenderTarget(0, &pRT)) && pRT)
        {
            D3DSURFACE_DESC desc = {};
            HRESULT hr = pRT->GetDesc(&desc);
            pRT->Release();
            if (SUCCEEDED(hr) && desc.MultiSampleType != D3DMULTISAMPLE_NONE)
                return true;
        }

        IDirect3DSwapChain9* pSwapChain = nullptr;
        if (SUCCEEDED(device->GetSwapChain(0, &pSwapChain)) && pSwapChain)
        {
            D3DPRESENT_PARAMETERS d3dpp = {};
            HRESULT hr = pSwapChain->GetPresentParameters(&d3dpp);
            pSwapChain->Release();
            if (SUCCEEDED(hr) && d3dpp.MultiSampleType != D3DMULTISAMPLE_NONE)
                return true;
        }

        DWORD dwMSAA = 0;
        if (SUCCEEDED(device->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS, &dwMSAA)))
            return dwMSAA != 0;

        return false;
    }

public:
    MSAA()
    {
        WFP::onInitEvent() += []()
        {
            // The game creates its device with the saved FSAA level and only clamps it to what the device supports later.
            // A higher level (e.g. the default 7) fails the MSAA device and the game turns FSAA off; clamp it first.
            auto pattern = hook::pattern("53 57 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 85 C0 74 ? 39 35 ? ? ? ? 7E ? 53 57 89 35 ? ? ? ? 89 35");
            static int32_t* pFSAALevel = *pattern.get_first<int32_t*>(21);
            static int32_t* pMaxFSAALevel = *pattern.get_first<int32_t*>(37);
            static auto FSAALevelHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (*pFSAALevel > *pMaxFSAALevel)
                    *pFSAALevel = (std::max)(*pMaxFSAALevel, 0);
            });

            static auto nHideAABug = std::clamp(FixSettings().Int("GRAPHICS.HideAABug"), 0, 2);
            static bool bMSAAEnabled = false;
            static bool bMSAAChecked = false;

            WFP::onBeforeReset() += []()
            {
                bMSAAEnabled = false;
                bMSAAChecked = false;
            };

            WFP::onEndScene() += []()
            {
                if (!nHideAABug)
                    return;

                IDirect3DDevice9* device = Direct3DDevice;
                if (!device)
                    return;

                if (!bMSAAChecked)
                {
                    bMSAAEnabled = IsMSAAEnabled(device);
                    bMSAAChecked = true;
                }

                if (!bMSAAEnabled)
                    return;

                IDirect3DSurface9* pBackBuffer = nullptr;
                if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)) || !pBackBuffer)
                    return;

                D3DSURFACE_DESC desc = {};
                HRESULT hr = pBackBuffer->GetDesc(&desc);
                pBackBuffer->Release();

                if (FAILED(hr) || !desc.Width || !desc.Height)
                    return;

                D3DCOLOR black = D3DCOLOR_ARGB(255, 0, 0, 0);

                // Top 1px
                D3DRECT topRect = { 0, -5, static_cast<LONG>(desc.Width), 1 };
                device->Clear(1, &topRect, D3DCLEAR_TARGET, black, 1.0f, 0);

                // Left 1px
                D3DRECT leftRect = { -5, 0, 1, static_cast<LONG>(desc.Height) };
                device->Clear(1, &leftRect, D3DCLEAR_TARGET, black, 1.0f, 0);

                if (nHideAABug > 1)
                {
                    // Bottom 1px
                    D3DRECT bottomRect = { 0, static_cast<LONG>(desc.Height) - 1, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height) + 5 };
                    device->Clear(1, &bottomRect, D3DCLEAR_TARGET, black, 1.0f, 0);

                    // Right 1px
                    D3DRECT rightRect = { static_cast<LONG>(desc.Width) - 1, 0, static_cast<LONG>(desc.Width) + 5, static_cast<LONG>(desc.Height) + 5 };
                    device->Clear(1, &rightRect, D3DCLEAR_TARGET, black, 1.0f, 0);
                }
            };
        };
    }
} MSAA;
