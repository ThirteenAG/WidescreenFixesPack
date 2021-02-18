#include "stdafx.h"
#include "dxsdk\d3d8.h"

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    int32_t video_mode;
    bool isMenu;
    int32_t* dword_A8F0C0;
    int32_t* dword_A8F0C4;
    int32_t* dword_A8F0C8;
    int32_t* dword_A8F0CC;
    bool bFix2D;
    bool bDrawPillarboxes;
    bool bDisableCutsceneBorders;
} Screen;

class CRect
{
public:
    float m_fLeft;
    float m_fBottom;
    float m_fRight;
    float m_fTop;

    bool operator==(const CRect& rect)
    {
        return ((*(uint32_t*)&this->m_fLeft == *(uint32_t*)&rect.m_fLeft) && (*(uint32_t*)&this->m_fBottom == *(uint32_t*)&rect.m_fBottom) &&
                (*(uint32_t*)&this->m_fRight == *(uint32_t*)&rect.m_fRight) && (*(uint32_t*)&this->m_fTop == *(uint32_t*)&rect.m_fTop));
    }

    bool operator==(CRect& rect)
    {
        return ((*(uint32_t*)&this->m_fLeft == *(uint32_t*)&rect.m_fLeft) && (*(uint32_t*)&this->m_fBottom == *(uint32_t*)&rect.m_fBottom) &&
                (*(uint32_t*)&this->m_fRight == *(uint32_t*)&rect.m_fRight) && (*(uint32_t*)&this->m_fTop == *(uint32_t*)&rect.m_fTop));
    }

    inline CRect() {}
    inline CRect(float a, float b, float c, float d)
        : m_fLeft(a), m_fBottom(b), m_fRight(c), m_fTop(d)
    {}
    inline CRect(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
        : m_fLeft(*(float*)&a), m_fBottom(*(float*)&b), m_fRight(*(float*)&c), m_fTop(*(float*)&d)
    {}
};

void DrawRect(LPDIRECT3DDEVICE8 pDevice, int32_t x, int32_t y, int32_t w, int32_t h, D3DCOLOR color = D3DCOLOR_XRGB(0, 0, 0))
{
    D3DRECT BarRect = { x, y, x + w, y + h };
    pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 0, 0);
}

HWND g_hWnd;
HWND g_hWndInsertAfter;
UINT g_uFlags;
BOOL WINAPI SetWindowPosHook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    g_hWnd = hWnd;
    g_hWndInsertAfter = hWndInsertAfter;
    g_uFlags = uFlags;
    tagRECT rc;
    auto [DesktopResW, DesktopResH] = GetDesktopRes();
    rc.left = (LONG)(((float)DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
    rc.top = (LONG)(((float)DesktopResH / 2.0f) - (Screen.nHeight / 2.0f));
    rc.right = Screen.nWidth;
    rc.bottom = Screen.nHeight;
    return ::SetWindowPos(hWnd, hWndInsertAfter, rc.left, rc.top, rc.right, rc.bottom, uFlags);
}

static auto GetRes = []()
{
    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
};

std::vector<uint32_t> xrefs_hud;
std::vector<uint32_t> xrefs_fullscreen;

///////////////////////
uint8_t* byte_104C8CFC;
uint32_t dword_A909F9;
uint32_t dword_65B45C;
uint32_t dword_544758;
uint32_t dword_4B8EC7;
char in_menu;
void __declspec(naked) menu_check()
{
    _asm
    {
        mov eax, dword_A909F9
        cmp[eax], 0
        je label1
        mov eax, dword_65B45C
        cmp[eax], 0
        jne label1
        mov in_menu, 1
        push 0xFF
        push dword_544758
        jmp dword_4B8EC7
        label1 :
        mov in_menu, 0
        ret 8
    }
}
//CPatch::RedirectJump(0x4B8EC0, menu_check);
////////////////////////

float* dword_A909E4;
float* dword_A909E8;
float* dword_A909EC;
float* dword_A909F0;
uint32_t* dword_A90A1C;
void __stdcall sub_4B6940(float a1, float a2, float a3)
{
    static void* stack[6];
    CaptureStackBackTrace(0, 3, stack, NULL);

    static auto GetModuleName = [](uint32_t addr) -> std::string
    {
        HMODULE hm = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)addr, &hm);
        const std::string moduleFileName = GetModulePath<std::string>(hm);
        return moduleFileName.substr(moduleFileName.find_last_of("/\\") + 1);
    };

    auto module_name = GetModuleName((uint32_t)stack[1]);
    auto module_base = ((uint32_t)GetModuleHandleA(module_name.c_str()));
    std::transform(module_name.begin(), module_name.end(), module_name.begin(), tolower);

