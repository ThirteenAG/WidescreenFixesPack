module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module KnifeParry;

// MAIN.KnifeParry (live): the knife blocks an enemy's melee attack that lands within a short time after the knife was
// raised or swung. The hit then does no damage, the character plays the game's own guard reaction (or keeps swinging),
// and the enemy staggers into its head-hit flinch, which opens the usual melee prompt for a follow-up.
//
// Hits on a local player wait in its hit record (pawn +0x129C) until the player's update applies them; that is where
// the parry decides. Only the Majini family's melee attacks (punches, kicks, weapon swings, shield and spear attacks,
// Plaga whips) and grab attempts are parried; a grab that already holds the character, kill moves, knock-downs,
// fire, explosions and every other enemy hit as usual.
// Not in online sessions (the enemy's reaction belongs to the machine that owns the enemy).

namespace
{
    constexpr uint32_t ChActionId = 0x10E0;
    constexpr uint32_t ChHitRecord = 0x129C;
    constexpr uint32_t ChHp = 0x1364;
    constexpr uint32_t ChSlot = 0x2D9C;

    constexpr uint32_t HitAttacker = 0x20;
    constexpr uint32_t HitFlagsA = 0x3C;
    constexpr uint32_t HitFlagsB = 0x40;
    constexpr uint32_t HitPending = 0x48;

    constexpr uint32_t ActKnifeBegin = 0x22;
    constexpr uint32_t ActKnifeFromGun = 0x25;
    constexpr uint32_t ActKnifeAttack = 0x27;
    constexpr uint32_t ActGuard = 0x99;
    constexpr uint32_t EnemyDmgHead = 0x6D;

    constexpr double Window = 0.35; // seconds after the knife came up or swung

    uintptr_t* pSession = nullptr;
    using RumbleFn = void(__fastcall*)(uintptr_t player, void* edx, int id, int flag);
    RumbleFn Rumble = nullptr;

    struct Player
    {
        uintptr_t ch = 0;
        uint32_t lastAction = 0xFFFFFFFF;
        double knifeTime = -10.0;
    };
    Player Players[4];

    template<typename T> T& Field(uintptr_t base, uint32_t offset) { return *reinterpret_cast<T*>(base + offset); }

    double Now()
    {
        static LARGE_INTEGER freq{};
        if (!freq.QuadPart)
            QueryPerformanceFrequency(&freq);
        LARGE_INTEGER c;
        QueryPerformanceCounter(&c);
        return double(c.QuadPart) / double(freq.QuadPart);
    }

    bool Online()
    {
        auto session = pSession ? *pSession : 0;
        auto state = session ? Field<uintptr_t>(session, 0x1042C) : 0;
        return state && Field<uint8_t>(state, 0x56C) != 0;
    }

    // Class id of a game object (its DTI, from vtable slot 4: "mov eax, dti; ret")
    uint32_t ClassId(uintptr_t object)
    {
        if (!object)
            return 0;
        auto vtable = Field<uintptr_t>(object, 0);
        auto getDti = Field<uintptr_t>(vtable, 0x10);
        if (Field<uint8_t>(getDti, 0) != 0xB8 || Field<uint8_t>(getDti, 5) != 0xC3)
            return 0;
        auto dti = Field<uintptr_t>(getDti, 1);
        return dti ? Field<uint32_t>(dti, 0x20) : 0;
    }

    // em10..em14 (Majini and their variants all derive from uEm10)
    bool IsMajini(uint32_t id)
    {
        return id >= 0x03001000 && id <= 0x03001400 && (id & 0xFF) == 0;
    }

    // The Majini's grab attempts: from the front, on the run, around the neck and from behind (not the Plaga bites
    // and the other kill moves)
    bool GrabAttempt(uint32_t action)
    {
        return action == 0x97 || action == 0x98 || action == 0x99 || action == 0x9B;
    }

    bool MeleeAttack(uint32_t action)
    {
        return (action >= 0x2B && action <= 0x37) || action == 0x4F || (action >= 0x59 && action <= 0x5C) ||
               action == 0x5E || action == 0x5F || (action >= 0x60 && action <= 0x63);
    }

