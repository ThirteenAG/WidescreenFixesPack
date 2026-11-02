module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"

export module FreeCam;

import Camera;
import Main;
import Timer;
import Pad;
import Physical;
import Vehicle;
import Entity;
import Draw;

// MAIN.FreeCamera: an orbiting camera behind the vehicle (CCam::Process_Cam_On_A_String and Process_BehindBoat) and
// behind the player on foot (CCam::Process_FollowPed, the pad's camera; the mouse keeps the game's own). The right
// stick, or the mouse in a vehicle when it doesn't steer, turns it around freely; it stays where it was turned while
// the vehicle drives on, and after a moment without input it swings back behind the direction the vehicle or the
// player moves. Looking left, right and behind, the tank's and the fire truck's turrets, cutscenes and scripted
// cameras stay the game's.

namespace
{
    static_assert(offsetof(CCam, ResetStatics) == 0x0A);
    static_assert(offsetof(CCam, Mode) == 0x0C);
    static_assert(offsetof(CCam, Alpha) == 0xAC);
    static_assert(offsetof(CCam, FOV) == 0xB4);
    static_assert(offsetof(CCam, Beta) == 0xBC);
    static_assert(offsetof(CCam, Front) == 0x168);
    static_assert(offsetof(CCam, Source) == 0x174);
    static_assert(offsetof(CCam, CamTargetEntity) == 0x1B0);

    constexpr int16_t MI_FIRETRUCK = 137, MI_RHINO = 162;
    constexpr int LOOKING_FORWARD = 3;
    constexpr float DefaultNear = 0.9f;
    constexpr float DefaultFOV = 70.0f;

    struct CColPoint
    {
        CVector point;
        float pad0;
        CVector normal;
        float pad1;
        uint8_t surfaceA, pieceA, surfaceB, pieceB;
        float depth;
    };
    static_assert(sizeof(CColPoint) == 0x28);

    bool (__cdecl* ProcessLineOfSight)(const CVector&, const CVector&, CColPoint&, CEntity*&, bool, bool, bool, bool, bool, bool, bool, bool) = nullptr;
    CEntity* (__cdecl* TestSphereAgainstWorld)(CVector, float, CEntity*, bool, bool, bool, bool, bool, bool) = nullptr;
    bool (__cdecl* GetWaterLevelNoWaves)(float, float, float, float*) = nullptr;
    GameRef<CEntity*> pIgnoreEntity;
    uint8_t*** ModelInfoPtrs = nullptr;
    GameRef<bool> DisableMouseSteering;

    // The vehicle's collision box (min, max)
    bool BoundingBox(CEntity* entity, CVector& min, CVector& max)
    {
        auto info = (*ModelInfoPtrs)[entity->GetModelIndex()];
        if (!info)
            return false;
        auto col = *reinterpret_cast<uint8_t**>(info + 0x1C);
        if (!col)
            return false;
        min = *reinterpret_cast<CVector*>(col + 0x10);
        max = *reinterpret_cast<CVector*>(col + 0x1C);
        return true;
    }

    float Wrap(float a)
    {
        while (a > PI)
            a -= TWOPI;
        while (a < -PI)
            a += TWOPI;
        return a;
    }

    // Approaches target by a share that grows with the frame's length
    float Ease(float value, float target, float perStep, float steps)
    {
        float share = 1.0f - std::pow(1.0f - std::clamp(perStep, 0.0f, 1.0f), steps);
        return value + (target - value) * share;
    }

    float EaseAngle(float value, float target, float perStep, float steps)
    {
        float share = 1.0f - std::pow(1.0f - std::clamp(perStep, 0.0f, 1.0f), steps);
        return Wrap(value + Wrap(target - value) * share);
    }

    CVector Direction(float yaw, float pitch)
    {
        return CVector(std::cos(yaw) * std::cos(pitch), std::sin(yaw) * std::cos(pitch), std::sin(pitch));
    }

    RwCamera* SceneCamera()
    {
        return Scene.get_ptr() ? Scene->m_pRwCamera : nullptr;
    }

