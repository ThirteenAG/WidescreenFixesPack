module;

#include <stdafx.h>

export module Traffic;

import ComVars;
import Camera;

// Ambient traffic. A lane spawns a car at a random point of its length once its nearest point is close enough ahead,
// in a 90 degree cone. The car's driver removes the car outside a zone around the camera: as deep as the despawn
// distance, half as wide (the 4:3 view), shorter behind. Three things showed on screen:
// - points of the lane outside that zone gave cars that were removed again a moment later
// - the zone is narrower than a wide picture, so cars at its sides vanished in view
// - cars were removed, and appeared, at once; the game fades its parked cars instead
namespace Traffic
{
    GameRef<uint32_t> nStepCount;
    GameRef<float> fDespawnDistance;
    // The world camera's view matrix (row vectors)
    GameRef<std::array<float, 16>> mView;

    void (__thiscall* FadeIn)(void* vehicle, int steps, void* owner) = nullptr;
    void (__thiscall* FadeOut)(void* vehicle, int steps, void* owner) = nullptr;

    // Vehicle: fading in/out, opacity, step it was last drawn in
    constexpr size_t oFadingIn = 0x110, oFadingOut = 0x111, oOpacity = 0x1B0, oDrawnStep = 0x47C;
    constexpr int nFadeInSteps = 30, nFadeOutSteps = 45, nSeenSteps = 15;
    constexpr float fBehindScale = 1.25f;
    // Crowd pedestrian: flags, opacity
    constexpr size_t oPedestrianFlags = 0x30, oPedestrianOpacity = 0x58;
    constexpr uint8_t nPedestrianLeaving = 4;

    // Sideways distances count this much in the zone: 2 at 4:3, less for a wider picture
    float fLateralScale = 2.0f;

    template<class T> T& At(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
    }

    void UpdateLateralScale()
    {
        // The projection's horizontal and vertical scale
        const auto projection = reinterpret_cast<const float*>(TCCamera::addresses.projection + 768);
        const float horizontal = projection[0], vertical = projection[5];
        if (horizontal > 0.0f && vertical > 0.0f)
            fLateralScale = std::min(2.0f, 2.0f * (4.0f / 3.0f) * horizontal / vertical);
    }

    // The driver's distance of a point, as it compares it with the despawn distance
    float ZoneDistance(const float* position)
    {
        const auto& view = mView.get();
        const float x = position[0] * view[0] + position[1] * view[4] + position[2] * view[8] + view[12];
        const float z = position[0] * view[2] + position[1] * view[6] + position[2] * view[10] + view[14];
        const float distance = std::hypot(x * fLateralScale, z);
        return z < 0.0f ? distance * fBehindScale : distance;
    }

    bool Fading(void* vehicle)
    {
        return At<uint8_t>(vehicle, oFadingIn) || At<uint8_t>(vehicle, oFadingOut);
    }

    bool Seen(void* vehicle)
    {
        return At<float>(vehicle, oOpacity) > 0.0f && nStepCount - At<uint32_t>(vehicle, oDrawnStep) <= nSeenSteps;
    }

    void* (__thiscall* CreateVehicle)(void* world, int model, int group, float* position, int color, int variant) = nullptr;
    void* __fastcall CreateLaneVehicle(void* world, void*, int model, int group, float* position, int color, int variant)
    {
        if (ZoneDistance(position) > fDespawnDistance * 0.9f)
            return nullptr;
        return CreateVehicle(world, model, group, position, color, variant);
    }

