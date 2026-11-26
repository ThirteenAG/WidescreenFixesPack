module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SplinterCell/ViewportMouse.hpp"
export module Settings;

import ComVars;
import Gamepad;

namespace
{
    SafetyHookInline configStringHook, setConfigStringHook, saveConfigHook;
    thread_local bool savingConfig = false;

    const wchar_t* ConfigFileName(const wchar_t* filename)
    {
        const std::wstring_view path{ filename ? filename : L"" };
        return path.substr(path.find_last_of(L"\\/") + 1).data();
    }

    bool IsUserConfig(const wchar_t* name)
    {
        return !_wcsicmp(name, L"User") || !_wcsicmp(name, L"User.ini") || !_wcsicmp(name, L"SplinterCell2User.ini");
    }

    bool IsSystemConfig(const wchar_t* name)
    {
        return !*name || !_wcsicmp(name, L"System") || !_wcsicmp(name, L"System.ini") || !_wcsicmp(name, L"SplinterCell2.ini");
    }

    std::optional<std::wstring> ConfigOverride(const wchar_t* section, const wchar_t* key, const wchar_t* name)
    {
        const bool system = IsSystemConfig(name);
        if (system && !_wcsicmp(section, L"WinDrv.WindowsClient"))
        {
            if (!RequestedNativeResolution)
            {
                if (!_wcsicmp(key, L"WindowedViewportX"))
                    return std::to_wstring(Screen.Width);
                if (!_wcsicmp(key, L"WindowedViewportY"))
                    return std::to_wstring(Screen.Height);
            }
            if (!_wcsicmp(key, L"UseJoystick"))
                return L"True";
        }
        if (system && !_wcsicmp(section, L"Engine.GameInfo") && !_wcsicmp(key, L"UseRumble"))
            return L"True";
        if (!RequestedNativeResolution && !_wcsicmp(section, L"Engine.EPCGameOptions") && !_wcsicmp(key, L"Resolution") && IsUserConfig(name))
            return std::format(L"{}x{}", Screen.Width, Screen.Height);
        return std::nullopt;
    }

    int __fastcall GetConfigString(void* config, void* edx, const wchar_t* section, const wchar_t* key, wchar_t* value, int size, const wchar_t* filename)
    {
        const auto readNative = [&]()
        {
            return configStringHook.unsafe_fastcall<int>(config, edx, section, key, value, size, filename);
        };
        if (!section || !key || !value || size <= 0)
            return readNative();

        const auto set = [&](const wchar_t* answer)
        {
            wcsncpy_s(value, static_cast<size_t>(size), answer, _TRUNCATE);
            return 1;
        };
        const auto* name = ConfigFileName(filename);
        if (const auto answer = ConfigOverride(section, key, name))
            return set(answer->c_str());
        if (_wcsicmp(section, L"Engine.Input") || !IsUserConfig(name))
            return readNative();

        const auto result = readNative();
        struct Alias
        {
            const wchar_t* key;
            const char* id;
            const wchar_t* original;
        };
        constexpr Alias aliases[] =
        {
            { L"Aliases[14]", "INPUT.Aliases14", L"(Command=\"ShowNavPoints\",Alias=\"ShowNavPoints\")" },
            { L"Aliases[15]", "INPUT.Aliases15", L"(Command=\"Health\",Alias=\"Health\")" },
            { L"Aliases[33]", "INPUT.Aliases33", L"(Command=\"Button SmoothPathToggle\",Alias=\"STle\")" },
            { L"Aliases[33]", "INPUT.Aliases33", L"(Command=\"\",Alias=\"None\")" }
        };
        for (const auto& [alias, id, original] : aliases)
        {
            if (!_wcsicmp(key, alias) && (!result || !*value || !_wcsicmp(value, original)))
            {
                const auto binding = FixSettings().Get(id);
                if (!binding.empty())
                    return set(std::wstring(binding.begin(), binding.end()).c_str());
            }
        }

        if (result && *value)
            return result;

        constexpr const wchar_t* padKeys[] =
        {
            L"Joy1", L"Joy2", L"Joy3", L"Joy4", L"Joy5", L"Joy6", L"Joy7", L"Joy8",
            L"Joy9", L"Joy10", L"Joy11", L"Joy12", L"Joy13", L"Joy14", L"Joy15", L"Joy16",
            L"JoyX", L"JoyY", L"JoyV", L"JoyZ", L"JoyR", L"JoyU",
            L"AnalogUp", L"AnalogDown", L"AnalogLeft", L"AnalogRight"
        };
        for (const auto* padKey : padKeys)
        {
            if (!_wcsicmp(key, padKey))
            {
                auto id = std::string("INPUT.");
                for (const auto c : std::wstring_view(padKey))
                    id += static_cast<char>(c);
                const auto binding = FixSettings().Get(id);
                if (!binding.empty())
                    return set(std::wstring(binding.begin(), binding.end()).c_str());
                break;
            }
        }
        return result;
    }

