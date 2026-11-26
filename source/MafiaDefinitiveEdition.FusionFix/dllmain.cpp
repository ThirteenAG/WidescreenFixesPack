#include "stdafx.h"
#include "FixSettings.hpp"

import Mouse;

ASI_SETTINGS_EXPORT(FixSettings())

namespace MenuCursor
{
    // The menu cursor is a sprite the game moves by DirectInput counts through its own dead zone and speed, and twice
    // per move (the cursor and the map navigation actions both drive it), so it does not follow the Windows pointer
    // speed and acceleration and feels detached from the hand. While the Windows pointer moves, the cursor takes its
    // position in the game window (scaled to the cursor area) instead. When the pointer does not move with the mouse
    // (exclusive mouse mode, pointer held at a clipped edge) and for gamepad input the game's own movement stays.
    SafetyHookInline shGuiInput{};
    HWND gameWindow = nullptr;
    uint8_t** windowOwner = nullptr;
    int32_t windowOffset = 0;
    HWINEVENTHOOK foregroundEvents = nullptr;
    std::atomic<uint64_t> focusGeneration{};
    uint64_t sampledGeneration = 0;
    POINT lastPointer{};
    ULONGLONG lastPointerMove = 0;

    void CALLBACK ForegroundChanged(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD)
    {
        ++focusGeneration;
    }

    bool PointerInWindow(HWND window, POINT& point, RECT& client)
    {
        auto owner = *windowOwner;
        if (!owner || !window || window != *reinterpret_cast<HWND*>(owner + windowOffset) ||
            !GetCursorPos(&point) || !ScreenToClient(window, &point) || !GetClientRect(window, &client) ||
            client.right <= 0 || client.bottom <= 0)
        {
            gameWindow = nullptr;
            lastPointerMove = 0;
            return false;
        }
        const auto generation = focusGeneration.load();
        if (gameWindow != window || sampledGeneration != generation)
        {
            gameWindow = window;
            sampledGeneration = generation;
            lastPointer = point;
            lastPointerMove = 0;
            return false;
        }
        return true;
    }

    bool SyncPointer(uint8_t* input, bool axisEvent)
    {
        POINT point{};
        RECT client{};
        if (!PointerInWindow(GetForegroundWindow(), point, client) || !PtInRect(&client, point))
            return false;

        const auto area = reinterpret_cast<const float*>(input + 0x38);
        if (area[2] <= area[0] || area[3] <= area[1])
            return false;

        const auto now = GetTickCount64();
        if (point.x != lastPointer.x || point.y != lastPointer.y)
        {
            lastPointer = point;
            lastPointerMove = now;
        }
        else if (!axisEvent || !lastPointerMove || now - lastPointerMove >= 100)
            return false;

        auto cursor = reinterpret_cast<float*>(input + 0x10);
        cursor[0] = area[0] + (area[2] - area[0]) * float(point.x) / float(client.right);
        cursor[1] = area[1] + (area[3] - area[1]) * float(point.y) / float(client.bottom);
        return true;
    }

    uint32_t __fastcall GuiInput(uint8_t* input, uint16_t action, float value, uint8_t pressed, int32_t type, uint32_t id, int32_t parameter, int32_t direction, int32_t deviceIndex)
    {
        // The translator ignores unbound cursor directions before it can notify the active movie.
        if (type == 1 && (id == 300 || id == 301) && direction < 4 && SyncPointer(input, true))
            value = 0.0f;
        return shGuiInput.unsafe_call<uint32_t>(input, action, value, pressed, type, id, parameter, direction, deviceIndex);
    }

