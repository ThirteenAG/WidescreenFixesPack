module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <Windows.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <cwchar>
#include <map>
#include <mutex>
#include <optional>
#include <array>
#include <cmath>
#include <cstring>
#include <sstream>
#include <vector>
#include "Shared/RWGTA/FixMenu.hpp"
#include <deque>
#include <memory>
#include <safetyhook.hpp>
#include <set>

export module Frontend;

import Window;

// CryScriptSystem has separate native layouts for 32-bit and 64-bit builds.
export namespace FarCryMenu
{
    using ScriptFunction = int (__cdecl*)(void*);
#ifdef _WIN64
    constexpr size_t integerParamSlot = 11, booleanReturnSlot = 21, stringParamSlot = 9;
#else
    constexpr size_t integerParamSlot = 10, booleanReturnSlot = 20, stringParamSlot = 8;
#endif
    template<typename R, typename... A>
    inline R Virtual(void* object, size_t slot, A... arguments)
    {
#ifdef _WIN64
        using Function = R (__fastcall*)(void*, A...);
#else
        using Function = R (__thiscall*)(void*, A...);
#endif
        return reinterpret_cast<Function>((*static_cast<uintptr_t**>(object))[slot])(object, arguments...);
    }
    // Class tables of the native objects, found by pattern at Init
    inline uintptr_t scriptTable, handlerTable, objectTable;
    inline uintptr_t uiTable, buttonTable, staticTable, systemTable;
    inline uintptr_t fontServiceTable, fontTable;
    inline bool Table(void* object, uintptr_t table)
    {
        return table && object && *static_cast<uintptr_t*>(object) == table;
    }
    // Address an instruction operand refers to: absolute on x86, RIP-relative on x64
    inline uintptr_t ModuleOperand(HMODULE module, std::string_view signature, ptrdiff_t offset)
    {
        auto operand = hook::module_pattern(module, signature).get_first<uint8_t>(offset);
#ifdef _WIN64
        return reinterpret_cast<uintptr_t>(operand) + 4 + *reinterpret_cast<int32_t*>(operand);
#else
        return *reinterpret_cast<uintptr_t*>(operand);
#endif
    }
    inline void* Handler(void* script) { return Virtual<void*>(script, 0); }
    inline void* State(void* script) { return Virtual<void*>(script, 1); }
    inline bool Execute(void* script, const std::string& buffer)
    {
        return Virtual<bool>(script, 3, buffer.c_str(), buffer.size());
    }
    inline void* CreateGlobal(void* script, const char* name) { return Virtual<void*>(script, 12, name); }
    inline bool NamespaceFree(void* script, HMODULE module)
    {
        // Installed slot9 creates a temporary wrapper around the globals;
        // object slot40 returns svtNull (0) for an absent/nil member. Neither
        // string-only probes nor ExecuteBuffer success can detect collisions.
        auto globals = Virtual<void*>(script, 9);
        if (!Table(globals, objectTable)) return false;
        bool free = true;
        for (const auto* name : {"WFP_Bridge", "WFP_Menu", "WFP_FindScreen", "WFP_FindWidget",
            "WFP_Identity", "WFP_OwnRow", "WFP_Command", "WFP_Key", "WFP_Focus", "WFP_Hover",
            "WFP_Released", "WFP_Entry", "WFP_Rect", "WFP_RectMatches", "WFP_Constructing",
            "WFP_Prepare", "WFP_Snapshot", "WFP_Commit", "WFP_Enter", "WFP_Select", "WFP_Describe",
            "WFP_Leave", "WFP_Retire", "g_WFPMenuStatus", "g_WFPMenuSelected", "g_WFPMenuPause", "g_WFPMenuResult"})
        {
            if (Virtual<int>(globals, 40, name) != 0) { free = false; break; }
        }
        Virtual<void>(globals, 78);
        return free;
    }

    inline bool AddFunction(void* object, const char* name, ScriptFunction function, int id)
    {
        // Non-overloaded slot74 retains the cdecl thunk in a native Lua closure.
        return Virtual<bool>(object, 74, name, function, id);
    }
    inline void ReleaseObject(void* object) { Virtual<void>(object, 78); }
    inline void Attach(void* handler, void* state) { Virtual<void>(handler, 0, state); }
    inline int ParamCount(void* handler) { return Virtual<int>(handler, 3); }
    inline bool Integer(void* handler, int index, int& value)
    {
        // MSVC orders these overload groups differently from lexical header
        // order. Installed32 slot10 and64 slot11 are proved integer getters.
        return Virtual<bool>(handler, integerParamSlot, index, &value);
    }
    inline bool String(void* handler, int index, std::string& value, size_t limit = 64)
    {
        const char* text = nullptr;
        if (!Virtual<bool>(handler, stringParamSlot, index, &text) || !text) return false;
        value.clear();
        for (size_t offset = 0; offset <= limit; ++offset)
        {
            if (!text[offset]) return true;
            if (offset == limit) return false;
            value += text[offset];
        }
        return false;
    }
    inline void* NativeParameter(void* script, void* handler, int index, HMODULE module)
    {
        // Both installed variants have CreateEmptyObject at10, object param
        // at6 and GetNativeData at69. Release returns only this temporary
        // wrapper to the pool; its parent sink is null and borrowed controls
        // retain their native lifetime. All use sites revalidate membership.
        auto object = Virtual<void*>(script, 10);
        if (!Table(object, objectTable)) return nullptr;
        void* native = nullptr;
        if (Virtual<bool>(handler, 6, index, object)) native = Virtual<void*>(object, 69);
        ReleaseObject(object);
        return native;
    }
    inline int Return(void* handler, bool accepted) { return Virtual<int>(handler, booleanReturnSlot, accepted); }
    inline std::string NativeLanguage(void* script)
    {
        const char* value = nullptr;
        // Actual32/64 slot35 obtains the rooted global string and pops its
        // temporary Lua stack entry. Copy it before any foreign/API operation.
        if (!Virtual<bool>(script, 35, "g_language", &value) || !value) return "en";
        std::string language;
        for (size_t index = 0; index < 32; ++index)
        {
            const auto c = value[index];
            if (!c) break;
            language += c >= 'A' && c <= 'Z' ? static_cast<char>(c + 'a' - 'A') : c;
        }
        for (const auto& pair : { std::pair{"english", "en"}, {"german", "de"}, {"french", "fr"},
            {"italian", "it"}, {"spanish", "es"}, {"russian", "ru"}, {"polish", "pl"}, {"japanese", "ja"} })
            if (language == pair.first) return pair.second;
        return "en";
    }
}

export namespace FarCryText
{
    inline std::mutex Mutex;
    inline std::map<std::string, std::string> Tokens;
    inline std::map<std::wstring, std::wstring> Strings;

    inline std::optional<std::string> Literal(const std::string& text)
    {
        if (text.size() > 4096)
            return {};
        bool encoded = false;
        std::string literal;
        for (size_t i = 0; i < text.size(); ++i)
        {
            const auto c = static_cast<unsigned char>(text[i]);
            if (c < 32)
                return {};
            encoded |= c > 126 || c == '@' || (c == '\\' && i + 1 < text.size() && text[i + 1] == 'n');
            if (c == '$')
                literal += '$';
            literal += static_cast<char>(c);
        }
        if (!encoded)
            return literal;
        std::lock_guard lock(Mutex);
        if (const auto found = Tokens.find(text); found != Tokens.end())
            return found->second;
        if (Tokens.size() >= 16384)
            return {};
        const auto token = "__FFTXT" + std::to_string(Tokens.size()) + "__";
        Strings.emplace(asi_settings::text::Widen(token), asi_settings::text::Widen(literal));
        Tokens.emplace(text, token);
        return token;
    }

    inline const std::wstring* Resolve(const wchar_t* text)
    {
        if (!text || std::wcsncmp(text, L"__FFTXT", 7))
            return nullptr;
        std::lock_guard lock(Mutex);
        const auto found = Strings.find(text);
        if (found != Strings.end())
            return &found->second;
        return nullptr;
    }

    inline std::wstring Wide(const std::string& literal)
    {
        auto wide = asi_settings::text::Widen(literal);
        if (auto translated = Resolve(wide.c_str()))
            return *translated;
        return wide;
    }
}

export namespace FarCryMenu
{
#ifdef _WIN64
    constexpr size_t uiBytes = 688, uiSystemOffset = 104, uiScriptOffset = 112, uiRendererOffset = 120;
    constexpr size_t verticalScaleOffset = 632, uiFlagsOffset = 92;
    constexpr size_t widgetsBeginOffset = 200, screensBeginOffset = 672, focusOffset = 360, modalOffset = 368;
    constexpr size_t widgetRectOffset = 136, widgetUiOffset = 304, widgetScreenOffset = 312;
    constexpr size_t systemFontOffset = 10208, uiScriptObjectOffset = 184, scriptOwnerUiOffset = 40, scriptOwnerServiceOffset = 24, screenActiveOffset = 112;
#else
    constexpr size_t uiBytes = 420, uiSystemOffset = 92, uiScriptOffset = 96, uiRendererOffset = 100;
    constexpr size_t verticalScaleOffset = 384, uiFlagsOffset = 84;
    constexpr size_t widgetsBeginOffset = 128, screensBeginOffset = 412, focusOffset = 220, modalOffset = 224;
    constexpr size_t widgetRectOffset = 84, widgetUiOffset = 180, widgetScreenOffset = 184;
    constexpr size_t systemFontOffset = 10044, uiScriptObjectOffset = 124, scriptOwnerUiOffset = 20, scriptOwnerServiceOffset = 12, screenActiveOffset = 44;
#endif
    template<typename T> inline T Field(void* object, size_t offset)
    {
        T result{};
        if (object)
            std::memcpy(&result, static_cast<unsigned char*>(object) + offset, sizeof(T));
        return result;
    }
    inline bool Member(void* ui, size_t vectorOffset, void* object)
    {
        if (!object || !ui) return false;
        const auto first = Field<uintptr_t>(ui, vectorOffset);
        const auto last = Field<uintptr_t>(ui, vectorOffset + sizeof(void*));
        if (last < first || (last - first) % sizeof(void*) || last - first > 4096 * sizeof(void*)) return false;
        if (last == first || !first) return false;
        for (auto current = first; current < last; current += sizeof(void*))
        {
            void* entry{};
            std::memcpy(&entry, reinterpret_cast<void*>(current), sizeof(entry));
            if (entry == object) return true;
        }
        return false;
    }
    struct NativeRect { float x{}, y{}, width{}, height{}; };
    inline bool Rect(void* widget, NativeRect expected)
    {
        if (!widget) return false;
        const auto actual = Field<NativeRect>(widget, widgetRectOffset);
        for (const auto value : {actual.x, actual.y, actual.width, actual.height})
            if (!std::isfinite(value)) return false;
        // Native button layout can round the requested rectangle. Permit one
        // design unit while retaining the intended position and extent.
        return std::abs(actual.x - expected.x) <= 1.0f && std::abs(actual.y - expected.y) <= 1.0f &&
            std::abs(actual.width - expected.width) <= 1.0f && std::abs(actual.height - expected.height) <= 1.0f &&
            actual.width > 0 && actual.height > 0;
    }
    struct NativePool
    {
        void* ui{};
        void* script{};
        void* source{};
        void* page{};
        void* entry{};
        void* heading{};
        void* description{};
        void* prototype{};
        std::array<void*, 7> rows{};
        NativeRect entryBounds{};
        bool retired{};
        bool Owns(void* sender) const
        {
            return sender && (sender == page || sender == entry ||
                std::find(rows.begin(), rows.end(), sender) != rows.end());
        }
        bool Live(HMODULE game, HMODULE scriptModule) const
        {
            if (retired || !Table(ui, uiTable) ||
                Field<void*>(ui, uiScriptOffset) != script || !Table(script, scriptTable) ||
                !Member(ui, screensBeginOffset, source) || !Member(ui, screensBeginOffset, page)) return false;
            const auto button = [&](void* object)
            {
                return Member(ui, widgetsBeginOffset, object) && Table(object, buttonTable) &&
                    Field<void*>(object, widgetUiOffset) == ui;
            };
            const auto decorative = [&](void* object)
            {
                return Member(ui, widgetsBeginOffset, object) && Table(object, staticTable) &&
                    Field<void*>(object, widgetUiOffset) == ui;
            };
            if (!button(entry) || !button(prototype) || !decorative(heading) || !decorative(description) ||
                Field<void*>(entry, widgetScreenOffset) != source || Field<void*>(prototype, widgetScreenOffset) != source ||
                Field<void*>(heading, widgetScreenOffset) != page || Field<void*>(description, widgetScreenOffset) != page ||
                !Rect(entry, entryBounds) || !Rect(heading, {220, 152, 540, 32}) ||
                !Rect(description, {220, 470, 540, 56})) return false;
            for (size_t i = 0; i < rows.size(); ++i)
                if (!button(rows[i]) || Field<void*>(rows[i], widgetScreenOffset) != page || !Rect(rows[i], {220, 200 + static_cast<float>(i) * 36, 540, 32})) return false;
            return true;
        }
        // GetFocusScreen's script wrapper dereferences this optional field.
        // Never call it: the installed variants prove the nullable native slot.
        bool Modal() const { return Field<void*>(ui, modalOffset) != nullptr; }
        bool Selected(size_t index) const
        {
            return index < rows.size() && Field<void*>(ui, focusOffset) == rows[index];
        }
    };
}

