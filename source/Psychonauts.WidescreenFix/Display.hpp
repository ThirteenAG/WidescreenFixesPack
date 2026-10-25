#pragma once
#include <d3d9.h>
#include <commctrl.h>
#include <atomic>
#pragma comment(lib, "comctl32.lib")

namespace PsychonautsDisplay
{
    inline SafetyHookInline createHook, parametersHook, placeHook, resetHook;
    inline void* renderer = nullptr;
    inline void** engine = nullptr;
    inline char(__thiscall* validateMode)(void*) = nullptr;
    inline int(__thiscall* refresh)(void*) = nullptr;
    inline int(__thiscall* refreshScreen)(void*) = nullptr;
    inline std::atomic<HWND> window{ nullptr };
    inline std::atomic_bool changing{ false };
    inline int mode = 0, lastWindowMode = 1;
    inline int normalWidth = 640, normalHeight = 480;
    inline constexpr UINT ChangeMessage = WM_APP + 0x507;

    template<class T> T& Field(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
    }

    inline void MonitorSize(void* object)
    {
        const auto hwnd = window.load(std::memory_order_acquire);
        const auto monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info{ sizeof(info) };
        if (GetMonitorInfoW(monitor, &info))
        {
            Field<int>(object, 708) = info.rcMonitor.right - info.rcMonitor.left;
            Field<int>(object, 712) = info.rcMonitor.bottom - info.rcMonitor.top;
        }
    }

