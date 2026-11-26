module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <charconv>
#include <chrono>
#include <unordered_map>

export module FrameRate;

import ComVars;
import Camera;

// Above 30 frames per second the game shows its frames between the 30 Hz physics ticks. A few things weren't made
// for that, and are fixed here, together with the frame rate unlock and limit.

namespace
{
    std::atomic<int> RequestedFrameRateLimit{ 0 };

    bool AboveThirtyFps()
    {
        float dt = fb::GameTimer::frameDeltaTime;
        return dt > 0.0f && dt < 1.0f / 32.0f;
    }

    // The terrain texture tree stores its fade age in four bits per tile. Keep the original 30 Hz age units
    // while carrying fractional steps between draws of the same tree.
    struct TerrainFadeClock
    {
        std::chrono::steady_clock::time_point previousDraw{};
        double remainder = 0.0;

        uint32_t Advance(std::chrono::steady_clock::time_point now)
        {
            const auto elapsed = std::chrono::duration<double>(now - previousDraw).count();
            if (previousDraw == std::chrono::steady_clock::time_point{} || elapsed > 0.1)
            {
                previousDraw = now;
                remainder = 0.0;
                return 1;
            }

            previousDraw = now;
            const auto age = remainder + elapsed * 30.0;
            const auto steps = static_cast<uint32_t>(age + 0.000001);
            remainder = std::max(age - steps, 0.0);
            return steps;
        }
    };

    thread_local uint32_t TerrainFadeSteps = 1;

    void AdvanceTerrainFades(uintptr_t tree)
    {
        static std::mutex mutex;
        static std::unordered_map<uintptr_t, TerrainFadeClock> clocks;
        static std::chrono::steady_clock::time_point lastPrune{};
        const std::lock_guard lock(mutex);
        const auto now = std::chrono::steady_clock::now();
        if (now - lastPrune > std::chrono::seconds(2))
        {
            std::erase_if(clocks, [now](const auto& entry)
            {
                return now - entry.second.previousDraw > std::chrono::seconds(2);
            });
            lastPrune = now;
        }
        TerrainFadeSteps = clocks[tree].Advance(now);
    }

    // Snow spray, drift smoke and dirt dust inherit part of the speed of what emits them. The particle update takes
    // that speed from how far the emitter moved since its last position, divided by the frame time. The emitters move
    // on the physics ticks only, so above 30 frames per second a tick's movement is divided by a part of the tick: the
    // particles get several times the car's speed, fly ahead of it and fill the screen (issue #2063). The movement is
    // divided by the time since the emitter moved before instead, which is the frame time at 30 frames per second.
    struct EmitterMotion
    {
        float position[3] = {};
        float sinceMove = 0.0f;     // the time since the position changed
        float interval = 0.0f;      // the time between the last two changes
        uint32_t seenFrame = 0;
    };
    std::unordered_map<uintptr_t, EmitterMotion> EmitterMotions;   // by the emitter's address
    std::mutex EmitterMotionsMutex;                                 // the particles are updated on several threads
    uint32_t EmitterFrame = 0;

    // emitter +0x30: its position, +0xB0: its position before the last move
    float EmitterMoveTime(uintptr_t emitter, float dt)
    {
        auto position = (const float*)(emitter + 0x30);
        std::lock_guard lock(EmitterMotionsMutex);
        auto [it, added] = EmitterMotions.try_emplace(emitter);
        auto& motion = it->second;
        motion.seenFrame = EmitterFrame;
        if (added || memcmp(motion.position, position, sizeof(motion.position)) != 0)
        {
            memcpy(motion.position, position, sizeof(motion.position));
            motion.interval = added ? dt : std::min(motion.sinceMove + dt, 0.1f);
            motion.sinceMove = 0.0f;
        }
        else
            motion.sinceMove += dt;
        return std::max(motion.interval, dt);
    }

