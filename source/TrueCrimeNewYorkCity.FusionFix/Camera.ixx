module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;

import ComVars;
import Cinema;

// Field of view: Hor+ for the world camera and the FOV factor in gameplay
export namespace TCCamera
{
    std::atomic<float> factor{ 1.0f };
    bool ready = false;

    // Game data and native camera types, found by the code that uses them
    struct Addresses
    {
        uintptr_t stack{}, projection{}, projectionCaller{};
        uintptr_t cameraTable{}, cameraUpdate{}, followTable{}, vehicleTable{};
    };
    Addresses addresses;

    template<class T> T Read(const void* pointer, size_t offset)
    {
        T value{};
        std::memcpy(&value, static_cast<const uint8_t*>(pointer) + offset, sizeof(value));
        return value;
    }

    void SetFactor(float value)
    {
        factor.store(std::isfinite(value) ? std::clamp(value, 0.5f, 2.0f) : 1.0f, std::memory_order_relaxed);
    }

    bool Gameplay(const void* world, const void* stack, const void* player, bool blending)
    {
        if (blending || !world || !player || Read<int>(stack, 40) != 0 || Read<uint8_t>(world, 10868))
            return false;

        auto camera = Read<const void*>(world, 204);
        if (!camera || Read<const void*>(stack, 0) != camera ||
            (Read<uint8_t>(camera, 31) & 0x80) || Read<uintptr_t>(camera, 0) != addresses.cameraTable ||
            Read<uintptr_t>(camera, 24) != addresses.cameraUpdate || Read<uint8_t>(camera, 144) != 10 ||
            Read<uint8_t>(camera, 428) != 1 || Read<uint8_t>(camera, 409))
            return false;

        const auto mode = Read<uint8_t>(camera, 145);
        if (mode != 0 && mode != 13)
            return false;
        auto handler = Read<const void*>(camera, 404);
        if (!handler || Read<const void*>(camera, 308 + 4 * mode) != handler ||
            Read<const void*>(handler, 52) != camera ||
            Read<uintptr_t>(handler, 0) != (mode == 0 ? addresses.followTable : addresses.vehicleTable))
            return false;

        const auto target = Read<const void*>(camera, 412);
        if (mode == 0)
            return target == player;

        const auto vehicle = Read<const void*>(player, 48);
        // Vehicle entry has its own blend, in addition to the common camera blend.
        return vehicle && target == vehicle && Read<uint8_t>(vehicle, 144) == 2 &&
            Read<const void*>(handler, 584) == vehicle && !Read<uint8_t>(handler, 657) &&
            Read<float>(handler, 668) == 1.0f;
    }

    float Scale(float nativeScale, float verticalCoefficient, float multiplier)
    {
        if (!std::isfinite(nativeScale) || nativeScale <= 0.0f || !std::isfinite(verticalCoefficient) ||
            verticalCoefficient <= 0.0f || !std::isfinite(multiplier) || multiplier == 1.0f)
            return nativeScale;

        constexpr double pi = 3.14159265358979323846;
        const double halfAngle = std::atan(1.0 / (double(nativeScale) * verticalCoefficient));
        const double adjusted = std::clamp(halfAngle * std::clamp(double(multiplier), 0.5, 2.0), pi / 360.0, pi * 179.0 / 360.0);
        const double scale = 1.0 / (std::tan(adjusted) * verticalCoefficient);
        return std::isfinite(scale) && scale > 0.0 && scale <= std::numeric_limits<float>::max() ? float(scale) : nativeScale;
    }

