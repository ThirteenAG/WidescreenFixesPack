module;

#include <stdafx.h>
#include <ddraw.h>
#include "FixSettings.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"

export module ComVars;

// The configured resolution and its 4:3 width
export struct Screen
{
    int32_t Width;
    int32_t Height;
    int32_t Width43;
} Screen;

// The executables this fix supports are three different compiles of the game:
//
// - Classics: the Rockstar Classics / Steam GTA1 and London 1969/1961 (WINO).
// - Retail: the original GTA1 retail release.
// - London: the original London 1969 and 1961 retail releases.
//
// Retail and London pack their game structures (player, car and ped records are
// smaller), keep the raster globals one slot further from the row table and, in
// London, call every function through a jump stub. Signatures, the byte offsets
// read from them and these constants are chosen per build.
export namespace GTA1Build
{
    enum class Kind { Classics, Retail, London };
    Kind Current = Kind::Classics;

    // Game records.
    int PlayerStride = 444, ViewOffset = -0xA4;
    int CarStride = 688, CarHeading = 144, PedHeading = 168;
    // Raster globals in front of the row table: pitch, bytes per pixel, pitch in pixels.
    int RasterPitch = -4, RasterBytes = -3, RasterPixels = -2;
    // Mode list entries: size and the offset of the mode pointer.
    int ModeEntrySize = 28, ModeEntryMode = 24;

    void Set(Kind kind)
    {
        Current = kind;
        if (kind == Kind::Classics) return;
        PlayerStride = 417; ViewOffset = -0xA1;
        CarStride = 664; CarHeading = 144; PedHeading = 153;
        RasterPitch = -5; RasterBytes = -4; RasterPixels = -3;
        ModeEntrySize = 25; ModeEntryMode = 21;
    }

    // The signature and an offset for the running build.
    const char* Sig(const char* classics, const char* retail, const char* london)
    {
        switch (Current)
        {
            case Kind::Retail: return retail;
            case Kind::London: return london;
            default: return classics;
        }
    }
    int Off(int classics, int retail, int london)
    {
        switch (Current)
        {
            case Kind::Retail: return retail;
            case Kind::London: return london;
            default: return classics;
        }
    }
    hook::pattern Find(const char* classics, const char* retail, const char* london)
    {
        return hook::pattern(Sig(classics, retail, london));
    }

    // London calls through jump stubs: the function a call or stub leads to.
    uint8_t* Body(void* address)
    {
        auto code = static_cast<uint8_t*>(address);
        for (int i = 0; code && i < 4 && code[0] == 0xE9; ++i)
            code = code + 5 + *reinterpret_cast<int32_t*>(code + 1);
        return code;
    }
    uint8_t* Target(void* call) { return Body(injector::GetBranchDestination(call).get<void>()); }
}

// Game state
export GameRef<int> nLocalPlayer;               // the local player's index
export GameRef<int> nReplay;                    // replay mode
export GameRef<int> nPlayerCount;
export GameRef<int> nPaused;                    // 2: paused
export GameRef<int> nRecording;
export char* PlayerStates = nullptr;            // player records, GTA1Build::PlayerStride bytes each
export char* (__cdecl* GetPed)(int16_t) = nullptr;

