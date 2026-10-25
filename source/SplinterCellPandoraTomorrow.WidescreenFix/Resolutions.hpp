#pragma once

namespace PTMenu
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
        const auto engine = GetModuleHandleW(L"Engine.dll");
        auto entry = GetProcAddress(engine, "?execUpdateEngineSettings@UEPCGameOptions@@QAEXAAUFFrame@@QAX@Z");
        auto body = reinterpret_cast<uint8_t*>(entry);
        for (int i = 0; body && i < 4 && body[0] == 0xE9; ++i)
            body += 5 + *reinterpret_cast<int32_t*>(body + 1);
        if (!body)
            return false;
        const auto start = reinterpret_cast<uintptr_t>(body);
        auto digital = hook::range_pattern(start, start + 48,
            "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 83 EC 28 53 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4");
        auto retail = hook::range_pattern(start, start + 48,
            "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 24 53 56 57 8B 7D 08 89 65 F0 8B F1");
        if ((digital.size() != 1 || digital.get_first() != body) && (retail.size() != 1 || retail.get_first() != body))
            return false;
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
        ProtectedGameRef<int>{static_cast<int*>(result)} = index;
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
