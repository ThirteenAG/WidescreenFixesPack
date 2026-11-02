#pragma once

namespace Cinematics
{
inline int borders{ 3 };
inline bool active;
inline uintptr_t multiplexor;
inline injector::hook_back<void()> present;
inline injector::hook_back<int(uintptr_t, float, float, float, float)> cull;
inline void (*canvas)();
inline unsigned barFrames;

inline bool Framed(float aspect)
{
    constexpr float native = 480.0f / 272.0f;
    return active && (borders & (aspect < native ? 1 : 2));
}
inline int Cull(uintptr_t object, float halfAngle, float aspect, float nearPlane, float farPlane)
{
    // This independent visibility frustum is built from the native aspect,
    // rather than from the camera projection used to draw the world.
    return cull.fun(object, halfAngle, std::max(aspect, Hud::AspectRatio()), nearPlane, farPlane);
}
inline void Camera(SafetyMipsContext& regs)
{
    const auto object = uintptr_t(regs.s0);
    const bool main = *reinterpret_cast<const uintptr_t*>(object + 204) == multiplexor;
    if (main)
    {
        // Scripted mocap playback registers camera 3 in the native multiplexor.
        const int current = *reinterpret_cast<const int*>(object + 240);
        const int next = *reinterpret_cast<const int*>(object + 244);
        active = current == 3 || next == 3;
    }
    const float aspect = forcedAspect > 0.0f ? forcedAspect : Aspect();
    constexpr float native = 480.0f / 272.0f;
    if (main)
        Hud::cinematicFrame = Framed(aspect);
    if (main && Framed(aspect) && aspect < native)
    {
        constexpr float radians = 0.017453292519943295f;
        regs.f12 = std::atan(std::tan(regs.f12 * radians) * native / aspect) / radians;
    }
    regs.f13 = aspect;
}
inline void Present()
{
    if (Framed(Hud::AspectRatio()))
    {
        // Scripted scenes bypass the ordinary HUD observer branch. Finish the
        // main render list on an explicit native canvas in either branch.
        canvas();
        const auto fit = Hud::Fit();
        const int x = int(std::ceil(240 * (1.0f - fit.x)));
        const int y = int(std::ceil(136 * (1.0f - fit.y)));
        Hud::submit.fun(Hud::raw);
        Hud::scissor.fun(0, 0, 480, 272);
        if (x > 0)
        {
            Hud::fill.fun(0, 0, x, 272, 0xFF000000);
            Hud::fill.fun(480 - x, 0, x, 272, 0xFF000000);
        }
        if (y > 0)
        {
            Hud::fill.fun(0, 0, 480, y, 0xFF000000);
            Hud::fill.fun(0, 272 - y, 480, y, 0xFF000000);
        }
        ++barFrames;
    }
    present.fun();
}
inline void ReadOptions()
{
    borders = std::clamp(inireader.ReadInteger("MAIN", "CutsceneBorders", 3), 0, 3);
}
inline void Install()
{
    const auto type = Hud::Required("? ? 04 3C 00 60 80 44 ? ? 84 24 FF FF 05 24 CC 00 04 AE F0 00 05 AE F4 00 05 AE F8 00 0C E6");
    multiplexor = Absolute(type, 0, 8);
    const auto site = Hud::Required("2C 02 B4 C7 25 20 40 00 ? ? ? ? 06 A3 00 46 ? ? ? ? 00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 24 02 AC C7", 16);
    const auto canvasSite = Hud::Required("B0 FF BD 27 ? ? ? ? 00 60 80 44 25 20 A0 03 40 00 BF AF ? ? ? ? 00 68 85 44 ? ? ? ?");
    const auto cullSite = Hud::Required("C4 00 6E C6 06 A3 00 46 C8 00 6F C6 46 B3 00 46 ? ? ? ? 25 20 A0 02 ? ? ? ? 25 20 20 02 ? ? ? ? 06 F3 00 46", 16);
    canvas = reinterpret_cast<decltype(canvas)>(canvasSite);
    present.fun = injector::MakeCALL(site, Present).get();
    cull.fun = injector::MakeCALL(cullSite, Cull).get();
}
}
