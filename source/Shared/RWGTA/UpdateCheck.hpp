#pragma once
#include <windows.h>
#include <winhttp.h>
#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <ctime>
#include "Shared/AsiSettings/asi_settings.hpp"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "version.lib")

// MAIN.CheckForUpdates (off by default). FusionFixHub is the only updater; the fix only tells the player:
// - off: nothing at all, no network request;
// - on (at the start, or when turned on while playing): one request to https://fusionfix.io/api/versions on a thread
//   of its own; when the fix's release there is newer than this build, a notice once the game is playable, pointing
//   to FusionFixHub when it's in the game's folder, else to the fix's game page on fusionfix.io ({page} in the text);
// - with the hub, when the game exits: FusionFixHub.exe --show-update <id> --wait-pid <pid>, which opens on the fix's
//   update page. Never while the game runs; turning the option off cancels it.
// The game shows the notice through Config::show (its help box), on the game's thread, from Tick.
namespace RWGTA::UpdateCheck
{
    struct Config
    {
        std::string pluginId;                               // e.g. "GTA3.FusionFix"
        std::string fileVersion;                            // rsc_FileVersion, for the user agent
        asi_settings::Plugin* settings = nullptr;           // the fix's settings and texts
        std::string settingId = "MAIN.CheckForUpdates";
        std::function<bool()> playable;                     // the notice waits until this is true
        std::function<void(const std::wstring&)> show;      // shows one notice line
        std::function<std::wstring(const std::string& latest, bool hub)> format; // optional: the notice line in the
                                                            // game's own text language (default: the fix's texts)
        std::function<bool(const std::wstring&)> tryShow;   // keeps the notice pending when the native UI rejects it
    };

    namespace detail
    {
        enum class State { Idle, Checking, Available, Shown, ShownWithHub };

        struct CheckResult
        {
            std::atomic<bool> stopping{ false };
            std::atomic<State> state{ State::Idle };
            std::mutex lock;
            std::string latest;
            std::string page = "fusionfix.io";
        };

        struct Data
        {
            Config config;
            std::atomic<std::shared_ptr<CheckResult>> result;
            std::mutex cycleLock;
            bool exiting = false;
            bool installed = false;
        };

        inline Data& Get()
        {
            static Data data;
            return data;
        }

        inline bool Enabled()
        {
            auto& d = Get();
            return d.config.settings && d.config.settings->Get(d.config.settingId) != "0";
        }

