module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Updates;

// Update notice: in place of the version number at the top of the main menu
export namespace Updates
{
    std::string notice;
    bool available{}, hubPresent{}, drawingVersion{}, drewVersion{};
    std::atomic_bool languageChanged{}, dismissNotice{};
    void* versionTextbox{};

    using MakeStringFn = int(__cdecl*)(const char* text, int type, int flags);
    using SetTextFn = int(__thiscall*)(void* textbox, int text);
    using ReleaseStringFn = int(__cdecl*)(int text);
    MakeStringFn MakeString = nullptr;
    SetTextFn SetText = nullptr;
    ReleaseStringFn ReleaseString = nullptr;
    SafetyHookInline VersionHook, TextboxDestructor;
    SafetyHookMid TextboxDraw;

    void RefreshNotice()
    {
        static auto texts = FixSettings().GetTexts();
        const auto language = FixSettings().Get("MAIN.TextLanguage");
        texts.Select(language == "auto" ? "en" : language);
        const auto line = RWGTA::UpdateCheck::ReplacePage(asi_settings::text::Widen(
            texts.Resolve(hubPresent ? "@update.availablehub" : "@update.availablesite")));
        // The engine's textbox takes a single-byte string.
        notice.clear();
        for (const auto character : line)
            notice.push_back(character < 256 ? static_cast<char>(character) : '?');
    }

    int __stdcall Version(int* out)
    {
        if (!available)
            return VersionHook.stdcall<int>(out);
        *out = MakeString(notice.c_str(), 45, 0);
        return *out;
    }

    int __fastcall RememberVersionTextbox(void* textbox, void*, int text)
    {
        versionTextbox = textbox;
        return SetText(textbox, text);
    }

    int __fastcall DeleteTextbox(void* textbox, void*)
    {
        if (versionTextbox == textbox)
            versionTextbox = nullptr;
        return TextboxDestructor.unsafe_thiscall<int>(textbox);
    }

    void SetVersionText()
    {
        int text{};
        Version(&text);
        SetText(versionTextbox, text);
        ReleaseString(text);
    }

    void DrawVersionTextbox(SafetyHookContext& context)
    {
        const auto textbox = reinterpret_cast<const uint8_t*>(context.esi);
        if (!versionTextbox || textbox != versionTextbox)
            return;
        const auto string = *reinterpret_cast<const int* const*>(textbox + 0xAC);
        if (!string || !*string)
            return;
        drewVersion = true;
        const bool dismiss = dismissNotice.exchange(false, std::memory_order_relaxed);
        const bool localize = languageChanged.exchange(false, std::memory_order_relaxed);
        if (available && (dismiss || localize))
        {
            available = !dismiss;
            if (available)
                RefreshNotice();
            SetVersionText();
        }
        drawingVersion = true;
        RWGTA::UpdateCheck::Tick();
        drawingVersion = false;
    }

    void Tick()
    {
        if (!std::exchange(drewVersion, false))
            RWGTA::UpdateCheck::Tick();
    }

    void Install()
    {
        auto version = hook::pattern("83 EC 34 FF 15 ? ? ? ? 35 D3 3F EA 19 50 8D 44 24 04 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 0C 8D 0C 24 51 6A 00 6A 02 FF 15 ? ? ? ? 85 C0 74 24");
        auto versionText = hook::pattern("E8 ? ? ? ? 8B 6C 24 68 8B 8E 94 00 00 00 55 E8 ? ? ? ? 55 E8 ? ? ? ? 83 C4 04 8D 4C 24 20");
        auto destroyText = hook::pattern("56 8B F1 57 8B BE AC 00 00 00 85 FF C7 06 ? ? ? ? C7 46 24 ? ? ? ? 74 2B");
        auto drawText = hook::pattern("0F 2F C8 0F 87 ? ? ? ? 8B 86 AC 00 00 00 3B C5 74 ? 39 28 0F 84 ? ? ? ? F6 46 14 08");
        MakeString = reinterpret_cast<MakeStringFn>(injector::GetBranchDestination(version.get_first(0x43)).as_int()); //0x488290
        SetText = reinterpret_cast<SetTextFn>(injector::GetBranchDestination(versionText.get_first(16)).as_int());
        ReleaseString = reinterpret_cast<ReleaseStringFn>(injector::GetBranchDestination(versionText.get_first(22)).as_int());
        injector::MakeCALL(versionText.get_first(16), RememberVersionTextbox, true);
        VersionHook = safetyhook::create_inline(version.get_first(), Version);                                           //0x4856F0
        TextboxDestructor = safetyhook::create_inline(destroyText.get_first(), DeleteTextbox);
        // After the native visibility tests, before the current draw reads the textbox string.
        TextboxDraw = safetyhook::create_mid(drawText.get_first(9), DrawVersionTextbox);

        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TotalOverdose.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return drawingVersion; };
        config.format = [](const std::string&, bool hub)
        {
            hubPresent = hub;
            RefreshNotice();
            available = true;
            return std::wstring{};
        };
        config.show = [](const std::wstring&) { SetVersionText(); };
        FixSettings().OnChange("MAIN.TextLanguage", []()
        {
            languageChanged.store(true, std::memory_order_relaxed);
        });
        FixSettings().OnChange("MAIN.CheckForUpdates", []()
        {
            if (FixSettings().Get("MAIN.CheckForUpdates") == "0")
                dismissNotice.store(true, std::memory_order_relaxed);
        });
        RWGTA::UpdateCheck::Init(std::move(config));
    }
}

