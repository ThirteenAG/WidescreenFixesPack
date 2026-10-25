module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module FrameRate;

import ComVars;

// Above 30 frames per second the game shows its frames between the 30 Hz physics ticks. A few things weren't made
// for that, and are fixed here, together with the frame rate unlock and limit.

namespace
{
    int nFPSLimit = 0;

    bool AboveThirtyFps()
    {
        float dt = fb::GameTimer::frameDeltaTime;
        return dt > 0.0f && dt < 1.0f / 32.0f;
    }

    // Snow spray, drift smoke and dirt dust inherit part of the speed of what emits them. Above 30 frames per second
    // they get too much of it and fill the screen (issue #2063), so the share is scaled to the frame time.
    void ScaleInheritedSpeed(SafetyHookContext& regs)
    {
        float scale = std::clamp(fb::GameTimer::frameDeltaTime * 30.0f, 0.0f, 1.0f);
        if (scale > 0.0f)
            regs.xmm0.f32[0] *= scale;
    }

    // Rotation matrix (rows right, up, forward) to a quaternion and back
    void ToQuaternion(const float* m, float* q)
    {
        float trace = m[0] + m[5] + m[10];
        if (trace > 0.0f)
        {
            float s = std::sqrt(trace + 1.0f) * 2.0f;
            q[3] = 0.25f * s; q[0] = (m[6] - m[9]) / s; q[1] = (m[8] - m[2]) / s; q[2] = (m[1] - m[4]) / s;
        }
        else if (m[0] > m[5] && m[0] > m[10])
        {
            float s = std::sqrt(1.0f + m[0] - m[5] - m[10]) * 2.0f;
            q[3] = (m[6] - m[9]) / s; q[0] = 0.25f * s; q[1] = (m[4] + m[1]) / s; q[2] = (m[8] + m[2]) / s;
        }
        else if (m[5] > m[10])
        {
            float s = std::sqrt(1.0f + m[5] - m[0] - m[10]) * 2.0f;
            q[3] = (m[8] - m[2]) / s; q[0] = (m[4] + m[1]) / s; q[1] = 0.25f * s; q[2] = (m[9] + m[6]) / s;
        }
        else
        {
            float s = std::sqrt(1.0f + m[10] - m[0] - m[5]) * 2.0f;
            q[3] = (m[1] - m[4]) / s; q[0] = (m[8] + m[2]) / s; q[1] = (m[9] + m[6]) / s; q[2] = 0.25f * s;
        }
    }

    void FromQuaternion(const float* q, float* m)
    {
        float x = q[0], y = q[1], z = q[2], w = q[3];
        m[0] = 1 - 2 * (y * y + z * z); m[1] = 2 * (x * y + z * w);     m[2] = 2 * (x * z - y * w);
        m[4] = 2 * (x * y - z * w);     m[5] = 1 - 2 * (x * x + z * z); m[6] = 2 * (y * z + x * w);
        m[8] = 2 * (x * z + y * w);     m[9] = 2 * (y * z - x * w);     m[10] = 1 - 2 * (x * x + y * y);
    }

    // A transform between two others: positions in a line, rotations along the shorter way
    void BlendTransform(const float* a, const float* b, float f, float* out)
    {
        float qa[4], qb[4], q[4];
        ToQuaternion(a, qa);
        ToQuaternion(b, qb);
        float sign = (qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3]) < 0.0f ? -1.0f : 1.0f;
        float length = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            q[i] = qa[i] * sign + (qb[i] - qa[i] * sign) * f;
            length += q[i] * q[i];
        }
        length = std::sqrt(length);
        for (int i = 0; i < 4; ++i)
            q[i] /= length;
        FromQuaternion(q, out);
        out[3] = out[7] = out[11] = 0.0f;
        for (int i = 12; i < 15; ++i)
            out[i] = a[i] + (b[i] - a[i]) * f;
        out[15] = 1.0f;
    }

    // Where a transform moving from a through b in a line ends up, k times as far: positions in a line, rotations
    // along the same arc
    void ExtrapolateTransform(const float* a, const float* b, float k, float* out)
    {
        float qa[4], qb[4], q[4];
        ToQuaternion(a, qa);
        ToQuaternion(b, qb);
        float cosine = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3];
        if (cosine < 0.0f)
        {
            cosine = -cosine;
            for (auto& v : qb)
                v = -v;
        }
        float angle = std::acos(std::min(cosine, 1.0f));
        if (angle < 0.0001f)
            memcpy(q, qb, sizeof(q));
        else
        {
            float total = std::min(angle * k, 3.14159265f);
            float wa = std::sin(angle - total) / std::sin(angle), wb = std::sin(total) / std::sin(angle);
            for (int i = 0; i < 4; ++i)
                q[i] = qa[i] * wa + qb[i] * wb;
        }
        FromQuaternion(q, out);
        out[3] = out[7] = out[11] = 0.0f;
        for (int i = 12; i < 15; ++i)
            out[i] = a[i] + (b[i] - a[i]) * k;
        out[15] = 1.0f;
    }
}

