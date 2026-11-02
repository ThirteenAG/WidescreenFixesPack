module;

#include "stdafx.h"
#include "NativeMenuView.hpp"
#include "RTTI.h"
#include "TextLookup.hpp"
#include <charconv>
#include <stdexcept>

export module NativeMenu;

export namespace ScarfaceMenu
{
    inline RWGTAMenu::Model model;
    inline View view;
    inline SafetyHookInline activateHook, deactivateHook, destroyHook, backHook, rowHook, selectionHook, evaluateHook;
    inline void* active = nullptr;
    inline bool enabled = false, opened = false, rebuilding = false, refresh = false;
    inline int nativeCount = 0;
    inline std::string countText, shownLanguage;
    inline int(__cdecl* nativeLanguage)() = nullptr;
    inline std::map<std::wstring, std::string> textKeys;
    inline std::map<std::string, std::wstring> texts;
    inline std::map<std::string, std::string> restartValues;
    struct Rectangle { int16_t x = 0, y = 0, width = 0, height = 0; };
    inline std::map<void*, Rectangle> rectangles;
    inline std::map<void*, int> rowIndices;

    template<class Result = void, class... Args> Result Call(void* object, size_t method, Args... args)
    {
        const auto methods = *static_cast<uintptr_t**>(object);
        return reinterpret_cast<Result(__thiscall*)(void*, Args...)>(methods[method])(object, args...);
    }

