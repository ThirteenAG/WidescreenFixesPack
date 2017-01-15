#pragma once
#pragma warning(disable:4458)
#pragma warning(disable:4201)

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "IniReader.h"
#include "injector\injector.hpp"
#include "injector\calling.hpp"
#include "injector\hooking.hpp"
#include "injector\assembly.hpp"
#include "injector\utility.hpp"
#include <stdint.h>

typedef unsigned char uchar;
typedef unsigned int uint;
