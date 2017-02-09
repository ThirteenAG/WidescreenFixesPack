#include "stdafx.h"
#include <string>

bool bDelay;

struct Screen
{
	int Width;
	int Height;
	float fWidth;
	float fHeight;
	float fFieldOfView;
	float fAspectRatio;
	float fUnk1;
	float fHudOffset;
	float f2DSpritesOffset;
	float f2DSpritesScale;
	float fHudHorScale;
	float fHudVerScale;
} Screen;

char *szForceAspectRatio;
float fFOVFactor;
char* szCustomUserFilesDirectory;
bool bDisableGamepadInput;

int FrontendAspectRatioWidth;
int FrontendAspectRatioHeight;

char* __cdecl InitUserDirectories()
{
	CreateDirectory(szCustomUserFilesDirectory, NULL);
	return szCustomUserFilesDirectory;
}

DWORD jmpAddr = 0x475C4F;
void __declspec(naked)setScreenFieldOfView()
{
	_asm
	{
		fdiv dword ptr ds : [0x715C94]
		fmul dword ptr ds : [fFOVFactor]
		fstp dword ptr[esp]
		fld dword ptr[esp + 4]
		fdiv dword ptr ds : [0x715C94]
		fmul dword ptr ds : [fFOVFactor]
		jmp jmpAddr
	}
}

injector::hook_back<void(__cdecl*)(float, float, float, float, int, int, int, int, int)> hbDrawRect;
void __cdecl DrawRectHook(float fLeft, float fTop, float fRight, float fBottom, int r, int g, int b, int a, int unk)
{
	if (fLeft == 0.0f && fTop == 0.0f && fRight == 1.0f && fBottom == 1.0f)
	{
		hbDrawRect.fun(fLeft, fTop, fRight, fBottom, 0, 0, 0, 255, unk);
		hbDrawRect.fun(Screen.f2DSpritesOffset, fTop, Screen.f2DSpritesScale, fBottom, r, g, b, a, unk);
		return;
	}
	return hbDrawRect.fun(fLeft, fTop, fRight, fBottom, r, g, b, a, unk);
}

injector::hook_back<int(__cdecl*)(float a1, float a2, float a3, float a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18, int a19, int a20, int a21, int a22, int a23, int a24, int a25, int a26, int a27, int a28)> hbDrawText;
int __cdecl DrawTextHook(float a1, float a2, float a3, float a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18, int a19, int a20, int a21, int a22, int a23, int a24, int a25, int a26, int a27, int a28)
{
	a1 += 0.5f;
	return hbDrawText.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28);
}

