#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Geometry
{
inline float forcedAspect{}, fovFactor{ 1.0f };
inline int borders{ 3 };
inline bool worldSeen;
inline uintptr_t projectionReturns[9];
inline uintptr_t* sceneCamera;
inline uintptr_t* manager;
inline uintptr_t worldOffsets;
inline SafetyMipsInline prepareHook, groundHook, clipHook;
inline injector::hook_back<uintptr_t(uintptr_t, uintptr_t)> ground;
inline injector::hook_back<int(uintptr_t, uintptr_t)> clipPlanes;
inline SafetyMipsMid projectionHook, projectionExitHook;
inline injector::hook_back<int(uintptr_t, int)> prepare;
inline void (*setHorizontal)(uintptr_t, int, float);
inline void (*setVertical)(uintptr_t, int, float);

inline float AspectRatio()
{
    return forcedAspect > 0.0f ? forcedAspect : Aspect();
}
inline bool Gameplay()
{
    return *manager && *reinterpret_cast<const int*>(*manager + 380) == 2;
}
inline float Factor()
{
    return Gameplay() ? fovFactor : 1.0f;
}
inline bool Cinematic()
{
    if (!*manager)
        return false;
    const int mode = *reinterpret_cast<const int*>(*manager + 380);
    return mode == 0 || mode == 1;
}
inline bool Framed()
{
    return Cinematic() && (borders & (AspectRatio() < 480.0f / 272.0f ? 1 : 2));
}
inline bool WorldReturn(uintptr_t address)
{
    for (const auto site : projectionReturns)
        if (site == address)
            return true;
    return false;
}
// Native conservative culling angles, expanded to contain the wider view.
inline void CullAngles(uintptr_t camera, float& horizontal, float& vertical)
{
    const auto angles = reinterpret_cast<const float*>(camera + 416);
    const float zoom = *reinterpret_cast<const float*>(camera + 744);
    vertical = std::clamp(angles[0] * (272.0f / 480.0f) * Factor() / zoom, 0.001f, 1.55f);
    if (Framed() && AspectRatio() < 480.0f / 272.0f)
        vertical = std::atan(std::tan(vertical) * (480.0f / 272.0f) / AspectRatio());
    horizontal = std::max(angles[0], std::atan(std::tan(vertical) * AspectRatio()));
    vertical = std::max(angles[1], vertical);
}
inline int Prepare(uintptr_t camera, int flags)
{
    if (camera != *sceneCamera)
        return prepare.fun(camera, flags);

    auto angles = reinterpret_cast<float*>(camera + 416);
    auto derived = reinterpret_cast<float*>(camera + 712);
    float originalAngles[2], originalDerived[8];
    std::memcpy(originalAngles, angles, sizeof(originalAngles));
    std::memcpy(originalDerived, derived, sizeof(originalDerived));
    float horizontal, vertical;
    CullAngles(camera, horizontal, vertical);
    setHorizontal(camera, 1, horizontal);
    setVertical(camera, 1, vertical);
    const int result = prepare.fun(camera, flags);
    std::memcpy(angles, originalAngles, sizeof(originalAngles));
    std::memcpy(derived, originalDerived, sizeof(originalDerived));
    return result;
}
// The world renderer builds the terrain's visible ground area from the
// camera angles again; give it the same expanded angles.
inline uintptr_t Ground(uintptr_t renderer, uintptr_t output)
{
    const auto camera = *reinterpret_cast<const uintptr_t*>(renderer + 11628);
    if (camera != *sceneCamera)
        return ground.fun(renderer, output);
    auto angles = reinterpret_cast<float*>(camera + 416);
    const float original[2]{ angles[0], angles[1] };
    CullAngles(camera, angles[0], angles[1]);
    const auto result = ground.fun(renderer, output);
    angles[0] = original[0];
    angles[1] = original[1];
    return result;
}
// The renderer clips geometry against side planes at 1.04 times the native
// half angle (+1312 full horizontal angle, +1316 aspect). Give it the visible
// half angles so terrain at the screen sides is not cut off.
inline int ClipPlanes(uintptr_t renderer, uintptr_t view)
{
    auto angles = reinterpret_cast<float*>(renderer + 1312);
    const float original[2]{ angles[0], angles[1] };
    if (angles[0] > 0.0f && angles[1] > 0.0f)
    {
        constexpr float native = 480.0f / 272.0f;
        const float nativeVertical = angles[0] * 0.5f / angles[1];
        float vertical = std::clamp(nativeVertical * Factor(), 0.001f, 1.55f);
        if (Framed() && AspectRatio() < native)
            vertical = std::atan(std::tan(vertical) * native / AspectRatio());
        const float horizontal = std::max(angles[0] * 0.5f, std::atan(std::tan(vertical) * AspectRatio()));
        vertical = std::max(vertical, nativeVertical);
        angles[0] = horizontal * 2.0f;
        angles[1] = horizontal / vertical;
    }
    const int result = clipPlanes.fun(renderer, view);
    angles[0] = original[0];
    angles[1] = original[1];
    return result;
}
inline void Projection(SafetyMipsContext& regs)
{
    if (!WorldReturn(regs.ra))
        return;
    worldSeen = true;
    regs.f13 = AspectRatio();
    // The HUD scene (compass, fader) keeps its size at any FOV factor.
    if (regs.ra != projectionReturns[7])
        regs.f12 *= Factor();
}
inline void ProjectionExit(SafetyMipsContext& regs)
{
    const float aspect = AspectRatio();
    if (!WorldReturn(regs.ra) || !Framed() || aspect >= 480.0f / 272.0f)
        return;
    auto matrix = reinterpret_cast<float*>(uintptr_t(regs.a0));
    const float factor = aspect / (480.0f / 272.0f);
    matrix[0] *= factor;
    matrix[5] *= factor;
}
inline void Install()
{
    const auto projectionSite = sites::Required("91 3E 06 3C 11 11 C6 34 00 80 86 44 02 63 10 46 80 3F 06 3C 00 88 86 44 43 8C 0C 46 AA 3E 06 3C");
    const auto prepareSite = sites::Required("90 FF BD 27 54 00 B0 AF 25 80 80 00 4C 3D 04 3C CD CC 84 34 48 00 B4 E7 00 60 84 44 A8 01 14 C6");
    const auto horizontalSite = sites::Required("F0 FF BD 27 FF 00 A5 30 A0 01 8C E4 00 00 AC E7 C8 02 86 24 D0 02 87 24 00 00 A0 CB 20 00 65 D0");
    const auto verticalSite = sites::Required("F0 FF BD 27 FF 00 A5 30 A4 01 8C E4 00 00 AC E7 CC 02 86 24 D4 02 87 24 00 00 A0 CB 20 00 65 D0");
    const auto managerSite = sites::Required("E0 FF BD 27 40 00 8C E4 ? ? 04 3C 10 00 BF AF ? ? ? ? ? ? 84 8C 10 00 BF 8F 08 00 E0 03");
    const auto cameraSite = sites::Required("? ? 06 3C 48 2D 0C E6 3C 02 A5 8C ? ? C6 24 ? ? C6 8C 6C 2D 06 AE 01 00 07 3C 25 30 A0 00");
    const auto contextSite = sites::Required("? ? 14 3C 80 40 09 3C 90 00 BC E7 9C 00 B2 AF A0 00 B3 AF 00 E0 89 44 00 00 04 34 28 00 B2 27");
    const char* callers[]
    {
        "24 05 CD C6 50 01 B0 27 28 05 CE C6 25 20 00 02 ? ? ? ? DC 01 A5 8F 25 20 C0 02 ? ? ? ?",
        "20 05 CC C6 24 05 CD C6 28 05 CE C6 25 20 00 02 ? ? ? ? DC 01 A5 8F 25 20 C0 02 ? ? ? ?",
        "20 00 B2 27 24 05 2D C6 28 05 2E C6 34 05 25 26 ? ? ? ? 25 20 40 02 25 20 20 02 ? ? ? ?",
        "30 01 B1 27 24 05 0D C6 28 05 0E C6 34 05 05 26 ? ? ? ? 25 20 20 02 25 20 00 02 ? ? ? ?",
        "24 05 0D C6 30 01 B6 27 28 05 0E C6 34 05 05 26 ? ? ? ? 25 20 C0 02 25 20 00 02 ? ? ? ?",
        "30 00 B0 27 00 E0 84 44 2C 05 FC E6 C6 E3 00 46 ? ? ? ? 25 20 00 02 25 20 E0 02 ? ? ? ?",
        "86 63 00 46 90 00 B0 27 06 A3 00 46 25 20 00 02 ? ? ? ? D4 00 A5 8F 25 20 E0 02 ? ? ? ?",
        "20 00 B5 27 24 05 8D C6 28 05 8E C6 34 05 85 26 ? ? ? ? 25 20 A0 02 25 20 80 02 ? ? ? ?",
        "38 05 84 AE 46 A3 00 46 40 00 B2 27 34 05 85 26 ? ? ? ? 25 20 40 02 25 20 80 02 ? ? ? ?",
    };
    for (unsigned i = 0; i < 9; ++i)
        projectionReturns[i] = sites::Required(callers[i], 24);
    const auto exitSite = sites::Required("38 00 85 24 00 00 AC E4 3C 00 84 24 08 00 E0 03 00 00 8D E4 D0 FF BD 27 00 00 C8 8C 04 00 C9 8C");
    manager = reinterpret_cast<uintptr_t*>(Absolute(managerSite + 8, 0, 12));
    sceneCamera = reinterpret_cast<uintptr_t*>(Absolute(cameraSite, 0, 12) + 320);
    worldOffsets = Absolute(contextSite, 0, 40) + 1332;
    setHorizontal = reinterpret_cast<decltype(setHorizontal)>(horizontalSite);
    setVertical = reinterpret_cast<decltype(setVertical)>(verticalSite);
    const auto groundSite = sites::Required("E0 FE BD 27 08 01 B4 E7 0C 01 B6 E7 10 01 B0 AF 14 01 B1 AF 18 01 B2 AF 1C 01 BF AF 25 88 A0 00 25 80 80 00 6C 2D 04 8E");
    prepareHook = safetymips::create_inline(prepareSite, Prepare);
    groundHook = safetymips::create_inline(groundSite, Ground);
    ground.fun = groundHook.original<decltype(ground.fun)>();
    const auto clipSite = sites::Required("00 FE BD 27 C8 01 B4 E7 CC 01 B6 E7 D0 01 B8 E7 D4 01 BA E7 D8 01 BC E7 DC 01 BE E7 E0 01 B0 AF E4 01 B1 AF");
    clipHook = safetymips::create_inline(clipSite, ClipPlanes);
    clipPlanes.fun = clipHook.original<decltype(clipPlanes.fun)>();
    prepare.fun = prepareHook.original<decltype(prepare.fun)>();
    safetymips::Options scalar;
    scalar.preserve = PSP_HOOK_SAVE_FPU;
    projectionHook = safetymips::create_mid(projectionSite, [](SafetyMipsContext& regs)
    {
        Projection(regs);
    }, scalar);
    projectionExitHook = safetymips::create_mid(exitSite, [](SafetyMipsContext& regs)
    {
        ProjectionExit(regs);
    }, scalar);
}
}
