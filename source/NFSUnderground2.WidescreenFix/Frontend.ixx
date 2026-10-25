module;

#include <stdafx.h>
#include "../Shared/PatternPreflight.hpp"
#include <d3d9.h>
#include "FixSettings.hpp"

export module Frontend;

import ComVars;
import Resolution;

std::optional<float> fHudAspectRatioConstraint;
float fWidescreenHudOffset = 120.0f;
union HudPos
{
    uint32_t dwPos;
    float fPos;
    HudPos(uint32_t pos) : dwPos(pos) {}
    HudPos(float pos) : fPos(pos) {}
};

class CDatEntry
{
public:
    float fPosX;
    float fPosY;
    float fOffsetX;
    float fOffsetY;

    CDatEntry(float posx, float posy, float fOffsetX, float fOffsetY)
        : fPosX(posx), fPosY(posy), fOffsetX(fOffsetX), fOffsetY(fOffsetY)
    {
    }
};

std::vector<CDatEntry> HudCoords;

// The screen size the HUD offset was made for; the offset follows a change of the constraint at once
float fHudScreenWidth = 640.0f;
float fHudScreenHeight = 480.0f;

void UpdateWidescreenHudOffset()
{
    float width = fHudScreenWidth;
    float height = fHudScreenHeight;
    fWidescreenHudOffset = -CalculateWidescreenOffset(width, height, 640.0f, 480.0f);
    if (fHudAspectRatioConstraint.has_value())
    {
        float value = fHudAspectRatioConstraint.value();
        if (value < 0.0f || value > (32.0f / 9.0f))
            fWidescreenHudOffset = value;
        else
        {
            value = ClampHudAspectRatio(value, width / height);
            fWidescreenHudOffset = -CalculateWidescreenOffset(height * value, height, 640.0f, 480.0f);
        }
    }
}

// Layout table: original element position and how far to move it for 16:9.
// Embedded from HudLayout.dat.
void LoadDatFile()
{
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&LoadDatFile), &module);
    HRSRC resource = FindResourceA(module, "HUDLAYOUT", MAKEINTRESOURCEA(10));
    HGLOBAL handle = resource ? LoadResource(module, resource) : nullptr;
    auto text = handle ? static_cast<const char*>(LockResource(handle)) : nullptr;
    if (!text)
        return;

    std::istringstream stream(std::string(text, SizeofResource(module, resource)));
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.empty() || line[0] == '#' || line[0] == '\r')
            continue;

        float PosX, PosY, fOffsetX, fOffsetY;
        if (sscanf_s(line.c_str(), "%*s %f %f %f %f", &PosX, &PosY, &fOffsetX, &fOffsetY) == 4)
            HudCoords.push_back(CDatEntry(PosX, PosY, fOffsetX, fOffsetY));
    }
}

// Movies
namespace
{
    bool IsPowerOfTwo(int value)
    {
        return value > 0 && (value & (value - 1)) == 0;
    }

    // 0 original aspect, 1 fill the screen, 2 stretch
    const int* pFMVWidescreenMode = nullptr;

    struct MovieQuad
    {
        float x0, y0, x1, y1, z;
        float u0, v0, u1, v1;
    };

    void FitMovie(void* renderObject, MovieQuad& quad, int frameWidth, int frameHeight, bool playing)
    {
        // Object transform in canvas units, copied to the render object by the game
        auto matrix = reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(renderObject) + 0x20);
        float objectWidth = std::fabs(matrix[0]);
        float objectHeight = std::fabs(matrix[5]);
        if (objectWidth < 1.0f || objectHeight < 1.0f || frameWidth <= 0 || frameHeight <= 0)
            return;

        // The game's own movies are anamorphic: power-of-two frames stretched over the movie object,
        // which is 4:3. 2:1 frames contain a 16:9 picture with black bars, which are cropped.
        // Replacement movies with other sizes are shown with square pixels.
        bool fullScreen = objectWidth >= 600.0f && objectHeight >= 450.0f;

