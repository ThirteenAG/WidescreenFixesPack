module;
#include <stdafx.h>
#include <shlobj.h>
#include "FixSettings.hpp"

export module Storage;


export namespace OnimushaStorage
{
    inline SafetyHookInline PathHook, LanguageHook;
    inline int Language = 0;
    inline std::filesystem::path Save, Fallback;
    inline std::once_flag Destination;

    inline bool Writable(const std::filesystem::path& path, DWORD access, DWORD flags)
    {
        const auto file = CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, flags, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return false;
        CloseHandle(file);
        return true;
    }

    inline void SelectDestination()
    {
        wchar_t module[MAX_PATH]{};
        GetModuleFileNameW(nullptr, module, DWORD(std::size(module)));
        Save = std::filesystem::path(module).parent_path() / L"SAVE";
        std::error_code error;
        const bool exists = std::filesystem::is_directory(Save, error);
        const auto parent = exists ? Save : Save.parent_path();
        const auto file = Save / L"SAVE";
        if (Writable(parent, FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY, FILE_FLAG_BACKUP_SEMANTICS) &&
            (!std::filesystem::exists(file, error) || Writable(file, GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL)))
            return;
        PWSTR savedGames = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &savedGames)))
            return;
        const auto target = std::filesystem::path(savedGames) / L"Onimusha 3" / L"SAVE";
        CoTaskMemFree(savedGames);
        std::filesystem::create_directories(target, error);
        if (error || target.native().size() + 6 >= MAX_PATH)
            return;
        if (std::filesystem::exists(file, error))
        {
            error.clear();
            if (std::filesystem::copy_file(file, target / L"SAVE", std::filesystem::copy_options::skip_existing, error))
            {
                const auto attributes = GetFileAttributesW((target / L"SAVE").c_str());
                if (attributes != INVALID_FILE_ATTRIBUTES)
                    SetFileAttributesW((target / L"SAVE").c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
            }
            if (error)
                return;
        }
        Fallback = target;
    }

    inline LPWSTR __fastcall Path(LPWSTR result, LPCSTR name)
    {
        const auto nativeResult = PathHook.unsafe_fastcall<LPWSTR>(result, name);
        std::call_once(Destination, SelectDestination);
        if (!Fallback.empty())
        {
            const std::wstring original(result), prefix = Save.native();
            if (original.size() >= prefix.size() && _wcsnicmp(original.c_str(), prefix.c_str(), prefix.size()) == 0 &&
                (original.size() == prefix.size() || original[prefix.size()] == L'\\'))
            {
                const auto path = Fallback.native() + original.substr(prefix.size());
                if (path.size() < MAX_PATH)
                {
                    const auto tail = nativeResult - result - ptrdiff_t(original.size());
                    wcscpy_s(result, MAX_PATH, path.c_str());
                    return result + std::clamp(ptrdiff_t(path.size()) + tail, ptrdiff_t(0), ptrdiff_t(path.size()));
                }
            }
        }
        return nativeResult;
    }

    inline int __cdecl GameLanguage()
    {
        static constexpr int native[]{ 0, 5, 4, 2, 1, 3 };
        return Language ? native[Language] : LanguageHook.unsafe_ccall<int>();
    }

    inline void Init()
    {
        Language = std::clamp(FixSettings().Int("MAIN.GameLanguage"), 0, 5);
        LanguageHook = safetyhook::create_inline(hook::pattern("81 EC 04 01 00 00 A1 ? ? ? ? 33 C4 89 84 24 00 01 00 00 6A 03").get_first(), GameLanguage);
        PathHook = safetyhook::create_inline(hook::pattern("81 EC 0C 03 00 00 80 3A 2F 56 57 8B F9 8B F7 75 55 B8").get_first(), Path);
    }
}
