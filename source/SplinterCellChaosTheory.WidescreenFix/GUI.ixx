module;

#include <stdafx.h>

export module GUI;

import ComVars;
import Hashes;

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
            IsMenuDisplayedCache[hash] = true;
        }
        shDisplayMenuAtPos.unsafe_fastcall(menusManager, edx, entry, a3, a4);
    }

    SafetyHookInline shRemoveMenuItems = {};
    void __fastcall RemoveMenuItems(int* menusManager, void* edx, char a2, char a3)
    {
        // Extract and cache items before they're removed
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
                        IsMenuDisplayedCache[hash] = false;
                    }
                }
            }
        }

        shRemoveMenuItems.unsafe_fastcall(menusManager, edx, a2, a3);
    }

    SafetyHookInline shOnMenuItemDeleted = {};
    void __fastcall OnMenuItemDeleted(int* item, void* edx)
    {
        shOnMenuItemDeleted.unsafe_fastcall(item, edx);
    }

    export bool IsMenuDisplayed(uint32_t hash)
    {
        auto it = IsMenuDisplayedCache.find(hash);
        return (it != IsMenuDisplayedCache.end()) ? it->second : false;
    }

    export bool IsMenuDisplayed(KnownHashes hash)
    {
        return IsMenuDisplayed(std::to_underlying(hash));
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
        static const KnownHashes menuHashes[] = {
            //KnownHashes::P_ChatIngame,
            //KnownHashes::P_OptionPopup,
            //KnownHashes::P_SavingScreen,
            //KnownHashes::P_Progress,
            //KnownHashes::P_ContextMenu,
            //KnownHashes::P_endmission,
            //KnownHashes::P_MenuLoadLevel,
            //KnownHashes::P_MenuLoadGame,
            //KnownHashes::P_MenuSaveGame,
            //KnownHashes::P_MenuGameOver,
            KnownHashes::P_MenuOnline,
            KnownHashes::P_MenuLobbyList,
            KnownHashes::P_MenuRoomListLan,
            KnownHashes::P_MenuRoomCreateLan,
            KnownHashes::P_MenuCoop,
            //KnownHashes::P_MenuGameType,
            KnownHashes::P_MenuSolo,
            //KnownHashes::P_PauseOnline,
            //KnownHashes::P_Pause,
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
            //KnownHashes::P_Difficulty,
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

        return std::any_of(std::begin(menuHashes), std::end(menuHashes), [](KnownHashes hash)
        {
            return CMenusManager::IsMenuDisplayed(hash);
        });
    }

    export bool IsOpsatDisplayed()
    {
        static const KnownHashes opsatHashes[] = {
            KnownHashes::P_KeyPad,
            KnownHashes::P_Opsat,
            KnownHashes::P_Goals,
            KnownHashes::P_Goals_details,
            KnownHashes::P_Notes,
            KnownHashes::P_Datas,
            KnownHashes::P_Computer,
        };

        return std::any_of(std::begin(opsatHashes), std::end(opsatHashes), [](KnownHashes hash)
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

    if (Screen.fRawInputMouseForMenu > 0.0f)
    {
        float normalizedX = CMenusManager::NormalizeRawInputX(RawInputHandler<>::RawMouseCursorX);
        int16_t menuX = CMenusManager::NormalizedToGameX(normalizedX, vmin, vmax);
        return std::clamp(menuX, vmin, vmax);
    }
    else
        return std::clamp(value, int16_t(0 - delta), int16_t(640 + delta));
}

int16_t __cdecl ClampYAxisState(int16_t value, int16_t min, int16_t max)
{
    max += 15;

    if (Screen.fRawInputMouseForMenu > 0.0f)
    {
        float normalizedY = CMenusManager::NormalizeRawInputY(RawInputHandler<>::RawMouseCursorY);
        int16_t menuY = CMenusManager::NormalizedToGameY(normalizedY, min, max);
        return std::clamp(menuY, min, max);
    }
    else
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

                if (Screen.fRawInputMouseForMenu > 0.0f)
                {
                    RawInputHandler<>::RawMouseCursorX = CMenusManager::GameToRawInputX(clampedGameX, vmin, vmax);
                    RawInputHandler<>::RawMouseCursorY = CMenusManager::GameToRawInputY(clampedGameY, 0, 480);
                }

                *(int16_t*)&regs.eax = clampedGameX;
                *(int16_t*)&regs.ecx = clampedGameY;
            }
        }; injector::MakeInline<CMenusManagerSetMousePositionHook>(pattern.get_first(0), pattern.get_first(35));
    }

    pattern = hook::pattern("51 53 8B 5C 24 0C 85 DB");
    CMenusManager::shDisplayMenu = safetyhook::create_inline(pattern.get_first(), CMenusManager::DisplayMenu);

    pattern = hook::pattern("83 EC 08 53 56 8B F1 8B 4E 04");
    CMenusManager::shDisplayMenuAtPos = safetyhook::create_inline(pattern.get_first(), CMenusManager::DisplayMenuAtPos);

    pattern = hook::pattern("83 EC 0C 53 55 56 57 8B F1 C6 44 24 13 00");
    CMenusManager::shRemoveMenuItems = safetyhook::create_inline(pattern.get_first(), CMenusManager::RemoveMenuItems);

    //pattern = hook::pattern("56 57 8B F9 8B 47 28 85 C0 74");
    //CMenusManager::shOnMenuItemDeleted = safetyhook::create_inline(pattern.get_first(), CMenusManager::OnMenuItemDeleted);
}
