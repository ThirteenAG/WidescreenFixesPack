#pragma once

namespace ScarfaceOverlayInput
{
    inline bool (*message)(HWND, UINT, WPARAM, LPARAM, LRESULT&) = nullptr;
    inline bool (*blocks)() = nullptr;

    inline bool Message(HWND window, UINT id, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        return message && message(window, id, wParam, lParam, result);
    }

    inline bool BlocksInput()
    {
        return blocks && blocks();
    }
}
