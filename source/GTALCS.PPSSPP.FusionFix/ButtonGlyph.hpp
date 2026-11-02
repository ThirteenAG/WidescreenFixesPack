#pragma once
#include "Game.hpp"

namespace lcsws::button_glyph
{
inline constexpr uint32_t words[] = {0x4600C346u, 0x44866000u, 0x24A50014u, 0x460C8C41u,
    0x00402025u, 0x4600C3C6u, 0x4600D406u, 0x46007486u, 0x4600D306u, 0x0C000000u,
    0x46008CC6u, 0x10000072u, 0x00000000u, 0x24900000u, 0x3C044200u, 0xC60C000Cu};
inline constexpr uint8_t masks[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0};
inline constexpr std::array<console::AddressRule, 1> rules = {{{0x08A55F88u, 0x08A55F78u,
    words, masks, 16, 2, 16, 0, 0, console::AddressKind::Code}}};
inline console::NativeProfile<rules> profile;
}
