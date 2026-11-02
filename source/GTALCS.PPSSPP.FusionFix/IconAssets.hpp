#pragma once
#include <cstdint>

namespace lcsws::icon_assets
{
struct Asset { const char* name; uint32_t bytes, hash; };
inline constexpr Asset assets[] = {
    {"btn_up_PSP", 320u, 0xEC66B5E1u},
    {"btn_triangle_PSP", 320u, 0x93C3FE59u},
    {"btn_square_PSP", 320u, 0x3F00C112u},
    {"btn_right_PSP", 320u, 0xD7411443u},
    {"btn_R_PSP", 320u, 0x3E710C70u},
    {"btn_left_PSP", 320u, 0xCF02C213u},
    {"btn_L_PSP", 320u, 0x1F07CA51u},
    {"btn_down_PSP", 320u, 0xF27584A9u},
    {"btn_cross_PSP", 320u, 0x89757382u},
    {"btn_circle_PSP", 320u, 0xC9A6C9CAu},
    {"fe_arrows_layout_right", 8256u, 0x2BD6E236u},
    {"fe_arrows_invehicle_layout2", 8256u, 0xB6315471u},
    {"fe_arrows_onfoot_layout1", 8256u, 0x77A4AE59u},
    {"fe_controller_new", 16448u, 0x431317F6u},
    {"font2", 32832u, 0x1AF39B14u},
};
}
