#pragma once
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <mutex>
#include "d3dvtbl.h"

// Frame rate limit, anisotropic filtering and window mode for D3D9 games, applied in the D3D9 runtime's own methods.
// - frameLimit: frames per second, 0 = off. Each Present waits for its time slot.
// - limitDevice: optional device predicate that selects which Present calls use the frame limiter.
// - anisotropy: 2 to 16, 0 = the game's own filtering. Linear texture filtering becomes anisotropic, limited to what
//   the device supports.
// - presentParameters: called with the parameters (and the focus window, null on a reset) of every CreateDevice and
//   Reset, and may change them (windowed).
// - frame: called before every Present, on the game's render thread (overlays, per-frame window work).
// - beforeReset: called before every Reset, to release what the fix created on the device.
// - afterReset: called after a successful Reset, before the game resumes drawing.
// - waitForDevice: nothing runs on a lost device (the display is off, asleep or locked, or another program took it in
//   full screen): CreateDevice and Reset wait until it can be reset and try again, a new device lost before the game
//   used it is reset at once, and a Present that finds it lost returns only once it can be reset, so the game resets it
//   before it draws or creates anything again. The window stays responsive while it waits.
// Set them before installing: a method is hooked only when its option is on. The hooks are on the methods' code, not
// on a device's method table, which the runtime keeps per device and may replace; they stay for later devices.
// - HookCreation(): before the game creates its device; the devices created afterwards get the options.
// - HookDevice(device): for a device from the fix's own CreateDevice hook.
namespace D3D9Options
{
    inline std::atomic<int> frameLimit{ 0 };
    inline bool (*limitDevice)(IDirect3DDevice9*) = nullptr;
    inline int anisotropy = 0;
    inline void (*presentParameters)(D3DPRESENT_PARAMETERS*, HWND) = nullptr;
    inline void (*frame)(IDirect3DDevice9*) = nullptr;
    inline void (*beforeReset)(IDirect3DDevice9*) = nullptr;
    inline void (*afterReset)(IDirect3DDevice9*) = nullptr;
    inline bool waitForDevice = false;

    namespace detail
    {
        inline SafetyHookInline present, setSamplerState, reset, createDevice, create9;
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
            // One Present owns the auto-reset timer until the wait finishes.
            static std::mutex pacingMutex;
            const std::lock_guard lock(pacingMutex);
            static LONGLONG next = 0;
            static int previousLimit = 0;
            const int limit = frameLimit.load(std::memory_order_relaxed);
            if (limit <= 0)
            {
                next = 0;
                previousLimit = 0;
                return;
            }
            const LONGLONG period = Frequency() / std::clamp(limit, 10, 1000);
            const LONGLONG now = Now();
            if (limit != previousLimit || now >= next)
            {
                previousLimit = limit;
                next = now + period;
                return;
            }

            constexpr double SpinTime = 0.0015;
            const double sleepSeconds = double(next - now) / double(Frequency()) - SpinTime;
            if (sleepSeconds > 0.0)
            {
                struct PresentTimer
                {
                    HANDLE handle = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

                    ~PresentTimer()
                    {
                        if (handle)
                            CloseHandle(handle);
                    }
                };
                static PresentTimer timer;
                LARGE_INTEGER due;
                due.QuadPart = -static_cast<LONGLONG>(sleepSeconds * 10000000.0);
                const auto sleepMs = static_cast<DWORD>(sleepSeconds * 1000.0);
                if (timer.handle && SetWaitableTimer(timer.handle, &due, 0, nullptr, nullptr, FALSE))
                    WaitForSingleObject(timer.handle, sleepMs + 2);
                else
                    Sleep(sleepMs);
            }
            LONGLONG ready;
            while ((ready = Now()) < next)
                YieldProcessor();
            next = ready + period;
        }

        inline bool WaitForDevice();

        inline HRESULT STDMETHODCALLTYPE Present(IDirect3DDevice9* device, const RECT* source, const RECT* dest, HWND window, const RGNDATA* dirty)
        {
            if (frame)
                frame(device);
            if (!limitDevice || limitDevice(device))
                Limit();
            const HRESULT hr = present.unsafe_stdcall<HRESULT>(device, source, dest, window, dirty);
            if (waitForDevice && hr == D3DERR_DEVICELOST)
            {
                while (device->TestCooperativeLevel() == D3DERR_DEVICELOST && WaitForDevice())
                {
                }
            }
            return hr;
        }

