#include "Game.hpp"
#include "Shared/Console/PortableIni.hpp"
#include <algorithm>
#include <cmath>
extern "C"
{
#include "psp/nanoprintf.h"
}
namespace ctw
{
    namespace
    {
        SafetyMipsMid height;
        bool enabled, dirty;
        constexpr int minimumAngle = -10000;
        constexpr int minimumElevation = 13464;
        constexpr int maximumElevation = 120000;
        int angle, elevation = minimumElevation;
        uintptr_t drivingCamera;
        int32_t targetX, targetY, targetZ;
        uint32_t previous;
        uint32_t lastSample;
        bool padSynchronized;
        uint32_t retryAt, retryDelay;
        void Save(uint32_t now)
        {
            if (retryDelay && int32_t(now - retryAt) < 0)
                return;
            IniValue values[] = {{"CAMERA", "Active", {}}, {"CAMERA", "Angle", {}}, {"CAMERA", "Elevation", {}}};
            npf_snprintf(values[0].value, sizeof(values[0].value), "%d", int(enabled));
            npf_snprintf(values[1].value, sizeof(values[1].value), "%d", angle);
            npf_snprintf(values[2].value, sizeof(values[2].value), "%d", elevation);
            static IniWriter<> writer;
            if (writer.update(cfgPath, values, 3))
            {
                dirty = false;
                retryDelay = 0;
            }
            else
            {
                retryDelay = retryDelay ? std::min<uint32_t>(retryDelay * 2, 32000000) : 1000000;
                retryAt = now + retryDelay;
            }
        }
        void Load()
        {
            const int active = SettingInteger("CAMERA", "Active", 0);
            const int savedAngle = SettingInteger("CAMERA", "Angle", 0);
            const int savedElevation = SettingInteger("CAMERA", "Elevation", minimumElevation);
            if (active < 0 || active > 1 || savedAngle < minimumAngle || savedAngle > 0 ||
                savedElevation < minimumElevation || savedElevation > maximumElevation)
                return;
            enabled = active != 0;
            angle = savedAngle;
            elevation = savedElevation;
        }
        int Axis(uint8_t value)
        {
            return value < 118 ? int(value) - 118 : value > 138 ? int(value) - 138
                                                                : 0;
        }
        void UpdateDrivingCamera(SafetyMipsContext& regs)
        {
            regs.a0 = at<uint32_t>(regs.sp + 0xB4);
            at<uint32_t>(regs.s6 + 0x104) = regs.a1; // displaced native Y store
            if (!GameplayCamera())
            {
                if (dirty)
                    Save(sceKernelGetSystemTimeLow());
                drivingCamera = 0;
                padSynchronized = false;
                return;
            }

            const uint32_t now = sceKernelGetSystemTimeLow();
            SceCtrlData pad{};
            if (sceCtrlPeekBufferPositive(&pad, 1) > 0)
            {
                // A held button after an inactive frame or a failed read is not a new press.
                const bool continuous = padSynchronized && drivingCamera == regs.s6 && now - lastSample <= 100000u;
                const uint32_t pressed = continuous ? pad.Buttons & ~previous : 0;
                padSynchronized = true;
                lastSample = now;
                previous = pad.Buttons;
                if (pressed & PSP_CTRL_RIGHT)
                {
                    enabled = !enabled;
                    dirty = true;
                }
                if (enabled)
                {
                    if (pressed & PSP_CTRL_UP)
                    {
                        angle = 0;
                        elevation = minimumElevation;
                        dirty = true;
                    }
                    const int x = Axis(pad.Rsrv[0]), y = Axis(pad.Rsrv[1]);
                    if (x || y)
                    {
                        angle = std::clamp(angle - x * 8, minimumAngle, 0);
                        elevation = std::clamp(elevation - y * 8, minimumElevation, maximumElevation);
                        dirty = true;
                    }
                    else if (dirty)
                        Save(now);
                }
                else if (dirty)
                    Save(now);
            }
            else
                padSynchronized = false;

            drivingCamera = regs.s6;
            if (enabled)
            {
                // The native chase routine uses this same target to aim its yaw.
                targetX = at<int32_t>(regs.s7 + 0x30);
                targetY = at<int32_t>(regs.s7 + 0x34);
                targetZ = at<int32_t>(regs.s7 + 0x38);
                regs.a0 = uint32_t(targetZ + elevation);
            }
        }
    }

    void FlushCameraSave(uint32_t now)
    {
        if (dirty && !GameplayCamera())
            Save(now);
    }

    int16_t DrivingCameraPitch(uintptr_t camera, float tangentScale)
    {
        const auto native = at<int16_t>(camera + 0x12C);
        if (!enabled || camera != drivingCamera || !GameplayCamera())
            return native;

        const float dx = float(targetX) - float(at<int32_t>(camera + 0x100));
        const float dy = float(targetY) - float(at<int32_t>(camera + 0x104));
        const float dz = float(at<int32_t>(camera + 0x108)) - float(targetZ);
        const float inclination = std::atan2(dz, std::sqrt(dx * dx + dy * dy));
        const int cosine = at<int>(camera + 84);
        const float tangent = cosine > 0 ? float(at<int>(camera + 80)) * tangentScale / float(cosine) : 0.0f;
        // Adjust pitch around the vehicle. Express the bias as a fraction of
        // the visible half-height so changing FOV cannot move it out of frame.
        const float bias = 0.45f - 0.9f * float(angle) / float(minimumAngle);
        constexpr float angleUnitsPerRadian = 65536.0f / 6.283185307f;
        return int16_t(-std::lround((inclination - std::atan(tangent * bias)) * angleUnitsPerRadian));
    }

    void InstallCamera()
    {
        if (!Option("Enable3rdPersonCamera", 1))
            return;
        Load();
        injector::MakeNOP(Address<0x088704C8>()); // retain the original cinematic-camera opt-out
        safetymips::Options replace;
        replace.execute_original = false;
        height = safetymips::create_mid(Address<0x088CBFA8>(), [](SafetyMipsContext& regs)
                                        { UpdateDrivingCamera(regs); }, replace);
    }
}
