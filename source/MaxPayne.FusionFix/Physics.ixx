module;

#include <stdafx.h>
#include <array>
#include <map>

export module Physics;


namespace MaxPaynePhysics
{
    using Vector = std::array<float, 3>;
    SafetyHookInline BodyHook, GravityHook;
    char(__fastcall* OriginalGround)(uint8_t*, void*, const float*, float, const float*, int*) = nullptr;
    uint8_t*(__thiscall* CharacterProperties)(uint8_t*) = nullptr;
    char(__thiscall* IsShootDodging)(uint8_t*) = nullptr;
    std::map<void*, float> Takeoff;
    std::map<void*, float> PendingLift;

    struct Body
    {
        uint8_t* character;
        uint8_t* gravity;
        Vector acceleration;
        Vector position;
        bool leavingGround;
        bool shootDodging;
        float delta;
    };
    thread_local Body* Current = nullptr;

    float Dot(const Vector& first, const Vector& second)
    {
        return first[0] * second[0] + first[1] * second[1] + first[2] * second[2];
    }

    Vector Read(const uint8_t* address)
    {
        Vector value;
        std::memcpy(value.data(), address, sizeof(value));
        return value;
    }

    void Write(uint8_t* address, const Vector& value)
    {
        std::memcpy(address, value.data(), sizeof(value));
    }

    char __fastcall Ground(uint8_t* self, void* edx, const float* up, float slope, const float* movement,
                           int* source)
    {
        char result = OriginalGround(self, edx, up, slope, movement, source);
        if (!result || !Current || self != Current->character + 30)
            return result;
        Vector velocity = Read(Current->gravity + 13);
        float rising = up[0] * movement[0] + up[1] * movement[1] + up[2] * movement[2];
        float upwardVelocity = up[0] * velocity[0] + up[1] * velocity[1] + up[2] * velocity[2];
        // An upward jump impulse is not grounded by a floor still within the short ground probe.
        // Walking up steps and movement inherited from a platform do not have that impulse.
        if (rising > 0.000001f && (Current->shootDodging || upwardVelocity > 0.001f))
        {
            Vector position = Read(Current->character + 0x72);
            float actual = 0.0f;
            for (size_t i = 0; i < position.size(); ++i)
                actual += up[i] * (position[i] - Current->position[i]);
            // The floor contact can discard a rising step smaller than 5 mm. Carry it into the next sweep,
            // preserving displacement without bypassing collision checks or changing the jump impulse.
            if (upwardVelocity > 0.001f && rising < 0.005f && std::abs(actual) < 0.000001f)
                PendingLift[Current->gravity] = rising;
            // The native downward step probe must also wait while a jump or shootdodge is rising.
            float skipDragDown;
            std::memcpy(&skipDragDown, Current->character + 552, sizeof(skipDragDown));
            skipDragDown = std::max(skipDragDown, Current->delta);
            std::memcpy(Current->character + 552, &skipDragDown, sizeof(skipDragDown));
            if (source)
                *source = 0;
            static bool reported = false;
            if (!std::exchange(reported, true))
                OutputDebugStringA("Max Payne: ascending jump ground contact ignored.\n");
            return false;
        }
        return result;
    }

    uintptr_t ApplyPendingLift(uint8_t* self, Vector* movement, uintptr_t result)
    {
        auto found = PendingLift.find(self);
        if (found == PendingLift.end())
            return result;
        float scale = found->second / std::sqrt(Dot(Current->acceleration, Current->acceleration));
        auto displacement = reinterpret_cast<uint8_t*>(movement);
        Vector corrected = Read(displacement);
        for (size_t i = 0; i < corrected.size(); ++i)
            corrected[i] -= Current->acceleration[i] * scale;
        Write(displacement, corrected);
        PendingLift.erase(found);
        return result;
    }

