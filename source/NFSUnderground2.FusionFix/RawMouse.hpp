#pragma once

#include "Shared/NFS/BufferedRawMouse.hpp"

inline constexpr wchar_t RawMouseWindow[] = L"NFSUnderground2RawMouse";
using RawMouse = NFSBufferedRawMouse<RawMouseWindow>;
