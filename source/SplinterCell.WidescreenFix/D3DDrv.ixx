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

    // Skip redundant SetRes calls to avoid unnecessary device resets and screen flicker
    // Lost devices still fall through so D3D can recover
    IDirect3DDevice8* pExistingDevice = *(IDirect3DDevice8**)((uintptr_t)UD3DRenderDevice + 0x4684);
    if (pExistingDevice && width == Screen.Width && height == Screen.Height &&
        pExistingDevice->TestCooperativeLevel() == D3D_OK)
    {
        return 1;
    }

    auto ret = shUD3DRenderDeviceSetRes.unsafe_fastcall<int>(UD3DRenderDevice, edx, UViewport, width, height, a5);

    Screen.Width = width;
    Screen.Height = height;
    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.HUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fFMVoffsetStartX = (Screen.fWidth - 640.0f) / 2.0f;
    Screen.fFMVoffsetStartY = (Screen.fHeight - 480.0f) / 2.0f;
    constexpr auto HUD_OFFSET_X = 24.0f; // Default offset used in original scripts
    Screen.fWidescreenHudOffset = CalculateWidescreenOffset(Screen.fWidth, Screen.fHeight, 640.0f, 480.0f, HUD_OFFSET_X, Screen.nHudWidescreenMode == 1);

    if (Screen.fHudAspectRatioConstraint.has_value())
    {
        float value = Screen.fHudAspectRatioConstraint.value();
        if (value < 0.0f || value > (32.0f / 9.0f))
            Screen.fWidescreenHudOffset = value;
        else
        {
            value = ClampHudAspectRatio(value, Screen.fAspectRatio);
            Screen.fWidescreenHudOffset = CalculateWidescreenOffset(Screen.fHeight * value, Screen.fHeight, 640.0f, 480.0f, HUD_OFFSET_X, Screen.nHudWidescreenMode == 1);
        }
    }

    // Enhanced widescreen setting
    if (IsEnhanced() && Screen.nHudWidescreenMode == 2)
    {
        static bool bWidescreenOverrideRegistered = false;
        if (!bWidescreenOverrideRegistered)
        {
            UIntOverrides::Register(L"IntProperty Echelon.EchelonGameInfo.bWidescreenMode", +[]() -> int
            {
                // Dynamic check if user switches resolution while game is open
                return (Screen.fHudAspectRatioConstraint.has_value() ?
                        Screen.fHudAspectRatioConstraint.value() >= (3.0f / 2.0f) :
                        Screen.fAspectRatio >= (3.0f / 2.0f)) ? 1 : 0;
            });
            bWidescreenOverrideRegistered = true;
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

    if (pPresentParams->Windowed)
    {
        tagRECT rect;
        rect.left = (LONG)(((float)DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
        rect.top = (LONG)(((float)DesktopResH / 2.0f) - (Screen.fHeight / 2.0f));
        rect.right = (LONG)Screen.Width;
        rect.bottom = (LONG)Screen.Height;
        SetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE, GetWindowLong(pPresentParams->hDeviceWindow, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(pPresentParams->hDeviceWindow, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
        SetForegroundWindow(pPresentParams->hDeviceWindow);
        SetCursor(NULL);
    }

    return ret;
}

// High resolution Bink cutscene fix
typedef int      (__stdcall* BinkCopyToBuffer_t)(void* bink, void* dest, int destpitch, unsigned destheight, unsigned destx, unsigned desty, unsigned flags);
typedef void*    (__fastcall* PrepareTexture_t)(void* renderDevice, void* edx, int width, int height);

BinkCopyToBuffer_t   pBinkCopyToBuffer   = nullptr;
PrepareTexture_t     pPrepareTexture     = nullptr;

// Prevent crashes if BinkCopyToBufferRect runs after the Bink handle is freed.
SafetyHookInline shBinkCopyToBufferRect = {};

int __stdcall BinkCopyToBufferRectGuard(void* bink, void* dest, int destpitch, unsigned destheight,
                                        unsigned destx, unsigned desty, unsigned srcx, unsigned srcy,
                                        unsigned srcw, unsigned srch, unsigned flags)
{
    if (!bink || *(int*)bink <= 0 || *(int*)((uintptr_t)bink + 4) <= 0)
        return 0;

    return shBinkCopyToBufferRect.unsafe_stdcall<int>(bink, dest, destpitch, destheight, destx, desty,
                                                      srcx, srcy, srcw, srch, flags);
}

constexpr uintptr_t kCanvas_hBink             = 0x98;
constexpr uintptr_t kCanvas_IsPlaying         = 0x9C;
constexpr uintptr_t kCanvas_PlaybackTexture   = 0xB0;
constexpr uintptr_t kCanvas_VideoOnBackBuffer = 0xC8;
constexpr uintptr_t kCanvas_Viewport          = 0x90;
constexpr uintptr_t kViewport_Width           = 0x7C;
constexpr uintptr_t kViewport_Height          = 0x80;
constexpr uintptr_t kRenderDevice_VideoMode   = 0x5D58; // 0 = texture playback
constexpr uintptr_t kRenderDevice_VideoFrame  = 0x5D54;

IDirect3DTexture8* pHiResVideoTexture = nullptr;
int                nHiResVideoTexW    = 0;
int                nHiResVideoTexH    = 0;

int NextPow2(int v)
{
    int r = 1;
    while (r < v)
        r <<= 1;
    return r;
}

// Don't handle close/quit during a cutscene frame.
// Shutdown would free Bink mid-frame, so delay it until the cutscene ends.
void PumpMessagesDeferringClose()
{
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_CLOSE)
        {
            bShuttingDown = true;
            PostMessageA(msg.hwnd ? msg.hwnd : hGameWindow, WM_CLOSE, 0, 0);
            break;
        }
        if (msg.message == WM_QUIT)
        {
            bShuttingDown = true;
            PostQuitMessage(static_cast<int>(msg.wParam));
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

// Draw Bink frame using a custom texture with correct aspect ratio scaling
void DrawHiResCutsceneFrame(IDirect3DDevice8* pD3DDevice, void* UCanvas)
{
    // This keeps the window responsive and lets Alt+F4 take effect immediately instead of waiting for the video
    PumpMessagesDeferringClose();

    if (!pBinkCopyToBuffer)
        return;

    void* hBink = *(void**)((uintptr_t)UCanvas + kCanvas_hBink);
    if (!hBink)
        return;

    int videoWidth  = *(int*)hBink;
    int videoHeight = *(int*)((uintptr_t)hBink + 4);
    if (videoWidth <= 0 || videoHeight <= 0)
        return;

    // Recreate texture when video resolution changes
    if (!pHiResVideoTexture || nHiResVideoTexW != videoWidth || nHiResVideoTexH != videoHeight)
    {
        if (pHiResVideoTexture)
        {
            pHiResVideoTexture->Release();
            pHiResVideoTexture = nullptr;
        }
        if (FAILED(pD3DDevice->CreateTexture(videoWidth, videoHeight, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &pHiResVideoTexture)))
        {
            pHiResVideoTexture = nullptr;
            return;
        }
        nHiResVideoTexW = videoWidth;
        nHiResVideoTexH = videoHeight;
    }

    // Copy decoded Bink frame into texture
    D3DLOCKED_RECT lockedRect;
    if (SUCCEEDED(pHiResVideoTexture->LockRect(0, &lockedRect, nullptr, 0)))
    {
        pBinkCopyToBuffer(hBink, lockedRect.pBits, lockedRect.Pitch, (unsigned)videoHeight, 0, 0, 0x80000005u);
        pHiResVideoTexture->UnlockRect(0);
    }

    // Use viewport size (not back buffer) to handle windowed/borderless scaling
    int screenW = Screen.Width;
    int screenH = Screen.Height;
    void* pViewport = *(void**)((uintptr_t)UCanvas + kCanvas_Viewport);
    if (pViewport)
    {
        int viewportW = *(int*)((uintptr_t)pViewport + kViewport_Width);
        int viewportH = *(int*)((uintptr_t)pViewport + kViewport_Height);
        if (viewportW > 0 && viewportH > 0)
        {
            screenW = viewportW;
            screenH = viewportH;
        }
    }
    if (screenW <= 0 || screenH <= 0)
        return;

    float fScreenW = (float)screenW;
    float fScreenH = (float)screenH;

    float vTop = 0.0f;
    float vBottom = 1.0f;
    float contentHeight = (float)videoHeight;
    if (Screen.nFMVWidescreenMode)
    {
        int barPixels = (videoHeight - (videoWidth * 9) / 16) / 2;
        if (barPixels > 0)
        {
            contentHeight = (float)(videoHeight - 2 * barPixels);
            vTop = (float)barPixels / (float)videoHeight;
            vBottom = 1.0f - vTop;
        }
    }

    float videoAspect  = (float)videoWidth / contentHeight;
    float screenAspect = fScreenW / fScreenH;

    float targetW, targetH, targetX, targetY;
    if (screenAspect > videoAspect)
    {
        // Pillarbox
        targetH = fScreenH;
        targetW = targetH * videoAspect;
        targetX = (fScreenW - targetW) / 2.0f;
        targetY = 0.0f;
    }
    else
    {
        // Letterbox
        targetW = fScreenW;
        targetH = targetW / videoAspect;
        targetX = 0.0f;
        targetY = (fScreenH - targetH) / 2.0f;
    }

    pD3DDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    pD3DDevice->SetTexture(0, pHiResVideoTexture);

    struct Vertex { float x, y, z, rhw, u, v; };
    Vertex vertices[4] =
    {
        { targetX,           targetY,           0.0f, 1.0f, 0.0f, vTop    },
        { targetX + targetW, targetY,           0.0f, 1.0f, 1.0f, vTop    },
        { targetX,           targetY + targetH, 0.0f, 1.0f, 0.0f, vBottom },
        { targetX + targetW, targetY + targetH, 0.0f, 1.0f, 1.0f, vBottom },
    };
    pD3DDevice->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
    pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
}

namespace UD3DRenderDevice
{
    SafetyHookInline shGetShadowBufferResolutionWidth = {};
    int __fastcall GetShadowBufferResolutionWidth(void* self, void* edx)
    {
        int stock = shGetShadowBufferResolutionWidth.unsafe_fastcall<int>(self, edx);

        if (Screen.nShadowBufferResolution <= 0 || stock <= 0)
            return stock;

        int raw;

        if (Screen.nShadowBufferResolution == 1)
            raw = Screen.Width;
        else
            raw = static_cast<int>(Screen.nShadowBufferResolution);

        if (raw < stock)
            raw = stock;

        // Clamping to 4096x4096 for dgVoodoo 2 compatability for now
        //return raw > 16384 ? 16384 : raw;
        return raw > 4096 ? 4096 : raw;
    }

    SafetyHookInline shGetShadowBufferResolutionHeight = {};
    int __fastcall GetShadowBufferResolutionHeight(void* self, void* edx)
    {
        int stock = shGetShadowBufferResolutionHeight.unsafe_fastcall<int>(self, edx);

        if (Screen.nShadowBufferResolution <= 0 || stock <= 0)
            return stock;

        int raw;

        if (Screen.nShadowBufferResolution == 1)
            raw = Screen.Width;
        else
            raw = static_cast<int>(Screen.nShadowBufferResolution); // Default game used nShadowBufferResolution / 2 for height
        
        if (raw < stock)
            raw = stock;

        // Clamping to 4096x4096 for dgVoodoo 2 compatability for now
        //return raw > 16384 ? 16384 : raw;
        return raw > 4096 ? 4096 : raw;
    }

    SafetyHookInline shDisplayVideo = {};
    SafetyHookInline shStartVideo = {};
    SafetyHookInline shStopVideo = {};
    SafetyHookInline shUnlock = {};

    void __fastcall StartVideo(void* renderDevice, void* edx, void* UCanvas, int posX, int posY, int centered)
    {
        if (pPrepareTexture && bPlayingVideo && !bGadgetVideoIsPlaying &&
            *(uint32_t*)((uintptr_t)renderDevice + kRenderDevice_VideoMode) == 0)
        {
            void* hBink = *(void**)((uintptr_t)UCanvas + kCanvas_hBink);
            if (hBink)
            {
                int videoWidth  = *(int*)hBink;
                int videoHeight = *(int*)((uintptr_t)hBink + 4);

                // Non 640 videos use a small playback texture resize to prevent overflow
                if (videoWidth > 0 && videoHeight > 0 && videoWidth != 640)
                {
                    void* bigTexture = pPrepareTexture(renderDevice, nullptr,
                                                       NextPow2(videoWidth), NextPow2(videoHeight));
                    if (bigTexture)
                    {
                        *(void**)((uintptr_t)UCanvas + kCanvas_PlaybackTexture) = bigTexture;
                        *(uint32_t*)((uintptr_t)renderDevice + kRenderDevice_VideoFrame) = 0;
                    }
                }
            }
        }

        shStartVideo.unsafe_fastcall(renderDevice, edx, UCanvas, posX, posY, centered);
    }

    void __fastcall StopVideo(void* renderDevice, void* edx, void* UCanvas, int a3, int a4)
    {
        if (pHiResVideoTexture)
        {
            pHiResVideoTexture->Release();
            pHiResVideoTexture = nullptr;
            nHiResVideoTexW = 0;
            nHiResVideoTexH = 0;
        }
        shStopVideo.unsafe_fastcall(renderDevice, edx, UCanvas, a3, a4);
    }

    void __fastcall Unlock(void* renderDevice, void* edx, void* renderInterface)
    {
        // Lock() can fail during device loss, skip Unlock() if nothing was locked
        if (!renderInterface)
            return;

        shUnlock.unsafe_fastcall(renderDevice, edx, renderInterface);
    }

    void __fastcall DisplayVideo(void* UD3DRenderDevice, void* edx, void* UCanvas, void* a3)
    {
        // Exit cutscene loop cleanly during shutdown so the game can process WM_CLOSE
        if (bShuttingDown)
        {
            *(uint32_t*)((uintptr_t)UCanvas + kCanvas_IsPlaying) = 0;
            return;
        }

        // Exclusive fullscreen alt+tab can lose the D3D device
        // Skip cutscene rendering until the engine restores it
        if (!bGadgetVideoIsPlaying)
        {
            IDirect3DDevice8* pDevice = *(IDirect3DDevice8**)((uintptr_t)UD3DRenderDevice + 0x4684);
            if (!pDevice || pDevice->TestCooperativeLevel() != D3D_OK)
            {
                if (pHiResVideoTexture)
                {
                    pHiResVideoTexture->Release();
                    pHiResVideoTexture = nullptr;
                    nHiResVideoTexW = 0;
                    nHiResVideoTexH = 0;
                }
                return;
            }
        }

        // Skip if Bink handle is invalid
        void* hBink = *(void**)((uintptr_t)UCanvas + kCanvas_hBink);
        if (!hBink || *(int*)hBink <= 0 || *(int*)((uintptr_t)hBink + 4) <= 0)
            return;

        shDisplayVideo.unsafe_fastcall(UD3DRenderDevice, edx, UCanvas, a3);

        // Do not apply to gadget videos
        if (bGadgetVideoIsPlaying)
            return;

        IDirect3DDevice8* pD3DDevice = *(IDirect3DDevice8**)((uintptr_t)UD3DRenderDevice + 0x4684);
        if (!pD3DDevice)
            return;

        // Back-buffer path for standard 640x480 cutscenes
        if (*(uint32_t*)((uintptr_t)UCanvas + kCanvas_VideoOnBackBuffer) != 0)
        {
            IDirect3DSurface8* pBackBuffer = nullptr;

            if (SUCCEEDED(pD3DDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)))
            {
                D3DSURFACE_DESC desc;
                if (SUCCEEDED(pBackBuffer->GetDesc(&desc)))
                {
                    int W = desc.Width;
                    int H = desc.Height;

                    const int videoWidth  = 640;
                    const int videoHeight = 480;
                    const int srcX = 0;
                    const int srcY = 0;

                    // Clamp in case of an unusually small back buffer
                    const int captureW = (W < videoWidth)  ? W : videoWidth;
                    const int captureH = (H < videoHeight) ? H : videoHeight;

                    // Create a 640x480 temporary surface for the FMV region
                    IDirect3DSurface8* pTemp = nullptr;
                    if (SUCCEEDED(pD3DDevice->CreateImageSurface(videoWidth, videoHeight, desc.Format, &pTemp)))
                    {
                        // Copy only the 640x480 FMV region of the back buffer to temp
                        RECT srcRect = { 0, 0, captureW, captureH };
                        POINT pt = { 0, 0 };
                        pD3DDevice->CopyRects(pBackBuffer, &srcRect, 1, pTemp, &pt);

                        // Clear the back buffer to black
                        pD3DDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

                        // Create a 640x480 texture from the temp surface
                        IDirect3DTexture8* pTexture = nullptr;
                        if (SUCCEEDED(pD3DDevice->CreateTexture(videoWidth, videoHeight, 1, 0, desc.Format, D3DPOOL_MANAGED, &pTexture)))
                        {
                            IDirect3DSurface8* pTexSurf = nullptr;
                            if (SUCCEEDED(pTexture->GetSurfaceLevel(0, &pTexSurf)))
                            {
                                // Copy temp to texture surface using CopyRects (corrected direction)
                                pD3DDevice->CopyRects(pTemp, nullptr, 0, pTexSurf, &pt);
                                pTexSurf->Release();

                                // Set up render states for 2D drawing
                                pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
                                pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                                pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                                pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                                pD3DDevice->SetTexture(0, pTexture);

                                // Add for smoother stretching
                                pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
                                pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);

                                // Define vertices for a quad (TL, TR, BL, BR)
                                struct Vertex
                                {
                                    float x, y, z, rhw;
                                    float u, v;
                                };

                                float targetW, targetH, targetX, targetY;

                                if (Screen.nFMVWidescreenMode)
                                {
                                    const int barPixels = 60;
                                    int innerHeight = videoHeight - 2 * barPixels;
                                    float contentAspect = (float)videoWidth / (float)innerHeight;
                                    float screenAspect = (float)Screen.Width / (float)Screen.Height;

                                    if (screenAspect > contentAspect)
                                    {
                                        // Pillarbox: fit to height
                                        targetH = (float)Screen.Height;
                                        targetW = targetH * contentAspect;
                                        targetX = ((float)Screen.Width - targetW) / 2.0f;
                                        targetY = 0.0f;
                                    }
                                    else
                                    {
                                        // Letterbox: fit to width
                                        targetW = (float)Screen.Width;
                                        targetH = targetW / contentAspect;
                                        targetX = 0.0f;
                                        targetY = ((float)Screen.Height - targetH) / 2.0f;
                                    }

                                    // UVs for cropped inner content (texture is exactly 640x480)
                                    float uLeft = (float)srcX / (float)videoWidth;
                                    float uRight = (float)(srcX + videoWidth) / (float)videoWidth;
                                    float vTop = (float)(srcY + barPixels) / (float)videoHeight;
                                    float vBottom = (float)(srcY + barPixels + innerHeight) / (float)videoHeight;

                                    Vertex vertices[4] = {
                                        { targetX, targetY, 0.0f, 1.0f, uLeft, vTop },
                                        { targetX + targetW, targetY, 0.0f, 1.0f, uRight, vTop },
                                        { targetX, targetY + targetH, 0.0f, 1.0f, uLeft, vBottom },
                                        { targetX + targetW, targetY + targetH, 0.0f, 1.0f, uRight, vBottom }
                                    };

                                    // Draw the quad
                                    pD3DDevice->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
                                    pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
                                }
                                else // Original pillarbox without cropping (includes bars)
                                {
                                    // Pillarboxing
                                    targetW = (float)(Screen.Height * videoWidth / videoHeight);
                                    targetH = (float)Screen.Height;
                                    targetX = (float)((Screen.Width - (int)targetW) / 2);
                                    targetY = 0.0f;

                                    float uLeft = (float)srcX / (float)videoWidth;
                                    float uRight = (float)(srcX + videoWidth) / (float)videoWidth;
                                    float vTop = (float)srcY / (float)videoHeight;
                                    float vBottom = (float)(srcY + videoHeight) / (float)videoHeight;

                                    Vertex vertices[4] = {
                                        { targetX, targetY, 0.0f, 1.0f, uLeft, vTop },
                                        { targetX + targetW, targetY, 0.0f, 1.0f, uRight, vTop },
                                        { targetX, targetY + targetH, 0.0f, 1.0f, uLeft, vBottom },
                                        { targetX + targetW, targetY + targetH, 0.0f, 1.0f, uRight, vBottom }
                                    };

                                    // Draw the quad
                                    pD3DDevice->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
                                    pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
                                }

                                if (targetX > 0.0f)
                                {
                                    D3DRECT leftRect = { 0, 0, (LONG)targetX, H };
                                    pD3DDevice->Clear(1, &leftRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                                }

                                // Clear right side
                                if (targetX + targetW < (float)W)
                                {
                                    D3DRECT rightRect = { (LONG)(targetX + targetW), 0, W, H };
                                    pD3DDevice->Clear(1, &rightRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                                }

                                // Clear top side
                                if (targetY > 0.0f)
                                {
                                    D3DRECT topRect = { 0, 0, W, (LONG)targetY };
                                    pD3DDevice->Clear(1, &topRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                                }

                                // Clear bottom side
                                if (targetY + targetH < (float)H)
                                {
                                    D3DRECT bottomRect = { 0, (LONG)(targetY + targetH), W, H };
                                    pD3DDevice->Clear(1, &bottomRect, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
                                }
                            }
                            pTexture->Release();
                        }
                        pTemp->Release();
                    }
                }
                pBackBuffer->Release();
            }
        }
        else if (bPlayingVideo)
        {
            DrawHiResCutsceneFrame(pD3DDevice, UCanvas);
        }
    }
}

export void InitD3DDrv()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 44 24 04 81 EC 80 00 00 00");
    shUD3DRenderDeviceSetRes = safetyhook::create_inline(pattern.get_first(), UD3DRenderDeviceSetRes);
    pPresentParams = *hook::module_pattern(GetModuleHandle(L"D3DDrv"), "BF ? ? ? ? 33 C0 8B D9 C1 E9 02 83 E3 03").get_first<D3DPRESENT_PARAMETERS*>(1);

    //FMV
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "50 8B CF FF 55");
    static auto StartVideoHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto& width = *(uint32_t*)(regs.esp + 0);
        auto& height = *(uint32_t*)(regs.esp + 4);

        width = Screen.Width;
        height = Screen.Height;
    });

    static auto SetFMVPos = [](SafetyHookContext& regs)
    {
        PumpMessagesDeferringClose();

        auto& dest_height = *(uint32_t*)(regs.esp + 12);
        auto& dest_x = *(uint32_t*)(regs.esp + 16);
        auto& dest_y = *(uint32_t*)(regs.esp + 20);

        dest_x = 0;
        dest_y = (480 - dest_height) / 2;
    };

    //pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 85 C0 74 ? 8B 86");
    //static auto BinkCopyToBufferHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    //{
    //    SetFMVPos(regs);
    //});
    //
    //pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 85 C0 75 ? 8B 83");
    //static auto BinkCopyToBufferHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    //{
    //    SetFMVPos(regs);
    //});

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 85 C0 74 ? 8B 96");
    static auto BinkCopyToBufferHook3 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        SetFMVPos(regs);
    });

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "FF 15 ? ? ? ? 85 C0 75 ? 8B 44 24 ? 50");
    static auto BinkCopyToBufferHook4 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        SetFMVPos(regs);
    });

    UD3DRenderDevice::shDisplayVideo = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?DisplayVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@PAX@Z"), UD3DRenderDevice::DisplayVideo);

    UD3DRenderDevice::shGetShadowBufferResolutionWidth = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?GetShadowBufferResolutionWidth@UD3DRenderDevice@@UAEHXZ"), UD3DRenderDevice::GetShadowBufferResolutionWidth);
    UD3DRenderDevice::shGetShadowBufferResolutionHeight = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?GetShadowBufferResolutionHeight@UD3DRenderDevice@@UAEHXZ"), UD3DRenderDevice::GetShadowBufferResolutionHeight);
    
    // High resolution Bink cutscene fix
    pPrepareTexture = (PrepareTexture_t)GetProcAddress(GetModuleHandle(L"D3DDrv"), "?PrepareTexture@UD3DRenderDevice@@QAEPAVUTexture@@HH@Z");
    if (HMODULE hBinkModule = GetModuleHandle(L"binkw32"))
    {
        pBinkCopyToBuffer = (BinkCopyToBuffer_t)GetProcAddress(hBinkModule, "_BinkCopyToBuffer@28");

        // Prevent crash during shutdown while a video is still playing
        if (auto pBinkRect = GetProcAddress(hBinkModule, "_BinkCopyToBufferRect@44"))
            shBinkCopyToBufferRect = safetyhook::create_inline(pBinkRect, BinkCopyToBufferRectGuard);
    }
    UD3DRenderDevice::shStartVideo = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?StartVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@HHH@Z"), UD3DRenderDevice::StartVideo);
    UD3DRenderDevice::shStopVideo = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?StopVideo@UD3DRenderDevice@@UAEXPAVUCanvas@@HH@Z"), UD3DRenderDevice::StopVideo);
    UD3DRenderDevice::shUnlock = safetyhook::create_inline(GetProcAddress(GetModuleHandle(L"D3DDrv"), "?Unlock@UD3DRenderDevice@@UAEXPAVFRenderInterface@@@Z"), UD3DRenderDevice::Unlock);

    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "8B 10 FF 92 ? ? ? ? 8B 86 14 5D 00 00");
    static auto EndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (bDisplayingSplash)
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
                D3DRECT rightEdge = { (LONG)floor(targetX + targetW) - 1, 0, (LONG)floor(targetX + targetW), H };
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

    // Fix night vision grain scaling at high resolutions
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "D9 5C 24 14 FF 15 ? ? ? ? D9 5C 24 10");
    if (!pattern.empty())
    {
        static auto NightVisionGrainHook = safetyhook::create_mid(pattern.get_first(4), [](SafetyHookContext& regs)
        {
            uint32_t height = *(uint32_t*)(regs.esp + 0xCD0);
            if (height == 0)
                return;

            float scale = 480.0f / static_cast<float>(height);
            *(float*)(regs.esp + 0x14) *= scale;
            *(float*)(regs.esp + 0x28) *= scale;
        });
    }
}
