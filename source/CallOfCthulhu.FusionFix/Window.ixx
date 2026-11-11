module;

#include <stdafx.h>
#include <d3d9.h>
#include <mutex>
#include "d3dvtbl.h"
#include "FixSettings.hpp"
#include "include/WindowStyle.hpp"

export module Window;

import ComVars;


export namespace CoCWindow
{
    inline SafetyHookInline styleHook, selectionHook;
    inline void* (__cdecl* getContext)() = nullptr;
    inline bool hasPresentation = false;
    inline int mode = 0;
    inline bool ready = false;

    inline HWND Window()
    {
        auto context = getContext ? static_cast<uint8_t*>(getContext()) : nullptr;
        return context ? *reinterpret_cast<HWND*>(context + 4) : nullptr;
    }

    inline void Resolution(int& width, int& height)
    {
        if (!ready || mode != 2)
            return;
        const auto monitor = WindowStyle::MonitorRect(Window());
        const int monitorWidth = monitor.right - monitor.left;
        const int monitorHeight = monitor.bottom - monitor.top;
        if (monitorWidth > 0 && monitorHeight > 0 && monitorWidth <= 16384 && monitorHeight <= 16384)
        {
            width = monitorWidth;
            height = monitorHeight;
        }
    }

    inline void Place()
    {
        const auto window = Window();
        if (!window || !hasPresentation)
            return;
        WindowStyle::SquareCorners(window);
        if (mode == 0)
            return;
        const DWORD frameBits = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(frameBits | WS_POPUP);
        style |= mode == 2 ? WS_POPUP : WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
        SetWindowLongW(window, GWL_STYLE, LONG(style));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        WindowStyle::Place(window, LONG(PresentationParameters->BackBufferWidth), LONG(PresentationParameters->BackBufferHeight));
        // The native app also caches these rectangles when creating its window.
        auto context = static_cast<uint8_t*>(getContext());
        GetWindowRect(window, reinterpret_cast<RECT*>(context + 20));
        GetClientRect(window, reinterpret_cast<RECT*>(context + 36));
    }

    inline LONG __cdecl Style()
    {
        if (ready && mode != 0 && hasPresentation)
        {
            PresentationParameters->Windowed = TRUE;
            PresentationParameters->FullScreen_RefreshRateInHz = 0;
        }
        const auto result = styleHook.ccall<LONG>();
        if (ready)
            Place();
        return result;
    }

    inline void* __cdecl Selection(uint8_t windowed, int format, int depth)
    {
        return selectionHook.ccall<void*>(ready && mode != 0 ? uint8_t(1) : windowed, format, depth);
    }

    // The game sets its brightness option and each level's light (and some fades) through the device's gamma ramp,
    // which Direct3D applies only to a full screen display. In a window the ramp is applied to the finished frame
    // instead: the frame is copied to a texture and drawn back through a shader that looks each channel up in a 256x1
    // table made from the ramp.
    namespace Gamma
    {
        inline SafetyHookInline setGammaRampHook;
        inline SafetyHookMid deviceHook;
        inline std::mutex lock;
        inline D3DGAMMARAMP ramp{};
        inline bool active = false;     // the ramp isn't the identity
        inline bool dirty = false;
        inline bool unsupported = false;
        inline IDirect3DPixelShader9* shader = nullptr;
        inline IDirect3DTexture9* table = nullptr;
        inline IDirect3DTexture9* frame = nullptr;
        inline UINT width = 0, height = 0;