export namespace FarCryMenu
{
    struct NativeExtent { float x{}, y{}; };
#ifdef _WIN64
    constexpr size_t fontStyleOffset = 48, fontEffectOffset = 104, fontObjectBytes = 464;
    constexpr size_t fontSystemOffset = 384, textureWidthOffset = 120, textureHeightOffset = 124, passBeginOffset = 48;
#else
    constexpr size_t fontStyleOffset = 44, fontEffectOffset = 80, fontObjectBytes = 304;
    constexpr size_t fontSystemOffset = 264, textureWidthOffset = 92, textureHeightOffset = 96, passBeginOffset = 12;
#endif

    // The UI stores ASCII keys; the native wide font path resolves their text.
    inline std::optional<std::string> NativeLiteral(const std::string& text)
    {
        return FarCryText::Literal(text);
    }

    // Call only while the owning font/UI service is protected by the native
    // traversal lease. Font layouts differ between architectures.
    inline std::optional<NativeExtent> MeasureLiteral(void* service, HMODULE module,
        const std::string& text, float size)
    {
        auto literal = NativeLiteral(text);
        if (!literal || !std::isfinite(size) || size < 8.0f || size > 64.0f ||
            !Table(service, fontServiceTable)) return {};
        auto font = Virtual<void*>(service, 2, "default");
        if (!Table(font, fontTable)) return {};
        auto bytes = static_cast<unsigned char*>(font);
        struct Restore
        {
            unsigned char* object;
            std::array<unsigned char, 24> style{};
            uintptr_t effect{};
            explicit Restore(unsigned char* value) : object(value)
            {
                std::memcpy(style.data(), object + fontStyleOffset, style.size());
                std::memcpy(&effect, object + fontEffectOffset, sizeof(effect));
            }
            ~Restore()
            {
                std::memcpy(object + fontStyleOffset, style.data(), style.size());
                std::memcpy(object + fontEffectOffset, &effect, sizeof(effect));
            }
        } restore(bytes);
        Virtual<void>(font, 5, "default");
        uintptr_t effect{};
        std::memcpy(&effect, bytes + fontEffectOffset, sizeof(effect));
        if (!effect || Field<int>(font, textureWidthOffset) <= 0 || Field<int>(font, textureHeightOffset) <= 0) return {};
        // Both actual metrics traverse forty-byte passes. The vector's own
        // field offsets differ between architectures; verify all records
        // before dispatch rather than treating a non-null effect as ready.
        const auto begin = Field<uintptr_t>(reinterpret_cast<void*>(effect), passBeginOffset);
        const auto end = Field<uintptr_t>(reinterpret_cast<void*>(effect), passBeginOffset + sizeof(void*));
        if (end <= begin || (end - begin) % 40 || end - begin > 64 * 40 ||
            !begin) return {};
        for (auto current = begin; current < end; current += 40)
        {
            auto pass = reinterpret_cast<void*>(current);
            const auto x = Field<float>(pass, 16), y = Field<float>(pass, 20);
            if (!std::isfinite(x) || !std::isfinite(y) || x <= 0 || y <= 0 ||
                !std::isfinite(Field<float>(pass, 24)) || !std::isfinite(Field<float>(pass, 28))) return {};
        }
        Virtual<void>(font, 9, true);
        Virtual<void>(font, 15, false);
        Virtual<void>(font, 17, 1.0f);
        const NativeExtent base{size, size};
        Virtual<void>(font, 11, &base);
        const auto wide = FarCryText::Wide(*literal);
        NativeExtent extent{};
        // Actual wide metric: x86 thiscall(self,out,text,multiline)/RET12;
        // x64 RCX=self,RDX=out,R8=text,R9=multiline. Both return out.
        if (Virtual<NativeExtent*>(font, 23, &extent, wide.c_str(), false) != &extent ||
            !std::isfinite(extent.x) || !std::isfinite(extent.y) || extent.x < 0 || extent.y < 0 ||
            (!text.empty() && (extent.x <= 0 || extent.y <= 0))) return {};
        return extent;
    }
}

export namespace FarCryMenu
{
    inline void PopCharacter(std::string& text)
    {
        if (text.empty())
            return;
        auto begin = text.size() - 1;
        while (begin && (static_cast<unsigned char>(text[begin]) & 0xC0) == 0x80)
            --begin;
        text.resize(begin);
    }

    inline std::vector<std::string> Wrap(void* service, HMODULE module, const std::string& text, float size = 14)
    {
        std::vector<std::string> lines;
        std::string line;
        std::istringstream words(text);
        std::string word;
        while (words >> word)
        {
            auto candidate = line.empty() ? word : line + " " + word;
            const auto extent = MeasureLiteral(service, module, candidate, size);
            if (!extent)
                return {};
            if (!line.empty() && extent->x > 524)
            {
                lines.push_back(std::move(line));
                line = word;
            }
            else
                line = std::move(candidate);
            while (!line.empty())
            {
                const auto width = MeasureLiteral(service, module, line, size);
                if (!width)
                    return {};
                if (width->x <= 524)
                    break;
                auto prefix = line;
                do
                {
                    PopCharacter(prefix);
                    const auto part = MeasureLiteral(service, module, prefix, size);
                    if (!part)
                        return {};
                    if (part->x <= 524)
                        break;
                } while (!prefix.empty());
                if (prefix.empty())
                    return {};
                lines.push_back(prefix);
                line.erase(0, prefix.size());
            }
        }
        if (!line.empty())
            lines.push_back(std::move(line));
        return lines;
    }

    struct FittedText { std::string text; float size{}; NativeExtent extent{}; };
    inline std::optional<FittedText> FitRow(void* service, HMODULE fontModule,
        std::string label, const std::string& value, float width, float height, float preferred = 20, float minimum = 8)
    {
        if (!NativeLiteral(label) || !NativeLiteral(value) || !std::isfinite(width) ||
            !std::isfinite(height) || width < 64 || height < 8 || preferred < 8 || preferred > 64 || !std::isfinite(minimum) || minimum < 8 || minimum > preferred) return {};
        const auto suffix = value.empty() ? std::string() : ": " + value;
        // Native font size stays readable. Full values are never truncated or
        // converted to replacement glyphs. An unsupported row is unavailable.
        for (float size = preferred; size >= minimum; size -= 1)
        {
            auto suffixExtent = MeasureLiteral(service, fontModule, suffix, size);
            if (!suffixExtent || suffixExtent->x > width || suffixExtent->y > height) continue;
            auto extent = MeasureLiteral(service, fontModule, label + suffix, size);
            if (!extent) return {};
            if (extent->x <= width && extent->y <= height)
                return FittedText{*NativeLiteral(label + suffix), size, *extent};
            std::string shortened = label;
            while (!shortened.empty())
            {
                PopCharacter(shortened);
                const auto combined = shortened + "..." + suffix;
                auto fitted = MeasureLiteral(service, fontModule, combined, size);
                if (!fitted) return {};
                if (fitted->x <= width && fitted->y <= height)
                    return FittedText{*NativeLiteral(combined), size, *fitted};
            }
        }
        return {};
    }
}

