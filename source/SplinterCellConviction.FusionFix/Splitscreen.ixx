module;

#include <stdafx.h>
#include <xinput.h>
#include <fstream> // DEBUG

export module Splitscreen;

import ComVars;

// Split screen, two ways:
// [2INSTANCESPLITSCREEN] two instances of the game on one PC, connected over LAN, each in half of the screen:
// - the first instance starts the second one (SCC_SPLITSCREEN_INSTANCE=2 in its environment), which is closed with it
// - both are windowed, the resolution and window of each is its half of the primary monitor (Layout: top and bottom, left and right, set in Window.ixx)
// - WinMain quits when the "sc5_semaphore" semaphore exists, the second instance uses another one
// - each instance reads its own pad (GamepadInstance1/2), both see it as the first pad, Use Controller is set for the one with a pad
// [SPLITSCREEN] the Xbox 360 split screen in one instance, the PC menu doesn't list the Split Screen connection type, the rest of it is still in the game
// (InitMPConnection TYPE=SPLIT, the split lobby, StartMatch travels with ?Splitscreen):
// - both players' viewports read input, but the second one never gets a pad (joystick id -1), player 1 reads pad 0 and player 2 pad 1,
//   which are mapped to GamepadPlayer1/2, the keyboard and mouse are player 1's (they go to the window)
// In both, pads are read with XInput only and without focus (UWindowsViewport reads them only when the window has focus).
namespace Splitscreen
{
    enum class Layout { TopBottom, LeftRight };
    Layout layout = Layout::TopBottom;

    // pad of each instance or player, 1-4, 0 for none
    int32_t padInstance1 = 1;
    int32_t padInstance2 = 2;
    int32_t padPlayer1 = 0;
    int32_t padPlayer2 = 1;
    bool localSplitscreen = false;              // [SPLITSCREEN] enabled
    std::atomic<bool> localSplitscreenPlaying = false; // two local players right now
    uintptr_t viewportPlayer1 = 0;              // the players' viewports in split screen
    uintptr_t viewportPlayer2 = 0;
    std::unordered_map<uintptr_t, int32_t> controllerPlayers; // controller -> 0/1, the viewport that has it
    uintptr_t(__cdecl* GetEngine)() = nullptr;                 // sub_792B49

    SafetyHookInline shCreateSemaphoreA = {};
    HANDLE WINAPI CreateSemaphoreA(LPSECURITY_ATTRIBUTES attributes, LONG initialCount, LONG maximumCount, LPCSTR name)
    {
        if (!bInstance1 && name && strcmp(name, "sc5_semaphore") == 0)
            name = "sc5_semaphore2";
        return shCreateSemaphoreA.stdcall<HANDLE>(attributes, initialCount, maximumCount, name);
    }

    void LaunchSecondInstance()
    {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring commandLine = GetCommandLineW();
        wchar_t currentDirectory[MAX_PATH] = {};
        GetCurrentDirectoryW(MAX_PATH, currentDirectory);

        // closed together with this instance
        static auto job = CreateJobObjectW(nullptr, nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
        info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &info, sizeof(info));

        SetEnvironmentVariableW(L"SCC_SPLITSCREEN_INSTANCE", L"2");
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};
        if (CreateProcessW(exePath, commandLine.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, currentDirectory, &si, &pi))
        {
            AssignProcessToJobObject(job, pi.hProcess);
            ResumeThread(pi.hThread);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        SetEnvironmentVariableW(L"SCC_SPLITSCREEN_INSTANCE", nullptr);
    }

    decltype(&::XInputGetState) GetState = nullptr;
    decltype(&::XInputSetState) SetState = nullptr;

    // pad the game's index is read from, XUSER_MAX_COUNT for none
    DWORD Pad(DWORD userIndex)
    {
        int32_t pad = 0;
        if (bEnableSplitscreen)
            pad = userIndex == 0 ? (bInstance1 ? padInstance1 : padInstance2) : 0;
        else if (localSplitscreenPlaying)
            pad = userIndex == 0 ? padPlayer1 : userIndex == 1 ? padPlayer2 : 0;
        else if (localSplitscreen && userIndex == 1)
            pad = padPlayer2; // the split screen lobby shows player 2's pad, the menus are used with any
        else
            return userIndex;
        return pad >= 1 && pad <= XUSER_MAX_COUNT ? pad - 1 : XUSER_MAX_COUNT;
    }

