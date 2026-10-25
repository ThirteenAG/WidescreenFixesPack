module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cmath>
#include <chrono>

export module Camera;

import ComVars;

bool bFreeHeading = false;  // the free look has its own direction instead of the game's camera
float WorldYaw = 0.0f;      // degrees, direction from the car to the camera in the world
float PitchOffset = 0.0f;   // degrees, relative to the game's camera

bool InvertLook = false;
bool OnlyAllowHorizontalStickMovement = false;
float StickLookSensitivity = 1.0f;
float MouseLookSensitivity = 1.0f;

float IdleTimer = 0.0f;
float IdleTimeoutSeconds = 3.0f;
float ReturnSpeed = 2.0f;
float CollisionOffset = 0.3f;
float MinCamDistance = 1.5f;

constexpr float DEG2RAD = 0.0174533f;

constexpr float StickDegreesPerSecond = 150.0f;
constexpr float StickDeadzone = 0.1f;
constexpr float CarMoveThreshold = 0.5f;

float CarVelocityX = 0.0f;
float CarVelocityZ = 0.0f;
float PrevCarPos[3] = {};
bool bPrevCarPosValid = false;

float WrapDegrees(float angle)
{
    angle = std::fmod(angle + 180.0f, 360.0f);
    return (angle < 0.0f ? angle + 360.0f : angle) - 180.0f;
}

// The mouse is read as the cursor's distance from the window centre, where it's put back every frame. When it
// can't be put back exactly (display scaling, Proton/Gamescope), the same offset comes every frame and the
// camera turns on its own. A hand never moves a mouse by exactly the same amount for this many frames in a row.
class MouseSanity
{
    int lastX = 0, lastY = 0, repeats = 0;
    bool broken = false;

public:
    bool Accept(int dx, int dy)
    {
        if (dx == 0 && dy == 0)
        {
            repeats = 0;
            return !broken;
        }

        if (dx == lastX && dy == lastY)
        {
            if (++repeats >= 30)
                broken = true;
        }
        else
        {
            repeats = 0;
            broken = false;
        }

        lastX = dx;
        lastY = dy;
        return !broken;
    }
} MouseFilter;

void ResetLook()
{
    bFreeHeading = false;
    PitchOffset = 0.0f;
    IdleTimer = 0.0f;
    CarVelocityX = CarVelocityZ = 0.0f;
    bPrevCarPosValid = false;
}

using SteadyClock = std::chrono::steady_clock;
using TimePoint = SteadyClock::time_point;
constexpr auto HideCursorDuration = std::chrono::milliseconds(100);
TimePoint LastCameraInputTime = {};

uint8_t  bLookBehind = 0;
uint8_t  bFreeLook = 0;
uint32_t nCameraMode = 0;
std::vector<std::string> vCameraStack;

struct alignas(16) RayCastHit
{
    __m128   m_position;     // +0
    __m128   m_normal;       // +16
    void* m_rigidBody;       // +32  (non-null = hit)
    void* m_material;        // +36
    uint32_t m_part;         // +40
    uint32_t m_bone;         // +44
    float    m_lambda;       // +48  (hit fraction 0..1 along ray)
    uint32_t _pad[3];        // +52  (pad to 64 bytes)

    void init()
    {
        m_position = _mm_setzero_ps();
        m_normal = _mm_setzero_ps();
        m_rigidBody = nullptr;
        m_material = (void*)(uintptr_t)-1;
        m_part = 0;
        m_bone = 0;
        m_lambda = 1.0f;
        _pad[0] = _pad[1] = _pad[2] = 0;
    }
};

namespace fb
{
    namespace CinebotRay
    {
        bool (__cdecl* test)(__m128* from, __m128* to, RayCastHit* hit, void* ignore) = nullptr;
    }