class FrameRate
{
public:
    FrameRate()
    {
        WFP::onInitEvent() += []()
        {
            static bool bUnlockFrameRate = FixSettings().Int("FRAMERATE.UnlockFrameRate") != 0;
            static bool bUnlockCutscenes = FixSettings().Int("FRAMERATE.UnlockCutscenes") != 0;

            // The frame rate limit; 0 gives the game its own value back
            WFP::onGameProcessEvent() += []()
            {
                static float gameMaxFps = -1.0f;
                nFPSLimit = FixSettings().Int("MAIN.FPSLimit");
                if (nFPSLimit <= 0 && gameMaxFps < 0.0f)
                    return;
                if (auto maxFps = fb::Settings::Field<float>("GameTimeSettings", "MaxVariableFps"))
                {
                    if (nFPSLimit > 0)
                    {
                        if (gameMaxFps < 0.0f)
                            gameMaxFps = *maxFps;
                        *maxFps = (float)std::max(nFPSLimit, 30);
                    }
                    else
                    {
                        *maxFps = gameMaxFps;
                        gameMaxFps = -1.0f;
                    }
                }
            };

            // The game forces VSync on in load screens and in some states of a race, which also turns off the frames
            // between the physics ticks, so the game stays at 30 frames per second even with VSync off in its options.
            // Load screens get no VSync, everything else the player's VSync setting. Only in 1.1, 1.0 has no such code.
            if (bUnlockFrameRate)
            {
                auto pattern = hook::pattern("C6 05 ? ? ? ? 01 C6 05 ? ? ? ? 01 E9 ? ? ? ? C6 44 24 ? 01");
                if (!pattern.empty())
                {
                    static auto vsync = *pattern.get_first<uint8_t*>(2);
                    static auto reloadSetting = *pattern.get_first<uint8_t*>(9);
                    static auto ApplyVSyncHook = safetyhook::create_mid(injector::GetBranchDestination(pattern.get_first(14)).as_int(), [](SafetyHookContext& regs)
                    {
                        auto inLoadScreen = fb::Settings::Field<bool>("ClientSettings", "InLoadScreen");
                        if (inLoadScreen && *inLoadScreen)
                        {
                            *vsync = 0;
                            *reloadSetting = 1;
                        }
                        // Cutscenes and quick-time events keep the game's own handling, 30 frames per second without
                        // frames between the ticks, unless they're unlocked as well
                        else if ((bUnlockCutscenes || !fb::Vehicle::InRaceWithoutControl()) && fb::g_profileOptionsManager.get() && fb::ProfileOptions::getInt)
                        {
                            // The player's own VSync setting instead of the forced one
                            *vsync = fb::ProfileOptions::getInt(fb::g_profileOptionsManager + 0x14, "VSyncEnabled", 1) != 0;
                        }
                    });
                }
            }

            // The engine sound's pitch glides to each new value. Above 30 frames per second a new value comes before
            // the glide moves, the glide starts over and the pitch stays where it was, so the engine sounds stuck at one
            // tone. Then the glide carries on to the new value instead of starting over.
            auto pattern = hook::pattern("D9 83 C8 01 00 00 F3 0F 11 8B CC 01 00 00 D9 9B C0 01 00 00");
            if (!pattern.empty())
            {
                static auto skip = (uintptr_t)pattern.get_first(14);
                static auto PitchGlideHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    if (AboveThirtyFps())
                        regs.eip = skip;
                });
            }

