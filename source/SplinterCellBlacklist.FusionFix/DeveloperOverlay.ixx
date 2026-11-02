module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"
#include "overlay/OverlayD3D11.hpp"
#include <xinput.h>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

export module DeveloperOverlay;

import ComVars;
import FrameLimit;
import Camera;

namespace
{
    asi_overlay::D3D9Overlay Overlay9;
    asi_overlay::D3D11Overlay Overlay11;
    SafetyHookInline Create9Hook, Present9Hook, Reset9Hook, Present11Hook;
    IDirect3DDevice9* Device9 = nullptr;
    IDXGISwapChain* Chain11 = nullptr;
    decltype(&Direct3DCreate9) OriginalFactory9 = nullptr;
    decltype(&D3D11CreateDeviceAndSwapChain) OriginalCreate11 = nullptr;
    decltype(&XInputGetState) OriginalPad = nullptr;

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

    void Info()
    {
        static const bool page = []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto build = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", build.c_str());
                ui.Value("Graphics API", Chain11 ? "Direct3D 11" : "Direct3D 9");
                UINT width = 0, height = 0;
                if (Chain11)
                {
                    ID3D11Texture2D* buffer = nullptr;
                    if (SUCCEEDED(Chain11->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&buffer))))
                    {
                        D3D11_TEXTURE2D_DESC description{};
                        buffer->GetDesc(&description);
                        width = description.Width;
                        height = description.Height;
                        buffer->Release();
                    }
                }
                else if (Device9)
                {
                    IDirect3DSurface9* buffer = nullptr;
                    if (SUCCEEDED(Device9->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &buffer)))
                    {
                        D3DSURFACE_DESC description{};
                        buffer->GetDesc(&description);
                        width = description.Width;
                        height = description.Height;
                        buffer->Release();
                    }
                }
                ui.Value("Resolution", std::to_string(width) + " x " + std::to_string(height));
                const auto camera = GetCameraDebugState();
                ui.Value("Camera hooks", camera.available ? "Available" : "Unavailable");
                ui.Value("FOV callback calls", std::to_string(camera.calls));
                ui.Value("Viewport matched", camera.valid ? "Yes" : "No");
                if (camera.sampled)
                {
                    ui.Value("Camera owner", std::format("0x{:08X}", camera.owner));
                    ui.Value("Camera kind", std::to_string(camera.kind));
                    ui.Value("Camera mode", std::to_string(camera.mode));
                    ui.Value("Gameplay camera", camera.gameplay ? "Yes" : "No");
                    ui.Value("Cinematic camera", camera.cinematic ? "Yes" : "No");
                    ui.Value("Native FOV target", std::to_string(camera.target));
                    ui.Value("Current camera FOV", std::to_string(camera.current));
                }
                ui.Value("FOV multiplier", std::to_string(camera.factor));
            });
            return true;
        }();
    }

    HRESULT WINAPI Present9(IDirect3DDevice9* device, const RECT* source, const RECT* destination, HWND window, const RGNDATA* dirty)
    {
        if (device != Device9)
            return Present9Hook.unsafe_stdcall<HRESULT>(device, source, destination, window, dirty);
        PaceFrame();
        Info();
        Overlay9.Present(device, ThisModule());
        return Present9Hook.unsafe_stdcall<HRESULT>(device, source, destination, window, dirty);
    }

    HRESULT WINAPI Reset9(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        if (device == Device9)
            Overlay9.BeforeReset();
        return Reset9Hook.unsafe_stdcall<HRESULT>(device, parameters);
    }

    HRESULT WINAPI Present11(IDXGISwapChain* chain, UINT interval, UINT flags)
    {
        if (chain != Chain11)
            return Present11Hook.unsafe_stdcall<HRESULT>(chain, interval, flags);
        if (!(flags & DXGI_PRESENT_TEST))
            PaceFrame();
        Info();
        if (!(flags & DXGI_PRESENT_TEST))
            Overlay11.Present(chain, ThisModule());
        return Present11Hook.unsafe_stdcall<HRESULT>(chain, interval, flags);
    }

    bool OwnedWindow(HWND window)
    {
        DWORD owner = 0;
        return window && GetWindowThreadProcessId(window, &owner) && owner == GetCurrentProcessId();
    }

    HRESULT WINAPI Create9(IDirect3D9* factory, UINT adapter, D3DDEVTYPE type, HWND window, DWORD behavior,
        D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
    {
        const auto result = Create9Hook.unsafe_stdcall<HRESULT>(factory, adapter, type, window, behavior, parameters, device);
        const auto error = GetLastError();
        const auto target = parameters && parameters->hDeviceWindow ? parameters->hDeviceWindow : window;
        if (SUCCEEDED(result) && device && *device && OwnedWindow(target))
        {
            auto table = *reinterpret_cast<void***>(*device);
            if (!Present9Hook)
                Present9Hook = safetyhook::create_inline(table[17], Present9);
            if (!Reset9Hook)
                Reset9Hook = safetyhook::create_inline(table[16], Reset9);
            Device9 = *device;
            WindowHandle = target;
        }
        SetLastError(error);
        return result;
    }

    IDirect3D9* WINAPI Factory9(UINT version)
    {
        auto factory = OriginalFactory9(version);
        const auto error = GetLastError();
        if (factory && !Create9Hook)
            Create9Hook = safetyhook::create_inline((*reinterpret_cast<void***>(factory))[16], Create9);
        SetLastError(error);
        return factory;
    }

    HRESULT WINAPI Create11(IDXGIAdapter* adapter, D3D_DRIVER_TYPE type, HMODULE software, UINT flags,
        const D3D_FEATURE_LEVEL* levels, UINT count, UINT version, const DXGI_SWAP_CHAIN_DESC* description,
        IDXGISwapChain** chain, ID3D11Device** device, D3D_FEATURE_LEVEL* level, ID3D11DeviceContext** context)
    {
        const auto result = OriginalCreate11(adapter, type, software, flags, levels, count, version, description, chain, device, level, context);
        const auto error = GetLastError();
        if (SUCCEEDED(result) && chain && *chain)
        {
            DXGI_SWAP_CHAIN_DESC actual{};
            if (SUCCEEDED((*chain)->GetDesc(&actual)) && OwnedWindow(actual.OutputWindow))
            {
                if (!Present11Hook)
                    Present11Hook = safetyhook::create_inline((*reinterpret_cast<void***>(*chain))[8], Present11);
                Chain11 = *chain;
                WindowHandle = actual.OutputWindow;
            }
        }
        SetLastError(error);
        return result;
    }

    DWORD WINAPI Pad(DWORD user, XINPUT_STATE* state)
    {
        const auto result = OriginalPad(user, state);
        const auto error = GetLastError();
        if (result == ERROR_SUCCESS && state)
        {
            if (user == 0)
            {
                auto& overlay = Chain11 ? static_cast<asi_overlay::Overlay&>(Overlay11) : static_cast<asi_overlay::Overlay&>(Overlay9);
                overlay.Gamepad(state->Gamepad.wButtons, int8_t(state->Gamepad.sThumbLX / 256), int8_t(state->Gamepad.sThumbLY / 256));
            }
            if (Overlay9.BlocksInput() || Overlay11.BlocksInput())
                state->Gamepad = {};
        }
        SetLastError(error);
        return result;
    }

    template<class Function> void Import(const char* library, const char* name, Function replacement, Function& original)
    {
        auto hooks = IATHook::Replace(GetModuleHandleW(nullptr), library, std::forward_as_tuple(name, replacement));
        auto found = hooks.find(name);
        if (found != hooks.end())
            original = reinterpret_cast<Function>(found->second.get());
    }
}

