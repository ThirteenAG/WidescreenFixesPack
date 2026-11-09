module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <optional>

export module Hud;

import ComVars;

export
{
bool InGameplay()
{
    return GameFlags.is_initialized() && (GameFlags.get() & 0x81) == 0x80;
}

// MAIN.HudAspectRatioConstraint: the HUD's box; Auto (a very wide ratio) uses the whole screen
std::atomic<float> fHudAspect = 100.0f;
void UpdateHudAspect()
{
    const auto parsed = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
    fHudAspect = parsed ? std::clamp(*parsed, 4.0f / 3.0f, 32.0f / 9.0f) : 100.0f;
}

// Where a stretched 4:3 element (left edge and width in screen pixels) goes: x / fAspectRatioDiff + the returned
// shift. In play each element keeps its 4:3 size and its distance from the side it belongs to (left, right or
// centred by where it sits) inside the HUD box, the whole screen with Auto; menus keep the centred 4:3 box.
float HudShift(float left, float width)
{
    if (!InGameplay())
        return Screen.fHudOffsetReal;
    const float centre = (left + width * 0.5f) / Screen.fWidth;
    const float boxWidth = std::min(Screen.fWidth, Screen.fHeight * fHudAspect.load());
    const float boxLeft = (Screen.fWidth - boxWidth) * 0.5f;
    if (centre < 1.0f / 3.0f)
        return boxLeft;
    if (centre > 2.0f / 3.0f)
        return boxLeft + boxWidth - Screen.fWidth / Screen.fAspectRatioDiff;
    return Screen.fHudOffsetReal;
}

// The 3D weapon and item icons: the game clears the depth of the icon's stretched 4:3 rectangle, draws the model
// with the scene camera at a spot worked out for a stretched 4:3 view, then sets the camera's field of view back.
// The cleared rectangle now goes where the HUD puts that box, and until the field of view is set back the camera has
// the game's own lens and maps the screen's width to 4:3 at the same shift (in the camera for the game's own
// projection, in the device's projection for the vertex shaders and fixed function), so the model lands in the
// rectangle.
uint32_t nIconLensOffset = 0;       // the camera's lens
uint32_t nIconViewportOffset = 0;   // its viewport: x, y, width, height
uint32_t nIconHalfWidthOffset = 0;  // minus half the viewport's width, screen x per projected x
uint32_t nIconCentreOffset = 0;     // the viewport's centre x

struct IconCamera
{
    uintptr_t camera;
    float lens;
    float shift;
};
std::optional<IconCamera> IconView;

uintptr_t CurrentCamera()
{
    const auto renderer = reinterpret_cast<uint32_t*>(Memory.get());
    return renderer && renderer[0x10] ? renderer[0x10] + off_2C0 * renderer[0x13] : 0;
}

// The camera's viewport setter also rebuilds everything that comes from the lens
void RebuildCamera(uintptr_t camera)
{
    using SetViewportFn = void(__thiscall*)(uintptr_t camera, int32_t x, int32_t y, int32_t width, int32_t height);
    const auto viewport = reinterpret_cast<int32_t*>(camera + nIconViewportOffset);
    (*reinterpret_cast<SetViewportFn**>(camera))[0](camera, viewport[0], viewport[1], viewport[2], viewport[3]);
}

template<typename T> T& CameraField(uintptr_t camera, uint32_t offset)
{
    return *reinterpret_cast<T*>(camera + offset);
}

// rebuild: false when the game's field of view setter is about to rebuild the camera anyway
void EndIconView(bool rebuild)
{
    if (!IconView)
        return;
    CameraField<float>(IconView->camera, nIconLensOffset) = IconView->lens;
    if (rebuild)
        RebuildCamera(IconView->camera);
    IconView.reset();
}

void BeginIconView(float shift)
{
    EndIconView(true);
    const auto camera = CurrentCamera();
    if (!camera || !NativeFOV.is_initialized() || Screen.fFieldOfView <= 0.0f)
        return;
    auto& lens = CameraField<float>(camera, nIconLensOffset);
    IconView = IconCamera{ camera, lens, shift };
    lens *= NativeFOV.get() / Screen.fFieldOfView;
    RebuildCamera(camera);
    const auto viewport = reinterpret_cast<int32_t*>(camera + nIconViewportOffset);
    CameraField<float>(camera, nIconHalfWidthOffset) /= Screen.fAspectRatioDiff;
    CameraField<float>(camera, nIconCentreOffset) = viewport[0] + shift + viewport[2] * 0.5f / Screen.fAspectRatioDiff;
}

// Clip-space x becomes x / fAspectRatioDiff + offset * w: the screen's width as 4:3, moved by the icon's shift
float IconClipOffset()
{
    const float halfWidth = Screen.fWidth * 0.5f;
    return (halfWidth / Screen.fAspectRatioDiff + IconView->shift - halfWidth) / halfWidth;
}

SafetyHookInline shSetTransform = {};
long __stdcall SetTransform(void* device, uint32_t state, const float* matrix)
{
    constexpr uint32_t Projection = 3;
    if (state != Projection || !IconView || !matrix)
        return shSetTransform.unsafe_stdcall<long>(device, state, matrix);
    // Row-major: column 0 gives x, column 3 gives w
    float projection[16];
    std::copy_n(matrix, 16, projection);
    const float offset = IconClipOffset();
    for (int row = 0; row < 4; ++row)
        projection[row * 4] = projection[row * 4] / Screen.fAspectRatioDiff + offset * projection[row * 4 + 3];
    return shSetTransform.unsafe_stdcall<long>(device, state, projection);
}

SafetyHookInline shSetVertexShaderConstantF = {};
long __stdcall SetVertexShaderConstantF(void* device, uint32_t start, const float* data, uint32_t count)
{
    if (start != 0 || count < 4 || !IconView || !data)
        return shSetVertexShaderConstantF.unsafe_stdcall<long>(device, start, data, count);
    // The transposed world, view and projection: register 0 gives x, register 3 gives w
    std::vector<float> constants(data, data + count * 4);
    const float offset = IconClipOffset();
    for (int i = 0; i < 4; ++i)
        constants[i] = constants[i] / Screen.fAspectRatioDiff + offset * constants[12 + i];
    return shSetVertexShaderConstantF.unsafe_stdcall<long>(device, start, constants.data(), count);
}

void HookIconDevice(void* device)
{
    if (shSetTransform)
        return;
    const auto methods = *reinterpret_cast<void***>(device);
    shSetTransform = safetyhook::create_inline(methods[44], SetTransform);
    shSetVertexShaderConstantF = safetyhook::create_inline(methods[94], SetVertexShaderConstantF);
}

SafetyHookInline shClearRect = {};
int __stdcall ClearRect(int left, int top, int right, int bottom, int target, int stencil)
{
    if (!target && (left > 0 || right < Screen.Width))
    {
        const float shift = HudShift(float(left), float(right - left));
        left = int(std::lround(left / Screen.fAspectRatioDiff + shift));
        right = int(std::lround(right / Screen.fAspectRatioDiff + shift));
        BeginIconView(shift);
    }
    return shClearRect.unsafe_stdcall<int>(left, top, right, bottom, target, stencil);
}

void ResetIconProjection()
{
    EndIconView(true);
}

void __cdecl sub_44A390(uintptr_t a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, uintptr_t a12, uintptr_t a13, uintptr_t a14)
{
    if (!a1 || !Memory.is_initialized() || !Memory.get() ||
        (!a14 && (!std::isfinite(a10) || !std::isfinite(a11) || a10 == 0.0f || a11 == 0.0f)))
        return;
    uintptr_t rendererMemory = Memory.get();

    if (a2 == 0.0f && a4 == Screen.fWidth)
    {
        if (a12)
        {
            if (*(uint32_t*)(a12 + 0x00) == 0xffffffff && *(uint32_t*)(a12 + 0x04) == 0xffffffff && *(uint32_t*)(a12 + 0x08) == 0xffffffff && *(uint32_t*)(a12 + 0x0C) == 0xffffffff)
            {
                a2 /= Screen.fAspectRatioDiff;
                a4 /= Screen.fAspectRatioDiff;
                a2 += Screen.fHudOffsetReal;
            }
        }
    }
    else
    {
        a2 = a2 / Screen.fAspectRatioDiff + HudShift(a2, a4);
        a4 /= Screen.fAspectRatioDiff;
    }

    float v14 = a6 + 0.5f;
    float v22 = a11 - a7 - 0.5f;
    float v15 = 1.0f / a10;
    float v23 = 1.0f / a11;

    if (a14)
    {
        *reinterpret_cast<float*>(a1 + 0x20) = a2 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_288) + a4;
        *reinterpret_cast<float*>(a1 + 0x38) = a8;
        *reinterpret_cast<float*>(a1 + 0x24) = a3 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_290);
        *reinterpret_cast<float*>(a1 + 0x3C) = a7;
        *reinterpret_cast<float*>(a1 + 0x18) = a6;
        *reinterpret_cast<float*>(a1 + 0x00) = a2 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_288);
        *reinterpret_cast<float*>(a1 + 0x1C) = a9;
        *reinterpret_cast<float*>(a1 + 0x04) = a5 + *(float*)(a1 + 0x24);
    }
    else
    {
        *reinterpret_cast<float*>(a1 + 0x00) = a2 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_288);
        *reinterpret_cast<float*>(a1 + 0x04) = a3 + *(float*)(off_2C0 * *((uint32_t*)rendererMemory + 0x13) + *((uint32_t*)rendererMemory + 0x10) + off_290);
        *reinterpret_cast<float*>(a1 + 0x18) = v15 * v14;
        *reinterpret_cast<float*>(a1 + 0x1C) = v23 * v22;
        *reinterpret_cast<float*>(a1 + 0x20) = a4 + *(float*)a1;
        *reinterpret_cast<float*>(a1 + 0x24) = a5 + *(float*)(a1 + 4);
        *reinterpret_cast<float*>(a1 + 0x38) = (v14 + a8) * v15;
        *reinterpret_cast<float*>(a1 + 0x3C) = (v22 - a9) * v23;
    }
    *reinterpret_cast<uint32_t*>(a1 + 0x40) = *(uint32_t*)(a1 + 0x20);
    *reinterpret_cast<uint32_t*>(a1 + 0x44) = *(uint32_t*)(a1 + 0x24);
    *reinterpret_cast<uint32_t*>(a1 + 0x48) = *(uint32_t*)(a1 + 0x28);
    *reinterpret_cast<uint32_t*>(a1 + 0x4C) = *(uint32_t*)(a1 + 0x2C);
    *reinterpret_cast<uint32_t*>(a1 + 0x50) = *(uint32_t*)(a1 + 0x30);
    *reinterpret_cast<uint32_t*>(a1 + 0x54) = *(uint32_t*)(a1 + 0x34);
    *reinterpret_cast<uint32_t*>(a1 + 0x58) = *(uint32_t*)(a1 + 0x38);
    *reinterpret_cast<uint32_t*>(a1 + 0x5C) = *(uint32_t*)(a1 + 0x3C);
    *reinterpret_cast<uint32_t*>(a1 + 0x60) = *(uint32_t*)(a1 + 0x00);
    *reinterpret_cast<uint32_t*>(a1 + 0x64) = *(uint32_t*)(a1 + 0x04);
    *reinterpret_cast<uint32_t*>(a1 + 0x68) = *(uint32_t*)(a1 + 0x08);
    *reinterpret_cast<uint32_t*>(a1 + 0x6C) = *(uint32_t*)(a1 + 0x0C);
    *reinterpret_cast<uint32_t*>(a1 + 0x70) = *(uint32_t*)(a1 + 0x10);
    *reinterpret_cast<uint32_t*>(a1 + 0x74) = *(uint32_t*)(a1 + 0x14);
    *reinterpret_cast<uint32_t*>(a1 + 0x78) = *(uint32_t*)(a1 + 0x18);
    *reinterpret_cast<uint32_t*>(a1 + 0x7C) = *(uint32_t*)(a1 + 0x1C);
    *reinterpret_cast<uint32_t*>(a1 + 0x3C) = *(uint32_t*)(a1 + 0x1C);
    *reinterpret_cast<uint32_t*>(a1 + 0x24) = *(uint32_t*)(a1 + 0x04);
    *reinterpret_cast<uint32_t*>(a1 + 0x7C) = *(uint32_t*)(a1 + 0x5C);
    *reinterpret_cast<uint32_t*>(a1 + 0x64) = *(uint32_t*)(a1 + 0x44);
    *reinterpret_cast<uint32_t*>(a1 + 0x08) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x0C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x10) = *(uint32_t*)(a12 + 0x00);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x10) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x14) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x14) = 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x28) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x2C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x30) = *(uint32_t*)(a12 + 0x04);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x30) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x34) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x34) = 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x48) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x4C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x50) = *(uint32_t*)(a12 + 0x08);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x50) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x54) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x54) = 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x68) = 0x3727C5AC;
    *reinterpret_cast<uint32_t*>(a1 + 0x6C) = 0x3F800000;
    if (a12)
        *reinterpret_cast<uint32_t*>(a1 + 0x70) = *(uint32_t*)(a12 + 0x0C);
    else
        *reinterpret_cast<uint32_t*>(a1 + 0x70) = 0xFFFFFFFF;
    *reinterpret_cast<uint32_t*>(a1 + 0x74) &= 0xFF000000;
    *reinterpret_cast<uint32_t*>(a1 + 0x74) = 0xFF000000;
    if (a13)
        *reinterpret_cast<uint32_t*>(a13) = 0;
}

