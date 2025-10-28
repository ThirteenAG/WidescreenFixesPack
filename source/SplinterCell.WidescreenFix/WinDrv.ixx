module;

#include <stdafx.h>

export module WinDrv;

import ComVars;

export void InitWinDrv()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8B 5D 10 8B 46 18 83 FB FF");
    struct OpenWindowHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            tagRECT rect;
            rect.left = (LONG)(((float)DesktopResW / 2.0f) - ((float)Screen.Width / 2.0f));
            rect.top = (LONG)(((float)DesktopResH / 2.0f) - ((float)Screen.Height / 2.0f));
            rect.right = (LONG)Screen.Width;
            rect.bottom = (LONG)Screen.Height;

            *(int32_t*)(regs.esp + 0x474) = rect.right;
            *(int32_t*)(regs.esp + 0x478) = rect.bottom;
            *(int32_t*)(regs.esp + 0x47C) = rect.left;
            *(int32_t*)(regs.esp + 0x480) = rect.top;

            regs.ebx = *(uint32_t*)(regs.ebp + 0x10);
            regs.eax = *(uint32_t*)(regs.esi + 0x18);
        }
    }; injector::MakeInline<OpenWindowHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8D 4D B8 68 ? ? ? ? C7 45 ? ? ? ? ? 51");
    injector::WriteMemory(pattern.get_first(4), *pattern.get_first<LONG>(4) & ~WS_OVERLAPPEDWINDOW, true);
    injector::WriteMemory(pattern.get_first(11), *pattern.get_first<LONG>(11) & ~WS_OVERLAPPEDWINDOW, true);

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "6A 00 8D 55 B8 68 ? ? ? ? C7 45");
    injector::WriteMemory(pattern.get_first(6), *pattern.get_first<LONG>(6) & ~WS_OVERLAPPEDWINDOW, true);
    injector::WriteMemory(pattern.get_first(13), *pattern.get_first<LONG>(13) & ~WS_OVERLAPPEDWINDOW, true);

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "68 ? ? ? ? 6A F0 83 38 00 74 1D 8B 8E");
    injector::WriteMemory(pattern.get_first(1), *pattern.get_first<LONG>(1) & ~WS_OVERLAPPEDWINDOW, true);

    pattern = hook::module_pattern(GetModuleHandle(L"WinDrv"), "8B 8E ? ? ? ? 8B 51 04 52 FF 15 ? ? ? ? 8D A5");
    struct OpenWindowHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            auto [DesktopResW, DesktopResH] = GetDesktopRes();
            tagRECT rect;
            rect.left = (LONG)(((float)DesktopResW / 2.0f) - ((float)Screen.Width / 2.0f));
            rect.top = (LONG)(((float)DesktopResH / 2.0f) - ((float)Screen.Height / 2.0f));
            rect.right = (LONG)Screen.Width;
            rect.bottom = (LONG)Screen.Height;

            regs.ecx = *(uint32_t*)(regs.esi + 0x180);
            regs.edx = *(uint32_t*)(regs.ecx + 0x04);

            SetWindowLong((HWND)regs.edx, GWL_STYLE, GetWindowLong((HWND)regs.edx, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos((HWND)regs.edx, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
            SetForegroundWindow((HWND)regs.edx);
            SetCursor(NULL);
        }
    }; injector::MakeInline<OpenWindowHook2>(pattern.get_first(0), pattern.get_first(9));
}
