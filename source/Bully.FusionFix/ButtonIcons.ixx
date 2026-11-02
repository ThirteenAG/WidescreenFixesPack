module;

#include <stdafx.h>
#include "Shared/PadInput.hpp"
#include <unordered_map>

export module ButtonIcons;

export namespace BullyIcons
{
    SafetyHookInline HudLoadHook;
    const int* Selected = nullptr;
    std::array<std::array<uintptr_t, 29>, 3> Textures{};
    std::array<std::array<std::array<uint32_t, 14>, 29>, 3> Rasters{};
    std::unordered_map<uintptr_t, std::pair<const char*, int>> Classes;
    int(__cdecl* CreateStore)(const char*) = nullptr;
    void(__thiscall* ConstructStream)(void*) = nullptr;
    void(__thiscall* DestroyStream)(void*) = nullptr;
    char(__thiscall* ReadStream)(void*, const char*) = nullptr;
    char(__cdecl* LoadStore)(int, void*, int) = nullptr;
    void(__cdecl* AddRef)(int) = nullptr;
    uintptr_t(__cdecl* LoadTexture)(const char*) = nullptr;
    bool Loaded = false;

    constexpr const char* Names[] =
    {
        "a", "b", "x", "y", "lb", "rb", "lt", "rt", "ls", "rs", "back", "start",
        "up", "down", "left", "right", "dpad", "updown", "leftright", "lstick",
        "lstickupdown", "lstickleftright", "lstickup", "lstickdown", "rstick", "rstickupdown", "rstickleftright", "lbrb", "ltrt"
    };

    int Classify(const char* name)
    {
        if (!name)
            return -1;
        auto file = strrchr(name, '\\');
        name = file ? file + 1 : name;
        auto slash = strrchr(name, '/');
        name = slash ? slash + 1 : name;
        std::string key(name);
        if (auto extension = key.rfind('.'); extension != std::string::npos)
            key.resize(extension);
        for (auto& c : key)
            c = char(std::tolower(static_cast<unsigned char>(c)));
        static const std::pair<const char*, int> icons[] =
        {
            { "hud_buttoncross", 0 }, { "hrbutton_cross", 0 },
            { "hud_buttoncircle", 1 }, { "hrbutton_circl", 1 },
            { "hud_buttonsquare", 2 }, { "hrbutton_squa", 2 },
            { "hud_buttontriangle", 3 }, { "hrbutton_tri", 3 },
            { "button_l1", 4 }, { "button_r1", 5 }, { "button_l2", 6 }, { "button_r2", 7 },
            { "hrbutton_l1", 4 }, { "hrbutton_r1", 5 }, { "hrbutton_l2", 6 }, { "hrbutton_r2", 7 },
            { "analog_l3", 8 }, { "analog_r3", 9 }, { "button_select", 10 }, { "button_start", 11 },
            { "hrbutton_l3", 8 }, { "hrbutton_r3", 9 },
            { "button_arrowu", 12 }, { "button_arrowd", 13 }, { "button_arrowl", 14 }, { "button_arrowr", 15 },
            { "analog_l", 19 }, { "analog_r", 24 }, { "hrbutton_l3a", 19 }, { "hrbutton_r3a", 24 },
            { "hrbutton_lr1_glow", 27 }, { "hrbutton_lr2_glow", 28 }
        };
        for (const auto& [text, icon] : icons)
        {
            if (key == text)
                return icon;
        }
        return -1;
    }

    uintptr_t Texture(uintptr_t original)
    {
        if (!Loaded || !original || !Selected)
            return original;
        const auto source = *reinterpret_cast<uintptr_t*>(original + 52);
        if (!source)
            return original;
        const auto name = *reinterpret_cast<const char**>(source + 8);
        auto found = Classes.find(original);
        if (found == Classes.end() || found->second.first != name)
        {
            if (Classes.size() >= 4096)
                Classes.clear();
            found = Classes.insert_or_assign(original, std::pair{ name, Classify(name) }).first;
        }
        if (found->second.second < 0)
            return original;
        const auto family = size_t(PadInput::ResolveFamily(*Selected)) - 1;
        const auto replacement = Textures[family][found->second.second];
        return replacement ? reinterpret_cast<uintptr_t>(Rasters[family][found->second.second].data()) : original;
    }

    void Load()
    {
        if (Loaded)
            return;
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Load), &module);
        char path[MAX_PATH]{};
        if (!GetModuleFileNameA(module, path, DWORD(std::size(path))))
            return;
        const auto dictionary = std::filesystem::path(path).replace_extension() / "buttons.nft";
        if (!std::filesystem::exists(dictionary))
        {
            OutputDebugStringA("Bully: controller artwork is missing.\n");
            return;
        }
        alignas(16) std::array<uint8_t, 1036> stream{};
        ConstructStream(stream.data());
        if (ReadStream(stream.data(), dictionary.string().c_str()))
        {
            const int store = CreateStore("BullyWidescreenButtons");
            if (store >= 0 && LoadStore(store, stream.data(), 0))
            {
                AddRef(store);
                const char* families[] = { "xbox", "playstation", "nintendo" };
                for (size_t family = 0; family < Textures.size(); ++family)
                {
                    for (size_t icon = 0; icon < std::size(Names); ++icon)
                    {
                        const auto name = std::string("wfp_") + families[family] + "_" + Names[icon];
                        Textures[family][icon] = LoadTexture(name.c_str());
                        auto& raster = Rasters[family][icon];
                        raster[3] = raster[4] = raster[10] = raster[11] = 64;
                        raster[8] = 0x05000004;
                        raster[13] = Textures[family][icon];
                    }
                }
                Loaded = true;
            }
        }
        DestroyStream(stream.data());
        if (!Loaded)
            OutputDebugStringA("Bully: controller artwork could not be loaded.\n");
    }

    int __cdecl HudLoad()
    {
        const int result = HudLoadHook.ccall<int>();
        Classes.clear();
        Load();
        return result;
    }

    void Install(const int& selected)
    {
        auto hud = hook::pattern("53 56 57 6A 17 E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B D8 68 ? ? ? ? 53 E8 ? ? ? ? 53 E8 ? ? ? ? E8");
        auto stream = hook::pattern("8B 44 24 08 81 EC 0C 04 00 00 56 50 E8 ? ? ? ? 83 C4 04 8D 4C 24 04 8B F0 E8 ? ? ? ? 56 8D 4C 24 08 E8");
        auto texture = hook::pattern("8B 0D ? ? ? ? 85 C9 56 57 8B 7C 24 0C 74 0C 57 E8 ? ? ? ? 8B F0 85 F6 75 0F 57");
        auto branch = [](void* site) { return injector::GetBranchDestination(site).as_int(); };
        auto start = stream.get_first<uint8_t>();
        CreateStore = reinterpret_cast<decltype(CreateStore)>(branch(hud.get_first(15)));
        AddRef = reinterpret_cast<decltype(AddRef)>(branch(hud.get_first(34)));
        ConstructStream = reinterpret_cast<decltype(ConstructStream)>(branch(start + 26));
        ReadStream = reinterpret_cast<decltype(ReadStream)>(branch(start + 36));
        DestroyStream = reinterpret_cast<decltype(DestroyStream)>(branch(start + 49));
        LoadStore = reinterpret_cast<decltype(LoadStore)>(branch(start + 75));
        LoadTexture = reinterpret_cast<decltype(LoadTexture)>(texture.get_first());
        Selected = &selected;
        HudLoadHook = safetyhook::create_inline(hud.get_first(), HudLoad);
    }
}