export namespace FarCryMenu
{
    // Native Lua 4 syntax and existing shipped UI skins/helpers only. The
    // fixed screen/control pool belongs to the engine and is retired by its
    // ordinary Release/reload paths; no stock page or callback is replaced.
    inline constexpr const char* nativeMenuLua = R"WFP(
function WFP_FindScreen(name)
    local count=UI:GetScreenCount();
    if count<0 or count>256 then return nil end
    for i=0,count-1 do
        local screen=UI:GetScreen(i);
        local actual=screen and screen:GetName();
        if actual==name or (actual and strlower and strlower(actual)==strlower(name)) then return screen end
    end
    return nil;
end
function WFP_FindWidget(screen,name)
    if not screen or not screen.GetWidgetCount or not screen.GetWidget then return nil end
    -- Shipped screens access child widgets as Sender.GameOptions, etc.
    -- The installed screen:GetWidget wrapper discards its native return value.
    local direct=screen[name];
    if direct and direct.GetScreen and direct:GetScreen()==screen then return direct end
    -- Shipped UI code uses the global getter for dynamically created widgets.
    if UI.GetWidget then
        direct=UI:GetWidget(name,screen);
        if direct and direct.GetScreen and direct:GetScreen()==screen then return direct end
    end
    direct=screen:GetWidget(name);
    if direct then return direct end
    local count=screen:GetWidgetCount();
    if count<0 or count>256 then return nil end
    for i=0,count-1 do
        local widget=screen:GetWidget(i);
        if widget and widget:GetName()==name then return widget end
    end
    return nil;
end
function WFP_Identity(expected)
    local s=WFP_Menu;
    return s and (not expected or expected==s) and not s.dead and UI==s.ui and
        WFP_FindScreen("Options")==s.source and
        WFP_FindScreen("WFPAdvancedOptions")==s.private;
end
function WFP_OwnRow(sender,expected)
    local s=WFP_Menu;
    if not WFP_Identity(expected) or not sender or not sender.user or
        sender.user.epoch~=s.epoch or sender:GetScreen()~=s.private then return nil end
    local index=sender.user.index;
    return index and s.rows[index+1]==sender and index>=0 and index<s.count;
end
function WFP_Command(sender)
    local s=WFP_Menu;
    if WFP_OwnRow(sender,s) and not s.committing and UI:IsScreenActive(s.private)==1 then
        WFP_Bridge:Queue(s.serial,1,sender.user.index,0,sender);
    end
    return 1;
end
function WFP_Key(sender,key)
    local s=WFP_Menu;
    if WFP_OwnRow(sender,s) and not s.committing and UI:IsScreenActive(s.private)==1 then
        if key==39 then
            return 0;
        end
        if WFP_Bridge:Queue(s.serial,2,sender.user.index,key,sender) then return 0 end
    end
    return 1;
end
function WFP_Focus(sender)
    local s=WFP_Menu;
    if not WFP_OwnRow(sender,s) then return 1 end
    DefOnGotFocusSideButton(sender);
    if WFP_OwnRow(sender,s) and not s.committing then
        s.selected=sender.user.index;
        WFP_Bridge:Queue(s.serial,3,s.selected,0,sender);
    end
    return 1;
end
function WFP_Hover(sender)
    local s=WFP_Menu;
    if not WFP_OwnRow(sender,s) then return 1 end
    DefOnMouseEnterSideButton(sender);
    if WFP_OwnRow(sender,s) and not s.committing then
        WFP_Bridge:Queue(s.serial,4,sender.user.index,0,sender);
    end
    return 1;
end
function WFP_Released(sender)
    local s=WFP_Menu;
    local owned=s and (sender==s.private or sender==s.entry);
    if s and not owned then
        for i=1,7 do
            if sender==s.rows[i] then owned=1 end
        end
    end
    if owned and not s.dead then
        s.dead=1;
        g_WFPMenuStatus="retired";
        WFP_Bridge:Queue(s.epoch,5,-1,0,sender);
    end
    return 1;
end
function WFP_Entry(sender)
    local s=WFP_Menu;
    if WFP_Identity(s) and sender==s.entry and sender:GetScreen()==s.source and
        not s.committing and UI:IsScreenActive(s.source)==1 then
        WFP_Bridge:Queue(s.epoch,0,-1,0,sender);
    end
    return 1;
end
function WFP_Rect(widget)
    if not widget or not widget.GetRect then return nil end
    local rect=widget:GetRect();
    return {UI:ExtractLeft(rect),UI:ExtractTop(rect),UI:ExtractWidth(rect),UI:ExtractHeight(rect)};
end
function WFP_RectMatches(bounds,expected)
    if not bounds then return nil end
    -- Native button layout can round the requested rectangle.
    for i=1,4 do
        if type(bounds[i])~="number" or not (bounds[i]>=expected[i]-1 and bounds[i]<=expected[i]+1) then return nil end
    end
    return bounds[3]>0 and bounds[4]>0;
end
function WFP_Constructing(s)
    return s and WFP_Menu==s and UI==s.ui and not s.dead and
        WFP_FindScreen("Options")==s.source and
        (not s.private or WFP_FindScreen("WFPAdvancedOptions")==s.private);
end
function WFP_Prepare(epoch,title,entrySize)
    g_WFPMenuStatus="source-not-ready";
    if not UI or not UI.GetScreenCount or not UI.CreateScreenFromTable or
        not UI.CreateObjectFromTable or not UI.skins or not UI.skins.SideMenuButton or
        not UI.skins.MenuStatic or not GotoPage or not DefOnGotFocusSideButton or
        not DefOnMouseEnterSideButton or not UIFLAG_DEFAULT or not UIFLAG_CANHAVEFOCUS or
        not UISTYLE_TRANSPARENT or not UIALIGN_LEFT or not UIFLAG_VISIBLE then
        g_WFPMenuStatus="source-not-ready:ui-api-or-skin-missing";
        if UI then
            for i,key in {"GetScreenCount","CreateScreenFromTable","CreateObjectFromTable"} do
                if not UI[key] then g_WFPMenuStatus="source-not-ready:missing-"..key end
            end
        end
        return nil;
    end
    local source=WFP_FindScreen("Options");
    g_WFPMenuStatus="source-not-ready:Options-missing";
    if not source then return nil end
    local prototype=WFP_FindWidget(source,"MainMenu");
    g_WFPMenuStatus="source-not-ready:MainMenu-missing";
    if not source or not prototype or not prototype.SetText or not prototype.GetFontSize or
        WFP_FindScreen("WFPAdvancedOptions") or WFP_FindWidget(source,"WFPAdvancedEntry") then return nil end
    local rect=WFP_Rect(prototype);
    g_WFPMenuStatus="source-not-ready:MainMenu-geometry";
    if not rect or rect[1]<0 or rect[2]<0 or rect[3]<64 or rect[4]<16 or
        rect[1]+rect[3]>800 or rect[2]+2*rect[4]>580 or
        entrySize<8 or entrySize>20 then return nil end
    WFP_Menu={ui=UI,source=source,epoch=epoch,serial=epoch,rows={},count=0,selected=0,committing=1};
    local page={OnRelease=WFP_Released};
    page.WFPHeading={skin=UI.skins.MenuStatic,left=220,top=152,width=540,height=32,
        fontname="default",fonteffect="default",fontsize=24,halign=UIALIGN_LEFT,style=UISTYLE_TRANSPARENT,bordersize=0,
        flags=UIFLAG_DEFAULT-UIFLAG_CANHAVEFOCUS,text=""};
    page.WFPDescription={skin=UI.skins.MenuStatic,left=220,top=470,width=540,height=56,
        fontname="default",fonteffect="default",fontsize=14,halign=UIALIGN_LEFT,style=UISTYLE_TRANSPARENT,bordersize=0,
        flags=UIFLAG_DEFAULT-UIFLAG_CANHAVEFOCUS,text=""};
    for i=0,6 do
        page["WFPRow"..tostring(i)]={skin=UI.skins.SideMenuButton,left=220,top=200+i*36,
            width=540,height=32,fontname="default",fonteffect="default",fontsize=20,halign=UIALIGN_LEFT,tabstop=200+i,
            user={epoch=epoch,index=i},text="",OnCommand=WFP_Command,OnKeyDown=WFP_Key,
            OnGotFocus=WFP_Focus,OnMouseEnter=WFP_Hover,OnRelease=WFP_Released};
    end
    local s=WFP_Menu;
    UI:CreateScreenFromTable("WFPAdvancedOptions",page);
    s.private=WFP_FindScreen("WFPAdvancedOptions");
    g_WFPMenuStatus="private-screen-refused";
    if not WFP_Constructing(s) or not s.private then return nil end
    s.heading=WFP_FindWidget(s.private,"WFPHeading");
    if not WFP_Constructing(s) then return nil end
    s.description=WFP_FindWidget(s.private,"WFPDescription");
    if not WFP_Constructing(s) then return nil end
    g_WFPMenuStatus="private-static-controls-refused";
    if not s.heading or not s.description or not s.heading.SetText or not s.description.SetText then return nil end
    for i,item in {{s.heading,24},{s.description,14}} do
        local widget=item[1];
        if not WFP_Constructing(s) or not widget.GetFontName or not widget.GetFontEffect or not widget.GetFontSize then return nil end
        local fontName=widget:GetFontName();
        if fontName~="default" or not WFP_Constructing(s) then g_WFPMenuStatus="private-static-font:"..tostring(fontName); return nil end
        local fontEffect=widget:GetFontEffect();
        if fontEffect~="default" or not WFP_Constructing(s) then g_WFPMenuStatus="private-static-effect:"..tostring(fontEffect); return nil end
        local fontSize=widget:GetFontSize();
        if fontSize~=item[2] or not WFP_Constructing(s) then g_WFPMenuStatus="private-static-size:"..tostring(fontSize); return nil end
    end
    for i=0,6 do
        if not WFP_Constructing(s) then return nil end
        s.rows[i+1]=WFP_FindWidget(s.private,"WFPRow"..tostring(i));
        local row=s.rows[i+1];
        g_WFPMenuStatus="private-row-contract:"..tostring(i);
        if not row then g_WFPMenuStatus="private-row-missing:"..tostring(i); return nil end
        for _,key in {"SetText","SetFontSize","GetText","GetFontSize","GetFontName","GetFontEffect","GetScreen"} do
            if not row[key] then g_WFPMenuStatus="private-row-api:"..key; return nil end
        end
        if row:GetScreen()~=s.private then g_WFPMenuStatus="private-row-owner:"..tostring(i); return nil end
        if not WFP_Constructing(s) then return nil end
        local fontName=row:GetFontName();
        if fontName~="default" or not WFP_Constructing(s) then g_WFPMenuStatus="private-row-font:"..tostring(fontName); return nil end
        local fontEffect=row:GetFontEffect();
        if fontEffect~="default" or not WFP_Constructing(s) then g_WFPMenuStatus="private-row-effect:"..tostring(fontEffect); return nil end
        local fontSize=row:GetFontSize();
        if fontSize~=20 or not WFP_Constructing(s) then g_WFPMenuStatus="private-row-size:"..tostring(fontSize); return nil end
        if not WFP_Constructing(s) then return nil end
        local bounds=WFP_Rect(row);
        if not WFP_Constructing(s) then return nil end
        if not bounds then g_WFPMenuStatus="private-row-rect-missing"; return nil end
        if not WFP_RectMatches(bounds,{220,200+i*36,540,32}) then
            g_WFPMenuStatus="private-row-rect:"..tostring(bounds[1])..","..tostring(bounds[2])..","..tostring(bounds[3])..","..tostring(bounds[4]); return nil
        end
        UI:HideWidget(row);
        if not WFP_Constructing(s) then return nil end
    end
    UI:DeactivateScreen(s.private);
    if not WFP_Constructing(s) then return nil end
    g_WFPMenuStatus="entry-controls-refused";
    local createdEntry=UI:CreateObjectFromTable("WFPAdvancedEntry",{
        skin=UI.skins.SideMenuButton,left=rect[1],top=rect[2]+rect[4],width=rect[3],height=rect[4],
        fontname="default",fonteffect="default",fontsize=entrySize,text=title,
        flags=UIFLAG_DEFAULT-UIFLAG_VISIBLE,tabstop=10002,user={epoch=epoch},
        OnCommand=WFP_Entry,OnRelease=WFP_Released},source);
    s.entry=WFP_FindWidget(source,"WFPAdvancedEntry") or createdEntry;
    if not WFP_Constructing(s) then return nil end
    if not s.entry then g_WFPMenuStatus="entry-missing"; return nil end
    for _,key in {"GetFontName","GetFontEffect","GetFontSize","GetScreen","GetText"} do
        if not s.entry[key] then g_WFPMenuStatus="entry-api:"..key; return nil end
    end
    if s.entry:GetScreen()~=source or not WFP_Constructing(s) then g_WFPMenuStatus="entry-owner"; return nil end
    if WFP_FindWidget(source,"WFPAdvancedEntry")~=s.entry then g_WFPMenuStatus="entry-name"; return nil end
    local fontName=s.entry:GetFontName();
    if not WFP_Constructing(s) or fontName~="default" then g_WFPMenuStatus="entry-font:"..tostring(fontName); return nil end
    local fontEffect=s.entry:GetFontEffect();
    if not WFP_Constructing(s) or fontEffect~="default" then g_WFPMenuStatus="entry-effect:"..tostring(fontEffect); return nil end
    local fontSize=s.entry:GetFontSize();
    if not WFP_Constructing(s) or fontSize~=entrySize then g_WFPMenuStatus="entry-size:"..tostring(fontSize); return nil end
    if not WFP_Constructing(s) then return nil end
    UI:HideWidget(s.entry);
    if not WFP_Constructing(s) then return nil end
    local bounds=WFP_Rect(s.entry);
    if not WFP_Constructing(s) then return nil end
    if not WFP_RectMatches(bounds,{rect[1],rect[2]+rect[4],rect[3],rect[4]}) then g_WFPMenuStatus="entry-rect"; return nil end
    if s.entry:GetText()~=title then g_WFPMenuStatus="entry-text"; return nil end
    if not WFP_Bridge:Bind(epoch,source,s.private,s.entry,s.heading,s.description,prototype,
        s.rows[1],s.rows[2],s.rows[3],s.rows[4],s.rows[5],s.rows[6],s.rows[7]) then return nil end
    if not WFP_Constructing(s) then return nil end
    s.committing=nil;
    UI:ShowWidget(s.entry);
    if not WFP_Constructing(s) then return nil end
    g_WFPMenuStatus="ready";
    return 1;
end
function WFP_Snapshot()
    g_WFPMenuStatus="retired";
    g_WFPMenuSelected="-1";
    g_WFPMenuPause="0";
    local s=WFP_Menu;
    if not WFP_Identity(s) then return nil end
    if ClientStuff then g_WFPMenuPause="1" end
    if UI:IsScreenActive(s.private)==1 then
        g_WFPMenuStatus="private";
        local focus=UI:GetFocus();
        for i=0,s.count-1 do
            if focus==s.rows[i+1] then g_WFPMenuSelected=tostring(i) end
        end
    elseif UI:IsScreenActive(s.source)==1 then
        g_WFPMenuStatus="options";
        if UI:GetFocus()==s.entry then g_WFPMenuSelected="entry" end
    else g_WFPMenuStatus="covered-or-left" end
    return 1;
end
function WFP_Commit(serial,title,items,selected)
    g_WFPMenuStatus="commit-refused";
    if not WFP_Identity() or not items or getn(items)<1 or getn(items)>7 then return nil end
    local s=WFP_Menu;
    s.committing=1;
    s.serial=serial;
    s.count=getn(items);
    s.description:SetText("");
    if not WFP_Identity(s) then return nil end
    s.heading:SetText(title);
    if not WFP_Identity(s) then return nil end
    for i=0,6 do
        if not WFP_Identity(s) then return nil end
        local row=s.rows[i+1];
        local item=items[i+1];
        if item then
            row:SetText(item.text);
            if not WFP_Identity(s) then return nil end
            row:SetFontSize(item.size);
            if not WFP_Identity(s) then return nil end
            if row:GetText()~=item.text then return nil end
            if not WFP_Identity(s) then return nil end
            if row:GetFontSize()~=item.size then return nil end
            if not WFP_Identity(s) then return nil end
            if item.enabled==1 then UI:EnableWidget(row) else UI:DisableWidget(row) end
            if not WFP_Identity(s) then return nil end
            UI:ShowWidget(row);
        else UI:HideWidget(row) end
        if not WFP_Identity(s) then return nil end
    end
    if not WFP_Identity(s) or selected<0 or selected>=s.count then return nil end
    s.selected=selected;
    s.committing=nil;
    g_WFPMenuStatus="committed";
    return 1;
end
function WFP_Enter()
    local s=WFP_Menu;
    if not WFP_Identity(s) then return nil end
    if UI:IsScreenActive(s.source)~=1 or UI:GetFocus()~=s.entry then return nil end
    s.parentTab="GameOptions";
    for i,name in {"GameOptions","VideoOptions","SoundOptions","ControlOptions"} do
        if UI:IsScreenActive(name)==1 then s.parentTab=name end
    end
    GotoPage("WFPAdvancedOptions");
    if not WFP_Identity(s) or UI:IsScreenActive(s.private)~=1 then
        return nil;
    end
    UI:SetFocus(s.rows[s.selected+1]);
    if not WFP_Identity(s) then return nil end
    return 1;
end
function WFP_Select(index)
    local s=WFP_Menu;
    if not WFP_Identity(s) then return nil end
    if index<0 or index>=s.count or UI:IsScreenActive(s.private)~=1 then return nil end
    UI:SetFocus(s.rows[index+1]);
    if not WFP_Identity(s) then return nil end
    return 1;
end
function WFP_Describe(text,size)
    local s=WFP_Menu;
    if WFP_Identity(s) and UI:IsScreenActive(s.private)==1 then
        if not size or size<8 or size>14 then return nil end
        s.description:SetText(text);
        if not WFP_Identity(s) then return nil end
        s.description:SetFontSize(size);
        if not WFP_Identity(s) then return nil end
        local actual=s.description:GetFontSize();
        if not WFP_Identity(s) or actual~=size then return nil end
        return 1;
    end
    return nil;
end
function WFP_Retire(epoch)
    local s=WFP_Menu;
    if not s or s.epoch~=epoch or UI~=s.ui then return nil end
    s.dead=1;
    s.committing=1;
    if WFP_FindScreen("Options")~=s.source then return 1 end
    if s.entry and WFP_FindWidget(s.source,"WFPAdvancedEntry")==s.entry then UI:HideWidget(s.entry) end
    if WFP_Menu~=s or UI~=s.ui then return nil end
    if s.private and WFP_FindScreen("WFPAdvancedOptions")==s.private and UI:IsScreenActive(s.private)==1 then
        GotoPage("Options");
        if WFP_Menu~=s or UI~=s.ui or WFP_FindScreen("Options")~=s.source then return nil end
        local button=WFP_FindWidget(s.source,s.parentTab or "GameOptions");
        if button and button.OnCommand then button.OnCommand(button) end
        if WFP_Menu~=s or UI~=s.ui then return nil end
        local back=WFP_FindWidget(s.source,"MainMenu");
        if back then UI:SetFocus(back) end
    end
    return 1;
end
function WFP_Leave()
    local s=WFP_Menu;
    if not WFP_Identity(s) then return nil end
    if UI:IsScreenActive(s.private)~=1 then return nil end
    s.committing=1;
    GotoPage("Options");
    if not WFP_Identity(s) then return nil end
    local button=WFP_FindWidget(s.source,s.parentTab);
    if button and button.OnCommand then button.OnCommand(button) end
    if not WFP_Identity(s) then return nil end
    UI:SetFocus(s.entry);
    if not WFP_Identity(s) then return nil end
    s.committing=nil;
    return 1;
end
)WFP";
}

