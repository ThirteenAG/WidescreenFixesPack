#pragma once
#include <cstdint>

namespace lcsws::icon_owners
{
inline constexpr uint32_t fontOwnerWords[] = {0x3C040000u, 0x24900000u, 0x3C050000u, 0x3C060000u, 0x02002025u, 0x24A50000u, 0x0C000000u, 0x24C60000u, 0x3C050000u, 0x3C060000u, 0x26040004u, 0x24A50000u, 0x0C000000u, 0x24C60000u, 0x3C050000u, 0x3C060000u};
inline constexpr uint8_t fontOwnerMasks[] = {1, 1, 1, 1, 0, 1, 2, 1, 1, 1, 0, 1, 2, 1, 1, 1};
inline constexpr uint32_t menuOwnerWords[] = {0x27BDFFD0u, 0x9085048Cu, 0xAFB00014u, 0xAFB10018u, 0xAFB2001Cu, 0xAFB30020u, 0xAFB40024u, 0xAFB50028u, 0xAFBF002Cu, 0x14A000A5u, 0x00808025u, 0x3C040000u, 0x0C000000u, 0x24840000u, 0x3C0542AAu, 0x02002025u};
inline constexpr uint8_t menuOwnerMasks[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0};
inline constexpr uint32_t menuSlotsWords[] = {0x26130004u, 0x34110001u, 0x26730490u, 0x26B50008u, 0x8EA50000u, 0x8EA60004u, 0x0C000000u, 0x02602025u, 0x02602025u, 0x0C000000u, 0x34050001u, 0x26310001u, 0x26730004u, 0x2A240027u, 0x1480FFF5u, 0x26B50008u};
inline constexpr uint8_t menuSlotsMasks[] = {0, 0, 0, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0};
struct MenuBinding { const char* name; uint8_t slot; };
constexpr MenuBinding menuBindings[] = {
    {"fe_controller_new", 12},
    {"fe_arrows_onfoot_layout1", 13},
    {"fe_arrows_invehicle_layout2", 16},
    {"fe_arrows_layout_right", 21},
    {"btn_circle_PSP", 26},
    {"btn_cross_PSP", 27},
    {"btn_down_PSP", 28},
    {"btn_L_PSP", 30},
    {"btn_left_PSP", 31},
    {"btn_R_PSP", 32},
    {"btn_right_PSP", 33},
    {"btn_square_PSP", 35},
    {"btn_triangle_PSP", 37},
    {"btn_up_PSP", 38},
};
}
