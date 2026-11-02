module;

#include <stdafx.h>
#include "common.h"
#include "FixSettings.hpp"
#include <control.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

export module Frontend2;

import Skeleton;
import Sprite2d;
import Draw;

namespace VideoPlayer
{
    constexpr UINT_PTR SubclassId = 0x57465056;
    constexpr UINT RepositionMessage = WM_APP + 0x57F;  // the movie mode changed while a movie plays
    static HWND owner = nullptr;
    static IVideoWindow* currentVideo = nullptr; // Borrowed; checked against the game's pointer.
    static long sourceWidth = 4, sourceHeight = 3;
    static RECT videoRect{};
    static bool positioning = false;

    static bool IsPlaying()
    {
        return currentVideo && currentVideo == *reinterpret_cast<IVideoWindow**>(0xC920E0);
    }

    static void PaintMargins(HWND window, HDC dc)
    {
        RECT client{};
        if (!GetClientRect(window, &client)) return;
        const RECT margins[] = {
            { 0, 0, client.right, videoRect.top },
            { 0, videoRect.bottom, client.right, client.bottom },
            { 0, videoRect.top, videoRect.left, videoRect.bottom },
            { videoRect.right, videoRect.top, client.right, videoRect.bottom }
        };
        for (const auto& margin : margins)
            FillRect(dc, &margin, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    }

    static void Position(HWND window)
    {
        if (positioning || !IsPlaying()) return;
        RECT client{};
        if (!GetClientRect(window, &client) || client.right <= 0 || client.bottom <= 0) return;
        // Videos are never cut off: the whole movie at its own aspect with black bars, fitted into the game's 4:3
        // picture (Original) or into the whole screen (Fill)
        static auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
        double frameWidth = client.right, frameHeight = client.bottom;
        if (mode == 0)
        {
            if (frameWidth * 3.0 > frameHeight * 4.0)
                frameWidth = frameHeight * 4.0 / 3.0;
            else
                frameHeight = frameWidth * 3.0 / 4.0;
        }
        const double scale = std::min(frameWidth / sourceWidth, frameHeight / sourceHeight);
        const long width = std::max(1L, static_cast<long>(sourceWidth * scale));
        const long height = std::max(1L, static_cast<long>(sourceHeight * scale));
        const long left = (client.right - width) / 2;
        const long top = (client.bottom - height) / 2;
        positioning = true;
        if (SUCCEEDED(currentVideo->SetWindowPosition(left, top, width, height)))
            videoRect = { left, top, left + width, top + height };
        positioning = false;
        InvalidateRect(window, nullptr, FALSE);
    }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam,
        LPARAM lParam, UINT_PTR id, DWORD_PTR)
    {
        if (message == WM_NCDESTROY || !IsPlaying())
        {
            RemoveWindowSubclass(window, WindowProc, id);
            owner = nullptr;
            currentVideo = nullptr;
            return DefSubclassProc(window, message, wParam, lParam);
        }
        if (message == WM_SIZE)
        {
            const auto result = DefSubclassProc(window, message, wParam, lParam);
            if (wParam != SIZE_MINIMIZED) Position(window);
            return result;
        }
        if (message == RepositionMessage)
        {
            Position(window);
            return 0;
        }
        if (message == WM_ERASEBKGND)
        {
            PaintMargins(window, reinterpret_cast<HDC>(wParam));
            return 1;
        }
        if (message == WM_PAINT)
        {
            PAINTSTRUCT paint{};
            const HDC dc = BeginPaint(window, &paint);
            PaintMargins(window, dc);
            EndPaint(window, &paint);
            return 0;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }
}

void VideoPlayerShowHook()
{
    using namespace VideoPlayer;
    auto* videoWindow = *reinterpret_cast<IVideoWindow**>(0xC920E0);
    if (!videoWindow) return;

    OAHWND videoOwner = 0;
    if (FAILED(videoWindow->get_Owner(&videoOwner))) return;
    const HWND window = reinterpret_cast<HWND>(videoOwner);
    if (!IsWindow(window)) return;

    // Cache the source ratio before SetWindowPosition changes the video window.
    if (currentVideo != videoWindow || owner != window)
    {
        long width = 0, height = 0;
        const HRESULT widthResult = videoWindow->get_Width(&width);
        const HRESULT heightResult = videoWindow->get_Height(&height);
        if (FAILED(widthResult) || FAILED(heightResult) || width <= 0 || height <= 0 || width == height)
        {
            width = 4;
            height = 3;
        }
        if (owner) RemoveWindowSubclass(owner, WindowProc, SubclassId);
        sourceWidth = width;
        sourceHeight = height;
        owner = window;
        currentVideo = videoWindow;
        videoRect = {};
    }
    SetWindowSubclass(window, WindowProc, SubclassId, 0);
    Position(window);
    // Paint immediately as well as on subsequent expose/resize messages.
    UpdateWindow(window);
    videoWindow->put_MessageDrain(videoOwner);
    SetFocus(window);
}

class Frontend2
{
public:
    Frontend2()
    {
        WFP::onInitEvent() += []()
        {
            // Intro videos play before game initialization.
            injector::MakeJMP(0x7466D0, VideoPlayerShowHook);
            static auto releaseVideoHook = safetyhook::create_mid(0x746740, [](SafetyHookContext&)
            {
                if (VideoPlayer::owner)
                    RemoveWindowSubclass(VideoPlayer::owner, VideoPlayer::WindowProc, VideoPlayer::SubclassId);
                VideoPlayer::owner = nullptr;
                VideoPlayer::currentVideo = nullptr;
            });
            FixSettings().OnChange("MAIN.FMVWidescreenMode", []()
            {
                if (auto window = VideoPlayer::owner)
                    PostMessageW(window, VideoPlayer::RepositionMessage, 0, 0);
            });
        };

    }
} Frontend2;