// Supported native host. Per-architecture contracts gate transactional Init;
// all native page actions are deferred until complete traversal scopes unwind.
export namespace FarCryMenu
{
    enum class ItemKind { Row, Page, Next, Previous, Back, Text, HelpNext, HelpPrevious, HelpBack, RowNext, RowPrevious };
    struct Item
    {
        ItemKind kind = ItemKind::Back;
        RWGTAMenu::Row row{};
        size_t target{};
        std::string label, value, description;
        bool enabled = true;
        FittedText fitted;
    };
    struct Context
    {
        NativePool native;
        RWGTAMenu::Model model;
        std::string epoch, serial;
        std::vector<Item> items;
        std::optional<size_t> page;
        size_t rootOffset{}, selected{};
        bool open{}, retired{}, returnPending{}, pause{};
        double layoutScaleY{};
        uint32_t lifetime{};
        bool details{};
        size_t helpPage{}, helpSelected{};
        std::string helpTitle;
        std::vector<std::string> helpLines;
        size_t rowOffset{};
    };
    struct Pending
    {
        std::string serial;
        int kind{}, index{}, key{};
        void* sender{};
    };
    inline std::shared_ptr<Context> context;
    inline std::deque<Pending> pending;
    inline HMODULE hostModule{}, gameModule{}, scriptModule{}, fontModule{}, systemModule{};
    inline unsigned nativeDepth{}, foreignDepth{};
    inline bool draining{};
    inline uint32_t generation{};
    inline void (*Tick)() = nullptr;
    inline std::map<HMODULE, std::map<std::string, std::string>> RestartValues;
    inline HMODULE RestartHost = nullptr;

