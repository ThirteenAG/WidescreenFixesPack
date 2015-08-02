#include "stdafx.h"
#include "..\includes\CPatch.h"
#include "..\includes\IniReader.h"
#include <stdio.h>
#pragma warning(disable: 4733)



void Init();
void ClearLog();
void WriteLog(const char*, ...);
void ReadINI(char*);
void DetectGameVersion();
void SetGameHooks();
void FOVHook();
void HUDHook();
void MapHook();
void TextHook();
void MenuHook();
void MovieHook();
void VDHook();
void OtherHook();
void CalcGameValues();
void HUDCalcValues();
void MapCalcValues();
void TextCalcValues();
void MenuCalcValues();
void MovieCalcValues();
void VDCalcValues();
void OtherCalcValues();

// ClearLog(), WriteLog()
FILE *LogFile;

// Init()
enum { MAFIA_UNKNOWN = -1, MAFIA_1_0_ENG, MAFIA_1_1_ENG, MAFIA_1_2_ENG };
int game_version;
int v10_sub_0062439A[3] = { 0x0062439A, 0x0060F508, 0x0060FCD8 };
unsigned int LS3DF_base_addr;

// ReadINI()
int fov_patch;
int hud_patch;
int hud_stretch;
int map_patch;
int map_stretch;
int text_patch;
int text_stretch;
int menu_patch;
int movie_patch;
int movie_stretch;
int vd_patch;
float vd_value;
float vd_min;
float vd_max;
int write_log;
int fps_limit;
int ru_credits;


// SetGameHooks()
void Get_Game_Resolution_1_0_ENG(), Get_Game_Resolution_1_1_ENG(), Get_Game_Resolution_1_2_ENG();
int Get_Game_Resolution[3] = { (int)Get_Game_Resolution_1_0_ENG, (int)Get_Game_Resolution_1_1_ENG, (int)Get_Game_Resolution_1_2_ENG };
int sub_005BF074[3] = { 0x005FA58C, 0x005BF074, 0x005BF814 };

// FOVHook()
void FOV_Hook_1_0_ENG(), FOV_Hook_1_1_ENG(), FOV_Hook_1_2_ENG();
int FOV_Hook[3] = { (int)FOV_Hook_1_0_ENG, (int)FOV_Hook_1_1_ENG, (int)FOV_Hook_1_2_ENG };
int v10_sub_1000ABEE[3] = { 0x0000ABEE, NULL, NULL };
int v10_sub_1000ACA1[3] = { 0x0000ACA1, NULL, NULL };
int v10_sub_1000B0AC[3] = { 0x0000B0AC, NULL, NULL };
int v10_sub_1000B166[3] = { 0x0000B166, NULL, NULL };
int v10_sub_1000B34C[3] = { 0x0000B34C, NULL, NULL };
int v10_sub_1000B406[3] = { 0x0000B406, NULL, NULL };
int v10_sub_1000B5C4[3] = { 0x0000B5C4, NULL, NULL };
int v10_sub_1000B677[3] = { 0x0000B677, NULL, NULL };
int v10_sub_1000B834[3] = { 0x0000B834, NULL, NULL };
int v10_sub_1000B8E7[3] = { 0x0000B8E7, NULL, NULL };
int v10_sub_1000BA9A[3] = { 0x0000BA9A, NULL, NULL };
int v10_sub_1000BB54[3] = { 0x0000BB54, NULL, NULL };
int v10_sub_1000BCC6[3] = { 0x0000BCC6, NULL, NULL };
int v10_sub_1000BD7D[3] = { 0x0000BD7D, NULL, NULL };
int v10_sub_1000BF20[3] = { 0x0000BF20, NULL, NULL };
int v10_sub_1000BFD7[3] = { 0x0000BFD7, NULL, NULL };
int v11_sub_1000B0B9[3] = { NULL, 0x0000B0B9, 0x0000B519 };
int v11_sub_1000B1A4[3] = { NULL, 0x0000B1A4, 0x0000B604 };

// HUDHook()
int sub_00600FBF[3] = { 0x005533A7, 0x00600FBF, 0x006017AF };
int sub_00601003[3] = { 0x005533EB, 0x00601003, 0x006017F3 };
// *HUD*
int sub_0060116A[3] = { 0x0055340C, 0x0060116A, 0x0060195A };
int sub_006011AB[3] = { 0x0055344D, 0x006011AB, 0x0060199B };
int sub_00601B21[3] = { 0x00553C2D, 0x00601B21, 0x00602311 };
int sub_00601BB5[3] = { 0x00553CAD, 0x00601BB5, 0x006023A5 };

int sub_005FE753[3] = { 0x00550EF4, 0x005FE753, 0x005FEF43 };
int sub_005FD5CF[3] = { 0x0054FDBC, 0x005FD5CF, 0x005FDDBF };
int sub_005FC142[3] = { 0x0054E9F5, 0x005FC142, 0x005FC932 };
int sub_005FC333[3] = { 0x0054EBD4, 0x005FC333, 0x005FCB23 };
int sub_005FCA99[3] = { 0x0054F2F3, 0x005FCA99, 0x005FD289 };
int sub_005F71EF[3] = { 0x00549ACF, 0x005F71EF, 0x005F79DF };
int sub_005F73EF[3] = { 0x00549CD3, 0x005F73EF, 0x005F7BDF };
int sub_005F72E6[3] = { 0x00549BC6, 0x005F72E6, 0x005F7AD6 };
int sub_005F77A9[3] = { 0x0054A089, 0x005F77A9, 0x005F7F99 };
int sub_005FB757[3] = { 0x0054E022, 0x005FB757, 0x005FBF47 };
int v10_sub_0054ED61[3] = { 0x0054ED61, NULL, NULL };
int sub_005FCD62[3] = { 0x0054F5AA, 0x005FCD62, 0x005FD552 };
int sub_005FD0CB[3] = { 0x0054F8F5, 0x005FD0CB, 0x005FD8BB };
int sub_005FCCFE[3] = { 0x0054F54C, 0x005FCCFE, 0x005FD4EE };
int sub_005FC81D[3] = { 0x0054F086, 0x005FC81D, 0x005FD00D };
// *Race Mission Specific*
int sub_006000F0[3] = { 0x0055274A, 0x006000F0, 0x006008E0 };
int sub_00600331[3] = { 0x00552954, 0x00600331, 0x00600B21 };
int sub_006006AE[3] = { 0x00552C90, 0x006006AE, 0x00600E9E };
int sub_00600609[3] = { 0x00552BFE, 0x00600609, 0x00600DF9 };
int sub_006004F0[3] = { 0x00552AEC, 0x006004F0, 0x00600CE0 };
// *Map*
int sub_005FF933[3] = { 0x00552017, 0x005FF933, 0x00600123 };
// *Black Borders, Text*
int sub_005F9869[3] = { 0x0054C189, 0x005F9869, 0x005FA059 };
int sub_0046DA4F[3] = { 0x005BC418, 0x0046DA4F, 0x0046E60F };
int sub_00548C1D[3] = { 0x00594899, 0x00548C1D, 0x005492AD };
int sub_005F661C[3] = { 0x00548E6C, 0x005F661C, 0x005F6E0C };
int sub_005F8EFB[3] = { 0x0054B80B, 0x005F8EFB, 0x005F96EB };

int sub_0060DD09[3] = { 0x005601D9, 0x0060DD09, 0x0060E1F9 };
int sub_0060DD6D[3] = { 0x00560237, 0x0060DD6D, 0x0060E25D };
int sub_0060DDD1[3] = { 0x00560295, 0x0060DDD1, 0x0060E2C1 };
int sub_0060DE23[3] = { 0x005602E7, 0x0060DE23, 0x0060E313 };
int sub_0060DEA6[3] = { 0x00560364, 0x0060DEA6, 0x0060E396 };
int sub_0060DEE1[3] = { 0x00560399, 0x0060DEE1, 0x0060E3D1 };
int sub_0060DF72[3] = { 0x0056042C, 0x0060DF72, 0x0060E462 };
int sub_0060DFA8[3] = { 0x00560461, 0x0060DFA8, 0x0060E498 };
int v12_sub_0060E4C7[3] = { NULL, NULL, 0x0060E4C7 };
int sub_0060DFFF[3] = { 0x005604BD, 0x0060DFFF, 0x0060E523 };
int sub_0060E05D[3] = { 0x00560510, 0x0060E05D, 0x0060E57C };
// *Menus*
int sub_005A3741[3] = { 0x0055F47B, 0x005A3741, 0x005A3D31 };
int sub_0060CEF5[3] = { 0x005DF781, 0x0060CEF5, 0x0060D3E5 };

// MapHook()
// *Map*
void Map_Hook_1_0_ENG_Player_Marker_x(), Map_Hook_1_1_ENG_Player_Marker_x();
int Map_Hook_Player_Marker_x[3] = { (int)Map_Hook_1_0_ENG_Player_Marker_x, (int)Map_Hook_1_1_ENG_Player_Marker_x, (int)Map_Hook_1_1_ENG_Player_Marker_x };
int sub_005FF0B5[3] = { 0x005517D1, 0x005FF0B5, 0x005FF8A5 };
int sub_005FF0C9[3] = { 0x00551838, 0x005FF0C9, 0x005FF8B9 };
int sub_005FF5C7[3] = { 0x00551CD5, 0x005FF5C7, 0x005FFDB7 };
int sub_005FFA6A[3] = { 0x0055214C, 0x005FFA6A, 0x0060025A };

int sub_005FF022[3] = { 0x0055178D, 0x005FF022, 0x005FF812 };
int sub_005FF1F4[3] = { 0x0055191D, 0x005FF1F4, 0x005FF9E4 };
int sub_005FEFD4[3] = { 0x0055173F, 0x005FEFD4, 0x005FF7C4 };
int sub_005FF5B3[3] = { 0x00551CC1, 0x005FF5B3, 0x005FFDA3 };
int sub_005FF978[3] = { 0x00552056, 0x005FF978, 0x00600168 };
int sub_005FF037[3] = { 0x005517A2, 0x005FF037, 0x005FF827 };
int sub_005FF98E[3] = { 0x0055206C, 0x005FF98E, 0x0060017E };

// TextHook()
int sub_005F82D7[3] = { 0x0054ABC6, 0x005F82D7, 0x005F8AC7 };
int sub_005F833A[3] = { 0x0054AC3B, 0x005F833A, 0x005F8B2A };
int sub_005F85E7[3] = { 0x0054AEE8, 0x005F85E7, 0x005F8DD7 };
int sub_005F864D[3] = { 0x0054AF60, 0x005F864D, 0x005F8E3D };
int sub_005F9264[3] = { 0x0054BBA2, 0x005F9264, 0x005F9A54 };
int sub_005F92D4[3] = { 0x0054BC0B, 0x005F92D4, 0x005F9AC4 };

// MenuHook()
void Menu_Hook_1_0_ENG_1(), Menu_Hook_1_1_ENG_1(), Menu_Hook_1_2_ENG_1();
int Menu_Hook_1[3] = { (int)Menu_Hook_1_0_ENG_1, (int)Menu_Hook_1_1_ENG_1, (int)Menu_Hook_1_2_ENG_1 };
void Menu_Hook_1_0_ENG_2(), Menu_Hook_1_1_ENG_2(), Menu_Hook_1_2_ENG_2();
int Menu_Hook_2[3] = { (int)Menu_Hook_1_0_ENG_2, (int)Menu_Hook_1_1_ENG_2, (int)Menu_Hook_1_2_ENG_2 };
void Menu_Hook_1_0_ENG_3();
int Menu_Hook_3[3] = { (int)Menu_Hook_1_0_ENG_3, NULL, NULL };
int sub_005E3303[3] = { NULL, 0x005E3303, 0x005E3B03 };
int sub_005EB28B[3] = { 0x0056E525, 0x005EB28B, 0x005EBA8B };
int v10_sub_0056BF09[3] = { 0x0056BF09, NULL, NULL };
int v10_sub_0056BF95[3] = { 0x0056BF95, NULL, NULL };
int v10_sub_0056C007[3] = { 0x0056C007, NULL, NULL };
int v10_sub_0056C0AF[3] = { 0x0056C0AF, NULL, NULL };
int v10_sub_005758DF[3] = { 0x005758DF, NULL, NULL };
int v11_sub_005D8D5B[3] = { NULL, 0x005D8D5B, 0x005D950B };
int v11_sub_005D90CB[3] = { NULL, 0x005D90CB, 0x005D98DB };