    DWORD WINAPI XInputGetState(DWORD userIndex, XINPUT_STATE* state)
    {
        auto pad = Pad(userIndex);
        return pad < XUSER_MAX_COUNT && GetState ? GetState(pad, state) : ERROR_DEVICE_NOT_CONNECTED;
    }

    DWORD WINAPI XInputSetState(DWORD userIndex, XINPUT_VIBRATION* vibration)
    {
        auto pad = Pad(userIndex);
        return pad < XUSER_MAX_COUNT && SetState ? SetState(pad, vibration) : ERROR_DEVICE_NOT_CONNECTED;
    }

    // Pads are found with DirectInput (UWindowsViewport joystick init, sub_8F3E00), each one read with XInput or DirectInput,
    // only XInput is remapped: XInput for every pad
    uint32_t* padsFound = nullptr;  // dword_1431E4C
    uint32_t* padXInput = nullptr;  // dword_1431E34[2]

    // Profile settings (FProfileManager, sub_790ED4), Use Controller at +10h is on for the instance with a pad,
    // or player 1 in split screen (its prompts and settings are player 1's)
    SafetyHookInline shGetProfileSettings = {};
    uint8_t* __fastcall GetProfileSettings(void* profileManager, void* edx)
    {
        auto settings = shGetProfileSettings.fastcall<uint8_t*>(profileManager, edx);
        if (settings && (bEnableSplitscreen || localSplitscreenPlaying))
            *reinterpret_cast<int32_t*>(settings + 0x10) = (bEnableSplitscreen ? (bInstance1 ? padInstance1 : padInstance2) : padPlayer1) > 0;
        return settings;
    }

    // Profiles (FProfileManager, the engine's virtual +1F0h): 4 slots at +A8h, the players' slot indices at +230h and +234h (also their joystick ids).
    // PC signs in one profile, <name>@profile.sav, at startup (UGameEngine sub_7997BB), the second player gets its own:
    // [SPLITSCREEN] in slot 1, [2INSTANCESPLITSCREEN] the second instance's player 1 (both instances would use and save the same one)
    std::string profileName2 = "Guest";
    void(__fastcall* StringFromChars)(void* string, void* edx, const char* text) = nullptr;
    void(__fastcall* StringDestructor)(void* string, void* edx) = nullptr;
    bool startupSignIn = false;

    uintptr_t GetProfileManager()
    {
        auto engine = GetEngine ? GetEngine() : 0;
        return engine ? reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>((*reinterpret_cast<uintptr_t**>(engine))[496 / 4])(engine) : 0;
    }

    // FWinOrbitProfileManager::SignIn(name, slot, a4), virtual +14h (sub_414FFF)
    SafetyHookInline shSignIn = {};
    int __fastcall SignIn(uintptr_t profileManager, void* edx, void* name, int32_t slot, int32_t a4)
    {
        if (!startupSignIn || !bEnableSplitscreen || bInstance1 || profileName2.empty())
            return shSignIn.fastcall<int>(profileManager, edx, name, slot, a4);
        uint8_t name2[16] = {};
        StringFromChars(name2, nullptr, profileName2.c_str());
        auto result = shSignIn.fastcall<int>(profileManager, edx, name2, slot, a4);
        StringDestructor(name2, nullptr);
        return result;
    }

    SafetyHookInline shStartupSignIn = {};
    void __fastcall StartupSignIn(uintptr_t engine, void* edx, int32_t a2, int32_t a3)
    {
        startupSignIn = true;
        shStartupSignIn.fastcall<void>(engine, edx, a2, a3);
        startupSignIn = false;

        auto profileManager = GetProfileManager();
        if (!localSplitscreen || !profileManager || profileName2.empty())
            return;
        auto profiles = reinterpret_cast<uintptr_t*>(profileManager + 0xA8);
        // not player 1's own profile again (profile: its name at +0)
        auto name1 = profiles[0] && *reinterpret_cast<const char**>(profiles[0]) ? *reinterpret_cast<const char**>(profiles[0]) : "";
        if (profiles[1] || _stricmp(name1, profileName2.c_str()) == 0)
            return;
        uint8_t name2[16] = {};
        StringFromChars(name2, nullptr, profileName2.c_str());
        reinterpret_cast<int(__thiscall*)(uintptr_t, void*, int32_t, int32_t)>((*reinterpret_cast<uintptr_t**>(profileManager))[0x14 / 4])(profileManager, name2, 1, 0);
        StringDestructor(name2, nullptr);
    }

