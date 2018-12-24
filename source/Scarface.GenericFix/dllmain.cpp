#include "stdafx.h"

int32_t MouseSensX, MouseSensY;

int sub_457130()
{
    return MouseSensX;
}

int sub_457140()
{
    return MouseSensY;
}

void Init()
{
    CIniReader iniReader("");
    MouseSensX = iniReader.ReadInteger("MAIN", "MouseSensX", 640);
    MouseSensY = iniReader.ReadInteger("MAIN", "MouseSensY", 480);

    auto pattern = hook::pattern("E8 ? ? ? ? 89 44 24 20 DB 44 24 20 D9 5C 24 28");
    injector::MakeCALL(pattern.get_first(0), sub_457130, true);
    injector::MakeCALL(pattern.get_first(17), sub_457140, true);
    injector::MakeCALL(pattern.get_first(38), sub_457130, true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("E8 ? ? ? ? 89 44 24 20 DB 44 24 20 D9 5C 24 28"));
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