    // Keeps the camera out of walls: pulled in front of what lies between it and the target, the near plane
    // brought closer when that is close, and moved forward while its near plane still cuts into something
    void AvoidCollision(CCam* cam, const CVector& target, CEntity* ignore, bool withCars)
    {
        auto rwCamera = SceneCamera();
        if (!rwCamera)
            return;

        CColPoint point{};
        CEntity* hit = nullptr;
        *pIgnoreEntity = ignore;
        if (ProcessLineOfSight(target, cam->Source, point, hit, true, withCars, false, true, false, true, true, false) && hit)
        {
            cam->Source = point.point;
            float distance = (target - point.point).Magnitude();
            if (distance < DefaultNear + 0.3f)
                RwCameraSetNearClipPlane(rwCamera, std::max(distance - 0.3f, 0.05f));
        }
        *pIgnoreEntity = nullptr;

        CVector front = target - cam->Source;
        front.Normalise();
        float viewWidth = std::tan(DEGTORAD(cam->FOV) * 0.5f) * SCREEN_ASPECT_RATIO;
        for (int i = 0; i < 5; ++i)
        {
            float nearPlane = rwCamera->nearPlane;
            if (!TestSphereAgainstWorld(cam->Source + front * nearPlane, viewWidth * nearPlane, ignore, true, withCars, false, true, false, true))
                break;
            if (nearPlane > 0.15f)
                RwCameraSetNearClipPlane(rwCamera, std::max(nearPlane * 0.5f, 0.1f));
            else
                cam->Source += (target - cam->Source) * 0.3f;
        }
    }

    void KeepAboveWater(CCam* cam)
    {
        float level = 0.0f;
        if (GetWaterLevelNoWaves && GetWaterLevelNoWaves(cam->Source.x, cam->Source.y, cam->Source.z, &level) && cam->Source.z < level + 0.5f)
            cam->Source.z = level + 0.5f;
    }

    void Finish(CCam* cam, const CVector& target)
    {
        cam->Front = target - cam->Source;
        cam->Front.Normalise();
        cam->m_cvecTargetCoorsForFudgeInter = target;
        cam->Alpha = std::asin(std::clamp(cam->Front.z, -1.0f, 1.0f));
        cam->Beta = std::atan2(-cam->Front.y, -cam->Front.x);
        cam->ResetStatics = false;
        // Up, square to the view and without roll
        CVector right = CrossProduct(cam->Front, CVector(0.0f, 0.0f, 1.0f));
        if (right.MagnitudeSqr() < 1e-6f)
            right = CVector(1.0f, 0.0f, 0.0f);
        right.Normalise();
        cam->Up = CrossProduct(right, cam->Front);
        cam->Up.Normalise();
    }

    // The player's input for the camera: radians to turn this frame (yaw left positive, pitch up positive)
    bool CameraInput(bool mouseAllowed, float& yaw, float& pitch)
    {
        yaw = pitch = 0.0f;
        static auto& speed = FixSettings().Float("MAIN.GamepadCameraSpeed");
        float sx, sy;
        Pad::RightStick(sx, sy);
        if (sx || sy)
        {
            float dt = CTimer::ms_fTimeStep / 50.0f;
            auto curve = [](float v) { return v * std::abs(v); };
            yaw = -curve(sx) * DEGTORAD(150.0f) * speed * dt;
            pitch = curve(sy) * DEGTORAD(90.0f) * speed * dt;
            return true;
        }
        if (mouseAllowed && !Pad::IsPadLastDevice())
        {
            float mx, my;
            Pad::MouseDelta(mx, my);
            if (mx || my)
            {
                float sensitivity = Pad::MouseSensitivity();
                yaw = -mx * sensitivity * 2.5f;
                pitch = my * sensitivity * 2.5f;
                return true;
            }
        }
        return false;
    }

    struct OrbitState
    {
        float yaw = 0.0f;               // where the camera looks, 0 = +x
        float pitch = 0.0f;             // up positive
        uint32_t lastInput = 0;
        int lastDirection = LOOKING_FORWARD;
        CEntity* target = nullptr;
    };
    OrbitState CarOrbit, PedOrbit;

    bool FreeCameraOn()
    {
        static auto& freeCamera = FixSettings().Int("MAIN.FreeCamera");
        return freeCamera && !TheCamera->m_WideScreenOn && TheCamera->WhoIsInControlOfTheCamera == 0;
    }