void Fix2DSprites()
{
	CIniReader iniReader("");
	szForceAspectRatio = iniReader.ReadString("MAIN", "FrontendAspectRatio", "auto");
	if (strncmp(szForceAspectRatio, "auto", 4) != 0)
	{
		FrontendAspectRatioWidth = std::stoi(szForceAspectRatio);
		FrontendAspectRatioHeight = std::stoi(strchr(szForceAspectRatio, ':') + 1);
	}
	else
	{
		FrontendAspectRatioWidth = 4;
		FrontendAspectRatioHeight = 3;
	}

	hbDrawRect.fun = injector::MakeCALL(0x4C0F31, DrawRectHook).get(); //fmv

	injector::MakeCALL(0x474D36, DrawRectHook);
	//injector::MakeCALL(0x475D84, DrawRectHook);
	injector::MakeCALL(0x476A5B, DrawRectHook);
	injector::MakeCALL(0x48ABA3, DrawRectHook);
	injector::MakeCALL(0x48AC11, DrawRectHook);
	injector::MakeCALL(0x4AAC2F, DrawRectHook);
	injector::MakeCALL(0x4C0F31, DrawRectHook);
	injector::MakeCALL(0x4C1021, DrawRectHook);
	injector::MakeCALL(0x58DE69, DrawRectHook);
	injector::MakeCALL(0x58DFC9, DrawRectHook);
	injector::MakeCALL(0x58E2E1, DrawRectHook);
	injector::MakeCALL(0x58E30E, DrawRectHook);
	injector::MakeCALL(0x592428, DrawRectHook);
	injector::MakeCALL(0x59400B, DrawRectHook);
	//injector::MakeCALL(0x59CEB0, DrawRectHook);
	injector::MakeCALL(0x59D366, DrawRectHook);
	injector::MakeCALL(0x59D956, DrawRectHook);
	injector::MakeCALL(0x59DFCE, DrawRectHook);
	//injector::MakeCALL(0x5C3B6D, DrawRectHook); //related to player's shadow
	//injector::MakeCALL(0x5C3FC9, DrawRectHook);
	//injector::MakeCALL(0x5C3FFC, DrawRectHook);
	//injector::MakeCALL(0x5C402F, DrawRectHook);
	//injector::MakeCALL(0x5C4062, DrawRectHook);
	injector::MakeCALL(0x5CB59D, DrawRectHook);
	///injector::MakeCALL(0x5D571C, DrawRectHook);
	injector::MakeCALL(0x5D57EB, DrawRectHook);
	injector::MakeCALL(0x5D5AB6, DrawRectHook);
	injector::MakeCALL(0x5D5CBC, DrawRectHook);
	injector::MakeCALL(0x5D625D, DrawRectHook);
	injector::MakeCALL(0x5D62EC, DrawRectHook);
	injector::MakeCALL(0x5D6338, DrawRectHook);
	injector::MakeCALL(0x5D636C, DrawRectHook);
	injector::MakeCALL(0x5D7369, DrawRectHook);
	injector::MakeCALL(0x5D7471, DrawRectHook);
	//injector::MakeCALL(0x5D74A8, DrawRectHook);
	injector::MakeCALL(0x5D7504, DrawRectHook);
	injector::MakeCALL(0x5D78BE, DrawRectHook);
	injector::MakeCALL(0x5D823F, DrawRectHook);
	injector::MakeCALL(0x5D94A5, DrawRectHook);
	////injector::MakeCALL(0x5DB0C4, DrawRectHook);
	injector::MakeCALL(0x5DB632, DrawRectHook);
	injector::MakeCALL(0x5DB728, DrawRectHook);
	injector::MakeCALL(0x5DB820, DrawRectHook);
	injector::MakeCALL(0x5DB918, DrawRectHook);
	injector::MakeCALL(0x5DBA20, DrawRectHook);
	injector::MakeCALL(0x5DBB24, DrawRectHook);
	injector::MakeCALL(0x5DBC2C, DrawRectHook);
	injector::MakeCALL(0x5DBD34, DrawRectHook);
	injector::MakeCALL(0x5DDBE4, DrawRectHook);
	injector::MakeCALL(0x5DEB57, DrawRectHook);
	injector::MakeCALL(0x5DEBD7, DrawRectHook);
	injector::MakeCALL(0x5DEC18, DrawRectHook);
	injector::MakeCALL(0x5DEC5D, DrawRectHook);
	injector::MakeCALL(0x5DEFBE, DrawRectHook);
	injector::MakeCALL(0x5DF007, DrawRectHook);
	injector::MakeCALL(0x5E03A6, DrawRectHook);
	injector::MakeCALL(0x5E1062, DrawRectHook);
	injector::MakeCALL(0x5E14CE, DrawRectHook);
	injector::MakeCALL(0x5E1ED0, DrawRectHook);
	injector::MakeCALL(0x5E2652, DrawRectHook);
	injector::MakeCALL(0x5E26DC, DrawRectHook);
	injector::MakeCALL(0x5E5347, DrawRectHook);
	injector::MakeCALL(0x5E53B3, DrawRectHook);
	injector::MakeCALL(0x5EEB8A, DrawRectHook);
	injector::MakeCALL(0x5EEBBF, DrawRectHook);
	injector::MakeCALL(0x5EF02C, DrawRectHook);
	injector::MakeCALL(0x5EF1C7, DrawRectHook);
	injector::MakeCALL(0x5EF347, DrawRectHook);
	injector::MakeCALL(0x5EF38F, DrawRectHook);
	injector::MakeCALL(0x5F00F8, DrawRectHook);
	injector::MakeCALL(0x5F012E, DrawRectHook);
	injector::MakeCALL(0x5F01FA, DrawRectHook);
	injector::MakeCALL(0x5F085E, DrawRectHook);
	injector::MakeCALL(0x5F0F05, DrawRectHook);
	injector::MakeCALL(0x5F0F7A, DrawRectHook);
	injector::MakeCALL(0x5F1862, DrawRectHook);
	injector::MakeCALL(0x5F191D, DrawRectHook);
	injector::MakeCALL(0x5F1968, DrawRectHook);
	injector::MakeCALL(0x5F1E31, DrawRectHook);
	injector::MakeCALL(0x5F2A70, DrawRectHook);
	injector::MakeCALL(0x5F2AF8, DrawRectHook);
	injector::MakeCALL(0x5F2B4E, DrawRectHook);
	injector::MakeCALL(0x5F2BDE, DrawRectHook);
	injector::MakeCALL(0x5F3C1C, DrawRectHook);
	injector::MakeCALL(0x5F3D3C, DrawRectHook);
	injector::MakeCALL(0x5F3DCB, DrawRectHook);
	injector::MakeCALL(0x5F4344, DrawRectHook);
	injector::MakeCALL(0x5F4450, DrawRectHook);
	injector::MakeCALL(0x5F44CD, DrawRectHook);
	injector::MakeCALL(0x5F46D0, DrawRectHook);
	injector::MakeCALL(0x5F47F4, DrawRectHook);
	injector::MakeCALL(0x5F4883, DrawRectHook);
	injector::MakeCALL(0x5F4B1D, DrawRectHook);
	injector::MakeCALL(0x5F4C3C, DrawRectHook);
	injector::MakeCALL(0x5F4CC9, DrawRectHook);
	injector::MakeCALL(0x5F51FF, DrawRectHook);
	injector::MakeCALL(0x5F531D, DrawRectHook);
	injector::MakeCALL(0x5F53AA, DrawRectHook);
	injector::MakeCALL(0x5F54D5, DrawRectHook);
	injector::MakeCALL(0x5F558D, DrawRectHook);
	injector::MakeCALL(0x5FE993, DrawRectHook);
	injector::MakeCALL(0x5FEB6A, DrawRectHook);
	injector::MakeCALL(0x5FF25B, DrawRectHook);
	injector::MakeCALL(0x5FF406, DrawRectHook);
	injector::MakeCALL(0x5FF43B, DrawRectHook);
	injector::MakeCALL(0x60073B, DrawRectHook);
	injector::MakeCALL(0x600CB3, DrawRectHook);
	injector::MakeCALL(0x6014A2, DrawRectHook);
	injector::MakeCALL(0x601C9E, DrawRectHook);
	injector::MakeCALL(0x601F85, DrawRectHook);
	injector::MakeCALL(0x601FEE, DrawRectHook);
	injector::MakeCALL(0x603C43, DrawRectHook);
	injector::MakeCALL(0x603D34, DrawRectHook);
	injector::MakeCALL(0x603D5D, DrawRectHook);
	injector::MakeCALL(0x603DA0, DrawRectHook);
	injector::MakeCALL(0x603DC8, DrawRectHook);
	injector::MakeCALL(0x603DF7, DrawRectHook);
	injector::MakeCALL(0x60460E, DrawRectHook);
	injector::MakeCALL(0x60478E, DrawRectHook);
	injector::MakeCALL(0x6047FF, DrawRectHook);
	injector::MakeCALL(0x608841, DrawRectHook);
}

