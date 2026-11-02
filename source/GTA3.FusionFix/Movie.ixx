module;

#include <stdafx.h>
#include <dshow.h>
#include <commctrl.h>
#include "FixSettings.hpp"

#pragma comment(lib, "comctl32.lib")

export module Movie;

// Install before the startup movies, which run before the frontend initializes.
namespace Movie
{
    using GetClientRectFn = BOOL(WINAPI*)(HWND, LPRECT);
    GetClientRectFn* GetClientRectImport = nullptr;
    IUnknown** VideoWindow = nullptr;
    HWND Owner = nullptr;                                   // the game's window, which the movies play in
    constexpr UINT_PTR SubclassId = 0x4D4F56;
    constexpr UINT RepositionMessage = WM_APP + 0x57F;      // the movie mode changed while a movie plays

    bool GetVideoAspectRatio(LONG& width, LONG& height)
    {
        if (!VideoWindow || !*VideoWindow) return false;
        // The filter graph exposes these interfaces through IVideoWindow too.
        // Prefer the display aspect ratio, which accounts for MPEG pixel shape.
        IBasicVideo2* video2 = nullptr;
        if (SUCCEEDED((*VideoWindow)->QueryInterface(__uuidof(IBasicVideo2), reinterpret_cast<void**>(&video2))))
        {
            const HRESULT result = video2->GetPreferredAspectRatio(&width, &height);
            video2->Release();
            if (SUCCEEDED(result) && width > 0 && height > 0) return true;
        }
        IBasicVideo* video = nullptr;
        if (SUCCEEDED((*VideoWindow)->QueryInterface(__uuidof(IBasicVideo), reinterpret_cast<void**>(&video))))
        {
            const HRESULT result = video->GetVideoSize(&width, &height);
            video->Release();
            if (SUCCEEDED(result) && width > 0 && height > 0) return true;
        }
        return false;
    }

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);

    BOOL WINAPI GetMovieRect(HWND window, LPRECT rect)
    {
        // Videos are never cut off: the whole movie at its own aspect with black bars, fitted into the game's 4:3
        // picture (Original) or into the whole screen (Fill)
        const BOOL result = (*GetClientRectImport)(window, rect);
        if (!result) return result;

        if (window && window != Owner && SetWindowSubclass(window, WindowProc, SubclassId, 0))
            Owner = window;

        const LONG width = rect->right - rect->left;
        const LONG height = rect->bottom - rect->top;
        if (width <= 0 || height <= 0) return result;

        LONG aspectWidth = 0;
        LONG aspectHeight = 0;
        // Leave an unknown/replacement renderer's presentation unchanged.
        if (!GetVideoAspectRatio(aspectWidth, aspectHeight)) return result;

        // Clear the entire client area before shrinking the movie child window.
        // Use its actual dimensions, independently of HUD/menu constraints.
        if (auto dc = GetDC(window))
        {
            PatBlt(dc, rect->left, rect->top, width, height, BLACKNESS);
            ReleaseDC(window, dc);
        }

        LONG frameWidth = width;
        LONG frameHeight = height;
        static auto& mode = FixSettings().Int("MAIN.FMVWidescreenMode");
        if (mode == 0)
        {
            if (int64_t(frameWidth) * 3 > int64_t(frameHeight) * 4)
                frameWidth = std::max<LONG>(1, static_cast<LONG>(int64_t(frameHeight) * 4 / 3));
            else
                frameHeight = std::max<LONG>(1, static_cast<LONG>(int64_t(frameWidth) * 3 / 4));
        }
        LONG movieWidth = frameWidth;
        LONG movieHeight = frameHeight;
        if (int64_t(frameWidth) * aspectHeight > int64_t(frameHeight) * aspectWidth)
            movieWidth = std::max<LONG>(1, static_cast<LONG>(int64_t(frameHeight) * aspectWidth / aspectHeight));
        else
            movieHeight = std::max<LONG>(1, static_cast<LONG>(int64_t(frameWidth) * aspectHeight / aspectWidth));

        rect->left += (width - movieWidth) / 2;
        rect->top += (height - movieHeight) / 2;
        // Native code passes these fields as width/height to DirectShow's
        // SetWindowPosition, rather than as the right/bottom rectangle edges.
        rect->right = movieWidth;
        rect->bottom = movieHeight;
        return result;
    }

    // A changed movie mode places the playing movie again, on the window's own thread
    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, WindowProc, SubclassId);
            if (Owner == window)
                Owner = nullptr;
        }
        else if (message == RepositionMessage)
        {
            auto video = VideoWindow ? static_cast<IVideoWindow*>(static_cast<void*>(*VideoWindow)) : nullptr;
            RECT rect{};
            if (video && GetMovieRect(window, &rect))
                video->SetWindowPosition(rect.left, rect.top, rect.right, rect.bottom);
            return 0;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    export void Init(std::initializer_list<const char*> signatures)
    {
        std::vector<uintptr_t> calls;
        IUnknown** videoWindow = nullptr;
        for (auto signature : signatures)
        {
            auto pattern = hook::pattern(signature);
            const auto call = reinterpret_cast<uintptr_t>(pattern.get_first());
            auto window = hook::pattern(call + 6, call + 64, "A1 ? ? ? ? 8B ? ? 50 FF ? 9C 00 00 00");
            auto address = *window.get_first<IUnknown**>(1);
            videoWindow = address;
            calls.push_back(call);
        }

        // Only replace the movie-specific calls. Preserve the original IAT
        // entry so other window-management hooks still see GetClientRect.
        auto importAddress = *reinterpret_cast<GetClientRectFn**>(calls.front() + 2);
        if (!importAddress || !*importAddress) return;
        for (auto call : calls)
            if (*reinterpret_cast<GetClientRectFn**>(call + 2) != importAddress) return;
        GetClientRectImport = importAddress;
        VideoWindow = videoWindow;
        for (auto call : calls)
        {
            injector::MakeCALL(call, GetMovieRect, true);
            injector::MakeNOP(call + 5, 1, true); // six-byte indirect call
        }
        FixSettings().OnChange("MAIN.FMVWidescreenMode", []()
        {
            if (auto window = Owner)
                PostMessageW(window, RepositionMessage, 0, 0);
        });
    }
}
