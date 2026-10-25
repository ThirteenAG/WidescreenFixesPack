#pragma once

namespace PTMenu
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
        ProtectedGameRef<VideoDelegate>{ static_cast<VideoDelegate*>(Address(destination, to)) } = value;
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