    void InitProfiles()
    {
        StringFromChars = reinterpret_cast<decltype(StringFromChars)>(hook::get_pattern("33 C0 56 8B F1 57 8B 7C 24 0C 89 46 04 89 46 08 89 06 89 46 0C 38 07 74"));
        StringDestructor = reinterpret_cast<decltype(StringDestructor)>(hook::get_pattern("56 8B F1 33 C0 39 46 08 89 46 04 74 0A 6A 01 89"));
        GetEngine = reinterpret_cast<decltype(GetEngine)>(hook::get_pattern("83 3D ? ? ? ? 00 75 09 A1 ? ? ? ? 85 C0 75 05 A1 ? ? ? ? C3"));
        shSignIn = safetyhook::create_inline(hook::get_pattern("55 8B EC 53 56 57 FF 75 0C 8B F1 FF 75 08 8B 06 FF 90 00 01 00 00"), SignIn);
        shStartupSignIn = safetyhook::create_inline(hook::get_pattern("55 8B EC 83 EC 14 56 8B F1 8B 06 FF 90 F0 01 00 00 85 C0 0F 84 ? ? ? ? 53 57 B9"), StartupSignIn);
    }

    void* GetProfile1 = nullptr; // sub_7909A0
    // FProfileManager player 2's profile (sub_7909AC, slot at +234h), player 1's if player 2 has none
    SafetyHookInline shGetProfile2 = {};
    uintptr_t __fastcall GetProfile2(uintptr_t profileManager, void* edx)
    {
        if (auto profile = shGetProfile2.fastcall<uintptr_t>(profileManager, edx))
            return profile;
        return reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(GetProfile1)(profileManager);
    }

    // which player a controller is, the viewport (UViewport) that points to it
    int32_t ControllerPlayer(uintptr_t controller)
    {
        if (auto it = controllerPlayers.find(controller); it != controllerPlayers.end())
            return it->second;
        int32_t player = -1;
        for (auto [viewport, index] : { std::pair{ viewportPlayer1, 0 }, { viewportPlayer2, 1 } })
        {
            if (viewport && std::ranges::contains(std::span(reinterpret_cast<uintptr_t*>(viewport), 0x200), controller))
                player = index;
        }
        if (player >= 0) // it can be looked at before the viewport has it
            controllerPlayers[controller] = player;
        //std::ofstream(GetExeModulePath<std::filesystem::path>() / "_camera.log", std::ios::app) << std::format("controller {:X} player {} viewports {:X} {:X}", controller, player, viewportPlayer1, viewportPlayer2) << std::endl; // DEBUG
        return player;
    }