    void PruneEmitterMotions()
    {
        std::lock_guard lock(EmitterMotionsMutex);
        if (++EmitterFrame % 256 != 0)
            return;
        std::erase_if(EmitterMotions, [](const auto& entry) { return EmitterFrame - entry.second.seenFrame > 256; });
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
}

export void ApplyFrameRateLimit(bool loading)
{
    static std::optional<float> originalLimit;
    static int previousLimit = 0;
    static LONGLONG nextFrame = 0;

    int limit = loading ? 0 : RequestedFrameRateLimit.load();
    if (limit == 0 && !originalLimit)
        return;

    auto nativeLimit = fb::Settings::Field<float>("GameTimeSettings", "MaxVariableFps");
    if (!nativeLimit)
        return;
    if (limit == 0)
    {
        *nativeLimit = *originalLimit;
        originalLimit.reset();
        previousLimit = 0;
        nextFrame = 0;
        return;
    }

    if (!originalLimit)
        originalLimit = *nativeLimit;
    // The native limiter rounds its sleep to whole milliseconds. Leave it above the supported FPS range.
    *nativeLimit = 10000.0f;

    static uint32_t refreshRate = 60;
    static HMONITOR refreshMonitor = nullptr;
    static ULONGLONG refreshCheckedAt = 0;
    if (limit < 0)
    {
        const auto window = hWnd.get();
        const auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
        const auto now = GetTickCount64();
        if (monitor != refreshMonitor || !refreshCheckedAt || now - refreshCheckedAt >= 2000)
        {
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            if (monitor && GetMonitorInfoW(monitor, &info) &&
                EnumDisplaySettingsExW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode, 0) &&
                mode.dmDisplayFrequency > 1)
                refreshRate = mode.dmDisplayFrequency;
            else
                refreshRate = GetDesktopRefreshRate();
            refreshMonitor = monitor;
            refreshCheckedAt = now;
        }
        limit = static_cast<int>(refreshRate);
    }
    limit = std::clamp(limit, 30, 1000);

    static const auto frequency = []()
    {
        LARGE_INTEGER result;
        QueryPerformanceFrequency(&result);
        return result.QuadPart;
    }();
    const auto clock = []()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return now.QuadPart;
    };
    const auto period = (frequency + limit / 2) / limit;
    const auto now = clock();
    if (limit != previousLimit || now >= nextFrame)
    {
        previousLimit = limit;
        nextFrame = now + period;
        return;
    }

    constexpr double spinTime = 0.0015;
    const double remaining = double(nextFrame - now) / double(frequency);
    if (remaining > spinTime)
    {
        static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        LARGE_INTEGER due{};
        due.QuadPart = -static_cast<LONGLONG>((remaining - spinTime) * 10000000.0);
        if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
            WaitForSingleObject(timer, INFINITE);
        else
            Sleep(static_cast<DWORD>((remaining - spinTime) * 1000.0));
    }
    auto release = clock();
    while (release < nextFrame)
    {
        YieldProcessor();
        release = clock();
    }
    // A late wake must not shorten the interval to the next update.
    nextFrame = release + period;
}

