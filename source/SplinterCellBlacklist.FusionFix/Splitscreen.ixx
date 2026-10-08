module;

#include <stdafx.h>
#include <Xinput.h>
#include <intrin.h>

export module Splitscreen;

import ComVars;

// Split screen co-op (Xbox 360/PS3/Wii U), most of it is still in the PC game:
// - the co-op launch box handler (SMI) still has button 1, StartLaunchSplitscreenCoop: a local co-op game for 2 local players
// - the co-op lobby asks for START on the second controller (LocID_C_LOB_Splitscreen_Prompt), SMICOOPLobby::StartPressed
//   calls UGameEngine::AddSecondaryLocalPlayer with the last pad that pressed a button, which is never set on PC
// - the mission start adds ?SplitScreen when the profile manager (engine +288h) has a second controller (+18h != -1),
//   UGameEngine::LoadMap sets the split screen flag and the next tick creates the second viewport and spawns the second player
// - UWindowsViewport input reads XInput for the input's controller index, a new input takes the active pad
// The PC game has one gamepad mode (icons, no cursor, pads read) for everything, it's switched on around what is player 2's.
namespace Splitscreen
{
    uint8_t* (*GetEngine)() = nullptr;
    int32_t* pLastPad = nullptr;

    uint8_t* GetProfileManager(uint8_t* engine)
    {
        return engine ? *reinterpret_cast<uint8_t**>(engine + 0x288) : nullptr;
    }

    int32_t GetController(int index)
    {
        auto profiles = GetProfileManager(GetEngine());
        return profiles ? *reinterpret_cast<int32_t*>(profiles + 0x14 + index * 4) : -1;
    }

    uint8_t* GetSplitViewport()
    {
        auto engine = GetEngine();
        return engine ? *reinterpret_cast<uint8_t**>(engine + 0x25C) : nullptr;
    }

    // a split screen view was there since the last map without it (it's gone before that map loads: the mission's exit)
    bool wasSplit = false;

    // the first viewport's size before split screen
    int32_t fullWidth = 0;
    int32_t fullHeight = 0;

    uint8_t* GetFirstViewport()
    {
        auto engine = GetEngine();
        auto client = engine ? *reinterpret_cast<uint8_t**>(engine + 0x44) : nullptr;
        auto viewports = client ? *reinterpret_cast<uint8_t***>(client + 0x2C) : nullptr;
        return viewports && *reinterpret_cast<int32_t*>(client + 0x30) > 0 ? viewports[0] : nullptr;
    }

    // Player 2's controller (split viewport +30h)
    uint8_t* GetPlayer2Controller()
    {
        auto viewport = GetSplitViewport();
        return viewport ? *reinterpret_cast<uint8_t**>(viewport + 0x30) : nullptr;
    }

    // pad of each player, 1-4, 0 for none (keyboard and mouse)
    int32_t padPlayer1 = 0;
    int32_t padPlayer2 = 1;

    // the game's controllers for the players
    constexpr int32_t controllerPlayer1 = 0;
    constexpr int32_t controllerPlayer2 = 1;

    bool IsPlaying()
    {
        return GetEngine && GetController(1) != -1;
    }

    // the split screen lobby (before player 2 joined) or player 2 in
    ULONGLONG lobbyTime = 0;
    bool IsSplitSession()
    {
        return IsPlaying() || GetTickCount64() - lobbyTime < 1000;
    }

    // While the second player is in, the game's controller 0 and 1 read the players' pads, otherwise any pad works as usual
    SafetyHookInline shXInputGetState{};
    SafetyHookInline shXInputSetState{};
    SafetyHookInline shXInputGetCapabilities{};

    DWORD Pad(DWORD userIndex)
    {
        if (!IsPlaying())
            return userIndex;
        auto pad = userIndex == controllerPlayer1 ? padPlayer1 : userIndex == controllerPlayer2 ? padPlayer2 : 0;
        return pad >= 1 && pad <= XUSER_MAX_COUNT ? pad - 1 : XUSER_MAX_COUNT;
    }

    DWORD WINAPI XInputGetStateHook(DWORD userIndex, XINPUT_STATE* state)
    {
        auto pad = Pad(userIndex);
        return pad < XUSER_MAX_COUNT ? shXInputGetState.stdcall<DWORD>(pad, state) : ERROR_DEVICE_NOT_CONNECTED;
    }

    DWORD WINAPI XInputGetCapabilitiesHook(DWORD userIndex, DWORD flags, XINPUT_CAPABILITIES* capabilities)
    {
        auto pad = Pad(userIndex);
        return pad < XUSER_MAX_COUNT ? shXInputGetCapabilities.stdcall<DWORD>(pad, flags, capabilities) : ERROR_DEVICE_NOT_CONNECTED;
    }

    DWORD WINAPI XInputSetStateHook(DWORD userIndex, XINPUT_VIBRATION* vibration)
    {
        auto pad = Pad(userIndex);
        return pad < XUSER_MAX_COUNT ? shXInputSetState.stdcall<DWORD>(pad, vibration) : ERROR_DEVICE_NOT_CONNECTED;
    }

    // buttons of player 2's pad that were just pressed, polled by a caller with its own state
    WORD PollPlayer2(WORD& previous)
    {
        XINPUT_STATE state{};
        WORD buttons = shXInputGetState.stdcall<DWORD>(padPlayer2 - 1, &state) == ERROR_SUCCESS ? state.Gamepad.wButtons : 0;
        WORD pressed = buttons & ~previous;
        previous = buttons;
        return pressed;
    }

    // UWindowsViewport input reads XInput for controllers marked as XInput pads (found at startup)
    int32_t* padIsXInput = nullptr;

    void SetInputController(uint8_t* input, int32_t controller)
    {
        reinterpret_cast<void(__thiscall*)(uint8_t*, int32_t)>((*reinterpret_cast<void***>(input))[0xA4 / 4])(input, controller);
    }

    int32_t GetInputController(uint8_t* input)
    {
        return reinterpret_cast<int32_t(__thiscall*)(uint8_t*)>((*reinterpret_cast<void***>(input))[0xA0 / 4])(input);
    }

    // input of the first viewport
    uint8_t* firstInput = nullptr;

    // Gamepad mode (icons, no cursor), set by any input with a pad, pads are only read in it
    int32_t* pGamepadMode = nullptr;
    void(__stdcall* SetKeyboardMode)(uint8_t* input, int32_t keyboard) = nullptr;

    // Player 1's input without a pad in split screen switches modes only by the plugin: the game also switches it by the profile's
    // gamepad setting (applied when the gear screen opens) and by a pad on the input. A pad button's release after a switch has
    // another key in the keyboard layout, the UI keeps that button as held for the controller and takes no other button.
    bool IsModeByPlugin(uint8_t* input)
    {
        auto split = GetSplitViewport();
        return !padPlayer1 && input && IsSplitSession() && !(split && input == *reinterpret_cast<uint8_t**>(split + 0x54));
    }

    bool modeByPlugin = false;
    SafetyHookInline shKeyboardModeOn{};
    SafetyHookInline shGamepadModeOn{};

    char __fastcall KeyboardModeOn(uint8_t* input, void* edx, void* profile)
    {
        return modeByPlugin || !IsModeByPlugin(input) ? shKeyboardModeOn.thiscall<char>(input, profile) : 0;
    }

    char __fastcall GamepadModeOn(uint8_t* input, void* edx)
    {
        return modeByPlugin || !IsModeByPlugin(input) ? shGamepadModeOn.thiscall<char>(input) : 0;
    }

    void SwitchMode(uint8_t* input, bool keyboard)
    {
        modeByPlugin = true;
        SetKeyboardMode(input, keyboard);
        modeByPlugin = false;
    }

    // the input's keyboard layout (+3638h, set with keyboard mode): pad keys go through the keyboard remap, A is lost and B becomes A
    bool HasKeyboardLayout(uint8_t* input)
    {
        return input && *reinterpret_cast<int32_t*>(input + 0x3638) != 0;
    }

    template<typename F>
    auto WithGamepadMode(bool player2, F&& f)
    {
        if (!player2 || !IsPlaying() || *pGamepadMode)
            return f();
        *pGamepadMode = 1;
        auto result = f();
        *pGamepadMode = 0;
        return result;
    }

    uint8_t** pUIManager = nullptr;

    // a screen owned by player 2: UI manager +78h locked to the controller at +7Ch
    bool IsPlayer2Screen()
    {
        auto ui = pUIManager ? *pUIManager : nullptr;
        return ui && *reinterpret_cast<bool*>(ui + 0x78) && *reinterpret_cast<int32_t*>(ui + 0x7C) == controllerPlayer2;
    }

