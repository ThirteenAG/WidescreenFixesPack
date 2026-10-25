#pragma once
#include "FixSettings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

// PT's GUI objects, strings, arrays, input and clipping stay engine-owned.
// The retail/digital Core contracts differ from SC1: FString is 16 bytes,
// Children/PropertiesSize are +60/+64, UFunction parameters are +100/+102.
namespace PTMenu
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
        ProtectedGameRef<T>{static_cast<T*>(Address(object, prop))} = value; return true;
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
        if (value) ProtectedGameRef<uint32_t>{data} = (*data & ~mask) | (*value ? mask : 0);
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
        Call(void* target, const wchar_t* name) : object(target)
        {
            function = target ? Field(Read<void*>(target, 36), name, L"Function") : nullptr;
            if (function && !Read<uint16_t>(function, 96) && Read<uint16_t>(function, 100) <= 1024)
                params.resize(Read<uint16_t>(function, 100));
            else function = nullptr;
        }
        template<class T> bool Put(const wchar_t* name, const wchar_t* kind, T value)
        {
            auto prop = function ? Field(function, name, kind) : nullptr;
            if (!prop || Read<uint16_t>(prop, 56) != 1 || Read<uint16_t>(prop, 58) != sizeof(T) ||
                !(Read<uint32_t>(prop, 60) & 0x80) || size_t(Read<uint16_t>(prop, 68)) + sizeof(T) > params.size()) return false;
            std::memcpy(Address(params.data(), prop), &value, sizeof(T)); return true;
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
        expanded = { std::clamp(strip.x - width * .5f, 0.0f, 640 - width * 4), strip.y, width * 4, strip.h };
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
        if (!tick || !button || !tab || !measure) { LogOnce("init", nullptr, "native drawing/font exports unavailable"); return; }
        textSize = reinterpret_cast<decltype(textSize)>(measure);
        tickHook = safetyhook::create_inline(tick, Tick);
        buttonDrawHook = safetyhook::create_inline(button, DrawButton);
        tabDrawHook = safetyhook::create_inline(tab, DrawTab);
        InitControlsDrawing();
        if (!tickHook || !buttonDrawHook || !tabDrawHook) { tickHook.reset(); buttonDrawHook.reset(); tabDrawHook.reset(); textSize = nullptr; LogOnce("init", nullptr, "native drawing hook transaction failed"); }
    }
    inline bool ResolveBounds()
    {
        auto gui = GetModuleHandleW(L"GUI.dll");
        if (!gui) return false;
        actualLeft = reinterpret_cast<decltype(actualLeft)>(GetProcAddress(gui, "?ActualLeft@UGUIComponent@@UAEMXZ"));
        actualTop = reinterpret_cast<decltype(actualTop)>(GetProcAddress(gui, "?ActualTop@UGUIComponent@@UAEMXZ"));
        actualWidth = reinterpret_cast<decltype(actualWidth)>(GetProcAddress(gui, "?ActualWidth@UGUIComponent@@UAEMXZ"));
        actualHeight = reinterpret_cast<decltype(actualHeight)>(GetProcAddress(gui, "?ActualHeight@UGUIComponent@@UAEMXZ"));
        return actualLeft && actualTop && actualWidth && actualHeight && tickHook && buttonDrawHook && tabDrawHook;
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
    #include "Details.hpp"
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
        if (!ResolveBounds()) { LogOnce("prepare", parent, "GUI bounds exports unavailable"); return false; }
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
        // The shipped strip is three 160px tabs in 480px. Expand it around its
        // center to four tabs on the same 640px canvas, preserving font space.
        if (!PlanTabs(Bounds(tabBox), originalTabs, expandedStrip, placedTabs, entryRect))
        { LogOnce("prepare", parent, "native tab strip cannot fit a fourth full-width caption"); return false; }
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
        ProtectedGameRef<uint32_t>{static_cast<uint32_t*>(result)} = 1; return true;
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
        if (!core || internalHook) return;
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
        { LogOnce("init", nullptr, "required Core export unavailable"); return; }
        auto body = [](void* value)
        {
            auto p = static_cast<uint8_t*>(value);
            for (int i = 0; p && i < 4 && p[0] == 0xE9; ++i) p += 5 + *reinterpret_cast<int32_t*>(p + 1);
            return p;
        };
        auto matches = [core, &body](void* target, const char* retail, const char* digital, bool unique = true)
        {
            if (!unique)
            {
                // Retail has two equivalent zero-init FString constructors.
                // Validate the actual exported body's bytes, not an unrelated
                // constructor's address or module-wide uniqueness.
                auto start = uintptr_t(body(target));
                auto first = hook::range_pattern(start, start + 64, retail), second = hook::range_pattern(start, start + 64, digital);
                return (first.size() == 1 && first.get_first() == body(target)) || (second.size() == 1 && second.get_first() == body(target));
            }
            auto first = hook::module_pattern(core, retail), second = hook::module_pattern(core, digital);
            return (first.size() == 1 && first.get_first() == body(target)) || (second.size() == 1 && second.get_first() == body(target));
        };
        if (!matches(internal, "81 EC 08 04 00 00 53 55 56 8B B4 24 18 04 00 00 8B 46 04 57 8B 78 5C 8B E9 8B 4E 10 8B 55 00 6A 00",
            "55 8B EC 81 EC 08 04 00 00 A1 ? ? ? ? 33 C5 89 45 FC 8B 45 0C 53 56 8B 75 08 8B D9 57 6A 00") ||
            !matches(boolCopy, "8B 41 54 8B 4C 24 04 56 8B 31 8B D0 F7 D2 23 D6", "55 8B EC 8B 51 54 8B 45 0C 8B 4D 08 56 8B F2 8B 00 F7 D6") ||
            !matches(processEvent, "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 28 A1 ? ? ? ? 85 C0 53 56 57 8B F9",
                "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 1C A1 ? ? ? ? 33 C5 89 45 F0 53 56 57 50 8D 45 F4") ||
            !matches(stringInit, "8B C1 33 C9 89 08 89 48 04 89 48 08 88 48 0C C3",
                "55 8B EC 51 89 4D FC 8B C1 C7 01 00 00 00 00 C7 41 04 00 00 00 00 C7 41 08 00 00 00 00 C6 41 0C 00", false))
        { LogOnce("init", nullptr, "Core body/signature layout mismatch"); return; }
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&hostModule), &hostModule);
        destroyHook = safetyhook::create_inline(destroy, Destroy);
        if (!destroyHook) { LogOnce("init", nullptr, "ConditionalDestroy hook failed"); return; }
        internalHook = safetyhook::create_inline(internal, Internal);
        if (!internalHook) LogOnce("init", nullptr, "ProcessInternal hook failed");
    }
}

#include "Controls.hpp"
#include "Resolutions.hpp"
#include "Video.hpp"
