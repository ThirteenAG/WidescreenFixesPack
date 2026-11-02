module;
#include <stdafx.h>
#include "overlay/OverlayD3D9.hpp"
#include "FixSettings.hpp"

export module DeveloperOverlay;
import Window;
import Focus;

#pragma comment(lib, "version.lib")

export namespace OnimushaOverlay
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline IDirect3DDevice9* Device = nullptr;
    inline bool Hotkey = false;
    inline bool Reopen = false;
    inline std::function<void()> InputCreated;
    inline SafetyHookMid Destroy;
    inline SafetyHookInline EnumHook, InputHook;
    inline std::mutex InputLock;
    inline std::map<void*, SafetyHookInline> StateHooks, DataHooks;
    inline HRESULT(WINAPI* NativeInput)(HINSTANCE, DWORD, REFIID, void**, IUnknown*) = nullptr;
    using Enumerate = BOOL(CALLBACK*)(const void*, void*, DWORD, DWORD, void*);

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
        DWORD unused = 0;
        const auto size = GetFileVersionInfoSizeW(path, &unused);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }

    inline bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    inline void Neutralize(uint32_t* state)
    {
        std::fill_n(state, 22, 0u);
        state[22] = 0xFFFFFFFF;
    }

    inline int __fastcall Input(void* input, void*, uint32_t* state)
    {
        // Buffered DirectInput does not resend a held control after its events have been drained.
        if (BlocksInput())
            Neutralize(state);
        return InputHook.unsafe_thiscall<int>(input, state);
    }

    inline HRESULT WINAPI State(void* device, DWORD size, void* data)
    {
        using Read = HRESULT(WINAPI*)(void*, DWORD, void*);
        Read read = nullptr;
        {
            const std::lock_guard lock(InputLock);
            read = StateHooks.at((*reinterpret_cast<void***>(device))[9]).original<Read>();
        }
        const auto result = read(device, size, data);
        if (SUCCEEDED(result) && data && BlocksInput())
            std::memset(data, 0, size);
        return result;
    }

    inline HRESULT WINAPI Data(void* device, DWORD size, void* data, DWORD* count, DWORD flags)
    {
        using Read = HRESULT(WINAPI*)(void*, DWORD, void*, DWORD*, DWORD);
        Read read = nullptr;
        {
            const std::lock_guard lock(InputLock);
            read = DataHooks.at((*reinterpret_cast<void***>(device))[10]).original<Read>();
        }
        const auto result = read(device, size, data, count, flags);
        if (SUCCEEDED(result) && count && BlocksInput())
            *count = 0;
        return result;
    }

    struct Enumeration
    {
        Enumerate callback;
        void* context;
    };

    inline BOOL CALLBACK Enumerated(const void* instance, void* device, DWORD flags, DWORD remaining, void* context)
    {
        if (device)
        {
            const auto methods = *reinterpret_cast<void***>(device);
            const std::lock_guard lock(InputLock);
            if (!StateHooks.contains(methods[9]))
                StateHooks.emplace(methods[9], safetyhook::create_inline(methods[9], State));
            if (!DataHooks.contains(methods[10]))
                DataHooks.emplace(methods[10], safetyhook::create_inline(methods[10], Data));
        }
        const auto& enumeration = *static_cast<const Enumeration*>(context);
        return enumeration.callback(instance, device, flags, remaining, enumeration.context);
    }

    inline HRESULT WINAPI Enum(void* input, const void* user, void* actions, Enumerate callback, void* context, DWORD flags)
    {
        if (!callback)
            return EnumHook.unsafe_stdcall<HRESULT>(input, user, actions, callback, context, flags);
        Enumeration enumeration{ callback, context };
        return EnumHook.unsafe_stdcall<HRESULT>(input, user, actions, Enumerated, &enumeration, flags);
    }

    inline HRESULT WINAPI CreateInput(HINSTANCE instance, DWORD version, REFIID iid, void** input, IUnknown* outer)
    {
        const auto result = NativeInput(instance, version, iid, input, outer);
        if (SUCCEEDED(result) && InputCreated)
            InputCreated();
        if (SUCCEEDED(result) && input && *input && !EnumHook)
        {
            const auto methods = *reinterpret_cast<void***>(*input);
            EnumHook = safetyhook::create_inline(methods[9], Enum);
        }
        return result;
    }

    inline bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || Hotkey))
        {
            result = 0;
            return true;
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    inline void Present(IDirect3DDevice9* device)
    {
        Device = device;
        static const bool info = []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto built = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", built.c_str());
                ui.Value("Graphics API", "Direct3D 9");
                ui.Value("Resolution", std::to_string(OnimushaWindow::RenderWidth) + " x " + std::to_string(OnimushaWindow::RenderHeight));
            });
            return true;
        }();
        const bool down = OnimushaWindow::Window && GetForegroundWindow() == OnimushaWindow::Window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down && !Hotkey)
            Overlay.SetOpen(!Overlay.IsOpen());
        Hotkey = down;
        Overlay.Present(device, Module());
        if (Reopen)
        {
            Overlay.SetOpen(true);
            Reopen = false;
        }
    }

    inline void Init()
    {
        InputHook = safetyhook::create_inline(hook::pattern("81 EC 1C 03 00 00 80 3D ? ? ? ? 01 57 8B F9 89 7C 24 10 75 0E").get_first(), Input);
        // DXUT requires its final Release to reach zero when it recreates or destroys the device.
        // The ImGui backend owns a device reference even while its panel is closed.
        Destroy = safetyhook::create_mid(hook::pattern("51 55 88 4C 24 04 E8 ? ? ? ? 8B E8 85 ED 0F 84").get_first(),
            [](SafetyHookContext&)
            {
                Reopen = Reopen || Overlay.IsOpen();
                Overlay.ShutdownRenderer();
                Device = nullptr;
            });
        auto original = IATHook::Replace(GetModuleHandleW(nullptr), "DINPUT8.dll",
            std::pair{ "DirectInput8Create", reinterpret_cast<void*>(&CreateInput) });
        NativeInput = reinterpret_cast<decltype(NativeInput)>(original.at("DirectInput8Create").get());
        const auto draw = OnimushaWindow::Draw;
        OnimushaWindow::Draw = [draw](IDirect3DDevice9* device)
        {
            if (draw)
                draw(device);
            Present(device);
        };
        OnimushaWindow::Filter = Message;
        OnimushaFocus::BlocksInput = BlocksInput;
        OnimushaWindow::SetBeforeReset([](IDirect3DDevice9* device)
        {
            if (device == Device)
                Overlay.BeforeReset();
        });
    }
}
