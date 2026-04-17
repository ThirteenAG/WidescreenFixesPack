module;

#include <stdafx.h>
#include <d3d9.h>

export module UIScissor;

import ComVars;
import Resolution;

void StartHUDScissorTest(IDirect3DDevice9* pDevice, float safeAspectRatio = (4.0f / 3.0f), UINT forcedWidth = 0, UINT forcedHeight = 0)
{
    if (!pDevice) return;

    UINT screenWidth = 0;
    UINT screenHeight = 0;

    if (forcedWidth > 0 && forcedHeight > 0)
    {
        screenWidth = forcedWidth;
        screenHeight = forcedHeight;
    }
    else
    {
        IDirect3DSurface9* pBackBuffer = nullptr;
        if (SUCCEEDED(pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)) && pBackBuffer)
        {
            D3DSURFACE_DESC desc = {};
            pBackBuffer->GetDesc(&desc);
            screenWidth = desc.Width;
            screenHeight = desc.Height;
            pBackBuffer->Release();
        }
        else
        {
            D3DVIEWPORT9 vp = {};
            pDevice->GetViewport(&vp);
            screenWidth = vp.Width;
            screenHeight = vp.Height;
        }
    }

    if (screenWidth == 0 || screenHeight == 0)
        return;

    RECT scissorRect = { 0, 0, (LONG)screenWidth, (LONG)screenHeight };

    float currentAspect = (float)screenWidth / (float)screenHeight;

    if (currentAspect > safeAspectRatio)
    {
        LONG desiredWidth = (LONG)(screenHeight * safeAspectRatio + 0.5f);
        LONG marginX = ((LONG)screenWidth - desiredWidth) / 2;

        scissorRect.left = marginX;
        scissorRect.right = (LONG)screenWidth - marginX;
    }
    else if (currentAspect < safeAspectRatio)
    {
        LONG desiredHeight = (LONG)(screenWidth / safeAspectRatio + 0.5f);
        LONG marginY = ((LONG)screenHeight - desiredHeight) / 2;

        scissorRect.top = marginY;
        scissorRect.bottom = (LONG)screenHeight - marginY;
    }

    pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    pDevice->SetScissorRect(&scissorRect);
}

void EndHUDScissorTest(IDirect3DDevice9* pDevice)
{
    if (!pDevice) return;

    pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
}

class UIScissor
{
public:
    UIScissor()
    {
        WFP::onInitEventAsync() += []()
        {
            auto pattern = hook::pattern("A1 ? ? ? ? 74 ? 85 C0");
            static auto ScissorTest1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (nGameState == 3 && !MovieIsPlaying())
                {
                    auto [Width, Height] = GetRes();
                    StartHUDScissorTest(Direct3DDevice, 4.0f / 3.0f, Width, Height);
                }
            });

            pattern = hook::pattern("E9 ? ? ? ? 85 C0 74 ? A1");
            static auto ScissorTestRestore1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (nGameState == 3 && !MovieIsPlaying())
                    EndHUDScissorTest(Direct3DDevice);
            });

            pattern = find_pattern("A1 ? ? ? ? 33 F6 85 C0 7E ? 8D 49", "A1 ? ? ? ? 33 F6 85 C0 7E ? EB ? 8D A4 24 ? ? ? ? 90 8B 04 B5 ? ? ? ? 8B 80");
            static auto ScissorTestRestore2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (nGameState == 3 && !MovieIsPlaying())
                    EndHUDScissorTest(Direct3DDevice);
            });
        };
    }
} UIScissor;