#pragma once

#include <cstdint>

namespace lcsws
{
    enum class WeaponState : int32_t { Ready = 0, Reloading = 2 };

    struct Weapon
    {
        int32_t type;
        WeaponState state;
        int32_t clip, total;
        uint32_t timer;
        uint8_t reserved[8];
    };
    static_assert(sizeof(Weapon) == 28);

    struct WeaponInfo
    {
        int32_t fireType;
        float range;
        uint32_t firingInterval, reloadDuration;
        int32_t clipSize;
    };

    inline int CurrentWeaponSlot(uintptr_t ped)
    {
        return ped ? *reinterpret_cast<const int8_t*>(ped + 0x6B8) : -1;
    }

    inline Weapon* CurrentWeapon(uintptr_t ped, int slot)
    {
        if (!ped || slot < 0 || slot >= 10)
            return nullptr;
        return reinterpret_cast<Weapon*>(ped + 0x594 + sizeof(Weapon) * slot);
    }
}
