#pragma once
#include "../Shared/Console/PortableLights.hpp"
namespace lcsfx {
inline constexpr uintptr_t poolPointers[][2] = {
    {0x089F9ED0, 0x089F9EE4},
    {0x089FA048, 0x089FA050},
    {0x089FA150, 0x089FA160},
    {0x089FB1E0, 0x089FB208},
    {0x089FBB70, 0x089FBBB0},
    {0x089FCA3C, 0x089FCA4C},
};
inline constexpr uintptr_t poolLimits[] = {0x089F9F94, 0x089FA070, 0x089FA198, 0x089FA19C, 0x089FB8E0, 0x089FBADC, 0x089FBFF4, 0x089FBBE4, 0x089FCA7C, 0x089FCA48, 0x089FCAD0};
inline constexpr uintptr_t trafficCalls[] = {0x08866928, 0x088669DC, 0x0886722C, 0x08867C78, 0x08867D28};
}
