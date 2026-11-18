module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Settings;

import ComVars;

// The game reads its ini files through the config cache (GetString, slot 5; the bool, int and float readers call it too).
// The fix answers those reads instead of writing the game's files: resolution and startup mode always, the optional
// graphics values when enabled, and controller bindings only where the player's user ini has none.
namespace ConfigReads
{
    struct Answer
    {
        std::wstring value;
        bool onlyMissing = false;
    };

    std::once_flag answersBuilt;
    std::map<std::wstring, Answer> answers;

    std::wstring Lower(std::wstring_view text)
    {
        std::wstring out(text);
        std::transform(out.begin(), out.end(), out.begin(), [](wchar_t c) { return wchar_t(towlower(c)); });
        return out;
    }

    std::wstring Wide(std::string_view text)
    {
        return std::wstring(text.begin(), text.end());
    }

    void Add(std::wstring_view section, std::wstring_view key, std::wstring value, bool onlyMissing = false)
    {
        answers[Lower(section) + L"|" + Lower(key)] = { std::move(value), onlyMissing };
    }

    void BuildAnswers()
    {
        auto width = FixSettings().Int("MAIN.ResX");
        auto height = FixSettings().Int("MAIN.ResY");
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
            std::tie(width, height) = GetDesktopRes();
        if (width <= 0 || height <= 0)
            std::tie(width, height) = std::make_tuple(640, 480);

        for (auto key : { L"WindowedViewportX", L"FullscreenViewportX" })
            Add(L"WinDrv.WindowsClient", key, std::to_wstring(width));
        for (auto key : { L"WindowedViewportY", L"FullscreenViewportY" })
            Add(L"WinDrv.WindowsClient", key, std::to_wstring(height));
        Add(L"WinDrv.WindowsClient", L"StartupFullscreen", FixSettings().Int("MAIN.WindowMode") == 0 ? L"True" : L"False");
        // A DirectInput mouse adds its own movement on top of raw input, past the raw sensitivity
        if (FixSettings().Bool("MAIN.RawInputMouse"))
            Add(L"Init", L"UseDIMouse", L"False");

        if (FixSettings().Bool("NATIVE.Enabled"))
        {
            const std::tuple<const char*, const wchar_t*, const wchar_t*> graphics[] =
            {
                { "NATIVE_WINDOW.TextureDetail", L"WinDrv.WindowsClient", L"TextureDetail" },
                { "NATIVE_GENERAL.DisableHDRTonemapping", L"GENERAL", L"DisableHDRTonemapping" },
                { "NATIVE_GENERAL.LowResTex", L"GENERAL", L"LowResTex" },
                { "NATIVE_GENERAL.SimpleShader", L"GENERAL", L"SimpleShader" },
                { "NATIVE_GENERAL.SimpleEffect", L"GENERAL", L"SimpleEffect" },
                { "NATIVE_GENERAL.HidePieces", L"GENERAL", L"HidePieces" },
                { "NATIVE_GRAPHICS.bEnableHDR", L"Graphics", L"bEnableHDR" },
                { "NATIVE_GRAPHICS.bSpecularLighting", L"Graphics", L"bSpecularLighting" },
                { "NATIVE_GRAPHICS.bHighQualityVisualFX", L"Graphics", L"bHighQualityVisualFX" },
                { "NATIVE_D3D.UseHWShadowMapping", L"D3DDrv.D3DRenderDevice", L"UseHWShadowMapping" },
                { "NATIVE_D3D.ShadowResolutionDegree", L"D3DDrv.D3DRenderDevice", L"ShadowResolutionDegree" },
                { "NATIVE_D3D.ShadowTurnOffDegree", L"D3DDrv.D3DRenderDevice", L"ShadowTurnOffDegree" },
                { "NATIVE_D3D.bUseSmoothShadow", L"D3DDrv.D3DRenderDevice", L"bUseSmoothShadow" },
                { "NATIVE_D3D.FSAADegree", L"D3DDrv.D3DRenderDevice", L"FSAADegree" },
            };
            for (const auto& [id, section, key] : graphics)
            {
                if (const auto& value = FixSettings().String(id); !value.empty())
                    Add(section, key, Wide(value));
            }
        }

        for (auto key : { "Joy1", "Joy2", "Joy3", "Joy4", "Joy5", "Joy6", "Joy7", "Joy8", "Joy9", "Joy10", "Joy11", "Joy12", "Joy13", "Joy14", "Joy15", "Joy16", "JoyX", "JoyY", "JoyZ", "JoyV", "JoyR", "JoyU", "AnalogUp", "AnalogDown", "AnalogLeft", "AnalogRight", "MouseWheelUp", "MouseWheelDown", "Tilde" })
        {
            if (const auto& value = FixSettings().String(std::string("INPUT.") + key); !value.empty())
                Add(L"Engine.Input", Wide(key), Wide(value), true);
        }
    }