// MovieHook()
void Movie_Hook_1_0_ENG(), Movie_Hook_1_1_ENG(), Movie_Hook_1_2_ENG();
int Movie_Hook[3] = { (int)Movie_Hook_1_0_ENG, (int)Movie_Hook_1_1_ENG, (int)Movie_Hook_1_2_ENG };
int sub_10071490[3] = { 0x00070430, 0x00071490, 0x000745A0 };

// VDHook()
void VD_Hook_1_0_ENG();
int VD_Hook[3] = { (int)VD_Hook_1_0_ENG, (int)VD_Hook_1_0_ENG, (int)VD_Hook_1_0_ENG };
int v10_sub_00600E99[3] = { 0x00600E99, 0x0054143E, 0x0054192E };

int v10_sub_005A89C9[3] = { 0x005A89C9, NULL, NULL };
int v11_sub_004021E1[3] = { NULL, 0x004021E1, 0x00402201 };

int v10_sub_005A87BC[3] = { 0x005A87BC, NULL, NULL };
int v10_sub_005A88FC[3] = { 0x005A88FC, NULL, NULL };
int v10_sub_005A8919[3] = { 0x005A8919, NULL, NULL };
int v10_sub_005A892C[3] = { 0x005A892C, NULL, NULL };
int v10_sub_005A895F[3] = { 0x005A895F, NULL, NULL };
int v11_sub_004021E9[3] = { NULL, 0x004021E9, 0x00402209 };

// OtherHook()
int v10_sub_005F95BC[3] = { 0x005F95BC, 0x005BE02C, 0x005BE7CC };
void Other_Ru_Credits_Hook_1_0_ENG(), Other_Ru_Credits_Hook_1_1_ENG();
int Other_Ru_Credits_Hook[3] = { (int)Other_Ru_Credits_Hook_1_0_ENG, (int)Other_Ru_Credits_Hook_1_1_ENG, NULL };
int v10_sub_0056017C[3] = { 0x0056017C, 0x0060DCAC, NULL };
int v10_sub_00560548[3] = { 0x00560548, 0x0060E09C, NULL };

// int sub_[3] = { 0x, 0x, NULL };

// CalcValues()
float cur_game_width;
float cur_game_height;

float ori_width_p_height = 4.0f / 3.0f;
float cur_width_p_height;
float ori_height_p_width = 3.0f / 4.0f;
float cur_height_p_width;

float ori_base_width = 800.0f;
float cur_base_width;
float ori_base_height = 600.0f;
float cur_base_height = 600.0f;

float one_p_ori_base_width = 1.0f / ori_base_width;
float one_p_cur_base_width;
float one_p_ori_base_height = 1.0f / ori_base_height;
float one_p_cur_base_height;



// HUDCalcValues()
// *HUD*
float hud_speedometer_x = 189.0f;
float hud_tachometer_x = 259.0f;
float hud_compas_x = 0.0f;
float hud_minimap_x = 29.0f;
float hud_transmission_x_v10 = 758.0f;
float hud_transmission_x_v11 = 42.0f;
float hud_fuel_led_x_v10 = 668.0f;
float hud_fuel_led_x_v11 = 132.0f;
float hud_action_bar_x = 9.0f;
float hud_health_bar_x = 7.0f;
float hud_health_bar_text_x = 42.0f;
float hud_ammo_bar_x = 62.0f;
float hud_ammo_bar_text_x = 112.0f;
float hud_search_bar_x = 336.0f;
float hud_live_bar_x_v10 = 368.0f;
float hud_progress_bar_x = 250.0f;
float hud_progress_bar2_x = 350.0f;
float hud_left_popup_text_x = 9.0f;
float hud_money_right_text_x = 760.0f;
// *Race Mission Specific*
float hud_race_321go_x = 400.0f;
float hud_race_right_text_x = 780.0f;
// *Map*
float hud_map_text_x = 350.0f;
// *Borders, Text*
float hud_borders_width = 801.0f;
float hud_borders_1 = 0.540540516376495f;
float hud_borders_2 = 0.5f;
float hud_black_centred_text_x = 400.0f;
float hud_credits_text_1_x = 370.0f;
float hud_credits_text_2_x = 390.0f;
float hud_credits_text_3_x = 400.0f;
float hud_credits_text_4_x = 410.0f;
// *Menus*
float hud_please_wait_text_x = 780.0f;

// MapCalcValues()
// *Map*
float map_left_x;
float map_right_x;
float map_width;
float map_player_marker_x;
float map_aspect;
float map_scale_x;
float map_pos_x;
float map_target_pos_x;

// TextCalcValues()
float text_width = 0.9f;

// MenuCalcValues()
float menu_please_wait_text_x = 780.0f;
float menu_menu_x = 0.0f;

// MovieCalcValues()
float movie_aspect = 1.0f;

// VDCalcValues()
float vd_value1 = 200.0f;
float vd_value2 = 230.0f;

// OtherCalcValues()
char fps_limit_ms = 0x10; // 0x0 в версии 1.0



void Init() {
	ReadINI("Mafia_widescreen_fix.ini");
	ClearLog();
	DetectGameVersion();
	WriteLog("Detected Game.exe: %i\n", game_version);
	if (game_version != MAFIA_UNKNOWN) {
		unsigned int LS3DF_TEV_addr = **(unsigned int **)(v10_sub_0062439A[game_version] + 0x2);
		WriteLog("LS3DF.dll TEV func address: 0x%X\n", LS3DF_TEV_addr);
		HMODULE hLS3DF;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)LS3DF_TEV_addr, &hLS3DF);
		LS3DF_base_addr = (unsigned int)hLS3DF;
		WriteLog("LS3DF.dll base address: 0x%X\n", LS3DF_base_addr);
		if (LS3DF_base_addr) {
			SetGameHooks();
		}
		else {
			MessageBox(0, "Can't get LS3DF.dll base address.\nWideScreen Fix will be disabled.", "Mafia WideScreen Fix", 0);
		}
	}
	else {
		MessageBox(0, "This version of Game.exe is not supported.\nWideScreen Fix will be disabled.", "Mafia WideScreen Fix", 0);
	}
}


void ClearLog() {
	if (write_log) {
		fopen_s(&LogFile, "Mafia_widescreen_fix.log", "w");
		fclose(LogFile);
	}
}


void WriteLog(const char* str, ...) {
	if (write_log) {
		va_list args;
		va_start(args, str);
		fopen_s(&LogFile, "Mafia_widescreen_fix.log", "a");
		vfprintf(LogFile, str, args);
		fclose(LogFile);
	}
}


void ReadINI(char* name) {
	CIniReader iniReader(name);
	// WideScreen
	fov_patch = iniReader.ReadInteger("WideScreen", "FOV Hor+", 0);

	hud_patch = iniReader.ReadInteger("WideScreen", "Fix HUD", 0);
	hud_stretch = iniReader.ReadInteger("WideScreen", "Wide HUD", 1);

	map_patch = iniReader.ReadInteger("WideScreen", "Fix Map", 0);
	map_stretch = iniReader.ReadInteger("WideScreen", "Wide Map", 1);

	text_patch = iniReader.ReadInteger("WideScreen", "Fix Text", 0);
	text_stretch = iniReader.ReadInteger("WideScreen", "Wide Text", 1);

	menu_patch = iniReader.ReadInteger("WideScreen", "Fix Menu", 0);

	movie_patch = iniReader.ReadInteger("WideScreen", "Fix Movie", 0);
	movie_stretch = iniReader.ReadInteger("WideScreen", "Wide Movie", 1);
	// View Distance
	vd_patch = iniReader.ReadInteger("View Distance", "Change Distance", 0);
	vd_value = iniReader.ReadFloat("View Distance", "New Distance", 600.0f);
	vd_min = iniReader.ReadFloat("View Distance", "Skip Min Distance", 20.0f);
	vd_max = iniReader.ReadFloat("View Distance", "Skip Max Distance", 450.0f);
	// Other
	write_log = iniReader.ReadInteger("Other", "Write Log", 1);
	fps_limit = iniReader.ReadInteger("Other", "FPS Limit", 0);
	ru_credits = iniReader.ReadInteger("Other", "Ru Credits Fix", 0);
}


void DetectGameVersion() {
	if      ((*(unsigned int *)0x0063C000) == 0x004C6150) { game_version = MAFIA_1_0_ENG; }	// 1.0 Eng
	else if ((*(unsigned int *)0x00623000) == 0x0044D570) { game_version = MAFIA_1_1_ENG; }	// 1.1 Eng, 1.1 Rus 1C
	else if ((*(unsigned int *)0x00624000) == 0x0044E120) { game_version = MAFIA_1_2_ENG; }	// 1.2 Eng, 1.2 Rus 1C
	else                                                  { game_version = MAFIA_UNKNOWN; }	// Unknown game version
}


void SetGameHooks() {
	// Ставим главный хук - получаем разрешение игры и вызываем инструкцию расчета новых значений - CalcGameValues()
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::RedirectCall(sub_005BF074[game_version], (void *)Get_Game_Resolution[game_version]);
	}
	else {
		CPatch::RedirectCall(sub_005BF074[game_version], (void *)Get_Game_Resolution[game_version]);
		CPatch::Nop(sub_005BF074[game_version] + 0x5, 0x6);
	}
	// Ставим хуки на исправление FOV
	if (fov_patch) { FOVHook(); }
	// Ставим хуки на исправление HUD
	if (hud_patch) { HUDHook(); }
	// Ставим хуки на исправление Map
	if (map_patch) { MapHook(); }
	// Ставим хуки на исправление разделителя текста
	if (text_patch) { TextHook(); }
	// Ставим хуки на исправление загрузочных экранов и меню
	if (menu_patch) { MenuHook(); }
	// Ставим хуки на исправление пропорций видеороликов
	if (movie_patch) { MovieHook(); }
	// Ставим хуки на изменение дальности прорисовки
	if (vd_patch) { VDHook(); }

	OtherHook();
}


