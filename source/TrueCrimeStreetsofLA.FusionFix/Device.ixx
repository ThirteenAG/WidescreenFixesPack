module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <atomic>


export module Device;

import Window;

export namespace TrueCrimeDevice
{
    // The game renders with Direct3D 8; the loader's d3d8to9 bridge turns that into a Direct3D 9 device.
    inline SafetyHookInline Create9Hook, CreateDeviceHook, PresentHook, ResetHook;
    inline IDirect3DDevice9* Device = nullptr;
    inline int (*FrameLimit)() = nullptr;
    // Optional: called before every Present (overlay)
    inline void (*Frame)(IDirect3DDevice9*) = nullptr;
    inline std::atomic<uint32_t> Frames{ 0 };
    // The game releases its device and creates a new one on a resolution change: everything made on the old device
    // goes first
    inline std::vector<void (*)()> Releasing;
    inline SafetyHookMid ReleaseHook;
    // The back buffer size the game asks for (its resolution choice)
    inline void (*Resolution)(int, int) = nullptr;

    inline HRESULT WINAPI Present(IDirect3DDevice9* device, const RECT* source, const RECT* destination,
        HWND window, const RGNDATA* dirty)
    {
        Frames.fetch_add(1, std::memory_order_relaxed);
        if (Frame)
            Frame(device);
        const int limit = FrameLimit ? FrameLimit() : 0;
        static LARGE_INTEGER frequency = []()
        {
            LARGE_INTEGER value{};
            QueryPerformanceFrequency(&value);
            return value;
        }();
        static LONGLONG next = 0;
        static int previous = 0;
        if (limit > 0 && frequency.QuadPart > 0)
        {
            LARGE_INTEGER now{};
            QueryPerformanceCounter(&now);
            const auto period = frequency.QuadPart / limit;
            if (previous != limit || next == 0 || now.QuadPart > next + period)
                next = now.QuadPart + period;
            while (now.QuadPart < next)
            {
                if (next - now.QuadPart > frequency.QuadPart / 500)
                    Sleep(1);
                else
                    YieldProcessor();
                QueryPerformanceCounter(&now);
            }
            next += period;
        }
        else
            next = 0;
        previous = limit;
        return PresentHook.stdcall<HRESULT>(device, source, destination, window, dirty);
    }

    inline void Parameters(D3DPRESENT_PARAMETERS* parameters)
    {
        if (!parameters)
            return;
        if (Resolution && parameters->BackBufferWidth >= 640 && parameters->BackBufferHeight >= 480)
            Resolution(int(parameters->BackBufferWidth), int(parameters->BackBufferHeight));
        parameters->Windowed = TrueCrimeWindow::Mode != 0;
        parameters->BackBufferWidth = TrueCrimeWindow::Width;
        parameters->BackBufferHeight = TrueCrimeWindow::Height;
        if (parameters->Windowed)
        {
            parameters->FullScreen_RefreshRateInHz = 0;
            parameters->BackBufferFormat = D3DFMT_UNKNOWN;
        }
    }

    inline HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        WFP::onBeforeReset().executeAll();
        Parameters(parameters);
        return ResetHook.stdcall<HRESULT>(device, parameters);
    }

    inline HRESULT WINAPI CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window,
        DWORD behavior, D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
    {
        Parameters(parameters);
        const auto result = CreateDeviceHook.stdcall<HRESULT>(d3d, adapter, type, window, behavior, parameters, device);
        if (SUCCEEDED(result) && device && *device)
        {
            Device = *device;
            auto table = *reinterpret_cast<void***>(*device);
            if (!PresentHook)
                PresentHook = safetyhook::create_inline(table[17], Present);
            if (!ResetHook)
                ResetHook = safetyhook::create_inline(table[16], Reset);
        }
        return result;
    }

    inline IDirect3D9* WINAPI Create9(UINT version)
    {
        auto result = Create9Hook.stdcall<IDirect3D9*>(version);
        if (result && !CreateDeviceHook)
        {
            auto table = *reinterpret_cast<void***>(result);
            CreateDeviceHook = safetyhook::create_inline(table[16], CreateDevice);
        }
        return result;
    }

    inline void Initialize(int (*limit)())
    {
        FrameLimit = limit;
        ReleaseHook = safetyhook::create_mid(hook::pattern("A0 ? ? ? ? 84 C0 75 ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 6A 00 B9").get_first(),
            [](SafetyHookContext&)
        {
            for (auto release : Releasing)
                release();
        });
        CallbackHandler::RegisterCallback(L"d3d9.dll", []()
        {
            if (auto create = GetProcAddress(GetModuleHandleW(L"d3d9.dll"), "Direct3DCreate9"))
                Create9Hook = safetyhook::create_inline(create, Create9);
        });
    }
}
