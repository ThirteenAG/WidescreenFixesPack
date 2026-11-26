#pragma once

#include "Shared/NFS/BufferedRawMouse.hpp"

inline constexpr wchar_t RawMouseWindow[] = L"NFSCarbonRawMouse";
using RawMouse = NFSBufferedRawMouse<RawMouseWindow>;
