#pragma once
#include "Game.hpp"

namespace lcsws::menu_back
{
inline constexpr uint32_t words[] = {0x8485002Cu, 0x14A00005u, 0x34060000u, 0x8485005Eu, 0x50A00003u, 0x84870028u, 0x34060001u, 0x84870028u, 0x14E00005u, 0x34050000u, 0x8484005Au, 0x10800003u, 0x30A400FFu, 0x34050001u, 0x30A400FFu, 0x30C500FFu};
inline constexpr uint8_t masks[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
inline constexpr std::array<console::AddressRule, 1> rules = {{{0x08A970E0u, 0x08A970E0u, words, masks, 16, 2, 0, 0, 0, console::AddressKind::Code}}};
inline console::NativeProfile<rules> profile;
}