#if 0
    auto xref1 = (uint32_t)(stack[1]) - module_base;

    if (module_name == "gcore.dll" || module_name == "g_rhapsody.sgl")
        xref1 += 0x10000000;
    else if (module_name == "gettingup.exe")
        xref1 += 0x400000;

    uint32_t xrefs_hud[] =
    {
        0x100E4F04, //red weapon icon half circle bar
        0x100E4F70, //red weapon icon half circle bar
        0x100E4FD6, //red weapon icon half circle bar
        0x100E5042, //red weapon icon half circle bar
        0x102C7DB8, //weapon icons
        0x102C7E21, //weapon icons
        0x102C7E8B, //weapon icons
        0x102C7EF5, //weapon icons
        0x102C8CEA, //weapon icons background
        0x102C8D53, //weapon icons background
        0x102C8DBD, //weapon icons background
        0x102C8E27, //weapon icons background
    };

    uint32_t xrefs_fullscreen[] =
    {
        0x004e51da,
        0x004e5249,
        0x004e516b,
        0x004e52c5,
        0x004e516b,
    };
#endif

    auto xref1 = (uint32_t)(stack[1]);

    if (!(std::end(xrefs_hud) == std::find(std::begin(xrefs_hud), std::end(xrefs_hud), xref1)))
    {
        //scaling handled in postrenderhook
        a1 += (Screen.fWidth - 0.0f) / 2.0f;
        a2 += (Screen.fHeight - 0.0f) / 2.0f;
    }
    else
    {
        //DBGONLY(KEYPRESS(VK_F2) { spd::log()->info(int_to_hex_str(xref1)); });

        if ((std::end(xrefs_fullscreen) == std::find(std::begin(xrefs_fullscreen), std::end(xrefs_fullscreen), xref1)))
        {
            if (Screen.bFix2D)
            {
                a1 /= Screen.fHudScale;
                a1 += Screen.fHudOffsetReal;
            }
        }
    }

    dword_A909E4[7 * *dword_A90A1C] = a1;
    dword_A909E8[7 * *dword_A90A1C] = a2;
    dword_A909EC[7 * *dword_A90A1C] = a3;
    dword_A909F0[7 * *dword_A90A1C] = 1.0f;
}

void __cdecl TGORender__QueryResolutionHook(int32_t* x, int32_t* y)
{
    *x = 0; //640
    *y = 0; //480
}

