module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <map>
#include <mutex>

export module Runtime;

import ComVars;
import DeveloperOverlay;
import Frontend;

export namespace ColdFearRuntime
{
    inline HWND Window = nullptr;
    inline WNDPROC OriginalWindowProc = nullptr;
    
    inline char (__cdecl* Pause)(char) = nullptr;
    inline char (__cdecl* Resume)() = nullptr;
    inline bool OwnFocusPause = false;
    inline bool Ready = false;
    inline std::string Notice;
    inline SafetyHookInline InputDeviceHook;
    inline std::mutex InputHooksMutex;
    inline std::map<void*, SafetyHookInline> InputStateHooks, InputDataHooks;
    inline HRESULT (WINAPI* InputCreate)(HINSTANCE, DWORD, const IID*, void**, void*) = nullptr;

    inline bool BlocksInput()
    {
        return ColdFearOverlay::BlocksInput() || (Window && GetForegroundWindow() != Window);
    }

    inline HRESULT WINAPI InputState(void* device, DWORD size, void* data)
    {
        SafetyHookInline* original;
        {
            std::lock_guard lock(InputHooksMutex);
            original = &InputStateHooks.at((*reinterpret_cast<void***>(device))[9]);
        }
        const auto result = original->unsafe_stdcall<HRESULT>(device, size, data);
        if (SUCCEEDED(result) && BlocksInput())
        {
            if (data)
                std::memset(data, 0, size);
            return HRESULT(0x8007001E);
        }
        return result;
    }

    inline HRESULT WINAPI InputData(void* device, DWORD size, void* data, DWORD* count, DWORD flags)
    {
        const bool blocked = BlocksInput();
        SafetyHookInline* original;
        {
            std::lock_guard lock(InputHooksMutex);
            original = &InputDataHooks.at((*reinterpret_cast<void***>(device))[10]);
        }
        const auto result = original->unsafe_stdcall<HRESULT>(device, size, data, count, blocked ? flags & ~1u : flags);
        if (SUCCEEDED(result) && blocked)
        {
            if (count)
                *count = 0;
            return HRESULT(0x8007001E);
        }
        return result;
    }

    inline HRESULT WINAPI InputDevice(void* input, const GUID* guid, void** device, void* outer)
    {
        const auto result = InputDeviceHook.unsafe_stdcall<HRESULT>(input, guid, device, outer);
        if (SUCCEEDED(result) && device && *device)
        {
            const auto table = *reinterpret_cast<void***>(*device);
            std::lock_guard lock(InputHooksMutex);
            if (auto [entry, added] = InputStateHooks.try_emplace(table[9]); added)
                entry->second = safetyhook::create_inline(table[9], InputState);
            if (auto [entry, added] = InputDataHooks.try_emplace(table[10]); added)
                entry->second = safetyhook::create_inline(table[10], InputData);
        }
        return result;
    }

    inline HRESULT WINAPI CreateInput(HINSTANCE instance, DWORD version, const IID* iid, void** output, void* outer)
    {
        const auto result = InputCreate(instance, version, iid, output, outer);
        if (SUCCEEDED(result) && output && *output && !InputDeviceHook)
            InputDeviceHook = safetyhook::create_inline((*reinterpret_cast<void***>(*output))[3], InputDevice);
        return result;
    }

    inline void SyncFocus()
    {
        if (!Ready || !Window || !Paused.is_initialized() || !Pause || !Resume)
            return;
        const bool wanted = FixSettings().Bool("MAIN.PauseOnFocusLoss") &&
            (GetForegroundWindow() != Window || IsIconic(Window));
        if (!wanted && OwnFocusPause)
        {
            Resume();
            OwnFocusPause = false;
        }
        else if (wanted && !OwnFocusPause && !Paused[0] && !Paused[1])
        {
            Pause(1);
            OwnFocusPause = true;
        }
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;
        if (ColdFearOverlay::Message(window, message, wParam, lParam, result))
            return result;
        const bool focusMessage = message == WM_ACTIVATEAPP || message == WM_ACTIVATE || message == WM_SIZE;
        const bool gainingFocus = (message == WM_ACTIVATEAPP && wParam) ||
            (message == WM_ACTIVATE && LOWORD(wParam) != WA_INACTIVE);
        if (gainingFocus && OwnFocusPause)
        {
            Resume();
            OwnFocusPause = false;
        }
        if (message == WM_DESTROY)
            RWGTA::UpdateCheck::OnExit();
        const auto native = CallWindowProcW(OriginalWindowProc, window, message, wParam, lParam);
        if (focusMessage && !gainingFocus)
            SyncFocus();
        return native;
    }