// Frontend and presentation
export GameRef<int> nMenuState;                 // frontend state
export GameRef<int*> pDisplayDC;                // MGL device contexts: the display and the offscreen page
export GameRef<int*> pOffscreenDC;
export GameRef<int> nDisplayWidth;              // the frame's size
export GameRef<int> nDisplayHeight;
export GameRef<uint8_t*> pDrawPixels;           // where the software renderer draws
export GameRef<int> nScreenWidth;               // the rasteriser's screen size
export GameRef<int> nScreenHeight;
export GameRef<int> nClipLeft;                 // the rasteriser's clip rectangle
export GameRef<int> nClipRight;
export GameRef<int> nClipTop;
export GameRef<int> nClipBottom;
// The simulation's tick timer
export GameRef<uint8_t> bTimerActive;
export GameRef<int> nTimerEnabled;
export GameRef<int> nTimerTicks;                // timer ticks counted since the last step
export GameRef<uint8_t> nModeIndex;            // the display mode applied when a game starts
export GameRef<DDSURFACEDESC> sLockedSurface;  // the primary surface MGL locked
export GameRef<int> nMenuSelection;            // the frontend's selected item, from 1
export GameRef<int> nSmallFont;                 // the frontend's fonts
export GameRef<int> nHeaderFont;
export GameRef<int> nMarkerFont;
export GameRef<uint8_t*> pTextPixels;           // where the frontend's text routines draw
export GameRef<int> nTextPitch;                 // in pixels
// The display mode switch's width stores
export GameRef<uint32_t> nWidth;
export GameRef<uint32_t> dword_787370;
export GameRef<uint32_t> dword_504CC0;
export void* MenuFrame = nullptr;               // the frontend state machine
export void* PresentPage = nullptr;             // the page presenter

// The native quit prompt ("Press ENTER to confirm") is open
export bool (__cdecl* QuitPromptOpen)() = nullptr;
export bool QuitPromptActive() { return QuitPromptOpen && QuitPromptOpen(); }

export bool Available()
{
    return !nReplay && nPlayerCount == 1 && !nRecording;
}
export bool IsPaused() { return nPaused == 2; }
export bool TextEntry() { return nMenuState == 12; }

// A native key code (DirectInput scan code) for the event reader's next read
export std::atomic<int> PendingKey = 0;

// The rotating camera's turn, shared with the input
export namespace GTA1Rotation
{
    bool Active = false, Enabled = true;
    int ToggleKey = 'C';                        // F6 is the native pause key
    float Angle = 0.0f, Sin = 0.0f, Cos = 1.0f;

    // A direction on screen (x right, y down) to the direction it has in the world.
    void ScreenToWorld(float& x, float& y)
    {
        if (!Active) return;
        float wx = x * Cos + y * Sin, wy = -x * Sin + y * Cos;
        x = wx; y = wy;
    }

    void KeyDown(WPARAM key)
    {
        if (ToggleKey && int(key) == ToggleKey) Enabled = !Enabled;
    }
}