    // Behind the vehicle: CCam::Process_Cam_On_A_String / Process_BehindBoat
    bool ProcessCar(CCam* cam, const CVector& cameraTarget)
    {
        auto entity = cam->CamTargetEntity;
        if (!FreeCameraOn() || !entity || !entity->IsVehicle() || !::FindPlayerVehicle || entity != reinterpret_cast<CEntity*>(::FindPlayerVehicle()))
            return false;
        auto model = entity->GetModelIndex();
        if (model == MI_RHINO || model == MI_FIRETRUCK)
            return false;
        auto car = reinterpret_cast<CVehicle*>(entity);
        CVector boxMin, boxMax;
        if (!BoundingBox(entity, boxMin, boxMax))
            return false;

        auto& orbit = CarOrbit;
        uint32_t now = CTimer::m_snTimeInMilliseconds;
        float steps = std::max(float(CTimer::ms_fTimeStep), 0.01f);
        const CVector& forward = car->GetForward();
        float carYaw = std::atan2(forward.y, forward.x);
        float carPitch = std::atan2(forward.z, std::sqrt(forward.x * forward.x + forward.y * forward.y));

        // Higher than the roof and as far as the vehicle is long, by the game's zoom
        float raise = std::max(boxMax.z * 1.3f - 0.4f, 0.0f);
        float length = 2.0f * std::abs(boxMin.y);
        float distance = std::max(TheCamera->CarZoomValueSmooth + 1.0f + length + raise, 3.5f);
        CVector target = cameraTarget;
        target.z += raise;
        float defaultPitch = -0.12f - 0.3f / distance * raise;

        // Just entered, back from a scripted camera, or back from looking aside or behind: behind the vehicle again
        if (cam->ResetStatics || orbit.target != entity || orbit.lastDirection != LOOKING_FORWARD || TheCamera->m_bJustCameOutOfGarage)
        {
            orbit.yaw = carYaw;
            orbit.pitch = defaultPitch + carPitch;
            orbit.lastInput = 0;
            orbit.target = entity;
            cam->FOV = DefaultFOV;
        }
        orbit.lastDirection = cam->DirectionWasLooking;

        float yawInput, pitchInput;
        bool mouseFree = *DisableMouseSteering;
        if (CameraInput(mouseFree, yawInput, pitchInput))
        {
            orbit.yaw = Wrap(orbit.yaw + yawInput);
            orbit.pitch += pitchInput;
            orbit.lastInput = now;
        }
        else if (now - orbit.lastInput > 1200)
        {
            // Swing back behind where the vehicle goes; reversing keeps the camera behind it
            const CVector& speed = car->GetMoveSpeed();
            float speed2D = std::sqrt(speed.x * speed.x + speed.y * speed.y);
            float forwardSpeed = DotProduct(speed, forward);
            if (speed2D > 0.02f)
            {
                float goal = forwardSpeed >= 0.0f ? std::atan2(speed.y, speed.x) : carYaw;
                float rate = 0.015f + 0.1f * std::min(speed2D, 1.0f);
                orbit.yaw = EaseAngle(orbit.yaw, goal, rate, steps);
                orbit.pitch = Ease(orbit.pitch, defaultPitch + carPitch, rate, steps);
            }
        }
        orbit.pitch = std::clamp(orbit.pitch, -DEGTORAD(70.0f), DEGTORAD(40.0f));

        // The field of view: the game's, widened with speed when that option is on
        static auto& speedFov = FixSettings().Int("MISC.CarSpeedDependantFOV");
        if (speedFov && !cam->ResetStatics)
        {
            float forwardSpeed = DotProduct(forward, car->GetMoveSpeed());
            if (forwardSpeed > 0.4f)
                cam->FOV += (forwardSpeed - 0.4f) * CTimer::ms_fTimeStep;
            if (cam->FOV > DefaultFOV)
                cam->FOV = std::pow(0.98f, CTimer::ms_fTimeStep) * (cam->FOV - DefaultFOV) + DefaultFOV;
            cam->FOV = std::clamp(cam->FOV, DefaultFOV, DefaultFOV + 30.0f);
        }
        else
            cam->FOV = DefaultFOV;

        cam->Source = target - Direction(orbit.yaw, orbit.pitch) * distance;
        AvoidCollision(cam, target, entity, car->GetMoveSpeed().Magnitude() < 0.1f);
        KeepAboveWater(cam);
        Finish(cam, target);
        return true;
    }

    // Behind the player on foot: CCam::Process_FollowPed, while the pad drives the console camera
    bool ProcessPed(CCam* cam, const CVector& cameraTarget, float orientation)
    {
        auto entity = cam->CamTargetEntity;
        if (!FreeCameraOn() || !entity || !entity->IsPed() || !FindPlayerPed || entity != FindPlayerPed() || !Pad::IsPadLastDevice())
            return false;

        auto& orbit = PedOrbit;
        uint32_t now = CTimer::m_snTimeInMilliseconds;
        float steps = std::max(float(CTimer::ms_fTimeStep), 0.01f);
        float distance = 2.0f + TheCamera->m_fPedZoomValueSmooth;
        CVector target = cameraTarget;
        target.z += 0.75f;
        // The player's heading: orientation is the way the player faces (0 = +y)
        float pedYaw = orientation + HALFPI;

        if (cam->ResetStatics || orbit.target != entity)
        {
            CVector toTarget = target - cam->Source;
            orbit.yaw = (toTarget.x || toTarget.y) ? std::atan2(toTarget.y, toTarget.x) : pedYaw;
            orbit.pitch = -0.1f;
            orbit.lastInput = 0;
            orbit.target = entity;
        }
        if (TheCamera->m_bUseTransitionBeta)
            orbit.yaw = Wrap(cam->m_fTransitionBeta + PI);

        float yawInput, pitchInput;
        if (CameraInput(false, yawInput, pitchInput))
        {
            orbit.yaw = Wrap(orbit.yaw + yawInput);
            orbit.pitch += pitchInput;
            orbit.lastInput = now;
        }
        else if (now - orbit.lastInput > 2000)
        {
            const CVector& speed = reinterpret_cast<CPhysical*>(entity)->GetMoveSpeed();
            float speed2D = std::sqrt(speed.x * speed.x + speed.y * speed.y);
            if (speed2D > 0.03f)
                orbit.yaw = EaseAngle(orbit.yaw, pedYaw, 0.01f + 0.05f * std::min(speed2D * 5.0f, 1.0f), steps);
            orbit.pitch = Ease(orbit.pitch, -0.1f, 0.02f, steps);
        }
        orbit.pitch = std::clamp(orbit.pitch, -DEGTORAD(70.0f), DEGTORAD(60.0f));

        cam->FOV = DefaultFOV;
        cam->Source = target - Direction(orbit.yaw, orbit.pitch) * distance;
        AvoidCollision(cam, target, entity, true);
        Finish(cam, target);
        return true;
    }