        // One wait step while the device is lost: handles the thread's window messages, then sleeps. False when the
        // game is quitting (the quit message stays queued for it).
        inline bool WaitForDevice()
        {
            thread_local bool waiting = false;
            if (!waiting)
            {
                waiting = true;
                MSG message;
                while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                    {
                        PostQuitMessage(static_cast<int>(message.wParam));
                        waiting = false;
                        return false;
                    }
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                waiting = false;
            }
            Sleep(100);
            return true;
        }

        inline HRESULT STDMETHODCALLTYPE Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params)
        {
            if (beforeReset)
                beforeReset(device);
            if (presentParameters && params)
                presentParameters(params, nullptr);
            HRESULT hr = reset.unsafe_stdcall<HRESULT>(device, params);
            // Lost again before or during the reset: wait until the device can be reset
            while (waitForDevice && hr == D3DERR_DEVICELOST && WaitForDevice())
            {
                if (device->TestCooperativeLevel() != D3DERR_DEVICELOST)
                    hr = reset.unsafe_stdcall<HRESULT>(device, params);
            }
            if (SUCCEEDED(hr) && afterReset)
                afterReset(device);
            return hr;
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
            if (!hook)
                hook = safetyhook::create_inline(target, detour);
        }
    }

    inline void HookDevice(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        auto methods = *reinterpret_cast<void***>(device);
        if (presentParameters || beforeReset || afterReset || waitForDevice)
            detail::Hook(detail::reset, methods[IDirect3DDevice9VTBL::Reset], reinterpret_cast<void*>(&detail::Reset));
        if (frameLimit > 0 || frame || waitForDevice)
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
            while (waitForDevice && hr == D3DERR_DEVICELOST && WaitForDevice())
                hr = createDevice.unsafe_stdcall<HRESULT>(d3d, adapter, type, window, flags, params, device);
            if (SUCCEEDED(hr) && device)
                HookDevice(*device);
            // Lost before the game created anything on it: reset now, while that needs nothing released
            if (waitForDevice && SUCCEEDED(hr) && device && *device && params)
            {
                auto created = *device;
                HRESULT state;
                while ((state = created->TestCooperativeLevel()) == D3DERR_DEVICELOST && WaitForDevice())
                {
                }
                if (state == D3DERR_DEVICENOTRESET)
                {
                    auto copy = *params;
                    created->Reset(&copy);
                }
            }
            return hr;
        }

        inline void HookCreateDevice(IDirect3D9* d3d)
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            auto methods = *reinterpret_cast<void***>(d3d);
            Hook(createDevice, methods[IDirect3D9VTBL::CreateDevice], reinterpret_cast<void*>(&CreateDevice));
        }

        inline IDirect3D9* WINAPI Create9(UINT sdkVersion)
        {
            auto* d3d = create9.unsafe_stdcall<IDirect3D9*>(sdkVersion);
            if (d3d)
                HookCreateDevice(d3d);
            return d3d;
        }
    }

    // Loads the d3d9.dll the game will use if it isn't loaded yet; does nothing when no option is on
    inline void HookCreation()
    {
        if (frameLimit <= 0 && anisotropy <= 1 && !presentParameters && !frame && !beforeReset && !afterReset && !waitForDevice)
            return;
        auto module = GetModuleHandleW(L"d3d9.dll");
        if (!module)
            module = LoadLibraryW(L"d3d9.dll");
        if (!module)
            return;
        auto create = reinterpret_cast<IDirect3D9*(WINAPI*)(UINT)>(GetProcAddress(module, "Direct3DCreate9"));
        if (!create)
            return;
        if (auto* d3d = create(D3D_SDK_VERSION))
        {
            detail::HookCreateDevice(d3d);
            d3d->Release();
        }
        else if (!detail::create9)
        {
            // A failed probe leaves the game's factory calls available for a later successful attempt.
            detail::create9 = safetyhook::create_inline(reinterpret_cast<void*>(create), reinterpret_cast<void*>(&detail::Create9),
                SafetyHookInline::StartDisabled);
            (void)detail::create9.enable();
        }
    }
}
