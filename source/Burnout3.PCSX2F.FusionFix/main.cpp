#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/safetymips.hpp"
#include "../Shared/Console/PS2Display.hpp"
#include "../Shared/Console/Input.hpp"
#include "../Shared/Console/PS2Settings.hpp"
#include "../Shared/Console/Ps2PluginConflict.hpp"
#include "MovieFrame.hpp"
#include "CameraLook.hpp"
#include "NativeMenu.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
PCSX2F_CFG_SETTINGS()
PCSX2F_CFG_PEER_SETTINGS()
PCSX2F_STATE_LOAD_GENERATION()
extern "C"
{
    int CompatibleCRCList[] = {static_cast<int>(0xBEBF8793)};
    int CompatibleElfCRCList[] = {static_cast<int>(0xBEBF8793)};
    int PCSX2Data[PCSX2Data_Size] = {1};
    char OSDText[OSDStringNum][OSDStringSize] = {{1}};
    char FrameLimitUnthrottle = 0;
    CMouseControllerState MouseState[StateNum] = {{1}};
}

namespace
{
console::ps2settings::Value options[] = {
    {"MAIN", "ForceAspectRatio", "Auto"}, {"MAIN", "HudAspectRatioConstraint", "Auto"}, {"MAIN", "FOVFactor", "1.0"},
    {"MAIN", "SkipIntro", "1"}, {"MAIN", "HudScale", "1.0"},
    {"MAIN", "FMVWidescreenMode", "1"}, {"MAIN", "MouseSensitivity", "1.0"},
    {"MAIN", "InvertMouseY", "0"}, {"MAIN", "GamepadCameraSpeed", "1.0"},
    {"MAIN", "InvertGamepadY", "0"}, {"MAIN", "SplitScreenSideBySide", "0"},
    {"MAIN", "FPSLimit", "60"}};
uint32_t lastStateLoad;
bool fps60Applied;
uint32_t nativeFrameInterval = 1;
pcsx2::GameFunction<void(int)> setFrameInterval;
pcsx2::GameFunction<void(uintptr_t)> updateTimer;
bool skipIntro;
uint32_t refreshFrames;
uint32_t menuNoticeFrames;
float forcedAspect, hudConstraint, fovFactor, hudScaleFactor, mouseSensitivity, gamepadCameraSpeed;
bool invertMouseY, invertGamepadY, sideBySide;
burnout::PadOrbit padOrbit[2];
uint32_t padOrbitFrame[2] = {~0u, ~0u};
console::Input mouseInput;
char noKeyboard[StateSize]{};
float mouseYaw, mousePitch;
unsigned mouseHold;
int movieMode;
void ReadSettings()
{
    forcedAspect = console::parse_forced_aspect(options[0].value);
    hudConstraint = console::parse_hud_aspect(options[1].value);
    char* end = nullptr;
    float factor = std::strtof(options[2].value, &end);
    if (end == options[2].value || !std::isfinite(factor)) factor = 1.0f;
    fovFactor = std::clamp(factor, 0.5f, 2.0f);
    end = nullptr;
    factor = std::strtof(options[4].value, &end);
    if (end == options[4].value || !std::isfinite(factor)) factor = 1.0f;
    hudScaleFactor = std::clamp(factor, 0.5f, 1.5f);
    movieMode = std::clamp(std::atoi(options[5].value), 0, 1);
    end = nullptr;
    factor = std::strtof(options[6].value, &end);
    if (end == options[6].value || !std::isfinite(factor)) factor = 1.0f;
    mouseSensitivity = std::clamp(factor, 0.1f, 5.0f);
    invertMouseY = console::ps2settings::boolean(options[7], false);
    end = nullptr;
    factor = std::strtof(options[8].value, &end);
    if (end == options[8].value || !std::isfinite(factor)) factor = 1.0f;
    gamepadCameraSpeed = std::clamp(factor, 0.25f, 4.0f);
    invertGamepadY = console::ps2settings::boolean(options[9], false);
    sideBySide = console::ps2settings::boolean(options[10], false);
}
struct Layout
{
    float aspect = 0, width = 640, hudScale = 1, offset = 0;
    float hudOrigin = 0, normalizedOrigin = 0, menuWidth = 640, rightAnchor = 1, leftAnchor = 0;
    int players = 0;
    bool sideSplit = false;
} layout;
std::array<SafetyMipsMid, 47> hooks;
size_t hookCount;
pcsx2::GameCallback<void()> hudReset1, hudReset2;
pcsx2::GameFunction<void(float*, float*, int)> hudAnchor;
pcsx2::GameFunction<void(uintptr_t)> rebuildSplit;
pcsx2::GameFunction<void(uintptr_t, int)> setupSplitHud;
safetymips::GameInline<void(uintptr_t)> hudTransform;
safetymips::GameInline<uint64_t(uintptr_t)> hudRender;
float appliedLeft, appliedRight = 1.0f;
bool anchorsInitialized;
struct MessagePlacement { uintptr_t object = 0, parent = 0; bool left = false; };
MessagePlacement messagePlacement[2];

void SetAnchors(float left, float right)
{
    for (uintptr_t address : {0x4B7688u, 0x4B7678u})
        injector::WriteMemory<float>(address, right);
    for (uintptr_t address : {0x4B7658u, 0x4B7668u})
        injector::WriteMemory<float>(address, left);
}

struct HudNode
{
    uintptr_t object;
    float* node;
    float* viewport;
    int anchor;
    float oldPosition[2];
};

constexpr uintptr_t hudManager = 0x66A3D0;
bool CollectHudNodes(std::array<HudNode, 128>& nodes, size_t& count)
{
    const int players = *reinterpret_cast<const int*>(hudManager + 368);
    count = 0;
    bool valid = players >= 0 && players <= 2;
    for (int player = 0; valid && player < players; ++player)
    {
        const uintptr_t viewport = hudManager + 112 * player;
        uintptr_t object = *reinterpret_cast<const uint32_t*>(viewport + 104);
        while (object)
        {
            if (count == nodes.size())
            {
                valid = false;
                break;
            }
            const uintptr_t node = *reinterpret_cast<const uint32_t*>(object + 8);
            const int anchor = *reinterpret_cast<const int*>(object + 24);
            if (*reinterpret_cast<const uint32_t*>(object + 4) != hudManager ||
                *reinterpret_cast<const int8_t*>(object + 28) != player ||
                !node || anchor < 0 || anchor > 8)
            {
                valid = false;
                break;
            }
            nodes[count++] = {object, reinterpret_cast<float*>(node), reinterpret_cast<float*>(viewport), anchor, {0, 0}};
            object = *reinterpret_cast<const uint32_t*>(object + 12);
        }
    }
    return valid;
}

bool ReanchorHud(float left, float right, bool rebuildViewport = false)
{
    if (!anchorsInitialized)
    {
        appliedLeft = *reinterpret_cast<const float*>(0x4B7658);
        appliedRight = *reinterpret_cast<const float*>(0x4B7688);
        anchorsInitialized = true;
    }
    if (!rebuildViewport && left == appliedLeft && right == appliedRight)
        return true;
    // Collect and validate the game-owned lists before changing any node.
    std::array<HudNode, 128> nodes{};
    size_t count;
    if (!CollectHudNodes(nodes, count))
        return false;

    // Message drawables have separate storage and native animation centers.
    // Validate every active left-anchored group before changing the tables.
    struct MovingMessage { uintptr_t object; float* parent; float* children[3]; float oldAnchor[2]; float newAnchor[2]; };
    std::array<MovingMessage, 2> moving{};
    size_t movingCount = 0;
    for (size_t i = 0; i < count; ++i)
    {
        if (nodes[i].anchor != 8 ||
            *reinterpret_cast<const uint32_t*>(nodes[i].object) != 0x4DFD10) continue;
        const int player = *reinterpret_cast<const int8_t*>(nodes[i].object + 28);
        const auto& placement = messagePlacement[player];
        if (placement.object != nodes[i].object || placement.parent != uintptr_t(nodes[i].node) || !placement.left) continue;
        float* parent = nodes[i].node;
        if (!std::isfinite(parent[2]) || !std::isfinite(parent[3]) ||
            std::fabs(parent[2]) < 0.0001f || std::fabs(parent[3]) < 0.0001f || movingCount == moving.size()) return false;
        MovingMessage item{nodes[i].object, parent, {nullptr, nullptr, nullptr}, {0, 0}, {0, 0}};
        bool active = false;
        for (unsigned j = 0; j < 3; ++j)
        {
            const uintptr_t p = *reinterpret_cast<const uint32_t*>(item.object + 76 + 4 * j);
            if (!p) continue;
            item.children[j] = reinterpret_cast<float*>(p);
            if (!std::isfinite(item.children[j][0]) || !std::isfinite(item.children[j][1])) return false;
            for (unsigned k = 0; k < j; ++k) if (item.children[k] == item.children[j]) return false;
            active = true;
        }
        if (!active) continue;
        for (uintptr_t offset : {88u, 92u, 104u, 108u})
            if (!std::isfinite(*reinterpret_cast<const float*>(item.object + offset))) return false;
        moving[movingCount++] = item;
    }
    SetAnchors(appliedLeft, appliedRight);
    for (size_t i = 0; i < count; ++i)
        hudAnchor(nodes[i].oldPosition, nodes[i].viewport, nodes[i].anchor);
    for (size_t i = 0; i < movingCount; ++i)
    {
        const int player = *reinterpret_cast<const int8_t*>(moving[i].object + 28);
        hudAnchor(moving[i].oldAnchor, reinterpret_cast<float*>(hudManager + 112 * player), 1);
    }
    if (rebuildViewport)
    {
        *reinterpret_cast<uint8_t*>(0x665ECF) = sideBySide ? 1 : 0;
        rebuildSplit(0x665EC0);
        setupSplitHud(hudManager, 2);
    }
    SetAnchors(left, right);
    for (size_t i = 0; i < movingCount; ++i)
    {
        auto& message = moving[i];
        const int player = *reinterpret_cast<const int8_t*>(message.object + 28);
        hudAnchor(message.newAnchor, reinterpret_cast<float*>(hudManager + 112 * player), 1);
        const float dx = (message.newAnchor[0] - message.oldAnchor[0]) / message.parent[2];
        const float dy = (message.newAnchor[1] - message.oldAnchor[1]) / message.parent[3];
        for (float* child : message.children)
            if (child) { child[0] += dx; child[1] += dy; }
        for (uintptr_t offset : {88u, 104u}) *reinterpret_cast<float*>(message.object + offset) += dx;
        for (uintptr_t offset : {92u, 108u}) *reinterpret_cast<float*>(message.object + offset) += dy;
    }
    for (size_t i = 0; i < count; ++i)
    {
        float position[2];
        hudAnchor(position, nodes[i].viewport, nodes[i].anchor);
        nodes[i].node[0] += position[0] - nodes[i].oldPosition[0];
        nodes[i].node[1] += position[1] - nodes[i].oldPosition[1];
    }
    appliedLeft = left;
    appliedRight = right;
    return true;
}

void BuildHudTransforms(uintptr_t tree)
{
    if (hudScaleFactor == 1.0f || (tree != hudManager + 20 && tree != hudManager + 132))
    {
        hudTransform.call(tree);
        return;
    }
    std::array<HudNode, 128> nodes{};
    size_t count;
    if (!CollectHudNodes(nodes, count))
    {
        hudTransform.call(tree);
        return;
    }
    for (size_t i = 0; i < count; ++i)
    {
        if (reinterpret_cast<uintptr_t>(nodes[i].viewport) + 20 != tree || nodes[i].anchor == 8)
            continue;
        nodes[i].oldPosition[0] = nodes[i].node[2];
        nodes[i].oldPosition[1] = nodes[i].node[3];
        nodes[i].node[2] *= hudScaleFactor;
        nodes[i].node[3] *= hudScaleFactor;
    }
    hudTransform.call(tree);
    for (size_t i = 0; i < count; ++i)
    {
        if (reinterpret_cast<uintptr_t>(nodes[i].viewport) + 20 != tree || nodes[i].anchor == 8)
            continue;
        nodes[i].node[2] = nodes[i].oldPosition[0];
        nodes[i].node[3] = nodes[i].oldPosition[1];
    }
}

// Message drawables live outside the transform-node list and are read after
// the transform pass. Modify their payload only around the native draw call.
uint64_t RenderHudTree(uintptr_t tree)
{
    if (hudScaleFactor == 1.0f || (tree != hudManager + 20 && tree != hudManager + 132))
        return hudRender.call(tree);
    std::array<HudNode, 128> nodes{};
    size_t count;
    if (!CollectHudNodes(nodes, count))
        return hudRender.call(tree);
    struct MessageNode { float* node; float saved[4]; };
    std::array<MessageNode, 6> messages{};
    size_t messageCount = 0;
    for (size_t i = 0; i < count; ++i)
    {
        if (reinterpret_cast<uintptr_t>(nodes[i].viewport) + 20 != tree || nodes[i].anchor != 8 ||
            *reinterpret_cast<const uint32_t*>(nodes[i].object) != 0x4DFD10)
            continue;
        // This native race-message object owns up to three independent nodes
        // in the viewport tree. Scale them together so the animated icon and
        // its text keep their relative placement.
        float minX = 1.0e9f, minY = 1.0e9f, maxX = -1.0e9f, maxY = -1.0e9f;
        const size_t first = messageCount;
        for (uintptr_t offset : {76u, 80u, 84u})
        {
            const uintptr_t p = *reinterpret_cast<const uint32_t*>(nodes[i].object + offset);
            if (!p) continue;
            if (messageCount == messages.size())
            {
                messageCount = first;
                break;
            }
            auto* rect = reinterpret_cast<float*>(p);
            bool duplicate = false;
            for (size_t j = 0; j < messageCount; ++j)
                duplicate = duplicate || messages[j].node == rect;
            if (duplicate)
            {
                messageCount = first;
                break;
            }
            if (!std::isfinite(rect[0]) || !std::isfinite(rect[1]) || !std::isfinite(rect[2]) || !std::isfinite(rect[3]) ||
                std::fabs(rect[0]) > 8192 || std::fabs(rect[1]) > 8192 ||
                rect[2] < 0 || rect[3] < 0 || rect[2] > 2048 || rect[3] > 2048)
            {
                messageCount = first;
                break;
            }
            messages[messageCount++] = {rect, {rect[0], rect[1], rect[2], rect[3]}};
            minX = std::min(minX, rect[0]); minY = std::min(minY, rect[1]);
            maxX = std::max(maxX, rect[0] + rect[2]); maxY = std::max(maxY, rect[1] + rect[3]);
        }
        const float centerX = (minX + maxX) * 0.5f, centerY = (minY + maxY) * 0.5f;
        for (size_t j = first; j < messageCount; ++j)
        {
            float* rect = messages[j].node;
            rect[0] = centerX + (rect[0] - centerX) * hudScaleFactor;
            rect[1] = centerY + (rect[1] - centerY) * hudScaleFactor;
            rect[2] *= hudScaleFactor; rect[3] *= hudScaleFactor;
        }
    }
    const uint64_t result = hudRender.call(tree);
    for (size_t i = 0; i < messageCount; ++i)
        for (int j = 0; j < 4; ++j) messages[i].node[j] = messages[i].saved[j];
    return result;
}

void UpdateLayout()
{
    // Recent PCSX2 builds also expose a fixed 10:7 display mode.
    const int displayMode = PCSX2Data[PCSX2Data_AspectRatioSetting];
    // PCSX2 presents 480p at 3:2 in Auto 4:3/3:2. This is the native
    // active video mode, including the temporary 480p preview and fallback.
    const bool progressive = *reinterpret_cast<const uint32_t*>(0x665EC0 + 952) == 1;
    float aspect = forcedAspect > 0 ? forcedAspect :
        (displayMode == 4 ? 10.0f / 7.0f :
         displayMode == RAuto4_3_3_2 && progressive ? 3.0f / 2.0f : console::ps2Aspect(PCSX2Data));
    const int players = *reinterpret_cast<const int*>(hudManager + 368) == 2 ? 2 : 1;
    const bool sideSplit = players == 2 && *reinterpret_cast<const uint8_t*>(0x665ECF) != 0;
    if (layout.aspect == aspect && layout.players == players && layout.sideSplit == sideSplit)
    {
        return;
    }
    layout.aspect = aspect;
    layout.players = players;
    layout.sideSplit = sideSplit;
    layout.width = 480.0f * aspect;
    layout.hudScale = (4.0f / 3.0f) / aspect;
    layout.offset = (640.0f - layout.width) * 0.5f;
    layout.hudOrigin = 320.0f * (1.0f - layout.hudScale);
    layout.normalizedOrigin = layout.hudOrigin / 640.0f;
    layout.menuWidth = 640.0f * layout.hudScale;
    const float paneWidthFraction = sideSplit ? 0.5f : 1.0f;
    const float paneHeightFraction = players == 2 && !sideSplit ? 0.5f : 1.0f;
    const float paneAspect = aspect * paneWidthFraction / paneHeightFraction;
    const float boxFraction = hudConstraint > 0 ? std::min(1.0f, hudConstraint / paneAspect) : 1.0f;
    const float paneWidth = 640.0f * paneWidthFraction;
    const float nativeAnchorWidth = sideSplit ? 260.0f : 580.0f;
    const float anchorExtension = paneWidth * (boxFraction / layout.hudScale - 1.0f) / (2.0f * nativeAnchorWidth);
    layout.rightAnchor = 1.0f + anchorExtension;
    layout.leftAnchor = -anchorExtension;
    injector::WriteMemory<float>(0x4E0A38, 1.0f / layout.hudScale);
    injector::WriteMemory<float>(0x4E0C70, aspect);
    injector::WriteMemory<float>(0x4E0C7C, aspect);
    injector::WriteMemory<float>(0x4E0C80, aspect * 2.0f);
}
bool IsDrivingCamera(uintptr_t camera)
{
    const uintptr_t source = *reinterpret_cast<const uint32_t*>(camera + 104);
    if (!source || *reinterpret_cast<const uint32_t*>(source) != 0x4DE010) return false;
    const uint64_t modes = *reinterpret_cast<const uint64_t*>(source - 512 + 2944);
    return modes == (1ull << 2) || modes == (1ull << 5);
}
void PollMouse(uintptr_t game)
{
    static_assert(sizeof(CMouseControllerState) == sizeof(console::Mouse));
    mouseInput.sample(noKeyboard, reinterpret_cast<console::Mouse&>(MouseState[0]));
    for (int player = 0; player < 2; ++player)
        if (game != 0x4EE040 || *reinterpret_cast<const uint32_t*>(game + 187024) != 5 ||
            *reinterpret_cast<const uint32_t*>(0x1E90430) != 0 || !IsDrivingCamera(player ? 0x665FF0 : 0x665F50))
        {
            padOrbit[player] = {};
            padOrbitFrame[player] = ~0u;
        }
    if (game != 0x4EE040 || *reinterpret_cast<const uint32_t*>(game + 187024) != 5 ||
        *reinterpret_cast<const uint32_t*>(0x1E90430) != 0 || !IsDrivingCamera(0x665F50))
    {
        mouseYaw = mousePitch = 0.0f;
        mouseHold = 0;
        return;
    }
    const float dx = mouseInput.mouse.x, dy = mouseInput.mouse.y;
    if (dx != 0.0f || dy != 0.0f)
    {
        mouseYaw = std::clamp(mouseYaw + dx * 0.0025f * mouseSensitivity, -2.0943951f, 2.0943951f);
        mousePitch = std::clamp(mousePitch + dy * (invertMouseY ? 0.0025f : -0.0025f) * mouseSensitivity,
                                -0.6108652f, 0.6108652f);
        mouseHold = 90;
    }
    else if (mouseHold)
        --mouseHold;
    else
    {
        mouseYaw *= 0.9f;
        mousePitch *= 0.9f;
        if (std::fabs(mouseYaw) < 0.0001f) mouseYaw = 0.0f;
        if (std::fabs(mousePitch) < 0.0001f) mousePitch = 0.0f;
    }
}
void RateError(uint32_t generation)
{
    lastStateLoad = generation;
    std::snprintf(OSDText[0], OSDStringSize, "Could not restore the selected frame rate. Restart the game.");
    menuNoticeFrames = 180;
}
void PollSettings()
{
    if (menuNoticeFrames && !--menuNoticeFrames)
        OSDText[0][0] = 0;
    const uint32_t generation = *reinterpret_cast<volatile uint32_t*>(&PCSX2FStateLoadGeneration);
    const bool loaded = generation != lastStateLoad;
    if (generation != lastStateLoad || ++refreshFrames >= 60)
    {
        refreshFrames = 0;
        if (console::ps2settings::read(options))
        {
            ReadSettings();
            layout.aspect = 0;
            const bool wanted60 = std::clamp(std::atoi(options[11].value), 30, 60) > 30;
            if (loaded && wanted60 != fps60Applied)
            {
                const uint32_t mode = *reinterpret_cast<const uint32_t*>(0x4E2CFC);
                if (mode > 3) { RateError(generation); return; }
                if (nativeFrameInterval != 1 && nativeFrameInterval != 2) { RateError(generation); return; }
                const bool halfMode = mode >= 2;
                const int nativeRate = (mode & 1) ? 50 : 60;
                const int oldRate = nativeRate / (halfMode && !fps60Applied ? 2 : 1);
                const int newRate = nativeRate / (halfMode && !wanted60 ? 2 : 1);
                const float oldStep = *reinterpret_cast<const float*>(0x4E2740);
                if (!std::isfinite(oldStep) || std::fabs(oldStep - 1.0f / oldRate) > 0.00001f) { RateError(generation); return; }
                constexpr uintptr_t timers[] = {0x4F5080, 0x522660, 0x1EBE360};
                if (oldRate != newRate) for (uintptr_t address : timers)
                {
                    const int divisor = *reinterpret_cast<const int*>(address + 24);
                    if (!divisor && address == 0x1EBE360) continue;
                    const int targetDivisor = *reinterpret_cast<const int*>(address + 36);
                    if (divisor <= 0 || divisor > 1024 || targetDivisor <= 0 || targetDivisor > 1024) { RateError(generation); return; }
                    updateTimer(address);
                    const int updatedDivisor = *reinterpret_cast<const int*>(address + 24);
                    const int whole = *reinterpret_cast<const int*>(address + 12);
                    const int remainder = *reinterpret_cast<const int*>(address + 16);
                    if (updatedDivisor <= 0 || updatedDivisor > 1024 || whole < 0 ||
                        remainder < 0 || remainder > updatedDivisor || whole > 0x10000000) { RateError(generation); return; }
                }
                constexpr uint32_t sites[] = {0x1D3F2C, 0x1321FC, 0x132F58, 0x132F60};
                constexpr uint32_t original[] = {0x10A0000A, 0x24040002, 0x24050002, 0x24060008};
                constexpr uint32_t full[] = {0x1000000A, 0x24040001, 0x24050001, 0x24060004};
                const uint32_t* before = fps60Applied ? full : original;
                const uint32_t* after = wanted60 ? full : original;
                unsigned written = 0;
                while (written < 4 && pcsx2_memory_write(&injector::detail::backend, sites[written], &after[written], 4) == PCSX2_HOOK_OK)
                    ++written;
                if (written != 4)
                {
                    while (written)
                    {
                        --written;
                        if (pcsx2_memory_write(&injector::detail::backend, sites[written], &before[written], 4) != PCSX2_HOOK_OK)
                            __builtin_trap();
                    }
                    RateError(generation);
                    return;
                }
                fps60Applied = wanted60;
                setFrameInterval(wanted60 ? 1 : nativeFrameInterval);
                if (oldRate != newRate)
                {
                    *reinterpret_cast<float*>(0x4E2740) = 1.0f / newRate;
                    *reinterpret_cast<int*>(0x4E2744) = 1000 / newRate;
                    for (uintptr_t address : timers)
                    {
                        const int divisor = *reinterpret_cast<const int*>(address + 24);
                        if (!divisor && address == 0x1EBE360) continue;
                        int64_t units = int64_t(*reinterpret_cast<const int*>(address + 12)) * divisor +
                                        *reinterpret_cast<const int*>(address + 16);
                        units = wanted60 ? units * 2 : (units + 1) / 2;
                        *reinterpret_cast<int*>(address + 12) = int(units / divisor);
                        *reinterpret_cast<int*>(address + 16) = int(units % divisor);
                        *reinterpret_cast<float*>(address + 28) = 1.0f / newRate / divisor;
                        *reinterpret_cast<float*>(address + 32) = float(units) / divisor / newRate;
                    }
                }
            }
            lastStateLoad = generation;
        }
        else if (loaded) RateError(generation);
    }
}
void SkipStartupMovies()
{
    // The movie player ends a playing movie in state 4, as when a skip button
    // is pressed; the owning screen then continues natively. Movies 1 to 9 are
    // the legal and Criterion videos, movie 0 is the intro before the title.
    constexpr uintptr_t player = 0x1E98478;
    auto& state = *reinterpret_cast<int32_t*>(player + 4);
    const uint32_t movie = *reinterpret_cast<const uint32_t*>(0x51A9A4);
    if (!skipIntro || state != 2 || movie > 9)
        return;
    if (movie == 0)
        skipIntro = false;
    state = 4;
}
void MenuChanged(bool restart)
{
    ReadSettings();
    layout.aspect = 0;
    if (restart)
    {
        std::snprintf(OSDText[0], OSDStringSize, "Restart the game to apply startup settings.");
        menuNoticeFrames = 180;
    }
}
void MenuError()
{
    std::snprintf(OSDText[0], OSDStringSize, "Could not save plugin settings.");
    menuNoticeFrames = 180;
}
void MenuReload()
{
    if (console::ps2settings::read(options)) MenuChanged(false);
    else MenuError();
}
void ApplyHud()
{
    UpdateLayout();
    const bool anchorsReady = ReanchorHud(layout.leftAnchor, layout.rightAnchor);
    unsigned player = 0;
    for (uintptr_t address : {0x6682B0u, 0x669B30u})
    {
        const float center = layout.sideSplit ? (player == 0 ? 160.0f : 480.0f) : 320.0f;
        injector::WriteMemory<float>(address, center * (1.0f - layout.hudScale));
        // The native HUD setup caches this scale alongside the origin.
        injector::WriteMemory<float>(address + 8, layout.hudScale);
        ++player;
    }
    if (anchorsReady)
        SetAnchors(layout.leftAnchor, layout.rightAnchor);
    for (uintptr_t address : {0x4CA660u, 0x4CA640u, 0x4CA650u})
    {
        injector::WriteMemory<float>(address, layout.width);
    }
    for (uintptr_t address : {0x4CA638u, 0x4CA658u})
    {
        injector::WriteMemory<float>(address, layout.offset);
    }
    injector::WriteMemory<float>(0x4E105C, 1.0f / layout.width);
}
void HudReset1()
{
    reinterpret_cast<void (*)()>(0x4D5350)();
    ApplyHud();
}
void HudReset2()
{
    reinterpret_cast<void (*)()>(0x4DB330)();
    ApplyHud();
}
void Store(uintptr_t address, float value)
{
    *reinterpret_cast<float*>(address) = value;
}
void Rejected(pcsx2_hook_status status)
{
    std::snprintf(OSDText[0], OSDStringSize, "Burnout 3 fix disabled: patch validation failed (%u)", unsigned(status));
}
template <class Callback> void Mid(uintptr_t address, Callback callback, bool executeOriginal = true)
{
    safetymips::Options options;
    options.execute_original = executeOriginal;
    if (hookCount >= hooks.size())
    {
        Rejected(PCSX2_HOOK_INVALID_ARGUMENT);
        return;
    }
    hooks[hookCount++] = safetymips::create_mid(address, callback, options);
}
} // namespace

