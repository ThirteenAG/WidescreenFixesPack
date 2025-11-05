module;

#include <stdafx.h>
#include <hidusage.h>

export module Window;

import ComVars;

RAWINPUTDEVICE rid[1];
void RegisterRawInput(HWND hwndTarget)
{
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hwndTarget;
    RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE));
}

WNDPROC DefaultWndProc = nullptr;
LRESULT CALLBACK RawInputWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INPUT)
    {
        UINT dwSize = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
        std::vector<BYTE> buffer(dwSize);
        RAWINPUT* raw = (RAWINPUT*)buffer.data();
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)
        {
            if (raw->header.dwType == RIM_TYPEMOUSE)
            {
                int32_t dx = static_cast<int32_t>(raw->data.mouse.lLastX);
                int32_t dy = static_cast<int32_t>(raw->data.mouse.lLastY);

                RawMouseDeltaX = dx;
                RawMouseDeltaY = dy;

                RawMouseCursorX += dx;
                RawMouseCursorY += dy;
                RawMouseCursorX = std::max(int32_t(0), std::min(RawMouseCursorX, static_cast<int32_t>(Screen.Width)));
                RawMouseCursorY = std::max(int32_t(0), std::min(RawMouseCursorY, static_cast<int32_t>(Screen.Height)));
            }
        }
        return 0;  // Consume the message to avoid game interference.
    }
    return CallWindowProc(DefaultWndProc, hWnd, uMsg, wParam, lParam);
}

export void InitWindow()
{
    auto pattern = find_module_pattern(GetModuleHandle(L"Window"), "89 4D EC 83 7D EC 00 75", "89 8D C8 FB FF FF");
    static auto CreateWindowWHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        hGameWindow = (HWND)regs.ecx;
        if (Screen.bRawInputMouseForMenu)
        {
            DefaultWndProc = (WNDPROC)SetWindowLongPtr(hGameWindow, GWL_WNDPROC, (LONG_PTR)RawInputWndProc);
            RegisterRawInput(hGameWindow);
        }
    });
}