    namespace CinebotCamera
    {
        SafetyHookInline shCommitShot = {};
        void __fastcall commitShot(float* CinebotCamera, void* edx, void* CinebotCameraShot)
        {
            bHideCursorForMouseLook = true;
            LastCameraInputTime = SteadyClock::now();

            bool bIsChaseCamera = false;
            for (auto& it : vCameraStack)
            {
                if (it.ends_with("chaseMode") || it.ends_with("lookBackMode"))
                {
                    bIsChaseCamera = true;
                    break;
                }
            }

            float* shotEye = (float*)((uintptr_t)CinebotCameraShot);
            float* shotTgt = (float*)((uintptr_t)CinebotCameraShot + 16);

            float origEyeX = shotEye[0], origEyeY = shotEye[1], origEyeZ = shotEye[2];
            float tgtX = shotTgt[0], tgtY = shotTgt[1], tgtZ = shotTgt[2];

            // A new race, or the chase camera back after another view: the look starts behind the car, and
            // mouse movement made in the menus or while loading doesn't turn it. The same when the car is
            // put somewhere else, such as back on the road.
            bool bActive = fb::Vehicle::bHasControl && !bFreeLook && !bLookBehind && bIsChaseCamera;
            static bool bWasActive = false;
            static auto lastUpdate = SteadyClock::time_point{};
            auto now = SteadyClock::now();
            bool bRestart = (bActive && !bWasActive) || now - lastUpdate > std::chrono::milliseconds(250) ||
                (bPrevCarPosValid && hypotf(tgtX - PrevCarPos[0], tgtZ - PrevCarPos[2]) > 20.0f);
            lastUpdate = now;
            bWasActive = bActive;
            if (bRestart)
            {
                ResetLook();
                g_Mouse.DeltaX = 0;
                g_Mouse.DeltaY = 0;
            }

            if (!bActive)
            {
                ResetLook();
                return shCommitShot.unsafe_fastcall(CinebotCamera, edx, CinebotCameraShot);
            }

            float dt = std::clamp((float)fb::GameTimer::frameDeltaTime, 0.0f, 0.1f);

            // The car's velocity on the ground (x, z), smoothed a little against frame time noise
            if (bPrevCarPosValid && dt > 0.0f)
            {
                float blend = 1.0f - expf(-10.0f * dt);
                CarVelocityX += ((tgtX - PrevCarPos[0]) / dt - CarVelocityX) * blend;
                CarVelocityZ += ((tgtZ - PrevCarPos[2]) / dt - CarVelocityZ) * blend;
            }
            PrevCarPos[0] = tgtX;
            PrevCarPos[1] = tgtY;
            PrevCarPos[2] = tgtZ;
            bPrevCarPosValid = true;

            // The game's camera: where it sits around the car (y is up)
            float dx = origEyeX - tgtX;
            float dy = origEyeY - tgtY;
            float dz = origEyeZ - tgtZ;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);
            float distXZ = sqrtf(dx * dx + dz * dz);
            float gameYaw = atan2f(dx, dz) / DEG2RAD;
            float gamePitch = atan2f(dy, distXZ) / DEG2RAD;

            float mouseDeltaX = 0.0f;
            float mouseDeltaY = 0.0f;
            if (MouseFilter.Accept(g_Mouse.DeltaX, g_Mouse.DeltaY))
            {
                mouseDeltaX = (float)g_Mouse.DeltaX;
                mouseDeltaY = (float)g_Mouse.DeltaY;
            }
            g_Mouse.DeltaX = 0;
            g_Mouse.DeltaY = 0;

            // Degrees: the stick turns at a speed, the same at any frame rate; the mouse by its movement
            // A stick at rest isn't exactly centred: below the deadzone it doesn't turn the camera
            auto deadzone = [](float value)
            {
                float magnitude = fabsf(value);
                return magnitude < StickDeadzone ? 0.0f : copysignf((magnitude - StickDeadzone) / (1.0f - StickDeadzone), value);
            };
            float stickX = -deadzone(g_RightStick.X) * StickDegreesPerSecond * StickLookSensitivity * dt;
            float stickY = (OnlyAllowHorizontalStickMovement ? 0.0f : deadzone(g_RightStick.Y)) * StickDegreesPerSecond * StickLookSensitivity * dt;
            float deltaX = stickX - mouseDeltaX * MouseLookSensitivity;
            float deltaY = stickY + mouseDeltaY * MouseLookSensitivity;

            // Like San Andreas: the look direction is a direction in the world, so turning the car doesn't
            // turn it. After the return timeout without looking, the camera eases back behind the car.
            if (fabsf(deltaX) > 0.001f || fabsf(deltaY) > 0.001f)
            {
                if (!bFreeHeading)
                {
                    bFreeHeading = true;
                    WorldYaw = gameYaw;
                }
                WorldYaw = WrapDegrees(WorldYaw + deltaX);
                PitchOffset = std::clamp(PitchOffset + deltaY * (InvertLook ? -1.0f : 1.0f), -30.0f, 60.0f);
                IdleTimer = 0.0f;
            }
            else if (bFreeHeading)
            {
                IdleTimer += dt;
                if (IdleTimer >= IdleTimeoutSeconds)
                {
                    // Ease back to the game's own camera while the car moves. The game's camera is the target,
                    // so there's no jump when it takes over again.
                    if (hypotf(CarVelocityX, CarVelocityZ) > CarMoveThreshold)
                    {
                        float blend = 1.0f - expf(-std::max(0.1f, ReturnSpeed) * dt);
                        WorldYaw = WrapDegrees(WorldYaw + WrapDegrees(gameYaw - WorldYaw) * blend);
                        PitchOffset -= PitchOffset * blend;
                    }

                    // Back with the game's own camera once the difference can't be seen
                    if (fabsf(WrapDegrees(WorldYaw - gameYaw)) < 0.05f && fabsf(PitchOffset) < 0.05f)
                    {
                        bFreeHeading = false;
                        PitchOffset = 0.0f;
                    }
                }
            }

