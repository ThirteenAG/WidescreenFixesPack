module;

#include <stdafx.h>
#include <cmath>
#include "FixSettings.hpp"
#include "../Shared/NFS/FieldOfView.hpp"
#include <usercall.hpp>

export module Camera;

import ComVars;
import Input;

using namespace usercall;

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
    const float* pFovFactor = nullptr;           // Multiplier for the driving cameras' field of view

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
        // Far, close and hood views; the bumper and drift views stay fixed
        return !bLookBehind && (nCameraMode == 0 || nCameraMode == 1 || nCameraMode == 4);
    }

    injector::hook_back<void(__cdecl*)(bVector3*, bVector3*, bMatrix4*, bVector3*)> hb_eCreateLookAtMatrix;
    void CreateLookAtMatrix(bVector3* to, bVector3* from, bMatrix4* out, bVector3* up)
    {
        Call<reg::eax, reg::edi, reg::esi, reg::stack>(hb_eCreateLookAtMatrix.fun, to, from, out, up);
    }

    // The last update of the chase camera. The camera isn't updated while the game is paused; the pause counts
    // as updates, so the look direction is kept through it.
    SteadyClock::time_point LastLookUpdate = {};

    void __cdecl eCreateLookAtMatrix(bVector3* to, bVector3* from, bMatrix4* out, bVector3* up)
    {
        auto look = Input::ConsumeLook();

        // A new race, or the chase camera back after another view: the look starts behind the car, and
        // mouse movement made in the menus or while loading doesn't turn it
        auto now = SteadyClock::now();
        bool bRestart = now - LastLookUpdate > std::chrono::milliseconds(250);
        LastLookUpdate = now;
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
            return CreateLookAtMatrix(to, from, out, up);
        }

        // The game's time step of this frame, including the frame rate limiter's wait
        float dt = std::clamp(RealTimeElapsed.get(), 0.0f, 0.1f);

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
        float pitchInput = (*pOnlyHorizontalStick != 0) ? 0.0f : look.stickY * StickDegreesPerSecond * (*pStickSensitivity) * dt;

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
            return CreateLookAtMatrix(to, from, out, up);

        // Put the camera at the look direction around the car, at the game's distance
        float yaw = WorldYaw * DEG2RAD;
        float pitch = std::clamp((gamePitch + PitchOffset) * DEG2RAD, -1.39626f, 1.39626f);

        bVector3 orbit =
        {
            to->x + dist * std::cos(pitch) * std::cos(yaw),
            to->y + dist * std::cos(pitch) * std::sin(yaw),
            to->z + dist * std::sin(pitch)
        };
        *from = orbit;
        CreateLookAtMatrix(to, from, out, up);
    }

}

// Mouse cursor. The game shows it whenever the mouse moves, also while driving, where the mouse turns
// the camera. It's hidden while the race HUD is up and the game isn't paused, so the pause menu, the
// result screens and the front end still have it. It's also hidden while a pad is used.
namespace
{
    SteadyClock::time_point LastHudUpdate = {};

    SafetyHookInline shHudUpdate = {};
    void __stdcall HudUpdate(void* hud, void* player, void* car)
    {
        LastHudUpdate = SteadyClock::now();
        shHudUpdate.unsafe_stdcall(hud, player, car);
    }

    SafetyHookInline shDrawCursor = {};
    void __cdecl DrawCursor(int x, int y)
    {
        bool driving = !GameFlowManager::IsPaused() && SteadyClock::now() - LastHudUpdate < std::chrono::milliseconds(200);
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
            auto pattern = hook::pattern("53 55 8B 6C 24 0C 8B 5D 18 85 DB 57 8B 7C 24 14 74 06 57 E8");
            if (!pattern.empty())
                shHudUpdate = safetyhook::create_inline(pattern.get_first(), HudUpdate);

            pattern = hook::pattern("55 8B EC 83 E4 ? 81 EC ? ? ? ? A1 ? ? ? ? 53 8B 5D");
            if (!pattern.empty() && shHudUpdate)
                shDrawCursor = safetyhook::create_inline(pattern.get_first(), DrawCursor);
        };

        WFP::onInitEventAsync() += []()
        {
            pFreeLook = &FixSettings().Int("CAMERA.FreeLook");
            pInvertLook = &FixSettings().Int("CAMERA.InvertLook");
            pOnlyHorizontalStick = &FixSettings().Int("CAMERA.OnlyAllowHorizontalStickMovement");
            pStickSensitivity = &FixSettings().Float("MAIN.GamepadCameraSpeed");
            pMouseSensitivity = &FixSettings().Float("MAIN.MouseSensitivity");
            pReturnDelay = &FixSettings().Float("CAMERA.ReturnDelay");
            pReturnSpeed = &FixSettings().Float("CAMERA.ReturnSpeed");
            pFovFactor = &FixSettings().Float("MAIN.FOVFactor");

            // While paused the camera keeps its look direction, and the mouse moves the cursor, not the camera
            WFP::onMenuDrawingEvent() += []()
            {
                LastLookUpdate = SteadyClock::now();
                Input::ConsumeLook();
            };

            // CubicCameraMover::Update
            auto pattern = hook::pattern("E8 ? ? ? ? 8B 43 ? 50 8D BC 24");
            hb_eCreateLookAtMatrix.fun = MakeUsercall(pattern.get_first(), [](SafetyHookContext& regs)
            {
                eCreateLookAtMatrix((bVector3*)regs.eax, (bVector3*)regs.edi, (bMatrix4*)regs.esi, *(bVector3**)(regs.esp + 0x0));
            });

            // CubicCameraMover::Update, the camera mover of the driving views (chase, hood, bumper, drift and look
            // back), sets the camera's field of view (bAngle: 65536 = 360 degrees) from its smoothed value every
            // frame: the field-of-view multiplier applies to it. The front end's cameras are other
            // movers and stay as they are.
            pattern = hook::pattern("8B 4B 1C 66 89 81 C4 00 00 00 66 83 3F 03");
            static std::vector<SafetyHookMid> DrivingFOVHooks;
            NFSFieldOfView::InstallDrivingHooks(pattern, 3, 1, DrivingFOVHooks, +[](SafetyHookContext& regs)
            {
                regs.eax = NFSFieldOfView::ScaleDrivingAngle(regs.eax, *pFovFactor, float(0x7000));
            });

            pattern = hook::pattern("C6 43 ? ? 74 ? 8B 43 ? 85 C0 0F 84");
            static auto GetCameraData = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bLookBehind = *(uint8_t*)(regs.ebx + 0x45);
                nCameraMode = *(uint32_t*)(regs.ebx + 0x40);
            });
        };
    }
} Camera;
