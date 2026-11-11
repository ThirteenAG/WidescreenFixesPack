module;

#include <stdafx.h>
#include <stdexcept>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"

export module NativeMenu;

import ComVars;

// PT's GUI objects, strings, arrays, input and clipping stay engine-owned.
// The retail/digital Core contracts differ from SC1: FString is 16 bytes,
// Children/PropertiesSize are +60/+64, UFunction parameters are +100/+102.
export namespace PTMenu
{
    inline bool ResolutionJoined(void*);
    inline bool ResolutionBefore(void*, void*, void*, const std::wstring&);
    inline void ResolutionAfter(void*, const std::wstring&);
    inline void ForgetResolution(void*);
    inline SafetyHookInline internalHook, destroyHook, buttonDrawHook, tabDrawHook, tickHook;
    inline void InitControlsDrawing();
    inline void (__fastcall* textSize)(void*, void*, const wchar_t*, float*, float*) = nullptr;
    inline wchar_t* (__fastcall* fullName)(void*, void*, wchar_t*) = nullptr;
    inline void* (__cdecl* findObject)(void*, void*, const wchar_t*, int) = nullptr;
    inline void* (__cdecl* constructObject)(void*, void*, int, uint32_t, void*, void*, int) = nullptr;
    inline void (__fastcall* processEvent)(void*, void*, void*, void*, void*) = nullptr;
    inline void (__fastcall* copyString)(void*, void*, void*, const void*, void*) = nullptr;
    inline void (__fastcall* copyArray)(void*, void*, void*, const void*, void*) = nullptr;
    inline void (__fastcall* stringInit)(void*, void*) = nullptr;
    inline void* (__fastcall* stringAssign)(void*, void*, const wchar_t*) = nullptr;
    inline void (__fastcall* stringDestroy)(void*, void*) = nullptr;
    inline const wchar_t* (__cdecl* getLanguage)(int) = nullptr;
    inline float (__fastcall* actualLeft)(void*, void*) = nullptr;
    inline float (__fastcall* actualTop)(void*, void*) = nullptr;
    inline float (__fastcall* actualWidth)(void*, void*) = nullptr;
    inline float (__fastcall* actualHeight)(void*, void*) = nullptr;
    inline HMODULE hostModule = nullptr;
    inline bool busy = false;
    inline RWGTAMenu::Model model;
    inline std::unordered_map<void*, std::wstring> functionNames;

