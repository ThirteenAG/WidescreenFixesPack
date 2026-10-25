#pragma once

// Included inside SCMenu after its checked reflection helpers.
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
    const auto base = reinterpret_cast<uintptr_t>(engine);
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(engine);
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    const auto imageSize = nt->OptionalHeader.SizeOfImage;
    const auto inside = [base, imageSize](const void* pointer, size_t size)
    {
        const auto address = reinterpret_cast<uintptr_t>(pointer);
        return address >= base && size <= imageSize && address - base <= imageSize - size;
    };
    auto body = reinterpret_cast<uint8_t*>(entry);
    for (int i = 0; i < 4; ++i)
    {
        if (!inside(body, 5))
            return false;
        if (body[0] != 0xE9)
            break;
        body += 5 + *reinterpret_cast<int32_t*>(body + 1);
    }
    // Another native handler shares this prologue. Validate the resolved
    // export itself; a module-wide uniqueness check rejects supported engines.
    constexpr uint8_t expected[]{
        0x83, 0xEC, 0x10, 0x56, 0x57, 0x8B, 0x7C, 0x24, 0x1C, 0x33,
        0xD2, 0xC7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, 0x8B,
        0xF1, 0x8B, 0x47, 0x0C, 0x8B, 0x4F, 0x08, 0x8A, 0x10
    };
    if (!inside(body, sizeof(expected)) || std::memcmp(body, expected, sizeof(expected)))
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