    void __fastcall SetConfigString(void* config, void* edx, const wchar_t* section, const wchar_t* key, const wchar_t* value, const wchar_t* filename)
    {
        // SaveConfig serializes runtime properties, including values supplied by the read hook.
        if (savingConfig && section && key && value)
        {
            const auto* name = ConfigFileName(filename);
            // ResizeViewport saves the applied size before the renderer publishes it.
            if (!RequestedNativeResolution && IsSystemConfig(name) && !_wcsicmp(section, L"WinDrv.WindowsClient") &&
                (!_wcsicmp(key, L"WindowedViewportX") || !_wcsicmp(key, L"WindowedViewportY")))
                return;
            const auto answer = ConfigOverride(section, key, name);
            if (answer && !_wcsicmp(value, answer->c_str()))
                return;
        }
        setConfigStringHook.unsafe_fastcall(config, edx, section, key, value, filename);
    }

    void __fastcall SaveConfig(void* object, void* edx, unsigned flags, const wchar_t* filename)
    {
        struct Restore
        {
            bool previous;
            ~Restore() { savingConfig = previous; }
        } restore{ std::exchange(savingConfig, true) };
        saveConfigHook.unsafe_fastcall(object, edx, flags, filename);
    }
}

export void InitConfigReads()
{
    const auto core = GetModuleHandleW(L"Core.dll");
    configStringHook = safetyhook::create_inline(FindProcAddress(core,
        "?GetString@FConfigCacheIni@@UAEHPB_W0PA_WH0@Z",
        "?GetString@FConfigCacheIni@@UAEHPBG0PAGH0@Z"), GetConfigString);
    setConfigStringHook = safetyhook::create_inline(FindProcAddress(core,
        "?SetString@FConfigCacheIni@@UAEXPB_W000@Z",
        "?SetString@FConfigCacheIni@@UAEXPBG000@Z"), SetConfigString);
    saveConfigHook = safetyhook::create_inline(FindProcAddress(core,
        "?SaveConfig@UObject@@QAEXKPB_W@Z",
        "?SaveConfig@UObject@@QAEXKPBG@Z"), SaveConfig);
}
class Settings
{
public:
    Settings()
    {
        WFP::onInitEvent() += []()
        {
            Gamepad::Init();
            auto& settings = FixSettings();
            asi_settings::LiveLink::Start();
            Screen.Width = settings.Int("MAIN.ResX");
            Screen.Height = settings.Int("MAIN.ResY");
            Screen.bRawInputMouse = settings.Int("MAIN.RawInputMouse") != 0;
            ViewportMouse::ReadSensitivity();
            settings.OnChange("MAIN.MouseSensitivity", ViewportMouse::ReadSensitivity);
            Screen.bRawInputMouseRawData = settings.Int("MAIN.RawInputMouseRawData") != 0;
            ViewportMouse::rawData.store(Screen.bRawInputMouseRawData);
            Screen.nFMVWidescreenMode = settings.Int("MAIN.FMVWidescreenMode");
            Screen.bScopeWidescreenMode = settings.Int("MAIN.ScopeWidescreenMode") != 0;
            Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(settings.Get("MAIN.HudAspectRatioConstraint"));
            Screen.nPostProcessFixedScale = settings.Int("MAIN.PostProcessFixedScale");
            Screen.fGrainScale = std::clamp(settings.Float("MAIN.GrainScale"), 0.0f, 1.0f);
            Screen.nShadowMapResolution = settings.Int("MAIN.ShadowMapResolution");
            Screen.nReflectionsResolution = settings.Int("MAIN.ReflectionsResolution");
            Screen.nBloomResolutionMultiplier = std::clamp(settings.Int("MAIN.BloomResolutionMultiplier"), 0, 4);
            fLightDistanceMultiplier = std::clamp(settings.Float("MAIN.LightDistanceMultiplier"), 1.0f, 8.0f);
            nSuitLightColor = settings.Int("BONUS.GogglesLightColor");
            bSkipIntro = settings.Int("MAIN.SkipIntro") != 0;
            bSkipPressStartToContinue = settings.Int("MAIN.SkipPressStart") != 0;
            bEnableConsole = settings.Int("MAIN.EnableConsole") != 0;
            bEnableCheckpoints = settings.Int("MAIN.EnableCheckpoints") != 0;

            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

            // 1 follows the screen width, rounded up to a whole multiple of 1024: the vision effects shrink this target
            // into fixed 256 and 1024 buffers, and only whole ratios keep every sample on the same texel spacing
            if (Screen.nPostProcessFixedScale == 1)
                Screen.nPostProcessFixedScale = (static_cast<uint32_t>(Screen.Width) + 1023) / 1024 * 1024;

            std::vector<std::string> list;
            GetResolutionsList(list);

            // Add current resolution to list
            auto str = std::format("{}x{}", Screen.Width, Screen.Height);
            if (std::find(list.begin(), list.end(), str) == list.end())
            {
                list.push_back(str);
            }

            NativeDisplayModes = { { 640, 480 }, { 800, 600 } };
            for (const auto& resolution : list)
            {
                int width = 0, height = 0;
                if (sscanf_s(resolution.c_str(), "%dx%d", &width, &height) == 2 &&
                    width >= 640 && height >= 480 && width <= 16384 && height <= 16384)
                    NativeDisplayModes.emplace_back(width, height);
            }
            std::sort(NativeDisplayModes.begin(), NativeDisplayModes.end());
            NativeDisplayModes.erase(std::unique(NativeDisplayModes.begin(), NativeDisplayModes.end()), NativeDisplayModes.end());

            // Sort list by resolution size (descending)
            std::sort(list.begin(), list.end(), [](const std::string& lhs, const std::string& rhs)
            {
                int32_t x1, y1, x2, y2;
                sscanf_s(lhs.c_str(), "%dx%d", &x1, &y1);
                sscanf_s(rhs.c_str(), "%dx%d", &x2, &y2);
                return (x1 * y1) > (x2 * y2);
            });

            // Group resolutions by string length
            std::map<size_t, std::vector<std::string>> lengthToRes;
            for (const auto& res : list)
            {
                lengthToRes[res.length()].push_back(res);
            }

            // Assign to ResList slots in reverse order (largest slots first)
            for (auto it = ResList.rbegin(); it != ResList.rend(); ++it)
            {
                size_t targetLength = it->first.length();
                auto& candidates = lengthToRes[targetLength];
                if (!candidates.empty())
                {
                    // Assign the largest available resolution for this length
                    it->second = std::wstring(candidates.front().begin(), candidates.front().end());
                    candidates.erase(candidates.begin());  // Remove to prevent reuse
                }
                else
                {
                    // No matching length available, use the template resolution
                    it->second = it->first;
                }
            }

            // Ensure current resolution is in the list
            auto strWide = std::wstring(str.begin(), str.end());
            bool currentResInList = std::any_of(ResList.begin(), ResList.end(), [&strWide](const auto& pair) { return pair.second == strWide; });

            if (!currentResInList && !ResList.empty())
            {
                auto it = std::find_if(ResList.begin(), ResList.end(), [&strWide](const auto& pair) { return pair.first == strWide; });
                if (it != ResList.end())
                {
                    it->second = strWide;
                }
                else
                {
                    // Find a slot with matching string length and replace it
                    for (auto& pair : ResList)
                    {
                        if (pair.second.length() == strWide.length() && pair.second != strWide)
                        {
                            pair.second = strWide;
                            break;
                        }
                    }
                }
            }

            // Keep 640x480 and 800x600
            ResList[0].second = L"640x480";
            ResList[1].second = L"800x600";

            #ifdef _DEBUG
            // Log the assignments
            for (size_t i = 0; i < ResList.size(); ++i)
            {
                spd::log()->info("{0} : {1}", std::string(ResList[i].first.begin(), ResList[i].first.end()), std::string(ResList[i].second.begin(), ResList[i].second.end()));
            }
            #endif
        };
    }
} Settings;