    struct Array { void* data; int count, capacity; };
    static_assert(sizeof(Array) == 12);
    struct String { void* data; int count, capacity; uint32_t flags; };
    static_assert(sizeof(String) == 16);
    template<class T> T Read(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
    }
    inline std::wstring Name(void* object)
    {
        if (!object || !fullName) return {};
        wchar_t buffer[1024]{};
        auto value = fullName(object, nullptr, buffer);
        return value ? value : L"";
    }
    inline void LogOnce(const std::string& phase, void* object, const std::string& reason)
    {
        static std::set<std::tuple<std::string, void*, std::string>> written;
        if (!written.emplace(phase, object, reason).second) return;
        auto name = Name(object);
        auto line = "PT native menu " + phase + ": " + reason;
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
        return Name(object).starts_with(std::wstring(kind) + L" ");
    }
    inline void* Field(void* scope, const wchar_t* name, const wchar_t* kind)
    {
        for (int depth = 0; scope && depth < 32; ++depth, scope = Read<void*>(scope, 44))
        {
            int count = 0;
            for (auto field = Read<void*>(scope, 60), next = field; field && ++count <= 512; field = next)
            {
                next = Read<void*>(field, 48);
                if (_wcsicmp(Leaf(field).c_str(), name) == 0 && Type(field, kind)) return field;
                if (next == field) break;
            }
        }
        return nullptr;
    }
    inline void* ClassProperty(void* cls, const wchar_t* name, const wchar_t* kind, size_t size)
    {
        auto prop = cls ? Field(cls, name, kind) : nullptr;
        if (!prop || Read<uint16_t>(prop, 56) != 1 || Read<uint16_t>(prop, 58) != size) return nullptr;
        auto total = Read<int>(cls, 64);
        return total > 0 && total < 65536 && size_t(Read<uint16_t>(prop, 68)) + size <= size_t(total) ? prop : nullptr;
    }
    inline void* Property(void* object, const wchar_t* name, const wchar_t* kind, size_t size)
    {
        return object ? ClassProperty(Read<void*>(object, 36), name, kind, size) : nullptr;
    }
    inline void* Address(void* object, void* prop)
    {
        return static_cast<uint8_t*>(object) + Read<uint16_t>(prop, 68);
    }
    template<class T> bool Get(void* object, const wchar_t* name, const wchar_t* kind, T& value)
    {
        auto prop = Property(object, name, kind, sizeof(T));
        if (!prop) return false;
        std::memcpy(&value, Address(object, prop), sizeof(T)); return true;
    }
    template<class T> bool Set(void* object, const wchar_t* name, const wchar_t* kind, T value)
    {
        auto prop = Property(object, name, kind, sizeof(T));
        if (!prop) return false;
        *static_cast<T*>(Address(object, prop)) = value; return true;
    }
    inline void* Object(void* object, const wchar_t* name)
    {
        void* value = nullptr; Get(object, name, L"ObjectProperty", value); return value;
    }
    inline bool Bool(void* object, const wchar_t* name, bool* value = nullptr)
    {
        auto prop = Property(object, name, L"BoolProperty", 4);
        if (!prop) return false;
        auto mask = Read<uint32_t>(prop, 84);
        if (!mask || (mask & (mask - 1))) return false;
        auto data = static_cast<uint32_t*>(Address(object, prop));
        if (value) *data = (*data & ~mask) | (*value ? mask : 0);
        return (*data & mask) != 0;
    }
    inline bool SetBool(void* object, const wchar_t* name, bool value)
    {
        auto prop = Property(object, name, L"BoolProperty", 4);
        if (!prop) return false;
        const auto mask = Read<uint32_t>(prop, 84);
        return mask && !(mask & (mask - 1)) && Bool(object, name, &value) == value;
    }
    inline bool WriteStringValue(void* object, const wchar_t* name, const std::string& text)
    {
        auto prop = Property(object, name, L"StrProperty", 16);
        if (!prop) return false;
        int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (!length && !text.empty()) return false;
        std::wstring wide(length, L'\0');
        if (length) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), wide.data(), length);
        String temporary{};
        stringInit(&temporary, nullptr);
        stringAssign(&temporary, nullptr, wide.c_str());
        copyString(prop, nullptr, Address(object, prop), &temporary, object);
        stringDestroy(&temporary, nullptr);
        return true;
    }
    inline std::string Language()
    {
        auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        auto language = getLanguage ? getLanguage(0) : nullptr;
        if (!language) return "en";
        if (!_wcsicmp(language, L"fra") || !_wcsicmp(language, L"fre")) return "fr";
        if (_wcsicmp(language, L"deu") == 0) return "de";
        if (_wcsicmp(language, L"ita") == 0) return "it";
        if (!_wcsicmp(language, L"esp") || !_wcsicmp(language, L"spa")) return "es";
        if (_wcsicmp(language, L"rus") == 0) return "ru";
        return "en";
    }
    struct Caption
    {
        std::wstring requested;
        void* font = nullptr;
        float width = 0, height = 0;
        bool fitted = false;
    };
    inline std::map<void*, Caption> captions;
    inline bool StringValue(void* object, const wchar_t* name, const std::string& text, bool uppercase = false)
    {
        if (_wcsicmp(name, L"Caption") != 0) return WriteStringValue(object, name, text);
        int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (!size && !text.empty()) return false;
        std::wstring value(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), value.data(), size);
        for (auto& c : value) if (c < 32 || c == 8232 || c == 8233) c = L' ';
        auto tag = Language();
        std::wstring locale(tag.begin(), tag.end());
        size = uppercase ? LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr, 0) : 0;
        if (size > 0)
        {
            std::wstring upper(size, L'\0');
            if (LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, value.data(), int(value.size()), upper.data(), size, nullptr, nullptr, 0)) value = std::move(upper);
        }
        auto& caption = captions[object];
        if (caption.requested != value) { caption.requested = std::move(value); caption.fitted = false; }
        return true;
    }
    struct Call
    {
        void* object = nullptr;
        void* function = nullptr;
        std::vector<uint8_t> params;
        std::vector<size_t> strings;
        Call(const Call&) = delete;
        Call& operator=(const Call&) = delete;
        Call(void* target, const wchar_t* name) : object(target)
        {
            function = target ? Field(Read<void*>(target, 36), name, L"Function") : nullptr;
            if (function && !Read<uint16_t>(function, 96) && Read<uint16_t>(function, 100) <= 1024)
                params.resize(Read<uint16_t>(function, 100));
            else function = nullptr;
        }
        // ProcessEvent hands the parameters back as they were left, so their strings are the caller's to free
        ~Call()
        {
            for (auto offset : strings)
                stringDestroy(params.data() + offset, nullptr);
        }
        void* Param(const wchar_t* name, const wchar_t* kind, size_t size)
        {
            auto prop = function ? Field(function, name, kind) : nullptr;
            if (!prop || Read<uint16_t>(prop, 56) != 1 || Read<uint16_t>(prop, 58) != size ||
                !(Read<uint32_t>(prop, 60) & 0x80) || size_t(Read<uint16_t>(prop, 68)) + size > params.size()) return nullptr;
            return Address(params.data(), prop);
        }
        template<class T> bool Put(const wchar_t* name, const wchar_t* kind, T value)
        {
            auto slot = Param(name, kind, sizeof(T));
            if (!slot) return false;
            std::memcpy(slot, &value, sizeof(T)); return true;
        }
        bool PutBool(const wchar_t* name, bool value)
        {
            auto prop = function ? Field(function, name, L"BoolProperty") : nullptr;
            auto slot = prop ? Param(name, L"BoolProperty", 4) : nullptr;
            const auto mask = prop ? Read<uint32_t>(prop, 84) : 0;
            if (!slot || !mask || (mask & (mask - 1))) return false;
            auto bits = Read<uint32_t>(slot, 0);
            bits = value ? bits | mask : bits & ~mask;
            std::memcpy(slot, &bits, sizeof(bits)); return true;
        }
        bool PutString(const wchar_t* name, const std::wstring& text)
        {
            auto slot = Param(name, L"StrProperty", sizeof(String));
            if (!slot) return false;
            const size_t offset = static_cast<uint8_t*>(slot) - params.data();
            if (std::find(strings.begin(), strings.end(), offset) == strings.end())
            {
                stringInit(slot, nullptr);
                strings.push_back(offset);
            }
            stringAssign(slot, nullptr, text.c_str()); return true;
        }
        bool Run()
        {
            if (!function) return false;
            processEvent(object, nullptr, function, params.data(), nullptr); return true;
        }
    };
    inline bool InitComponent(void* object, void* controller, void* owner)
    {
        Call call(object, L"InitComponent");
        return call.Put(L"MyController", L"ObjectProperty", controller) &&
            call.Put(L"MyOwner", L"ObjectProperty", owner) && call.Run();
    }
    inline void Focus(void* object)
    {
        Call call(object, L"SetFocus");
        if (call.Put(L"Who", L"ObjectProperty", static_cast<void*>(nullptr))) call.Run();
    }
    inline bool ArrayValues(void* owner, std::vector<void*>& out, const wchar_t* name = L"Controls")
    {
        auto prop = Property(owner, name, L"ArrayProperty", sizeof(Array));
        auto inner = prop ? Read<void*>(prop, 84) : nullptr;
        if (!inner || !Type(inner, L"ObjectProperty") || Read<uint16_t>(inner, 58) != 4 ||
            (Read<uint32_t>(inner, 60) & 0x400000)) return false;
        auto array = Read<Array>(Address(owner, prop), 0);
        if (array.count < 0 || array.count > 128 || array.capacity < array.count || (array.count && !array.data)) return false;
        out.clear();
        if (array.count) out.assign(static_cast<void**>(array.data), static_cast<void**>(array.data) + array.count);
        return true;
    }
    inline bool Attach(void* owner, void* child, bool add = true)
    {
        std::vector<void*> controls;
        if (!ArrayValues(owner, controls)) return false;
        auto at = std::find(controls.begin(), controls.end(), child);
        if (add && at == controls.end()) controls.push_back(child);
        else if (!add && at != controls.end()) controls.erase(at);
        else return true;
        Array source{ controls.data(), int(controls.size()), int(controls.size()) };
        auto prop = Property(owner, L"Controls", L"ArrayProperty", sizeof(Array));
        copyArray(prop, nullptr, Address(owner, prop), &source, owner); return true;
    }
    inline bool Alive(void* object) { return object && !(Read<uint32_t>(object, 28) & 0x800000); }
    inline void Visible(void* object, bool value) { if (Alive(object)) Bool(object, L"bVisible", &value); }
    struct Rect { float x, y, w, h; };
    inline Rect Bounds(void* object)
    {
        return { actualLeft(object, nullptr), actualTop(object, nullptr), actualWidth(object, nullptr), actualHeight(object, nullptr) };
    }
    inline bool Position(void* object, Rect rect)
    {
        bool no = false;
        if (!std::isfinite(rect.x) || !std::isfinite(rect.y) || !std::isfinite(rect.w) || !std::isfinite(rect.h) || rect.w <= 0 || rect.h <= 0 ||
            !Property(object, L"bBoundToParent", L"BoolProperty", 4) || !Property(object, L"bScaleToParent", L"BoolProperty", 4)) return false;
        for (auto name : { L"WinLeft", L"WinTop", L"WinWidth", L"WinHeight" })
            if (!Property(object, name, L"FloatProperty", 4)) return false;
        // Native Actual* treats values <= 1 as fractions of the 640x480 canvas.
        auto horizontal = [](float value) { return value > 1 ? value : value / 640.0f; };
        auto vertical = [](float value) { return value > 1 ? value : value / 480.0f; };
        Bool(object, L"bBoundToParent", &no); Bool(object, L"bScaleToParent", &no);
        return Set(object, L"WinLeft", L"FloatProperty", horizontal(rect.x)) && Set(object, L"WinTop", L"FloatProperty", vertical(rect.y)) &&
            Set(object, L"WinWidth", L"FloatProperty", horizontal(rect.w)) && Set(object, L"WinHeight", L"FloatProperty", vertical(rect.h));
    }
    struct Placement
    {
        void* object = nullptr;
        float left = 0, top = 0, width = 0, height = 0;
        bool bound = false, scale = false;
    };
    inline bool SavePlacement(void* object, Placement& saved)
    {
        if (!Property(object, L"bBoundToParent", L"BoolProperty", 4) || !Property(object, L"bScaleToParent", L"BoolProperty", 4)) return false;
        saved.object = object; saved.bound = Bool(object, L"bBoundToParent"); saved.scale = Bool(object, L"bScaleToParent");
        return Get(object, L"WinLeft", L"FloatProperty", saved.left) && Get(object, L"WinTop", L"FloatProperty", saved.top) &&
            Get(object, L"WinWidth", L"FloatProperty", saved.width) && Get(object, L"WinHeight", L"FloatProperty", saved.height);
    }
    inline void RestorePlacement(const Placement& saved)
    {
        if (!Alive(saved.object)) return;
        Set(saved.object, L"WinLeft", L"FloatProperty", saved.left); Set(saved.object, L"WinTop", L"FloatProperty", saved.top);
        Set(saved.object, L"WinWidth", L"FloatProperty", saved.width); Set(saved.object, L"WinHeight", L"FloatProperty", saved.height);
        auto bound = saved.bound, scale = saved.scale;
        Bool(saved.object, L"bBoundToParent", &bound); Bool(saved.object, L"bScaleToParent", &scale);
    }
    inline bool PlanTabs(Rect strip, const std::array<Rect, 3>& tabs, Rect& expanded, std::array<Rect, 3>& placed, Rect& entry)
    {
        auto finite = [](Rect rect) { return std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.w) && std::isfinite(rect.h); };
        if (!finite(strip) || strip.x < 0 || strip.y < 0 || strip.x + strip.w > 640 || strip.y + strip.h > 480 || strip.h < 12 || strip.h > 48) return false;
        std::array<size_t, 3> order{ 0, 1, 2 };
        std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return tabs[a].x < tabs[b].x; });
        auto width = tabs[order[0]].w;
        if (width < 100 || width > 160 || std::abs(strip.w - width * 3) > 1) return false;
        for (size_t i = 0; i < order.size(); ++i)
        {
            const auto rect = tabs[order[i]];
            if (!finite(rect) || std::abs(rect.w - width) > 1 || std::abs(rect.h - strip.h) > 1 ||
                std::abs(rect.y - strip.y) > 1 || std::abs(rect.x - (strip.x + float(i) * width)) > 1) return false;
        }
        expanded = strip;
        width = strip.w / 4.0f;
        for (size_t i = 0; i < order.size(); ++i) placed[order[i]] = { expanded.x + float(i) * width, strip.y, width, strip.h };
        entry = { expanded.x + width * 3, strip.y, width, strip.h };
        return true;
    }
    inline std::wstring ReadString(void* object, const wchar_t* name)
    {
        String value{};
        if (!Get(object, name, L"StrProperty", value) || value.count <= 0 || value.count > 1024 || value.capacity < value.count || !value.data) return {};
        auto text = static_cast<const wchar_t*>(value.data);
        return text[value.count - 1] == 0 ? std::wstring(text, value.count - 1) : std::wstring{};
    }
    inline void FitCaption(void* object, void* canvas, bool tab)
    {
        auto found = captions.find(object);
        if (found == captions.end() || !canvas || !textSize) return;
        auto controller = Object(object, L"Controller");
        auto name = ReadString(object, L"FontName");
        std::vector<void*> fonts;
        if (name.empty() || !ArrayValues(controller, fonts, L"FontStack")) return;
        void* font = nullptr;
        for (auto candidate : fonts)
            if (_wcsicmp(ReadString(candidate, L"KeyName").c_str(), name.c_str()) == 0) { font = Object(candidate, L"OnlyDefaultFont"); break; }
        if (!font) { LogOnce("font", object, "native menu font unavailable"); return; }
        auto rect = Bounds(object);
        float width = rect.w - (tab ? 10.0f : 18.0f), height = rect.h - 2;
        auto& caption = found->second;
        if (caption.fitted && caption.font == font && caption.width == width && caption.height == height) return;
        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0) return;
        auto old = Object(canvas, L"Font");
        if (!Set(canvas, L"Font", L"ObjectProperty", font)) return;
        auto measure = [&](const std::wstring& value)
        {
            float w = 0, h = 0; textSize(canvas, nullptr, value.c_str(), &w, &h);
            return std::isfinite(w) && std::isfinite(h) && h <= height && (value.empty() || w > 0) ? w : INFINITY;
        };
        auto value = caption.requested;
        for (auto& c : value) if (c != L' ' && !std::isfinite(measure(std::wstring(1, c)))) c = L'?';
        if (measure(value) > width)
        {
            while (!value.empty() && measure(value + L"...") > width)
            {
                value.pop_back();
                if (!value.empty() && value.back() >= 0xD800 && value.back() <= 0xDBFF) value.pop_back();
            }
            value = measure(value + L"...") <= width ? value + L"..." : L"";
        }
        Set(canvas, L"Font", L"ObjectProperty", old);
        int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr);
        std::string fitted(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()), fitted.data(), size, nullptr, nullptr);
        WriteStringValue(object, L"Caption", fitted);
        caption.font = font; caption.width = width; caption.height = height; caption.fitted = true;
    }
    inline void DrawDetails(void* object, void* canvas);
    inline void __fastcall DrawButton(void* object, void*, void* canvas)
    {
        DrawDetails(object, canvas);
        FitCaption(object, canvas, false);
        buttonDrawHook.thiscall<void>(object, canvas);
    }
    inline void __fastcall DrawTab(void* object, void*, void* canvas)
    {
        FitCaption(object, canvas, true);
        tabDrawHook.thiscall<void>(object, canvas);
    }
    inline void __fastcall Tick(void* controller, void*, float delta);
    inline void InitDrawing()
    {
        if (buttonDrawHook || tabDrawHook) return;
        auto gui = GetModuleHandleW(L"GUI.dll"), engine = GetModuleHandleW(L"Engine.dll");
        auto tick = GetProcAddress(gui, "?NativeTick@UGUIController@@UAEXM@Z");
        auto button = GetProcAddress(gui, "?Draw@UGUIButtonSC2@@UAEXPAVUCanvas@@@Z");
        auto tab = GetProcAddress(gui, "?Draw@UGUITabButtonSC2@@UAEXPAVUCanvas@@@Z");
        auto measure = GetProcAddress(engine, "?TextSize@UCanvas@@UAEXPBGPAM1@Z");
        if (!measure) measure = GetProcAddress(engine, "?TextSize@UCanvas@@UAEXPB_WPAM1@Z");
        if (!tick || !button || !tab || !measure)
        {
            throw std::runtime_error("Native drawing or font export unavailable");
        }
        textSize = reinterpret_cast<decltype(textSize)>(measure);
        tickHook = safetyhook::create_inline(tick, Tick);
        buttonDrawHook = safetyhook::create_inline(button, DrawButton);
        tabDrawHook = safetyhook::create_inline(tab, DrawTab);
        if (!tickHook || !buttonDrawHook || !tabDrawHook)
            throw std::runtime_error("Native drawing hook installation failed");
        actualLeft = reinterpret_cast<decltype(actualLeft)>(GetProcAddress(gui, "?ActualLeft@UGUIComponent@@UAEMXZ"));
        actualTop = reinterpret_cast<decltype(actualTop)>(GetProcAddress(gui, "?ActualTop@UGUIComponent@@UAEMXZ"));
        actualWidth = reinterpret_cast<decltype(actualWidth)>(GetProcAddress(gui, "?ActualWidth@UGUIComponent@@UAEMXZ"));
        actualHeight = reinterpret_cast<decltype(actualHeight)>(GetProcAddress(gui, "?ActualHeight@UGUIComponent@@UAEMXZ"));
        if (!actualLeft || !actualTop || !actualWidth || !actualHeight)
            throw std::runtime_error("GUI bounds exports unavailable");
        InitControlsDrawing();
    }
    struct Item
    {
        RWGTAMenu::Row row;
        std::string label, value, help;
        int link = 0;
        bool enabled = true;
    };
    struct Controls;
    struct Video;
    struct Host
    {
        void* parent = nullptr;
        void* controller = nullptr;
        void* tabBox = nullptr;
        void* entry = nullptr;
        void* panel = nullptr;
        void* title = nullptr;
        void* help = nullptr;
        void* savedFocus = nullptr;
        std::vector<void*> labels, values, widgets;
        std::vector<Item> items;
        std::vector<std::pair<void*, bool>> visibility;
        std::vector<std::pair<void*, Rect>> stockTabs;
        std::vector<Placement> placements;
        size_t page = SIZE_MAX, rootStart = 0, rowStart = 0, pageRows = 6;
        bool open = false, pause = false, pendingOpen = false, retired = false, building = false;
        size_t helpIndex = SIZE_MAX, detailsPage = 0;
        bool details = false, detailsDirty = false;
        std::wstring detailsText;
        std::vector<std::wstring> detailsLines;
        void* detailsFont = nullptr;
        float detailsWidth = 0;
        uint64_t viewGeneration = 0;
        std::shared_ptr<Controls> controls;
        std::shared_ptr<Video> video;
    };
    using HostRef = std::shared_ptr<Host>;
    inline std::unordered_map<void*, HostRef> hosts;
    inline HostRef active;
    inline std::set<std::string> JoinedControlIDs(const Host& host);
    inline void DrainControls(const HostRef& host);
    inline bool ControlsBefore(void* object, void* frame, void* result, const std::wstring& leaf);
    inline void ControlsAfter(void* object, void* frame, const std::wstring& leaf);
    inline bool ControlsTrack(const Host& host, void* object);
    inline void RetireControls(Host& host, void* dying);
    inline bool VideoJoined(const Host& host);
    inline bool VideoBefore(void* object, void* frame, void* result, const std::wstring& leaf);
    inline void VideoAfter(void* object, const std::wstring& leaf);
    inline bool VideoTrack(const Host& host, void* object);
    inline void RetireVideo(Host& host, void* dying);
    inline size_t internalDepth = 0, tickDepth = 0;
    inline bool draining = false;
    inline void* menuController = nullptr;
    inline HostRef Keep(Host& host)
    {
        auto found = hosts.find(host.parent);
        return found != hosts.end() && found->second.get() == &host ? found->second : HostRef{};
    }
    inline bool OwnerCurrent(const HostRef& host)
    {
        return host && !host->retired && !host->building && Alive(host->parent) && Alive(host->controller) &&
            Object(host->controller, L"ActivePage") == host->parent;
    }
    inline bool Current(const HostRef& host)
    {
        return OwnerCurrent(host) && !Object(host->parent, L"theMsgBox");
    }
    inline bool PlanHint(Rect canvas, Rect title, Rect tabs, Rect body, Rect& hint)
    {
        auto finite = [](Rect r) { return std::isfinite(r.x) && std::isfinite(r.y) && std::isfinite(r.w) && std::isfinite(r.h); };
        if (!finite(canvas) || !finite(title) || !finite(tabs) || !finite(body) || canvas.x < 0 || canvas.y < 0 || canvas.w <= 0 || canvas.h <= 0 || canvas.x + canvas.w > 640 || canvas.y + canvas.h > 480 ||
            title.h < 12 || title.h > 48 || tabs.h < 12 || tabs.h > 48 || body.w < 200 || body.w > 640) return false;
        const float top = title.y + title.h + 2;
        const float height = std::min(18.0f, tabs.y - top - 2);
        if (height < 14 || body.x < canvas.x || body.x + body.w > canvas.x + canvas.w ||
            top < canvas.y || top + height > canvas.y + canvas.h) return false;
        hint = { body.x, top, body.w, height };
        return true;
    }
    inline bool NativeHint(const HostRef& host, Rect body, Rect& hint)
    {
        if (!Current(host)) return false;
        auto canvas = Bounds(host->parent);
        // Root pages may leave their own height at zero on the fixed native canvas.
        if (canvas.x == 0 && canvas.y == 0 && canvas.w == 640 && canvas.h == 0)
        {
            float width = 0, height = -1;
            if (!Get(host->parent, L"WinWidth", L"FloatProperty", width) || width != 1 ||
                !Get(host->parent, L"WinHeight", L"FloatProperty", height) || height != 0 ||
                !Property(host->parent, L"MenuOwner", L"ObjectProperty", 4) || Object(host->parent, L"MenuOwner") ||
                !Property(host->parent, L"bBoundToParent", L"BoolProperty", 4) || Bool(host->parent, L"bBoundToParent") ||
                !Property(host->parent, L"bScaleToParent", L"BoolProperty", 4) || Bool(host->parent, L"bScaleToParent")) return false;
            canvas.h = 480;
        }
        std::vector<void*> children;
        if (!ArrayValues(host->parent, children)) return false;
        void* title = nullptr;
        for (auto child : children)
        {
            if (!Alive(child) || !Type(child, L"GUIMenuTitleAndLines") || Object(child, L"MenuOwner") != host->parent) continue;
            if (title) return false;
            title = child;
        }
        return title && Alive(host->tabBox) && PlanHint(canvas, Bounds(title), Bounds(host->tabBox), body, hint);
    }
    enum class Action { Toggle, Activate, Back, Tab, Close, Details, DetailNext, DetailPrevious };
    struct Pending
    {
        std::weak_ptr<Host> owner;
        uint64_t viewGeneration;
        Action action;
        size_t selected;
        int direction;
        void* sender;
    };
    inline std::vector<Pending> pendingActions;
    inline void Queue(const HostRef& host, Action action, size_t selected = 0, int direction = 0, void* sender = nullptr)
    {
        if (Current(host) && pendingActions.size() < 64)
            pendingActions.push_back({ host, host->viewGeneration, action, selected, direction, sender });
    }
    inline size_t RootLinks(const Host& host) { return host.pageRows - 2; }
    inline size_t ContentLinks(const Host& host) { return host.pageRows - 3; }
    inline size_t Links(const Host& host)
    {
        return std::min(RootLinks(host), model.Pages().size() - std::min(host.rootStart, model.Pages().size()));
    }
    inline void RestoreStock(Host& host, void* dying = nullptr)
    {
        if (host.retired) return;
        host.open = false; host.pendingOpen = false; host.details = false; host.detailsText.clear(); host.detailsLines.clear(); host.detailsFont = nullptr;
        if (host.panel != dying) Visible(host.panel, false);
        for (auto [object, value] : host.visibility) if (object != dying) Visible(object, value);
        host.visibility.clear();
    }
    inline void Close(Host& host, bool focus = true, void* dying = nullptr)
    {
        const auto keep = Keep(host);
        if (host.retired) return;
        RestoreStock(host, dying);
        auto savedFocus = host.savedFocus;
        host.savedFocus = nullptr;
        if (active.get() == &host) { active.reset(); model.Close(); }
        if (!host.retired && focus && savedFocus != dying && Alive(savedFocus)) Focus(savedFocus);
    }
    inline void Populate(Host& host)
    {
        const auto keep = Keep(host);
        if (!Current(keep) || !host.open || active != keep) return;
        const auto sourceSnapshot = model;
        ++host.viewGeneration;
        if (host.page != SIZE_MAX && host.page >= model.Pages().size()) host.page = SIZE_MAX;
        const bool root = host.page == SIZE_MAX;
        host.items.clear(); host.helpIndex = SIZE_MAX;
        auto title = root ? model.Text("menu.plugins") : model.Pages()[host.page].title;
        if (model.RestartRequired()) title += " - " + model.Text("menu.restartwarning");
        StringValue(host.title, L"Caption", title, true); Visible(host.title, true);
        if (root)
        {
            auto links = Links(host);
            for (size_t i = 0; i < links; ++i) host.items.push_back({ {}, model.Pages()[host.rootStart + i].title, "", "", int(host.rootStart + i + 1) });
            if (host.rootStart) host.items.push_back({ {}, model.Text("menu.previous"), "", "", -2 });
            if (host.rootStart + links < model.Pages().size()) host.items.push_back({ {}, model.Text("menu.next"), "", "", -3 });
        }
        else
        {
            auto visible = model.VisibleRows(host.page);
            if (!Current(keep) || active != keep) return;
            std::vector<RWGTAMenu::Row> rows;
            for (const auto row : visible) rows.push_back(*row);
            auto content = ContentLinks(host);
            if (host.rowStart >= rows.size()) host.rowStart = 0;
            for (size_t i = host.rowStart; i < rows.size() && i < host.rowStart + content; ++i)
            {
                Item item; item.row = rows[i];
                item.label = model.Label(item.row);
                item.value = model.Value(item.row);
                if (!Current(keep) || active != keep) return;
                item.help = model.Help(item.row);
                item.enabled = model.Enabled(item.row, host.pause);
                if (!Current(keep) || active != keep) return;
                host.items.push_back(std::move(item));
            }
            host.items.push_back({ {}, model.Text("menu.details"), "", "", -4 });
            if (host.rowStart) host.items.push_back({ {}, model.Text("menu.previous"), "", "", -2 });
            if (host.rowStart + content < rows.size()) host.items.push_back({ {}, model.Text("menu.next"), "", "", -3 });
        }
        host.items.push_back({ {}, model.Text("menu.back"), "", "", -1 });
        if (!root && !host.items.empty()) host.helpIndex = 0;
        auto focused = Object(host.controller, L"FocusedControl");
        for (size_t i = 0; i < host.labels.size(); ++i)
        {
            if (i >= host.items.size()) { Visible(host.labels[i], false); Visible(host.values[i], false); continue; }
            auto& item = host.items[i];
            auto labelBounds = Bounds(host.labels[i]);
            const auto valueBounds = Bounds(host.values[i]);
            labelBounds.w = (item.link ? valueBounds.x + valueBounds.w : valueBounds.x) - labelBounds.x;
            Position(host.labels[i], labelBounds);
            auto value = item.enabled ? item.value : model.Text("menu.unavailable");
            StringValue(host.labels[i], L"Caption", item.label); StringValue(host.values[i], L"Caption", value);
            StringValue(host.labels[i], L"Hint", ""); StringValue(host.values[i], L"Hint", "");
            Bool(host.labels[i], L"bAcceptsInput", &item.enabled); Bool(host.labels[i], L"bTabStop", &item.enabled);
            Bool(host.values[i], L"bAcceptsInput", &item.enabled);
            Set(host.labels[i], L"MenuState", L"ByteProperty", uint8_t(item.enabled ? (focused == host.labels[i] ? 2 : 0) : 4));
            Set(host.values[i], L"MenuState", L"ByteProperty", uint8_t(item.enabled ? (focused == host.values[i] ? 2 : 0) : 4));
            Visible(host.labels[i], true); Visible(host.values[i], !value.empty());
        }
    }
    inline void DisabledStates(Host& host)
    {
        if (host.retired || host.details) return;
        // GUIMultiComponent.MenuStateChange re-enables the visual state of
        // disabled children when focus propagates. Keep availability drawn as
        // disabled, while native focus/navigation still skips bTabStop=false.
        for (auto object : host.labels)
            if (Bool(object, L"bVisible") && !Bool(object, L"bAcceptsInput")) Set(object, L"MenuState", L"ByteProperty", uint8_t(4));
        for (auto object : host.values)
            if (Bool(object, L"bVisible") && !Bool(object, L"bAcceptsInput")) Set(object, L"MenuState", L"ByteProperty", uint8_t(4));
    }
    inline void First(Host& host)
    {
        const auto keep = Keep(host);
        if (host.retired) return;
        for (auto label : host.labels)
            if (Bool(label, L"bVisible") && Bool(label, L"bAcceptsInput")) { Focus(label); if (!host.retired) DisabledStates(host); return; }
    }
    inline std::wstring DetailWide(const std::string& text)
    {
        if (text.size() > size_t(INT_MAX)) return {};
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (count <= 0) return {};
        std::wstring result(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), result.data(), count);
        for (auto& c : result) if (c < 32 && c != L'\n') c = L' ';
        return result;
    }
    inline std::string DetailUtf8(const std::wstring& text)
    {
        if (text.empty()) return {};
        const int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0, nullptr, nullptr);
        std::string result(count, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.data(), int(text.size()), result.data(), count, nullptr, nullptr);
        return result;
    }
    inline void* PageFocus(const Host& host)
    {
        auto current = host.parent;
        std::set<void*> seen;
        while (Alive(current) && seen.size() < 32 && seen.insert(current).second)
        {
            auto next = Object(current, L"FocusedControl");
            if (!next) return current;
            current = next;
        }
        return nullptr;
    }
    inline void BeginDetails(Host& host)
    {
        const auto keep = Keep(host);
        if (!Current(keep) || !host.open || host.page == SIZE_MAX || host.helpIndex >= host.items.size()) return;
        const auto& row = host.items[host.helpIndex];
        if (row.link || row.help.empty()) return;
        auto text = row.label;
        if (!row.value.empty()) text += ": " + row.value;
        text += "\n" + row.help;
        auto wide = DetailWide(text);
        if (wide.empty()) return;
        host.detailsText = std::move(wide);
        host.details = host.detailsDirty = true;
        host.detailsPage = 0; host.detailsFont = nullptr; host.detailsLines.clear();
    }
    inline void EndDetails(Host& host)
    {
        const auto selected = host.helpIndex;
        host.details = false; host.detailsText.clear(); host.detailsLines.clear(); host.detailsFont = nullptr;
        Populate(host);
        if (!host.retired && selected < host.items.size()) Focus(host.labels[selected]);
    }
    inline void StepDetails(Host& host, int direction)
    {
        if (!host.details || host.labels.size() < 2) return;
        const auto count = host.labels.size() - 1;
        const auto pages = std::max(size_t(1), (host.detailsLines.size() + count - 1) / count);
        host.detailsPage = direction < 0 ? (host.detailsPage + pages - 1) % pages : (host.detailsPage + 1) % pages;
        host.detailsDirty = true;
    }
    inline std::vector<std::wstring> WrapDetails(const std::wstring& text, float width, const std::function<float(const std::wstring&)>& measure)
    {
        std::vector<std::wstring> lines;
        std::wstring line;
        auto flush = [&]() { if (!line.empty()) { lines.push_back(std::move(line)); line.clear(); } };
        for (size_t at = 0; at < text.size();)
        {
            if (text[at] == L'\n') { flush(); ++at; continue; }
            if (iswspace(text[at])) { ++at; continue; }
            auto end = at;
            while (end < text.size() && !iswspace(text[end])) ++end;
            auto word = text.substr(at, end - at); at = end;
            if (!line.empty() && measure(line + L" " + word) > width) flush();
            while (!word.empty() && measure(word) > width)
            {
                size_t take = 0;
                for (size_t i = 0; i < word.size();)
                {
                    size_t next = i + 1;
                    if (word[i] >= 0xD800 && word[i] <= 0xDBFF && next < word.size() && word[next] >= 0xDC00 && word[next] <= 0xDFFF) ++next;
                    if (measure(word.substr(0, next)) > width) break;
                    take = next; i = next;
                }
                if (!take) return {};
                lines.push_back(word.substr(0, take)); word.erase(0, take);
            }
            if (!word.empty()) { if (!line.empty()) line += L' '; line += word; }
        }
        flush();
        return lines;
    }
    inline void DrawDetails(void* object, void* canvas)
    {
        const auto host = active;
        if (!Current(host) || !host->open || !host->details || !canvas || !textSize || host->labels.size() < 2) return;
        const auto count = host->labels.size() - 1;
        if (std::find(host->labels.begin(), host->labels.end() - 1, object) != host->labels.end() - 1)
            Set(object, L"MenuState", L"ByteProperty", uint8_t(0));
        const auto key = ReadString(host->labels.front(), L"FontName");
        std::vector<void*> fonts;
        if (key.empty() || !ArrayValues(host->controller, fonts, L"FontStack")) return;
        void* font = nullptr;
        for (auto candidate : fonts)
            if (_wcsicmp(ReadString(candidate, L"KeyName").c_str(), key.c_str()) == 0) { font = Object(candidate, L"OnlyDefaultFont"); break; }
        const auto left = Bounds(host->labels.front()), value = Bounds(host->values.front());
        const float width = value.x + value.w - left.x - 18;
        if (!Alive(font) || !std::isfinite(width) || width < 100) return;
        if (font != host->detailsFont || width != host->detailsWidth)
        {
            auto old = Object(canvas, L"Font");
            if (!Set(canvas, L"Font", L"ObjectProperty", font)) return;
            auto measure = [&](const std::wstring& text)
            {
                float w = 0, h = 0; textSize(canvas, nullptr, text.c_str(), &w, &h);
                return std::isfinite(w) && std::isfinite(h) && h <= left.h - 2 && (text.empty() || w > 0) ? w : INFINITY;
            };
            auto lines = WrapDetails(host->detailsText, width, measure);
            Set(canvas, L"Font", L"ObjectProperty", old);
            if (lines.empty()) return;
            host->detailsLines = std::move(lines); host->detailsFont = font; host->detailsWidth = width; host->detailsDirty = true;
        }
        if (!host->detailsDirty) return;
        host->detailsDirty = false;
        const auto pages = std::max(size_t(1), (host->detailsLines.size() + count - 1) / count);
        host->detailsPage = std::min(host->detailsPage, pages - 1);
        StringValue(host->title, L"Caption", model.Text("menu.details") + " (" + std::to_string(host->detailsPage + 1) + "/" + std::to_string(pages) + ")", true);
        for (size_t i = 0; i < count; ++i)
        {
            auto rect = Bounds(host->labels[i]); rect.w = width + 18; Position(host->labels[i], rect);
            const auto index = host->detailsPage * count + i;
            SetBool(host->labels[i], L"bAcceptsInput", false); SetBool(host->labels[i], L"bTabStop", false);
            Set(host->labels[i], L"MenuState", L"ByteProperty", uint8_t(0));
            if (index < host->detailsLines.size()) StringValue(host->labels[i], L"Caption", DetailUtf8(host->detailsLines[index]));
            Visible(host->labels[i], index < host->detailsLines.size()); Visible(host->values[i], false);
        }
        auto rect = Bounds(host->labels[count]); const auto right = Bounds(host->values[count]);
        rect.w = pages > 1 ? right.x - rect.x : right.x + right.w - rect.x; Position(host->labels[count], rect);
        StringValue(host->labels[count], L"Caption", model.Text("menu.back")); SetBool(host->labels[count], L"bAcceptsInput", true); SetBool(host->labels[count], L"bTabStop", true); Visible(host->labels[count], true);
        StringValue(host->values[count], L"Caption", model.Text("menu.next")); SetBool(host->values[count], L"bAcceptsInput", pages > 1); SetBool(host->values[count], L"bTabStop", pages > 1); Visible(host->values[count], pages > 1);
        StringValue(host->help, L"Caption", model.Text("menu.detailshint")); Visible(host->help, true);
        Focus(host->labels[count]);
    }
    inline void Activate(Host& host, size_t selected, int direction)
    {
        const auto keep = Keep(host);
        if (host.retired || selected >= host.items.size()) return;
        auto item = host.items[selected];
        if (item.link == -4) { if (!direction) BeginDetails(host); return; }
        bool changedPage = false;
        if (!item.link) model.Change(item.row, direction, host.pause);
        else if (!direction)
        {
            changedPage = true;
            if (item.link == -1)
            {
                if (host.page == SIZE_MAX) { Close(host); return; }
                model.PageShown(host.page); host.page = SIZE_MAX; host.rowStart = 0;
            }
            else if (item.link == -2)
            {
                auto& start = host.page == SIZE_MAX ? host.rootStart : host.rowStart;
                const auto step = host.page == SIZE_MAX ? RootLinks(host) : ContentLinks(host);
                start = start >= step ? start - step : 0;
            }
            else if (item.link == -3) (host.page == SIZE_MAX ? host.rootStart : host.rowStart) += host.page == SIZE_MAX ? RootLinks(host) : ContentLinks(host);
            else { host.page = size_t(item.link - 1); host.rowStart = 0; model.PageShown(host.page); }
        }
        if (!Current(keep) || active != keep) return;
        Populate(host);
        if (Current(keep) && active == keep && changedPage) First(host);
    }
    inline bool PauseContext(Host& host, bool& pause)
    {
        // The shipped title and pause pages both open MenuOptions. Require an
        // actual parent-page route; a controller existing does not imply pause.
        auto page = Object(host.parent, L"ParentPage");
        for (int i = 0; page && i < 16; ++i)
        {
            if (Type(page, L"MenuMainMenu")) { pause = false; return true; }
            if (Type(page, L"DemoMenuInGamePause")) { pause = true; return true; }
            auto next = Object(page, L"ParentPage");
            if (next == page) break;
            page = next;
        }
        return false;
    }
    inline void Open(Host& host)
    {
        const auto keep = Keep(host);
        if (!Current(keep)) return;
        host.pendingOpen = false;
        if (Object(host.controller, L"ActivePage") != host.parent || Object(host.parent, L"theMsgBox") || !PauseContext(host, host.pause))
        { LogOnce("open", host.parent, "active page/modal/title-or-pause route guard rejected"); return; }
        if (active) Close(*active);
        if (host.retired) return;
        model.OpenPlugins(hostModule, Language(), 4096, JoinedControlIDs(host));
        if (host.retired) { model.Close(); return; }
        host.page = SIZE_MAX; host.rootStart = host.rowStart = 0;
        host.savedFocus = Object(host.controller, L"FocusedControl");
        std::vector<void*> controls;
        if (!ArrayValues(host.parent, controls)) { model.Close(); return; }
        for (auto object : controls)
        {
            if (!object || object == host.tabBox || object == host.panel) continue;
            host.visibility.emplace_back(object, Bool(object, L"bVisible")); Visible(object, false);
        }
        active = keep; host.open = true;
        Populate(host); if (host.retired) return; Visible(host.panel, true); First(host);
        if (host.retired) return;
    }
    inline void SelectTab(Host& host, void* sender, int direction)
    {
        const auto keep = Keep(host);
        if (!Current(keep)) return;
        std::vector<void*> tabs;
        for (auto [tab, rect] : host.stockTabs) tabs.push_back(tab);
        tabs.push_back(host.entry);
        auto at = std::find(tabs.begin(), tabs.end(), sender);
        if (at == tabs.end()) return;
        auto next = (int(at - tabs.begin()) + direction + int(tabs.size())) % int(tabs.size());
        if (tabs[next] == host.entry) { if (!host.open) Open(host); return; }
        if (host.open) Close(host, false);
        if (host.retired) return;
        // The stock Options handler selects its tab, switches its native body
        // and sets focus. Do not synthesize a click on a settings control.
        Call call(host.parent, L"OnActivateHandle");
        if (call.Put(L"Sender", L"ObjectProperty", tabs[next])) call.Run();
    }
    inline bool WidgetContract(void* cls)
    {
        for (auto name : { L"bVisible", L"bAcceptsInput", L"bTabStop", L"bBoundToParent", L"bScaleToParent", L"bNeedLocalized" })
            if (!ClassProperty(cls, name, L"BoolProperty", 4)) return false;
        for (auto name : { L"WinLeft", L"WinTop", L"WinWidth", L"WinHeight" })
            if (!ClassProperty(cls, name, L"FloatProperty", 4)) return false;
        for (auto name : { L"Caption", L"FontName", L"Hint" })
            if (!ClassProperty(cls, name, L"StrProperty", 16)) return false;
        return ClassProperty(cls, L"TextAlign", L"ByteProperty", 1) && ClassProperty(cls, L"MenuState", L"ByteProperty", 1);
    }
    struct Parameter { const wchar_t* name; const wchar_t* kind; uint16_t size; };
    inline bool FunctionContract(void* cls, const wchar_t* name, std::initializer_list<Parameter> expected)
    {
        auto function = Field(cls, name, L"Function");
        if (!function || Read<uint16_t>(function, 96) || Read<uint16_t>(function, 100) > 1024) return false;
        size_t count = 0;
        size_t visited = 0;
        for (auto prop = Read<void*>(function, 60); prop; prop = Read<void*>(prop, 48))
        {
            if (++visited > 512) return false;
            if (!(Read<uint32_t>(prop, 60) & 0x80)) continue;
            ++count;
            if (count > expected.size()) return false;
            auto match = std::find_if(expected.begin(), expected.end(), [prop](const auto& item)
            { return _wcsicmp(Leaf(prop).c_str(), item.name) == 0 && Type(prop, item.kind) && Read<uint16_t>(prop, 58) == item.size; });
            if (match == expected.end() || Read<uint16_t>(prop, 56) != 1 ||
                size_t(Read<uint16_t>(prop, 68)) + match->size > Read<uint16_t>(function, 100)) return false;
            if (_wcsicmp(match->name, L"ReturnValue") == 0 &&
                (Read<uint32_t>(prop, 84) != 1 || Read<uint16_t>(prop, 68) != Read<uint16_t>(function, 102))) return false;
        }
        return count == expected.size();
    }
    inline bool InputContract(void* cls)
    {
        return FunctionContract(cls, L"InitComponent", {{L"myController", L"ObjectProperty", 4}, {L"MyOwner", L"ObjectProperty", 4}}) &&
            FunctionContract(cls, L"SetFocus", {{L"Who", L"ObjectProperty", 4}}) &&
            FunctionContract(cls, L"OnActivate", {{L"Sender", L"ObjectProperty", 4}}) &&
            FunctionContract(cls, L"OnClick", {{L"Sender", L"ObjectProperty", 4}, {L"ReturnValue", L"BoolProperty", 4}}) &&
            FunctionContract(cls, L"OnKeyCancel", {{L"Sender", L"ObjectProperty", 4}, {L"ReturnValue", L"BoolProperty", 4}}) &&
            FunctionContract(cls, L"OnKeyEvent", {{L"Key", L"ByteProperty", 1}, {L"State", L"ByteProperty", 1},
                {L"Delta", L"FloatProperty", 4}, {L"ReturnValue", L"BoolProperty", 4}});
    }
    inline bool UnboundInput(void* object)
    {
        // Empty native FScriptDelegates dispatch the class's own script body.
        // Refuse a class template binding which could redirect our input hook.
        for (auto name : { L"__OnClick__Delegate", L"__OnKeyEvent__Delegate", L"__OnActivate__Delegate" })
        {
            auto prop = Property(object, name, L"DelegateProperty", 8);
            if (!prop || Read<uint64_t>(Address(object, prop), 0)) return false;
        }
        return true;
    }
    inline bool Prepare(void* parent)
    {
        if (auto found = hosts.find(parent); found != hosts.end()) return !found->second->building && !found->second->retired;
        auto controller = Object(parent, L"Controller"), tabBox = Object(parent, L"boxTabButtons"), area = Object(parent, L"boxVideo");
        auto controls = Object(parent, L"btnControls"), video = Object(parent, L"btnVideo"), sound = Object(parent, L"btnSound");
        auto buttonClass = findObject(nullptr, nullptr, L"GUI.GUIButtonSC2", 0);
        auto tabClass = findObject(nullptr, nullptr, L"GUI.GUITabButtonSC2", 0);
        auto panelClass = findObject(nullptr, nullptr, L"GUI.GUIMultiComponent", 0);
        if (!controller || !tabBox || !area || !controls || !video || !sound ||
            !Type(buttonClass, L"Class") || !Type(tabClass, L"Class") || !Type(panelClass, L"Class") ||
            !WidgetContract(buttonClass) || !WidgetContract(tabClass) || !InputContract(buttonClass) || !InputContract(tabClass) ||
            !InputContract(panelClass) || !InputContract(Read<void*>(parent, 36)) ||
            !FunctionContract(Read<void*>(parent, 36), L"OnActivateHandle", {{L"Sender", L"ObjectProperty", 4}}))
        { LogOnce("prepare", parent, "native objects/widget properties/input contracts unavailable"); return false; }
        std::vector<void*> parentControls, tabs;
        if (!ArrayValues(parent, parentControls) || !ArrayValues(tabBox, tabs) ||
            std::find(tabs.begin(), tabs.end(), controls) == tabs.end() || std::find(tabs.begin(), tabs.end(), video) == tabs.end() ||
            std::find(tabs.begin(), tabs.end(), sound) == tabs.end() || !Call(controls, L"InitComponent").function || !Call(controls, L"SetFocus").function)
        { LogOnce("prepare", parent, "native controls/tab ownership contract unavailable"); return false; }
        auto first = Bounds(controls), last = Bounds(sound), body = Bounds(area);
        auto finite = [](Rect rect) { return std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.w) && std::isfinite(rect.h); };
        float span = Bounds(tabBox).w;
        if (!finite(first) || !finite(last) || !finite(body) || !std::isfinite(span) || span < 280 || span > 640 || first.h < 12 || first.h > 48 ||
            body.w < 300 || body.w > 640 || body.h < 120 || body.h > 480 || body.x < 0 || body.y < 0 || body.x + body.w > 640 || body.y + body.h > 480)
        { LogOnce("prepare", parent, "native tab/body geometry out of guarded bounds"); return false; }
        auto nativeList = Object(parent, L"ControlsListBox"), nativeFooter = Object(parent, L"boxButtons");
        if (!Type(nativeList, L"GUIKeyListBoxSC2_PC") || !Alive(nativeFooter)) return false;
        const auto viewport = Bounds(nativeList), footer = Bounds(nativeFooter);
        if (!finite(viewport) || !finite(footer) || viewport.x < 0 || viewport.y < 0 || viewport.w < 300 || viewport.h < 120 ||
            viewport.x + viewport.w > 640 || viewport.y + viewport.h > footer.y || footer.h < 12 || footer.h > 48 ||
            footer.y + footer.h > 480) return false;
        body = { viewport.x, viewport.y, viewport.w, footer.y + footer.h - viewport.y };
        auto building = std::make_shared<Host>();
        auto& host = *building;
        host.parent = parent; host.controller = controller; host.tabBox = tabBox; host.building = true;
        for (auto tab : { controls, video, sound }) host.stockTabs.emplace_back(tab, Bounds(tab));
        std::sort(host.stockTabs.begin(), host.stockTabs.end(), [](const auto& a, const auto& b) { return a.second.x < b.second.x; });
        host.pageRows = size_t(std::clamp(int((body.h - 50) / 22) - 1, 3, 8));
        if (50 + float(host.pageRows + 1) * 22 > body.h)
        { LogOnce("prepare", parent, "native body too short for bounded rows and help"); return false; }
        Rect expandedStrip{}, entryRect{};
        std::array<Rect, 3> placedTabs{};
        std::array<Rect, 3> originalTabs{ host.stockTabs[0].second, host.stockTabs[1].second, host.stockTabs[2].second };
        // Keep the native strip inside its frame and share it between four tabs.
        if (!PlanTabs(Bounds(tabBox), originalTabs, expandedStrip, placedTabs, entryRect))
        { LogOnce("prepare", parent, "native tab strip cannot fit four tabs"); return false; }
        for (auto object : { tabBox, controls, video, sound })
        {
            Placement saved;
            if (!SavePlacement(object, saved)) return false;
            host.placements.push_back(saved);
        }
        hosts.emplace(parent, building);
        struct Cleanup
        {
            HostRef host;
            bool committed = false;
            ~Cleanup()
            {
                if (committed || host->retired) return;
                if (host->panel) { Visible(host->panel, false); Attach(host->parent, host->panel, false); }
                if (host->entry) { Visible(host->entry, false); Attach(host->tabBox, host->entry, false); }
                for (const auto& saved : host->placements) RestorePlacement(saved);
                for (auto widget : host->widgets) captions.erase(widget);
                host->retired = true;
                RetireControls(*host, host->parent); RetireVideo(*host, host->parent);
                auto found = hosts.find(host->parent);
                if (found != hosts.end() && found->second == host) hosts.erase(found);
            }
        } cleanup{ building };
        auto create = [&](void* cls, void* owner, Rect rect, bool input, uint8_t align)
        {
            if (host.retired) return static_cast<void*>(nullptr);
            auto object = constructObject(cls, owner, 0, 0, nullptr, nullptr, 0);
            if (!object || host.retired) return static_cast<void*>(nullptr);
            host.widgets.push_back(object);
            Visible(object, false);
            bool no = false; Bool(object, L"bNeedLocalized", &no); Bool(object, L"bAcceptsInput", &input); Bool(object, L"bTabStop", &input);
            if (!Position(object, rect) || !Attach(owner, object) || !InitComponent(object, controller, owner))
            { if (!host.retired) Attach(owner, object, false); return static_cast<void*>(nullptr); }
            if (host.retired) return static_cast<void*>(nullptr);
            Visible(object, false);
            if (!UnboundInput(object)) { LogOnce("create", object, "class-template input delegate is bound"); Attach(owner, object, false); return static_cast<void*>(nullptr); }
            if (Property(object, L"TextAlign", L"ByteProperty", 1)) Set(object, L"TextAlign", L"ByteProperty", align);
            return object;
        };
        host.panel = create(panelClass, parent, body, false, 0);
        if (host.retired) return false;
        if (!host.panel) { LogOnce("prepare", parent, "native panel creation failed"); return false; }
        bool yes = true; Bool(host.panel, L"bAcceptsInput", &yes);
        host.title = create(buttonClass, host.panel, {body.x, body.y, body.w, 28}, false, 0);
        bool complete = host.title && StringValue(host.title, L"FontName", "GUITitleBoldFont");
        if (host.retired) return false;
        if (!complete) LogOnce("prepare", parent, "native heading/font creation failed");
        host.help = create(buttonClass, host.panel, {body.x, body.y + body.h - 22, body.w, 22}, false, 0);
        complete = complete && host.help;
        if (host.retired) return false;
        if (!host.help) LogOnce("prepare", parent, "native help control creation failed");
        for (size_t i = 0; complete && i < host.pageRows + 1; ++i)
        {
            auto y = body.y + 28 + float(i) * 22;
            auto label = create(buttonClass, host.panel, {body.x, y, body.w * 0.62f, 22}, true, 0);
            auto value = create(buttonClass, host.panel, {body.x + body.w * 0.62f, y, body.w * 0.38f, 22}, true, 2);
            complete = label && value;
            if (host.retired) return false;
            if (!complete) LogOnce("prepare", parent, "native label/value creation failed at row " + std::to_string(i));
            if (complete) { bool no = false; Bool(value, L"bTabStop", &no); host.labels.push_back(label); host.values.push_back(value); }
        }
        if (complete)
            host.entry = create(tabClass, tabBox, entryRect, true, 1);
        if (host.retired) return false;
        if (complete && !host.entry) LogOnce("prepare", parent, "native Plugins tab construction failed");
        complete = complete && host.entry && StringValue(host.entry, L"Caption", RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"), true);
        if (!complete || host.retired)
        {
            if (!host.retired) LogOnce("prepare", parent, "native row/tab creation failed");
            return false;
        }
        bool positioned = Position(tabBox, expandedStrip);
        for (size_t i = 0; positioned && i < host.stockTabs.size(); ++i) positioned = Position(host.stockTabs[i].first, placedTabs[i]);
        if (!positioned) { LogOnce("prepare", parent, "native tab placement failed"); return false; }
        Visible(host.entry, true);
        host.building = false; cleanup.committed = true;
        return true;
    }
    inline bool Locals(void* frame, const wchar_t* name, const wchar_t* kind, size_t size, void* output)
    {
        auto function = Read<void*>(frame, 4), locals = Read<void*>(frame, 16);
        auto prop = Field(function, name, kind);
        if (!locals || !prop || Read<uint16_t>(prop, 56) != 1 || Read<uint16_t>(prop, 58) != size ||
            !(Read<uint32_t>(prop, 60) & 0x80) || size_t(Read<uint16_t>(prop, 68)) + size > Read<uint16_t>(function, 100)) return false;
        std::memcpy(output, Address(locals, prop), size); return true;
    }
    inline bool Result(void* frame, void* result)
    {
        auto function = Read<void*>(frame, 4);
        auto prop = Field(function, L"ReturnValue", L"BoolProperty");
        if (!result || !prop || Read<uint16_t>(prop, 58) != 4 || Read<uint32_t>(prop, 84) != 1 ||
            Read<uint16_t>(prop, 68) != Read<uint16_t>(function, 102)) return false;
        injector::WriteMemory<uint32_t>(static_cast<uint32_t*>(result), 1, true); return true;
    }
    inline void Drain(void* controller)
    {
        if (draining || internalDepth || tickDepth) return;
        draining = true;
        const bool previous = busy; busy = true;
        std::vector<HostRef> controlsSnapshot;
        for (const auto& [parent, host] : hosts) if (host->controller == controller) controlsSnapshot.push_back(host);
        for (const auto& host : controlsSnapshot) DrainControls(host);
        auto actions = std::move(pendingActions); pendingActions.clear();
        for (const auto& action : actions)
        {
            auto host = action.owner.lock();
            if (!Current(host) || (action.action != Action::Close && action.viewGeneration != host->viewGeneration)) continue;
            if (host->controller != controller) { pendingActions.push_back(action); continue; }
            if (action.action == Action::Toggle) { if (host->open) Close(*host); else Open(*host); }
            else if (action.action == Action::Activate && host->open) Activate(*host, action.selected, action.direction);
            else if (action.action == Action::Details && host->open) BeginDetails(*host);
            else if (action.action == Action::DetailNext && host->open) StepDetails(*host, 1);
            else if (action.action == Action::DetailPrevious && host->open) StepDetails(*host, -1);
            else if (action.action == Action::Tab) SelectTab(*host, action.sender, action.direction);
            else if (action.action == Action::Close) Close(*host, false);
            else if (action.action == Action::Back && host->open)
            {
                if (host->details) EndDetails(*host);
                else if (host->page == SIZE_MAX) Close(*host);
                else { host->page = SIZE_MAX; Populate(*host); if (!host->retired) First(*host); }
            }
        }
        std::vector<HostRef> snapshot;
        for (const auto& [parent, host] : hosts) if (host->controller == controller) snapshot.push_back(host);
        for (const auto& host : snapshot)
            if (Current(host) && host->pendingOpen)
            {
                host->pendingOpen = false;
                if (!host->open && Object(controller, L"FocusedControl") == host->entry) Open(*host);
            }
        auto host = active;
        if (host && !host->retired && host->controller == controller)
        {
            if (!OwnerCurrent(host) || !host->open) Close(*host, false);
            else if (Current(host))
            {
                if (RWGTAMenu::Model::Changed().exchange(false) && !host->details) Populate(*host);
                if (Current(host) && active == host && !host->details)
                {
                    auto focused = PageFocus(*host);
                    std::string help;
                    for (size_t i = 0; i < host->items.size(); ++i)
                        if (focused == host->labels[i] || focused == host->values[i])
                        {
                            help = host->items[i].help;
                            if (!host->items[i].link) host->helpIndex = i;
                            break;
                        }
                    if (!help.empty()) help = model.Text("menu.detailskey") + "  " + help;
                    StringValue(host->help, L"Caption", help); Visible(host->help, true);
                }
            }
        }
        busy = previous; draining = false;
    }
    inline void __fastcall Tick(void* controller, void*, float delta)
    {
        menuController = controller;
        ++tickDepth;
        tickHook.thiscall<void>(controller, delta);
        --tickDepth;
        if (!tickDepth) Drain(controller);
    }
    struct Watch
    {
        void* object;
        bool destroyed = false;
        Watch* previous;
        static inline Watch* top = nullptr;
        explicit Watch(void* value) : object(value), previous(top) { top = this; }
        ~Watch() { top = previous; }
    };
    inline void __fastcall Internal(void* object, void*, void* frame, void* result)
    {
        struct Depth
        {
            Depth() { ++internalDepth; }
            ~Depth() { --internalDepth; }
        } depth;
        if (busy) { internalHook.thiscall<void>(object, frame, result); return; }
        auto function = Read<void*>(frame, 4);
        auto [cached, inserted] = functionNames.try_emplace(function);
        if (inserted) cached->second = Leaf(function);
        const auto& leaf = cached->second;
        if (ResolutionBefore(object, frame, result, leaf)) return;
        if (ControlsBefore(object, frame, result, leaf) || VideoBefore(object, frame, result, leaf)) return;
        auto own = hosts.find(object);
        if (own != hosts.end() && own->second->open)
        {
            auto host = own->second;
            if (leaf == L"OnKeyCancel" && Result(frame, result)) { Queue(host, Action::Back); return; }
            if (leaf == L"OnClose" || leaf == L"OnOpen" || leaf == L"OnReOpen")
            {
                Queue(host, Action::Close);
                RestoreStock(*host);
            }
            else if (leaf == L"OnClickHandle" || leaf == L"OnActivateHandle")
            {
                void* sender = nullptr;
                if (Locals(frame, L"Sender", L"ObjectProperty", 4, &sender) && sender != host->entry)
                {
                    Queue(host, Action::Close);
                    RestoreStock(*host);
                }
            }
        }
        for (const auto& [parent, owner] : hosts)
        {
            auto keep = owner;
            auto& host = *keep;
            if (host.retired) continue;
            auto row = std::find(host.labels.begin(), host.labels.end(), object);
            auto value = std::find(host.values.begin(), host.values.end(), object);
            bool entry = object == host.entry;
            bool ours = entry || row != host.labels.end() || value != host.values.end() || object == host.panel;
            if (!ours || (host.open && Object(host.parent, L"theMsgBox"))) continue;
            auto selected = row != host.labels.end() ? size_t(row - host.labels.begin()) : size_t(value - host.values.begin());
            if (entry && leaf == L"OnActivate")
            {
                host.pendingOpen = !host.open;
                return;
            }
            if ((entry || host.open) && leaf == L"OnKeyEvent" && (entry || row != host.labels.end() || value != host.values.end()))
            {
                uint8_t key = 0, state = 0;
                if (Locals(frame, L"Key", L"ByteProperty", 1, &key) && Locals(frame, L"State", L"ByteProperty", 1, &state) && host.open &&
                    (key == 112 || (host.details && (key == 37 || key == 39 || key == 33 || key == 34 || key == 198 || key == 199 || key == 215 || key == 213))) && Result(frame, result))
                {
                    if (state == 1 || (state == 2 && key != 112))
                        Queue(keep, key == 112 ? (host.details ? Action::Back : Action::Details) :
                            (key == 37 || key == 33 || key == 198 || key == 215 ? Action::DetailPrevious : Action::DetailNext));
                    return;
                }
                if (Locals(frame, L"Key", L"ByteProperty", 1, &key) && Locals(frame, L"State", L"ByteProperty", 1, &state) &&
                    (key == 37 || key == 39 || key == 198 || key == 199 || key == 215 || key == 213) && (state == 1 || state == 2) && Result(frame, result))
                {
                    Queue(keep, entry ? Action::Tab : Action::Activate, selected, key == 37 || key == 198 || key == 215 ? -1 : 1, host.entry);
                    return;
                }
            }
            if (leaf == L"OnClick" && (entry || row != host.labels.end() || value != host.values.end()) && Result(frame, result))
            {
                if (host.details)
                {
                    if (object == host.labels.back()) Queue(keep, Action::Back);
                    else if (object == host.values.back()) Queue(keep, Action::DetailNext);
                }
                else Queue(keep, entry ? Action::Toggle : Action::Activate, selected);
                return;
            }
            if (host.open && leaf == L"OnKeyCancel" && Result(frame, result)) { Queue(keep, Action::Back); return; }
        }
        const bool prepare = (leaf == L"InitComponent" || leaf == L"OnOpen") && Type(object, L"MenuOptions") &&
            function == Field(Read<void*>(object, 36), leaf.c_str(), L"Function");
        Watch watch(object);
        internalHook.thiscall<void>(object, frame, result);
        if (prepare && !watch.destroyed)
        {
            const bool previous = busy; busy = true; Prepare(object); busy = previous;
        }
        if (!watch.destroyed) ResolutionAfter(object, leaf);
        if (!watch.destroyed) ControlsAfter(object, frame, leaf);
        if (!watch.destroyed) VideoAfter(object, leaf);
        auto host = active;
        if (host && !host->retired && (leaf == L"SetFocus" || leaf == L"MenuStateChange")) DisabledStates(*host);
    }
    // The main menu page while it waits for input: no message box of its own, no plugin page open
    inline void* NoticePage()
    {
        auto controller = menuController;
        if (!Alive(controller) || active || UObject::GetState(L"EchelonMainHUD") == L"s_Cinematic") return nullptr;
        auto page = Object(controller, L"ActivePage");
        if (!Alive(page) || !Type(page, L"MenuMainMenu") || Object(page, L"theMsgBox")) return nullptr;
        return page;
    }
    inline bool NoticeReady()
    {
        return NoticePage() != nullptr;
    }
    // The update notice: the menus' own message box with an OK button, its text as given
    inline bool ShowNotice(const std::wstring& text)
    {
        auto page = NoticePage();
        if (!page) return false;
        constexpr uint8_t eMsgBtnBox_OK = 1;
        Call message(page, L"OpenMessageBox");
        if (message.Put(L"eType", L"ByteProperty", eMsgBtnBox_OK) && message.PutString(L"strCaption", text) &&
            message.PutBool(L"bNotLocalizeMsg", true) && message.Run())
            return true;
        LogOnce("notice", page, "native message box unavailable");
        return false;
    }
    inline void Retire(const HostRef& host, void* dying)
    {
        if (!host || host->retired) return;
        host->retired = true;
        RetireControls(*host, dying);
        RetireVideo(*host, dying);
        auto found = hosts.find(host->parent);
        if (found != hosts.end() && found->second == host) hosts.erase(found);
        if (active == host) { active.reset(); model.Close(); }
        if (dying != host->parent && dying != host->controller && Alive(host->parent) && Alive(host->controller))
        {
            for (auto [object, visible] : host->visibility) if (object != dying) Visible(object, visible);
            if (host->entry != dying) Visible(host->entry, false);
            if (host->panel != dying) Visible(host->panel, false);
            for (const auto& saved : host->placements) if (saved.object != dying) RestorePlacement(saved);
        }
        for (auto widget : host->widgets) captions.erase(widget);
        host->visibility.clear(); host->open = false; host->pendingOpen = false;
    }
    inline int __fastcall Destroy(void* object, void*)
    {
        ForgetResolution(object);
        for (auto watch = Watch::top; watch; watch = watch->previous)
            if (watch->object == object) watch->destroyed = true;
        std::vector<HostRef> snapshot;
        for (const auto& [parent, host] : hosts) snapshot.push_back(host);
        for (const auto& keep : snapshot)
        {
            const auto& host = *keep;
            bool tracked = ControlsTrack(host, object) || VideoTrack(host, object) || object == host.parent || object == host.controller || object == host.tabBox || object == host.entry ||
                object == host.panel || object == host.title || object == host.help || std::find(host.labels.begin(), host.labels.end(), object) != host.labels.end() ||
                std::find(host.values.begin(), host.values.end(), object) != host.values.end() ||
                std::find(host.widgets.begin(), host.widgets.end(), object) != host.widgets.end() ||
                std::any_of(host.stockTabs.begin(), host.stockTabs.end(), [object](const auto& item) { return item.first == object; }) ||
                std::any_of(host.visibility.begin(), host.visibility.end(), [object](auto item) { return item.first == object; });
            if (tracked) Retire(keep, object);
        }
        captions.erase(object); functionNames.erase(object);
        return destroyHook.thiscall<int>(object);
    }
    inline void Init()
    {
        auto core = GetModuleHandleW(L"Core.dll");
        if (internalHook)
        {
            return;
        }
        if (!core)
        {
            throw std::runtime_error("Core module unavailable");
        }
        auto resolve = [core](const char* oldName, const char* newName = nullptr)
        { auto value = GetProcAddress(core, oldName); return value || !newName ? value : GetProcAddress(core, newName); };
        fullName = reinterpret_cast<decltype(fullName)>(resolve("?GetFullName@UObject@@QBEPBGPAG@Z", "?GetFullName@UObject@@QBEPB_WPA_W@Z"));
        findObject = reinterpret_cast<decltype(findObject)>(resolve("?StaticFindObject@UObject@@SAPAV1@PAVUClass@@PAV1@PBGH@Z", "?StaticFindObject@UObject@@SAPAV1@PAVUClass@@PAV1@PB_WH@Z"));
        constructObject = reinterpret_cast<decltype(constructObject)>(resolve("?StaticConstructObject@UObject@@SAPAV1@PAVUClass@@PAV1@VFName@@K1PAVFOutputDevice@@H@Z"));
        processEvent = reinterpret_cast<decltype(processEvent)>(resolve("?ProcessEvent@UObject@@UAEXPAVUFunction@@PAX1@Z"));
        copyString = reinterpret_cast<decltype(copyString)>(resolve("?CopySingleValue@UStrProperty@@UBEXPAX0PAVUObject@@@Z"));
        copyArray = reinterpret_cast<decltype(copyArray)>(resolve("?CopyCompleteValue@UArrayProperty@@UBEXPAX0PAVUObject@@@Z"));
        stringInit = reinterpret_cast<decltype(stringInit)>(resolve("??0FString@@QAE@XZ"));
        stringAssign = reinterpret_cast<decltype(stringAssign)>(resolve("??4FString@@QAEAAV0@PBG@Z", "??4FString@@QAEAAV0@PB_W@Z"));
        stringDestroy = reinterpret_cast<decltype(stringDestroy)>(resolve("??1FString@@QAE@XZ"));
        getLanguage = reinterpret_cast<decltype(getLanguage)>(resolve("?GetLanguage@UObject@@SAPBGH@Z", "?GetLanguage@UObject@@SAPB_WH@Z"));
        auto internal = resolve("?ProcessInternal@UObject@@QAEXAAUFFrame@@QAX@Z");
        auto destroy = resolve("?ConditionalDestroy@UObject@@QAEHXZ");
        auto boolCopy = resolve("?CopySingleValue@UBoolProperty@@UBEXPAX0PAVUObject@@@Z");
        if (!fullName || !findObject || !constructObject || !processEvent || !copyString || !copyArray ||
            !stringInit || !stringAssign || !stringDestroy || !getLanguage || !internal || !destroy || !boolCopy)
        {
            throw std::runtime_error("Required Core export unavailable");
        }
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&hostModule), &hostModule) || !hostModule)
        {
            throw std::runtime_error("Native menu host module unavailable");
        }
        destroyHook = safetyhook::create_inline(destroy, Destroy);
        if (!destroyHook)
        {
            throw std::runtime_error("ConditionalDestroy hook failed");
        }
        internalHook = safetyhook::create_inline(internal, Internal);
        if (!internalHook)
        {
            destroyHook.reset();
            throw std::runtime_error("ProcessInternal hook failed");
        }
    }
}