    // UI manager: menus blocking the players' input (mask at +5A0h)
    bool IsMenuBlockingInput()
    {
        auto ui = pUIManager ? *pUIManager : nullptr;
        return ui && *reinterpret_cast<uint32_t*>(ui + 0x5A0) != 0;
    }

    // ALevelInfo (engine level, virtual +114h, first actor) paused (+5E8h)
    bool IsPaused()
    {
        auto engine = GetEngine();
        if (!engine)
            return false;
        auto level = reinterpret_cast<uint8_t*(__thiscall*)(uint8_t*)>((*reinterpret_cast<void***>(engine))[0x114 / 4])(engine);
        if (!level || *reinterpret_cast<int32_t*>(level + 0x2C) <= 0)
            return false;
        auto levelInfo = **reinterpret_cast<uint8_t***>(level + 0x28);
        return levelInfo && *reinterpret_cast<int32_t*>(levelInfo + 0x5E8) != 0;
    }

    // player 2's pad read by player 1's input, its events (pad keys from 196 on) get player 2's controller
    bool sharedUpdate = false;
    ULONGLONG sharedUpdateTime = 0;

    // the menus show the prompts of the last used device: player 2's pad or the keyboard and mouse
    bool lastInputPad = false;

    // Offline, the pause menus take some buttons from the primary controller only, player 2 is primary on their screens in the
    // mission (in the lobby it breaks their pad on the gear screen)
    SafetyHookInline shIsPrimaryController{};
    bool __stdcall IsPrimaryController(int32_t controller)
    {
        return (controller == controllerPlayer2 && IsPlaying() && GetSplitViewport() && IsPlayer2Screen()) || shIsPrimaryController.stdcall<bool>(controller);
    }

    // Input key bindings (the input's controller) run before the UI and can take the event, player 2's pad in the menus goes
    // to the UI only: read by player 1's input, or their own while a menu is open
    SafetyHookInline shProcessBindings{};
    int32_t __fastcall ProcessBindings(uint8_t* input, void* edx, int32_t key, int32_t action, float delta)
    {
        if (key >= 196 && (sharedUpdate || (IsPlaying() && GetInputController(input) == controllerPlayer2 && (IsPlayer2Screen() || IsPaused() || IsMenuBlockingInput()))))
            return 0;
        return shProcessBindings.thiscall<int32_t>(input, key, action, delta);
    }

    // Software occlusion culling (the renderer's default, Lead option +420h = 2): visibility jobs for the camera and up to 3 shadow lights,
    // each run of a job reads the results of its last run. The split screen views take turns, each view has its own jobs (with the
    // shared ones the first view culled by the second's camera: missing geometry, shadow casters).
    void*** pCameraJob = nullptr;
    void**** pLightJobs = nullptr;
    void* (__cdecl* CreateVisibilityJob)(const uint32_t* params) = nullptr;
    char(__cdecl* DestroyVisibilityJob)(void* job) = nullptr;

    struct VisibilityJobs
    {
        void** camera = nullptr;
        void*** lights = nullptr;
    };
    VisibilityJobs gameJobs{};
    bool jobsSwapped = false;
    int32_t views[2] = { -1, -1 };

    // A view's index is its slot in the renderer's view pool (10 a frame, RenderContext +24h, count +20CCh): player 2's view is prepared
    // first, then its reflections and sun cascades, then player 1's view, its index moved with player 2's sun and reflections. The jobs and
    // slots kept per index were another view's (player 1's view missed geometry: Voron Station, Kigali). The views are kept by their order
    // of preparation in the frame instead.
    int32_t prepareOrder = 0;
    int32_t viewOrders[16] = {};

    void ResetViewOrders()
    {
        prepareOrder = 0;
        std::fill(std::begin(viewOrders), std::end(viewOrders), -1);
    }

    int32_t PreparedViewOrder(int32_t index)
    {
        auto order = prepareOrder++;
        if (index >= 0 && index < int32_t(std::size(viewOrders)))
            viewOrders[index] = order;
        return order;
    }

    int32_t RenderedViewOrder(int32_t index)
    {
        return index >= 0 && index < int32_t(std::size(viewOrders)) && viewOrders[index] >= 0 ? viewOrders[index] : index;
    }

    // the jobs of every view but the first one
    struct ViewJobs
    {
        void* camera = nullptr;
        void* lights[3] = {};
        VisibilityJobs Get() { return { reinterpret_cast<void**>(camera), reinterpret_cast<void***>(lights) }; }
    };
    std::map<int32_t, ViewJobs> otherViewJobs;

    void UseJobs(const VisibilityJobs& jobs, bool swapped)
    {
        if (!jobsSwapped)
            gameJobs = { *pCameraJob, *pLightJobs };
        *pCameraJob = jobs.camera;
        *pLightJobs = jobs.lights;
        jobsSwapped = swapped;
    }

    void DestroySecondViewJobs()
    {
        if (jobsSwapped)
            UseJobs(gameJobs, false);
        for (auto& [view, jobs] : otherViewJobs)
        {
            if (jobs.camera)
                DestroyVisibilityJob(jobs.camera);
            for (auto job : jobs.lights)
            {
                if (job)
                    DestroyVisibilityJob(job);
            }
        }
        otherViewJobs.clear();
        views[0] = views[1] = -1;
    }

    // A frame prepares each view (its jobs start), then renders each one (its jobs finish): the view's index (view parameters +26Ch)
    void SelectVisibilityJobs(int32_t view)
    {
        if (!GetSplitViewport() || !*pCameraJob || view < 0)
        {
            if (jobsSwapped)
                UseJobs(gameJobs, false);
            views[0] = views[1] = -1;
            return;
        }

        if (views[0] == -1)
            views[0] = view;
        else if (views[1] == -1 && view != views[0])
            views[1] = view;

        auto jobs = view != views[0] ? otherViewJobs.find(view) : otherViewJobs.end();
        if (view != views[0] && jobs == otherViewJobs.end() && otherViewJobs.size() < 8)
        {
            // the game's parameters (SoftwareRasterizer init): size, then the same for the camera and the lights
            const uint32_t camera[9] = { 512, 256, 0x8000, 0, 0x40000, 40, std::bit_cast<uint32_t>(0.01f), std::bit_cast<uint32_t>(1.5f), 0x1000101 };
            const uint32_t light[9] = { 256, 256, 0x8000, 0, 0x40000, 40, std::bit_cast<uint32_t>(0.01f), std::bit_cast<uint32_t>(1.5f), 0x1000101 };
            jobs = otherViewJobs.try_emplace(view).first;
            jobs->second.camera = CreateVisibilityJob(camera);
            for (auto& job : jobs->second.lights)
                job = CreateVisibilityJob(light);
        }
        if (jobs == otherViewJobs.end())
        {
            if (jobsSwapped)
                UseJobs(gameJobs, false);
            return;
        }
        UseJobs(jobs->second.Get(), true);
    }

    // Sun shadow casters: preparing a view queues each node of the scene grid (HGrid +10Ch nodes, +110h count) in the cascade jobs and
    // keeps its slot in them in the node (GeomNode +18Ch, a dword for each of the 3 cascades), rendering the view reads the job results by
    // these. Both views are prepared before they're rendered (player 2's split viewport first, then player 1's), the first view read the
    // second's slots (shadows appearing and disappearing). Each view's slots are kept and put back before it's rendered.
    struct CascadeSlots
    {
        uint8_t* node;
        uint32_t slot[3];
    };
    std::vector<CascadeSlots> viewSlots[2];
    bool viewSlotsValid[2] = {};

    bool SaveCascadeSlots(uint8_t* grid, int index)
    {
        auto& out = viewSlots[index];
        out.clear();
        viewSlotsValid[index] = false;
        if (!grid)
            return false;
        auto nodes = *reinterpret_cast<uint8_t***>(grid + 0x10C);
        auto count = *reinterpret_cast<int32_t*>(grid + 0x110);
        if (!nodes || count <= 0 || count > 0x100000)
            return false;
        for (int32_t i = 0; i < count; i++)
        {
            if (!nodes[i])
                continue;
            CascadeSlots slots{ nodes[i] };
            memcpy(slots.slot, nodes[i] + 0x18C, sizeof(slots.slot));
            out.push_back(slots);
        }
        viewSlotsValid[index] = true;
        return true;
    }

    void RestoreCascadeSlots(int index)
    {
        if (!viewSlotsValid[index])
            return;
        for (auto& slots : viewSlots[index])
            memcpy(slots.node + 0x18C, slots.slot, sizeof(slots.slot));
    }