    const Answer* Find(const wchar_t* section, const wchar_t* key, const wchar_t* filename)
    {
        if (!section || !key)
            return nullptr;
        // Localization and other reads are frequent; leave them before any allocation
        constexpr const wchar_t* sections[] = { L"WinDrv.WindowsClient", L"Engine.Input", L"GENERAL", L"Graphics", L"D3DDrv.D3DRenderDevice", L"Init" };
        if (std::none_of(std::begin(sections), std::end(sections), [section](const wchar_t* name) { return !_wcsicmp(section, name); }))
            return nullptr;
        // Default*.ini and DefUser.ini hold the values the game's menus reset to
        if (filename)
        {
            const auto name = Lower(filename);
            if (name.find(L"default") != std::wstring::npos || name.find(L"defuser") != std::wstring::npos)
                return nullptr;
        }
        std::call_once(answersBuilt, BuildAnswers);
        const auto found = answers.find(Lower(section) + L"|" + Lower(key));
        return found != answers.end() ? &found->second : nullptr;
    }

    void Copy(const Answer& answer, wchar_t* value, int size)
    {
        if (value && size > 0)
            wcsncpy_s(value, size_t(size), answer.value.c_str(), _TRUNCATE);
    }

    template<size_t Slot>
    struct Reader
    {
        static inline SafetyHookInline hook = {};

        static int __fastcall GetString(void* config, void* edx, const wchar_t* section, const wchar_t* key, wchar_t* value, int size, const wchar_t* filename, int extra)
        {
            const auto answer = Find(section, key, filename);
            if (answer && !answer->onlyMissing)
            {
                Copy(*answer, value, size);
                return 1;
            }
            const auto result = hook.unsafe_fastcall<int>(config, edx, section, key, value, size, filename, extra);
            if (!result && answer)
            {
                Copy(*answer, value, size);
                return 1;
            }
            return result;
        }
    };

    constexpr size_t GetStringSlot = 5;
    void* engineReader = nullptr;

    void* ReaderOf(void* config)
    {
        return (*reinterpret_cast<void***>(config))[GetStringSlot];
    }

    void HookGlobalConfig(void* config)
    {
        const auto reader = ReaderOf(config);
        if (reader == engineReader || Reader<0>::hook)
            return;
        Reader<0>::hook = safetyhook::create_inline(reader, Reader<0>::GetString);
    }

    void** GConfig = nullptr;
    SafetyHookInline shAppInit = {};
    void __cdecl AppInit(const wchar_t* a1, const wchar_t* a2, void* a3, void* a4, void* a5, void* a6, void* a7, void* (*a8)(), int a9, void* a10)
    {
        shAppInit.unsafe_ccall(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
        HookGlobalConfig(*GConfig);
    }
}

export void InitConfigReads()
{
    const auto core = GetModuleHandleW(L"Core.dll");
    ConfigReads::GConfig = reinterpret_cast<void**>(GetProcAddress(core, "?GConfig@@3PAVFConfigCache@@A"));
    if (*ConfigReads::GConfig)
        ConfigReads::HookGlobalConfig(*ConfigReads::GConfig);
    else
        ConfigReads::shAppInit = safetyhook::create_inline(GetProcAddress(core, "?appInit@@YAXPBG0PAVFMalloc@@PAVFOutputDevice@@PAVFOutputDeviceError@@PAVFFeedbackContext@@PAVFFileManager@@P6APAVFConfigCache@@XZHPAVFStreamManager@@@Z"), ConfigReads::AppInit);
}

// Engine keeps its own config cache for the graphics profile it applies at startup.
export void InitEngineConfigReads()
{
    auto pattern = hook::module_pattern(GetModuleHandleW(L"Engine.dll"), "8D 4E 18 C6 44 24 14 02 C7 06");
    const auto vtable = *pattern.get_first<void**>(10);
    ConfigReads::engineReader = vtable[ConfigReads::GetStringSlot];
    ConfigReads::Reader<1>::hook = safetyhook::create_inline(ConfigReads::engineReader, ConfigReads::Reader<1>::GetString);
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
            Screen.bRawInputMouse = FixSettings().Bool("MAIN.RawInputMouse");
            FixSettings().OnChange("MAIN.MouseSensitivity", []
            {
                RawInputHandler<>::SetSensitivity(FixSettings().Float("MAIN.MouseSensitivity") * 0.5f);
            });
            Screen.bRawInputMouseRawData = FixSettings().Int("MAIN.RawInputMouseRawData") != 0;
            Screen.fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            Screen.LoadscreenPicture = std::filesystem::path(FixSettings().CfgPath()).replace_extension(L".png");
            gBlacklistIndicators = FixSettings().Int("BONUS.BlacklistIndicators");
            gColor = FixSettings().Int("BONUS.GogglesLightColor");
            bLightSyncRGB = FixSettings().Int("MAIN.LightSyncRGB");

            if (Screen.Width <= 0 || Screen.Height <= 0 || Screen.Width > 16384 || Screen.Height > 16384)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();
            if (Screen.Width <= 0 || Screen.Height <= 0)
                std::tie(Screen.Width, Screen.Height) = std::make_tuple(640, 480);
        };
    }
} Settings;
