module;

#include <stdafx.h>
#include <commctrl.h>
#include <d3d9.h>
#include <dinput.h>
#include <intrin.h>
#include <cwchar>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/DirectInputRawMouse.hpp"
#include "Shared/WindowCursor.hpp"
#include "include/WindowStyle.hpp"
#include "rawinput.hpp"

export module Window;

import ComVars;
import Layout;
import Draw;
import Cutscenes;
import DeveloperOverlay;
import FrameRate;

// Window modes, focus and the per-frame work on the game's render thread
namespace MatrixWindow
{
    int Mode = 0;
    std::atomic<HWND> Window{ nullptr };
    std::atomic<bool> PauseOnFocusLoss{ true };
    constexpr UINT FocusSettingsMessage = WM_APP + 0x2A1;
    constexpr UINT_PTR SubclassId = 0;
    SafetyHookInline RegisterRawMouseHook;
    SafetyHookMid MouseSampleHook;
    uint32_t MouseStateOffset = 0;

    BOOL WINAPI RegisterRawMouse(PCRAWINPUTDEVICE devices, UINT count, UINT size)
    {
        HMODULE caller = nullptr;
        wchar_t path[MAX_PATH]{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(_ReturnAddress()), &caller))
            return RegisterRawMouseHook.stdcall<BOOL>(devices, count, size);
        const auto length = GetModuleFileNameW(caller, path, MAX_PATH);
        const auto name = length && length < MAX_PATH ? std::wcsrchr(path, L'\\') : nullptr;
        if (!name || (_wcsicmp(name + 1, L"dinput8.dll") != 0 && _wcsicmp(name + 1, L"dinput.dll") != 0))
            return RegisterRawMouseHook.stdcall<BOOL>(devices, count, size);