extern "C" void init()
{
    static constexpr console::ps2conflict::OldPlugin oldPlugins[] = {
        {"Burnout3.PCSX2F.WidescreenFix.elf", "MAIN", "ForceAspectRatio"},
    };
    if (console::ps2conflict::Found(oldPlugins, OSDText))
        return;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK)
    {
        return;
    }
    if (!console::ps2settings::load(options))
    {
        std::snprintf(OSDText[0], OSDStringSize,
                      "CFG settings unavailable: update the plugin injector for CFG support.");
    }
    ReadSettings();
    setFrameInterval.bind(0x1F4290);
    updateTimer.bind(0x1D3A60);
    burnout::menu::Install(options, MenuChanged, MenuError, MenuReload);
    hudAnchor.bind(0x1A02C0);
    rebuildSplit.bind(0x19E760);
    setupSplitHud.bind(0x1A0660);
    hudTransform.create(0x1D4780, BuildHudTransforms);
    hudRender.create(0x1D75C0, RenderHudTree);
    ApplyHud();
    injector::WriteMemory<uint32_t>(0x228194, 0x24020001); // Select the native widescreen camera.
    // Rebuild the player viewports between frames, retaining each HUD node's
    // offset from its native anchor as the viewport bounds change.
    Mid(0x13CA20, [](SafetyMipsContext&)
    {
        if (!anchorsInitialized || *reinterpret_cast<const int*>(hudManager + 368) != 2 ||
            (*reinterpret_cast<const uint8_t*>(0x665ECF) != 0) == sideBySide) return;
        if (ReanchorHud(appliedLeft, appliedRight, true))
        {
            layout.aspect = 0;
            ApplyHud();
        }
    });
    // Track the interval the original game requested, even while 60 FPS
    // substitutes interval 1. This survives a save and handles flag changes.
    Mid(0x1D3EE0, [](SafetyMipsContext& regs) { nativeFrameInterval = regs.a1 ? 2 : 1; });
    Mid(0x1321E8, [](SafetyMipsContext& regs) { nativeFrameInterval = uint8_t(regs.a0) ? 2 : 1; });
    fps60Applied = std::clamp(std::atoi(options[11].value), 30, 60) > 30;
    if (fps60Applied)
    {
        // Keep the native mode flags while using its full-rate timing path.
        injector::WriteMemory<uint32_t>(0x1D3F2C, 0x1000000A);
        // Keep crash-state transitions but request the normal display interval.
        injector::WriteMemory<uint32_t>(0x1321FC, 0x24040001);
        // The native half-rate mode asks for two simulation steps per frame.
        // Its render interval is now one, so retain the original simulation rate.
        injector::WriteMemory<uint32_t>(0x132F58, 0x24050001);
        injector::WriteMemory<uint32_t>(0x132F60, 0x24060004);
    }
    Mid(0x3250C8, [](SafetyMipsContext& regs)
    {
        const uint32_t mode = *reinterpret_cast<const uint32_t*>(0x4E2CFC);
        if (fps60Applied && (mode == 2 || mode == 3)) regs.f1 *= 0.5f;
    });
    // Read once: the startup videos play before the first settings refresh.
    skipIntro = console::ps2settings::boolean(options[3], true);
    hudReset1.bind(HudReset1);
    hudReset2.bind(HudReset2);
    injector::WriteMemory<uint32_t>(0x4DD840, hudReset1.address());
    injector::WriteMemory<uint32_t>(0x4DD9E4, hudReset2.address());
    // Refresh CFG from the main update, outside the camera/render callbacks.
    Mid(0x132600, [](SafetyMipsContext& regs)
    {
        const uintptr_t game = uintptr_t(regs.a0);
        // State 4 runs the native scene-loading state machine; state 5 is
        // normal gameplay and frontend updates.
        FrameLimitUnthrottle = game == 0x4EE040 && *reinterpret_cast<const uint32_t*>(game + 187024) == 4;
        PollSettings();
        PollMouse(game);
        SkipStartupMovies();
    });
    // The loading state can enter gameplay in the same native update.
    Mid(0x132D04, [](SafetyMipsContext&) { FrameLimitUnthrottle = 0; });
    // The native message creator tells us whether its point uses the left
    // anchor (0) or the viewport center (1). Remember it only for this live
    // game-owned object so an aspect change can preserve animated offsets.
    Mid(0x3D56D0, [](SafetyMipsContext& regs)
    {
        const uintptr_t state = uintptr_t(regs.a0);
        const uintptr_t object = state - 64;
        if (*reinterpret_cast<const uint32_t*>(object) != 0x4DFD10 ||
            *reinterpret_cast<const uint32_t*>(object + 4) != hudManager || regs.a7 > 1) return;
        const int player = *reinterpret_cast<const int8_t*>(object + 28);
        if (player < 0 || player > 1) return;
        const uintptr_t parent = *reinterpret_cast<const uint32_t*>(object + 8);
        if (parent != uintptr_t(regs.a2)) return;
        messagePlacement[player] = {object, parent, regs.a7 == 0};
    });
    // The native split viewport builder consumes this flag once per race.
    // Change it before the native builder computes either player's rectangle.
    Mid(0x19E760, [](SafetyMipsContext& regs)
    {
        if (uintptr_t(regs.a0) == 0x665EC0)
            *reinterpret_cast<uint8_t*>(uintptr_t(regs.a0) + 15) = sideBySide ? 1 : 0;
    });
    Mid(0x1D475C, [](SafetyMipsContext&) { ApplyHud(); });
    Mid(0x1E26D8, [](SafetyMipsContext&) { ApplyHud(); });
    // The movie renderer receives a temporary rectangle after the native UI
    // transform. Embedded UI videos and animated track textures keep theirs.
    Mid(0x326960, [](SafetyMipsContext& regs)
    {
        const uintptr_t rectAddress = uintptr_t(regs.a0);
        const uintptr_t data = uintptr_t(regs.a1);
        if (*reinterpret_cast<const uint32_t*>(data + 16) != 0x51A870 ||
            *reinterpret_cast<const uint8_t*>(data + 20) != 0 ||
            *reinterpret_cast<const uint32_t*>(0x51A9A4) >= 82)
            return;
        burnout::FitMovieFrame(reinterpret_cast<float*>(rectAddress), movieMode);
    });
    // The native camera frame is complete here and is about to be published.
    Mid(0x212264, [](SafetyMipsContext& regs)
    {
        const uintptr_t camera = uintptr_t(regs.s4);
        const int player = camera == 0x665F50 ? 0 : camera == 0x665FF0 ? 1 : -1;
        if (player < 0 || *reinterpret_cast<const uint32_t*>(0x4EE040 + 187024) != 5 ||
            *reinterpret_cast<const uint32_t*>(0x1E90430) != 0) return;
        if (!IsDrivingCamera(camera)) return;
        constexpr uintptr_t manager = 0x4F50C0;
        float x = 0.0f, y = 0.0f;
        if (*reinterpret_cast<const uint8_t*>(manager + 4003 + player))
        {
            const int slot = *reinterpret_cast<const int8_t*>(manager + 4001 + player);
            if (slot >= 0 && slot <= 1)
            {
                const uintptr_t device = *reinterpret_cast<const uint32_t*>(manager + 3400 + 300 * slot + 4);
                if ((device == manager + 640 || device == manager + 876) &&
                    *reinterpret_cast<const uint8_t*>(device + 8))
                {
                    x = *reinterpret_cast<const float*>(device + 176);
                    y = *reinterpret_cast<const float*>(device + 180);
                }
            }
        }
        const uintptr_t frame = uintptr_t(regs.s3);
        const uint32_t tick = *reinterpret_cast<const uint32_t*>(0x4E2A14);
        if (padOrbitFrame[player] != tick)
        {
            padOrbitFrame[player] = tick;
            padOrbit[player].Update(x, y, *reinterpret_cast<const float*>(0x4F509C), gamepadCameraSpeed, invertGamepadY);
        }
        if (player == 0 && (std::fabs(x) > 0.12f || std::fabs(y) > 0.12f))
            mouseYaw = mousePitch = 0.0f;
        const bool mouseActive = player == 0 && (mouseYaw != 0.0f || mousePitch != 0.0f);
        burnout::RotateCameraBasis(reinterpret_cast<float*>(frame + 16),
            mouseActive ? mouseYaw : padOrbit[player].yaw, mouseActive ? mousePitch : padOrbit[player].pitch);
    });
    // The source belongs to the player camera only when its native vtable
    // matches. The native active-mode bitset is authoritative: the last mode
    // field can remain stale after the game switches back to a driving view.
    Mid(0x212294, [](SafetyMipsContext& regs)
    {
        const uintptr_t camera = uintptr_t(regs.s4);
        if (camera != 0x665F50 && camera != 0x665FF0) return;
        // The native setup caches these values once in sub_19E8C0. Refresh
        // them here so an aspect change also reaches existing player cameras.
        if (*reinterpret_cast<const uint8_t*>(0x665ECD))
        {
            UpdateLayout();
            const uint32_t fullWidth = *reinterpret_cast<const uint32_t*>(0x665EC0);
            const uint32_t fullHeight = *reinterpret_cast<const uint32_t*>(0x665EC4);
            const uint32_t viewWidth = *reinterpret_cast<const uint32_t*>(camera + 120);
            const uint32_t viewHeight = *reinterpret_cast<const uint32_t*>(camera + 124);
            float aspect = layout.aspect;
            if (fullWidth && fullHeight && viewWidth && viewHeight)
                aspect *= (float(viewWidth) / float(fullWidth)) / (float(viewHeight) / float(fullHeight));
            Store(camera + 112, aspect * 0.75f);
            Store(camera + 116, aspect);
        }
        if (fovFactor == 1.0f) return;
        const uintptr_t source = *reinterpret_cast<const uint32_t*>(camera + 104);
        if (!source || *reinterpret_cast<const uint32_t*>(source) != 0x4DE010) return;
        const uint64_t modes = *reinterpret_cast<const uint64_t*>(source - 512 + 2944);
        if (modes != (1ull << 2) && modes != (1ull << 5)) return;
        const float horizontalScale = *reinterpret_cast<const float*>(camera + 112);
        if (!(horizontalScale > 0.0f) || !std::isfinite(regs.f0)) return;
        const float original = std::atan(regs.f0 * horizontalScale);
        const float limit = std::max(original, 1.48352986f); // Preserve native FOVs already over 170 degrees.
        regs.f0 = std::tan(std::clamp(original * fovFactor, 0.008726646f, limit)) / horizontalScale;
    });
    // Run after the native call and its SQ delay slot; no fabricated call ABI.
    Mid(0x1D5188, [](SafetyMipsContext&) { ApplyHud(); });

    // Change each value after its native LUI and before its first consumer.
    // All displaced instructions and upper halves of the EE registers survive.
    for (uintptr_t site : {0x3D723Cu, 0x134F30u, 0x38AE3Cu, 0x31D6E8u, 0x31D740u, 0x31D7ECu, 0x31D794u, 0x31D844u,
                           0x31DA24u, 0x3A6988u, 0x3A69CCu})
    {
        Mid(site, [](SafetyMipsContext& regs) { regs.v0 = injector::WordBits(layout.width); });
    }
    Mid(0x31B184, [](SafetyMipsContext& regs) { regs.v1 = injector::WordBits(layout.width); });
    Mid(0x30D7E8, [](SafetyMipsContext& regs) { regs.v1 = injector::WordBits(layout.menuWidth); });
    Mid(0x3D70C0, [](SafetyMipsContext& regs) { regs.f1 = layout.offset; });
    Mid(0x3D72F8, [](SafetyMipsContext& regs) { regs.f2 = layout.offset; });

    // These two sites write width and origin together. Keeping each pair in a
    // single callback avoids overlapping hooks in the adjacent native stores.
    Mid(
        0x1A1770,
        [](SafetyMipsContext& regs)
        {
            regs.v0 = injector::WordBits(layout.width);
            Store(uintptr_t(regs.sp) + 0xA8, layout.offset);
            Store(uintptr_t(regs.sp) + 0xA0, layout.width);
        },
        false);
    Mid(
        0x1A17D4,
        [](SafetyMipsContext& regs)
        {
            regs.v0 = injector::WordBits(layout.width);
            Store(uintptr_t(regs.sp) + 0x98, layout.offset);
            Store(uintptr_t(regs.sp) + 0x90, layout.width);
        },
        false);
    Mid(0x134F74, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x168, layout.hudScale * 0.5f); });
    Mid(
        0x30D834,
        [](SafetyMipsContext& regs)
        {
            Store(uintptr_t(regs.a0) + 8, regs.f3);
            Store(uintptr_t(regs.a0), layout.hudOrigin);
            Store(uintptr_t(regs.a0) + 12, regs.f0);
            regs.f3 = layout.hudOrigin;
        },
        false);
    Mid(0x4DC71C, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.v1) + 0x1568, layout.offset); });
    Mid(0x4DC73C, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.v1) + 0x1570, layout.width); });
    Mid(0x31B1F4,
        [](SafetyMipsContext& regs)
        {
            regs.f7 = 0.0f;
            Store(uintptr_t(regs.a0), 0.0f);
        });
    Mid(
        0x38AE04,
        [](SafetyMipsContext& regs)
        {
            regs.v0 = injector::WordBits(layout.width);
            Store(uintptr_t(regs.sp) + 0x78, layout.normalizedOrigin);
            Store(uintptr_t(regs.sp) + 0x70, layout.width);
        },
        false);
    Mid(0x38AE50, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x68, layout.offset); });
    Mid(0x31D6FC, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x110, layout.normalizedOrigin); });
    Mid(0x31D754, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0xF8, layout.normalizedOrigin); });
    Mid(0x31D800, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0xC8, layout.normalizedOrigin); });
    Mid(0x31D7A8, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0xE0, layout.normalizedOrigin); });
    Mid(0x31D858, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0xB0, layout.normalizedOrigin); });
    Mid(0x31DA44, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x1D8, layout.offset); });
    Mid(0x3A699C, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x78, layout.offset); });
    Mid(0x3A69E0, [](SafetyMipsContext& regs) { Store(uintptr_t(regs.sp) + 0x60, layout.offset); });
    injector::FlushCaches();
}
extern "C" int main()
{
    return 0;
}
