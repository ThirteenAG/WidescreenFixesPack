#include "stdafx.h"
#include "cxbxr/cxbxr.h"

struct cxbxr
{
    uintptr_t begin;
    uintptr_t end;
} cxbxr;

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScale;
} Screen;

struct Point
{
    float x;
    float y;
};

struct Rect
{
    Point p1;
    Point p2;
    union
    {
        Point p3;
        uint32_t p3_x;
        uint32_t p3_y;
    };
    Point p4;

    Rect(float _p1_x, float _p1_y, float _p2_x, float _p2_y, uint32_t clr1, uint32_t clr2, float _p4_x, float _p4_y)
    {
        p1.x = _p1_x;
        p1.y = _p1_y;
        p2.x = _p2_x;
        p2.y = _p2_y;
        p3_x = clr1;
        p3_y = clr2;
        p4.x = _p4_x;
        p4.y = _p4_y;
    }

    Rect(float _p1_x, float _p1_y, float _p2_x, float _p2_y, float clr1, float clr2, float _p4_x, float _p4_y)
    {
        p1.x = _p1_x;
        p1.y = _p1_y;
        p2.x = _p2_x;
        p2.y = _p2_y;
        p3.x = clr1;
        p3.y = clr2;
        p4.x = _p4_x;
        p4.y = _p4_y;
    }
};

int32_t GetResX()
{
    if ((*(uint32_t*)0x3DC830 & 0x80u) == 0)
        return *(int32_t*)0x3DC814;
    else
        return *(int32_t*)0x3DC824;
}

int32_t GetResY()
{
    if ((*(uint32_t*)0x3DC830 & 0x80u) == 0)
        return *(int32_t*)0x3DC818;
    else
        return *(int32_t*)0x3DC828;
}

float fGetResX()
{
    return static_cast<float>(GetResX());
}

float fGetResY()
{
    return static_cast<float>(GetResY());
}

void SetTextureCoords(Point* src1, Point* src2, float a3, float a4, float a5, float a6, uint32_t color, Rect* dest)
{
    dest[0] = { src1->x, src1->y, 0.0f, 1.0f, color, color, a3, a4 };
    dest[1] = { src2->x + src1->x, src1->y, 0.0f, 1.0f, color, color, a5, a4 };
    dest[2] = { src1->x, src1->y + src2->y, 0.0f, 1.0f, color, color, a3, a6 };
    dest[3] = { src2->x + src1->x, src1->y + src2->y, 0.0f, 1.0f, color, color, a5, a6 };
    dest[4] = dest[2];
    dest[5] = dest[1];
}

void SetTextureCoords2(Point* src1, Point* src2, float a3, float a4, float a5, float a6, uint32_t color, Rect* dest)
{
    dest[0] = { src1->x - src2->x, src1->y, 0.0f, 1.0f, color, color, a3, a4 };
    dest[1] = { src1->x, src1->y - src2->y, 0.0f, 1.0f, color, color, a5, a4 };
    dest[2] = { src1->x, src2->y + src1->y, 0.0f, 1.0f, color, color, a3, a6 };
    dest[3] = { src2->x + src1->x, src1->y, 0.0f, 1.0f, color, color, a5, a6 };
    dest[4] = dest[2];
    dest[5] = dest[1];
}

void SetRotatingTextureCoords(Point* a1, Point* a2, Point* a3, Point* a4, Rect* dest)
{
    auto hud_scale = a1[0x83D];
    auto v = (a3->x - a2[1].x) / a2[1].y;

    if (v >= 0.0f)
    {
        if (v > 1.0f)
            v = 1.0f;
    }
    else
    {
        v = 0.0f;
    }

    auto sin_v = sin(v * a2->y + a2->x);
    auto cos_v = cos(v * a2->y + a2->x);

    a3[0].y = a4->x + (hud_scale.x * ((a2[2].x - (a2[4].y * cos_v)) + (a2[3].y * sin_v)));
    a3[1].x = (hud_scale.y * ((a2[2].y - (a2[4].y * sin_v)) + ((0.0f - cos_v) * a2[3].y))) + a4->y;
    a3[1].y = a4->x + (hud_scale.x * ((a2[2].x - (a2[4].x * cos_v)) + (a2[3].x * sin_v)));
    a3[2].x = (hud_scale.y * ((a2[2].y - (a2[4].x * sin_v)) + ((0.0f - cos_v) * a2[3].x))) + a4->y;
    a3[2].y = a4->x + (hud_scale.x * (((a2[4].y * cos_v) + a2[2].x) + (a2[3].y * sin_v)));
    a3[3].x = (hud_scale.y * (((a2[4].y * sin_v) + a2[2].y) + ((0.0f - cos_v) * a2[3].y))) + a4->y;
    a3[3].y = a4->x + ((((a2[4].x * cos_v) + a2[2].x) + (a2[3].x * sin_v)) * hud_scale.x);
    a3[4].x = (hud_scale.y * (((a2[4].x * sin_v) + a2[2].y) + ((0.0f - cos_v) * a2[3].x))) + a4->y;

    dest[0] = { a3[0].y, a3[1].x, 0.0f, 1.0f, a2[5].x, a2[5].x, 0.0f, 0.0f };
    dest[1] = { a3[1].y, a3[2].x, 0.0f, 1.0f, a2[5].x, a2[5].x, 1.0f, 0.0f };
    dest[2] = { a3[2].y, a3[3].x, 0.0f, 1.0f, a2[5].x, a2[5].x, 0.0f, 1.0f };
    dest[3] = { a3[3].y, a3[4].x, 0.0f, 1.0f, a2[5].x, a2[5].x, 1.0f, 1.0f };
    dest[4] = dest[2];
    dest[5] = dest[1];
}

