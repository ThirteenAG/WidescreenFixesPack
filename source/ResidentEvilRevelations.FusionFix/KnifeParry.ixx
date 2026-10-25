module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "PlayerControls.hpp"

export module KnifeParry;

namespace
{
    using namespace RevPlayer;
    SafetyHookInline KnifeHook;
    SafetyHookMid DamageHook;
    uintptr_t knifePlayer{};
    uint32_t knifeSerial{};
    double knifeTime{ -10.0 };

    int __fastcall BeginKnife(uintptr_t state, void*, uintptr_t ch, int param, void* args)
    {
        auto result = KnifeHook.unsafe_thiscall<int>(state, ch, param, args);
        if (Offline(ch))
        {
            knifePlayer = ch;
            knifeSerial = Field<uint32_t>(ch, 0xE2C);
            knifeTime = Now();
        }
        return result;
    }

    bool Melee(uintptr_t enemy)
    {
        auto name = TypeName(enemy);
        if (!name)
            return false;
        auto type = std::string_view(name);
        if (type != "uZombieBase" && type != "uZombieBaseLight" &&
            type != "uZombieLongArm" && type != "uZombieLongArmLight")
            return false;
        if (!(Field<float>(enemy, 0xE38) > 0.0f))
            return false;
        auto machine = Field<uintptr_t>(enemy, 0xE80);
        auto state = machine ? Field<uintptr_t>(machine, 20) : 0;
        name = TypeName(state);
        if (!name)
            return false;
        auto action = std::string_view(name);
        return action == "cZombieBaseStateAttackBeat" || action == "cZombieBaseStateAttackBeatL" ||
            action == "cZombieBaseStateAttackSweep" || action == "cZombieBaseStateAttackSweepL" ||
            action == "cZombieBaseStateAttackSweepFront" || action == "cZombieBaseStateAttackCrawling" ||
            action == "cZombieBaseStateAttackSwingArms" || action == "cZombieBaseStateAttackWideSwing";
    }

    bool Stagger(uintptr_t enemy)
    {
        if (!(Field<float>(enemy, 0xE38) > 0.0f))
            return false;
        auto machine = Field<uintptr_t>(enemy, 0xE80);
        auto states = Field<uintptr_t*>(machine, 48);
        auto count = Field<uint32_t>(machine, 52);
        if (!states || count > 256)
            return false;
        for (uint32_t i = 0; i < count; ++i)
        {
            auto name = TypeName(states[i]);
            if (name && std::string_view(name) == "cZombieBaseStateDamageGroggyHeadF")
                return ChangeState(machine, static_cast<int>(i), 0, nullptr);
        }
        return false;
    }

    bool TryParry(uintptr_t ch)
    {
        if (ch != knifePlayer || !Offline(ch) || Field<uint32_t>(ch, 0xE2C) != knifeSerial ||
            Upper(ch) != 10 || Now() - knifeTime > 0.35 || (Field<uint32_t>(ch, 0xE1C) & 1) ||
            Field<uint32_t>(ch, 4932))
            return false;
        auto lower = Lower(ch);
        if (lower < 1 || lower > 5)
            return false;
        auto pool = Field<uintptr_t>(ch, 0xEA0);
        if (!pool || Field<uint32_t>(pool, 20) != 1 || !Field<uint32_t>(pool, 8))
            return false;
        auto hit = Field<uintptr_t>(pool, 4);
        if (!hit)
            return false;
        auto enemy = Field<uintptr_t>(hit, 68);
        auto source = Field<uintptr_t>(hit, 64);
        // Projectile sources and mixed hit batches retain the game's damage handling.
        if (!enemy || (source && source != enemy) || !Melee(enemy) || !Stagger(enemy))
            return false;
        Write(pool, 20, uint32_t{ 0 });
        knifeTime = -10.0;
        return true;
    }

    void CheckDamage(SafetyHookContext& regs)
    {
        // Reject the pending hit before damage and its follow-up reaction are dispatched.
        if ((regs.eax & 0xFF) && TryParry(static_cast<uintptr_t>(regs.esi)))
            regs.eax &= ~0xFFu;
    }
}

class KnifeParry
{
public:
    KnifeParry()
    {
        WFP::onInitEvent() += []()
        {
            if (!RevPlayer::Init())
                return;
            auto knife = find_pattern("55 8B EC 83 E4 F0 83 EC 34 8B 45 0C 53 56 8B 75 08 8B D9 89 43 20 83 BE 30 0E 00 00 01");
            auto damage = find_pattern("E8 ? ? ? ? 8B CE 84 C0 74 ? E8 ? ? ? ? 8B CE E8 ? ? ? ? EB ? 83 BE 44 13 00 00 00");
            if (knife.size() != 1 || damage.size() != 1)
            {
                OutputDebugStringA("Resident Evil Revelations Fusion Fix: knife parry layout unavailable.\n");
                return;
            }
            KnifeHook = safetyhook::create_inline(knife.get_first(), BeginKnife, SafetyHookInline::StartDisabled);
            DamageHook = safetyhook::create_mid(damage.get_first(5), CheckDamage, SafetyHookMid::StartDisabled);
            if (!KnifeHook || !DamageHook)
                return;
            auto apply = []()
            {
                auto enabled = FixSettings().Int("MAIN.KnifeParry") != 0;
                SetEnabled(KnifeHook, enabled);
                SetEnabled(DamageHook, enabled);
            };
            FixSettings().OnChange("MAIN.KnifeParry", apply);
            apply();
        };
    }
} KnifeParry;
