#pragma once
#include <windows.h>
#include <cstdint>

namespace MaxPayneOverlay
{
    void Draw(void* device);
    void BeforeReset();
    void Release();
    bool BlocksInput();
    void Gamepad(uint16_t buttons, int8_t x, int8_t y);
    void Mouse(const uint8_t* state);
}