        return DirectInputRawMouse::RegisterDevices(devices, count, size, [](PCRAWINPUTDEVICE list, UINT entries, UINT bytes)
        {
            return RegisterRawMouseHook.stdcall<BOOL>(list, entries, bytes);
        });
    }

    void MouseSample(SafetyHookContext& context)
    {
        if (!DirectInputRawMouse::Active())
            return;
        auto* input = *reinterpret_cast<uint8_t**>(context.ebp - 8);
        auto& mouse = *reinterpret_cast<DIMOUSESTATE2*>(input + MouseStateOffset);
        const auto window = Window.load();
        const auto state = DirectInputRawMouse::Take();
        if (!window || window != GetForegroundWindow())
        {
            mouse = {};
            return;
        }
        mouse.lX = state.x;
        mouse.lY = state.y;
        mouse.lZ = state.wheel;
        for (size_t button = 0; button < 5; ++button)
            mouse.rgbButtons[button] = ((state.down | state.pressed) & (1u << button)) ? 0x80 : 0;
    }

    void InstallMouse()
    {
        auto input = hook::pattern("55 8B EC 83 EC 08 89 4D F8 8B 45 F8 05 ? ? 00 00 50 6A 14");
        MouseStateOffset = *input.get_first<uint32_t>(13);
        // The native poll has succeeded and is about to translate the wheel into button actions.
        auto sample = hook::pattern("8B 55 F8 83 BA ? ? 00 00 00 7E ? 8B 45 F8 C6 80 ? ? 00 00 FF");
        MouseSampleHook = safetyhook::create_mid(sample.get_first(), MouseSample);
        DirectInputRawMouse::Start();
        RegisterRawMouseHook = safetyhook::create_inline(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "RegisterRawInputDevices"), RegisterRawMouse);
    }

    std::pair<int32_t, int32_t> MonitorSize()
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &info);
        return { info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
    }

    LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR appActive)
    {
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Procedure, SubclassId);
            auto current = window;
            Window.compare_exchange_strong(current, nullptr);
            return DefSubclassProc(window, message, wParam, lParam);
        }
        if (message == WM_SIZE && wParam == SIZE_MINIMIZED && window == Window.load())
            CursorConfinement::Release();
        LRESULT result = 0;
        if (MatrixOverlay::Message(window, message, wParam, lParam, result))
            return result;
        if (message == WM_ACTIVATEAPP)
        {
            if (window == Window.load())
                DirectInputRawMouse::Clear();
            SetWindowSubclass(window, Procedure, SubclassId, wParam != FALSE);
            wParam = wParam || (Mode != 0 && !PauseOnFocusLoss);
        }
        if (message == FocusSettingsMessage)
            return DefSubclassProc(window, WM_ACTIVATEAPP, appActive || !PauseOnFocusLoss, 0);
        return DefSubclassProc(window, message, wParam, lParam);
    }

    bool Attach(HWND window)
    {
        DWORD_PTR appActive = 0;
        if (!GetWindowSubclass(window, Procedure, SubclassId, &appActive))
        {
            DWORD foregroundProcess = 0;
            GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
            if (!SetWindowSubclass(window, Procedure, SubclassId, foregroundProcess == GetCurrentProcessId()))
                return false;
        }
        Window = window;
        return true;
    }

    // The game's window frame counted a menu bar it doesn't have
    BOOL WINAPI AdjustRect(LPRECT rect, DWORD style, BOOL)
    {
        return AdjustWindowRect(rect, style, FALSE);
    }

    // Style and size for the window modes: the client area is exactly the back buffer
    void Place(HWND window, LONG width, LONG height, bool force)
    {
        if (width <= 0 || height <= 0 || IsIconic(window))
            return;
        const DWORD style = Mode == 2 ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        RECT client{};
        GetClientRect(window, &client);
        const DWORD current = static_cast<DWORD>(GetWindowLongW(window, GWL_STYLE)) & ~(WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        const auto monitor = WindowStyle::MonitorRect(window);
        POINT origin{};
        ClientToScreen(window, &origin);
        const bool aligned = !WindowStyle::CoversMonitor(monitor, width, height) ||
            (origin.x == monitor.left && origin.y == monitor.top);
        if (!force && current == style && client.right == width && client.bottom == height && aligned)
            return;
        if (current != style)
        {
            SetWindowLongW(window, GWL_STYLE, static_cast<LONG>(style | WS_VISIBLE));
            SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
        WindowStyle::Place(window, width, height);
    }

    void PrepareWindow(D3DPRESENT_PARAMETERS* params, HWND window)
    {
        if (Mode != 0 && window && GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId())
        {
            const auto width = params->BackBufferWidth ? static_cast<LONG>(params->BackBufferWidth) : Screen.nWidth;
            const auto height = params->BackBufferHeight ? static_cast<LONG>(params->BackBufferHeight) : Screen.nHeight;
            Place(window, width, height, true);
        }
    }

    std::pair<LONG, LONG> BackBufferSize(IDirect3DDevice9* device)
    {
        IDirect3DSurface9* surface = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
            return {};
        D3DSURFACE_DESC desc{};
        const bool described = SUCCEEDED(surface->GetDesc(&desc));
        surface->Release();
        if (!described)
            return {};
        return { static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height) };
    }

    void AcceptSize(LONG width, LONG height)
    {
        if (width <= 0 || height <= 0 || (Screen.nWidth == width && Screen.nHeight == height))
            return;
        Screen.nWidth = width;
        Screen.nHeight = height;
        nResolutionWidth = width;
        nResolutionHeight = height;
        Layout::Update();
        Draw::DisplayResized();
        Cutscenes::DisplayResized();
    }

    void AfterReset(IDirect3DDevice9* device)
    {
        const auto [width, height] = BackBufferSize(device);
        AcceptSize(width, height);
    }

    // Every frame before Present
    void Frame(IDirect3DDevice9* device)
    {
        D3DDEVICE_CREATION_PARAMETERS creation{};
        if (FAILED(device->GetCreationParameters(&creation)) || !creation.hFocusWindow)
            return;
        const auto window = creation.hFocusWindow;
        if (GetWindowThreadProcessId(window, nullptr) == GetCurrentThreadId())
        {
            const bool newWindow = window != Window.load();
            if (newWindow && Attach(window))
            {
                if (Mode == 0)
                    WindowStyle::SquareCorners(window);
                else
                    PostMessageW(window, FocusSettingsMessage, 0, 0);
            }
            WindowCursor::Follow(window);
            CursorConfinement::Suspend(MatrixOverlay::Overlay.BlocksInput());
            CursorConfinement::Frame(window);
            if (Mode != 0 || newWindow)
            {
                const auto [width, height] = BackBufferSize(device);
                AcceptSize(width, height);
                if (Mode != 0)
                    Place(window, width, height, newWindow);
            }
            WFP::onGameProcessEvent().executeAll();
            MatrixOverlay::Present(device, window);
        }
        MatrixFrameRate::Frame(window);
    }
}

