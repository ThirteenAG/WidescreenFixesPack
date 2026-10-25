#pragma once
#include <windows.h>

// Small declarations shared by the game modules; ImGui stays in Overlay.cpp.
namespace TCOverlay
{
    void Init();
    bool BlocksInput();
    bool Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam, LRESULT& result);
}
