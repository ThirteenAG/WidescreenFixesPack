#pragma once
#include "Hud.hpp"

// The game world is drawn through one camera wrapper (0x45BFF0, NiCamera at
// +8). Its frustum is set up for the screen by the projection hooks (Hor+).
namespace kr::camera
{
    inline uintptr_t Main()
    {
        const auto wrapper = Read<uintptr_t>(0x45BFF0);
        return wrapper ? Read<uintptr_t>(wrapper + 8) : 0;
    }

    // Keep the projection written by the game and the display aspect it used.
    // The game normally writes it only on a camera mode change.
    inline float base[4], written[4], frustum[6];
    inline float baseAspect;
    inline uintptr_t baseCamera;
    inline pcsx2::GameFunction<void(uintptr_t)> markCameraProjectionDirty;

    // 0x3233BC in NiCamera::Click (see hud::CameraData). Returns false for
    // cameras other than the main camera.
    inline bool CameraData(SafetyMipsContext& regs)
    {
        const auto camera = uintptr_t(regs.s2);
        if (!camera || camera != Main())
        {
            return false;
        }
        auto stored = reinterpret_cast<float*>(camera + 0x154);
        const float aspect = Aspect();
        if (camera != baseCamera || std::memcmp(stored, written, sizeof(written)))
        {
            std::copy(stored, stored + 4, base);
            baseAspect = aspect;
            baseCamera = camera;
        }
        float zoom = 1;
        const float half = 0.5f * (base[2] - base[3]);
        if (fovFactor != 1 && !Cinematic() && half > 0)
        {
            const float angle = std::clamp(std::atan(half) * fovFactor, 0.02f, 1.48f);
            zoom = std::tan(angle) / half;
        }
        const float xScale = zoom * (aspect / baseAspect);
        written[0] = base[0] * xScale;
        written[1] = base[1] * xScale;
        written[2] = base[2] * zoom;
        written[3] = base[3] * zoom;
        if (std::memcmp(stored, written, sizeof(written)))
        {
            // The renderer and culling both consume the camera's projection.
            std::copy(written, written + 4, stored);
            if (markCameraProjectionDirty)
            {
                markCameraProjectionDirty(camera);
            }
        }
        // The cockpit dashboard covers only the 4:3 frame, and scripted scenes
        // with Pillarbox keep their 4:3 framing: draw the world in the 4:3
        // frame and black at the sides.
        const float k = Fit();
        const auto renderer = Read<uintptr_t>(camera + 0x1B0);
        const bool cockpit = Cockpit();
        if (k < 1 && renderer && (cockpit || (Cinematic() && (cutsceneBorders & 2))))
        {
            // The dashboard mesh ends a little inside its 4:3 frame: keep one
            // more pixel black at each side there.
            const float width = float(Read<int32_t>(renderer + 0x828));
            const float part = cockpit && width > 0 ? k - 2.0f / width : k;
            std::copy(stored, stored + 6, frustum);
            const float center = 0.5f * (stored[0] + stored[1]);
            frustum[0] = center + (stored[0] - center) * part;
            frustum[1] = center + (stored[1] - center) * part;
            regs.t1 = uint32_t(uintptr_t(frustum));
            const auto port = reinterpret_cast<const float*>(camera + 0x16C);
            hud::SetPort(camera, 0.5f - 0.5f * part, 0.5f + 0.5f * part, port[2], port[3]);
            draw::Outside(renderer, part, 1);
        }
        return true;
    }

    inline void SetFovDefaults(SafetyMipsContext& regs)
    {
        const auto bits = Bits(Aspect());
        regs.v0 = bits & 0xFFFF0000;
        regs.v1 = bits;
    }
    inline void ProjectionDefaults(SafetyMipsContext& regs)
    {
        regs.v0 = Bits(Aspect());
    }
    // 0x3EB910 (horizontal FOV in degrees, aspect).
    inline void HorizontalProjection(SafetyMipsContext& regs)
    {
        const float aspect = Aspect();
        regs.f12 = console::horizontal_fov(regs.f12, 4.0f / 3.0f, aspect);
        regs.f13 = aspect;
    }
    // 0x3EBA60: horizontal and vertical angles separately.
    inline void SeparateProjection(SafetyMipsContext& regs)
    {
        // 0x3EBAE0 (from 0x125350) copies the angles that 0x3EBFE0 measured back
        // from the main camera's frustum, which is already widened: keep them.
        if (regs.ra == 0x3EBB34)
        {
            return;
        }
        const float aspect = Aspect();
        // 0x3EB660 passes FOV and FOV / AspectRatio from its settings; the
        // default AspectRatio is the screen aspect (0x3EB864), so restore the
        // vertical angle of the 4:3 default.
        if (regs.ra == 0x3EB6FC && std::fabs(Read<float>(uintptr_t(regs.s3) + 8) - aspect) < 0.001f)
        {
            regs.f13 = regs.f12 * 0.75f;
        }
        regs.f12 = console::horizontal_fov(regs.f12, 4.0f / 3.0f, aspect);
    }
} // namespace kr::camera
