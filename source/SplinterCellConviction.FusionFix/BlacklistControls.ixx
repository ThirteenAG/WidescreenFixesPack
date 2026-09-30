module;

#include <stdafx.h>

export module BlacklistControls;

import ComVars;

// Blacklist control layout. The button layout is data, loaded from update instead of the originals:
//   blacklist.ini                       ActionScheme.ini, the actions of every pad button (keyboard keys are translated to pad buttons)
//   data\Blacklist\Menus.*              Localization\Menus.*, the button names in the controls menu
//   data\Blacklist\Sequences\...        data\Sequences\..., scripted scenes enable buttons (PlayerEnableButton) and check which one
//                                       was pressed by pad button, remapped the same way: B -> X (attack), LT -> B (cover), X -> LB (gadget),
//                                       LB -> left stick (crouch), left stick -> right stick (reload), right stick -> LT (aim)
// The code adds what the layout can't do with the game's events: tap to toggle cover, hold to aim and hold to sprint.
// Everything is per pawn, in coop the pawn events and states also run for the other player.

// Returns the Blacklist version of a file the game opens, if it's in update
export std::optional<std::string> BlacklistControlsOverride(std::string_view path)
{
    auto find_i = [](std::string_view str, std::string_view what) -> size_t
    {
        auto it = std::ranges::search(str, what, [](char a, char b) { return ::tolower(a) == ::tolower(b); });
        return it.empty() ? std::string_view::npos : static_cast<size_t>(it.begin() - str.begin());
    };

    std::string result;
    if (auto file = path.substr(path.find_last_of("\\/") + 1); _stricmp(std::string(file).c_str(), "ActionScheme.ini") == 0)
        result = std::string(path.substr(0, path.size() - file.size())) + "blacklist.ini";
    else if (auto pos = find_i(path, "Localization\\Menus"); pos != std::string_view::npos)
        result = std::string(path.substr(0, pos)) + "Blacklist\\Menus" + std::string(path.substr(pos + 18));
    else if (auto pos = find_i(path, "data\\Sequences\\"); pos != std::string_view::npos)
        result = "..\\..\\data\\Blacklist\\Sequences\\" + std::string(path.substr(pos + 15)); // opened by the absolute path

    if (result.empty() || !GetOverloadedFilePathA(result.c_str(), nullptr, 0))
        return std::nullopt;
    return result;
}

namespace BlacklistControls
{
    // AEPlayerPawn
    constexpr auto PrecisionModeFlags = 0xA3C; // 0x1000000: precision mode (aiming)
    constexpr auto B2WFlags = 0xA44;           // 0x100: wants back to wall (cover), DetectB2W sets it and StopDetectB2W clears it
    constexpr auto MaxClampedSpeed = 0xBC4;

    struct PawnState
    {
        bool sprint = false;
        bool crouchSprint = false;
        bool runDisabled = false; // by aiming, precision mode can't be entered while running
        float maxClampedSpeed = 0.0f;
        bool inCover = false;     // in one of the cover states
        bool coverLeft = false;   // a cover state ended, if no other one begins the pawn left cover (opened a door, took down someone...)
        std::chrono::steady_clock::time_point coverLeftTime;
        bool coverRequested = false;
        std::chrono::steady_clock::time_point coverRequestTime;
    };
    std::unordered_map<uintptr_t, PawnState> pawns;

    int32_t Sprint, SprintOff, CrouchSprint, CrouchSprintOff, PrecisionModeOn, PrecisionModeOff, DetectB2W, StopDetectB2W;

    void(__fastcall* FName)(int32_t* out, void* edx, const char* name, int32_t findType) = nullptr;
    void(__fastcall* TogglePrecisionMode)(uintptr_t pawn, void* edx) = nullptr;
    void(__fastcall* DisableRun)(uintptr_t pawn, void* edx, int32_t disable) = nullptr;

    uint32_t& Flags(uintptr_t pawn, uint32_t offset) { return *reinterpret_cast<uint32_t*>(pawn + offset); }

    void SetPrecisionMode(uintptr_t pawn, PawnState& state, bool on)
    {
        if (on && !state.runDisabled)
        {
            state.maxClampedSpeed = *reinterpret_cast<float*>(pawn + MaxClampedSpeed);
            DisableRun(pawn, nullptr, 1);
            state.runDisabled = true;
        }
        else if (!on && state.runDisabled)
        {
            *reinterpret_cast<float*>(pawn + MaxClampedSpeed) = state.maxClampedSpeed;
            state.runDisabled = false;
        }

        // TogglePrecisionMode doesn't toggle while running or busy, on is sent again while the button is held
        if (on != ((Flags(pawn, PrecisionModeFlags) & 0x1000000) != 0))
            TogglePrecisionMode(pawn, nullptr);
    }

    // any way out of cover counts as leaving it, only the button takes cover again
    void LeaveCover(uintptr_t pawn, PawnState& state)
    {
        Flags(pawn, B2WFlags) &= ~0x100;
        state.inCover = false;
        state.coverLeft = false;
        state.coverRequested = false;
    }