    inline void InitRestartValues()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&InitRestartValues), &RestartHost);
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.apply == "restart")
                RestartValues[RestartHost].emplace(setting.id, FixSettings().Wanted(setting.id));
    }
    struct Depth
    {
        unsigned& counter;
        explicit Depth(unsigned& value) : counter(value) { ++counter; }
        ~Depth() { --counter; }
    };
    inline std::string Token()
    {
        if (generation == UINT32_MAX) return {};
        return std::to_string(++generation); // Lua receives a string, never float32.
    }
    inline std::string Quote(const std::string& text)
    {
        std::string output = "\"";
        for (const unsigned char c : text)
        {
            if (c == '\\' || c == '"') output += '\\';
            if (c < 32 || c > 126) return {}; // Only verified native literals.
            output += static_cast<char>(c);
        }
        return output + '"';
    }
    inline bool Current(const std::shared_ptr<Context>& owner)
    {
        return owner && context == owner && !owner->retired &&
            owner->native.Live(gameModule, scriptModule);
    }
    inline bool RestartRequired(const std::shared_ptr<Context>& owner)
    {
        if (!owner || !owner->model.IsOpen())
            return false;
        const auto pages = owner->model.Pages();
        for (const auto& page : pages)
        {
            if (!Current(owner))
                return false;
            const auto plugin = page.plugin;
            if (!plugin)
                continue;
            const auto module = plugin->module;
            const auto settings = plugin->file.description.settings;
            for (const auto& setting : settings)
            {
                if (!Current(owner))
                    return false;
                if (setting.apply != "restart")
                    continue;
                std::string value;
                if (module == RestartHost)
                    value = FixSettings().Wanted(setting.id);
                else
                {
                    Depth scope(foreignDepth);
                    value = OtherPlugins::Get(*plugin, setting);
                }
                if (!Current(owner))
                    return false;
                const auto [initial, inserted] = RestartValues[module].emplace(setting.id, value);
                if (!inserted && initial->second != value)
                    return true;
            }
        }
        return false;
    }
    inline bool Available(const std::shared_ptr<Context>& owner)
    {
        return Current(owner) && !owner->native.Modal() && (Field<unsigned char>(owner->native.ui, uiFlagsOffset) & 4);
    }
    inline void Retire(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner->retired) return;
        owner->retired = true;
        owner->model.Close(); // Logical cancellation only; no native mutations.
    }
    inline bool Global(void* script, const char* key, std::string& output)
    {
        const char* value = nullptr;
        if (!Virtual<bool>(script, 35, key, &value) || !value) return false;
        output.clear();
        for (size_t offset = 0; offset <= 64; ++offset)
        {
            if (!value[offset]) return true;
            if (offset == 64) return false;
            output += value[offset];
        }
        return false;
    }
    inline bool Call(const std::shared_ptr<Context>& owner, const std::string& expression)
    {
        if (!Available(owner)) return false;
        Depth scope(nativeDepth);
        if (!Execute(owner->native.script, "g_WFPMenuResult=tostring(" + expression + ");") || !Available(owner)) return false;
        std::string result;
        return Global(owner->native.script, "g_WFPMenuResult", result) && result == "1";
    }
    inline bool Snapshot(const std::shared_ptr<Context>& owner, const char* required)
    {
        if (!Available(owner)) return false;
        {
            Depth scope(nativeDepth);
            if (!Execute(owner->native.script, "WFP_Snapshot();") || !Available(owner)) return false;
        }
        std::string status, pause;
        if (!Global(owner->native.script, "g_WFPMenuStatus", status) || status != required) return false;
        if (Global(owner->native.script, "g_WFPMenuPause", pause)) owner->pause = pause == "1";
        return Available(owner);
    }
    inline void* FontService(const std::shared_ptr<Context>& owner)
    {
        if (!Available(owner)) return nullptr;
        const auto system = Field<void*>(owner->native.ui, uiSystemOffset);
        if (!Table(system, systemTable)) return nullptr;
        return Field<void*>(system, systemFontOffset);
    }
    inline bool Build(const std::shared_ptr<Context>& owner)
    {
        if (!Available(owner) || !owner->model.IsOpen()) return false;
        const auto oldSerial = owner->serial;
        const auto current = [&] { return Available(owner) && owner->serial == oldSerial && owner->model.IsOpen(); };
        std::vector<Item> items;
        if (owner->page && *owner->page >= owner->model.Pages().size()) return false;
        auto title = owner->page ? owner->model.Pages()[*owner->page].title : owner->model.Text("menu.title");
        if (!NativeLiteral(title)) title = owner->model.Text("menu.title");
        if (owner->details)
        {
            title = owner->helpTitle;
            for (size_t i = owner->helpPage * 4; i < owner->helpLines.size() && i < owner->helpPage * 4 + 4; ++i)
                items.push_back({ItemKind::Text, {}, 0, owner->helpLines[i]});
            if (owner->helpPage)
                items.push_back({ItemKind::HelpPrevious, {}, 0, owner->model.Text("menu.previous")});
            if ((owner->helpPage + 1) * 4 < owner->helpLines.size())
                items.push_back({ItemKind::HelpNext, {}, 0, owner->model.Text("menu.next")});
        }
        else if (!owner->page)
        {
            const auto pages = owner->model.Pages();
            for (size_t i = owner->rootOffset; i < pages.size() && items.size() < 4; ++i)
            {
                auto label = pages[i].title;
                if (!NativeLiteral(label)) label = owner->model.Text("menu.title") + " " + std::to_string(i + 1);
                items.push_back({ItemKind::Page, {}, i, std::move(label)});
            }
            if (owner->rootOffset) items.push_back({ItemKind::Previous, {}, owner->rootOffset >= 4 ? owner->rootOffset - 4 : 0, owner->model.Text("menu.previous")});
            if (owner->rootOffset + 4 < pages.size()) items.push_back({ItemKind::Next, {}, owner->rootOffset + 4, owner->model.Text("menu.next")});
        }
        else
        {
            std::vector<const RWGTAMenu::Row*> rows;
            { Depth scope(foreignDepth); rows = owner->model.VisibleRows(*owner->page); }
            if (!current()) return false;
            if (owner->rowOffset >= rows.size())
                owner->rowOffset = rows.empty() ? 0 : (rows.size() - 1) / 4 * 4;
            for (size_t index = owner->rowOffset; index < rows.size() && index < owner->rowOffset + 4; ++index)
            {
                if (!current() || items.size() >= 6) return false;
                const auto row = *rows[index];
                Item item{ItemKind::Row, row};
                { Depth scope(foreignDepth); item.enabled = owner->model.Enabled(row, owner->pause); }
                if (!current()) return false;
                item.label = owner->model.Label(row);
                if (!current()) return false;
                { Depth scope(foreignDepth); item.value = owner->model.Value(row); }
                if (!current()) return false;
                item.description = owner->model.Help(row);
                items.push_back(std::move(item));
            }
            if (owner->rowOffset)
                items.push_back({ItemKind::RowPrevious, {}, owner->rowOffset - 4, owner->model.Text("menu.previous")});
            if (owner->rowOffset + 4 < rows.size())
                items.push_back({ItemKind::RowNext, {}, owner->rowOffset + 4, owner->model.Text("menu.next")});
        }
        items.push_back({owner->details ? ItemKind::HelpBack : ItemKind::Back, {}, 0, owner->model.Text("menu.back")});
        const auto service = FontService(owner);
        if (!service || !NativeLiteral(title)) return false;
        const auto scaleY = Field<double>(owner->native.ui, verticalScaleOffset);
        if (!std::isfinite(scaleY) || scaleY <= 0) return false;
        const auto minimum = static_cast<float>(std::max(8.0, std::ceil(8.0 / scaleY)));
        for (auto& item : items)
        {
            if (!current()) return false;
            if (!NativeLiteral(item.label) || !NativeLiteral(item.value))
            {
                // Unsupported native glyph/escape route is explicit. Never
                // edit a row whose complete value cannot be represented.
                item.enabled = false;
                item.label = item.row.setting && NativeLiteral(item.row.setting->key) ?
                    item.row.setting->key : owner->model.Text("menu.unavailable");
                item.value = owner->model.Text("menu.unavailable");
            }
            const auto preferred = item.kind == ItemKind::Text ? 14.0f : 20.0f;
            const auto fitted = FitRow(service, fontModule, item.label, item.value, 524, 28, preferred, std::min(minimum, preferred));
            if (!fitted || (!fitted->text.empty() && fitted->extent.y * scaleY < 8)) return false; // Complete, readable values only.
            item.fitted = *fitted;
        }
        const auto heading = FitRow(service, fontModule, title, "", 524, 28, 24, 24);
        if (!heading || heading->extent.y * scaleY < 8 || !current()) return false;
        const auto serial = Token();
        if (serial.empty() || !current()) return false;
        auto selected = std::min(owner->selected, items.size() - 1);
        if (!items[selected].enabled)
            for (size_t i = 0; i < items.size(); ++i) if (items[i].enabled) { selected = i; break; }
        std::string expression = "WFP_Commit(" + Quote(serial) + "," + Quote(heading->text) + ",{ ";
        for (const auto& item : items)
            expression += "{text=" + Quote(item.fitted.text) + ",size=" + std::to_string(static_cast<int>(item.fitted.size)) +
                ",enabled=" + (item.enabled ? "1" : "0") + "},";
        expression += "}," + std::to_string(selected) + ")";
        if (!Call(owner, expression) || !current()) return false;
        owner->items = std::move(items);
        owner->serial = serial;
        owner->selected = selected;
        owner->layoutScaleY = scaleY;
        RWGTAMenu::Model::Changed() = false;
        if (owner->open && (!Call(owner, "WFP_Select(" + std::to_string(selected) + ")") ||
            !Available(owner) || owner->serial != serial || !owner->native.Selected(selected))) return false;
        return true;
    }
    inline bool Queue(Pending action)
    {
        auto owner = context;
        if (!owner) return false;
        if (action.kind == 5)
        {
            if (action.serial != owner->epoch || !owner->native.Owns(action.sender)) return false;
            owner->native.retired = true;
            Retire(owner);
            return true;
        }
        if (pending.size() >= 32 || !Available(owner)) return false;
        bool cancelForeign = false;
        if (action.kind == 0)
        {
            if (action.serial != owner->epoch || action.sender != owner->native.entry ||
                Field<void*>(owner->native.ui, focusOffset) != action.sender) return false;
        }
        else
        {
            if (!owner->open || action.serial != owner->serial || action.index < 0 ||
                static_cast<size_t>(action.index) >= owner->items.size() ||
                action.sender != owner->native.rows[action.index] || !owner->native.Selected(action.index)) return false;
            // Supported installed32 and64 GetKeyName non-GUI switch proves
            // Escape9 and arrows15..18. Never rely on GUI string key names.
            if (action.kind == 2 && action.key != 9 && (action.key < 15 || action.key > 18)) return false;
            if (action.kind == 6 && action.key != 39) return false;
            const auto back = (action.kind == 2 && action.key == 9) ||
                (action.kind == 1 && owner->items[action.index].kind == ItemKind::Back);
            cancelForeign = back && foreignDepth;
        }
        // Admission must finish before logical cancellation. Allocation failure
        // leaves both the model and native input behavior unchanged.
        try { pending.push_back(std::move(action)); }
        catch (...) { return false; }
        if (cancelForeign)
        {
            owner->returnPending = true;
            Retire(owner); // Abort ongoing shared API burst; native Back is deferred.
        }
        return true;
    }
    inline bool NativeCurrent(const std::shared_ptr<Context>& owner)
    {
        return owner && context == owner && owner->native.Live(gameModule, scriptModule);
    }
    inline bool NativeCall(const std::shared_ptr<Context>& owner, const std::string& expression)
    {
        if (!NativeCurrent(owner) || owner->native.Modal()) return false;
        Depth scope(nativeDepth);
        if (!Execute(owner->native.script, "g_WFPMenuResult=tostring(" + expression + ");") ||
            !NativeCurrent(owner) || owner->native.Modal()) return false;
        std::string result;
        return Global(owner->native.script, "g_WFPMenuResult", result) && result == "1";
    }
    inline bool Leave(const std::shared_ptr<Context>& owner)
    {
        if (!NativeCurrent(owner)) return false;
        if (owner->native.Modal()) { owner->returnPending = true; return false; }
        if (!NativeCall(owner, "WFP_Leave()")) { owner->returnPending = true; return false; }
        owner->returnPending = false;
        owner->open = false;
        owner->retired = false;
        owner->model.Close();
        owner->items.clear();
        owner->page.reset();
        owner->details = false;
        owner->selected = owner->rootOffset = 0;
        return true;
    }
    inline void Open(const std::shared_ptr<Context>& owner)
    {
        if (!Snapshot(owner, "options") || Field<void*>(owner->native.ui, focusOffset) != owner->native.entry) return;
        const auto serial = owner->serial;
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language != "en" && language != "fr" && language != "de" && language != "it" && language != "es")
            language = NativeLanguage(owner->native.script);
        { Depth scope(foreignDepth); owner->model.OpenPlugins(hostModule, language, 5, {}, true); }
        if (!Available(owner) || owner->serial != serial || !owner->model.IsOpen() ||
            !Snapshot(owner, "options") || Field<void*>(owner->native.ui, focusOffset) != owner->native.entry) return;
        owner->page.reset();
        owner->rowOffset = 0;
        owner->details = false;
        owner->selected = owner->rootOffset = 0;
        if (!Build(owner)) { owner->model.Close(); return; }
        if (!Call(owner, "WFP_Enter()") || !Snapshot(owner, "private"))
        {
            owner->returnPending = true;
            Retire(owner);
            return;
        }
        owner->open = true;
    }
    inline void Describe(const std::shared_ptr<Context>& owner, size_t index)
    {
        if (!Available(owner) || index >= owner->items.size()) return;
        auto service = FontService(owner);
        const auto scaleY = Field<double>(owner->native.ui, verticalScaleOffset);
        const auto restart = RestartRequired(owner);
        auto description = restart ? owner->model.Text("menu.restartwarning") :
            owner->details ? std::string() : owner->items[index].description;
        if (!restart && !owner->details && !description.empty())
            description = owner->model.Text("menu.helpkey") + " | " + description;
        auto fitted = std::isfinite(scaleY) && scaleY > 0 ?
            FitRow(service, fontModule, description, "", 524, 52, 14,
                static_cast<float>(std::max(8.0, std::ceil(8.0 / scaleY)))) : std::optional<FittedText>();
        if (fitted && !fitted->text.empty() && fitted->extent.y * scaleY < 8) fitted.reset();
        if (Available(owner)) Call(owner, "WFP_Describe(" + Quote(fitted ? fitted->text : std::string()) + "," +
            std::to_string(static_cast<int>(fitted ? fitted->size : 14)) + ")");
    }
    inline void Act(const std::shared_ptr<Context>& owner, const Pending& action)
    {
        if (!Available(owner) || action.serial != owner->serial || action.index < 0 ||
            static_cast<size_t>(action.index) >= owner->items.size() || !Snapshot(owner, "private") ||
            action.sender != owner->native.rows[action.index] || !owner->native.Selected(action.index)) return;
        const auto item = owner->items[action.index];
        owner->selected = static_cast<size_t>(action.index);
        if (action.kind == 6)
        {
            if (owner->details)
            {
                owner->details = false;
                owner->selected = owner->helpSelected;
            }
            else
            {
                owner->helpLines = Wrap(FontService(owner), fontModule, item.description);
                if (owner->helpLines.empty())
                    return;
                owner->helpTitle = item.label;
                owner->helpSelected = owner->selected;
                owner->helpPage = 0;
                owner->selected = 0;
                owner->details = true;
            }
            if (!Build(owner))
            {
                owner->returnPending = true;
                Retire(owner);
            }
            else
                Describe(owner, owner->selected);
            return;
        }
        if (action.kind == 3 || action.kind == 4)
        {
            owner->selected = action.index;
            Describe(owner, owner->selected);
            return;
        }
        if (action.kind == 2 && (action.key == 16 || action.key == 18))
        {
            size_t selected = action.index;
            for (size_t tries = 0; tries < owner->items.size(); ++tries)
            {
                selected = action.key == 16 ? (selected + owner->items.size() - 1) % owner->items.size() :
                    (selected + 1) % owner->items.size();
                if (owner->items[selected].enabled) break;
            }
            if (Call(owner, "WFP_Select(" + std::to_string(selected) + ")")) owner->selected = selected;
            Describe(owner, owner->selected);
            return;
        }
        const auto accept = action.kind == 1;
        const auto back = action.kind == 2 && action.key == 9;
        const auto direction = accept ? 0 : action.key == 15 ? -1 : 1;
        const auto oldPage = owner->page;
        const auto oldOffset = owner->rootOffset;
        const auto oldSelection = owner->selected;
        bool navigation = true;
        if (owner->details)
        {
            if (back || (accept && item.kind == ItemKind::HelpBack))
            {
                owner->details = false;
                owner->selected = owner->helpSelected;
            }
            else if (accept && item.kind == ItemKind::HelpNext)
                ++owner->helpPage;
            else if (accept && item.kind == ItemKind::HelpPrevious && owner->helpPage)
                --owner->helpPage;
            else
                return;
            if (!Build(owner))
            {
                owner->returnPending = true;
                Retire(owner);
            }
            else
                Describe(owner, owner->selected);
            return;
        }
        if (back || (accept && item.kind == ItemKind::Back))
        {
            if (!owner->page) { Leave(owner); return; }
            owner->page.reset();
        }
        else if (accept && item.kind == ItemKind::Page)
        {
            if (item.target >= owner->model.Pages().size()) return;
            owner->page = item.target;
            owner->rowOffset = 0;
            owner->model.PageShown(item.target);
        }
        else if (accept && (item.kind == ItemKind::Next || item.kind == ItemKind::Previous)) owner->rootOffset = item.target;
        else if (accept && (item.kind == ItemKind::RowNext || item.kind == ItemKind::RowPrevious))
        {
            owner->rowOffset = item.target;
            owner->model.PageShown(*owner->page);
        }
        else if (item.kind == ItemKind::Row)
        {
            navigation = false;
            if (!item.enabled || (!accept && item.row.kind != RWGTAMenu::RowKind::Setting &&
                item.row.kind != RWGTAMenu::RowKind::Preset)) return;
            bool enabled;
            { Depth scope(foreignDepth); enabled = owner->model.Enabled(item.row, owner->pause); }
            if (!enabled || !Available(owner) || owner->serial != action.serial ||
                !owner->native.Selected(action.index) || !Snapshot(owner, "private")) return;
            { Depth scope(foreignDepth); owner->model.Change(item.row, direction, owner->pause); }
            if (!Available(owner) || owner->serial != action.serial || !Snapshot(owner, "private")) return;
        }
        else return;
        owner->selected = navigation ? 0 : oldSelection;
        if (!Build(owner))
        {
            owner->page = oldPage;
            owner->rootOffset = oldOffset;
            // Failed native/foreign commit never leaves old visible widgets
            // bound to new navigation. Return through the native parent route.
            owner->returnPending = true;
            Retire(owner);
            return;
        }
        Call(owner, "WFP_Select(" + std::to_string(owner->selected) + ")");
        Describe(owner, owner->selected);
    }
    inline void Drain()
    {
        if (nativeDepth || foreignDepth || draining) return;
        struct Draining { Draining() { draining = true; } ~Draining() { draining = false; } } scope;
        auto owner = context;
        if (!owner) { pending.clear(); return; }
        if (!owner->open && !owner->returnPending && !owner->retired && pending.empty()) return;
        if (!NativeCurrent(owner))
        {
            Retire(owner);
            pending.clear();
            // Missing private controls do not authorize touching them. Lua's
            // retirement path checks exact screen/table ownership and only
            // restores a still-active owned screen to the native Options.
            auto ui = owner->native.ui;
            if (Table(ui, uiTable) &&
                Field<void*>(ui, uiScriptOffset) == owner->native.script &&
                Table(owner->native.script, scriptTable))
            {
                if (Field<void*>(ui, modalOffset)) return;
                Depth depth(nativeDepth);
                Execute(owner->native.script, "WFP_Retire(" + Quote(owner->epoch) + ");");
            }
            if (context == owner) context.reset();
            return;
        }
        if (owner->native.Modal()) return; // Hold covered owner; no Model calls.
        if (owner->open && !Field<unsigned char>(owner->native.page, screenActiveOffset))
        {
            // A stock transition can cover/leave the private page without a
            // foreign settings callback. Drop only the logical session at the
            // safe native boundary; keep the engine-owned pool reusable.
            owner->returnPending = owner->retired = owner->open = false;
            owner->model.Close(); owner->items.clear(); owner->page.reset();
            owner->selected = owner->rootOffset = 0;
            pending.clear();
            return;
        }
        if (owner->returnPending)
        {
            if (!Field<unsigned char>(owner->native.page, screenActiveOffset))
            {
                owner->returnPending = owner->retired = owner->open = false;
                owner->model.Close(); owner->items.clear(); owner->page.reset();
                owner->selected = owner->rootOffset = 0;
            }
            else Leave(owner);
            pending.clear();
            return;
        }
        while (!pending.empty())
        {
            auto action = std::move(pending.front());
            pending.pop_front();
            if (!Available(owner)) break;
            if (action.kind == 0 && action.serial == owner->epoch) Open(owner);
            else Act(owner, action);
        }
        if (Available(owner) && owner->open && Snapshot(owner, "private") && (RWGTAMenu::Model::Changed() ||
            owner->layoutScaleY != Field<double>(owner->native.ui, verticalScaleOffset)))
        {
            if (!Build(owner)) { owner->returnPending = true; Retire(owner); }
            else Describe(owner, owner->selected);
        }
    }

    inline void Hotkeys()
    {
        static bool previous = false;
        const auto owner = context;
        const bool down = Available(owner) && owner->open && GetForegroundWindow() == FarCryWindow::Window &&
            (GetAsyncKeyState(VK_F1) & 0x8000);
        if (down && !previous)
        {
            const auto focus = Field<void*>(owner->native.ui, focusOffset);
            const auto row = std::find(owner->native.rows.begin(), owner->native.rows.end(), focus);
            if (row != owner->native.rows.end())
                Queue({owner->serial, 6, int(row - owner->native.rows.begin()), 39, focus});
        }
        previous = down;
    }

}

