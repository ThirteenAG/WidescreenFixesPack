#include "stdafx.h"

struct Screen
{
    int Width;
	int Width43;
	int WidthFMV;
	int OffsetX;
    int Height;
	int Height43;
	int HeightFMV;
	int OffsetY;
	bool AspectRatioAffected;
	bool PreserveHorizontalPosition;
	int ContinuePositionChange;
    float fWidth;
    float fHeight;
    float fAspectRatio;
	float fConditionalAspectRatio;
	float fZoomFactor;
    float fHudScale;
} Screen;

float iniZoomFactor = 1.0f;
bool bFixLensFlare = true;
float fLensFlareScalar;

void updateValues(const float& newWidth, const float& newHeight)
{
	//Screen resolution
	Screen.Width = newWidth;
	Screen.Height = newHeight;
	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	// Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
	// Screen.Height43 = (int)(Screen.Height * Screen.fZoomFactor);

	if (Screen.fAspectRatio < Screen.fConditionalAspectRatio)
	{
		Screen.fZoomFactor = iniZoomFactor * (Screen.fAspectRatio / Screen.fConditionalAspectRatio);
	}
	Screen.fHudScale = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f)) * Screen.fZoomFactor;

	Screen.OffsetX = (int)(Screen.Width - (Screen.Width * Screen.fHudScale)) / 2;
	Screen.OffsetY = (int)(Screen.Height - (Screen.Height * Screen.fZoomFactor)) / 2;

	Screen.Width43 = (int)(Screen.Width * Screen.fHudScale);
	Screen.Height43 = (int)(Screen.Height * Screen.fZoomFactor);

	Screen.WidthFMV = Screen.Width;
	Screen.HeightFMV = Screen.Height;

	if (Screen.fAspectRatio < (4.0f / 3.0f))
	{
		Screen.HeightFMV = (int)(Screen.HeightFMV * (Screen.fAspectRatio / (4.0f / 3.0f)));
	}
	else
	{
		Screen.WidthFMV = (int)(Screen.WidthFMV / (Screen.fAspectRatio / (4.0f / 3.0f)));
	}

	// write resolution vars
	*(uint32_t*)0x00A7793C = Screen.Width;
	*(uint32_t*)0x00A77940 = Screen.Height;

	*(float*)0x0078A08C = 1.0f / (480.0f * Screen.fAspectRatio);

	*(float*)0x00AA7140 = Screen.fWidth;
	*(float*)0x00AA7144 = Screen.fHeight;

	if (bFixLensFlare)
	{
		*(float*)0x0078A184 = Screen.fWidth;
		*(float*)0x0078A180 = Screen.fHeight;
		// lens flare size
		fLensFlareScalar = Screen.fHeight / 480.0f;
	}
}

unsigned int GameWndProcAddr = 0;
LRESULT(WINAPI* GameWndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WSFixWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
	{
		if (!WindowedModeWrapper::bEnableWindowResize) return TRUE;

		updateValues((float)LOWORD(lParam), (float)HIWORD(lParam));
		return GameWndProc(hWnd, msg, wParam, lParam);
		// bIsResizing = true;
		// *DrawHUD_57CAA8 = false;
		// updateValues((float)LOWORD(lParam), (float)HIWORD(lParam));
	}

	case WM_SIZING:
	{
		if (!WindowedModeWrapper::bEnableWindowResize) return TRUE;
		return GameWndProc(hWnd, msg, wParam, lParam);
	}
	
	}

	return GameWndProc(hWnd, msg, wParam, lParam);
}

BOOL WINAPI GetClientRectHook(HWND hWnd, LPRECT lpRect)
{
	lpRect->top = 0;
	lpRect->left = 0;
	
	lpRect->right = Screen.Width;
	lpRect->bottom = Screen.Height;
	return TRUE;
}

bool bStretchOnBootFlag = false;
uintptr_t ptrWindowSizeX = 0x00AA5384;
uintptr_t loc_6567BF = 0x6567BF;
void __declspec(naked) StretchOnBoot()
{
	if (!bStretchOnBootFlag)
	{
		bStretchOnBootFlag = true;
		*(uint32_t*)ptrWindowSizeX = 640; // invalidate it on purpose to trigger the window to update & device reset
	}
	_asm
	{
		mov edx, [esp + 0x20]
		mov eax, ptrWindowSizeX
		mov eax, [eax]
		jmp loc_6567BF
	}
}