void Init()
{
    CIniReader iniReader(".\\videomode.ini");
    Screen.nWidth = iniReader.ReadInteger("Settings", "Width", 0);
    Screen.nHeight = iniReader.ReadInteger("Settings", "Height", 0);
    Screen.bFix2D = iniReader.ReadInteger("Settings", "WidescreenUIPatch", 1) != 0;
    Screen.bDrawPillarboxes = iniReader.ReadInteger("Settings", "DrawPillarboxes", 1) != 0;
    Screen.bDisableCutsceneBorders = iniReader.ReadInteger("Settings", "DisableCutsceneBorders", 0) != 0;
    static int32_t nToggleHudKey = iniReader.ReadInteger("MAIN", "ToggleHudKey", VK_F2);

    if (!Screen.nWidth || !Screen.nHeight)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();

    GetRes();

    //Default Windowed Res Hook
    auto pattern = hook::pattern("FF 15 ? ? ? ? 8B 47 08 A3");
    injector::MakeNOP(pattern.get_first(0), 6, true);
    injector::MakeCALL(pattern.get_first(0), SetWindowPosHook, true);

    //windowed mode crash in gameplay workaround
    pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 53 56 57 8B F1");
    static auto sub_4B8EC0 = (uint32_t)pattern.get_first(0);
    dword_4B8EC7 = (uint32_t)pattern.get_first(7);
    dword_544758 = *pattern.get_first<uint32_t>(3);
    pattern = hook::pattern("89 88 ? ? ? ? 8B 15 ? ? ? ? 8B 44 24 08 6B D2 1C 89 82");
    dword_A909F9 = (uint32_t)(*pattern.count(4).get(1).get<void*>(2)) + 1;
    dword_65B45C = (uint32_t)(*hook::get_pattern<void*>("B8 ? ? ? ? 57 8B FF", 1)) + 0x564;
    pattern = hook::pattern("8B 4C 24 10 5F 5E B0 01 5B 64 89 0D ? ? ? ? 83 C4 10 C2 08 00");
    struct InitiatingWindowModeHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.esp + 0x10);
            regs.eax = 1;
            //injector::MakeRET(sub_4B8EC0, 8, true);
            injector::MakeJMP(sub_4B8EC0, menu_check, true);
        }
    }; injector::MakeInline<InitiatingWindowModeHook>(pattern.get_first(0), pattern.get_first(8));
    injector::WriteMemory<uint16_t>(pattern.get_first(5), 0x5E5F, true); // pop edi pop esi

    //Default Fullscreen Res Hook
    pattern = hook::pattern("A3 ? ? ? ? 8B 4F 0C");
    Screen.dword_A8F0C8 = *pattern.get_first<int32_t*>(1);
    struct FullscreenResXHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.edi + 8) = Screen.nWidth;
            *Screen.dword_A8F0C8 = Screen.nWidth;
        }
    }; injector::MakeInline<FullscreenResXHook1>(pattern.get_first(0));

    pattern = hook::pattern("89 0D ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 75 27 8B CE 88 1D");
    Screen.dword_A8F0CC = *pattern.get_first<int32_t*>(2);
    struct FullscreenResYHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.edi + 0x0C) = Screen.nHeight;
            *Screen.dword_A8F0CC = Screen.nHeight;
        }
    }; injector::MakeInline<FullscreenResYHook1>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("A3 ? ? ? ? 8B 86 ? ? ? ? 8D 14 40 8B 44 D1 04");
    struct FullscreenResXHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ecx + regs.edx * 8) = Screen.nWidth;
            *Screen.dword_A8F0C8 = Screen.nWidth;
        }
    }; injector::MakeInline<FullscreenResXHook2>(pattern.get_first(0));

    pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 85 C0 BF");
    struct FullscreenResYHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ecx + regs.edx * 8 + 4) = Screen.nHeight;
            *Screen.dword_A8F0CC = Screen.nHeight;
        }
    }; injector::MakeInline<FullscreenResYHook2>(pattern.get_first(0));

    //
    pattern = hook::pattern("89 15 ? ? ? ? 89 3D ? ? ? ? 8B 08");
    Screen.dword_A8F0C0 = *pattern.get_first<int32_t*>(2);
    struct FullscreenResXHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            *Screen.dword_A8F0C0 = Screen.nWidth;
        }
    }; injector::MakeInline<FullscreenResXHook3>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("89 0D ? ? ? ? EB 1D");
    struct FullscreenResXHook4
    {
        void operator()(injector::reg_pack& regs)
        {
            *Screen.dword_A8F0C0 = Screen.nWidth;
        }
    }; injector::MakeInline<FullscreenResXHook4>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("89 0D ? ? ? ? 8B 40 0C");
    struct FullscreenResXHook5
    {
        void operator()(injector::reg_pack& regs)
        {
            *Screen.dword_A8F0C0 = Screen.nWidth;
        }
    }; injector::MakeInline<FullscreenResXHook5>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("89 3D ? ? ? ? 8B 08");
    Screen.dword_A8F0C4 = *pattern.get_first<int32_t*>(2);
    struct FullscreenResYHook3
    {
        void operator()(injector::reg_pack& regs)
        {
            *Screen.dword_A8F0C4 = Screen.nHeight;
        }
    }; injector::MakeInline<FullscreenResYHook3>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("A3 ? ? ? ? 89 3D");
    struct FullscreenResYHook4
    {
        void operator()(injector::reg_pack& regs)
        {
            *Screen.dword_A8F0C4 = Screen.nHeight;
        }
    }; injector::MakeInline<FullscreenResYHook4>(pattern.get_first(0));

    //Default dimensions overwrite !BREAKS SOME LEVELS!
    //pattern = hook::pattern("8B 0D ? ? ? ? A1 ? ? ? ? 8D 5C 24 2C");
    //injector::WriteMemory(pattern.get_first(2), &Screen.nHeight, true);
    //injector::WriteMemory(pattern.get_first(7), &Screen.nWidth, true);

    pattern = hook::pattern("8B 08 89 4C 24 2C 8B 48 04 89 4C 24 30 38 9E ? ? ? ? 75 17 B9");
    struct FullscreenResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.esp + 0x2C) = Screen.nWidth;
            *(int32_t*)(regs.esp + 0x30) = Screen.nHeight;
        }
    }; injector::MakeInline<FullscreenResHook>(pattern.get_first(0), pattern.get_first(13));

    //BINK
    pattern = hook::pattern("8B 42 04 8B 48 14 51 8D 54 24 18 6A 04 52 E8 ? ? ? ? B8 ? ? ? ? 81 C4 ? ? ? ? C3");
    struct BinkVideoHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.edx + 0x04);
            regs.ecx = *(uint32_t*)(regs.eax + 0x14);

            if (Screen.bFix2D)
            {
                *(float*)(regs.esp + 0x14) += Screen.fHudOffsetReal;
                *(float*)(regs.esp + 0x44) += Screen.fHudOffsetReal;
                *(float*)(regs.esp + 0x2C) -= Screen.fHudOffsetReal;
                *(float*)(regs.esp + 0x5C) -= Screen.fHudOffsetReal;
            }
        }
    }; injector::MakeInline<BinkVideoHook>(pattern.get_first(0), pattern.get_first(6));

