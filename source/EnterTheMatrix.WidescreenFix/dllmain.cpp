#include "stdafx.h"

import ComVars;
import Matrix;
import MatrixOptions;

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitMatrix, hook::pattern("55 8B EC 83 EC 34 56 6A 00 6A FF"));
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(InitMatrixOptions, hook::pattern("8B 45 04 85 C0 74"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}