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
    constexpr uint32_t TerrainFadeQueueScale = 4;

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

    struct VehiclePose
    {
        uintptr_t entity = 0;
        const float* transform = nullptr;
    };

    VehiclePose GetLocalVehiclePose(const uintptr_t* managerAddress)
    {
        const auto manager = managerAddress ? *managerAddress : 0;
        const auto player = manager ? *reinterpret_cast<const uintptr_t*>(manager + 0xBC) : 0;
        const auto vehicle = player ? *reinterpret_cast<const uintptr_t*>(player + 0x320) : 0;
        const auto data = vehicle ? *reinterpret_cast<const uint8_t* const*>(vehicle + 0x1C) : nullptr;
        if (!data)
            return {};

        // The vehicle render list takes its chassis transform from this active entity slot.
        const bool streamed = (*reinterpret_cast<const uint32_t*>(vehicle + 8) & 0x20000) != 0;
        return { vehicle, reinterpret_cast<const float*>(data + (streamed ? 0x10 + 80 * data[6] : 0)) };
    }

    using Transform = std::array<float, 16>;

    Transform CameraInVehicleSpace(const float* camera, const float* vehicle)
    {
        Transform relative{};
        for (int row = 0; row < 4; ++row)
        {
            const auto* v = camera + 4 * row;
            const float x = row == 3 ? v[0] - vehicle[12] : v[0];
            const float y = row == 3 ? v[1] - vehicle[13] : v[1];
            const float z = row == 3 ? v[2] - vehicle[14] : v[2];
            for (int axis = 0; axis < 3; ++axis)
                relative[4 * row + axis] = x * vehicle[4 * axis] + y * vehicle[4 * axis + 1] + z * vehicle[4 * axis + 2];
        }
        relative[15] = 1.0f;
        return relative;
    }

    void PlaceVehicleCamera(const Transform& relative, const float* vehicle, float* output)
    {
        for (int row = 0; row < 4; ++row)
        {
            const auto* v = relative.data() + 4 * row;
            for (int axis = 0; axis < 3; ++axis)
                output[4 * row + axis] = v[0] * vehicle[axis] + v[1] * vehicle[4 + axis] + v[2] * vehicle[8 + axis]
                    + (row == 3 ? vehicle[12 + axis] : 0.0f);
            output[4 * row + 3] = v[3];
        }
    }

    using UpdateAnimationParent = void(__thiscall*)(uintptr_t);

    void UpdateDashboardAttachments(uintptr_t vehicle, const uintptr_t* updaterAddress, UpdateAnimationParent updateParent)
    {
        const auto updater = updaterAddress && updateParent ? *updaterAddress : 0;
        if (!updater)
            return;

        const auto* first = *reinterpret_cast<const uintptr_t* const*>(updater + 0xC);
        const auto* last = *reinterpret_cast<const uintptr_t* const*>(updater + 0x10);
        constexpr uint32_t DashboardMarker = 161;
        for (auto item = first; item != last; ++item)
        {
            const auto entity = *item;
            if (!entity || *reinterpret_cast<const uint8_t*>(entity + 0x40))
                continue;
            const auto parent = *reinterpret_cast<const uintptr_t* const*>(entity + 0x3C);
            const auto config = *reinterpret_cast<const uintptr_t*>(entity + 0x14);
            if (parent && *parent == vehicle + 4 && config && *reinterpret_cast<const uint32_t*>(config + 0xA8) == DashboardMarker)
                updateParent(entity);
        }
    }
}

