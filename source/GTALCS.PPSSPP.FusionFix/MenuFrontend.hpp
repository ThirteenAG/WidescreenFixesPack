#pragma once
#include "Game.hpp"

namespace lcsws::menu_frontend
{
inline constexpr uint32_t words[] = {0x27BDFD60u, 0xE7B40270u, 0xE7B60274u, 0xE7B80278u,
    0xAFB0027Cu, 0xAFB10280u, 0xAFB20284u, 0xAFB30288u, 0xAFB4028Cu, 0xAFBF0290u,
    0x0C000000u, 0x00808025u, 0x0C000000u, 0x00000000u, 0x8E040584u, 0x14800009u};
inline constexpr uint8_t masks[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0};
inline constexpr std::array<console::AddressRule, 1> rules = {{{0x08ADF724u, 0x08ADF724u,
    words, masks, 16, 2, 0, 0, 0, console::AddressKind::Code}}};
inline console::NativeProfile<rules> profile;
}