        // ps_2_0:
        //   float4 c = tex2D(frame, uv);
        //   c.r = tex2D(ramp, float2(c.r, 0.5)).r; c.g = tex2D(ramp, float2(c.g, 0.5)).g; c.b = tex2D(ramp, float2(c.b, 0.5)).b;
        inline const DWORD ShaderCode[] =
        {
            0xFFFF0200, 0x05000051, 0xA00F0000, 0x3F000000, 0x00000000, 0x00000000,
            0x00000000, 0x0200001F, 0x80000000, 0xB0030000, 0x0200001F, 0x90000000,
            0xA00F0800, 0x0200001F, 0x90000000, 0xA00F0801, 0x03000042, 0x800F0000,
            0xB0E40000, 0xA0E40800, 0x02000001, 0x80020001, 0xA0000000, 0x02000001,
            0x80010001, 0x80000000, 0x02000001, 0x80020002, 0xA0000000, 0x02000001,
            0x80010002, 0x80550000, 0x02000001, 0x80010000, 0x80AA0000, 0x02000001,
            0x80020000, 0xA0000000, 0x03000042, 0x800F0001, 0x80E40001, 0xA0E40801,
            0x03000042, 0x800F0002, 0x80E40002, 0xA0E40801, 0x03000042, 0x800F0003,
            0x80E40000, 0xA0E40801, 0x02000001, 0x80010003, 0x80000001, 0x02000001,
            0x80020003, 0x80550002, 0x02000001, 0x80080003, 0x80FF0000, 0x02000001,
            0x800F0800, 0x80E40003, 0x0000FFFF,
        };

        template<class T>
        inline void Release(T*& object)
        {
            if (object)
            {
                object->Release();
                object = nullptr;
            }
        }

        inline void STDMETHODCALLTYPE SetGammaRamp(IDirect3DDevice9* device, UINT swapChain, DWORD flags, const D3DGAMMARAMP* value)
        {
            if (value && swapChain == 0)
            {
                std::lock_guard guard(lock);
                ramp = *value;
                active = false;
                for (int i = 0; i < 256 && !active; ++i)
                {
                    const int identity = i * 257;
                    active = std::abs(int(value->red[i]) - identity) > 256 || std::abs(int(value->green[i]) - identity) > 256 ||
                        std::abs(int(value->blue[i]) - identity) > 256;
                }
                dirty = true;
            }
            setGammaRampHook.unsafe_stdcall(device, swapChain, flags, value);
        }

        // Before a reset: the frame copy lives in the default pool
        inline void BeforeReset()
        {
            Release(frame);
            width = height = 0;
        }

        inline bool Prepare(IDirect3DDevice9* device, const D3DSURFACE_DESC& desc)
        {
            if (!shader && FAILED(device->CreatePixelShader(ShaderCode, &shader)))
                return false;
            if (!frame || width != desc.Width || height != desc.Height)
            {
                Release(frame);
                if (FAILED(device->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &frame, nullptr)))
                    return false;
                width = desc.Width;
                height = desc.Height;
            }
            if (!table)
            {
                if (FAILED(device->CreateTexture(256, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &table, nullptr)))
                    return false;
                dirty = true;
            }
            if (dirty)
            {
                D3DLOCKED_RECT locked{};
                if (FAILED(table->LockRect(0, &locked, nullptr, 0)))
                    return false;
                auto texels = static_cast<DWORD*>(locked.pBits);
                for (int i = 0; i < 256; ++i)
                    texels[i] = 0xFF000000 | (DWORD(ramp.red[i] >> 8) << 16) | (DWORD(ramp.green[i] >> 8) << 8) | DWORD(ramp.blue[i] >> 8);
                table->UnlockRect(0);
                dirty = false;
            }
            return true;
        }