export namespace FarCryMenu
{
    inline safetyhook::InlineHook updateHook, drawHook, inputHook, brokerHook, releaseHook, destroyHook,
        scriptReleaseHook, fontReleaseHook, switchHook;
    inline bool ready{}, preparing{}, bridgeReady{};
    inline unsigned releasing{}, scriptClosing{}, fontClosing{};
    inline void* frameUI{};
    inline void* bridgeScript{};
    inline void* bridgeState{};
    inline void* preparingUI{};
    inline std::string preparingEpoch;
    inline uint32_t uiLifetime{}, preparingLifetime{};
    inline size_t preparationDelay{};
    inline std::optional<uint32_t> blockedLifetime;
    inline std::set<std::string> logged;
    inline void Log(const std::string& reason) noexcept
    {
        try
        {
            if (!logged.insert(reason).second) return;
            OutputDebugStringA(("FarCry native menu: " + reason + "\n").c_str());
        }
        catch (...) {} // Diagnostics cannot unwind into native engine frames.
    }
    inline void Invalidate()
    {
        ++uiLifetime;
        if (context) { context->native.retired = true; Retire(context); }
        pending.clear();
    }
    inline bool BridgeAvailable(void* state)
    {
        return ready && bridgeReady && !scriptClosing && bridgeScript && bridgeState == state &&
            Table(bridgeScript, scriptTable) && State(bridgeScript) == state;
    }
    inline void* CallbackHandler(void* state)
    {
        if (!BridgeAvailable(state)) return nullptr;
        auto handler = Handler(bridgeScript);
        if (!Table(handler, handlerTable)) return nullptr;
        Attach(handler, state);
        return handler;
    }
    inline int BindCallbackBody(void* state)
    {
        Depth scope(nativeDepth);
        auto handler = CallbackHandler(state);
        if (!handler) return 0;
        bool accepted = false;
        std::string epoch;
        if (preparing && !releasing && preparingLifetime == uiLifetime && ParamCount(handler) == 14 &&
            String(handler, 1, epoch) && epoch == preparingEpoch && Table(preparingUI, uiTable))
        {
            auto owner = std::make_shared<Context>();
            auto& pool = owner->native;
            pool.ui = preparingUI; pool.script = bridgeScript;
            pool.source = NativeParameter(bridgeScript, handler, 2, scriptModule);
            pool.page = NativeParameter(bridgeScript, handler, 3, scriptModule);
            pool.entry = NativeParameter(bridgeScript, handler, 4, scriptModule);
            pool.heading = NativeParameter(bridgeScript, handler, 5, scriptModule);
            pool.description = NativeParameter(bridgeScript, handler, 6, scriptModule);
            pool.prototype = NativeParameter(bridgeScript, handler, 7, scriptModule);
            for (size_t i = 0; i < pool.rows.size(); ++i)
                pool.rows[i] = NativeParameter(bridgeScript, handler, static_cast<int>(8 + i), scriptModule);
            const auto prototype = Field<NativeRect>(pool.prototype, widgetRectOffset);
            pool.entryBounds = {prototype.x, prototype.y + prototype.height, prototype.width, prototype.height};
            owner->epoch = owner->serial = epoch;
            owner->lifetime = preparingLifetime;
            if (preparingLifetime == uiLifetime && !releasing && pool.Live(gameModule, scriptModule) &&
                prototype.width >= 64 && prototype.height >= 16 && pool.entryBounds.y + pool.entryBounds.height <= 580)
            {
                std::set<void*> unique{pool.source, pool.page, pool.entry, pool.heading, pool.description, pool.prototype};
                unique.insert(pool.rows.begin(), pool.rows.end());
                auto system = Field<void*>(pool.ui, uiSystemOffset);
                auto font = Table(system, systemTable) ? Field<void*>(system, systemFontOffset) : nullptr;
                auto texts = FixSettings().GetTexts(); texts.Select("en");
                const auto title = texts.Resolve("@menu.title");
                auto extent = MeasureLiteral(font, fontModule, title, 14);
                const auto scaleY = Field<double>(pool.ui, verticalScaleOffset);
                accepted = unique.size() == 13 && extent && extent->x <= prototype.width - 16 &&
                    extent->y <= prototype.height - 4 && std::isfinite(scaleY) && scaleY > 0 && extent->y * scaleY >= 8;
                if (accepted) context = owner;
            }
        }
        if (!accepted) Log("native source/pool/font/geometry binding refused");
        return Return(handler, accepted);
    }
    inline int QueueCallbackBody(void* state)
    {
        Depth scope(nativeDepth);
        auto handler = CallbackHandler(state);
        if (!handler) return 0;
        Pending action;
        bool accepted = ParamCount(handler) == 5 && String(handler, 1, action.serial) &&
            Integer(handler, 2, action.kind) && Integer(handler, 3, action.index) && Integer(handler, 4, action.key);
        if (accepted)
        {
            action.sender = NativeParameter(bridgeScript, handler, 5, scriptModule);
            accepted = action.kind >= 0 && action.kind <= 5 && Queue(std::move(action));
        }
        return Return(handler, accepted);
    }
    inline void CallbackFailure(const char* phase) noexcept
    {
        try
        {
            if (context) { context->returnPending = true; Retire(context); }
            pending.clear();
            Log(phase);
        }
        catch (...) {} // Only logical cancellation; native retirement stays deferred.
    }
    inline int __cdecl BindCallback(void* state) noexcept
    {
        try { return BindCallbackBody(state); }
        catch (...) { CallbackFailure("native Bind callback exception refused"); return 0; }
    }
    inline int __cdecl QueueCallback(void* state) noexcept
    {
        try { return QueueCallbackBody(state); }
        catch (...) { CallbackFailure("native Queue callback exception refused"); return 0; }
    }
    inline bool Bootstrap(void* ui)
    {
        if (preparing || releasing || scriptClosing || fontClosing || !ready ||
            !Table(ui, uiTable) ||
            !(Field<unsigned char>(ui, uiFlagsOffset) & 4)) return false;
        auto script = Field<void*>(ui, uiScriptOffset);
        if (!Table(script, scriptTable)) return false;
        if (!bridgeScript)
        {
            if (!NamespaceFree(script, scriptModule)) { Log("reserved Lua namespace occupied"); return false; }
            bridgeScript = script;
            bridgeState = State(script);
            if (!bridgeState || !Execute(script, nativeMenuLua)) { Log("native Lua definitions refused"); return false; }
            auto object = CreateGlobal(script, "WFP_Bridge");
            if (!Table(object, objectTable)) return false;
            const auto installed = AddFunction(object, "Bind", BindCallback, 1) &&
                AddFunction(object, "Queue", QueueCallback, 2);
            ReleaseObject(object);
            if (!installed) { Log("native retained Lua closures refused"); return false; }
            bridgeReady = true;
        }
        if (!bridgeReady || script != bridgeScript || State(script) != bridgeState) return false;
        preparingEpoch = Token();
        if (preparingEpoch.empty()) { Log("native generation exhausted"); return false; }
        preparingUI = frameUI = ui;
        preparingLifetime = uiLifetime;
        struct Preparing { Preparing() { preparing = true; } ~Preparing() { preparing = false; preparingUI = nullptr; } } guard;
        auto texts = FixSettings().GetTexts(); texts.Select("en");
        auto title = NativeLiteral(texts.Resolve("@menu.title"));
        if (!title) return false;
        {
            Depth depth(nativeDepth);
            if (!Execute(script, "WFP_Prepare(" + Quote(preparingEpoch) + "," + Quote(*title) + ",14);"))
            { Log("native Lua factory buffer refused"); return false; }
        }
        std::string status;
        const bool statusRead = Global(script, "g_WFPMenuStatus", status);
        const bool accepted = context && context->epoch == preparingEpoch && Available(context) &&
            statusRead && status == "ready";
        if (!accepted)
        {
            Log("native prepare refused: " + status);
            // A failed partial pool stays engine-owned and hidden until the
            // normal native Release. Never repeat a colliding factory round.
            if (status.rfind("source-not-ready", 0) != 0 && !status.empty()) blockedLifetime = uiLifetime;
            { Depth depth(nativeDepth); Execute(script, "WFP_Retire(" + Quote(preparingEpoch) + ");"); }
            if (context && context->epoch == preparingEpoch) { Retire(context); context.reset(); }
        }
        return accepted;
    }
    inline void FrameBody(void* ui)
    {
        if (!ready || nativeDepth || foreignDepth || draining || preparing || releasing || scriptClosing || fontClosing) return;
        if (Tick)
            Tick();
        Hotkeys();
        frameUI = ui;
        if (!context)
        {
            if (blockedLifetime && *blockedLifetime == uiLifetime) return;
            if (preparationDelay) { --preparationDelay; return; }
            if (!Bootstrap(ui)) preparationDelay = 60;
            return;
        }
        auto before = context;
        Drain();
        if (before && !context) blockedLifetime = before->lifetime;
    }
    inline void Frame(void* ui) noexcept
    {
        try { FrameBody(ui); }
        catch (...)
        {
            CallbackFailure("native post-update menu exception refused");
            if (!context) blockedLifetime = uiLifetime;
        }
    }
    inline void ForeignTransition()
    {
        auto owner = context;
        if (!foreignDepth || !owner || owner->retired || !owner->model.IsOpen()) return;
        const auto still = NativeCurrent(owner) && !owner->native.Modal() &&
            (Field<unsigned char>(owner->native.ui, uiFlagsOffset) & 4) &&
            (owner->open ? owner->native.Selected(owner->selected) &&
                Field<unsigned char>(owner->native.page, screenActiveOffset) != 0 :
                Field<void*>(owner->native.ui, focusOffset) == owner->native.entry &&
                Field<unsigned char>(owner->native.source, screenActiveOffset) != 0);
        if (!still) { owner->returnPending = true; Retire(owner); }
    }
    inline bool PrivateSwitchOwner(void* receiver, bool in)
    {
        auto owner = context;
        return !in && NativeCurrent(owner) && (owner->open || owner->returnPending) && !owner->native.Modal() &&
            Field<void*>(owner->native.ui, uiScriptObjectOffset) == receiver &&
            Field<void*>(receiver, scriptOwnerUiOffset) == owner->native.ui &&
            Field<void*>(receiver, scriptOwnerServiceOffset) == owner->native.script &&
            (Field<unsigned char>(owner->native.ui, uiFlagsOffset) & 4) &&
            Field<unsigned char>(owner->native.page, screenActiveOffset) != 0 &&
            std::find(owner->native.rows.begin(), owner->native.rows.end(),
                Field<void*>(owner->native.ui, focusOffset)) != owner->native.rows.end();
    }
#ifdef _WIN64
    inline int CanSwitch(void* receiver, bool in)
    { Depth scope(nativeDepth); return PrivateSwitchOwner(receiver, in) ? 0 : switchHook.call<int>(receiver, in); }
    inline uintptr_t Updated(void* ui) { const auto lifetime = uiLifetime; uintptr_t result; { Depth scope(nativeDepth); result = updateHook.call<uintptr_t>(ui); } if (lifetime == uiLifetime) Frame(ui); return result; }
    inline uintptr_t Drawn(void* ui) { Depth scope(nativeDepth); return drawHook.call<uintptr_t>(ui); }
    inline char Input(void* ui, void* event) { Depth scope(nativeDepth); return inputHook.call<char>(ui, event); }
    inline intptr_t Broker(void* ui, void* widget, unsigned message, uintptr_t wParam, intptr_t lParam)
    { Depth scope(nativeDepth); ForeignTransition(); auto result = brokerHook.call<intptr_t>(ui, widget, message, wParam, lParam); ForeignTransition(); return result; }
    inline int Released(void* ui)
    {
        Depth scope(nativeDepth); Depth phase(releasing); Invalidate();
        const auto result = releaseHook.call<int>(ui);
        preparationDelay = 0; return result;
    }
    inline uintptr_t Destroyed(void* ui, uintptr_t allocatorContext) { Depth scope(nativeDepth); Invalidate(); return destroyHook.call<uintptr_t>(ui, allocatorContext); }
    inline uintptr_t ScriptReleased(void* script)
    { Depth scope(nativeDepth); Depth phase(scriptClosing); Invalidate(); const auto result = scriptReleaseHook.call<uintptr_t>(script); bridgeScript = bridgeState = nullptr; bridgeReady = false; return result; }
    inline uintptr_t FontReleased(void* font)
    { Depth scope(nativeDepth); Depth phase(fontClosing); Invalidate(); return fontReleaseHook.call<uintptr_t>(font); }
#else
    inline int __fastcall CanSwitch(void* receiver, void*, bool in)
    { Depth scope(nativeDepth); return PrivateSwitchOwner(receiver, in) ? 0 : switchHook.thiscall<int>(receiver, in); }
    inline uintptr_t __fastcall Updated(void* ui, void*) { const auto lifetime = uiLifetime; uintptr_t result; { Depth scope(nativeDepth); result = updateHook.thiscall<uintptr_t>(ui); } if (lifetime == uiLifetime) Frame(ui); return result; }
    inline uintptr_t __fastcall Drawn(void* ui, void*) { Depth scope(nativeDepth); return drawHook.thiscall<uintptr_t>(ui); }
    inline char __fastcall Input(void* ui, void*, void* event) { Depth scope(nativeDepth); return inputHook.thiscall<char>(ui, event); }
    inline intptr_t __fastcall Broker(void* ui, void*, void* widget, unsigned message, uintptr_t wParam, intptr_t lParam)
    { Depth scope(nativeDepth); ForeignTransition(); auto result = brokerHook.thiscall<intptr_t>(ui, widget, message, wParam, lParam); ForeignTransition(); return result; }
    inline int __fastcall Released(void* ui, void*)
    { Depth scope(nativeDepth); Depth phase(releasing); Invalidate(); const auto result = releaseHook.thiscall<int>(ui); preparationDelay = 0; return result; }
    inline uintptr_t __fastcall Destroyed(void* ui, void*) { Depth scope(nativeDepth); Invalidate(); return destroyHook.thiscall<uintptr_t>(ui); }
    inline uintptr_t __fastcall ScriptReleased(void* script, void*)
    { Depth scope(nativeDepth); Depth phase(scriptClosing); Invalidate(); const auto result = scriptReleaseHook.thiscall<uintptr_t>(script); bridgeScript = bridgeState = nullptr; bridgeReady = false; return result; }
    inline uintptr_t __fastcall FontReleased(void* font, void*)
    { Depth scope(nativeDepth); Depth phase(fontClosing); Invalidate(); return fontReleaseHook.thiscall<uintptr_t>(font); }
#endif
    inline void Init()
    {
        if (ready) return;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &hostModule);
        gameModule = GetModuleHandleW(L"CryGame.dll"); scriptModule = GetModuleHandleW(L"CryScriptSystem.dll");
        fontModule = GetModuleHandleW(L"CryFont.dll"); systemModule = GetModuleHandleW(L"CrySystem.dll");
        if (!gameModule || !scriptModule || !fontModule || !systemModule || !GetModuleHandleW(L"CryInput.dll")) return;
#ifdef _WIN64
        auto update = hook::module_pattern(gameModule, "48 8B C4 48 81 EC F8 00 00 00 48 C7 44 24 60 FE FF FF FF 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 4C 89 60 F8 4C 89 68 F0 4C 89 70 E8 4C 89 78 E0 66 0F 7F 70 C8 66 0F 7F 78 B8");
#else
        auto update = hook::module_pattern(gameModule, "B8 ? ? ? ? E8 ? ? ? ? 83 EC 3C 53 33 DB 56 43 84 1D ? ? ? ? 57 8B F1 75 22 09 1D");
#endif
        const auto updateSite = update.get_first();
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &hostModule)) { Log("process lifetime pin refused"); return; }
#ifdef _WIN64
        uiTable = ModuleOperand(gameModule, "48 8D 05 ? ? ? ? 48 89 01 48 8D 05 ? ? ? ? 48 89 41 08 33 F6", 3);
        buttonTable = ModuleOperand(gameModule, "48 8D 05 ? ? ? ? 48 89 01 48 8D 05 ? ? ? ? 48 89 81 88 01 00 00 48 83 B9 C8 01 00 00 08", 3);
        staticTable = ModuleOperand(gameModule, "48 8D 05 ? ? ? ? 48 89 07 48 8D 05 ? ? ? ? 48 89 87 88 01 00 00 48 89 B7 B0 01 00 00", 3);
        systemTable = ModuleOperand(systemModule, "48 8D 05 ? ? ? ? 48 89 01 0F B6 91 C9 26 00 00", 3);
        scriptTable = ModuleOperand(scriptModule, "48 8D 05 ? ? ? ? 48 89 01 48 8B 49 08", 3);
        handlerTable = ModuleOperand(scriptModule, "48 8D 05 ? ? ? ? 48 89 01 C3 CC", 3);
        objectTable = ModuleOperand(scriptModule, "48 8D 05 ? ? ? ? 41 89 4B 18 49 89 03", 3);
        fontServiceTable = ModuleOperand(fontModule, "48 8D 05 ? ? ? ? 48 89 01 48 83 C1 08", 3);
        fontTable = ModuleOperand(fontModule, "48 8D 05 ? ? ? ? 48 89 01 45 33 ED", 3);
        auto draw = hook::module_pattern(gameModule, "48 8B C4 48 81 EC F8 00 00 00 48 C7 84 24 A8 00 00 00 FE FF FF FF").get_first();
        auto input = hook::module_pattern(gameModule, "48 83 EC 48 48 89 74 24 60 48 89 7C 24 68").get_first();
        auto broker = hook::module_pattern(gameModule, "48 83 EC 58 4C 8B 91 D0 00 00 00 48 8B 81 C8 00 00 00").get_first();
        auto release = hook::module_pattern(gameModule, "48 83 EC 28 48 89 7C 24 48 48 8B F9 48 8B 89 88 00 00 00").get_first();
        auto destroy = hook::module_pattern(gameModule, "48 89 4C 24 08 48 83 EC 38 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 48 48 89 74 24 50 48 89 7C 24 58 48 8B D9 48 8D 05 ? ? ? ? 48 89 01 48 8D 05 ? ? ? ? 48 89 41 08 48 8B 89 E0 02 00 00").get_first();
        auto canSwitch = hook::module_pattern(gameModule, "48 83 EC 28 48 89 5C 24 40 48 89 7C 24 48 0F B6 FA 8B 51 34").get_first();
        auto scriptRelease = hook::module_pattern(scriptModule, "48 53 48 83 EC 20 48 8B D9 66 66 90").get_first();
        auto fontRelease = hook::module_pattern(fontModule, "48 83 EC 28 48 85 C9 74 0B 48 8B 01").get_first();