void FixTextPos()
{
	hbDrawText.fun = injector::MakeCALL(0x5E5B5B, DrawTextHook).get();
	//injector::MakeCALL(0x58E224, DrawTextHook);
	//injector::MakeCALL(0x59D1B1, DrawTextHook);
	injector::MakeCALL(0x5E5B5B, DrawTextHook);
	/*injector::MakeCALL(0x5F1463, DrawTextHook);
	injector::MakeCALL(0x5F15D0, DrawTextHook);
	injector::MakeCALL(0x5F7C72, DrawTextHook);
	injector::MakeCALL(0x5F7E64, DrawTextHook);
	injector::MakeCALL(0x5FF4B9, DrawTextHook);
	injector::MakeCALL(0x5FF52F, DrawTextHook);*/

}

void FixAspectRatio()
{
	Screen.Width = *(int *)0x829584;
	Screen.Height = *(int *)0x829588;
	Screen.fUnk1 = *(float *)0x7D3450;
	Screen.fFieldOfView = *(float *)0x715C94;
	Screen.fWidth = static_cast<float>(Screen.Width);
	Screen.fHeight = static_cast<float>(Screen.Height);
	Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
	Screen.f2DSpritesOffset = ((Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f) / Screen.fWidth;
	Screen.f2DSpritesScale = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));

	//Disable menu aspect ratio switch
	injector::MakeNOP(0x5DAA20, 9, true);
	injector::WriteMemory<unsigned char>(0x5DAA30, 0xC3, true); //ret

	Screen.fHudHorScale = static_cast<float>(Screen.fHeight / ((Screen.fHeight / (640.0f / Screen.fAspectRatio)) / (Screen.fUnk1) * Screen.fWidth));
	injector::WriteMemory(0x7D3458, Screen.fHudHorScale, true);

	Screen.fHudVerScale = 0.7f * (Screen.fAspectRatio / (4.0f / 3.0f));
	injector::WriteMemory(0x475BE2 + 0x2, &Screen.fHudVerScale, true);
	injector::WriteMemory(0x475C09 + 0x2, &Screen.fHudVerScale, true);
	injector::WriteMemory(0x476246 + 0x1, &Screen.fHudVerScale, true);

	injector::WriteMemory<unsigned char>(0x475BF3, 0xEB);
	static float fScreenWidthScale = 0.546875f;
	injector::WriteMemory(0x475C1B + 0x2, &fScreenWidthScale, true);

	Fix2DSprites();
}

