module;

#include <stdafx.h>

export module Xidi;

import ComVars;
import GUI;

typedef bool (*XidiSendVibrationFunc)(unsigned int, unsigned short, unsigned short);
export XidiSendVibrationFunc XidiSendVibration = nullptr;

export void InitXidi()
{
    typedef bool (*XidiRegisterProfileCallbackFunc)(const wchar_t* (*callback)());
    auto xidiModule = GetModuleHandleW(L"Xidi.32.dll");

    if (xidiModule)
    {
        auto XidiRegisterProfileCallback = (XidiRegisterProfileCallbackFunc)GetProcAddress(xidiModule, "XidiRegisterProfileCallback");
        XidiSendVibration = (XidiSendVibrationFunc)GetProcAddress(xidiModule, "XidiSendVibration");

        if (XidiRegisterProfileCallback)
        {
            XidiRegisterProfileCallback([]() -> const wchar_t*
            {
                static const KnownHashes menuHashes[] = {
                    KnownHashes::P_MenuGameOver,
                    KnownHashes::P_MenuSaveGame,
                    KnownHashes::P_MenuLoadGame,
                    KnownHashes::P_MenuLoadLevel,
                    KnownHashes::P_MenuOnline,
                    KnownHashes::P_MenuLobbyList,
                    KnownHashes::P_MenuRoomListLan,
                    KnownHashes::P_MenuRoomCreateLan,
                    KnownHashes::P_MenuCoop,
                    KnownHashes::P_MenuGameType,
                    KnownHashes::P_MenuSolo,
                };
            
                static const KnownHashes opsatHashes[] = {
                    KnownHashes::P_Pause,
                    KnownHashes::P_KeyPad,
                    KnownHashes::P_Opsat,
                    KnownHashes::P_Goals,
                    KnownHashes::P_Notes,
                    KnownHashes::P_Datas,
                };
            
                if (std::any_of(std::begin(menuHashes), std::end(menuHashes), [](KnownHashes hash) { return CMenusManager::IsMenuDisplayed(hash); }))
                    return L"Menu";
                else if (std::any_of(std::begin(opsatHashes), std::end(opsatHashes), [](KnownHashes hash) { return CMenusManager::IsMenuDisplayed(hash); }))
                    return L"Opsat";
                return L"Main";
            });
        }
    }
}