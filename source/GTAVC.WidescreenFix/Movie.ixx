module;

#include <stdafx.h>
#include <dshow.h>

export module Movie;

// Install before the startup movies, which run before the frontend initializes.
namespace Movie
{
    using GetClientRectFn = BOOL(WINAPI*)(HWND, LPRECT);
    GetClientRectFn* GetClientRectImport = nullptr;
    IUnknown** VideoWindow = nullptr;

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

    BOOL WINAPI GetMovieRect(HWND window, LPRECT rect)
    {
        const BOOL result = (*GetClientRectImport)(window, rect);
        if (!result) return result;

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

        LONG movieWidth = width;
        LONG movieHeight = height;
        if (int64_t(width) * aspectHeight > int64_t(height) * aspectWidth)
            movieWidth = std::max<LONG>(1, static_cast<LONG>(int64_t(height) * aspectWidth / aspectHeight));
        else
            movieHeight = std::max<LONG>(1, static_cast<LONG>(int64_t(width) * aspectHeight / aspectWidth));

        rect->left += (width - movieWidth) / 2;
        rect->top += (height - movieHeight) / 2;
        // Native code passes these fields as width/height to DirectShow's
        // SetWindowPosition, rather than as the right/bottom rectangle edges.
        rect->right = movieWidth;
        rect->bottom = movieHeight;
        return result;
    }

    export void Init(std::initializer_list<const char*> signatures)
    {
        std::vector<uintptr_t> calls;
        IUnknown** videoWindow = nullptr;
        for (auto signature : signatures)
        {
            auto pattern = hook::pattern(signature);
            if (pattern.size() != 1) return;
            const auto call = reinterpret_cast<uintptr_t>(pattern.get_first());
            auto window = hook::pattern(call + 6, call + 64, "A1 ? ? ? ? 8B ? ? 50 FF ? 9C 00 00 00");
            if (window.size() != 1) return;
            auto address = *window.get_first<IUnknown**>(1);
            if (!address || (videoWindow && videoWindow != address)) return;
            videoWindow = address;
            calls.push_back(call);
        }
        if (calls.empty()) return;

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
    }
}
