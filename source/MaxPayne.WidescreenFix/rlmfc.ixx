module;

#include <stdafx.h>

export module rlmfc;

import ComVars;

export void InitRLMFC()
{
    //moved to msvcp60 wrapper
    //auto pattern = hook::module_pattern(GetModuleHandle(L"rlmfc"), "83 E0 ? 89 45 ? 0F 84");
    //injector::MakeNOP(pattern.get_first(), 3);
}