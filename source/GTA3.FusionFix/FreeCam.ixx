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

// MAIN.FreeCamera: the rotating cameras of the later games.
// - On foot (CCam::Process_FollowPed): the right stick or the mouse turns the camera around the player, who walks and
//   runs where the stick or the keys point, under the camera. The camera goes behind the player on the camera-behind
//   button and while shooting. Aiming (LT in the modern layout, the target or fire button with the mouse) is the
//   game's own mouse aiming: the player turns with the camera and fires at the crosshair (see Pad.ixx).
// - In a vehicle (CCam::Process_Cam_On_A_String and Process_BehindBoat): a chase camera that swings behind the way the
//   vehicle moves, with distances, heights and limits for each kind of vehicle. The right stick, or the mouse when it
//   doesn't steer, turns it; the tank's and the fire truck's turrets follow it.
// Looking left, right and behind, cutscenes and scripted cameras stay the game's.

namespace
{
    static_assert(offsetof(CCam, ResetStatics) == 0x0A);
    static_assert(offsetof(CCam, Alpha) == 0xA8);
    static_assert(offsetof(CCam, FOV) == 0xB0);
    static_assert(offsetof(CCam, Beta) == 0xB8);
    static_assert(offsetof(CCam, m_cvecTargetCoorsForFudgeInter) == 0xF8);
    static_assert(offsetof(CCam, Front) == 0x140);
    static_assert(offsetof(CCam, Source) == 0x14C);
    static_assert(offsetof(CCam, CamTargetEntity) == 0x188);

    constexpr int16_t MI_FIRETRUCK = 97, MI_RHINO = 122, MI_DODO = 126, MI_RCBANDIT = 131, MI_YARDIE = 135;
    constexpr int16_t MODE_CAM_ON_A_STRING = 18;
    constexpr int LOOKING_FORWARD = 3;
    constexpr int CAM_ZOOM_1 = 1, CAM_ZOOM_2 = 2, CAM_ZOOM_3 = 3;
    constexpr int WEAPON_UNARMED = 0, WEAPON_BASEBALLBAT = 1, WEAPON_DETONATOR = 12;
    constexpr float DefaultNear = 0.9f;
    constexpr float DefaultFOV = 70.0f;
    constexpr float TweakFOV = 1.05f;
    constexpr float StickSensitivity = 0.01f;

    // CAutomobile
    constexpr size_t HeightAboveRoad = 0x50C, CarGunLR = 0x580, CarGunUD = 0x584, WheelsOnGround = 0x590;

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

    bool (__cdecl* ProcessLineOfSight)(const CVector&, const CVector&, CColPoint&, CEntity*&, bool, bool, bool, bool, bool, bool, bool) = nullptr;
    CEntity* (__cdecl* TestSphereAgainstWorld)(CVector, float, CEntity*, bool, bool, bool, bool, bool, bool) = nullptr;
    bool (__cdecl* GetWaterLevelNoWaves)(float, float, float, float*) = nullptr;
    void (__fastcall* GetVectorsReadyForRW)(CCam*, void*) = nullptr;
    GameRef<CEntity*> pIgnoreEntity;
    CColPoint* TempSphereColPoints = nullptr;
    uint8_t*** ModelInfoPtrs = nullptr;
    GameRef<bool> DisableMouseSteering;

