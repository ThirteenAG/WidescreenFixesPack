#pragma once
#include <minidx8/d3d8.h>
#include "../CursorConfinement.hpp"
#include <functional>
#include <mutex>
#include <chrono>
#include <thread>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")

// Display handling shared by the Silent Hill 3 and 4 fixes (both Direct3D 8).
//
// The game creates its device as usual; the device creation and reset are intercepted so that the game runs in a
// borderless window covering the monitor, in a normal window, or in exclusive full screen. Alt+Enter switches
// fullscreen and the last window mode through each game's resource-aware reset routine. The game's own window style and position
// changes are ignored while the fix manages the window. The module also gives the other modules device events
// and an accurate frame limiter that replaces the games' coarse Sleep based pacing.
namespace SHDisplay
{
    enum class Mode : int
    {
        Fullscreen = 0,
        Borderless = 1,
        Windowed = 2,
    };

    struct State
    {
        Mode mode = Mode::Borderless;
        Mode lastWindowMode = Mode::Windowed;
        RECT windowRect{};
        bool haveWindowRect = false;
        bool changingMode = false;
        HWND window = nullptr;
        IDirect3DDevice8* device = nullptr;
        UINT backBufferWidth = 0;
        UINT backBufferHeight = 0;
        double frameLimit = 0.0;            // frames per second, 0 = off
        int anisotropy = 0;                 // world textures, 0 = the game's filtering
        int requestedAnisotropy = 0;        // the setting, before the device's limit
        int maxAnisotropy = -1;             // the device's limit, -1 until a device exists
        bool managingWindow = false;
        bool followGame = false;            // the game's own window/full screen choice picks Windowed/Borderless
    };

    inline State& Get()
    {
        static State state;
        return state;
    }

    // Events for the game modules
    inline std::vector<std::function<void(IDirect3DDevice8*)>>& OnDeviceReady()      // created or reset
    {
        static std::vector<std::function<void(IDirect3DDevice8*)>> handlers;
        return handlers;
    }
    inline std::vector<std::function<void(IDirect3DDevice8*)>>& OnBeforeReset()
    {
        static std::vector<std::function<void(IDirect3DDevice8*)>> handlers;
        return handlers;
    }
    inline std::vector<std::function<void(IDirect3DDevice8*)>>& OnEndScene()
    {
        static std::vector<std::function<void(IDirect3DDevice8*)>> handlers;
        return handlers;
    }
    inline std::vector<std::function<void()>>& OnPresent()
    {
        static std::vector<std::function<void()>> handlers;
        return handlers;
    }
    inline std::vector<std::function<bool(HWND, UINT, WPARAM, LPARAM, LRESULT&)>>& OnWindowMessage()
    {
        static std::vector<std::function<bool(HWND, UINT, WPARAM, LPARAM, LRESULT&)>> handlers;
        return handlers;
    }