            if (!bFreeHeading)
                return shCommitShot.unsafe_fastcall(CinebotCamera, edx, CinebotCameraShot);

            float finalYaw = WorldYaw * DEG2RAD;
            float finalPitch = (gamePitch + PitchOffset) * DEG2RAD;
            if (finalPitch > 1.39626f) finalPitch = 1.39626f;
            if (finalPitch < -1.39626f) finalPitch = -1.39626f;

            float cp = cosf(finalPitch);
            shotEye[0] = tgtX + dist * cp * sinf(finalYaw);
            shotEye[1] = tgtY + dist * sinf(finalPitch);
            shotEye[2] = tgtZ + dist * cp * cosf(finalYaw);

            if (dist > MinCamDistance)
            {
                RayCastHit hit;
                hit.init();

                //__m128 rayFrom = _mm_setr_ps(tgtX, tgtY, tgtZ, 0.0f);
                float skip = 1.4f;
                if (skip >= dist) skip = dist * 0.3f;
                float invD = 1.0f / dist;
                __m128 rayFrom = _mm_setr_ps(
                    tgtX + (shotEye[0] - tgtX) * invD * skip,
                    tgtY + (shotEye[1] - tgtY) * invD * skip,
                    tgtZ + (shotEye[2] - tgtZ) * invD * skip, 0.0f);

                __m128 rayTo = _mm_setr_ps(shotEye[0], shotEye[1], shotEye[2], 0.0f);

                if (fb::CinebotRay::test(&rayFrom, &rayTo, &hit, nullptr))
                {
                    // m_lambda = fraction along ray (0 = at target, 1 = at eye)
                    float safeDist = skip + (dist - skip) * hit.m_lambda - CollisionOffset;

                    if (safeDist < MinCamDistance)
                        safeDist = MinCamDistance;

                    if (safeDist < dist)
                    {
                        float t = safeDist / dist;
                        shotEye[0] = tgtX + (shotEye[0] - tgtX) * t;
                        shotEye[1] = tgtY + (shotEye[1] - tgtY) * t;
                        shotEye[2] = tgtZ + (shotEye[2] - tgtZ) * t;
                    }
                }
            }

            shCommitShot.unsafe_fastcall(CinebotCamera, edx, CinebotCameraShot);

