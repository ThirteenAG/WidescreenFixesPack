module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SplinterCell/ViewportMouse.hpp"

export module Settings;

import ComVars;
import Gamepad;

namespace
{
    SafetyHookInline configStringHook, setConfigStringHook, saveConfigHook, appInitHook;
    void** globalConfig = nullptr;
    thread_local bool savingConfig = false;

    const wchar_t* ConfigFileName(const wchar_t* filename)
    {
        const std::wstring_view path{ filename ? filename : L"" };
        return path.substr(path.find_last_of(L"\\/") + 1).data();
    }

    bool IsUserConfig(const wchar_t* name)
    {
        return !_wcsicmp(name, L"User") || !_wcsicmp(name, L"User.ini") || !_wcsicmp(name, L"SplinterCellUser.ini");
    }

    bool IsSystemConfig(const wchar_t* name)
    {
        return !*name || !_wcsicmp(name, L"System") || !_wcsicmp(name, L"System.ini") || !_wcsicmp(name, L"SplinterCell.ini");
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
        if (system && !_wcsicmp(section, L"D3DDrv.D3DRenderDevice"))
        {
            if (!_wcsicmp(key, L"FullScreenVideo"))
                return L"True";
            if (!_wcsicmp(key, L"ForceShadowMode"))
                return FixSettings().Bool("MAIN.ForceShadowBufferMode") ? L"1" : L"0";
            if (!_wcsicmp(key, L"EmulateGF2Mode") && FixSettings().Bool("MAIN.ForceShadowBufferMode"))
                return L"0";
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
        if (section && key && value && size > 0)
        {
            const auto* name = ConfigFileName(filename);
            const auto set = [&](const wchar_t* answer)
            {
                wcsncpy_s(value, static_cast<size_t>(size), answer, _TRUNCATE);
                return 1;
            };

            if (const auto answer = ConfigOverride(section, key, name))
                return set(answer->c_str());
            if (!_wcsicmp(section, L"Engine.Input") && IsUserConfig(name))
            {
                const auto result = readNative();
                if (result && *value)
                    return result;

                constexpr const wchar_t* padKeys[] =
                {
                    L"Joy1", L"Joy2", L"Joy3", L"Joy4", L"Joy5", L"Joy6", L"Joy7", L"Joy8",
                    L"Joy9", L"Joy10", L"Joy11", L"Joy12", L"Joy13", L"Joy14", L"Joy15", L"Joy16",
                    L"JoyX", L"JoyY", L"JoyZ", L"JoyV"
                };
                for (const auto* padKey : padKeys)
                {
                    if (!_wcsicmp(key, padKey))
                    {
                        auto id = std::string("INPUT.");
                        for (const auto c : std::wstring_view(padKey))
                            id += static_cast<char>(c);
                        const auto binding = FixSettings().Get(id);
                        const int bytes = static_cast<int>(binding.size());
                        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, binding.data(), bytes, nullptr, 0);
                        if (count > 0)
                        {
                            std::wstring text(count, L'\0');
                            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, binding.data(), bytes, text.data(), count);
                            return set(text.c_str());
                        }
                        break;
                    }
                }
                return result;
            }
        }
        return readNative();
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

    void InstallConfigHooks()
    {
        if (!configStringHook)
        {
            constexpr size_t getStringSlot = 4;
            constexpr size_t setStringSlot = 12;
            const auto table = *reinterpret_cast<void***>(*globalConfig);
            configStringHook = safetyhook::create_inline(table[getStringSlot], GetConfigString);
            setConfigStringHook = safetyhook::create_inline(table[setStringSlot], SetConfigString);
        }
    }

    using ConfigFactory = void* (__cdecl*)();

    void __cdecl AppInit(const wchar_t* name, const wchar_t* command, void* allocator, void* log, void* error,
        void* feedback, void* files, ConfigFactory config, int requireConfig)
    {
        appInitHook.unsafe_ccall(name, command, allocator, log, error, feedback, files, config, requireConfig);
        InstallConfigHooks();
    }
}