    void ResetCascadeSlots()
    {
        for (int i = 0; i < 2; i++)
        {
            viewSlots[i].clear();
            viewSlotsValid[i] = false;
        }
    }

    // the second view is about to be prepared: the first view's slots
    void PreparingView(int32_t view, uint8_t* grid)
    {
        if (!GetSplitViewport() || views[0] == -1 || views[1] == -1)
            return ResetCascadeSlots();
        if (view == views[1])
            SaveCascadeSlots(grid, 0);
    }

    // the first view is about to be rendered: the second view's slots are kept, its own put back; the second view: its own put back
    void RenderingView(int32_t view, uint8_t* grid)
    {
        if (!GetSplitViewport() || views[0] == -1 || views[1] == -1)
            return ResetCascadeSlots();
        if (view == views[0] && viewSlotsValid[0])
        {
            SaveCascadeSlots(grid, 1);
            RestoreCascadeSlots(0);
            viewSlotsValid[0] = false;
        }
        else if (view == views[1])
        {
            RestoreCascadeSlots(1);
            viewSlotsValid[1] = false;
        }
    }

    // Spot light shadows (ShadowPass::NewComposite, after the light's depth pass): the light's volume marks the stencil where the scene is
    // inside it (depth test fails behind the scene), the shadow is applied where it's marked. In split screen the marking failed in
    // player 1's view (the second one rendered) depending on the camera angle: lamps lit Sam and the walls without their shadows. The
    // stencil test is off there, the shadow is applied over the volume's screen area.
    // The sun's cascades go through the same composite: player 1 lost the farther ones (a cut-off line, the floor lit past it).
    bool inShadowComposite = false;

    // the composite enabling the stencil test: the value pushed for SetRenderState(D3DRS_STENCILENABLE)
    void StencilEnable(uint32_t* value)
    {
        if (inShadowComposite && GetSplitViewport())
            *value = 0;
    }

    // Lens flare and sun godray occlusion (hardware queries): a flare node (+40h) and the sun light node (+2CCh) have 16 query slots, the
    // game uses 2 per GPU (frame parity, slot k = (frame / GPUs) % 2 + 2 * (frame % GPUs)), issues them while rendering a view and reads the
    // previous frame's. The split screen views shared them: each view restarted the other's query and read its result (flares of lights
    // hidden by walls). The second view (with its own visibility jobs) takes the slots after 8.
    void(__fastcall* CreateQuery)(void* device, void* edx, int32_t type, void** out) = nullptr;
    int32_t queryType = 1;

