module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <d3d9.h>
#include <bit>

export module VehicleGlass;

// Vehicle windows: the PC window pixel shader binds the damage texture (shattered glass) but never samples it,
// so windows stay intact however damaged the car is. This replacement keeps the original window lighting
// (with its constants limited to the -1..1 range of the original shader model) and blends the shattered
// glass in by the window's damage (1 - health). Collisions take a window's health slowly, so the cracks
// come in early and are complete at a quarter of the damage; a scratch shows nothing.
//
//   c0.w specularity, c2.w = -damage, c3.w transparency; t3/s3 = damage texture
//   texld r0, t0, s0; texld r2, t2, s2; texld r3, t3, s3
//   r1.xy  = clamp(c0.w, c3.w; -1, 1)
//   r5.rgb = r2 * (r0.a * r1.x) + r0
//   r5.a   = dot(r2 * r1.x, luminance) + r1.y
//   crack  = 1 - (1 - saturate(-c2.w * CrackScale))^4
//   r5.rgb = saturate(r5 + r3 * crack); r5.a = max(r5.a, dot(r3, luminance) * crack)
namespace
{
    constexpr float CrackScale = 4.0f;

    constexpr DWORD windowPixelCode[] =
    {
        0xFFFF0200, 0x05000051, 0xA00F0004, 0x3E99999A, 0x3F170A3D, 0x3DE147AE,
        0x3F800000, 0x05000051, 0xA00F0005, std::bit_cast<DWORD>(CrackScale), 0x3F800000, 0xBF800000,
        0x00000000, 0x0200001F, 0x80000000, 0xB0030000, 0x0200001F, 0x80000000,
        0xB0030002, 0x0200001F, 0x80000000, 0xB0030003, 0x0200001F, 0x90000000,
        0xA00F0800, 0x0200001F, 0x90000000, 0xA00F0802, 0x0200001F, 0x90000000,
        0xA00F0803, 0x03000042, 0x800F0000, 0xB0E40000, 0xA0E40800, 0x03000042,
        0x800F0002, 0xB0E40002, 0xA0E40802, 0x03000042, 0x800F0003, 0xB0E40003,
        0xA0E40803, 0x02000001, 0x80010001, 0xA0FF0000, 0x02000001, 0x80020001,
        0xA0FF0003, 0x0300000A, 0x80030001, 0x80E40001, 0xA0550005, 0x0300000B,
        0x80030001, 0x80E40001, 0xA0AA0005, 0x03000005, 0x80080004, 0x80FF0000,
        0x80000001, 0x04000004, 0x80070005, 0x80E40002, 0x80FF0004, 0x80E40000,
        0x03000005, 0x80070006, 0x80E40002, 0x80000001, 0x03000008, 0x80010006,
        0x80E40006, 0xA0E40004, 0x03000002, 0x80080005, 0x80000006, 0x80550001,
        0x02000001, 0x80010007, 0xA1FF0002, 0x03000005, 0x80110007, 0x80000007,
        0xA0000005, 0x03000002, 0x80010007, 0x81000007, 0xA0550005, 0x03000005,
        0x80010007, 0x80000007, 0x80000007, 0x03000005, 0x80010007, 0x80000007,
        0x80000007, 0x03000002, 0x80010007, 0x81000007, 0xA0550005, 0x03000008,
        0x80020007, 0x80E40003, 0xA0E40004, 0x03000005, 0x80020007, 0x80550007,
        0x80000007, 0x04000004, 0x80170005, 0x80E40003, 0x80000007, 0x80E40005,
        0x0300000B, 0x80080005, 0x80FF0005, 0x80550007, 0x02000001, 0x800F0800,
        0x80E40005, 0x0000FFFF,
    };
}

class VehicleGlass
{
public:
    VehicleGlass()
    {
        WFP::onInitEvent() += []()
        {
            if (!FixSettings().Bool("GRAPHICS.RestoreWindowDamage"))
                return;

            // d3dVehicleShader constructor: CreatePixelShader(device, windowCode, &windowShader)
            auto pattern = hook::pattern("8B 0B 68 ? ? ? ? 68 ? ? ? ? 53 FF 91 A8 01 00 00");
            injector::WriteMemory(pattern.get_first(8), &windowPixelCode[0], true);
        };
    }
} VehicleGlass;