#ifdef _DEBUG
    //Do not pause on minimize
    pattern = hook::pattern("32 DB E8 ? ? ? ? 8B 4C 24 14 55 57 56 51");
    injector::WriteMemory<uint16_t>(pattern.get_first(0), 0x01B3, true); //mov bl,01
#endif

#ifdef _DEBUG
    //registry debugger crash
    pattern = hook::pattern("E8 ? ? ? ? 57 57 8D 8D ? ? ? ? 51 8D 95 ? ? ? ? 52");
    injector::MakeNOP(pattern.get_first(0), 5, true);
#endif

    //Interface scaling
    pattern = hook::pattern("A1 ? ? ? ? 8B 4C 24 04 6B C0 1C 89 88 ? ? ? ? 8B 15 ? ? ? ? 8B 44 24 08 6B D2 1C 89 82 ? ? ? ? 8B 0D ? ? ? ? 8B 54 24 0C 6B C9 1C 89 91 ? ? ? ? A1");
    dword_A90A1C = *pattern.get_first<uint32_t*>(1);
    dword_A909E4 = *pattern.get_first<float*>(14);
    dword_A909E8 = dword_A909E4 + 1;
    dword_A909EC = dword_A909E4 + 2;
    dword_A909F0 = dword_A909E4 + 3;
    injector::MakeJMP(pattern.get_first(0), sub_4B6940, true);

    //D3D8 EndScene Hook
    static uint16_t oldState = 0;
    static uint16_t curState = 0;
    pattern = hook::pattern("A1 ? ? ? ? 8B 10 50 FF 92");
    static LPDIRECT3DDEVICE8* pDevice = *pattern.get_first<LPDIRECT3DDEVICE8*>(1);
    struct EndSceneHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)pDevice;
            regs.edx = *(uint32_t*)(regs.eax);

            if (Screen.bFix2D)
            {
                if (Screen.bDrawPillarboxes && *(uint32_t*)dword_65B45C == 0)
                {
                    DrawRect(*pDevice, 0, 0, static_cast<int32_t>(Screen.fHudOffsetReal), Screen.nHeight);
                    DrawRect(*pDevice, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal), 0, static_cast<int32_t>(Screen.fWidth43 + Screen.fHudOffsetReal + Screen.fHudOffsetReal), Screen.nHeight);
                }
            }

            if (nToggleHudKey)
            {
                curState = GetAsyncKeyState(nToggleHudKey);
                if (!curState && oldState)
                    *byte_104C8CFC = !*byte_104C8CFC;
                oldState = curState;
            }
        }
    }; injector::MakeInline<EndSceneHook>(pattern.get_first(0), pattern.get_first(7));

    //xrefs
    pattern = hook::pattern("8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 8B 01 FF 50 1C 8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 8B 01 33 D2");
    xrefs_fullscreen.push_back((uint32_t)pattern.get_first(0)); //0x004e51da
    pattern = hook::pattern("8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 8B 01 FF 50 1C 8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 33 D2 8A 56 03 8B 01");
    xrefs_fullscreen.push_back((uint32_t)pattern.count(2).get(0).get<uint32_t>(0)); //0x004e516b
    xrefs_fullscreen.push_back((uint32_t)pattern.count(2).get(1).get<uint32_t>(0)); //0x004e5249
    pattern = hook::pattern("E9 ? ? ? ? 8B 0D ? ? ? ? 8B 51 38");
    xrefs_fullscreen.push_back((uint32_t)pattern.get_first(0)); //0x004e52c5
    pattern = hook::pattern("8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 8B 01 FF 50 1C 8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 33 D2 8A 56 03 8B 01 52");
    xrefs_fullscreen.push_back((uint32_t)pattern.get_first(0)); //0x004e516b
}