            // On the frames between the ticks the cutscene cameras play their animation on to the frame's time, which
            // is up to a tick ahead of everything else, drawn between the last two ticks. Objects in the shot shake
            // against the view, and around a cut the view slides toward the next shot or gets its zoom early. Outside
            // the player's control the camera and its field of view are put between their last two tick values too.
            // The tick before a cut often has the camera a little way toward the next shot already, while everything
            // else is in the next shot. In that tick the camera's own animation moves on in a line and reaches the next
            // shot at the next tick, so after a sudden jump the camera is put where that line ends, and the next tick
            // cuts.
            auto frameUpdate = hook::pattern("E8 ? ? ? ? 8D 77 10 C6 44 24 2C 00 0F 28 46 30");                 // ClientCameraManager::update, after the camera's own update
            auto afterUpdate = hook::pattern("8B 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? D9 43 04 51 D9 1C 24");        // after ClientCameraManager::update
            if (!frameUpdate.empty() && !afterUpdate.empty() && bUnlockFrameRate && bUnlockCutscenes)
            {
                struct CameraState
                {
                    float previousFov = 0.0f, currentFov = 0.0f;    // the field of view at the last two ticks, from their first frames
                    bool bTickFrame = false;                        // the first frame after a tick
                    float alpha = 1.0f;                             // the frame's place between the ticks
                    bool bBlend = false;                            // this frame's camera is put between the ticks
                    bool bCut = false, bAhead = false;              // a cut now, or one coming at the next tick
                    bool bFirstFrame = false, bFirstFov = false;    // the first frame of a tick with a cut coming
                    float firstAlpha = 0.0f, first[16] = {}, firstFov = 0.0f;   // the camera's own values on that frame
                    float ahead = 0.0f;                             // how much farther the camera's line goes from there
                };
                static CameraState state;

                static auto CameraFrameHook = safetyhook::create_mid(frameUpdate.get_first(5), [](SafetyHookContext& regs)
                {
                    state.bBlend = false;
                    auto manager = regs.ebx;
                    auto camera = regs.edi;
                    float alpha = *(float*)(regs.ebp + 0x10);
                    bool driving = fb::Vehicle::bHasControl && GetTickCount64() - fb::Vehicle::LastUpdate < 500;
                    if (!manager || !camera || driving || !(alpha >= 0.0f && alpha <= 1.0f))
                        return;
                    auto current = (const float*)(manager + 0xEF0);    // the camera at the last two ticks: right, up, forward, position
                    auto previous = (const float*)(manager + 0xF30);
                    auto Length = [](const float* v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };
                    if (std::fabs(Length(current + 8) - 1.0f) > 0.01f || std::fabs(Length(previous + 8) - 1.0f) > 0.01f)
                        return;

                    // Once a tick: another camera, or the same one moving or turning much more than in the tick before
                    static uintptr_t lastCamera = 0;
                    static float lastCurrent[16] = {}, lastPrevious[16] = {};
                    static float lastMove = 0.0f, lastTurn = 0.0f;
                    if (memcmp(lastCurrent, current, sizeof(lastCurrent)) != 0 || memcmp(lastPrevious, previous, sizeof(lastPrevious)) != 0)
                    {
                        memcpy(lastCurrent, current, sizeof(lastCurrent));
                        memcpy(lastPrevious, previous, sizeof(lastPrevious));
                        float dx = current[12] - previous[12], dy = current[13] - previous[13], dz = current[14] - previous[14];
                        float move = std::sqrt(dx * dx + dy * dy + dz * dz);
                        float turn = std::acos(std::clamp(current[8] * previous[8] + current[9] * previous[9] + current[10] * previous[10], -1.0f, 1.0f));
                        state.bTickFrame = true;
                        bool switched = camera != lastCamera;
                        bool jump = move > std::max(0.15f, lastMove * 4.0f) || turn > std::max(0.07f, lastTurn * 4.0f);  // 4 degrees
                        if (switched || state.bAhead)
                        {
                            state.bAhead = false;
                            state.bCut = true;
                        }
                        else if (jump)
                        {
                            state.bAhead = true;
                            state.bFirstFrame = true;
                            state.bFirstFov = false;
                            state.bCut = false;
                        }
                        else
                            state.bCut = false;
                        lastCamera = camera;
                        lastMove = move;
                        lastTurn = turn;
                    }

                    state.alpha = alpha;
                    state.bBlend = true;
                    auto out = (float*)(camera + 0x10);
                    if (state.bAhead)
                    {
                        // On the first frame of the tick the shot before stays, after that the camera is where its line ends
                        state.ahead = 0.0f;
                        if (state.bFirstFrame)
                        {
                            state.bFirstFrame = false;
                            state.bFirstFov = true;
                            state.firstAlpha = alpha;
                            memcpy(state.first, out, sizeof(state.first));
                            memcpy(out, previous, 0x40);
                        }
                        else if (alpha - state.firstAlpha > 0.02f)
                        {
                            state.ahead = (1.0f - state.firstAlpha) / (alpha - state.firstAlpha);
                            ExtrapolateTransform(state.first, out, state.ahead, out);
                        }
                        else
                            memcpy(out, previous, 0x40);
                    }
                    else if (state.bCut)
                        memcpy(out, current, 0x40);
                    else
                        BlendTransform(previous, current, alpha, out);
                });

                // The field of view, with the cutscene's own on top, is in the view at the end of the update
                static auto pManager = *afterUpdate.get_first<uintptr_t*>(2);
                static auto CameraFovHook = safetyhook::create_mid(afterUpdate.get_first(14), [](SafetyHookContext& regs)
                {
                    auto manager = *pManager;
                    if (!state.bBlend || !manager)
                        return;
                    auto view = manager + 0xA50;
                    auto fov = (float*)(view + 0x48);       // radians
                    float own = *fov;
                    if (state.bTickFrame)
                    {
                        state.bTickFrame = false;
                        state.previousFov = state.currentFov;
                        state.currentFov = own;
                    }
                    if (state.previousFov <= 0.0f)
                        return;

                    // The cutscenes change the lens' focal length evenly, not the angle
                    auto Focal = [](float fov) { return 1.0f / std::tan(fov * 0.5f); };
                    auto Angle = [](float focal) { return 2.0f * std::atan(1.0f / focal); };
                    float value = own;
                    if (state.bAhead)
                    {
                        if (state.bFirstFov)
                        {
                            state.bFirstFov = false;
                            state.firstFov = own;
                            value = state.previousFov;
                        }
                        else if (state.ahead > 0.0f)
                        {
                            float focal = Focal(state.firstFov) + (Focal(own) - Focal(state.firstFov)) * state.ahead;
                            value = focal > 0.0f ? Angle(focal) : own;
                        }
                        else
                            value = state.previousFov;
                    }
                    else if (state.bCut)
                        value = state.currentFov;
                    else
                        value = state.previousFov + (state.currentFov - state.previousFov) * state.alpha;
                    if (value != own && value > 0.0f && value < 3.14f)
                    {
                        *fov = value;
                        *(uint32_t*)(view + 0xA0) |= 0xD;   // the projection needs updating
                    }
                });
            }

            pattern = hook::pattern("80 7E 39 00 F3 0F 10 46 34 0F C6 C0 00");
            if (!pattern.empty())
            {
                static auto InheritSpeedHook1 = safetyhook::create_mid(pattern.get_first(9), ScaleInheritedSpeed);
            }

            pattern = hook::pattern("80 78 39 00 F3 0F 10 40 34 0F C6 C0 00");
            if (!pattern.empty())
            {
                static auto InheritSpeedHook2 = safetyhook::create_mid(pattern.get_first(9), ScaleInheritedSpeed);
            }
        };
    }
} FrameRate;