        float contentAspect;
        float crop = 0.0f;
        if (IsPowerOfTwo(frameWidth) && IsPowerOfTwo(frameHeight))
        {
            contentAspect = fullScreen ? 4.0f / 3.0f : objectWidth / objectHeight;
            if (frameWidth == frameHeight * 2)
            {
                crop = 0.125f;
                contentAspect /= 1.0f - 2.0f * crop;
            }
        }
        else
            contentAspect = float(frameWidth) / float(frameHeight);

        // Full screen movies use the whole screen, others keep their box. Full screen movies are shown whole with
        // black bars, fill the screen cropped, or stretched to it (MAIN.FMVWidescreenMode).
        float boxWidth = fullScreen ? 480.0f * GetAspectRatio() : objectWidth;
        float boxHeight = fullScreen ? 480.0f : objectHeight;
        int mode = fullScreen ? *pFMVWidescreenMode : 0;

        float width = boxWidth;
        float height = boxWidth / contentAspect;
        if (mode == 2)
            height = boxHeight;
        else if (mode == 1 ? height < boxHeight : height > boxHeight)
        {
            height = boxHeight;
            width = boxHeight * contentAspect;
        }

        float halfX = 0.5f * width / objectWidth;
        float halfY = 0.5f * height / objectHeight;
        quad.x0 = -halfX;
        quad.y0 = -halfY;
        quad.x1 = halfX;
        quad.y1 = halfY;

        // Texture coordinates may be flipped vertically
        float v = quad.v1 - quad.v0;
        quad.v0 += v * crop;
        quad.v1 -= v * crop;

        // Black bars around a playing full screen movie. Menus also contain movie objects that aren't playing.
        if (fullScreen && playing)
        {
            if (auto device = Direct3DDevice.get())
                device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
        }
    }
}

// Cutscene borders. The race intro scenes (NIS) set a letterbox height on the player's camera (+0xC8, a part of
// the screen height, 0.125 when the bars are in), animated with the scene. After the player's view the game draws
// a black bar of that height at the top and at the bottom of the screen (sub_5CBDC0). The bars were made for 4:3,
// the picture between them is 16:9. MAIN.CutsceneBorders: 0 off, 1 letterbox, 2 pillarbox, 3 both. Without the
// letterbox bars the scene's projection is zoomed in, so the picture shows what was between the bars and nothing
// more. The pillarbox bars limit the picture to 16:9 on wider screens while the scene's letterbox is in.
namespace CutsceneBorders
{
    constexpr float FullHeight = 0.125f;
    const int* pMode = nullptr;
    const int* pAnimated = nullptr;
    float fLetterbox = 0.0f;    // the scene's letterbox height in this frame

    bool Letterbox() { return *pMode == 1 || *pMode == 3; }
    bool Pillarbox() { return *pMode == 2 || *pMode == 3; }

    // The bars' height: the scene's as it slides in and out, or the whole height at once
    float Height(float letterbox)
    {
        if (letterbox <= 0.0f)
            return 0.0f;
        return std::min(*pAnimated ? letterbox : std::max(letterbox, FullHeight), 0.45f);
    }

    // The game's screen quads (640x480 canvas, stretched over the screen) and the bars' texture
    void (__fastcall* InitQuad)(void* quad, void* edx) = nullptr;
    void (__cdecl* DrawQuad)(void* quad, void* texture, int) = nullptr;
    void** pTexture = nullptr;