    uintptr_t __fastcall Gravity(uint8_t* self, void* edx, Vector* movement, const float* time)
    {
        if (!Current || Current->gravity != self)
            return GravityHook.unsafe_fastcall<uintptr_t>(self, edx, movement, time);
        float dt = time[1];
        float strength = Dot(Current->acceleration, Current->acceleration);
        if (!std::isfinite(dt) || dt <= 0.0f || strength <= 0.000001f)
            return GravityHook.unsafe_fastcall<uintptr_t>(self, edx, movement, time);

        auto acceleration = self + 1;
        Vector projected = Read(acceleration);
        auto found = Takeoff.find(self);
        if (found == Takeoff.end() && Current->leavingGround &&
            Dot(projected, Current->acceleration) < strength * 0.95f)
        {
            // The previous floor contact suppresses gravity for one frame at takeoff. Keep the 60 Hz delay
            // in simulation time, so changing the render rate or using slow motion does not change jump height.
            found = Takeoff.emplace(self, 1.0f / 60.0f).first;
            static bool reported = false;
            if (!std::exchange(reported, true))
                OutputDebugStringA("Max Payne: takeoff timing normalization active.\n");
        }
        if (found == Takeoff.end())
            return ApplyPendingLift(self, movement, GravityHook.unsafe_fastcall<uintptr_t>(self, edx, movement, time));

        float delay = std::min(dt, found->second);
        float active = dt - delay;
        Vector effective = Current->acceleration;
        for (auto& value : effective)
            value *= active / dt;
        Write(acceleration, effective);
        struct Restore
        {
            uint8_t* address;
            const Vector& value;
            ~Restore()
            {
                Write(address, value);
            }
        } restore{ acceleration, projected };
        auto result = GravityHook.unsafe_fastcall<uintptr_t>(self, edx, movement, time);
        auto displacement = reinterpret_cast<uint8_t*>(movement);
        Vector corrected = Read(displacement);
        for (size_t i = 0; i < corrected.size(); ++i)
            corrected[i] -= 0.5f * Current->acceleration[i] * active * delay;
        Write(displacement, corrected);
        found->second = std::max(0.0f, found->second - dt);
        return ApplyPendingLift(self, movement, result);
    }

    int __fastcall UpdateBody(uint8_t* self, void* edx, const float* time, int animation)
    {
        auto gravity = self + 1028;
        Vector acceleration = Read(gravity + 1);
        Vector velocity = Read(gravity + 13);
        bool ascending = Dot(velocity, acceleration) < -0.000001f;
        if (!ascending)
        {
            Takeoff.erase(gravity);
            PendingLift.erase(gravity);
        }
        Body body{ self, gravity, acceleration, Read(self + 0x72), ascending,
                   IsShootDodging(CharacterProperties(self)) != 0, time[1] };
        struct Restore
        {
            Body* previous;
            ~Restore()
            {
                Current = previous;
            }
        } restore{ std::exchange(Current, &body) };
        auto result = BodyHook.unsafe_fastcall<int>(self, edx, time, animation);
        return result;
    }

    void Init()
    {
        auto body = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC A0 00 00 "
                                  "00 53 56 57 8B F9 68 ? ? ? ? 8D 4D C8");
        auto gravity = hook::pattern("8B 44 24 04 56 57 8B 7C 24 10 8B F1 33 C9 57 89 08 89 48 04 89 48 08 50 8B CE E8 "
                                     "? ? ? ? 57 8B CE E8 ? ? ? ? 5F 5E C2 08 00");
        auto ground = hook::pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC 98 00 "
                                    "00 00 8B 45 14 53 56 57 89 4D CC C7 00 00 00 00 00");
        auto query = hook::pattern(
            "8D 4D F0 51 8D 55 B8 52 50 8B CE E8 ? ? ? ? 50 8B CE E8 ? ? ? ? 8A D8 F6 DB 1A DB FE C3 88 5D C4");
        auto dodge = hook::pattern("8B CF C7 45 FC 00 00 00 00 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 6A 00 75");
        auto propertiesCall = dodge.get_first<uint8_t>(9);
        auto dodgeCall = dodge.get_first<uint8_t>(16);
        CharacterProperties = reinterpret_cast<decltype(CharacterProperties)>(injector::GetBranchDestination(propertiesCall).as_int());
        IsShootDodging = reinterpret_cast<decltype(IsShootDodging)>(injector::GetBranchDestination(dodgeCall).as_int());
        auto call = query.get_first<uint8_t>(19);
        OriginalGround = reinterpret_cast<decltype(OriginalGround)>(ground.get_first());
        GravityHook = safetyhook::create_inline(gravity.get_first(), Gravity);
        BodyHook = safetyhook::create_inline(body.get_first(), UpdateBody);
        if (!GravityHook || !BodyHook)
        {
            BodyHook.reset();
            GravityHook.reset();
            throw std::runtime_error("Max Payne: takeoff timing hooks unavailable");
        }
        injector::MakeCALL(call, Ground, true);
    }
}

class Physics
{
public:
    Physics()
    {
        WFP::onInitEvent() += []()
        {
            MaxPaynePhysics::Init();
        };
    }
} Physics;