    inline void TrackWindow(HWND window)
    {
        if (!window || Window == window)
            return;
        Window = window;
        OriginalWindowProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(WindowProc)));
    }

    inline std::atomic<int> requestedFrameLimit{ 0 };

    inline void UpdateFrameLimit()
    {
        requestedFrameLimit.store(std::clamp(FixSettings().Int("MAIN.FPSLimit"), -1, 1000), std::memory_order_relaxed);
    }

    inline void ApplyFrameLimit(HWND target)
    {
        const int requested = requestedFrameLimit.load(std::memory_order_relaxed);
        if (requested >= 0)
        {
            D3D9Options::frameLimit = requested > 0 ? std::clamp(requested, 30, 1000) : 0;
            return;
        }
        static HMONITOR previous = nullptr;
        static ULONGLONG lastCheck = 0;
        static int refresh = 60;
        const auto monitor = MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount64();
        if (monitor != previous || now - lastCheck >= 2000)
        {
            previous = monitor;
            lastCheck = now;
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            refresh = GetMonitorInfoW(monitor, &info) &&
                EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ?
                int(mode.dmDisplayFrequency) : 60;
        }
        D3D9Options::frameLimit = refresh;
    }

    inline void InstallRetailController()
    {
        // Use the updated default control layout and clear transient controller state after each input poll.
        auto controls = hook::pattern("56 8B F1 8B 86 BC 08 00 00 48 0F 84 ? ? ? ? 83 E8 02 0F 84 ? ? ? ? 48 0F 85");
        auto code = controls.get_first<uint8_t>();
        const auto bindButton = injector::GetBranchDestination(code + 0x30).as_int();
        const auto bindAxis = injector::GetBranchDestination(code + 0x19D).as_int();
        for (const auto offset : { 0xA, 0x13, 0x1A })
            injector::MakeNOP(code + offset, 6, true);
        injector::MakeJMP(code + 0x1D7, code + 0x2E9, true);
        injector::MakeCALL(code + 0x2EB, bindAxis, true);
        for (const auto offset : { 0x35B, 0x36F, 0x383 })
            injector::MakeCALL(code + offset, bindButton, true);
        for (const auto offset : { 0x2F9, 0x310, 0x327, 0x33E })
            injector::WriteMemory<uint32_t>(code + offset, 0xFFFFFFFF, true);
        constexpr std::array<std::pair<ptrdiff_t, uint8_t>, 36> bindings
        {{
            { 0x29, 0x1A }, { 0x3E, 0x19 }, { 0x55, 0x1B }, { 0x6C, 0x18 },
            { 0x83, 0x32 }, { 0x9A, 0x33 }, { 0xB1, 0x2D }, { 0xDF, 0x2A },
            { 0xF6, 0x2C }, { 0x10D, 0x2E }, { 0x124, 0x2F }, { 0x13B, 0x30 },
            { 0x152, 0x31 }, { 0x197, 0x07 }, { 0x1AB, 0x08 }, { 0x1BF, 0x10 },
            { 0x1D3, 0x11 }, { 0x2FE, 0x10 }, { 0x315, 0x11 }, { 0x32C, 0x12 },
            { 0x343, 0x13 }, { 0x355, 0xFF }, { 0x357, 0x14 }, { 0x369, 0xFF },
            { 0x36B, 0x15 }, { 0x37D, 0xFF }, { 0x37F, 0x16 }, { 0x391, 0x11 },
            { 0x5AF, 0x1C }, { 0x6E5, 0x04 }, { 0x6F9, 0x04 }, { 0x70D, 0x04 },
            { 0x71B, 0x04 }, { 0x735, 0x04 }, { 0x749, 0x04 }, { 0x75D, 0x04 },
        }};
        for (const auto [offset, value] : bindings)
            injector::WriteMemory<uint8_t>(code + offset, value, true);
        auto reset = hook::pattern("8B 76 04 56 8B 06 FF 50 1C 5F 5E 8B E5 5D C3");
        static auto ResetControllerState = safetyhook::create_mid(reset.get_first(), [](SafetyHookContext& regs)
        {
            std::memset(reinterpret_cast<void*>(regs.esi + 0x24), 0, 3);
        });

    }

    inline void Init(bool gog)
    {
        auto pause = hook::pattern("55 8B EC A0 ? ? ? ? 84 C0 8A 45 08 74 ? 84 C0 74 ? 68 CD CC CC 3D B9");
        auto resume = hook::pattern("A0 ? ? ? ? 84 C0 75 08 C6 05 ? ? ? ? 00 C3 C6 05 ? ? ? ? 00 E8");
        Pause = reinterpret_cast<decltype(Pause)>(pause.get_first());
        Resume = reinterpret_cast<decltype(Resume)>(resume.get_first());
        Paused.SetAddress(*pause.get_first<uint8_t*>(4));

        auto imports = IATHook::Replace(GetModuleHandleW(nullptr), "DINPUT8.DLL",
            std::forward_as_tuple("DirectInput8Create", reinterpret_cast<void*>(CreateInput)));
        InputCreate = reinterpret_cast<decltype(InputCreate)>(imports.at("DirectInput8Create").get());
        if (!gog)
        {
            wchar_t path[MAX_PATH]{};
            GetModuleFileNameW(ColdFearOverlay::ThisModule(), path, DWORD(std::size(path)));
            const auto library = std::filesystem::path(path).parent_path().parent_path() / L"Xidi.32.dll";
            if (const auto xidi = LoadLibraryW(library.c_str()))
            {
                if (const auto create = GetProcAddress(xidi, "dinput8_DirectInput8Create"))
                    InputCreate = reinterpret_cast<decltype(InputCreate)>(create);
            }
        }

        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        D3D9Options::frame = [](IDirect3DDevice9* device)
        {
            Ready = true;
            if (!Window)
            {
                D3DDEVICE_CREATION_PARAMETERS parameters{};
                if (SUCCEEDED(device->GetCreationParameters(&parameters)))
                    TrackWindow(parameters.hFocusWindow);
            }
            SyncFocus();
            ApplyFrameLimit(Window);
            RWGTA::UpdateCheck::Tick();
            ColdFearMenu::Get().notice = FixSettings().Bool("MAIN.CheckForUpdates") ? Notice : "";
            CursorConfinement::Suspend(ColdFearOverlay::BlocksInput());
            CursorConfinement::Frame(Window);
            ColdFearOverlay::Present(device, Window);
        };
        D3D9Options::beforeReset = ColdFearOverlay::BeforeReset;
        D3D9Options::HookCreation();

        RWGTA::UpdateCheck::Config update;
        update.pluginId = "ColdFear.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []() { return Window != nullptr; };
        update.show = [](const std::wstring& line) { Notice = asi_settings::text::Narrow(line); };
        RWGTA::UpdateCheck::Init(std::move(update));
    }
}
