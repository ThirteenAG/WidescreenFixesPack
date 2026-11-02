#pragma once
#include "Game.hpp"
#include "IconOwners.hpp"

namespace lcsws::icon_profile
{
inline constexpr std::array<console::AddressRule, 3> rules = {{
    {0x08E55970u, 0x08A55508u, icon_owners::fontOwnerWords, icon_owners::fontOwnerMasks, 16, 2, 4, 0, 0, console::AddressKind::Absolute},
    {0x08AE4E2Cu, 0x08AE4E2Cu, icon_owners::menuOwnerWords, icon_owners::menuOwnerMasks, 16, 2, 0, 0, 0, console::AddressKind::Code},
    {0x08AE5004u, 0x08AE5004u, icon_owners::menuSlotsWords, icon_owners::menuSlotsMasks, 16, 2, 0, 0, 0, console::AddressKind::Code}
}};
inline console::NativeProfile<rules> profile;
}
