#pragma once

namespace FrameRate
{
inline int limit{ 60 };
inline int appliedLimit{ 30 };
inline bool available;
inline constexpr const char* pacingSignature =
    "? ? 44 8E 25 98 40 00 23 20 64 02 02 00 84 2C 06 00 80 14 00 00 00 00 ? ? ? ? 64 00 04 34 ? ? 44 8E 23 20 64 02 02 00 84 2C "
    "? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 02 00 84 2C "
    "? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 02 00 84 2C";
inline uintptr_t pacing[4];
inline SafetyMipsInline drawHook;
inline SafetyMipsMid textHook, shakeHook;
inline injector::hook_back<int(uintptr_t, uint8_t, float)> draw;
using EffectFunction = int(uintptr_t, uintptr_t, uintptr_t, uintptr_t, float);
inline injector::hook_back<EffectFunction> effects[2];

inline float Retention(float delta)
{
    return std::pow(0.3f, delta * 30.0f);
}
inline int Draw(uintptr_t object, uint8_t flags, float delta)
{
    const float previous = Hud::renderDelta;
    const auto scene = *reinterpret_cast<const uintptr_t*>(object + 8);
    Hud::renderDelta = *reinterpret_cast<const uint8_t*>(scene + 269) ? 0.0f : delta;
    const int result = draw.fun(object, flags, delta);
    Hud::renderDelta = previous;
    return result;
}
template<unsigned Index> int Effect(uintptr_t object, uintptr_t owner, uintptr_t first, uintptr_t second, float)
{
    return effects[Index].fun(object, owner, first, second, Hud::renderDelta);
}
inline bool WritePacing(int value)
{
    const uint32_t instruction = 0x2C840000 | (value == 60 ? 1 : 2);
    for (const auto site : pacing)
        if (injector::WriteMemory<uint32_t>(site, instruction) != PSP_HOOK_OK)
            return false;
    return true;
}
inline bool ApplyLive()
{
    if (!available)
    {
        limit = appliedLimit;
        PatchRejected(PSP_HOOK_INVALID_STATE);
        return false;
    }
    if (!WritePacing(limit) || Finish() != 0)
    {
        const bool restored = WritePacing(appliedLimit);
        const int flushed = Finish();
        limit = appliedLimit;
        PatchRejected(restored && flushed == 0 ? PSP_HOOK_WRITE_FAILED : PSP_HOOK_INVALID_STATE);
        return false;
    }
    appliedLimit = limit;
    return true;
}
inline void ReadOptions()
{
    limit = std::clamp(inireader.ReadInteger("MAIN", "FPSLimit", 60), 30, 60);
}
inline void Install()
{
    const auto first = Hud::Required(pacingSignature, 12);
    const auto drawSite = Hud::Required("70 FD BD 27 6C 02 B1 AF 25 88 80 00 08 00 24 8E 1C 00 84 8C 50 02 B4 E7 78 02 B4 AF 88 02 BE AF 06 65 00 46 FF 00 B4 30");
    const auto textSite = Hud::Required("14 00 0C C6 89 88 A5 34 00 68 85 44 24 00 04 AE 00 63 0D 46 18 00 0E C6 3E 60 0E 46", 12);
    const auto shakeSite = Hud::Required("41 6B 14 46 10 00 0E C6 9A 99 84 34 00 78 84 44 82 73 0F 46 01 65 14 46 10 00 0E E6", 16);
    const auto effectFirst = Hud::Required("08 3D 08 3C 20 00 06 26 89 88 08 35 D8 00 04 8E 00 60 88 44 25 28 00 02 ? ? ? ? 25 38 C0 00 DC 00 04 8E 0B 00 80 10", 24);
    const auto effectSecond = Hud::Required("08 3D 08 3C 20 00 06 26 89 88 08 35 DC 00 04 8E 00 60 88 44 25 28 00 02 ? ? ? ? 25 38 C0 00", 24);
    constexpr unsigned offsets[]{ 0, 28, 56, 96 };
    for (unsigned i = 0; i < 4; ++i)
        pacing[i] = first + offsets[i];
    if (!WritePacing(limit)) return;
    available = true;
    appliedLimit = limit;
    drawHook = safetymips::create_inline(drawSite, Draw);
    draw.fun = drawHook.original<decltype(draw.fun)>();
    safetymips::Options scalar;
    scalar.preserve = PSP_HOOK_SAVE_FPU;
    textHook = safetymips::create_mid(textSite, [](SafetyMipsContext& regs)
    {
        regs.f13 = regs.f20;
    }, scalar);
    shakeHook = safetymips::create_mid(shakeSite, [](SafetyMipsContext& regs)
    {
        regs.f15 = Retention(regs.f20);
    }, scalar);
    effects[0].fun = injector::GetBranchDestination(effectFirst).get();
    effects[1].fun = injector::GetBranchDestination(effectSecond).get();
    injector::MakeCALL(effectFirst, Effect<0>);
    injector::MakeCALL(effectSecond, Effect<1>);
}
}