    void CreateSecondViewQueries(uint8_t* pass, void** first)
    {
        auto device = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(pass) + 0xC);
        auto gpus = device ? *reinterpret_cast<int32_t*>(device + 0x10) : 0;
        if (gpus <= 0 || gpus > 4)
            return;
        for (int32_t i = 0; i < 2 * gpus; i++)
            CreateQuery(device, nullptr, queryType, first + i);
    }

    // eax: the game's slot for the frame parity, moved to the second view's when it exists
    void SecondViewQuerySlot(SafetyHookContext& regs, void** queries, uint32_t gpu)
    {
        if (!jobsSwapped || gpu >= 4)
            return;
        auto slot = regs.eax + 2 * gpu + 8;
        if (slot < 16 && queries[slot])
            regs.eax += 8;
    }

    // Ambient occlusion (Lead options +3CCh, DX9 +3C8h, ResourceDB +8): a split screen view is smaller than the buffers, the game
    // takes its own SSAO instead of HBAO+ and it darkens a band along player 1's view that moves with the camera. It's off while
    // in split screen (applying the video options in the mission sets it again).
    uint8_t** pResourceDB = nullptr;
    uintptr_t aoOffset = 0x3CC;
    float savedAO = 0.0f;
    bool aoOff = false;

    // Applying the video options (gamma calibration, any video setting) resizes player 1's viewport to the whole window (the render
    // device's SetRes), only entering split screen and the end of a cinematic halve it (sub_55C7B0(engine, 0)): player 1's view covered
    // player 2's half (player 2's camera seemed stuck) until the lobby. It's laid out again when player 1's viewport is wider than player 2's
    // origin (+4A0h width, +4A8h x) outside a cinematic (byte_310A450 player 1 full width, engine +27Ch cinematic count).
    void(__fastcall* SplitLayout)(uint8_t* engine, void* edx, int32_t fullWidth) = nullptr;
    uint8_t* pPlayer1FullWidth = nullptr;

    void KeepSplitLayout(uint8_t* viewport)
    {
        auto engine = GetEngine();
        auto split = GetSplitViewport();
        if (!SplitLayout || !pPlayer1FullWidth || !engine || !split || viewport != GetFirstViewport() || *pPlayer1FullWidth ||
            *reinterpret_cast<int32_t*>(engine + 0x27C) != 0)
            return;
        auto origin = *reinterpret_cast<int32_t*>(split + 0x4A8);
        if (origin > 0 && *reinterpret_cast<int32_t*>(viewport + 0x4A0) > origin)
        {
            *pPlayer1FullWidth = 1; // the layout returns when it's already the asked state
            SplitLayout(engine, nullptr, 0);
        }
    }

    // Planar reflections (Lead options +26Dh) take 4 slots of the view pool (10 a frame) for each view, with the sun's cascades two views
    // don't fit: off in split screen.
    uint8_t savedReflections = 0;
    bool reflectionsOff = false;

    void UpdateReflections()
    {
        if (!pResourceDB || !*pResourceDB)
            return;
        auto& reflections = *(*reinterpret_cast<uint8_t**>(*pResourceDB + 8) + 0x26D);
        if (bSplitscreen)
        {
            if (reflections != 0)
            {
                savedReflections = reflections;
                reflections = 0;
                reflectionsOff = true;
            }
        }
        else if (reflectionsOff)
        {
            reflections = savedReflections;
            reflectionsOff = false;
        }
    }

    void UpdateAmbientOcclusion()
    {
        if (!pResourceDB || !*pResourceDB)
            return;
        auto& ao = *reinterpret_cast<float*>(*reinterpret_cast<uint8_t**>(*pResourceDB + 8) + aoOffset);
        if (bSplitscreen)
        {
            if (ao != 0.0f)
            {
                savedAO = ao;
                ao = 0.0f;
                aoOff = true;
            }
        }
        else if (aoOff)
        {
            ao = savedAO;
            aoOff = false;
        }
    }

    // Player 2 is a guest with the guest loadout (CLoadoutManager, flag +2Ch), +2Dh makes it customizable in the lobby's gear screen
    // and saved in the profile (its own section, by name)
    uint8_t** pLoadoutGlobal = nullptr;
    uint8_t* (__fastcall* GetGuestLoadout)(void* loadouts, void* edx) = nullptr;

    void UnlockGuestLoadout()
    {
        auto global = pLoadoutGlobal ? *pLoadoutGlobal : nullptr;
        auto loadouts = global ? *reinterpret_cast<uint8_t**>(global + 0x2C) : nullptr;
        auto guest = loadouts && GetGuestLoadout ? GetGuestLoadout(loadouts, nullptr) : nullptr;
        if (guest)
            guest[0x2D] = 1;
    }

    // UWindowsViewport input update, player 2's pad is read in gamepad mode while player 1 stays on keyboard and mouse
    SafetyHookInline shUpdateInput{};
    void __fastcall UpdateInput(uint8_t* viewport, void* edx, uint8_t* input, uint32_t delta1, uint32_t delta2, int32_t a4)
    {
        bSplitscreen = GetSplitViewport() != nullptr;
        if (bSplitscreen)
            wasSplit = true;
        else if (!wasSplit && viewport && viewport == GetFirstViewport() && !IsPlaying())
        {
            fullWidth = *reinterpret_cast<int32_t*>(viewport + 0x4A0);
            fullHeight = *reinterpret_cast<int32_t*>(viewport + 0x4A4);
        }
        UpdateAmbientOcclusion();
        UpdateReflections();
        KeepSplitLayout(viewport);

        // from the start, before the profile loads: the profile saves and loads customizable loadouts only
        UnlockGuestLoadout();

        // the controller player 1's input had before player 2 joined, it gets it back after
        static int32_t singleController = INT32_MIN;
        if (!padPlayer1 && viewport != GetSplitViewport() && !IsSplitSession() && singleController != INT32_MIN)
        {
            SetInputController(input, singleController);
            std::memset(input + 0x3434 + 196, 0, 224 - 196);
            singleController = INT32_MIN;
        }

        if (!padPlayer1 && viewport != GetSplitViewport() && IsSplitSession())
        {
            // Player 1 without a pad stays in keyboard and mouse mode from the split screen lobby on (the profile's gamepad setting
            // switches to gamepad mode, a pad would also press player 2's START). In the menus after player 2 joined, this input also
            // reads player 2's pad (events with their controller), the lobby gives the screen to whoever pressed a button, so player 2
            // customizes their own loadout. In the mission it does on player 2's screens: the menus take the first viewport's input
            // only, player 2's loadout at a supply crate (locked to their controller) got no input from anyone.
            if (singleController == INT32_MIN)
                singleController = GetInputController(input);
            auto shared = IsPlaying() && (!GetSplitViewport() || IsPlayer2Screen());
            auto controller = shared ? controllerPlayer2 : -1;
            if (GetInputController(input) != controller)
            {
                SetInputController(input, controller);

                // pad buttons held when the pad changed never get their release, the input ignores presses of held keys (+3434h)
                std::memset(input + 0x3434 + 196, 0, 224 - 196);
            }

            // moving the mouse also switches back to keyboard and mouse
            static POINT lastCursor{};
            POINT cursor{};
            if (GetCursorPos(&cursor) && (cursor.x != lastCursor.x || cursor.y != lastCursor.y))
            {
                lastCursor = cursor;
                lastInputPad = false;
            }

            // the menus switch to gamepad mode the game's way when player 2's pad was used last, the UI updates its prompts
            auto gamepad = IsPlaying() && lastInputPad && (shared || IsPlayer2Screen());
            // the gamepad mode can be on already (set by a pad event) with the input still in the keyboard layout
            if ((*pGamepadMode != 0) != gamepad || (gamepad && HasKeyboardLayout(input)))
            {
                SwitchMode(input, !gamepad);

                // the layout remaps the pad keys, a press can miss its release and the input ignores presses of held keys (+3434h)
                std::memset(input + 0x3434 + 196, 0, 224 - 196);
            }

            // in the lobby menus (not while a mission loads) the cursor comes back the game's way: a pad key hides it until the mouse
            // moves (+5D0h, player 2's pad or player 1's in single player before), viewport virtual +9Ch shows it outside gamepad mode
            if (!gamepad && !GetSplitViewport() && (GetTickCount64() - lobbyTime < 1000 || IsPlayer2Screen()))
            {
                *reinterpret_cast<int32_t*>(viewport + 0x5D0) = 0;
                reinterpret_cast<void(__thiscall*)(uint8_t*)>((*reinterpret_cast<void***>(viewport))[0x9C / 4])(viewport);
            }
            if (shared)
            {
                auto mode = *pGamepadMode;
                *pGamepadMode = 1;
                sharedUpdate = true;
                sharedUpdateTime = GetTickCount64();
                shUpdateInput.thiscall<void>(viewport, input, delta1, delta2, a4);
                sharedUpdate = false;
                *pGamepadMode = mode;
                return;
            }
        }
        // player 2's own input waits while player 1's reads their pad for their screen (the screen got every button twice)
        if (!padPlayer1 && viewport == GetSplitViewport() && IsPlayer2Screen() && GetTickCount64() - sharedUpdateTime < 250)
            return;
        WithGamepadMode(IsPlaying() && GetInputController(input) == controllerPlayer2, [&] { shUpdateInput.thiscall<void>(viewport, input, delta1, delta2, a4); return 0; });
    }

    // Viewport input events (input, key, action, delta, controller): the game passes controller 0 for pads
    void ViewportEvent(uint8_t* viewport, uint32_t* args)
    {
        // player 2's pad: read by player 1's input in the menus, or their own viewport's in the mission
        auto player2Pad = sharedUpdate || (IsPlaying() && viewport == GetSplitViewport());
        if (player2Pad && args[1] >= 196 && args[1] < 224 && args[2] == 1)
        {
            lastInputPad = true;

            // a press of a key the input still has as held (+3434h) is dropped, a release can be missed when the pad changes
            reinterpret_cast<uint8_t*>(args[0])[0x3434 + args[1]] = 0;
        }
        else if (!player2Pad && (args[2] == 1 || ((args[1] == 228 || args[1] == 229) && *reinterpret_cast<float*>(&args[3]) != 0.0f)))
            lastInputPad = false;

        // player 2's events, keyboard and mouse also work on a screen owned by player 2 (either player can close it)
        if ((sharedUpdate && args[1] >= 196) || (IsPlaying() && (viewport == GetSplitViewport() || IsPlayer2Screen())))
            args[4] = controllerPlayer2;
    }

    // UWindowsViewport input event (input, key, action, delta, controller, ?): player 2's pad keys skip player 1's keyboard layout
    // (+3638h remaps the keys), a press and its release then always have the same key: the UI keeps a pressed button per controller
    // until its release and takes no other button meanwhile (START pressed in the layout to join kept A and B from working)
    SafetyHookInline shViewportInput{};
    int32_t __fastcall ViewportInput(uint8_t* viewport, void* edx, uint8_t* input, int32_t key, int32_t action, float delta, int32_t controller, int32_t a7)
    {
        uint32_t args[5] = { uint32_t(uintptr_t(input)), uint32_t(key), uint32_t(action), std::bit_cast<uint32_t>(delta), uint32_t(controller) };
        ViewportEvent(viewport, args);

        auto player2Pad = (sharedUpdate || (IsPlaying() && viewport == GetSplitViewport())) && key >= 196 && key < 224;
        auto& layout = *reinterpret_cast<int32_t*>(input + 0x3638);
        auto saved = layout;
        if (player2Pad)
            layout = 0;
        auto result = shViewportInput.thiscall<int32_t>(viewport, input, key, action, delta, int32_t(args[4]), a7);
        if (player2Pad)
            layout = saved;
        return result;
    }

    // SMICOOPLobby::StartPressed, ButtonX
    void(__fastcall* StartPressed)(void* lobby, void* edx) = nullptr;
    void(__fastcall* ButtonX)(void* lobby, void* edx) = nullptr;

    // co-op lobby tick, state 2 is the local lobby
    void LobbyTick(uint8_t* lobby)
    {
        if (*reinterpret_cast<int32_t*>(lobby + 0x2AC) != 2)
            return;
        lobbyTime = GetTickCount64();

        static WORD previous = 0;
        auto pressed = PollPlayer2(previous);
        if (IsPlaying())
        {
            // X customizes player 2's gear (in gamepad mode the button bar takes X before the lobby, then does nothing for player 2)
            // it opens once the input update switched to gamepad mode and out of the keyboard layout
            static bool pendingX = false;
            auto ui = pUIManager ? *pUIManager : nullptr;
            if (pressed & XINPUT_GAMEPAD_X)
            {
                lastInputPad = true;
                pendingX = true;
            }
            pendingX = pendingX && lastInputPad;
            if (pendingX && *pGamepadMode && !HasKeyboardLayout(firstInput))
            {
                pendingX = false;
                if (ui && !ui[0x78])
                {
                    *pLastPad = controllerPlayer2;
                    ButtonX(lobby, nullptr);
                }
            }
            return;
        }

        // player 2 joins with START on their pad
        if (!(pressed & XINPUT_GAMEPAD_START))
            return;

        *reinterpret_cast<int32_t*>(GetProfileManager(GetEngine()) + 0x14) = controllerPlayer1;
        *pLastPad = controllerPlayer2;
        StartPressed(lobby, nullptr);
        if (IsPlaying())
        {
            padIsXInput[controllerPlayer1] = padIsXInput[controllerPlayer2] = 1;
            if (firstInput && padPlayer1)
                SetInputController(firstInput, controllerPlayer1);
        }
    }

    // UWindowsViewport: pad for an input that has none yet
    SafetyHookInline shDefaultController{};
    int32_t __fastcall DefaultController(uint8_t* viewport, void* edx, uint8_t* input)
    {
        auto split = viewport == GetSplitViewport();
        if (!split)
            firstInput = input;

        // player 1 without a pad never gets one in split screen, an input with a pad switches the game to gamepad mode
        if (!split && !padPlayer1 && IsSplitSession())
            return -1;

        if (IsPlaying())
        {
            auto controller = split ? controllerPlayer2 : controllerPlayer1;
            padIsXInput[controller] = 1;
            SetInputController(input, controller);
            return controller;
        }
        return shDefaultController.thiscall<int32_t>(viewport, input);
    }

    // RTTI: is the object of the class (or derived), name like ".?AVHudScene@UI@@"
    bool IsA(void* object, const char* name)
    {
        static std::unordered_map<void*, std::unordered_map<std::string, bool>> cache;
        auto vtable = *reinterpret_cast<void***>(object);
        auto& entry = cache[vtable];
        if (auto it = entry.find(name); it != entry.end())
            return it->second;

        bool result = false;
        auto locator = reinterpret_cast<uint8_t*>(vtable[-1]);
        auto hierarchy = *reinterpret_cast<uint8_t**>(locator + 0x10);
        auto count = *reinterpret_cast<uint32_t*>(hierarchy + 8);
        auto bases = *reinterpret_cast<uint8_t***>(hierarchy + 0xC);
        for (uint32_t i = 0; i < count && !result; i++)
        {
            auto type = *reinterpret_cast<uint8_t**>(bases[i]);
            result = strcmp(reinterpret_cast<const char*>(type + 8), name) == 0;
        }
        entry[name] = result;
        return result;
    }

    // HUD scenes in player 2's viewport, the world tips (SceneGeoTip, owner +1C8h, +34h, pawn at +8Ch) of their pawn
    uint8_t* (__fastcall* GetHudViewport)(void* scene, void* edx) = nullptr;
    bool IsPlayer2Scene(void* scene)
    {
        if (!IsPlaying() || !GetSplitViewport())
            return false;
        if (IsA(scene, ".?AVHudScene@UI@@"))
            return GetHudViewport(scene, nullptr) == GetSplitViewport();
        if (IsA(scene, ".?AVSceneGeoTip@UI@@"))
        {
            auto owner = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(scene) + 0x1C8);
            auto source = owner ? *reinterpret_cast<uint8_t**>(owner + 0x34) : nullptr;
            auto controller = GetPlayer2Controller();
            return source && controller && *reinterpret_cast<void**>(source + 0x8C) == *reinterpret_cast<void**>(controller + 0x310);
        }
        return false;
    }

    // Player 2's pause menu in the mission: START doesn't reach it (taken before), the plugin calls its handler (scene virtual +60h)
    void PauseMenuPlayer2(void* scene)
    {
        static WORD previous = 0;
        static void* lastScene = nullptr;
        static ULONGLONG lastTime = 0;

        // a new pause menu (or one that didn't tick for a while) ignores the buttons held when it opened
        auto now = GetTickCount64();
        if (scene != lastScene || now - lastTime > 500)
            PollPlayer2(previous);
        lastScene = scene;
        lastTime = now;

        // player 1's input reads player 2's pad on their screens while it updates, START then reaches the menu the usual way
        auto pressed = PollPlayer2(previous);
        if (GetTickCount64() - sharedUpdateTime < 250)
            return;
        if ((pressed & XINPUT_GAMEPAD_START) && IsPlayer2Screen())
            reinterpret_cast<void(__thiscall*)(void*)>((*reinterpret_cast<void***>(scene))[0x60 / 4])(scene);
    }

    // Scene tick slot handlers (identical copies): player 2's HUD and world tips show pad prompts
    std::vector<SafetyHookInline> shSceneTick;
    template<size_t N>
    bool __fastcall SceneTick(void* scene, void* edx, void* slot, float delta)
    {
        // the input update stops while paused, the pause menu still ticks
        if (IsPlaying() && GetSplitViewport() && IsA(scene, ".?AVScenePause@UI@@"))
            PauseMenuPlayer2(scene);

        if (IsPlayer2Scene(scene))
        {
            auto mode = *pGamepadMode;
            *pGamepadMode = 1;
            auto result = shSceneTick[N].thiscall<bool>(scene, slot, delta);
            *pGamepadMode = mode;
            return result;
        }
        return shSceneTick[N].thiscall<bool>(scene, slot, delta);
    }

    // Player 2's controller, pawn (controller +310h) and camera tick in gamepad mode: the controller tick clears the look axes in
    // keyboard mode, the pawn's tips (prompts) take the button of the input mode, the camera picks the invert setting by it
    SafetyHookInline shControllerTick{};
    int32_t __fastcall ControllerTick(uint8_t* controller, void* edx, float delta, int32_t tickType)
    {
        return WithGamepadMode(controller == GetPlayer2Controller(), [&] { return shControllerTick.thiscall<int32_t>(controller, delta, tickType); });
    }

    SafetyHookInline shPawnTick{};
    int32_t __fastcall PawnTick(uint8_t* pawn, void* edx, float delta, int32_t tickType)
    {
        auto controller = GetPlayer2Controller();
        return WithGamepadMode(controller && pawn == *reinterpret_cast<uint8_t**>(controller + 0x310), [&] { return shPawnTick.thiscall<int32_t>(pawn, delta, tickType); });
    }

    // Movement stick (controller +368h, +36Ch) to speed: keyboard mode takes the larger axis, a diagonal is a walk
    SafetyHookInline shMoveStick{};
    float* __fastcall MoveStick(uint8_t* controller, void* edx, float* x, float* y)
    {
        return WithGamepadMode(controller == GetPlayer2Controller(), [&] { return shMoveStick.thiscall<float*>(controller, x, y); });
    }

    SafetyHookInline shCameraUpdate{};
    void __fastcall CameraUpdate(uint8_t* camera, void* edx)
    {
        WithGamepadMode(*reinterpret_cast<uint8_t**>(camera + 0x1C) == GetPlayer2Controller(), [&] { shCameraUpdate.thiscall<void>(camera); return 0; });
    }

    // Leaving split screen requests the resolution back as twice the first viewport's width (consoles halve it), the PC game never
    // changes the resolution for split screen and it can be off (half the height, an odd width lost). It's the size before split screen.
    std::vector<void*> resolutionRestores;
    SafetyHookInline shRequestResolution{};
    void __fastcall RequestResolution(void* viewport, void* edx, int32_t width, int32_t height, int32_t mode, int32_t refresh)
    {
        if (std::find(resolutionRestores.begin(), resolutionRestores.end(), _ReturnAddress()) != resolutionRestores.end())
        {
            if (fullWidth <= 0 || fullHeight <= 0)
                return;
            width = fullWidth;
            height = fullHeight;
        }
        shRequestResolution.thiscall<void>(viewport, width, height, mode, refresh);
    }

    // Leaving split screen (UGameEngine::EndSplitScreen, a map loaded without ?SplitScreen: the hub after the mission) keeps player 2
    // (the console game removes them when the next mission is picked), the plugin's split screen state stayed on. Player 2 is removed
    // like the game's leave of a local match does (UGameEngine::RemoveSecondaryLocalPlayer).
    void(__fastcall* RemoveSecondaryLocalPlayer)(uint8_t* engine, void* edx, int32_t controller) = nullptr;
    SafetyHookInline shEndSplitScreen{};
    void __fastcall EndSplitScreen(uint8_t* engine, void* edx)
    {
        auto split = *reinterpret_cast<uint8_t**>(engine + 0x25C) != nullptr || wasSplit;
        wasSplit = false;
        auto controller = GetController(1);
        shEndSplitScreen.thiscall<void>(engine);
        if (split && controller != -1 && RemoveSecondaryLocalPlayer)
            RemoveSecondaryLocalPlayer(engine, nullptr, controller);
    }

    // UWindowsViewport::Repaint draws a viewport when its player has a pawn (controller +310h) or a few other states, a player 1 who bled
    // out has none: their viewport isn't drawn and the split screen view (drawn with it) freezes too. It's drawn anyway in split screen.
    SafetyHookInline shRepaint{};
    void __fastcall Repaint(uint8_t* viewport, void* edx, int32_t a2, int32_t a3)
    {
        auto split = GetSplitViewport();
        auto controller = *reinterpret_cast<uint8_t**>(viewport + 0x30);
        if (split && viewport != split && controller && !*reinterpret_cast<void**>(controller + 0x310))
        {
            auto engine = *reinterpret_cast<uint8_t**>(*reinterpret_cast<uint8_t**>(viewport + 0x18) + 0x28);
            reinterpret_cast<void(__thiscall*)(uint8_t*, uint8_t*, int32_t, int32_t, int32_t, int32_t)>((*reinterpret_cast<void***>(engine))[0x80 / 4])(engine, viewport, a2, 0, 0, 0);
            return;
        }
        shRepaint.thiscall<void>(viewport, a2, a3);
    }

    // EnterSplitscreen sets GIsRequestingExit when the second player can't be spawned (a map without a second player start)
    uint32_t* pRequestingExit = nullptr;
    SafetyHookInline shEnter{};
    void __fastcall Enter(void* engine, void* edx)
    {
        auto requested = *pRequestingExit;
        shEnter.thiscall<void>(engine);
        if (!requested)
            *pRequestingExit = 0;
    }

    // co-op launch box, adds the split screen button: options {id, FString label} at ebp - boxOptions, the count 40h after, up to 4
    wchar_t* (__cdecl* Localize)(const wchar_t* section, const wchar_t* key, const char* package, int, int, int) = nullptr;
    void(__fastcall* FStringFromText)(void* string, void* edx, const wchar_t* text) = nullptr;
    const wchar_t* section = nullptr;
    const char* package = nullptr;
    uintptr_t boxOptions = 0;

    void LaunchBox(uintptr_t ebp)
    {
        auto count = reinterpret_cast<int32_t*>(ebp - boxOptions + 0x40);
        if (*count < 2 || *count >= 4)
            return;

        auto option = reinterpret_cast<uint8_t*>(ebp - boxOptions) + *count * 0x10;
        auto text = Localize(section, L"LocID_CONF_Option_COOP_Splitscreen", package, 0, 0, 0);
        FStringFromText(option + 4, nullptr, text && *text ? text : L"SPLIT-SCREEN");
        *reinterpret_cast<int32_t*>(option) = 1;
        ++*count;
    }
}

