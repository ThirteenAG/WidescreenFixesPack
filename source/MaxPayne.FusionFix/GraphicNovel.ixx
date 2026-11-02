module;

#include <stdafx.h>

export module GraphicNovel;

import ComVars;
import Camera;

// Graphic novel pages are 3D scenes, meshes and a camera to show them with, which the page copies to
// the camera of the graphic novel mode when it shows. Original framing keeps that camera: the page
// across the 4:3 width at the top, the playback controls below it. Otherwise the camera moves to the
// middle of the page and zooms in as far as the whole page, its black border included, still fits
// on the screen.
export namespace MaxPayne_GraphicNovelPage
{
    enum
    {
        OBJECT_ANIMATION = 0x2B, // KF2::KF_ObjectAnimation*, the meshes of the page
        CAMERA_MATRIX = 0x33,    // M_Matrix4x3
        FOV = 0x6B,
    };

    // The page shown and the camera showing it, cleared when the graphic novel mode ends
    uint8_t* pPage = nullptr;
    uint8_t* pCamera = nullptr;

    // The page as seen with its own camera
    struct Extents
    {
        float fCenterX; // how far the camera moves along its right and up axes to face the middle of the page
        float fCenterY;
        float fTanX;    // tangents of half the page width and height from there
        float fTanY;
    };
    std::optional<Extents> PageExtents;

    uint32_t nGeneration = 0;

    // From the bounding boxes of the meshes, the page only animates how opaque they are
    std::optional<Extents> Measure(uint8_t* page, uint8_t* camera)
    {
        auto pAnimation = *(uint8_t**)(page + OBJECT_ANIMATION);
        if (!pAnimation)
            return std::nullopt;

        P_BaseObject::calculateObjectToWorldMatrix(camera, nullptr);
        auto pView = (const float*)(camera + P_BaseObject::WORLD_MATRIX);

        struct Point { float x, y, z; };
        std::vector<Point> Points;
        for (uint32_t i = 0, nMeshes = KF_ObjectAnimation::getTotalMeshes(pAnimation); i < nMeshes; ++i)
        {
            auto pMesh = KF_ObjectAnimation::getMesh(pAnimation, i);
            if (!pMesh)
                continue;

            auto pMin = P_BaseObject::getBoundingBoxMin(pMesh);
            auto pMax = P_BaseObject::getBoundingBoxMax(pMesh);
            if (!(pMin[0] <= pMax[0] && pMin[1] <= pMax[1] && pMin[2] <= pMax[2]))
                continue;

            P_BaseObject::calculateObjectToWorldMatrix(pMesh, nullptr);
            auto pWorld = (const float*)(pMesh + P_BaseObject::WORLD_MATRIX);
            for (int nCorner = 0; nCorner < 8; ++nCorner)
            {
                float local[3] = { (nCorner & 1) ? pMax[0] : pMin[0], (nCorner & 2) ? pMax[1] : pMin[1], (nCorner & 4) ? pMax[2] : pMin[2] };
                float relative[3];
                for (int k = 0; k < 3; ++k)
                    relative[k] = local[0] * pWorld[k] + local[1] * pWorld[3 + k] + local[2] * pWorld[6 + k] + pWorld[9 + k] - pView[9 + k];

                auto Dot = [&](const float* pAxis) { return relative[0] * pAxis[0] + relative[1] * pAxis[1] + relative[2] * pAxis[2]; };
                Point point = { Dot(pView), Dot(pView + 3), Dot(pView + 6) };
                if (!(point.z > 0.0f))
                    return std::nullopt;
                Points.push_back(point);
            }
        }

        if (Points.empty())
            return std::nullopt;

        constexpr float fInfinity = std::numeric_limits<float>::infinity();
        float fMinX = fInfinity, fMaxX = -fInfinity, fMinY = fInfinity, fMaxY = -fInfinity, fDepth = 0.0f;
        for (auto& point : Points)
        {
            fMinX = std::min(fMinX, point.x / point.z);
            fMaxX = std::max(fMaxX, point.x / point.z);
            fMinY = std::min(fMinY, point.y / point.z);
            fMaxY = std::max(fMaxY, point.y / point.z);
            fDepth += point.z;
        }
        fDepth /= Points.size();

        // The page is flat and faces the camera, the depths only differ by its layers
        Extents extents = { (fMinX + fMaxX) * 0.5f * fDepth, (fMinY + fMaxY) * 0.5f * fDepth, 0.0f, 0.0f };
        for (auto& point : Points)
        {
            extents.fTanX = std::max(extents.fTanX, std::abs(point.x - extents.fCenterX) / point.z);
            extents.fTanY = std::max(extents.fTanY, std::abs(point.y - extents.fCenterY) / point.z);
        }

        if (!std::isfinite(extents.fCenterX) || !std::isfinite(extents.fCenterY) || !std::isfinite(extents.fTanX) || !std::isfinite(extents.fTanY) || extents.fTanX <= 0.0f || extents.fTanY <= 0.0f)
            return std::nullopt;
        return extents;
    }

    void Apply()
    {
        if (!pPage || !pCamera)
            return;

        nGeneration = Screen.nGeneration;

        // the page's camera, as MaxPayne_GraphicNovelPage::show sets it
        auto pMatrix = (float*)(pCamera + P_BaseObject::LOCAL_MATRIX);
        std::memcpy(pMatrix, pPage + CAMERA_MATRIX, 12 * sizeof(float));
        float fPageFOV = *(float*)(pPage + FOV);
        float fFOV = fPageFOV;
        if (!Screen.bGraphicNovelMode && PageExtents)
        {
            for (int k = 0; k < 3; ++k)
                pMatrix[9 + k] += pMatrix[k] * PageExtents->fCenterX + pMatrix[3 + k] * PageExtents->fCenterY;

            // P_Camera::validate widens the tangents for the screen
            float fTan = std::max(PageExtents->fTanX / Screen.fAspectScaleX, PageExtents->fTanY / (P_Camera::GetViewportRatio(pCamera) * Screen.fAspectScaleY));
            fFOV = 2.0f * atanf(fTan);
        }

        P_BaseObject::invalidateMatrices(pCamera, nullptr);
        P_Camera::setFOV(pCamera, nullptr, fFOV);
    }

    SafetyHookInline shShow = {};
    void __fastcall show(uint8_t* _this, void* edx, uint8_t* camera)
    {
        shShow.unsafe_fastcall(_this, edx, camera);
        if (!P_BaseObject::invalidateMatrices || !P_BaseObject::calculateObjectToWorldMatrix || !P_Camera::setFOV)
            return;

        pPage = _this;
        pCamera = camera;
        PageExtents = Measure(_this, camera);
        Apply();
    }

    // For the key toggling original framing
    void Refresh()
    {
        Apply();
    }

    // Called every frame in the graphic novel mode
    void Update()
    {
        if (pPage && nGeneration != Screen.nGeneration)
            Apply();
    }

    void Reset()
    {
        pPage = nullptr;
        pCamera = nullptr;
        PageExtents.reset();
    }
}

class GraphicNovel
{
public:
    GraphicNovel()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("55 8B EC 83 EC 0C 53 56 8B F1 57 89 75 F4 E8 ? ? ? ? 8B 45 08 83 C6 33 83 C0 2C"); // MaxPayne_GraphicNovelPage::show
            MaxPayne_GraphicNovelPage::shShow = safetyhook::create_inline(pattern.get_first(), MaxPayne_GraphicNovelPage::show); //0x4A8860
        };
    }
} GraphicNovel;