#else
        uiTable = ModuleOperand(gameModule, "C7 06 ? ? ? ? C7 46 04 ? ? ? ? 89 7E 58", 2);
        buttonTable = ModuleOperand(gameModule, "C7 06 ? ? ? ? C7 07 ? ? ? ? 83 65 FC 00 8D 8E E4 00 00 00", 2);
        staticTable = ModuleOperand(gameModule, "C7 06 ? ? ? ? E8 ? ? ? ? D9 EE D9 9E F4 00 00 00", 2);
        systemTable = ModuleOperand(systemModule, "C7 06 ? ? ? ? E8 ? ? ? ? 33 DB", 2);
        scriptTable = ModuleOperand(scriptModule, "C7 06 ? ? ? ? 8B 46 04 33 DB 3B C3", 2);
        handlerTable = ModuleOperand(scriptModule, "C7 00 ? ? ? ? C3 F6 44 24 04 01", 2);
        objectTable = ModuleOperand(scriptModule, "C7 00 ? ? ? ? 89 48 08 89 48 14", 2);
        fontServiceTable = ModuleOperand(fontModule, "C7 06 ? ? ? ? 8B 4E 10 8B 01 33 FF", 2);
        fontTable = ModuleOperand(fontModule, "C7 06 ? ? ? ? C7 45 FC 02 00 00 00", 2);
        auto draw = hook::module_pattern(gameModule, "B8 ? ? ? ? E8 ? ? ? ? 83 EC 20 F6 05 ? ? ? ? 01").get_first();
        auto input = hook::module_pattern(gameModule, "55 8B EC 51 56 8B F1 8B 4E 58 8B 01").get_first();
        auto broker = hook::module_pattern(gameModule, "55 8B EC 56 8B 75 08 56 E8 ? ? ? ?").get_first();
        auto release = hook::module_pattern(gameModule, "56 8B F1 8B 46 6C 85 C0 74 0D 8B 08").get_first();
        auto destroy = hook::module_pattern(gameModule, "56 8B F1 8D 8E B4 01 00 00 C7 06 ? ? ? ?").get_first();
        auto canSwitch = hook::module_pattern(gameModule, "55 8B EC 51 56 8B F1 8B 46 1C 85 C0").get_first();
        auto scriptRelease = hook::module_pattern(scriptModule, "56 8B F1 EB 18 8B 46 4C 8B 48 FC 85 C9").get_first();
        auto fontRelease = hook::module_pattern(fontModule, "85 C9 74 07 8B 01 6A 01 FF 50 10 C3").get_first();
#endif
        updateHook = safetyhook::create_inline(updateSite, &Updated);
        drawHook = safetyhook::create_inline(draw, &Drawn);
        inputHook = safetyhook::create_inline(input, &Input);
        brokerHook = safetyhook::create_inline(broker, &Broker);
        releaseHook = safetyhook::create_inline(release, &Released);
        destroyHook = safetyhook::create_inline(destroy, &Destroyed);
        scriptReleaseHook = safetyhook::create_inline(scriptRelease, &ScriptReleased);
        fontReleaseHook = safetyhook::create_inline(fontRelease, &FontReleased);
        switchHook = safetyhook::create_inline(canSwitch, &CanSwitch);
        if (!updateHook || !drawHook || !inputHook || !brokerHook || !releaseHook || !destroyHook || !scriptReleaseHook || !fontReleaseHook || !switchHook)
        {
            switchHook.reset(); fontReleaseHook.reset(); scriptReleaseHook.reset(); destroyHook.reset(); releaseHook.reset();
            brokerHook.reset(); inputHook.reset(); drawHook.reset(); updateHook.reset();
            Log("native hook transaction refused"); return;
        }
        ready = true;
    }
}