    // CameraMovement (camera: controller at +20h): the controller's +604h & 2 is the pad mode (stick input clamped to 1,
    // mouse input otherwise), it's the same for both players, set it for each one's device
    SafetyHookInline shCameraMovement = {};
    int __fastcall CameraMovement(uintptr_t camera, void* edx)
    {
        auto controller = *reinterpret_cast<uintptr_t*>(camera + 0x20);
        if (localSplitscreenPlaying && controller)
        {
            if (auto player = ControllerPlayer(controller); player >= 0)
            {
                auto& flags = *reinterpret_cast<uint32_t*>(controller + 0x604);
                flags = ((player == 0 ? padPlayer1 : padPlayer2) > 0) ? (flags | 2) : (flags & ~2u);
            }
        }
        auto result = shCameraMovement.fastcall<int>(camera, edx);
        if (localSplitscreenPlaying && controller) // DEBUG
        {
            static int counter[2] = {};
            auto player = ControllerPlayer(controller);
            if (player >= 0 && ++counter[player] % 30 == 0)
            {
                auto input = *reinterpret_cast<uintptr_t*>(camera + 40);
                auto f = [&](int off) { return *reinterpret_cast<float*>(camera + off); };
                XINPUT_STATE state = {};
                if (GetState) GetState(Pad(player), &state);
                std::ofstream(GetExeModulePath<std::filesystem::path>() / "_camera.log", std::ios::app) << std::format("p{} flags {:X} input {:X} vt {:X} vf104 {:X} vf36 {:X} vf44 {:X} dt {} in {} {} last {} {} rx {} ry {} yaw {} pitch {}",
                    player, *reinterpret_cast<uint32_t*>(controller + 0x604), input, input ? *reinterpret_cast<uintptr_t*>(input) : 0,
                    input ? (*reinterpret_cast<uintptr_t**>(input))[104 / 4] : 0, input ? (*reinterpret_cast<uintptr_t**>(input))[36 / 4] : 0, input ? (*reinterpret_cast<uintptr_t**>(input))[44 / 4] : 0,
                    f(0), f(1060), f(1064), f(1112), f(1116), state.Gamepad.sThumbRX, state.Gamepad.sThumbRY, *reinterpret_cast<int32_t*>(camera + 48), *reinterpret_cast<int32_t*>(camera + 52)) << std::endl;
            }
        }
        return result;
    }

    // UWindowsViewport, can the pad be read: has focus (+414h), pads were found, active (+388h, the second split screen viewport never is)
    SafetyHookInline shCanReadPad = {};
    int __fastcall CanReadPad(uintptr_t viewport, void* edx)
    {
        if (!bEnableSplitscreen && !localSplitscreenPlaying)
            return shCanReadPad.fastcall<int>(viewport, edx);
        auto& focus = *reinterpret_cast<int32_t*>(viewport + 0x414);
        auto& active = *reinterpret_cast<int32_t*>(viewport + 0x388);
        auto hadFocus = std::exchange(focus, 1);
        auto wasActive = std::exchange(active, 1);
        auto result = shCanReadPad.fastcall<int>(viewport, edx);
        focus = hadFocus;
        active = wasActive;
        return result;
    }

    // UWindowsViewport input (sub_8F0A0B), the player's joystick id (virtual +A4h get, +A8h set) is the pad index.
    // In split screen (engine +1FCh & 100h, UGameEngine sub_79844B) player 1 is the client's first viewport and player 2 the one at engine +210h.
    SafetyHookInline shReadInput = {};
    int __fastcall ReadInput(uintptr_t viewport, void* edx, int32_t a2, uintptr_t player, double delta)
    {
        auto engine = GetEngine();
        // the split screen lobby gives the players these joystick ids (FWinProfileManager +230h and +234h, -1 on PC)
        if (localSplitscreen && engine)
        {
            auto profileManager = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>((*reinterpret_cast<uintptr_t**>(engine))[496 / 4])(engine);
            if (profileManager)
            {
                auto joystickIds = reinterpret_cast<int32_t*>(profileManager + 560);
                if (joystickIds[0] == -1)
                    joystickIds[0] = 0;
                if (joystickIds[1] == -1)
                    joystickIds[1] = 1;
            }
        }
        auto client = engine ? *reinterpret_cast<uintptr_t*>(engine + 0x44) : 0;
        auto secondViewport = engine && (*reinterpret_cast<uint16_t*>(engine + 0x1FC) & 0x100) ? *reinterpret_cast<uintptr_t*>(engine + 0x210) : 0;
        auto firstViewport = client && *reinterpret_cast<int32_t*>(client + 48) > 0 ? **reinterpret_cast<uintptr_t**>(client + 44) : 0;
        localSplitscreenPlaying = firstViewport && secondViewport;
        if (localSplitscreenPlaying && player && (viewport == firstViewport || viewport == secondViewport))
        {
            padXInput[0] = padXInput[1] = 1;
            auto vtable = *reinterpret_cast<uintptr_t**>(player);
            int32_t index = viewport == firstViewport ? 0 : 1;
            if (std::exchange(index == 0 ? viewportPlayer1 : viewportPlayer2, viewport) != viewport)
                controllerPlayers.clear();
            if (reinterpret_cast<int32_t(__thiscall*)(uintptr_t)>(vtable[0xA4 / 4])(player) != index)
                reinterpret_cast<void(__thiscall*)(uintptr_t, int32_t)>(vtable[0xA8 / 4])(player, index);
        }
        return shReadInput.fastcall<int>(viewport, edx, a2, player, delta);
    }

