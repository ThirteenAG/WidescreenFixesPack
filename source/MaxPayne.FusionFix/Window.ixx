module;

#include <stdafx.h>
#include <commctrl.h>
#include "FixSettings.hpp"

#pragma comment(lib, "comctl32.lib")

export module Window;

import ComVars;
import PostFX;

// Window mode (MAIN.WindowMode). The game opens its video mode with sub_40D040(windowed), windowed for -window, and the
// window as large as the resolution plus the frame sizes kept at CWnd[22] and CWnd[23], at CWnd[24], CWnd[25]. A window
// larger than the screen, CWnd[18] and CWnd[19], was moved to the corner and shrunk to it, the game rendering at less
// than the resolution. Windowed: the frame of the window's real style, the window centered on its monitor by its
// client area, which a window as large as the monitor covers exactly with the frame outside. Borderless: no frame.
// Full screen with -window keeps the borderless window it had.
export namespace WindowMode
{
    enum
    {
        Fullscreen,
        Windowed,
        Borderless,
    };

    // MAIN.WindowMode as it was at startup
    int32_t nWindowMode = Fullscreen;
    bool firstMode = true;
    bool previousWindowed = false;
    thread_local bool changingMode = false;
    HMONITOR lastWindowMonitor = nullptr;
    constexpr UINT_PTR DpiSubclassId = 0x4D504450;
    constexpr UINT DpiChanged = 0x02E0;

