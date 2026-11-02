module;

#include <stdafx.h>
#include <d3d9.h>

export module UIScissor;

import ComVars;
import Resolution;
import Frontend;

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
    // A resizable window can display a backbuffer with a different aspect ratio.
    // Convert the intended client-area aspect to backbuffer coordinates before clipping.
    auto [clientWidth, clientHeight] = GetRes();
    if (clientWidth > 0 && clientHeight > 0)
        safeAspectRatio *= currentAspect / (float(clientWidth) / float(clientHeight));

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

namespace
{
    bool RestoreBackdropScissor = false;
    RECT BackdropScissorRect = {};
}

class UIScissor
{
public:
    UIScissor()
    {
        WFP::onInitEventAsync() += []()
        {
            {
                // The renderer draws its depth-sorted images here. Only the full-screen gradient
                // images leave the menu clip; all other objects retain the original scissor.
                auto drawObject = hook::pattern("8B 45 08 E8 ? ? ? ? 8B 6D 00 3B EB 75 ? 8B 35 ? ? ? ?");
                static auto BackdropScissorBegin = safetyhook::create_mid(drawObject.get_first(3), [](SafetyHookContext& regs)
                {
                    RestoreBackdropScissor = false;
                    if (IsFrontEnd() && !MovieIsPlaying() && IsScreenBackdropImage(reinterpret_cast<FEObject*>(regs.eax)) && Direct3DDevice)
                    {
                        DWORD enabled = FALSE;
                        if (SUCCEEDED(Direct3DDevice->GetRenderState(D3DRS_SCISSORTESTENABLE, &enabled)) && enabled &&
                            SUCCEEDED(Direct3DDevice->GetScissorRect(&BackdropScissorRect)))
                        {
                            RestoreBackdropScissor = true;
                            EndHUDScissorTest(Direct3DDevice);
                        }
                    }
                });
                static auto BackdropScissorEnd = safetyhook::create_mid(drawObject.get_first(8), [](SafetyHookContext&)
                {
                    if (RestoreBackdropScissor)
                    {
                        Direct3DDevice->SetScissorRect(&BackdropScissorRect);
                        Direct3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
                        RestoreBackdropScissor = false;
                    }
                });

                auto pattern = hook::pattern("A1 ? ? ? ? 74 ? 85 C0");
                static auto ScissorTest1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (IsFrontEnd() && !MovieIsPlaying())
                    {
                        // The backbuffer's size: in the scaled window mode the window is larger
                        StartHUDScissorTest(Direct3DDevice, 4.0f / 3.0f);
                    }
                });

                pattern = hook::pattern("E9 ? ? ? ? 85 C0 74 ? A1");
                static auto ScissorTestRestore1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (IsFrontEnd() && !MovieIsPlaying())
                        EndHUDScissorTest(Direct3DDevice);
                });

                pattern = find_pattern("A1 ? ? ? ? 33 F6 85 C0 7E ? 8D 49", "A1 ? ? ? ? 33 F6 85 C0 7E ? EB ? 8D A4 24 ? ? ? ? 90 8B 04 B5 ? ? ? ? 8B 80");
                static auto ScissorTestRestore2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
                {
                    if (IsFrontEnd() && !MovieIsPlaying())
                        EndHUDScissorTest(Direct3DDevice);
                });
            }
        };
    }
} UIScissor;