    void InitPads()
    {
        static bool once = false;
        if (std::exchange(once, true))
            return;

        // XInput is imported by ordinal, the import slots are taken from the jump thunks
        auto pattern = hook::pattern("8D 4C 24 04 E9 ? ? ? ? CC FF 25 ? ? ? ? FF 25 ? ? ? ?");
        auto setStateSlot = *pattern.get_first<decltype(SetState)*>(12);
        auto getStateSlot = *pattern.get_first<decltype(GetState)*>(18);
        SetState = *setStateSlot;
        GetState = *getStateSlot;
        injector::WriteMemory(setStateSlot, &XInputSetState, true);
        injector::WriteMemory(getStateSlot, &XInputGetState, true);

        pattern = hook::pattern("89 BE ? ? ? ? 89 3D ? ? ? ? EB 06 89 9E ? ? ? ? 83 C6 04 83 FE 08 0F 8C ? ? ? ? 5F 5E 5B C9 C3");
        padXInput = *pattern.get_first<uint32_t*>(2) - 2; // dword_1431E3C - 8
        padsFound = *pattern.get_first<uint32_t*>(8);
        static auto PadsFoundHook = safetyhook::create_mid(pattern.get_first(32), [](SafetyHookContext& regs)
        {
            if (!bEnableSplitscreen)
                return;
            // an instance without a pad doesn't find any
            if ((bInstance1 ? padInstance1 : padInstance2) <= 0)
                *padsFound = 0;
            else
                padXInput[0] = padXInput[1] = 1;
        });

        pattern = hook::pattern("33 C0 39 81 14 04 00 00 74 ? 39 05");
        shCanReadPad = safetyhook::create_inline(pattern.get_first(), CanReadPad);
    }
}

// Half of the primary monitor this instance uses
export RECT GetSplitscreenRect()
{
    MONITORINFO info = { sizeof(info) };
    GetMonitorInfo(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &info);
    auto r = info.rcMonitor;
    if (Splitscreen::layout == Splitscreen::Layout::LeftRight)
    {
        auto middle = r.left + (r.right - r.left) / 2;
        return bInstance1 ? RECT{ r.left, r.top, middle, r.bottom } : RECT{ middle, r.top, r.right, r.bottom };
    }
    auto middle = r.top + (r.bottom - r.top) / 2;
    return bInstance1 ? RECT{ r.left, r.top, r.right, middle } : RECT{ r.left, middle, r.right, r.bottom };
}

export void InitSplitscreen()
{
    if (!bEnableSplitscreen)
        return;

    wchar_t instance[8] = {};
    bInstance1 = GetEnvironmentVariableW(L"SCC_SPLITSCREEN_INSTANCE", instance, 8) == 0 || wcscmp(instance, L"2") != 0;
    if (bInstance1)
        Splitscreen::LaunchSecondInstance();

    CIniReader iniReader("");
    Splitscreen::layout = iniReader.ReadInteger("2INSTANCESPLITSCREEN", "Layout", 0) == 1 ? Splitscreen::Layout::LeftRight : Splitscreen::Layout::TopBottom;
    Splitscreen::padInstance1 = iniReader.ReadInteger("2INSTANCESPLITSCREEN", "GamepadInstance1", 1);
    Splitscreen::padInstance2 = iniReader.ReadInteger("2INSTANCESPLITSCREEN", "GamepadInstance2", 2);
    Splitscreen::profileName2 = iniReader.ReadString("2INSTANCESPLITSCREEN", "ProfileInstance2", "Guest");

    Splitscreen::shCreateSemaphoreA = safetyhook::create_inline(::CreateSemaphoreA, Splitscreen::CreateSemaphoreA);
    Splitscreen::InitPads();
    Splitscreen::InitProfiles();

    auto pattern = hook::pattern("FF B1 30 02 00 00 E8 ? ? ? ? 50 E8 ? ? ? ? C3");
    Splitscreen::shGetProfileSettings = safetyhook::create_inline(pattern.get_first(), Splitscreen::GetProfileSettings);
}