    void ToggleCover(uintptr_t pawn, PawnState& state)
    {
        if (state.inCover)
            LeaveCover(pawn, state);
        else
        {
            state.coverLeft = false;
            Flags(pawn, B2WFlags) |= 0x100;
            state.coverRequested = true;
            state.coverRequestTime = std::chrono::steady_clock::now();
        }
    }

    // Cover states: BeginState (vtable slot 41) and EndState (slot 43), the pawn is at +5Ch
    constexpr std::array CoverStates = { ".?AVUCoverNavStartState@@", ".?AVUCoverNavState@@", ".?AVUCoverToCoverMkIIState@@", ".?AVUCoverShootToTargetState@@",
                                         ".?AVUCoverShootToTargetMissState@@", ".?AVUCoverThrowGrenadeState@@", ".?AVUCoverMarkAndExecReloadState@@" };
    constexpr auto ExitCoverState = ".?AVUExitBackAgainstWallState@@";
    using StateFunction = void(__fastcall*)(uintptr_t state, void* edx);
    std::array<std::array<StateFunction, 2>, CoverStates.size() + 1> stateFunctions;

    uintptr_t StatePawn(uintptr_t state) { return *reinterpret_cast<uintptr_t*>(state + 0x5C); }

    template<size_t Index, bool Begin>
    void __fastcall CoverStateFunction(uintptr_t state, void* edx)
    {
        auto pawn = StatePawn(state);
        auto it = pawns.find(pawn);
        if constexpr (Index == CoverStates.size()) // leaving cover
        {
            if (Begin && it != pawns.end())
                LeaveCover(pawn, it->second);
        }
        else if (Begin)
        {
            auto& pawnState = pawns[pawn];
            pawnState.inCover = true;
            pawnState.coverLeft = false;
        }
        else if (it != pawns.end() && it->second.inCover)
        {
            it->second.coverLeft = true;
            it->second.coverLeftTime = std::chrono::steady_clock::now();
        }
        stateFunctions[Index][Begin](state, edx);
    }

    uintptr_t FindVtable(std::string_view typeName)
    {
        auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + reinterpret_cast<IMAGE_DOS_HEADER*>(base)->e_lfanew);
        auto end = base + nt->OptionalHeader.SizeOfImage;
        auto hex = [](const void* data, size_t size)
        {
            std::string str;
            for (size_t i = 0; i < size; i++)
                str += std::format("{:02X} ", static_cast<const uint8_t*>(data)[i]);
            return str;
        };

        // RTTI: type descriptor (name at +8) <- complete object locator (signature 0, offset 0, cd offset, type descriptor) <- vtable[-1]
        auto name = hook::range_pattern(base, end, hex(typeName.data(), typeName.size() + 1));
        if (name.empty())
            return 0;
        auto typeDescriptor = reinterpret_cast<uintptr_t>(name.get_first()) - 8;
        auto locator = hook::range_pattern(base, end, "00 00 00 00 00 00 00 00 ? ? ? ? " + hex(&typeDescriptor, 4));
        if (locator.empty())
            return 0;
        auto locatorAddress = reinterpret_cast<uintptr_t>(locator.get_first());
        auto vtable = hook::range_pattern(base, end, hex(&locatorAddress, 4));
        return vtable.empty() ? 0 : reinterpret_cast<uintptr_t>(vtable.get_first()) + 4;
    }

    template<size_t Index>
    void HookCoverState(const char* typeName)
    {
        auto vtable = FindVtable(typeName);
        if (!vtable)
            return;
        stateFunctions[Index][1] = *reinterpret_cast<StateFunction*>(vtable + 41 * 4);
        stateFunctions[Index][0] = *reinterpret_cast<StateFunction*>(vtable + 43 * 4);
        injector::WriteMemory(vtable + 41 * 4, &CoverStateFunction<Index, true>, true);
        injector::WriteMemory(vtable + 43 * 4, &CoverStateFunction<Index, false>, true);
    }

    template<size_t... Index>
    void HookCoverStates(std::index_sequence<Index...>)
    {
        (HookCoverState<Index>(CoverStates[Index]), ...);
        HookCoverState<CoverStates.size()>(ExitCoverState);
    }

    // Drops and the slide are on the cover button too: the press also asked for cover, which would be taken after the drop
    // or would end the slide (the original only asked while the button was held). Their BeginState (slot 41) cancels it.
    constexpr std::array DropStates = { ".?AVUGeoDropToLedgeState@@", ".?AVUGeoLowDropState@@", ".?AVUGeoNavExitState@@", ".?AVUSlideOnGroundState@@" };
    std::array<StateFunction, DropStates.size()> dropBeginState;

    void CancelCoverRequest(uintptr_t pawn)
    {
        auto it = pawns.find(pawn);
        if (it == pawns.end() || !it->second.coverRequested || it->second.inCover)
            return;
        Flags(pawn, B2WFlags) &= ~0x100;
        it->second.coverRequested = false;
    }

    template<size_t Index>
    void __fastcall DropStateBegin(uintptr_t state, void* edx)
    {
        CancelCoverRequest(StatePawn(state));
        dropBeginState[Index](state, edx);
    }

    template<size_t... Index>
    void HookDropStates(std::index_sequence<Index...>)
    {
        ([] {
            if (auto vtable = FindVtable(DropStates[Index]))
            {
                dropBeginState[Index] = *reinterpret_cast<StateFunction*>(vtable + 41 * 4);
                injector::WriteMemory(vtable + 41 * 4, &DropStateBegin<Index>, true);
            }
        }(), ...);
    }

    // returns true if the event was handled
    bool HandleEvent(uintptr_t pawn, int32_t name)
    {
        static bool once = [] {
            for (auto [out, str] : { std::pair{ &Sprint, "Sprint" }, { &SprintOff, "SprintOff" }, { &CrouchSprint, "CrouchSprint" }, { &CrouchSprintOff, "CrouchSprintOff" },
                                     { &PrecisionModeOn, "TogglePrecisionModeOn" }, { &PrecisionModeOff, "TogglePrecisionModeOff" }, { &DetectB2W, "DetectB2W" }, { &StopDetectB2W, "StopDetectB2W" } })
                FName(out, nullptr, str, 2);
            return true;
        }();

        auto& state = pawns[pawn];
        if (name == Sprint || name == SprintOff)
            state.sprint = name == Sprint;
        else if (name == CrouchSprint || name == CrouchSprintOff)
            state.crouchSprint = name == CrouchSprint;
        else if (name == PrecisionModeOn || name == PrecisionModeOff)
            SetPrecisionMode(pawn, state, name == PrecisionModeOn);
        else if (name == DetectB2W)
            ToggleCover(pawn, state);
        else if (name != StopDetectB2W) // releasing the button doesn't leave cover
            return false;
        return true;
    }
}