    inline int __fastcall Parameters(void* object, void*, char flags)
    {
        if (mode == 2)
            MonitorSize(object);
        const auto result = parametersHook.thiscall<int>(object, flags);
        auto data = Field<uint8_t*>(object, 36);
        auto parameters = reinterpret_cast<D3DPRESENT_PARAMETERS*>(data + 328);
        if (parameters->Windowed)
        {
            parameters->BackBufferFormat = D3DFMT_UNKNOWN;
            parameters->FullScreen_RefreshRateInHz = 0;
            auto d3d = *reinterpret_cast<IDirect3D9**>(data);
            D3DDISPLAYMODE desktop{};
            DWORD colour = 0, depth = 0;
            const auto sample = parameters->MultiSampleType;
            if (sample != D3DMULTISAMPLE_NONE && d3d
                && SUCCEEDED(d3d->GetAdapterDisplayMode(0, &desktop))
                && SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, desktop.Format, TRUE, sample, &colour))
                && SUCCEEDED(d3d->CheckDeviceMultiSampleType(0, D3DDEVTYPE_HAL, parameters->AutoDepthStencilFormat, TRUE, sample, &depth))
                && colour && depth)
                parameters->MultiSampleQuality = std::min(parameters->MultiSampleQuality, std::min(colour, depth) - 1);
            else
            {
                parameters->MultiSampleType = D3DMULTISAMPLE_NONE;
                parameters->MultiSampleQuality = 0;
            }
        }
        return result;
    }

    inline void Change(int wanted);

    inline LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam,
        LPARAM lParam, UINT_PTR id, DWORD_PTR)
    {
        if (message == ChangeMessage)
        {
            if (!changing && wParam <= 2)
                Change(static_cast<int>(wParam));
            return 0;
        }
        if (message == WM_SYSKEYDOWN && wParam == VK_RETURN)
        {
            if (!(lParam & (1u << 30)))
                PostMessageW(hwnd, ChangeMessage, mode == 0 ? lastWindowMode : 0, 0);
            return 0;
        }
        if ((message == WM_SYSKEYUP || message == WM_SYSCHAR) && wParam == VK_RETURN)
            return 0;
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(hwnd, WindowProc, id);
            window.store(nullptr, std::memory_order_release);
        }
        return DefSubclassProc(hwnd, message, wParam, lParam);
    }

    inline BOOL __fastcall Place(void* object, void*, char)
    {
        const auto hwnd = Field<HWND>(object, 5700);
        if (!IsWindow(hwnd)) return FALSE;
        if (window.exchange(hwnd, std::memory_order_acq_rel) != hwnd)
            SetWindowSubclass(hwnd, WindowProc, 0x507, 0);
        MONITORINFO monitor{ sizeof(monitor) };
        if (!GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor)) return FALSE;
        const auto bounds = monitor.rcMonitor;
        const int width = Field<int>(object, 708), height = Field<int>(object, 712);
        if (width <= 0 || height <= 0) return FALSE;
        DWORD style = static_cast<DWORD>(GetWindowLongW(hwnd, GWL_STYLE));
        style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        style |= mode == 1 ? WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX : WS_POPUP;
        SetWindowLongW(hwnd, GWL_STYLE, style);
        const auto exStyle = static_cast<DWORD>(GetWindowLongW(hwnd, GWL_EXSTYLE)) & ~WS_EX_TOPMOST;
        SetWindowLongW(hwnd, GWL_EXSTYLE, exStyle);
        RECT frame{ 0, 0, width, height };
        WindowedModeWrapper::GameHWND = hwnd;
        if (mode == 1)
            WindowedModeWrapper::AdjustFrame(&frame, style, FALSE, exStyle);
        const int left = mode == 1 ? bounds.left + (bounds.right - bounds.left - width) / 2 : bounds.left;
        const int top = mode == 1 ? bounds.top + (bounds.bottom - bounds.top - height) / 2 : bounds.top;
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static const auto setAttribute = []() -> SetAttribute
        {
            const auto dwm = LoadLibraryW(L"dwmapi.dll");
            return dwm ? reinterpret_cast<SetAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
        }();
        if (setAttribute)
        {
            const DWORD square = 1;
            setAttribute(hwnd, 33, &square, sizeof(square));
        }
        return SetWindowPos(hwnd, nullptr, left + frame.left, top + frame.top,
            frame.right - frame.left, frame.bottom - frame.top,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    inline char __fastcall Create(void* object, void*, HWND hwnd, int flags)
    {
        renderer = object;
        window.store(hwnd, std::memory_order_release);
        normalWidth = Field<int>(object, 708);
        normalHeight = Field<int>(object, 712);
        if (normalWidth <= 0 || normalHeight <= 0 || normalWidth > 16384 || normalHeight > 16384)
        {
            std::tie(normalWidth, normalHeight) = GetDesktopRes();
            if (normalWidth <= 0 || normalHeight <= 0 || normalWidth > 16384 || normalHeight > 16384)
            {
                normalWidth = 640;
                normalHeight = 480;
            }
            Field<int>(object, 708) = normalWidth;
            Field<int>(object, 712) = normalHeight;
        }
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        if (mode) lastWindowMode = mode;
        Field<uint8_t>(object, 720) = mode == 0;
        if (mode == 2) MonitorSize(object);
        const auto result = createHook.thiscall<char>(object, hwnd, flags);
        if (result)
        {
            if (mode != 2)
            {
                normalWidth = Field<int>(object, 708);
                normalHeight = Field<int>(object, 712);
            }
            SetWindowSubclass(hwnd, WindowProc, 0x507, 0);
            Place(object, nullptr, 0);
        }
        return result;
    }

    inline int __fastcall Reset(void* object, void*, int flags)
    {
        if (!changing)
        {
            // Native Display options remain authoritative when used directly.
            if (Field<uint8_t>(object, 720)) mode = 0;
            else if (mode == 0) mode = lastWindowMode;
            if (mode != 2)
            {
                normalWidth = Field<int>(object, 708);
                normalHeight = Field<int>(object, 712);
            }
        }
        const auto result = resetHook.thiscall<int>(object, flags);
        if (SUCCEEDED(result) && !changing)
        {
            if (mode) lastWindowMode = mode;
            changing = true;
            FixSettings().Set("MAIN.WindowMode", std::to_string(mode));
            changing = false;
        }
        return result;
    }

    inline void Change(int wanted)
    {
        if (!renderer || wanted == mode || changing.exchange(true)) return;
        const auto previous = mode;
        const auto width = Field<int>(renderer, 708), height = Field<int>(renderer, 712);
        const auto fullscreen = Field<uint8_t>(renderer, 720);
        const auto refreshRate = Field<int>(renderer, 716);
        mode = wanted;
        Field<uint8_t>(renderer, 720) = mode == 0;
        Field<int>(renderer, 708) = normalWidth;
        Field<int>(renderer, 712) = normalHeight;
        if (mode == 2) MonitorSize(renderer);
        if (mode == 0) validateMode(renderer);
        const auto flags = Field<int>(renderer, 1436);
        auto result = resetHook.thiscall<int>(renderer, flags);
        if (FAILED(result))
        {
            mode = previous;
            Field<int>(renderer, 708) = width;
            Field<int>(renderer, 712) = height;
            Field<uint8_t>(renderer, 720) = fullscreen;
            Field<int>(renderer, 716) = refreshRate;
            result = resetHook.thiscall<int>(renderer, flags);
            OutputDebugStringA(FAILED(result)
                ? "Psychonauts Widescreen Fix: window change and native display restore refused.\n"
                : "Psychonauts Widescreen Fix: window change refused; native display restored.\n");
        }
        if (SUCCEEDED(result) && engine && *engine && Field<void*>(*engine, 404) == renderer)
        {
            refresh(*engine);
            refreshScreen(*engine);
        }
        if (mode) lastWindowMode = mode;
        FixSettings().Set("MAIN.WindowMode", std::to_string(mode));
        changing = false;
    }

    inline void Init()
    {
        auto create = hook::pattern("55 8B EC 83 EC 2C 89 4D D4 83 7D 08 00 75 09 FF 15 ? ? ? ? 89 45 08");
        auto parameters = hook::pattern("55 8B EC 51 89 4D FC 6A 38 6A 00 8B 45 FC 8B 48 24 81 C1 48 01 00 00 51 E8");
        auto place = hook::pattern("55 8B EC 83 EC 58 89 4D A8 8B 45 A8 0F B6 88 D0 02 00 00 85 C9 0F 85 ? ? ? ?");
        auto reset = hook::pattern("55 8B EC 51 89 4D FC 8B 45 08 50 8B 4D FC E8 ? ? ? ? 8B 4D FC 8B 55 08 89 91 9C 05 00 00");
        auto validate = hook::pattern("55 8B EC 83 EC 3C 89 4D C4 8B 45 C4 0F B6 88 D0 02 00 00 85 C9 0F 84 ? ? ? ? C7 45 E8 16 00 00 00");
        auto display = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 8B 4D 8C E8");
        if (create.size() != 1 || parameters.size() != 1 || place.size() != 1 || reset.size() != 1 || display.size() != 1 || validate.size() != 1)
        {
            OutputDebugStringA("Psychonauts Widescreen Fix: native display contracts unavailable.\n");
            return;
        }
        validateMode = reinterpret_cast<decltype(validateMode)>(validate.get_first());
        const auto applied = display.get_first<uint8_t>();
        engine = *reinterpret_cast<void***>(applied + 2);
        refresh = reinterpret_cast<decltype(refresh)>(applied + 11 + *reinterpret_cast<int32_t*>(applied + 7));
        refreshScreen = reinterpret_cast<decltype(refreshScreen)>(applied + 22 + *reinterpret_cast<int32_t*>(applied + 18));
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &pinned)) return;
        createHook = safetyhook::create_inline(create.get_first(), Create, SafetyHookInline::StartDisabled);
        parametersHook = safetyhook::create_inline(parameters.get_first(), Parameters, SafetyHookInline::StartDisabled);
        placeHook = safetyhook::create_inline(place.get_first(), Place, SafetyHookInline::StartDisabled);
        resetHook = safetyhook::create_inline(reset.get_first(), Reset, SafetyHookInline::StartDisabled);
        if (!createHook || !parametersHook || !placeHook || !resetHook
            || !createHook.enable() || !parametersHook.enable() || !placeHook.enable() || !resetHook.enable())
        {
            createHook.reset(); parametersHook.reset(); placeHook.reset(); resetHook.reset();
            return;
        }
        FixSettings().OnChange("MAIN.WindowMode", []()
        {
            if (const auto hwnd = window.load(std::memory_order_acquire); hwnd && !changing)
                PostMessageW(hwnd, ChangeMessage, std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2), 0);
        });
    }
}
