#pragma once
#include "Game.hpp"

namespace lcsws::text_spacing
{
inline constexpr uint32_t words[] = {0x02602025u, 0x00002825u, 0x0C000000u, 0x460CA502u, 0xC60D000Cu, 0x8EA40000u, 0x460D0342u, 0x44847000u, 0x468073A0u, 0x460E6B42u, 0x461E6B03u, 0x4600630Fu, 0x46806320u, 0x460EF383u, 0x8EE40000u, 0x460CA500u};
inline constexpr uint8_t masks[] = {0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
inline constexpr std::array<console::AddressRule, 1> rules = {{{0x08A565D0u, 0x08A565D0u, words, masks, 16, 2, 0, 0, 0, console::AddressKind::Code}}};
inline console::NativeProfile<rules> profile;
}