    void Install()
    {
        const auto operand = [](const char* pattern, ptrdiff_t offset)
        {
            return *hook::pattern(pattern).get_first<uintptr_t>(offset);
        };
        // Camera constructor: vtable and update function
        auto camera = hook::pattern("53 56 8B F1 33 DB C7 06 ? ? ? ? C7 46 18 ? ? ? ? 57 89 74 24 18 89 5E 40 89 5C 24 3C E8 ? ? ? ? C7 06 ? ? ? ? C7 44 24 3C 01");
        addresses.cameraTable = *camera.get_first<uintptr_t>(8);
        addresses.cameraUpdate = *camera.get_first<uintptr_t>(15);
        // Camera mode handlers: on foot (mode 0) and in a vehicle (mode 13)
        addresses.followTable = operand("24 3C 05 74 ? 56 8B CF E8 ? ? ? ? 0F 57 C0 F3 0F 11 87 30 02 00 00 F3 0F 11 87 34 02 00 00 F3 0F 11 87 3C 02 00 00 F3 0F 10 05 ? ? ? ? F3 0F 11 47 14 C7 07", 55);
        addresses.vehicleTable = operand("8D 8E C0 02 00 00 C7 06 ? ? ? ? 89 9E 48 02 00 00", 8);
        addresses.stack = operand("56 B9 ? ? ? ? C7 84 24 C0 00 00 00 FF FF FF FF E8", 2);
        pPlayer.SetAddress(operand("83 BE 48 02 00 00 00 0F 85 ? ? ? ? A1 ? ? ? ? 85 C0 0F 84", 14));
        bCameraBlending.SetAddress(operand("81 EC B8 01 00 00 53 33 DB 38 1D ? ? ? ? 56 8B F1 75 0A", 11));
        pCameraWorld.SetAddress(operand("E8 ? ? ? ? 8B 0D ? ? ? ? 66 A3 ? ? ? ? E8", 7));
        // The world camera's projection, set before the frame is drawn
        auto projection = hook::pattern("8B 86 A4 01 00 00 50 B9 ? ? ? ? E8 ? ? ? ? 8B 4D 0C 68");
        addresses.projection = *projection.get_first<uintptr_t>(8);
        addresses.projectionCaller = reinterpret_cast<uintptr_t>(projection.get_first(17));
        ready = true;
    }

    // The world camera's projection, set before the frame is drawn
    bool World(const void* projection, uintptr_t caller)
    {
        return ready && reinterpret_cast<uintptr_t>(projection) == addresses.projection && caller == addresses.projectionCaller;
    }

    float Apply(const void* projection, uintptr_t caller, float nativeScale, bool blocked)
    {
        const float multiplier = factor.load(std::memory_order_relaxed);
        if (multiplier == 1.0f || !ready || blocked || reinterpret_cast<uintptr_t>(projection) != addresses.projection ||
            caller != addresses.projectionCaller)
            return nativeScale;

        const void* world = pCameraWorld;
        const auto stack = reinterpret_cast<const void*>(addresses.stack);
        const void* player = pPlayer;
        const bool blending = bCameraBlending != 0;
        return Gameplay(world, stack, player, blending) ? Scale(nativeScale, Read<float>(projection, 16), multiplier) : nativeScale;
    }
}

class Camera
{
public:
    Camera()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            TCCamera::SetFactor(settings.Float("MAIN.FOVFactor"));
            settings.OnChange("MAIN.FOVFactor", []
            {
                TCCamera::SetFactor(FixSettings().Float("MAIN.FOVFactor"));
            });
            TCCamera::Install();

            auto pattern = hook::pattern("F3 0F 10 45 08 56 8B F1 8B 46 1C");
            struct FOVHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    const auto projection = reinterpret_cast<const void*>(regs.ecx);
                    const auto caller = *reinterpret_cast<const uintptr_t*>(regs.ebp + 4);
                    float scale = *reinterpret_cast<const float*>(regs.ebp + 8);
                    if (TCCamera::World(projection, caller))
                        scale = TCCinema::Scale(scale, Screen.fWidth, Screen.fHeight);
                    else if (std::isfinite(Screen.fAspectRatio) && Screen.fAspectRatio > 0.0f)
                        scale *= (4.0f / 3.0f) / Screen.fAspectRatio;
                    const bool blocked = bPause || bCutscene || nLoading != 0 || TCCinema::Active();
                    scale = TCCamera::Apply(projection, caller, scale, blocked);
                    Screen.fFieldOfView = scale;
                    regs.xmm0.f32[0] = scale;
                }
            }; injector::MakeInline<FOVHook>(pattern.get_first());
        };
    }
} Camera;