// Shipped PT KeyListBoxItem arrays remain engine-owned. Nonzero special-row
// flags keep these rows out of native key-binding refresh/save/restore loops.
export namespace PTMenu
{
    inline SafetyHookInline controlsDrawHook;
    inline void (__fastcall* destroyArray)(void*, void*, void*) = nullptr;
    inline const std::set<std::string> ControlSettings{ "MAIN.RawInputMouse", "MAIN.RawInputMouseRawData", "MAIN.MouseSensitivity" };
    struct KeyItem
    {
        String caption{}, key{}, alternate{}, action{};
        int title = 0, line = 0, flipped = 0, checkbox = 0, scroll = 0, disabled = 0;
    };
    static_assert(sizeof(KeyItem) == 88);
    struct Controls
    {
        void* box = nullptr;
        void* list = nullptr;
        void* scroll = nullptr;
        void* help = nullptr;
        size_t first = 0;
        bool joined = false, attempted = false, restart = false;
        RWGTAMenu::Model model;
        std::vector<RWGTAMenu::Row> rows;
        std::vector<Item> text;
        struct Action { size_t row; int direction; bool defaults; };
        std::vector<Action> pending;
    };
    inline std::set<std::string> JoinedControlIDs(const Host& host)
    {
        std::set<std::string> result;
        if (host.controls && host.controls->joined)
            result.insert(ControlSettings.begin(), ControlSettings.end());
        if (ResolutionJoined(host.parent))
        {
            result.insert("MAIN.ResX");
            result.insert("MAIN.ResY");
        }
        if (VideoJoined(host))
            result.insert("MAIN.WindowMode");
        return result;
    }
    inline bool ControlsTrack(const Host& host, void* object)
    {
        const auto state = host.controls;
        return state && (object == state->box || object == state->list || object == state->scroll || object == state->help);
    }
    inline void RetireControls(Host& host, void* dying)
    {
        const auto state = host.controls;
        if (!state) return;
        if (dying != state->help && dying != host.parent && dying != host.controller) Visible(state->help, false);
        state->joined = false; state->pending.clear(); state->model.Close();
        captions.erase(state->help);
    }
    inline bool KeyArray(void* owner, const wchar_t* name, void*& prop, Array& array)
    {
        prop = Property(owner, name, L"ArrayProperty", sizeof(Array));
        auto inner = prop ? Read<void*>(prop, 84) : nullptr;
        auto structure = inner && Type(inner, L"StructProperty") ? Read<void*>(inner, 84) : nullptr;
        if (!structure || !(Read<uint32_t>(inner, 60) & 0x400000) || Read<uint16_t>(inner, 58) != sizeof(KeyItem) || Read<int>(structure, 64) != sizeof(KeyItem)) return false;
        struct Member { const wchar_t* name; const wchar_t* kind; uint16_t size, offset; };
        for (const auto& member : {
            Member{ L"m_szCaption", L"StrProperty", 16, 0 }, { L"m_szKeyName", L"StrProperty", 16, 16 },
            { L"m_szAltKeyName", L"StrProperty", 16, 32 }, { L"m_szActionKey", L"StrProperty", 16, 48 },
            { L"m_bIsTitle", L"IntProperty", 4, 64 }, { L"m_bIsLine", L"IntProperty", 4, 68 },
            { L"m_bDrawFlipped", L"IntProperty", 4, 72 }, { L"m_bIsCheckBoxLine", L"IntProperty", 4, 76 },
            { L"m_bIsScrollLine", L"IntProperty", 4, 80 }, { L"m_bIsDisabled", L"IntProperty", 4, 84 } })
        {
            auto field = ClassProperty(structure, member.name, member.kind, member.size);
            if (!field || Read<uint16_t>(field, 68) != member.offset) return false;
        }
        array = Read<Array>(Address(owner, prop), 0);
        return array.count >= 0 && array.count <= 512 && array.capacity >= array.count && array.capacity <= 65536 && (!array.count || array.data);
    }
    inline bool NativeText(const String& value, std::wstring& text)
    {
        if (!value.count) { text.clear(); return !value.data || value.capacity >= 0; }
        if (!value.data || value.count < 0 || value.count > 1024 || value.capacity < value.count) return false;
        const auto data = static_cast<const wchar_t*>(value.data);
        if (data[value.count - 1]) return false;
        text.assign(data, size_t(value.count - 1)); return true;
    }
    inline std::wstring Wide(const std::string& text)
    {
        int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        std::wstring result(count, L'\0');
        if (count) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), result.data(), count);
        for (auto& c : result) if (c < 32 || c == 8232 || c == 8233) c = L' ';
        return result;
    }
    inline bool JoinedArray(const Controls& state, void* owner, const wchar_t* name, void*& prop, Array& array)
    {
        if (!KeyArray(owner, name, prop, array) || state.first + state.rows.size() > size_t(array.count)) return false;
        auto items = static_cast<KeyItem*>(array.data);
        for (size_t i = 0; i < state.rows.size(); ++i)
        {
            const auto& item = items[state.first + i];
            std::wstring marker;
            if (item.title || item.line || item.scroll != 2 || item.checkbox || !NativeText(item.action, marker) ||
                marker != L"WFP_MOUSE_" + std::to_wstring(i)) return false;
        }
        return true;
    }
    inline int SelectedControl(const Controls& state)
    {
        int top = -1, selected = -1, visible = 0;
        void* prop = nullptr; Array array{};
        if (!state.joined || !JoinedArray(state, state.list, L"KeyListItems", prop, array) ||
            !Get(state.list, L"iCurTop", L"IntProperty", top) || !Get(state.list, L"iCurSelected", L"IntProperty", selected) ||
            !Get(state.list, L"iListNum", L"IntProperty", visible) || top < 0 || top > array.count || selected < 0 ||
            selected >= visible || visible < 1 || visible > 32 || selected >= array.count - top) return -1;
        auto index = size_t(top + selected);
        return index >= state.first && index - state.first < state.rows.size() ? int(index - state.first) : -1;
    }
    inline bool ControlsCurrent(const HostRef& host, const std::shared_ptr<Controls>& state)
    {
        return Current(host) && host->controls == state && state && state->joined && !host->open &&
            Alive(state->box) && Alive(state->list) && Bool(state->box, L"bVisible");
    }
    struct OwnedKeyArray
    {
        void* prop;
        Array value{};
        ~OwnedKeyArray() { if (prop && value.data) destroyArray(prop, nullptr, &value); }
        void Copy(void* owner, const std::vector<KeyItem>& items)
        {
            Array source{ const_cast<KeyItem*>(items.data()), int(items.size()), int(items.size()) };
            copyArray(prop, nullptr, &value, &source, owner);
        }
        void Publish(void* owner) { std::swap(*static_cast<Array*>(Address(owner, prop)), value); }
    };
    inline bool JoinControls(const HostRef& host, const std::shared_ptr<Controls>& state)
    {
        bool pause = false;
        if (!Current(host) || !PauseContext(*host, pause) || host->open || !controlsDrawHook || !destroyArray || !state || state->joined) return false;
        host->pause = pause;
        state->box = Object(host->parent, L"ControlsListBox");
        state->list = Object(state->box, L"List");
        state->scroll = Object(state->list, L"ScrollBarComponet");
        auto buttons = Object(host->parent, L"boxButtons");
        if (!Type(state->box, L"GUIKeyListBoxSC2_PC") || !Type(state->list, L"GUIKeyListSC2_PC") || !state->scroll || !buttons) return false;
        auto listClass = Read<void*>(state->list, 36), boxClass = Read<void*>(state->box, 36);
        if (!InputContract(listClass) || !FunctionContract(listClass, L"KeyPressed", {{ L"Key", L"IntProperty", 4 }}) ||
            !FunctionContract(boxClass, L"OnClickHandle", {{ L"Sender", L"ObjectProperty", 4 }, { L"ReturnValue", L"BoolProperty", 4 }}) ||
            !FunctionContract(Read<void*>(host->parent, 36), L"ResetControlsToDefault", {}) ||
            !FunctionContract(Read<void*>(state->scroll, 36), L"SetParams", {{ L"iTotal", L"IntProperty", 4 },
                { L"iCurIndex", L"IntProperty", 4 }, { L"iDisplayed", L"IntProperty", 4 }})) return false;
        auto body = Bounds(state->box), footer = Bounds(buttons);
        Rect hint{};
        if (!NativeHint(host, body, hint)) return false;
        auto fits = [](Rect rect) { return std::isfinite(rect.x) && std::isfinite(rect.y) && std::isfinite(rect.w) &&
            std::isfinite(rect.h) && rect.x >= 0 && rect.y >= 0 && rect.w >= 200 && rect.h >= 14 && rect.x + rect.w <= 640 && rect.y + rect.h <= 480; };
        if (!fits(body) || !fits(footer) || !fits(hint) || body.y + body.h > footer.y) return false;
        std::vector<void*> children;
        if (!ArrayValues(host->parent, children)) return false;
        for (auto child : children)
        {
            if (!child || child == host->panel || child == host->tabBox || child == state->box || child == buttons ||
                child == Object(host->parent, L"boxSound") || child == Object(host->parent, L"boxVideo")) continue;
            auto rect = Bounds(child);
            if (Bool(child, L"bVisible") && rect.w > 0 && rect.h > 0 && rect.x < hint.x + hint.w && hint.x < rect.x + rect.w &&
                rect.y < hint.y + hint.h && hint.y < rect.y + rect.h) return false;
        }
        void *outerProp = nullptr, *liveProp = nullptr, *oldProp = nullptr;
        Array outer{}, live{}, old{};
        if (!KeyArray(state->box, L"Items", outerProp, outer) || !KeyArray(state->list, L"KeyListItems", liveProp, live) ||
            !KeyArray(state->list, L"KeyListItemsOldVaue", oldProp, old) || outer.count < 6 || live.count != outer.count ||
            (old.count && old.count != live.count)) return false;
        int top = -1, selected = -1, visible = 0;
        if (!Get(state->list, L"iCurTop", L"IntProperty", top) || !Get(state->list, L"iCurSelected", L"IntProperty", selected) ||
            !Get(state->list, L"iListNum", L"IntProperty", visible) || top != 0 || selected < 0 || selected > 3 || visible < 1 || visible > 32 ||
            !Property(state->list, L"m_bModified", L"BoolProperty", 4) || Bool(state->list, L"m_bModified")) return false;
        auto source = static_cast<KeyItem*>(live.data), outerItems = static_cast<KeyItem*>(outer.data);
        size_t insertion = SIZE_MAX;
        for (int i = 0; i < live.count; ++i)
        {
            std::wstring marker;
            if (!NativeText(source[i].action, marker) || marker.starts_with(L"WFP_MOUSE_")) return false;
            if (source[i].scroll != 1) continue;
            if (insertion != SIZE_MAX || outerItems[i].scroll != 1) return false;
            insertion = size_t(i + 1);
        }
        if (insertion == SIZE_MAX) return false;
        state->model.OpenPlugins(hostModule, Language(), 4096, ControlSettings);
        if (!Current(host) || host->controls != state || state->model.JoinedRows().size() != ControlSettings.size()) return false;
        state->rows = state->model.JoinedRows();
        // A plugin getter may reenter native code. Revalidate the three exact
        // array owners after discovery before reading the borrowed old rows.
        Array checkOuter{}, checkLive{}, checkOld{}; void* checkProp = nullptr;
        if (!KeyArray(state->box, L"Items", checkProp, checkOuter) || checkProp != outerProp ||
            !KeyArray(state->list, L"KeyListItems", checkProp, checkLive) || checkProp != liveProp ||
            !KeyArray(state->list, L"KeyListItemsOldVaue", checkProp, checkOld) || checkProp != oldProp ||
            std::memcmp(&checkOuter, &outer, sizeof(Array)) || std::memcmp(&checkLive, &live, sizeof(Array)) ||
            std::memcmp(&checkOld, &old, sizeof(Array))) return false;
        struct NewItems
        {
            KeyItem items[3]{};
            NewItems() { for (auto& item : items) for (auto str : { &item.caption, &item.key, &item.alternate, &item.action }) stringInit(str, nullptr); }
            ~NewItems() { for (auto& item : items) for (auto str : { &item.caption, &item.key, &item.alternate, &item.action }) stringDestroy(str, nullptr); }
        } additions;
        for (size_t i = 0; i < state->rows.size(); ++i)
        {
            auto& item = additions.items[i]; item.scroll = 2;
            stringAssign(&item.action, nullptr, (L"WFP_MOUSE_" + std::to_wstring(i)).c_str());
        }
        auto expanded = [&](const Array& array)
        {
            std::vector<KeyItem> items;
            if (array.count) items.assign(static_cast<KeyItem*>(array.data), static_cast<KeyItem*>(array.data) + array.count);
            if (array.count) items.insert(items.begin() + insertion, std::begin(additions.items), std::end(additions.items));
            return items;
        };
        OwnedKeyArray nextOuter{ outerProp }, nextLive{ liveProp }, nextOld{ oldProp };
        nextOuter.Copy(state->box, expanded(outer)); nextLive.Copy(state->list, expanded(live));
        if (old.count) nextOld.Copy(state->list, expanded(old));
        if (nextOuter.value.count != outer.count + 3 || nextLive.value.count != live.count + 3 ||
            (old.count && nextOld.value.count != old.count + 3)) return false;
        auto cls = findObject(nullptr, nullptr, L"GUI.GUIButtonSC2", 0);
        if (!WidgetContract(cls)) return false;
        state->help = constructObject(cls, host->parent, 0, 0, nullptr, nullptr, 0);
        if (!state->help || host->retired) return false;
        host->widgets.push_back(state->help);
        struct HelpCleanup
        {
            HostRef host; std::shared_ptr<Controls> state; bool published = false;
            ~HelpCleanup()
            {
                if (published || host->retired) return;
                Visible(state->help, false); Attach(host->parent, state->help, false); captions.erase(state->help);
            }
        } helpCleanup{ host, state };
        Visible(state->help, false);
        bool no = false;
        Bool(state->help, L"bNeedLocalized", &no); Bool(state->help, L"bAcceptsInput", &no); Bool(state->help, L"bTabStop", &no);
        if (!Position(state->help, hint) || !Attach(host->parent, state->help) || !InitComponent(state->help, host->controller, host->parent) || host->retired) return false;
        Visible(state->help, false);
        // All new arrays have independent native string ownership before the
        // publication. Their destructors free only the old arrays after swaps.
        state->first = insertion;
        nextOuter.Publish(state->box); nextLive.Publish(state->list);
        if (old.count) nextOld.Publish(state->list);
        state->joined = true; helpCleanup.published = true;
        Call params(state->scroll, L"SetParams");
        if (params.Put(L"iTotal", L"IntProperty", live.count + 3) && params.Put(L"iCurIndex", L"IntProperty", top) &&
            params.Put(L"iDisplayed", L"IntProperty", visible)) params.Run();
        if (host->retired) return false;
        return true;
    }
    inline void DrainControls(const HostRef& host)
    {
        if (!Current(host))
        {
            if (host && !host->retired && host->controls) { host->controls->pending.clear(); Visible(host->controls->help, false); }
            return;
        }
        if (!host->controls) host->controls = std::make_shared<Controls>();
        const auto state = host->controls;
        if (!state->attempted && !host->open)
        {
            state->attempted = true;
            if (!JoinControls(host, state) && !host->retired) LogOnce("controls", host->parent, "native row/help/ownership contract unavailable; mouse settings remain in Plugins");
        }
        if (!ControlsCurrent(host, state)) { if (!host->retired) Visible(state->help, false); state->pending.clear(); return; }
        auto actions = std::move(state->pending); state->pending.clear();
        for (const auto& action : actions)
        {
            if (!ControlsCurrent(host, state)) return;
            if (action.defaults)
            {
                if (RWGTAMenu::RestoreDefaults(FixSettings(), ControlSettings)) state->restart = true;
            }
            else if (action.row < state->rows.size()) state->model.Change(state->rows[action.row], action.direction, host->pause);
        }
        if (!ControlsCurrent(host, state)) return;
        PauseContext(*host, host->pause);
        const auto snapshot = state->model;
        std::vector<Item> next;
        for (auto row : state->rows)
        {
            Item item; item.row = row; item.label = snapshot.Label(row); item.value = snapshot.Value(row);
            if (!ControlsCurrent(host, state)) return;
            item.help = snapshot.Help(row); item.enabled = snapshot.Enabled(row, host->pause);
            if (!ControlsCurrent(host, state)) return;
            const char* key = !row.setting ? nullptr : row.setting->id == "MAIN.RawInputMouse" ? "menu.rawmouse" :
                row.setting->id == "MAIN.RawInputMouseRawData" ? "menu.rawdata" :
                row.setting->id == "MAIN.MouseSensitivity" ? "menu.rawsensitivity" : nullptr;
            if (key)
            {
                item.label = snapshot.Text(std::string(key) + ".label");
                item.help = snapshot.Text(std::string(key) + ".help");
            }
            next.push_back(std::move(item));
        }
        state->text = std::move(next);
        std::string help;
        int selected = SelectedControl(*state);
        if (selected >= 0) help = state->text[size_t(selected)].help;
        if (state->restart || state->model.RestartRequired()) help = state->model.Text("menu.restartwarning");
        StringValue(state->help, L"Caption", help); Visible(state->help, !help.empty());
    }
    inline bool ControlsBefore(void* object, void* frame, void* result, const std::wstring& leaf)
    {
        for (const auto& [parent, host] : hosts)
        {
            const auto state = host->controls;
            if (!ControlsCurrent(host, state) || (object != state->list && object != state->box)) continue;
            int selected = SelectedControl(*state);
            if (selected < 0) continue;
            if (leaf == L"KeyPressed") return true;
            if (object == state->list && leaf == L"OnKeyEvent")
            {
                uint8_t key = 0, phase = 0;
                if (!Locals(frame, L"Key", L"ByteProperty", 1, &key) || !Locals(frame, L"State", L"ByteProperty", 1, &phase)) continue;
                bool left = key == 37 || key == 100 || key == 198 || key == 215;
                bool right = key == 39 || key == 102 || key == 199 || key == 213;
                bool confirm = key == 13 || key == 32 || key == 202;
                if (!(left || right || confirm) || !Result(frame, result)) continue;
                if (((left || right) && (phase == 1 || phase == 2)) || (confirm && phase == 3))
                    if (state->pending.size() < 64) state->pending.push_back({ size_t(selected), left ? -1 : right ? 1 : 0, false });
                return true;
            }
            if (object == state->box && leaf == L"OnDblClickHandle" && Result(frame, result)) return true;
        }
        return false;
    }
    inline void ControlsAfter(void* object, void* frame, const std::wstring& leaf)
    {
        for (const auto& [parent, host] : hosts)
        {
            const auto state = host->controls;
            if (!state || !state->joined) continue;
            if (object == host->parent && leaf == L"OnOpen") { state->pending.clear(); continue; }
            if (!ControlsCurrent(host, state)) continue;
            if (object == host->parent && leaf == L"ResetControlsToDefault")
            { if (state->pending.size() < 64) state->pending.push_back({ 0, 0, true }); }
            else if (object == state->box && leaf == L"OnClickHandle")
            {
                void* sender = nullptr;
                if (!Locals(frame, L"Sender", L"ObjectProperty", 4, &sender) || sender != state->list || Bool(state->list, L"m_bNoItemSelectedOnClick")) continue;
                int selected = SelectedControl(*state);
                if (selected >= 0 && state->pending.size() < 64) state->pending.push_back({ size_t(selected), 0, false });
            }
        }
    }
    inline void __fastcall DrawControls(void* object, void*, void* canvas)
    {
        // Only cached model text is used in drawing; no plugin callbacks run
        // while the engine iterates its list or renders its font.
        for (const auto& [parent, host] : hosts)
        {
            const auto state = host->controls;
            if (!ControlsCurrent(host, state) || object != state->list || !canvas || state->text.size() != state->rows.size()) continue;
            void *prop = nullptr, *outerProp = nullptr; Array live{}, outer{};
            if (!JoinedArray(*state, object, L"KeyListItems", prop, live) || !JoinedArray(*state, state->box, L"Items", outerProp, outer)) continue;
            std::vector<void*> fonts;
            auto key = ReadString(object, L"FontName"); void* font = nullptr;
            if (ArrayValues(host->controller, fonts, L"FontStack")) for (auto candidate : fonts)
                if (_wcsicmp(ReadString(candidate, L"KeyName").c_str(), key.c_str()) == 0) { font = Object(candidate, L"OnlyDefaultFont"); break; }
            auto rect = Bounds(object);
            if (!font || !std::isfinite(rect.w) || rect.w < 200 || rect.w > 640) continue;
            auto previous = Object(canvas, L"Font");
            if (!Set(canvas, L"Font", L"ObjectProperty", font)) continue;
            auto fit = [&](std::wstring value, float width)
            {
                auto measure = [&](const std::wstring& text) { float w = 0, h = 0; textSize(canvas, nullptr, text.c_str(), &w, &h); return std::isfinite(w) && std::isfinite(h) && h <= 22 && (text.empty() || w > 0) ? w : FLT_MAX; };
                for (auto& c : value) if (c != L' ' && measure(std::wstring(1, c)) == FLT_MAX) c = L'?';
                if (measure(value) <= width) return value;
                size_t lo = 0, hi = value.size();
                while (lo < hi) { auto mid = (lo + hi + 1) / 2; if (measure(value.substr(0, mid) + L"...") <= width) lo = mid; else hi = mid - 1; }
                if (lo && value[lo - 1] >= 0xD800 && value[lo - 1] <= 0xDBFF) --lo;
                return measure(value.substr(0, lo) + L"...") <= width ? value.substr(0, lo) + L"..." : std::wstring{};
            };
            for (size_t i = 0; i < state->text.size(); ++i)
            {
                const auto& item = state->text[i];
                auto label = fit(Wide(item.label), rect.w * .4f - 12);
                auto value = fit(Wide(item.enabled ? item.value : state->model.Text("menu.unavailable")), rect.w * .3f - 8);
                for (auto array : { live, outer })
                {
                    auto& native = static_cast<KeyItem*>(array.data)[state->first + i];
                    stringAssign(&native.caption, nullptr, label.c_str()); stringAssign(&native.key, nullptr, value.c_str());
                    native.disabled = item.enabled ? 0 : 1;
                }
            }
            Set(canvas, L"Font", L"ObjectProperty", previous);
        }
        controlsDrawHook.thiscall<void>(object, canvas);
    }
    inline void InitControlsDrawing()
    {
        if (controlsDrawHook) return;
        auto gui = GetModuleHandleW(L"GUI.dll"), core = GetModuleHandleW(L"Core.dll");
        if (!gui || !core)
        {
            throw std::runtime_error("GUI or Core module unavailable");
        }
        auto draw = GetProcAddress(gui, "?Draw@UGUIKeyListSC2_PC@@UAEXPAVUCanvas@@@Z");
        destroyArray = reinterpret_cast<decltype(destroyArray)>(GetProcAddress(core, "?DestroyValue@UArrayProperty@@UBEXPAX@Z"));
        if (!draw || !destroyArray)
        {
            throw std::runtime_error("Native controls export unavailable");
        }
        controlsDrawHook = safetyhook::create_inline(draw, DrawControls);
        if (!controlsDrawHook)
        {
            destroyArray = nullptr;
            throw std::runtime_error("Native controls drawing hook installation failed");
        }
    }
}