void __fastcall SetupHUD(float* v1, void*)
{
    Point v65, v66;
    auto& unk_var1 = *(uint32_t*)0x3B7AB8;

    if (fGetResX() > fGetResY() * 2.0f)
    {
        unk_var1 = 1;

        if (fGetResX() > fGetResY() * 3.0f)
        {
            unk_var1 = 2;
        }
    }

    *(float*)0x3BC984 = fGetResX();
    *(float*)0x3BC988 = fGetResY();
    *(float*)0x3BC98C = 0.0f;
    *(float*)0x3BC990 = 0.0f;

    auto v4 = unk_var1 - 1;
    if (v4)
    {
        if (v4 == 1)
        {
            *(float*)0x3BC984 = 2.0f * fGetResX() / 3.0f;
            *(float*)0x3BC988 = fGetResY();
            *(float*)0x3BC98C = fGetResX() / 3.0f;
            *(float*)0x3BC990 = 0.0f;
        }
    }

    v1[0x102D] = fGetResX();
    v1[0x1031] = fGetResX();
    v1[0x102E] = fGetResY();
    v1[0x1032] = fGetResY();
    if (unk_var1 == 1)
    {
        v65.x = fGetResY() * 0.0016666667f;
        v66.x = fGetResX() * 0.00062499999f;
        v66.y = v65.x;
        v1[0x102F] = v66.x;
        v1[0x1030] = v65.x;
        v65.x = fGetResY() * 0.0016666667f;
        v66.x = fGetResX() * 0.00062499999f;
        v66.y = v65.x;
        v1[0x1031] = v1[0x1031] * 0.5f;
    }
    else if (unk_var1 == 2)
    {
        v65.x = fGetResY() * 0.0016666667f;
        v66.x = fGetResX() * 0.00041666668f;
        v66.y = v65.x;
        v1[0x102F] = v66.x;
        v1[0x1030] = v65.x;
        v65.x = fGetResY() * 0.0016666667f;
        v66.x = fGetResX() * 0.00041666668f;
        v66.y = v65.x;
        v1[0x1031] = v1[0x1031] * 0.33333334f;
    }
    else
    {
        v65.x = fGetResY() * 0.0016666667f;
        v66.x = fGetResX() * 0.00125f;
        v66.y = v65.x;
        v1[0x102F] = v66.x / Screen.fHudScale;
        v1[0x1030] = v65.x;
        v65.x = fGetResY() * 0.0016666667f;
        v66.x = fGetResX() * 0.00125f;
        v66.y = v65.x;
    }

    v1[0x107A] = v66.x / Screen.fHudScale;
    v66.x = 1.0f;
    v66.y = 0.90909088f;
    v1[0x107B] = v66.y;
    v1[0x107A] = 1.0f / Screen.fHudScale;
    v1[0x107B] = 0.90909088f;
    float v36 = v1[0x107B] * 160.0f;
    float v37 = (float)(*(float*)0x3BC984 - (float)(v1[0x107A] * 160.0f)) - 24.0f;
    v65.x = v1[0x107A] * 160.0f;
    v65.y = v36;
    v66.x = v37;
    v66.y = (float)(*(float*)0x3BC988 - v36) - 36.0f;
    auto v38 = v1 + 0x107C;
    v38[0] = v37;
    v38[1] = (float)(*(float*)0x3BC988 - v36) - 36.0f;
    SetTextureCoords((Point*)v1 + 0x83E, &v65, 0.37254903f, 0.0078431377f, 1.0f, 0.63529414f, 0xFFFFFFFF, (Rect*)0x3B7028);


    auto v40 = v1[0x107D] + (float)(v1[0x107B] * 55.0f);
    auto v41 = v1[0x107C] + (float)(v1[0x107A] * 69.0f);
    auto v42 = (float)(v1[0x107A] * 90.0f) + v1[0x107C];
    auto v43 = (float)(v1[0x1078] + 43.0f) - 2.0f;
    v66.y = (float)(v1[0x107D] + (float)(v1[0x107B] * 68.0f)) - v40;
    v66.x = v42 - v41;
    v65.x = v41;
    v65.y = v40;
    SetTextureCoords(&v65, &v66, (v1[0x1078] + 22.0f) * 0.0039215689f, 0.78039217f, v43 * 0.0039215689f, 0.82352942f, 0x70FFFFFF, (Rect*)0x3B7568);


    v66.x = v1[0x107A] * 10.0f;
    v66.y = v1[0x107B] * 10.0f;
    v65.x = (float)(v1[0x107A] * 55.0f) + v1[0x107C];
    v65.y = v1[0x107D] + (v1[0x107B] * 93.0f);
    SetTextureCoords(&v65, &v66, 0.0039215689f, 0.0039215689f, 0.70588237f, 0.70588237f, 0x70FFFFFF, (Rect*)0x3B73E8);


    v66.x = v1[0x107A] * 10.0f;
    v66.y = v1[0x107B] * 10.0f;
    v65.x = v1[0x107C] + (v1[0x107A] * 65.0f);
    v65.y = v1[0x107D] + (float)(v1[0x107B] * 93.0f);
    SetTextureCoords(&v65, &v66, 0.0039215689f, 0.0039215689f, 0.70588237f, 0.70588237f, 0x70FFFFFF, (Rect*)0x3B7328);


    v66.x = v1[0x107A] * 10.0f;
    v66.y = v1[0x107B] * 10.0f;
    v65.x = (float)(v1[0x107A] * 75.0f) + v1[0x107C];
    v65.y = v1[0x107D] + v1[0x107B] * 93.0f;
    SetTextureCoords(&v65, &v66, 0.0039215689f, 0.0039215689f, 0.70588237f, 0.70588237f, 0x70FFFFFF, (Rect*)0x3B7268);



    v66.x = v1[0x107A] * 10.0f;
    v66.y = v1[0x107B] * 10.0f;
    v65.x = (float)(v1[4218] * 85.0f) + v1[0x107C];
    v65.y = v1[0x107D] + (v1[0x107B] * 93.0f);
    SetTextureCoords(&v65, &v66, 0.0039215689f, 0.0039215689f, 0.70588237f, 0.70588237f, 0x70FFFFFF, (Rect*)0x3B71A8);


    v66.x = v1[0x107A] * 10.0f;
    v66.y = v1[0x107B] * 10.0f;
    v65.x = v1[0x107C] + (v1[0x107A] * 95.0f);
    v65.y = v1[0x107D] + (float)(v1[0x107B] * 93.0f);
    SetTextureCoords(&v65, &v66, 0.0039215689f, 0.0039215689f, 0.70588237f, 0.70588237f, 0x70FFFFFF, (Rect*)0x3B70E8);


    v66.x = v1[0x107A] * 20.0f;
    v66.y = v1[0x107B] * 20.0f;
    v65.x = (float)((float)(v1[0x107A] * -20.0f) + *(float*)0x3BC984) - 24.0f;
    v65.y = (float)((float)(v1[0x107B] * -170.0f) + *(float*)0x3BC988) - 36.0f;
    SetTextureCoords2(&v65, &v66, 0.13725491f, 0.45490196f, 0.24705882f, 0.56470591f, 0xFFFFFFFF, (Rect*)0x3B74A8);
    SetRotatingTextureCoords((Point*)v1, (Point*)0x31860C, (Point*)v1 + 0x82E, (Point*)v1 + 0x83E, (Rect*)0x3B76F0);
    SetRotatingTextureCoords((Point*)v1, (Point*)0x318664, (Point*)v1 + 0x837, (Point*)v1 + 0x83E, (Rect*)0x3B77B0);


    //clock texture
    auto& ptr = *(Point*)&v1[0x1036];
    ptr.x = ((v1[0x107A] * -86.0f) - 24.0f) + *(float*)0x3BC984;
    ptr.y = (v1[0x107B] * 0.0f) + 36.0f;
    v66.x = v1[0x107A] * 86.0f;
    v66.y = v1[0x107B] * 105.0f;
    SetTextureCoords(&ptr, &v66, 0.0039215689f, 0.0039215689f, 0.35294119f, 0.43921569f, 0xFFFFFFFF, (Rect*)0x3B52B0);

    SetRotatingTextureCoords((Point*)v1, (Point*)0x3186BC, (Point*)v1 + 0x81D, (Point*)v1 + 0x81B, (Rect*)0x3B5370);
    SetRotatingTextureCoords((Point*)v1, (Point*)0x318690, (Point*)(v1 + 0x1043), (Point*)v1 + 0x81B, (Rect*)0x3B5430);
    SetRotatingTextureCoords((Point*)v1, (Point*)0x3186E8, (Point*)v1 + 0x826, (Point*)v1 + 0x81B, (Rect*)0x3B54F0);

    v1[0x1099] = fGetResX() * 0.5f;
    v1[0x109A] = fGetResY() * 0.5f;

    v66.x = v1[0x107A] * 10.0f;
    v66.y = v1[0x107B] * 24.0f;
    v65.x = v1[0x107A] * 145.0f;
    v65.y = v1[0x107B] * 569.0f;
    SetTextureCoords(&v65, &v66, 0.83137256f, 0.30980393f, 0.87450981f, 0.40784314f, 0xFFFFFFFF, (Rect*)0x3B7870);

    v66.x = v1[0x107A] * 16.0f;
    v66.y = v1[0x107B] * 25.0f;
    v65.x = v1[0x107A] * 55.0f;
    v65.y = v1[0x107B] * 567.0f;
    SetTextureCoords(&v65, &v66, 0.88627452f, 0.30980393f, 0.94901961f, 0.40784314f, 0xFFFFFFFF, (Rect*)0x3B7930);

    v66.x = v1[0x107A] * 30.0f;
    v66.y = v1[0x107B] * 30.0f;
    v65.x = v1[0x107A] * 15.0f;
    v65.y = v1[0x107B] * 500.0f;
    SetTextureCoords(&v65, &v66, 0.70980394f, 0.44313726f, 0.83137256f, 0.56470591f, 0xFFFFFFFF, (Rect*)0x3B79F0);
}

