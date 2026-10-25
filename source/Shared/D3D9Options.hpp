#pragma once
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include "d3dvtbl.h"

// Frame rate limit, anisotropic filtering and window mode for D3D9 games, applied in the D3D9 runtime's own methods.
// - frameLimit: frames per second, 0 = off. Each Present waits for its time slot.
// - anisotropy: 2 to 16, 0 = the game's own filtering. Linear texture filtering becomes anisotropic, limited to what
//   the device supports.
// - presentParameters: called with the parameters (and the focus window, null on a reset) of every CreateDevice and
//   Reset, and may change them (windowed).
// Set them before installing: a method is hooked only when its option is on. The hooks are on the methods' code, not
// on a device's method table, which the runtime keeps per device and may replace; they stay for later devices.
// - HookCreation(): before the game creates its device; the devices created afterwards get the options.
// - HookDevice(device): for a device from the fix's own CreateDevice hook.
namespace D3D9Options
{
    inline int frameLimit = 0;
    inline int anisotropy = 0;
    inline void (*presentParameters)(D3DPRESENT_PARAMETERS*, HWND) = nullptr;

    namespace detail
    {
        inline SafetyHookInline present, setSamplerState, reset, createDevice;
        inline DWORD maxAnisotropy = 0;
        inline bool magAnisotropic = false;

        inline LONGLONG Frequency()
        {
            static LONGLONG frequency = []() { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f.QuadPart; }();
            return frequency;
        }

        inline LONGLONG Now()
        {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            return now.QuadPart;
        }

        // Sleeps on a high resolution timer until shortly before the frame's slot, then spins the rest. A frame that came
        // late (loading, a hitch) starts the count anew instead of rushing the next ones.
        inline void Limit()
        {
            static LONGLONG next = 0;
            static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            const LONGLONG period = Frequency() / std::clamp(frameLimit, 10, 1000);
            LONGLONG now = Now();
            if (next == 0 || now - next > period * 4)
            {
                next = now + period;
                return;
            }

            constexpr double SpinTime = 0.0015;
            double seconds = double(next - now) / double(Frequency());
            if (seconds > SpinTime)
            {
                LARGE_INTEGER due;
                due.QuadPart = -static_cast<LONGLONG>((seconds - SpinTime) * 10000000.0);
                if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(timer, INFINITE);
                else
                    Sleep(static_cast<DWORD>((seconds - SpinTime) * 1000.0));
            }
            while (Now() < next)
                YieldProcessor();
            next += period;
        }

        inline HRESULT STDMETHODCALLTYPE Present(IDirect3DDevice9* device, const RECT* source, const RECT* dest, HWND window, const RGNDATA* dirty)
        {
            Limit();
            return present.unsafe_stdcall<HRESULT>(device, source, dest, window, dirty);
        }

        inline HRESULT STDMETHODCALLTYPE Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params)
        {
            if (presentParameters && params)
                presentParameters(params, nullptr);
            return reset.unsafe_stdcall<HRESULT>(device, params);
        }

        inline HRESULT STDMETHODCALLTYPE SetSamplerState(IDirect3DDevice9* device, DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value)
        {
            const DWORD level = (std::min)(DWORD(anisotropy), maxAnisotropy);
            if (level > 1 && sampler < 16)
            {
                if (type == D3DSAMP_MAXANISOTROPY)
                    value = level;
                else if ((type == D3DSAMP_MINFILTER || (type == D3DSAMP_MAGFILTER && magAnisotropic)) && (value == D3DTEXF_LINEAR || value == D3DTEXF_ANISOTROPIC))
                {
                    setSamplerState.unsafe_stdcall<HRESULT>(device, sampler, D3DSAMP_MAXANISOTROPY, level);
                    value = D3DTEXF_ANISOTROPIC;
                }
            }
            return setSamplerState.unsafe_stdcall<HRESULT>(device, sampler, type, value);
        }

        inline void Hook(SafetyHookInline& hook, void* target, void* detour)
        {
            if (!hook && target)
                hook = safetyhook::create_inline(target, detour);
        }
    }

    inline void HookDevice(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        auto methods = *reinterpret_cast<void***>(device);
        if (presentParameters)
            detail::Hook(detail::reset, methods[IDirect3DDevice9VTBL::Reset], reinterpret_cast<void*>(&detail::Reset));
        if (frameLimit > 0)
            detail::Hook(detail::present, methods[IDirect3DDevice9VTBL::Present], reinterpret_cast<void*>(&detail::Present));
        if (anisotropy > 1)
        {
            D3DCAPS9 caps{};
            if (SUCCEEDED(device->GetDeviceCaps(&caps)) && (caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC))
            {
                detail::maxAnisotropy = (std::max)(DWORD(1), caps.MaxAnisotropy);
                detail::magAnisotropic = (caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) != 0;
                detail::Hook(detail::setSamplerState, methods[IDirect3DDevice9VTBL::SetSamplerState], reinterpret_cast<void*>(&detail::SetSamplerState));
            }
        }
    }

    namespace detail
    {
        inline HRESULT STDMETHODCALLTYPE CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags, D3DPRESENT_PARAMETERS* params, IDirect3DDevice9** device)
        {
            if (presentParameters && params)
                presentParameters(params, window);
            HRESULT hr = createDevice.unsafe_stdcall<HRESULT>(d3d, adapter, type, window, flags, params, device);
            if (SUCCEEDED(hr) && device)
                HookDevice(*device);
            return hr;
        }
    }

    // Loads the d3d9.dll the game will use if it isn't loaded yet; false when nothing is on or D3D9 is unavailable
    inline bool HookCreation()
    {
        if (frameLimit <= 0 && anisotropy <= 1 && !presentParameters)
            return false;
        auto module = GetModuleHandleW(L"d3d9.dll");
        if (!module)
            module = LoadLibraryW(L"d3d9.dll");
        auto create = module ? reinterpret_cast<IDirect3D9*(WINAPI*)(UINT)>(GetProcAddress(module, "Direct3DCreate9")) : nullptr;
        auto d3d = create ? create(D3D_SDK_VERSION) : nullptr;
        if (!d3d)
            return false;
        auto methods = *reinterpret_cast<void***>(d3d);
        detail::Hook(detail::createDevice, methods[IDirect3D9VTBL::CreateDevice], reinterpret_cast<void*>(&detail::CreateDevice));
        d3d->Release();
        return bool(detail::createDevice);
    }
}
