#pragma once

namespace NativeMenu { int Language(); }
namespace PadIcons
{
using Button = console::padbuttons::Button;
struct Name { const char* name; Button button; };
inline constexpr Name names[]
{
    { "Tri", Button::Triangle }, { "Circ", Button::Circle }, { "Sq", Button::Square },
    { "X", Button::Cross }, { "L1", Button::L1 }, { "R1", Button::R1 },
    { "Select", Button::Select }, { "Start", Button::Start },
    { "dLeft", Button::Left }, { "dRight", Button::Right },
    { "dUp", Button::Up }, { "dDown", Button::Down }, { "Stick", Button::LeftStick },
};
inline constexpr unsigned buttonCount = sizeof(names) / sizeof(names[0]);
struct Vertex { uint16_t u, v; float x, y, z; };
static_assert(sizeof(Vertex) == 16);
struct Texture
{
    bool ready;
    int left, top, right, bottom;
    alignas(16) uint8_t native[48];
    alignas(16) uint32_t pixels[32 * 32];
};
struct Shape
{
    bool ready;
    alignas(16) uint32_t native[32];
    alignas(16) Vertex vertices[2];
};
struct Entry
{
    uintptr_t definition;
    unsigned button;
    int16_t bounds[4];
    Shape shapes[2];
};
struct MovieCache
{
    uint32_t hash;
    uintptr_t resource, root;
    unsigned count, frame;
    Entry entries[buttonCount];
};
inline MovieCache movies[16];
inline MovieCache* current;
inline unsigned replacement;
inline Texture textures[2][buttonCount];
inline int style;
inline SafetyMipsInline drawHook;
inline injector::hook_back<int(uintptr_t, uintptr_t, uintptr_t, uintptr_t, unsigned, const float*, const int16_t*, unsigned)> draw;
inline void ArtworkUnavailable()
{
    static bool reported;
    if (reported) return;
    reported = true;
    static constexpr const char* messages[] = {
        "Fusion Fix: button artwork unavailable; using native icons.\n",
        "Fusion Fix : image de bouton indisponible ; icones d'origine utilisees.\n",
        "Fusion Fix: Tastensymbol nicht verfuegbar; Originalsymbole werden verwendet.\n",
        "Fusion Fix: immagine pulsante non disponibile; uso icone originali.\n",
        "Fusion Fix: imagen de boton no disponible; se usan iconos originales.\n"
    };
    PatchNotice(messages[NativeMenu::Language()]);
}

inline uint32_t Pixel(const uint8_t* artwork, int x, int y)
{
    if (x < 0 || y < 0 || x >= 64 || y >= 64)
        return 0;
    const unsigned index = artwork[y * 64 + x];
    const unsigned swizzled = (index & ~0x18u) | ((index & 8) << 1) | ((index & 16) >> 1);
    uint32_t pixel;
    std::memcpy(&pixel, artwork + 4096 + swizzled * 4, 4);
    return (pixel & 0xFFFFFF) | (std::min(unsigned(pixel >> 24) * 255u / 128u, 255u) << 24);
}
inline Texture* PrepareTexture(unsigned family, unsigned button)
{
    auto& item = textures[family][button];
    if (item.ready)
        return &item;
    const auto artwork = console::padbuttons::artwork[family][unsigned(names[button].button)];
    item.left = item.top = 64;
    item.right = item.bottom = 0;
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            if (Pixel(artwork, x, y) >> 24)
            {
                item.left = std::min(item.left, x);
                item.top = std::min(item.top, y);
                item.right = std::max(item.right, x + 1);
                item.bottom = std::max(item.bottom, y + 1);
            }
    if (item.left >= item.right || item.top >= item.bottom)
    {
        ArtworkUnavailable();
        return nullptr;
    }
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x)
        {
            unsigned alpha{}, rgb[3]{};
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx)
                {
                    const uint32_t pixel = Pixel(artwork, x * 2 + dx, y * 2 + dy);
                    const unsigned a = pixel >> 24;
                    alpha += a;
                    for (unsigned c = 0; c < 3; ++c)
                        rgb[c] += ((pixel >> (c * 8)) & 255) * a;
                }
            uint32_t value = ((alpha + 2) / 4) << 24;
            if (alpha)
                for (unsigned c = 0; c < 3; ++c)
                    value |= ((rgb[c] + alpha / 2) / alpha) << (c * 8);
            item.pixels[y * 32 + x] = value;
        }
    *reinterpret_cast<uintptr_t*>(item.native + 12) = uintptr_t(item.pixels);
    *reinterpret_cast<uint32_t*>(item.native + 16) = GU_PSM_8888;
    for (unsigned offset : { 20u, 22u, 24u })
        *reinterpret_cast<uint16_t*>(item.native + offset) = 32;
    item.native[37] = 8;
    item.native[40] = item.native[41] = 1;
    *reinterpret_cast<uint16_t*>(item.native + 42) = 0x10;
    sceKernelDcacheWritebackRange(&item, sizeof(item));
    item.ready = true;
    return &item;
}
inline Shape* PrepareShape(Entry& entry, unsigned family)
{
    auto& shape = entry.shapes[family];
    if (shape.ready)
        return &shape;
    const auto* texture = PrepareTexture(family, entry.button);
    if (!texture) return nullptr;
    const float width = float(entry.bounds[2] - entry.bounds[0]);
    const float height = float(entry.bounds[3] - entry.bounds[1]);
    const float scale = std::min(width / float(texture->right - texture->left), height / float(texture->bottom - texture->top));
    const float left = float(entry.bounds[0] + entry.bounds[2]) * 0.5f - (texture->left + texture->right) * scale * 0.5f;
    const float top = float(entry.bounds[1] + entry.bounds[3]) * 0.5f - (texture->top + texture->bottom) * scale * 0.5f;
    // Native sprites use 16-bit texture coordinates where 32768 is the full texture.
    shape.vertices[0] = { 0, 0, left, top, 0 };
    shape.vertices[1] = { 32768, 32768, left + 64 * scale, top + 64 * scale, 0 };
    shape.native[0] = uintptr_t(texture->native);
    shape.native[1] = 2;
    shape.native[2] = uintptr_t(shape.vertices);
    shape.native[28] = 1;
    sceKernelDcacheWritebackRange(&shape, sizeof(shape));
    shape.ready = true;
    return &shape;
}
inline MovieCache* Find(uint32_t hash, uintptr_t movie)
{
    const auto resource = *reinterpret_cast<const uintptr_t*>(movie + 3152);
    const auto root = *reinterpret_cast<const uintptr_t*>(movie + 3164);
    for (auto& entry : movies)
        if (entry.hash == hash && entry.resource == resource && entry.root == root)
            return &entry;
    return nullptr;
}
inline void Before(uint32_t hash, uintptr_t movie)
{
    current = Find(hash, movie);
}
inline void After(uint32_t hash, uintptr_t movie)
{
    const auto root = *reinterpret_cast<const uintptr_t*>(movie + 3164);
    if (!root)
        return;
    const auto frame = *reinterpret_cast<const unsigned*>(root + 16);
    auto found = Find(hash, movie);
    if (found && (found->count == buttonCount || found->frame == frame))
        return;
    if (!found)
    {
        found = &movies[replacement++ % (sizeof(movies) / sizeof(movies[0]))];
        *found = {};
        found->hash = hash;
        found->root = root;
        found->resource = *reinterpret_cast<const uintptr_t*>(movie + 3152);
    }
    auto& item = *found;
    item.frame = frame;
    const auto definition = *reinterpret_cast<const uintptr_t*>(root + 28);
    const auto count = *reinterpret_cast<const uint16_t*>(definition + 2);
    const auto slots = *reinterpret_cast<const uintptr_t*>(root + 40);
    const auto type = *reinterpret_cast<const uintptr_t*>(root + 4);
    for (unsigned i = 0; i < count; ++i)
    {
        const auto clip = *reinterpret_cast<const uintptr_t*>(slots + i * 8);
        if (!clip || *reinterpret_cast<const uintptr_t*>(clip + 4) != type)
            continue;
        const auto name = *reinterpret_cast<const char* const*>(clip + 8);
        if (!name)
            continue;
        for (unsigned button = 0; button < buttonCount; ++button)
            if (!std::strcmp(name, names[button].name))
            {
                bool known = false;
                for (unsigned j = 0; j < item.count; ++j)
                    known |= item.entries[j].button == button;
                if (known)
                    break;
                if (item.count == buttonCount)
                {
                    ArtworkUnavailable();
                    break;
                }
                auto& entry = item.entries[item.count];
                entry.definition = *reinterpret_cast<const uintptr_t*>(clip + 28);
                entry.button = button;
                std::memcpy(entry.bounds, reinterpret_cast<const void*>(entry.definition + 6), sizeof(entry.bounds));
                if (entry.bounds[0] >= entry.bounds[2] || entry.bounds[1] >= entry.bounds[3])
                {
                    ArtworkUnavailable();
                    break;
                }
                ++item.count;
                break;
            }
    }
}
inline int Draw(uintptr_t definition, uintptr_t clip, uintptr_t slots, uintptr_t movie, unsigned depth,
    const float* matrix, const int16_t* color, unsigned frame)
{
    if (style != 2 && current)
        for (unsigned i = 0; i < current->count; ++i)
        {
            auto& entry = current->entries[i];
            if (entry.definition != definition)
                continue;
            auto* shape = PrepareShape(entry, style == 3 ? 1 : 0);
            if (!shape) break;
            const uint32_t alpha = uint32_t(std::clamp(int(color[3]), 0, 256)) * 255 / 256;
            const uint32_t colors[3]{ 0xFFFFFF | (alpha << 24), 0, 0 };
            // Some prompts scale the native icon clip unevenly; keep the
            // replacement artwork round, centred in the native bounds.
            const float width = std::hypot(matrix[0], matrix[1]) * Geometry::AspectRatio() / (480.0f / 272.0f);
            const float height = std::hypot(matrix[4], matrix[5]);
            alignas(16) float even[16];
            std::memcpy(even, matrix, sizeof(even));
            if (width > 0.0f && height > 0.0f && std::fabs(width - height) > 0.01f * height)
            {
                const unsigned axis = width > height ? 0 : 4;
                const float factor = width > height ? height / width : width / height;
                const float center = (entry.bounds[axis ? 1 : 0] + entry.bounds[axis ? 3 : 2]) * 0.5f;
                for (unsigned row = 0; row < 4; ++row)
                {
                    even[12 + row] += even[axis + row] * center * (1.0f - factor);
                    even[axis + row] *= factor;
                }
            }
            return Graphics::Leaf(Hud::renderer, uintptr_t(shape->native), even, colors);
        }
    return draw.fun(definition, clip, slots, movie, depth, matrix, color, frame);
}
inline void ReadOptions()
{
    style = std::clamp(inireader.ReadInteger("MAIN", "GamepadIcons", 0), 0, 3);
}
inline void Install()
{
    const auto site = sites::Required("60 FE BD 27 7C 01 B4 AF 50 0C E2 8C FF FF 74 31 FF FF 08 31 02 00 8B 94 24 01 A8 AF 00 00 03 34");
    drawHook = safetymips::create_inline(site, Draw);
    draw.fun = drawHook.original<decltype(draw.fun)>();
    Hud::beforeMovie = Before;
}
}