    template <typename T>
    T& Field(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(object) + offset);
    }

    // The vehicle's collision box (min, max)
    bool BoundingBox(CEntity* entity, CVector& min, CVector& max)
    {
        auto info = (*ModelInfoPtrs)[entity->GetModelIndex()];
        if (!info)
            return false;
        auto col = *reinterpret_cast<uint8_t**>(info + 0x1C);
        if (!col)
            return false;
        min = *reinterpret_cast<CVector*>(col + 0x14);
        max = *reinterpret_cast<CVector*>(col + 0x20);
        return true;
    }

    float LimitRadianAngle(float angle)
    {
        angle = std::clamp(angle, -25.0f, 25.0f);
        while (angle >= PI)
            angle -= TWOPI;
        while (angle < -PI)
            angle += TWOPI;
        return angle;
    }

    // The angle of (x, y), 0..2pi
    float ATanOfXY(float x, float y)
    {
        float angle = std::atan2(y, x);
        return angle < 0.0f ? angle + TWOPI : angle;
    }

    // Moves a value towards a target with a speed that accelerates and is capped
    void WellBufferMe(float target, float* value, float* speed, float maxSpeed, float acceleration, bool isAngle)
    {
        float step = CTimer::ms_fTimeStep;
        float delta = target - *value;
        if (isAngle)
        {
            while (delta >= PI)
                delta -= TWOPI;
            while (delta < -PI)
                delta += TWOPI;
        }
        float targetSpeed = delta * maxSpeed;
        *speed += acceleration * (targetSpeed - *speed) * step;
        if (targetSpeed < 0.0f && *speed < targetSpeed)
            *speed = targetSpeed;
        else if (targetSpeed > 0.0f && *speed > targetSpeed)
            *speed = targetSpeed;
        *value += *speed * std::min(10.0f, step);
    }

    // The target averaged over the last frames, as CCam::DoAverageOnVector
    CVector DoAverageOnVector(CCam* cam, const CVector& vec)
    {
        if (cam->ResetStatics)
        {
            cam->m_iRunningVectorArrayPos = 0;
            cam->m_iRunningVectorCounter = 1;
        }
        if (cam->m_iRunningVectorCounter == 3)
        {
            cam->m_arrPreviousVectors[0] = cam->m_arrPreviousVectors[1];
            cam->m_arrPreviousVectors[1] = vec;
        }
        else
            cam->m_arrPreviousVectors[cam->m_iRunningVectorArrayPos] = vec;

        CVector average(0.0f, 0.0f, 0.0f);
        int i = 0;
        for (; i <= cam->m_iRunningVectorArrayPos; i++)
            average += cam->m_arrPreviousVectors[i];
        average /= float(i);

        cam->m_iRunningVectorArrayPos = std::min(cam->m_iRunningVectorArrayPos + 1, int(std::size(cam->m_arrPreviousVectors)) - 1);
        cam->m_iRunningVectorCounter = std::min(cam->m_iRunningVectorCounter + 1, int(std::size(cam->m_arrPreviousVectors)) + 1);
        return average;
    }

    RwCamera* SceneCamera()
    {
        return Scene.get_ptr() ? Scene->m_pRwCamera : nullptr;
    }

    // Keeps the camera out of what lies between it and the target, and its near plane out of walls
    void ClipSource(CCam* cam, RwCamera* rwCamera, const CVector& target, float distance, bool vehicles, bool peds,
        bool seeThrough, bool sphereIgnoreSomeObjects)
    {
        CColPoint point{};
        CEntity* entity = nullptr;
        *pIgnoreEntity = cam->CamTargetEntity;
        if (ProcessLineOfSight(target, cam->Source, point, entity, true, vehicles, peds, true, false, seeThrough, true) && entity)
        {
            float pedColDist = (target - point.point).Magnitude();
            float colCamDist = distance - pedColDist;
            if (entity->IsPed() && colCamDist > DefaultNear + 0.1f)
            {
                // A pedestrian in the way that the camera doesn't pass through
                if (ProcessLineOfSight(point.point, cam->Source, point, entity, true, vehicles, peds, true, false, seeThrough, true))
                {
                    pedColDist = (target - point.point).Magnitude();
                    cam->Source = point.point;
                    if (pedColDist < DefaultNear + 0.3f)
                        RwCameraSetNearClipPlane(rwCamera, std::max(pedColDist - 0.3f, 0.05f));
                }
                else
                    RwCameraSetNearClipPlane(rwCamera, std::min(colCamDist - 0.35f, DefaultNear));
            }
            else
            {
                cam->Source = point.point;
                if (pedColDist < DefaultNear + 0.3f)
                    RwCameraSetNearClipPlane(rwCamera, std::max(pedColDist - 0.3f, 0.05f));
            }
        }
        *pIgnoreEntity = nullptr;

        float viewPlaneHeight = std::tan(DEGTORAD(cam->FOV) / 2.0f);
        float viewPlaneWidth = viewPlaneHeight * SCREEN_ASPECT_RATIO * TweakFOV;
        float nearPlane = rwCamera->nearPlane;
        float radius = viewPlaneWidth * nearPlane;
        entity = TestSphereAgainstWorld(cam->Source + cam->Front * nearPlane, radius, nullptr, true, true, false, true, false, sphereIgnoreSomeObjects);
        for (int i = 0; entity && i <= 5; ++i)
        {
            CVector camToCol = TempSphereColPoints[0].point - cam->Source;
            float frontDist = DotProduct(camToCol, cam->Front);
            float dist = (camToCol - cam->Front * frontDist).Magnitude() / viewPlaneWidth;

            // A nearer near plane, or a step forward
            dist = std::max(std::min(nearPlane, dist), 0.1f);
            if (dist < nearPlane)
                RwCameraSetNearClipPlane(rwCamera, dist);
            if (dist == 0.1f)
                cam->Source += (target - cam->Source) * 0.3f;

            nearPlane = rwCamera->nearPlane;
            radius = viewPlaneWidth * nearPlane;
            entity = TestSphereAgainstWorld(cam->Source + cam->Front * nearPlane, radius, nullptr, true, true, false, true, false, sphereIgnoreSomeObjects);
        }
    }

    bool FreeCameraOn()
    {
        static auto& freeCamera = FixSettings().Int("MAIN.FreeCamera");
        return freeCamera && !TheCamera->m_WideScreenOn && TheCamera->WhoIsInControlOfTheCamera == 0;
    }

    // Behind the player on foot, turned by the right stick or the mouse
    bool ProcessPed(CCam* cam, const CVector& cameraTarget, float targetOrientation)
    {
        auto entity = cam->CamTargetEntity;
        auto rwCamera = SceneCamera();
        // With GInput the pad's camera is GInput's
        if (!FreeCameraOn() || Pad::IsGInputLoaded() || !rwCamera || !entity || !entity->IsPed() || !FindPlayerPed || entity != FindPlayerPed())
            return false;

        cam->FOV = DefaultFOV;
        constexpr float MinDist = 2.0f;
        const float MaxDist = 2.0f + TheCamera->m_fPedZoomValueSmooth;
        constexpr float BaseOffset = 0.75f;     // the camera's height above the target, so that alpha evens out at 0

        CVector targetCoors = cameraTarget;
        targetCoors.z += cam->m_fSyphonModeTargetZOffSet;
        targetCoors = DoAverageOnVector(cam, targetCoors);
        targetCoors.z += BaseOffset;

        CVector dist = cam->Source - targetCoors;

        auto weapon = Pad::PlayerWeapon();
        bool shooting = weapon != WEAPON_UNARMED && weapon != WEAPON_DETONATOR && weapon != WEAPON_BASEBALLBAT && Pad::Fire();

        if (cam->ResetStatics)
        {
            // Keep the direction the camera had, level
            cam->Beta = ATanOfXY(dist.x, dist.y);
            cam->Alpha = 0.0f;
            dist = MaxDist * CVector(std::cos(cam->Alpha) * std::cos(cam->Beta), std::cos(cam->Alpha) * std::sin(cam->Beta), std::sin(cam->Alpha));
            cam->Source = targetCoors + dist;
            cam->ResetStatics = false;
        }

        // Dragged along at a distance between the minimum and the zoom's
        float camDist = dist.Magnitude();
        if (camDist == 0.0f)
            dist = CVector(1.0f, 1.0f, 0.0f);
        else if (camDist < MinDist)
            dist *= MinDist / camDist;
        else if (camDist > MaxDist)
            dist *= MaxDist / camDist;
        camDist = dist.Magnitude();

        // Beta 0 looks east, half pi north; alpha positive looks up
        float groundDist = dist.Magnitude2D();
        cam->Beta = LimitRadianAngle(ATanOfXY(-dist.x, -dist.y));
        cam->Alpha = LimitRadianAngle(ATanOfXY(groundDist, -dist.z));

        // Looking around: the mouse turns it by its own sensitivity, the stick by its speed
        float betaOffset = 0.0f, alphaOffset = 0.0f;
        float stickX, stickY;
        Pad::RightStick(stickX, stickY);
        float mouseX, mouseY;
        Pad::MouseDelta(mouseX, mouseY);
        if (!Pad::IsPadLastDevice() && (mouseX || mouseY) && !Pad::ControlsDisabled())
        {
            float accelX, accelY;
            Pad::MouseAccel(accelX, accelY);
            betaOffset = -2.5f * mouseX * accelX * cam->FOV / 80.0f;
            alphaOffset = 4.0f * mouseY * accelY * cam->FOV / 80.0f;
        }
        else if (stickX || stickY)
        {
            static auto& speed = FixSettings().Float("MAIN.GamepadCameraSpeed");
            // The stick past the game's own dead zone, in its units
            float lookLeftRight = -stickX * 170.0f * speed;
            float lookUpDown = stickY * 170.0f * speed;
            betaOffset = lookLeftRight * StickSensitivity * (1.0f / 20.0f) * cam->FOV / 80.0f * CTimer::ms_fTimeStep;
            alphaOffset = lookUpDown * StickSensitivity * (0.6f / 20.0f) * cam->FOV / 80.0f * CTimer::ms_fTimeStep;
        }

        // Turning by hand stops the swing behind
        if (betaOffset)
            cam->Rotating = false;

        cam->Beta = LimitRadianAngle(cam->Beta + betaOffset);
        cam->Alpha = std::clamp(cam->Alpha + alphaOffset, -DEGTORAD(89.5f), DEGTORAD(45.0f));

        float betaDiff = LimitRadianAngle(targetOrientation + PI - cam->Beta);
        float targetAlpha = cam->Alpha;
        // Walking towards the camera keeps the view near level (12 degrees for the height offset)
        constexpr float AlphaLimitUp = DEGTORAD(15.0f) + DEGTORAD(12.0f);
        constexpr float AlphaLimitDown = -DEGTORAD(15.0f) + DEGTORAD(12.0f);
        if (std::abs(betaDiff) < DEGTORAD(25.0f) && static_cast<CPhysical*>(entity)->GetMoveSpeed().Magnitude2D() > 0.01f)
            targetAlpha = std::clamp(targetAlpha, AlphaLimitDown, AlphaLimitUp);

        WellBufferMe(targetAlpha, &cam->Alpha, &cam->AlphaSpeed, 0.2f, 0.1f, true);

        if (Pad::CameraBehind() || shooting)
        {
            cam->m_fTargetBeta = targetOrientation;
            cam->Rotating = true;
        }

        if (cam->Rotating)
        {
            WellBufferMe(cam->m_fTargetBeta, &cam->Beta, &cam->BetaSpeed, 0.1f, 0.06f, true);
            if (std::abs(LimitRadianAngle(cam->m_fTargetBeta - cam->Beta)) < 0.06f)
                cam->Rotating = false;
        }

        if (TheCamera->m_bUseTransitionBeta)
            cam->Beta = ATanOfXY(-std::cos(cam->m_fTransitionBeta), -std::sin(cam->m_fTransitionBeta));

        cam->Front = CVector(std::cos(cam->Alpha) * std::cos(cam->Beta), std::cos(cam->Alpha) * std::sin(cam->Beta), std::sin(cam->Alpha));
        cam->Source = targetCoors - cam->Front * camDist;
        targetCoors.z -= BaseOffset;
        cam->m_cvecTargetCoorsForFudgeInter = targetCoors;
        cam->Front = targetCoors - cam->Source;
        cam->Front.Normalise();

        ClipSource(cam, rwCamera, targetCoors, camDist, true, true, false, false);
        GetVectorsReadyForRW(cam, nullptr);
        return true;
    }

    // The distance, height and limits for each kind of vehicle: 0 cars, 1 bikes, 2 helicopters, 3 planes, 4 boats,
    // 5 remote control cars, 6 remote control aircraft, 7 the fire truck
    constexpr float CarCamSet[8][15] =
    {
        { 1.3f, 1.0f, 0.4f, 10.0f, 15.0f, 0.5f, 1.0f, 1.0f, 0.85f, 0.2f, 0.075f, 0.05f, 0.8f, DEGTORAD(45.0f), DEGTORAD(89.0f) },
        { 1.1f, 1.0f, 0.1f, 10.0f, 11.0f, 0.5f, 1.0f, 1.0f, 0.85f, 0.2f, 0.075f, 0.05f, 0.75f, DEGTORAD(45.0f), DEGTORAD(89.0f) },
        { 1.1f, 1.0f, 0.2f, 10.0f, 15.0f, 0.05f, 0.05f, 0.0f, 0.9f, 0.05f, 0.01f, 0.05f, 1.0f, DEGTORAD(10.0f), DEGTORAD(70.0f) },
        { 1.1f, 1.0f, 0.2f, 10.0f, 15.0f, 0.5f, 1.0f, 1.0f, 0.75f, 0.1f, 0.005f, 0.2f, 1.0f, DEGTORAD(89.0f), DEGTORAD(89.0f) },
        { 0.9f, 1.0f, 0.1f, 10.0f, 15.0f, 0.5f, 1.0f, 0.0f, 0.9f, 0.05f, 0.005f, 0.05f, 1.0f, -0.2f, DEGTORAD(70.0f) },
        { 1.1f, 1.0f, 0.2f, 10.0f, 5.0f, 0.5f, 1.0f, 1.0f, 0.75f, 0.1f, 0.005f, 0.2f, 1.0f, DEGTORAD(45.0f), DEGTORAD(89.0f) },
        { 1.1f, 1.0f, 0.2f, 10.0f, 5.0f, 0.5f, 1.0f, 1.0f, 0.75f, 0.1f, 0.005f, 0.2f, 1.0f, DEGTORAD(20.0f), DEGTORAD(70.0f) },
        { 1.3f, 1.0f, 0.4f, 10.0f, 15.0f, 0.5f, 1.0f, 1.0f, 0.85f, 0.2f, 0.075f, 0.05f, 0.8f, -0.18f, DEGTORAD(40.0f) },
    };
    constexpr float ZoomOneAlphaOffset[] = { 0.12f, 0.08f, 0.15f, 0.08f, 0.08f };
    constexpr float ZoomTwoAlphaOffset[] = { 0.1f, 0.08f, 0.3f, 0.08f, 0.08f };
    constexpr float ZoomThreeAlphaOffset[] = { 0.065f, 0.05f, 0.15f, 0.06f, 0.08f };
    // The zoom steps of this camera, for cars and for boats
    constexpr float CarZoom[] = { -1.0f, 2.0f, 6.0f };
    constexpr float BoatZoom[] = { -2.41f, 6.49f, 15.0f };

    struct CarState
    {
        CVector historyOne, historyTwo;
        float lastBeta = -9999.0f;
        float lastAlpha = -9999.0f;
        float stepsLeftToChangeBetaByMouse = 0.0f;
        float dontCollideWithCars = 0.0f;
        bool alphaCorrected = false;
        float zoom = 0.0f;
        uint32_t lastTime = 0;
    } Car;

    // The zoom of the game's three steps in this camera's distances
    float CarZoomValue(bool boat, bool reset)
    {
        if (TheCamera->m_bUseScriptZoomValueCar || TheCamera->m_bPlayerIsInGarage)
            return TheCamera->CarZoomValueSmooth;
        int step = std::clamp(TheCamera->CarZoomIndicator, CAM_ZOOM_1, CAM_ZOOM_3) - CAM_ZOOM_1;
        float target = boat ? BoatZoom[step] : CarZoom[step];
        if (reset)
            Car.zoom = target;
        else if (Car.zoom < target)
            Car.zoom = std::min(Car.zoom + 0.12f * CTimer::ms_fTimeStep, target);
        else
            Car.zoom = std::max(Car.zoom - 0.12f * CTimer::ms_fTimeStep, target);
        return Car.zoom;
    }

    float HeightAboveRoadOf(CVehicle* car, const CVector& boxMin)
    {
        return car->IsCar() ? Field<float>(car, HeightAboveRoad) : -boxMin.z;
    }

    // The tank's turret and the fire truck's water cannon turn towards where the camera looks
    void TurretFollowsCamera(CCam* cam, CVehicle* car)
    {
        auto model = car->GetModelIndex();
        float& gunLR = Field<float>(car, CarGunLR);
        CVector hi = Multiply3x3(cam->Front, car->GetMatrix());
        float angleToFace = model == MI_FIRETRUCK ? -hi.Heading() : hi.Heading();
        if (angleToFace > gunLR + PI)
            angleToFace -= TWOPI;
        else if (angleToFace < gunLR - PI)
            angleToFace += TWOPI;

        float neededTurn = angleToFace - gunLR;
        float turnPerFrame = CTimer::ms_fTimeStep * (model == MI_FIRETRUCK ? 0.05f : 0.015f);
        if (neededTurn > turnPerFrame)
            angleToFace = gunLR + turnPerFrame;
        else if (neededTurn < -turnPerFrame)
            angleToFace = gunLR - turnPerFrame;
        gunLR = angleToFace;
        if (gunLR < -PI)
            gunLR += TWOPI;
        else if (gunLR > PI)
            gunLR -= TWOPI;

        if (model == MI_FIRETRUCK)
        {
            float& gunUD = Field<float>(car, CarGunUD);
            float alphaToFace = std::atan2(hi.z, hi.Magnitude2D()) + DEGTORAD(15.0f);
            float neededAlphaTurn = alphaToFace - gunUD;
            float alphaTurnPerFrame = CTimer::ms_fTimeStep * 0.02f;
            if (neededAlphaTurn > alphaTurnPerFrame)
                gunUD += alphaTurnPerFrame;
            else if (neededAlphaTurn < -alphaTurnPerFrame)
                gunUD -= alphaTurnPerFrame;
            else
                gunUD = alphaToFace;
            gunUD = std::clamp(gunUD, 0.05f, 0.3f);
        }
    }

    // Behind the vehicle
    bool ProcessCar(CCam* cam, const CVector& cameraTarget)
    {
        auto entity = cam->CamTargetEntity;
        auto rwCamera = SceneCamera();
        if (!FreeCameraOn() || !rwCamera || !entity || !entity->IsVehicle() || !FindPlayerVehicle || entity != FindPlayerVehicle())
            return false;
        auto car = static_cast<CVehicle*>(entity);
        CVector boxMin, boxMax;
        if (!BoundingBox(entity, boxMin, boxMax))
            return false;

        float step = CTimer::ms_fTimeStep;
        auto model = car->GetModelIndex();
        CVector targetCoors = cameraTarget;
        bool isPlane = model == MI_DODO;
        bool isCar = car->IsCar() && !isPlane;

        // The player looks forward
        bool nextDirectionIsForward = !(Pad::LookBehindForCar() || Pad::LookBehindForPed() || Pad::LookLeft() || Pad::LookRight()) &&
            cam->DirectionWasLooking == LOOKING_FORWARD;

        int set = 0;
        if (model == MI_FIRETRUCK)
            set = 7;
        else if (model == MI_RCBANDIT)
            set = 5;
        else if (car->IsBoat())
            set = 4;
        else if (isPlane)
            set = 3;
        const float* params = CarCamSet[set];

        int alphaSet = set > 4 ? (isPlane ? 3 : 0) : set;
        float zoomModeAlphaOffset = 0.0f;
        switch (TheCamera->CarZoomIndicator)
        {
            case CAM_ZOOM_1: zoomModeAlphaOffset = ZoomOneAlphaOffset[alphaSet]; break;
            case CAM_ZOOM_2: zoomModeAlphaOffset = ZoomTwoAlphaOffset[alphaSet]; break;
            case CAM_ZOOM_3: zoomModeAlphaOffset = ZoomThreeAlphaOffset[alphaSet]; break;
            default: break;
        }

        float colMaxZ = boxMax.z;
        float approxCarLength = 2.0f * std::abs(boxMin.y);
        float newDistance = CarZoomValue(car->IsBoat(), cam->ResetStatics) + params[1] + approxCarLength;
        float minDistForThisCar = approxCarLength * params[3];

        float radiusToStayOutside = colMaxZ * params[0] - params[2];
        if (radiusToStayOutside > 0.0f)
        {
            targetCoors.z += radiusToStayOutside;
            newDistance += radiusToStayOutside;
            zoomModeAlphaOffset += 0.3f / newDistance * radiusToStayOutside;
        }

        float minDistForVehType = params[4];
        if (TheCamera->CarZoomIndicator == CAM_ZOOM_1 && (set < 2 || set == 7))
            minDistForVehType *= 0.65f;
        float nextDistance = std::max(newDistance, minDistForVehType);

        cam->CA_MAX_DISTANCE = newDistance;
        cam->CA_MIN_DISTANCE = 3.5f;

        // The field of view: wider with speed when that option is on
        static auto& speedFov = FixSettings().Int("MISC.CarSpeedDependantFOV");
        if (cam->ResetStatics || !speedFov)
        {
            cam->FOV = DefaultFOV;
            if (cam->ResetStatics && TheCamera->m_bIdleOn)
                TheCamera->m_uiTimeWeEnteredIdle = CTimer::m_snTimeInMilliseconds;
        }
        else
        {
            if (isCar)
            {
                float forwardSpeed = DotProduct(car->GetForward(), car->m_vecMoveSpeed);
                if (forwardSpeed > 0.4f)
                    cam->FOV += (forwardSpeed - 0.4f) * step;
            }
            if (cam->FOV > DefaultFOV)
                cam->FOV = std::pow(0.98f, step) * (cam->FOV - DefaultFOV) + DefaultFOV;
            cam->FOV = std::clamp(cam->FOV, DefaultFOV, DefaultFOV + 30.0f);
        }

        // Looking behind, and back from looking aside or behind
        if (Pad::LookBehindForCar() && (cam->DirectionWasLooking == LOOKING_FORWARD || !cam->LookingBehind))
            TheCamera->m_bCamDirectlyInFront = true;
        if (!(Pad::LookBehindForCar() || Pad::LookBehindForPed() || Pad::LookLeft() || Pad::LookRight()) && cam->DirectionWasLooking != LOOKING_FORWARD)
            TheCamera->m_bCamDirectlyBehind = true;

        // Just entered, or back from looking aside or behind: behind the vehicle
        if (cam->ResetStatics || TheCamera->m_bCamDirectlyBehind || TheCamera->m_bCamDirectlyInFront)
        {
            cam->ResetStatics = false;
            cam->Rotating = false;
            cam->m_bCollisionChecksOn = true;
            cam->Alpha = 0.0f;
            cam->Beta = car->GetForward().Heading() - HALFPI;
            if (TheCamera->m_bCamDirectlyInFront)
                cam->Beta += PI;
            cam->BetaSpeed = 0.0f;
            cam->AlphaSpeed = 0.0f;
            cam->Distance = 1000.0f;

            cam->Front.x = -(std::cos(cam->Beta) * std::cos(cam->Alpha));
            cam->Front.y = -(std::sin(cam->Beta) * std::cos(cam->Alpha));
            cam->Front.z = std::sin(cam->Alpha);
            Car.historyOne = targetCoors - nextDistance * cam->Front;
            Car.historyTwo = targetCoors - newDistance * cam->Front;
            if (!TheCamera->m_bJustCameOutOfGarage)
                cam->Alpha = -zoomModeAlphaOffset;
        }

        cam->Front = targetCoors - Car.historyOne;
        cam->Front.Normalise();

        // Swings round towards where the vehicle goes
        float camRightHeading = cam->Front.Heading() - HALFPI;
        if (camRightHeading < -PI)
            camRightHeading += TWOPI;
        float velocityRightHeading = car->m_vecMoveSpeed.Magnitude2D() <= 0.02f ? camRightHeading : car->m_vecMoveSpeed.Heading() - HALFPI;
        if (velocityRightHeading < camRightHeading - PI)
            velocityRightHeading += TWOPI;
        else if (velocityRightHeading > camRightHeading + PI)
            velocityRightHeading -= TWOPI;

        float betaChangeMult1 = step * params[10];
        float betaChangeLimit = step * params[11];
        float betaChangeMult2 = (car->m_vecMoveSpeed - DotProduct(car->m_vecMoveSpeed, cam->Front) * cam->Front).Magnitude();
        float betaChange = std::clamp(std::min(1.0f, betaChangeMult1 * betaChangeMult2) * (velocityRightHeading - camRightHeading), -betaChangeLimit, betaChangeLimit);
        float targetBeta = camRightHeading + betaChange;
        if (targetBeta < cam->Beta - HALFPI)
            targetBeta += TWOPI;
        else if (targetBeta > cam->Beta + PI)
            targetBeta -= TWOPI;

        float carPosChange = (targetCoors - Car.historyTwo).Magnitude();
        if (carPosChange < newDistance && newDistance > minDistForThisCar)
            newDistance = std::max(minDistForThisCar, carPosChange);

        // Not below the vehicle's roof line seen from behind
        float maxAlphaAllowed = params[13];
        if (model != MI_FIRETRUCK)
        {
            const CVector& forward = car->GetForward();
            CVector left = CrossProduct(forward, CVector(0.0f, 0.0f, 1.0f));
            left.Normalise();
            CVector up = CrossProduct(left, forward);
            up.Normalise();
            if (DotProduct(up, cam->Front) > 0.0f)
            {
                float side = std::asin(std::abs(std::sin(cam->Beta - (forward.Heading() - HALFPI))));
                float reach;
                if (side <= std::atan2(boxMax.x, -boxMin.y))
                    reach = (1.5f - boxMin.y) / std::cos(side);
                else
                    reach = (1.2f + boxMax.x) / std::cos(std::max(0.0f, HALFPI - side));
                maxAlphaAllowed = std::cos(cam->Beta - (forward.Heading() - HALFPI)) * std::atan2(forward.z, forward.Magnitude2D()) +
                    std::atan2(targetCoors.z - car->GetPosition().z + HeightAboveRoadOf(car, boxMin), reach * 1.2f);
                if (isCar && Field<uint8_t>(car, WheelsOnGround) > 1 && std::abs(DotProduct(car->m_vecTurnSpeed, forward)) < 0.05f)
                {
                    const CVector& right = car->GetRight();
                    maxAlphaAllowed += std::cos(cam->Beta - (forward.Heading() - HALFPI) + HALFPI) * std::atan2(right.z, right.Magnitude2D());
                }
            }
        }

        float targetAlpha = std::asin(std::clamp(cam->Front.z, -1.0f, 1.0f)) - zoomModeAlphaOffset;
        if (targetAlpha > maxAlphaAllowed)
            targetAlpha = maxAlphaAllowed;
        else if (targetAlpha < -params[14])
            targetAlpha = -params[14];
        float maxAlphaBlendAmount = step * params[6];
        float targetAlphaBlendAmount = std::clamp((1.0f - std::pow(params[5], step)) * (targetAlpha - cam->Alpha), -maxAlphaBlendAmount, maxAlphaBlendAmount);

        // The right stick, in the game's units (-128..127)
        float rx, ry;
        Pad::RightStick(rx, ry);
        float stickX = -rx * 128.0f;
        float stickY = ry * 128.0f;
        static auto& speed = FixSettings().Float("MAIN.GamepadCameraSpeed");
        float xMovement = std::abs(stickX) * (cam->FOV / 80.0f * 5.0f / 70.0f) * stickX * 0.007f * 0.007f * speed;
        float yMovement = std::abs(stickY) * (cam->FOV / 80.0f * 3.0f / 70.0f) * stickY * 0.007f * 0.007f * speed;

        // The Yardie Lobo levels the camera instead of turning it
        bool correctAlpha = isCar && model == MI_YARDIE;
        if (correctAlpha)
            xMovement = yMovement = 0.0f;
        if (!nextDirectionIsForward)
            xMovement = yMovement = 0.0f;

        if ((set == 0 || set == 7) && std::abs(Pad::SteeringUpDown()) > 120 && car->pDriver)
        {
            float upDown = Pad::SteeringUpDown();
            yMovement += std::abs(upDown) * (cam->FOV / 80.0f * 3.0f / 70.0f) * upDown * 0.007f * 0.007f * 0.5f;
        }
        if (yMovement > 0.0f)
            yMovement *= 0.5f;

        // The mouse turns it when it doesn't steer
        bool mouseChangesBeta = false;
        if (!Pad::IsPadLastDevice() && *DisableMouseSteering && !Pad::ControlsDisabled() && nextDirectionIsForward)
        {
            float mouseX, mouseY;
            Pad::MouseDelta(mouseX, mouseY);
            mouseX *= -2.0f;
            mouseY *= 2.0f;
            float accelX, accelY;
            Pad::MouseAccel(accelX, accelY);
            if (mouseX || mouseY)
            {
                yMovement = mouseY * cam->FOV / 80.0f * accelX;
                xMovement = mouseX * cam->FOV / 80.0f * accelX;
                cam->BetaSpeed = cam->AlphaSpeed = 0.0f;
                targetAlpha = cam->Alpha;
                Car.stepsLeftToChangeBetaByMouse = 50.0f;
                mouseChangesBeta = true;
            }
            else if (Car.stepsLeftToChangeBetaByMouse > 0.0f)
            {
                // Settles once the mouse stops
                cam->BetaSpeed = cam->AlphaSpeed = 0.0f;
                xMovement = yMovement = 0.0f;
                targetAlpha = cam->Alpha;
                Car.stepsLeftToChangeBetaByMouse = std::max(0.0f, Car.stepsLeftToChangeBetaByMouse - step);
                mouseChangesBeta = true;
            }
        }

        if (correctAlpha)
        {
            // Corrected once each time this camera starts
            if (CTimer::m_snTimeInMilliseconds - Car.lastTime > 200)
                Car.alphaCorrected = false;
            if (!Car.alphaCorrected && std::abs(zoomModeAlphaOffset + cam->Alpha) > 0.05f)
                yMovement = (-zoomModeAlphaOffset - cam->Alpha) * 0.05f;
            else
                Car.alphaCorrected = true;
        }
        Car.lastTime = CTimer::m_snTimeInMilliseconds;

        float alphaSpeedFromStickY = yMovement * params[12];
        float betaSpeedFromStickX = xMovement * params[12];

        float newAngleSpeedMaxBlendAmount = params[9];
        float angleChangeStep = std::pow(params[8], step);
        float targetBetaWithStickBlendAmount = std::clamp(betaSpeedFromStickX + (targetBeta - cam->Beta) / std::max(step, 1.0f),
            -newAngleSpeedMaxBlendAmount, newAngleSpeedMaxBlendAmount);

        float angleChangeStepLeft = 1.0f - angleChangeStep;
        cam->BetaSpeed = targetBetaWithStickBlendAmount * angleChangeStepLeft + angleChangeStep * cam->BetaSpeed;
        if (std::abs(cam->BetaSpeed) < 0.0001f)
            cam->BetaSpeed = 0.0f;
        cam->Beta += mouseChangesBeta ? betaSpeedFromStickX : step * cam->BetaSpeed;

        if (TheCamera->m_bJustCameOutOfGarage)
        {
            float invHeading = std::atan2(cam->Front.y, cam->Front.x);
            if (invHeading < 0.0f)
                invHeading += TWOPI;
            cam->Beta = invHeading + PI;
        }

        cam->Beta = LimitRadianAngle(cam->Beta);
        if (cam->Beta < 0.0f)
            cam->Beta += TWOPI;

        if ((set <= 1 || set == 7) && targetAlpha < cam->Alpha && carPosChange >= newDistance && isCar && Field<uint8_t>(car, WheelsOnGround) > 1)
            alphaSpeedFromStickY += (targetAlpha - cam->Alpha) * 0.075f;

        cam->AlphaSpeed = angleChangeStepLeft * alphaSpeedFromStickY + angleChangeStep * cam->AlphaSpeed;
        float maxAlphaSpeed = alphaSpeedFromStickY > 0.0f ? newAngleSpeedMaxBlendAmount * 0.5f : newAngleSpeedMaxBlendAmount;
        cam->AlphaSpeed = std::clamp(cam->AlphaSpeed, -maxAlphaSpeed, maxAlphaSpeed);
        if (std::abs(cam->AlphaSpeed) < 0.0001f)
            cam->AlphaSpeed = 0.0f;

        float alphaWithSpeedAccounted;
        if (mouseChangesBeta)
        {
            alphaWithSpeedAccounted = alphaSpeedFromStickY + targetAlpha;
            cam->Alpha += alphaSpeedFromStickY;
        }
        else
        {
            alphaWithSpeedAccounted = step * cam->AlphaSpeed + targetAlpha;
            cam->Alpha += targetAlphaBlendAmount;
        }

        if (cam->Alpha > maxAlphaAllowed)
        {
            cam->Alpha = maxAlphaAllowed;
            cam->AlphaSpeed = 0.0f;
        }
        else if (cam->Alpha < -params[14])
        {
            cam->Alpha = -params[14];
            cam->AlphaSpeed = 0.0f;
        }

        // No tiny changes
        if (std::abs(Car.lastAlpha - cam->Alpha) < 0.0001f)
            cam->Alpha = Car.lastAlpha;
        Car.lastAlpha = cam->Alpha;
        if (std::abs(Car.lastBeta - cam->Beta) < 0.0001f)
            cam->Beta = Car.lastBeta;
        Car.lastBeta = cam->Beta;

        cam->Front.x = -(std::cos(cam->Beta) * std::cos(cam->Alpha));
        cam->Front.y = -(std::sin(cam->Beta) * std::cos(cam->Alpha));
        cam->Front.z = std::sin(cam->Alpha);
        GetVectorsReadyForRW(cam, nullptr);
        TheCamera->m_bCamDirectlyBehind = false;
        TheCamera->m_bCamDirectlyInFront = false;

        cam->Source = targetCoors - newDistance * cam->Front;
        cam->m_cvecTargetCoorsForFudgeInter = targetCoors;

        float nextAlpha = alphaWithSpeedAccounted + zoomModeAlphaOffset;
        CVector nextFront(-(std::cos(cam->Beta) * std::cos(nextAlpha)), -(std::sin(cam->Beta) * std::cos(nextAlpha)), std::sin(nextAlpha));
        Car.historyOne = targetCoors - nextFront * nextDistance;
        Car.historyTwo = targetCoors - nextFront * newDistance;

        if (nextDirectionIsForward)
        {
            float timestepFactor = std::pow(0.99f, step);
            Car.dontCollideWithCars = timestepFactor * Car.dontCollideWithCars + (1.0f - timestepFactor) * car->m_vecMoveSpeed.Magnitude();
            ClipSource(cam, rwCamera, targetCoors, newDistance, Car.dontCollideWithCars < 0.1f, false, true, true);
        }
        TheCamera->m_bCamDirectlyBehind = false;
        TheCamera->m_bCamDirectlyInFront = false;

        if (set == 5 && cam->Source.z < 1.0f)
            cam->Source.z = 1.0f;

        // A spot in Liberty City where the camera is kept higher
        if (cam->Source.x > 11.0f && cam->Source.x < 91.0f && cam->Source.y > -680.0f && cam->Source.y < -600.0f && cam->Source.z < 24.4f)
            cam->Source.z = 24.4f;

        // Above the water
        if (cameraTarget.z >= -2.0f)
        {
            float level = -6000.0f;
            if (GetWaterLevelNoWaves(cam->Source.x, cam->Source.y, cam->Source.z, &level) && cam->Source.z < level + 0.5f)
                cam->Source.z = level + 0.5f;
        }
        cam->Front = targetCoors - cam->Source;
        GetVectorsReadyForRW(cam, nullptr);

        // The turret follows the camera when the player turns it
        if ((model == MI_RHINO || model == MI_FIRETRUCK) && (Pad::IsPadLastDevice() || *DisableMouseSteering))
            TurretFollowsCamera(cam, car);
        return true;
    }

    SafetyHookInline shProcessCamOnAString, shProcessBehindBoat, shProcessFollowPed;

    void __fastcall ProcessCamOnAString(CCam* cam, void* edx, const CVector& target, float orientation, float a, float b)
    {
        if (!ProcessCar(cam, target))
            shProcessCamOnAString.unsafe_thiscall(cam, &target, orientation, a, b);
    }

    void __fastcall ProcessBehindBoat(CCam* cam, void* edx, const CVector& target, float orientation, float a, float b)
    {
        if (!ProcessCar(cam, target))
            shProcessBehindBoat.unsafe_thiscall(cam, &target, orientation, a, b);
    }

    void __fastcall ProcessFollowPed(CCam* cam, void* edx, const CVector& target, float orientation, float a, float b)
    {
        if (!ProcessPed(cam, target, orientation))
            shProcessFollowPed.unsafe_thiscall(cam, &target, orientation, a, b);
    }
}