export namespace PTMenu
{
    inline SafetyHookInline resolutionApplyHook;
    struct ResolutionState
    {
        void* combo = nullptr;
        void* list = nullptr;
    };
    inline std::map<void*, ResolutionState> resolutions;

    struct ResolutionItem
    {
        String text{};
        uint32_t enabled = 1;
    };
    static_assert(sizeof(ResolutionItem) == 20);

    inline bool ResolutionArray(void* owner, const wchar_t* name, void*& prop, Array& value)
    {
        prop = Property(owner, name, L"ArrayProperty", sizeof(Array));
        auto inner = prop ? Read<void*>(prop, 84) : nullptr;
        auto structure = inner && Type(inner, L"StructProperty") ? Read<void*>(inner, 84) : nullptr;
        if (!structure || Leaf(structure) != L"Pair_string_bool" ||
            !(Read<uint32_t>(inner, 60) & 0x400000) || Read<uint16_t>(inner, 58) != sizeof(ResolutionItem) ||
            Read<int>(structure, 64) != sizeof(ResolutionItem))
            return false;
        auto text = ClassProperty(structure, L"strText", L"StrProperty", sizeof(String));
        auto enabled = ClassProperty(structure, L"bValue", L"BoolProperty", 4);
        if (!text || !enabled || Read<uint16_t>(text, 68) != 0 ||
            Read<uint16_t>(enabled, 68) != 16 || Read<uint32_t>(enabled, 84) != 1)
            return false;
        value = Read<Array>(Address(owner, prop), 0);
        return value.count >= 0 && value.count <= 512 && value.capacity >= value.count &&
            value.capacity <= 65536 && (!value.count || value.data);
    }