export void InitConfigReads()
{
    const auto core = GetModuleHandleW(L"Core.dll");
    globalConfig = reinterpret_cast<void**>(GetProcAddress(core, "?GConfig@@3PAVFConfigCache@@A"));
    saveConfigHook = safetyhook::create_inline(GetProcAddress(core, "?SaveConfig@UObject@@QAEXKPBG@Z"), SaveConfig);
    if (*globalConfig)
        InstallConfigHooks();
    else
        appInitHook = safetyhook::create_inline(GetProcAddress(core,
            "?appInit@@YAXPBG0PAVFMalloc@@PAVFOutputDevice@@PAVFOutputDeviceError@@PAVFFeedbackContext@@PAVFFileManager@@P6APAVFConfigCache@@XZH@Z"), AppInit);
}
class Settings
{
public:
    Settings()
    {
        WFP::onInitEvent() += []()
        {
            Screen.Width = FixSettings().Int("MAIN.ResX");
            Screen.Height = FixSettings().Int("MAIN.ResY");
            Screen.nShadowBufferResolution = FixSettings().Int("MAIN.ShadowBufferResolution");
            Screen.bRawInputMouse = FixSettings().Bool("MAIN.RawInputMouse");
            ViewportMouse::ReadSensitivity();
            FixSettings().OnChange("MAIN.MouseSensitivity", ViewportMouse::ReadSensitivity);
            Screen.bRawInputMouseRawData = FixSettings().Bool("MAIN.RawInputMouseRawData");
            ViewportMouse::rawData.store(Screen.bRawInputMouseRawData);
            Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            Screen.fGrainScale = std::clamp(FixSettings().Float("MAIN.GrainScale"), 0.0f, 1.0f);
            bSkipIntro = FixSettings().Bool("MAIN.SkipIntro");
            bSkipPressStartToContinue = FixSettings().Bool("MAIN.SkipPressStart");
            
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);

            if (IsEnhanced())
            {
                bSkipIntro = false;
            }

            const auto exePath = GetExeModulePath();
            mINI::INIStructure userIni;
            if (!mINI::INIFile(exePath / "SplinterCellUser.ini").read(userIni))
                mINI::INIFile(exePath / "DefUser.ini").read(userIni);
            const auto& input = userIni["Engine.Input"];
            constexpr const char* axes[] = { "JoyX", "JoyY", "JoyZ", "JoyV" };
            for (int button = 1; button <= 20; ++button)
            {
                const auto key = button <= 16 ? "Joy" + std::to_string(button) : std::string(axes[button - 17]);
                const auto binding = input.get(key);
                Gamepad::SetBinding(button, binding.empty() ? FixSettings().String("INPUT." + key) : binding);
            }
            std::vector<std::string> list;
            GetResolutionsList(list);

            // Add current resolution to list
            auto str = std::format("{}x{}", Screen.Width, Screen.Height);
            if (std::find(list.begin(), list.end(), str) == list.end())
            {
                list.push_back(str);
            }

            // Sort list by resolution size (descending)
            std::sort(list.begin(), list.end(), [](const std::string& lhs, const std::string& rhs)
            {
                int32_t x1, y1, x2, y2;
                sscanf_s(lhs.c_str(), "%dx%d", &x1, &y1);
                sscanf_s(rhs.c_str(), "%dx%d", &x2, &y2);
                return (x1 * y1) > (x2 * y2);
            });

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
            bool currentResInList = std::any_of(ResList.begin(), ResList.end(), [&strWide](const auto& pair)
            {
                return pair.second == strWide;
            });

            if (!currentResInList && !ResList.empty())
            {
                auto it = std::find_if(ResList.begin(), ResList.end(), [&strWide](const auto& pair)
                {
                    return pair.first == strWide;
                });
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
