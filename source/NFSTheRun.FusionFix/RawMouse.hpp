#pragma once

#include "Shared/NFS/BufferedRawMouse.hpp"

inline constexpr wchar_t RawMouseWindow[] = L"NFSTheRunRawMouse";
using RawMouse = NFSBufferedRawMouse<RawMouseWindow>;
