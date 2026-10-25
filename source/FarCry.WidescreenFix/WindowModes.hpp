#pragma once
#include <d3d9.h>
#include "../Shared/CursorConfinement.hpp"

namespace FarCryWindow
{
    struct Layout
    {
        size_t width, height, fullscreen, cvarWidth, cvarHeight, cvarFullscreen, window, savedStyle, windowRect, clientRect, device;
    };
#ifdef _WIN64
    inline constexpr Layout Fields{ 97224, 97228, 139376, 95848, 95856, 95864, 130328, 130724, 130728, 130744, 134744 };
#else
    inline constexpr Layout Fields{ 93524, 93528, 119924, 92204, 92208, 92212, 112148, 112436, 112440, 112456, 116376 };
#endif
    template<class T> inline T& Field(void* renderer, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(renderer) + offset);
    }

    inline constexpr UINT DpiChangedMessage = 0x02E0;
    inline SafetyHookInline CreateHook, ChangeHook;
    inline SafetyHookMid FrameHook;
    inline std::atomic<int> Mode{ 0 }, Pending{ -1 };
    inline int LastWindowMode = 1, Transition = -1, PreviousMode = 0;
    inline HWND Window = nullptr;
    inline WNDPROC OriginalProc = nullptr;
    inline void* Renderer = nullptr;
    inline bool Created = false, Initializing = false, Applying = false, Changing = false;
    inline HMONITOR TargetMonitor = nullptr;
    inline RECT FramedRect{};
    inline SIZE FramedSize{}, SavedResolution{};
    inline bool HaveFramedRect = false, RestorePlacement = false;
    inline std::map<HMONITOR, SIZE> Desktops;
    inline std::function<void(int)> OnModeChanged;
    inline int(WINAPI* OriginalMetrics)(int) = nullptr;
    inline decltype(OriginalMetrics)* MetricsSlot = nullptr;
    inline uint8_t* ClampBranch = nullptr;
    inline uint8_t OriginalClamp = 0;
    inline HMODULE DisplayModule = nullptr;

    inline MONITORINFOEXW Monitor(HMONITOR monitor)
    {
        MONITORINFOEXW info{};
        info.cbSize = sizeof(info);
        GetMonitorInfoW(monitor, &info);
        return info;
    }

    inline HMONITOR CurrentMonitor()
    {
        return TargetMonitor ? TargetMonitor : MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);
    }

    inline SIZE Desktop(HMONITOR monitor)
    {
        const auto info = Monitor(monitor);
        SIZE result{ info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
        if (Mode.load() == 0 || (Transition >= 0 && PreviousMode == 0))
        {
            const auto saved = Desktops.find(monitor);
            if (saved != Desktops.end())
                return saved->second;
            DEVMODEW mode{ .dmSize = sizeof(DEVMODEW) };
            if (EnumDisplaySettingsW(info.szDevice, ENUM_REGISTRY_SETTINGS, &mode))
                result = { static_cast<LONG>(mode.dmPelsWidth), static_cast<LONG>(mode.dmPelsHeight) };
        }
        result.cx = std::max(result.cx, 640L);
        result.cy = std::max(result.cy, 480L);
        return result;
    }

    inline int WINAPI Metrics(int index)
    {
        if (index == SM_CXFULLSCREEN || index == SM_CYFULLSCREEN)
        {
            const auto desktop = Desktop(CurrentMonitor());
            return index == SM_CXFULLSCREEN ? desktop.cx : desktop.cy;
        }
        if (Mode.load() != 1 && (index == SM_CXDLGFRAME || index == SM_CYCAPTION))
            return 0;
        return OriginalMetrics ? OriginalMetrics(index) : GetSystemMetrics(index);
    }

    inline void Publish(int mode)
    {
        Mode.store(mode);
        if (mode)
            LastWindowMode = mode;
        if (OnModeChanged)
            OnModeChanged(mode);
    }

    inline void Remember()
    {
        if (Mode.load() == 1 && Window && GetWindowRect(Window, &FramedRect))
        {
            RECT client{};
            if (GetClientRect(Window, &client))
            {
                FramedSize = { client.right, client.bottom };
                HaveFramedRect = true;
            }
        }
    }

    inline bool SetVariable(void* renderer, size_t offset, int value)
    {
        const auto variable = Field<void*>(renderer, offset);
        if (!variable)
            return false;
#ifdef _WIN64
        using Set = void(__fastcall*)(void*, int);
#else
        using Set = void(__thiscall*)(void*, int);
#endif
        reinterpret_cast<Set>((*static_cast<uintptr_t**>(variable))[4])(variable, value);
        return true;
    }

    inline void SquareCorners(HWND window)
    {
        using Set = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static const auto set = []() -> Set
        {
            const auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<Set>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const int preference = 1;
        if (set)
            set(window, 33, &preference, sizeof(preference));
    }

    inline void ApplyWindow(void* renderer)
    {
        if (!Window || !IsWindow(Window) || IsIconic(Window))
            return;
        SquareCorners(Window);
        const auto mode = Mode.load();
        if (!mode)
            return;
        const auto monitor = CurrentMonitor();
        const auto info = Monitor(monitor);
        const auto width = Field<int>(renderer, Fields.width);
        const auto height = Field<int>(renderer, Fields.height);
        if (width <= 0 || height <= 0)
            return;
        const auto previousStyle = GetWindowLongW(Window, GWL_STYLE);
        const DWORD style = (previousStyle & (WS_VISIBLE | WS_DISABLED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)) |
            (mode == 1 ? WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX : WS_POPUP);
        RECT rect{ 0, 0, width, height };
        WindowedModeWrapper::GameHWND = Window;
        WindowedModeWrapper::AdjustFrame(&rect, style, FALSE, GetWindowLongW(Window, GWL_EXSTYLE));
        auto x = info.rcMonitor.left + (info.rcMonitor.right - info.rcMonitor.left - width) / 2 + rect.left;
        auto y = info.rcMonitor.top + (info.rcMonitor.bottom - info.rcMonitor.top - height) / 2 + rect.top;
        if (mode == 1 && HaveFramedRect && FramedSize.cx == width && FramedSize.cy == height)
        {
            x = FramedRect.left;
            y = FramedRect.top;
        }
        if (mode == 2)
        {
            x = info.rcMonitor.left;
            y = info.rcMonitor.top;
        }
        Applying = true;
        SetWindowLongW(Window, GWL_STYLE, style);
        ProtectedGameRef<LONG>{ &Field<LONG>(renderer, Fields.savedStyle) } = static_cast<LONG>(style);
        SetWindowPos(Window, HWND_NOTOPMOST, x, y, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
        RECT windowRect{}, clientRect{};
        GetWindowRect(Window, &windowRect);
        GetClientRect(Window, &clientRect);
        ProtectedGameRef<RECT>{ &Field<RECT>(renderer, Fields.windowRect) } = windowRect;
        ProtectedGameRef<RECT>{ &Field<RECT>(renderer, Fields.clientRect) } = clientRect;
        Applying = false;
        const auto size = SIZE{ info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
        Desktops[monitor] = size;
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        CursorConfinement::Message(message, wParam);
        if ((message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && wParam == VK_RETURN)
        {
            if (message == WM_SYSKEYDOWN && (lParam & (1u << 29)) && !(lParam & (1u << 30)))
                Pending.store(3);
            return 0;
        }
        if ((Applying || Changing) && (message == WM_SIZE || message == WM_ACTIVATE || message == WM_ACTIVATEAPP ||
            message == WM_KILLFOCUS || message == WM_SETFOCUS))
            return DefWindowProcW(window, message, wParam, lParam);
        if (!Applying && !Changing && (message == WM_DISPLAYCHANGE || message == DpiChangedMessage))
        {
            if (Mode.load() == 2)
                Pending.store(2);
            else if (Mode.load() == 0)
                Desktops.erase(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY));
        }
        const auto original = OriginalProc;
        if (message == WM_NCDESTROY)
        {
            Window = nullptr;
            OriginalProc = nullptr;
            if (!Initializing)
            {
                Renderer = nullptr;
                Created = false;
            }
            Pending.store(-1);
            Transition = -1;
        }
        return original ? CallWindowProcW(original, window, message, wParam, lParam) : DefWindowProcW(window, message, wParam, lParam);
    }

    inline void BeforeCreate(void* renderer, int& width, int& height, char& fullscreen, HWND parent)
    {
        if (parent)
            return;
        if (Renderer != renderer)
        {
            Renderer = renderer;
            Created = false;
            Initializing = true;
            HaveFramedRect = false;
            RestorePlacement = false;
        }
        if (!Created)
        {
            const auto monitor = MonitorFromWindow(nullptr, MONITOR_DEFAULTTOPRIMARY);
            const auto info = Monitor(monitor);
            Desktops[monitor] = { info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
            const auto desktop = Desktop(monitor);
            if (width < 640 || height < 480 || width > 16384 || height > 16384)
            {
                width = desktop.cx;
                height = desktop.cy;
            }
            SavedResolution = { width, height };
            if (Mode.load() == 2)
            {
                width = desktop.cx;
                height = desktop.cy;
            }
            fullscreen = Mode.load() == 0;
            ProtectedGameRef<int>{ &Field<int>(renderer, Fields.width) } = width;
            ProtectedGameRef<int>{ &Field<int>(renderer, Fields.height) } = height;
            ProtectedGameRef<uint8_t>{ &Field<uint8_t>(renderer, Fields.fullscreen) } = fullscreen;
        }
        else
        {
            // The native initializer retries the other display mode after a failed device creation.
            fullscreen = Field<uint8_t>(renderer, Fields.fullscreen);
            Publish(fullscreen ? 0 : LastWindowMode);
        }
        SetVariable(renderer, Fields.cvarWidth, width);
        SetVariable(renderer, Fields.cvarHeight, height);
        SetVariable(renderer, Fields.cvarFullscreen, fullscreen);
    }

    inline void AfterCreate(void* renderer, HWND parent)
    {
        if (parent)
            return;
        Created = true;
        Window = Field<HWND>(renderer, Fields.window);
        if (!Window || !IsWindow(Window))
            return;
        if (reinterpret_cast<WNDPROC>(GetWindowLongPtrW(Window, GWLP_WNDPROC)) != WindowProc)
            OriginalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(Window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
        ApplyWindow(renderer);
    }

    template<class Call>
    inline char Change(void* renderer, int width, int height, int bits, int refresh, char fullscreen, Call call)
    {
        if (Changing || renderer != Renderer)
            return call(width, height, bits, refresh, fullscreen);
        const auto previous = Transition >= 0 ? PreviousMode : Mode.load();
        const auto target = Transition >= 0 ? Transition : (fullscreen ? 0 : (Mode.load() ? Mode.load() : LastWindowMode));
        if (Transition < 0 && !HaveFramedRect)
            Remember();
        TargetMonitor = MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);
        const auto desktop = Desktop(TargetMonitor);
        if (width < 640 || height < 480 || width > 16384 || height > 16384)
        {
            width = desktop.cx;
            height = desktop.cy;
        }
        if (target == 2)
        {
            width = desktop.cx;
            height = desktop.cy;
        }
        Mode.store(target);
        Changing = true;
        const auto result = call(width, height, bits, refresh, fullscreen);
        Changing = false;
        auto actual = result ? target : previous;
        if (Field<uint8_t>(renderer, Fields.fullscreen))
            actual = 0;
        else if (!actual)
            actual = LastWindowMode;
        Publish(actual);
        ApplyWindow(renderer);
        // Native context changes can apply their cached placement after this call returns.
        RestorePlacement = actual == 1 && HaveFramedRect;
        TargetMonitor = nullptr;
        Transition = -1;
        return result;
    }

    inline void ObserveFrame(void* renderer);

    inline void BeforeFrame(void* renderer)
    {
        ObserveFrame(renderer);
        CursorConfinement::Update(Window);
        CursorConfinement::Bind();
        if (renderer != Renderer || Transition >= 0 || Pending.load() < 0 || !Window || GetForegroundWindow() != Window)
            return;
        const auto device = Field<IDirect3DDevice9*>(renderer, Fields.device);
        if (!device || device->TestCooperativeLevel() != D3D_OK)
            return;
        const auto request = Pending.exchange(-1);
        const auto previous = Mode.load();
        const auto target = request == 3 ? (previous ? 0 : LastWindowMode) : std::clamp(request, 0, 2);
        Remember();
        TargetMonitor = MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY);
        auto width = Field<int>(renderer, Fields.width);
        auto height = Field<int>(renderer, Fields.height);
        if (previous != 2)
            SavedResolution = { width, height };
        if (target == 2)
        {
            const auto desktop = Desktop(TargetMonitor);
            width = desktop.cx;
            height = desktop.cy;
        }
        else if (previous == 2 && SavedResolution.cx > 0 && SavedResolution.cy > 0)
        {
            width = SavedResolution.cx;
            height = SavedResolution.cy;
        }
        if (!Field<void*>(renderer, Fields.cvarWidth) || !Field<void*>(renderer, Fields.cvarHeight) || !Field<void*>(renderer, Fields.cvarFullscreen))
        {
            TargetMonitor = nullptr;
            OutputDebugStringA("Far Cry: native display variables unavailable.\n");
            Publish(previous);
            return;
        }
        PreviousMode = previous;
        Transition = target;
        Mode.store(target);
        SetVariable(renderer, Fields.cvarWidth, width);
        SetVariable(renderer, Fields.cvarHeight, height);
        SetVariable(renderer, Fields.cvarFullscreen, target == 0);
        if (width == Field<int>(renderer, Fields.width) && height == Field<int>(renderer, Fields.height) &&
            (target == 0) == bool(Field<uint8_t>(renderer, Fields.fullscreen)))
        {
            Publish(target);
            ApplyWindow(renderer);
            TargetMonitor = nullptr;
            Transition = -1;
        }
    }

    inline void ObserveFrame(void* renderer)
    {
        if (renderer != Renderer || !Window || !IsWindow(Window))
            return;
        Initializing = false;
        if (RestorePlacement && Mode.load() == 1 && !IsIconic(Window))
        {
            ApplyWindow(renderer);
            RestorePlacement = false;
        }
        else if (Mode.load() == 1 && !Field<uint8_t>(renderer, Fields.fullscreen) && Transition < 0)
            Remember();
        if (Mode.load() && !IsIconic(Window))
        {
            RECT client{};
            const auto caption = GetWindowLongW(Window, GWL_STYLE) & WS_CAPTION;
            if (GetClientRect(Window, &client) && (client.right != Field<int>(renderer, Fields.width) ||
                client.bottom != Field<int>(renderer, Fields.height) || (Mode.load() == 1 ? caption != WS_CAPTION : caption != 0)))
                ApplyWindow(renderer);
        }
    }

#ifndef _WIN64
    inline char __fastcall Create(void* renderer, void*, int width, int height, char fullscreen, HWND parent)
    {
        BeforeCreate(renderer, width, height, fullscreen, parent);
        const auto result = CreateHook.unsafe_fastcall<char>(renderer, nullptr, width, height, fullscreen, parent);
        AfterCreate(renderer, parent);
        return result;
    }
    inline char __fastcall ChangeResolution(void* renderer, void*, int width, int height, int bits, int refresh, char fullscreen)
    {
        return Change(renderer, width, height, bits, refresh, fullscreen, [&](int w, int h, int b, int r, char f)
        {
            return ChangeHook.unsafe_fastcall<char>(renderer, nullptr, w, h, b, r, f);
        });
    }
#else
    inline char __fastcall Create(void* renderer, int width, int height, char fullscreen, HWND parent)
    {
        BeforeCreate(renderer, width, height, fullscreen, parent);
        const auto result = CreateHook.unsafe_fastcall<char>(renderer, width, height, fullscreen, parent);
        AfterCreate(renderer, parent);
        return result;
    }
    inline char __fastcall ChangeResolution(void* renderer, int width, int height, int bits, int refresh, char fullscreen)
    {
        return Change(renderer, width, height, bits, refresh, fullscreen, [&](int w, int h, int b, int r, char f)
        {
            return ChangeHook.unsafe_fastcall<char>(renderer, w, h, b, r, f);
        });
    }
#endif

    inline void BeginFrame(SafetyHookContext& regs)
    {
#ifndef _WIN64
        BeforeFrame(reinterpret_cast<void*>(regs.ecx));
#else
        BeforeFrame(reinterpret_cast<void*>(regs.rcx));
#endif
    }

    inline uintptr_t FrameMethod(HMODULE module, uintptr_t change)
    {
        constexpr char digits[] = "0123456789ABCDEF";
        std::string signature;
        for (size_t index = 0; index < sizeof(change); ++index)
        {
            const auto byte = static_cast<uint8_t>(change >> (index * 8));
            signature += digits[byte >> 4];
            signature += digits[byte & 15];
            signature += ' ';
        }
        const auto base = reinterpret_cast<uintptr_t>(module);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const auto end = base + nt->OptionalHeader.SizeOfImage;
        const auto executable = [&](uintptr_t address)
        {
            MEMORY_BASIC_INFORMATION info{};
            return address >= base && address < end && VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) &&
                (info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));
        };
        uintptr_t result = 0;
        const auto sections = IMAGE_FIRST_SECTION(nt);
        for (WORD sectionIndex = 0; sectionIndex < nt->FileHeader.NumberOfSections; ++sectionIndex)
        {
            const auto& section = sections[sectionIndex];
            if (!(section.Characteristics & IMAGE_SCN_MEM_READ) ||
                (section.Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_DISCARDABLE)))
                continue;
            const auto begin = base + section.VirtualAddress;
            const auto limit = std::min(end, begin + section.Misc.VirtualSize);
            auto matches = hook::range_pattern(begin, limit, signature);
            for (size_t index = 0; index < matches.size(); ++index)
            {
                const auto slot = reinterpret_cast<uintptr_t>(matches.get(index).get<void>());
                if (slot % sizeof(uintptr_t) || slot < begin + 8 * sizeof(uintptr_t) || slot + 8 * sizeof(uintptr_t) > limit)
                    continue;
                const auto table = reinterpret_cast<uintptr_t*>(slot - 8 * sizeof(uintptr_t));
                if (!executable(table[0]) || !executable(table[14]) || !executable(table[15]))
                    continue;
                if (result && result != table[14])
                    return 0;
                result = table[14];
            }
        }
        return result;
    }

    inline void ResetHooks()
    {
        CursorConfinement::Shutdown();
        if (Window && IsWindow(Window) && OriginalProc &&
            reinterpret_cast<WNDPROC>(GetWindowLongPtrW(Window, GWLP_WNDPROC)) == WindowProc)
            SetWindowLongPtrW(Window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OriginalProc));
        OriginalProc = nullptr;
        CreateHook.reset();
        ChangeHook.reset();
        FrameHook.reset();
        // The game unloads its renderer before process-detach cleanup. Its IAT
        // and code slots can already be unmapped; never read those stale slots.
        auto mapped = [](const void* address)
        {
            HMODULE module = nullptr;
            return address && DisplayModule && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(address), &module) && module == DisplayModule;
        };
        if (mapped(MetricsSlot) && *MetricsSlot == Metrics)
            ProtectedGameRef<decltype(OriginalMetrics)>{ MetricsSlot } = OriginalMetrics;
        if (mapped(ClampBranch) && *ClampBranch == 0xEB)
            ProtectedGameRef<uint8_t>{ ClampBranch } = OriginalClamp;
        MetricsSlot = nullptr;
        ClampBranch = nullptr;
        DisplayModule = nullptr;
    }

    inline bool Install(HMODULE module)
    {
        if (CreateHook && ChangeHook && FrameHook)
            return true;
        DisplayModule = module;
#ifndef _WIN64
        auto create = hook::module_pattern(module, "83 EC 1C 53 55 56 57 8B D9 FF 15 ? ? ? ? 8D 4C 24 1C 51 50");
        auto change = hook::module_pattern(module, "8A 44 24 14 83 EC 14 84 C0 53 55 56 8B F1 57 B9 ? ? ? ? 75 05 B9");
        auto metrics = hook::module_pattern(module, "8B 35 ? ? ? ? 6A 10");
        auto clamp = hook::module_pattern(module, "8B 86 ? ? ? ? 83 C0 F0 3B F8 7E 02 8B F8");
#else
        auto create = hook::module_pattern(module, "48 8B C4 48 81 EC B8 00 00 00 48 89 58 F8 48 89 70 E8 48 89 78 E0 4C 89 60 D8 41 0F B6 F1");
        auto change = hook::module_pattern(module, "48 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 58 40 8A AC 24 C8 00 00 00 8B FA 4C 8B E1");
        auto metrics = hook::module_pattern(module, "FF 15 ? ? ? ? B9 ? ? ? ? 2B C3 99 2B C2 D1 F8 44 8B F0");
        auto clamp = hook::module_pattern(module, "41 8B 84 24 ? ? ? ? 83 C0 F0 3B F8 0F 4F F8");
#endif
        if (create.size() != 1 || change.size() != 1 || metrics.size() != 1 || clamp.size() != 1 || *clamp.get_first<uint8_t>(-2) != 0x75)
        {
            OutputDebugStringA("Far Cry: native window-mode patterns unavailable.\n");
            return false;
        }
        const auto frame = FrameMethod(module, reinterpret_cast<uintptr_t>(change.get_first()));
        if (!frame)
        {
            OutputDebugStringA("Far Cry: native display frame method unavailable.\n");
            return false;
        }
        CreateHook = safetyhook::create_inline(create.get_first(), Create);
        ChangeHook = safetyhook::create_inline(change.get_first(), ChangeResolution);
        FrameHook = safetyhook::create_mid(frame, BeginFrame);
        if (!CreateHook || !ChangeHook || !FrameHook)
        {
            ResetHooks();
            OutputDebugStringA("Far Cry: native window-mode hooks unavailable.\n");
            return false;
        }
#ifndef _WIN64
        MetricsSlot = *metrics.get_first<decltype(OriginalMetrics)*>(2);
#else
        const auto call = metrics.get_first<uint8_t>();
        MetricsSlot = reinterpret_cast<decltype(MetricsSlot)>(call + 6 + *reinterpret_cast<int32_t*>(call + 2));
#endif
        OriginalMetrics = *MetricsSlot;
        ProtectedGameRef<decltype(OriginalMetrics)>{ MetricsSlot } = Metrics;
        ClampBranch = clamp.get_first<uint8_t>(-2);
        OriginalClamp = *ClampBranch;
        ProtectedGameRef<uint8_t>{ ClampBranch } = 0xEB;
        return true;
    }
}
