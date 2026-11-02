#pragma once

namespace HudText
{
inline SafetyMipsInline leftHook, rightHook, topHook, bottomHook;
inline SafetyMipsMid fitHook;
inline injector::hook_back<float(uintptr_t)> left, right, top, bottom;

enum class Side { None, Left, Right, Center, Top, LeftTop, RightMiddle, Middle, WorldMarker, WheelMiddle, WheelTop, WheelBottom };
inline bool WindowHasFont(uintptr_t window, uintptr_t font)
{
    if (!font || !*reinterpret_cast<const uintptr_t*>(window)) return false;
    // These windows allocate their text fonts as messages arrive. The native
    // text manager owns a circular list of nodes, each holding a font at +8.
    const auto head = window + 24;
    for (auto node = *reinterpret_cast<const uintptr_t*>(head); node != head;
         node = *reinterpret_cast<const uintptr_t*>(node))
        if (font == *reinterpret_cast<const uintptr_t*>(node + 8)) return true;
    return false;
}
inline Side FontSide(uintptr_t context)
{
    const auto font = *reinterpret_cast<const uintptr_t*>(context);
    // Frontend text keeps its native layout; Fit() places it with the menus.
    if (!*reinterpret_cast<const uintptr_t*>(Hud::manager)) return Side::None;
    if (WindowHasFont(Hud::manager + 80, font)) return Side::LeftTop;
    if (WindowHasFont(Hud::manager + 5792, font)) return Side::Left;
    const auto tcm = Hud::manager + 3032;
    for (unsigned i = 0; font && i < 20; ++i)
        if (font == *reinterpret_cast<const uintptr_t*>(tcm + 44 + i * 28)) return Side::WheelMiddle;
    if (font && font == *reinterpret_cast<const uintptr_t*>(tcm + 584)) return Side::WheelTop;
    if (font && font == *reinterpret_cast<const uintptr_t*>(tcm + 588)) return Side::WheelBottom;
    const auto pause = Hud::manager + 2672;
    for (unsigned i = 0; i < 6; ++i)
        if (font && font == *reinterpret_cast<const uintptr_t*>(pause + 24 + i * 16)) return Side::Center;
    const auto scope = Hud::manager + 2928;
    if (font && (font == *reinterpret_cast<const uintptr_t*>(scope + 96) ||
                 font == *reinterpret_cast<const uintptr_t*>(scope + 100))) return Side::Center;
    const auto map = Hud::manager + 4944;
    for (unsigned i = 0; font && i < 10; ++i)
        if (font == *reinterpret_cast<const uintptr_t*>(map + 176 + i * 4)) return Side::Center;
    for (unsigned i = 0; font && i < 24; ++i)
        if (font == *reinterpret_cast<const uintptr_t*>(map + 224 + i * 4)) return Side::Center;
    for (unsigned i = 0; font && i < 3; ++i)
        if (font == *reinterpret_cast<const uintptr_t*>(map + 592 + i * 4)) return Side::Center;
    const auto actions = Hud::manager + 1776;
    for (unsigned i = 0; font && i < 5; ++i)
        if (font == *reinterpret_cast<const uintptr_t*>(actions + 24 + i * 4)) return Side::Top;
    const auto actionMarker = Hud::manager + 4576;
    if (font && font == *reinterpret_cast<const uintptr_t*>(actionMarker + 40)) return Side::WorldMarker;
    const auto targeting = Hud::manager + 6760;
    if (font && font == *reinterpret_cast<const uintptr_t*>(targeting + 20)) return Side::Middle;
    const auto timers = Hud::manager + 1464;
    if (font && (font == *reinterpret_cast<const uintptr_t*>(timers + 28) ||
                 font == *reinterpret_cast<const uintptr_t*>(timers + 44))) return Side::LeftTop;
    const auto notification = Hud::manager + 1524;
    if (font && font == *reinterpret_cast<const uintptr_t*>(notification + 20)) return Side::RightMiddle;
    const auto pspIcons = Hud::manager + 4912;
    if (font && font == *reinterpret_cast<const uintptr_t*>(pspIcons + 24)) return Side::RightMiddle;
    const auto weapon = Hud::manager + 932;
    if (font == *reinterpret_cast<const uintptr_t*>(weapon + 44) ||
        font == *reinterpret_cast<const uintptr_t*>(weapon + 48)) return Side::Left;
    const auto selection = Hud::manager + 400;
    for (unsigned i = 0; font && i < 4; ++i)
        if (font == *reinterpret_cast<const uintptr_t*>(selection + 116 + i * 4)) return Side::Left;
    const auto team = Hud::manager + 576;
    if (font == *reinterpret_cast<const uintptr_t*>(team + 12)) return Side::Right;
    for (unsigned i = 0; i < 4; ++i)
        if (font == *reinterpret_cast<const uintptr_t*>(team + 132 + i * 44)) return Side::Right;
    return Side::None;
}
// Text positions are in native canvas pixels; Fit() later maps the canvas
// into the screen like the projection does for the HUD artwork.
inline float X(uintptr_t context, float value)
{
    const auto side = FontSide(context);
    if (side == Side::None || side == Side::Center || side == Side::Middle) return value;
    if (side == Side::WorldMarker)
    {
        const auto node = *reinterpret_cast<const uintptr_t*>(Hud::manager + 4576);
        if (!node) return value;
        const float anchor = 240.0f + *reinterpret_cast<const float*>(node + 64) * Hud::pixels;
        return anchor + (value - anchor) * Hud::scale;
    }
    if (side == Side::Top) return 240.0f + (value - 240.0f) * Hud::scale;
    if (side == Side::WheelMiddle || side == Side::WheelTop || side == Side::WheelBottom)
        return 240.0f + (value - 240.0f) * std::min(Hud::scale, 1.2f);
    const float native = side == Side::Left || side == Side::LeftTop ? 0.0f : 480.0f;
    const float edge = 240.0f + (native - 240.0f) * Hud::wide * Hud::edgeRatio;
    return edge + (value - native) * Hud::scale;
}
inline float Y(uintptr_t context, float value)
{
    const auto side = FontSide(context);
    if (side == Side::None || side == Side::Center || side == Side::Middle) return value;
    if (side == Side::WorldMarker)
    {
        const auto node = *reinterpret_cast<const uintptr_t*>(Hud::manager + 4576);
        if (!node) return value;
        const float anchor = 136.0f - *reinterpret_cast<const float*>(node + 68) * Hud::pixels;
        return anchor + (value - anchor) * Hud::scale;
    }
    if (side == Side::RightMiddle) return 136.0f + (value - 136.0f) * Hud::scale;
    if (side == Side::WheelMiddle) return 136.0f + (value - 136.0f) * std::min(Hud::scale, 1.2f);
    if (side == Side::WheelTop) return value * std::min(Hud::scale, 1.2f);
    if (side == Side::WheelBottom) return 272.0f + (value - 272.0f) * std::min(Hud::scale, 1.2f);
    const float top = 136.0f - 136.0f * Hud::tall;
    if (side == Side::Top || side == Side::LeftTop) return top + value * Hud::scale;
    return 272.0f - top + (value - 272.0f) * Hud::scale;
}
inline float Left(uintptr_t context) { return X(context, left.fun(context)); }
inline float Right(uintptr_t context) { return X(context, right.fun(context)); }
inline float Top(uintptr_t context) { return Y(context, top.fun(context)); }
inline float Bottom(uintptr_t context) { return Y(context, bottom.fun(context)); }
// Screen-space text corners after the text's own 2D transform, before the
// game builds its sprite. Fit them like the projection fits the menus and HUD.
inline void Fit(SafetyMipsContext& regs)
{
    constexpr float native = 480.0f / 272.0f;
    const float aspect = Geometry::AspectRatio();
    const float sx = std::min(native / aspect, 1.0f);
    const float sy = std::min(aspect / native, 1.0f);
    auto corners = reinterpret_cast<float*>(uintptr_t(regs.sp) + 0x30);
    for (unsigned i = 0; i < 6; i += 3)
    {
        corners[i] = 240.0f + (corners[i] - 240.0f) * sx;
        corners[i + 1] = 136.0f + (corners[i + 1] - 136.0f) * sy;
    }
}
inline void Install()
{
    const auto l = sites::Required("C0 FF BD 27 1C 00 85 8C 20 00 86 90 08 00 A7 84 28 00 B4 E7 00 00 85 8C 00 A0 87 44");
    const auto r = sites::Required("C0 FF BD 27 2C 00 B0 AF 25 80 80 00 1C 00 04 8E 02 00 85 84 00 68 85 44 0C 00 8C C4");
    const auto t = sites::Required("C0 FF BD 27 2C 00 B0 AF 25 80 80 00 10 00 05 8E 28 00 B4 E7 30 00 BF AF 03 00 A0 14");
    const auto b = sites::Required("C0 FF BD 27 2C 00 B0 AF 10 00 90 8C 28 00 B4 E7 30 00 BF AF 03 00 00 16 00 00 85 8C");
    leftHook = safetymips::create_inline(l, Left); left.fun = leftHook.original<decltype(left.fun)>();
    rightHook = safetymips::create_inline(r, Right); right.fun = rightHook.original<decltype(right.fun)>();
    topHook = safetymips::create_inline(t, Top); top.fun = topHook.original<decltype(top.fun)>();
    bottomHook = safetymips::create_inline(b, Bottom); bottom.fun = bottomHook.original<decltype(bottom.fun)>();
    safetymips::Options scalar;
    scalar.preserve = PSP_HOOK_SAVE_FPU;
    const auto corners = sites::Required("30 00 AC C7 34 00 AD C7 10 00 AC E7 3C 00 AC C7 14 00 AD E7 40 00 AD C7 18 00 AC E7");
    fitHook = safetymips::create_mid(corners, [](SafetyMipsContext& regs) { Fit(regs); }, scalar);
}
}