    inline bool ResolutionJoined(void* owner)
    {
        auto it = resolutions.find(owner);
        if (it == resolutions.end() || NativeDisplayModes.empty() || !Alive(owner) || !Alive(it->second.combo) || !Alive(it->second.list) ||
            Object(owner, L"m_ComboResolution") != it->second.combo || Object(it->second.combo, L"ComboList") != it->second.list)
            return false;
        const auto& state = it->second;
        void* prop = nullptr;
        Array value{};
        return ResolutionArray(owner, L"m_ComboResolutionData", prop, value) && value.count == int(NativeDisplayModes.size()) &&
            ResolutionArray(state.combo, L"Itemlist", prop, value) && value.count == int(NativeDisplayModes.size()) &&
            ResolutionArray(state.list, L"ListButtons", prop, value) && value.count == int(NativeDisplayModes.size());
    }

    inline bool ResolutionString(const String& value, std::wstring& text)
    {
        if (!value.data || value.count < 2 || value.count > 32 || value.capacity < value.count || value.capacity > 65536)
            return false;
        const auto data = static_cast<const wchar_t*>(value.data);
        if (data[value.count - 1] != L'\0')
            return false;
        text.assign(data, value.count - 1);
        return true;
    }

    inline std::wstring ResolutionName(const std::pair<int, int>& mode)
    {
        return std::to_wstring(mode.first) + L"x" + std::to_wstring(mode.second);
    }