void InitGCore()
{
    static std::vector<std::string> list;
    GetResolutionsList(list);
    auto iniRes = format("%dx%d", Screen.nWidth, Screen.nHeight);
    Screen.video_mode = 0;
    int32_t i = 0;
    for (auto& res : list)
    {
        if (res == iniRes)
        {
            Screen.video_mode = i;
            break;
        }
        ++i;
    }

    auto pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "75 09 8B F0 BF ? ? ? ? EB 3E 3D");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (regs.ebp == 0x26 || regs.ebp == 0x27)
            {
                if (Screen.isMenu)
                {
                    if (regs.ebp == 0x26)
                        Screen.video_mode--;
                    else if (regs.ebp == 0x27)
                        Screen.video_mode++;

                    if (Screen.video_mode >= (int32_t)list.size())
                        Screen.video_mode = 0;
                    else if (Screen.video_mode < 0)
                        Screen.video_mode = (int32_t)list.size() - 1;
                }

                sscanf_s(list[Screen.video_mode].c_str(), "%dx%d", &Screen.nWidth, &Screen.nHeight);
                SetWindowPosHook(g_hWnd, g_hWndInsertAfter, 0, 0, 0, 0, g_uFlags);
                GetRes();
                CIniReader iniWriter(".\\videomode.ini");
                iniWriter.WriteInteger("Settings", "Width", Screen.nWidth);
                iniWriter.WriteInteger("Settings", "Height", Screen.nHeight);
                Screen.isMenu = false;
            }

            *Screen.dword_A8F0C0 = Screen.nWidth;
            *Screen.dword_A8F0C4 = Screen.nHeight;
            *Screen.dword_A8F0C8 = Screen.nWidth;
            *Screen.dword_A8F0CC = Screen.nHeight;
            regs.esi = Screen.nWidth;
            regs.edi = Screen.nHeight;
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(9));

#ifdef _DEBUG
    //Fullscreen res change to windowed (in menu selector) (hack!)
    //pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "74 0B 57 56 FF 52 38 5F 5E 83 C4 0C C3 8D 44 24 08 50 8D 44 24 10");
    //injector::MakeNOP(pattern.get_first(0), 2, true);
#endif

    //Interface