class FreeCam
{
public:
    FreeCam()
    {
        WFP::onGameInitEvent() += []()
        {
            auto lineOfSight = hook::pattern("53 56 57 55 81 EC 60 02 00 00 66 83 3D");
            auto sphere = hook::pattern("53 56 57 55 83 EC 38 8B 5C 24 5C");
            auto water = hook::pattern("D9 05 ? ? ? ? 83 EC 10 8B 54");
            auto vectors = hook::pattern("53 89 CB 83 EC 28 C7 83");
            auto followPed = hook::pattern("53 56 57 55 89 CD 81 EC 68");
            auto onAString = hook::pattern("53 56 89 CB 57 83 EC 38");
            auto behindBoat = hook::pattern("53 56 89 CB 57 83 EC 48 8B BB");
            // CWorld::pIgnoreEntity, written after the line of sight in CCam::Process_FollowPedWithMouse
            auto ignore = hook::pattern("C7 05 ? ? ? ? 00 00 00 00 31 FF");
            // gaTempSphereColPoints, passed by CWorld::TestSphereAgainstSectorList
            auto sphereColPoints = hook::pattern("8B 70 1C 8D 45 04 68 ? ? ? ? 56 50");
            // CModelInfo::ms_modelInfoPtrs, read by Process_Cam_On_A_String for the car's box
            auto modelInfo = hook::pattern("8B 34 95 ? ? ? ? 8B 14 BD ? ? ? ? 8B 4E 1C");
            auto mouseSteering = hook::pattern("80 3D ? ? ? ? 00 0F 85 68");
            ProcessLineOfSight = reinterpret_cast<decltype(ProcessLineOfSight)>(lineOfSight.get(1).get<void>());
            TestSphereAgainstWorld = reinterpret_cast<decltype(TestSphereAgainstWorld)>(sphere.get_first());
            GetWaterLevelNoWaves = reinterpret_cast<decltype(GetWaterLevelNoWaves)>(water.get_first());
            GetVectorsReadyForRW = reinterpret_cast<decltype(GetVectorsReadyForRW)>(vectors.get_first());
            pIgnoreEntity.SetAddress(*ignore.get_first<CEntity**>(2));
            TempSphereColPoints = *sphereColPoints.get_first<CColPoint*>(7);
            ModelInfoPtrs = reinterpret_cast<uint8_t***>(modelInfo.get_first(3));
            DisableMouseSteering.SetAddress(*mouseSteering.get_first<bool*>(2));

            shProcessFollowPed = safetyhook::create_inline(followPed.get_first(), ProcessFollowPed);
            shProcessCamOnAString = safetyhook::create_inline(onAString.get_first(), ProcessCamOnAString);
            shProcessBehindBoat = safetyhook::create_inline(behindBoat.get_first(), ProcessBehindBoat);
        };
    }
} FreeCam;