void InitHud()
{
    hook::pattern pattern;
    //HUD
    UpdateHudAspect();
    FixSettings().OnChange("MAIN.HudAspectRatioConstraint", UpdateHudAspect);
    GameFlags.SetAddress(*hook::get_pattern<uint32_t*>("F6 05 ? ? ? ? 01 75 0C D9 41 18", 2)); //0x5C6E70
    Memory.SetAddress(*(uintptr_t**)hook::get_pattern("8B 0D ? ? ? ? DD D8 8B 51 4C DD D8", 2)); //0x5C5D7C
    off_288 = *hook::get_pattern<uint32_t>("D8 84 0A ? ? ? ? D8 44 24 14 D9 58 ? 8B 0D", 3);
    off_290 = *hook::get_pattern<uint32_t>("8B 54 24 24 8B 4C 24 20 89 50 38 D9 58 24", -4);
    off_2C0 = *hook::get_pattern<uint32_t>("8B 54 24 24 8B 4C 24 20 89 50 38 D9 58 24", -11);
    pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 83 C4 38 50");
    injector::MakeCALL(pattern.get_first(0), sub_44A390, true); //0x44A6C0
    pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 38 51");
    injector::MakeCALL(pattern.get_first(0), sub_44A390, true); //0x44AB58

    // 3D icons
    pattern = hook::pattern("8B 0D ? ? ? ? 85 C9 53 55 56 57 74 ? 8B 74 24 14 8B 7C 24 1C 3B F7");
    shClearRect = safetyhook::create_inline(pattern.get_first(), ClearRect); //0x4B09D0
    // The camera's lens setter: the lens, then the viewport (height, width, y, x) handed to the viewport setter
    pattern = hook::pattern("8B 44 24 04 8B 11 89 81 ? ? ? ? 8B 81 ? ? ? ? 50 8B 81 ? ? ? ? 50 8B 81 ? ? ? ? 50 8B 81 ? ? ? ? 50");
    nIconLensOffset = *pattern.get_first<uint32_t>(8); //0x27C
    nIconViewportOffset = *pattern.get_first<uint32_t>(35); //0x298
    // The viewport setter: minus half the width, minus half the height, then the centre
    pattern = hook::pattern("D9 9E ? ? ? ? 50 D9 44 24 20 D8 0D ? ? ? ? D9 9E ? ? ? ? DB 44 24 14 89 54 24 14 D9 44 24 1C D8 0D ? ? ? ? D8 C1 D9 9E");
    nIconHalfWidthOffset = *pattern.get_first<uint32_t>(2); //0x260
    nIconCentreOffset = *pattern.get_first<uint32_t>(45); //0x280
    // The camera's field of view being set (back) ends the icon; the setter rebuilds the camera
    pattern = hook::pattern("D9 44 24 04 8B 91 ? ? ? ? D8 0D ? ? ? ? 8B 01 52 8B 91");
    static auto FieldOfViewHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (IconView && IconView->camera == regs.ecx)
            EndIconView(false);
    }); //0x4B21F0


}

}
