module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "d3d9.h"

export module MSAA;

import ComVars;

class MSAA
{
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
            WFP::onEndScene() += []()
            {
                if (!nHideAABug)
                    return;

                IDirect3DDevice9* device = Direct3DDevice;
                if (!device)
                    return;

                IDirect3DSurface9* target = nullptr;
                if (FAILED(device->GetRenderTarget(0, &target)) || !target)
                    return;

                D3DSURFACE_DESC desc = {};
                HRESULT hr = target->GetDesc(&desc);
                target->Release();

                if (FAILED(hr) || desc.MultiSampleType == D3DMULTISAMPLE_NONE || !desc.Width || !desc.Height)
                    return;

                const auto width = static_cast<LONG>(desc.Width);
                const auto height = static_cast<LONG>(desc.Height);
                const D3DRECT edges[]
                {
                    { 0, 0, width, 1 },
                    { 0, 0, 1, height },
                    { 0, height - 1, width, height },
                    { width - 1, 0, width, height },
                };
                device->Clear(nHideAABug == 2 ? 4 : 2, edges, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 0, 0), 1.0f, 0);
            };
        };
    }
} MSAA;
