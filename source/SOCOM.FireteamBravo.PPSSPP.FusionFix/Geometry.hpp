#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Geometry
{
inline float forcedAspect{}, fovFactor{ 1.0f };
inline int borders{ 3 };
inline bool worldSeen;
inline uintptr_t projectionReturns[7];
inline uintptr_t* sceneCamera;
inline uintptr_t* manager;
inline uintptr_t* player;
inline uintptr_t* scriptHudState;
inline SafetyMipsInline prepareHook;
inline SafetyMipsMid projectionHook;
inline injector::hook_back<int(uintptr_t, int)> prepare;
inline injector::game_fn<void(uintptr_t, int, float)> setHorizontal;
inline injector::game_fn<void(uintptr_t, int)> setVertical;

inline float AspectRatio()
{
    return forcedAspect > 0.0f ? forcedAspect : Aspect();
}
// First-person view uses camera mode 3; sights and scopes raise the camera zoom.
inline bool Zoomed()
{
    if (!*manager || *reinterpret_cast<const int*>(*manager + 380) != 3 || !*sceneCamera) return false;
    return *reinterpret_cast<const float*>(*sceneCamera + 740) > 1.05f; // Plain first person uses 1.01.
}
// The player's view state: 1 is plain first person, 5 and up are the scope
// zoom levels that draw the scope overlay.
inline bool ScopeOverlay()
{
    return Zoomed() && *player && *reinterpret_cast<const int*>(*player + 752) >= 5;
}
inline bool Gameplay()
{
    if (*scriptHudState) return false;
    if (!*manager) return false;
    const int mode = *reinterpret_cast<const int*>(*manager + 380);
    return mode == 2 || (mode == 3 && !Zoomed());
}
inline float Factor()
{
    return Gameplay() ? fovFactor : 1.0f;
}
inline bool Cinematic()
{
    // Mission scripts save the current HUD mode while HUD_OFF is active.
    // Scripted cameras can leave the player camera manager in mode 2.
    if (*scriptHudState) return true;
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
inline int Prepare(uintptr_t camera, int flags)
{
    if (camera != *sceneCamera)
        return prepare.fun(camera, flags);

    // Native culling planes come from the half angles at +416/+420 and the
    // sines, cosines and tangents after them; widen them to the visible view.
    constexpr float native = 480.0f / 272.0f;
    auto angles = reinterpret_cast<float*>(camera + 416);
    auto derived = reinterpret_cast<float*>(camera + 708);
    float originalAngles[2], originalDerived[8];
    std::memcpy(originalAngles, angles, sizeof(originalAngles));
    std::memcpy(originalDerived, derived, sizeof(originalDerived));
    const float aspect = AspectRatio();
    const float tangent = std::tan(std::clamp(angles[0] * Factor(), 0.001f, 1.55f));
    const float horizontal = std::atan(tangent * std::max(aspect / native, 1.0f));
    const float vertical = std::atan(tangent / std::min(aspect, native));
    setHorizontal(camera, 1, std::max(angles[0], horizontal));
    setVertical(camera, 1);
    if (vertical > angles[1])
    {
        // sin, cos, tan and 1/tan of the vertical half angle.
        angles[1] = vertical;
        derived[1] = std::sin(vertical);
        derived[3] = std::cos(vertical);
        derived[5] = derived[1] / derived[3];
        derived[7] = derived[3] / derived[1];
    }
    const int result = prepare.fun(camera, flags);
    std::memcpy(angles, originalAngles, sizeof(originalAngles));
    std::memcpy(derived, originalDerived, sizeof(originalDerived));
    return result;
}
inline void Projection(SafetyMipsContext& regs)
{
    // The game passes the horizontal angle and derives the vertical one from
    // the aspect. Wider screens keep the native height (Hor+); narrower ones
    // keep the native width, so menus and scenes always fit whole.
    if (!WorldReturn(regs.ra))
        return;
    worldSeen = true;
    constexpr float native = 480.0f / 272.0f;
    const float aspect = AspectRatio();
    // The HUD plane uses the camera angle too; keep it independent of FOVFactor.
    const float factor = regs.ra == projectionReturns[3] ? 1.0f : Factor();
    const float half = std::clamp(regs.f12 * factor * 0.5f, 0.001f, 1.55f);
    regs.f12 = 2.0f * std::atan(std::tan(half) * std::max(aspect / native, 1.0f));
    regs.f13 = aspect;
}
inline void Install()
{
    const auto projectionSite = sites::Required("F0 FF BD 27 00 3F 06 3C 00 80 86 44 02 63 10 46 00 00 AC E7 00 00 A0 CB 20 00 65 D0", 8);
    const auto prepareSite = sites::Required("90 FF BD 27 54 00 B0 AF 25 80 80 00 4C 3D 04 3C CD CC 84 34 48 00 B4 E7 00 60 84 44 A8 01 14 C6");
    const auto horizontalSite = sites::Required("F0 FF BD 27 FF 00 A5 30 A0 01 8C E4 00 00 AC E7 00 00 A0 CB 20 00 65 D0");
    const auto verticalSite = sites::Required("F0 FF BD 27 FF 00 A5 30 A0 01 8D C4 ? ? 06 3C ? ? C6 24 C4 05 CE C4 80 3F 06 3C 00 60 86 44");
    const auto managerSite = sites::Required("E0 FF BD 27 40 00 8C E4 ? ? 04 3C 10 00 BF AF ? ? ? ? ? ? 84 8C 10 00 BF 8F 08 00 E0 03");
    const auto cameraSite = sites::Required("? ? 04 3C ? ? 91 24 01 00 05 34 40 01 32 8E ? ? ? ? 25 20 C0 02");
    const auto playerSite = sites::Required("50 FF BD 27 98 00 B2 AF ? ? 12 3C 9C 00 B3 AF ? ? 53 8E 94 00 B1 AF E0 00 91 8C 18 00 65 26");
    const auto scriptHudSite = sites::Required("E0 FF BD 27 10 00 B0 AF ? ? 10 3C ? ? 04 8E 14 00 BF AF 04 00 80 10 25 28 80 00");
    const char* callers[] {
        "C0 05 4C C6 D4 05 45 AE 50 00 B3 27 C4 05 4D C6 C8 05 4E C6 D0 05 45 26 ? ? ? ? 25 20 60 02",
        "06 73 00 46 30 00 B2 27 CC 05 11 E6 86 7B 00 46 25 20 40 02 C6 8B 00 46 ? ? ? ? D0 00 A5 8F",
        "86 6B 00 46 90 00 B1 27 46 7B 00 46 CC 05 10 E6 25 20 20 02 C6 83 00 46 ? ? ? ? D0 00 A5 8F",
        "C0 05 8C C6 D4 05 85 AE 20 00 B5 27 C4 05 8D C6 C8 05 8E C6 D0 05 85 26 ? ? ? ? 25 20 A0 02",
        "CC 05 0F E6 50 00 B3 27 D0 05 05 26 ? ? ? ? 25 20 60 02",
        "C0 05 0C C6 C4 05 0D C6 C8 05 0E C6 64 00 B1 AF 20 00 B1 27 CC 05 0F C6 D0 05 05 26 68 00 BF AF ? ? ? ? 25 20 20 02",
        "C8 05 0E E6 CC 05 14 E6 25 20 A0 02 C6 A3 00 46 ? ? ? ? 25 28 80 02"
    };
    constexpr int returnOffsets[] {32, 32, 32, 32, 20, 40, 24};
    for (unsigned i = 0; i < 7; ++i)
        projectionReturns[i] = sites::Required(callers[i], returnOffsets[i]);
    manager = reinterpret_cast<uintptr_t*>(Absolute(managerSite + 8, 0, 12));
    player = reinterpret_cast<uintptr_t*>(Absolute(playerSite + 8, 0, 8));
    scriptHudState = reinterpret_cast<uintptr_t*>(Absolute(scriptHudSite + 8, 0, 4));
    sceneCamera = reinterpret_cast<uintptr_t*>(Absolute(cameraSite, 0, 4) + 0x140);
    setHorizontal = decltype(setHorizontal)(horizontalSite);
    setVertical = decltype(setVertical)(verticalSite);
    prepareHook = safetymips::create_inline(prepareSite, Prepare);
    prepare.fun = prepareHook.original<decltype(prepare.fun)>();
    safetymips::Options scalar;
    scalar.preserve = PSP_HOOK_SAVE_FPU;
    projectionHook = safetymips::create_mid(projectionSite, [](SafetyMipsContext& regs)
    {
        Projection(regs);
    }, scalar);
}
}