export void InitSplitscreen()
{
    CIniReader iniReader("");
    if (iniReader.ReadInteger("SPLITSCREEN", "Enable", 1) == 0)
        return;

    Splitscreen::padPlayer1 = std::clamp(iniReader.ReadInteger("SPLITSCREEN", "GamepadPlayer1", 0), 0, XUSER_MAX_COUNT);
    Splitscreen::padPlayer2 = std::clamp(iniReader.ReadInteger("SPLITSCREEN", "GamepadPlayer2", 1), 1, XUSER_MAX_COUNT);

    auto xinput = GetModuleHandleW(L"xinput1_3.dll");
    if (!xinput)
        xinput = LoadLibraryW(L"xinput1_3.dll");
    auto getState = xinput ? GetProcAddress(xinput, "XInputGetState") : nullptr;
    auto setState = xinput ? GetProcAddress(xinput, "XInputSetState") : nullptr;
    auto getCapabilities = xinput ? GetProcAddress(xinput, "XInputGetCapabilities") : nullptr;
    if (!getState || !setState || !getCapabilities)
        return;

    auto branch = [](void* call) { return reinterpret_cast<uintptr_t>(call) + 5 + *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(call) + 1); };

    // DX11 / DX9 (Blacklist_game.exe) where they differ
    auto start = hook::pattern("53 57 8B 3D ? ? ? ? 8B D9 E8 ? ? ? ? 84 C0 75 ? 56 E8 ? ? ? ? 8B F0 8B 06 8B 90 F0 01 00 00");
    auto start9 = hook::pattern("53 8B D9 8B 0D ? ? ? ? 57 8B 3D ? ? ? ? 85 C9 74 09 E8");
    auto buttonX = hook::pattern("53 56 8B F1 8B 0D ? ? ? ? 57 8B 3D ? ? ? ? B3 01 E8");
    auto buttonX9 = hook::pattern("A1 ? ? ? ? 53 56 8B 35 ? ? ? ? 57 8B F9 8B 48 54 B3 01");
    auto box = hook::pattern("8D 8D 6C FF FF FF 8D 86 BC 05 00 00 51 89 45 DC E8");
    auto box9 = hook::pattern("8D 95 68 FF FF FF 8D 8E BC 05 00 00 52");
    auto uiManager = hook::pattern("A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC A1 ? ? ? ? 85 C0 74 0C 83 78 04 00 74 06");
    auto uiManager9 = hook::pattern("8B 0D ? ? ? ? 80 79 78 00 75 ? 84 DB");
    if ((start.empty() && start9.empty()) || (buttonX.empty() && buttonX9.empty()) || (box.empty() && box9.empty()) || (uiManager.empty() && uiManager9.empty()))
        return;

    auto engine = hook::pattern("83 3D ? ? ? ? 00 75 09 A1 ? ? ? ? 85 C0 75 05 A1 ? ? ? ? C3");
    auto solo = hook::pattern("53 53 53 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 88 5D ? C7 45 ? 01 00 00 00 C7 45 ? 06 00 00 00 E8 ? ? ? ? 83 C4 18 50 8D 8D ? ? ? ? E8");
    auto lobbyTick = hook::pattern("8B 86 AC 02 00 00 83 E8 02 74 ? 83 E8 03 74 05 83 E8 04");
    auto viewportController = hook::pattern("55 8B EC A1 ? ? ? ? 56 8B F0 83 F8 FF 74 ? 83 3C 85 ? ? ? ? 00 74 ? 8B 4D 08 8B 11 50 8B 82 A4 00 00 00 FF D0");
    auto exitRequest = hook::pattern("8B 90 84 02 00 00 51 8B CE FF D2 85 C0 75 ? 5F C7 05");
    auto enter = hook::pattern("55 8B EC 83 EC 14 56 57 8B F1 E8 ? ? ? ? 8B 06 8B 90 68 02 00 00 6A 00 8B CE FF D2");
    auto xinputPad = hook::pattern("83 F8 FF 0F 84 ? ? ? ? 83 3C 85 ? ? ? ? 00 0F 84 ? ? ? ? 83 3D ? ? ? ? 00");
    auto keyboardMode = hook::pattern("55 8B EC E8 ? ? ? ? 8B 10 8B C8 8B 82 F0 01 00 00 FF D0 85 C0 74 09 8B C8 E8 ? ? ? ? EB 02 33 C0 8B 4D 08 85 C9 74 37 85 C0 74 33");
    auto resourceDB = hook::pattern("A1 ? ? ? ? 8B 48 10 85 C9 74 0A E8 ? ? ? ? A1 ? ? ? ? 56 8B F0 85 C0 74 10 8B C8 E8");
    auto createJob = hook::pattern("C7 45 D8 00 02 00 00 C7 45 DC 00 01 00 00 C7 45 F8 01 01 00 01 E8");
    auto destroyJobs = hook::pattern("A1 ? ? ? ? 56 50 E8 ? ? ? ? 83 C4 04 33 F6 8B 0D ? ? ? ? 8B 14 0E 52 E8");
    auto prepareView = hook::pattern("8B 07 89 82 6C 02 00 00 8B 0E 8B 41 14 8A 90 E9 01 00 00");
    auto renderView = hook::pattern("55 8B EC 83 EC 08 A1 ? ? ? ? 33 C5 89 45 FC 57 8B F9 8B 87 FC 20 00 00 83 B8 6C 02 00 00 FF 0F 84");
    auto guest = hook::pattern("8B 0D ? ? ? ? 8B 49 2C E8 ? ? ? ? EB 15");
    auto primary = hook::pattern("55 8B EC 8B 0D ? ? ? ? 85 C9 74 09 E8 ? ? ? ? 84 C0 75 1E E8");
    auto bindings = hook::pattern("55 8B EC 56 8B F1 8B 86 B8 26 00 00 85 C0 74 21 8B 40 48 85 C0 74 1A F3 0F 10 45 10 8B 55 08 51");
    auto hudViewport = hook::pattern("55 8B EC 83 EC 10 89 4D F0 C7 45 FC 00 00 00 00 8B 45 F0 83 B8 5C 02 00 00 00 74 3D");
    auto sceneTicks = hook::pattern("55 8B EC 8B 01 F3 0F 10 45 0C 8B 90 60 01 00 00 51 F3 0F 11 04 24 FF D2 B0 01 5D C2 08 00");
    auto controllerTick = hook::pattern("75 28 83 3D ? ? ? ? 00 75 1F 8B 86 D8 04 00 00 F6 40 28 04");
    auto pawnTick = hook::pattern("55 8B EC F3 0F 10 45 08 83 EC 08 80 3D ? ? ? ? 00 56 8B F1 F3 0F 10 96 08 0C 00 00");
    auto cameraUpdate = hook::pattern("89 96 84 06 00 00 83 3D ? ? ? ? 00 74 0E 8B 46 1C 8B 80 74 03 00 00 C1 E8 1A");
    auto keyboardModeOn = hook::pattern("55 8B EC 56 8B 75 08 57 8B F9 85 F6 75 33 39 35 ? ? ? ? 75 0A 8B 0D ? ? ? ? 85 C9 75 06 8B 0D ? ? ? ? 8B 01 8B 90 F0 01 00 00 FF D2");
    auto gamepadModeOn = hook::pattern("83 3D ? ? ? ? 00 56 8B F1 75 0A 8B 0D ? ? ? ? 85 C9 75 06 8B 0D ? ? ? ? 8B 01 8B 90 F0 01 00 00 FF D2 85 C0 74 16 8B C8 E8");
    auto repaint = hook::pattern("55 8B EC 83 3D ? ? ? ? 00 56 8B F1 74 12 83 7E 30 00 74 4F 8B 46 18 8B 48 28 8B 45 0C 50 EB 30 8B 46 30 85 C0 74 3C 83 B8 10 03 00 00 00 75 18 80 78 4D 00");
    auto moveStick = hook::pattern("55 8B EC F3 0F 10 81 6C 03 00 00 83 EC 08 83 3D ? ? ? ? 00 0F 5A C0 F2 0F 59 C0 74 1C F3 0F 10 89 68 03 00 00");
    auto viewportEvent = hook::pattern("55 8B EC 83 3D ? ? ? ? 00 57 8B F9 74 28 8B 45 1C F3 0F 10 45 14");
    if (engine.empty() || solo.empty() || lobbyTick.empty() || viewportController.empty() || exitRequest.empty() || enter.empty() || xinputPad.empty() ||
        keyboardMode.empty() || resourceDB.empty() || createJob.empty() || destroyJobs.empty() || prepareView.empty() || renderView.empty() || guest.empty() || primary.empty() || bindings.empty() || hudViewport.empty() || sceneTicks.empty() ||
        controllerTick.empty() || pawnTick.empty() || cameraUpdate.empty() || repaint.empty() || moveStick.empty() || keyboardModeOn.empty() || gamepadModeOn.empty() || viewportEvent.empty())
        return;

    Splitscreen::GetEngine = engine.get_first<uint8_t*()>();
    if (!start.empty())
    {
        Splitscreen::StartPressed = start.get_first<void(__fastcall)(void*, void*)>();
        Splitscreen::pLastPad = *start.get_first<int32_t*>(4);
    }
    else
    {
        Splitscreen::StartPressed = start9.get_first<void(__fastcall)(void*, void*)>();
        Splitscreen::pLastPad = *start9.get_first<int32_t*>(12);
    }
    Splitscreen::ButtonX = !buttonX.empty() ? buttonX.get_first<void(__fastcall)(void*, void*)>() : buttonX9.get_first<void(__fastcall)(void*, void*)>();
    Splitscreen::pUIManager = !uiManager.empty() ? *uiManager.get_first<uint8_t**>(1) : *uiManager9.get_first<uint8_t**>(2);
    Splitscreen::padIsXInput = *xinputPad.get_first<int32_t*>(12);
    Splitscreen::pGamepadMode = *xinputPad.get_first<int32_t*>(25);
    Splitscreen::SetKeyboardMode = keyboardMode.get_first<void(__stdcall)(uint8_t*, int32_t)>();
    Splitscreen::shKeyboardModeOn = safetyhook::create_inline(keyboardModeOn.get_first(), Splitscreen::KeyboardModeOn);
    Splitscreen::shGamepadModeOn = safetyhook::create_inline(gamepadModeOn.get_first(), Splitscreen::GamepadModeOn);
    Splitscreen::pRequestingExit = *exitRequest.get_first<uint32_t*>(18);
    Splitscreen::pResourceDB = *resourceDB.get_first<uint8_t**>(1);
    Splitscreen::aoOffset = !box.empty() ? 0x3CC : 0x3C8;
    Splitscreen::CreateVisibilityJob = reinterpret_cast<decltype(Splitscreen::CreateVisibilityJob)>(branch(createJob.get_first(21)));
    Splitscreen::pCameraJob = *destroyJobs.get_first<void***>(1);
    Splitscreen::DestroyVisibilityJob = reinterpret_cast<decltype(Splitscreen::DestroyVisibilityJob)>(branch(destroyJobs.get_first(7)));
    Splitscreen::pLightJobs = *destroyJobs.get_first<void****>(19);
    Splitscreen::pLoadoutGlobal = *guest.get_first<uint8_t**>(2);
    Splitscreen::GetGuestLoadout = reinterpret_cast<decltype(Splitscreen::GetGuestLoadout)>(branch(guest.get_first(9)));
    Splitscreen::GetHudViewport = hudViewport.get_first<uint8_t*(__fastcall)(void*, void*)>();

    Splitscreen::package = *solo.get_first<const char*>(4);
    Splitscreen::section = *solo.get_first<const wchar_t*>(14);
    Splitscreen::Localize = reinterpret_cast<decltype(Splitscreen::Localize)>(branch(solo.get_first(35)));
    Splitscreen::FStringFromText = reinterpret_cast<decltype(Splitscreen::FStringFromText)>(branch(solo.get_first(50)));

    // the box options at ebp-70h (DX9 ebp-74h)
    Splitscreen::boxOptions = !box.empty() ? 0x70 : 0x74;
    static auto LaunchBox = safetyhook::create_mid(!box.empty() ? box.get_first() : box9.get_first(), [](SafetyHookContext& regs)
    {
        Splitscreen::LaunchBox(regs.ebp);
    });

    // a view prepared (its index set from the new view in edi), rendered (renderer in ecx, view parameters +20FCh), the game destroying
    // its jobs
    // (the scene grid: prepare's argument in ebx, renderer +44h)
    static auto PrepareView = safetyhook::create_mid(prepareView.get_first(8), [](SafetyHookContext& regs)
    {
        auto view = Splitscreen::PreparedViewOrder(*reinterpret_cast<int32_t*>(regs.edi));
        Splitscreen::SelectVisibilityJobs(view);
        Splitscreen::PreparingView(view, reinterpret_cast<uint8_t*>(regs.ebx));
    });
    static auto RenderView = safetyhook::create_mid(renderView.get_first(), [](SafetyHookContext& regs)
    {
        auto params = *reinterpret_cast<uint8_t**>(regs.ecx + 0x20FC);
        auto view = Splitscreen::RenderedViewOrder(*reinterpret_cast<int32_t*>(params + 0x26C));
        Splitscreen::SelectVisibilityJobs(view);
        Splitscreen::RenderingView(view, *reinterpret_cast<uint8_t**>(regs.ecx + 0x44));
    });

    // the spot light composite (the call after the depth pass at +12h, after it +16h), its SetRenderState(D3DRS_STENCILENABLE, 1) call
    // (DX11 the game's state cache, value at esp+4; DX9 the device's, value at esp+8)
    auto spotComposite = hook::pattern("E8 ? ? ? ? 83 BF 8C 01 00 00 00 7E 08 53 8B CF E8");
    auto spotComposite9 = hook::pattern("E8 ? ? ? ? 83 BF 54 01 00 00 00 7E 08 53 8B CF E8");
    auto stencilEnable = hook::pattern("6A 01 6A 34 8B CE E8 ? ? ? ? 6A 01 6A 39 8B CE E8");
    auto stencilEnable9 = hook::pattern("6A 01 6A 34 50 FF D2 8B 46 34 8B 08 8B 91 E4 00 00 00 6A 01 6A 39 50 FF D2");
    // the sun's cascades (DirPasses) go through the same composite: player 1's view lost all but the nearest (shorter shadow distance)
    auto sunComposite = hook::pattern("E8 ? ? ? ? 83 BE 8C 01 00 00 00 7E 08 53 8B CE E8");
    auto sunComposite9 = hook::pattern("E8 ? ? ? ? 83 BE 54 01 00 00 00 7E 08 53 8B CE E8");
    auto& composite = spotComposite.size() == 2 ? spotComposite : spotComposite9;
    auto& sun = !sunComposite.empty() ? sunComposite : sunComposite9;
    if (composite.size() == 2 && (!stencilEnable.empty() || !stencilEnable9.empty()))
    {
        static auto CompositeStart = safetyhook::create_mid(composite.get(0).get<void>(5), [](SafetyHookContext& regs)
        {
            Splitscreen::inShadowComposite = true;
        });
        static auto CompositeEnd = safetyhook::create_mid(composite.get(0).get<void>(0x16), [](SafetyHookContext& regs)
        {
            Splitscreen::inShadowComposite = false;
        });
        if (!sun.empty())
        {
            static auto SunCompositeStart = safetyhook::create_mid(sun.get_first(5), [](SafetyHookContext& regs)
            {
                Splitscreen::inShadowComposite = true;
            });
            static auto SunCompositeEnd = safetyhook::create_mid(sun.get_first(0x16), [](SafetyHookContext& regs)
            {
                Splitscreen::inShadowComposite = false;
            });
        }
        if (!stencilEnable.empty())
        {
            static auto StencilEnable = safetyhook::create_mid(stencilEnable.get_first(6), [](SafetyHookContext& regs)
            {
                Splitscreen::StencilEnable(reinterpret_cast<uint32_t*>(regs.esp + 4));
            });
        }
        else
        {
            static auto StencilEnable = safetyhook::create_mid(stencilEnable9.get_first(5), [](SafetyHookContext& regs)
            {
                Splitscreen::StencilEnable(reinterpret_cast<uint32_t*>(regs.esp + 8));
            });
        }
    }

    auto splitLayout = hook::pattern("55 8B EC 8B 41 44 83 EC 10 85 C0 0F 84 ? ? ? ? 56 0F B6 35 ? ? ? ? 57 8B 7D 08");
    if (!splitLayout.empty())
    {
        Splitscreen::SplitLayout = splitLayout.get_first<void(__fastcall)(uint8_t*, void*, int32_t)>();
        Splitscreen::pPlayer1FullWidth = *splitLayout.get_first<uint8_t*>(0x15);
    }

    // the view pool reset once a frame (RenderContext in ecx)
    auto viewPoolReset = hook::pattern("33 C0 89 81 ? ? 00 00 89 41 1C 89 81 ? ? 00 00");
    if (!viewPoolReset.empty())
    {
        static auto ViewPoolReset = safetyhook::create_mid(viewPoolReset.get_first(), [](SafetyHookContext& regs)
        {
            Splitscreen::ResetViewOrders();
        });
    }
    Splitscreen::ResetViewOrders();

    static auto DestroyJobs = safetyhook::create_mid(destroyJobs.get_first(), [](SafetyHookContext& regs)
    {
        Splitscreen::DestroySecondViewJobs();
        Splitscreen::ResetCascadeSlots();
    });

    // flare and godray queries: created (VisibilityPass esi, node arg), issued (flare edi / sun esi, eax slot parity, edx GPU), read
    // (node ebp-8, type ebx+4: 2 flare, 4 godray, GPU ebp-14h); the game's queries are released with all 16 slots
    auto createFlare = hook::pattern("8B 45 08 C6 40 3C 01");
    auto createGodray = hook::pattern("8B 45 08 C6 80 C8 02 00 00 01");
    auto issueFlare = hook::pattern("25 01 00 00 80 79 05 48 83 C8 FE 40 8D 54 50 10 8B 04 97");
    auto issueGodray = hook::pattern("25 01 00 00 80 79 05 48 83 C8 FE 40 8D 14 50 8B 84 96 CC 02 00 00");
    auto readQueries = hook::pattern("25 01 00 00 80 79 05 48 83 C8 FE 40 8B 4B 04 83 F9 02");
    if (!createFlare.empty() && !createGodray.empty() && !issueFlare.empty() && !issueGodray.empty() && !readQueries.empty())
    {
        Splitscreen::CreateQuery = reinterpret_cast<decltype(Splitscreen::CreateQuery)>(branch(createFlare.get_first(-0x13)));
        Splitscreen::queryType = *createFlare.get_first<int8_t>(-0x14);
        static auto CreateFlareQueries = safetyhook::create_mid(createFlare.get_first(), [](SafetyHookContext& regs)
        {
            Splitscreen::CreateSecondViewQueries(reinterpret_cast<uint8_t*>(regs.esi), reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(regs.ebp + 8) + 0x60));
        });
        static auto CreateGodrayQueries = safetyhook::create_mid(createGodray.get_first(), [](SafetyHookContext& regs)
        {
            Splitscreen::CreateSecondViewQueries(reinterpret_cast<uint8_t*>(regs.esi), reinterpret_cast<void**>(*reinterpret_cast<uint8_t**>(regs.ebp + 8) + 0x2EC));
        });
        static auto IssueFlare = safetyhook::create_mid(issueFlare.get_first(12), [](SafetyHookContext& regs)
        {
            Splitscreen::SecondViewQuerySlot(regs, reinterpret_cast<void**>(regs.edi + 0x40), regs.edx);
        });
        static auto IssueGodray = safetyhook::create_mid(issueGodray.get_first(12), [](SafetyHookContext& regs)
        {
            Splitscreen::SecondViewQuerySlot(regs, reinterpret_cast<void**>(regs.esi + 0x2CC), regs.edx);
        });
        static auto ReadQueries = safetyhook::create_mid(readQueries.get_first(12), [](SafetyHookContext& regs)
        {
            auto node = *reinterpret_cast<uint8_t**>(regs.ebp - 8);
            auto type = *reinterpret_cast<int32_t*>(regs.ebx + 4);
            if (node && (type == 2 || type == 4))
                Splitscreen::SecondViewQuerySlot(regs, reinterpret_cast<void**>(node + (type == 2 ? 0x40 : 0x2CC)), *reinterpret_cast<uint32_t*>(regs.ebp - 0x14));
        });
    }

    static auto LobbyTick = safetyhook::create_mid(lobbyTick.get_first(), [](SafetyHookContext& regs)
    {
        Splitscreen::LobbyTick(reinterpret_cast<uint8_t*>(regs.esi));
    });

    Splitscreen::shViewportInput = safetyhook::create_inline(viewportEvent.get_first(), Splitscreen::ViewportInput);

    static void* sceneTickHandlers[] = { &Splitscreen::SceneTick<0>, &Splitscreen::SceneTick<1>, &Splitscreen::SceneTick<2>, &Splitscreen::SceneTick<3>,
                                         &Splitscreen::SceneTick<4>, &Splitscreen::SceneTick<5>, &Splitscreen::SceneTick<6>, &Splitscreen::SceneTick<7> };
    Splitscreen::shSceneTick.resize(std::size(sceneTickHandlers));
    for (size_t i = 0; i < sceneTicks.size() && i < std::size(sceneTickHandlers); i++)
        Splitscreen::shSceneTick[i] = safetyhook::create_inline(sceneTicks.get(i).get<void>(), sceneTickHandlers[i]);

    // the calls after the width is doubled
    for (auto restores : { hook::pattern("8B ? A4 04 00 00 8B ? A0 04 00 00 50 51 03 D2 52 8B CF E8"), hook::pattern("8B ? A0 04 00 00 50 8B ? A4 04 00 00 03 C9 50 51 8B CE E8") })
    {
        restores.for_each_result([](hook::pattern_match match)
        {
            Splitscreen::resolutionRestores.push_back(match.get<uint8_t>(0x18));
        });
    }
    auto endSplit = hook::pattern("56 8B F1 83 A6 EC 01 00 00 EF E8 ? ? ? ? C7 86 7C 02 00 00 00 00 00 00 5E C3");
    auto removePlayer = hook::pattern("55 8B EC 56 57 8B F9 8B 47 44 33 F6 39 70 30 7E ? 8B 40 2C 8B 0C B0 6A FF E8");
    if (!endSplit.empty() && !removePlayer.empty())
    {
        Splitscreen::RemoveSecondaryLocalPlayer = removePlayer.get_first<void(__fastcall)(uint8_t*, void*, int32_t)>();
        Splitscreen::shEndSplitScreen = safetyhook::create_inline(endSplit.get_first(), Splitscreen::EndSplitScreen);
    }

    if (!Splitscreen::resolutionRestores.empty())
    {
        auto call = reinterpret_cast<uintptr_t>(Splitscreen::resolutionRestores.front()) - 5;
        Splitscreen::shRequestResolution = safetyhook::create_inline(reinterpret_cast<void*>(branch(reinterpret_cast<void*>(call))), Splitscreen::RequestResolution);
    }

    Splitscreen::shDefaultController = safetyhook::create_inline(viewportController.get_first(), Splitscreen::DefaultController);
    Splitscreen::shEnter = safetyhook::create_inline(enter.get_first(), Splitscreen::Enter);
    Splitscreen::shIsPrimaryController = safetyhook::create_inline(primary.get_first(), Splitscreen::IsPrimaryController);
    Splitscreen::shProcessBindings = safetyhook::create_inline(bindings.get_first(), Splitscreen::ProcessBindings);
    Splitscreen::shControllerTick = safetyhook::create_inline(controllerTick.get_first(-0x15B), Splitscreen::ControllerTick);
    Splitscreen::shPawnTick = safetyhook::create_inline(pawnTick.get_first(), Splitscreen::PawnTick);
    Splitscreen::shRepaint = safetyhook::create_inline(repaint.get_first(), Splitscreen::Repaint);
    Splitscreen::shMoveStick = safetyhook::create_inline(moveStick.get_first(), Splitscreen::MoveStick);
    Splitscreen::shCameraUpdate = safetyhook::create_inline(cameraUpdate.get_first(-0xAF), Splitscreen::CameraUpdate);
    Splitscreen::shUpdateInput = safetyhook::create_inline(xinputPad.get_first(-0x74), Splitscreen::UpdateInput);
    Splitscreen::shXInputGetState = safetyhook::create_inline(getState, Splitscreen::XInputGetStateHook);
    Splitscreen::shXInputSetState = safetyhook::create_inline(setState, Splitscreen::XInputSetStateHook);
    Splitscreen::shXInputGetCapabilities = safetyhook::create_inline(getCapabilities, Splitscreen::XInputGetCapabilitiesHook);
}