    void Install()
    {
        auto pattern = hook::pattern("8B 0D ? ? ? ? 2B 8A 7C 04 00 00");
        nStepCount.SetAddress(*pattern.get_first<uint32_t*>(2));
        static auto pThink = pattern.get_first();

        pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 1C 56 8B 75 08 8D 44 24 10 50 8B CE E8 ? ? ? ? 0F 28 00 0F 28 1D ? ? ? ?");
        mView.SetAddress(reinterpret_cast<std::array<float, 16>*>(*pattern.get_first<uintptr_t>(31) - 0x30));

        FadeIn = reinterpret_cast<decltype(FadeIn)>(hook::pattern("80 B9 11 01 00 00 00 0F 57 C0 8B 44 24 04").get_first());
        FadeOut = reinterpret_cast<decltype(FadeOut)>(hook::pattern("80 B9 10 01 00 00 00 0F 57 C0 8B 44 24 04").get_first());

        // The driver's comparison with the despawn distance; a car inside it is kept
        pattern = hook::pattern("0F 2F 0D ? ? ? ? 0F 86 ? ? ? ? 8B 4E 30");
        fDespawnDistance.SetAddress(*pattern.get_first<float*>(3));
        static auto pKeep = uintptr_t(pattern.get_first(13)) + *pattern.get_first<int32_t>(9);
        static auto pRemove = uintptr_t(pattern.get_first(13));

        // A lane's car is created at the lane's chosen point: not where its driver would remove it at once
        pattern = hook::pattern("E8 ? ? ? ? 8B F8 85 FF 0F 84 ? ? ? ? E8 ? ? ? ? 8B CF 89 44 24 1C");
        CreateVehicle = reinterpret_cast<decltype(CreateVehicle)>(injector::MakeCALL(pattern.get_first(), CreateLaneVehicle, true).as_int());

        // The new car is in the world
        pattern = hook::pattern("8B CE E8 ? ? ? ? 6A 00 25 FF 01 00 00 6A 08 6A 00");
        static auto AddedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            const auto vehicle = reinterpret_cast<void*>(regs.edi);
            FadeIn(vehicle, nFadeInSteps, nullptr);
            // Invisible until its first step of the fade
            At<float>(vehicle, oOpacity) = 0.0f;
        });

        // The zone's width
        pattern = hook::pattern("F3 0F 10 0D ? ? ? ? F3 0F 11 4C 24 18 8D 4C 24 18 F3 0F 10 11 0F 28 C8 F3 0F 59 CA");
        injector::WriteMemory(pattern.get_first(4), &fLateralScale, true);

        // Each time the driver of an ambient car checks its distance
        static auto ThinkHook = safetyhook::create_mid(pThink, [](SafetyHookContext& regs)
        {
            UpdateLateralScale();

            // Faded out: nothing left to see
            const auto vehicle = reinterpret_cast<void*>(regs.edx);
            if (!Fading(vehicle) && At<float>(vehicle, oOpacity) <= 0.0f)
                regs.eip = pRemove;
        });

        // The driver removes its car: one that is being drawn fades out first. The renderer doesn't mark a
        // see-through car as drawn, so a fading one finishes its fade.
        static auto RemoveHook = safetyhook::create_mid(pRemove, [](SafetyHookContext& regs)
        {
            const auto vehicle = At<void*>(reinterpret_cast<void*>(regs.esi), 0x30);
            if (!vehicle)
                return;
            if (Fading(vehicle))
                regs.eip = pKeep;
            else if (Seen(vehicle))
            {
                FadeOut(vehicle, nFadeOutSteps, nullptr);
                regs.eip = pKeep;
            }
        });

        // The crowd. A pedestrian fades out when it leaves, 16 of 255 a step, but appeared at full opacity, in view
        // from 12.5 units on: it fades in the same way.
        pattern = hook::pattern("C6 46 58 FF 5E 5B C2 1C 00");
        injector::WriteMemory<uint8_t>(pattern.get_first(3), 1, true);

        pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC C4 02 00 00 53 56 8B F1 8B 46 3C");
        static auto PedestrianHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            const auto pedestrian = reinterpret_cast<void*>(regs.ecx);
            auto& opacity = At<uint8_t>(pedestrian, oPedestrianOpacity);
            if (!(At<uint8_t>(pedestrian, oPedestrianFlags) & nPedestrianLeaving) && opacity < 255)
                opacity = uint8_t(std::min(255, opacity + 16));
        });
    }
}

class TrafficFix
{
public:
    TrafficFix()
    {
        WFP::onInitEvent() += []()
        {
            Traffic::Install();
        };
    }
} TrafficFix;
