module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module Windowed;

import ComVars;

// The game's device creation tries hardware and then software vertex processing once and goes on without a device
// when both fail, which crashed it. Creation fails while another program changes the display mode, and a full
// screen device made while the window isn't in front starts out lost: the game crashed on a missing texture when
// the window came back. The full screen device waits until the window is in front, and a failed creation is tried
// again.
namespace DeviceCreation
{
    // Handles the window's messages for a while; a window closed meanwhile ends the game
    void PumpMessages(DWORD ms)
    {
        MsgWaitForMultipleObjects(0, nullptr, FALSE, ms, QS_ALLINPUT);
        MSG msg{};
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                ExitProcess(0);
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    void WaitForForeground(const D3DPRESENT_PARAMETERS* pp, HWND window)
    {
        if (pp->Windowed || !window)
            return;
        while (GetForegroundWindow() != window || IsIconic(window))
            PumpMessages(100);
    }

    void Install()
    {
        // After the present parameters are filled in: esi = the renderer, the parameters at esi + disp
        auto before = hook::pattern("E8 ? ? ? ? 8B 4C 24 18 8B 54 24 1C 8B 45 00 68 ? ? ? ? 8D BE ? ? ? ?");
        static auto ppOffset = *before.get_first<int32_t>(23);
        static auto BeforeCreateHook = safetyhook::create_mid(before.get_first(5), [](SafetyHookContext& regs)
        {
            auto pp = reinterpret_cast<const D3DPRESENT_PARAMETERS*>(regs.esi + ppOffset);
            WaitForForeground(pp, *reinterpret_cast<HWND*>(regs.esp + 0x18));
        });

        // After both attempts: ebx = the result, ebp = IDirect3D9, edi = the present parameters; the first attempt's
        // adapter, device type and focus window at esp+0x10, +0x1C, +0x18, the second's adapter at esp+0x478
        auto after = hook::pattern("81 FB 6C 08 76 88 7E ? 85 DB 75 ? A1");
        static auto ppDevice = *after.get_first<IDirect3DDevice9**>(13);
        static auto AfterCreateHook = safetyhook::create_mid(after.get_first(), [](SafetyHookContext& regs)
        {
            auto d3d = reinterpret_cast<IDirect3D9*>(regs.ebp);
            auto pp = reinterpret_cast<D3DPRESENT_PARAMETERS*>(regs.edi);
            auto adapter = *reinterpret_cast<UINT*>(regs.esp + 0x10);
            auto deviceType = *reinterpret_cast<D3DDEVTYPE*>(regs.esp + 0x1C);
            auto window = *reinterpret_cast<HWND*>(regs.esp + 0x18);
            auto fallbackAdapter = *reinterpret_cast<UINT*>(regs.esp + 0x478);

            // A lost device waits for its display; other failures get a few seconds, then the game's own fallback
            // (it tries once more without antialiasing) takes over
            HRESULT hr = static_cast<HRESULT>(regs.ebx);
            auto giveUp = GetTickCount64() + 3000;
            while (FAILED(hr) && (hr == D3DERR_DEVICELOST || GetTickCount64() < giveUp))
            {
                PumpMessages(250);
                WaitForForeground(pp, window);
                hr = d3d->CreateDevice(adapter, deviceType, window, D3DCREATE_HARDWARE_VERTEXPROCESSING, pp, ppDevice);
                if (FAILED(hr))
                    hr = d3d->CreateDevice(fallbackAdapter, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, pp, ppDevice);
            }
            regs.ebx = static_cast<uintptr_t>(hr);
        });
    }
}

class Windowed
{
public:
    Windowed()
    {
        WFP::onInitEvent() += []()
        {
            DeviceCreation::Install();

            // 0 full screen, 1 windowed, 2 borderless at the monitor's size, 3 resizable window, 4 borderless
            const auto mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 4);

            // Per monitor DPI awareness, so the game gets real pixels on scaled desktops
            WindowedModeWrapper::EnableDpiAwareness();

            if (mode != 0)
            {
                WindowedModeWrapper::bBorderlessWindowed = mode == 2 || mode == 4;
                WindowedModeWrapper::bScaleWindow = mode == 2;
                WindowedModeWrapper::bEnableWindowResize = mode == 3;
                bAspectFromWindow = mode == 3;

                dwWindowedMode = 1;

                auto hooks = IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                    std::forward_as_tuple("CreateWindowExA", WindowedModeWrapper::CreateWindowExA_Hook),
                    std::forward_as_tuple("CreateWindowExW", WindowedModeWrapper::CreateWindowExW_Hook),
                    //std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
                    //std::forward_as_tuple("SetWindowLongW", WindowedModeWrapper::SetWindowLongW_Hook),
                    std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
                    std::forward_as_tuple("AdjustWindowRectEx", WindowedModeWrapper::AdjustWindowRectEx_Hook),
                    std::forward_as_tuple("SetWindowPos", WindowedModeWrapper::SetWindowPos_Hook),
                    std::forward_as_tuple("ShowCursor", WindowedModeWrapper::ShowCursor_Hook)
                );

                if (hooks.empty())
                {
                    static auto IsCallerFromModule = [](HMODULE hModule) -> bool
                    {
                        for (const auto& entry : std::stacktrace::current(1, 4))
                        {
                            HMODULE hCaller = NULL;
                            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                (LPCSTR)entry.native_handle(), &hCaller) && hCaller == hModule)
                                return true;
                        }
                        return false;
                    };

