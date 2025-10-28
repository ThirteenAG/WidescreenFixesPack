#include <stdafx.h>

import ComVars;
import Core;
import Engine;
import D3DDrv;
import WinDrv;
import Echelon;
import Xidi;

void Init()
{
    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    Screen.bHudWidescreenMode = iniReader.ReadInteger("MAIN", "HudWidescreenMode", 1) != 0;
    Screen.fIniHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 120.0f);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    char UserIni[MAX_PATH];
    GetModuleFileNameA(GetModuleHandle(NULL), UserIni, (sizeof(UserIni)));
    *strrchr(UserIni, '\\') = '\0';
    strcat(UserIni, "\\SplinterCellUser.ini");

    CIniReader iniWriter(UserIni);
    char szRes[50];
    sprintf(szRes, "%dx%d", Screen.Width, Screen.Height);
    iniWriter.WriteString("Engine.EPCGameOptions", "Resolution", szRes);

    *strrchr(UserIni, '\\') = '\0';
    strcat(UserIni, "\\SplinterCell.ini");
    iniWriter.SetIniPath(UserIni);
    iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportX", Screen.Width);
    iniWriter.WriteInteger("WinDrv.WindowsClient", "WindowedViewportY", Screen.Height);
    iniWriter.WriteString("D3DDrv.D3DRenderDevice", "FullScreenVideo", "True");
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init);
        CallbackHandler::RegisterCallback(L"Engine.dll", InitEngine);
        CallbackHandler::RegisterCallback(L"D3DDrv.dll", InitD3DDrv);
        CallbackHandler::RegisterCallback(L"WinDrv.dll", InitWinDrv);
        CallbackHandler::RegisterCallback(L"Core.dll", InitCore);
        CallbackHandler::RegisterCallback(L"Echelon.dll", InitEchelon);
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