module;

#include <stdafx.h>
#include <cmath>
#include "FixSettings.hpp"

export module Camera;

import ComVars;
import Input;

// Free look for the chase cameras: the right stick or the mouse orbits the camera around the car. Like the
// car camera of GTA San Andreas, the look direction stays put in the world when the car turns. Once the
// camera is let go, it eases back to the game's own camera while the car moves.

namespace
{
    bool bFreeHeading = false;  // the free look has its own direction instead of the game's camera
    float WorldYaw = 0.0f;      // degrees, direction from the car to the camera in the world
    float PitchOffset = 0.0f;   // degrees, relative to the game's camera
    float HoldTime = 0.0f;      // seconds the camera keeps its direction after the last look input

    // Settings, also changed in the game's options menu
    const int* pFreeLook = nullptr;
    const int* pInvertLook = nullptr;
    const int* pOnlyHorizontalStick = nullptr;  // the right stick may be bound to vertical actions, such as shifting
    const float* pStickSensitivity = nullptr;
    const float* pMouseSensitivity = nullptr;
    const float* pReturnDelay = nullptr;
    const float* pReturnSpeed = nullptr;

    constexpr float StickDegreesPerSecond = 150.0f;
    constexpr float MouseDegreesPerCount = 0.15f;
    constexpr float CarMoveThreshold = 0.5f;

    float CarVelocityX = 0.0f;
    float CarVelocityY = 0.0f;
    bVector3 PrevCarPos = {};
    bool bPrevCarPosValid = false;

    uint8_t bLookBehind = 0;
    uint32_t nCameraMode = 0;

    using SteadyClock = std::chrono::steady_clock;

    constexpr float DEG2RAD = 0.0174532925f;

    float WrapDegrees(float angle)
    {
        angle = std::fmod(angle + 180.0f, 360.0f);
        return (angle < 0.0f ? angle + 360.0f : angle) - 180.0f;
    }

    // Under Proton/Gamescope (Steam Deck) the mouse cursor can't be warped back to the window centre,
    // so DirectInput keeps reporting the same offset every frame and the camera spins on its own.
    // A hand never moves a mouse by exactly the same amount for this many frames in a row.
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
        HoldTime = 0.0f;
        CarVelocityX = CarVelocityY = 0.0f;
        bPrevCarPosValid = false;
    }

    bool IsChaseCamera()
    {
        // Chase views; the bumper and hood views stay fixed
        return !bLookBehind && (nCameraMode == 2 || nCameraMode == 3);
    }

    injector::hook_back<void(__cdecl*)(bMatrix4*, bVector3*, bVector3*, bVector3*)> hb_eCreateLookAtMatrix;
    void __cdecl eCreateLookAtMatrix(bMatrix4* out, bVector3* from, bVector3* to, bVector3* up)
    {
        auto look = Input::ConsumeLook();

        // A new race, or the chase camera back after another view: the look starts behind the car, and
        // mouse movement made in the menus or while loading doesn't turn it
        static auto lastUpdate = SteadyClock::time_point{};
        auto now = SteadyClock::now();
        bool bRestart = now - lastUpdate > std::chrono::milliseconds(250);
        lastUpdate = now;
        if (bRestart)
        {
            ResetLook();
            look = {};
        }

        // The car was put somewhere else, such as back on the road
        if (bPrevCarPosValid && std::hypot(to->x - PrevCarPos.x, to->y - PrevCarPos.y) > 20.0f)
            ResetLook();

        if (!*pFreeLook || !IsChaseCamera() || GameFlowManager::IsPaused())
        {
            if (!*pFreeLook || !IsChaseCamera())
                ResetLook();
            return hb_eCreateLookAtMatrix.fun(out, from, to, up);
        }

        float dt = std::clamp(static_cast<float>(Sim::Internal::mLastFrameTime), 0.0f, 0.1f);

        // The car's velocity, smoothed a little against frame time noise
        if (bPrevCarPosValid && dt > 0.0f)
        {
            float blend = 1.0f - std::exp(-10.0f * dt);
            CarVelocityX += ((to->x - PrevCarPos.x) / dt - CarVelocityX) * blend;
            CarVelocityY += ((to->y - PrevCarPos.y) / dt - CarVelocityY) * blend;
        }
        PrevCarPos = *to;
        bPrevCarPosValid = true;

        // The game's camera: where it sits around the car
        float dx = from->x - to->x;
        float dy = from->y - to->y;
        float dz = from->z - to->z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        float gameYaw = std::atan2(dy, dx) / DEG2RAD;
        float gamePitch = std::atan2(dz, std::hypot(dx, dy)) / DEG2RAD;

        float yawInput = -look.stickX * StickDegreesPerSecond * (*pStickSensitivity) * dt;
        // The right stick only turns the camera sideways when its vertical axis drives the car, such as shifting in
        // the default pad controls, or when the option says so
        bool onlyHorizontalStick = *pOnlyHorizontalStick != 0 || Input::RightStickVerticalBound();
        float pitchInput = onlyHorizontalStick ? 0.0f : look.stickY * StickDegreesPerSecond * (*pStickSensitivity) * dt;

        if (MouseFilter.Accept(look.mouseX, look.mouseY))
        {
            yawInput -= look.mouseX * MouseDegreesPerCount * (*pMouseSensitivity);
            pitchInput += look.mouseY * MouseDegreesPerCount * (*pMouseSensitivity);
        }

        if ((*pInvertLook != 0))
            pitchInput = -pitchInput;

        // Like San Andreas: the look direction is a direction in the world, so turning the car doesn't
        // turn it. After the return delay without looking, the camera eases back behind the car.
        if (std::fabs(yawInput) > 0.001f || std::fabs(pitchInput) > 0.001f)
        {
            if (!bFreeHeading)
            {
                bFreeHeading = true;
                WorldYaw = gameYaw;
            }
            WorldYaw = WrapDegrees(WorldYaw + yawInput);
            PitchOffset = std::clamp(PitchOffset + pitchInput, -20.0f, 60.0f);
            HoldTime = std::max(0.0f, *pReturnDelay);
        }
        else if (bFreeHeading)
        {
            HoldTime -= dt;
            if (HoldTime <= 0.0f)
            {
                HoldTime = 0.0f;

                // Ease back to the game's own camera while the car moves. The game's camera is the target, so
                // there's no jump when it takes over again.
                if (std::hypot(CarVelocityX, CarVelocityY) > CarMoveThreshold)
                {
                    float blend = 1.0f - std::exp(-std::max(0.1f, *pReturnSpeed) * dt);
                    WorldYaw = WrapDegrees(WorldYaw + WrapDegrees(gameYaw - WorldYaw) * blend);
                    PitchOffset -= PitchOffset * blend;
                }

                // Back with the game's own camera once the difference can't be seen
                if (std::fabs(WrapDegrees(WorldYaw - gameYaw)) < 0.05f && std::fabs(PitchOffset) < 0.05f)
                {
                    bFreeHeading = false;
                    PitchOffset = 0.0f;
                }
            }
        }

        if (!bFreeHeading)
            return hb_eCreateLookAtMatrix.fun(out, from, to, up);

        // Put the camera at the look direction around the car, at the game's distance
        float yaw = WorldYaw * DEG2RAD;
        float pitch = std::clamp((gamePitch + PitchOffset) * DEG2RAD, -1.39626f, 1.39626f);

        from->x = to->x + dist * std::cos(pitch) * std::cos(yaw);
        from->y = to->y + dist * std::cos(pitch) * std::sin(yaw);
        from->z = to->z + dist * std::sin(pitch);
        hb_eCreateLookAtMatrix.fun(out, from, to, up);
    }
}

