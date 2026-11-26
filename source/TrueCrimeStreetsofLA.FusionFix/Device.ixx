module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include <atomic>
#include <wrl/client.h>


export module Device;

import Window;

export namespace TrueCrimeDevice
{
    // The game renders with Direct3D 8; the loader's d3d8to9 bridge turns that into a Direct3D 9 device.
    inline SafetyHookInline Create9Hook, CreateDeviceHook, PresentHook, ResetHook;
    inline GameRef<IUnknown*> NativeDevice;
    inline std::atomic<IDirect3DDevice9*> CreatedGameDevice{ nullptr };
    inline int (*FrameLimit)() = nullptr;
    // Optional: called before every Present (overlay)
    inline void (*Frame)(IDirect3DDevice9*) = nullptr;
    // The game releases its device and creates a new one on a resolution change: everything made on the old device
    // goes first
    inline std::vector<void (*)()> Releasing;
    inline SafetyHookMid ReleaseHook;
    // At exit the game frees the world before its device; the vehicle textures, released with the device, would still
    // clear their slots in the world's freed texture table, so they go before the world
    inline GameRef<uintptr_t> Vehicles;
    inline SafetyHookMid WorldShutdownHook;
    // The back buffer size the game asks for (its resolution choice)
    inline void (*Resolution)(int, int) = nullptr;

    inline Microsoft::WRL::ComPtr<IDirect3DDevice9> RenderDevice()
    {
        Microsoft::WRL::ComPtr<IDirect3DDevice9> result;
        if (const auto device = NativeDevice.get())
            device->QueryInterface(IID_PPV_ARGS(result.GetAddressOf()));
        return result;
    }

    inline HRESULT WINAPI Present(IDirect3DDevice9* device, const RECT* source, const RECT* destination,
        HWND window, const RGNDATA* dirty)
    {
        if (CreatedGameDevice.load() != device)
            return PresentHook.stdcall<HRESULT>(device, source, destination, window, dirty);
        const auto game = RenderDevice();
        if (!game || game.Get() != device)
            return PresentHook.stdcall<HRESULT>(device, source, destination, window, dirty);
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

    inline std::pair<int, int> Parameters(D3DPRESENT_PARAMETERS* parameters)
    {
        int width = TrueCrimeWindow::Width;
        int height = TrueCrimeWindow::Height;
        if (!parameters)
            return { width, height };
        if (TrueCrimeWindow::Mode != 2 && parameters->BackBufferWidth >= 640 && parameters->BackBufferHeight >= 480)
        {
            width = int(parameters->BackBufferWidth);
            height = int(parameters->BackBufferHeight);
        }
        parameters->Windowed = TrueCrimeWindow::Mode != 0;
        parameters->BackBufferWidth = width;
        parameters->BackBufferHeight = height;
        if (parameters->Windowed)
        {
            parameters->FullScreen_RefreshRateInHz = 0;
            parameters->BackBufferFormat = D3DFMT_UNKNOWN;
        }
        return { width, height };
    }

    inline HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        if (CreatedGameDevice.load() != device)
            return ResetHook.stdcall<HRESULT>(device, parameters);
        const auto game = RenderDevice();
        if (!game || game.Get() != device)
            return ResetHook.stdcall<HRESULT>(device, parameters);
        WFP::onBeforeReset().executeAll();
        const auto [width, height] = Parameters(parameters);
        const auto result = ResetHook.stdcall<HRESULT>(device, parameters);
        if (SUCCEEDED(result) && Resolution)
            Resolution(width, height);
        return result;
    }

    inline HRESULT WINAPI CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window,
        DWORD behavior, D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
    {
        const auto gameWindow = TrueCrimeWindow::Window.load();
        if (!gameWindow || (window != gameWindow && (!parameters || parameters->hDeviceWindow != gameWindow)))
            return CreateDeviceHook.stdcall<HRESULT>(d3d, adapter, type, window, behavior, parameters, device);
        const auto [width, height] = Parameters(parameters);
        const auto result = CreateDeviceHook.stdcall<HRESULT>(d3d, adapter, type, window, behavior, parameters, device);
        if (SUCCEEDED(result) && device && *device)
        {
            CreatedGameDevice.store(*device);
            if (Resolution)
                Resolution(width, height);
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
        // The renderer stores its current Direct3D 8 device just before clearing and presenting it.
        auto native = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 85 C0 A3 ? ? ? ? 0F 84 ? ? ? ? 68 ? ? ? ? 50 6A 01 B9 ? ? ? ? E8");
        NativeDevice.SetAddress(*native.get_first<IUnknown**>(14));
        ReleaseHook = safetyhook::create_mid(hook::pattern("A0 ? ? ? ? 84 C0 75 ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 6A 00 B9").get_first(),
            [](SafetyHookContext&)
        {
            for (auto release : Releasing)
                release();
        });
        auto world = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 CB 00 00 00");
        Vehicles.SetAddress(*world.get_first<uintptr_t*>(11));
        WorldShutdownHook = safetyhook::create_mid(hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 3B CB 74 ? E8 ? ? ? ? 89 1D ? ? ? ? E8").get_first(),
            [](SafetyHookContext&)
        {
            const auto vehicles = Vehicles.get();
            if (vehicles && (*reinterpret_cast<uint32_t*>(vehicles + 4) & 2))
            {
                using Release = void(__thiscall*)(uintptr_t);
                (*reinterpret_cast<Release**>(vehicles))[3](vehicles);
                *reinterpret_cast<uint32_t*>(vehicles + 4) &= ~2u;
            }
        });
        CallbackHandler::RegisterCallback(L"d3d9.dll", []()
        {
            Create9Hook = safetyhook::create_inline(GetProcAddress(GetModuleHandleW(L"d3d9.dll"), "Direct3DCreate9"), Create9);
        });
    }
}
