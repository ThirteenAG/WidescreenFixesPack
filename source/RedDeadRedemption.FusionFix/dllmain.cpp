#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

namespace rdrFlashIntroHandler
{
    struct FlashIntroHandler
    {
        char unk[17];
        char bLegalsCompleted;
    };

    SafetyHookInline FlashIntroHandlerHook{};
    void __fastcall Update(FlashIntroHandler* smp_Instance)
    {
        // The startup logos count as finished; the title screen follows right away
        smp_Instance->bLegalsCompleted = 1;
    }
}

namespace MouseCursor
{
    struct GameWindow
    {
        uint8_t pad[0xA8];
        bool bCursorShown;
        uint8_t pad2[4];
        bool bActive;
    };

    struct MouseDevice
    {
        uint8_t pad[0x0B];
        bool bButtons[3];
        uint8_t pad2[0x44 - 0x0E];
        float fDeltaX;
        float fDeltaY;
        float fWheel;
    };

    // The title screen and other mouse-driven menus keep the pointer up from the moment they open until the mouse is
    // no longer wanted, and pad input doesn't change that. With the intro shown, a pad user has pressed buttons long
    // before the title appears and the menu opens in pad mode; without it, the title opens before the first pad read
    // and the pointer stayed in the middle of the screen. Hide it from the first pad input until the mouse is used.
    bool bPadInUse = false;

    void ReadPad()
    {
        static ULONGLONG nextSearch = 0;
        auto now = GetTickCount64();
        if (now < nextSearch)
            return;

        XINPUT_STATE state{};
        DWORD slot = 0;
        if (!PadInput::Read(state, slot))
        {
            // Polling empty slots is slow; look for a newly connected pad once a second
            nextSearch = now + 1000;
            return;
        }

        auto& pad = state.Gamepad;
        if (pad.wButtons || pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD || pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ||
            std::abs(pad.sThumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE || std::abs(pad.sThumbLY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
            std::abs(pad.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE || std::abs(pad.sThumbRY) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
        {
            bPadInUse = true;
        }
    }

    SafetyHookInline UpdateCursorHook{};
    void __fastcall UpdateCursor(GameWindow* window)
    {
        ReadPad();
        if (bPadInUse && window->bActive)
        {
            if (window->bCursorShown)
            {
                while (ShowCursor(FALSE) >= 0)
                    ;
                window->bCursorShown = false;
            }
            // The window has no cursor of its own; the game sets the arrow again when it shows the pointer
            if (GetCursor())
                SetCursor(nullptr);
            return;
        }
        UpdateCursorHook.fastcall(window);
    }

    SafetyHookInline MouseUpdateHook{};
    bool __fastcall MouseUpdate(MouseDevice* mouse)
    {
        auto result = MouseUpdateHook.fastcall<bool>(mouse);
        if (result && (mouse->fDeltaX != 0.0f || mouse->fDeltaY != 0.0f || mouse->fWheel != 0.0f ||
            mouse->bButtons[0] || mouse->bButtons[1] || mouse->bButtons[2]))
        {
            bPadInUse = false;
        }
        return result;
    }

    void Init()
    {
        // Shows or hides the system pointer once per frame and when the window gets or loses focus
        auto pattern = hook::pattern("40 53 48 83 EC 20 80 B9 AD 00 00 00 00 48 8B D9 74 ? 80 B9 B1 00 00 00 00");
        UpdateCursorHook = safetyhook::create_inline(pattern.get_first(), UpdateCursor);

        // Reads the mouse movement and buttons each frame
        pattern = hook::pattern("48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC 40 48 83 79 70 00 48 8B F9 75");
        MouseUpdateHook = safetyhook::create_inline(pattern.get_first(), MouseUpdate);
    }
}

void Init()
{
    MouseCursor::Init();

    bool bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");

    if (bSkipIntro)
    {
        // rdrFlashIntroHandler::Update
        auto pattern = hook::pattern("40 56 57 41 57 48 83 EC 30 48 89 6C 24 58 45 33 FF 40 32 ED 48 8B F9 81 79 08 E8 03 00 00");
        rdrFlashIntroHandler::FlashIntroHandlerHook = safetyhook::create_inline(pattern.get_first(), rdrFlashIntroHandler::Update);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init);
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