        // Called before each present, on the render thread
        inline void Apply(IDirect3DDevice9* device)
        {
            std::lock_guard guard(lock);
            if (!active || unsupported || !device)
                return;
            IDirect3DSurface9* backBuffer = nullptr;
            if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)))
                return;
            D3DSURFACE_DESC desc{};
            backBuffer->GetDesc(&desc);
            IDirect3DSurface9* copy = nullptr;
            const bool copied = Prepare(device, desc) && SUCCEEDED(frame->GetSurfaceLevel(0, &copy)) &&
                SUCCEEDED(device->StretchRect(backBuffer, nullptr, copy, nullptr, D3DTEXF_NONE));
            Release(copy);
            IDirect3DStateBlock9* state = nullptr;
            if (!copied || FAILED(device->CreateStateBlock(D3DSBT_ALL, &state)))
            {
                // Without the copy the frame stays as the game drew it
                unsupported = !copied;
                backBuffer->Release();
                return;
            }
            state->Capture();
            IDirect3DSurface9* target = nullptr;
            IDirect3DSurface9* depth = nullptr;
            device->GetRenderTarget(0, &target);
            device->GetDepthStencilSurface(&depth);
            device->SetRenderTarget(0, backBuffer);
            device->SetDepthStencilSurface(nullptr);

            D3DVIEWPORT9 viewport{ 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
            device->SetViewport(&viewport);
            device->SetRenderState(D3DRS_ZENABLE, FALSE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            device->SetRenderState(D3DRS_FOGENABLE, FALSE);
            device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
            device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
            for (DWORD stage = 0; stage < 2; ++stage)
            {
                device->SetTexture(stage, stage == 0 ? frame : table);
                device->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_POINT);
                device->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
                device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
                device->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
                device->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
                device->SetSamplerState(stage, D3DSAMP_SRGBTEXTURE, FALSE);
            }
            device->SetVertexShader(nullptr);
            device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
            device->SetPixelShader(shader);

            struct Vertex { float x, y, z, w, u, v; };
            const float right = float(desc.Width) - 0.5f;
            const float bottom = float(desc.Height) - 0.5f;
            const Vertex quad[4] =
            {
                { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
                { right, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f },
                { -0.5f, bottom, 0.0f, 1.0f, 0.0f, 1.0f },
                { right, bottom, 0.0f, 1.0f, 1.0f, 1.0f },
            };
            const bool ownScene = SUCCEEDED(device->BeginScene());
            device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));
            if (ownScene)
                device->EndScene();

            device->SetTexture(0, nullptr);
            device->SetTexture(1, nullptr);
            device->SetRenderTarget(0, target);
            device->SetDepthStencilSurface(depth);
            Release(target);
            Release(depth);
            state->Apply();
            state->Release();
            backBuffer->Release();
        }

        // The game makes its device once, at startup; its wrapper holds the device at +104
        inline void Init()
        {
            auto created = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 8B C8 E8 ? ? ? ? E8");
            deviceHook = safetyhook::create_mid(created.get_first(10), [](SafetyHookContext& regs)
            {
                if (setGammaRampHook || !regs.eax)
                    return;
                auto device = *reinterpret_cast<IDirect3DDevice9**>(regs.eax + 104);
                if (!device)
                    return;
                auto methods = *reinterpret_cast<void***>(device);
                setGammaRampHook = safetyhook::create_inline(methods[IDirect3DDevice9VTBL::SetGammaRamp], reinterpret_cast<void*>(&SetGammaRamp));
            });
        }
    }

    inline void Init()
    {
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        auto style = hook::pattern("83 EC 2C 56 E8 ? ? ? ? 83 3D ? ? ? ? 00 8B 70 04 74 17 E8 ? ? ? ? 8B 40 10 50 6A F0 56");
        auto selection = hook::pattern("83 EC 34 A1 ? ? ? ? 8B 40 0C 53 56 33 F6 33 DB 3B C6 89 74 24 30 89 5C 24 34 89 74 24 38 89 44 24 28");
        getContext = reinterpret_cast<decltype(getContext)>(injector::GetBranchDestination(style.get_first(4)).as_int());
        auto windowed = *style.get_first<uint8_t*>(11);
        PresentationParameters.SetAddress(windowed - offsetof(D3DPRESENT_PARAMETERS, Windowed));
        hasPresentation = true;
        styleHook = safetyhook::create_inline(style.get_first(), Style);
        selectionHook = safetyhook::create_inline(selection.get_first(), Selection);
        ready = true;
        if (mode != 0)
        {
            // When the device is windowed, the post effect pass that redraws the frame adds a flat grey (0x30) to every
            // pixel, which washed out the picture; it now draws as in full screen
            auto windowedPost = hook::pattern("39 3D ? ? ? ? 0F 84 ? ? ? ? B9 07 00 00 00 39 8E");
            injector::WriteMemory<uint16_t>(windowedPost.get_first(6), 0xE990, true);
            Gamma::Init();
        }
    }
}
