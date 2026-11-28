module;

#include <stdafx.h>
#include <stdexcept>
#include "FixSettings.hpp"
#include <set>
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/MenuList.hpp"
#include "Shared/PadInput.hpp"
#include <atomic>

export module NativeMenu;

import ComVars;
import Gamepad;

// Builds the Plugins page from the game's own UWindow controls, found through
// reflection on the installed script package.
export namespace SCMenu
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
    inline void* automaticPadBindingEvent = nullptr;

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
        *static_cast<T*>(Address(object, prop)) = value; return true;
    }
    inline bool Bool(void* object, const wchar_t* name, bool* write = nullptr)
    {
        auto prop = Property(object, name, L"BoolProperty", 4);
        if (!prop) return false;
        auto mask = Read<uint32_t>(prop, 84); // native UBoolProperty::CopySingleValue
        if (!mask || (mask & (mask - 1))) return false;
        auto address = static_cast<uint32_t*>(Address(object, prop));
        if (write) *address = (*address & ~mask) | (*write ? mask : 0);
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
        // Captured software-cursor menus have shown=true, available=false.
        // Both native cursor routes must keep their own movement and picking.
        if (!enabled(show))
            return false;

        // Returning to play leaves bShowWindowsMouse set; the menus are the console's UWindow state (FakeWindow extends it)
        static void* consoleProp = nullptr;
        if (!consoleProp)
            consoleProp = Property(viewport, L"Console", L"ObjectProperty", 4);
        auto console = consoleProp ? Read<void*>(Address(viewport, consoleProp), 0) : nullptr;
        auto frame = console ? Read<void*>(console, 0x0C) : nullptr; // UObject::StateFrame
        if (!frame)
            return false;

        static std::unordered_map<void*, bool> menuStates;
        auto node = Read<void*>(frame, 28); // FStateFrame::StateNode, the class itself outside states
        auto [it, added] = menuStates.try_emplace(node, false);
        if (added)
        {
            for (auto state = node; state; state = Read<void*>(state, 44)) // UStruct::SuperField
            {
                if (Type(state, L"State") && Leaf(state) == L"UWindow")
                {
                    it->second = true;
                    break;
                }
            }
        }
        return it->second;
    }
    inline void __cdecl FocusPause(void* viewport, bool lost)
    {
        static auto setPause = reinterpret_cast<unsigned int (__thiscall*)(void*, unsigned int)>(
            GetProcAddress(GetModuleHandleW(L"Engine.dll"), "?eventSetPause@APlayerController@@QAEKK@Z"));
        static void* ownedPlayer = nullptr;
        void *player = nullptr, *level = nullptr, *pauser = nullptr;
        if (!Get(viewport, L"Actor", L"ObjectProperty", player) || !player ||
            !Get(player, L"Level", L"ObjectProperty", level) || !level ||
            !Get(level, L"Pauser", L"ObjectProperty", pauser))
        {
            ownedPlayer = nullptr;
            return;
        }
        if (lost)
        {
            ownedPlayer = nullptr;
            if (!pauser && setPause && setPause(player, 1)) ownedPlayer = player;
        }
        else
        {
            if (ownedPlayer == player && pauser && setPause) setPause(player, 0);
            ownedPlayer = nullptr;
        }
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
    inline SafetyHookInline resolutionApplyHook;
    struct JoinedResolution
    {
        void* original = nullptr;
        void* combo = nullptr;
    };
    inline std::map<void*, JoinedResolution> joinedResolutions;

    inline bool ResolutionText(void* options, std::wstring& text)
    {
        auto property = Property(options, L"Resolution", L"StrProperty", 12);
        if (!property)
            return false;
        auto value = Address(options, property);
        auto data = Read<const wchar_t*>(value, 0);
        int count = Read<int>(value, 4), capacity = Read<int>(value, 8);
        if (!data || count < 2 || count > 32 || capacity < count || data[count - 1] != L'\0')
            return false;
        text.assign(data, count - 1);
        return true;
    }

    inline void __fastcall ApplyResolution(void* options, void*, void* frame, void* result)
    {
        std::optional<std::pair<int, int>> requested;
        std::wstring value;
        if (NativeResolutionListPublished && ResolutionText(options, value))
        {
            for (const auto& mode : NativeDisplayModes)
            {
                if (value == std::to_wstring(mode.first) + L"x" + std::to_wstring(mode.second))
                {
                    requested = mode;
                    break;
                }
            }
        }
        struct Restore
        {
            std::optional<std::pair<int, int>> previous;
            ~Restore() { RequestedNativeResolution = previous; }
        } restore{ std::exchange(RequestedNativeResolution, requested) };
        // Keep native graphics application, profile writes and confirmation behavior.
        // Its five recognized SETRES aliases are resolved only within this call.
        resolutionApplyHook.thiscall<void>(options, frame, result);
    }

    inline bool InitResolutionApply(HMODULE engine)
    {
        auto entry = GetProcAddress(engine, "?execUpdateEngineSettings@UEPCGameOptions@@QAEXAAUFFrame@@QAX@Z");
        if (!entry)
            return false;
        resolutionApplyHook = safetyhook::create_inline(entry, ApplyResolution);
        return bool(resolutionApplyHook);
    }

    inline bool JoinResolutions(void* area)
    {
        if (joinedResolutions.contains(area))
            return true;
        if (!resolutionApplyHook || NativeDisplayModes.empty())
            return false;
        auto original = Object(area, L"m_ComboResolution");
        auto comboClass = findObject(nullptr, nullptr, L"EchelonMenus.EPCComboControl", 0);
        float x = 0, y = 0, width = 0, height = 0;
        int font = 0;
        if (!comboClass || !Type(original, L"EPCComboControl") ||
            !Property(area, L"m_ComboResolution", L"ObjectProperty", 4) ||
            !Get(original, L"WinLeft", L"FloatProperty", x) || !Get(original, L"WinTop", L"FloatProperty", y) ||
            !Get(original, L"WinWidth", L"FloatProperty", width) || !Get(original, L"WinHeight", L"FloatProperty", height) ||
            !Get(original, L"Font", L"IntProperty", font) || width < 100 || height < 12 ||
            !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height))
            return false;
        auto combo = Create(area, true, comboClass, x, y, width, height);
        if (!combo || !Type(combo, L"EPCComboControl"))
            return false;
        struct Cleanup
        {
            void* combo;
            bool previous = NativeResolutionListPublished;
            bool committed = false;
            ~Cleanup()
            {
                if (!committed)
                {
                    Invoke(combo, L"HideWindow");
                    NativeResolutionListPublished = previous;
                }
            }
        } cleanup{ combo };
        Call setFont(combo, L"SetFont"), editable(combo, L"SetEditable");
        if (!setFont.Put(L"NewFont", L"IntProperty", font) || !setFont.Run() ||
            !editable.Put(L"bNewCanEdit", L"BoolProperty", uint32_t(0)) || !editable.Run())
            return false;
        // Disable the old five-string rewrite before copying any new native item.
        NativeResolutionListPublished = true;
        int selected = -1;
        for (size_t i = 0; i < NativeDisplayModes.size(); ++i)
        {
            const auto& mode = NativeDisplayModes[i];
            Call add(combo, L"AddItem");
            if (!add.PutText(L"S", std::to_string(mode.first) + "x" + std::to_string(mode.second)) || !add.Run())
                return false;
            if (mode.first == Screen.Width && mode.second == Screen.Height)
                selected = int(i);
        }
        Call select(combo, L"SetSelectedIndex");
        if (selected < 0 || !select.Put(L"Index", L"IntProperty", selected) || !select.Run() ||
            !Set(area, L"m_ComboResolution", L"ObjectProperty", combo))
            return false;
        Invoke(original, L"HideWindow");
        joinedResolutions.emplace(area, JoinedResolution{ original, combo });
        cleanup.committed = true;
        return true;
    }

    inline void ForgetResolution(void* object)
    {
        for (auto it = joinedResolutions.begin(); it != joinedResolutions.end();)
        {
            if (object == it->first || object == it->second.original || object == it->second.combo)
                it = joinedResolutions.erase(it);
            else
                ++it;
        }
    }

    struct Caption
    {
        std::wstring requested;
        void* font = nullptr;
        float width = 0, height = 0, spacing = 0;
        bool fitted = false;
    };
    inline std::map<void*, Caption> captions;
    inline std::wstring Upper(std::wstring wide)
    {
        auto locale = Wide(Language());
        int size = LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), int(wide.size()), nullptr, 0, nullptr, nullptr, 0);
        if (size > 0)
        {
            std::wstring upper(size, L'\0');
            if (LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), int(wide.size()), upper.data(), size, nullptr, nullptr, 0)) wide = std::move(upper);
        }
        return wide;
    }
    // A caption fitted to its control when it is drawn (see FitLabel); the button font has capitals only
    inline void Label(void* button, const std::string& text)
    {
        if (!button) return;
        auto wide = Wide(text);
        for (auto& c : wide) if (c < 32 || c == 8232 || c == 8233) c = L' ';
        wide = Upper(std::move(wide));
        auto& caption = captions[button];
        if (caption.requested != wide) { caption.requested = std::move(wide); caption.fitted = false; }
    }
    inline std::string Utf8(const std::wstring& value)
    {
        int count = WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr);
        std::string result(count, '\0');
        if (count) WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()), result.data(), count, nullptr, nullptr);
        return result;
    }
    inline float Measure(void* canvas, const std::wstring& value, float spacing, float height)
    {
        float w = 0, h = 0;
        textSize(canvas, nullptr, value.c_str(), &w, &h);
        if (!std::isfinite(w) || !std::isfinite(h) || h > height || (!value.empty() && w <= 0)) return INFINITY;
        return w + float(value.size()) * spacing;
    }
    // A missing glyph must not silently disappear from a caption
    inline void MarkMissing(void* canvas, std::wstring& value, float height)
    {
        for (auto& c : value)
        {
            if (c == L' ') continue;
            if (!std::isfinite(Measure(canvas, std::wstring(1, c), 0, height))) c = L'?';
        }
    }
    inline std::wstring Ellipsis(void* canvas, std::wstring value, float width, float spacing, float height)
    {
        if (Measure(canvas, value, spacing, height) <= width) return value;
        while (!value.empty() && Measure(canvas, value + L"...", spacing, height) > width)
        {
            value.pop_back();
            if (!value.empty() && value.back() >= 0xD800 && value.back() <= 0xDBFF) value.pop_back();
        }
        return Measure(canvas, value + L"...", spacing, height) <= width ? value + L"..." : L"";
    }
    inline void FitLabel(void* button, void* canvas)
    {
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
        auto value = caption.requested;
        MarkMissing(canvas, value, height);
        value = Ellipsis(canvas, value, width, spacing, height);
        bool previous = busy; busy = true;
        String(button, L"Text", Utf8(value));
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

    // The Plugins page: the game's own key list (EPCOptionKeysListBox, the Controls page's scrolling list with its
    // scroll bar), a row per entry with the value in a text button where the key boxes are; the page's title above
    // it, the selected row's help below it in the Graphics page's label style, and Back in the place of the page's
    // Reset To Default button.
    struct Host
    {
        void* parent = nullptr, * entry = nullptr, * panel = nullptr, * savedTab = nullptr, * sample = nullptr;
        void* title = nullptr, * area = nullptr, * list = nullptr, * back = nullptr;
        std::vector<void*> help, values, items;
        std::vector<const RWGTAMenu::Row*> rows;
        std::vector<bool> wide;
        std::wstring helpText;
        void* helpFont = nullptr;
        float helpWidth = 0, itemHeight = 18, firstOffset = 3;
        size_t page = RWGTAMenu::Model::RootPage, slots = 1;
        float shownPos = -1;
        bool pause = false, open = false, helpDirty = true;
        // Input the page reads itself: held keys and the pad (XInput) repeat at the game's menu rate
        MenuList::Repeat up, down, left, right, pageUp, pageDown, padUp, padDown, padLeft, padRight;
        MenuList::StickAxis stickX, stickY;
        WORD pad = 0;
        bool padSeen = false;
    };
    inline std::map<void*, Host> hosts;
    inline Host* active = nullptr;
    inline RWGTAMenu::Model model;
    inline MenuList::WheelNotches wheel;
    inline std::atomic<int> wheelDelta{ 0 };
    inline constexpr size_t HelpLines = 2;

    inline void* Popup(Host& host)
    {
        return Object(Object(host.parent, L"Root"), L"m_MessageBoxCW");
    }
    // The game's message box is up (a row's help, a notice): it has the keys
    inline bool PopupShown(Host& host)
    {
        auto popup = Popup(host);
        return popup && Bool(popup, L"bWindowVisible");
    }
    inline const RWGTAMenu::Row* Selected(Host& host)
    {
        auto index = model.Window(host.page).Selected();
        return index < host.rows.size() ? host.rows[index] : nullptr;
    }

    // The selected row's help, wrapped to the help lines in the label font (measured when they are drawn)
    inline void WrapHelp(Host& host, void* canvas)
    {
        if (host.help.empty() || !canvas || !textSize) return;
        auto font = Object(canvas, L"Font");
        float width = 0, height = 0;
        if (!font || !Get(host.help[0], L"WinWidth", L"FloatProperty", width) || !Get(host.help[0], L"WinHeight", L"FloatProperty", height)) return;
        width -= 4;
        if (!host.helpDirty && host.helpFont == font && host.helpWidth == width) return;
        std::wstring text = host.helpText;
        MarkMissing(canvas, text, height);
        // Every line of a text wrapped at spaces to the help's width
        auto wrap = [&](const std::wstring& source)
        {
            std::vector<std::wstring> result;
            size_t begin = 0;
            while (begin < source.size())
            {
                while (begin < source.size() && source[begin] == L' ') ++begin;
                if (begin == source.size()) break;
                size_t end = begin, space = std::wstring::npos;
                while (end < source.size())
                {
                    size_t next = end + 1;
                    if (source[end] >= 0xD800 && source[end] <= 0xDBFF && next < source.size()) ++next;
                    if (Measure(canvas, source.substr(begin, next - begin), 0, height) > width) break;
                    if (source[end] == L' ') space = end;
                    end = next;
                }
                if (end == begin) end = begin + 1;
                else if (end < source.size() && space != std::wstring::npos && space > begin) end = space;
                result.push_back(source.substr(begin, end - begin));
                begin = end;
            }
            return result;
        };
        // The whole help when it fits the lines; else its first sentences that do (F1 shows all of it); else the key
        auto lines = wrap(text);
        for (size_t cut = text.size(); lines.size() > HelpLines && cut > 0;)
        {
            cut = text.find_last_of(L".!?", cut - 1);
            if (cut == std::wstring::npos) { lines = wrap(Wide(model.Text("menu.detailskey"))); break; }
            if (cut + 1 < text.size() && text[cut + 1] != L' ') continue;
            lines = wrap(text.substr(0, cut + 1));
        }
        const bool previous = busy; busy = true;
        for (size_t i = 0; i < host.help.size(); ++i)
            String(host.help[i], L"Text", i < lines.size() ? Utf8(lines[i]) : "");
        busy = previous;
        host.helpFont = font;
        host.helpWidth = width;
        host.helpDirty = false;
    }

    // The list's scroll position, the selected row and the help follow the page's window
    inline void Show(Host& host)
    {
        auto& window = model.Window(host.page);
        const bool previous = busy; busy = true;
        for (size_t i = 0; i < host.rows.size() && i < host.items.size(); ++i)
        {
            bool selected = i == window.Selected();
            Bool(host.items[i], L"bSelected", &selected);
            Bool(host.values[i], L"m_bSelected", &selected);
        }
        if (auto bar = Object(host.list, L"VertSB"))
        {
            host.shownPos = float(window.First());
            Set(bar, L"Pos", L"FloatProperty", host.shownPos);
        }
        std::wstring help;
        if (model.RestartRequired())
            help = Wide(model.Text("menu.restartwarning")) + L"  ";
        if (auto row = Selected(host))
            help += Wide(model.Help(*row));
        for (auto& c : help) if (c < 32 || c == 8232 || c == 8233) c = L' ';
        if (help != host.helpText) { host.helpText = std::move(help); host.helpDirty = true; }
        busy = previous;
    }

    // The page's rows into the list; the items are made again only when the rows changed
    inline void Populate(Host& host)
    {
        if (!host.list) return;
        const bool previous = busy; busy = true;
        if (host.page >= model.Pages().size()) host.page = RWGTAMenu::Model::RootPage;
        auto rows = model.ListRows(host.page, false);
        Label(host.title, model.Pages()[host.page].title);
        if (host.back)
        {
            Label(host.back, model.Text("menu.back"));
            Invoke(host.back, L"ShowWindow");
            Invoke(host.back, L"BringToFront");
        }
        auto buttonClass = findObject(nullptr, nullptr, L"EchelonMenus.EPCTextButton", 0);
        void* listClass = nullptr; Get(host.list, L"ListClass", L"ClassProperty", listClass);
        auto items = Object(host.list, L"Items");
        int highlight = 0, highlightPadding = 0;
        Get(host.list, L"m_IHighLightWidth", L"IntProperty", highlight);
        Get(host.list, L"m_IHighLightRightPadding", L"IntProperty", highlightPadding);
        const float valueWidth = float(std::max(60, highlight + highlightPadding - 6));
        int font = 0; Get(host.sample, L"Font", L"IntProperty", font);
        if (rows != host.rows)
        {
            Invoke(host.list, L"Clear");
            host.items.clear();
            for (size_t i = 0; i < rows.size(); ++i)
            {
                if (i == host.values.size())
                {
                    auto value = buttonClass ? Create(host.area, true, buttonClass, 0, 0, valueWidth, host.itemHeight) : nullptr;
                    if (!value || !ControlContract(value)) { if (value) Invoke(value, L"HideWindow"); break; }
                    uint8_t align = 0;
                    Set(value, L"Font", L"IntProperty", font); Set(value, L"Align", L"ByteProperty", align);
                    // The text inset in its bar as in the game's combo boxes
                    Set(value, L"m_fLMarge", L"FloatProperty", 3.0f);
                    String(value, L"ToolTipString", "");
                    Invoke(value, L"HideWindow");
                    host.values.push_back(value);
                }
                Call append(items, L"Append");
                if (!listClass || !append.Put(L"C", L"ClassProperty", listClass) || !append.Run()) break;
                auto item = append.Result();
                if (!item || !Set(item, L"m_Control", L"ObjectProperty", host.values[i])) break;
                // Rows without key boxes, as the page's other settings
                bool noBoxes = true; Bool(item, L"m_bIsNotSelectable", &noBoxes);
                host.items.push_back(item);
            }
            host.rows = rows;
        }
        uint8_t centre = 2; Get(host.sample, L"Align", L"ByteProperty", centre);
        host.wide.assign(host.items.size(), false);
        for (size_t i = 0; i < host.rows.size() && i < host.items.size(); ++i)
        {
            const auto& row = *host.rows[i];
            const bool enabled = model.Enabled(row, host.pause);
            bool disabled = !enabled;
            // Plugins, categories and the page's actions are entries like the menus' own (a button across the row);
            // settings are a caption of the list with their value where the key boxes are
            const bool wide = RWGTAMenu::Model::Opens(row) || row.kind == RWGTAMenu::RowKind::RestoreDefaults ||
                row.kind == RWGTAMenu::RowKind::Action || row.kind == RWGTAMenu::RowKind::SettingsFile;
            host.wide[i] = wide;
            const auto value = enabled ? model.Value(row) : model.Text("menu.unavailable");
            String(host.items[i], L"Caption", wide ? "" : model.Label(row));
            Label(host.values[i], wide ? model.Label(row) + (value.empty() ? "" : "  " + value) : value);
            uint8_t align = wide ? centre : 0;
            Set(host.values[i], L"Align", L"ByteProperty", align);
            Bool(host.values[i], L"bDisabled", &disabled);
            Set(host.values[i], L"WinWidth", L"FloatProperty", valueWidth);
        }
        Show(host);
        busy = previous;
    }

    inline void Close(Host& host, bool restore = true)
    {
        bool oldBusy = busy; busy = true;
        host.open = false;
        bPluginsMenuOpen = false;
        bool selected = false; Bool(host.entry, L"m_bSelected", &selected);
        if (host.panel) Invoke(host.panel, L"HideWindow");
        if (host.back) Invoke(host.back, L"HideWindow");
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

    inline void Back(Host& host)
    {
        auto parent = model.Back(host.page);
        if (parent == SIZE_MAX) { Close(host); return; }
        host.page = parent;
        Populate(host);
    }

    inline void Click(Host& host)
    {
        if (auto root = Object(host.parent, L"Root")) Invoke(root, L"PlayClickSound");
    }

    // The selected row: left (-1), right (+1) or accept (0); accepting a plugin or a category opens it
    inline void Accept(Host& host, int direction)
    {
        auto row = Selected(host);
        if (!row) return;
        if (RWGTAMenu::Model::Opens(*row))
        {
            if (direction) return;
            auto page = model.Enter(*row);
            if (page == SIZE_MAX) return;
            host.page = page;
        }
        else if (!model.Change(*row, direction, host.pause) && direction)
            return;
        if (!direction) Click(host);
        Populate(host);
    }

    // The selected row's whole help in the game's message box
    inline void Details(Host& host)
    {
        auto row = Selected(host);
        if (!row || RWGTAMenu::Model::Opens(*row)) return;
        auto help = model.Help(*row);
        if (help.empty()) return;
        Call message(Popup(host), L"CreateMessageBox");
        if (!message.Put(L"_NotifyWindow", L"ObjectProperty", host.panel) ||
            !message.PutText(L"Title", Utf8(Upper(Wide(model.Label(*row))))) || !message.PutText(L"Message", help) ||
            !message.Put(L"Buttons", L"ByteProperty", uint8_t(2)) || !message.Put(L"ESCResult", L"ByteProperty", uint8_t(3)) ||
            !message.Put(L"EnterResult", L"ByteProperty", uint8_t(3)) || !message.Run())
            LogOnce("details", host.panel, "native message box unavailable");
    }

    // A key press on the page; false for keys it leaves to the game
    inline bool Key(Host& host, int key)
    {
        auto& window = model.Window(host.page);
        switch (key)
        {
            case VK_UP: window.Move(-1); break;
            case VK_DOWN: window.Move(1); break;
            case VK_PRIOR: window.Page(-1); break;
            case VK_NEXT: window.Page(1); break;
            case VK_HOME: window.Edge(-1); break;
            case VK_END: window.Edge(1); break;
            case VK_LEFT: Accept(host, -1); return true;
            case VK_RIGHT: Accept(host, 1); return true;
            case VK_RETURN: Accept(host, 0); return true;
            case VK_ESCAPE: case VK_BACK: Back(host); return true;
            case VK_F1: Details(host); return true;
            default: return false;
        }
        Show(host);
        return true;
    }

    // Once a frame while the page is open: held keys repeat, the pad, the wheel, the scroll bar
    inline void Frame(Host& host)
    {
        auto& window = model.Window(host.page);
        if (PopupShown(host))
        {
            host.padSeen = false;
            wheel.Clear();
            wheelDelta = 0;
            return;
        }
        const auto now = GetTickCount64();
        const bool focused = GetForegroundWindow() == hGameWindow;
        auto held = [focused](int key) { return focused && (GetAsyncKeyState(key) & 0x8000) != 0; };
        bool moved = false;
        // The press itself comes as a key event; these are its repeats
        if (host.up.Step(held(VK_UP), now, false)) moved |= window.Move(-1);
        if (host.down.Step(held(VK_DOWN), now, false)) moved |= window.Move(1);
        if (host.pageUp.Step(held(VK_PRIOR), now, false)) moved |= window.Page(-1);
        if (host.pageDown.Step(held(VK_NEXT), now, false)) moved |= window.Page(1);
        if (host.left.Step(held(VK_LEFT), now, false)) { Accept(host, -1); return; }
        if (host.right.Step(held(VK_RIGHT), now, false)) { Accept(host, 1); return; }

        XINPUT_STATE state{};
        DWORD slot = 0;
        if (PadInput::Read(state, slot))
        {
            const WORD buttons = state.Gamepad.wButtons;
            // Buttons held when the page opened (the press that opened it) act only after a release
            const WORD pressed = host.padSeen ? WORD(buttons & ~host.pad) : 0;
            host.pad = buttons;
            host.padSeen = true;
            const int y = host.stickY.Direction(state.Gamepad.sThumbLY), x = host.stickX.Direction(state.Gamepad.sThumbLX);
            const bool upHeld = (buttons & XINPUT_GAMEPAD_DPAD_UP) || y > 0, downHeld = (buttons & XINPUT_GAMEPAD_DPAD_DOWN) || y < 0;
            const bool leftHeld = (buttons & XINPUT_GAMEPAD_DPAD_LEFT) || x < 0, rightHeld = (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) || x > 0;
            if (host.padUp.Step(upHeld, now)) moved |= window.Move(-1);
            if (host.padDown.Step(downHeld, now)) moved |= window.Move(1);
            if (pressed & XINPUT_GAMEPAD_LEFT_SHOULDER) moved |= window.Page(-1);
            if (pressed & XINPUT_GAMEPAD_RIGHT_SHOULDER) moved |= window.Page(1);
            if (host.padLeft.Step(leftHeld, now)) { Accept(host, -1); return; }
            if (host.padRight.Step(rightHeld, now)) { Accept(host, 1); return; }
            if (pressed & XINPUT_GAMEPAD_A) { Accept(host, 0); return; }
            if (pressed & XINPUT_GAMEPAD_B) { Back(host); return; }
            if (pressed & XINPUT_GAMEPAD_Y) { Details(host); return; }
        }
        else
            host.padSeen = true;

        for (int notches = wheel.Take(wheelDelta.exchange(0)); notches; notches += notches > 0 ? -1 : 1)
            moved |= window.Scroll(notches > 0 ? -1 : 1);

        // The scroll bar's arrows and thumb move the list: the window follows
        if (auto bar = Object(host.list, L"VertSB"))
        {
            float pos = 0;
            if (Get(bar, L"Pos", L"FloatProperty", pos) && pos != host.shownPos)
            {
                const int target = int(std::lround(pos));
                for (int guard = 0; int(window.First()) != target && guard < 512; ++guard)
                {
                    if (!window.Scroll(target > int(window.First()) ? 1 : -1))
                        break;
                }
                moved = true;
            }
        }
        if (moved) Show(host);
    }

    inline bool __cdecl MenuWheel(int delta)
    {
        if (!active || !active->open || PopupShown(*active)) return false;
        wheelDelta += delta;
        return true;
    }

    // The slot under a point of the list (its coordinates); SIZE_MAX outside the rows
    inline size_t SlotAt(const Host& host, float y)
    {
        if (y < host.firstOffset || host.itemHeight <= 0) return SIZE_MAX;
        auto slot = size_t((y - host.firstOffset) / host.itemHeight);
        return slot < host.slots ? slot : SIZE_MAX;
    }

    inline bool BuildPanel(Host& host)
    {
        if (host.panel) return true;
        auto cls = findObject(nullptr, nullptr, L"UWindow.UWindowDialogClientWindow", 0);
        auto buttonClass = findObject(nullptr, nullptr, L"EchelonMenus.EPCTextButton", 0);
        auto listClass = findObject(nullptr, nullptr, L"EchelonMenus.EPCOptionKeysListBox", 0);
        auto labelClass = findObject(nullptr, nullptr, L"UWindow.UWindowLabelControl", 0);
        auto area = Object(host.parent, L"m_ControlsArea");
        float x, y, w, h;
        if (!cls || !buttonClass || !listClass || !labelClass || !Get(area, L"WinLeft", L"FloatProperty", x) || !Get(area, L"WinTop", L"FloatProperty", y) ||
            !Get(area, L"WinWidth", L"FloatProperty", w) || !Get(area, L"WinHeight", L"FloatProperty", h) || w < 300 || h < 140) return false;
        // The pause menu's pages hold their Reset To Default button at the bottom: the page ends above its frame
        float pageHeight = h;
        if (auto inner = Object(area, L"m_ResetToDefault"); inner && !Object(host.parent, L"m_ResetToDefault"))
        {
            float top = 0;
            if (Get(inner, L"WinTop", L"FloatProperty", top) && top > 100 && top < h)
                pageHeight = top - 16;
        }
        auto panel = Create(host.parent, false, cls, x, y, w, h);
        if (!panel) return false;
        struct Cleanup
        {
            void* panel;
            bool committed = false;
            ~Cleanup() { if (!committed) Invoke(panel, L"HideWindow"); }
        } cleanup{ panel };
        int font = 0; Get(host.sample, L"Font", L"IntProperty", font);
        uint8_t left = 0;
        // The list in its own client window, so the value buttons share its coordinates (as on the Controls page)
        const float helpHeight = 14, listTop = 20;
        const float listSpace = pageHeight - listTop - helpHeight * float(HelpLines) - 4;
        auto listArea = Create(panel, false, cls, 0, listTop, w, listSpace);
        if (!listArea) return false;
        auto list = Create(listArea, true, listClass, 0, 0, w, listSpace);
        float itemHeight = 0;
        int firstOffset = 3;
        if (!list || !Type(list, L"EPCOptionKeysListBox") || !Get(list, L"ItemHeight", L"FloatProperty", itemHeight) ||
            itemHeight < 8 || !Object(list, L"VertSB") || !Object(list, L"Items") || !Call(list, L"Clear").function) return false;
        Get(list, L"m_IFirstItemYOffset", L"IntProperty", firstOffset);
        bool behind = true; Bool(list, L"bAlwaysBehind", &behind);
        // A height in which the list paints whole rows and its scroll bar counts the same number of rows
        size_t slots = 1;
        while (float(firstOffset) + float(slots + 1) * itemHeight + 1 < listSpace) ++slots;
        const float listHeight = float(firstOffset) + float(slots) * itemHeight + 1;
        if (int(listHeight / itemHeight) != int(slots))
            LogOnce("open", list, "list rows and scroll bar range differ");
        Set(listArea, L"WinHeight", L"FloatProperty", listHeight);
        Set(list, L"WinHeight", L"FloatProperty", listHeight);
        Invoke(list, L"Resized");
        // The page's title, like the list's title rows: capitals in the list's colour, left aligned
        uint32_t listColor = 0;
        Get(list, L"TextColor", L"StructProperty", listColor);
        auto title = Create(panel, false, labelClass, 5, 4, w - 10, 16);
        if (!title || !Type(title, L"UWindowLabelControl") || !Set(title, L"Font", L"IntProperty", font) ||
            !Set(title, L"Align", L"ByteProperty", left) || !Set(title, L"TextColor", L"StructProperty", listColor) ||
            !String(title, L"Text", "")) return false;
        // The Graphics page's labels give the help its font and colour; the pause menu makes that page when it is
        // first shown, the list's text colour and the labels' normal font stand in until then
        int labelFont = 0;
        uint32_t labelColor = 0;
        Get(list, L"TextColor", L"StructProperty", labelColor);
        auto label = Object(Object(host.parent, L"m_GraphicArea"), L"m_LResolution");
        if (Type(label, L"UWindowLabelControl"))
        {
            Get(label, L"Font", L"IntProperty", labelFont);
            Get(label, L"TextColor", L"StructProperty", labelColor);
        }
        std::vector<void*> help;
        for (size_t i = 0; i < HelpLines; ++i)
        {
            auto line = Create(panel, false, labelClass, 4, pageHeight - helpHeight * float(HelpLines - i) - 2, w - 8, helpHeight);
            if (!line || !Type(line, L"UWindowLabelControl") || !Set(line, L"Font", L"IntProperty", labelFont) ||
                !Set(line, L"Align", L"ByteProperty", left) || !Set(line, L"TextColor", L"StructProperty", labelColor) ||
                !String(line, L"Text", "")) return false;
            help.push_back(line);
        }
        // Back in the page's Reset To Default button (the pause menu's pages have their own, inside the page)
        float offsetX = 0, offsetY = 0;
        auto reset = Object(host.parent, L"m_ResetToDefault");
        if (!reset)
        {
            reset = Object(area, L"m_ResetToDefault");
            offsetX = x;
            offsetY = y;
        }
        if (reset)
        {
            float bx = 0, by = 0, bw = 0, bh = 0;
            int backFont = 0;
            uint8_t backAlign = 0;
            if (Type(reset, L"EPCTextButton") && Get(reset, L"WinLeft", L"FloatProperty", bx) && Get(reset, L"WinTop", L"FloatProperty", by) &&
                Get(reset, L"WinWidth", L"FloatProperty", bw) && Get(reset, L"WinHeight", L"FloatProperty", bh) &&
                Get(reset, L"Font", L"IntProperty", backFont) && Get(reset, L"Align", L"ByteProperty", backAlign))
            {
                host.back = Create(host.parent, true, Read<void*>(reset, 36), bx + offsetX, by + offsetY, bw, bh);
                if (host.back && ControlContract(host.back))
                {
                    Set(host.back, L"Font", L"IntProperty", backFont); Set(host.back, L"Align", L"ByteProperty", backAlign);
                    String(host.back, L"ToolTipString", "");
                    Invoke(host.back, L"HideWindow");
                }
                else if (host.back)
                {
                    Invoke(host.back, L"HideWindow");
                    host.back = nullptr;
                }
            }
        }
        host.panel = panel; host.title = title; host.area = listArea; host.list = list; host.help = std::move(help);
        host.itemHeight = itemHeight; host.firstOffset = float(firstOffset); host.slots = slots;
        cleanup.committed = true;
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

    inline bool NoticeReady(void* viewport)
    {
        return viewport && Object(Object(Object(viewport, L"Console"), L"Root"), L"m_MessageBoxCW");
    }

    // A message box of the menus (the update notice); false while the menus aren't there
    inline bool ShowNotice(void* viewport, const std::wstring& text)
    {
        auto root = Object(Object(viewport, L"Console"), L"Root");
        auto popup = Object(root, L"m_MessageBoxCW");
        if (!popup)
            return false;
        Call message(popup, L"CreateMessageBox");
        return message.Put(L"_NotifyWindow", L"ObjectProperty", root) &&
            message.PutText(L"Title", UpperText(RWGTAMenu::PluginText(FixSettings(), Language(), "plugin.name"))) &&
            message.PutText(L"Message", asi_settings::text::Narrow(text)) &&
            message.Put(L"Buttons", L"ByteProperty", uint8_t(2)) &&
            message.Put(L"ESCResult", L"ByteProperty", uint8_t(3)) &&
            message.Put(L"EnterResult", L"ByteProperty", uint8_t(3)) && message.Run();
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
        model.OpenMenu(hostModule, Language(), host.slots, JoinedSettings(host));
        active = &host;
        host.page = RWGTAMenu::Model::RootPage;
        host.rows.clear();
        host.padSeen = false;
        wheel.Clear();
        wheelDelta = 0;
        bPluginsMenuOpen = true;
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
        Invoke(host.panel, L"ShowWindow"); Invoke(host.panel, L"BringToFront"); ActivateWindow(host.panel);
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
        if (inserted)
        {
            cached->second = Leaf(function);
            if (cached->second == L"KeyEvent" && Name(function) == L"Function Echelon.EGameInteraction.KeyEvent")
                automaticPadBindingEvent = function;
        }
        const auto& leaf = cached->second;
        if (automaticPadBindingEvent && automaticPadBindingEvent == function)
        {
            const Gamepad::AutomaticBindingScope scope;
            internalHook.thiscall<void>(object, frame, result);
            return;
        }
        if (active && active->open && leaf == L"KeyEvent" && Type(object, L"EPCConsole") && result && !PopupShown(*active))
        {
            uint8_t key = 0, action = 0;
            auto returned = Field(function, L"ReturnValue", L"BoolProperty");
            static const std::set<uint8_t> PageKeys{ VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_PRIOR, VK_NEXT, VK_HOME, VK_END,
                VK_RETURN, VK_ESCAPE, VK_BACK, VK_F1 };
            if (returned && Read<uint16_t>(returned, 58) == 4 &&
                Locals(frame, L"Key", L"ByteProperty", 1, &key) && PageKeys.contains(key) &&
                Locals(frame, L"Action", L"ByteProperty", 1, &action))
            {
                // The page takes its keys from the console, before the windows' own key routes and the console's
                // native close can handle them; their releases are kept from the windows too
                if (action == 1)
                {
                    busy = true;
                    Key(*active, key);
                    busy = false;
                }
                *static_cast<uint32_t*>(result) = 1;
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
                busy = true; Back(host); busy = false; return;
            }
            if (leaf == L"Notify")
            {
                void* control = nullptr; uint8_t event = 0;
                if (Locals(frame, L"C", L"ObjectProperty", 4, &control) && Locals(frame, L"E", L"ByteProperty", 1, &event) && event == 2)
                {
                    // The Plugins tab, as the other tabs, does nothing while its page is shown
                    if (control == host.entry)
                    { busy = true; if (!host.open) Open(host); busy = false; return; }
                    if (host.open && host.back && control == host.back)
                    { busy = true; Back(host); busy = false; return; }
                    if (host.open) Close(host, false); // vanilla tabs/main-menu keep their own Notify/confirmation
                }
            }
            else if (host.open && (leaf == L"HideWindow" || leaf == L"Close" || leaf == L"WindowHidden")) Close(host, false);
        }
        if (active && active->open)
        {
            auto& host = *active;
            auto value = std::find(host.values.begin(), host.values.end(), object);
            const size_t valueIndex = value != host.values.end() ? size_t(value - host.values.begin()) : SIZE_MAX;
            const bool onList = object == host.list || valueIndex != SIZE_MAX;
            if (object == host.panel && leaf == L"EscClose")
            {
                busy = true; Back(host); busy = false; return;
            }
            if (object == host.panel && (leaf == L"Close" || leaf == L"HideWindow" || leaf == L"WindowHidden"))
            {
                // Parent/level teardown still executes the engine operation.
                // Only the private model state is discarded here.
                host.open = false; active = nullptr; bPluginsMenuOpen = false; model.Close();
            }
            if (object == host.panel && leaf == L"Tick")
            {
                busy = true;
                if (RWGTAMenu::Model::Changed().exchange(false)) Populate(host);
                Frame(host);
                busy = false;
            }
            // The mouse: the row under it while it moves, a click accepts it; the wheel comes raw (MenuWheel)
            if (onList && (leaf == L"MouseMove" || leaf == L"LMouseDown" || leaf == L"LMouseUp" || leaf == L"Click" ||
                leaf == L"DoubleClick" || leaf == L"MouseWheelDown" || leaf == L"MouseWheelUp" || leaf == L"KeyDown" || leaf == L"KeyUp"))
            {
                if (leaf == L"MouseMove" || leaf == L"LMouseDown")
                {
                    float y = 0;
                    auto& window = model.Window(host.page);
                    size_t slot = SIZE_MAX;
                    if (valueIndex != SIZE_MAX)
                        slot = valueIndex >= window.First() ? valueIndex - window.First() : SIZE_MAX;
                    else if (Locals(frame, L"Y", L"FloatProperty", 4, &y))
                        slot = SlotAt(host, y);
                    if (slot != SIZE_MAX && !PopupShown(host))
                    {
                        busy = true;
                        const bool changed = window.Point(slot);
                        if (leaf == L"LMouseDown" && window.Selected() == window.First() + slot)
                            Accept(host, 0);
                        else if (changed)
                            Show(host);
                        busy = false;
                    }
                }
                return;
            }
        }
        internalHook.thiscall<void>(object, frame, result);
        if (leaf == L"BeforePaint" && captions.contains(object))
        {
            void* canvas = nullptr;
            if (Locals(frame, L"C", L"ObjectProperty", 4, &canvas)) FitLabel(object, canvas);
        }
        if (leaf == L"BeforePaint" && active && active->open && !active->help.empty() && object == active->help[0])
        {
            void* canvas = nullptr;
            if (Locals(frame, L"C", L"ObjectProperty", 4, &canvas)) WrapHelp(*active, canvas);
        }
        // A wide row's button spans the row the list just drew
        if (leaf == L"DrawItem" && active && object == active->list)
        {
            void* item = nullptr;
            float x = 0, w = 0;
            if (Locals(frame, L"Item", L"ObjectProperty", 4, &item) && Locals(frame, L"X", L"FloatProperty", 4, &x) &&
                Locals(frame, L"W", L"FloatProperty", 4, &w))
            {
                auto at = std::find(active->items.begin(), active->items.end(), item);
                const size_t index = size_t(at - active->items.begin());
                if (at != active->items.end() && index < active->wide.size() && active->wide[index])
                {
                    int padding = 0; Get(active->list, L"m_IRightPadding", L"IntProperty", padding);
                    busy = true;
                    Set(active->values[index], L"WinLeft", L"FloatProperty", x);
                    Set(active->values[index], L"WinWidth", L"FloatProperty", std::max(40.0f, w - x - float(padding)));
                    busy = false;
                }
            }
        }
        // The list shows the value buttons of the rows it draws; the others stay hidden
        if (leaf == L"HideControls" && active && object == active->list)
        {
            busy = true;
            for (auto button : active->values) Invoke(button, L"HideWindow");
            busy = false;
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
                if (active == &host) { active = nullptr; bPluginsMenuOpen = false; model.Close(); }
                it = hosts.erase(it);
            }
            else if (object == host.panel || object == host.title || object == host.area || object == host.list || object == host.back ||
                std::find(host.help.begin(), host.help.end(), object) != host.help.end() ||
                std::find(host.values.begin(), host.values.end(), object) != host.values.end() ||
                std::find(host.items.begin(), host.items.end(), object) != host.items.end())
            {
                // HideWindow removes the panel from the parent's GC-visible
                // child list. Its entry remains native-owned; reconstruct the
                // panel on a later click if the hidden panel was collected.
                if (active == &host) { active = nullptr; bPluginsMenuOpen = false; model.Close(); }
                host.open = false;
                host.panel = host.title = host.area = host.list = host.back = nullptr;
                host.help.clear(); host.values.clear(); host.items.clear(); host.rows.clear();
                ++it;
            }
            else ++it;
        }
        captions.erase(object);
        functionNames.erase(object);
        if (automaticPadBindingEvent == object)
            automaticPadBindingEvent = nullptr;
        return destroyHook.thiscall<int>(object);
    }
    inline void Init()
    {
        auto core = GetModuleHandleW(L"Core.dll");
        if (!core)
            throw std::runtime_error("Core module unavailable");
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
            throw std::runtime_error("Required Core export unavailable");
        auto engine = GetModuleHandleW(L"Engine.dll");
        textSize = engine ? reinterpret_cast<decltype(textSize)>(GetProcAddress(engine, "?TextSize@UCanvas@@UAEXPBGPAM1@Z")) : nullptr;
        if (!textSize)
            throw std::runtime_error("Native font measurement unavailable");
        if (!InitResolutionApply(engine))
            throw std::runtime_error("Native resolution apply hook unavailable");
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&hostModule), &hostModule) || !hostModule)
        {
            resolutionApplyHook.reset();
            throw std::runtime_error("Native menu host module unavailable");
        }
        destroyHook = safetyhook::create_inline(destroy, Destroy);
        if (!destroyHook)
        {
            resolutionApplyHook.reset();
            throw std::runtime_error("ConditionalDestroy hook failed");
        }
        internalHook = safetyhook::create_inline(internal, Internal);
        if (!internalHook)
        {
            destroyHook.reset();
            resolutionApplyHook.reset();
            throw std::runtime_error("ProcessInternal hook failed");
        }
        UWindowsViewport::WindowsMenuMouseActive = WindowsMenuMouseActive;
        UWindowsViewport::FocusPause = FocusPause;
        UWindowsViewport::MenuWheel = MenuWheel;
    }
}