                    static SafetyHookInline shCreateWindowExA = {};
                    shCreateWindowExA = safetyhook::create_inline(CreateWindowExA, static_cast<decltype(&CreateWindowExA)>([](DWORD dw, LPCSTR cls, LPCSTR name, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param) -> HWND
                    {
                        if (IsCallerFromModule(GetModuleHandleA(NULL)))
                            return WindowedModeWrapper::CreateWindowExA_Hook(dw, cls, name, style, x, y, w, h, parent, menu, inst, param);
                        return shCreateWindowExA.stdcall<HWND>(dw, cls, name, style, x, y, w, h, parent, menu, inst, param);
                    }));

                    static SafetyHookInline shCreateWindowExW = {};
                    shCreateWindowExW = safetyhook::create_inline(CreateWindowExW, static_cast<decltype(&CreateWindowExW)>([](DWORD dw, LPCWSTR cls, LPCWSTR name, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param) -> HWND
                    {
                        if (IsCallerFromModule(GetModuleHandleA(NULL)))
                            return WindowedModeWrapper::CreateWindowExW_Hook(dw, cls, name, style, x, y, w, h, parent, menu, inst, param);
                        return shCreateWindowExW.stdcall<HWND>(dw, cls, name, style, x, y, w, h, parent, menu, inst, param);
                    }));

                    static SafetyHookInline shSetWindowLongA = {};
                    shSetWindowLongA = safetyhook::create_inline(SetWindowLongA, static_cast<decltype(&SetWindowLongA)>([](HWND hWnd, int idx, LONG val) -> LONG
                    {
                        if (IsCallerFromModule(GetModuleHandleA(NULL)))
                            return WindowedModeWrapper::SetWindowLongA_Hook(hWnd, idx, val);
                        return shSetWindowLongA.stdcall<LONG>(hWnd, idx, val);
                    }));

                    static SafetyHookInline shSetWindowLongW = {};
                    shSetWindowLongW = safetyhook::create_inline(SetWindowLongW, static_cast<decltype(&SetWindowLongW)>([](HWND hWnd, int idx, LONG val) -> LONG
                    {
                        if (IsCallerFromModule(GetModuleHandleA(NULL)))
                            return WindowedModeWrapper::SetWindowLongW_Hook(hWnd, idx, val);
                        return shSetWindowLongW.stdcall<LONG>(hWnd, idx, val);
                    }));

                    static SafetyHookInline shAdjustWindowRect = {};
                    shAdjustWindowRect = safetyhook::create_inline(AdjustWindowRect, static_cast<decltype(&AdjustWindowRect)>([](LPRECT rect, DWORD style, BOOL menu) -> BOOL
                    {
                        if (IsCallerFromModule(GetModuleHandleA(NULL)))
                            return WindowedModeWrapper::AdjustWindowRect_Hook(rect, style, menu);
                        return shAdjustWindowRect.stdcall<BOOL>(rect, style, menu);
                    }));

                    static SafetyHookInline shAdjustWindowRectEx = {};
                    shAdjustWindowRectEx = safetyhook::create_inline(AdjustWindowRectEx, static_cast<decltype(&AdjustWindowRectEx)>([](LPRECT rect, DWORD style, BOOL menu, DWORD exstyle) -> BOOL
                    {
                        if (IsCallerFromModule(GetModuleHandleA(NULL)))
                            return WindowedModeWrapper::AdjustWindowRectEx_Hook(rect, style, menu, exstyle);
                        return shAdjustWindowRectEx.stdcall<BOOL>(rect, style, menu, exstyle);
                    }));

                    static SafetyHookInline shSetWindowPos = {};
                    shSetWindowPos = safetyhook::create_inline(SetWindowPos, static_cast<decltype(&SetWindowPos)>([](HWND hWnd, HWND insert, int x, int y, int cx, int cy, UINT flags) -> BOOL
                    {
                        if (IsCallerFromModule(GetModuleHandleA(NULL)))
                            return WindowedModeWrapper::SetWindowPos_Hook(hWnd, insert, x, y, cx, cy, flags);
                        return shSetWindowPos.stdcall<BOOL>(hWnd, insert, x, y, cx, cy, flags);
                    }));
                }

                // The device is always made windowed: the flag in the present parameters the game fills in
                auto presentParams = hook::pattern("89 5D 44 89 5D 48 8B 0D ? ? ? ? C6 44 24 ? 02 E8");
                static auto PresentParamsHook = safetyhook::create_mid(presentParams.get_first(6), [](SafetyHookContext& regs)
                {
                    *reinterpret_cast<uint32_t*>(regs.ebp + 0x3C) = 1;
                });
            }
        };
    }
} Windowed;