export void ApplyFrameRateLimit(bool loading)
{
    static std::optional<float> originalLimit;

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
        return;
    }

    if (!originalLimit)
        originalLimit = *nativeLimit;

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

    // The game's own limiter paces the frames. It keeps the game's background work going while it waits; frames
    // paced outside it give physics debris less simulated time, and the pieces of a broken barrier stop flying early.
    *nativeLimit = static_cast<float>(limit);
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
                // The native ring is sized for one age step per terrain update. Let it hold four
                // updates per step, with room for the additional dirty tiles in the companion list.
                auto fadeQueueSize = hook::pattern("D1 EA 0B C2 8B C8 8D 6C 00 02 6A 02 89 8E B8 00 00 00");
                static auto TerrainFadeQueueSizeHook = safetyhook::create_mid(fadeQueueSize.get_first(4), [](SafetyHookContext& regs)
                {
                    regs.eax = (regs.eax + 1) * TerrainFadeQueueScale - 1;
                });
                auto dirtyListSize = hook::pattern("8D 0C 49 C1 E9 03 C1 E0 06 03 C1 6A 10 8D 14 00 89 86 D8 00 00 00");
                static auto TerrainDirtyListSizeHook = safetyhook::create_mid(dirtyListSize.get_first(11), [](SafetyHookContext& regs)
                {
                    regs.eax += *reinterpret_cast<const uint32_t*>(regs.esi + 0xB8) + 1;
                });

                auto fadeEnqueue = hook::pattern("8B 46 08 2B 87 A0 00 00 00 8B 8F BC 00 00 00 8B 97 B4 00 00 00 C1 F8 04 66 89 04 4A 8B 87 BC 00 00 00");
                static auto TerrainFadeEnqueueHook = safetyhook::create_mid(fadeEnqueue.get_first(21), [](SafetyHookContext& regs)
                {
                    const auto oldIndex = reinterpret_cast<const uint16_t*>(regs.edx)[regs.ecx];
                    if (oldIndex == 0xFFFF || oldIndex == (regs.eax >> 4))
                        return;

                    const auto tree = regs.edi;
                    const auto tile = *reinterpret_cast<const uintptr_t*>(tree + 0xA0) + 16 * oldIndex;
                    auto& flags = *reinterpret_cast<uint16_t*>(tile + 2);
                    const auto maximum = std::min(*reinterpret_cast<const uint32_t*>(tree + 0xC0), 15u);
                    if (uint32_t(flags >> 12) >= maximum || (flags & 0x100) || *reinterpret_cast<const uint8_t*>(tile + 0xA) == 0xFF)
                        return;

                    // A burst of uploads can still wrap the ring. Publish the final blend before
                    // replacing its slot so a resident tile cannot retain partial alpha indefinitely.
                    flags = static_cast<uint16_t>((flags & 0xFFF) | (maximum << 12));
                    auto& dirtyCount = *reinterpret_cast<uint32_t*>(tree + 0xD4);
                    auto* dirtyTiles = *reinterpret_cast<uint16_t**>(tree + 0xD0);
                    dirtyTiles[dirtyCount++] = oldIndex;
                });

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
                else // 1.0 uses the interpolation setting directly to select variable frame pacing.
                {
                    auto olderLayout = hook::pattern("80 B8 A8 00 00 00 00 75 ? A1 ? ? ? ? 80 78 40 00");
                    static auto ApplyInterpolationHook = safetyhook::create_mid(olderLayout.get_first(), [](SafetyHookContext& regs)
                    {
                        auto& interpolate = *reinterpret_cast<bool*>(regs.eax + 0xA8);
                        if (LoadScreen::IsActive())
                            interpolate = true;
                        else if (fb::g_profileOptionsManager.get())
                        {
                            const bool vsync = fb::ProfileOptions::getInt(fb::g_profileOptionsManager + 0x14, "VSyncEnabled", 1) != 0;
                            interpolate = !vsync && (bUnlockCutscenes || !fb::Vehicle::InRaceWithoutControl());
                        }
                    });
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
                    float displayFov = 0.0f, outgoingFov = 0.0f;
                    float alpha = 1.0f;
                    bool cinebot = false, blend = false, snapCut = false;
                };
                static CameraState state;

                struct VehicleCameraSample
                {
                    uintptr_t camera = 0, vehicle = 0;
                    uint32_t source = 0;
                    uint64_t tick = 0;
                    std::array<float, 3> authoredEye{};
                    Transform body{};
                };
                static std::array<VehicleCameraSample, 4> vehicleShots;
                struct VehicleView
                {
                    uintptr_t camera = 0, vehicle = 0;
                    uint32_t source = 0;
                    uint64_t tick = 0;
                    Transform relative{};
                    float fov = 0.0f;
                };
                static VehicleView vehicleView;
                enum class VehicleLens { Native, Store, Hold, HoldOutgoing };
                static VehicleLens vehicleLens = VehicleLens::Native;
                static uintptr_t sampledVehicle = 0;
                static std::array<float, 3> sampledPosition{};
                static uint64_t relocatedTick = 0;
                static uintptr_t lastParentedVehicle = 0;
                static Transform lastParentedTransform{};

                auto playerVehicle = hook::pattern("80 7A 64 00 74 ? 8B 0D ? ? ? ? E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 80 20 03 00 00 85 C0 0F 84");
                static const auto playerManager = *playerVehicle.get_first<const uintptr_t*>(8);
                auto antCamera = hook::pattern("8B 8B 48 02 00 00 E8 ? ? ? ? 89 86 ? ? 00 00 5F 5E 5B 8B E5 5D C3");
                static const auto antSourceOffset = *antCamera.get_first<uint32_t>(13);
                static auto AntCameraHook = safetyhook::create_mid(antCamera.get_first(11), [](SafetyHookContext& regs)
                {
                    if (!state.cinebot || regs.esi != state.activeCamera)
                        return;
                    const auto body = fb::Vehicle::InRaceWithoutControl() ? GetLocalVehiclePose(playerManager) : VehiclePose{};
                    if (!body.transform)
                    {
                        sampledVehicle = 0;
                        return;
                    }
                    const auto position = body.transform + 12;
                    const float movement = std::hypot(position[0] - sampledPosition[0],
                        position[1] - sampledPosition[1], position[2] - sampledPosition[2]);
                    const bool moved = sampledVehicle != body.entity || movement > 0.02f;
                    // A tick's travel far beyond any driving speed: the scene put the car somewhere else.
                    if (sampledVehicle == body.entity && movement > 4.0f)
                        relocatedTick = state.tick;
                    sampledVehicle = body.entity;
                    std::copy_n(position, 3, sampledPosition.begin());
                    if (!moved)
                        return;

                    const auto authoredEye = reinterpret_cast<const float*>(regs.esi + antSourceOffset - 0x8C) + 12;
                    const float x = authoredEye[0] - body.transform[12];
                    const float y = authoredEye[1] - body.transform[13];
                    const float z = authoredEye[2] - body.transform[14];
                    const auto& previous = vehicleShots.front();
                    const bool continuing = previous.camera == regs.esi && previous.vehicle == body.entity && previous.source == regs.eax;
                    const float range = continuing ? 12.0f : 3.0f;
                    if (x * x + y * y + z * z < range * range)
                    {
                        std::move_backward(vehicleShots.begin(), vehicleShots.end() - 1, vehicleShots.end());
                        auto& sample = vehicleShots.front();
                        sample = { regs.esi, body.entity, regs.eax, state.tick,
                            { authoredEye[0], authoredEye[1], authoredEye[2] }, {} };
                        std::copy_n(body.transform, 16, sample.body.begin());
                    }
                });

                // Dashboard props use the simulation pose for their native parenting. Reapply that
                // attachment with the drawn chassis during the moving shot.
                auto parenting = hook::pattern("8B 3D ? ? ? ? 85 FF 75 3C 6A 10 6A 4C B9 ? ? ? ? E8 ? ? ? ? 8B F8 85 FF 74 20 6A 64 68 ? ? ? ? 53 6A 00 8B CF E8 ? ? ? ? C7 47 48 ? ? ? ? C7 07 ? ? ? ?");
                static const auto parentingUpdater = *parenting.get_first<const uintptr_t*>(2);
                const auto* methods = *parenting.get_first<const uint8_t* const*>(56);
                static const auto updateParent = reinterpret_cast<UpdateAnimationParent>(
                    injector::GetBranchDestination(reinterpret_cast<uintptr_t>(methods[2] + 34)).as_int());

                // The newer Cinebot blends the incoming shot with the outgoing one before committing it.
                // Commit the complete incoming shot and lens on every evaluation. The incoming shot
                // can be published a tick after the transition marks the cut.
                auto commit = hook::pattern("55 8B EC 83 E4 F0 8B 45 08 83 EC 44 80 B8 80 00 00 00 00 53 56 57 8B D9");
                static auto CommitShotHook = safetyhook::create_mid(commit.get_first(6), [](SafetyHookContext& regs)
                {
                    auto& shot = *reinterpret_cast<uintptr_t*>(regs.ebp + 8);
                    const auto camera = regs.ecx;
                    if (shot != camera + 0x580 || (fb::Vehicle::bHasControl && GetTickCount64() - fb::Vehicle::LastUpdate < 500))
                        return;

                    const auto list = camera + 0x3C0;
                    if (*reinterpret_cast<const uintptr_t*>(list) == list)
                        return;
                    const auto transition = *reinterpret_cast<const uintptr_t*>(*reinterpret_cast<const uintptr_t*>(list + 4) + 8);
                    const auto incoming = camera + 0x460;
                    const bool cutTick = camera == state.activeCamera && state.snapCut && state.tick - state.lastCutTick <= 1;
                    if ((cutTick || *reinterpret_cast<const bool*>(transition + 0x50)) && *reinterpret_cast<const bool*>(incoming + 0x80))
                        shot = incoming;
                });

                // Capture the cut before commitShot clears it so frame interpolation can avoid the old endpoint.
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
                    vehicleLens = VehicleLens::Native;
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
                        state.outgoingFov = state.displayFov;
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
                    const bool driving = fb::Vehicle::bHasControl && GetTickCount64() - fb::Vehicle::LastUpdate < 500;
                    const auto placeVehicleShot = [&](bool interpolate)
                    {
                        if (driving || !state.cinebot)
                            return false;
                        const auto body = GetLocalVehiclePose(playerManager);
                        if (!body.transform)
                            return false;

                        const auto source = *reinterpret_cast<const uint32_t*>(camera + antSourceOffset);
                        if (!source)
                            return false;
                        // When the scene puts the car somewhere else at a cut, the new rig can publish while the
                        // chassis is still traversing its old and new positions. Carry the outgoing rig with the
                        // drawn chassis through that cut tick. Ordinary cuts show the incoming rig at once.
                        const bool hasVehicleView = vehicleView.camera == camera && vehicleView.vehicle == body.entity &&
                            state.tick - vehicleView.tick <= 2;
                        const bool relocated = relocatedTick && state.tick - relocatedTick <= 2;
                        const bool hold = recentCut && state.snapCut && hasVehicleView && vehicleView.source != source && relocated;
                        // The finalized camera can lag the latest ANT publication by a tick. Pair each
                        // endpoint with its closest authored eye, then retain the finalized rig and lens aim.
                        const auto match = [&](const float* pose) -> const VehicleCameraSample*
                        {
                            const VehicleCameraSample* nearest = nullptr;
                            float distanceSquared = 2.25f;
                            for (const auto& sample : vehicleShots)
                            {
                                if (sample.camera != camera || sample.vehicle != body.entity || state.tick - sample.tick > 3 ||
                                    sample.source != source)
                                    continue;
                                const float x = pose[12] - sample.authoredEye[0];
                                const float y = pose[13] - sample.authoredEye[1];
                                const float z = pose[14] - sample.authoredEye[2];
                                const float distance = x * x + y * y + z * z;
                                if (distance < distanceSquared)
                                {
                                    distanceSquared = distance;
                                    nearest = &sample;
                                }
                            }
                            return nearest;
                        };
                        const auto* anchor = hold ? nullptr : match(current);
                        if (!hold && !anchor)
                            return false;
                        // On a long cut into the car, the native driver and the interpolating chassis
                        // can occupy different positions for the first tick. Keep the exterior shot until
                        // the incoming camera and its scene have both advanced.
                        if (!hasVehicleView && anchor && state.snapCut && state.tick == state.lastCutTick &&
                            std::hypot(previous[12] - anchor->body[12], previous[13] - anchor->body[13], previous[14] - anchor->body[14]) > 12.0f)
                        {
                            memcpy(output, previous, 0x40);
                            vehicleLens = VehicleLens::HoldOutgoing;
                            return true;
                        }
                        auto relative = hold ? vehicleView.relative : CameraInVehicleSpace(current, anchor->body.data());
                        if (interpolate)
                        {
                            if (const auto* earlier = match(previous); earlier && earlier != anchor)
                            {
                                const auto prior = CameraInVehicleSpace(previous, earlier->body.data());
                                Transform blended{};
                                BlendTransform(prior.data(), relative.data(), alpha, blended.data());
                                relative = blended;
                            }
                        }
                        Transform drawn{};
                        std::copy_n(body.transform, drawn.size(), drawn.begin());
                        if (lastParentedVehicle != body.entity || drawn != lastParentedTransform)
                        {
                            lastParentedVehicle = body.entity;
                            lastParentedTransform = drawn;
                            UpdateDashboardAttachments(body.entity, parentingUpdater, updateParent);
                        }
                        PlaceVehicleCamera(relative, drawn.data(), output);
                        if (hold)
                            vehicleLens = VehicleLens::Hold;
                        else
                        {
                            vehicleView = { camera, body.entity, source, state.tick, relative, vehicleView.fov };
                            vehicleLens = VehicleLens::Store;
                        }
                        return true;
                    };
                    if (recentCut && state.snapCut)
                    {
                        if (!placeVehicleShot(false))
                            memcpy(output, current, 0x40);
                        return;
                    }

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
                    if (!placeVehicleShot(true))
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
                    float displayFov = own;
                    if (vehicleLens == VehicleLens::Hold && vehicleView.fov > 0.0f)
                        displayFov = vehicleView.fov;
                    else if (vehicleLens == VehicleLens::HoldOutgoing && state.outgoingFov > 0.0f)
                        displayFov = state.outgoingFov;
                    else if (state.blend && state.previousFov > 0.0f)
                    {
                        const float previousFocal = 1.0f / std::tan(state.previousFov * 0.5f);
                        const float currentFocal = 1.0f / std::tan(state.currentFov * 0.5f);
                        const float focal = std::lerp(previousFocal, currentFocal, state.alpha);
                        if (focal > 0.0f)
                            displayFov = 2.0f * std::atan(1.0f / focal);
                    }
                    if (vehicleLens == VehicleLens::Store)
                        vehicleView.fov = displayFov;
                    state.displayFov = displayFov;
                    if (displayFov != own)
                    {
                        *fov = displayFov;
                        *reinterpret_cast<uint32_t*>(view + 0xA0) |= 0xD;
                    }
                });

                // The visual environment publishes the letterbox on simulation ticks: its width, two opacities and how
                // far the bars are in (+0xAB4 in the frame's view data, multiplied with the width for the bars' height),
                // so the bars slide in and out in 30 Hz steps. Those values are animated between the ticks in the
                // renderer, keeping each simulated value as the target.
                struct LetterboxBlend
                {
                    bool active = false;
                    float height = 0.0f;
                };
                static thread_local LetterboxBlend letterbox{};

                auto screenEffect = hook::pattern("F3 0F 10 8B B4 06 00 00 8B 9B A0 06 00 00 0F 5A C9 F2 0F 5E 0D");
                static auto LetterboxHook = safetyhook::create_mid(screenEffect.get_first(), [](SafetyHookContext& regs)
                {
                    using Clock = std::chrono::steady_clock;
                    using Values = std::array<float, 4>;
                    struct Transition
                    {
                        Clock::time_point frame, changed;
                        Values from, to;
                    };
                    static thread_local Transition transition{};
                    constexpr int Letterbox = 3;
                    letterbox.active = false;
                    if (*reinterpret_cast<const int*>(regs.ebx + 0x6A0) != Letterbox)
                    {
                        transition.frame = {};
                        return;
                    }

                    const auto now = Clock::now();
                    const Values values{ regs.xmm5.f32[0], regs.xmm3.f32[0], regs.xmm4.f32[0], *reinterpret_cast<const float*>(regs.ebx + 0xAB4) };
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
                    letterbox = { true, output[0] * output[3] };
                });

                // The bars' height: the width times how far they are in
                auto letterboxHeight = hook::pattern("F3 0F 10 81 B4 0A 00 00 8B 8C 24 ? ? ? ? 0F 5A C0 0F 5A CD F2 0F 59 C1 66 0F 5A F0");
                static auto LetterboxHeightHook = safetyhook::create_mid(letterboxHeight.get_first(29), [](SafetyHookContext& regs)
                {
                    if (letterbox.active)
                        regs.xmm6.f32[0] = letterbox.height;
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