// Mouse cursor. The game shows it whenever the mouse moves, also while driving, where the mouse turns
// the camera. It's hidden while the race HUD is up and the game isn't paused, so the pause menu, the
// result screens and the front end still have it.
namespace
{
    SteadyClock::time_point LastHudUpdate = {};

    SafetyHookInline shHudUpdate = {};
    void __fastcall HudUpdate(void* hud, void* edx, void* player, float dt)
    {
        LastHudUpdate = SteadyClock::now();
        shHudUpdate.unsafe_thiscall(hud, player, dt);
    }

    SafetyHookInline shDrawCursor = {};
    void __cdecl DrawCursor(int x, int y)
    {
        bool driving = IsRacing() && !GameFlowManager::IsPaused() && SteadyClock::now() - LastHudUpdate < std::chrono::milliseconds(200);
        if (!driving && !Input::UsingGamepad())
            shDrawCursor.unsafe_ccall(x, y);
    }
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            // FEngHud::Update
            auto pattern = hook::pattern("56 8B F1 83 BE D4 02 00 00 00 0F 84 ? ? ? ? 80 BE 70 03 00 00 00");
            if (!pattern.empty())
                shHudUpdate = safetyhook::create_inline(pattern.get_first(), HudUpdate);

            pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? 80 3D ? ? ? ? ? 53 56");
            if (!pattern.empty() && shHudUpdate)
                shDrawCursor = safetyhook::create_inline(pattern.get_first(), DrawCursor);

            pFreeLook = &FixSettings().Int("CAMERA.FreeLook");
            pInvertLook = &FixSettings().Int("CAMERA.InvertLook");
            pOnlyHorizontalStick = &FixSettings().Int("CAMERA.OnlyAllowHorizontalStickMovement");
            pStickSensitivity = &FixSettings().Float("MAIN.GamepadCameraSpeed");
            pMouseSensitivity = &FixSettings().Float("MAIN.MouseSensitivity");
            pReturnDelay = &FixSettings().Float("CAMERA.ReturnDelay");
            pReturnSpeed = &FixSettings().Float("CAMERA.ReturnSpeed");

            // DrivingCameraMover::Update
            pattern = hook::pattern("E8 ? ? ? ? 0F 57 C0 83 C4 ? 0F 2F 86 ? ? ? ? 0F 82");
            hb_eCreateLookAtMatrix.fun = injector::MakeCALL(pattern.get_first(), eCreateLookAtMatrix, true).get();

            pattern = hook::pattern("8B 56 ? F3 0F 10 05 ? ? ? ? 88 44 24");
            static auto GetCameraData = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bLookBehind = *(uint8_t*)(regs.esi + 0xBD);
                nCameraMode = *(uint32_t*)(regs.esi + 0xC8);
            });
        };
    }
} Camera;