void FOVHook() {
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000ABEE[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000ABEE[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000ACA1[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000ACA1[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000B0AC[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000B0AC[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000B166[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000B166[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000B34C[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000B34C[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000B406[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000B406[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000B5C4[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000B5C4[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000B677[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000B677[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000B834[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000B834[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000B8E7[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000B8E7[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000BA9A[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000BA9A[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000BB54[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000BB54[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000BCC6[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000BCC6[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000BD7D[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000BD7D[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000BF20[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000BF20[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v10_sub_1000BFD7[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v10_sub_1000BFD7[game_version] + 0x5, 0x1);
	}
	else {
		CPatch::RedirectCall(LS3DF_base_addr + v11_sub_1000B0B9[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v11_sub_1000B0B9[game_version] + 0x5, 0x1);
		CPatch::RedirectCall(LS3DF_base_addr + v11_sub_1000B1A4[game_version], (void *)FOV_Hook[game_version]);
		CPatch::Nop(LS3DF_base_addr + v11_sub_1000B1A4[game_version] + 0x5, 0x1);
	}
}


void HUDHook() {
	// Устанавливаем правильные пропорции:
	CPatch::SetPointer(sub_00600FBF[game_version] + 0x2, &one_p_cur_base_width);	// HUD, титры авторов... 
	CPatch::SetPointer(sub_00601003[game_version] + 0x2, &one_p_cur_base_width);	// HUD: спидометр, тахометр, часы, значек ограничителя скорости. Расчитывется только при старте игры

// *HUD*
	// Спидометр:
	CPatch::SetPointer(sub_0060116A[game_version] + 0x2, &hud_speedometer_x);
	// Тахометр:
	CPatch::SetPointer(sub_006011AB[game_version] + 0x2, &hud_tachometer_x);
	// Значек ограничителя скорости:
	// Часы:

	// Компас:
	CPatch::SetPointer(sub_005FE753[game_version] + 0x2, &hud_compas_x);
	// МиниКарта:
	CPatch::SetPointer(sub_005FD5CF[game_version] + 0x2, &hud_minimap_x);
	// Тип коробки передач:
	if (game_version == MAFIA_1_0_ENG) { CPatch::SetPointer(sub_005FC142[game_version] + 0x2, &hud_transmission_x_v10); }
	else { CPatch::SetPointer(sub_005FC142[game_version] + 0x2, &hud_transmission_x_v11); }
	// Индикатор отсутствия топлива:
	if (game_version == MAFIA_1_0_ENG) { CPatch::SetPointer(sub_005FC333[game_version] + 0x2, &hud_fuel_led_x_v10); }
	else { CPatch::SetPointer(sub_005FC333[game_version] + 0x2, &hud_fuel_led_x_v11); }
	// Значек действия:
	CPatch::SetPointer(sub_005FCA99[game_version] + 0x2, &hud_action_bar_x);
	// Значек здоровья персонажей (без текста):
	CPatch::SetPointer(sub_005F71EF[game_version] + 0x2, &hud_health_bar_x);
	// Значек здоровья персонажей (текст):
	CPatch::SetPointer(sub_005F73EF[game_version] + 0x2, &hud_health_bar_text_x);
	// Значек боеприпасов (без текста):
	CPatch::SetPointer(sub_005F72E6[game_version] + 0x2, &hud_ammo_bar_x);
	// Значек боеприпасов (текст):
	CPatch::SetPointer(sub_005F77A9[game_version] + 0x2, &hud_ammo_bar_text_x);
	// Значек поиска (розыск):
	CPatch::SetPointer(sub_005FB757[game_version] + 0x2, &hud_search_bar_x);
	// Значек "Здоровье" (у машины):
	if (game_version == MAFIA_1_0_ENG) { CPatch::SetPointer(v10_sub_0054ED61[game_version] + 0x2, &hud_live_bar_x_v10); }
	// Полоса действия (взлом машины или при замехе кулаком, битой):
	CPatch::SetPointer(sub_005FCD62[game_version] + 0x2, &hud_progress_bar_x);
	// Полоса действия 2 (самолет в миссии Сливки общества - Аэропорт):
	CPatch::SetPointer(sub_005FD0CB[game_version] + 0x2, &hud_progress_bar2_x);
	// Всплывающий текст слева:
	CPatch::SetPointer(sub_005FCCFE[game_version] + 0x2, &hud_left_popup_text_x);
	// Деньги, текст справа сверху:
	CPatch::SetPointer(sub_005FC81D[game_version] + 0x2, &hud_money_right_text_x);

// *Race Mission Specific*
	// Гонка "3, 2, 1, Go":
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::SetPointer(sub_006000F0[game_version] + 0x2, &hud_race_321go_x);
		CPatch::SetPointer(sub_00600331[game_version] + 0x2, &hud_race_321go_x);
	}
	// Гонка, "Едем не туда":
	// Гонка, текст по центру ("Контрольная точка"):
	// Гонка, текст справа сверху:
	CPatch::SetPointer(sub_006004F0[game_version] + 0x2, &hud_race_right_text_x);

// *Map*
	// Надпись "Нет карты":
	CPatch::SetPointer(sub_005FF933[game_version] + 0x2, &hud_map_text_x);

// *Black Borders, Text*
	// Черные полосы:
	CPatch::SetPointer(sub_005F9869[game_version] + 0x2, &hud_borders_width);
	// Текст в центре на черном фоне:
	CPatch::SetPointer(sub_005F8EFB[game_version] + 0x2, &hud_black_centred_text_x);
	// Титры:
	CPatch::SetPointer(sub_0060DD09[game_version] + 0x2, &hud_credits_text_2_x);
	CPatch::SetPointer(sub_0060DD6D[game_version] + 0x2, &hud_credits_text_2_x);
	CPatch::SetPointer(sub_0060DDD1[game_version] + 0x2, &hud_credits_text_2_x);
	CPatch::SetPointer(sub_0060DE23[game_version] + 0x2, &hud_credits_text_1_x);
	CPatch::SetPointer(sub_0060DEA6[game_version] + 0x2, &hud_credits_text_3_x);
	CPatch::SetPointer(sub_0060DEE1[game_version] + 0x2, &hud_credits_text_3_x);
	CPatch::SetPointer(sub_0060DF72[game_version] + 0x2, &hud_credits_text_2_x);
	CPatch::SetPointer(sub_0060DFFF[game_version] + 0x2, &hud_credits_text_1_x);
	CPatch::SetPointer(sub_0060E05D[game_version] + 0x2, &hud_credits_text_4_x);

// *Menus*
	// Надпись "Пожалуйста подождите":
	if (!menu_patch) {
		CPatch::SetPointer(sub_005A3741[game_version] + 0x2, &hud_please_wait_text_x);
		CPatch::SetPointer(sub_0060CEF5[game_version] + 0x2, &hud_please_wait_text_x);
	}
}


void MapHook() {
// *Map*
	// Левая граница карты:
	CPatch::SetPointer(sub_005FF0B5[game_version] + 0x2, &map_left_x);
	// Правая граница карты:
	CPatch::SetPointer(sub_005FF0C9[game_version] + 0x2, &map_right_x);
	// Позиция маркера игрока:
	CPatch::RedirectCall(sub_005FF5C7[game_version], (void *)Map_Hook_Player_Marker_x[game_version]);
	CPatch::Nop(sub_005FF5C7[game_version] + 0x5, 0x1);
	// Позиция маркера цели;
	CPatch::SetPointer(sub_005FFA6A[game_version] + 0x2, &map_width);

	CPatch::SetPointer(sub_005FF022[game_version] + 0x2, &map_scale_x);
	CPatch::SetPointer(sub_005FF1F4[game_version] + 0x2, &map_scale_x);

	CPatch::SetPointer(sub_005FEFD4[game_version] + 0x2, &map_pos_x);
	CPatch::SetPointer(sub_005FF5B3[game_version] + 0x2, &map_pos_x);
	CPatch::SetPointer(sub_005FF978[game_version] + 0x2, &map_pos_x);

	CPatch::SetPointer(sub_005FF98E[game_version] + 0x2, &map_target_pos_x);
}


void TextHook() {
	CPatch::SetPointer(sub_005F82D7[game_version] + 0x2, &text_width);
	CPatch::SetPointer(sub_005F833A[game_version] + 0x2, &text_width);
	CPatch::SetPointer(sub_005F85E7[game_version] + 0x2, &text_width);
	CPatch::SetPointer(sub_005F864D[game_version] + 0x2, &text_width);
	CPatch::SetPointer(sub_005F9264[game_version] + 0x2, &text_width);
	CPatch::SetPointer(sub_005F92D4[game_version] + 0x2, &text_width);
}


void MenuHook() {
	// Устанавливаем правильные пропорции:
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::SetPointer(sub_005EB28B[game_version] + 0x2, &one_p_cur_base_width);				// Загрузочные экраны, меню, текст в машинопедии
	}
	else {
		CPatch::SetPointer(sub_005E3303[game_version] + 0x2, &one_p_cur_base_width);				// Загрузочные экраны
		CPatch::SetPointer(sub_005EB28B[game_version] + 0x2, &one_p_cur_base_width);				// Меню, текст в машинопедии
	}

// *Menus*
	// Надпись "Пожалуйста подождите":
	CPatch::SetPointer(sub_005A3741[game_version] + 0x2, &menu_please_wait_text_x);
	CPatch::SetPointer(sub_0060CEF5[game_version] + 0x2, &menu_please_wait_text_x);
	// Меню, загрузочные экраны, Машинопедия
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::RedirectCall(v10_sub_0056BF09[game_version], (void *)Menu_Hook_2[game_version]);	// Загрузочные экраны, текст в машинопедии
		CPatch::RedirectCall(v10_sub_0056BF95[game_version], (void *)Menu_Hook_1[game_version]);	// Меню
		CPatch::RedirectCall(v10_sub_0056C007[game_version], (void *)Menu_Hook_3[game_version]);	// "В гараже новый транспорт"
		CPatch::RedirectCall(v10_sub_0056C0AF[game_version], (void *)Menu_Hook_3[game_version]);	// Гонка: таблица рекордов
		CPatch::RedirectCall(v10_sub_005758DF[game_version], (void *)Menu_Hook_3[game_version]);	// Машинопедия, список авто
	}
	else {
		CPatch::RedirectCall(v11_sub_005D8D5B[game_version], (void *)Menu_Hook_1[game_version]);	// Меню
		CPatch::RedirectCall(v11_sub_005D90CB[game_version], (void *)Menu_Hook_2[game_version]);	// Загрузочные экраны, Машинопедия
	}
}


void MovieHook() {
// *Movies*
	// Видео:
	CPatch::RedirectJump(LS3DF_base_addr + sub_10071490[game_version], (void *)Movie_Hook[game_version]);
	if (game_version == MAFIA_1_0_ENG) { CPatch::Nop(LS3DF_base_addr + sub_10071490[game_version] + 0x5, 0x73); }
	else { CPatch::Nop(LS3DF_base_addr + sub_10071490[game_version] + 0x5, 0x8D); }
}


void VDHook() {
		CPatch::RedirectCall(v10_sub_00600E99[game_version], (void *)VD_Hook[game_version]);
		CPatch::Nop(v10_sub_00600E99[game_version] + 0x5, 0x2);
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::SetPointer(v10_sub_005A87BC[game_version] + 0x2, &vd_value2);
		CPatch::SetPointer(v10_sub_005A88FC[game_version] + 0x2, &vd_value2);
		CPatch::SetPointer(v10_sub_005A8919[game_version] + 0x2, &vd_value2);
		CPatch::SetPointer(v10_sub_005A892C[game_version] + 0x2, &vd_value2);
		CPatch::SetPointer(v10_sub_005A895F[game_version] + 0x2, &vd_value2);
	}
}


void OtherHook() {

	// Ограничитель кадров

	// Русские титры

}


void CalcGameValues() {
	WriteLog("Detected game width: %f\nDetected game height: %f\n", cur_game_width, cur_game_height);

	cur_width_p_height = cur_game_width / cur_game_height;
	cur_height_p_width = cur_game_height / cur_game_width;

	cur_base_width = ori_base_height * cur_width_p_height;

	one_p_cur_base_width = 1.0f / cur_base_width;
	one_p_cur_base_height = 1.0f / cur_base_height;

	// Считаем новые значения для исправления HUD
	if (hud_patch) { HUDCalcValues(); }
	// Считаем новые значения для исправления Map
	if (map_patch) { MapCalcValues(); }
	// Считаем новые значения для исправления разделителя текста
	if (text_patch) { TextCalcValues(); }
	// Считаем новые значения для исправления загрузочных экранов и меню
	if (menu_patch) { MenuCalcValues(); }
	// Считаем новые значения для исправления пропорций видеороликов
	if (movie_patch) { MovieCalcValues(); }
	// Считаем новые значения для изменения дальности прорисовки
	if (vd_patch) { VDCalcValues(); }

	OtherCalcValues();
}


void HUDCalcValues() {
// *HUD*
	float fix_hud_left;
	float fix_hud_center;
	float fix_hud_right;
	float fix_hud_double;
	if (hud_stretch) {
		fix_hud_left = 0.0f;
		fix_hud_center = (cur_base_width - ori_base_width) / 2.0f;
		fix_hud_right = cur_base_width - ori_base_width;
		fix_hud_double = cur_base_width - ori_base_width;
	}
	else {
		fix_hud_left = (cur_base_width - ori_base_width) / 2.0f;
		fix_hud_center = (cur_base_width - ori_base_width) / 2.0f;
		fix_hud_right = (cur_base_width - ori_base_width) / 2.0f;
		fix_hud_double = cur_base_width - ori_base_width;
	}
	// Спидометр:						Привязка к правому краю.
	hud_speedometer_x = 189.0f + fix_hud_left;
	// Тахометр:						Привязка к правому краю.
	hud_tachometer_x = 259.0f + fix_hud_left;
	// Значек ограничителя скорости:	С версии 1.1 привязка к правому краю.
	if (game_version == MAFIA_1_0_ENG) { CPatch::SetFloat(sub_00601B21[game_version] + 0x4, 760.0f + fix_hud_right); }
	else { CPatch::SetFloat(sub_00601B21[game_version] + 0x4, -40.0f - fix_hud_left); }
	// Часы:							С версии 1.1 привязка к правому краю.
	if (game_version == MAFIA_1_0_ENG) { CPatch::SetFloat(sub_00601BB5[game_version] + 0x4, 694.0f + fix_hud_right); }
	else { CPatch::SetFloat(sub_00601BB5[game_version] + 0x4, -106.0f - fix_hud_left); }

	// Компас:
	hud_compas_x = 0.0f + fix_hud_left;
	// МиниКарта:
	hud_minimap_x = 29.0f + fix_hud_left;
	// Тип коробки передач:				С версии 1.1 привязка к правому краю.
	hud_transmission_x_v10 = 758.0f + fix_hud_right;
	hud_transmission_x_v11 = 42.0f + fix_hud_left;
	// Индикатор отсутствия топлива:	С версии 1.1 привязка к правому краю.
	hud_fuel_led_x_v10 = 668.0f + fix_hud_right;
	hud_fuel_led_x_v11 = 132.0f + fix_hud_left;
	// Значек действия:
	hud_action_bar_x = 9.0f + fix_hud_left;
	// Значек здоровья персонажей (без текста):
	hud_health_bar_x = 7.0f + fix_hud_left;
	// Значек здоровья персонажей (текст):
	hud_health_bar_text_x = 42.0f + fix_hud_left;
	// Значек боеприпасов (без текста):
	hud_ammo_bar_x = 62.0f + fix_hud_left;
	// Значек боеприпасов (текст):
	hud_ammo_bar_text_x = 112.0f + fix_hud_left;
	// Значек поиска (розыск):
	hud_search_bar_x = 336.0f + fix_hud_center;
	// Значек "Здоровье" (у машины):	С версии 1.1 привязка к центру.
	hud_live_bar_x_v10 = 368.0f + fix_hud_center;
	// Полоса действия (взлом машины или при замехе кулаком, битой):
	hud_progress_bar_x = 250.0f + fix_hud_center;
	// Полоса действия 2 (самолет в миссии Сливки общества - Аэропорт):
	hud_progress_bar2_x = 350.0f + fix_hud_center;
	// Всплывающий текст слева:
	hud_left_popup_text_x = 9.0f + fix_hud_left;
	// Деньги, текст справа сверху:
	hud_money_right_text_x = 760.0f + fix_hud_right;

// *Race Mission Specific*
	// Гонка "3, 2, 1, Go":
	hud_race_321go_x = 400.0f + fix_hud_center;
	if (game_version != MAFIA_1_0_ENG) {
		CPatch::SetFloat(sub_006000F0[game_version] + 0x1, hud_race_321go_x);
		CPatch::SetFloat(sub_00600331[game_version] + 0x1, hud_race_321go_x);
	}
	// Гонка, "Едем не туда":
	CPatch::SetFloat(sub_006006AE[game_version] + 0x1, 272.0f + fix_hud_center);
	// Гонка, текст по центру ("Контрольная точка"):
	if (game_version == MAFIA_1_0_ENG) { CPatch::SetFloat(sub_00600609[game_version] + 0x4, 390.0f + fix_hud_center); }
	else { CPatch::SetFloat(sub_00600609[game_version] + 0x1, 390.0f + fix_hud_center); }
	// Гонка, текст справа сверху:
	hud_race_right_text_x = 780.0f + fix_hud_right;

// *Map*
	// Надпись "Нет карты":
	hud_map_text_x = 350.0f + fix_hud_center;

// *Black Borders, Text*
	// Черные полосы:
	hud_borders_width = cur_base_width + 1.0f;
	hud_borders_1 = (ori_base_width * 0.540540516376495f) / cur_base_width;
	hud_borders_2 = (ori_base_width * 0.5f) / cur_base_width;
	CPatch::SetFloat(sub_0046DA4F[game_version] + 0x1, hud_borders_1);
	CPatch::SetFloat(sub_00548C1D[game_version] + 0x1, hud_borders_1);
	CPatch::SetFloat(sub_005F661C[game_version] + 0x1, hud_borders_2);
	// Текст в центре на черном фоне:
	hud_black_centred_text_x = 400.0f + fix_hud_center;
	// Титры:
	hud_credits_text_1_x = 370.0f + fix_hud_center;
	hud_credits_text_2_x = 390.0f + fix_hud_center;
	hud_credits_text_3_x = 400.0f + fix_hud_center;
	hud_credits_text_4_x = 410.0f + fix_hud_center;
	CPatch::SetFloat(sub_0060DFA8[game_version] + 0x1, hud_credits_text_2_x);
	// Rus 1C титры:
	if (game_version == MAFIA_1_2_ENG) { CPatch::SetFloat(v12_sub_0060E4C7[game_version] + 0x1, hud_credits_text_2_x); };
// *Menus*
	// Надпись "Пожалуйста подождите":
	hud_please_wait_text_x = 780.0f + fix_hud_double;
}


void MapCalcValues() {
// *Map*
	float fix_map_left;
//	float fix_map_center;
//	float fix_map_right;
//	float fix_map_double;
	if (map_stretch) {
		fix_map_left = 0.0f;
//		fix_map_center = (cur_base_width - ori_base_width) / 2.0f;
//		fix_map_right = cur_base_width - ori_base_width;
//		fix_map_double = cur_base_width - ori_base_width;
	}
	else {
		fix_map_left = (cur_base_width - ori_base_width) / 2.0f;
//		fix_map_center = (cur_base_width - ori_base_width) / 2.0f;
//		fix_map_right = (cur_base_width - ori_base_width) / 2.0f;
//		fix_map_double = cur_base_width - ori_base_width;
	}
	// Карта:
	map_left_x = (ori_base_width * 0.1f + fix_map_left) / cur_base_width;
	map_right_x = 1.0f - map_left_x;
	map_width = map_right_x - map_left_x;
	map_player_marker_x = cur_game_height * ori_width_p_height;

	map_aspect = (map_width / (((ori_base_width * 0.9f + fix_map_left) / cur_base_width) - map_left_x));
	map_scale_x = 0.4f * map_aspect;
	map_pos_x = 0.2f * map_aspect;
	map_target_pos_x = 2.5f / map_aspect;
	CPatch::SetFloat(sub_005FF037[game_version] + 0x4, 1.0f - map_scale_x);
}


void TextCalcValues() {
	if (text_stretch) { text_width = (cur_base_width - (ori_base_width - (ori_base_width * 0.9f))) / cur_base_width; }
	else { text_width = (ori_base_width * 0.9f) / cur_base_width; }
}


void MenuCalcValues() {
// *Menus*
	float fix_menu_right = (cur_base_width - ori_base_width) / 2.0f;
	// Надпись "Пожалуйста подождите":
	menu_please_wait_text_x = 780.0f + fix_menu_right;
	// Меню, загрузочные экраны, машинопедия
	menu_menu_x = (cur_game_width - (cur_game_height * ori_width_p_height)) / 2.0f;
}


void MovieCalcValues() {
// *Movies*
	// Видео:
	movie_aspect = cur_width_p_height / ori_width_p_height;
}


void VDCalcValues() {
	vd_value1 = vd_value - 50.0f;
	vd_value2 = vd_value - 20.0f;

	if (game_version == MAFIA_1_0_ENG) {
		CPatch::SetFloat(v10_sub_005A89C9[game_version] + 0x1, vd_value1);
	}
	else {
		CPatch::SetFloat(v11_sub_004021E1[game_version] + 0x4, vd_value1);
		CPatch::SetFloat(v11_sub_004021E9[game_version] + 0x4, vd_value2);
	}
}


void OtherCalcValues() {

	// Ограничитель кадров
	if (fps_limit) {
		fps_limit_ms = 1000 / fps_limit;
		if (fps_limit_ms > 127) { fps_limit_ms = 127; }
		CPatch::SetChar(v10_sub_005F95BC[game_version] + 0x1, fps_limit_ms);
	}

	// Русские титры
	if (ru_credits && game_version != MAFIA_1_2_ENG) {
		CPatch::SetChar(v10_sub_0056017C[game_version] + 0x2, 0x07);
		CPatch::SetInt(v10_sub_00560548[game_version], Other_Ru_Credits_Hook[game_version]);
	}
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}


void __declspec(naked)Get_Game_Resolution_1_0_ENG() {
	_asm {
		PUSHAD;
		PUSHFD;
		FILD DWORD PTR DS : [0x006F952E];
		FSTP DWORD PTR DS : cur_game_width;
		FILD DWORD PTR DS : [0x006F9532];
		FSTP DWORD PTR DS : cur_game_height;
		CALL CalcGameValues;
		POPFD;
		POPAD;

		MOV EAX, DWORD PTR DS : [0x006F952E];
		RETN;
	}
}


void __declspec(naked)Get_Game_Resolution_1_1_ENG() {
	_asm {
		ADD[ESP], 0x6;
		PUSHAD;
		PUSHFD;
		FILD DWORD PTR DS : [0x00646E16];
		FSTP DWORD PTR DS : cur_game_width;
		FILD DWORD PTR DS : [0x00646E1A];
		FSTP DWORD PTR DS : cur_game_height;
		CALL CalcGameValues;
		POPFD;
		POPAD;

		MOV EAX, DWORD PTR DS : [0x00646E1A];
		MOV ECX, DWORD PTR DS : [0x00646E16];
		RETN;
	}
}


void __declspec(naked)Get_Game_Resolution_1_2_ENG() {
	_asm {
		ADD[ESP], 0x6;
		PUSHAD;
		PUSHFD;
		FILD DWORD PTR DS : [0x00647EE6];
		FSTP DWORD PTR DS : cur_game_width;
		FILD DWORD PTR DS : [0x00647EEA];
		FSTP DWORD PTR DS : cur_game_height;
		CALL CalcGameValues;
		POPFD;
		POPAD;

		MOV EAX, DWORD PTR DS : [0x00647EEA];
		MOV ECX, DWORD PTR DS : [0x00647EE6];
		RETN;
	}
}


void __declspec(naked)FOV_Hook_1_0_ENG() {
	_asm {
		ADD[ESP], 0x1;
		PUSH EAX;
		MOV EAX, LS3DF_base_addr;
		FMUL DWORD PTR DS : [EAX + 0x0009756C];
		FLD ST;
		FCOS;
		FXCH ST(1);
		FSIN;
		FDIVP ST(1), ST;
		FMUL DWORD PTR DS : ori_width_p_height;
		FLD DWORD PTR DS : cur_width_p_height;
		FXCH ST(1);
		FPATAN;
		POP EAX;
		RETN;
	}
}


void __declspec(naked)FOV_Hook_1_1_ENG() {
	_asm {
		ADD[ESP], 0x1;
		PUSH EAX;
		MOV EAX, LS3DF_base_addr;
		FMUL DWORD PTR DS : [EAX + 0x000985B0];
		FLD ST;
		FCOS;
		FXCH ST(1);
		FSIN;
		FDIVP ST(1), ST;
		FMUL DWORD PTR DS : ori_width_p_height;
		FLD DWORD PTR DS : cur_width_p_height;
		FXCH ST(1);
		FPATAN;
		POP EAX;
		RETN;
	}
}


void __declspec(naked)FOV_Hook_1_2_ENG() {
	_asm {
		ADD[ESP], 0x1;
		PUSH EAX;
		MOV EAX, LS3DF_base_addr;
		FMUL DWORD PTR DS : [EAX + 0x0009B5B0];
		FLD ST;
		FCOS;
		FXCH ST(1);
		FSIN;
		FDIVP ST(1), ST;
		FMUL DWORD PTR DS : ori_width_p_height;
		FLD DWORD PTR DS : cur_width_p_height;
		FXCH ST(1);
		FPATAN;
		POP EAX;
		RETN;
	}
}


void __declspec(naked)Map_Hook_1_0_ENG_Player_Marker_x() {
	_asm {
		FMUL DWORD PTR SS : map_player_marker_x;
		RETN;
	}
}


void __declspec(naked)Map_Hook_1_1_ENG_Player_Marker_x() {
	_asm {
		FMUL DWORD PTR SS : map_player_marker_x;
		FXCH ST(1);
		RETN;
	}
}


void __declspec(naked)Menu_Hook_1_0_ENG_1() {
	static int sub_0057D850 = 0x0057D850;

	_asm {
		MOV EAX, DWORD PTR SS : [ESP + 0x8];
		SUB ESP, 0x8;
		PUSH ESI;
		MOV ESI, ECX;
		MOV DWORD PTR DS : [ESI + 0x34], EAX;
		MOV EAX, DWORD PTR SS : [ESP + 0x10];
		MOV DWORD PTR DS : [ESI], 0x0063DBDC;
		MOV ECX, DWORD PTR DS : [EAX];
		MOV DWORD PTR DS : [ESI + 0x4], ECX;
		XOR ECX, ECX;
		MOV DWORD PTR DS : [ESI + 0x8], ECX;
		MOV EDX, DWORD PTR DS : [EAX + 0x18];
		MOV DWORD PTR DS : [ESI + 0x0C], EDX;
		MOV DL, BYTE PTR DS : [0x672628];
		CMP DL, CL;
		JE SHORT label_005704FB;
		MOV EDX, DWORD PTR DS : [EAX + 0x8];
		MOV DWORD PTR DS : [ESI + 0x10], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x0C];
		MOV DWORD PTR DS : [ESI + 0x14], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x10];
		MOV DWORD PTR DS : [ESI + 0x18], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x14];
		MOV DWORD PTR DS : [ESI + 0x1C], EDX;
		JMP SHORT label_0057052B;
	label_005704FB:
		FLD DWORD PTR DS : [0x67A4C0];
		FMUL DWORD PTR DS : [EAX + 0x8];

		CALL label_filter;

		FSTP DWORD PTR DS : [ESI + 0x10];
		FLD DWORD PTR DS : [0x67A4C4];
		FMUL DWORD PTR DS : [EAX + 0x0C];
		FSTP DWORD PTR DS : [ESI + 0x14];
		FLD DWORD PTR DS : [0x67A4C0];
		FMUL DWORD PTR DS : [EAX + 0x10];
		FSTP DWORD PTR DS : [ESI + 0x18];
		FLD DWORD PTR DS : [0x67A4C4];
		FMUL DWORD PTR DS : [EAX + 0x14];
		FSTP DWORD PTR DS : [ESI + 0x1C];
	label_0057052B:
		MOV EDX, DWORD PTR DS : [EAX + 0x1C];
		MOV DWORD PTR SS : [ESP + 0x8], ECX;
		MOV DWORD PTR DS : [ESI + 0x20], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x20];
		MOV DWORD PTR DS : [ESI + 0x24], EDX;
		MOV EAX, DWORD PTR DS : [EAX + 0x20];
		MOV DWORD PTR SS : [ESP + 0x4], EAX;
		MOV EAX, DWORD PTR DS : [ESI + 0x0C];
		FILD QWORD PTR SS : [ESP + 0x4];
		CMP EAX, ECX;
		MOV DWORD PTR DS : [ESI + 0x2C], ECX;
		FSTP DWORD PTR DS : [ESI + 0x28];
		JE SHORT label_0057056A;
		PUSH EAX;
		MOV ECX, 0x0067A57C;
		CALL sub_0057D850;
		MOV DWORD PTR DS : [ESI + 0x30], EAX;
		MOV EAX, ESI;
		POP ESI;
		ADD ESP, 0x8;
		RET 0x8;
	label_0057056A:
		MOV DWORD PTR DS : [ESI + 0x30], ECX;
		MOV EAX, ESI;
		POP ESI;
		ADD ESP, 0x8;
		RET 0x8;

	label_filter:
		CMP EAX, 0x006728DC;		// Player Profile
		JE label_add;
		CMP EAX, 0x00672828;		// Player Profile -> Create New Profile
		JE label_add;
		CMP EAX, 0x00672C60;		// Main Menu
		JE label_add;
		CMP EAX, 0x00674A9C;		// Main Menu -> Load Game
		JE label_add;
		CMP EAX, 0x00674E68;		// Main Menu -> Free Ride
		JE label_add;
		CMP EAX, 0x00678F84;		// Main Menu -> Free Ride -> Location
		JE label_add;
		CMP EAX, 0x00676770;		// Main Menu -> Free Ride -> Garage
		JE label_add;
		CMP EAX, 0x00676AAC;		// Main Menu -> Free Ride -> Garage -> Models
		JE label_add;
		CMP EAX, 0x00676BF0;		// Main Menu -> Free Ride -> Garage -> Models -> Car Colors
		JE label_add;
		CMP EAX, 0x0067341C;		// Main Menu -> Options
		JE label_add;
		CMP EAX, 0x006734F4;		// Main Menu -> Options -> Player Controls
		JE label_add;
		CMP EAX, 0x0067353C;		// Main Menu -> Options -> Car Controls
		JE label_add;
		CMP EAX, 0x00673584;		// Main Menu -> Options -> Other Controls
		JE label_add;
		CMP EAX, 0x00672F0C;		// Main Menu -> Options -> Audio & Video
		JE label_add;
		CMP EAX, 0x006792E4;		// Main Menu -> Quit Game, Game Menu -> Exit Game
		JE label_add;
		CMP EAX, 0x00672E34;		// Game Menu
		JE label_add;
		CMP EAX, 0x006750A8;		// Game -> Inventory
		JE label_add;
		CMP EAX, 0x006750F0;		// Game -> Inventory
		JE label_add;
		CMP EAX, 0x006774A8;		// Game -> Garage
		JE label_add;
		CMP EAX, 0x00677580;		// Game -> Garage
		JE label_add;
		CMP EAX, 0x00675474;		// Game -> Pickup/Use
		JE label_add;
		CMP EAX, 0x00678C00;		// Game -> Game Over
		JE label_add;
		CMP EAX, 0x00678A98;		// Game Menu (in Race Mission)
		JE label_add;
		RETN;
	label_add:
		FADD DWORD PTR DS : menu_menu_x;
		RETN;
	}
}


void __declspec(naked)Menu_Hook_1_1_ENG_1() {
	static int sub_0060F370 = 0x0060F370;
	static int sub_005E9650 = 0x005E9650;

	_asm {
		PUSH - 0x1;
		PUSH 0x0062172B;
		MOV EAX, DWORD PTR FS : [0x0];
		PUSH EAX;
		MOV DWORD PTR FS : [0x0], ESP;
		PUSH ECX;
		MOV AL, BYTE PTR SS : [ESP + 0x14];
		PUSH ESI;
		PUSH EDI;
		MOV ESI, ECX;
		XOR EDI, EDI;
		MOV DWORD PTR SS : [ESP + 0x8], ESI;
		MOV BYTE PTR DS : [ESI + 0x30], AL;
		MOV DWORD PTR DS : [ESI + 0x34], EDI;
		MOV DWORD PTR DS : [ESI + 0x38], EDI;
		MOV DWORD PTR DS : [ESI + 0x3C], EDI;
		MOV EAX, DWORD PTR SS : [ESP + 0x1C];
		MOV DWORD PTR DS : [ESI], 0x00624FF8;
		MOV DWORD PTR SS : [ESP + 0x14], EDI;
		MOV ECX, DWORD PTR DS : [EAX];
		MOV DWORD PTR DS : [ESI + 0x4], ECX;
		MOV EDX, DWORD PTR DS : [EAX + 0x1C];
		MOV DWORD PTR DS : [ESI + 0x8], EDX;
		FLD DWORD PTR DS : [0x6BC7B8];
		FMUL DWORD PTR DS : [EAX + 0x8];

		CALL label_filter;

		FST DWORD PTR DS : [ESI + 0x24];
		FSTP DWORD PTR DS : [ESI + 0x14];
		FLD DWORD PTR DS : [0x6BC7BC];
		FMUL DWORD PTR DS : [EAX + 0x0C];
		FST DWORD PTR DS : [ESI + 0x28];
		FSTP DWORD PTR DS : [ESI + 0x18];
		FLD DWORD PTR DS : [0x6BC7B8];
		FMUL DWORD PTR DS : [EAX + 0x10];
		FSTP DWORD PTR DS : [ESI + 0x1C];
		FLD DWORD PTR DS : [0x6BC7BC];
		FMUL DWORD PTR DS : [EAX + 0x14];
		FSTP DWORD PTR DS : [ESI + 0x20];
		MOV EAX, DWORD PTR DS : [EAX + 0x18];
		CMP EAX, EDI;
		MOV DWORD PTR DS : [ESI + 0x0C], EAX;
		JE SHORT label_005E9479;
		PUSH EAX;
		MOV ECX, 0x006D7644;
		CALL sub_0060F370;
		MOV DWORD PTR DS : [ESI + 0x10], EAX;
		JMP SHORT label_005E947C;
	label_005E9479:
		MOV DWORD PTR DS : [ESI + 0x10], EDI;
	label_005E947C:
		MOV ECX, DWORD PTR SS : [ESP + 0x20];
		CMP ECX, EDI;
		MOV DWORD PTR DS : [ESI + 0x2C], ECX;
		JE SHORT label_005E948D;
		PUSH ESI;
		CALL sub_005E9650;
	label_005E948D:
		MOV ECX, DWORD PTR SS : [ESP + 0x0C];
		MOV EAX, ESI;
		POP EDI;
		POP ESI;
		MOV DWORD PTR FS : [0x0], ECX;
		ADD ESP, 0x10;
		RET 0x8;

	label_filter:
		CMP ECX, 0x1;				// Load Screens, Carcyclopedia
		JE label_add;
		CMP ECX, 0x2;				// Carcyclopedia - Show Mode
		JE label_test;
		CMP ECX, 0x3;				// Menu
		JE label_add;
		CMP ECX, 0x82;				// Menu
		JE label_add;
		RETN;
	label_test:
		CMP EDX, 0x00FF00B2;		// Options
		JE label_add;
		CMP EDX, 0x00FF0032;		// Options -> Controls
		JE label_add;
		CMP EDX, 0x001F0032;		// Options -> Audio & Video
		JE label_add;
		RETN;
	label_add:
		FADD DWORD PTR DS : menu_menu_x;
		RETN;
	}
}


void __declspec(naked)Menu_Hook_1_2_ENG_1() {
	static int sub_0060FB40 = 0x0060FB40;
	static int sub_005E9E50 = 0x005E9E50;

	_asm {
		PUSH - 0x1;
		PUSH 0x00621EFB;
		MOV EAX, DWORD PTR FS : [0x0];
		PUSH EAX;
		MOV DWORD PTR FS : [0x0], ESP;
		PUSH ECX;
		MOV AL, BYTE PTR SS : [ESP + 0x14];
		PUSH ESI;
		PUSH EDI;
		MOV ESI, ECX;
		XOR EDI, EDI;
		MOV DWORD PTR SS : [ESP + 0x8], ESI;
		MOV BYTE PTR DS : [ESI + 0x30], AL;
		MOV DWORD PTR DS : [ESI + 0x34], EDI;
		MOV DWORD PTR DS : [ESI + 0x38], EDI;
		MOV DWORD PTR DS : [ESI + 0x3C], EDI;
		MOV EAX, DWORD PTR SS : [ESP + 0x1C];
		MOV DWORD PTR DS : [ESI], 0x00625FF8;
		MOV DWORD PTR SS : [ESP + 0x14], EDI;
		MOV ECX, DWORD PTR DS : [EAX];
		MOV DWORD PTR DS : [ESI + 0x4], ECX;
		MOV EDX, DWORD PTR DS : [EAX + 0x1C];
		MOV DWORD PTR DS : [ESI + 0x8], EDX;
		FLD DWORD PTR DS : [0x6BD888];
		FMUL DWORD PTR DS : [EAX + 0x8];

		CALL label_filter;

		FST DWORD PTR DS : [ESI + 0x24];
		FSTP DWORD PTR DS : [ESI + 0x14];
		FLD DWORD PTR DS : [0x6BD88C];
		FMUL DWORD PTR DS : [EAX + 0x0C];
		FST DWORD PTR DS : [ESI + 0x28];
		FSTP DWORD PTR DS : [ESI + 0x18];
		FLD DWORD PTR DS : [0x6BD888];
		FMUL DWORD PTR DS : [EAX + 0x10];
		FSTP DWORD PTR DS : [ESI + 0x1C];
		FLD DWORD PTR DS : [0x6BD88C];
		FMUL DWORD PTR DS : [EAX + 0x14];
		FSTP DWORD PTR DS : [ESI + 0x20];
		MOV EAX, DWORD PTR DS : [EAX + 0x18];
		CMP EAX, EDI;
		MOV DWORD PTR DS : [ESI + 0x0C], EAX;
		JE SHORT label_005E9C79;
		PUSH EAX;
		MOV ECX, 0x006D8714;
		CALL sub_0060FB40;
		MOV DWORD PTR DS : [ESI + 0x10], EAX;
		JMP SHORT label_005E9C7C;
	label_005E9C79:
		MOV DWORD PTR DS : [ESI + 0x10], EDI;
	label_005E9C7C:
		MOV ECX, DWORD PTR SS : [ESP + 0x20];
		CMP ECX, EDI;
		MOV DWORD PTR DS : [ESI + 0x2C], ECX;
		JE SHORT label_005E9C8D;
		PUSH ESI;
		CALL sub_005E9E50;
	label_005E9C8D:
		MOV ECX, DWORD PTR SS : [ESP + 0x0C];
		MOV EAX, ESI;
		POP EDI;
		POP ESI;
		MOV DWORD PTR FS : [0x0], ECX;
		ADD ESP, 0x10;
		RET 0x8;

	label_filter:
		CMP ECX, 0x1;				// Load Screens, Carcyclopedia
		JE label_add;
		CMP ECX, 0x2;				// Carcyclopedia - Show Mode
		JE label_test;
		CMP ECX, 0x3;				// Menu
		JE label_add;
		CMP ECX, 0x82;				// Menu
		JE label_add;
		RETN;
	label_test:
		CMP EDX, 0x00FF00B2;		// Options
		JE label_add;
		CMP EDX, 0x00FF0032;		// Options -> Controls
		JE label_add;
		CMP EDX, 0x001F0032;		// Options -> Audio & Video
		JE label_add;
		RETN;
	label_add:
		FADD DWORD PTR DS : menu_menu_x;
		RETN;
	}
}


void __declspec(naked)Menu_Hook_1_0_ENG_2() {
	static int sub_0057D850 = 0x0057D850;

	_asm {
		MOV EAX, DWORD PTR SS : [ESP + 0x8];
		SUB ESP, 0x8;
		PUSH ESI;
		MOV ESI, ECX;
		MOV DWORD PTR DS : [ESI + 0x34], EAX;
		MOV EAX, DWORD PTR SS : [ESP + 0x10];
		MOV DWORD PTR DS : [ESI], 0x0063DBDC;
		MOV ECX, DWORD PTR DS : [EAX];
		MOV DWORD PTR DS : [ESI + 0x4], ECX;
		XOR ECX, ECX;
		MOV DWORD PTR DS : [ESI + 0x8], ECX;
		MOV EDX, DWORD PTR DS : [EAX + 0x18];
		MOV DWORD PTR DS : [ESI + 0x0C], EDX;
		MOV DL, BYTE PTR DS : [0x672628];
		CMP DL, CL;
		JE SHORT label_005704FB;
		MOV EDX, DWORD PTR DS : [EAX + 0x8];
		MOV DWORD PTR DS : [ESI + 0x10], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x0C];
		MOV DWORD PTR DS : [ESI + 0x14], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x10];
		MOV DWORD PTR DS : [ESI + 0x18], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x14];
		MOV DWORD PTR DS : [ESI + 0x1C], EDX;
		JMP SHORT label_0057052B;
	label_005704FB:
		FLD DWORD PTR DS : [0x67A4C0];
		FMUL DWORD PTR DS : [EAX + 0x8];

		CALL label_filter;

		FSTP DWORD PTR DS : [ESI + 0x10];
		FLD DWORD PTR DS : [0x67A4C4];
		FMUL DWORD PTR DS : [EAX + 0x0C];
		FSTP DWORD PTR DS : [ESI + 0x14];
		FLD DWORD PTR DS : [0x67A4C0];
		FMUL DWORD PTR DS : [EAX + 0x10];
		FSTP DWORD PTR DS : [ESI + 0x18];
		FLD DWORD PTR DS : [0x67A4C4];
		FMUL DWORD PTR DS : [EAX + 0x14];
		FSTP DWORD PTR DS : [ESI + 0x1C];
	label_0057052B:
		MOV EDX, DWORD PTR DS : [EAX + 0x1C];
		MOV DWORD PTR SS : [ESP + 0x8], ECX;
		MOV DWORD PTR DS : [ESI + 0x20], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x20];
		MOV DWORD PTR DS : [ESI + 0x24], EDX;
		MOV EAX, DWORD PTR DS : [EAX + 0x20];
		MOV DWORD PTR SS : [ESP + 0x4], EAX;
		MOV EAX, DWORD PTR DS : [ESI + 0x0C];
		FILD QWORD PTR SS : [ESP + 0x4];
		CMP EAX, ECX;
		MOV DWORD PTR DS : [ESI + 0x2C], ECX;
		FSTP DWORD PTR DS : [ESI + 0x28];
		JE SHORT label_0057056A;
		PUSH EAX;
		MOV ECX, 0x0067A57C;
		CALL sub_0057D850;
		MOV DWORD PTR DS : [ESI + 0x30], EAX;
		MOV EAX, ESI;
		POP ESI;
		ADD ESP, 0x8;
		RET 0x8;
	label_0057056A:
		MOV DWORD PTR DS : [ESI + 0x30], ECX;
		MOV EAX, ESI;
		POP ESI;
		ADD ESP, 0x8;
		RET 0x8;

	label_filter:
		CMP EAX, 0x00678DB0;		// Load Screens
		JE label_add;
		CMP EAX, 0x00674C94;		// Carcyclopedia
		JE label_add;
		RETN;
	label_add :
		FADD DWORD PTR DS : menu_menu_x;
		RETN;
	}
}


void __declspec(naked)Menu_Hook_1_1_ENG_2() {
	static int sub_0060F370 = 0x0060F370;
	static int sub_005E9650 = 0x005E9650;

	_asm {
		PUSH - 0x1;
		PUSH 0x0062172B;
		MOV EAX, DWORD PTR FS : [0x0];
		PUSH EAX;
		MOV DWORD PTR FS : [0x0], ESP;
		PUSH ECX;
		MOV AL, BYTE PTR SS : [ESP + 0x14];
		PUSH ESI;
		PUSH EDI;
		MOV ESI, ECX;
		XOR EDI, EDI;
		MOV DWORD PTR SS : [ESP + 0x8], ESI;
		MOV BYTE PTR DS : [ESI + 0x30], AL;
		MOV DWORD PTR DS : [ESI + 0x34], EDI;
		MOV DWORD PTR DS : [ESI + 0x38], EDI;
		MOV DWORD PTR DS : [ESI + 0x3C], EDI;
		MOV EAX, DWORD PTR SS : [ESP + 0x1C];
		MOV DWORD PTR DS : [ESI], 0x00624FF8;
		MOV DWORD PTR SS : [ESP + 0x14], EDI;
		MOV ECX, DWORD PTR DS : [EAX];
		MOV DWORD PTR DS : [ESI + 0x4], ECX;
		MOV EDX, DWORD PTR DS : [EAX + 0x1C];
		MOV DWORD PTR DS : [ESI + 0x8], EDX;
		FLD DWORD PTR DS : [0x6BC7B8];
		FMUL DWORD PTR DS : [EAX + 0x8];

		CALL label_filter;

		FST DWORD PTR DS : [ESI + 0x24];
		FSTP DWORD PTR DS : [ESI + 0x14];
		FLD DWORD PTR DS : [0x6BC7BC];
		FMUL DWORD PTR DS : [EAX + 0x0C];
		FST DWORD PTR DS : [ESI + 0x28];
		FSTP DWORD PTR DS : [ESI + 0x18];
		FLD DWORD PTR DS : [0x6BC7B8];
		FMUL DWORD PTR DS : [EAX + 0x10];
		FSTP DWORD PTR DS : [ESI + 0x1C];
		FLD DWORD PTR DS : [0x6BC7BC];
		FMUL DWORD PTR DS : [EAX + 0x14];
		FSTP DWORD PTR DS : [ESI + 0x20];
		MOV EAX, DWORD PTR DS : [EAX + 0x18];
		CMP EAX, EDI;
		MOV DWORD PTR DS : [ESI + 0x0C], EAX;
		JE SHORT label_005E9479;
		PUSH EAX;
		MOV ECX, 0x006D7644;
		CALL sub_0060F370;
		MOV DWORD PTR DS : [ESI + 0x10], EAX;
		JMP SHORT label_005E947C;
	label_005E9479:
		MOV DWORD PTR DS : [ESI + 0x10], EDI;
	label_005E947C:
		MOV ECX, DWORD PTR SS : [ESP + 0x20];
		CMP ECX, EDI;
		MOV DWORD PTR DS : [ESI + 0x2C], ECX;
		JE SHORT label_005E948D;
		PUSH ESI;
		CALL sub_005E9650;
	label_005E948D:
		MOV ECX, DWORD PTR SS : [ESP + 0x0C];
		MOV EAX, ESI;
		POP EDI;
		POP ESI;
		MOV DWORD PTR FS : [0x0], ECX;
		ADD ESP, 0x10;
		RET 0x8;

	label_filter:
		CMP ECX, 0x1;				// Load Screens, Carcyclopedia
		JE label_add;
		CMP ECX, 0x2;				// Carcyclopedia - Show Mode
		JE label_add;
		CMP ECX, 0x3;				// Load Game
		JE label_add;
		RETN;
	label_add:
		FADD DWORD PTR DS : menu_menu_x;
		RETN;
	}
}


void __declspec(naked)Menu_Hook_1_2_ENG_2() {
	static int sub_0060FB40 = 0x0060FB40;
	static int sub_005E9E50 = 0x005E9E50;

	_asm {
		PUSH - 0x1;
		PUSH 0x00621EFB;
		MOV EAX, DWORD PTR FS : [0x0];
		PUSH EAX;
		MOV DWORD PTR FS : [0x0], ESP;
		PUSH ECX;
		MOV AL, BYTE PTR SS : [ESP + 0x14];
		PUSH ESI;
		PUSH EDI;
		MOV ESI, ECX;
		XOR EDI, EDI;
		MOV DWORD PTR SS : [ESP + 0x8], ESI;
		MOV BYTE PTR DS : [ESI + 0x30], AL;
		MOV DWORD PTR DS : [ESI + 0x34], EDI;
		MOV DWORD PTR DS : [ESI + 0x38], EDI;
		MOV DWORD PTR DS : [ESI + 0x3C], EDI;
		MOV EAX, DWORD PTR SS : [ESP + 0x1C];
		MOV DWORD PTR DS : [ESI], 0x00625FF8;
		MOV DWORD PTR SS : [ESP + 0x14], EDI;
		MOV ECX, DWORD PTR DS : [EAX];
		MOV DWORD PTR DS : [ESI + 0x4], ECX;
		MOV EDX, DWORD PTR DS : [EAX + 0x1C];
		MOV DWORD PTR DS : [ESI + 0x8], EDX;
		FLD DWORD PTR DS : [0x6BD888];
		FMUL DWORD PTR DS : [EAX + 0x8];

		CALL label_filter;

		FST DWORD PTR DS : [ESI + 0x24];
		FSTP DWORD PTR DS : [ESI + 0x14];
		FLD DWORD PTR DS : [0x6BD88C];
		FMUL DWORD PTR DS : [EAX + 0x0C];
		FST DWORD PTR DS : [ESI + 0x28];
		FSTP DWORD PTR DS : [ESI + 0x18];
		FLD DWORD PTR DS : [0x6BD888];
		FMUL DWORD PTR DS : [EAX + 0x10];
		FSTP DWORD PTR DS : [ESI + 0x1C];
		FLD DWORD PTR DS : [0x6BD88C];
		FMUL DWORD PTR DS : [EAX + 0x14];
		FSTP DWORD PTR DS : [ESI + 0x20];
		MOV EAX, DWORD PTR DS : [EAX + 0x18];
		CMP EAX, EDI;
		MOV DWORD PTR DS : [ESI + 0x0C], EAX;
		JE SHORT label_005E9C79;
		PUSH EAX;
		MOV ECX, 0x006D8714;
		CALL sub_0060FB40;
		MOV DWORD PTR DS : [ESI + 0x10], EAX;
		JMP SHORT label_005E9C7C;
	label_005E9C79:
		MOV DWORD PTR DS : [ESI + 0x10], EDI;
	label_005E9C7C:
		MOV ECX, DWORD PTR SS : [ESP + 0x20];
		CMP ECX, EDI;
		MOV DWORD PTR DS : [ESI + 0x2C], ECX;
		JE SHORT label_005E9C8D;
		PUSH ESI;
		CALL sub_005E9E50;
	label_005E9C8D:
		MOV ECX, DWORD PTR SS : [ESP + 0x0C];
		MOV EAX, ESI;
		POP EDI;
		POP ESI;
		MOV DWORD PTR FS : [0x0], ECX;
		ADD ESP, 0x10;
		RET 0x8;

	label_filter:
		CMP ECX, 0x1;				// Load Screens, Carcyclopedia
		JE label_add;
		CMP ECX, 0x2;				// Carcyclopedia - Show Mode
		JE label_add;
		CMP ECX, 0x3;				// Load Game
		JE label_add;
		RETN;
	label_add:
		FADD DWORD PTR DS : menu_menu_x;
		RETN;
	}
}


void __declspec(naked)Menu_Hook_1_0_ENG_3() {
	static int sub_0057D850 = 0x0057D850;

	_asm {
		MOV EAX, DWORD PTR SS : [ESP + 0x8];
		SUB ESP, 0x8;
		PUSH ESI;
		MOV ESI, ECX;
		MOV DWORD PTR DS : [ESI + 0x34], EAX;
		MOV EAX, DWORD PTR SS : [ESP + 0x10];
		MOV DWORD PTR DS : [ESI], 0x0063DBDC;
		MOV ECX, DWORD PTR DS : [EAX];
		MOV DWORD PTR DS : [ESI + 0x4], ECX;
		XOR ECX, ECX;
		MOV DWORD PTR DS : [ESI + 0x8], ECX;
		MOV EDX, DWORD PTR DS : [EAX + 0x18];
		MOV DWORD PTR DS : [ESI + 0x0C], EDX;
		MOV DL, BYTE PTR DS : [0x672628];
		CMP DL, CL;
		JE SHORT label_005704FB;
		MOV EDX, DWORD PTR DS : [EAX + 0x8];
		MOV DWORD PTR DS : [ESI + 0x10], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x0C];
		MOV DWORD PTR DS : [ESI + 0x14], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x10];
		MOV DWORD PTR DS : [ESI + 0x18], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x14];
		MOV DWORD PTR DS : [ESI + 0x1C], EDX;
		JMP SHORT label_0057052B;
	label_005704FB:
		FLD DWORD PTR DS : [0x67A4C0];
		FMUL DWORD PTR DS : [EAX + 0x8];

		CALL label_filter;

		FSTP DWORD PTR DS : [ESI + 0x10];
		FLD DWORD PTR DS : [0x67A4C4];
		FMUL DWORD PTR DS : [EAX + 0x0C];
		FSTP DWORD PTR DS : [ESI + 0x14];
		FLD DWORD PTR DS : [0x67A4C0];
		FMUL DWORD PTR DS : [EAX + 0x10];
		FSTP DWORD PTR DS : [ESI + 0x18];
		FLD DWORD PTR DS : [0x67A4C4];
		FMUL DWORD PTR DS : [EAX + 0x14];
		FSTP DWORD PTR DS : [ESI + 0x1C];
	label_0057052B:
		MOV EDX, DWORD PTR DS : [EAX + 0x1C];
		MOV DWORD PTR SS : [ESP + 0x8], ECX;
		MOV DWORD PTR DS : [ESI + 0x20], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x20];
		MOV DWORD PTR DS : [ESI + 0x24], EDX;
		MOV EAX, DWORD PTR DS : [EAX + 0x20];
		MOV DWORD PTR SS : [ESP + 0x4], EAX;
		MOV EAX, DWORD PTR DS : [ESI + 0x0C];
		FILD QWORD PTR SS : [ESP + 0x4];
		CMP EAX, ECX;
		MOV DWORD PTR DS : [ESI + 0x2C], ECX;
		FSTP DWORD PTR DS : [ESI + 0x28];
		JE SHORT label_0057056A;
		PUSH EAX;
		MOV ECX, 0x0067A57C;
		CALL sub_0057D850;
		MOV DWORD PTR DS : [ESI + 0x30], EAX;
		MOV EAX, ESI;
		POP ESI;
		ADD ESP, 0x8;
		RET 0x8;
	label_0057056A:
		MOV DWORD PTR DS : [ESI + 0x30], ECX;
		MOV EAX, ESI;
		POP ESI;
		ADD ESP, 0x8;
		RET 0x8;

	label_filter:
		CMP EAX, 0x00677658;		// "New cars have been added to the garage"
		JE label_add;
		CMP EAX, 0x00676F98;		// Racing (in Race Mission)
		JE label_add;
		CMP EAX, 0x00674DB4;		// Carcyclopedia (Cars List)
		JE label_add;
		RETN;
	label_add:
		FADD DWORD PTR DS : menu_menu_x;
		RETN;
	}
}


void __declspec(naked)Movie_Hook_1_0_ENG() {
	static int init = 0x1;
	static float half = 0.5f;
	static int sub_10070240 = 0x00070240;
	static int sub_101C13A8 = 0x001C13A8;
	static int sub_101C13F4 = 0x001C13F4;
	static int sub_101C1588 = 0x001C1588;

	_asm {
		CMP init, 0;
		JZ label_begin;
		MOV init, 0;
		PUSH EAX;
		MOV EAX, sub_10070240;
		ADD EAX, LS3DF_base_addr;
		MOV sub_10070240, EAX;
		MOV EAX, sub_101C13A8;
		ADD EAX, LS3DF_base_addr;
		MOV sub_101C13A8, EAX;
		MOV EAX, sub_101C13F4;
		ADD EAX, LS3DF_base_addr;
		MOV sub_101C13F4, EAX;
		MOV EAX, sub_101C1588;
		ADD EAX, LS3DF_base_addr;
		MOV sub_101C1588, EAX;
		POP EAX;

	label_begin:
		MOV EAX, DWORD PTR DS : [sub_101C1588];		// MOV AL,BYTE PTR DS:[101C1588]
		MOV AL, BYTE PTR DS : [EAX];				// MOV AL,BYTE PTR DS:[101C1588]
		MOV ECX, DWORD PTR DS : [sub_101C13A8];		// MOV ECX,DWORD PTR DS:[101C13A8]
		MOV ECX, DWORD PTR DS : [ECX];				// MOV ECX,DWORD PTR DS:[101C13A8]
		SUB ESP, 0x10;
		TEST AL, AL;
		JS SHORT label_10070448;
		MOV ECX, DWORD PTR DS : [sub_101C13F4];		// MOV ECX,DWORD PTR DS:[101C13F4]
		MOV ECX, DWORD PTR DS : [ECX];				// MOV ECX,DWORD PTR DS:[101C13F4]
	label_10070448:
		MOV EAX, DWORD PTR SS : [ESP + 0x18];
		TEST EAX, EAX;
		JNE SHORT label_1007045E;
		LEA EAX, [ESP];
		PUSH EAX;
		PUSH ECX;
		CALL DWORD PTR DS : [GetClientRect];
		JMP SHORT label_10070479;
	label_1007045E:
		MOV ECX, DWORD PTR DS : [EAX];
		MOV EDX, DWORD PTR DS : [EAX + 0x4];
		MOV DWORD PTR SS : [ESP], ECX;
		MOV ECX, DWORD PTR DS : [EAX + 0x8];
		MOV DWORD PTR SS : [ESP + 4], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x0C];
		MOV DWORD PTR SS : [ESP + 0x8], ECX;
		MOV DWORD PTR SS : [ESP + 0x0C], EDX;
	label_10070479:
		MOV ECX, DWORD PTR SS : [ESP + 0x14];
		LEA EAX, [ECX + 0x28];

		CALL label_fix;

		CALL sub_10070240;
		XOR EAX, EAX;
		ADD ESP, 0x10;
		RET 0x8;

	label_fix:
		CMP movie_stretch, 1;
		JE label_fix2;
		// Левая сторона видео
//		MOV EDX, DWORD PTR SS : [ESP + 0x4];
		FLD DWORD PTR DS : cur_game_width;
		FILD DWORD PTR DS : [ESP + 0xC];
		FDIV DWORD PTR DS : movie_aspect;
		FSUBP ST(1), ST;
		FMUL DWORD PTR DS : half;
		FISTP DWORD PTR DS : [EAX];
//		MOV DWORD PTR DS : [EAX], EDX;
		// Верхняя сторона видео
		MOV EDX, DWORD PTR SS : [ESP + 0x8];
		MOV DWORD PTR DS : [EAX + 0x4], EDX;
		// Ширина видео
//		MOV EDX, DWORD PTR SS : [ESP + 0xC];
		FILD DWORD PTR DS : [ESP + 0xC];
		FDIV DWORD PTR DS : movie_aspect;
		FISTP DWORD PTR DS : [EAX + 0x8];
//		MOV DWORD PTR DS : [EAX + 0x8], EDX;
		// Высота видео
		MOV EDX, DWORD PTR SS : [ESP + 0x10];
		MOV DWORD PTR DS : [EAX + 0x0C], EDX;
		RETN;
	label_fix2:
		// Левая сторона видео
		MOV EDX, DWORD PTR SS : [ESP + 0x4];
		MOV DWORD PTR DS : [EAX], EDX;
		// Верхняя сторона видео
//		MOV EDX, DWORD PTR SS : [ESP + 0x8];
		FLD DWORD PTR DS : cur_game_height;
		FILD DWORD PTR DS : [ESP + 0x10];
		FMUL DWORD PTR DS : movie_aspect;
		FSUBP ST(1), ST;
		FMUL DWORD PTR DS : half;
		FISTP DWORD PTR DS : [EAX + 0x4];
//		MOV DWORD PTR DS : [EAX + 0x4], EDX;
		// Ширина видео
		MOV EDX, DWORD PTR SS : [ESP + 0xC];
		MOV DWORD PTR DS : [EAX + 0xC], EDX;
		// Высота видео
//		MOV EDX, DWORD PTR SS : [ESP + 0x10];
		FILD DWORD PTR DS : [ESP + 0x10];
		FMUL DWORD PTR DS : movie_aspect;
		FISTP DWORD PTR DS : [EAX + 0x0C];
//		MOV DWORD PTR DS : [EAX + 0x0C], EDX;
		RETN;
	}
}


void __declspec(naked)Movie_Hook_1_1_ENG() {
	static int init = 0x1;
	static float half = 0.5f;
	static int sub_10071270 = 0x00071270;
	static int sub_101C2508 = 0x001C2508;
	static int sub_101C2558 = 0x001C2558;
	static int sub_101C26E8 = 0x001C26E8;

	_asm {
		CMP init, 0;
		JZ label_begin;
		MOV init, 0;
		PUSH EAX;
		MOV EAX, sub_10071270;
		ADD EAX, LS3DF_base_addr;
		MOV sub_10071270, EAX;
		MOV EAX, sub_101C2508;
		ADD EAX, LS3DF_base_addr;
		MOV sub_101C2508, EAX;
		MOV EAX, sub_101C2558;
		ADD EAX, LS3DF_base_addr;
		MOV sub_101C2558, EAX;
		MOV EAX, sub_101C26E8;
		ADD EAX, LS3DF_base_addr;
		MOV sub_101C26E8, EAX;
		POP EAX;

	label_begin:
		MOV EAX, DWORD PTR DS : [sub_101C26E8];		// MOV AL,BYTE PTR DS:[101C26E8]
		MOV AL, BYTE PTR DS : [EAX];				// MOV AL,BYTE PTR DS:[101C26E8]
		MOV ECX, DWORD PTR DS : [sub_101C2508];		// MOV ECX,DWORD PTR DS:[101C2508]
		MOV ECX, DWORD PTR DS : [ECX];				// MOV ECX,DWORD PTR DS:[101C2508]
		SUB ESP, 0x10;
		TEST AL, AL;
		JS SHORT label_100714A8;
		MOV ECX, DWORD PTR DS : [sub_101C2558];		// MOV ECX,DWORD PTR DS:[101C2558]
		MOV ECX, DWORD PTR DS : [ECX];				// MOV ECX,DWORD PTR DS:[101C2558]
	label_100714A8:
		MOV EAX, DWORD PTR SS : [ESP + 0x18];
		XOR EDX, EDX;
		CMP EAX, EDX;
		MOV DWORD PTR SS : [ESP], EDX;
		MOV DWORD PTR SS : [ESP + 0x4], EDX;
		MOV DWORD PTR SS : [ESP + 0x8], 0x280;
		MOV DWORD PTR SS : [ESP + 0x0C], 0x1E0;
		JNE SHORT label_100714D8;
		LEA EAX, [ESP];
		PUSH EAX;
		PUSH ECX;
		CALL DWORD PTR DS : [GetClientRect];
		JMP SHORT label_100714F3;
	label_100714D8:
		MOV ECX, DWORD PTR DS : [EAX];
		MOV EDX, DWORD PTR DS : [EAX + 0x4];
		MOV DWORD PTR SS : [ESP], ECX;
		MOV ECX, DWORD PTR DS : [EAX + 0x8];
		MOV DWORD PTR SS : [ESP + 0x4], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x0C];
		MOV DWORD PTR SS : [ESP + 0x8], ECX;
		MOV DWORD PTR SS : [ESP + 0x0C], EDX;
	label_100714F3:
		MOV ECX, DWORD PTR SS : [ESP + 0x14];
		LEA EAX, [ECX + 0x28];

		CALL label_fix;

		CALL sub_10071270;
		XOR EAX, EAX;
		ADD ESP, 0x10;
		RET 0x8;

	label_fix:
		CMP movie_stretch, 1;
		JE label_fix2;
		// Левая сторона видео
//		MOV EDX, DWORD PTR SS : [ESP + 0x4];
		FLD DWORD PTR DS : cur_game_width;
		FILD DWORD PTR DS : [ESP + 0xC];
		FDIV DWORD PTR DS : movie_aspect;
		FSUBP ST(1), ST;
		FMUL DWORD PTR DS : half;
		FISTP DWORD PTR DS : [EAX];
//		MOV DWORD PTR DS : [EAX], EDX;
		// Верхняя сторона видео
		MOV EDX, DWORD PTR SS : [ESP + 0x8];
		MOV DWORD PTR DS : [EAX + 0x4], EDX;
		// Ширина видео
//		MOV EDX, DWORD PTR SS : [ESP + 0xC];
		FILD DWORD PTR DS : [ESP + 0xC];
		FDIV DWORD PTR DS : movie_aspect;
		FISTP DWORD PTR DS : [EAX + 0x8];
//		MOV DWORD PTR DS : [EAX + 0xC], EDX;
		// Высота видео
		MOV EDX, DWORD PTR SS : [ESP + 0x10];
		MOV DWORD PTR DS : [EAX + 0x0C], EDX;
		RETN;
	label_fix2:
		// Левая сторона видео
		MOV EDX, DWORD PTR SS : [ESP + 0x4];
		MOV DWORD PTR DS : [EAX], EDX;
		// Верхняя сторона видео
//		MOV EDX, DWORD PTR SS : [ESP + 0x8];
		FLD DWORD PTR DS : cur_game_height;
		FILD DWORD PTR DS : [ESP + 0x10];
		FMUL DWORD PTR DS : movie_aspect;
		FSUBP ST(1), ST;
		FMUL DWORD PTR DS : half;
		FISTP DWORD PTR DS : [EAX + 0x4];
//		MOV DWORD PTR DS : [EAX + 0x4], EDX;
		// Ширина видео
		MOV EDX, DWORD PTR SS : [ESP + 0xC];
		MOV DWORD PTR DS : [EAX + 0xC], EDX;
		// Высота видео
//		MOV EDX, DWORD PTR SS : [ESP + 0x10];
		FILD DWORD PTR DS : [ESP + 0x10];
		FMUL DWORD PTR DS : movie_aspect;
		FISTP DWORD PTR DS : [EAX + 0x0C];
//		MOV DWORD PTR DS : [EAX + 0x0C], EDX;
		RETN;
	}
}


void __declspec(naked)Movie_Hook_1_2_ENG() {
	static int init = 0x1;
	static float half = 0.5f;
	static int sub_10074380 = 0x00074380;
	static int sub_101C5408 = 0x001C5408;
	static int sub_101C5458 = 0x001C5458;
	static int sub_101C55E8 = 0x001C55E8;

	_asm {
		CMP init, 0;
		JZ label_begin;
		MOV init, 0;
		PUSH EAX;
		MOV EAX, sub_10074380;
		ADD EAX, LS3DF_base_addr;
		MOV sub_10074380, EAX;
		MOV EAX, sub_101C5408;
		ADD EAX, LS3DF_base_addr;
		MOV sub_101C5408, EAX;
		MOV EAX, sub_101C5458;
		ADD EAX, LS3DF_base_addr;
		MOV sub_101C5458, EAX;
		MOV EAX, sub_101C55E8;
		ADD EAX, LS3DF_base_addr;
		MOV sub_101C55E8, EAX;
		POP EAX;

	label_begin:
		MOV EAX, DWORD PTR DS : [sub_101C55E8];		// MOV AL,BYTE PTR DS:[101C55E8]
		MOV AL, BYTE PTR DS : [EAX];				// MOV AL,BYTE PTR DS:[101C55E8]
		MOV ECX, DWORD PTR DS : [sub_101C5408];		// MOV ECX,DWORD PTR DS:[101C5408]
		MOV ECX, DWORD PTR DS : [ECX];				// MOV ECX,DWORD PTR DS:[101C5408]
		SUB ESP, 0x10;
		TEST AL, AL;
		JS SHORT label_100714A8;
		MOV ECX, DWORD PTR DS : [sub_101C5458];		// MOV ECX,DWORD PTR DS:[101C5458]
		MOV ECX, DWORD PTR DS : [ECX];				// MOV ECX,DWORD PTR DS:[101C5458]
	label_100714A8:
		MOV EAX, DWORD PTR SS : [ESP + 0x18];
		XOR EDX, EDX;
		CMP EAX, EDX;
		MOV DWORD PTR SS : [ESP], EDX;
		MOV DWORD PTR SS : [ESP + 0x4], EDX;
		MOV DWORD PTR SS : [ESP + 0x8], 0x280;
		MOV DWORD PTR SS : [ESP + 0x0C], 0x1E0;
		JNE SHORT label_100714D8;
		LEA EAX, [ESP];
		PUSH EAX;
		PUSH ECX;
		CALL DWORD PTR DS : [GetClientRect];
		JMP SHORT label_100714F3;
	label_100714D8:
		MOV ECX, DWORD PTR DS : [EAX];
		MOV EDX, DWORD PTR DS : [EAX + 0x4];
		MOV DWORD PTR SS : [ESP], ECX;
		MOV ECX, DWORD PTR DS : [EAX + 0x8];
		MOV DWORD PTR SS : [ESP + 0x4], EDX;
		MOV EDX, DWORD PTR DS : [EAX + 0x0C];
		MOV DWORD PTR SS : [ESP + 0x8], ECX;
		MOV DWORD PTR SS : [ESP + 0x0C], EDX;
	label_100714F3:
		MOV ECX, DWORD PTR SS : [ESP + 0x14];
		LEA EAX, [ECX + 0x2C];

		CALL label_fix;

		CALL sub_10074380;
		XOR EAX, EAX;
		ADD ESP, 0x10;
		RET 0x8;

	label_fix:
		CMP movie_stretch, 1;
		JE label_fix2;
		// Левая сторона видео
		//		MOV EDX, DWORD PTR SS : [ESP + 0x4];
		FLD DWORD PTR DS : cur_game_width;
		FILD DWORD PTR DS : [ESP + 0xC];
		FDIV DWORD PTR DS : movie_aspect;
		FSUBP ST(1), ST;
		FMUL DWORD PTR DS : half;
		FISTP DWORD PTR DS : [EAX];
		//		MOV DWORD PTR DS : [EAX], EDX;
		// Верхняя сторона видео
		MOV EDX, DWORD PTR SS : [ESP + 0x8];
		MOV DWORD PTR DS : [EAX + 0x4], EDX;
		// Ширина видео
		//		MOV EDX, DWORD PTR SS : [ESP + 0xC];
		FILD DWORD PTR DS : [ESP + 0xC];
		FDIV DWORD PTR DS : movie_aspect;
		FISTP DWORD PTR DS : [EAX + 0x8];
		//		MOV DWORD PTR DS : [EAX + 0xC], EDX;
		// Высота видео
		MOV EDX, DWORD PTR SS : [ESP + 0x10];
		MOV DWORD PTR DS : [EAX + 0x0C], EDX;
		RETN;
	label_fix2:
		// Левая сторона видео
		MOV EDX, DWORD PTR SS : [ESP + 0x4];
		MOV DWORD PTR DS : [EAX], EDX;
		// Верхняя сторона видео
		//		MOV EDX, DWORD PTR SS : [ESP + 0x8];
		FLD DWORD PTR DS : cur_game_height;
		FILD DWORD PTR DS : [ESP + 0x10];
		FMUL DWORD PTR DS : movie_aspect;
		FSUBP ST(1), ST;
		FMUL DWORD PTR DS : half;
		FISTP DWORD PTR DS : [EAX + 0x4];
		//		MOV DWORD PTR DS : [EAX + 0x4], EDX;
		// Ширина видео
		MOV EDX, DWORD PTR SS : [ESP + 0xC];
		MOV DWORD PTR DS : [EAX + 0xC], EDX;
		// Высота видео
		//		MOV EDX, DWORD PTR SS : [ESP + 0x10];
		FILD DWORD PTR DS : [ESP + 0x10];
		FMUL DWORD PTR DS : movie_aspect;
		FISTP DWORD PTR DS : [EAX + 0x0C];
		//		MOV DWORD PTR DS : [EAX + 0x0C], EDX;
		RETN;
	}
}


void __declspec(naked)VD_Hook_1_0_ENG() {
	_asm {
		ADD [ESP], 0x2;
		FLD DWORD PTR DS : [ESP + 0x0B8];		// Грузим знечение в стек
		FLD DWORD PTR DS : vd_min;				// Грузим минимальное знечение в стек
		FCOMiP ST, ST(1);						// Сравниваем с минимальным значением, вытылкиваем минимальное знечение
		JAE finaly;								// Если меньше или равно, то прыг
		FLD DWORD PTR DS : vd_max;				// Грузим максимальное знечение в стек
		FCOMiP ST, ST(1);						// Сравниваем с максимальным значением, вытылкиваем максимальное знечение
		JBE finaly;								// Если больше или равно, то прыг
		FFREE ST;								// Освобождаем стек
		FLD DWORD PTR DS : vd_value;			// Грузим новое знечение в стек
//		FADD DWORD PTR DS : view_distance;		// Добавляем к значению данные
	finaly:
		FSTP DWORD PTR DS : [ESP + 0x0B8];		// Сохраняем значение, вытылкиваем знечение
		MOV ECX, DWORD PTR SS : [ESP + 0x0B8];
		RETN;
	}
}


void __declspec(naked)Other_Ru_Credits_Hook_1_0_ENG() {
	static int sub_00560550 = 0x00560550;

	_asm {
		MOV ECX, DWORD PTR SS : [ESP + 0x0C];
		MOV EDX, DWORD PTR DS : [ESI + 0x8];
		PUSH 0x0;
		PUSH 0x1;
		PUSH 0x2;
		PUSH - 0x1;
		PUSH 0x41F00000;
		PUSH 0x41A00000;
		PUSH ECX;

		CMP hud_patch, 0;
		JE label_ori;
		PUSH hud_credits_text_2_x;
		JMP label_end;
	label_ori:
		PUSH 0x43C30000;

	label_end:
		PUSH EDX;
		MOV ECX, 0x00658330;
		CALL sub_00560550;
		POP ESI;
		POP ECX;
		RET 0x4;
	}
}


void __declspec(naked)Other_Ru_Credits_Hook_1_1_ENG() {
	static int sub_0060E0A0 = 0x0060E0A0;

	_asm {
		MOV ECX, DWORD PTR SS : [ESP + 0x0C];
		MOV EDX, DWORD PTR DS : [ESI + 0x8];
		PUSH 0x0;
		PUSH 0x1;
		PUSH 0x2;
		PUSH - 0x1;
		PUSH 0x41F00000;
		PUSH 0x41A00000;
		PUSH ECX;

		CMP hud_patch, 0;
		JE label_ori;
		PUSH hud_credits_text_2_x;
		JMP label_end;
	label_ori:
		PUSH 0x43C30000;
	
	label_end:
		PUSH EDX;
		MOV ECX, 0x006BE8B0;
		CALL sub_0060E0A0;
		POP ESI;
		POP ECX;
		RET 0x4;
	}
}