    void OnPlayerUpdate(uintptr_t ch, bool processesHits)
    {
        static auto& enabled = FixSettings().Int("MAIN.KnifeParry");
        if (!enabled)
            return;
        auto index = Field<uint8_t>(ch, ChSlot);
        if (index > 3)
            return;
        auto& p = Players[index];
        if (p.ch != ch)
        {
            p = Player{};
            p.ch = ch;
        }
        double now = Now();
        auto action = Field<uint32_t>(ch, ChActionId);
        if (action != p.lastAction && (action == ActKnifeBegin || action == ActKnifeFromGun || action == ActKnifeAttack))
            p.knifeTime = now;
        p.lastAction = action;

        auto hit = processesHits ? Field<uintptr_t>(ch, ChHitRecord) : 0;
        if (!hit || !Field<uint8_t>(hit, HitPending) || now - p.knifeTime > Window || Field<int16_t>(ch, ChHp) <= 0 || Online())
            return;

        auto flagsA = Field<uint32_t>(hit, HitFlagsA);
        auto flagsB = Field<uint32_t>(hit, HitFlagsB);
        if (int32_t(flagsA) < 0 || (flagsB & (0x1000000 | 0x2000000 | 0x4000000 | 0x8000000 | 0x400000 | 1)))
            return;

        auto attacker = Field<uintptr_t>(hit, HitAttacker);
        if ((ClassId(attacker) >> 24) == 0x07)
            attacker = Field<uintptr_t>(attacker, 0xFDC); // a weapon: its holder
        if (!IsMajini(ClassId(attacker)))
            return;
        auto attack = Field<uint32_t>(attacker, ChActionId);
        bool grab = (flagsA & 0x40000000) != 0;
        if (grab ? !GrabAttempt(attack) : !MeleeAttack(attack))
            return;

        // Blocked: a grab never latches on, and the character plays the game's guard reaction; for blows the
        // swing goes on mid-swing, otherwise the guard reaction plays without damage
        if (grab || action == ActKnifeAttack)
            Field<uint8_t>(hit, HitPending) = 0;
        else
            Field<uint32_t>(hit, HitFlagsB) |= 1;
        if (grab && action != ActKnifeAttack)
        {
            auto playerSetAction = Field<uintptr_t>(Field<uintptr_t>(ch, 0), 0xDC);
            reinterpret_cast<bool(__fastcall*)(uintptr_t, void*, uint32_t, uint32_t, uintptr_t, int32_t, uint32_t)>(playerSetAction)(ch, nullptr, ActGuard, 0, 0, -1, 0);
        }
        if (Rumble)
            Rumble(ch, nullptr, 34, 0);

        // The enemy reels back as from a head hit, open to a melee follow-up
        auto setAction = Field<uintptr_t>(Field<uintptr_t>(attacker, 0), 0xDC);
        reinterpret_cast<bool(__fastcall*)(uintptr_t, void*, uint32_t, uint32_t, uintptr_t, int32_t, uint32_t)>(setAction)(attacker, nullptr, EnemyDmgHead, 0, 0, -1, 0);
        p.knifeTime = -10.0;
    }
}

export namespace KnifeParry
{
    void Init()
    {
        // Without the session the online check can't be made: no parry then
        auto session = hook::pattern("33 FF 8B F1 89 BE EC 2D 00 00 A1 ? ? ? ?");
        if (!PatternAvailable(session, "ResidentEvil5.FusionFix: KnifeParry"))
            return;
        pSession = *session.get_first<uintptr_t*>(11);
        auto rumble = hook::pattern("83 EC 10 56 8B F1 83 BE A8 2D 00 00 00 0F 85 EB 00 00 00");
        if (PatternUnique(rumble, "ResidentEvil5.FusionFix: KnifeParry"))
            Rumble = (RumbleFn)rumble.get_first();

        // The player's update, after it decided whether this machine applies the player's hits (esi = player)
        auto pattern = hook::pattern("84 C0 74 07 8B CE E8 ? ? ? ? 8B 06 8B 90 F0 00 00 00");
        if (PatternUnique(pattern, "ResidentEvil5.FusionFix: KnifeParry"))
        {
            static auto PlayerUpdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                OnPlayerUpdate(regs.esi, (regs.eax & 0xFF) != 0);
            });
        }
    }
}
