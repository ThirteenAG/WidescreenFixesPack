#pragma once

// Aspect ratio, field of view and HUD placement shared by the 3D cameras,
// the 2D canvas and movies. The game lays out its 3D view and 2D canvas for a
// 512x320 (16:10) screen; PSP movies fill the 480x272 frame.
namespace Display
{
constexpr float native = 512.0f / 320.0f;
constexpr float frame = 480.0f / 272.0f;
inline float forced, fovFactor{ 1.0f }, hudConstraint;
inline float aspect{ native }, cameraAspect;
inline float shiftX, shiftY;
inline injector::game_fn<void(float)> setGameAspect;
inline SafetyMipsMid cameraHook, projectionHook, canvasHook;
inline SafetyMipsInline flipHook;
inline injector::hook_back<int(unsigned, unsigned)> flip;

inline void Update()
{
    aspect = forced > 0.0f ? forced : Aspect();
    // Wider screens widen the 2D canvas; narrower ones keep its whole width
    // and add height. The HUD box is centered and keeps the full height; a box
    // narrower than the native layout keeps the native layout.
    const float halfWidth = std::max(aspect, native) * 0.5f;
    const float halfHeight = halfWidth / aspect;
    const float box = hudConstraint > 0.0f ? std::min(halfWidth, halfHeight * std::max(hudConstraint, native)) : halfWidth;
    shiftX = box - native * 0.5f;
    shiftY = halfHeight - 0.5f;
}

// Edge anchoring for a canvas position in screen fractions (0..1): elements
// in the outer thirds follow their screen edge, centered ones stay.
inline float Side(float position)
{
    return position < 1.0f / 3.0f ? -1.0f : position > 2.0f / 3.0f ? 1.0f : 0.0f;
}

inline int Flip(unsigned wait, unsigned sync)
{
    Update();
    if (aspect != cameraAspect)
        setGameAspect(aspect);
    return flip.fun(wait, sync);
}

inline void Install()
{
    Update();
    safetymips::Options scalar;
    scalar.preserve = PSP_HOOK_SAVE_FPU;
    // CamSetGameAspect: every camera's projection aspect follows the display.
    const auto setSite = Required("E0 FF BD 27 ? ? 02 3C 0C 00 BF AF 00 00 B0 AF ? ? 4C E4 08 00 B2 AF 05 00 12 24");
    setGameAspect = decltype(setGameAspect)(setSite);
    cameraHook = safetymips::create_mid(setSite, [](SafetyMipsContext& regs)
    {
        regs.f12 = aspect;
        cameraAspect = aspect;
    }, scalar);
    // Projection tangents (x, y) before they are stored. The vertical angle is
    // kept on wider screens; narrower screens keep the native horizontal view.
    projectionHook = safetymips::create_mid(Required("5C 00 01 C6 60 00 00 C6 04 00 BF 8F 64 00 01 E6 68 00 00 E6", 12),
        [](SafetyMipsContext& regs)
    {
        float scale = fovFactor;
        if (aspect < native)
            scale *= native / aspect;
        regs.f1 *= scale;
        regs.f0 *= scale;
        const auto camera = uintptr_t(regs.s0);
        *reinterpret_cast<float*>(camera + 0x5C) = regs.f1;
        *reinterpret_cast<float*>(camera + 0x60) = regs.f0;
    }, scalar);
    // 2D canvas: half height (f0) and aspect (f1) of the orthographic camera.
    safetymips::Options replace = scalar;
    replace.execute_original = false;
    canvasHook = safetymips::create_mid(Required("02 00 02 46 00 00 C3 8F", -4), [](SafetyMipsContext& regs)
    {
        regs.f0 *= regs.f2;
        if (aspect < native)
            regs.f0 *= native / aspect;
        regs.f1 = aspect;
    }, replace);
    flipHook = safetymips::create_inline(Required("D0 FF BD 27 0C 00 B3 AF ? ? 13 3C 01 00 03 24 ? ? 62 8E 00 00 B0 AF FF FF 10 24"), Flip);
    flip.fun = flipHook.original<decltype(flip.fun)>();
}
}
