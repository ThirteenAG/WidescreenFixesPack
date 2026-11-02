module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module DeveloperOverlay;
import OverlayD3D7;
import Frame;
import Window;
import Updates;


export namespace KnightRider2Overlay
{
    inline D3D7Overlay Overlay;
    inline bool HotkeyDown = false;
    inline HWND AttachedWindow = nullptr;
    inline bool SystemInteraction = false;
    inline SafetyHookInline KeyboardHook, MouseHook;
    inline uint8_t* KeyboardDevice = nullptr;
    inline uint8_t* MouseDevice = nullptr;
    inline void (*ReadKeyboard)(void*) = nullptr;
    inline void (*ReadMouse)(void*) = nullptr;
    inline std::array<uint8_t, 256> PreviousKeys{};
    inline bool NativeChord = false;

    inline void ReadMouseState(uint8_t* device)
    {
        MouseDevice = device;
        const auto state = device + 124;
        for (int button = 0; button < 3; ++button)
            Overlay.MouseButton(button, (state[12 + button] & 0x80) != 0);
        Overlay.MouseWheel(static_cast<float>(*reinterpret_cast<int32_t*>(state + 8)) / WHEEL_DELTA);
        if ((state[12] || state[13] || *reinterpret_cast<int32_t*>(state) || *reinterpret_cast<int32_t*>(state + 4))
            && KnightRider2Frame::KeyboardInput)
            KnightRider2Frame::KeyboardInput();
        if (Overlay.BlocksInput())
            std::memset(state, 0, 16);
    }

    inline void ReadKeys(uint8_t* device)
    {
        KeyboardDevice = device;
        const auto keys = device + 120;
        NativeChord = (keys[0x1D] || keys[0x9D]) && keys[0x44];
        Overlay.Keyboard(keys, PreviousKeys.data());
        for (size_t i = 0; i < PreviousKeys.size(); ++i)
        {
            if ((keys[i] & 0x80) && !(PreviousKeys[i] & 0x80) && KnightRider2Frame::KeyboardInput)
                KnightRider2Frame::KeyboardInput();
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
            && KnightRider2Frame::KeyboardInput)
            KnightRider2Frame::KeyboardInput();
        if (message == WM_ENTERSIZEMOVE || message == WM_ENTERMENULOOP)
            SystemInteraction = true;
        else if (message == WM_EXITSIZEMOVE || message == WM_EXITMENULOOP)
            SystemInteraction = false;
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP)
            && wParam == VK_F10)
            return 0;
        if (message == WM_DESTROY)
            KnightRider2Updates::OnExit();
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
        const auto device = *reinterpret_cast<IDirect3DDevice7**>(renderer + 48);
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

    inline int __fastcall Keyboard(uint8_t* device, void*)
    {
        const auto result = KeyboardHook.thiscall<int>(device);
        ReadKeys(device);
        return result;
    }

    inline int __fastcall Mouse(uint8_t* device, void*)
    {
        const auto result = MouseHook.thiscall<int>(device);
        ReadMouseState(device);
        return result;
    }

    inline void Init()
    {
        auto keyboard = hook::pattern("51 56 8B F1 8B 46 3C 85 C0 57 75 33 68 ? ? ? ? 8D 4C 24 0C E8 ? ? ? ? 8D 44 24 08 50 68 08 01 00 00 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 8D 4C 24 08 E8 ? ? ? ? 8B 46 3C 8B 08 8D 7E 78");
        KeyboardHook = safetyhook::create_inline(keyboard.get_first(), Keyboard);
        ReadKeyboard = +[](void* device) { KeyboardHook.thiscall<int>(device); };
        auto mouse = hook::pattern("51 56 8B F1 8B 46 3C 85 C0 57 75 33 68 ? ? ? ? 8D 4C 24 0C E8 ? ? ? ? 8D 44 24 08 50 68 08 01 00 00 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 8D 4C 24 08 E8 ? ? ? ? 8B 46 3C 8B 08 8D 7E 7C");
        MouseHook = safetyhook::create_inline(mouse.get_first(), Mouse);
        ReadMouse = +[](void* device) { MouseHook.thiscall<int>(device); };
        asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
        {
            static const auto build = BuildTime();
            ui.Value("Version", rsc_ProductVersion);
            ui.Value("BuildTimeUtc", build.c_str());
            ui.Value("Graphics API", "Direct3D 7");
            ui.Value("Resolution", std::format("{} x {}", KnightRider2Window::Width, KnightRider2Window::Height).c_str());
        });
        KnightRider2Frame::OverlayFrame = Frame;
        KnightRider2Frame::InputBlocked = []()
        {
            return Overlay.BlocksInput();
        };
    }
}