#if 0
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "D9 44 24 54 D8 C9 D8 0D ? ? ? ? D9 5C 24 54 DB 44 24 28 D9 44 24 58 D8 C9");
    struct TGuiTextureRendererRenderHook
    {
        void operator()(injector::reg_pack& regs)
        {
#ifdef _DEBUG
            float x1 = *(float*)(regs.esp + 0x54);
            float y1 = *(float*)(regs.esp + 0x58);
            float x2 = *(float*)(regs.esp + 0x5C);
            float y2 = *(float*)(regs.esp + 0x60);
#endif
            static float temp = 0.0f;
            if (Screen.bFix2D)
            {
                CRect rect(*(float*)(regs.esp + 0x54), *(float*)(regs.esp + 0x58), *(float*)(regs.esp + 0x5C), *(float*)(regs.esp + 0x60));

                if (rect == CRect(0.0f, 375.0f, 640.0f, 430.0f) || // menu black overlay texture
                        rect == CRect(0.0f, 430.0f, 640.0f, 485.0f)    //menu black overlay texture
                   )
                {
                    *(float*)(regs.esp + 0x54) = *(float*)(regs.esp + 0x54) * Screen.fWidth * ((1.0f / 640.0f));
                    *(float*)(regs.esp + 0x58) = *(float*)(regs.esp + 0x58) * Screen.fHeight * (1.0f / 480.0f);
                    *(float*)(regs.esp + 0x5C) = Screen.fWidth * *(float*)(regs.esp + 0x5C) * ((1.0f / 640.0f));
                    *(float*)(regs.esp + 0x60) = Screen.fHeight * *(float*)(regs.esp + 0x60) * (1.0f / 480.0f);
                }
                else
                {
                    *(float*)(regs.esp + 0x54) = *(float*)(regs.esp + 0x54) * Screen.fWidth * ((1.0f / 640.0f) / Screen.fHudScale);
                    *(float*)(regs.esp + 0x58) = *(float*)(regs.esp + 0x58) * Screen.fHeight * (1.0f / 480.0f);
                    *(float*)(regs.esp + 0x5C) = Screen.fWidth * *(float*)(regs.esp + 0x5C) * ((1.0f / 640.0f) / Screen.fHudScale);
                    *(float*)(regs.esp + 0x60) = Screen.fHeight * *(float*)(regs.esp + 0x60) * (1.0f / 480.0f);
                    *(float*)(regs.esp + 0x54) += Screen.fHudOffsetReal;
                    *(float*)(regs.esp + 0x5C) += Screen.fHudOffsetReal;
                }
            }
            else
            {
                *(float*)(regs.esp + 0x54) = *(float*)(regs.esp + 0x54) * Screen.fWidth * ((1.0f / 640.0f));
                *(float*)(regs.esp + 0x58) = *(float*)(regs.esp + 0x58) * Screen.fHeight * (1.0f / 480.0f);
                *(float*)(regs.esp + 0x5C) = Screen.fWidth * *(float*)(regs.esp + 0x5C) * ((1.0f / 640.0f));
                *(float*)(regs.esp + 0x60) = Screen.fHeight * *(float*)(regs.esp + 0x60) * (1.0f / 480.0f);
            }
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]
        }
    }; //injector::MakeInline<TGuiTextureRendererRenderHook>(pattern.get_first(0), pattern.get_first(66));

    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "D9 44 24 3C D8 C9 D8 0D ? ? ? ? D9 5C 24 3C DB 44 24 08 D9 44 24 40 D8 C9");
    struct TGuiTextureRendererGroupRenderHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static float temp = 0.0f;
            if (Screen.bFix2D)
            {
                *(float*)(regs.esp + 0x3C) = *(float*)(regs.esp + 0x3C) * Screen.fWidth * ((1.0f / 640.0f) / Screen.fHudScale);
                *(float*)(regs.esp + 0x40) = *(float*)(regs.esp + 0x40) * Screen.fHeight * (1.0f / 480.0f);
                *(float*)(regs.esp + 0x34) = Screen.fWidth * *(float*)(regs.esp + 0x34) * ((1.0f / 640.0f) / Screen.fHudScale);
                *(float*)(regs.esp + 0x38) = Screen.fHeight * *(float*)(regs.esp + 0x38) * (1.0f / 480.0f);
                *(float*)(regs.esp + 0x3C) += Screen.fHudOffsetReal;
                *(float*)(regs.esp + 0x34) += Screen.fHudOffsetReal;
            }
            else
            {
                *(float*)(regs.esp + 0x3C) = *(float*)(regs.esp + 0x3C) * Screen.fWidth * ((1.0f / 640.0f));
                *(float*)(regs.esp + 0x40) = *(float*)(regs.esp + 0x40) * Screen.fHeight * (1.0f / 480.0f);
                *(float*)(regs.esp + 0x34) = Screen.fWidth * *(float*)(regs.esp + 0x34) * ((1.0f / 640.0f));
                *(float*)(regs.esp + 0x38) = Screen.fHeight * *(float*)(regs.esp + 0x38) * (1.0f / 480.0f);
            }
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]
        }
    }; //injector::MakeInline<TGuiTextureRendererGroupRenderHook>(pattern.get_first(0), pattern.get_first(66));

    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "D8 0D ? ? ? ? D9 44 24 24 D8 C9 D9 5C 24 18 D9 C1 D8 4C 24 28 D9 5C 24 1C");
    struct TGOFontDrawStringHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static float temp = 0.0f;
            if (Screen.bFix2D)
            {
                *(float*)(regs.esp + 0x18) = *(float*)(regs.esp + 0x24) * Screen.fWidth * ((1.0f / 640.0f) / Screen.fHudScale);
                *(float*)(regs.esp + 0x1C) = *(float*)(regs.esp + 0x28) * Screen.fHeight * (1.0f / 480.0f);
                *(float*)(regs.esp + 0x10) = Screen.fWidth * *(float*)(regs.esp + 0x34) * ((1.0f / 640.0f) / Screen.fHudScale);
                *(float*)(regs.esp + 0x14) = Screen.fHeight * *(float*)(regs.esp + 0x38) * (1.0f / 480.0f);
                *(float*)(regs.esp + 0x18) += Screen.fHudOffsetReal;
                //*(float*)(regs.esp + 0x10) += Screen.fHudOffsetReal;
            }
            else
            {
                *(float*)(regs.esp + 0x18) = *(float*)(regs.esp + 0x24) * Screen.fWidth * ((1.0f / 640.0f));
                *(float*)(regs.esp + 0x1C) = *(float*)(regs.esp + 0x28) * Screen.fHeight * (1.0f / 480.0f);
                *(float*)(regs.esp + 0x10) = Screen.fWidth * *(float*)(regs.esp + 0x34) * ((1.0f / 640.0f));
                *(float*)(regs.esp + 0x14) = Screen.fHeight * *(float*)(regs.esp + 0x38) * (1.0f / 480.0f);
            }
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]
            regs.eax = *(uint32_t*)(regs.esp + 0x10);
        }
    };
    //injector::MakeInline<TGOFontDrawStringHook>(pattern.count(2).get(0).get<void>(0), pattern.count(2).get(0).get<void>(46));
    //injector::MakeInline<TGOFontDrawStringHook>(pattern.count(2).get(1).get<void>(0), pattern.count(2).get(1).get<void>(46));