uintptr_t loc_48F41F = 0x48F41F;
void __declspec(naked) LensFlareScale()
{
	_asm
	{
		fmul[fLensFlareScalar]
		mov eax, [esi + 0x1C]
		fmul dword ptr[esi + 0x10]
		jmp loc_48F41F
	}
}

void Init()
{
    CIniReader iniReader("");
	DWORD dummyoldprotect = 0;
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
	Screen.fConditionalAspectRatio = iniReader.ReadFloat("MAIN", "Horizontal_Aspect_Lock", (4.0f / 3.0f));
	Screen.fZoomFactor = iniReader.ReadFloat("MAIN", "FOV_Zoom_Factor", 1.0f);
	iniZoomFactor = Screen.fZoomFactor;
	int nWindowedMode = iniReader.ReadInteger("MISC", "WindowedMode", 0);
	static bool bShadowFix = iniReader.ReadInteger("MISC", "ShadowFix", 1) != 0;
	bFixLensFlare = iniReader.ReadInteger("MISC", "LensFlareFix", 1) != 0;
	static bool bDisableMouseInput = iniReader.ReadInteger("MISC", "DisableMouseInput", 1) != 0;
	static bool bDisableFrameSkipping = iniReader.ReadInteger("MISC", "DisableFrameSkipping", 1) != 0;

	static auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");

	if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
		szCustomUserFilesDirectoryInGameDir.clear();

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	if (Screen.fAspectRatio < Screen.fConditionalAspectRatio)
	{
		Screen.fZoomFactor *= (Screen.fAspectRatio / Screen.fConditionalAspectRatio);
	}
    Screen.fHudScale = ((1.0f / Screen.fAspectRatio) * (4.0f / 3.0f)) * Screen.fZoomFactor;

	Screen.OffsetX = (int)(Screen.Width - (Screen.Width * Screen.fHudScale)) / 2;
	Screen.OffsetY = (int)(Screen.Height - (Screen.Height * Screen.fZoomFactor)) / 2;

	Screen.Width43 = (int)(Screen.Width * Screen.fHudScale);
	Screen.Height43 = (int)(Screen.Height * Screen.fZoomFactor);

	Screen.WidthFMV = Screen.Width;
	Screen.HeightFMV = Screen.Height;

	if (Screen.fAspectRatio < (4.0f / 3.0f))
	{
		Screen.HeightFMV = (int)(Screen.HeightFMV * (Screen.fAspectRatio / (4.0f / 3.0f)));
	}
	else
	{
		Screen.WidthFMV = (int)(Screen.WidthFMV / (Screen.fAspectRatio / (4.0f / 3.0f)));
	}

	// disable writes to resolution vars
	injector::MakeNOP(0x427722, 10);
	injector::MakeNOP(0x00427735, 10);
	injector::MakeNOP(0x00444892, 5);
	injector::MakeNOP(0x00444897, 5);

	injector::MakeJMP(0x446B06, 0x446B51);

	// write resolution vars
	*(uint32_t*)0x00A7793C = Screen.Width;
	*(uint32_t*)0x00A77940 = Screen.Height;

	if (bFixLensFlare)
	{
		injector::UnprotectMemory(0x0078A180, 2 * sizeof(float), dummyoldprotect);
		*(float*)0x0078A184 = Screen.fWidth;
		*(float*)0x0078A180 = Screen.fHeight;
		// lens flare size
		fLensFlareScalar = Screen.fHeight / 480.0f;

		injector::MakeJMP(0x0048F419, LensFlareScale, true);
	}
	auto pattern = hook::pattern("0F BF 4E ? 0F BF C0 89 44 ? ? DB 44 ? ? 89 4C ? ? 85 DB"); // 662C2D
	struct ResHook6
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(uint16_t*)(regs.esi + 0x1E);

			if (Screen.AspectRatioAffected)
			{
				if (Screen.PreserveHorizontalPosition == true)
				{
					regs.eax -= Screen.OffsetX;
					if (Screen.ContinuePositionChange != false)
					{
						Screen.PreserveHorizontalPosition = true;
					}
					else
					{
						Screen.PreserveHorizontalPosition = false;
					}
				}
				regs.eax += Screen.OffsetX;
				regs.ecx += Screen.OffsetY;
				if (Screen.ContinuePositionChange != false)
				{
					Screen.AspectRatioAffected = true;
					Screen.ContinuePositionChange -= 1;
				}
				else
				{
					Screen.AspectRatioAffected = false;
				}
			}
		}
	};

	injector::MakeInline<ResHook6>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(7));
	pattern = hook::pattern("66 8B ? ? 66 85 C0 75 29 66 39 ? ? 75 23"); // 662BFB
	injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<int8_t>(7), 0xEB, true);

    pattern = hook::pattern("D9 42 68 D8 08 D9 42 68 D8 48 04 D9 42 68 D8 48 08"); // 0x64AFDC
    struct CutOffAreaHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float edx68 = *(float*)(regs.edx + 0x68) / Screen.fHudScale;
            float eax00 = *(float*)(regs.eax + 0);
            float eax04 = *(float*)(regs.eax + 4);
            float eax08 = *(float*)(regs.eax + 8);
            _asm
            {
                fld     dword ptr[edx68]
                fmul    dword ptr[eax00]
                fld     dword ptr[edx68]
                fmul    dword ptr[eax04]
                fld     dword ptr[edx68]
                fmul    dword ptr[eax08]
            }
        }
    };

	struct CutOffAreaHookY
	{
		void operator()(injector::reg_pack& regs)
		{
			float yScale = *(float*)(regs.edx + 0x6C) / Screen.fZoomFactor * *(float*)(regs.eax + 0x10);
		}
	};

	struct CutOffAreaHookY2
	{
		void operator()(injector::reg_pack& regs)
		{
			float yScale = *(float*)(regs.edx + 0x6C) / Screen.fZoomFactor * *(float*)(regs.eax + 0x14);
		}
	};

	struct CutOffAreaHookY3
	{
		void operator()(injector::reg_pack& regs)
		{
			float yScale = *(float*)(regs.edx + 0x6C) / Screen.fZoomFactor * *(float*)(regs.eax + 0x18);
		}
	};
	
	injector::MakeInline<CutOffAreaHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(17));
	injector::MakeInline<CutOffAreaHookY>(pattern.count(1).get(0).get<uint32_t>(42), pattern.count(1).get(0).get<uint32_t>(48));
	injector::MakeInline<CutOffAreaHookY2>(pattern.count(1).get(0).get<uint32_t>(54), pattern.count(1).get(0).get<uint32_t>(60));
	injector::MakeInline<CutOffAreaHookY3>(pattern.count(1).get(0).get<uint32_t>(64), pattern.count(1).get(0).get<uint32_t>(70));

    pattern = hook::pattern("D9 05 ? ? ? ? 89 4E 68 8B 50 04 D8 76 68"); // 0x64AC8B
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.fHudScale, true);
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(28), &Screen.fZoomFactor, true);

	pattern = hook::pattern("83 EC ? 56 57 E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 BF"); // 0x63AA30

	// 2P Interface
	pattern = hook::pattern("8B 48 60 DB 41 0C 83 C4 ? BF ? ? ? ? BE ? ? ? ? D9 5C"); // 0x422C2B
	struct HUDHor
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(int*)(regs.eax + 0x60);
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
		}
	};

	struct HUDVert
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ebp |= -1;
			float ResolutionY = (float)(*(int*)(regs.ecx + 0x10)) * Screen.fZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
		}
	};

	injector::MakeInline<HUDHor>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::MakeInline<HUDVert>(pattern.count(1).get(0).get<uint32_t>(23), pattern.count(1).get(0).get<uint32_t>(29));

	pattern = hook::pattern("D9 44 ? ? D8 64 ? ? 6A 04 68 ? ? ? ? 6A 04 D9 15 ? ? ? ? D9 1D ? ? ? ? D9 44"); // 0x422A85

	struct HUDPos
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			float Temp = (*(float*)(regs.esp + 0x0C)) - (*(float*)(regs.esp + 0x14));
			_asm
			{
				fld dword ptr[Temp]
			}
		}
	};

	injector::MakeInline<HUDPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8));

	// Credits
	pattern = hook::pattern("DB 05 ? ? ? ? D8 0F D8 0D ? ? ? ? D9 9E ? ? ? ? 8B 57 04 89 96 ? ? ? ?"); // 0x4543FF
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &Screen.Width43, true);

	pattern = hook::pattern("DB 05 ? ? ? ? D9 5C ? ? DB 05 ? ? ? ? D9 5C ? ? E8 ? ? ? ? D9 05 ? ? ? ?"); // 0x4548B1

	struct CreditPicturePos
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			_asm
			{
				fild dword ptr[Screen.Width43]
			}
		}
	};

	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(12), &Screen.Height43, true);

	pattern = hook::pattern("8B D1 C1 EA 10 0F B6 C6 C1 E0 08 0F B6 F9 0B C7 89 4C ? ? C1 E0 08"); // 0x454FFD

	struct CreditPicturePos2
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			regs.edx = regs.ecx;
			regs.edx >>= 10;
		}
	};

	injector::MakeInline<CreditPicturePos2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));

	pattern = hook::pattern("8B 44 ? ? EB 10 8B 1D ? ? ? ? 8B 2D ? ? ? ? 89 5C ? ?"); //0x6445C0

	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(8), &Screen.WidthFMV, true);
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(14), &Screen.HeightFMV, true);

	pattern = hook::pattern("83 EC ? 56 57 E8 ? ? ? ? D9 05 ? ? ? ? D8 B0 ? ? ? ? 8B 84 ?"); // 0x6445C0
	struct CreditFMVPos
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			_asm
			{
				fld1
			}
		}
	};
	struct CreditFMVPos2
	{
		void operator()(injector::reg_pack& regs)
		{
			if (!Screen.AspectRatioAffected)
			{
				Screen.AspectRatioAffected = true;
			}
			regs.ecx = *(int*)(regs.esp + 8);
			_asm
			{
				fstp dword ptr[esi - 8]
			}
		}
	};
	injector::MakeInline<CreditFMVPos>(pattern.count(1).get(0).get<uint32_t>(10), pattern.count(1).get(0).get<uint32_t>(16));
	injector::MakeInline<CreditFMVPos2>(pattern.count(1).get(0).get<uint32_t>(332), pattern.count(1).get(0).get<uint32_t>(339));

	// Mission text

	struct MissionTextHor
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
			regs.esi = *(int*)(regs.esi + 0x98);
		}
	};

	struct MissionTextVert
	{
		void operator()(injector::reg_pack& regs)
		{
			float ResolutionY = (float)(*(int*)(regs.ecx + 0x10)) * Screen.fZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
			*(float*)(regs.esp + 0x10) = ResolutionY;
		}
	};

	pattern = hook::pattern("DB 41 0C 8B B6 ? ? ? ? 83 C4 08 83 EE 02"); // 0x44296C
	injector::MakeInline<MissionTextHor>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(9));
	injector::MakeInline<MissionTextVert>(pattern.count(1).get(0).get<uint32_t>(19), pattern.count(1).get(0).get<uint32_t>(26));

	struct NowLoadingTextPos
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			float eax80 = *(float*)(regs.eax + 0x80);
			_asm
			{
				fdiv dword ptr[eax80]
			}
		}
	};
	pattern = hook::pattern("DB 05 ? ? ? ? D9 5C ? ? DB 47 ? D8 4C ? ? D8 0D ? ? ? ? D9 5C ? ? DB 05 ? ? ? ?");
	injector::MakeInline<NowLoadingTextPos>(pattern.count(1).get(0).get<uint32_t>(69), pattern.count(1).get(0).get<uint32_t>(75));

	// Powerup Icons

	struct PowerupIcon1
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			regs.ecx = *(int*)(regs.eax + 0x60);
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
		}
	};

	struct PowerupIcon2
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(int*)(regs.eax + 0x60);
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
		}
	};

	struct PowerupIcon3
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.edx = *(int*)(regs.eax + 0x60);
			float ResolutionY = (float)(*(int*)(regs.edx + 0x10)) * Screen.fZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
		}
	};

	struct PowerupIcon4
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.edi = 1;
			_asm
			{
				fmul dword ptr[Screen.fHudScale]
			}
		}
	};

	pattern = hook::pattern("8B 48 60 DB 41 0C 6A 04 68 ? ? ? ? 6A 04 D8 4C ? ?"); //0x479AEA
	injector::MakeInline<PowerupIcon1>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	pattern = hook::pattern("8B 48 60 DB 41 0C D9 5C ? ? E8 ? ? ? ? 8B 50 60 DB 42 10 D9 5C ? ? E8 ? ? ? ? D9 05"); // 0x479EB1
	injector::MakeInline<PowerupIcon2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::MakeInline<PowerupIcon3>(pattern.count(1).get(0).get<uint32_t>(15), pattern.count(1).get(0).get<uint32_t>(21));
	pattern = hook::pattern("DB 40 0C 8B 54 ? ? 8B 44 ? ? 8B 4D 34 D8 0D ? ? ? ? 4A 89 54 ? ? BF 01 00 00 00 3B C7"); // 0x479F26
	injector::MakeInline<PowerupIcon4>(pattern.count(1).get(0).get<uint32_t>(25), pattern.count(1).get(0).get<uint32_t>(30));

	// Results screen
	pattern = hook::pattern("8B 48 60 DB 41 0C E8 ? ? ? ? 8B 50 60 DB 42 10 D9 5C ? ? E8 ? ? ? ? D9 05 ? ? ? ? BE"); // 0x438CDD
	struct ResultsHor
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(int*)(regs.eax + 0x60);
			float ResolutionX = (float)(*(int*)(regs.ecx + 0x0C)) * Screen.fHudScale;
			_asm
			{
				fld dword ptr[ResolutionX]
			}
		}
	};

	struct ResultsVert
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.edx = *(int*)(regs.eax + 0x60);
			float ResolutionY = (float)(*(int*)(regs.edx + 0x10)) * Screen.fZoomFactor;
			_asm
			{
				fld dword ptr[ResolutionY]
			}
		}
	};

	struct ResultsPos
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.eax <<= 8;
			regs.eax |= regs.ecx;
			Screen.AspectRatioAffected = true;
		}
	};

	injector::MakeInline<ResultsHor>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::MakeInline<ResultsVert>(pattern.count(1).get(0).get<uint32_t>(11), pattern.count(1).get(0).get<uint32_t>(17));
	pattern = hook::pattern("8A 44 B4 ? C1 E0 08 0B C1 C1 E0 08 0B C2 A3"); //0x438E42
	injector::MakeInline<ResultsPos>(pattern.count(1).get(0).get<uint32_t>(4), pattern.count(1).get(0).get<uint32_t>(9));

	// Special Stage
	pattern = hook::pattern("DB 05 ? ? ? ? 8B 00 D9 84 ? ? ? ? ? 53 55 D8 C9 57 50 6A 01 D8 0D ? ? ? ? D9 9C"); // 0x45894A
	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(38), &Screen.Height43, true);

	pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 ? ? D8 8C"); // 0x526F83
	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(40), &Screen.Height43, true);

	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(236), &Screen.Width43, true);

	pattern = hook::pattern("8A 0E C1 E1 08 0B CA C1 E1 08 0B C8 89 0D ? ? ? ? E8 ? ? ? ? 6A 03"); // 0x525AD7

	struct DashStreaks
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			regs.ecx = *(int8_t*)(regs.esi);
			regs.ecx <<= 8;
		}
	};

	injector::MakeInline<DashStreaks>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));

	pattern = hook::pattern("E8 ? ? ? ? 83 C4 10 E8 ? ? ? ? 5F C3 CC CC CC CC CC CC 83 EC 0C 56 33 F6 89 74"); // 0x527470

	struct MeteorRightPos
	{
		void operator()(injector::reg_pack& regs)
		{
			if (regs.eax != 0)
			{
				Screen.PreserveHorizontalPosition = true;
				Screen.ContinuePositionChange = 3;
			}
		}
	};

	injector::MakeInline<MeteorRightPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));
	injector::WriteMemory<uint16_t>(pattern.count(1).get(0).get<uint16_t>(5), 0xD1EB, true);
	injector::MakeNOP(pattern.count(1).get(0).get<uint16_t>(7), 1, true);

	pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 0F B6 9C 24 ? ? ? ? D9 54 24 ? C1 E3 18"); // 0x5263DE

	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(63), &Screen.Height43, true);

	pattern = hook::pattern("DB 05 ? ? ? ? A1 ? ? ? ? 83 C4 08 D9 54 24 ? BF ? ? ? ? D8 8C 24 ? ? ? ? 8D 74 24 ?"); // 0x526297

	injector::MakeInline<CreditPicturePos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
	injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(49), &Screen.Height43, true);

	pattern = hook::pattern("85 C0 74 0F 8B 46 28 85 C0 75 08 C7 44 24 04 00 80 FD 43 8A 42 1E 84 C0 74 0E"); // 0x527936

	struct LevelUpPos1
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.PreserveHorizontalPosition = false;
			regs.eax = *(int*)(regs.esi + 0x28);
			if (regs.eax == 0)
			{
				Screen.PreserveHorizontalPosition = true;
				*(float*)(regs.esp + 4) = 507.0f;
			}
		}
	};

	injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 4, true);
	injector::MakeInline<LevelUpPos1>(pattern.count(1).get(0).get<uint32_t>(4), pattern.count(1).get(0).get<uint32_t>(19));

	struct LevelUpPos2
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = false;
			Screen.ContinuePositionChange = true;
			regs.eax = *(int*)(regs.esi + 0x2C);
			if (regs.eax < 95)
			{
				Screen.ContinuePositionChange = true;
			}
			regs.ecx = *(int*)(regs.esi + 0x48);
		}
	};

	injector::MakeInline<LevelUpPos2>(pattern.count(1).get(0).get<uint32_t>(40), pattern.count(1).get(0).get<uint32_t>(46));

	pattern = hook::pattern("D9 C9 C1 E0 08 D9 5C ? ? 0B C1 C1 E0 08 D9 5C ? ? 0B C2 8D 4C ? ? BA"); //0x4578A3

	struct WindowTextPos
	{
		void operator()(injector::reg_pack& regs)
		{
			_asm
			{
				fxch st(1)
			}
			Screen.AspectRatioAffected = true;
			regs.eax <<= 8;
		}
	};

	injector::MakeInline<WindowTextPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));

	pattern = hook::pattern("C1 E0 08 0B C2 D9 54 ? ? C1 E0 08 D9 41 ? 0F B6 D3 D8 05 ? ? ? ? 0B C2 8B 54"); //0x4583FB

	struct WindowTextPos2
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			regs.eax <<= 8;
			regs.eax |= regs.edx;
		}
	};

	injector::MakeInline<WindowTextPos2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(5));

	pattern = hook::pattern("D9 00 8B 39 D8 02 8B 50 04 89 54 ? ? 8B 50 08 89 7C ? ? D9 54"); //0x4574F9

	struct WindowPos
	{
		void operator()(injector::reg_pack& regs)
		{
			Screen.AspectRatioAffected = true;
			float Temp = *(float*)(regs.eax) + *(float*)(regs.edx);
			regs.edi = *(int*)(regs.ecx);
			_asm
			{
				fld dword ptr[Temp]
			}
		}
	};

	injector::MakeInline<WindowPos>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

	// unprotect and set scaled X res divider for Advertise
	injector::UnprotectMemory(0x0078A08C, sizeof(float), dummyoldprotect);
	*(float*)0x0078A08C = 1.0f / (480.0f * Screen.fAspectRatio);

	if (bShadowFix)
	{
		//fShadowHalfScale = 0;
		//fShadowScalar2 = (0.025f / 480.0f) * Screen.fHeight;

		// struct ShadowFix
		// {
		// 	void operator()(injector::reg_pack& regs)
		// 	{
		// 		//*(float*)(regs.esp + 4) = (5.0f / 480.0f) * Screen.fHeight;
		// 		*(float*)(regs.esp + 4) = 0;
		// 	}
		// }; injector::MakeInline<ShadowFix>(0x0063B11B, 0x0063B123);

		// TODO - fix shadows from squishing (stretching vertically) - this is very visible on ultrawide

		struct ShadowFix2
		{
			void operator()(injector::reg_pack& regs)
			{
				//*(float*)(regs.esp + 0x10) *= Screen.fHeight / 480.0f;

				//float val = *(float*)(regs.esp + 0x10);
				//float scfactor = Screen.fHeight / 480.0f;
				float val = ((5.0f / 480.0f) * Screen.fHeight) - ((4 - regs.esi) * (Screen.fHeight * 0.0016339869281046));
				float val2 = val * 0.025;

				
				// 0.278645843267
				*(float*)(regs.esp + 0x3C) = val2;
				*(float*)(regs.esp + 0x40) = val2;
				*(float*)(regs.esp + 0x58) = val2;
				*(float*)(regs.esp + 0x78) = val2;
				// // 0.72135412693
				*(float*)(regs.esp + 0x5C) = 1.0f - val2;
				*(float*)(regs.esp + 0x74) = 1.0f - val2;
				*(float*)(regs.esp + 0x90) = 1.0f - val2;
				*(float*)(regs.esp + 0x94) = 1.0f - val2;
		
				// // -256.0f
				// *(float*)(regs.esp + 0x34) *= 2.0f;
				// *(float*)(regs.esp + 0x50) *= 2.0f;
				// *(float*)(regs.esp + 0x6C) *= 2.0f;
				// *(float*)(regs.esp + 0x88) *= 2.0f;
				// 
				// // 256.0f
				// *(float*)(regs.esp + 0x48) *= 2.0f;
				// *(float*)(regs.esp + 0x60) *= 2.0f;
				// *(float*)(regs.esp + 0x7C) *= 2.0f;
				// *(float*)(regs.esp + 0x80) *= 2.0f;
		
				reinterpret_cast<void(__cdecl*)(uint32_t, uint32_t, uint32_t)>(0x64CA10)(4, regs.edx, 4);
				_asm fld [val]
			}
		}; injector::MakeInline<ShadowFix2>(0x0063B173, 0x0063B17C);



		//injector::WriteMemory<float*>(0x63B182, &fShadowHalfScale, true);
		//injector::WriteMemory<float*>(0x63B138, &fShadowScalar2, true);
	}

	if (!szCustomUserFilesDirectoryInGameDir.empty())
	{
		szCustomUserFilesDirectoryInGameDir = GetExeModulePath<std::string>() + szCustomUserFilesDirectoryInGameDir;

		auto SHGetFolderPathAHook = [](HWND /*hwnd*/, int /*csidl*/, HANDLE /*hToken*/, DWORD /*dwFlags*/, LPSTR pszPath) -> HRESULT
		{
			CreateDirectoryA(szCustomUserFilesDirectoryInGameDir.c_str(), NULL);
			strcpy_s(pszPath, MAX_PATH, szCustomUserFilesDirectoryInGameDir.c_str());
			return S_OK;
		};

		auto PathAppendAHook = [](LPSTR pszPath, LPSTR) -> HRESULT
		{
			strcat_s(pszPath, MAX_PATH, "\\");
			return S_OK;
		};

		uintptr_t loc_629D17 = reinterpret_cast<uintptr_t>(hook::pattern("? ? ? 6A 1A ? FF 15").get_first(0)) + 6;
		uintptr_t loc_62A3FC = reinterpret_cast<uintptr_t>(hook::pattern("50 6A 00 6A 00 6A 1A 6A 00 FF 15").get_first(0)) + 9;
		uintptr_t loc_62DF99 = reinterpret_cast<uintptr_t>(hook::pattern("56 6A 00 6A 00 6A 1A 6A 00 FF 15").get_first(0)) + 9;
		uintptr_t loc_62DFAC = loc_62DF99 + 0x13;
		uintptr_t str_7468D0 = *reinterpret_cast<uintptr_t*>(loc_629D17 + 0xD);

		injector::MakeCALL(loc_629D17, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
		injector::MakeNOP(loc_629D17 + 5, 1, true);

		injector::MakeCALL(loc_62A3FC, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
		injector::MakeNOP(loc_62A3FC + 5, 1, true);

		injector::MakeCALL(loc_62DF99, static_cast<HRESULT(WINAPI*)(HWND, int, HANDLE, DWORD, LPSTR)>(SHGetFolderPathAHook), true);
		injector::MakeNOP(loc_62DF99 + 5, 1, true);
		
		injector::WriteMemory<uint8_t>(str_7468D0 + 2, 0, true);

		injector::MakeCALL(loc_62DFAC, static_cast<HRESULT(WINAPI*)(LPSTR, LPSTR)>(PathAppendAHook), true);
		injector::MakeNOP(loc_62DFAC + 5, 1, true);
	}
	if (nWindowedMode)
	{
		injector::MakeJMP(0x446D87, 0x446DA5, true);
		injector::MakeJMP(0x446E11, 0x446E24, true);
		injector::MakeNOP(0x4462F8, 6, true);
		injector::MakeCALL(0x4462F8, WindowedModeWrapper::CreateWindowExA_Hook, true);
		injector::MakeNOP(0x4462B5, 6, true);
		injector::MakeCALL(0x4462B5, WindowedModeWrapper::AdjustWindowRect_Hook, true);
		injector::MakeNOP(0x446DD7, 6, true);
		injector::MakeCALL(0x446DD7, WindowedModeWrapper::AdjustWindowRect_Hook, true);

		// disable cursor centering on boot
		injector::MakeNOP(0x00446EA8, 5, true);

		// dereference the current WndProc from the game executable and write to the function pointer (to maximize compatibility)
		uint32_t* wndproc_addr = hook::pattern("C7 44 24 14 ? ? ? ? 89 74 24 18 89 74 24 1C").count(1).get(0).get<uint32_t>(4);
		GameWndProcAddr = *(unsigned int*)wndproc_addr;
		GameWndProc = (LRESULT(WINAPI*)(HWND, UINT, WPARAM, LPARAM))GameWndProcAddr;
		injector::WriteMemory<unsigned int>(wndproc_addr, (unsigned int)&WSFixWndProc, true);

		if ((nWindowedMode == 4) || (nWindowedMode == 5))
		{
			uintptr_t addr = 0x004460A9;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);
			addr = 0x655794;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);
			// addr = 0x656644;
			// injector::MakeNOP(addr, 6, true);
			// injector::MakeCALL(addr, GetClientRectHook, true);
			addr = 0x656787;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);
			addr = 0x657C9C;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);
			addr = 0x658172;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);
			addr = 0x65822F;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);
			addr = 0x65DBE7;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);
			addr = 0x65DC2F;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);
			addr = 0x65E152;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);
			addr = 0x65E783;
			injector::MakeNOP(addr, 6, true);
			injector::MakeCALL(addr, GetClientRectHook, true);

			injector::MakeJMP(0x6567B6, StretchOnBoot, true);
		}

		switch (nWindowedMode)
		{
		case 5:
			WindowedModeWrapper::bStretchWindow = true;
			break;
		case 4:
			WindowedModeWrapper::bScaleWindow = true;
			break;
		case 3:
			WindowedModeWrapper::bEnableWindowResize = true;
		case 2:
			WindowedModeWrapper::bBorderlessWindowed = false;
			break;
		default:
			break;
		}
	}

	if (bDisableMouseInput)
	{
		// corrupt GUID_SysMouse on purpose
		injector::WriteMemory<uint32_t>(0x00722FA0, 0, true);
	}

	if (bDisableFrameSkipping)
	{
		injector::MakeJMP(0x402CF5, 0x402D20);
	}

}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("0F BE 0D ? ? ? ? 8D 0C 89 8B"));
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