class FrameRate
{
public:
    FrameRate()
    {
        WFP::onInitEvent() += []()
        {
            const auto refreshFrameRateLimit = []()
            {
                static std::mutex mutex;
                const std::lock_guard lock(mutex);
                const auto value = FixSettings().Get("MAIN.FPSLimit");
                int limit = 0;
                std::from_chars(value.data(), value.data() + value.size(), limit);
                RequestedFrameRateLimit.store(limit);
            };
            FixSettings().OnChange("MAIN.FPSLimit", refreshFrameRateLimit);
            refreshFrameRateLimit();

            static bool bUnlockFrameRate = FixSettings().Int("FRAMERATE.UnlockFrameRate") != 0;
            static bool bUnlockCutscenes = FixSettings().Int("FRAMERATE.UnlockCutscenes") != 0;
            LoadScreen::bEnabled = bUnlockFrameRate;
            FramePacing::bEnabled = bUnlockFrameRate;
            if (bUnlockFrameRate)
            {
                // Right after the render thread's call to Present
                auto present = hook::pattern("57 ? ? 52 51 50 8B 47");
                static auto PresentedHook = safetyhook::create_mid(present.get_first(0xB), [](SafetyHookContext& regs)
                {
                    FramePacing::Presented();
                });

                auto fadeQueue = hook::pattern("8B 96 B4 00 00 00 8B 86 B8 00 00 00 8D 5C 42 02 3B D3");
                static auto TerrainFadeQueueHook = safetyhook::create_mid(fadeQueue.get_first(), [](SafetyHookContext& regs)
                {
                    AdvanceTerrainFades(regs.esi);
                });

                auto fadeStep = hook::pattern("81 E5 00 F0 00 00 81 C5 00 10 00 00 81 E1 FF 0F 00 00 33 E9 66 89 68 02");
                static auto TerrainFadeStepHook = safetyhook::create_mid(fadeStep.get_first(6), [](SafetyHookContext& regs)
                {
                    const auto maximum = std::min(*reinterpret_cast<const uint32_t*>(regs.esi + 0xC0), 15u);
                    const auto age = std::min((regs.ebp >> 12) + TerrainFadeSteps, maximum);
                    regs.ebp = (age << 12) - 0x1000u; // The next native instruction advances one unit.
                });
            }

            // The game forces VSync on in load screens and in some states of a race, which also turns off the frames
            // between the physics ticks, so the game stays at 30 frames per second even with VSync off in its options.
            // Load screens get no VSync, everything else the player's VSync setting. Only in 1.1, 1.0 has no such code.
            if (bUnlockFrameRate)
            {
                auto pattern = hook::pattern("C6 05 ? ? ? ? 01 C6 05 ? ? ? ? 01 E9 ? ? ? ? C6 44 24 ? 01");
                if (!pattern.empty()) // 1.1
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
                        else if ((bUnlockCutscenes || !fb::Vehicle::InRaceWithoutControl()) && fb::g_profileOptionsManager.get())
                        {
                            // The player's own VSync setting instead of the forced one
                            *vsync = fb::ProfileOptions::getInt(fb::g_profileOptionsManager + 0x14, "VSyncEnabled", 1) != 0;
                        }
                    });
                }
                else // 1.0: the race update uses the older layout without forced VSync
                {
                    auto olderLayout = hook::pattern("80 B8 A8 00 00 00 00 75 ? A1 ? ? ? ? 80 78 40 00");
                    if (olderLayout.empty())
                    {
                        olderLayout.get_first();
                        std::abort();
                    }
                }
            }

            // The frame's place between the ticks comes from the frame times added up since the last tick, divided by
            // the tick's length. In slow motion the tick's length is in game time, a fraction of the real 1/30 s
            // between the ticks, while the frame times are real: the place reaches the end of the tick on the first
            // frame after it, and everything stops until the next tick, so slow motion plays at 30 frames per second.
            // The frame times are scaled to game time as well.
            if (bUnlockFrameRate)
            {
                auto pattern = hook::pattern("0F 28 D0 0F 5A C0 0F 5A C9 F2 0F 58 C1 66 0F 5A C0 F3 0F 11 86 A0 01 00 00");
                static auto SlowMotionPlaceHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
                {
                    float timeScale = *(float*)(regs.ebx + 0x1C);   // the update's time scale
                    if (timeScale > 0.0f)
                        regs.xmm1.f32[0] *= timeScale;              // the frame time
                });
            }

            // Between the ticks the game makes each frame at least as long as its average frame with a tick, and waits
            // for that again when no tick came. The frames with a tick are measured with that wait included, so the
            // average keeps itself at 8-13 ms and holds the pause menu at 50-100 frames per second whatever the limit.
            // The frame limit paces the frames instead.
            if (bUnlockFrameRate)
            {
                auto pattern = hook::pattern("F2 0F 10 86 98 02 00 00 8B CE F2 0F 11 44 24 30 E8");
                static auto FrameEvenWaitHook = safetyhook::create_mid(pattern.get_first(8), [](SafetyHookContext& regs)
                {
                    regs.xmm0.f64[0] = 0.0;
                });
            }

            // The engine sound's pitch glides to each new value. Above 30 frames per second a new value comes before
            // the glide moves, the glide starts over and the pitch stays where it was, so the engine sounds stuck at one
            // tone. Then the glide carries on to the new value instead of starting over.
            auto pattern = hook::pattern("D9 83 C8 01 00 00 F3 0F 11 8B CC 01 00 00 D9 9B C0 01 00 00");
            static auto skip = (uintptr_t)pattern.get_first(14);
            static auto PitchGlideHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
            {
                if (AboveThirtyFps())
                    regs.eip = skip;
            });

            if (bUnlockFrameRate && bUnlockCutscenes)
            {
                struct CameraState
                {
                    std::vector<uintptr_t> cuts;
                    uintptr_t activeCamera = 0;
                    uint64_t tick = 0, fovTick = 0, lastCutTick = 0;
                    float previousFov = 0.0f, currentFov = 0.0f;
                    float alpha = 1.0f;
                    bool cinebot = false, blend = false, snapCut = false;
                };
                static CameraState state;

                // Cinebot's transition marks an instant cut. Capture it before commitShot clears it.
                auto cut = hook::pattern("8A 51 50 88 54 24 ? C6 41 50 00 E8");
                static auto CameraCutHook = safetyhook::create_mid(cut.get_first(), [](SafetyHookContext& regs)
                {
                    if (*reinterpret_cast<const bool*>(regs.ecx + 0x50))
                        state.cuts.push_back(regs.ebx);
                });

                // ClientCameraManager::postSimUpdate rotates the two poses on every tick.
                auto tick = hook::pattern("8D B5 F0 0E 00 00 8D BD 30 0F 00 00 B9 10 00 00 00 F3 A5");
                static auto CameraTickHook = safetyhook::create_mid(tick.get_first(), [](SafetyHookContext&)
                {
                    ++state.tick;
                });

                // Match the cutscene lens and transform to the same position between ticks as the scene.
                auto frameUpdate = hook::pattern("E8 ? ? ? ? 8D 77 10 C6 44 24 2C 00 0F 28 46 30");
                static auto CameraFrameHook = safetyhook::create_mid(frameUpdate.get_first(5), [](SafetyHookContext& regs)
                {
                    state.blend = false;
                    const auto camera = regs.edi;
                    const bool switched = camera != state.activeCamera;
                    const bool nativeCut = std::ranges::find(state.cuts, camera) != state.cuts.end();
                    const bool cut = switched || nativeCut;
                    state.cuts.clear();
                    if (switched)
                    {
                        state.activeCamera = camera;
                        state.cinebot = IsCinebotCamera(camera);
                    }
                    if (cut)
                    {
                        state.lastCutTick = state.tick;
                        state.snapCut = nativeCut && !switched;
                        state.previousFov = state.currentFov = 0.0f;
                    }

                    const float alpha = *reinterpret_cast<const float*>(regs.ebp + 0x10);
                    if (!camera || !(alpha >= 0.0f && alpha <= 1.0f))
                        return;

                    const auto current = reinterpret_cast<const float*>(regs.ebx + 0xEF0);
                    const auto previous = reinterpret_cast<const float*>(regs.ebx + 0xF30);
                    const auto length = [](const float* v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };
                    if (std::fabs(length(current + 8) - 1.0f) > 0.01f || std::fabs(length(previous + 8) - 1.0f) > 0.01f)
                        return;

                    auto* output = reinterpret_cast<float*>(camera + 0x10);
                    const bool recentCut = state.tick - state.lastCutTick <= 1;
                    // Look-back switches and scene cuts both use the native cut flag. Keep the whole simulation pose
                    // until both endpoints can be blended in the new shot.
                    if (recentCut && state.snapCut)
                    {
                        memcpy(output, current, 0x40);
                        return;
                    }

                    const bool driving = fb::Vehicle::bHasControl && GetTickCount64() - fb::Vehicle::LastUpdate < 500;
                    if (driving && state.cinebot && IsCinebotDrivingView())
                    {
                        const float turn = current[8] * previous[8] + current[9] * previous[9] + current[10] * previous[10];
                        const float alignment = output[8] * current[8] + output[9] * current[9] + output[10] * current[10];
                        if (!cut && turn < 0.5f && alignment < 0.5f)
                        {
                            memcpy(output, current, 0x40);
                            return;
                        }
                        return;
                    }

                    if (recentCut)
                        return;

                    state.alpha = alpha;
                    state.blend = true;
                    BlendTransform(previous, current, alpha, output);
                });

                auto afterUpdate = hook::pattern("8B 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? D9 43 04 51 D9 1C 24");
                static auto pManager = *afterUpdate.get_first<uintptr_t*>(2);
                static auto CameraFovHook = safetyhook::create_mid(afterUpdate.get_first(14), [](SafetyHookContext&)
                {
                    const auto manager = *pManager;
                    if (!state.activeCamera || !manager)
                        return;
                    const auto view = manager + 0xA50;
                    auto* fov = reinterpret_cast<float*>(view + 0x48);
                    const float own = *fov;
                    if (state.fovTick != state.tick)
                    {
                        state.fovTick = state.tick;
                        state.previousFov = state.currentFov;
                        state.currentFov = own;
                    }
                    if (!state.blend || state.previousFov <= 0.0f)
                        return;

                    const float previousFocal = 1.0f / std::tan(state.previousFov * 0.5f);
                    const float currentFocal = 1.0f / std::tan(state.currentFov * 0.5f);
                    const float focal = std::lerp(previousFocal, currentFocal, state.alpha);
                    if (focal > 0.0f)
                    {
                        *fov = 2.0f * std::atan(1.0f / focal);
                        *reinterpret_cast<uint32_t*>(view + 0xA0) |= 0xD;
                    }
                });

                // The visual environment publishes the letterbox width and opacities on simulation ticks.
                // Animate those values between updates in the renderer, keeping each simulated value as the target.
                auto screenEffect = hook::pattern("F3 0F 10 8B B4 06 00 00 8B 9B A0 06 00 00 0F 5A C9 F2 0F 5E 0D");
                static auto LetterboxHook = safetyhook::create_mid(screenEffect.get_first(), [](SafetyHookContext& regs)
                {
                    using Clock = std::chrono::steady_clock;
                    using Values = std::array<float, 3>;
                    struct Transition
                    {
                        Clock::time_point frame, changed;
                        Values from, to;
                    };
                    static thread_local Transition transition{};
                    constexpr int Letterbox = 3;
                    if (*reinterpret_cast<const int*>(regs.ebx + 0x6A0) != Letterbox)
                    {
                        transition.frame = {};
                        return;
                    }

                    const auto now = Clock::now();
                    const Values values{ regs.xmm5.f32[0], regs.xmm3.f32[0], regs.xmm4.f32[0] };
                    const float frameTime = std::chrono::duration<float>(now - transition.frame).count();
                    if (transition.frame == Clock::time_point{} || frameTime >= 1.0f / 32.0f)
                    {
                        transition = { now, now, values, values };
                        return;
                    }

                    const float elapsed = std::chrono::duration<float>(now - transition.changed).count();
                    const float fraction = std::min(elapsed * 30.0f, 1.0f);
                    Values output{};
                    for (size_t i = 0; i < output.size(); ++i)
                        output[i] = std::lerp(transition.from[i], transition.to[i], fraction);
                    if (values != transition.to)
                    {
                        transition.from = output;
                        transition.to = values;
                        transition.changed = now;
                    }
                    transition.frame = now;
                    regs.xmm5.f32[0] = output[0];
                    regs.xmm3.f32[0] = output[1];
                    regs.xmm4.f32[0] = output[2];
                });
            }

            // The emitter's speed: its last move divided by the time since the move before. The divisor is in xmm0,
            // the emitter in edx and eax.
            WFP::onGameProcessEvent() += []()
            {
                PruneEmitterMotions();
            };

            pattern = hook::pattern("0F 28 9A B0 00 00 00 0F 28 62 30 0F 5C E3 0F 5E E0");
            static auto EmitterSpeedHook1 = safetyhook::create_mid(pattern.get_first(14), [](SafetyHookContext& regs)
            {
                if (!AboveThirtyFps())
                    return;
                float time = EmitterMoveTime(regs.edx, regs.xmm0.f32[0]);
                for (auto& lane : regs.xmm0.f32)
                    lane = time;
            });

            pattern = hook::pattern("0F 28 90 B0 00 00 00 0F 28 58 30 0F 5C DA 0F C6 C9 00 0F 29 8C 24 ? ? ? ? F3 0F 10 0D ? ? ? ? 0F C6 C9 00 0F 5F C1 0F 5E D8");
            static auto EmitterSpeedHook2 = safetyhook::create_mid(pattern.get_first(41), [](SafetyHookContext& regs)
            {
                if (!AboveThirtyFps())
                    return;
                float time = EmitterMoveTime(regs.eax, regs.xmm0.f32[0]);
                for (auto& lane : regs.xmm0.f32)
                    lane = time;
            });
        };
    }
} FrameRate;
