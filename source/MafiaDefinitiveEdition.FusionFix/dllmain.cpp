#include "stdafx.h"
#include "FixSettings.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

namespace MessagePump
{
    // The game's message pump calls PeekMessageA until the queue is empty, once per frame. Every mouse move ends in a
    // WM_MOUSEMOVE whose retrieval costs the kernel's input processing, and with a high polling-rate mouse new moves
    // arrive while the previous one is still being handled, so the queue is never empty and a single frame can take
    // hundreds of milliseconds. A pass now ends after one WM_MOUSEMOVE; the rest waits for the next frame. Windows
    // merges pending mouse moves into one message and the game reads mouse look through DirectInput, so no movement
    // is lost or added.
    decltype(&PeekMessageA) pPeekMessageA = nullptr;
    bool bMouseMoveThisPass = false;

    BOOL WINAPI PeekMessageAHook(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
    {
        if (bMouseMoveThisPass)
        {
            bMouseMoveThisPass = false;
            return FALSE;
        }

        BOOL result = pPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
        bMouseMoveThisPass = result && lpMsg->message == WM_MOUSEMOVE;
        return result;
    }

    void Init()
    {
        // call PeekMessageA in the game's pump (its only caller)
        auto pattern = hook::pattern("C7 44 24 20 01 00 00 00 FF 15 ? ? ? ? 85 C0 74 ? 48 8D 4C 24 48 FF 15 ? ? ? ? 48 8D 4C 24 48 FF 15 ? ? ? ? 83 7C 24 50 12");
        auto call = pattern.get_first<uint8_t>(8);
        auto slot = reinterpret_cast<decltype(&PeekMessageA)*>(call + 6 + *reinterpret_cast<int32_t*>(call + 2));
        pPeekMessageA = *slot;
        injector::WriteMemory(slot, &PeekMessageAHook, true);
    }
}

namespace MenuCursor
{
    // The menu cursor is a sprite the game moves by DirectInput counts through its own dead zone and speed, and twice
    // per move (the cursor and the map navigation actions both drive it), so it does not follow the Windows pointer
    // speed and acceleration and feels detached from the hand. While the Windows pointer moves, the cursor takes its
    // position in the game window (scaled to the cursor area) instead. When the pointer does not move with the mouse
    // (exclusive mouse mode, pointer held at a clipped edge) and for gamepad input the game's own movement stays.
    SafetyHookInline shGuiInput{};
    POINT lastPointer{ LONG_MIN, LONG_MIN };
    ULONGLONG lastPointerMove = 0;

    bool PointerInWindow(POINT& point, RECT& client)
    {
        HWND wnd = GetForegroundWindow();
        DWORD pid = 0;
        GetWindowThreadProcessId(wnd, &pid);
        return pid == GetCurrentProcessId() && GetCursorPos(&point) && ScreenToClient(wnd, &point) && GetClientRect(wnd, &client) &&
               client.right > 0 && client.bottom > 0;
    }

    uint32_t __fastcall GuiInput(uint8_t* input, uint16_t action, float value, uint8_t pressed, int32_t type, uint32_t id, int32_t a7, int32_t a8)
    {
        // type 1 = cursor axis, ids 300/301 = X/Y
        if (type == 1 && (id == 300 || id == 301))
        {
            POINT point{};
            RECT client{};
            if (PointerInWindow(point, client))
            {
                ULONGLONG now = GetTickCount64();
                if (point.x != lastPointer.x || point.y != lastPointer.y)
                {
                    lastPointer = point;
                    lastPointerMove = now;
                }

                if (now - lastPointerMove < 100)
                {
                    auto cursor = reinterpret_cast<float*>(input + 0x10);  // x, y
                    auto area = reinterpret_cast<float*>(input + 0x38);    // left, top, right, bottom
                    cursor[0] = area[0] + (area[2] - area[0]) * float(point.x) / float(client.right);
                    cursor[1] = area[1] + (area[3] - area[1]) * float(point.y) / float(client.bottom);
                    value = 0.0f;
                }
            }
        }
        return shGuiInput.unsafe_call<uint32_t>(input, action, value, pressed, type, id, a7, a8);
    }

    void Init()
    {
        // GUI input translator: moves the cursor by cursor axis input, clamps it to the cursor area and sends it to the UI
        auto pattern = hook::pattern("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 60 48 63 84 24 90 00 00 00 41 0F B6 E9 0F 29 74 24 50 0F 28 F2 48 8B F9 83 F8 05");
        shGuiInput = safetyhook::create_inline(pattern.get_first(), GuiInput);
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
    bool bTitleEnter = false;

    void __fastcall QueuePage(uint8_t* pages, int32_t page, bool force)
    {
        if (bTitleEnter && page == 9)
        {
            if (*reinterpret_cast<uint64_t*>(pages + 0x38) == 0)
                *(pages + 0x44) = 1;  // nothing else queued: the pages are done
            return;
        }
        shQueuePage.unsafe_call(pages, page, force);
    }

    void __fastcall TitleEnter(void* state)
    {
        bTitleEnter = true;
        shTitleEnter.unsafe_call(state);
        bTitleEnter = false;
    }

    void Init()
    {
        auto pattern = hook::pattern("48 89 5C 24 08 57 48 83 EC 20 48 8B D9 E8 ? ? ? ? 33 FF 48 8B 90 C0 00 00 00 48 85 D2 48 8D 8A D0 02 00 00");
        shStartupEnter = safetyhook::create_inline(pattern.get_first(), StartupEnter);

        pattern = hook::pattern("48 8B C4 48 89 58 08 57 48 83 EC 70 48 8B D9 89 50 10 48 83 C1 18 48 8D 50 10 41 0F B6 F8 E8 ? ? ? ? 83 7B 40 05");
        shQueuePage = safetyhook::create_inline(pattern.get_first(), QueuePage);

        pattern = hook::pattern("48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 40 48 8B D9 48 8D 54 24 30 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8D 4C 24 30");
        shTitleEnter = safetyhook::create_inline(pattern.get_first(), TitleEnter);
    }
}

void Init()
{
    MessagePump::Init();
    MenuCursor::Init();

    if (FixSettings().Bool("MAIN.SkipIntro"))
        StartupScreens::Init();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("C7 44 24 20 01 00 00 00 FF 15 ? ? ? ? 85 C0 74 ? 48 8D 4C 24 48 FF 15"));
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
