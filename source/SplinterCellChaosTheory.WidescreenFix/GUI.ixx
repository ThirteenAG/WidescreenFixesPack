module;

#include <stdafx.h>

export module GUI;

import ComVars;
import HudIDs;

namespace CMenusManager
{
    SafetyHookInline shDisplayMenu = {};
    void __fastcall DisplayMenu(int* menusManager, void* edx, void* entry, char show, char a4, char a5)
    {
        uint32_t hash = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(entry) + 0x14);
        //IsMenuDisplayedCache[hash] = (show != 0);
        shDisplayMenu.unsafe_fastcall(menusManager, edx, entry, show, a4, a5);
    }

    uint32_t GetMenuItemHash(void* item)
    {
        if (!item) return 0;
        return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(item) + 0x14);
    }

    SafetyHookInline shDisplayMenuAtPos = {};
    void __fastcall DisplayMenuAtPos(int* menusManager, void* edx, void* entry, char a3, char a4)
    {
        if (entry)
        {
            auto hash = GetMenuItemHash(entry);
            //IsMenuDisplayedCache[hash] = true;
        }
        shDisplayMenuAtPos.unsafe_fastcall(menusManager, edx, entry, a3, a4);
    }

    SafetyHookInline shRemoveMenuItems = {};
    void __fastcall RemoveMenuItems(int* menusManager, void* edx, char a2, char a3)
    {
        // Extract and cache items BEFORE they're removed
        int itemPtr = menusManager[1];
        if (itemPtr)
        {
            int itemCount = (menusManager[2] - itemPtr) >> 3;

            // Find the item that will be removed
            int index = itemCount - 1;
            if (index >= 0)
            {
                char* itemByte = reinterpret_cast<char*>(itemPtr + 8 * index + 5);
                while (*itemByte > a3 && index > 0)
                {
                    --index;
                    itemByte -= 8;
                }

                if (index >= 0)
                {
                    int* itemToRemove = *reinterpret_cast<int**>(itemPtr + 8 * index);
                    if (itemToRemove)
                    {
                        uint32_t hash = GetMenuItemHash(itemToRemove);
                        //IsMenuDisplayedCache[hash] = false;
                    }
                }
            }
        }

        shRemoveMenuItems.unsafe_fastcall(menusManager, edx, a2, a3);
    }

    SafetyHookInline shOnMenuItemDeleted = {};
    void __fastcall OnMenuItemDeleted(int* item, void* edx)
    {
        IsMenuDisplayedCache.clear();
        shOnMenuItemDeleted.unsafe_fastcall(item, edx);
    }

    SafetyHookInline shsub_10D30900 = {};
    void __fastcall sub_10D30900(void* item, void* edx, int a2)
    {
        if ((*(uint8_t*)((uintptr_t)item + 32) & 1) != 0)
        {
            uint32_t hash = GetMenuItemHash(item);
            IsMenuDisplayedCache[hash] = true;
        }
        return shsub_10D30900.unsafe_fastcall(item, edx, a2);
    }

    export bool IsMenuDisplayed(uint32_t hash)
    {
        auto it = IsMenuDisplayedCache.find(hash);
        return (it != IsMenuDisplayedCache.end()) ? it->second : false;
    }

    export bool IsMenuDisplayed(HudID hash)
    {
        return IsMenuDisplayed(uint32_t(hash));
    }

    export bool IsMenuDisplayed(const char* name)
    {
        if (!name)
            return false;

        uint32_t hash = crc32(0, name, std::strlen(name));
        return IsMenuDisplayed(hash);
    }

    export bool IsMainMenuDisplayed()
    {
        static const HudID menuHashes[] = {
            Page::P_Briefing,
            Page::P_CDKeyEntry,
            Page::P_CamControl,
            Page::P_Camera,
            Page::P_ChatIngame,
            Page::P_ContextMenu,
            Page::P_Controls_Popup_Joy_Selection,
            Page::P_Controls_Popup_Selection,
            //Page::P_Controls_joystick,
            Page::P_Controls_keyboard,
            Page::P_CreateAccount,
            Page::P_CreateProfile,
            Page::P_Difficulty,
            Page::P_DisplayAdvanced,
            Page::P_DisplaySettings,
            //Page::P_EEV,
            Page::P_Email,
            Page::P_Empty,
            Page::P_EnterServerPassword,
            Page::P_Equipment,
            Page::P_Extras,
            //Page::P_FadeBlack,
            Page::P_Footer_training2,
            Page::P_FriendsContext,
            Page::P_FriendsManagement,
            Page::P_GameOverOnline,
            Page::P_Image,
            Page::P_LoadOut,
            Page::P_LoadOut2,
            Page::P_LoadingScreen,
            Page::P_LobbyLan,
            Page::P_LobbyOnline,
            Page::P_Login,
            Page::P_ManageAccount,
            Page::P_ManageGame,
            //Page::P_Map,
            Page::P_MenuCoop,
            Page::P_MenuGameOver,
            Page::P_MenuGameType,
            Page::P_MenuLoadGame,
            Page::P_MenuLoadLevel,
            Page::P_MenuLobbyList,
            Page::P_MenuOnline,
            Page::P_MenuRoomCreateLan,
            Page::P_MenuRoomListLan,
            Page::P_MenuSaveGame,
            Page::P_MenuSettings,
            Page::P_MenuSolo,
            Page::P_MissionStatistics,
            Page::P_Movies,
            //Page::P_MsgBox,
            //Page::P_MsgBox_Training,
            //Page::P_MsgBox_Training2,
            Page::P_MusicsBio,
            Page::P_MusicsList,
            Page::P_ObjectiveReview,
            Page::P_OfflineOnline,
            Page::P_OnlineCreate,
            Page::P_OnlineESRB,
            Page::P_OnlineFilter,
            Page::P_OnlineQuickMatchFilter,
            Page::P_OnlineTermsOfUse,
            Page::P_OptionPopup,
            Page::P_OverallAchievement,
            Page::P_Pause,
            Page::P_PauseOnline,
            Page::P_PrivateMessage,
            Page::P_Profiles,
            Page::P_Progress,
            Page::P_RoomOnline,
            Page::P_SavingScreen,
            Page::P_SavingScreen1,
            //Page::P_ShaderAdvanced,
            //Page::P_Sniper,
            Page::P_Sound,
            Page::P_SoundSettings,
            Page::P_TrainingVideo,
            Page::P_TrainingVideoMain,
            //Page::P_endmission,
            Page::P_opticcable,
            Page::P_LoadOut_Infos,
            Page::P_LoadOut_Selection,
        };

        return std::any_of(std::begin(menuHashes), std::end(menuHashes), [](HudID hash)
        {
            return CMenusManager::IsMenuDisplayed(hash);
        });
    }

    export bool IsOpsatDisplayed()
    {
        static const HudID opsatHashes[] = {
            Page::P_KeyPad,
            Page::P_Opsat,
            Page::P_Goals,
            Page::P_Goals_details,
            Page::P_Notes,
            Page::P_Datas,
            Page::P_Computer,

            Page::P_QuickInventory_New,
            Page::P_QuickInventory_pc,
        };

        return std::any_of(std::begin(opsatHashes), std::end(opsatHashes), [](HudID hash)
        {
            return CMenusManager::IsMenuDisplayed(hash);
        });
    }

    // Helper to compute delta and bounds for X axis
    inline void GetXAxisBounds(int16_t& outDelta, int16_t& outVmin, int16_t& outVmax)
    {
        outDelta = static_cast<int16_t>(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
        outVmin = static_cast<int16_t>(0 - outDelta);
        outVmax = static_cast<int16_t>(640 + outDelta);
    }

    // Helper to convert raw input to normalized [0, 1] range
    inline float NormalizeRawInputX(int16_t rawX)
    {
        return static_cast<float>(rawX) / static_cast<float>(Screen.Width);
    }

    inline float NormalizeRawInputY(int16_t rawY)
    {
        return static_cast<float>(rawY) / static_cast<float>(Screen.Height);
    }

    // Helper to convert normalized coordinates to game space
    inline int16_t NormalizedToGameX(float normalized, int16_t minGame, int16_t maxGame)
    {
        return static_cast<int16_t>(normalized * (maxGame - minGame) + minGame);
    }

    inline int16_t NormalizedToGameY(float normalized, int16_t minGame, int16_t maxGame)
    {
        return static_cast<int16_t>(normalized * (maxGame - minGame) + minGame);
    }

    // Helper to convert game coordinates back to raw input space
    inline int16_t GameToRawInputX(int16_t gameX, int16_t vmin, int16_t vmax)
    {
        float normalized = static_cast<float>(gameX - vmin) / static_cast<float>(vmax - vmin);
        return static_cast<int16_t>(normalized * Screen.Width);
    }

    inline int16_t GameToRawInputY(int16_t gameY, int16_t minGame, int16_t maxGame)
    {
        float normalized = static_cast<float>(gameY) / static_cast<float>(maxGame);
        return static_cast<int16_t>(normalized * Screen.Height);
    }
}