export namespace DeveloperOverlay
{
    bool BlocksInput()
    {
        return Overlay9.BlocksInput() || Overlay11.BlocksInput();
    }

    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        // Buttons and vertical wheel steps come from the game's DirectInput events.
        if (BlocksInput() && (message == WM_INPUT || message == WM_MOUSEWHEEL))
        {
            result = message == WM_INPUT ? DefWindowProcW(window, message, wParam, lParam) : 0;
            return true;
        }
        return Chain11 ? Overlay11.Message(window, message, wParam, lParam, result) : Overlay9.Message(window, message, wParam, lParam, result);
    }

    void MouseInput(int key, int action)
    {
        auto& overlay = Chain11 ? static_cast<asi_overlay::Overlay&>(Overlay11) : static_cast<asi_overlay::Overlay&>(Overlay9);
        if (action != 1 && action != 5)
            return;
        const int button = key == 1 ? 0 : key == 2 ? 1 : key == 4 ? 2 : -1;
        if (button >= 0)
            overlay.MouseButton(button, action == 1);
        else if (action == 1 && (key == 236 || key == 237))
            overlay.MouseWheel(key == 236 ? 1.0f : -1.0f);
    }

    void Init()
    {
        Import("d3d9.dll", "Direct3DCreate9", Factory9, OriginalFactory9);
        Import("d3d11.dll", "D3D11CreateDeviceAndSwapChain", Create11, OriginalCreate11);
        Import("xinput1_3.dll", "XInputGetState@3", Pad, OriginalPad);
    }
}