void Init()
{
    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudScale = Screen.fAspectRatio / (4.0f / 3.0f);

    struct AspectRatioHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)(regs.ebx + 0x1A4) *= Screen.fHudScale;
            regs.eax = *(uint32_t*)(regs.ebx + 0x3E0);
        }
    }; injector::MakeInline<AspectRatioHook>(0x23DFC3, 0x23DFC3 + 6);

    static float fFOV = GetFOV2(0.5f, Screen.fAspectRatio);
    injector::WriteMemory(0x23DED7 + 4, &fFOV, true);

    injector::MakeCALL(0x1524B1, SetupHUD, true);

    //loadsc + menu
    //float t = Screen.fAspectRatio / ((640.0f - 30.0f) / 480.0f);
    //injector::WriteMemory<float>(0x2FBCA4, 0.0017089844 / t, true);
    //injector::WriteMemory<float>(0x2F4DB0, 160.0 / 2.0f, true);

    //injector::MakeRET(0x140EF0, 0x24, true);
}

CEXP void InitializeASI()
{
    auto pid = getParentPID(GetCurrentProcessId());
    auto name = getProcessName(pid).stem();
    if (name == "cxbx" || name == "cxbxr-ldr")
    {
        std::tie(Screen.Width, Screen.Height) = getWindowDimensions(pid);
        std::call_once(CallbackHandler::flag, []()
        {
            cxbxr.begin = (uintptr_t)GetModuleHandle(nullptr);
            PIMAGE_DOS_HEADER dosHeader = (IMAGE_DOS_HEADER*)(cxbxr.begin + 0);
            PIMAGE_NT_HEADERS ntHeader = (IMAGE_NT_HEADERS*)(cxbxr.begin + dosHeader->e_lfanew);
            cxbxr.end = cxbxr.begin + ntHeader->OptionalHeader.SizeOfImage;

            CallbackHandler::RegisterCallback(Init, hook::range_pattern(cxbxr.begin, cxbxr.end, "D8 74 24 10 8B 83"));
        });
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent())
        {
            InitializeASI();
        }
    }
    return TRUE;
}