    SafetyHookInline shProcessCamOnAString, shProcessBehindBoat, shProcessFollowPed;

    void __fastcall ProcessCamOnAString(CCam* cam, void* edx, const CVector& target, float orientation, float a, float b)
    {
        if (!ProcessCar(cam, target))
        {
            CarOrbit.target = nullptr;
            shProcessCamOnAString.unsafe_thiscall(cam, &target, orientation, a, b);
        }
    }

    void __fastcall ProcessBehindBoat(CCam* cam, void* edx, const CVector& target, float orientation, float a, float b)
    {
        if (!ProcessCar(cam, target))
        {
            CarOrbit.target = nullptr;
            shProcessBehindBoat.unsafe_thiscall(cam, &target, orientation, a, b);
        }
    }

    void __fastcall ProcessFollowPed(CCam* cam, void* edx, const CVector& target, float orientation, float a, float b)
    {
        if (!ProcessPed(cam, target, orientation))
        {
            PedOrbit.target = nullptr;
            shProcessFollowPed.unsafe_thiscall(cam, &target, orientation, a, b);
        }
    }
}

class FreeCam
{
public:
    FreeCam()
    {
        WFP::onGameInitEvent() += []()
        {
            auto lineOfSight = hook::pattern("53 56 57 55 81 EC 60 02 00 00 66 83 3D ? ? ? ? FF 73");
            auto sphere = hook::pattern("53 56 57 55 83 EC 38 8B 5C");
            auto water = hook::pattern("D9 05 ? ? ? ? 83 EC 10 8B 54");
            auto followPed = hook::pattern("53 56 57 55 89 CD 81 EC F0 00");
            auto onAString = hook::pattern("53 56 89 CB 57 55 81 EC 20");
            auto behindBoat = hook::pattern("53 56 89 CB 57 55 ? ? ? ? 00 00 8B BB B0 01 00 00 8A 47 50 8B 94");
            // CWorld::pIgnoreEntity, set before the line of sight in CCam::Process_FollowPedWithMouse
            auto ignore = hook::pattern("A3 ? ? ? ? 8D 84 24 84");
            // CModelInfo::ms_modelInfoPtrs, read by Process_Cam_On_A_String for the vehicle's box
            auto modelInfo = hook::pattern("8B 04 95 ? ? ? ? 8B 40 1C 83 C0 1C");
            // CVehicle::m_bDisableMouseSteering, read by CAutomobile::ProcessControlInputs
            auto mouseSteering = hook::pattern("80 3D ? ? ? ? 00 0F 85 66");
            ProcessLineOfSight = reinterpret_cast<decltype(ProcessLineOfSight)>(lineOfSight.get_first());
            TestSphereAgainstWorld = reinterpret_cast<decltype(TestSphereAgainstWorld)>(sphere.get_first());
            GetWaterLevelNoWaves = reinterpret_cast<decltype(GetWaterLevelNoWaves)>(water.get_first());
            pIgnoreEntity.SetAddress(*ignore.get_first<CEntity**>(1));
            ModelInfoPtrs = reinterpret_cast<uint8_t***>(modelInfo.get_first(3));
            DisableMouseSteering.SetAddress(*mouseSteering.get_first<bool*>(2));

            shProcessFollowPed = safetyhook::create_inline(followPed.get_first(), ProcessFollowPed);
            shProcessCamOnAString = safetyhook::create_inline(onAString.get_first(), ProcessCamOnAString);
            shProcessBehindBoat = safetyhook::create_inline(behindBoat.get_first(), ProcessBehindBoat);
        };
    }
} FreeCam;
