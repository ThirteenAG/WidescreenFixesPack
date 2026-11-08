module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Windowed;

import ComVars;

class Windowed
{
public:
    Windowed()
    {
        WFP::onInitEvent() += []()
        {
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