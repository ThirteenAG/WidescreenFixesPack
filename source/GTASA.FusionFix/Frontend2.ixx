module;

#include <stdafx.h>
#include "common.h"
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
    static std::atomic<HWND> owner = nullptr;
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
        // Videos are never cut off: the whole movie at its own aspect, fitted into the window with black bars
        const double scale = std::min(double(client.right) / sourceWidth, double(client.bottom) / sourceHeight);
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
        // The movie's own size in pixels: the video window's size still includes its former frame, and the
        // stock movies carry a pixel aspect that would make their 4:3 picture narrower
        long width = 0, height = 0;
        IBasicVideo* basicVideo = nullptr;
        if (SUCCEEDED(videoWindow->QueryInterface(__uuidof(IBasicVideo), reinterpret_cast<void**>(&basicVideo))))
        {
            if (FAILED(basicVideo->GetVideoSize(&width, &height)))
                width = height = 0;
            basicVideo->Release();
        }
        if (width <= 0 || height <= 0)
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
        };

    }
} Frontend2;