namespace LocalSplitscreen
{
    const wchar_t* (__cdecl* Localize)(const wchar_t* section, const wchar_t* key, const char* file, int32_t, int32_t) = nullptr;
    void(__fastcall* StringFromWide)(void* string, void* edx, const wchar_t* text) = nullptr;
    void(__fastcall* StringDestructor)(void* string, void* edx) = nullptr;
    void(__fastcall* AddString)(void* array, void* edx, void* string) = nullptr;
    void(__fastcall* AddInt)(void* array, void* edx, int32_t* value) = nullptr;

    // CFlashMenuPreGameManager command handler (sub_D7B89A). GetPlayerJoystickID MAIN=0 gives the lobby player 2's joystick id from the
    // viewport's second input (+96), which only the Xbox 360 sign in creates, player 2's pad icon is set instead.
    uintptr_t(__cdecl* GetFlashMenu)() = nullptr;
    // sub_88C08A(viewport, main, disable): SplitChangeInput turns off the input of the player not being edited, the viewport's second one (+96)
    // doesn't exist on PC, so switching to player 2 would turn off the only one. The menu input stays with the first one.
    SafetyHookInline shSetPlayerInputDisabled = {};
    int __fastcall SetPlayerInputDisabled(void* player, void* edx, uintptr_t viewport, int32_t main, int32_t disable)
    {
        if (viewport && *reinterpret_cast<uintptr_t*>(viewport + 96) == 0)
            disable = 0;
        return shSetPlayerInputDisabled.fastcall<int>(player, edx, viewport, main, disable);
    }

    // sub_785021(main, second): the closet / arsenal (ActivateCloset, ActivatePEC) enables only the input of the player being customized,
    // player 2's is the missing second one, so the menu got no input and the page couldn't be left.
    SafetyHookInline shSetCustomizationInput = {};
    void __stdcall SetCustomizationInput(int32_t main, int32_t second)
    {
        shSetCustomizationInput.stdcall<void>(1, second);
    }

    SafetyHookInline shMenuCommand = {};
    int __fastcall MenuCommand(void* manager, void* edx, const char* command, const char* args, int32_t a4)
    {
        auto result = shMenuCommand.fastcall<int>(manager, edx, command, args, a4);
        if (command && !std::string_view(command).starts_with("PlaySound") && !std::string_view(command).starts_with("Localize")) std::ofstream(GetExeModulePath<std::filesystem::path>() / "_menucommands.log", std::ios::app) << command << " | " << (args ? args : "") << " -> " << result << std::endl; // DEBUG
        if (command && args && std::string_view(command).starts_with("GetPlayerJoystickID") && std::string_view(args).contains("MAIN=0"))
        {
            if (auto flashMenu = GetFlashMenu())
            {
                uint8_t path[16] = {};
                Splitscreen::StringFromChars(path, nullptr, "_root.FlashMenu.frame.lobby.player2ID");
                reinterpret_cast<void(__thiscall*)(uintptr_t, void*, float)>((*reinterpret_cast<uintptr_t**>(flashMenu))[108 / 4])(flashMenu, path, 1.0f);
                Splitscreen::StringDestructor(path, nullptr);
            }
        }
        return result;
    }
}