    inline void __fastcall ApplyResolution(void* options, void*, void* frame, void* result)
    {
        std::optional<std::pair<int, int>> requested;
        String value{};
        std::wstring text;
        if (NativeResolutionListPublished && Get(options, L"Resolution", L"StrProperty", value) && ResolutionString(value, text))
            for (const auto& mode : NativeDisplayModes)
                if (ResolutionName(mode) == text)
                {
                    requested = mode;
                    break;
                }
        struct Restore
        {
            std::optional<std::pair<int, int>> previous;
            ~Restore() { RequestedNativeResolution = previous; }
        } restore{ std::exchange(RequestedNativeResolution, requested) };
        resolutionApplyHook.thiscall<void>(options, frame, result);
    }

    inline bool InitResolutionApply()
    {
        if (resolutionApplyHook)
            return true;
        auto entry = GetProcAddress(GetModuleHandleW(L"Engine.dll"), "?execUpdateEngineSettings@UEPCGameOptions@@QAEXAAUFFrame@@QAX@Z");
        resolutionApplyHook = safetyhook::create_inline(entry, ApplyResolution);
        return bool(resolutionApplyHook);
    }

    inline bool PrepareResolutions(void* owner)
    {
        if (ResolutionJoined(owner))
            return true;
        if (busy || NativeDisplayModes.empty() || NativeDisplayModes.size() > 512 || !destroyArray || !InitResolutionApply())
            return false;
        auto combo = Object(owner, L"m_ComboResolution");
        auto list = Object(combo, L"ComboList");
        if (!Alive(combo) || !Alive(list) || !Type(combo, L"GUIComboBoxSC2_PC") || !Type(list, L"GUIListSC2_PC"))
            return false;
        void* props[3]{};
        Array old[3]{};
        if (!ResolutionArray(owner, L"m_ComboResolutionData", props[0], old[0]) ||
            !ResolutionArray(combo, L"Itemlist", props[1], old[1]) || !ResolutionArray(list, L"ListButtons", props[2], old[2]) ||
            old[0].count < 1 || old[1].count != old[0].count || old[2].count != old[0].count)
            return false;
        int cap = 0, visible = 0, selected = 0, comboVisible = 0;
        if (!Get(list, L"iInitListNum", L"IntProperty", cap) || !Get(list, L"iListNum", L"IntProperty", visible) ||
            !Get(combo, L"iListNum", L"IntProperty", comboVisible) || !Get(combo, L"bCurrentValue", L"IntProperty", selected) ||
            cap < 1 || cap > 8 || visible < 1 || visible > 8 || selected < 0 || selected >= old[0].count)
            return false;
        Call dims(combo, L"InitDims"), select(combo, L"SetSelectedIndex");
        int current = 0;
        for (size_t i = 0; i < NativeDisplayModes.size(); ++i)
            if (NativeDisplayModes[i] == std::pair{ Screen.Width, Screen.Height })
                current = int(i);
        if (!dims.function || !select.Put(L"iIndex", L"IntProperty", current))
            return false;
        struct Busy
        {
            bool previous;
            Busy() : previous(busy) { busy = true; }
            ~Busy() { busy = previous; }
        } active;
        struct PublishFlag
        {
            bool previous = NativeResolutionListPublished;
            bool committed = false;
            PublishFlag() { NativeResolutionListPublished = true; }
            ~PublishFlag() { if (!committed) NativeResolutionListPublished = previous; }
        } flag;
        struct Items
        {
            std::vector<ResolutionItem> values;
            explicit Items(size_t count) : values(count) { for (auto& item : values) stringInit(&item.text, nullptr); }
            ~Items() { for (auto& item : values) stringDestroy(&item.text, nullptr); }
        } input(NativeDisplayModes.size());
        for (size_t i = 0; i < input.values.size(); ++i)
            stringAssign(&input.values[i].text, nullptr, ResolutionName(NativeDisplayModes[i]).c_str());
        struct OwnedArray
        {
            void* prop;
            Array value{};
            ~OwnedArray() { if (value.data) destroyArray(prop, nullptr, &value); }
            void Publish(void* owner) { std::swap(*static_cast<Array*>(Address(owner, prop)), value); }
        } next[]{ { props[0] }, { props[1] }, { props[2] } };
        const Array source{ input.values.data(), int(input.values.size()), int(input.values.size()) };
        void* owners[]{ owner, combo, list };
        for (size_t i = 0; i < 3; ++i)
        {
            copyArray(props[i], nullptr, &next[i].value, &source, owners[i]);
            if (next[i].value.count != source.count || !next[i].value.data)
                return false;
            const auto items = static_cast<ResolutionItem*>(next[i].value.data);
            for (int j = 0; j < source.count; ++j)
            {
                std::wstring value;
                if (items[j].enabled != 1 || !ResolutionString(items[j].text, value) || value != ResolutionName(NativeDisplayModes[j]))
                    return false;
            }
        }
        Watch ownerWatch(owner), comboWatch(combo), listWatch(list);
        for (size_t i = 0; i < 3; ++i)
            next[i].Publish(owners[i]);
        resolutions[owner] = { combo, list };
        const int shown = std::min({ visible, comboVisible, source.count });
        bool applied = Set(list, L"iListNum", L"IntProperty", shown) && Set(combo, L"iListNum", L"IntProperty", shown) &&
            dims.Run() && !ownerWatch.destroyed && !comboWatch.destroyed && !listWatch.destroyed && select.Run();
        const bool alive = !ownerWatch.destroyed && !comboWatch.destroyed && !listWatch.destroyed;
        if (!applied || !alive || !ResolutionJoined(owner))
        {
            resolutions.erase(owner);
            if (alive)
            {
                for (size_t i = 0; i < 3; ++i)
                    next[i].Publish(owners[i]);
                Set(list, L"iListNum", L"IntProperty", visible);
                Set(combo, L"iListNum", L"IntProperty", comboVisible);
                dims.Run();
                if (select.Put(L"iIndex", L"IntProperty", selected)) select.Run();
            }
            return false;
        }
        flag.committed = true;
        return true;
    }

