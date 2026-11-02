#pragma once

namespace Hud
{
enum class Role { Frontend, LeftBottom, RightBottom, RightTop, RightCenter, CenterTop, CenterBottom, Center, World, Scope };
constexpr uint32_t Hash(const char* text)
{
    uint32_t value = 0xFFFFFFFF;
    for (; *text; ++text)
    {
        value ^= uint8_t(*text);
        for (int bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ ((value & 1) ? 0xEDB88320 : 0);
    }
    return ~value;
}
struct Profile { uint32_t hash; Role role; };
inline constexpr Profile profiles[]
{
    { Hash("weaponinfobox.swf"), Role::LeftBottom },
    { Hash("weaponselection.swf"), Role::Center },
    { Hash("spinfobox.swf"), Role::RightBottom },
    { Hash("mpinfobox.swf"), Role::LeftBottom },
    { Hash("mpscores.swf"), Role::RightTop },
    { Hash("freelook.swf"), Role::RightCenter },
    { Hash("timers.swf"), Role::CenterTop },
    { Hash("messagewindow.swf"), Role::CenterTop },
    { Hash("gameupdatewindow.swf"), Role::CenterBottom },
    { Hash("tutorial.swf"), Role::CenterBottom },
    { Hash("subtitle.swf"), Role::CenterBottom },
    { Hash("actions.swf"), Role::CenterBottom },
    { Hash("tcm.swf"), Role::Center },
    { Hash("scopes.swf"), Role::Scope },
    { Hash("autotarget.swf"), Role::World },
    { Hash("navpoints.swf"), Role::World },
};
inline float scale{ 1.0f }, constraint{};
inline uintptr_t activeManager{}, activeMovie{};
inline Role role{ Role::Frontend };
inline uint32_t reported[48];
inline unsigned reportedCount;
inline uintptr_t renderer;
inline bool scopeDrawn;
inline void (*beforeMovie)(uint32_t, uintptr_t);
inline void (*afterMovie)(uint32_t, uintptr_t);
inline void (*afterFrame)();
inline SafetyMipsInline frameHook, movieHook, sceneHook;
inline injector::hook_back<int(uintptr_t, uintptr_t, uintptr_t)> scene;
inline SafetyMipsMid worldHook;
inline injector::hook_back<int(uintptr_t, float)> frame, movie;

inline Role Classify(uint32_t hash)
{
    for (const auto& profile : profiles)
        if (profile.hash == hash)
            return profile.role;
    return Role::Frontend;
}
// Canvas point (in pixels) that stays at its place relative to the HUD box.
inline console::Transform Anchored(float x, float y)
{
    const float aspect = Geometry::AspectRatio();
    const float edge = console::hud_edge(x, 480, aspect, constraint);
    return console::Transform::anchored(480.0f / 272.0f, aspect, scale, x * 20, y * 20, edge * 20);
}
inline console::Transform TransformFor(Role value)
{
    const float aspect = Geometry::AspectRatio();
    constexpr float native = 480.0f / 272.0f;
    if (value == Role::World)
        return {};
    if (value == Role::Frontend || value == Role::Scope)
    {
        const float sx = std::min(native / aspect, 1.0f);
        const float sy = std::min(aspect / native, 1.0f);
        return { sx, sy, 4800 * (1 - sx), 2720 * (1 - sy) };
    }
    const bool left = value == Role::LeftBottom;
    const bool right = value == Role::RightTop || value == Role::RightBottom || value == Role::RightCenter;
    const float x = left ? 0.0f : right ? 480.0f : 240.0f;
    const bool bottom = value == Role::LeftBottom || value == Role::RightBottom || value == Role::CenterBottom;
    const float y = bottom ? 272.0f : value == Role::CenterTop || value == Role::RightTop ? 0.0f : 136.0f;
    return Anchored(x, y);
}
inline void Apply(float* matrix, const console::Transform& transform)
{
    for (int column = 0; column < 4; ++column)
    {
        const int i = column * 4;
        matrix[i] = matrix[i] * transform.scaleX + matrix[i + 3] * transform.offsetX;
        matrix[i + 1] = matrix[i + 1] * transform.scaleY + matrix[i + 3] * transform.offsetY;
    }
}
inline int Frame(uintptr_t object, float delta)
{
    const auto previous = activeManager;
    activeManager = object;
    const int result = frame.fun(object, delta);
    if (afterFrame)
        afterFrame();
    activeManager = previous;
    return result;
}
inline int Movie(uintptr_t object, float delta)
{
    uint32_t hash{};
    const auto previousMovie = activeMovie;
    const auto previousRole = role;
    if (activeManager)
    {
        const auto end = *reinterpret_cast<const uintptr_t*>(activeManager + 968);
        for (auto entry = *reinterpret_cast<const uintptr_t*>(activeManager + 964); entry != end; entry += 16)
            if (*reinterpret_cast<const uintptr_t*>(entry + 4) == object)
            {
                hash = *reinterpret_cast<const uint32_t*>(entry);
                break;
            }
    }
    activeMovie = object;
    role = Classify(hash);
    if (beforeMovie)
        beforeMovie(hash, object);
    bool known = false;
    for (unsigned i = 0; i < reportedCount; ++i)
        known |= reported[i] == hash;
    if (!known && reportedCount < 48)
    {
        reported[reportedCount++] = hash;
        logger.WriteF("Native Flash canvas %08X, layout %u.\n", unsigned(hash), unsigned(role));
    }
    alignas(16) float original[16];
    auto matrix = reinterpret_cast<float*>(object);
    std::memcpy(original, matrix, sizeof(original));
    Apply(matrix, TransformFor(role));
    const int result = movie.fun(object, delta);
    const auto root = *reinterpret_cast<const uintptr_t*>(object + 3164);
    if (role == Role::Scope && root && *reinterpret_cast<const uint8_t*>(root + 52) &&
        !*reinterpret_cast<const uint32_t*>(object + 3252))
        scopeDrawn = true;
    if (afterMovie)
        afterMovie(hash, object);
    std::memcpy(matrix, original, sizeof(original));
    role = previousRole;
    activeMovie = previousMovie;
    return result;
}
inline void World(SafetyMipsContext& regs)
{
    if (role != Role::World || !activeMovie)
        return;
    const auto root = *reinterpret_cast<const uintptr_t*>(activeMovie + 3164);
    const auto clip = uintptr_t(regs.s3);
    if (*reinterpret_cast<const uintptr_t*>(clip) != root)
        return;
    auto matrix = reinterpret_cast<float*>(uintptr_t(regs.s4));
    const float sx = (480.0f / 272.0f) / Geometry::AspectRatio() * scale;
    for (int i = 0; i < 4; ++i)
    {
        matrix[i] *= sx;
        matrix[i + 4] *= scale;
    }
}
// The compass, its markers and the screen fader are 3D nodes on a plane that
// the root holds in front of the camera, facing it (x right, y up). Side
// nodes follow the HUD box like the Flash groups; a fader covering the whole
// plane is widened with the view.
inline int Scene(uintptr_t renderer, uintptr_t root, uintptr_t argument)
{
    const auto camera = *reinterpret_cast<const uintptr_t*>(renderer + 11628);
    if (!root || !camera)
        return scene.fun(renderer, root, argument);
    constexpr float native = 480.0f / 272.0f;
    const float aspect = Geometry::AspectRatio();
    const auto view = reinterpret_cast<const float*>(camera + 16);
    const auto plane = reinterpret_cast<const float*>(root + 16);
    float distance{};
    for (unsigned i = 0; i < 3; ++i)
        distance -= (plane[12 + i] - view[12 + i]) * view[8 + i];
    // Half height of the plane in view; the HUD pass uses the native angle.
    const float height = distance * std::tan(*reinterpret_cast<const float*>(camera + 416) * (272.0f / 480.0f));
    const float width = height * native;
    if (!(height > 0.0f))
        return scene.fun(renderer, root, argument);
    const auto begin = *reinterpret_cast<uintptr_t* const*>(root + 80);
    const auto end = *reinterpret_cast<uintptr_t* const*>(root + 84);
    alignas(16) float saved[16][16];
    unsigned count{};
    for (auto child = begin; child && child != end && count < 16; ++child, ++count)
    {
        auto matrix = reinterpret_cast<float*>(*child + 16);
        std::memcpy(saved[count], matrix, sizeof(saved[count]));
        const auto box = reinterpret_cast<const float*>(*child + 180);
        const bool empty = box[0] == box[3] && box[1] == box[4];
        const float bx = empty ? 0.0f : (box[0] + box[3]) * 0.5f;
        const float by = empty ? 0.0f : (box[1] + box[4]) * 0.5f;
        const float cx = (matrix[0] * bx + matrix[4] * by + matrix[12]) / width;
        const float cy = (matrix[1] * bx + matrix[5] * by + matrix[13]) / height;
        if (!empty && box[3] - box[0] >= width * 1.6f && box[4] - box[1] >= height * 1.6f && std::fabs(cx) < 0.1f && std::fabs(cy) < 0.1f)
        {
            const float widen = std::max(aspect / native, 1.0f);
            for (unsigned column = 0; column < 4; ++column)
                matrix[column * 4] *= widen;
            continue;
        }
        if (std::fabs(cx) < 0.25f)
            continue;
        // Native canvas point of the node, placed like a Flash HUD group.
        const auto transform = Anchored(cx > 0 ? 480.0f : 0.0f, cy > 0 ? 0.0f : 272.0f);
        const float x = (transform.x((cx + 1) * 4800) / 4800 - 1) * width * aspect / native;
        const float y = (1 - transform.y((1 - cy) * 2720) / 2720) * height;
        const float size = transform.scaleY;
        for (unsigned column = 0; column < 3; ++column)
        {
            matrix[column * 4] *= size;
            matrix[column * 4 + 1] *= size;
        }
        matrix[12] = (matrix[12] - cx * width) * size + x;
        matrix[13] = (matrix[13] - cy * height) * size + y;
    }
    const int result = scene.fun(renderer, root, argument);
    for (unsigned i = 0; i < count; ++i)
        std::memcpy(reinterpret_cast<float*>(begin[i] + 16), saved[i], sizeof(saved[i]));
    return result;
}
inline void ReadOptions()
{
    const float size = inireader.ReadFloat("MAIN", "HudScale", 1.0f);
    scale = std::clamp(size, 0.5f, 1.5f);
    char box[64]{};
    constraint = console::parse_hud_aspect(inireader.ReadString("MAIN", "HudAspectRatioConstraint", "Auto", box, sizeof(box)));
}
inline void Install()
{
    const auto frameSite = sites::Required("C0 FE BD 27 28 01 B7 AF ? ? 17 3C 20 01 B5 AF 25 A8 80 00 ? ? F7 26 04 01 B4 E7 06 65 00 46");
    const auto movieSite = sites::Required("C0 FF BD 27 5C 0C 85 8C 20 00 B4 E7 06 65 00 46 24 00 B0 AF 28 00 B1 AF 2C 00 B2 AF 30 00 B3 AF");
    const auto worldSite = sites::Required("20 00 B0 27 9C 01 A6 8F 25 20 80 02 ? ? ? ? 25 28 00 02 00 00 00 DA 10 00 01 DA 20 00 02 DA");
    renderer = Absolute(frameSite, 8, 20);
    frameHook = safetymips::create_inline(frameSite, Frame);
    frame.fun = frameHook.original<decltype(frame.fun)>();
    movieHook = safetymips::create_inline(movieSite, Movie);
    movie.fun = movieHook.original<decltype(movie.fun)>();
    const auto sceneSite = sites::Required("80 FF BD 27 64 00 B1 AF 25 88 80 00 60 00 B0 AF 68 00 B2 AF 6C 00 B3 AF 70 00 B4 AF 74 00 B5 AF 78 00 BF AF 44 00 C0 10");
    sceneHook = safetymips::create_inline(sceneSite, Scene);
    scene.fun = sceneHook.original<decltype(scene.fun)>();
    safetymips::Options scalar;
    scalar.preserve = PSP_HOOK_SAVE_FPU;
    worldHook = safetymips::create_mid(worldSite, [](SafetyMipsContext& regs)
    {
        World(regs);
    }, scalar);
}
}
