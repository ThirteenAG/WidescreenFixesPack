#include "stdafx.h"
#include "DeveloperOverlay.hpp"
#include "overlay/OverlayD3D9.hpp"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "version.lib")

namespace MaxPayneOverlay
{
    namespace
    {
        asi_overlay::D3D9Overlay Overlay;
        HMODULE Host = nullptr;
        HWND Window = nullptr;
        bool LegacyWheel = false;
        bool RawMouse = false;
        UINT Width = 0, Height = 0;
        constexpr UINT_PTR SubclassId = 0x4D504F56;

        std::string VersionString(const wchar_t* key)
        {
            wchar_t path[MAX_PATH * 4]{};
            GetModuleFileNameW(Host, path, static_cast<DWORD>(std::size(path)));
            DWORD unused = 0;
            DWORD size = GetFileVersionInfoSizeW(path, &unused);
            std::vector<uint8_t> data(size);
            if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
                return "Unavailable";
            struct Translation
            {
                WORD language, codepage;
            };
            Translation* translations = nullptr;
            UINT length = 0;
            if (!VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void**>(&translations),
                                &length) ||
                length < sizeof(Translation))
                return "Unavailable";
            wchar_t query[128]{};
            swprintf_s(query, L"\\StringFileInfo\\%04x%04x\\%s", translations[0].language, translations[0].codepage,
                       key);
            wchar_t* value = nullptr;
            if (!VerQueryValueW(data.data(), query, reinterpret_cast<void**>(&value), &length) || !value)
                return "Unavailable";
            return asi_settings::text::Narrow(value);
        }

        LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
        {
            if (message == WM_MOUSEWHEEL && Overlay.IsOpen())
                LegacyWheel = true;
            LRESULT result = 0;
            if (Overlay.Message(window, message, wParam, lParam, result))
                return result;
            if (message == WM_NCDESTROY)
            {
                RemoveWindowSubclass(window, WindowProc, SubclassId);
                if (Window == window)
                    Window = nullptr;
            }
            return DefSubclassProc(window, message, wParam, lParam);
        }

        void Initialize()
        {
            Overlay.pretransformed = true;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCWSTR>(&Initialize), &Host);
            static auto version = VersionString(L"ProductVersion");
            static auto built = VersionString(L"BuildTimeUtc");
            asi_overlay::Overlay::AddPage("Debug", "Info",
                                          [](asi_overlay::OverlayUI& ui)
                                          {
                                              ui.Value("Version", version.c_str());
                                              ui.Value("Build time (UTC)", built.c_str());
                                              ui.Value("Graphics API", "Direct3D 9");
                                              auto resolution = std::to_string(Width) + " x " + std::to_string(Height);
                                              ui.Value("Resolution", resolution.c_str());
                                          });
        }

        bool HasRawMouse(HWND window)
        {
            UINT count = 0;
            if (GetRegisteredRawInputDevices(nullptr, &count, sizeof(RAWINPUTDEVICE)) == UINT(-1) || !count)
                return false;
            std::vector<RAWINPUTDEVICE> devices(count);
            if (GetRegisteredRawInputDevices(devices.data(), &count, sizeof(RAWINPUTDEVICE)) == UINT(-1))
                return false;
            return std::any_of(devices.begin(), devices.end(),
                               [window](const RAWINPUTDEVICE& device)
                               {
                                   return device.usUsagePage == 1 && device.usUsage == 2 &&
                                          (!device.hwndTarget || device.hwndTarget == window);
                               });
        }
    } // namespace

    bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    void Gamepad(uint16_t buttons, int8_t x, int8_t y)
    {
        Overlay.Gamepad(buttons, x, y);
    }

    void Mouse(const uint8_t* state)
    {
        if (!Overlay.IsOpen())
            return;
        // The overlay reads registered raw packets itself, including short button presses between device polls.
        if (!RawMouse)
        {
            for (int i = 0; i < 5; ++i)
                Overlay.MouseButton(i, (state[12 + i] & 0x80) != 0);
            if (!LegacyWheel)
                Overlay.MouseWheel(float(*reinterpret_cast<const LONG*>(state + 8)) / WHEEL_DELTA);
        }
    }

    void Draw(void* current)
    {
        auto device = static_cast<IDirect3DDevice9*>(current);
        if (!device)
            return;
        static std::once_flag initialized;
        std::call_once(initialized, Initialize);
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        if (FAILED(device->GetCreationParameters(&parameters)))
            return;
        auto window = parameters.hFocusWindow;
        if (Window && Window != window)
            Release();
        RawMouse = HasRawMouse(window);
        if (Overlay.IsOpen())
        {
            IDirect3DSurface9* surface = nullptr;
            if (SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
            {
                D3DSURFACE_DESC desc{};
                if (SUCCEEDED(surface->GetDesc(&desc)))
                {
                    Width = desc.Width;
                    Height = desc.Height;
                }
                surface->Release();
            }
        }
        Overlay.Present(device, Host);
        if (Window != window && Overlay.context && SetWindowSubclass(window, WindowProc, SubclassId, 0))
            Window = window;
    }

    void BeforeReset()
    {
        Overlay.BeforeReset();
    }

    void Release()
    {
        if (Window)
            RemoveWindowSubclass(Window, WindowProc, SubclassId);
        Window = nullptr;
        LegacyWheel = false;
        RawMouse = false;
        Overlay.ShutdownRenderer();
    }
} // namespace MaxPayneOverlay