class Window
{
public:
    Window()
    {
        WFP::onInitEvent() += []()
        {
            WindowCursor::Initialize(FixSettings());
            MessagePumpGuard::Install(GetModuleHandleW(nullptr));
            MatrixWindow::InstallMouse();
            MatrixFrameRate::Init();
            MatrixWindow::Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
            WatchMatrixSetting("MAIN.PauseOnFocusLoss", [](const std::string& value)
            {
                MatrixWindow::PauseOnFocusLoss = value == "1";
                if (const auto window = MatrixWindow::Window.load(); window && MatrixWindow::Mode != 0)
                    PostMessageW(window, MatrixWindow::FocusSettingsMessage, 0, 0);
            });
            IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("AdjustWindowRect", MatrixWindow::AdjustRect));
            MatrixOverlay::Init();
            D3D9Options::presentParameters = MatrixWindow::PrepareWindow;
            D3D9Options::frame = MatrixWindow::Frame;
            D3D9Options::beforeReset = MatrixOverlay::BeforeReset;
            D3D9Options::afterReset = MatrixWindow::AfterReset;
            D3D9Options::HookCreation();

            static ptrdiff_t resHookEnd = 0;
            auto original = hook::pattern("A1 ? ? ? ? 89 45 E0 8B 0D ? ? ? ? 89 4D E4 33 D2");
            if (!original.empty()) // original release
            {
                nResolutionWidth.SetAddress(*original.get_first<int32_t*>(1));
                nResolutionHeight.SetAddress(*original.get_first<int32_t*>(10));
                resHookEnd = 186;
            }
            else // v1.2
            {
                auto updated = hook::pattern("8B ? ? ? ? 00 89 55 E0 A1 ? ? ? ? 89 45 E4 33 C9");
                nResolutionWidth.SetAddress(*updated.get_first<int32_t*>(2));
                nResolutionHeight.SetAddress(*updated.get_first<int32_t*>(10));
                resHookEnd = 237;
            }

            // The game's windowed flag, read from MatrixConfig.ini just before the resolution
            bWindowed.SetAddress(*hook::pattern("8A ? ? ? ? ? ? 68 00 00 CF 00 8D ? D8").get_first<uint8_t*>(2));

            auto pattern = hook::pattern("68 ? ? ? ? 6A 00 68 ? ? ? ? 68 ? ? ? ? FF 15 ? ? ? ? 89 85 EC FA FF FF 8B 8D");
            struct ResHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    std::tie(Screen.nWidth, Screen.nHeight) = MatrixNativeResolution();
                    if (MatrixWindow::Mode == 2)
                        std::tie(Screen.nWidth, Screen.nHeight) = MatrixWindow::MonitorSize();
                    nResolutionWidth = Screen.nWidth;
                    nResolutionHeight = Screen.nHeight;
                    bWindowed = MatrixWindow::Mode != 0;
                    Layout::Update();
                }
            }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(resHookEnd)); //0x7AD916

            // Aspect ratio
            pattern = hook::pattern("D9 05 ? ? ? ? D8 35 ? ? ? ? D9 1D ? ? ? ? 5D C3");
            fAspectRatioDivisor.SetAddress(*pattern.get_first<float*>(8));
            fAspectRatio.SetAddress(*pattern.get_first<float*>(14));
            struct ARHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    fAspectRatioDivisor = 1.0f;
                    fAspectRatio = 1.0f;
                }
            }; injector::MakeInline<ARHook>(pattern.get_first(0), pattern.get_first(18)); //0x6E6024, 0x6E6024+18
        };
        WFP::onShutdownEvent() += []() { WindowCursor::Shutdown(); };
    }
} Window;
