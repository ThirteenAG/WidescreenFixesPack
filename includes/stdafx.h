#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#pragma warning(disable: 4178 4305 4309 4510 4996)
#include <windows.h>
#include <stdint.h>
#include "IniReader.h"
#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\assembly.hpp"
#include "injector\utility.hpp"
#include "Hooking.Patterns.h"
#pragma warning(default: 4178 4305 4309 4510 4996)