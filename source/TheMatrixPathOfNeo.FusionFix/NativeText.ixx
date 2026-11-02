module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "NativeTextHooks.hpp"


export module NativeText;

import Hud;

export namespace NeoText
{
    struct Template
    {
        std::array<uint8_t, 1204> Node{};
        uintptr_t Root = 0;
        uintptr_t Scene = 0;
        uintptr_t Edi = 0;
        uintptr_t Esi = 0;
        bool Ready = false;
    };

    inline SafetyHookMid TextHook;
    inline uintptr_t NativeDraw = 0;
    inline thread_local bool Painting = false;
    inline thread_local Template Current;
    inline std::wstring Notice;
    inline ULONGLONG NoticeUntil = 0;
    inline void (*Tick)() = nullptr;
    inline std::map<std::string, std::string> Startup;
    inline std::mutex LanguageMutex;
    inline std::string Language = "en";
    inline bool LanguagePending = true;

    inline void ReadLanguage()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language.empty() || _stricmp(language.c_str(), "auto") == 0)
        {
            switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
            {
                case LANG_FRENCH: language = "fr"; break;
                case LANG_GERMAN: language = "de"; break;
                case LANG_ITALIAN: language = "it"; break;
                case LANG_SPANISH: language = "es"; break;
                default: language = "en"; break;
            }
        }
        const std::lock_guard lock(LanguageMutex);
        Language = language;
        LanguagePending = true;
    }

    inline void ApplyLanguage()
    {
        const std::lock_guard lock(LanguageMutex);
        if (LanguagePending)
        {
            FixSettings().GetTexts().Select(Language);
            LanguagePending = false;
        }
    }

    inline bool RestartRequired()
    {
        for (const auto& [id, value] : Startup)
        {
            if (FixSettings().Wanted(id) != value)
                return true;
        }
        return false;
    }

    inline void Capture(SafetyHookContext& context)
    {
        if (Painting)
            return;
        const auto root = *reinterpret_cast<uintptr_t*>(context.esp + 4);
        const auto scene = *reinterpret_cast<uintptr_t*>(context.esp + 8);
        if (!root || !scene)
            return;
        const int width = *reinterpret_cast<int*>(root + 600), height = *reinterpret_cast<int*>(root + 604);
        if (width < 320 || width > 8192 || height < 240 || height > 8192)
            return;
        std::memcpy(Current.Node.data(), reinterpret_cast<void*>(context.ecx), Current.Node.size());
        Current.Root = root;
        Current.Scene = scene;
        Current.Edi = context.edi;
        Current.Esi = context.esi;
        Current.Ready = true;
    }



    inline void DrawLine(const std::wstring& text, float y)
    {
        auto node = Current.Node;
        const auto write = [&node](size_t offset, auto value)
        {
            std::memcpy(node.data() + offset, &value, sizeof(value));
        };
        write(32, 0.06f);
        write(36, y);
        write(44, 0.88f);
        write(48, 0.05f);
        write(68, static_cast<uint32_t>(text.size()));
        write(72, text.c_str());
        write(620, 0.0f);
        write(624, 0.0f);
        write(628, 0.35f);
        Painting = true;
        Invoke(NativeDraw, node.data(), Current.Root, Current.Scene, Current.Edi, Current.Esi);
        Painting = false;
    }

    inline void BeforeRoot(void* root)
    {
        if (!Current.Ready || Current.Root != reinterpret_cast<uintptr_t>(root))
            return;
        ApplyLanguage();
        const auto name = *reinterpret_cast<const char**>(Current.Root + 632);
        if (!NeoHud::Contains(name, "start_screen") && !NeoHud::Contains(name, "gm_") &&
            !NeoHud::Contains(name, "pause_screen"))
        {
            Current.Ready = false;
            return;
        }
        if (Tick)
            Tick();
        if (NoticeUntil && GetTickCount64() >= NoticeUntil)
        {
            Notice.clear();
            NoticeUntil = 0;
        }
        auto message = Notice;
        if (message.empty() && RestartRequired())
            message = asi_settings::text::Widen(FixSettings().GetTexts().Resolve("@menu.restartwarning"));
        if (message.empty())
        {
            Current.Ready = false;
            return;
        }
        size_t begin = 0;
        for (unsigned line = 0; begin < message.size() && line < 4; ++line)
        {
            size_t end = std::min(begin + 62, message.size());
            if (end < message.size())
            {
                const auto space = message.rfind(L' ', end);
                if (space != std::wstring::npos && space > begin)
                    end = space;
            }
            DrawLine(message.substr(begin, end - begin), 0.04f + line * 0.045f);
            begin = end;
            while (begin < message.size() && message[begin] == L' ')
                ++begin;
        }
        Current.Ready = false;
    }

    inline void Install()
    {
        ReadLanguage();
        ApplyLanguage();
        FixSettings().OnChange("MAIN.TextLanguage", ReadLanguage);
        for (const auto& setting : FixSettings().GetDescription().settings)
        {
            if (setting.apply == "restart")
                Startup.emplace(setting.id, FixSettings().Wanted(setting.id));
        }
        auto text = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 81 EC 8C 01 00 00 53 55 8B E9 8B 45 44");
        NativeDraw = reinterpret_cast<uintptr_t>(text.get_first());
        TextHook = safetyhook::create_mid(text.get_first(), Capture);
        if (TextHook)
            NeoHud::BeforeRoot = BeforeRoot;
    }
}
