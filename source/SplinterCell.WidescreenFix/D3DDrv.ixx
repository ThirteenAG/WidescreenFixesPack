module;

#include <stdafx.h>
#include "../Shared/SplinterCell/Window.hpp"
#include "FixSettings.hpp"
#include "MovieFrame.hpp"
#include <minidx8/d3d8.h>

export module D3DDrv;

import ComVars;
import Xidi;

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

    // Skip redundant SetRes calls to avoid unnecessary device resets and screen flicker
    // Lost devices still fall through so D3D can recover
    IDirect3DDevice8* pExistingDevice = *(IDirect3DDevice8**)((uintptr_t)UD3DRenderDevice + 0x4684);
    if (pExistingDevice && pPresentParams && pPresentParams->Windowed == !a5 &&
        width == Screen.Width && height == Screen.Height &&
        pExistingDevice->TestCooperativeLevel() == D3D_OK)
    {
        return 1;
    }

    auto ret = shUD3DRenderDeviceSetRes.unsafe_fastcall<int>(UD3DRenderDevice, edx, UViewport, width, height, a5);
    if (!ret || !pPresentParams)
        return ret;

    if (nativeResolution && UViewport)
    {
        // Fullscreen may select the closest supported mode. Use what native resize published.
        const int actualWidth = *reinterpret_cast<int*>(static_cast<uint8_t*>(UViewport) + 0x7C);
        const int actualHeight = *reinterpret_cast<int*>(static_cast<uint8_t*>(UViewport) + 0x80);
        if (actualWidth > 0 && actualHeight > 0 && actualWidth <= 16384 && actualHeight <= 16384)
        {
            width = actualWidth;
            height = actualHeight;
        }
    }
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
    SCWindow::Track(pPresentParams->hDeviceWindow, mode, pPresentParams->Windowed != FALSE, Screen.Width, Screen.Height);
    SCWindow::SquareCorners(pPresentParams->hDeviceWindow);

    if (nativeResolution)
    {
        FixSettings().Set("MAIN.ResX", std::to_string(Screen.Width), true);
        FixSettings().Set("MAIN.ResY", std::to_string(Screen.Height), true);
    }

    if (Screen.Width < 640 || Screen.Height < 480)
        return ret;

    SettingsReader iniReader;
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
        SCWindow::Place(pPresentParams->hDeviceWindow, Screen.Width, Screen.Height, mode == 2);
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
void* pPendingVideoCanvas = nullptr;

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
    if (!pBinkCopyToBuffer || !UCanvas || !*reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(UCanvas) + kCanvas_IsPlaying))
        return;

    void* hBink = *(void**)((uintptr_t)UCanvas + kCanvas_hBink);
    if (!hBink)
        return;

    int videoWidth  = *(int*)hBink;
    int videoHeight = *(int*)((uintptr_t)hBink + 4);
    if (videoWidth <= 0 || videoHeight <= 0 || videoWidth > 16384 || videoHeight > 16384)
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

    D3DLOCKED_RECT lockedRect;
    if (FAILED(pHiResVideoTexture->LockRect(0, &lockedRect, nullptr, 0)))
        return;
    pBinkCopyToBuffer(hBink, lockedRect.pBits, lockedRect.Pitch, (unsigned)videoHeight, 0, 0, 0x80000005u);
    pHiResVideoTexture->UnlockRect(0);

    IDirect3DSurface8* target = nullptr;
    if (FAILED(pD3DDevice->GetRenderTarget(&target)))
        return;
    D3DSURFACE_DESC targetDesc{};
    const auto described = target->GetDesc(&targetDesc);
    target->Release();
    if (FAILED(described) || !targetDesc.Width || !targetDesc.Height)
        return;
    // Windowed presentation crops the desktop-sized buffer to this viewport.
    auto viewport = *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(UCanvas) + kCanvas_Viewport);
    if (!viewport)
        return;
    const int screenW = *reinterpret_cast<int*>(viewport + kViewport_Width);
    const int screenH = *reinterpret_cast<int*>(viewport + kViewport_Height);
    if (screenW <= 0 || screenH <= 0 || screenW > int(targetDesc.Width) || screenH > int(targetDesc.Height))
        return;

    DWORD state = 0;
    D3DVIEWPORT8 savedViewport{};
    if (FAILED(pD3DDevice->GetViewport(&savedViewport)) ||
        FAILED(pD3DDevice->CreateStateBlock(D3DSBT_ALL, &state)))
        return;
    struct RestoreState
    {
        IDirect3DDevice8* device;
        DWORD state;
        D3DVIEWPORT8 viewport;
        ~RestoreState()
        {
            device->ApplyStateBlock(state);
            device->SetViewport(&viewport);
            device->DeleteStateBlock(state);
        }
    } restore{ pD3DDevice, state, savedViewport };
    D3DVIEWPORT8 movieViewport{ 0, 0, DWORD(screenW), DWORD(screenH), 0.0f, 1.0f };
    pD3DDevice->SetViewport(&movieViewport);
    pD3DDevice->SetPixelShader(0);
    pD3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pD3DDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    pD3DDevice->SetRenderState(D3DRS_WRAP0, 0);
    pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pD3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
    pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pD3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
    pD3DDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    pD3DDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
    pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

    const auto frame = SCMovie::Layout(screenW, screenH, videoWidth, videoHeight,
        FixSettings().Int("MAIN.FMVWidescreenMode"));
    const float targetW = frame.width, targetH = frame.height, targetX = frame.x, targetY = frame.y;
    const float vTop = frame.top, vBottom = frame.bottom;

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
        { targetX,           targetY,           0.0f, 1.0f, frame.left, vTop    },
        { targetX + targetW, targetY,           0.0f, 1.0f, frame.right, vTop    },
        { targetX,           targetY + targetH, 0.0f, 1.0f, frame.left, vBottom },
        { targetX + targetW, targetY + targetH, 0.0f, 1.0f, frame.right, vBottom },
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

        // Clamping to 4096x4096 for now
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

        // Clamping to 4096x4096 for now
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
                if (videoWidth > 0 && videoHeight > 0 && videoWidth <= 16384 && videoHeight <= 16384 && videoWidth != 640)
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
        pPendingVideoCanvas = nullptr;
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

        if (!bGadgetVideoIsPlaying)
            PumpMessagesDeferringClose();
        shDisplayVideo.unsafe_fastcall(UD3DRenderDevice, edx, UCanvas, a3);

        // Do not apply to gadget videos
        if (bGadgetVideoIsPlaying)
            return;

        IDirect3DDevice8* pD3DDevice = *(IDirect3DDevice8**)((uintptr_t)UD3DRenderDevice + 0x4684);
        if (!pD3DDevice)
            return;

        if (bPlayingVideo)
        {
            // Native scene rendering follows decoding; publish only after it flushes.
            pPendingVideoCanvas = UCanvas;
        }
    }
}

