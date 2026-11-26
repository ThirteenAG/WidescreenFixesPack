#pragma once

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>
#include "iathook.hpp"

namespace NFSUserFiles
{
    template<int FolderId>
    class Redirect
    {
        static inline const std::filesystem::path* CustomPath = nullptr;
        static inline decltype(&SHGetFolderPathA) Original = nullptr;

        static HRESULT WINAPI FolderPath(HWND hwnd, int csidl, HANDLE token, DWORD flags, LPSTR output)
        {
            if (!CustomPath || (csidl & ~CSIDL_FLAG_CREATE) != FolderId)
            {
                const auto native = Original ? Original : &SHGetFolderPathA;
                return native(hwnd, csidl, token, flags, output);
            }

            if (!output)
                return E_INVALIDARG;
            output[0] = '\0';
            char path[MAX_PATH];
            BOOL usedDefaultChar = FALSE;
            const bool utf8 = GetACP() == CP_UTF8;
            const int length = WideCharToMultiByte(CP_ACP, utf8 ? 0 : WC_NO_BEST_FIT_CHARS,
                CustomPath->c_str(), -1, path, MAX_PATH, nullptr, utf8 ? nullptr : &usedDefaultChar);
            if (!length)
                return HRESULT_FROM_WIN32(GetLastError());
            if (usedDefaultChar)
                return HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);

            std::error_code error;
            std::filesystem::create_directories(*CustomPath, error);
            if (error)
                return HRESULT_FROM_WIN32(error.value());

            std::memcpy(output, path, length);
            return S_OK;
        }

    public:
        static void Install(const std::filesystem::path& path)
        {
            CustomPath = &path;
            auto hooks = IATHook::Replace(GetModuleHandleW(nullptr), "SHFOLDER.DLL",
                std::forward_as_tuple("SHGetFolderPathA", FolderPath));
            Original = reinterpret_cast<decltype(Original)>(hooks.at("SHGetFolderPathA").get());
            if (!Original)
                std::abort();
        }

        static HRESULT Get(int csidl, LPSTR path)
        {
            return FolderPath(nullptr, csidl, nullptr, 0, path);
        }
    };
}