#endif

    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "D8 4C 24 34 D8 0D ? ? ? ? D9 5C 24 34 DB 44 24 10 D8 4C 24 38 D8 0D ? ? ? ? D9 5C 24 38");
    struct IGuiManagerPostRenderHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static float temp = 0.0f;
            if (Screen.bFix2D)
            {
                *(float*)(regs.esp + 0x34) = Screen.fWidth * *(float*)(regs.esp + 0x34) * ((1.0f / 640.0f) / Screen.fHudScale);
                *(float*)(regs.esp + 0x38) = Screen.fHeight * *(float*)(regs.esp + 0x38) * (1.0f / 480.0f);
                *(float*)(regs.esp + 0x3C) += Screen.fHudOffsetReal;
                //*(float*)(regs.esp + 0x34) += Screen.fHudOffsetReal;
            }
            else
            {
                *(float*)(regs.esp + 0x34) = Screen.fWidth * *(float*)(regs.esp + 0x34) * ((1.0f / 640.0f));
                *(float*)(regs.esp + 0x38) = Screen.fHeight * *(float*)(regs.esp + 0x38) * (1.0f / 480.0f);
            }
            _asm fstp dword ptr[temp]
        }
    }; injector::MakeInline<IGuiManagerPostRenderHook>(pattern.get_first(0), pattern.get_first(32));

    //Fixes for things that break on very high res
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "51 E8 ? ? ? ? DB 44 24 0C");
    injector::MakeCALL(pattern.get_first(1), TGORender__QueryResolutionHook, true); //positioning
    //pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "E8 ? ? ? ? DB 44 24 1C");
    //injector::MakeCALL(pattern.get_first(0), TGORender__QueryResolutionHook, true); //scaling (not needed)

    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "8D 44 24 18 89 4C 24 18 50 8B CF 89 74 24 20");
    struct XMLParseHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = regs.esp + 0x18;
            *(uint32_t*)(regs.esp + 0x18) = regs.ecx;
            std::string_view name(*(char**)(regs.esi + 0x08));

            if (name == "item_arc-color" || name == "item_arc-trail" || name == "item-bak" || name == "item_icon")
            {
                auto x = float(*(int16_t*)(regs.esi + 0x8C + 0)) / 32.0f;
                auto y = float(*(int16_t*)(regs.esi + 0x8C + 2)) / 32.0f;

                x += (640.0f / 2.0f);
                y -= (480.0f / 2.0f);

                if (Screen.nWidth > 2560) //held item breaks on 4K still, moving it away from right edge
                    x -= 60.0f;

                if (name == "item_arc-color" || name == "item_arc-trail")
                    y -= 1.0f;

                *(int16_t*)(regs.esi + 0x8C + 0) = int16_t(x * 32.0f);
                *(int16_t*)(regs.esi + 0x8C + 2) = int16_t(y * 32.0f);
            }
            else if (name == "crop01" || name == "crop02" || name == "crop03" || name == "crop04")
            {
                *(int32_t*)(regs.esi + 0xA0) = 0; //rgba, hiding tv surv cam overlay
            }
        }
    }; injector::MakeInline<XMLParseHook>(pattern.get_first(0), pattern.get_first(8));

    //fullscreen res switch always
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "8B 0D ? ? ? ? 8B 41 38 8B 48 34 8B 01 52 FF 50 34 84 C0");
    auto loc_10057B25 = pattern.get_first(0);
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "3B CE 7E 25");
    injector::MakeJMP(pattern.get_first(0), loc_10057B25, true);

    //Hide hud button
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "A0 ? ? ? ? 84 C0 74 16 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 84 C0 75 06 B8");
    byte_104C8CFC = *pattern.get_first<uint8_t*>(1);

    //xrefs
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "A1 ? ? ? ? 8B 48 38 8B 46 04");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x102C7DB8, //weapon icons
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 8B 56 1C");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x102C7E21, //weapon icons
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "A1 ? ? ? ? 8B 48 38 8B 46 0C");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x102C7E8B, //weapon icons
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 8B 56 14");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x102C7EF5, //weapon icons
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 8B 56 04");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x102C8CEA, //weapon icons background
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "A1 ? ? ? ? 8B 48 38 8B 46 1C");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x102C8D53, //weapon icons background
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "8B 0D ? ? ? ? 8B 51 38 8B 8A ? ? ? ? 8B 56 0C");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x102C8DBD, //weapon icons background
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "A1 ? ? ? ? 8B 48 38 8B 46 14");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x102C8E27, //weapon icons background

    //borders
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "8B 86 ? ? ? ? 83 F8 01 74 04 3B C1 75 0B");
    struct ILetterBoxActivateHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esi + 0x21C);

            if (Screen.bDisableCutsceneBorders)
            {
                *(float*)(regs.esi + 0x214) = 0.0f;
                *(float*)(regs.esi + 0x218) = 0.0f;
                *(float*)(regs.esi + 0x220) = 0.0f;
                *(float*)(regs.esi + 0x224) = 0.0f;
            }
        }
    }; injector::MakeInline<ILetterBoxActivateHook>(pattern.get_first(0), pattern.get_first(6));
}