    void DrawPillarbox()
    {
        float letterbox = fLetterbox;
        if (!Pillarbox() || letterbox <= 0.0f || !*pTexture)
            return;

        float aspect = GetAspectRatio();
        if (aspect <= 16.0f / 9.0f + 0.001f)
            return;

        float width = 320.0f * (1.0f - (16.0f / 9.0f) / aspect);
        if (*pAnimated)
            width *= std::min(letterbox / FullHeight, 1.0f);

        for (float x0 : { 0.0f, 640.0f - width })
        {
            // Corners (x, y, z, w) clockwise from the top left, then texture coordinates and colours
            alignas(16) uint8_t quad[0x80] = {};
            InitQuad(quad, nullptr);
            auto v = reinterpret_cast<float*>(quad);
            v[0] = x0;          v[1] = 0.0f;    v[2] = 0.0f;
            v[4] = x0 + width;  v[5] = 0.0f;    v[6] = 0.0f;
            v[8] = x0 + width;  v[9] = 480.0f;  v[10] = 0.0f;
            v[12] = x0;         v[13] = 480.0f; v[14] = 0.0f;
            auto colors = reinterpret_cast<uint32_t*>(quad + 0x60);
            colors[0] = colors[1] = colors[2] = colors[3] = 0x80000000;
            DrawQuad(quad, *pTexture, 0);
        }
    }
}

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
            pFMVWidescreenMode = &FixSettings().Int("MAIN.FMVWidescreenMode");

            //HUD
            {
                [&]()
                {
                    if (!PatternPreflight::Check("NFSUnderground2 Frontend", {
                        { "D9 05 ? ? ? ? 89 7C 24 28 C7 44 24 2C 00 00 00 3F D8 C9 C7 44 24 30 00 00 00 3F", 1, true },
                        { "C7 84 24 A0 00 00 00 00 00 A0 43 C7 84 24 A4 00 00 00 00 00 70 43 C7 84 24 A8 00 00 00 00 00 00 00 0F B7 48 20", 1, false },
                        { "C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 4C 24 70", 1, false },
                        { "C7 84 24 84 00 00 00 00 00 A0 43 C7 84 24 88 00 00 00 00 00 70 43 C7 84 24 8C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 80 00 00 00", 1, false },
                        { "C7 84 24 94 00 00 00 00 00 A0 43 C7 84 24 98 00 00 00 00 00 70 43 C7 84 24 9C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 90 00 00 00", 1, false },
                        { "C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 54 24 70 52", 1, false },
                        { "C7 05 ? ? ? ? 00 00 A0 43 C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 00 00 00 00 C6 05 ? ? ? ? 82", 1, false },
                        { "C7 84 24 A0 01 00 00 00 00 A0 43 C7 84 24 A4 01 00 00 00 00 70 43 C7 84 24 A8 01 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 30 01 00 00 51 8B F0", 1, false },
                        { "D8 25 ? ? ? ? 8B 4C 24 28 6A 00 8D 54 24 48 D9 5C 24 48 89 4C 24 50 D9 44 24 28 8B 0B", 1, true },
                        { "D8 0D ? ? ? ? D9 5C 24 ? D9 44 24 ? D8 74 24", 1, true },
                        { "C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? 33 D2", 1, false },
                        { "C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? 7D", 1, false },
                        { "6A ? 68 ? ? ? ? 68 ? ? ? ? 8B C8 E8 ? ? ? ? EB ? 33 C0 8B CF", 1, false },
                        { "DA 0D ? ? ? ? 55 56 8B F0 A1 ? ? ? ? 99", 1, false },
                        { "89 4C 24 60 89 54 24 64 74 ? 8D 8C 24 F0 00 00 00 51", 1, true },
                        { "8B 4B 1C 8B 54 24 18 89 0A 8B 43 20", 1, true },
                        { "D9 56 48 8B 4E 1C D8 2D ? ? ? ? 89 4C 24 28 8B 56 20 89 54 24 2C 8B 46 24", 1, true },
                        { "D8 02 D9 1E D9 41 04 D8 60 04 D8 4C 24 10 D8 42 04 D8 40 04 D9 5E 04 D9 41 08 D8 60 08", 1, true },
                    }))
                        return;

                    static ProtectedGameRef<float> fHudScaleX;
                    fHudScaleX.SetAddress(*hook::pattern("D9 05 ? ? ? ? 89 7C 24 28 C7 44 24 2C 00 00 00 3F D8 C9 C7 44 24 30 00 00 00 3F").count(1).get(0).get<float*>(2));

                    static std::vector<ProtectedGameRef<float>> fHudPosXArray;

                    auto setHudPosRef = [&](hook::pattern pat, int offset)
                    {
                        ProtectedGameRef<float> ref;
                        ref.SetAddress(pat.count(1).get(0).get<float>(offset));
                        fHudPosXArray.push_back(std::move(ref));
                    };

                    setHudPosRef(hook::pattern("C7 84 24 A0 00 00 00 00 00 A0 43 C7 84 24 A4 00 00 00 00 00 70 43 C7 84 24 A8 00 00 00 00 00 00 00 0F B7 48 20"), 7);
                    setHudPosRef(hook::pattern("C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 4C 24 70"), 4);
                    setHudPosRef(hook::pattern("C7 84 24 84 00 00 00 00 00 A0 43 C7 84 24 88 00 00 00 00 00 70 43 C7 84 24 8C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 80 00 00 00"), 7);
                    setHudPosRef(hook::pattern("C7 84 24 94 00 00 00 00 00 A0 43 C7 84 24 98 00 00 00 00 00 70 43 C7 84 24 9C 00 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 90 00 00 00"), 7);
                    setHudPosRef(hook::pattern("C7 44 24 74 00 00 A0 43 C7 44 24 78 00 00 70 43 C7 44 24 7C 00 00 00 00 E8 ? ? ? ? 8D 54 24 70 52"), 4);
                    setHudPosRef(hook::pattern("C7 05 ? ? ? ? 00 00 A0 43 C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 00 00 00 00 C6 05 ? ? ? ? 82"), 6);
                    setHudPosRef(hook::pattern("C7 84 24 A0 01 00 00 00 00 A0 43 C7 84 24 A4 01 00 00 00 00 70 43 C7 84 24 A8 01 00 00 00 00 00 00 E8 ? ? ? ? 8D 8C 24 30 01 00 00 51 8B F0"), 7);

                    ProtectedGameRef<float> temp_ref;
                    temp_ref.SetAddress(*hook::pattern("D8 25 ? ? ? ? 8B 4C 24 28 6A 00 8D 54 24 48 D9 5C 24 48 89 4C 24 50 D9 44 24 28 8B 0B").count(1).get(0).get<float*>(2));
                    fHudPosXArray.push_back(std::move(temp_ref));

                    static ProtectedGameRef<float> fHudPosX_x2;
                    fHudPosX_x2.SetAddress(*hook::pattern("D8 0D ? ? ? ? D9 5C 24 ? D9 44 24 ? D8 74 24").count(1).get(0).get<float*>(2));

                    auto pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? 33 D2");

                    static ProtectedGameRef<float> fRearviewMirrorLeftX1;
                    fRearviewMirrorLeftX1.SetAddress(pattern.get_first<float>(4));

                    static ProtectedGameRef<float> fRearviewMirrorLeftX2;
                    fRearviewMirrorLeftX2.SetAddress(pattern.get_first<float>(4 + 16));

                    static ProtectedGameRef<float> fRearviewMirrorRightX1;
                    fRearviewMirrorRightX1.SetAddress(pattern.get_first<float>(4 - 16));

                    static ProtectedGameRef<float> fRearviewMirrorRightX2;
                    fRearviewMirrorRightX2.SetAddress(pattern.get_first<float>(4 + 32));

                    pattern = hook::pattern("C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? 7D");

                    static ProtectedGameRef<float> fRadarMaskWidth1;
                    fRadarMaskWidth1.SetAddress(pattern.get_first<float>(4));

                    static ProtectedGameRef<float> fRadarMaskWidth2;
                    fRadarMaskWidth2.SetAddress(pattern.get_first<float>(4 + 16));

                    static ProtectedGameRef<float> fRadarMaskHeight1;
                    fRadarMaskHeight1.SetAddress(pattern.get_first<float>(4 + 24));

                    static ProtectedGameRef<float> fRadarMaskHeight2;
                    fRadarMaskHeight2.SetAddress(pattern.get_first<float>(4 + 40));

                    //graph of the dyno result issue #366 / #462
                    pattern = hook::pattern("6A ? 68 ? ? ? ? 68 ? ? ? ? 8B C8 E8 ? ? ? ? EB ? 33 C0 8B CF");

                    static ProtectedGameRef<std::array<float, 2>> fDynoResultOffset;
                    fDynoResultOffset.SetAddress(*pattern.get_first<std::array<float, 2>*>(8));

                    //controls screenshot aspect ratio issue #462
                    static auto Width43 = 0;
                    pattern = hook::pattern("DA 0D ? ? ? ? 55 56 8B F0 A1 ? ? ? ? 99"); //5C30AC
                    injector::WriteMemory(pattern.get_first(2), &Width43, true);

                    onResChange() += [](int Width, int Height)
                    {
                        fHudScaleX = (1.0f / static_cast<float>(Width) * (static_cast<float>(Height) / 480.0f)) * 2.0f;

                        float fHudPosX = 640.0f / (640.0f * fHudScaleX);

                        for (auto& pos : fHudPosXArray)
                        {
                            pos = fHudPosX;
                        }

                        fHudPosX_x2 = fHudPosX * 2.0f;

                        float fHudOffset = ((480.0f * (static_cast<float>(Width) / static_cast<float>(Height))) - 640.0f) / 2.0f;

                        fRearviewMirrorLeftX1 = (fHudPosX - 320.0f) + 200.0f;
                        fRearviewMirrorLeftX2 = (fHudPosX - 320.0f) + 200.0f;
                        fRearviewMirrorRightX1 = (fHudPosX - 320.0f) + 440.0f;
                        fRearviewMirrorRightX2 = (fHudPosX - 320.0f) + 440.0f;

                        fRadarMaskWidth1 = static_cast<float>(Width) * 2.0f;
                        fRadarMaskWidth2 = static_cast<float>(Width) * 2.0f;
                        fRadarMaskHeight1 = static_cast<float>(Height) * 2.0f;
                        fRadarMaskHeight2 = static_cast<float>(Height) * 2.0f;

                        fDynoResultOffset = { 79.0f + fHudOffset, 60.0f };

                        Width43 = static_cast<uint32_t>(static_cast<float>(Height) * (4.0f / 3.0f));

                        fHudScreenWidth = static_cast<float>(Width);
                        fHudScreenHeight = static_cast<float>(Height);
                        UpdateWidescreenHudOffset();
                    };

                    FixSettings().OnChange("MAIN.HudAspectRatioConstraint", []()
                    {
                        fHudAspectRatioConstraint = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
                        UpdateWidescreenHudOffset();
                    });

                    LoadDatFile();

                    static auto WidescreenHud = [](HudPos& HudPosX, HudPos& HudPosY)
                    {
                        auto it = std::find_if(HudCoords.begin(), HudCoords.end(),
                                               [cc = CDatEntry(std::floor(HudPosX.fPos), std::floor(HudPosY.fPos), 0.0f, 0.0f)]
                                               (const CDatEntry& cde) -> bool
                        {
                            return (int(cc.fPosX) == int(cde.fPosX) && int(cc.fPosY) == int(cde.fPosY));
                        });

                        if (it != HudCoords.end())
                        {
                            constexpr auto fBaseOffset = (480.0f * (16.0f / 9.0f) - 640.0f) / 2.0f;

                            if (it->fOffsetX >= 0.0f)
                                HudPosX.fPos += fWidescreenHudOffset + (it->fOffsetX - fBaseOffset);
                            else
                                HudPosX.fPos -= fWidescreenHudOffset - (it->fOffsetX + fBaseOffset);

                            HudPosY.fPos += it->fOffsetY;
                        }
                    };

                    uint32_t* dword_51B190 = hook::pattern("89 4C 24 60 89 54 24 64 74 ? 8D 8C 24 F0 00 00 00 51").count(1).get(0).get<uint32_t>(0);
                    struct HudHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            HudPos HudPosX = regs.ecx;
                            HudPos HudPosY = regs.edx;
                            WidescreenHud(HudPosX, HudPosY);
                            *(uint32_t*)(regs.esp + 0x60) = HudPosX.dwPos;
                            *(uint32_t*)(regs.esp + 0x64) = HudPosY.dwPos;
                        }
                    }; injector::MakeInline<HudHook>((uint32_t)dword_51B190, (uint32_t)dword_51B190 + 8);

                    uint32_t* dword_51D64E = hook::pattern("8B 4B 1C 8B 54 24 18 89 0A 8B 43 20").count(1).get(0).get<uint32_t>(0);
                    struct BlipsHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            HudPos HudPosX = *(uint32_t*)(regs.ebx + 0x1C);
                            HudPos HudPosY = *(uint32_t*)(regs.ebx + 0x20);
                            WidescreenHud(HudPosX, HudPosY);
                            *(uint32_t*)(regs.ebx + 0x1C) = HudPosX.dwPos;
                            *(uint32_t*)(regs.ebx + 0x20) = HudPosY.dwPos;

                            *(uint32_t*)(regs.edx) = *(uint32_t*)(regs.ebx + 0x1C);
                            regs.eax = *(uint32_t*)(regs.ebx + 0x20);
                        }
                    }; injector::MakeInline<BlipsHook>((uint32_t)dword_51D64E + 7);

                    uint32_t* dword_4C66B3 = hook::pattern("D9 56 48 8B 4E 1C D8 2D ? ? ? ? 89 4C 24 28 8B 56 20 89 54 24 2C 8B 46 24").count(1).get(0).get<uint32_t>(0); //addresses from 1.1 exe
                    struct HudHook2
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            float esi48 = 0.0f;
                            _asm {fst dword ptr[esi48]}
                            *(float*)(regs.esi + 0x48) = esi48;
                            regs.ecx = *(uint32_t*)(regs.esi + 0x1C);
                            regs.edx = *(uint32_t*)(regs.esi + 0x20);
                            HudPos HudPosX = regs.ecx;
                            HudPos HudPosY = regs.edx;
                            WidescreenHud(HudPosX, HudPosY);
                            regs.ecx = HudPosX.dwPos;
                        }
                    }; injector::MakeInline<HudHook2>((uint32_t)dword_4C66B3, (uint32_t)dword_4C66B3 + 6);

                    uint32_t* dword_5050FB = hook::pattern("D8 02 D9 1E D9 41 04 D8 60 04 D8 4C 24 10 D8 42 04 D8 40 04 D9 5E 04 D9 41 08 D8 60 08").count(1).get(0).get<uint32_t>(0); //addresses from 1.1 exe
                    struct StopSignHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            float esi00 = 0.0f;
                            float ecx4 = *(float*)(regs.ecx + 0x04);
                            HudPos HudPosX = *(uint32_t*)(regs.edx + 0x00);
                            HudPos HudPosY = *(uint32_t*)(regs.edx + 0x04);
                            WidescreenHud(HudPosX, HudPosY);
                            _asm {fadd dword ptr[HudPosX.fPos]}
                            _asm {fstp dword ptr[esi00]}
                            *(float*)(regs.esi + 0x00) = esi00;
                            _asm fld dword ptr[ecx4]
                        }
                    }; injector::MakeInline<StopSignHook>((uint32_t)dword_5050FB, (uint32_t)dword_5050FB + 7);
                }();

            }

            // Cutscene borders
            CutsceneBorders::pMode = &FixSettings().Int("MAIN.CutsceneBorders");
            CutsceneBorders::pAnimated = &FixSettings().Int("MAIN.AnimatedCutsceneBorders");
            auto pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC 88 00 00 00 56 8B 35 ? ? ? ? 85 F6 57 0F 84");
            auto projection = hook::pattern("D9 44 24 38 8B 44 24 28 D9 E0 8B 4C 24 34 D9 5E 54");
            if (!pattern.empty() && !projection.empty())
            {
                CutsceneBorders::pTexture = *pattern.get_first<void**>(15);
                CutsceneBorders::InitQuad = reinterpret_cast<decltype(CutsceneBorders::InitQuad)>(injector::GetBranchDestination(pattern.get_first(0x20)).as_int());
                CutsceneBorders::DrawQuad = reinterpret_cast<decltype(CutsceneBorders::DrawQuad)>(injector::GetBranchDestination(pattern.get_first(0xFD)).as_int());

                // The bars' height, read from the player's camera after the checks that the view is drawn
                static auto BarsHeightHook = safetyhook::create_mid(pattern.get_first(0x84), [](SafetyHookContext& regs)
                {
                    auto& letterbox = *reinterpret_cast<float*>(regs.esp + 0x0C);
                    CutsceneBorders::fLetterbox = letterbox;
                    letterbox = CutsceneBorders::Letterbox() ? CutsceneBorders::Height(letterbox) : 0.0f;
                });

                // The pillarbox bars after the letterbox bars
                static SafetyHookInline shDrawBars = {};
                shDrawBars = safetyhook::create_inline(pattern.get_first(), static_cast<void(*)()>([]()
                {
                    CutsceneBorders::fLetterbox = 0.0f;
                    shDrawBars.unsafe_ccall();
                    CutsceneBorders::DrawPillarbox();
                }));

                // The player view's projection (ebp = view, edi = camera): without the letterbox bars both scales grow,
                // so the height between the bars fills the screen; the view's culling planes follow
                static auto ProjectionHook = safetyhook::create_mid(projection.get_first(), [](SafetyHookContext& regs)
                {
                    if (*reinterpret_cast<int32_t*>(regs.ebp + 4) != 1 || CutsceneBorders::Letterbox())
                        return;
                    float height = CutsceneBorders::Height(*reinterpret_cast<float*>(regs.edi + 0xC8));
                    if (height <= 0.0f)
                        return;
                    float zoom = 1.0f / (1.0f - 2.0f * height);
                    *reinterpret_cast<float*>(regs.esp + 0x38) *= zoom;
                    *reinterpret_cast<float*>(regs.esp + 0x28) *= zoom;
                });
            }

            // RenderMovie: the movie quad's corners and texture coordinates are pushed for the render object
            // setup call (render object in ebx), the movie's texture info is a local at esp+0x48
            static void** MoviePlayer = *hook::get_pattern<void**>("A1 ? ? ? ? 8A 48 ? 84 C9 DE F9", 1);
            pattern = hook::pattern("68 00 00 00 3F 68 00 00 00 3F 68 00 00 00 BF 68 00 00 00 BF 8B CB E8 ? ? ? ? 8B 44 24 18 8B CB");
            static auto RenderMovieHook = safetyhook::create_mid(pattern.get_first(22), [](SafetyHookContext& regs)
            {
                auto texture = *reinterpret_cast<uintptr_t*>(regs.esp + 0x48);
                if (texture && regs.ebx)
                    FitMovie(reinterpret_cast<void*>(regs.ebx), *reinterpret_cast<MovieQuad*>(regs.esp), *reinterpret_cast<int16_t*>(texture + 0x44), *reinterpret_cast<int16_t*>(texture + 0x46), *MoviePlayer != nullptr);
            });

            // RenderMovie reads a flag of the movie player even when no movie is playing; skip it then
            pattern = hook::pattern("A1 ? ? ? ? 8A 48 ? 84 C9 DE F9");
            static auto loc_5369CC = pattern.get_first(14);
            static auto MovieStartHookCrashFix = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
            {
                if (!regs.eax)
                    *(void**)(regs.esp - 4) = loc_5369CC;
            });
        };
    }
} Frontend;