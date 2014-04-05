#include "stdafx.h"
#include "stdio.h"
#include <windows.h>
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"

HMODULE h_e2mfc_dll = NULL;
HMODULE comics = NULL;

float res1 = 360.0f; //
float half_res1 = 180.0f; //
float door_fix = 0.0020833334f; //
float height_multipl;
float mul2; //
float div2; //
float div_comics;

int width = 0;
int height = 0;
float aspect_ratio = 0.0f;
int FOV_ptr;
float shadows_fix;
DWORD jmpAddress;
int counter;
float ini_FOV;

int __fastcall P_Driver__getWidth(int a1)
{
	int result; // eax@2

	if (*(DWORD *)(a1 + 48))
		result = *(DWORD *)(a1 + 52);
	else
		result = *(DWORD *)(a1 + 4);

	width = result;
	return result;
}


int __fastcall P_Driver__getHeight(int a1)
{
	int result; // eax@2

	if (*(DWORD *)(a1 + 48))
		result = *(DWORD *)(a1 + 56);
	else
		result = *(DWORD *)(a1 + 8);

	height = result;
	return result;
}


#define _USE_MATH_DEFINES
#include "math.h"
#define DEGREE_TO_RADIAN(fAngle) \
	((fAngle)* (float)M_PI / 180.0f)
#define RADIAN_TO_DEGREE(fAngle) \
	((fAngle)* 180.0f / (float)M_PI)
#define SCREEN_FOV_HORIZONTAL		1.221730471f
#define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / (4.0f / 3.0f))))	// Default is ~55.0f.

void setFOV()
{
	if (ini_FOV)
	{
		CPatch::SetFloat(FOV_ptr, ini_FOV);
		return;
	}

	float fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * aspect_ratio));

	CPatch::SetFloat(FOV_ptr, fDynamicScreenFieldOfViewScale); //1.221730471 4/3
	return;
}

void __declspec(naked)patch_FOV()
{
	__asm test esi, esi
	__asm jz loc_111
	__asm mov FOV_ptr, esi
	__asm add FOV_ptr, 0x58
loc_111:
	__asm mov[esi + 0x1C], eax
	__asm mov[esi + 0x20], eax
	__asm mov jmpAddress, 0x50A181
	__asm jmp jmpAddress
}

DWORD getScreenRectjmp;
void __declspec(naked)count_getScreenRect()
{
	__asm inc counter
	__asm mov     ebx, 0C00h
	__asm jmp getScreenRectjmp
}

void __declspec(naked)count_getScreenRect_default()
{
	__asm mov     ebx, 0C00h
	__asm jmp getScreenRectjmp
}

void __declspec(naked)DisableSubViewport()
{
	__asm ret 10h
}

DWORD WINAPI Thread(LPVOID)
{
	CIniReader iniReader("");
	ini_FOV = iniReader.ReadFloat("MAIN", "FOV", 0.0f);

	CPatch::RedirectJump(0x50A17B, patch_FOV); //fov hack

	do
	{
		Sleep(100);
		h_e2mfc_dll = GetModuleHandle("e2mfc.dll");
	} while (h_e2mfc_dll == NULL);

	CPatch::RedirectCall((DWORD)h_e2mfc_dll + 0x15582, P_Driver__getWidth);
	CPatch::RedirectCall((DWORD)h_e2mfc_dll + 0x155AB, P_Driver__getHeight);
	comics = GetModuleHandle("e2_d3d8_driver_mfc.dll") + (0x845E8 / 4);

	getScreenRectjmp = (DWORD)h_e2mfc_dll + 0x1478B;
	CPatch::RedirectJump((DWORD)h_e2mfc_dll + 0x14786, count_getScreenRect);

	CPatch::RedirectJump((DWORD)h_e2mfc_dll + 0x13FB0, DisableSubViewport);

	do
	{
		Sleep(0);
	} while (width == 0 || height == 0);


	aspect_ratio = (float)width / (float)height;

	res1 = 640.0f / aspect_ratio; // 640.0 original
	half_res1 = res1 / 2.0f; // 480.0 original
	height_multipl = 1.0f / res1;
	mul2 = 1.0f / res1 * 2.0f;
	div2 = 1.0f / res1 / 2.0f;
	div_comics = 1.0f / 480.0f / 2.0f;

	float div_comics2 = 1.0f / 640.0f / (640.0f / 480.0f) / 2.0f;
	float div22 = 1.0f / 640.0f / 2.0f;

			if (aspect_ratio >= 1.4f) //WS
			{
				//door fix
				CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x148ED + 0x2, &door_fix);
				//end

				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x12F6C, res1);
				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x13B5F, res1);
				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x15AC6, res1);
				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x15AF0, res1);
				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x16A6A, res1);
				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x16BDD, res1);
				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DDC, res1);
				//shadows fix
				//shadows_fix = res1 + 30.0;
				//changePointer((DWORD)h_e2mfc_dll + 0x1779A, &shadows_fix, 2); //mp2

				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DF0, half_res1);
				//Sleep(3000);
				//CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DE4, height_multipl); //D3DERR_INVALIDCALL

				CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x13D1D + 0x2, &height_multipl);
				CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x13D82 + 0x2, &height_multipl);
				CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x14238 + 0x2, &height_multipl);
				CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x1429D + 0x2, &height_multipl); 
				CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x146FA + 0x2, &height_multipl);
				//CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x14775 + 0x2, &height_multipl);
				CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x148ED + 0x2, &height_multipl);
				CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x14968 + 0x2, &height_multipl);
				CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x1566C + 0x2, &height_multipl);

				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DEC, mul2);
				CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DFC, div2);


				setFOV();

				
				while (true)
				{
					Sleep(0);

					if (counter > (iniReader.ReadInteger("MAIN", "LOADING_DELAY", 300)))
					{
						CPatch::SetPointer((DWORD)h_e2mfc_dll + 0x14775 + 0x2, &height_multipl);
						CPatch::RedirectJump((DWORD)h_e2mfc_dll + 0x14786, count_getScreenRect_default);
						counter = 0;
					}

					if ((unsigned char)*(DWORD*)comics == 255u)
					{
						CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49E00, div_comics2);
						CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DFC, div_comics);
					}
					else
					{
						CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49E00, div22);
						CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DFC, div2);
					}
				}
			}
	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}