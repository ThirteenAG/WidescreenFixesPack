#pragma once

// Shipped PT KeyListBoxItem arrays remain engine-owned. Nonzero special-row
// flags keep these rows out of native key-binding refresh/save/restore loops.
namespace PTMenu
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
        if (!gui || !core) return;
        auto draw = GetProcAddress(gui, "?Draw@UGUIKeyListSC2_PC@@UAEXPAVUCanvas@@@Z");
        destroyArray = reinterpret_cast<decltype(destroyArray)>(GetProcAddress(core, "?DestroyValue@UArrayProperty@@UBEXPAX@Z"));
        if (draw && destroyArray) controlsDrawHook = safetyhook::create_inline(draw, DrawControls);
    }
}