void ApplyIniOptions()
{
	if (fFOVFactor)
	{
		injector::MakeJMP(0x475C3C, setScreenFieldOfView, true);
	}

	if (strncmp(szCustomUserFilesDirectory, "0", 1) != 0)
	{
		char			moduleName[MAX_PATH];
		GetModuleFileName(NULL, moduleName, MAX_PATH);
		char* tempPointer = strrchr(moduleName, '\\');
		*(tempPointer + 1) = '\0';
		strcat(moduleName, szCustomUserFilesDirectory);
		strcpy(szCustomUserFilesDirectory, moduleName);

		injector::MakeCALL(0x605E0B, InitUserDirectories, true);
	}

	if (bDisableGamepadInput)
	{
		injector::MakeNOP(0x491DDE, 6, true);
		injector::MakeNOP(0x491DE7, 6, true);
		injector::MakeNOP(0x491DED, 10, true);
	}
}

DWORD WINAPI Init(LPVOID)
{
	CIniReader iniReader("");
	fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 0.0f);
	bDisableGamepadInput = iniReader.ReadInteger("MAIN", "DisableGamepadInput", 0) != 0;
	szCustomUserFilesDirectory = iniReader.ReadString("MAIN", "CustomUserFilesDirectoryInGameDir", "");

	auto pattern = hook::pattern("6A 02 6A 00 6A 00 68 01 20 00 00");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("6A 02 6A 00 6A 00 68 01 20 00 00");

		while (*(float *)0x7D3450 == 0.0f) //Screen.fUnk1
			Sleep(0);

		FixAspectRatio(); //steam version is being weird for some reason
	}
	else
		injector::MakeCALL(0x5E2579, FixAspectRatio, true);

	ApplyIniOptions();
	
	//FixText();

	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init(NULL);
	}
	return TRUE;
}