    inline bool ResolutionBefore(void* object, void* frame, void* result, const std::wstring& leaf)
    {
        if (!ResolutionJoined(object))
            return false;
        if (leaf == L"InitComboResolutionList")
            return true;
        if ((leaf != L"GetResolution" && leaf != L"GetResolutionIndex") || !result)
            return false;
        const auto function = Read<void*>(frame, 4);
        if (function != Field(Read<void*>(object, 36), leaf.c_str(), L"Function"))
            return false;
        if (leaf == L"GetResolution")
        {
            int index = 0;
            auto returned = Field(function, L"ReturnValue", L"StrProperty");
            if (!returned || Read<uint16_t>(returned, 58) != sizeof(String) ||
                Read<uint16_t>(returned, 68) != Read<uint16_t>(function, 102) ||
                !Locals(frame, L"index", L"IntProperty", sizeof(index), &index))
                return false;
            index = std::clamp(index, 0, int(NativeDisplayModes.size()) - 1);
            stringAssign(result, nullptr, ResolutionName(NativeDisplayModes[index]).c_str());
            return true;
        }
        String value{};
        std::wstring text;
        auto returned = Field(function, L"ReturnValue", L"IntProperty");
        if (!returned || Read<uint16_t>(returned, 58) != sizeof(int) ||
            Read<uint16_t>(returned, 68) != Read<uint16_t>(function, 102) ||
            !Locals(frame, L"_szRes", L"StrProperty", sizeof(String), &value) || !ResolutionString(value, text))
            return false;
        int index = 0;
        for (size_t i = 0; i < NativeDisplayModes.size(); ++i)
            if (ResolutionName(NativeDisplayModes[i]) == text)
                index = int(i);
        injector::WriteMemory<int>(static_cast<int*>(result), index, true);
        return true;
    }

    inline void ResolutionAfter(void* object, const std::wstring& leaf)
    {
        if ((leaf == L"InitComboResolutionList" || leaf == L"OnOpen" || leaf == L"RefreshVideo") && Type(object, L"MenuOptions"))
            PrepareResolutions(object);
    }

    inline void ForgetResolution(void* object)
    {
        for (auto it = resolutions.begin(); it != resolutions.end();)
            if (it->first == object || it->second.combo == object || it->second.list == object)
                it = resolutions.erase(it);
            else
                ++it;
    }
}

export namespace PTMenu
{
    inline const std::set<std::string> VideoSettings{ "MAIN.WindowMode" };
    struct Video
    {
        void* box = nullptr;
        void* label = nullptr;
        void* combo = nullptr;
        void* list = nullptr;
        void* listBox = nullptr;
        void* help = nullptr;
        void* resolution = nullptr;
        void* quality = nullptr;
        void* qualityLabel = nullptr;
        void* buttons = nullptr;
        bool joined = false, attempted = false, dropdown = false, restart = false;
        bool nativeHidden = false, nativeVisible = false;
        std::optional<int> pending;
        std::string language;
        uint64_t refreshed = 0;
        RWGTAMenu::Model model;
        std::vector<RWGTAMenu::Row> rows;
        struct Flag { void* object; const wchar_t* name; bool value; };
        std::vector<Flag> flags;
    };
    struct VideoDelegate
    {
        void* object = nullptr;
        uint32_t name = 0;
    };
    static_assert(sizeof(VideoDelegate) == 8);

    inline bool VideoJoined(const Host& host)
    {
        const auto state = host.video;
        if (!state || !state->joined || !Alive(state->box) || !Alive(state->combo) || !Alive(state->label) ||
            !Alive(state->list) || !Alive(state->listBox) || !Alive(state->help) ||
            Object(host.parent, L"boxVideo") != state->box || Object(state->combo, L"ComboList") != state->list ||
            Object(state->combo, L"boxListBox") != state->listBox)
            return false;
        void* property = nullptr;
        Array value{};
        return ResolutionArray(state->combo, L"Itemlist", property, value) && value.count == 3 &&
            ResolutionArray(state->list, L"ListButtons", property, value) && value.count == 3;
    }

    inline void RestoreVideoFlags(const std::shared_ptr<Video>& state, void* dying = nullptr)
    {
        for (const auto& flag : state->flags)
        {
            if (flag.object == dying || !Alive(flag.object)) continue;
            auto value = flag.value;
            Bool(flag.object, flag.name, &value);
        }
        state->flags.clear();
        state->dropdown = false;
    }

    inline void RetireVideo(Host& host, void* dying)
    {
        const auto state = host.video;
        if (!state) return;
        const bool published = state->joined;
        state->joined = false;
        state->pending.reset();
        if (published && dying != host.parent && dying != host.controller)
        {
            RestoreVideoFlags(state, dying);
            for (auto object : { state->label, state->combo, state->help, state->listBox })
                if (object != dying) Visible(object, false);
        }
        state->rows.clear();
        state->model.Close();
    }

    inline bool VideoTrack(const Host& host, void* object)
    {
        const auto state = host.video;
        return state && ((object && (object == state->box || object == state->resolution || object == state->quality ||
            object == state->qualityLabel || object == state->buttons)) || (state->joined && (object == state->label ||
            object == state->combo || object == state->list || object == state->listBox || object == state->help)));
    }

    inline bool VideoCloseList(const HostRef& host, void* combo)
    {
        if (!Current(host) || !Alive(combo)) return false;
        Watch watch(combo);
        Call close(combo, L"CloseList");
        return close.Run() && !watch.destroyed && Current(host);
    }

    inline bool VideoCopyDelegate(void* source, void* destination, void* owner, const wchar_t* property, const wchar_t* function)
    {
        auto from = Property(source, property, L"DelegateProperty", sizeof(VideoDelegate));
        auto to = Property(destination, property, L"DelegateProperty", sizeof(VideoDelegate));
        auto target = Field(Read<void*>(owner, 36), function, L"Function");
        if (!from || !to || !target) return false;
        const auto value = Read<VideoDelegate>(Address(source, from), 0);
        if (value.object != owner || !value.name || value.name != Read<uint32_t>(target, 32)) return false;
        injector::WriteMemory<VideoDelegate>(static_cast<VideoDelegate*>(Address(destination, to)), value, true);
        return true;
    }

    inline bool VideoChoices(const HostRef& host, const std::shared_ptr<Video>& state)
    {
        if (!Current(host) || host->video != state || !Alive(state->combo) || !Alive(state->list)) return false;
        void* properties[2]{};
        Array old[2]{};
        if (!ResolutionArray(state->combo, L"Itemlist", properties[0], old[0]) ||
            !ResolutionArray(state->list, L"ListButtons", properties[1], old[1])) return false;
        const auto wanted = FixSettings().Wanted("MAIN.WindowMode");
        const int index = wanted == "0" ? 0 : wanted == "1" ? 1 : wanted == "2" ? 2 : -1;
        if (index < 0 || !Current(host)) return false;
        struct Items
        {
            ResolutionItem values[3]{};
            Items() { for (auto& value : values) stringInit(&value.text, nullptr); }
            ~Items() { for (auto& value : values) stringDestroy(&value.text, nullptr); }
        } input;
        const auto language = Language();
        for (size_t i = 0; i < 3; ++i)
        {
            auto label = Wide(RWGTAMenu::PluginText(FixSettings(), language, "WindowMode." + std::to_string(i)));
            if (label.empty() || !Current(host)) return false;
            stringAssign(&input.values[i].text, nullptr, label.c_str());
        }
        OwnedKeyArray next[2]{ { properties[0] }, { properties[1] } };
        Array source{ input.values, 3, 3 };
        void* owners[]{ state->combo, state->list };
        for (size_t i = 0; i < 2; ++i)
        {
            copyArray(properties[i], nullptr, &next[i].value, &source, owners[i]);
            if (next[i].value.count != 3 || next[i].value.capacity < 3 || next[i].value.capacity > 65536 || !next[i].value.data) return false;
            for (int j = 0; j < 3; ++j)
            {
                const auto& copied = static_cast<ResolutionItem*>(next[i].value.data)[j];
                std::wstring actual, expected;
                if (!ResolutionString(copied.text, actual) || !ResolutionString(input.values[j].text, expected) ||
                    copied.enabled != 1 || actual != expected) return false;
            }
        }
        Call dimensions(state->combo, L"InitDims"), select(state->combo, L"SetSelectedIndex");
        if (!dimensions.function || !select.Put(L"iIndex", L"IntProperty", index) || !Current(host)) return false;
        Watch comboWatch(state->combo), listWatch(state->list);
        for (size_t i = 0; i < 2; ++i) next[i].Publish(owners[i]);
        bool good = Set(state->list, L"iListNum", L"IntProperty", 3) && Set(state->combo, L"iListNum", L"IntProperty", 3) &&
            dimensions.Run() && !comboWatch.destroyed && !listWatch.destroyed && Current(host) && select.Run();
        good = good && !comboWatch.destroyed && !listWatch.destroyed && Current(host);
        if (!good)
        {
            if (!comboWatch.destroyed && !listWatch.destroyed)
                for (size_t i = 0; i < 2; ++i) next[i].Publish(owners[i]);
            return false;
        }
        state->language = language;
        return true;
    }

    inline bool VideoComboContract(void* cls)
    {
        for (auto name : { L"bVisible", L"bAcceptsInput", L"bTabStop", L"bBoundToParent", L"bScaleToParent", L"bNeedLocalized" })
            if (!ClassProperty(cls, name, L"BoolProperty", 4)) return false;
        for (auto name : { L"WinLeft", L"WinTop", L"WinWidth", L"WinHeight" })
            if (!ClassProperty(cls, name, L"FloatProperty", 4)) return false;
        for (auto name : { L"ComboList", L"boxListBox", L"btnComboList", L"btnComboListUp" })
            if (!ClassProperty(cls, name, L"ObjectProperty", 4)) return false;
        for (auto name : { L"iListNum", L"ItemHeight", L"BoxButtonHeight", L"bCurrentValue" })
            if (!ClassProperty(cls, name, L"IntProperty", 4)) return false;
        return ClassProperty(cls, L"Itemlist", L"ArrayProperty", sizeof(Array)) && InputContract(cls) &&
            FunctionContract(cls, L"InitDims", {}) && FunctionContract(cls, L"CloseList", {}) &&
            FunctionContract(cls, L"SetSelectedIndex", {{ L"iIndex", L"IntProperty", 4 }});
    }