        // https://fusionfix.io/api/versions; FUSIONFIX_VERSIONS_URL in the environment points it elsewhere (tests)
        inline std::string Fetch(const std::string& userAgent, const CheckResult& result)
        {
            const auto deadline = GetTickCount64() + 30000;
            std::string body;
            std::wstring url = L"https://fusionfix.io/api/versions";
            wchar_t env[512] = {};
            if (GetEnvironmentVariableW(L"FUSIONFIX_VERSIONS_URL", env, DWORD(std::size(env))) && env[0])
                url = env;
            URL_COMPONENTS parts{ sizeof(parts) };
            wchar_t host[256] = {}, path[1024] = {};
            parts.lpszHostName = host;
            parts.dwHostNameLength = DWORD(std::size(host));
            parts.lpszUrlPath = path;
            parts.dwUrlPathLength = DWORD(std::size(path));
            if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts))
                return body;
            auto ua = asi_settings::text::Widen(userAgent);
            HINTERNET session = WinHttpOpen(ua.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!session)
                return body;
            WinHttpSetTimeouts(session, 10000, 10000, 10000, 15000);
            if (HINTERNET connect = WinHttpConnect(session, host, parts.nPort, 0))
            {
                DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
                if (HINTERNET request = WinHttpOpenRequest(connect, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags))
                {
                    DWORD status = 0, size = sizeof(status);
                    if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, nullptr) &&
                        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) &&
                        status == 200)
                    {
                        char chunk[8192];
                        constexpr size_t maxSize = 1u << 20;
                        bool complete = false;
                        while (!result.stopping && GetTickCount64() < deadline)
                        {
                            DWORD read = 0;
                            if (!WinHttpReadData(request, chunk, sizeof(chunk), &read) || body.size() + read > maxSize)
                                break;
                            if (!read)
                            {
                                complete = true;
                                break;
                            }
                            body.append(chunk, read);
                        }
                        if (!complete)
                            body.clear();
                    }
                    WinHttpCloseHandle(request);
                }
                WinHttpCloseHandle(connect);
            }
            WinHttpCloseHandle(session);
            return body;
        }

        // "2026-10-13T09:40:00Z" as seconds since 1970, -1 when it isn't one
        inline int64_t ParseUtc(std::string_view text)
        {
            int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
            if (sscanf_s(std::string(text).c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &y, &mo, &d, &h, &mi, &s) != 6)
                return -1;
            std::tm tm{};
            tm.tm_year = y - 1900;
            tm.tm_mon = mo - 1;
            tm.tm_mday = d;
            tm.tm_hour = h;
            tm.tm_min = mi;
            tm.tm_sec = s;
            return int64_t(_mkgmtime(&tm));
        }

        inline int64_t FileTime(const wchar_t* path)
        {
            WIN32_FILE_ATTRIBUTE_DATA data{};
            if (!GetFileAttributesExW(path, GetFileExInfoStandard, &data))
                return -1;
            ULARGE_INTEGER t{ data.ftLastWriteTime.dwLowDateTime, data.ftLastWriteTime.dwHighDateTime };
            return int64_t((t.QuadPart - 116444736000000000ull) / 10000000ull);
        }

        inline std::filesystem::path ModulePath()
        {
            HMODULE self = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Get), &self);
            wchar_t path[MAX_PATH * 4] = {};
            GetModuleFileNameW(self, path, DWORD(std::size(path)));
            return path;
        }

        // BuildTimeUtc of the plugin's version resource; a build without it counts from its file's time
        inline int64_t BuildTime()
        {
            auto path = ModulePath();
            int64_t fromResource = -1;
            DWORD handle = 0, size = GetFileVersionInfoSizeW(path.c_str(), &handle);
            std::vector<uint8_t> info(size);
            struct Translation { WORD language, codepage; }* translations = nullptr;
            UINT length = 0;
            wchar_t* value = nullptr;
            if (size && GetFileVersionInfoW(path.c_str(), 0, size, info.data()) &&
                VerQueryValueW(info.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void**>(&translations), &length) && length >= sizeof(Translation))
            {
                wchar_t key[64];
                swprintf_s(key, L"\\StringFileInfo\\%04x%04x\\BuildTimeUtc", translations[0].language, translations[0].codepage);
                if (VerQueryValueW(info.data(), key, reinterpret_cast<void**>(&value), &length) && value)
                    fromResource = ParseUtc(asi_settings::text::Narrow(value));
            }
            return fromResource >= 0 ? fromResource : FileTime(path.c_str());
        }

        inline std::filesystem::path GameFolder()
        {
            wchar_t path[MAX_PATH * 4] = {};
            GetModuleFileNameW(nullptr, path, DWORD(std::size(path)));
            return std::filesystem::path(path).parent_path();
        }

        inline bool HubPresent()
        {
            std::error_code ec;
            return std::filesystem::exists(GameFolder() / L"FusionFixHub.exe", ec);
        }

        struct Request
        {
            std::shared_ptr<CheckResult> result;
            std::string pluginId;
            std::string userAgent;
            HMODULE module;
        };

        // A template: as a plain inline function, its body crashes MSVC's Debug compiler in module builds that import
        // a module including this header
        template <class = void>
        void CALLBACK RunCheck(PTP_CALLBACK_INSTANCE callback, void* context)
        {
            std::unique_ptr<Request> request(static_cast<Request*>(context));
            FreeLibraryWhenCallbackReturns(callback, request->module);
            auto& result = *request->result;
            if (result.stopping)
                return;
            try
            {
                // Newer = the release was uploaded more than 30 minutes after this build was made (a release's own
                // build is made just before its upload). The version text is only shown.
                asi_settings::json::Value data;
                std::string version, uploaded, game;
                if (auto body = Fetch(request->userAgent, result); !body.empty() && asi_settings::json::Parse(body, data))
                {
                    if (auto fixes = data.Find("fixes"))
                        if (auto own = fixes->Find(request->pluginId))
                        {
                            version = std::string(own->Str("version"));
                            game = std::string(own->Str("game"));
                            if (auto asset = own->Find("asset"))
                                uploaded = std::string(asset->Str("updatedAt"));
                            if (uploaded.empty())
                                uploaded = std::string(own->Str("date"));
                        }
                }
                int64_t built = BuildTime(), released = ParseUtc(uploaded);
                if (built >= 0 && released >= 0 && released > built + 30 * 60)
                {
                    std::lock_guard guard(result.lock);
                    result.latest = version.empty() ? uploaded.substr(0, 10) : version;
                    if (!game.empty())
                        result.page = "fusionfix.io/games/" + game;
                    result.state = State::Available;
                }
                else
                    result.state = State::Idle;
            }
            catch (...)
            {
                result.state = State::Idle;
            }
        }

        inline void UpdateEnabled()
        {
            auto& d = Get();
            std::lock_guard guard(d.cycleLock);
            if (d.exiting)
                return;
            if (!Enabled())
            {
                if (auto previous = d.result.exchange(nullptr))
                    previous->stopping = true;
                return;
            }
            if (d.result.load())
                return;
            auto result = std::make_shared<CheckResult>();
            result->state = State::Checking;
            d.result.store(result);
            auto request = std::make_unique<Request>(Request{ result, d.config.pluginId, d.config.pluginId + "/" + d.config.fileVersion, nullptr });
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&Get), &request->module))
            {
                auto context = request.release();
                if (TrySubmitThreadpoolCallback(RunCheck<>, context, nullptr))
                    return;
                request.reset(context);
                FreeLibrary(request->module);
            }
            result->state = State::Idle;
        }

        inline std::string Replace(std::string text, std::string_view key, std::string_view value)
        {
            for (size_t at = text.find(key); at != std::string::npos; at = text.find(key, at + value.size()))
                text.replace(at, key.size(), value);
            return text;
        }
    }

    // The fix's game page for the notice without the hub, e.g. "fusionfix.io/games/gta3" (known after the check)
    inline std::string Page()
    {
        auto result = detail::Get().result.load();
        if (!result)
            return "fusionfix.io";
        std::lock_guard guard(result->lock);
        return result->page;
    }

    // {page} in a notice line from Config::format
    inline std::wstring ReplacePage(std::wstring line)
    {
        static const std::wstring key = L"{page}";
        auto page = asi_settings::text::Widen(Page());
        for (size_t at = line.find(key); at != std::wstring::npos; at = line.find(key, at + page.size()))
            line.replace(at, key.size(), page);
        return line;
    }

    // Once, when the fix starts (the settings loaded)
    inline void Init(Config config)
    {
        auto& d = detail::Get();
        if (d.installed)
            return;
        d.installed = true;
        d.config = std::move(config);
        d.config.settings->OnChange(d.config.settingId, []() { detail::UpdateEnabled(); });
        detail::UpdateEnabled();
    }

    // Every game frame, on the game's thread: the notice, once the game is playable
    inline void Tick()
    {
        auto& d = detail::Get();
        // Shown after the game has been playable for a few seconds (not into a loading screen's fade)
        static ULONGLONG playableSince = 0;
        // Nothing to show (update checks off, no answer yet, or up to date): the game state isn't looked at
        const auto result = d.result.load();
        if (!result || result->state != detail::State::Available || result->stopping || (!d.config.show && !d.config.tryShow))
        {
            playableSince = 0;
            return;
        }
        if (d.config.playable && !d.config.playable())
        {
            playableSince = 0;
            return;
        }
        if (!playableSince)
            playableSince = GetTickCount64();
        if (GetTickCount64() - playableSince < 4000)
            return;
        std::string latest;
        {
            std::lock_guard guard(result->lock);
            latest = result->latest;
        }
        bool hub = detail::HubPresent();
        std::wstring message;
        if (d.config.format)
            message = ReplacePage(d.config.format(latest, hub));
        else
        {
            auto& texts = d.config.settings->GetTexts();
            std::string line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
            line = detail::Replace(line, "{name}", texts.Resolve(d.config.settings->GetDescription().name));
            line = detail::Replace(line, "{version}", latest);
            line = detail::Replace(line, "{page}", Page());
            message = asi_settings::text::Widen(line);
        }
        if (d.config.tryShow)
        {
            if (!d.config.tryShow(message))
                return;
        }
        else
            d.config.show(message);
        result->state = hub ? detail::State::ShownWithHub : detail::State::Shown;
    }

    // When the game exits: the hub on the fix's update page, after the game has closed
    inline void OnExit()
    {
        auto& d = detail::Get();
        std::shared_ptr<detail::CheckResult> result;
        {
            std::lock_guard guard(d.cycleLock);
            d.exiting = true;
            result = d.result.exchange(nullptr);
        }
        if (!result)
            return;
        result->stopping = true;
        if (result->state != detail::State::ShownWithHub || !detail::HubPresent())
            return;
        auto hub = detail::GameFolder() / L"FusionFixHub.exe";
        std::wstring command = L"\"" + hub.wstring() + L"\" --show-update " + asi_settings::text::Widen(d.config.pluginId) + L" --wait-pid " + std::to_wstring(GetCurrentProcessId());
        STARTUPINFOW si{ sizeof(si) };
        PROCESS_INFORMATION pi{};
        if (CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, detail::GameFolder().c_str(), &si, &pi))
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }
}