    RECT GetDesktopRect(HWND window, HMONITOR monitor = nullptr)
    {
        MONITORINFOEXW info = { sizeof(info) };
        if (!monitor)
            monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
        if (!GetMonitorInfoW(monitor, reinterpret_cast<MONITORINFO*>(&info)))
            return { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

        // Exclusive fullscreen changes the current mode; the desktop mode is kept in the display configuration.
        DEVMODEW mode = {};
        mode.dmSize = sizeof(mode);
        if (EnumDisplaySettingsExW(info.szDevice, ENUM_REGISTRY_SETTINGS, &mode, 0) && mode.dmPelsWidth > 0 &&
            mode.dmPelsWidth <= 32768 && mode.dmPelsHeight > 0 && mode.dmPelsHeight <= 32768)
        {
            return { mode.dmPosition.x, mode.dmPosition.y,
                     mode.dmPosition.x + static_cast<LONG>(mode.dmPelsWidth),
                     mode.dmPosition.y + static_cast<LONG>(mode.dmPelsHeight) };
        }
        return info.rcMonitor;
    }

    // Position and frame size of a window with the client size given, centered on the window's monitor in its DPI
    std::tuple<int32_t, int32_t, int32_t, int32_t> GetCenteredWindowRect(HWND hWnd, int32_t nWidth, int32_t nHeight,
                                                                        HMONITOR preferredMonitor = nullptr, UINT dpi = 0)
    {
        using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using GetDpi = UINT(WINAPI*)(HWND);
        static auto user32 = GetModuleHandleW(L"user32.dll");
        static auto adjustForDpi = reinterpret_cast<AdjustForDpi>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
        static auto getDpi = reinterpret_cast<GetDpi>(GetProcAddress(user32, "GetDpiForWindow"));

        auto style = (DWORD)GetWindowLongW(hWnd, GWL_STYLE);
        auto exStyle = (DWORD)GetWindowLongW(hWnd, GWL_EXSTYLE);
        RECT rect = { 0, 0, nWidth, nHeight };
        UINT nDpi = dpi ? dpi : (getDpi && hWnd ? getDpi(hWnd) : 0);
        if (!(adjustForDpi && nDpi && adjustForDpi(&rect, style, FALSE, exStyle, nDpi)))
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        auto monitor = GetDesktopRect(hWnd, preferredMonitor);
        int32_t x = monitor.left + (monitor.right - monitor.left - nWidth) / 2 + rect.left;
        int32_t y = monitor.top + (monitor.bottom - monitor.top - nHeight) / 2 + rect.top;
        // a window that fits on the monitor with its frame stays whole on it
        if (rect.right - rect.left <= monitor.right - monitor.left)
            x = std::clamp(x, int32_t(monitor.left), int32_t(monitor.right - (rect.right - rect.left)));
        if (rect.bottom - rect.top <= monitor.bottom - monitor.top)
            y = std::clamp(y, int32_t(monitor.top), int32_t(monitor.bottom - (rect.bottom - rect.top)));
        return { x, y, (rect.right - rect.left) - nWidth, (rect.bottom - rect.top) - nHeight };
    }

    LRESULT CALLBACK DpiWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR data)
    {
        if (message == WM_NCDESTROY)
            RemoveWindowSubclass(window, DpiWindowProc, DpiSubclassId);
        if (message != DpiChanged || nWindowMode != Windowed || changingMode || !data)
            return DefSubclassProc(window, message, wParam, lParam);

        auto CWnd = reinterpret_cast<int*>(data);
        int width = CWnd[20];
        int height = CWnd[21];
        auto proposed = reinterpret_cast<const RECT*>(lParam);
        HMONITOR monitor = proposed ? MonitorFromRect(proposed, MONITOR_DEFAULTTONEAREST) : nullptr;
        auto result = DefSubclassProc(window, message, wParam, lParam);
        auto style = static_cast<DWORD>(GetWindowLongW(window, GWL_STYLE));
        if (IsWindow(window) && (style & WS_CAPTION) == WS_CAPTION && width > 0 && width <= 32768 && height > 0 && height <= 32768)
        {
            auto [x, y, frameX, frameY] = GetCenteredWindowRect(window, width, height, monitor, HIWORD(wParam));
            auto write = [CWnd](int index, int value)
            {
                CWnd[index] = value;
            };
            write(20, width);
            write(21, height);
            write(22, frameX);
            write(23, frameY);
            write(24, x);
            write(25, y);
            write(18, std::max(CWnd[18], width + frameX));
            write(19, std::max(CWnd[19], height + frameY));
            SetWindowPos(window, nullptr, x, y, width + frameX, height + frameY,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
        return result;
    }
}

SafetyHookInline shsub_40D040 = {};
int __fastcall sub_40D040(int* CWnd, void* edx, char a2)
{
    struct RestoreModeChange
    {
        bool previous;
        ~RestoreModeChange()
        {
            WindowMode::changingMode = previous;
        }
    } restoreModeChange{ std::exchange(WindowMode::changingMode, true) };
    PrepareVideoModeChange();
    auto write = [CWnd](int index, int value)
    {
        CWnd[index] = value;
    };
    // Choose the startup mode once; later native Alt+Enter requests must be able to enter fullscreen.
    bool firstMode = std::exchange(WindowMode::firstMode, false);
    if (firstMode && WindowMode::nWindowMode != WindowMode::Fullscreen)
        a2 = 1;

    auto hWnd = *(HWND*)&CWnd[8];
    if (!a2 && WindowMode::previousWindowed && IsWindow(hWnd))
        WindowMode::lastWindowMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    HMONITOR preferredMonitor = a2 && !firstMode && !WindowMode::previousWindowed ? WindowMode::lastWindowMonitor : nullptr;

    int width = CWnd[20];
    int height = CWnd[21];
    bool desktopMode = a2 && WindowMode::nWindowMode != WindowMode::Windowed &&
                       (WindowMode::nWindowMode == WindowMode::Borderless || !firstMode);

    if (a2 && WindowMode::nWindowMode != WindowMode::Windowed)
    {
        // border size to 0
        write(22, 0);
        write(23, 0);

        auto desktop = WindowMode::GetDesktopRect(hWnd, preferredMonitor);
        if (desktopMode)
        {
            write(20, desktop.right - desktop.left);
            write(21, desktop.bottom - desktop.top);
            write(18, std::max(CWnd[18], CWnd[20]));
            write(19, std::max(CWnd[19], CWnd[21]));
        }

        // center window position
        write(24, desktop.left + (desktop.right - desktop.left - CWnd[20]) / 2);
        write(25, desktop.top + (desktop.bottom - desktop.top - CWnd[21]) / 2);
    }
    else if (a2)
    {
        auto [x, y, nFrameX, nFrameY] = WindowMode::GetCenteredWindowRect(hWnd, CWnd[20], CWnd[21], preferredMonitor);
        write(22, nFrameX);
        write(23, nFrameY);
        write(24, x);
        write(25, y);
        write(18, std::max(CWnd[18], CWnd[20] + nFrameX));
        write(19, std::max(CWnd[19], CWnd[21] + nFrameY));
    }

    int result = shsub_40D040.unsafe_fastcall<int>(CWnd, edx, a2);
    hWnd = *(HWND*)&CWnd[8];
    if (a2 && WindowMode::nWindowMode == WindowMode::Windowed)
    {
        // The native switch restores the frame style after entering from fullscreen.
        auto [x, y, nFrameX, nFrameY] = WindowMode::GetCenteredWindowRect(hWnd, width, height, preferredMonitor);
        write(22, nFrameX);
        write(23, nFrameY);
        write(24, x);
        write(25, y);
        write(18, std::max(CWnd[18], width + nFrameX));
        write(19, std::max(CWnd[19], height + nFrameY));
        SetWindowPos(hWnd, nullptr, x, y, width + nFrameX, height + nFrameY,
                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
    if (desktopMode)
    {
        // Keep the selected resolution for the next exclusive fullscreen mode.
        write(20, width);
        write(21, height);
    }
    WindowMode::previousWindowed = a2 != 0;
    if (IsWindow(hWnd) && !SetWindowSubclass(hWnd, WindowMode::DpiWindowProc, WindowMode::DpiSubclassId,
                                              reinterpret_cast<DWORD_PTR>(CWnd)))
        OutputDebugStringA("Max Payne: DPI change handling unavailable.\n");
    return result;
}

// The display modes the startup dialog's options list, in the order the adapter enumerates them:
// smallest first. They're listed largest first instead.
// 16-bit modes are left out where the same size is there in 32-bit, P_Driver::setFullscreenMode
// opens those instead anyway.
namespace DisplayModeList
{
    enum
    {
        IDC_DISPLAY_MODE = 1002,
        CWND_HWND = 0x20, // CWnd::m_hWnd
    };

    // What the items point to, the game reads the choice back from it
    struct Mode
    {
        uint32_t nWidth;
        uint32_t nHeight;
        uint32_t nBitsPerPixel;
    };

    void Sort(HWND hComboBox)
    {
        struct Item
        {
            std::string text;
            const Mode* pMode;
        };

        std::vector<Item> Items;
        const Mode* pSelected = nullptr;
        auto nCount = (int)SendMessageA(hComboBox, CB_GETCOUNT, 0, 0);
        auto nSelected = (int)SendMessageA(hComboBox, CB_GETCURSEL, 0, 0);
        for (int i = 0; i < nCount; ++i)
        {
            auto pMode = (const Mode*)SendMessageA(hComboBox, CB_GETITEMDATA, i, 0);
            auto nLength = (int)SendMessageA(hComboBox, CB_GETLBTEXTLEN, i, 0);
            if (!pMode || pMode == (const Mode*)CB_ERR || nLength == CB_ERR)
                return;

            std::string text(nLength, '\0');
            SendMessageA(hComboBox, CB_GETLBTEXT, i, (LPARAM)text.data());
            if (i == nSelected)
                pSelected = pMode;
            Items.push_back({ std::move(text), pMode });
        }

        auto Is32Bit = [](const Item& item) { return item.pMode->nBitsPerPixel >= 32; };
        auto IsLeftOut = [&](const Item& item)
        {
            return !Is32Bit(item) && std::ranges::any_of(Items, [&](const Item& other)
            {
                return Is32Bit(other) && other.pMode->nWidth == item.pMode->nWidth && other.pMode->nHeight == item.pMode->nHeight;
            });
        };
        std::vector<Item> Kept;
        for (auto& item : Items)
        {
            if (!IsLeftOut(item))
                Kept.push_back(item);
        }
        Items = std::move(Kept);

        std::stable_sort(Items.begin(), Items.end(), [](const Item& a, const Item& b)
        {
            return std::tie(a.pMode->nWidth, a.pMode->nHeight, a.pMode->nBitsPerPixel) > std::tie(b.pMode->nWidth, b.pMode->nHeight, b.pMode->nBitsPerPixel);
        });

        SendMessageA(hComboBox, CB_RESETCONTENT, 0, 0);
        auto nNewSelected = CB_ERR;
        for (auto& item : Items)
        {
            // unlike CB_ADDSTRING, keeps the order even if the list sorts itself
            auto nIndex = (int)SendMessageA(hComboBox, CB_INSERTSTRING, -1, (LPARAM)item.text.c_str());
            SendMessageA(hComboBox, CB_SETITEMDATA, nIndex, (LPARAM)item.pMode);
            if (item.pMode == pSelected)
                nNewSelected = nIndex;
        }

        // a 16-bit mode picked before, its 32-bit one
        if (nNewSelected == CB_ERR && pSelected)
        {
            for (int i = 0; i < (int)Items.size(); ++i)
            {
                if (Items[i].pMode->nWidth == pSelected->nWidth && Items[i].pMode->nHeight == pSelected->nHeight)
                {
                    nNewSelected = i;
                    break;
                }
            }
        }

        SendMessageA(hComboBox, CB_SETCURSEL, nNewSelected == CB_ERR ? 0 : nNewSelected, 0);
    }

    // Fills the dialog's lists for the adapter picked
    SafetyHookInline shFill = {};
    int __fastcall Fill(uint8_t* _this, void* edx)
    {
        auto result = shFill.unsafe_fastcall<int>(_this, edx);
        if (auto hComboBox = GetDlgItem(*(HWND*)(_this + CWND_HWND), IDC_DISPLAY_MODE))
            Sort(hComboBox);
        return result;
    }
}

class Window
{
public:
    Window()
    {
        WFP::onInitEvent() += []()
        {
            // Startup dialog, see DisplayModeList
            auto pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 4C 53 55 56 57 8B D9 68 E8 03 00 00 89 5C 24 20 E8");
            DisplayModeList::shFill = safetyhook::create_inline(pattern.get_first(), DisplayModeList::Fill); //0x4078E0

            // Window mode, see sub_40D040
            pattern = hook::pattern("83 EC 08 53 55 56 57 8B F1 E8 ? ? ? ? 8B 78");
            shsub_40D040 = safetyhook::create_inline(pattern.get_first(), sub_40D040);
        };
    }
} Window;