    inline bool JoinVideo(const HostRef& host, const std::shared_ptr<Video>& state)
    {
        bool pause = false;
        if (!Current(host) || host->video != state || state->joined || state->attempted || !destroyArray || !PauseContext(*host, pause)) return false;
        host->pause = pause;
        Watch ownerWatch(host->parent);
        state->box = Object(host->parent, L"boxVideo");
        state->buttons = Object(host->parent, L"boxButtons");
        state->resolution = Object(host->parent, L"m_ComboResolution");
        state->quality = Object(host->parent, L"m_ComboGraphicsQuality");
        state->qualityLabel = Object(host->parent, L"m_btnGraphicsQuality");
        auto resolutionLabel = Object(host->parent, L"m_btnResolution");
        if (!Alive(state->box) || !Alive(state->buttons) || !Type(state->resolution, L"GUIComboBoxSC2_PC") ||
            !Type(state->quality, L"GUIComboBoxSC2_PC") || !Alive(state->qualityLabel) || !Alive(resolutionLabel)) return false;
        const auto body = Bounds(state->box), footer = Bounds(state->buttons);
        const auto quality = Bounds(state->quality), label = Bounds(state->qualityLabel);
        const auto resolution = Bounds(state->resolution), resolutionText = Bounds(resolutionLabel);
        const float gap = label.y - resolutionText.y;
        auto inside = [](Rect a, Rect b)
        {
            return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.w) && std::isfinite(a.h) &&
                a.w >= 16 && a.h >= 12 && a.x >= b.x && a.y >= b.y && a.x + a.w <= b.x + b.w && a.y + a.h <= b.y + b.h;
        };
        if (!std::isfinite(gap) || gap < 18 || gap > 40 || std::abs(quality.y - resolution.y - gap) > 1 ||
            !inside(quality, body) || !inside(label, body) || quality.w < 100 || label.w < 100) return false;
        const Rect movedQuality{ quality.x, quality.y + gap, quality.w, quality.h };
        const Rect movedLabel{ label.x, label.y + gap, label.w, label.h };
        if (!inside(movedQuality, body) || !inside(movedLabel, body) ||
            std::max(movedQuality.y + movedQuality.h, movedLabel.y + movedLabel.h) + 4 > footer.y) return false;
        Rect hint{};
        if (!NativeHint(host, body, hint)) return false;
        std::vector<void*> children, pageChildren;
        if (!ArrayValues(state->box, children) || !ArrayValues(host->parent, pageChildren)) return false;
        auto overlaps = [](Rect a, Rect b) { return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h; };
        for (auto child : children)
        {
            if (!child || child == state->quality || child == state->qualityLabel) continue;
            const auto rect = Bounds(child);
            if (Bool(child, L"bVisible") && (overlaps(rect, label) || overlaps(rect, quality) ||
                overlaps(rect, movedQuality) || overlaps(rect, movedLabel))) return false;
        }
        for (auto child : pageChildren)
        {
            if (!child || child == host->panel || child == state->box || child == state->buttons ||
                child == Object(host->parent, L"boxSound") || child == Object(host->parent, L"ControlsListBox")) continue;
            if (Bool(child, L"bVisible") && overlaps(Bounds(child), hint)) return false;
        }
        auto labelClass = findObject(nullptr, nullptr, L"GUI.GUIButtonSC2", 0);
        auto comboClass = findObject(nullptr, nullptr, L"GUI.GUIComboBoxSC2_PC", 0);
        if (!WidgetContract(labelClass) || !VideoComboContract(comboClass) ||
            !FunctionContract(Read<void*>(host->parent, 36), L"OnChangeNotifyHandle", {{ L"Sender", L"ObjectProperty", 4 }, { L"iIndex", L"IntProperty", 4 }}) ||
            !FunctionContract(Read<void*>(host->parent, 36), L"OnOpenCloseHandle", {{ L"Sender", L"ObjectProperty", 4 }, { L"bIsVisible", L"BoolProperty", 4 }})) return false;
        Placement savedLabel, savedQuality;
        if (!SavePlacement(state->qualityLabel, savedLabel) || !SavePlacement(state->quality, savedQuality)) return false;
        Watch boxWatch(state->box), qualityLabelWatch(state->qualityLabel), qualityWatch(state->quality);
        state->model.OpenPlugins(hostModule, Language(), 4096, VideoSettings);
        if (!Current(host) || host->video != state || state->model.JoinedRows().size() != 1) return false;
        state->rows = state->model.JoinedRows();
        state->attempted = true;
        state->label = constructObject(labelClass, host->parent, 0, 0, nullptr, nullptr, 0);
        if (!state->label || !Current(host)) return false;
        Watch labelWatch(state->label);
        state->combo = constructObject(comboClass, host->parent, 0, 0, nullptr, nullptr, 0);
        if (!state->combo || labelWatch.destroyed || !Current(host)) return false;
        Watch comboWatch(state->combo);
        state->help = constructObject(labelClass, host->parent, 0, 0, nullptr, nullptr, 0);
        if (!state->help || labelWatch.destroyed || comboWatch.destroyed || !Current(host)) return false;
        Watch helpWatch(state->help);
        struct Cleanup
        {
            std::shared_ptr<Video> state;
            Placement label, quality;
            Watch& labelWatch; Watch& comboWatch; Watch& helpWatch;
            Watch& qualityLabelWatch; Watch& qualityWatch;
            bool committed = false, movedLabel = false, movedQuality = false;
            ~Cleanup()
            {
                if (committed) return;
                if (!labelWatch.destroyed) Visible(state->label, false);
                if (!comboWatch.destroyed) Visible(state->combo, false);
                if (!helpWatch.destroyed) Visible(state->help, false);
                if (movedLabel && !qualityLabelWatch.destroyed) RestorePlacement(label);
                if (movedQuality && !qualityWatch.destroyed) RestorePlacement(quality);
            }
        } cleanup{ state, savedLabel, savedQuality, labelWatch, comboWatch, helpWatch, qualityLabelWatch, qualityWatch };
        auto current = [&]() { return Current(host) && host->video == state && !ownerWatch.destroyed && !boxWatch.destroyed &&
            !qualityLabelWatch.destroyed && !qualityWatch.destroyed && !labelWatch.destroyed && !comboWatch.destroyed && !helpWatch.destroyed; };
        if (!InitComponent(state->label, host->controller, state->box) || !current() ||
            !InitComponent(state->combo, host->controller, state->box) || !current() ||
            !InitComponent(state->help, host->controller, host->parent) || !current()) return false;
        state->list = Object(state->combo, L"ComboList");
        state->listBox = Object(state->combo, L"boxListBox");
        if (!Alive(state->list) || !Alive(state->listBox) || !FunctionContract(Read<void*>(state->combo, 36), L"CloseList", {})) return false;
        Watch listWatch(state->list), listBoxWatch(state->listBox);
        auto componentsCurrent = [&]() { return current() && !listWatch.destroyed && !listBoxWatch.destroyed; };
        uint8_t alignment = 0;
        const auto font = ReadString(state->qualityLabel, L"FontName");
        auto sourceFont = Property(state->qualityLabel, L"FontName", L"StrProperty", sizeof(String));
        auto labelFont = Property(state->label, L"FontName", L"StrProperty", sizeof(String));
        if (font.empty() || !sourceFont || !labelFont || !Get(state->qualityLabel, L"TextAlign", L"ByteProperty", alignment) ||
            !Set(state->label, L"TextAlign", L"ByteProperty", alignment)) return false;
        copyString(labelFont, nullptr, Address(state->label, labelFont), Address(state->qualityLabel, sourceFont), state->label);
        if (ReadString(state->label, L"FontName") != font) return false;
        if (!SetBool(state->label, L"bNeedLocalized", false) || !SetBool(state->help, L"bNeedLocalized", false) ||
            !SetBool(state->label, L"bAcceptsInput", false) || !SetBool(state->label, L"bTabStop", false) ||
            !SetBool(state->help, L"bAcceptsInput", false) || !SetBool(state->help, L"bTabStop", false) ||
            !SetBool(state->combo, L"bTabStop", true) || !SetBool(state->combo, L"bNeedLocalized", false) ||
            !SetBool(state->list, L"bNeedLocalized", false) || !SetBool(Object(state->combo, L"btnComboList"), L"bNeedLocalized", false) ||
            !VideoCopyDelegate(state->resolution, state->combo, host->parent, L"__OnChangeNotify__Delegate", L"OnChangeNotifyHandle") ||
            !VideoCopyDelegate(state->resolution, state->combo, host->parent, L"__OnOpenClose__Delegate", L"OnOpenCloseHandle") ||
            !VideoCopyDelegate(state->resolution, state->combo, host->parent, L"__OnActivate__Delegate", L"OnActivateHandle")) return false;
        if (!Position(state->label, label) || !Position(state->combo, quality) || !Position(state->help, hint) ||
            !VideoChoices(host, state) || !componentsCurrent()) return false;
        std::vector<void*> currentChildren, currentPage;
        if (!ArrayValues(state->box, currentChildren) || currentChildren != children ||
            !ArrayValues(host->parent, currentPage) || currentPage != pageChildren) return false;
        auto expanded = children;
        auto at = std::find(expanded.begin(), expanded.end(), state->qualityLabel);
        if (at == expanded.end()) return false;
        expanded.insert(at, state->label);
        at = std::find(expanded.begin(), expanded.end(), state->quality);
        if (at == expanded.end()) return false;
        expanded.insert(at, state->combo);
        auto expandedPage = pageChildren;
        expandedPage.push_back(state->help);
        void* owners[]{ state->box, host->parent };
        const std::vector<void*>* sourceValues[]{ &expanded, &expandedPage };
        void* properties[2]{};
        for (size_t i = 0; i < 2; ++i)
        {
            properties[i] = Property(owners[i], L"Controls", L"ArrayProperty", sizeof(Array));
            auto inner = properties[i] ? Read<void*>(properties[i], 84) : nullptr;
            if (!inner || !Type(inner, L"ObjectProperty") || Read<uint16_t>(inner, 58) != sizeof(void*)) return false;
        }
        OwnedKeyArray next[2]{ { properties[0] }, { properties[1] } };
        for (size_t i = 0; i < 2; ++i)
        {
            const auto& values = *sourceValues[i];
            Array source{ const_cast<void**>(values.data()), int(values.size()), int(values.size()) };
            copyArray(properties[i], nullptr, &next[i].value, &source, owners[i]);
            if (!componentsCurrent() || next[i].value.count != source.count || next[i].value.capacity < source.count ||
                next[i].value.capacity > 65536 || !next[i].value.data ||
                !std::equal(values.begin(), values.end(), static_cast<void**>(next[i].value.data))) return false;
        }
        auto samePlacement = [](const Placement& before)
        {
            Placement now;
            return SavePlacement(before.object, now) && now.left == before.left && now.top == before.top &&
                now.width == before.width && now.height == before.height && now.bound == before.bound && now.scale == before.scale;
        };
        if (!componentsCurrent() || !samePlacement(savedLabel) || !samePlacement(savedQuality)) return false;
        if (!Position(state->qualityLabel, movedLabel)) return false;
        cleanup.movedLabel = true;
        if (!Position(state->quality, movedQuality)) return false;
        cleanup.movedQuality = true;
        struct Publication
        {
            OwnedKeyArray (&arrays)[2];
            void* (&owners)[2];
            Watch& boxWatch; Watch& ownerWatch;
            bool published = false, committed = false;
            ~Publication()
            {
                if (!published || committed) return;
                if (!boxWatch.destroyed) arrays[0].Publish(owners[0]);
                if (!ownerWatch.destroyed) arrays[1].Publish(owners[1]);
            }
        } publication{ next, owners, boxWatch, ownerWatch };
        for (size_t i = 0; i < 2; ++i) next[i].Publish(owners[i]);
        publication.published = true;
        std::vector<void*> publishedBox, publishedPage;
        if (!componentsCurrent() || !ArrayValues(state->box, publishedBox) || publishedBox != expanded ||
            !ArrayValues(host->parent, publishedPage) || publishedPage != expandedPage) return false;
        host->widgets.insert(host->widgets.end(), { state->label, state->combo, state->help });
        host->placements.push_back(savedLabel); host->placements.push_back(savedQuality);
        StringValue(state->label, L"Caption", state->model.Label(state->rows[0]));
        Visible(state->label, true); Visible(state->combo, true); Visible(state->help, false);
        state->joined = true;
        cleanup.committed = true;
        publication.committed = true;
        return true;
    }

    inline bool VideoDropdown(const HostRef& host, const std::shared_ptr<Video>& state, bool visible)
    {
        if (!VideoJoined(*host) || !Current(host)) return false;
        if (!visible)
        {
            RestoreVideoFlags(state);
            return true;
        }
        if (!VideoCloseList(host, state->resolution) || !VideoCloseList(host, state->quality)) return false;
        RestoreVideoFlags(state);
        for (auto object : { Object(host->parent, L"btnSave"), Object(host->parent, L"btnResetToDefault"), state->resolution, state->quality })
        {
            if (!Alive(object) || !Property(object, L"bAcceptsInput", L"BoolProperty", 4)) return false;
            state->flags.push_back({ object, L"bAcceptsInput", Bool(object, L"bAcceptsInput") });
        }
        state->flags.push_back({ state->quality, L"bVisible", Bool(state->quality, L"bVisible") });
        bool no = false;
        for (const auto& flag : state->flags) Bool(flag.object, flag.name, &no);
        state->dropdown = true;
        return true;
    }

    inline bool VideoBefore(void* object, void* frame, void* result, const std::wstring& leaf)
    {
        auto found = hosts.find(object);
        if (found == hosts.end() || !Current(found->second) || !VideoJoined(*found->second)) return false;
        const auto host = found->second;
        const auto state = host->video;
        auto function = Read<void*>(frame, 4);
        if (function != Field(Read<void*>(object, 36), leaf.c_str(), L"Function")) return false;
        if (leaf == L"OnActivateHandle")
        {
            void* sender = nullptr;
            if (!Locals(frame, L"Sender", L"ObjectProperty", 4, &sender)) return false;
            uint8_t inactive = 0, selected = 5;
            Set(state->label, L"MenuState", L"ByteProperty", inactive);
            if (sender == state->combo)
            {
                Call activate(object, L"OnActivateHandle");
                const bool previous = busy; busy = true;
                const bool good = activate.Put(L"Sender", L"ObjectProperty", state->quality) && activate.Run();
                busy = previous;
                if (good && Current(host) && VideoJoined(*host))
                {
                    Set(state->qualityLabel, L"MenuState", L"ByteProperty", inactive);
                    Set(state->label, L"MenuState", L"ByteProperty", selected);
                }
                else Retire(host, nullptr);
                return true;
            }
        }
        if (leaf == L"OnChangeNotifyHandle")
        {
            void* sender = nullptr; int index = -1;
            if (Locals(frame, L"Sender", L"ObjectProperty", 4, &sender) && sender == state->combo &&
                Locals(frame, L"iIndex", L"IntProperty", 4, &index))
            {
                if (index >= 0 && index < 3) state->pending = index;
                return true;
            }
        }
        if (leaf == L"OnOpenCloseHandle")
        {
            void* sender = nullptr; uint32_t visible = 0;
            if (!Locals(frame, L"Sender", L"ObjectProperty", 4, &sender) || !Locals(frame, L"bIsVisible", L"BoolProperty", 4, &visible)) return false;
            if (sender == state->combo)
            {
                const bool previous = busy; busy = true;
                const bool good = VideoDropdown(host, state, visible != 0);
                busy = previous;
                if (!good) Retire(host, nullptr);
                return true;
            }
            if (visible && (sender == state->resolution || sender == state->quality))
            {
                const bool previous = busy; busy = true;
                VideoCloseList(host, state->combo); RestoreVideoFlags(state);
                busy = previous;
                if (!Current(host)) return true;
                state->nativeVisible = Bool(state->combo, L"bVisible"); state->nativeHidden = true;
                Visible(state->combo, false);
            }
        }
        if (leaf == L"OnKeyCancel" && Bool(state->listBox, L"bVisible") && Result(frame, result))
        {
            const bool previous = busy; busy = true;
            const bool closed = VideoCloseList(host, state->combo);
            RestoreVideoFlags(state);
            busy = previous;
            auto button = closed ? Object(state->combo, L"btnComboList") : nullptr;
            Call focus(button, L"SetFocus");
            void* none = nullptr;
            const bool focused = closed && Current(host) && Alive(button) && focus.Put(L"Who", L"ObjectProperty", none) && focus.Run();
            if (!focused || !Current(host)) Retire(host, nullptr);
            return true;
        }
        if (leaf == L"OnClickHandle")
        {
            const bool previous = busy; busy = true;
            VideoCloseList(host, state->combo); RestoreVideoFlags(state);
            busy = previous;
        }
        return false;
    }

    inline void VideoAfter(void* object, const std::wstring& leaf)
    {
        auto found = hosts.find(object);
        if (found == hosts.end() || found->second->retired || found->second->building) return;
        const auto host = found->second;
        if (!host->video) host->video = std::make_shared<Video>();
        const auto state = host->video;
        if (!Current(host))
        {
            if (state->joined)
            {
                if (Alive(state->listBox)) Visible(state->listBox, false);
                if (Alive(state->help)) Visible(state->help, false);
                RestoreVideoFlags(state);
            }
            state->pending.reset();
            return;
        }
        if (leaf != L"InitComponent" && leaf != L"OnOpen" && leaf != L"OnPageNativeTick" && leaf != L"ResetVideoToDefault") return;
        const bool previous = busy; busy = true;
        if (!state->joined)
        {
            if (!JoinVideo(host, state)) LogOnce("video", object, "native window-mode join unavailable; Plugins setting retained");
        }
        if (!Current(host) || !VideoJoined(*host)) { busy = previous; return; }
        if (leaf == L"ResetVideoToDefault")
            state->restart |= RWGTAMenu::RestoreDefaults(FixSettings(), VideoSettings);
        if (state->pending && Current(host))
        {
            const int index = *std::exchange(state->pending, std::nullopt);
            const auto wanted = FixSettings().Wanted("MAIN.WindowMode");
            const int old = wanted == "0" ? 0 : wanted == "1" ? 1 : wanted == "2" ? 2 : -1;
            if (old >= 0 && old != index)
                state->model.Change(state->rows[0], index == (old + 1) % 3 ? 1 : -1, host->pause);
            state->restart |= state->model.RestartRequired();
        }
        if (!Current(host) || !VideoJoined(*host)) { busy = previous; return; }
        if (GetTickCount64() - state->refreshed >= 100)
        {
            state->refreshed = GetTickCount64();
            if (state->language != Language() && !Bool(state->listBox, L"bVisible"))
            {
                state->model.OpenPlugins(hostModule, Language(), 4096, VideoSettings);
                if (!Current(host) || state->model.JoinedRows().size() != 1 || !VideoChoices(host, state))
                { Retire(host, nullptr); busy = previous; return; }
                state->rows = state->model.JoinedRows();
            }
            const auto wanted = FixSettings().Wanted("MAIN.WindowMode");
            const int index = wanted == "0" ? 0 : wanted == "1" ? 1 : wanted == "2" ? 2 : -1;
            int selected = -1;
            if (index >= 0 && Get(state->combo, L"bCurrentValue", L"IntProperty", selected) && selected != index)
            {
                Call select(state->combo, L"SetSelectedIndex");
                if (select.Put(L"iIndex", L"IntProperty", index)) select.Run();
            }
            if (!Current(host) || !VideoJoined(*host)) { busy = previous; return; }
            const bool nativeOpen = Bool(Object(state->resolution, L"boxListBox"), L"bVisible") || Bool(Object(state->quality, L"boxListBox"), L"bVisible");
            if (state->nativeHidden && !nativeOpen)
            { Visible(state->combo, state->nativeVisible); state->nativeHidden = false; }
            StringValue(state->label, L"Caption", state->model.Label(state->rows[0]));
            const auto help = state->restart || state->model.RestartRequired() ? state->model.Text("menu.restartwarning") : state->model.Help(state->rows[0]);
            StringValue(state->help, L"Caption", help);
            Visible(state->help, !host->open && Bool(state->box, L"bVisible") && (Bool(state->combo, L"bHasFocus") || state->restart || state->model.RestartRequired()) && !help.empty());
        }
        busy = previous;
    }
}
