module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module DeveloperOverlay;

namespace
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
    SafetyHookInline ResetHook;

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

export namespace DeveloperOverlay
{
    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    bool BlocksInput()
    {
        return Overlay.BlocksInput();
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
            if (!ResetHook)
                throw std::runtime_error("Developer overlay reset hook unavailable");
        }
        CurrentDevice = device;
        Overlay.Present(device, ThisModule());
    }
}