    namespace Detail
    {
        using CreateDeviceFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3D8*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice8**);
        using ResetFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, D3DPRESENT_PARAMETERS*);
        using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, const RECT*, const RECT*, HWND, const RGNDATA*);
        using EndSceneFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*);
        using Create8Fn = IDirect3D8*(WINAPI*)(UINT);

        inline Create8Fn OriginalCreate8 = nullptr;
        inline CreateDeviceFn OriginalCreateDevice = nullptr;
        inline ResetFn OriginalReset = nullptr;
        inline PresentFn OriginalPresent = nullptr;
        inline EndSceneFn OriginalEndScene = nullptr;
        inline WNDPROC OriginalWndProc = nullptr;
        inline std::function<HRESULT(Mode)> NativeModeReset;
        inline LONG ToggleRequested = 0;

        inline decltype(&SetWindowLongA) OriginalSetWindowLongA = nullptr;
        inline decltype(&SetWindowPos) OriginalSetWindowPos = nullptr;
        inline decltype(&MoveWindow) OriginalMoveWindow = nullptr;
        inline decltype(&ChangeDisplaySettingsA) OriginalChangeDisplaySettingsA = nullptr;
        inline decltype(&ClipCursor) OriginalClipCursor = nullptr;
        inline decltype(&ShowWindow) OriginalShowWindow = nullptr;

        inline RECT MonitorRect(HWND window)
        {
            MONITORINFO info = { sizeof(info) };
            GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &info);
            return info.rcMonitor;
        }

        constexpr LONG WindowedStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE | WS_CLIPSIBLINGS;
        constexpr LONG BorderlessStyle = WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS;

        // Sizes and places the game window for the current mode
        inline void ApplyWindow()
        {
            auto& state = Get();
            if (!state.window)
                return;
            using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
            static auto setAttribute = []() -> SetAttribute
            {
                auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
            }();
            const int preference = 1;
            if (setAttribute)
                setAttribute(state.window, 33, &preference, sizeof(preference));
            if (state.mode == Mode::Fullscreen)
            {
                const auto monitor = MonitorRect(state.window);
                state.managingWindow = false;
                SetWindowLongW(state.window, GWL_STYLE, BorderlessStyle);
                SetWindowLongW(state.window, GWL_EXSTYLE, WS_EX_APPWINDOW);
                OriginalSetWindowPos(state.window, HWND_TOP, monitor.left, monitor.top,
                    state.backBufferWidth, state.backBufferHeight, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
                return;
            }

            auto monitor = MonitorRect(state.window);
            LONG style = state.mode == Mode::Borderless ? BorderlessStyle : WindowedStyle;
            LONG exStyle = WS_EX_APPWINDOW;
            RECT rect = monitor;
            if (state.mode == Mode::Windowed)
            {
                RECT client{ 0, 0, LONG(state.backBufferWidth), LONG(state.backBufferHeight) };
                using GetDpi = UINT(WINAPI*)(HWND);
                using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
                auto user32 = GetModuleHandleW(L"user32.dll");
                auto getDpi = reinterpret_cast<GetDpi>(GetProcAddress(user32, "GetDpiForWindow"));
                auto adjust = reinterpret_cast<AdjustForDpi>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
                if (getDpi && adjust)
                    adjust(&client, style, FALSE, exStyle, getDpi(state.window));
                else
                    AdjustWindowRectEx(&client, style, FALSE, exStyle);
                const LONG x = monitor.left + (monitor.right - monitor.left - LONG(state.backBufferWidth)) / 2;
                const LONG y = monitor.top + (monitor.bottom - monitor.top - LONG(state.backBufferHeight)) / 2;
                rect = { x + client.left, y + client.top, x + client.right, y + client.bottom };
            }

            if (state.mode == Mode::Windowed && state.haveWindowRect &&
                state.windowRect.right - state.windowRect.left == rect.right - rect.left &&
                state.windowRect.bottom - state.windowRect.top == rect.bottom - rect.top)
                rect = state.windowRect;
            state.managingWindow = false;
            SetWindowLongW(state.window, GWL_STYLE, style);
            SetWindowLongW(state.window, GWL_EXSTYLE, exStyle);
            UINT flags = SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE;
            if (state.backBufferWidth >= UINT(monitor.right - monitor.left) || state.backBufferHeight >= UINT(monitor.bottom - monitor.top))
                flags |= SWP_NOSENDCHANGING;
            OriginalSetWindowPos(state.window, HWND_NOTOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, flags);
            state.managingWindow = true;
        }

        inline LONG WINAPI SetWindowLongA_Hook(HWND window, int index, LONG value)
        {
            auto& state = Get();
            if (state.managingWindow && window == state.window && (index == GWL_STYLE || index == GWL_EXSTYLE))
                return GetWindowLongA(window, index);
            return OriginalSetWindowLongA(window, index, value);
        }

        inline BOOL WINAPI SetWindowPos_Hook(HWND window, HWND after, int x, int y, int cx, int cy, UINT flags)
        {
            auto& state = Get();
            if (state.managingWindow && window == state.window)
                flags |= SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;
            return OriginalSetWindowPos(window, after, x, y, cx, cy, flags);
        }

        inline BOOL WINAPI MoveWindow_Hook(HWND window, int x, int y, int cx, int cy, BOOL repaint)
        {
            auto& state = Get();
            if (state.managingWindow && window == state.window)
                return TRUE;
            return OriginalMoveWindow(window, x, y, cx, cy, repaint);
        }

        inline LONG WINAPI ChangeDisplaySettingsA_Hook(DEVMODEA* mode, DWORD flags)
        {
            // The desktop display mode is never changed in a window
            if (Get().mode != Mode::Fullscreen && !(flags & CDS_TEST))
                return DISP_CHANGE_SUCCESSFUL;
            return OriginalChangeDisplaySettingsA(mode, flags);
        }

        // The games minimise themselves when they lose the foreground, which only makes sense in exclusive full screen
        inline BOOL WINAPI ShowWindow_Hook(HWND window, int command)
        {
            auto& state = Get();
            if (state.mode != Mode::Fullscreen && window == state.window &&
                (command == SW_MINIMIZE || command == SW_SHOWMINIMIZED || command == SW_SHOWMINNOACTIVE || command == SW_FORCEMINIMIZE))
                return TRUE;
            return OriginalShowWindow(window, command);
        }

        inline BOOL WINAPI ClipCursor_Hook(const RECT* rect)
        {
            if (!Get().window)
                return OriginalClipCursor(rect);
            if (!rect)
                CursorConfinement::Release();
            else
                CursorConfinement::Update(Get().window);
            return TRUE;
        }

        // Anisotropic filtering for 3D geometry only: draws with pre-transformed (2D, post effect) vertices and
        // stages without a texture or without linear filtering are left alone
        using DrawPrimitiveFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, D3DPRIMITIVETYPE, UINT, UINT);
        using DrawIndexedPrimitiveFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, D3DPRIMITIVETYPE, UINT, UINT, UINT, UINT);
        inline DrawPrimitiveFn OriginalDrawPrimitive = nullptr;
        inline DrawIndexedPrimitiveFn OriginalDrawIndexedPrimitive = nullptr;

        struct Anisotropic
        {
            IDirect3DDevice8* device;
            bool changed[4] = {};

            explicit Anisotropic(IDirect3DDevice8* d) : device(d)
            {
                auto& state = Get();
                if (state.anisotropy <= 1)
                    return;
                DWORD shader = 0;
                device->GetVertexShader(&shader);
                bool fvf = shader <= 0xFFFF;    // shader handles are larger than any FVF
                if (fvf && (shader & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
                    return;
                for (DWORD stage = 0; stage < 4; ++stage)
                {
                    IDirect3DBaseTexture8* texture = nullptr;
                    if (FAILED(device->GetTexture(stage, &texture)) || !texture)
                        continue;
                    texture->Release();
                    DWORD filter = 0;
                    device->GetTextureStageState(stage, D3DTSS_MINFILTER, &filter);
                    if (filter != D3DTEXF_LINEAR)
                        continue;
                    device->SetTextureStageState(stage, D3DTSS_MINFILTER, D3DTEXF_ANISOTROPIC);
                    device->SetTextureStageState(stage, D3DTSS_MAXANISOTROPY, state.anisotropy);
                    changed[stage] = true;
                }
            }

            ~Anisotropic()
            {
                for (DWORD stage = 0; stage < 4; ++stage)
                {
                    if (changed[stage])
                        device->SetTextureStageState(stage, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
                }
            }
        };

        inline HRESULT STDMETHODCALLTYPE DrawPrimitive(IDirect3DDevice8* device, D3DPRIMITIVETYPE type, UINT start, UINT count)
        {
            Anisotropic filtering(device);
            return OriginalDrawPrimitive(device, type, start, count);
        }

        inline HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(IDirect3DDevice8* device, D3DPRIMITIVETYPE type, UINT minIndex, UINT vertices, UINT start, UINT count)
        {
            Anisotropic filtering(device);
            return OriginalDrawIndexedPrimitive(device, type, minIndex, vertices, start, count);
        }

        // Gamma: Direct3D ignores gamma ramps unless the game owns the screen, so in a window the game's ramp is
        // applied to the finished frame instead. The frame is copied to a texture and drawn back through a ps.1.4
        // shader that looks every channel up in a 256x1 table made from the ramp.
        using SetGammaRampFn = void(STDMETHODCALLTYPE*)(IDirect3DDevice8*, DWORD, const D3DGAMMARAMP*);
        inline SetGammaRampFn OriginalSetGammaRamp = nullptr;

        struct Gamma
        {
            D3DGAMMARAMP ramp = {};
            bool active = false;            // the ramp isn't the identity
            bool tableDirty = true;
            bool unsupported = false;       // no ps.1.4 or the frame can't be copied
            IDirect3DTexture8* frame = nullptr;
            IDirect3DTexture8* table = nullptr;
            DWORD shader = 0;
            DWORD states = 0;
            UINT width = 0, height = 0;
        };

        inline Gamma& GammaState()
        {
            static Gamma gamma;
            return gamma;
        }

        inline void ReleaseGamma(IDirect3DDevice8* device)
        {
            auto& gamma = GammaState();
            if (gamma.frame) { gamma.frame->Release(); gamma.frame = nullptr; }
            if (gamma.table) { gamma.table->Release(); gamma.table = nullptr; }
            if (gamma.shader && device) { device->DeletePixelShader(gamma.shader); gamma.shader = 0; }
            if (gamma.states && device) { device->DeleteStateBlock(gamma.states); gamma.states = 0; }
            gamma.width = gamma.height = 0;
            gamma.tableDirty = true;
        }

        inline void STDMETHODCALLTYPE SetGammaRamp(IDirect3DDevice8* device, DWORD flags, const D3DGAMMARAMP* ramp)
        {
            auto& gamma = GammaState();
            if (ramp)
            {
                gamma.ramp = *ramp;
                gamma.active = false;
                for (int i = 0; i < 256 && !gamma.active; ++i)
                {
                    int identity = i * 257;
                    gamma.active = std::abs(ramp->red[i] - identity) > 256 || std::abs(ramp->green[i] - identity) > 256 || std::abs(ramp->blue[i] - identity) > 256;
                }
                gamma.tableDirty = true;
            }
            OriginalSetGammaRamp(device, flags, ramp);
        }

        inline bool PrepareGamma(IDirect3DDevice8* device, const D3DSURFACE_DESC& desc)
        {
            auto& gamma = GammaState();
            if (!gamma.shader)
            {
                D3DCAPS8 caps = {};
                if (FAILED(device->GetDeviceCaps(&caps)) || caps.PixelShaderVersion < D3DPS_VERSION(1, 4))
                    return false;
                // ps.1.4
                //   texld r0, t0           the frame
                //   mov r2, r0.g
                //   mov r3, r0.b
                //   phase
                //   texld r1, r0           table at red, green, blue (stages 1-3)
                //   texld r2, r2
                //   texld r3, r3
                //   mov r0.r, r1
                //   mov r0.g, r2
                //   mov r0.ba, r3
                static const DWORD code[] =
                {
                    0xFFFF0104,
                    0x00000042, 0x800F0000, 0xB0E40000,
                    0x00000001, 0x800F0002, 0x80550000,
                    0x00000001, 0x800F0003, 0x80AA0000,
                    0x0000FFFD,
                    0x00000042, 0x800F0001, 0x80E40000,
                    0x00000042, 0x800F0002, 0x80E40002,
                    0x00000042, 0x800F0003, 0x80E40003,
                    0x00000001, 0x80010000, 0x80E40001,
                    0x00000001, 0x80020000, 0x80E40002,
                    0x00000001, 0x800C0000, 0x80E40003,
                    0x0000FFFF,
                };
                if (FAILED(device->CreatePixelShader(code, &gamma.shader)))
                {
                    gamma.shader = 0;
                    return false;
                }
            }
            if (!gamma.states && FAILED(device->CreateStateBlock(D3DSBT_ALL, &gamma.states)))
            {
                gamma.states = 0;
                return false;
            }
            if (!gamma.frame || gamma.width != desc.Width || gamma.height != desc.Height)
            {
                if (gamma.frame) { gamma.frame->Release(); gamma.frame = nullptr; }
                if (FAILED(device->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &gamma.frame)))
                    return false;
                gamma.width = desc.Width;
                gamma.height = desc.Height;
            }
            if (!gamma.table && FAILED(device->CreateTexture(256, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &gamma.table)))
            {
                gamma.table = nullptr;
                return false;
            }
            if (gamma.tableDirty)
            {
                D3DLOCKED_RECT locked = {};
                if (FAILED(gamma.table->LockRect(0, &locked, nullptr, 0)))
                    return false;
                auto texels = static_cast<DWORD*>(locked.pBits);
                for (int i = 0; i < 256; ++i)
                    texels[i] = 0xFF000000 | (DWORD(gamma.ramp.red[i] >> 8) << 16) | (DWORD(gamma.ramp.green[i] >> 8) << 8) | DWORD(gamma.ramp.blue[i] >> 8);
                gamma.table->UnlockRect(0);
                gamma.tableDirty = false;
            }
            return true;
        }

        inline void ApplyGamma(IDirect3DDevice8* device)
        {
            auto& gamma = GammaState();
            if (!gamma.active || gamma.unsupported || Get().mode == Mode::Fullscreen)
                return;

            IDirect3DSurface8* backBuffer = nullptr;
            if (FAILED(device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)))
                return;
            D3DSURFACE_DESC desc = {};
            backBuffer->GetDesc(&desc);
            IDirect3DSurface8* copy = nullptr;
            bool ready = PrepareGamma(device, desc) && SUCCEEDED(gamma.frame->GetSurfaceLevel(0, &copy))
                && SUCCEEDED(device->CopyRects(backBuffer, nullptr, 0, copy, nullptr));
            if (copy)
                copy->Release();
            if (!ready)
            {
                backBuffer->Release();
                gamma.unsupported = true;
                ReleaseGamma(device);
                return;
            }

            device->CaptureStateBlock(gamma.states);
            IDirect3DSurface8* target = nullptr;
            IDirect3DSurface8* depth = nullptr;
            device->GetRenderTarget(&target);
            device->GetDepthStencilSurface(&depth);
            device->SetRenderTarget(backBuffer, nullptr);

            D3DVIEWPORT8 viewport = { 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
            device->SetViewport(&viewport);
            device->SetRenderState(D3DRS_ZENABLE, FALSE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            device->SetRenderState(D3DRS_FOGENABLE, FALSE);
            device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
            for (DWORD stage = 0; stage < 4; ++stage)
            {
                device->SetTexture(stage, stage == 0 ? static_cast<IDirect3DBaseTexture8*>(gamma.frame) : gamma.table);
                device->SetTextureStageState(stage, D3DTSS_MINFILTER, D3DTEXF_POINT);
                device->SetTextureStageState(stage, D3DTSS_MAGFILTER, D3DTEXF_POINT);
                device->SetTextureStageState(stage, D3DTSS_MIPFILTER, D3DTEXF_NONE);
                device->SetTextureStageState(stage, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
                device->SetTextureStageState(stage, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
                device->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, 0);
                device->SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
            }
            device->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
            device->SetPixelShader(gamma.shader);

            struct Vertex { float x, y, z, w, u, v; };
            float w = float(desc.Width) - 0.5f, h = float(desc.Height) - 0.5f;
            Vertex quad[4] =
            {
                { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
                { w,     -0.5f, 0.0f, 1.0f, 1.0f, 0.0f },
                { -0.5f, h,     0.0f, 1.0f, 0.0f, 1.0f },
                { w,     h,     0.0f, 1.0f, 1.0f, 1.0f },
            };
            if (SUCCEEDED(device->BeginScene()))
            {
                device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));
                OriginalEndScene(device);
            }

            device->SetPixelShader(0);
            for (DWORD stage = 0; stage < 4; ++stage)
                device->SetTexture(stage, nullptr);
            device->SetRenderTarget(target, depth);
            if (target) target->Release();
            if (depth) depth->Release();
            device->ApplyStateBlock(gamma.states);
            backBuffer->Release();
        }

        inline LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            auto& state = Get();
            if (window == state.window)
                CursorConfinement::Message(message, wParam);
            for (auto& handler : OnWindowMessage())
            {
                LRESULT result = 0;
                if (handler(window, message, wParam, lParam, result))
                    return result;
            }

            switch (message)
            {
            case WM_SYSKEYDOWN:
                if (wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN))
                {
                    if (!(HIWORD(lParam) & KF_REPEAT))
                    {
                        if (NativeModeReset)
                            InterlockedExchange(&ToggleRequested, 1);
                    }
                    return 0;
                }
                if (wParam == VK_F4 && (HIWORD(lParam) & KF_ALTDOWN))
                    break;
                break;
            case WM_SYSCHAR:
                if (wParam == VK_RETURN)
                    return 0;   // no beep
                break;
            case 0x02E0:    // WM_DPICHANGED
                if (state.managingWindow)
                    return 0;   // keep our own size
                break;
            case WM_ACTIVATE:
                if (state.changingMode)
                    return DefWindowProcW(window, message, wParam, lParam);
                break;
            case WM_NCDESTROY:
            {
                auto original = OriginalWndProc;
                SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original));
                state.window = nullptr;
                state.managingWindow = false;
                InterlockedExchange(&ToggleRequested, 0);
                return CallWindowProcW(original, window, message, wParam, lParam);
            }
            }
            return CallWindowProcW(OriginalWndProc, window, message, wParam, lParam);
        }

        inline void PrepareParameters(IDirect3D8* d3d, UINT adapter, D3DDEVTYPE type, D3DPRESENT_PARAMETERS* pp)
        {
            auto& state = Get();
            if (!pp)
                return;
            if (state.followGame)
                state.mode = pp->Windowed ? Mode::Windowed : Mode::Borderless;
            if (state.mode != Mode::Fullscreen)
            {
                pp->Windowed = TRUE;
                pp->FullScreen_RefreshRateInHz = 0;
                pp->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
                if (pp->SwapEffect == D3DSWAPEFFECT_FLIP)
                    pp->SwapEffect = D3DSWAPEFFECT_DISCARD;
                D3DDISPLAYMODE desktop = {};
                if (d3d && SUCCEEDED(d3d->GetAdapterDisplayMode(adapter, &desktop)))
                {
                    if (FAILED(d3d->CheckDeviceType(adapter, type, desktop.Format, pp->BackBufferFormat, TRUE)))
                        pp->BackBufferFormat = desktop.Format;
                }
            }
            else
            {
                pp->Windowed = FALSE;
                pp->FullScreen_RefreshRateInHz = 0;
            }
            state.backBufferWidth = pp->BackBufferWidth;
            state.backBufferHeight = pp->BackBufferHeight;
        }

        inline void HookDevice(IDirect3DDevice8* device);

        inline HRESULT STDMETHODCALLTYPE Reset(IDirect3DDevice8* device, D3DPRESENT_PARAMETERS* pp)
        {
            for (auto& handler : OnBeforeReset())
                handler(device);
            ReleaseGamma(device);
            GammaState().unsupported = false;   // tried again on the new device
            IDirect3D8* d3d = nullptr;
            D3DDEVICE_CREATION_PARAMETERS creation = {};
            if (SUCCEEDED(device->GetDirect3D(&d3d)))
            {
                device->GetCreationParameters(&creation);
                PrepareParameters(d3d, creation.AdapterOrdinal, creation.DeviceType, pp);
                d3d->Release();
            }
            HRESULT result = OriginalReset(device, pp);
            if (SUCCEEDED(result))
            {
                ApplyWindow();
                for (auto& handler : OnDeviceReady())
                    handler(device);
            }
            return result;
        }

        // Frame pacing: a high resolution waitable timer for the bulk of the wait, then a short spin
        inline void LimitFrameRate()
        {
            auto& state = Get();
            using Clock = std::chrono::steady_clock;
            static Clock::time_point next = {};
            if (state.frameLimit <= 0.0)
            {
                next = {};
                return;
            }
            auto period = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / state.frameLimit));
            auto now = Clock::now();
            if (next == Clock::time_point{} || now - next > period * 4)
                next = now;     // first frame or after a long stall: don't try to catch up
            else
            {
                static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
                auto remaining = next - now;
                if (timer && remaining > std::chrono::milliseconds(2))
                {
                    LARGE_INTEGER due;
                    due.QuadPart = -LONGLONG(std::chrono::duration_cast<std::chrono::nanoseconds>(remaining - std::chrono::milliseconds(1)).count() / 100);
                    if (SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                        WaitForSingleObject(timer, INFINITE);
                }
                while (Clock::now() < next)
                    YieldProcessor();
            }
            next += period;
        }

        inline void ChangeWindowMode(IDirect3DDevice8* device)
        {
            auto& state = Get();
            if (!InterlockedExchange(&ToggleRequested, 0) || !NativeModeReset || state.changingMode ||
                !state.window || GetForegroundWindow() != state.window || device->TestCooperativeLevel() != D3D_OK)
                return;
            const auto previous = state.mode;
            const auto next = previous == Mode::Fullscreen ? state.lastWindowMode : Mode::Fullscreen;
            RECT previousRect{};
            GetWindowRect(state.window, &previousRect);
            if (previous == Mode::Windowed)
            {
                state.windowRect = previousRect;
                state.haveWindowRect = true;
            }
            state.changingMode = true;
            state.mode = next;
            ApplyWindow();
            if (SUCCEEDED(NativeModeReset(next)))
            {
                ApplyWindow();
                if (previous != Mode::Fullscreen)
                    state.lastWindowMode = previous;
            }
            else
            {
                OutputDebugStringA("Silent Hill: window mode reset failed; restoring the previous mode.\n");
                state.mode = previous;
                ApplyWindow();
                if (FAILED(NativeModeReset(previous)))
                    OutputDebugStringA("Silent Hill: previous mode reset failed; native device recovery remains active.\n");
            }
            state.changingMode = false;
        }

        inline HRESULT STDMETHODCALLTYPE Present(IDirect3DDevice8* device, const RECT* source, const RECT* dest, HWND window, const RGNDATA* dirty)
        {
            ChangeWindowMode(device);
            CursorConfinement::Update(Get().window);
            CursorConfinement::Bind();
            ApplyGamma(device);
            for (auto& handler : OnPresent())
                handler();
            LimitFrameRate();
            return OriginalPresent(device, source, dest, window, dirty);
        }

        inline HRESULT STDMETHODCALLTYPE EndScene(IDirect3DDevice8* device)
        {
            for (auto& handler : OnEndScene())
                handler(device);
            return OriginalEndScene(device);
        }

        inline void HookDevice(IDirect3DDevice8* device)
        {
            auto vtable = *reinterpret_cast<void***>(device);
            if (vtable[14] != reinterpret_cast<void*>(&Reset))
            {
                OriginalReset = reinterpret_cast<ResetFn>(vtable[14]);
                OriginalPresent = reinterpret_cast<PresentFn>(vtable[15]);
                OriginalEndScene = reinterpret_cast<EndSceneFn>(vtable[35]);
                injector::WriteMemory(&vtable[14], &Reset, true);
                injector::WriteMemory(&vtable[15], &Present, true);
                injector::WriteMemory(&vtable[35], &EndScene, true);
                OriginalSetGammaRamp = reinterpret_cast<SetGammaRampFn>(vtable[18]);
                injector::WriteMemory(&vtable[18], &SetGammaRamp, true);
                OriginalDrawPrimitive = reinterpret_cast<DrawPrimitiveFn>(vtable[70]);
                OriginalDrawIndexedPrimitive = reinterpret_cast<DrawIndexedPrimitiveFn>(vtable[71]);
                injector::WriteMemory(&vtable[70], &DrawPrimitive, true);
                injector::WriteMemory(&vtable[71], &DrawIndexedPrimitive, true);
            }
        }

        inline HRESULT STDMETHODCALLTYPE CreateDevice(IDirect3D8* d3d, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
            D3DPRESENT_PARAMETERS* pp, IDirect3DDevice8** out)
        {
            auto& state = Get();
            HWND target = (pp && pp->hDeviceWindow) ? pp->hDeviceWindow : window;
            if (target != state.window)
            {
                state.window = target;
                OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(target, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProc)));
            }
            PrepareParameters(d3d, adapter, type, pp);
            ApplyWindow();
            HRESULT result = OriginalCreateDevice(d3d, adapter, type, window, flags, pp, out);
            if (SUCCEEDED(result) && out && *out)
            {
                state.device = *out;
                HookDevice(*out);
                for (auto& handler : OnDeviceReady())
                    handler(*out);
            }
            return result;
        }

        inline IDirect3D8* WINAPI Create8(UINT sdk)
        {
            auto d3d = OriginalCreate8(sdk);
            if (d3d)
            {
                auto vtable = *reinterpret_cast<void***>(d3d);
                if (vtable[15] != reinterpret_cast<void*>(&CreateDevice))
                {
                    OriginalCreateDevice = reinterpret_cast<CreateDeviceFn>(vtable[15]);
                    injector::WriteMemory(&vtable[15], &CreateDevice, true);
                }
            }
            return d3d;
        }
    }

    // Installs the hooks. Call from the plugin's Init, before the game creates its window. With followGame the
    // game's own full screen setting gives a borderless window and its window setting a normal window.
    inline void Install(Mode mode, bool followGame = false)
    {
        auto& state = Get();
        state.mode = mode;
        state.lastWindowMode = mode == Mode::Fullscreen ? Mode::Windowed : mode;
        state.followGame = followGame;

        WindowedModeWrapper::EnableDpiAwareness();

        timeBeginPeriod(1);

        auto exe = GetModuleHandleW(nullptr);
        auto d3d = IATHook::Replace(exe, "d3d8.dll", std::forward_as_tuple("Direct3DCreate8", &Detail::Create8));
        if (auto it = d3d.find("Direct3DCreate8"); it != d3d.end())
            Detail::OriginalCreate8 = reinterpret_cast<Detail::Create8Fn>(it->second.get());

        {
            auto user = IATHook::Replace(exe, "user32.dll",
                std::forward_as_tuple("SetWindowLongA", &Detail::SetWindowLongA_Hook),
                std::forward_as_tuple("SetWindowPos", &Detail::SetWindowPos_Hook),
                std::forward_as_tuple("MoveWindow", &Detail::MoveWindow_Hook),
                std::forward_as_tuple("ChangeDisplaySettingsA", &Detail::ChangeDisplaySettingsA_Hook),
                std::forward_as_tuple("ClipCursor", &Detail::ClipCursor_Hook),
                std::forward_as_tuple("ShowWindow", &Detail::ShowWindow_Hook));
            auto take = [&](const char* name, auto& original, auto fallback)
            {
                if (auto it = user.find(name); it != user.end() && it->second.valid())
                    original = reinterpret_cast<std::remove_reference_t<decltype(original)>>(it->second.get());
                if (!original)
                    original = fallback;
            };
            take("SetWindowLongA", Detail::OriginalSetWindowLongA, &::SetWindowLongA);
            take("SetWindowPos", Detail::OriginalSetWindowPos, &::SetWindowPos);
            take("MoveWindow", Detail::OriginalMoveWindow, &::MoveWindow);
            take("ChangeDisplaySettingsA", Detail::OriginalChangeDisplaySettingsA, &::ChangeDisplaySettingsA);
            take("ClipCursor", Detail::OriginalClipCursor, &::ClipCursor);
            take("ShowWindow", Detail::OriginalShowWindow, &::ShowWindow);
        }
    }

    // Anisotropic filtering level for 3D geometry (0 or 1 = off), limited to what the device supports; may be called
    // again at any time, the next draw uses the new level
    inline void SetAnisotropy(int level)
    {
        auto& state = Get();
        state.requestedAnisotropy = level;
        auto limit = [](int requested, int max) { return max < 0 ? requested : std::min(requested, max); };
        state.anisotropy = limit(level, state.maxAnisotropy);
        static bool hooked = false;
        if (hooked)
            return;
        hooked = true;
        OnDeviceReady().push_back([limit](IDirect3DDevice8* device)
        {
            auto& state = Get();
            D3DCAPS8 caps = {};
            if (SUCCEEDED(device->GetDeviceCaps(&caps)))
                state.maxAnisotropy = (caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) ? int(std::max<DWORD>(1, caps.MaxAnisotropy)) : 0;
            state.anisotropy = limit(state.requestedAnisotropy, state.maxAnisotropy);
        });
    }

    inline void SetFrameLimit(double fps)
    {
        Get().frameLimit = fps;
    }
}