int16_t __cdecl ClampXAxisState(int16_t value, int16_t min, int16_t max)
{
    int16_t delta, vmin, vmax;
    CMenusManager::GetXAxisBounds(delta, vmin, vmax);
    return std::clamp(value, int16_t(0 - delta), int16_t(640 + delta));
}

int16_t __cdecl ClampYAxisState(int16_t value, int16_t min, int16_t max)
{
    max += 15;
    return std::clamp(value, min, max);
}

export void InitGUI()
{
    auto pattern = hook::pattern("3B C5 89 44 24 44 7D ? 66 89 6C 24 40");
    if (!pattern.empty())
    {
        struct USCMagmaInteractionProcessInputAxisStateHookX
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esp + 0x44) = regs.eax;
                *(int16_t*)(regs.esp + 0x40) = ClampXAxisState(*(int16_t*)&regs.eax, int16_t(0), int16_t(640));
            }
        }; injector::MakeInline<USCMagmaInteractionProcessInputAxisStateHookX>(pattern.get_first(0), pattern.get_first(33));
    }

    pattern = hook::pattern("3B C5 89 44 24 44 7D ? 66 89 6C 24 42");
    if (!pattern.empty())
    {
        struct USCMagmaInteractionProcessInputAxisStateHookY
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esp + 0x44) = regs.eax;
                *(int16_t*)(regs.esp + 0x42) = ClampYAxisState(*(int16_t*)&regs.eax, int16_t(0), int16_t(480));
            }
        }; injector::MakeInline<USCMagmaInteractionProcessInputAxisStateHookY>(pattern.get_first(0), pattern.get_first(33));
    }

    pattern = hook::pattern("0F BF 42 02 85 C0");
    if (!pattern.empty())
    {
        struct CMenusManagerSetMousePositionHook
        {
            void operator()(injector::reg_pack& regs)
            {
                int16_t delta, vmin, vmax;
                CMenusManager::GetXAxisBounds(delta, vmin, vmax);

                int16_t clampedGameX = std::clamp(*(int16_t*)(regs.edx + 0), int16_t(0 - delta), int16_t(640 + delta));
                int16_t clampedGameY = std::clamp(*(int16_t*)(regs.edx + 2), int16_t(0), int16_t(480));

                if (Screen.fRawInputMouse > 0.0f)
                {
                    RawInputHandler<>::RawMouseCursorX = CMenusManager::GameToRawInputX(clampedGameX, vmin, vmax);
                    RawInputHandler<>::RawMouseCursorY = CMenusManager::GameToRawInputY(clampedGameY, 0, 480);
                }

                *(int16_t*)&regs.eax = clampedGameX;
                *(int16_t*)&regs.ecx = clampedGameY;
            }
        }; injector::MakeInline<CMenusManagerSetMousePositionHook>(pattern.get_first(0), pattern.get_first(35));
    }

    //pattern = hook::pattern("51 53 8B 5C 24 0C 85 DB");
    //CMenusManager::shDisplayMenu = safetyhook::create_inline(pattern.get_first(), CMenusManager::DisplayMenu);
    //
    //pattern = hook::pattern("83 EC 08 53 56 8B F1 8B 4E 04");
    //CMenusManager::shDisplayMenuAtPos = safetyhook::create_inline(pattern.get_first(), CMenusManager::DisplayMenuAtPos);
    //
    //pattern = hook::pattern("83 EC 0C 53 55 56 57 8B F1 C6 44 24 13 00");
    //CMenusManager::shRemoveMenuItems = safetyhook::create_inline(pattern.get_first(), CMenusManager::RemoveMenuItems);

    pattern = hook::pattern("56 57 8B F9 8B 47 28 85 C0 74");
    CMenusManager::shOnMenuItemDeleted = safetyhook::create_inline(pattern.get_first(), CMenusManager::OnMenuItemDeleted);

    pattern = hook::pattern("53 8B 5C 24 ? 56 8B F1 F6 46");
    CMenusManager::shsub_10D30900 = safetyhook::create_inline(pattern.get_first(), CMenusManager::sub_10D30900);
}
