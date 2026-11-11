module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module DeveloperOverlay;

import Focus;
import Cinematics;
import ComVars;

namespace
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
    SafetyHookInline ResetHook, PresentHook, CreateDeviceHook;

    HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(ThisModule(), path, DWORD(std::size(path)));
        DWORD ignored = 0;
        const auto size = GetFileVersionInfoSizeW(path, &ignored);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }

    HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        if (device == CurrentDevice)
            Overlay.BeforeReset();
        return ResetHook.unsafe_stdcall<HRESULT>(device, parameters);
    }
}

namespace
{
    HRESULT WINAPI OnPresent(IDirect3DDevice9* device, const RECT* source, const RECT* dest, HWND window, const RGNDATA* dirty);
    HRESULT WINAPI CreateDevice(IDirect3D9* factory, UINT adapter, D3DDEVTYPE type, HWND window, DWORD behavior,
        D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device);
}

export namespace DeveloperOverlay
{
    void Init()
    {
        auto module = GetModuleHandleW(L"d3d9.dll");
        if (!module)
            module = LoadLibraryW(L"d3d9.dll");
        using CreateFactory = IDirect3D9*(WINAPI*)(UINT);
        const auto create = module ? reinterpret_cast<CreateFactory>(GetProcAddress(module, "Direct3DCreate9")) : nullptr;
        if (create && !CreateDeviceHook)
        {
            if (auto factory = create(D3D_SDK_VERSION))
            {
                CreateDeviceHook = safetyhook::create_inline((*reinterpret_cast<void***>(factory))[16], CreateDevice);
                factory->Release();
            }
        }
    }

    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    void Attach(IDirect3DDevice9* device)
    {
        if (!PresentHook && device)
            PresentHook = safetyhook::create_inline((*reinterpret_cast<void***>(device))[17], OnPresent);
        if (device)
            CurrentDevice = device;
    }

    void Present(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        static const bool page = []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto buildTime = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", buildTime.c_str());
                ui.Value("Graphics API", "Direct3D 9");
                IDirect3DSurface9* surface = nullptr;
                if (CurrentDevice && SUCCEEDED(CurrentDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
                {
                    D3DSURFACE_DESC desc{};
                    surface->GetDesc(&desc);
                    surface->Release();
                    ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
                }
            });
            return true;
        }();
        if (!ResetHook)
        {
            auto target = (*reinterpret_cast<void***>(device))[16];
            ResetHook = safetyhook::create_inline(target, Reset);
        }
        CurrentDevice = device;
        Overlay.Present(device, ThisModule());
        Focus::Frame(Overlay.BlocksInput());
    }
}

namespace
{
    HRESULT WINAPI CreateDevice(IDirect3D9* factory, UINT adapter, D3DDEVTYPE type, HWND window, DWORD behavior,
        D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
    {
        const auto result = CreateDeviceHook.unsafe_stdcall<HRESULT>(factory, adapter, type, window, behavior, parameters, device);
        const auto error = GetLastError();
        const auto target = parameters && parameters->hDeviceWindow ? parameters->hDeviceWindow : window;
        DWORD owner = 0;
        wchar_t name[128]{};
        GetWindowThreadProcessId(target, &owner);
        GetClassNameW(target, name, int(std::size(name)));
        if (SUCCEEDED(result) && device && *device && owner == GetCurrentProcessId() &&
            _wcsicmp(name, L"convictionUnrealWWindowsViewportWindow") == 0)
        {
            hGameWindow = target;
            if (parameters)
            {
                BackBufferWidth = parameters->BackBufferWidth;
                BackBufferHeight = parameters->BackBufferHeight;
            }
            DeveloperOverlay::Attach(*device);
        }
        SetLastError(error);
        return result;
    }

    HRESULT WINAPI OnPresent(IDirect3DDevice9* device, const RECT* source, const RECT* dest, HWND window, const RGNDATA* dirty)
    {
        Cinematics::Present(device);
        DeveloperOverlay::Present(device);
        return PresentHook.unsafe_stdcall<HRESULT>(device, source, dest, window, dirty);
    }
}