export void InitBlacklistControls()
{
    if (!bBlacklistControlScheme)
        return;

    using namespace BlacklistControls;

    // AEPlayerPawn event handler (sub_56D957), after its event names are made, [esp+10h] is the event FName
    auto pattern = hook::pattern("3B 05 ? ? ? ? 75 07 8B CE E8 ? ? ? ? 5F 5E 5B");
    TogglePrecisionMode = reinterpret_cast<decltype(TogglePrecisionMode)>(injector::GetBranchDestination(pattern.get_first(10)).as_int());
    static auto HandleEventReturn = reinterpret_cast<uintptr_t>(pattern.get_first(15));

    pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 10 8B 00 3B 05 ? ? ? ? 75 04");
    FName = reinterpret_cast<decltype(FName)>(injector::GetBranchDestination(pattern.get_first(0)).as_int());
    static auto HandleEventHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
    {
        if (BlacklistControls::HandleEvent(regs.esi, **reinterpret_cast<int32_t**>(regs.esp + 0x10)))
            regs.eip = HandleEventReturn;
    });

    pattern = hook::pattern("83 7C 24 04 00 75 12 F3 0F 10 05 ? ? ? ? F3 0F 11 81 C4 0B 00 00");
    DisableRun = reinterpret_cast<decltype(DisableRun)>(pattern.get_first());

    // cover detection, a request to toggle cover on is dropped when there was no cover to take
    pattern = hook::pattern("66 F7 86 ? ? ? ? ? ? 0F 84 ? ? ? ? 0F 57 C0");
    static auto CoverDetectionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        using namespace std::chrono_literals;
        auto it = pawns.find(regs.esi);
        if (it == pawns.end())
            return;
        auto& state = it->second;
        auto now = std::chrono::steady_clock::now();
        // a cover state ended and no other one began (states change in between, like cover to shooting from cover)
        if (state.coverLeft && now - state.coverLeftTime > 250ms)
            LeaveCover(regs.esi, state);
        if (!state.coverRequested)
            return;
        if (state.inCover)
            state.coverRequested = false;
        else if (now - state.coverRequestTime > 1s)
        {
            Flags(regs.esi, B2WFlags) &= ~0x100;
            state.coverRequested = false;
        }
    });

    // cover states, to know when the pawn left cover
    HookCoverStates(std::make_index_sequence<CoverStates.size()>());

    // drop and slide states, they don't take cover after
    HookDropStates(std::make_index_sequence<DropStates.size()>());

    // sprint, the state's pawn is at +5Ch, [ebp+0Ch] is the speed
    pattern = hook::pattern("F3 0F 10 45 ? F3 0F 11 45 ? F3 0F 10 86");
    static auto SpeedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto it = pawns.find(*reinterpret_cast<uintptr_t*>(regs.esi + 0x5C));
        if (it == pawns.end())
            return;
        if (it->second.crouchSprint)
            *reinterpret_cast<float*>(regs.ebp + 0x0C) *= 1.5f;
        else if (it->second.sprint)
            *reinterpret_cast<float*>(regs.ebp + 0x0C) *= 1.25f;
    });
}
