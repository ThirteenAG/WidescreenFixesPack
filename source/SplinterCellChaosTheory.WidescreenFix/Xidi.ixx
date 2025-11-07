module;

#include <stdafx.h>

export module Xidi;

import ComVars;
import GUI;
import Hashes;

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
                    //KnownHashes::P_ChatIngame,
                    //KnownHashes::P_OptionPopup,
                    KnownHashes::P_SavingScreen,
                    //KnownHashes::P_Progress,
                    //KnownHashes::P_ContextMenu,
                    //KnownHashes::P_endmission,
                    KnownHashes::P_MenuLoadLevel,
                    KnownHashes::P_MenuLoadGame,
                    KnownHashes::P_MenuSaveGame,
                    KnownHashes::P_MenuGameOver,
                    KnownHashes::P_MenuOnline,
                    KnownHashes::P_MenuLobbyList,
                    KnownHashes::P_MenuRoomListLan,
                    KnownHashes::P_MenuRoomCreateLan,
                    KnownHashes::P_MenuCoop,
                    KnownHashes::P_MenuGameType,
                    KnownHashes::P_MenuSolo,
                    KnownHashes::P_PauseOnline,
                    KnownHashes::P_Pause,
                    KnownHashes::P_GameOverOnline,
                    KnownHashes::P_LoadOut,
                    KnownHashes::P_LoadOut_Selection,
                    KnownHashes::P_LoadOut_Infos,
                    //KnownHashes::P_Sound,
                    //KnownHashes::P_Image,
                    //KnownHashes::P_Email,
                    KnownHashes::P_OverallAchievement,
                    KnownHashes::P_MissionStatistics,
                    KnownHashes::P_ObjectiveReview,
                    KnownHashes::P_EnterServerPassword,
                    KnownHashes::P_CDKeyEntry,
                    KnownHashes::P_OnlineTermsOfUse,
                    KnownHashes::P_PrivateMessage,
                    KnownHashes::P_FriendsManagement,
                    KnownHashes::P_ManageAccount,
                    KnownHashes::P_RoomOnline,
                    KnownHashes::P_Login,
                    KnownHashes::P_OnlineQuickMatchFilter,
                    KnownHashes::P_CreateAccount,
                    KnownHashes::P_ManageGame,
                    KnownHashes::P_LobbyOnline,
                    KnownHashes::P_OnlineESRB,
                    KnownHashes::P_OfflineOnline,
                    KnownHashes::P_LobbyLan,
                    KnownHashes::P_OnlineCreate,
                    KnownHashes::P_Movies,
                    KnownHashes::P_MusicsList,
                    KnownHashes::P_MusicsBio,
                    KnownHashes::P_Extras,
                    //KnownHashes::P_TrainingVideo,
                    //KnownHashes::P_TrainingVideoMain,
                    KnownHashes::P_Difficulty,
                    KnownHashes::P_Controls_joystick,
                    KnownHashes::P_ShaderAdvanced,
                    KnownHashes::P_CreateProfile,
                    KnownHashes::P_SoundSettings,
                    KnownHashes::P_Profiles,
                    KnownHashes::P_DisplaySettings,
                    KnownHashes::P_DisplayAdvanced,
                    //KnownHashes::P_Controls_Popup_Selection,
                    KnownHashes::P_Controls_keyboard,
                    //KnownHashes::P_Panel,
                    //KnownHashes::P_MsgBox_Training,
                    KnownHashes::P_CamControl,
                    KnownHashes::P_Briefing,
                };
            
                static const KnownHashes opsatHashes[] = {
                    KnownHashes::P_KeyPad,
                    KnownHashes::P_Opsat,
                    KnownHashes::P_Goals,
                    KnownHashes::P_Goals_details,
                    KnownHashes::P_Notes,
                    KnownHashes::P_Datas,
                    KnownHashes::P_Computer,

                    // will work for these menus as well
                    KnownHashes::P_Controls_Popup_Joy_Selection,
                };
                
                if (CMenusManager::IsMenuDisplayed(KnownHashes::P_Controls_joystick))
                    return L"P_Controls_joystick";
                else if (!CMenusManager::IsMenuDisplayed(KnownHashes::P_Map) && std::any_of(std::begin(opsatHashes), std::end(opsatHashes), [](KnownHashes hash) { return CMenusManager::IsMenuDisplayed(hash); }))
                    return L"Opsat";
                else if (std::any_of(std::begin(menuHashes), std::end(menuHashes), [](KnownHashes hash) { return CMenusManager::IsMenuDisplayed(hash); }))
                    return L"Menu";
                return L"Main";
            });
        }
    }
}