// Light passes: spot lights project their light texture (stage 1) and buffered shadows their depth texture (stage 0)
// with homogeneous texture coordinates from the vertex shader. Drivers of the time divided by w on their own;
// current ones only do it with D3DTTFF_PROJECTED, and the vertex shader path always sets 0, so the spot light
// and its shadow come out black. Remember which stages each light vertex shader projects and set the flag for them.
namespace FD3DRenderInterface
{
    std::unordered_map<void*, uint32_t> ProjectedStages;

    SafetyHookInline shSetupLightPass = {};
    int __fastcall SetupLightPass(void* self, void* edx, void* lightInfo)
    {
        auto lightFlags = *(uint32_t*)(*(uintptr_t*)(*(uintptr_t*)(*(uintptr_t*)((uintptr_t)lightInfo + 0x4C) + 0x48) + 0xC8) + 0x7C);
        auto lightType = (lightFlags >> 2) & 3;
        bool bShadowBuffer = (lightFlags & 0x10) != 0;

        auto ret = shSetupLightPass.unsafe_fastcall<int>(self, edx, lightInfo);

        if (auto pVertexShader = *(void**)((uintptr_t)self + 0x410))
            ProjectedStages[pVertexShader] = (bShadowBuffer ? 1 : 0) | (lightType < 2 ? 2 : 0);

        return ret;
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
        if (auto canvas = std::exchange(pPendingVideoCanvas, nullptr);
            canvas && bPlayingVideo && !bGadgetVideoIsPlaying && !bShuttingDown)
        {
            auto device = reinterpret_cast<IDirect3DDevice8*>(regs.eax);
            if (device && device->TestCooperativeLevel() == D3D_OK)
                DrawHiResCutsceneFrame(device, canvas);
        }
        UpdateXidiProfile();

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

    // Light passes: projected light and shadow buffer stages (see FD3DRenderInterface above)
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "83 EC 38 53 55 56 57 8B F1 6A 00 6A 00 6A 00 8B 06 6A 00 FF 50 3C 8B 5C 24 4C 8B 43 4C");
    if (!pattern.empty())
    {
        FD3DRenderInterface::shSetupLightPass = safetyhook::create_inline(pattern.get_first(), FD3DRenderInterface::SetupLightPass);

        // State upload, vertex shader path: texture transform flags of every stage are set to 0 here
        pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "6A 00 6A 18 53 8B 82 84 46 00 00 50 8B 08 FF 91 FC 00 00 00");
        if (!pattern.empty())
        {
            static auto ProjectedStagesHook = safetyhook::create_mid(pattern.get_first(14), [](SafetyHookContext& regs)
            {
                auto stage = regs.ebx;
                auto pState = regs.edi - (48 + 38 * stage) * sizeof(uint32_t);
                auto pVertexShader = *(void**)pState;
                if (!pVertexShader)
                    pVertexShader = *(void**)(regs.esi + 69 * sizeof(uint32_t));

                auto it = FD3DRenderInterface::ProjectedStages.find(pVertexShader);
                if (it != FD3DRenderInterface::ProjectedStages.end() && (it->second & (1 << stage)))
                    *(DWORD*)(regs.esp + 12) = D3DTTFF_PROJECTED;
            });
        }
    }

    // Fix night vision grain scaling and FPS-dependent noise animation
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "D9 5C 24 14 FF 15 ? ? ? ? D9 5C 24 10 FF 15 ? ? ? ? D8 4C 24 14 D9 5C 24 24 FF 15 ? ? ? ? D8 4C 24 28");
    if (!pattern.empty())
    {
        auto base = reinterpret_cast<uintptr_t>(pattern.get_first(0));

        // Redirect call sites before creating the mid hook on the first call site
        if (VisionNoiseGrain::Init())
        {
            VisionNoiseGrain::RedirectCallSite(base + 0x04, VisionNoiseGrain::HeldFrand<0, 0>); // rotation angle
            VisionNoiseGrain::RedirectCallSite(base + 0x0E, VisionNoiseGrain::HeldFrand<0, 1>); // V offset
            VisionNoiseGrain::RedirectCallSite(base + 0x1C, VisionNoiseGrain::HeldFrand<0, 2>); // U offset
        }

        static auto NightVisionGrainHook = safetyhook::create_mid(reinterpret_cast<void*>(base + 0x04), [](SafetyHookContext& regs)
        {
            if (Screen.fGrainScale == 0.0f)
                return;

            uint32_t height = *(uint32_t*)(regs.esp + 0xCD0);
            if (height == 0)
                return;

            float fullScale = 480.0f / static_cast<float>(height);
            float blendedScale = 1.0f + (fullScale - 1.0f) * Screen.fGrainScale;
            *(float*)(regs.esp + 0x14) *= blendedScale;
            *(float*)(regs.esp + 0x28) *= blendedScale;
        });
    }

    // Fixed-function vision path for legacy GPUs like GeForce 2 (or EmulateGF2Mode=1 in SplinterCell.ini)
    pattern = hook::module_pattern(GetModuleHandle(L"D3DDrv"), "D9 5C 24 28 FF 15 ? ? ? ? D9 5C 24 1C FF 15 ? ? ? ? D8 4C 24 28 D9 5C 24 7C FF 15 ? ? ? ? D8 4C 24 14");
    if (!pattern.empty() && VisionNoiseGrain::Init())
    {
        auto base = reinterpret_cast<uintptr_t>(pattern.get_first(0));
        VisionNoiseGrain::RedirectCallSite(base + 0x04, VisionNoiseGrain::HeldFrand<1, 0>); // rotation angle
        VisionNoiseGrain::RedirectCallSite(base + 0x0E, VisionNoiseGrain::HeldFrand<1, 1>); // V offset
        VisionNoiseGrain::RedirectCallSite(base + 0x1C, VisionNoiseGrain::HeldFrand<1, 2>); // U offset
    }
}