void Initg_Rhapsody()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "8A 8E ? ? ? ? 88 88 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B 56 24 8B 4A 04 8B 01 6A 00 6A 05 FF 90 ? ? ? ? 8B CE FF 15 ? ? ? ? 8D 4C 24 10 68");
    struct MenuCheck
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)&regs.ecx = *(uint8_t*)(regs.esi + 0xB0);
            Screen.isMenu = true;
        }
    }; injector::MakeInline<MenuCheck>(pattern.get_first(0), pattern.get_first(6));

    //skip confirm dialogue
    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "8B CE FF 15 ? ? ? ? 8D 4C 24 10 68");
    injector::WriteMemory(pattern.get_first(0), 0x835B5E5F, true);
    injector::WriteMemory(pattern.get_first(4), 0x90C314C4, true);

    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "75 46 A1 ? ? ? ? 8B 48 38 8B 91 ? ? ? ? 8B 3A 6A 00");
    injector::MakeNOP(pattern.count(2).get(1).get<void>(0), 2, true);

    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "8B 8E ? ? ? ? 8B 11 EB 73");
    struct ResTextOverwrite
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.esi + 0xC0);
            swprintf((wchar_t*)(*(uint32_t*)(regs.eax) + 2), 18, L"%d x %d", Screen.nWidth, Screen.nHeight);
        }
    }; injector::MakeInline<ResTextOverwrite>(pattern.get_first(0), pattern.get_first(6));

    //surv camera rotating thingy centering
    static float f0 = 0.0f;
    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "D8 25 ? ? ? ? D8 4C 24 20 DB 44 24 74 D8 BC F4");
    injector::WriteMemory(pattern.get_first(2), &f0, true);
    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "D8 25 ? ? ? ? D8 4C 24 48 D9 E0 D9 C9 D8 44 24 7C");
    injector::WriteMemory(pattern.get_first(2), &f0, true);

    //Fullscreen res change always
    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "74 19 56");
    injector::MakeNOP(pattern.get_first(0), 2, true);

    //disabled in favor of EndScene borders
    //pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "81 C6 ? ? ? ? 56 68 ? ? ? ? 8B CF E8 ? ? ? ? 5F 5E C2 04 00");
    //struct DarkeningVeilXMLHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        *(float*)(regs.esi+0x9C) = 0.0f;
    //        *(float*)(regs.esi+0xA0) = 0.0f;
    //        *(float*)(regs.esi+0xA4) = 0.0f;
    //        *(float*)(regs.esi+0xA8) = 0.0f;
    //        static uint32_t temp = 0;
    //        regs.esi = (uint32_t)&temp;
    //    }
    //}; injector::MakeInline<DarkeningVeilXMLHook>(pattern.count(24).get(21).get<void>(0), pattern.count(24).get(21).get<void>(6));

    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "A1 ? ? ? ? 8B 48 38 8B 44 24 5C");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x100E4F04, //red weapon icon half circle bar
    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "A1 ? ? ? ? 8B 48 38 8B 44 24 7C");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x100E4F70, //red weapon icon half circle bar
    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "A1 ? ? ? ? 8B 48 38 8B 44 24 74");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x100E4FD6, //red weapon icon half circle bar
    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "A1 ? ? ? ? 8B 48 38 8B 84 24 ? ? ? ? 8B 89 ? ? ? ? 8B 11 50 8B 84 24");
    xrefs_hud.push_back((uint32_t)pattern.get_first(0)); //0x100E5042, //red weapon icon half circle bar

#if _DEBUG
    //security camera force overlay
    //pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "74 10 8B CF");
    //injector::WriteMemory<uint8_t>(pattern.get_first(0), 0x75, true);
#endif // _DEBUG
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("8B 47 0C 8B 4F 08"));
        CallbackHandler::RegisterCallback(L"GCore.dll", InitGCore);
        CallbackHandler::RegisterCallback(L"g_Rhapsody.sgl", Initg_Rhapsody);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