// The game globals the fixes share, looked up before any fix hooks the code they are read from
export void InitComVars()
{
    using GTA1Build::Find, GTA1Build::Off;
    // The player state array (from the packet handler), the local player, the replay
    // mode, the ped getter, the player count, the recording flag and the pause state.
    auto input = Find("A1 ? ? ? ? 53 8B 5C 24 08 56 F6 C3 01 74 35", "8B BC C1 ? ? ? ? 8D 04 C1 85 FF",
        "8B 0D ? ? ? ? 8D 04 49 8D 04 81 C1 E0 05 03 C1 8B 88 ? ? ? ? 83 E9 00 74 10 83 E9 02");
    auto local = Find("8B 0D ? ? ? ? 8B 15 ? ? ? ? 33 C0 3B CA 0F 94 C0 C3 90 90 90 90 90 90 90 90 90 90 90 90 0F BE 44 24 04",
        "33 C0 8B 0D ? ? ? ? 3B 0D ? ? ? ? 0F 94 C0 C3", "8B 0D ? ? ? ? 8B 15 ? ? ? ? 33 C0 3B CA 0F 94 C0 C3");
    auto replay = Find("51 A1 ? ? ? ? 53 55 33 ED 2B C5 56 89 6C 24 0C",
        "83 EC 04 A1 ? ? ? ? 85 C0 C7 44 24 00 00 00 00 00 53 56 57 55", "51 A1 ? ? ? ? 56 83 E8 00 C7 44 24 04 00 00 00 00");
    auto ped = Find("0F BF 44 24 04 C1 E0 08 05 ? ? ? ? C3 90 90 57",
        "0F BF 44 24 04 53 8B C8 8D 14 40 8D 04 91 8D 1C C0 8D 84 59 ? ? ? ? 5B C3", "0F BF 4C 24 04 8D 04 49 C1 E0 04 2B C1 8D 84 80 ? ? ? ? C3");
    auto mode = Find("83 3D ? ? ? ? 01 75 09 8B 44 24 04 A3 ? ? ? ? C3",
        "A1 ? ? ? ? 83 F8 01 75 09 8B 44 24 04 A3 ? ? ? ? C3", "83 3D ? ? ? ? 01 75 09 8B 44 24 04 A3 ? ? ? ? C3");
    auto recording = hook::pattern("89 15 ? ? ? ? 8B 54 24 60 A3 ? ? ? ? 8B 44 24 68");
    auto paused = Find("83 3D ? ? ? ? 02 0F 94 C0 C3", "83 3D ? ? ? ? 02 0F 94 C0 C3", "8B 0D ? ? ? ? 33 C0 83 F9 02 0F 94 C0 C3");

    nLocalPlayer.SetAddress(*local.get_first<int*>(Off(8, 4, 8)));
    nReplay.SetAddress(*replay.get_first<int*>(Off(2, 4, 2)));
    nPlayerCount.SetAddress(*mode.get_first<int*>(Off(2, 1, 2)));
    nPaused.SetAddress(*paused.get_first<int*>(2));
    nRecording.SetAddress(*recording.get_first<int*>(2));
    PlayerStates = *input.get_first<char*>(Off(49, 3, 19));
    GetPed = reinterpret_cast<decltype(GetPed)>(ped.get_first());

    // The frontend state machine and the page presenter.
    auto menu = Find("51 A1 ? ? ? ? 53 55 33 ED 56 83 F8 14 57 89 2D",
        "53 A1 ? ? ? ? 56 83 F8 14 C7 05 ? ? ? ? 00 00 00 00 55 0F 87", "A1 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 83 F8 14 0F 87");
    auto present = Find("A1 ? ? ? ? 85 C0 74 27 8B 0D ? ? ? ? 8B 15 ? ? ? ? 6A 00 6A 00 6A 00 51 52",
        "56 A1 ? ? ? ? 85 C0 74 2C 6A 00 A1 ? ? ? ? 6A 00 8B 0D ? ? ? ? 6A 00 8B 15 ? ? ? ? 50 51 6A 00 A1",
        "A1 ? ? ? ? 85 C0 74 27 8B 0D ? ? ? ? 8B 15 ? ? ? ? 6A 00 6A 00 6A 00 51 52");
    nMenuState.SetAddress(*menu.get_first<int*>(Off(2, 2, 1)));
    MenuFrame = menu.get_first();
    pOffscreenDC.SetAddress(*present.get_first<int**>(Off(1, 2, 1)));
    nDisplayHeight.SetAddress(*present.get_first<int*>(Off(11, 13, 11)));
    nDisplayWidth.SetAddress(*present.get_first<int*>(Off(17, 21, 17)));
    pDisplayDC.SetAddress(*present.get_first<int**>(Off(35, 38, 35)));
    PresentPage = present.get_first();
    auto drawPixels = Find("8B 96 A0 01 00 00 83 C4 30 89 15 ? ? ? ? 5E C3", "8B 8E A0 01 00 00 89 0D ? ? ? ? 5E C3",
        "8B 96 A0 01 00 00 83 C4 0C 89 15 ? ? ? ? 5E C3");
    pDrawPixels.SetAddress(*drawPixels.get_first<uint8_t**>(Off(11, 8, 11)));

    auto dimensions = hook::pattern("89 15 ? ? ? ? 8B 40 0C A3 ? ? ? ? C3");
    nScreenWidth.SetAddress(*dimensions.get_first<int*>(2));
    nScreenHeight.SetAddress(*dimensions.get_first<int*>(10));
}
