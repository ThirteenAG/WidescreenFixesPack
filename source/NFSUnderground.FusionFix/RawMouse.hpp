#pragma once

#include "Shared/NFS/BufferedRawMouse.hpp"

inline constexpr wchar_t RawMouseWindow[] = L"NFSUndergroundRawMouse";
using RawMouse = NFSBufferedRawMouse<RawMouseWindow>;