    inline HMODULE Module()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Module), &module);
        return module;
    }

    inline std::string Language()
    {
        const auto language = FixSettings().String("MAIN.TextLanguage");
        if (language != "auto")
            return language;
        switch (nativeLanguage ? nativeLanguage() : 'E')
        {
        case 'F': return "fr";
        case 'G': return "de";
        case 'I': return "it";
        case 'S': return "es";
        default: return "en";
        }
    }

    inline std::wstring Words(const char* key)
    {
        return asi_settings::text::Widen(RWGTAMenu::PluginText(FixSettings(), Language(), key));
    }

    inline const wchar_t* LookupText(std::string_view key)
    {
        if (!key.starts_with("FUSIONFIX_MENU_"))
            return nullptr;
        const auto found = texts.find(std::string(key));
        return found == texts.end() ? nullptr : found->second.c_str();
    }

    inline std::string Key(const std::wstring& value)
    {
        if (auto found = textKeys.find(value); found != textKeys.end())
            return found->second;
        const auto token = "FUSIONFIX_MENU_" + std::to_string(texts.size());
        texts.emplace(token, value);
        const auto label = "[" + token + "]";
        textKeys.emplace(value, label);
        return label;
    }

    inline int Selected()
    {
        const auto client = active ? *reinterpret_cast<const uint8_t* const*>(static_cast<uint8_t*>(active) + 104) : nullptr;
        return client ? *reinterpret_cast<const int*>(client + 40) + *reinterpret_cast<const int*>(client + 56) : -1;
    }

    inline bool RestartPending()
    {
        for (const auto& [id, value] : restartValues)
            if (FixSettings().Wanted(id) != value)
                return true;
        return false;
    }

    inline void RestoreRectangles()
    {
        for (const auto& [object, rectangle] : rectangles)
        {
            Call(object, 33, rectangle.x, rectangle.y);
            Call(object, 35, rectangle.width, rectangle.height);
        }
        rectangles.clear();
        rowIndices.clear();
    }

    inline void Expand(void* object)
    {
        if (!object)
            return;
        if (!rectangles.count(object))
        {
            Rectangle rectangle;
            Call(object, 34, &rectangle.x, &rectangle.y);
            Call(object, 36, &rectangle.width, &rectangle.height);
            rectangles.emplace(object, rectangle);
        }
        const auto& rectangle = rectangles.at(object);
        Call(object, 35, int16_t{ 360 }, rectangle.height);
    }

    inline void Reset(bool restore = true)
    {
        if (restore)
            RestoreRectangles();
        else
        {
            rectangles.clear();
            rowIndices.clear();
        }
        active = nullptr;
        opened = refresh = false;
        model.Close();
        view = {};
    }

    inline bool __fastcall Activate(void* self, void*, bool visible)
    {
        if (!rebuilding)
        {
            active = self;
            nativeCount = 0;
            opened = false;
            model.Close();
            view = {};
        }
        const bool failed = activateHook.unsafe_thiscall<bool>(self, visible);
        if (failed && active == self)
            Reset();
        return failed;
    }

    inline bool __fastcall Deactivate(void* self, void*, bool visible)
    {
        if (active == self)
        {
            RestoreRectangles();
            if (!rebuilding)
                Reset();
        }
        return deactivateHook.unsafe_thiscall<bool>(self, visible);
    }

    inline void* __fastcall Destroy(void* self, void*, uint8_t flags)
    {
        if (active == self)
            Reset(false);
        return destroyHook.unsafe_thiscall<void*>(self, flags);
    }

    inline void __fastcall Back(void* self, void*)
    {
        if (active == self && opened)
        {
            if (!view.Back(model))
            {
                opened = false;
                model.Close();
                view = {};
            }
            refresh = true;
            return;
        }
        backHook.unsafe_thiscall<void>(self);
    }

    inline void Arrows(void* self, int nativeIndex)
    {
        const int index = nativeIndex - 1;
        const bool shown = index >= 0 && size_t(index) < view.items.size() && view.items[index].kind == View::Kind::Row &&
            (view.items[index].row.kind == RWGTAMenu::RowKind::Setting || view.items[index].row.kind == RWGTAMenu::RowKind::Preset);
        for (const auto offset : { 60, 64 })
            if (auto arrow = *reinterpret_cast<void**>(static_cast<uint8_t*>(self) + offset))
                Call(arrow, 17, shown);
    }

    inline void __fastcall Highlight(void* self, void*)
    {
        selectionHook.unsafe_thiscall<void>(self);
        const auto row = rowIndices.find(self);
        if (!active || !opened || row == rowIndices.end())
            return;
        if (row->second > 0 && size_t(row->second - 1) < view.items.size())
        {
            view.selected = size_t(row->second - 1);
            view.Remember();
        }
        Arrows(self, row->second);
    }

    inline void __fastcall Row(void* self, void*, void* data)
    {
        const int index = *reinterpret_cast<const int*>(static_cast<uint8_t*>(data) + 8);
        if (active && opened)
            rowIndices[self] = index;
        rowHook.unsafe_thiscall<void>(self, data);
        if (!active || !opened)
            return;
        Expand(*reinterpret_cast<void**>(static_cast<uint8_t*>(self) + 52));
        Arrows(self, index);
    }

    inline std::string Return(const std::string& value)
    {
        std::string script = "return \"";
        for (const auto character : value)
        {
            if (character == '\\' || character == '"')
                script += '\\';
            script += character;
        }
        return script + "\";";
    }

    inline std::optional<std::string> Query(std::string_view call)
    {
        if (!active)
            return {};
        if (call == "PauseGetOptionCount();" && opened)
            return std::to_string(view.items.size() + 1 + size_t(RestartPending()));
        if (call == "PauseGetOptionValues();" && opened)
            return "";
        const auto bracket = call.find('(');
        if (bracket == std::string_view::npos || !call.ends_with(");"))
            return {};
        int index = -1;
        const auto parsed = std::from_chars(call.data() + bracket + 1, call.data() + call.size() - 2, index);
        if (parsed.ec != std::errc{} || index < 0)
            return {};
        const auto name = call.substr(0, bracket);
        const bool increment = name == "PauseIncrementOption";
        const auto remainder = std::string_view(parsed.ptr, size_t(call.data() + call.size() - parsed.ptr));
        if (increment ? (remainder != ",true);" && remainder != ",false);") : parsed.ptr != call.data() + call.size() - 2)
            return {};
        if (!opened && (nativeCount <= 0 || index != nativeCount))
            return {};
        if (opened && size_t(index) >= view.items.size() + 1 + size_t(RestartPending()))
            return {};
        if (name == "PauseIncrementOption" || name == "PauseOptionHandleResolutionChange")
        {
            if (!opened)
            {
                if (name != "PauseOptionHandleResolutionChange")
                    return "";
                shownLanguage = Language();
                model.OpenPlugins(Module(), shownLanguage, 32, {}, true);
                view = {};
                opened = view.Build(model, true);
            }
            else if (index > 0 && size_t(index - 1) < view.items.size())
            {
                view.selected = size_t(index - 1);
                view.Remember();
                const int direction = name == "PauseIncrementOption" ? (call.find(",true)") != std::string_view::npos ? 1 : -1) : 0;
                if (!view.Activate(model, true, direction))
                {
                    opened = false;
                    model.Close();
                    view = {};
                }
            }
            refresh = true;
            return "";
        }
        const View::Item* item = opened && index > 0 && size_t(index - 1) < view.items.size() ? &view.items[index - 1] : nullptr;
        if (name == "PauseOptionIsSelectable")
            return !opened || (item && item->enabled) ? "1" : "0";
        if (name == "PauseOptionIsIndented" || name == "PauseOptionIsNumeric" || name == "PauseOptionHasRangedValue")
            return "0";
        if (name == "PauseOptionGetRangedValue")
            return "";
        if (name == "PauseGetOptionName")
        {
            if (!opened)
                return Key(Words("menu.plugins"));
            if (index == 0)
                return Key(view.page < model.Pages().size() ? asi_settings::text::Widen(model.Pages()[view.page].title) : Words("menu.plugins"));
            return Key(item ? item->text : Words("menu.restartshort"));
        }
        return {};
    }

    inline const char* __fastcall Evaluate(void* self, void*, const char* filename, const char* source, const char* arguments)
    {
        std::string call;
        if (active && source && strnlen(source, 512) < 512)
        {
            for (const auto character : std::string_view(source))
                if (character != ' ' && character != '\t' && character != '\r' && character != '\n')
                    call += character;
            if (auto value = Query(call))
            {
                const auto script = Return(*value);
                return evaluateHook.unsafe_thiscall<const char*>(self, filename, script.c_str(), arguments);
            }
        }
        const auto result = evaluateHook.unsafe_thiscall<const char*>(self, filename, source, arguments);
        if (active && !opened && call == "PauseGetOptionCount();" && result)
        {
            nativeCount = std::atoi(result);
            if (nativeCount > 0 && nativeCount < 120)
            {
                countText = std::to_string(nativeCount + 1);
                return countText.c_str();
            }
        }
        return result;
    }

    inline void Tick()
    {
        if (!active || rebuilding)
            return;
        if (opened && shownLanguage != Language())
        {
            shownLanguage = Language();
            model.OpenPlugins(Module(), shownLanguage, 32, {}, true);
            view = {};
            refresh = true;
        }
        if (opened && RWGTAMenu::Model::Changed().exchange(false))
            refresh = true;
        if (!refresh)
            return;
        refresh = false;
        if (opened && !view.Build(model, true))
        {
            opened = false;
            model.Close();
        }
        const auto self = active;
        const auto desiredSelection = view.selected;
        rebuilding = true;
        if (Call<bool>(self, 45, true))
        {
            rebuilding = false;
            refresh = true;
            return;
        }
        if (Call<bool>(self, 44, true))
        {
            rebuilding = false;
            Reset();
            return;
        }
        if (opened)
        {
            auto client = *reinterpret_cast<void**>(static_cast<uint8_t*>(self) + 104);
            const auto target = int(desiredSelection + 1);
            for (size_t attempt = 0; client && attempt < view.items.size(); ++attempt)
            {
                if (Selected() == target)
                    break;
                Call(client, 3, true, "toggle");
            }
        }
        rebuilding = false;
    }

    inline bool Install()
    {
        const auto options = ScarfaceRTTI::FindVtable(".?AVPauseMenuPageOptions@@");
        const auto item = ScarfaceRTTI::FindVtable(".?AVPauseMenuPageOptionsItem@PauseMenuPageOptions@@");
        auto evaluator = hook::pattern("83 EC 44 53 56 57 6A 0F 8B F1 E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 04 C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 8B 7C 24 54 57 E8 ? ? ? ?");
        if (!options || !item || !ScarfaceText::Install())
            throw std::runtime_error("Native menu dependencies unavailable");
        auto language = hook::pattern("E8 ? ? ? ? 83 C0 BD 83 F8 10 77 61 0F B6 80 ? ? ? ? FF 24 85");
        nativeLanguage = reinterpret_cast<decltype(nativeLanguage)>(injector::GetBranchDestination(language.get_first()).as_int());
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.HasValue() && setting.apply == "restart")
                restartValues.emplace(setting.id, FixSettings().Wanted(setting.id));
        const auto methods = reinterpret_cast<const uintptr_t*>(options);
        const auto rowMethods = reinterpret_cast<const uintptr_t*>(item);
        activateHook = safetyhook::create_inline(reinterpret_cast<void*>(methods[44]), Activate);
        deactivateHook = safetyhook::create_inline(reinterpret_cast<void*>(methods[45]), Deactivate);
        destroyHook = safetyhook::create_inline(reinterpret_cast<void*>(methods[1]), Destroy);
        backHook = safetyhook::create_inline(reinterpret_cast<void*>(methods[65]), Back);
        rowHook = safetyhook::create_inline(reinterpret_cast<void*>(rowMethods[5]), Row);
        selectionHook = safetyhook::create_inline(reinterpret_cast<void*>(rowMethods[1]), Highlight);
        evaluateHook = safetyhook::create_inline(evaluator.get_first(), Evaluate);
        enabled = activateHook && deactivateHook && destroyHook && backHook && rowHook && selectionHook && evaluateHook;
        if (enabled)
            ScarfaceText::overrideText = LookupText;
        else
        {
            activateHook.reset(); deactivateHook.reset(); destroyHook.reset(); backHook.reset(); rowHook.reset(); selectionHook.reset(); evaluateHook.reset();
            ScarfaceText::Shutdown();
            throw std::runtime_error("Native menu hooks unavailable");
        }
        return enabled;
    }
}
