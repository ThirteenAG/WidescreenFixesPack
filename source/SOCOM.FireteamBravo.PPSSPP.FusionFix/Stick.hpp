#pragma once

namespace Stick
{
inline uint8_t Byte(float value)
{
    if (!std::isfinite(value))
        return 128;
    return uint8_t(128 + int(std::clamp(value, -1.0f, 1.0f) * 127.0f));
}
inline void Sample(Input* input)
{
    if (!input || input->pad.Rsrv[0] != 128 || input->pad.Rsrv[1] != 128)
        return;
    float x{}, y{};
    // Stock PPSSPP exposes the host XInput right stick as Z and RZ.
    if (sceIoDevctl("kemulator:", 0x33, reinterpret_cast<void*>(11), 0, &x, sizeof(x)) < 0 ||
        sceIoDevctl("kemulator:", 0x33, reinterpret_cast<void*>(14), 0, &y, sizeof(y)) < 0)
        return;
    input->pad.Rsrv[0] = Byte(x);
    input->pad.Rsrv[1] = Byte(-y);
}
}