            shotEye[0] = origEyeX;
            shotEye[1] = origEyeY;
            shotEye[2] = origEyeZ;
        }
    }

    namespace CinebotStateLogicEntity
    {
        SafetyHookInline shevent = {};
        void __fastcall event(void* _this, void* edx, int a2)
        {
            DWORD eventType = *(DWORD*)((uintptr_t)a2 + 4);

            std::string cameraName = "Unknown";

            if (eventType == 210993314) // BEGIN / PUSH
            {
                cameraName = std::string_view(*(const char**)(*(uintptr_t*)((*((DWORD*)_this + 5)) + 0x10) + 0xC));

                vCameraStack.push_back(cameraName);

                while (vCameraStack.size() > 16)
                    vCameraStack.erase(vCameraStack.begin());
            }
            else if (eventType == 193438506) // END / POP
            {
                cameraName = std::string_view(*(const char**)(*(uintptr_t*)((*((DWORD*)_this + 5)) + 0x10) + 0xC));

                for (auto it = vCameraStack.rbegin(); it != vCameraStack.rend(); ++it)
                {
                    if (*it == cameraName)
                    {
                        vCameraStack.erase(std::next(it).base());
                        break;
                    }
                }
            }

            return shevent.unsafe_fastcall(_this, edx, a2);
        }
    }
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEventAsync() += []()
        {
            // ClientCameraManager::update: the camera's field of view (vertical, radians) before it goes to the view.
            // The aspect ratio comes from the viewport, so the view is Hor+ by itself. In races the field of view is
            // multiplied by MAIN.FOVFactor; cutscenes and scripted shots set their own after this and keep it.
            auto fovPattern = hook::pattern("D9 44 24 0C 51 8B CE D9 1C 24 E8 ? ? ? ? 8B 9B 84 0F 00 00");
            if (!fovPattern.empty())
            {
                static auto& fFOVFactor = FixSettings().Float("MAIN.FOVFactor");
                static auto FieldOfViewHook = safetyhook::create_mid(fovPattern.get_first(), [](SafetyHookContext& regs)
                {
                    bool inRace = GetTickCount64() - fb::Vehicle::LastUpdate < 500;
                    float factor = std::clamp(fFOVFactor, 0.5f, 2.0f);
                    auto fov = (float*)(regs.esp + 0xC);
                    if (inRace && factor != 1.0f)
                        *fov = std::clamp(*fov * factor, 0.0002f, 2.96706f);   // below 170 degrees
                });
            }

            if (!FixSettings().Int("CAMERA.Enable"))
                return;

            IdleTimeoutSeconds = FixSettings().Float("CAMERA.CameraReturnTimeout");
            ReturnSpeed = FixSettings().Float("CAMERA.CameraReturnSpeed");
            StickLookSensitivity = FixSettings().Float("MAIN.GamepadCameraSpeed");
            MouseLookSensitivity = FixSettings().Float("MAIN.MouseSensitivity") * 0.15f;
            InvertLook = FixSettings().Int("CAMERA.InvertLook") != 0;
            CollisionOffset = FixSettings().Float("CAMERA.CollisionOffset");
            MinCamDistance = FixSettings().Float("CAMERA.MinCamDistance");
            OnlyAllowHorizontalStickMovement = FixSettings().Int("CAMERA.OnlyAllowHorizontalStickMovement") != 0;

            auto pattern = find_pattern("E8 ? ? ? ? 8B 44 24 ? 0F 57 D2", "E8 ? ? ? ? 0F 57 D2 F3 0F 11 93");
            fb::CinebotCamera::shCommitShot = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), fb::CinebotCamera::commitShot);

            pattern = hook::pattern("83 EC ? 8D 44 24 ? 8B C8");
            fb::CinebotRay::test = (decltype(fb::CinebotRay::test))pattern.get_first();

            pattern = hook::pattern("8B 44 24 ? 8B 40 ? 83 EC ? 56 8B F1 3D ? ? ? ? 0F 84");
            fb::CinebotStateLogicEntity::shevent = safetyhook::create_inline(pattern.get_first(), fb::CinebotStateLogicEntity::event);

            pattern = hook::pattern("8B 01 8B 90 ? ? ? ? 5F 5E FF E2");
            static auto LookBehindHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bLookBehind = *(uint8_t*)(regs.ecx + 0xE9);
            });

            static double g_minStickDeviationForFreeLook = 2.0;
            pattern = hook::pattern("F2 0F 59 C8 F2 0F 58 D1 66 0F 2F 15");
            injector::WriteMemory(pattern.get_first(12), &g_minStickDeviationForFreeLook, true);

            static auto FreeLookHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bFreeLook = *(uint8_t*)(regs.esi + 0xEA);

                static bool bWasPressed = false;
                if (bBothSticksClicked)
                {
                    if (!bWasPressed)
                    {
                        g_minStickDeviationForFreeLook = (g_minStickDeviationForFreeLook > 1.0) ? 0.25 : 2.0;
                        bWasPressed = true;
                    }
                }
                else
                {
                    bWasPressed = false;
                }
            });

            WFP::onGameProcessEvent() += []()
            {
                IdleTimeoutSeconds = FixSettings().Float("CAMERA.CameraReturnTimeout");
                ReturnSpeed = FixSettings().Float("CAMERA.CameraReturnSpeed");
                StickLookSensitivity = FixSettings().Float("MAIN.GamepadCameraSpeed");
                MouseLookSensitivity = FixSettings().Float("MAIN.MouseSensitivity") * 0.15f;
                InvertLook = FixSettings().Bool("CAMERA.InvertLook");
                CollisionOffset = FixSettings().Float("CAMERA.CollisionOffset");
                MinCamDistance = FixSettings().Float("CAMERA.MinCamDistance");
                OnlyAllowHorizontalStickMovement = FixSettings().Bool("CAMERA.OnlyAllowHorizontalStickMovement");
                bHideCursorForMouseLook = (SteadyClock::now() - LastCameraInputTime) < HideCursorDuration;
            };
        };
    }
} Camera;