module;

#include <stdafx.h>

export module Movie;

// Install before the startup movies, which run before the frontend initializes.
namespace Movie
{
    using GetClientRectFn = BOOL(WINAPI*)(HWND, LPRECT);
    GetClientRectFn* GetClientRectImport = nullptr;

    BOOL WINAPI GetMovieRect(HWND window, LPRECT rect)
    {
        const BOOL result = (*GetClientRectImport)(window, rect);
        if (!result) return result;

        const LONG width = rect->right - rect->left;
        const LONG height = rect->bottom - rect->top;
        if (width <= 0 || height <= 0) return result;

        // Clear the entire client area before shrinking the movie child window.
        // Use its actual dimensions, independently of HUD/menu constraints.
        if (auto dc = GetDC(window))
        {
            PatBlt(dc, rect->left, rect->top, width, height, BLACKNESS);
            ReleaseDC(window, dc);
        }

        LONG movieWidth = width;
        LONG movieHeight = height;
        if (int64_t(width) * 3 > int64_t(height) * 4)
            movieWidth = std::max<LONG>(1, static_cast<LONG>(int64_t(height) * 4 / 3));
        else
            movieHeight = std::max<LONG>(1, static_cast<LONG>(int64_t(width) * 3 / 4));

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
        for (auto signature : signatures)
        {
            auto pattern = hook::pattern(signature);
            if (pattern.size() != 1) return;
            calls.push_back(reinterpret_cast<uintptr_t>(pattern.get_first()));
        }
        if (calls.empty()) return;

        // Only replace the movie-specific calls. Preserve the original IAT
        // entry so other window-management hooks still see GetClientRect.
        auto importAddress = *reinterpret_cast<GetClientRectFn**>(calls.front() + 2);
        if (!importAddress || !*importAddress) return;
        for (auto call : calls)
            if (*reinterpret_cast<GetClientRectFn**>(call + 2) != importAddress) return;
        GetClientRectImport = importAddress;
        for (auto call : calls)
        {
            injector::MakeCALL(call, GetMovieRect, true);
            injector::MakeNOP(call + 5, 1, true); // six-byte indirect call
        }
    }
}
