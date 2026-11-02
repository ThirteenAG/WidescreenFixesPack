#include "Game.hpp"
#include <cmath>

namespace tcny
{
    namespace
    {
        SafetyMipsMid barsBeginHook, barsHeightHook, barsEndHook, projectionHook;
        // Draws one untextured sprite: (context, x0, y0, x1, y1, color, z), GS
        // window coordinates in 1/16 pixel with the 2048 offset.
        pcsx2::GameFunction<void(uintptr_t, int32_t, int32_t, int32_t, int32_t, uint32_t, uint32_t)> drawSprite;
        // The front end draws the same bars over its backgrounds; the settings
        // apply to cutscenes only.
        // Cinematic bars object: +0x14 state (0 shown, 1 sliding in, 2 sliding
        // out), +0x18 height of each bar in pixels.
        constexpr uintptr_t barState = 0x14, barHeight = 0x18;
        float fullHeight = 68.0f;
        uint32_t projectionReturn;

        void BarsBegin(SafetyMipsContext& regs)
        {
            if (animatedBorders || !Cinema())
            {
                return;
            }
            const auto bars = uintptr_t(regs.s1);
            const auto state = Read<int32_t>(bars + barState);
            const auto height = Read<float>(bars + barHeight);
            if (state == 1 || (state == 0 && height > fullHeight))
            {
                Write<float>(bars + barHeight, fullHeight);
            }
            else if (state == 2)
            {
                Write<float>(bars + barHeight, 0.0f);
            }
        }
        // f0 = bar height about to be converted for drawing.
        void BarsHeight(SafetyMipsContext& regs)
        {
            if (!(cutsceneBorders & 1) && Cinema())
            {
                regs.f0 = 0.0f;
            }
        }
        // Both bars are drawn; sp = the sprite context, s0 = screen height,
        // sp + 0x34 = screen width.
        void BarsEnd(SafetyMipsContext& regs)
        {
            if (!(cutsceneBorders & 2) || display.scale >= 1.0f || !Cinema())
            {
                return;
            }
            const auto bars = uintptr_t(regs.s1);
            const float progress = std::clamp(Read<float>(bars + barHeight) / fullHeight, 0.0f, 1.0f);
            const float width = float(Read<int16_t>(uintptr_t(regs.sp) + 0x34));
            const float height = float(int16_t(regs.s0));
            const float side = width * (1 - display.scale) * 0.5f * progress;
            if (side < 0.5f || width <= 0 || height <= 0)
            {
                return;
            }
            auto gs = [](float value) { return int32_t((value + 2048.0f) * 16.0f); };
            const auto context = uintptr_t(regs.sp);
            drawSprite(context, gs(0), gs(0), gs(side), gs(height), 0x80000000u, 0x00FFFFFFu);
            drawSprite(context, gs(width - side), gs(0), gs(width), gs(height), 0x80000000u, 0x00FFFFFFu);
        }

        // World projection (screen, f12 = camera zoom): the screen's vertical
        // scale is at +0x10. The FOV factor widens the view angle in gameplay.
        void Projection(SafetyMipsContext& regs)
        {
            if (fovFactor == 1.0f || uint32_t(regs.ra) != projectionReturn || Cinema())
            {
                return;
            }
            const auto camera = uintptr_t(regs.s0);
            const float native = regs.f12, vertical = Read<float>(uintptr_t(regs.a0) + 0x10);
            if (!camera || Read<uint32_t>(camera) != gameCamera || !(native > 0) || !(vertical > 0))
            {
                return;
            }
            constexpr float pi = 3.14159265f;
            const float half = std::atan(1.0f / (native * vertical));
            const float adjusted = std::clamp(half * fovFactor, pi / 360.0f, pi * 179.0f / 360.0f);
            const float zoom = 1.0f / (std::tan(adjusted) * vertical);
            if (std::isfinite(zoom) && zoom > 0)
            {
                regs.f12 = zoom;
            }
        }
    } // namespace

    void InstallCutscene(uintptr_t barsBegin, uintptr_t barsHeight, uintptr_t barsEnd, uintptr_t barsReset)
    {
        // lui at, height (high half of the float) in the bars reset.
        fullHeight = [](uint32_t bits) { float value; std::memcpy(&value, &bits, sizeof(value)); return value; }(
            uint32_t(Read<uint16_t>(barsReset + 4)) << 16);
        drawSprite.bind(injector::GetBranchDestination(barsEnd + 8).as_int());
        barsBeginHook = safetymips::create_mid<&BarsBegin>(barsBegin + 0x18);
        barsHeightHook = safetymips::create_mid<&BarsHeight>(barsHeight + 0x14);
        barsEndHook = safetymips::create_mid<&BarsEnd>(barsEnd + 0x10);
    }

    void InstallCamera(uintptr_t projectionCall)
    {
        projectionReturn = uint32_t(projectionCall + 0x1C);
        const auto projection = injector::GetBranchDestination(projectionCall + 0x14).as_int();
        projectionHook = safetymips::create_mid<&Projection>(projection);
    }
} // namespace tcny
