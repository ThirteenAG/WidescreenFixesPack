#pragma once
#include "FixSettings.hpp"
#include <set>
#include "../Shared/RWGTA/FixMenu.hpp"

// Builds the Plugins page from the game's own UWindow controls, found through
// reflection on the installed script package.
namespace SCMenu
{
    inline SafetyHookInline internalHook, destroyHook;
    inline wchar_t* (__fastcall* fullName)(void*, void*, wchar_t*) = nullptr;
    inline void* (__cdecl* findObject)(void*, void*, const wchar_t*, int) = nullptr;
    inline void (__fastcall* processEvent)(void*, void*, void*, void*, void*) = nullptr;
    inline void (__fastcall* copyString)(void*, void*, void*, const void*, void*) = nullptr;
    inline void (__fastcall* destroyString)(void*, void*, void*) = nullptr;
    inline const wchar_t* (__cdecl* getLanguage)() = nullptr;
    inline HMODULE hostModule = nullptr;
    inline void (__fastcall* textSize)(void*, void*, const wchar_t*, float*, float*) = nullptr;
    inline bool busy = false;
    inline std::unordered_map<void*, std::wstring> functionNames;

    template<class T> T Read(void* p, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(p) + offset);
    }
    inline std::wstring Name(void* object)
    {
        if (!object || !fullName) return {};
        wchar_t buffer[256]{};
        auto value = fullName(object, nullptr, buffer);
        return value ? value : L"";
    }
    inline void LogOnce(const std::string& phase, void* object, const std::string& reason)
    {
        static std::set<std::tuple<std::string, void*, std::string>> written;
        if (!written.emplace(phase, object, reason).second) return;
        auto name = Name(object);
        auto line = "SC1 native menu " + phase + ": " + reason;
        if (!name.empty()) line += " [" + std::string(name.begin(), name.end()) + "]";
        line += "\n";
        OutputDebugStringA(line.c_str());
    }
    inline std::wstring Leaf(void* object)
    {
        auto name = Name(object);
        auto at = name.find_last_of(L". ");
        return at == std::wstring::npos ? name : name.substr(at + 1);
    }
    inline bool Type(void* object, const wchar_t* kind)
    {
        auto name = Name(object);
        return name.starts_with(std::wstring(kind) + L" ");
    }
    // UObject.Class +36, UStruct.SuperField +44, Children +56, UField.Next +48.
    // The engine uses this same traversal in execSetPropertyText.
    inline void* Field(void* scope, const wchar_t* name, const wchar_t* kind)
    {
        for (int depth = 0; scope && depth < 32; ++depth, scope = Read<void*>(scope, 44))
        {
            int count = 0;
            for (auto field = Read<void*>(scope, 56), next = field; field && ++count <= 512; field = next)
            {
                next = Read<void*>(field, 48);
                if (_wcsicmp(Leaf(field).c_str(), name) == 0 && Type(field, kind)) return field;
                if (next == field) break;
            }
        }
        return nullptr;
    }
    inline void* Property(void* object, const wchar_t* name, const wchar_t* kind, size_t size)
    {
        if (!object) return nullptr;
        auto cls = Read<void*>(object, 36);
        auto prop = Field(cls, name, kind);
        if (!prop || Read<uint16_t>(prop, 56) != 1 || Read<uint16_t>(prop, 58) != size) return nullptr;
        auto end = size_t(Read<uint16_t>(prop, 68)) + size;
        auto total = Read<int>(cls, 60);
        return total > 0 && total < 65536 && end <= size_t(total) ? prop : nullptr;
    }
    inline void* Address(void* object, void* prop)
    {
        return static_cast<uint8_t*>(object) + Read<uint16_t>(prop, 68);
    }
    template<class T> bool Get(void* object, const wchar_t* name, const wchar_t* kind, T& out)
    {
        auto prop = Property(object, name, kind, sizeof(T));
        if (!prop) return false;
        std::memcpy(&out, Address(object, prop), sizeof(T)); return true;
    }
    template<class T> bool Set(void* object, const wchar_t* name, const wchar_t* kind, T value)
    {
        auto prop = Property(object, name, kind, sizeof(T));
        if (!prop) return false;
        ProtectedGameRef<T>(static_cast<T*>(Address(object, prop))) = value; return true;
    }
    inline bool Bool(void* object, const wchar_t* name, bool* write = nullptr)
    {
        auto prop = Property(object, name, L"BoolProperty", 4);
        if (!prop) return false;
        auto mask = Read<uint32_t>(prop, 84); // native UBoolProperty::CopySingleValue
        if (!mask || (mask & (mask - 1))) return false;
        auto address = static_cast<uint32_t*>(Address(object, prop));
        if (write) ProtectedGameRef<uint32_t>{address} = (*address & ~mask) | (*write ? mask : 0);
        return (*address & mask) != 0;
    }
    inline bool __cdecl WindowsMenuMouseActive(void* viewport)
    {
        if (!viewport) return false;
        static void* cachedClass = nullptr;
        static void* show = nullptr, * available = nullptr;
        auto cls = Read<void*>(viewport, 36);
        if (cachedClass != cls)
        {
            cachedClass = cls;
            show = Property(viewport, L"bShowWindowsMouse", L"BoolProperty", 4);
            available = Property(viewport, L"bWindowsMouseAvailable", L"BoolProperty", 4);
            if (!show || !available) LogOnce("mouse", viewport, "viewport mouse properties unavailable");
        }
        auto enabled = [viewport](void* prop)
        {
            if (!prop) return false;
            auto mask = Read<uint32_t>(prop, 84);
            return mask && !(mask & (mask - 1)) && (*static_cast<uint32_t*>(Address(viewport, prop)) & mask) != 0;
        };
        const bool shown = enabled(show), windows = enabled(available);
        // Captured software-cursor menus have shown=true, available=false.
        // Both native cursor routes must keep their own movement and picking.
        return shown;
    }
    inline std::wstring MenuCharacters(const std::wstring& text)
    {
        std::wstring result;
        result.reserve(text.size());
        for (auto c : text)
        {
            switch (c)
            {
                case 0x2018: case 0x2019: case 0x201A: case 0x2032: result += L'\''; break;
                case 0x201C: case 0x201D: case 0x201E: case 0x2033: result += L'"'; break;
                case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2212: result += L'-'; break;
                case 0x2026: result += L"..."; break;
                case 0x00A0: case 0x202F: result += L' '; break;
                case 0x00BF: result += L'?'; break;
                case 0x00A1: result += L'!'; break;
                case 0x00DF: result += L"ss"; break;
                case 0x1E9E: result += L"SS"; break;
                case 0x0152: result += L"OE"; break;
                case 0x0153: result += L"oe"; break;
                case 0x00C6: result += L"AE"; break;
                case 0x00E6: result += L"ae"; break;
                default:
                {
                    wchar_t decomposed[8]{};
                    const int count = c >= 0x00C0 && c <= 0x024F ? FoldStringW(MAP_COMPOSITE, &c, 1, decomposed, 8) : 0;
                    bool latin = count > 1 && ((decomposed[0] >= L'A' && decomposed[0] <= L'Z') || (decomposed[0] >= L'a' && decomposed[0] <= L'z'));
                    for (int i = 1; i < count && latin; ++i) latin = decomposed[i] >= 0x0300 && decomposed[i] <= 0x036F;
                    result += latin ? decomposed[0] : c;
                    break;
                }
            }
        }
        return result;
    }
    inline std::wstring Wide(const std::string& text)
    {
        int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        std::wstring out(size, L'\0');
        if (size) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), out.data(), size);
        if (out.size() > 256)
        {
            out.resize(256);
            if (out.back() >= 0xD800 && out.back() <= 0xDBFF) out.pop_back();
        }
        return MenuCharacters(out);
    }
    inline bool String(void* object, const wchar_t* name, const std::string& text)
    {
        auto prop = Property(object, name, L"StrProperty", 12);
        if (!prop) return false;
        auto wide = Wide(text);
        struct Array { const wchar_t* data; int count, capacity; } source{ wide.c_str(), int(wide.size()) + 1, int(wide.size()) + 1 };
        // Engine allocation/copy, never attach std::wstring storage to a UObject.
        copyString(prop, nullptr, Address(object, prop), &source, object); return true;
    }
    struct Call
    {
        void* object = nullptr, * function = nullptr;
        std::vector<uint8_t> data;
        std::vector<std::pair<void*, size_t>> strings;
        Call(const Call&) = delete;
        Call& operator=(const Call&) = delete;
        ~Call()
        {
            for (const auto& [property, offset] : strings)
                destroyString(property, nullptr, data.data() + offset);
        }
        Call(void* target, const wchar_t* name) : object(target)
        {
            if (!target) return;
            function = Field(Read<void*>(target, 36), name, L"Function");
            // ProcessEvent does not execute native-index functions. UWindow
            // CreateControl/CreateWindow/Show/Hide and focus are script functions.
            if (!function || Read<uint16_t>(function, 92) || Read<uint16_t>(function, 96) > 256)
            { LogOnce("function", target, "missing/unsupported reflected function " + std::string(name, name + wcslen(name))); function = nullptr; return; }
            data.resize(std::max<size_t>(1, Read<uint16_t>(function, 96)), 0);
        }
        void* Param(const wchar_t* name, const wchar_t* kind, size_t size)
        {
            auto p = function ? Field(function, name, kind) : nullptr;
            if (!p || Read<uint16_t>(p, 56) != 1 || Read<uint16_t>(p, 58) != size ||
                size_t(Read<uint16_t>(p, 68)) + size > Read<uint16_t>(function, 96))
            { LogOnce("parameter", object, "name/type/size mismatch " + std::string(name, name + wcslen(name))); return nullptr; }
            return data.data() + Read<uint16_t>(p, 68);
        }
        template<class T> bool Put(const wchar_t* name, const wchar_t* kind, T value)
        {
            auto p = Param(name, kind, sizeof(T));
            if (!p) return false;
            std::memcpy(p, &value, sizeof(T)); return true;
        }
        bool PutText(const wchar_t* name, const std::string& value)
        {
            auto destination = Param(name, L"StrProperty", 12);
            auto property = function ? Field(function, name, L"StrProperty") : nullptr;
            if (!destination || !property || !copyString || !destroyString) return false;
            const size_t offset = static_cast<uint8_t*>(destination) - data.data();
            if (std::none_of(strings.begin(), strings.end(), [offset](const auto& item) { return item.second == offset; }))
                strings.emplace_back(property, offset);
            auto wide = Wide(value);
            struct Array { const wchar_t* data; int count, capacity; } source{ wide.c_str(), int(wide.size()) + 1, int(wide.size()) + 1 };
            copyString(property, nullptr, destination, &source, object);
            return true;
        }
        bool Run()
        {
            if (!function) return false;
            processEvent(object, nullptr, function, data.data(), nullptr); return true;
        }
        void* Result()
        {
            auto p = Param(L"ReturnValue", L"ObjectProperty", 4);
            return p ? *static_cast<void**>(p) : nullptr;
        }
    };
    inline bool Invoke(void* object, const wchar_t* name)
    {
        Call call(object, name);
        if (!call.function || Read<uint16_t>(call.function, 96)) return false;
        return call.Run();
    }
    inline void* Create(void* parent, bool control, void* cls, float x, float y, float w, float h)
    {
        Call call(parent, control ? L"CreateControl" : L"CreateWindow");
        if (!call.Put(control ? L"ControlClass" : L"WndClass", L"ClassProperty", cls) ||
            !call.Put(L"X", L"FloatProperty", x) || !call.Put(L"Y", L"FloatProperty", y) ||
            !call.Put(L"W", L"FloatProperty", w) || !call.Put(L"H", L"FloatProperty", h) ||
            !call.Put(control ? L"OwnerWindow" : L"OwnerW", L"ObjectProperty", parent) ||
            !call.Param(L"ReturnValue", L"ObjectProperty", 4)) return nullptr;
        if (!call.Run()) return nullptr;
        // CreateWindow invokes Created/AfterCreate, Root/Parent/Owner setup and
        // ShowChildWindow. CreateControl additionally registers Notify and Tab.
        return call.Result();
    }
    inline void* Object(void* parent, const wchar_t* name)
    {
        void* out = nullptr; Get(parent, name, L"ObjectProperty", out); return out;
    }
    inline std::string Language()
    {
        auto chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        auto language = getLanguage ? getLanguage() : nullptr;
        auto native = language ? std::wstring(language) : L"int";
        if (!_wcsicmp(native.c_str(), L"fre") || !_wcsicmp(native.c_str(), L"fra")) return "fr";
        if (native == L"deu") return "de";
        if (native == L"ita") return "it";
        if (!_wcsicmp(native.c_str(), L"spa") || !_wcsicmp(native.c_str(), L"esp")) return "es";
        if (native == L"rus") return "ru";
        return "en";
    }
    struct Item
    {
        RWGTAMenu::Row row;
        std::string label, value;
        int link = 0;
        bool enabled = true;
    };
    #include "NativeMenuResolution.hpp"

    struct Host
    {
        void* parent = nullptr, * entry = nullptr, * panel = nullptr, * savedTab = nullptr;
        void* title = nullptr, * help = nullptr, * sample = nullptr;
        std::vector<void*> buttons, values;
        std::vector<Item> items;
        float rowWidth = 0;
        std::optional<RWGTAMenu::Row> helpRow;
        std::string helpLabel;
        std::wstring detailsText;
        std::vector<std::wstring> detailsLines;
        void* detailsFont = nullptr;
        float detailsWidth = 0, detailsSpacing = 0;
        size_t detailsPage = 0;
        bool details = false, detailsDirty = false, detailsNext = false;
        bool pause = false, open = false;
        size_t page = SIZE_MAX, rootStart = 0, rowStart = 0, selected = 0;
    };
    inline std::map<void*, Host> hosts;
    inline Host* active = nullptr;
    inline RWGTAMenu::Model model;
    inline constexpr size_t RootLinks = 3, PageRows = 5, ContentRows = 2;
    inline void RenderDetails(Host& host, void* canvas);
    inline void BeginDetails(Host& host);
    inline void EndDetails(Host& host);
    inline bool DetailsKey(Host& host, int key);
    inline void SelectDetailsNavigation(Host& host, bool next);
    inline void DetailsStep(Host& host, int direction);
    struct Caption
    {
        std::wstring requested;
        void* font = nullptr;
        float width = 0, height = 0, spacing = 0;
        bool fitted = false;
    };
    inline std::map<void*, Caption> captions;
    inline void Label(void* button, const std::string& text)
    {
        if (!button) return;
        auto wide = Wide(text);
        for (auto& c : wide) if (c < 32 || c == 8232 || c == 8233) c = L' ';
        auto locale = Wide(Language());
        int size = LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), int(wide.size()), nullptr, 0, nullptr, nullptr, 0);
        if (size > 0)
        {
            std::wstring upper(size, L'\0');
            if (LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), int(wide.size()), upper.data(), size, nullptr, nullptr, 0)) wide = std::move(upper);
        }
        auto& caption = captions[button];
        if (caption.requested != wide) { caption.requested = std::move(wide); caption.fitted = false; }
    }
    inline void FitLabel(void* button, void* canvas)
    {
        if (active && active->open && active->details)
            RenderDetails(*active, canvas);
        auto found = captions.find(button);
        if (found == captions.end() || !textSize || !canvas) return;
        auto font = Object(canvas, L"Font");
        float width = 0, height = 0, spacing = 0, margin = 0;
        if (!font || !Get(button, L"WinWidth", L"FloatProperty", width) || !Get(button, L"WinHeight", L"FloatProperty", height)) return;
        Get(button, L"m_fFontSpacing", L"FloatProperty", spacing); Get(button, L"m_fLMarge", L"FloatProperty", margin);
        width -= std::max(0.0f, margin) * 2 + 2;
        if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(spacing) || width <= 0 || height <= 0) return;
        auto& caption = found->second;
        if (caption.fitted && caption.font == font && caption.width == width && caption.height == height && caption.spacing == spacing) return;
        auto measure = [&](const std::wstring& value)
        {
            float w = 0, h = 0;
            textSize(canvas, nullptr, value.c_str(), &w, &h);
            if (!std::isfinite(w) || !std::isfinite(h) || h > height || (!value.empty() && w <= 0)) return INFINITY;
            return w + float(value.size()) * spacing;
        };
        auto value = caption.requested;
        // A missing glyph must not silently disappear from a plugin caption.
        for (auto& c : value)
        {
            if (c == L' ') continue;
            if (!std::isfinite(measure(std::wstring(1, c)))) c = L'?';
        }
        if (measure(value) > width)
        {
            while (!value.empty() && measure(value + L"...") > width)
            {
                value.pop_back();
                if (!value.empty() && value.back() >= 0xD800 && value.back() <= 0xDBFF) value.pop_back();
            }
            value = measure(value + L"...") <= width ? value + L"..." : L"";
        }
        int count = WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr);
        std::string fitted(count, '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()), fitted.data(), count, nullptr, nullptr);
        bool previous = busy; busy = true;
        String(button, L"Text", fitted);
        busy = previous;
        caption.font = font; caption.width = width; caption.height = height; caption.spacing = spacing; caption.fitted = true;
    }
    inline bool ActivateWindow(void* window)
    {
        Call call(window, L"ActivateWindow");
        if (!call.Put(L"Depth", L"IntProperty", 0) || !call.Param(L"bTransientNoDeactivate", L"BoolProperty", 4)) return false;
        // False is zero for any reflected bool mask. The script propagates
        // ActiveWindow to every parent and calls FocusWindow at Depth0.
        return call.Run();
    }
    inline bool ControlContract(void* control)
    {
        if (!control || !Type(control, L"EPCTextButton")) return false;
        for (auto name : { L"Text", L"ToolTipString" }) if (!Property(control, name, L"StrProperty", 12)) return false;
        for (auto name : { L"WinLeft", L"WinTop", L"WinWidth", L"WinHeight" }) if (!Property(control, name, L"FloatProperty", 4)) return false;
        for (auto name : { L"Root", L"ParentWindow", L"NotifyWindow", L"TabNext", L"TabPrev" }) if (!Property(control, name, L"ObjectProperty", 4)) return false;
        for (auto name : { L"m_bSelected", L"bDisabled", L"bAcceptsFocus", L"bWindowVisible", L"bNoKeyboard" })
        {
            auto prop = Property(control, name, L"BoolProperty", 4);
            if (!prop) return false;
            auto mask = Read<uint32_t>(prop, 84);
            if (!mask || (mask & (mask - 1))) return false;
        }
        if (!Property(control, L"Font", L"IntProperty", 4) || !Property(control, L"Align", L"ByteProperty", 1)) return false;
        Call activate(control, L"ActivateWindow");
        if (!activate.Param(L"Depth", L"IntProperty", 4) || !activate.Param(L"bTransientNoDeactivate", L"BoolProperty", 4)) return false;
        for (auto name : { L"ShowWindow", L"HideWindow", L"BringToFront", L"SetAcceptsFocus" })
        {
            Call call(control, name);
            if (!call.function || Read<uint16_t>(call.function, 96)) return false;
        }
        return true;
    }
    inline void Focus(Host& host)
    {
        if (host.details) return;
        bool previous = busy; busy = true;
        for (size_t i = 0; i < host.buttons.size(); ++i)
        {
            bool selected = i == host.selected;
            Bool(host.buttons[i], L"m_bSelected", &selected);
            Bool(host.values[i], L"m_bSelected", &selected);
        }
        if (host.open && host.selected < host.items.size())
        {
            const auto& item = host.items[host.selected];
            if (!item.link)
            {
                host.helpRow = item.row;
                host.helpLabel = item.label;
            }
            const auto help = !item.link ? model.Help(item.row) :
                item.link == -4 && host.helpRow ? model.Help(*host.helpRow) : std::string{};
            Label(host.help, help.empty() ? "" : model.Text("menu.detailskey") + "  " + help);
            ActivateWindow(host.buttons[host.selected]);
        }
        busy = previous;
    }
    inline void Close(Host& host, bool restore = true)
    {
        bool oldBusy = busy; busy = true;
        host.open = false; host.details = false; host.helpRow.reset(); host.detailsLines.clear(); host.detailsText.clear();
        bool selected = false; Bool(host.entry, L"m_bSelected", &selected);
        if (host.panel) Invoke(host.panel, L"HideWindow");
        if (restore && host.savedTab)
        {
            Call change(host.parent, L"ChangeTopButtonSelection");
            if (change.Put(L"_SelectMe", L"ObjectProperty", host.savedTab)) change.Run();
        }
        if (auto reset = Object(host.parent, L"m_ResetToDefault")) Invoke(reset, L"ShowWindow");
        if (restore && host.savedTab) ActivateWindow(host.savedTab);
        if (active == &host) { active = nullptr; model.Close(); }
        busy = oldBusy;
    }
    inline size_t Links(const Host& host)
    {
        return std::min(RootLinks, model.Pages().size() - std::min(host.rootStart, model.Pages().size()));
    }
    inline void Populate(Host& host)
    {
        if (host.details) return;
        const bool root = host.page == SIZE_MAX;
        host.items.clear();
        auto title = root ? model.Text("menu.plugins") : model.Pages()[host.page].title;
        if (model.RestartRequired()) title += " - " + model.Text("menu.restartwarning");
        Label(host.title, title);
        if (root)
        {
            auto links = Links(host);
            for (size_t i = 0; i < links; ++i)
                host.items.push_back({ {}, model.Pages()[host.rootStart + i].title, "", int(host.rootStart + i + 1) });
            if (host.rootStart) host.items.push_back({ {}, model.Text("menu.previous"), "", -2 });
            if (host.rootStart + links < model.Pages().size()) host.items.push_back({ {}, model.Text("menu.next"), "", -3 });
        }
        else
        {
            auto rows = model.VisibleRows(host.page);
            if (host.rowStart >= rows.size()) host.rowStart = 0;
            for (size_t i = host.rowStart; i < rows.size() && i < host.rowStart + ContentRows; ++i)
                host.items.push_back({ *rows[i], model.Label(*rows[i]), model.Value(*rows[i]), 0, model.Enabled(*rows[i], host.pause) });
            if (host.rowStart) host.items.push_back({ {}, model.Text("menu.previous"), "", -2 });
            if (host.rowStart + ContentRows < rows.size()) host.items.push_back({ {}, model.Text("menu.next"), "", -3 });
            host.items.push_back({ {}, model.Text("menu.details"), "", -4 });
        }
        host.items.push_back({ {}, model.Text("menu.back"), "", -1 });
        for (size_t i = 0; i < host.buttons.size(); ++i)
        {
            auto button = host.buttons[i], value = host.values[i];
            Invoke(value, L"HideWindow");
            Set(button, L"WinWidth", L"FloatProperty", host.rowWidth);
            if (i >= host.items.size()) { Invoke(button, L"HideWindow"); continue; }
            auto& item = host.items[i];
            bool disabled = !item.enabled;
            String(button, L"ToolTipString", "");
            if (!item.link)
            {
                Set(button, L"WinWidth", L"FloatProperty", host.rowWidth * 0.60f - 8);
                Label(value, disabled ? model.Text("menu.unavailable") : item.value);
                Bool(value, L"bDisabled", &disabled); Invoke(value, L"ShowWindow");
            }
            Label(button, item.label); Bool(button, L"bDisabled", &disabled); Invoke(button, L"ShowWindow");
        }
        host.selected = std::min(host.selected, host.items.size() - 1); Focus(host);
    }
    #include "NativeMenuHelp.hpp"

    inline void Activate(Host& host, int direction)
    {
        if (host.selected >= host.items.size()) return;
        auto item = host.items[host.selected];
        if (!item.link) model.Change(item.row, direction, host.pause);
        else if (!direction)
        {
            if (item.link == -1)
            {
                if (host.page == SIZE_MAX) { Close(host); return; }
                model.PageShown(host.page); host.page = SIZE_MAX; host.rowStart = host.selected = 0;
            }
            else if (item.link == -2)
            {
                auto& start = host.page == SIZE_MAX ? host.rootStart : host.rowStart;
                const auto step = host.page == SIZE_MAX ? RootLinks : ContentRows;
                start = start >= step ? start - step : 0; host.selected = 0;
            }
            else if (item.link == -3)
            {
                (host.page == SIZE_MAX ? host.rootStart : host.rowStart) += host.page == SIZE_MAX ? RootLinks : ContentRows; host.selected = 0;
            }
            else if (item.link == -4)
            {
                BeginDetails(host);
                return;
            }
            else
            {
                host.page = size_t(item.link - 1); host.rowStart = host.selected = 0; model.PageShown(host.page);
            }
        }
        Populate(host);
    }
    inline bool BuildPanel(Host& host)
    {
        if (host.panel) return true;
        auto cls = findObject(nullptr, nullptr, L"UWindow.UWindowDialogClientWindow", 0);
        auto buttonClass = findObject(nullptr, nullptr, L"EchelonMenus.EPCTextButton", 0);
        auto area = Object(host.parent, L"m_ControlsArea");
        float x, y, w, h;
        if (!cls || !buttonClass || !Get(area, L"WinLeft", L"FloatProperty", x) || !Get(area, L"WinTop", L"FloatProperty", y) ||
            !Get(area, L"WinWidth", L"FloatProperty", w) || !Get(area, L"WinHeight", L"FloatProperty", h) || w < 300 || h < 140) return false;
        auto panel = Create(host.parent, false, cls, x, y, w, h);
        if (!panel) return false;
        int font = 0; uint8_t align = 0;
        Get(host.sample, L"Font", L"IntProperty", font); Get(host.sample, L"Align", L"ByteProperty", align);
        auto control = [&](float top, float left = 0, float width = 0)
        {
            auto result = Create(panel, true, buttonClass, left, top, width ? width : w, 18);
            if (result && !ControlContract(result)) return static_cast<void*>(nullptr);
            if (result) { Set(result, L"Font", L"IntProperty", font); Set(result, L"Align", L"ByteProperty", align); }
            return result;
        };
        auto title = control(0);
        auto help = control(h - 18);
        std::vector<void*> buttons, values;
        for (size_t i = 0; i < PageRows + 1; ++i)
        {
            auto top = 22.0f + float(i) * 19;
            buttons.push_back(control(top));
            values.push_back(control(top, w * 0.60f, w * 0.40f));
            for (auto row : { buttons.back(), values.back() })
            {
                if (!row) continue;
                bool noKeyboard = false; Bool(row, L"bNoKeyboard", &noKeyboard);
                Invoke(row, L"SetAcceptsFocus");
            }
            if (values.back()) Invoke(values.back(), L"HideWindow");
        }
        if (!title || !help || std::find(buttons.begin(), buttons.end(), nullptr) != buttons.end() || std::find(values.begin(), values.end(), nullptr) != values.end()) { Invoke(panel, L"HideWindow"); return false; }
        bool disabled = true; Bool(title, L"bDisabled", &disabled); Bool(help, L"bDisabled", &disabled);
        host.panel = panel; host.title = title; host.help = help; host.buttons = std::move(buttons); host.values = std::move(values); host.rowWidth = w;
        return true;
    }
    struct JoinedControls
    {
        void* area = nullptr, * list = nullptr;
        RWGTAMenu::Model model;
        std::vector<void*> buttons, items;
        ULONGLONG refreshed = 0;
        bool retired = false;
    };
    inline const std::set<std::string> ControlSettings{ "MAIN.RawInputMouse", "MAIN.MouseSensitivity", "MAIN.RawInputMouseRawData" };
    inline std::map<void*, std::shared_ptr<JoinedControls>> joinedControls;
    inline void RefreshControls(const std::shared_ptr<JoinedControls>& joined)
    {
        if (!joined || joined->retired) return;
        auto& rows = joined->model.JoinedRows();
        for (size_t i = 0; i < joined->buttons.size() && i < rows.size(); ++i)
        {
            auto value = joined->model.Value(rows[i]);
            if (joined->retired) return;
            Label(joined->buttons[i], value);
        }
        if (joined->items.size() > joined->buttons.size())
            String(joined->items.back(), L"Caption", joined->model.RestartRequired() ? joined->model.Text("menu.restartwarning") : "");
        joined->refreshed = GetTickCount64();
    }
    inline bool JoinControls(void* area)
    {
        if (joinedControls.contains(area)) return true;
        auto list = Object(area, L"m_ListBox"), sample = Object(area, L"m_InvertMouseButton");
        auto items = Object(list, L"Items");
        void* itemClass = nullptr; Get(list, L"ListClass", L"ClassProperty", itemClass);
        auto buttonClass = findObject(nullptr, nullptr, L"EchelonMenus.EPCTextButton", 0);
        if (!list || !items || !itemClass || !sample || !buttonClass || !Call(items, L"Append").function) return false;
        auto joined = std::make_shared<JoinedControls>(); joined->area = area; joined->list = list;
        struct Cleanup
        {
            std::shared_ptr<JoinedControls> joined;
            bool committed = false;
            ~Cleanup()
            {
                if (committed) return;
                joined->retired = true;
                for (auto item : joined->items) Invoke(item, L"Remove");
                for (auto button : joined->buttons) Invoke(button, L"HideWindow");
                joined->model.Close();
            }
        } cleanup{ joined };
        joined->model.OpenPlugins(hostModule, Language(), 4096, ControlSettings);
        if (joined->model.JoinedRows().size() != ControlSettings.size()) return false;
        int font = 0; Get(sample, L"Font", L"IntProperty", font);
        for (const auto& row : joined->model.JoinedRows())
        {
            auto button = Create(area, true, buttonClass, 0, 0, 150, 18);
            if (!button) return false;
            joined->buttons.push_back(button);
            if (!ControlContract(button)) return false;
            Set(button, L"Font", L"IntProperty", font);
            uint8_t align = 0; Set(button, L"Align", L"ByteProperty", align);
            bool noKeyboard = false; Bool(button, L"bNoKeyboard", &noKeyboard); Invoke(button, L"SetAcceptsFocus");
            Invoke(button, L"HideWindow");
            Call append(items, L"Append");
            if (!append.Put(L"C", L"ClassProperty", itemClass) || !append.Run()) return false;
            auto item = append.Result();
            if (item) joined->items.push_back(item);
            bool nonselectable = true;
            if (!item || !Set(item, L"m_Control", L"ObjectProperty", button) ||
                !Property(item, L"m_bIsNotSelectable", L"BoolProperty", 4) ||
                !String(item, L"Caption", joined->model.Label(row))) return false;
            Bool(item, L"m_bIsNotSelectable", &nonselectable);
        }
        Call append(items, L"Append");
        if (!append.Put(L"C", L"ClassProperty", itemClass) || !append.Run()) return false;
        auto notice = append.Result();
        if (!notice) return false;
        joined->items.push_back(notice);
        bool nonselectable = true; Bool(notice, L"m_bIsNotSelectable", &nonselectable);
        joinedControls[area] = joined; cleanup.committed = true; RefreshControls(joined);
        return true;
    }
    struct JoinedVideo
    {
        void* area = nullptr;
        void* label = nullptr;
        void* combo = nullptr;
        std::string value;
        ULONGLONG refreshed = 0;
        bool retired = false;
        bool notice = false;
    };
    inline const std::set<std::string> VideoSettings{ "MAIN.WindowMode" };
    inline std::map<void*, std::shared_ptr<JoinedVideo>> joinedVideo;

    inline std::string UpperText(const std::string& text)
    {
        auto wide = Wide(text);
        auto locale = Wide(Language());
        const int count = LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), int(wide.size()), nullptr, 0, nullptr, nullptr, 0);
        if (count > 0)
        {
            std::wstring upper(count, L'\0');
            if (LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), int(wide.size()), upper.data(), count, nullptr, nullptr, 0))
                wide = std::move(upper);
        }
        const int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.data(), int(wide.size()), nullptr, 0, nullptr, nullptr);
        std::string result(bytes, '\0');
        if (bytes)
            WideCharToMultiByte(CP_UTF8, 0, wide.data(), int(wide.size()), result.data(), bytes, nullptr, nullptr);
        return result;
    }

    inline bool VideoDialogContract(void* area)
    {
        auto popup = Object(Object(area, L"Root"), L"m_MessageBoxCW");
        if (!popup)
        {
            LogOnce("graphics", area, "native restart popup is not ready yet");
            return false;
        }
        Call message(popup, L"CreateMessageBox");
        return message.Param(L"_NotifyWindow", L"ObjectProperty", 4) &&
            message.Param(L"Title", L"StrProperty", 12) && message.Param(L"Message", L"StrProperty", 12) &&
            message.Param(L"Buttons", L"ByteProperty", 1) && message.Param(L"ESCResult", L"ByteProperty", 1) &&
            message.Param(L"EnterResult", L"ByteProperty", 1);
    }

    inline void RefreshVideo(const std::shared_ptr<JoinedVideo>& joined)
    {
        if (!joined || joined->retired) return;
        auto wanted = FixSettings().Wanted("MAIN.WindowMode");
        if (wanted != "0" && wanted != "1" && wanted != "2") return;
        if (joined->value != wanted)
        {
            Call select(joined->combo, L"SetSelectedIndex");
            if (!select.Put(L"Index", L"IntProperty", wanted[0] - '0') || !select.Run()) return;
            joined->value = std::move(wanted);
        }
        joined->refreshed = GetTickCount64();
    }

    inline void VideoNotice(const std::shared_ptr<JoinedVideo>& joined)
    {
        if (!joined || joined->retired || !joined->notice || Bool(joined->combo, L"bListVisible")) return;
        joined->notice = false;
        auto popup = Object(Object(joined->area, L"Root"), L"m_MessageBoxCW");
        Call message(popup, L"CreateMessageBox");
        if (!message.Put(L"_NotifyWindow", L"ObjectProperty", joined->area) ||
            !message.PutText(L"Title", UpperText(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"))) ||
            !message.PutText(L"Message", RWGTAMenu::PluginText(FixSettings(), Language(), "menu.restartwarning")) ||
            !message.Put(L"Buttons", L"ByteProperty", uint8_t(2)) ||
            !message.Put(L"ESCResult", L"ByteProperty", uint8_t(3)) ||
            !message.Put(L"EnterResult", L"ByteProperty", uint8_t(3)) || !message.Run())
            LogOnce("graphics", joined->area, "native restart notice unavailable");
    }

    inline bool JoinVideo(void* area)
    {
        if (joinedVideo.contains(area)) return true;
        auto resolution = Object(area, L"m_ComboResolution");
        auto resolutionLabel = Object(area, L"m_LResolution");
        auto comboClass = findObject(nullptr, nullptr, L"EchelonMenus.EPCComboControl", 0);
        auto labelClass = findObject(nullptr, nullptr, L"UWindow.UWindowLabelControl", 0);
        float x = 0, y = 0, width = 0, height = 0, labelX = 0, labelWidth = 0, areaHeight = 0;
        int spacing = 0, font = 0;
        uint8_t align = 0;
        uint32_t color = 0;
        if (!comboClass || !labelClass || !Type(resolution, L"EPCComboControl") ||
            !Type(resolutionLabel, L"UWindowLabelControl") || !VideoDialogContract(area) ||
            !Get(resolution, L"WinLeft", L"FloatProperty", x) || !Get(resolution, L"WinTop", L"FloatProperty", y) ||
            !Get(resolution, L"WinWidth", L"FloatProperty", width) || !Get(resolution, L"WinHeight", L"FloatProperty", height) ||
            !Get(resolutionLabel, L"WinLeft", L"FloatProperty", labelX) || !Get(resolutionLabel, L"WinWidth", L"FloatProperty", labelWidth) ||
            !Get(resolutionLabel, L"Font", L"IntProperty", font) || !Get(resolutionLabel, L"Align", L"ByteProperty", align) ||
            !Get(resolutionLabel, L"TextColor", L"StructProperty", color) ||
            !Get(area, L"WinHeight", L"FloatProperty", areaHeight) || !Get(area, L"m_IYOffset", L"IntProperty", spacing) ||
            width < 120 || labelWidth < 150 || height < 12 || spacing < height || spacing > 40)
            return false;
        std::vector<const wchar_t*> controls{
            L"m_LEffectsQuality", L"m_ComboEffectsQuality", L"m_LGamma", L"m_GammaScroll", L"m_LBrightness", L"m_BrightnessScroll"
        };
        bool shadow = false;
        for (const auto& names : { std::pair{ L"m_LShadowResolution", L"m_ComboShadowResolution" },
            std::pair{ L"m_LShadow", L"m_ComboShadow" } })
        {
            auto label = Object(area, names.first);
            auto combo = Object(area, names.second);
            if (!label && !combo)
                continue;
            if (!label || !combo)
                return false;
            controls.push_back(names.first);
            controls.push_back(names.second);
            shadow = true;
        }
        if (!shadow)
            return false;
        std::vector<std::pair<void*, float>> moved;
        float required = y + spacing + height;
        for (auto name : controls)
        {
            auto control = Object(area, name);
            float top = 0, h = 0;
            if (!control || !Get(control, L"WinTop", L"FloatProperty", top) || !Get(control, L"WinHeight", L"FloatProperty", h) ||
                !std::isfinite(top) || !std::isfinite(h) || top < y + spacing - 0.5f || h <= 0)
                return false;
            moved.emplace_back(control, top);
            required = std::max(required, top + spacing + h);
        }
        required += 2;
        float allowed = areaHeight;
        int resetTop = 0;
        if (Get(area, L"m_IResetToDefaultYPos", L"IntProperty", resetTop))
        {
            if (required > resetTop - 3) return false;
        }
        else if (required > areaHeight)
        {
            float areaTop = 0;
            int buttonsTop = 0;
            auto parent = Object(area, L"ParentWindow");
            if (!Get(area, L"WinTop", L"FloatProperty", areaTop) ||
                !Get(parent, L"m_IMainButtonsYPos", L"IntProperty", buttonsTop)) return false;
            allowed = buttonsTop - areaTop - 3;
        }
        if (!std::isfinite(required) || required > allowed) return false;
        auto joined = std::make_shared<JoinedVideo>();
        joined->area = area;
        struct Cleanup
        {
            std::shared_ptr<JoinedVideo> joined;
            const std::vector<std::pair<void*, float>>& moved;
            float height;
            bool committed = false;
            ~Cleanup()
            {
                if (committed) return;
                joined->retired = true;
                Invoke(joined->combo, L"HideWindow");
                Invoke(joined->label, L"HideWindow");
                for (const auto& [control, top] : moved) Set(control, L"WinTop", L"FloatProperty", top);
                Set(joined->area, L"WinHeight", L"FloatProperty", height);
            }
        } cleanup{ joined, moved, areaHeight };
        joined->label = Create(area, false, labelClass, labelX, y + spacing, labelWidth, height);
        joined->combo = Create(area, true, comboClass, x, y + spacing, width, height);
        if (!joined->label || !joined->combo || !Type(joined->label, L"UWindowLabelControl") || !Type(joined->combo, L"EPCComboControl") ||
            !String(joined->label, L"Text", RWGTAMenu::PluginText(FixSettings(), Language(), "WindowMode")) ||
            !Set(joined->label, L"Font", L"IntProperty", font) || !Set(joined->label, L"Align", L"ByteProperty", align) ||
            !Set(joined->label, L"TextColor", L"StructProperty", color)) return false;
        Call setFont(joined->combo, L"SetFont");
        Call editable(joined->combo, L"SetEditable");
        if (!setFont.Put(L"NewFont", L"IntProperty", font) || !setFont.Run() ||
            !editable.Put(L"bNewCanEdit", L"BoolProperty", uint32_t(0)) || !editable.Run()) return false;
        for (int i = 0; i < 3; ++i)
        {
            Call add(joined->combo, L"AddItem");
            const auto key = "WindowMode." + std::to_string(i);
            if (!add.PutText(L"S", RWGTAMenu::PluginText(FixSettings(), Language(), key)) || !add.Run()) return false;
        }
        Call select(joined->combo, L"SetSelectedIndex");
        Call selected(joined->combo, L"GetSelectedIndex");
        if (!select.Param(L"Index", L"IntProperty", 4) || !selected.Param(L"ReturnValue", L"IntProperty", 4)) return false;
        for (const auto& [control, top] : moved)
            if (!Set(control, L"WinTop", L"FloatProperty", top + spacing)) return false;
        if (required > areaHeight && !Set(area, L"WinHeight", L"FloatProperty", required)) return false;
        joinedVideo[area] = joined;
        RefreshVideo(joined);
        if (joined->value.empty())
        {
            joinedVideo.erase(area);
            return false;
        }
        cleanup.committed = true;
        return true;
    }

    inline std::set<std::string> JoinedSettings(const Host& host)
    {
        std::set<std::string> result;
        if (joinedControls.contains(Object(host.parent, L"m_ControlsArea")))
            result.insert(ControlSettings.begin(), ControlSettings.end());
        auto graphics = Object(host.parent, L"m_GraphicArea");
        if (Type(Object(graphics, L"m_ComboResolution"), L"EPCComboControl"))
        {
            result.insert("MAIN.ResX");
            result.insert("MAIN.ResY");
        }
        if (joinedVideo.contains(graphics))
            result.insert(VideoSettings.begin(), VideoSettings.end());
        return result;
    }

    inline void Open(Host& host)
    {
        if (!BuildPanel(host)) { LogOnce("open", host.parent, "native panel/control creation failed"); return; }
        if (auto graphics = Object(host.parent, L"m_GraphicArea"))
        {
            JoinResolutions(graphics);
            JoinVideo(graphics);
        }
        if (active && active != &host) Close(*active);
        model.OpenPlugins(hostModule, Language(), 4096, JoinedSettings(host));
        active = &host; host.details = false; host.helpRow.reset(); host.detailsLines.clear(); host.page = SIZE_MAX; host.rootStart = host.rowStart = host.selected = 0;
        host.savedTab = Object(host.parent, L"m_Controls");
        for (auto name : { L"m_Controls", L"m_Graphics", L"m_Sounds" })
            if (auto tab = Object(host.parent, name); tab && Bool(tab, L"m_bSelected")) host.savedTab = tab;
        bool selected = false;
        for (auto name : { L"m_Controls", L"m_Graphics", L"m_Sounds" }) Bool(Object(host.parent, name), L"m_bSelected", &selected);
        selected = true; Bool(host.entry, L"m_bSelected", &selected);
        for (auto name : { L"m_ControlsArea", L"m_GraphicArea", L"m_SoundsArea" })
            Invoke(Object(host.parent, name), L"HideWindow");
        if (auto reset = Object(host.parent, L"m_ResetToDefault")) Invoke(reset, L"HideWindow");
        host.open = true; Populate(host);
        Invoke(host.panel, L"ShowWindow"); Invoke(host.panel, L"BringToFront"); Focus(host);
    }
    inline bool Prepare(void* parent, bool pause)
    {
        if (hosts.contains(parent)) return true;
        void* buttonClass = findObject(nullptr, nullptr, L"EchelonMenus.EPCTextButton", 0);
        void* panelClass = findObject(nullptr, nullptr, L"UWindow.UWindowDialogClientWindow", 0);
        auto sample = Object(parent, L"m_Controls"), graphics = Object(parent, L"m_Graphics"), sounds = Object(parent, L"m_Sounds");
        auto area = Object(parent, L"m_ControlsArea");
        float x, y, width, height, ax, ay, aw, ah, right, soundWidth;
        if (!buttonClass || !panelClass || !Type(buttonClass, L"Class") || !Type(panelClass, L"Class") ||
            !sample || !graphics || !sounds || !area || !ControlContract(sample) || !ControlContract(graphics) || !ControlContract(sounds) ||
            !Get(sample, L"WinLeft", L"FloatProperty", x) || !Get(sample, L"WinTop", L"FloatProperty", y) ||
            !Get(sample, L"WinWidth", L"FloatProperty", width) || !Get(sample, L"WinHeight", L"FloatProperty", height) ||
            !Get(sounds, L"WinLeft", L"FloatProperty", right) || !Get(sounds, L"WinWidth", L"FloatProperty", soundWidth) ||
            !Get(area, L"WinLeft", L"FloatProperty", ax) || !Get(area, L"WinTop", L"FloatProperty", ay) ||
            !Get(area, L"WinWidth", L"FloatProperty", aw) || !Get(area, L"WinHeight", L"FloatProperty", ah) ||
            width < 100 || aw < 300 || ah < 140 || height < 12 || right <= x)
        { LogOnce("prepare", parent, "control/property/geometry contract unavailable"); return false; }
        // Resolve contracts before allocating any controls. These functions are
        // script-owned and their live parameter names/types are checked by Create.
        if (!Call(parent, L"CreateControl").function || !Call(parent, L"CreateWindow").function ||
            !Call(parent, L"ChangeTopButtonSelection").function ||
            !Property(sample, L"Text", L"StrProperty", 12) || !Property(sample, L"Font", L"IntProperty", 4) ||
            !Property(sample, L"Align", L"ByteProperty", 1))
        { LogOnce("prepare", parent, "required reflected creation/tab/text contract unavailable"); return false; }
        Host host; host.parent = parent; host.sample = sample; host.pause = pause;
        auto newWidth = (right + soundWidth - x - 15.0f) / 4.0f;
        host.entry = Create(parent, true, buttonClass, x + 3 * (newWidth + 5), y, newWidth, height);
        if (!host.entry || !ControlContract(host.entry))
        {
            if (host.entry) Invoke(host.entry, L"HideWindow");
            return false;
        }
        int font = 0; uint8_t align = 0;
        Get(sample, L"Font", L"IntProperty", font); Get(sample, L"Align", L"ByteProperty", align);
        Set(host.entry, L"Font", L"IntProperty", font); Set(host.entry, L"Align", L"ByteProperty", align);
        Label(host.entry, RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"));
        int i = 0;
        for (auto tab : { sample, graphics, sounds })
        {
            Set(tab, L"WinLeft", L"FloatProperty", x + i++ * (newWidth + 5));
            Set(tab, L"WinWidth", L"FloatProperty", newWidth);
        }
        hosts.emplace(parent, std::move(host));
        return true;
    }
    inline bool Locals(void* frame, const wchar_t* name, const wchar_t* kind, size_t size, void* output)
    {
        auto function = Read<void*>(frame, 4), locals = Read<void*>(frame, 16);
        auto prop = Field(function, name, kind);
        if (!locals || !prop || Read<uint16_t>(prop, 58) != size ||
            size_t(Read<uint16_t>(prop, 68)) + size > Read<uint16_t>(function, 96)) return false;
        std::memcpy(output, Address(locals, prop), size); return true;
    }
    inline void __fastcall Internal(void* object, void*, void* frame, void* result)
    {
        if (busy) { internalHook.thiscall<void>(object, frame, result); return; }
        auto function = Read<void*>(frame, 4);
        auto [cached, inserted] = functionNames.try_emplace(function);
        if (inserted) cached->second = Leaf(function);
        const auto& leaf = cached->second;
        if (active && active->open && leaf == L"KeyEvent" && Type(object, L"EPCConsole") && result)
        {
            uint8_t key = 0, action = 0;
            auto returned = Field(function, L"ReturnValue", L"BoolProperty");
            if (returned && Read<uint16_t>(returned, 58) == 4 &&
                Locals(frame, L"Key", L"ByteProperty", 1, &key) && (key == VK_ESCAPE || key == VK_F1) &&
                Locals(frame, L"Action", L"ByteProperty", 1, &action))
            {
                // Consume the console event before its native close and key
                // routes can both handle the same Escape press.
                if (action == 1)
                {
                    auto& host = *active;
                    busy = true;
                    if (key == VK_F1)
                    {
                        if (host.details) EndDetails(host); else BeginDetails(host);
                    }
                    else if (host.details)
                        EndDetails(host);
                    else if (host.page == SIZE_MAX)
                        Close(host);
                    else
                    {
                        model.PageShown(host.page);
                        host.page = SIZE_MAX;
                        host.rowStart = host.selected = 0;
                        Populate(host);
                    }
                    busy = false;
                }
                ProtectedGameRef<uint32_t>{ static_cast<uint32_t*>(result) } = 1;
                return;
            }
        }
        auto video = joinedVideo.find(object);
        if (video != joinedVideo.end() && leaf == L"Notify")
        {
            auto joined = video->second;
            void* control = nullptr;
            uint8_t event = 0;
            if (Locals(frame, L"C", L"ObjectProperty", 4, &control) && Locals(frame, L"E", L"ByteProperty", 1, &event) &&
                event == 1 && control == joined->combo)
            {
                busy = true;
                Call selected(joined->combo, L"GetSelectedIndex");
                auto value = static_cast<int*>(selected.Param(L"ReturnValue", L"IntProperty", 4));
                if (value && selected.Run() && *value >= 0 && *value <= 2)
                {
                    auto wanted = std::to_string(*value);
                    if (wanted != FixSettings().Wanted("MAIN.WindowMode") && FixSettings().Set("MAIN.WindowMode", wanted, true) == ASI_SETTINGS_OK)
                        joined->notice = true;
                }
                RefreshVideo(joined);
                busy = false;
                return;
            }
        }
        auto joinedIt = joinedControls.find(object);
        if (joinedIt != joinedControls.end() && leaf == L"Notify")
        {
            auto joined = joinedIt->second;
            void* control = nullptr; uint8_t event = 0;
            if (Locals(frame, L"C", L"ObjectProperty", 4, &control) && Locals(frame, L"E", L"ByteProperty", 1, &event) && event == 2)
            {
                auto button = std::find(joined->buttons.begin(), joined->buttons.end(), control);
                if (button != joined->buttons.end())
                {
                    auto index = size_t(button - joined->buttons.begin());
                    busy = true; joined->model.Change(joined->model.JoinedRows()[index], 0, false); RefreshControls(joined); busy = false; return;
                }
            }
        }
        if (leaf == L"KeyDown")
        {
            std::shared_ptr<JoinedControls> joined;
            size_t index = 0;
            for (const auto& [area, candidate] : joinedControls)
            {
                auto button = std::find(candidate->buttons.begin(), candidate->buttons.end(), object);
                if (button != candidate->buttons.end()) { joined = candidate; index = size_t(button - candidate->buttons.begin()); break; }
            }
            int key = 0;
            if (joined && Locals(frame, L"Key", L"IntProperty", 4, &key) && (key == VK_LEFT || key == VK_RIGHT || key == VK_RETURN))
            {
                busy = true; joined->model.Change(joined->model.JoinedRows()[index], key == VK_LEFT ? -1 : key == VK_RIGHT ? 1 : 0, false);
                RefreshControls(joined); busy = false; return;
            }
        }
        auto it = hosts.find(object);
        if (it != hosts.end())
        {
            auto& host = it->second;
            if (host.open && leaf == L"EscapeMenu")
            {
                busy = true;
                if (host.details) EndDetails(host);
                else if (host.page == SIZE_MAX) Close(host);
                else { host.page = SIZE_MAX; host.selected = 0; Populate(host); }
                busy = false; return;
            }
            if (leaf == L"Notify")
            {
                void* control = nullptr; uint8_t event = 0;
                if (Locals(frame, L"C", L"ObjectProperty", 4, &control) && Locals(frame, L"E", L"ByteProperty", 1, &event) && event == 2)
                {
                    if (control == host.entry)
                    { busy = true; if (host.open) Close(host); else Open(host); busy = false; return; }
                    if (host.open) Close(host, false); // vanilla tabs/main-menu keep their own Notify/confirmation
                }
            }
            else if (host.open && (leaf == L"HideWindow" || leaf == L"Close" || leaf == L"WindowHidden")) Close(host, false);
        }
        if (active && active->open)
        {
            auto& host = *active;
            auto row = std::find(host.buttons.begin(), host.buttons.end(), object);
            auto valueRow = std::find(host.values.begin(), host.values.end(), object);
            auto index = row != host.buttons.end() ? size_t(row - host.buttons.begin()) :
                valueRow != host.values.end() ? size_t(valueRow - host.values.begin()) : SIZE_MAX;
            if (object == host.panel && leaf == L"EscClose")
            {
                busy = true;
                if (host.details) EndDetails(host);
                else if (host.page == SIZE_MAX) Close(host);
                else { host.page = SIZE_MAX; host.selected = 0; Populate(host); }
                busy = false; return;
            }
            if (object == host.panel && (leaf == L"Close" || leaf == L"HideWindow" || leaf == L"WindowHidden"))
            {
                // Parent/level teardown still executes the engine operation.
                // Only the private model state is discarded here.
                host.open = false; active = nullptr; model.Close();
            }
            if (object == host.panel && leaf == L"Notify")
            {
                void* control = nullptr; uint8_t event = 0;
                if (Locals(frame, L"C", L"ObjectProperty", 4, &control) && Locals(frame, L"E", L"ByteProperty", 1, &event) && event == 2)
                {
                    if (host.details)
                    {
                        busy = true;
                        if (control == host.buttons[PageRows]) EndDetails(host);
                        else if (control == host.values[PageRows]) DetailsStep(host, 1);
                        busy = false;
                        return;
                    }
                    auto at = std::find(host.buttons.begin(), host.buttons.end(), control);
                    auto valueAt = std::find(host.values.begin(), host.values.end(), control);
                    auto selected = at != host.buttons.end() ? size_t(at - host.buttons.begin()) :
                        valueAt != host.values.end() ? size_t(valueAt - host.values.begin()) : SIZE_MAX;
                    if (selected != SIZE_MAX) { host.selected = selected; busy = true; Activate(host, 0); busy = false; return; }
                }
            }
            if (index != SIZE_MAX && leaf == L"MouseEnter")
            {
                if (host.details)
                {
                    if (index == PageRows) SelectDetailsNavigation(host, valueRow != host.values.end());
                }
                else { host.selected = index; Focus(host); }
            }
            if ((index != SIZE_MAX || object == host.panel) && leaf == L"KeyDown")
            {
                int key = 0;
                if (Locals(frame, L"Key", L"IntProperty", 4, &key))
                {
                    auto count = host.items.size();
                    if (!count) { internalHook.thiscall<void>(object, frame, result); return; }
                    busy = true;
                    if (host.details)
                    {
                        const bool handled = DetailsKey(host, key);
                        busy = false;
                        if (!handled) internalHook.thiscall<void>(object, frame, result);
                        return;
                    }
                    if (key == VK_F1) BeginDetails(host);
                    else if (key == VK_UP) host.selected = (host.selected + count - 1) % count;
                    else if (key == VK_DOWN) host.selected = (host.selected + 1) % count;
                    else if (key == VK_LEFT) Activate(host, -1);
                    else if (key == VK_RIGHT) Activate(host, 1);
                    else if (key == VK_RETURN) Activate(host, 0);
                    else if (key == VK_ESCAPE) { if (host.page == SIZE_MAX) Close(host); else { host.page = SIZE_MAX; host.selected = 0; Populate(host); } }
                    else { busy = false; internalHook.thiscall<void>(object, frame, result); return; }
                    if (host.open) Focus(host); busy = false; return;
                }
            }
            if (leaf == L"Tick" && RWGTAMenu::Model::Changed().exchange(false)) { busy = true; Populate(host); busy = false; }
        }
        internalHook.thiscall<void>(object, frame, result);
        if (leaf == L"BeforePaint" && captions.contains(object))
        {
            void* canvas = nullptr;
            if (Locals(frame, L"C", L"ObjectProperty", 4, &canvas)) FitLabel(object, canvas);
        }
        if (leaf == L"HideControls")
        {
            for (const auto& [area, joined] : joinedControls)
                if (object == joined->list)
                {
                    busy = true;
                    for (auto button : joined->buttons) Invoke(button, L"HideWindow");
                    if (GetTickCount64() - joined->refreshed >= 100) RefreshControls(joined);
                    busy = false; break;
                }
        }
        if (leaf == L"ResetToDefault" && joinedControls.contains(object))
        {
            auto joined = joinedControls.at(object);
            busy = true; RWGTAMenu::RestoreDefaults(FixSettings(), ControlSettings); RefreshControls(joined); busy = false;
        }
        if (auto currentVideo = joinedVideo.find(object); currentVideo != joinedVideo.end())
        {
            auto joined = currentVideo->second;
            if (leaf == L"ResetToDefault")
            {
                busy = true;
                joined->notice |= RWGTAMenu::RestoreDefaults(FixSettings(), VideoSettings);
                RefreshVideo(joined);
                busy = false;
            }
            else if (leaf == L"Tick")
            {
                busy = true;
                if (GetTickCount64() - joined->refreshed >= 100) RefreshVideo(joined);
                VideoNotice(joined);
                busy = false;
            }
        }
        if (leaf == L"ShowWindow" && !joinedVideo.contains(object))
        {
            const auto type = Name(object);
            if (type.starts_with(L"EPCVideoConfigArea ") || type.starts_with(L"EPCInGameVideoConfigArea "))
            {
                busy = true;
                JoinResolutions(object);
                JoinVideo(object);
                busy = false;
            }
        }
        if (leaf == L"Created")
        {
            auto type = Name(object);
            if (type.starts_with(L"EPCVideoConfigArea ") || type.starts_with(L"EPCInGameVideoConfigArea "))
            {
                busy = true;
                if (!JoinResolutions(object)) LogOnce("resolution", object, "all-mode native resolution integration unavailable");
                if (!JoinVideo(object)) LogOnce("graphics", object, "native resolution/window-mode integration unavailable");
                busy = false;
            }
            if (type.starts_with(L"EPCControlsConfigArea ") || type.starts_with(L"EPCInGameControlsConfigArea "))
            {
                busy = true; if (!JoinControls(object)) LogOnce("controls", object, "native mouse list integration unavailable"); busy = false;
            }
            bool pause = type.starts_with(L"EPCInGameSettingsArea ");
            if (pause || type.starts_with(L"EPCOptionsMenu "))
            { busy = true; Prepare(object, pause); busy = false; }
        }
    }
    inline int __fastcall Destroy(void* object, void*)
    {
        ForgetResolution(object);
        for (auto it = joinedVideo.begin(); it != joinedVideo.end();)
        {
            auto joined = it->second;
            if (object == joined->area || object == joined->label || object == joined->combo)
            {
                joined->retired = true;
                it = joinedVideo.erase(it);
            }
            else ++it;
        }
        for (auto it = joinedControls.begin(); it != joinedControls.end();)
        {
            auto joined = it->second;
            if (object == joined->area || object == joined->list || std::find(joined->buttons.begin(), joined->buttons.end(), object) != joined->buttons.end() ||
                std::find(joined->items.begin(), joined->items.end(), object) != joined->items.end())
            { joined->retired = true; joined->model.Close(); it = joinedControls.erase(it); }
            else ++it;
        }
        // ConditionalDestroy precedes native destruction/GC; C++ pointers are
        // weak observations, never extra UObject roots or VM-owned allocations.
        for (auto it = hosts.begin(); it != hosts.end();)
        {
            auto& host = it->second;
            if (object == host.parent || object == host.entry)
            {
                if (active == &host) { active = nullptr; model.Close(); }
                it = hosts.erase(it);
            }
            else if (object == host.panel || object == host.title || object == host.help || std::find(host.buttons.begin(), host.buttons.end(), object) != host.buttons.end() ||
                std::find(host.values.begin(), host.values.end(), object) != host.values.end())
            {
                // HideWindow removes the panel from the parent's GC-visible
                // child list. Its entry remains native-owned; reconstruct the
                // panel on a later click if the hidden panel was collected.
                if (active == &host) { active = nullptr; model.Close(); }
                host.open = false; host.details = false; host.helpRow.reset(); host.detailsLines.clear(); host.detailsText.clear();
                host.panel = host.title = host.help = nullptr; host.buttons.clear(); host.values.clear();
                ++it;
            }
            else ++it;
        }
        captions.erase(object);
        functionNames.erase(object);
        return destroyHook.thiscall<int>(object);
    }
    inline void Init()
    {
        auto core = GetModuleHandleW(L"Core.dll");
        if (!core) { LogOnce("init", nullptr, "Core module unavailable"); return; }
        auto resolve = [core](const char* name) { return GetProcAddress(core, name); };
        fullName = reinterpret_cast<decltype(fullName)>(resolve("?GetFullName@UObject@@QBEPBGPAG@Z"));
        findObject = reinterpret_cast<decltype(findObject)>(resolve("?StaticFindObject@UObject@@SAPAV1@PAVUClass@@PAV1@PBGH@Z"));
        processEvent = reinterpret_cast<decltype(processEvent)>(resolve("?ProcessEvent@UObject@@UAEXPAVUFunction@@PAX1@Z"));
        copyString = reinterpret_cast<decltype(copyString)>(resolve("?CopySingleValue@UStrProperty@@UBEXPAX0PAVUObject@@@Z"));
        destroyString = reinterpret_cast<decltype(destroyString)>(resolve("?DestroyValue@UStrProperty@@UBEXPAX@Z"));
        getLanguage = reinterpret_cast<decltype(getLanguage)>(resolve("?GetLanguage@UObject@@SAPBGXZ"));
        auto internal = resolve("?ProcessInternal@UObject@@QAEXAAUFFrame@@QAX@Z");
        auto destroy = resolve("?ConditionalDestroy@UObject@@QAEHXZ");
        if (!fullName || !findObject || !processEvent || !copyString || !destroyString || !getLanguage || !internal || !destroy)
        { LogOnce("init", nullptr, "required Core export unavailable"); return; }
        // Follow export jump stubs to the function bodies before checking them.
        auto body = [](void* value)
        {
            auto p = static_cast<uint8_t*>(value);
            for (int i = 0; p && i < 4 && p[0] == 0xE9; ++i) p += 5 + *reinterpret_cast<int32_t*>(p + 1);
            return p;
        };
        auto frame = hook::module_pattern(core, "81 EC 08 04 00 00 53 55 56 8B B4 24 18 04 00 00 57 8B E9 8B 46 04 8B 4E 10 8B 55 00 6A 00 8B 78 58");
        auto event = hook::module_pattern(core, "55 8B EC 83 EC 24 A1 ? ? ? ? 53 8B D9 56 85 C0 57 89 5D F4 0F 84 84 01 00 00 8B 75 08 8B 46 20");
        auto bit = hook::module_pattern(core, "8B 41 54 8B 4C 24 04 8B D0 56 8B 31 F7 D2 23 D6 8B 74 24 0C 8B 36 23 F0 0B D6 5E 89 11 C2 0C 00");
        auto boolCopy = resolve("?CopySingleValue@UBoolProperty@@UBEXPAX0PAVUObject@@@Z");
        if (frame.size() != 1 || event.size() != 1 || bit.size() != 1 ||
            frame.get_first() != body(internal) || event.get_first() != body(reinterpret_cast<void*>(processEvent)) || bit.get_first() != body(boolCopy))
        { LogOnce("init", nullptr, "Core body/signature layout mismatch"); return; }
        auto engine = GetModuleHandleW(L"Engine.dll");
        textSize = reinterpret_cast<decltype(textSize)>(GetProcAddress(engine, "?TextSize@UCanvas@@UAEXPBGPAM1@Z"));
        auto measure = hook::module_pattern(engine, "83 EC 10 8B 41 48 33 D2 3B C2 89 4C 24 08 0F 84 D7 00 00 00");
        if (!textSize || measure.size() != 1 || measure.get_first() != body(reinterpret_cast<void*>(textSize)))
        { LogOnce("init", nullptr, "native font measurement contract unavailable"); return; }
        if (!InitResolutionApply(engine))
            LogOnce("resolution", nullptr, "native resolution apply contract unavailable; existing picker retained");
        UWindowsViewport::WindowsMenuMouseActive = WindowsMenuMouseActive;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&hostModule), &hostModule);
        destroyHook = safetyhook::create_inline(destroy, Destroy);
        if (!destroyHook) { LogOnce("init", nullptr, "ConditionalDestroy hook failed"); return; }
        internalHook = safetyhook::create_inline(internal, Internal);
        if (!internalHook) LogOnce("init", nullptr, "ProcessInternal hook failed");
    }
}