    void Init()
    {
        // The engine restores its main window from this object when returning to the application.
        auto restore = hook::pattern("48 8B 0D ? ? ? ? BA 09 00 00 00 48 8B 89 ? ? ? ? FF 15 ? ? ? ? 33 C9 FF 15");
        auto instruction = restore.get_first<uint8_t>();
        windowOwner = reinterpret_cast<uint8_t**>(instruction + 7 + *reinterpret_cast<int32_t*>(instruction + 3));
        windowOffset = *restore.get_first<int32_t>(15);
        foregroundEvents = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
            ForegroundChanged, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
        // GUI input translator: moves the cursor by cursor axis input, clamps it to the cursor area and sends it to the UI
        auto pattern = hook::pattern("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 60 48 63 84 24 90 00 00 00 41 0F B6 E9 0F 29 74 24 50 0F 28 F2 48 8B F9 83 F8 05");
        shGuiInput = safetyhook::create_inline(pattern.get_first(), GuiInput);

        // The GUI manager updates its input and movie components on the same thread as GUI actions.
        // A Windows pointer move can arrive without a DirectInput axis action.
        auto update = hook::pattern("48 89 5C 24 08 57 48 83 EC 20 48 8B D9 48 8B FA 48 8B 49 48 48 8B 01 FF 50 28 48 8B 4B 68 48 8B D7");
        static auto cursorUpdate = safetyhook::create_mid(update.get_first(), [](SafetyHookContext& ctx)
        {
            auto input = *reinterpret_cast<uint8_t**>(ctx.rcx + 0x20);
            if (input && SyncPointer(input, false))
                shGuiInput.unsafe_call<uint32_t>(input, uint16_t{}, 0.0f, uint8_t{}, 1, 300u, 0, 0, 0);
        });
    }
}

namespace StartupScreens
{
    // Startup state: on entering it queues the middleware logo and disclaimer pages, then plays the 2K and Hangar 13
    // logo movie while the profile signs in and the game loads. Starting in its loading step skips both; the waits for
    // the sign-in and the loading stay.
    SafetyHookInline shStartupEnter{};

    void __fastcall StartupEnter(uint8_t* state)
    {
        *reinterpret_cast<int32_t*>(state + 0x94) = 2;
    }

    // Title state: queues the "press Enter to start" page (sequence 9) unless first-time settings are shown first; the
    // state moves on to the main menu when the queued pages are done. Leaving the page out goes straight on.
    SafetyHookInline shTitleEnter{};
    SafetyHookInline shQueuePage{};
    thread_local bool bTitleEnter = false;

    void __fastcall QueuePage(uint8_t* pages, int32_t page, bool force)
    {
        if (bTitleEnter && page == 9 && FixSettings().Get("MAIN.SkipPressStart") == "1")
        {
            if (*reinterpret_cast<uint64_t*>(pages + 0x38) == 0)
                *(pages + 0x44) = 1;  // nothing else queued: the pages are done
            return;
        }
        shQueuePage.unsafe_call(pages, page, force);
    }

    void __fastcall TitleEnter(void* state)
    {
        const bool previous = std::exchange(bTitleEnter, true);
        shTitleEnter.unsafe_call(state);
        bTitleEnter = previous;
    }

    void Init(bool skipIntro)
    {
        if (skipIntro)
        {
            auto pattern = hook::pattern("48 89 5C 24 08 57 48 83 EC 20 48 8B D9 E8 ? ? ? ? 33 FF 48 8B 90 C0 00 00 00 48 85 D2 48 8D 8A D0 02 00 00");
            shStartupEnter = safetyhook::create_inline(pattern.get_first(), StartupEnter);
        }

        auto pattern = hook::pattern("48 8B C4 48 89 58 08 57 48 83 EC 70 48 8B D9 89 50 10 48 83 C1 18 48 8D 50 10 41 0F B6 F8 E8 ? ? ? ? 83 7B 40 05");
        shQueuePage = safetyhook::create_inline(pattern.get_first(), QueuePage);

        pattern = hook::pattern("48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 40 48 8B D9 48 8D 54 24 30 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8D 4C 24 30");
        shTitleEnter = safetyhook::create_inline(pattern.get_first(), TitleEnter);
    }
}

void Init(bool skipIntro)
{
    InitMouse();
    MenuCursor::Init();

    StartupScreens::Init(skipIntro);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        const bool skipIntro = FixSettings().Get("MAIN.SkipIntro") == "1";
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime([skipIntro] { Init(skipIntro); }, hook::pattern("C7 44 24 20 01 00 00 00 FF 15 ? ? ? ? 85 C0 74 ? 48 8D 4C 24 48 FF 15"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH && !lpReserved && MenuCursor::foregroundEvents)
        UnhookWinEvent(MenuCursor::foregroundEvents);
    return TRUE;
}