export void InitLocalSplitscreen()
{
    CIniReader iniReader("");
    if (iniReader.ReadInteger("SPLITSCREEN", "Enable", 0) == 0 || bEnableSplitscreen)
        return;

    Splitscreen::localSplitscreen = true;
    Splitscreen::padPlayer1 = iniReader.ReadInteger("SPLITSCREEN", "GamepadPlayer1", 0);
    Splitscreen::padPlayer2 = iniReader.ReadInteger("SPLITSCREEN", "GamepadPlayer2", 1);
    Splitscreen::profileName2 = iniReader.ReadString("SPLITSCREEN", "ProfilePlayer2", "Guest");
    Splitscreen::InitPads();
    Splitscreen::InitProfiles();

    using namespace LocalSplitscreen;

    // CFlashMenuPreGameManager, GetConnectionTypes: names at [ebp+44h], ids at [ebp+04h], available at [ebp+34h], PLAYERMATCH= at [ebp+7Ch]
    auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 14 50 8D 4D 1C E8 ? ? ? ? 8D 45 1C 50 8D 4D 44 E8 ? ? ? ? 8D 4D 1C E8 ? ? ? ? 8D 45 FC 50 8D 4D 34 E8 ? ? ? ? 68");
    Localize = reinterpret_cast<decltype(Localize)>(injector::GetBranchDestination(pattern.get_first(0)).as_int());
    StringFromWide = reinterpret_cast<decltype(StringFromWide)>(injector::GetBranchDestination(pattern.get_first(12)).as_int());
    AddString = reinterpret_cast<decltype(AddString)>(injector::GetBranchDestination(pattern.get_first(24)).as_int());
    StringDestructor = reinterpret_cast<decltype(StringDestructor)>(injector::GetBranchDestination(pattern.get_first(32)).as_int());
    AddInt = reinterpret_cast<decltype(AddInt)>(injector::GetBranchDestination(pattern.get_first(44)).as_int());
    static auto ConnectionTypesHook = safetyhook::create_mid(pattern.get_first(49), [](SafetyHookContext& regs)
    {
        if (*reinterpret_cast<int32_t*>(regs.ebp + 0x7C) != 0) // player match
            return;
        uint8_t name[32] = {};
        StringFromWide(name, nullptr, Localize(L"CONNECTIONTYPE", L"SplitScreen", "Localization\\Menus", 0, 0));
        AddString(reinterpret_cast<void*>(regs.ebp + 0x44), nullptr, name);
        StringDestructor(name, nullptr);
        int32_t id = 2, available = 1;
        AddInt(reinterpret_cast<void*>(regs.ebp + 0x04), nullptr, &id);
        AddInt(reinterpret_cast<void*>(regs.ebp + 0x34), nullptr, &available);
    });

    GetFlashMenu = reinterpret_cast<decltype(GetFlashMenu)>(hook::get_pattern("E8 ? ? ? ? 85 C0 74 ? E8 ? ? ? ? 8B 40 4C C3"));
    shSetPlayerInputDisabled = safetyhook::create_inline(hook::get_pattern("8B 44 24 04 85 C0 56 8B F1 74 2D 83 7C 24 0C 00"), SetPlayerInputDisabled);
    shSetCustomizationInput = safetyhook::create_inline(hook::get_pattern("57 E8 ? ? ? ? 8B F8 85 FF 74 41 56 E8 ? ? ? ? 8B 10 8B C8 FF 92 A4 00 00 00 8B F0 85 F6 74 2A 6A 01"), SetCustomizationInput);
    shMenuCommand = safetyhook::create_inline(hook::get_pattern("55 8D 6C 24 94 81 EC 20 02 00 00 53 56 8B 75 78"), MenuCommand);

    // engine (sub_792B49), its client at +44h has the viewports at +2Ch
    Splitscreen::shReadInput = safetyhook::create_inline(hook::get_pattern("55 8D AC 24 30 FD FF FF 81 EC 50 03 00 00 A1"), Splitscreen::ReadInput);
    Splitscreen::shCameraMovement = safetyhook::create_inline(hook::get_pattern("55 8B EC 83 EC 2C 53 56 8B D9 57 8D B3 28 04 00 00"), Splitscreen::CameraMovement);
    Splitscreen::shGetProfileSettings = safetyhook::create_inline(hook::get_pattern("FF B1 30 02 00 00 E8 ? ? ? ? 50 E8 ? ? ? ? C3"), Splitscreen::GetProfileSettings);
    Splitscreen::GetProfile1 = hook::get_pattern("FF B1 30 02 00 00 E8 ? ? ? ? C3");
    Splitscreen::shGetProfile2 = safetyhook::create_inline(hook::get_pattern("FF B1 34 02 00 00 E8 ? ? ? ? C3"), Splitscreen::GetProfile2);
}
