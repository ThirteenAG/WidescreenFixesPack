module;
#include <stdafx.h>
#include "Shared/CursorConfinement.hpp"
#include "FixSettings.hpp"

export module DeveloperOverlay;
import OverlayD3D7;
import Frame;
import Window;
import Updates;


export namespace KnightRiderOverlay
{
    inline D3D7Overlay Overlay;
    inline bool HotkeyDown = false;
    inline HWND AttachedWindow = nullptr;
    inline bool SystemInteraction = false;
    inline SafetyHookMid KeyboardHook, MouseHook;
    inline uint8_t* KeyboardDevice = nullptr;
    inline uint8_t* MouseDevice = nullptr;
    inline void (__thiscall* ReadKeyboard)(void*) = nullptr;
    inline void (__thiscall* ReadMouse)(void*) = nullptr;
    inline std::array<uint8_t, 256> PreviousKeys{};
    inline bool NativeChord = false;

    inline void ReadMouseState(uint8_t* device)
    {
        MouseDevice = device;
        const auto state = device + 0x88;
        for (int button = 0; button < 3; ++button)
            Overlay.MouseButton(button, (state[12 + button] & 0x80) != 0);
        Overlay.MouseWheel(static_cast<float>(*reinterpret_cast<int32_t*>(state + 8)) / WHEEL_DELTA);
        if ((state[12] || state[13] || *reinterpret_cast<int32_t*>(state) || *reinterpret_cast<int32_t*>(state + 4))
            && KnightRiderFrame::KeyboardInput)
            KnightRiderFrame::KeyboardInput();
        if (Overlay.BlocksInput())
            std::memset(state, 0, 16);
    }

    inline void ReadKeys(uint8_t* device)
    {
        KeyboardDevice = device;
        const auto keys = device + 0x84;
        NativeChord = (keys[0x1D] || keys[0x9D]) && keys[0x44];
        Overlay.Keyboard(keys, PreviousKeys.data());
        for (size_t i = 0; i < PreviousKeys.size(); ++i)
        {
            if ((keys[i] & 0x80) && !(PreviousKeys[i] & 0x80) && KnightRiderFrame::KeyboardInput)
                KnightRiderFrame::KeyboardInput();
            PreviousKeys[i] = keys[i];
        }
        if (Overlay.BlocksInput())
            std::memset(keys, 0, PreviousKeys.size());
        else if (NativeChord)
            keys[0x1D] = keys[0x9D] = keys[0x44] = 0;
    }

    inline HMODULE Module()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Module), &module);
        return module;
    }

    inline std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(Module(), path, DWORD(std::size(path)));
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

    inline LRESULT CALLBACK Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (message == WM_CHAR && Overlay.CharacterMessage(static_cast<wchar_t>(wParam)))
            return 0;
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN)
            && KnightRiderFrame::KeyboardInput)
            KnightRiderFrame::KeyboardInput();
        if (message == WM_ENTERSIZEMOVE || message == WM_ENTERMENULOOP)
            SystemInteraction = true;
        else if (message == WM_EXITSIZEMOVE || message == WM_EXITMENULOOP)
            SystemInteraction = false;
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP)
            && wParam == VK_F10)
            return 0;
        if (message == WM_DESTROY)
            KnightRiderUpdates::OnExit();
        if (message == WM_NCDESTROY)
        {
            Overlay.ShutdownRenderer();
            RemoveWindowSubclass(window, Message, 0x4B52);
            AttachedWindow = nullptr;
        }
        LRESULT result = 0;
        if (Overlay.Message(window, message, wParam, lParam, result))
            return result;
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline void Frame(uint8_t* renderer)
    {
        const auto window = WindowedModeWrapper::GameHWND;
        const auto device = *reinterpret_cast<IDirect3DDevice7**>(renderer + 40);
        // The game suspends its normal input update while the overlay is open.
        if (Overlay.BlocksInput() && KeyboardDevice)
        {
            ReadKeyboard(KeyboardDevice);
            ReadKeys(KeyboardDevice);
        }
        if (Overlay.BlocksInput() && MouseDevice)
        {
            ReadMouse(MouseDevice);
            ReadMouseState(MouseDevice);
        }
        Overlay.Present(device, window, Module());
        if (window && window != AttachedWindow && SetWindowSubclass(window, Message, 0x4B52, 0))
            AttachedWindow = window;
        const bool down = window && GetForegroundWindow() == window
            && (NativeChord || ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000)));
        if (down && !HotkeyDown && Overlay.Available())
        {
            Overlay.SetOpen(!Overlay.IsOpen());
        }
        HotkeyDown = down;
        CursorConfinement::Suspend(Overlay.BlocksInput() || SystemInteraction);
    }

    inline void Init()
    {
        auto keyboard = hook::pattern("8B 44 24 04 56 8B F1 50 E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 5E C2 04 00 90 53 56 57 8B 7C 24 1C 66 83 FF 01");
        const auto address = keyboard.get_first<uint8_t>(13);
        ReadKeyboard = reinterpret_cast<decltype(ReadKeyboard)>(address + 7 + *reinterpret_cast<int32_t*>(address + 3));
        KeyboardHook = safetyhook::create_mid(address + 7, [](SafetyHookContext& context)
        {
            ReadKeys(reinterpret_cast<uint8_t*>(context.esi));
        });
        auto mouse = hook::pattern("8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 5E C2 04 00");
        const auto mouseAddress = mouse.get_first<uint8_t>();
        ReadMouse = reinterpret_cast<decltype(ReadMouse)>(mouseAddress + 7 + *reinterpret_cast<int32_t*>(mouseAddress + 3));
        MouseHook = safetyhook::create_mid(mouseAddress + 7, [](SafetyHookContext& context)
        {
            ReadMouseState(reinterpret_cast<uint8_t*>(context.esi));
        });
        asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
        {
            static const auto build = BuildTime();
            ui.Value("Version", rsc_ProductVersion);
            ui.Value("BuildTimeUtc", build.c_str());
            ui.Value("Graphics API", "Direct3D 7");
            ui.Value("Resolution", std::format("{} x {}", KnightRiderWindow::Width, KnightRiderWindow::Height).c_str());
        });
        KnightRiderFrame::OverlayFrame = Frame;
        KnightRiderFrame::InputBlocked = []()
        {
            const auto window = WindowedModeWrapper::GameHWND;
            return Overlay.BlocksInput() || (window && GetForegroundWindow() != window && FixSettings().Bool("MAIN.PauseOnFocusLoss"));
        };
    }
}
