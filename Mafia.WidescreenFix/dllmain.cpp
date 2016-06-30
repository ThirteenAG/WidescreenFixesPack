//2016.06.29
//created by behar
//http://forum.mafia-game.ru/showthread.php?t=2059
#include "..\includes\stdafx.h"
#include "..\includes\CPatch.h"
#pragma warning(disable: 4733)


// ReadINI()
void ReadINI(char*);
int fov_patch;
int hud_patch;
int borders_patch;
int map_patch;
int text_patch;
int menu_patch;
int movie_patch;
int draw_dist_patch;
float draw_dist_value;
float draw_dist_min;
float draw_dist_max;
int write_log;
int fps_limit;
int ru_credits;

// ClearLog(), WriteLog()
void ClearLog();
void WriteLog(const char*, ...);
FILE *LogFile;

// Functions
void v10_Eng_Get_Game_Resolution(), v11_Eng_Get_Game_Resolution(), v12_Eng_Get_Game_Resolution();
void v10_Eng_FOV_Hook(), v11_Eng_FOV_Hook(), v12_Eng_FOV_Hook();
void v10_Eng_Map_Hook_Player_Marker_x(), v11_Eng_Map_Hook_Player_Marker_x();
void v10_Eng_Menu_Hook_01(), v11_Eng_Menu_Hook_01(), v12_Eng_Menu_Hook_01();
void v10_Eng_Menu_Hook_02(), v11_Eng_Menu_Hook_02(), v12_Eng_Menu_Hook_02();
void v10_Eng_Menu_Hook_03();
void v10_Eng_Movie_Hook(), v11_Eng_Movie_Hook(), v12_Eng_Movie_Hook();
void v10_Eng_Draw_Distance_Hook();
void v10_Eng_Ru_Credits_Hook(), v11_Eng_Ru_Credits_Hook();

// Modules Base Addresses
unsigned int base_LS3DF;

// Init()
int game_version;
enum { MAFIA_1_0_ENG, MAFIA_1_1_ENG, MAFIA_1_2_ENG };

// GetGameResolution()
float cur_game_width = 800.0f;
float cur_game_height = 600.0f;

// SetHooks()
float ori_base_width = 800.0f;
float cur_base_width = 800.0f;
float ori_base_height = 600.0f;
//float cur_base_height = 600.0f;
float ori_width_p_height = 4.0f / 3.0f;
float cur_width_p_height = 4.0f / 3.0f;
//float ori_height_p_width = 3.0f / 4.0f;
//float cur_height_p_width = 3.0f / 4.0f;
//float one_p_ori_base_width = 1.0f / 800.0f;
float one_p_cur_base_width = 1.0f / 800.0f;
//float one_p_ori_base_height = 1.0f / 600.0f;
//float one_p_cur_base_height = 1.0f / 600.0f;

// HUDHook()
float hud_credits_text_2_x;

// MapHook()
float map_player_marker_x;

// MenuHook()
float menu_menu_x;

// MovieHook()
float movie_aspect;
int temp = 0x0;
float const half = 0.5f;


bool Init() {
	// Detect game version
	if ((*(unsigned int *)0x0063C000) == 0x004C6150) {
		// 1.0 Eng
		if ((*(unsigned int *)0x00401000) != 0x0424448B) {
			MessageBox(NULL, "Detected Game.exe: v.1.0 Eng.\n\nThis version of Game.exe infected with SafeDisc.\nWideScreen Fix will be disabled.", "Mafia WideScreen Fix", MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND);
			return false;
		}
		game_version = MAFIA_1_0_ENG;
		WriteLog("Detected Game.exe: v.1.0 eng\n");
	}
	else if ((*(unsigned int *)0x00623000) == 0x0044D570) {
		// 1.1 Eng
		if ((*(unsigned int *)0x00401000) != 0x79780D8A) {
			MessageBox(NULL, "Detected Game.exe: v.1.1 Eng.\n\nThis version of Game.exe infected with SafeDisc.\nWideScreen Fix will be disabled.", "Mafia WideScreen Fix", MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND);
			return false;
		}
		game_version = MAFIA_1_1_ENG;
		WriteLog("Detected Game.exe: v.1.1 eng\n");
	}
	else if ((*(unsigned int *)0x00624000) == 0x0044E120) {
		// 1.2 Eng, 1.2 Rus 1C
		if ((*(unsigned int *)0x00401000) != 0x8A480D8A) {
			MessageBox(NULL, "Detected Game.exe: v.1.2 Eng.\n\nThis version of Game.exe infected with SafeDisc.\nWideScreen Fix will be disabled.", "Mafia WideScreen Fix", MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND);
			return false;
		}
		game_version = MAFIA_1_2_ENG;
		WriteLog("Detected Game.exe: v.1.2 eng\n");
	}
	else {
		MessageBox(NULL, "This version of Game.exe is not supported.\nWideScreen Fix will be disabled.", "Mafia WideScreen Fix", MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND);
		return false;
	}

	// Detect LS3DF.dll base address
	HMODULE hLS3DF;
	static int v10_sub_0062439A[3] = { 0x0062439A, 0x0060F508, 0x0060FCD8 };
	unsigned int LS3DF_TEV_addr = **(unsigned int **)(v10_sub_0062439A[game_version] + 0x2);
	WriteLog("LS3DF.dll TEV func address: 0x%X\n", LS3DF_TEV_addr);
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)LS3DF_TEV_addr, &hLS3DF);
	base_LS3DF = (unsigned int)hLS3DF;
	WriteLog("LS3DF.dll base address: 0x%X\n", base_LS3DF);
	if (!base_LS3DF) {
		MessageBox(NULL, "Can't get LS3DF.dll base address.\nWideScreen Fix will be disabled.", "Mafia WideScreen Fix", MB_ICONERROR | MB_SYSTEMMODAL | MB_SETFOREGROUND);
		return false;
	}
	return true;
}

void GetGameResolution() {
	// Ставим главный хук - получаем разрешение игры и вызываем SetHooks()
	static void *Get_Game_Resolution[3] = { v10_Eng_Get_Game_Resolution, v11_Eng_Get_Game_Resolution, v12_Eng_Get_Game_Resolution };

	static int v10_sub_005FA58C[3] = { 0x005FA58C, 0x005BF074, 0x005BF814 };
	CPatch::RedirectCall(v10_sub_005FA58C[game_version], Get_Game_Resolution[game_version]);

	// Прибиваем Matrox Parhelia
	if (game_version != MAFIA_1_0_ENG) {
		static int v11_sub_00600D8C[3] = { NULL, 0x00600D8C, 0x0060157C };
		static int v11_sub_00600DAE[3] = { NULL, 0x00600DAE, 0x0060159E };
		CPatch::SetUChar(v11_sub_00600D8C[game_version], 0xEB);
		CPatch::SetUChar(v11_sub_00600DAE[game_version], 0xEB);

		static int v11_sub_0055D025[3] = { NULL, 0x0055D025, 0x0055D6B5 };
		CPatch::SetUChar(v11_sub_0055D025[game_version], 0xEB);
	}
}

void FovHook() {
	static void *FOV_Hook[3] = { v10_Eng_FOV_Hook, v11_Eng_FOV_Hook, v12_Eng_FOV_Hook };

	if (game_version == MAFIA_1_0_ENG) {
		static int v10_sub_1000ABEE[3] = { 0x0000ABEE, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000ABEE[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000ABEE[game_version] + 0x5, 0x1);

		static int v10_sub_1000ACA1[3] = { 0x0000ACA1, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000ACA1[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000ACA1[game_version] + 0x5, 0x1);

		static int v10_sub_1000B0AC[3] = { 0x0000B0AC, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000B0AC[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000B0AC[game_version] + 0x5, 0x1);

		static int v10_sub_1000B166[3] = { 0x0000B166, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000B166[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000B166[game_version] + 0x5, 0x1);

		static int v10_sub_1000B34C[3] = { 0x0000B34C, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000B34C[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000B34C[game_version] + 0x5, 0x1);

		static int v10_sub_1000B406[3] = { 0x0000B406, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000B406[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000B406[game_version] + 0x5, 0x1);

		static int v10_sub_1000B5C4[3] = { 0x0000B5C4, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000B5C4[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000B5C4[game_version] + 0x5, 0x1);

		static int v10_sub_1000B677[3] = { 0x0000B677, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000B677[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000B677[game_version] + 0x5, 0x1);

		static int v10_sub_1000B834[3] = { 0x0000B834, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000B834[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000B834[game_version] + 0x5, 0x1);

		static int v10_sub_1000B8E7[3] = { 0x0000B8E7, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000B8E7[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000B8E7[game_version] + 0x5, 0x1);

		static int v10_sub_1000BA9A[3] = { 0x0000BA9A, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000BA9A[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000BA9A[game_version] + 0x5, 0x1);

		static int v10_sub_1000BB54[3] = { 0x0000BB54, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000BB54[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000BB54[game_version] + 0x5, 0x1);

		static int v10_sub_1000BCC6[3] = { 0x0000BCC6, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000BCC6[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000BCC6[game_version] + 0x5, 0x1);

		static int v10_sub_1000BD7D[3] = { 0x0000BD7D, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000BD7D[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000BD7D[game_version] + 0x5, 0x1);

		static int v10_sub_1000BF20[3] = { 0x0000BF20, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000BF20[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000BF20[game_version] + 0x5, 0x1);

		static int v10_sub_1000BFD7[3] = { 0x0000BFD7, NULL, NULL };
		CPatch::RedirectCall(base_LS3DF + v10_sub_1000BFD7[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v10_sub_1000BFD7[game_version] + 0x5, 0x1);
	}
	else {
		static int v11_sub_1000B0B9[3] = { NULL, 0x0000B0B9, 0x0000B519 };
		CPatch::RedirectCall(base_LS3DF + v11_sub_1000B0B9[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v11_sub_1000B0B9[game_version] + 0x5, 0x1);

		static int v11_sub_1000B1A4[3] = { NULL, 0x0000B1A4, 0x0000B604 };
		CPatch::RedirectCall(base_LS3DF + v11_sub_1000B1A4[game_version], FOV_Hook[game_version]);
		CPatch::Nop(base_LS3DF + v11_sub_1000B1A4[game_version] + 0x5, 0x1);
	}
}

void HUDHook() {
// Устанавливаем правильные пропорции:

	// HUD, титры авторов... 
	static int v10_sub_005533A7[3] = { 0x005533A7, 0x00600FBF, 0x006017AF };
	CPatch::SetPointer(v10_sub_005533A7[game_version] + 0x2, &one_p_cur_base_width);

	// HUD: спидометр, тахометр, часы, значок ограничителя скорости. Рассчитывается только при старте игры
	static int v10_sub_005533EB[3] = { 0x005533EB, 0x00601003, 0x006017F3 };
	CPatch::SetPointer(v10_sub_005533EB[game_version] + 0x2, &one_p_cur_base_width);

	float fix_hud_left;
	float fix_hud_center;
	float fix_hud_right;
	float fix_hud_double;
	if (hud_patch == 1) {
		fix_hud_left = (cur_base_width - ori_base_width) / 2.0f;
		fix_hud_center = (cur_base_width - ori_base_width) / 2.0f;
		fix_hud_right = (cur_base_width - ori_base_width) / 2.0f;
		fix_hud_double = cur_base_width - ori_base_width;
	}
	else {
		fix_hud_left = 0.0f;
		fix_hud_center = (cur_base_width - ori_base_width) / 2.0f;
		fix_hud_right = cur_base_width - ori_base_width;
		fix_hud_double = cur_base_width - ori_base_width;
	}
	
#pragma region *HUD*
	// Спидометр:						Привязка к правому краю.
	static float hud_speedometer_x = 189.0f + fix_hud_left;
	static int v10_sub_0055340C[3] = { 0x0055340C, 0x0060116A, 0x0060195A };
	CPatch::SetPointer(v10_sub_0055340C[game_version] + 0x2, &hud_speedometer_x);

	// Тахометр:						Привязка к правому краю.
	static float hud_tachometer_x = 259.0f + fix_hud_left;
	static int v10_sub_0055344D[3] = { 0x0055344D, 0x006011AB, 0x0060199B };
	CPatch::SetPointer(v10_sub_0055344D[game_version] + 0x2, &hud_tachometer_x);

	// Значок ограничителя скорости:	С версии 1.1 привязка к правому краю.
	float hud_speed_limiter_x;
	if (game_version == MAFIA_1_0_ENG) {
		hud_speed_limiter_x = 760.0f + fix_hud_right;
	}
	else {
		hud_speed_limiter_x = -40.0f - fix_hud_left;
	}
	static int v10_sub_00553C2D[3] = { 0x00553C2D, 0x00601B21, 0x00602311 };
	CPatch::SetFloat(v10_sub_00553C2D[game_version] + 0x4, hud_speed_limiter_x);

	// Часы:							С версии 1.1 привязка к правому краю.
	float hud_timer_x;
	if (game_version == MAFIA_1_0_ENG) {
		hud_timer_x = 694.0f + fix_hud_right;
	}
	else {
		hud_timer_x = -106.0f - fix_hud_left;
	}
	static int v10_sub_00553CAD[3] = { 0x00553CAD, 0x00601BB5, 0x006023A5 };
	CPatch::SetFloat(v10_sub_00553CAD[game_version] + 0x4, hud_timer_x);

	// Компас:
	static float hud_compas_x = 0.0f + fix_hud_left;
	static int v10_sub_00550EF4[3] = { 0x00550EF4, 0x005FE753, 0x005FEF43 };
	CPatch::SetPointer(v10_sub_00550EF4[game_version] + 0x2, &hud_compas_x);

	// МиниКарта:
	static float hud_minimap_x = 29.0f + fix_hud_left;
	static int v10_sub_0054FDBC[3] = { 0x0054FDBC, 0x005FD5CF, 0x005FDDBF };
	CPatch::SetPointer(v10_sub_0054FDBC[game_version] + 0x2, &hud_minimap_x);

	// Тип коробки передач:				С версии 1.1 привязка к правому краю.
	static float hud_transmission_x;
	static int v10_sub_0054E9F5[3] = { 0x0054E9F5, 0x005FC142, 0x005FC932 };
	if (game_version == MAFIA_1_0_ENG) {
		hud_transmission_x = 758.0f + fix_hud_right;
	}
	else {
		hud_transmission_x = 42.0f + fix_hud_left;
	}
	CPatch::SetPointer(v10_sub_0054E9F5[game_version] + 0x2, &hud_transmission_x);

	// Индикатор отсутствия топлива:	С версии 1.1 привязка к правому краю.
	static float hud_fuel_led_x;
	static int v10_sub_0054EBD4[3] = { 0x0054EBD4, 0x005FC333, 0x005FCB23 };
	if (game_version == MAFIA_1_0_ENG) {
		hud_fuel_led_x = 668.0f + fix_hud_right;
	}
	else {
		hud_fuel_led_x = 132.0f + fix_hud_left;
	}
	CPatch::SetPointer(v10_sub_0054EBD4[game_version] + 0x2, &hud_fuel_led_x);

	// Значок действия:
	static float hud_action_bar_x = 9.0f + fix_hud_left;
	static int v10_sub_0054F2F3[3] = { 0x0054F2F3, 0x005FCA99, 0x005FD289 };
	CPatch::SetPointer(v10_sub_0054F2F3[game_version] + 0x2, &hud_action_bar_x);

	// Значок здоровья персонажей (без текста):
	static float hud_health_bar_x = 7.0f + fix_hud_left;
	static int v10_sub_00549ACF[3] = { 0x00549ACF, 0x005F71EF, 0x005F79DF };
	CPatch::SetPointer(v10_sub_00549ACF[game_version] + 0x2, &hud_health_bar_x);

	// Значок здоровья персонажей (текст):
	static float hud_health_bar_text_x = 42.0f + fix_hud_left;
	static int v10_sub_00549CD3[3] = { 0x00549CD3, 0x005F73EF, 0x005F7BDF };
	CPatch::SetPointer(v10_sub_00549CD3[game_version] + 0x2, &hud_health_bar_text_x);

	// Значок боеприпасов (без текста):
	static float hud_ammo_bar_x = 62.0f + fix_hud_left;
	static int v10_sub_00549BC6[3] = { 0x00549BC6, 0x005F72E6, 0x005F7AD6 };
	CPatch::SetPointer(v10_sub_00549BC6[game_version] + 0x2, &hud_ammo_bar_x);

	// Значок боеприпасов (текст):
	static float hud_ammo_bar_text_x = 112.0f + fix_hud_left;
	static int v10_sub_0054A089[3] = { 0x0054A089, 0x005F77A9, 0x005F7F99 };
	CPatch::SetPointer(v10_sub_0054A089[game_version] + 0x2, &hud_ammo_bar_text_x);

	// Значок поиска (розыск):
	static float hud_search_bar_x = 336.0f + fix_hud_center;
	static int v10_sub_0054E022[3] = { 0x0054E022, 0x005FB757, 0x005FBF47 };
	CPatch::SetPointer(v10_sub_0054E022[game_version] + 0x2, &hud_search_bar_x);

	// Значок "Здоровье" (у машины):	С версии 1.1 привязка к центру.
	static float hud_live_bar_x = 368.0f + fix_hud_center;
	static int v10_sub_0054ED61[3] = { 0x0054ED61, NULL, NULL };
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::SetPointer(v10_sub_0054ED61[game_version] + 0x2, &hud_live_bar_x);
	}

	// Полоса действия (взлом машины или при замахе кулаком, битой):
	static float hud_progress_bar_x = 250.0f + fix_hud_center;
	static int v10_sub_0054F5AA[3] = { 0x0054F5AA, 0x005FCD62, 0x005FD552 };
	CPatch::SetPointer(v10_sub_0054F5AA[game_version] + 0x2, &hud_progress_bar_x);

	// Полоса действия 2 (самолет в миссии Сливки общества - Аэропорт):
	static float hud_progress_bar2_x = 350.0f + fix_hud_center;
	static int v10_sub_0054F8F5[3] = { 0x0054F8F5, 0x005FD0CB, 0x005FD8BB };
	CPatch::SetPointer(v10_sub_0054F8F5[game_version] + 0x2, &hud_progress_bar2_x);

	// Всплывающий текст слева:
	static float hud_left_popup_text_x = 9.0f + fix_hud_left;
	static int v10_sub_0054F54C[3] = { 0x0054F54C, 0x005FCCFE, 0x005FD4EE };
	CPatch::SetPointer(v10_sub_0054F54C[game_version] + 0x2, &hud_left_popup_text_x);

	// Деньги, текст справа сверху:
	static float hud_money_right_text_x = 760.0f + fix_hud_right;
	static int v10_sub_0054F086[3] = { 0x0054F086, 0x005FC81D, 0x005FD00D };
	CPatch::SetPointer(v10_sub_0054F086[game_version] + 0x2, &hud_money_right_text_x);
#pragma endregion

#pragma region *Race Mission Specific*
	// Гонка "3, 2, 1, Go":
	static float hud_race_321go_x = 400.0f + fix_hud_center;
	static int v10_sub_0055274A[3] = { 0x0055274A, 0x006000F0, 0x006008E0 };
	static int v10_sub_00552954[3] = { 0x00552954, 0x00600331, 0x00600B21 };
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::SetPointer(v10_sub_0055274A[game_version] + 0x2, &hud_race_321go_x);
		CPatch::SetPointer(v10_sub_00552954[game_version] + 0x2, &hud_race_321go_x);
	}
	else {
		CPatch::SetFloat(v10_sub_0055274A[game_version] + 0x1, hud_race_321go_x);
		CPatch::SetFloat(v10_sub_00552954[game_version] + 0x1, hud_race_321go_x);
	}

	// Гонка, "Едем не туда":
	float hud_race_wrong_way_x = 272.0f + fix_hud_center;
	static int v10_sub_00552C90[3] = { 0x00552C90, 0x006006AE, 0x00600E9E };
	CPatch::SetFloat(v10_sub_00552C90[game_version] + 0x1, hud_race_wrong_way_x);

	// Гонка, текст по центру ("Контрольная точка"):
	float hud_race_checkpoint_x = 390.0f + fix_hud_center;
	static int v10_sub_00552BFE[3] = { 0x00552BFE, 0x00600609, 0x00600DF9 };
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::SetFloat(v10_sub_00552BFE[game_version] + 0x4, hud_race_checkpoint_x);
	}
	else {
		CPatch::SetFloat(v10_sub_00552BFE[game_version] + 0x1, hud_race_checkpoint_x);
	}

	// Гонка, текст справа сверху:
	static float hud_race_right_text_x = 780.0f + fix_hud_right;
	static int v10_sub_00552AEC[3] = { 0x00552AEC, 0x006004F0, 0x00600CE0 };
	CPatch::SetPointer(v10_sub_00552AEC[game_version] + 0x2, &hud_race_right_text_x);
#pragma endregion

#pragma region *Map*
	// Надпись "Нет карты":
	static float hud_map_text_x = 350.0f + fix_hud_center;
	static int v10_sub_00552017[3] = { 0x00552017, 0x005FF933, 0x00600123 };
	CPatch::SetPointer(v10_sub_00552017[game_version] + 0x2, &hud_map_text_x);
#pragma endregion

#pragma region *Black Borders, Text*
	// Черные полосы:
	static float hud_borders_width = cur_base_width + 1.0f;
	float hud_borders_1 = (ori_base_width * 0.540540516376495f) / cur_base_width;
	float hud_borders_2 = (ori_base_width * 0.5f) / cur_base_width;
	static int v10_sub_0054C189[3] = { 0x0054C189, 0x005F9869, 0x005FA059 };
	static int v10_sub_005BC418[3] = { 0x005BC418, 0x0046DA4F, 0x0046E60F };
	static int v10_sub_00594899[3] = { 0x00594899, 0x00548C1D, 0x005492AD };
	static int v10_sub_00548E6C[3] = { 0x00548E6C, 0x005F661C, 0x005F6E0C };
	CPatch::SetPointer(v10_sub_0054C189[game_version] + 0x2, &hud_borders_width);
	if (borders_patch) {
		CPatch::SetFloat(v10_sub_005BC418[game_version] + 0x1, hud_borders_1);
		CPatch::SetFloat(v10_sub_00594899[game_version] + 0x1, hud_borders_1);
		CPatch::SetFloat(v10_sub_00548E6C[game_version] + 0x1, hud_borders_2);
	}

	// Текст в центре на черном фоне:
	static float hud_black_centred_text_x = 400.0f + fix_hud_center;
	static int v10_sub_0054B80B[3] = { 0x0054B80B, 0x005F8EFB, 0x005F96EB };
	CPatch::SetPointer(v10_sub_0054B80B[game_version] + 0x2, &hud_black_centred_text_x);

	// Титры:
	static float hud_credits_text_1_x = 370.0f + fix_hud_center;
	hud_credits_text_2_x = 390.0f + fix_hud_center;
	static float hud_credits_text_3_x = 400.0f + fix_hud_center;
	static float hud_credits_text_4_x = 410.0f + fix_hud_center;
	static int v10_sub_005601D9[3] = { 0x005601D9, 0x0060DD09, 0x0060E1F9 };
	static int v10_sub_00560237[3] = { 0x00560237, 0x0060DD6D, 0x0060E25D };
	static int v10_sub_00560295[3] = { 0x00560295, 0x0060DDD1, 0x0060E2C1 };
	static int v10_sub_005602E7[3] = { 0x005602E7, 0x0060DE23, 0x0060E313 };
	static int v10_sub_00560364[3] = { 0x00560364, 0x0060DEA6, 0x0060E396 };
	static int v10_sub_00560399[3] = { 0x00560399, 0x0060DEE1, 0x0060E3D1 };
	static int v10_sub_0056042C[3] = { 0x0056042C, 0x0060DF72, 0x0060E462 };
	static int v10_sub_00560461[3] = { 0x00560461, 0x0060DFA8, 0x0060E498 };
	static int v12_sub_0060E4C7[3] = { NULL, NULL, 0x0060E4C7 };
	static int v10_sub_005604BD[3] = { 0x005604BD, 0x0060DFFF, 0x0060E523 };
	static int v10_sub_00560510[3] = { 0x00560510, 0x0060E05D, 0x0060E57C };
	CPatch::SetPointer(v10_sub_005601D9[game_version] + 0x2, &hud_credits_text_2_x);
	CPatch::SetPointer(v10_sub_00560237[game_version] + 0x2, &hud_credits_text_2_x);
	CPatch::SetPointer(v10_sub_00560295[game_version] + 0x2, &hud_credits_text_2_x);
	CPatch::SetPointer(v10_sub_005602E7[game_version] + 0x2, &hud_credits_text_1_x);
	CPatch::SetPointer(v10_sub_00560364[game_version] + 0x2, &hud_credits_text_3_x);
	CPatch::SetPointer(v10_sub_00560399[game_version] + 0x2, &hud_credits_text_3_x);
	CPatch::SetPointer(v10_sub_0056042C[game_version] + 0x2, &hud_credits_text_2_x);
	CPatch::SetFloat(v10_sub_00560461[game_version] + 0x1, hud_credits_text_2_x);
	if (game_version == MAFIA_1_2_ENG) {
		CPatch::SetFloat(v12_sub_0060E4C7[game_version] + 0x1, hud_credits_text_2_x);
	}
	CPatch::SetPointer(v10_sub_005604BD[game_version] + 0x2, &hud_credits_text_1_x);
	CPatch::SetPointer(v10_sub_00560510[game_version] + 0x2, &hud_credits_text_4_x);
#pragma endregion

#pragma region *Menus*
	// Надпись "Пожалуйста подождите":
	if (!menu_patch) {
		static float hud_please_wait_text_x = 780.0f + fix_hud_double;
		static int v10_sub_0055F47B[3] = { 0x0055F47B, 0x005A3741, 0x005A3D31 };
		static int v10_sub_005DF781[3] = { 0x005DF781, 0x0060CEF5, 0x0060D3E5 };
		CPatch::SetPointer(v10_sub_0055F47B[game_version] + 0x2, &hud_please_wait_text_x);
		CPatch::SetPointer(v10_sub_005DF781[game_version] + 0x2, &hud_please_wait_text_x);
	}
#pragma endregion

}

void MapHook() {
	float fix_map_left;
	//	float fix_map_center;
	//	float fix_map_right;
	//	float fix_map_double;
	if (map_patch == 1) {
		fix_map_left = (cur_base_width - ori_base_width) / 2.0f;
		//		fix_map_center = (cur_base_width - ori_base_width) / 2.0f;
		//		fix_map_right = (cur_base_width - ori_base_width) / 2.0f;
		//		fix_map_double = cur_base_width - ori_base_width;
	}
	else {
		fix_map_left = 0.0f;
		//		fix_map_center = (cur_base_width - ori_base_width) / 2.0f;
		//		fix_map_right = cur_base_width - ori_base_width;
		//		fix_map_double = cur_base_width - ori_base_width;
	}

	// Левая граница карты:
	static float map_left_x = (ori_base_width * 0.1f + fix_map_left) / cur_base_width;
	static int v10_sub_005517D1[3] = { 0x005517D1, 0x005FF0B5, 0x005FF8A5 };
	CPatch::SetPointer(v10_sub_005517D1[game_version] + 0x2, &map_left_x);

	// Правая граница карты:
	static float map_right_x = 1.0f - map_left_x;
	static int v10_sub_00551838[3] = { 0x00551838, 0x005FF0C9, 0x005FF8B9 };
	CPatch::SetPointer(v10_sub_00551838[game_version] + 0x2, &map_right_x);

	// Позиция маркера игрока:
	static void *Map_Hook_Player_Marker_x[3] = { v10_Eng_Map_Hook_Player_Marker_x, v11_Eng_Map_Hook_Player_Marker_x, v11_Eng_Map_Hook_Player_Marker_x };
	map_player_marker_x = cur_game_height * ori_width_p_height;
	int v10_sub_00551CD5[3] = { 0x00551CD5, 0x005FF5C7, 0x005FFDB7 };
	CPatch::RedirectCall(v10_sub_00551CD5[game_version], Map_Hook_Player_Marker_x[game_version]);
	CPatch::Nop(v10_sub_00551CD5[game_version] + 0x5, 0x1);

	// Позиция маркера цели;
	static float map_width = map_right_x - map_left_x;
	static int v10_sub_0055214C[3] = { 0x0055214C, 0x005FFA6A, 0x0060025A };
	CPatch::SetPointer(v10_sub_0055214C[game_version] + 0x2, &map_width);

	float map_aspect = (map_width / (((ori_base_width * 0.9f + fix_map_left) / cur_base_width) - map_left_x));

	//
	static float map_scale_x = 0.4f * map_aspect;
	static int v10_sub_0055178D[3] = { 0x0055178D, 0x005FF022, 0x005FF812 };
	static int v10_sub_005517A2[3] = { 0x005517A2, 0x005FF037, 0x005FF827 };
	static int v10_sub_0055191D[3] = { 0x0055191D, 0x005FF1F4, 0x005FF9E4 };
	CPatch::SetPointer(v10_sub_0055178D[game_version] + 0x2, &map_scale_x);
	CPatch::SetFloat(v10_sub_005517A2[game_version] + 0x4, 1.0f - map_scale_x);
	CPatch::SetPointer(v10_sub_0055191D[game_version] + 0x2, &map_scale_x);

	//
	static float map_pos_x = 0.2f * map_aspect;
	static int v10_sub_0055173F[3] = { 0x0055173F, 0x005FEFD4, 0x005FF7C4 };
	static int v10_sub_00551CC1[3] = { 0x00551CC1, 0x005FF5B3, 0x005FFDA3 };
	static int v10_sub_00552056[3] = { 0x00552056, 0x005FF978, 0x00600168 };
	CPatch::SetPointer(v10_sub_0055173F[game_version] + 0x2, &map_pos_x);
	CPatch::SetPointer(v10_sub_00551CC1[game_version] + 0x2, &map_pos_x);
	CPatch::SetPointer(v10_sub_00552056[game_version] + 0x2, &map_pos_x);

	//
	static float map_target_pos_x = 2.5f / map_aspect;
	static int v10_sub_0055206C[3] = { 0x0055206C, 0x005FF98E, 0x0060017E };
	CPatch::SetPointer(v10_sub_0055206C[game_version] + 0x2, &map_target_pos_x);
}

void TextHook() {
	static float text_width;
/*	if (text_patch == 1) {
		text_width = (ori_base_width * 0.9f) / cur_base_width;
	}
	else {
		text_width = (cur_base_width - (ori_base_width - (ori_base_width * 0.9f))) / cur_base_width;
	}*/

	text_width = (ori_base_width * 0.9f) / cur_base_width;

	static int v10_sub_0054ABC6[3] = { 0x0054ABC6, 0x005F82D7, 0x005F8AC7 };
	static int v10_sub_0054AC3B[3] = { 0x0054AC3B, 0x005F833A, 0x005F8B2A };
	static int v10_sub_0054AEE8[3] = { 0x0054AEE8, 0x005F85E7, 0x005F8DD7 };
	static int v10_sub_0054AF60[3] = { 0x0054AF60, 0x005F864D, 0x005F8E3D };
	static int v10_sub_0054BBA2[3] = { 0x0054BBA2, 0x005F9264, 0x005F9A54 };
	static int v10_sub_0054BC0B[3] = { 0x0054BC0B, 0x005F92D4, 0x005F9AC4 };
	CPatch::SetPointer(v10_sub_0054ABC6[game_version] + 0x2, &text_width);
	CPatch::SetPointer(v10_sub_0054AC3B[game_version] + 0x2, &text_width);
	CPatch::SetPointer(v10_sub_0054AEE8[game_version] + 0x2, &text_width);
	CPatch::SetPointer(v10_sub_0054AF60[game_version] + 0x2, &text_width);
	CPatch::SetPointer(v10_sub_0054BBA2[game_version] + 0x2, &text_width);
	CPatch::SetPointer(v10_sub_0054BC0B[game_version] + 0x2, &text_width);
}

void MenuHook() {
// Устанавливаем правильные пропорции:

	static int v11_sub_005E3303[3] = { NULL, 0x005E3303, 0x005E3B03 };
	static int v10_sub_0056E525[3] = { 0x0056E525, 0x005EB28B, 0x005EBA8B };	
	if (game_version == MAFIA_1_0_ENG) {
		// Загрузочные экраны, меню, текст в машинопедии
		CPatch::SetPointer(v10_sub_0056E525[game_version] + 0x2, &one_p_cur_base_width);
	}	
	else{
		// Загрузочные экраны
		CPatch::SetPointer(v11_sub_005E3303[game_version] + 0x2, &one_p_cur_base_width);
		// Меню, текст в машинопедии
		CPatch::SetPointer(v10_sub_0056E525[game_version] + 0x2, &one_p_cur_base_width);
	}

	float fix_menu_right = (cur_base_width - ori_base_width) / 2.0f;

#pragma region *Menus*
	// Надпись "Пожалуйста подождите":
	static float menu_please_wait_text_x = 780.0f + fix_menu_right;
	static int v10_sub_0055F47B[3] = { 0x0055F47B, 0x005A3741, 0x005A3D31 };
	static int v10_sub_005DF781[3] = { 0x005DF781, 0x0060CEF5, 0x0060D3E5 };
	CPatch::SetPointer(v10_sub_0055F47B[game_version] + 0x2, &menu_please_wait_text_x);
	CPatch::SetPointer(v10_sub_005DF781[game_version] + 0x2, &menu_please_wait_text_x);

	// Меню, загрузочные экраны, машинопедия
	menu_menu_x = (cur_game_width - (cur_game_height * ori_width_p_height)) / 2.0f;
	static void *Menu_Hook_01[3] = { v10_Eng_Menu_Hook_01, v11_Eng_Menu_Hook_01, v12_Eng_Menu_Hook_01 };
	static void *Menu_Hook_02[3] = { v10_Eng_Menu_Hook_02, v11_Eng_Menu_Hook_02, v12_Eng_Menu_Hook_02 };
	static void *Menu_Hook_03[3] = { v10_Eng_Menu_Hook_03, nullptr, nullptr };
	if (game_version == MAFIA_1_0_ENG) {
		static int v10_sub_0056BF09[3] = { 0x0056BF09, NULL, NULL };
		static int v10_sub_0056BF95[3] = { 0x0056BF95, NULL, NULL };
		static int v10_sub_0056C007[3] = { 0x0056C007, NULL, NULL };
		static int v10_sub_0056C0AF[3] = { 0x0056C0AF, NULL, NULL };
		static int v10_sub_005758DF[3] = { 0x005758DF, NULL, NULL };
		// Загрузочные экраны, текст в машинопедии
		CPatch::RedirectCall(v10_sub_0056BF09[game_version], Menu_Hook_02[game_version]);
		// Меню
		CPatch::RedirectCall(v10_sub_0056BF95[game_version], Menu_Hook_01[game_version]);
		// "В гараже новый транспорт"
		CPatch::RedirectCall(v10_sub_0056C007[game_version], Menu_Hook_03[game_version]);
		// Гонка: таблица рекордов
		CPatch::RedirectCall(v10_sub_0056C0AF[game_version], Menu_Hook_03[game_version]);
		// Машинопедия, список авто
		CPatch::RedirectCall(v10_sub_005758DF[game_version], Menu_Hook_03[game_version]);
	}
	else {
		static int v11_sub_005D8D5B[3] = { NULL, 0x005D8D5B, 0x005D950B };
		static int v11_sub_005D90CB[3] = { NULL, 0x005D90CB, 0x005D98DB };
		// Меню
		CPatch::RedirectCall(v11_sub_005D8D5B[game_version], Menu_Hook_01[game_version]);
		// Загрузочные экраны, машинопедия
		CPatch::RedirectCall(v11_sub_005D90CB[game_version], Menu_Hook_02[game_version]);
	}
#pragma endregion
}

void MovieHook() {
#pragma region *Movies*
	// Видео:
	movie_aspect = cur_width_p_height / ori_width_p_height;
	static void *Movie_Hook[3] = { v10_Eng_Movie_Hook, v11_Eng_Movie_Hook, v12_Eng_Movie_Hook };
	static int v10_sub_10070430[3] = { 0x00070430, 0x00071490, 0x000745A0 };
	CPatch::RedirectJump(base_LS3DF + v10_sub_10070430[game_version], Movie_Hook[game_version]);
	if (game_version == MAFIA_1_0_ENG) {
		CPatch::Nop(base_LS3DF + v10_sub_10070430[game_version] + 0x5, 0x73);
	}
	else {
		CPatch::Nop(base_LS3DF + v10_sub_10070430[game_version] + 0x5, 0x8D);
	}
#pragma endregion
}

void DrawDistanceHook() {
	static float draw_dist_value1 = draw_dist_value - 50.0f;
	static float draw_dist_value2 = draw_dist_value - 20.0f;

	static void *Draw_Distance_Hook[3] = { v10_Eng_Draw_Distance_Hook, v10_Eng_Draw_Distance_Hook, v10_Eng_Draw_Distance_Hook };
	static int v10_sub_00600E99[3] = { 0x00600E99, 0x0054143E, 0x0054192E };
	CPatch::RedirectCall(v10_sub_00600E99[game_version], Draw_Distance_Hook[game_version]);
	CPatch::Nop(v10_sub_00600E99[game_version] + 0x5, 0x2);

	if (game_version == MAFIA_1_0_ENG) {
		static int v10_sub_005A89C9[3] = { 0x005A89C9, NULL, NULL };
		CPatch::SetFloat(v10_sub_005A89C9[game_version] + 0x1, draw_dist_value1);

		static int v10_sub_005A87BC[3] = { 0x005A87BC, NULL, NULL };
		static int v10_sub_005A88FC[3] = { 0x005A88FC, NULL, NULL };
		static int v10_sub_005A8919[3] = { 0x005A8919, NULL, NULL };
		static int v10_sub_005A892C[3] = { 0x005A892C, NULL, NULL };
		static int v10_sub_005A895F[3] = { 0x005A895F, NULL, NULL };
		CPatch::SetPointer(v10_sub_005A87BC[game_version] + 0x2, &draw_dist_value2);
		CPatch::SetPointer(v10_sub_005A88FC[game_version] + 0x2, &draw_dist_value2);
		CPatch::SetPointer(v10_sub_005A8919[game_version] + 0x2, &draw_dist_value2);
		CPatch::SetPointer(v10_sub_005A892C[game_version] + 0x2, &draw_dist_value2);
		CPatch::SetPointer(v10_sub_005A895F[game_version] + 0x2, &draw_dist_value2);
	}
	else {
		static int v11_sub_004021E1[3] = { NULL, 0x004021E1, 0x00402201 };
		CPatch::SetFloat(v11_sub_004021E1[game_version] + 0x4, draw_dist_value1);

		static int v11_sub_004021E9[3] = { NULL, 0x004021E9, 0x00402209 };
		CPatch::SetFloat(v11_sub_004021E9[game_version] + 0x4, draw_dist_value2);
	}
}

void OtherHooks() {
	// Frame limiter
	if (fps_limit) {
		static char fps_limit_ms = 1000 / fps_limit;
		if (fps_limit_ms > 127) {
			fps_limit_ms = 127;
		}
		static int v10_sub_005F95BC[3] = { 0x005F95BC, 0x005BE02C, 0x005BE7CC };
		CPatch::SetChar(v10_sub_005F95BC[game_version] + 0x1, fps_limit_ms);
	}

	// Cyrillic credits
	if (ru_credits && (game_version != MAFIA_1_2_ENG)) {
		static void *Ru_Credits_Hook[3] = { v10_Eng_Ru_Credits_Hook, v11_Eng_Ru_Credits_Hook, nullptr };
		static int v10_sub_0056017C[3] = { 0x0056017C, 0x0060DCAC, NULL };
		static int v10_sub_00560548[3] = { 0x00560548, 0x0060E09C, NULL };
		CPatch::SetChar(v10_sub_0056017C[game_version] + 0x2, 0x07);
		CPatch::SetInt(v10_sub_00560548[game_version], (int)Ru_Credits_Hook[game_version]);
	}
}

void SetHooks() {
	WriteLog("Detected game resolution: %.0fx%.0f\n", cur_game_width, cur_game_height);

	cur_width_p_height = cur_game_width / cur_game_height;
//	cur_height_p_width = cur_game_height / cur_game_width;

	cur_base_width = ori_base_height * cur_width_p_height;

	one_p_cur_base_width = 1.0f / cur_base_width;
//	one_p_cur_base_height = 1.0f / cur_base_height;

	// Исправляем FOV
	if (fov_patch) {
		FovHook();
	}
	// Исправляем HUD
	if (hud_patch) {
		HUDHook();
	}
	// Исправляем карту
	if (map_patch) {
		MapHook();
	}
	// Исправляем разбиение текста
	if (text_patch) {
		TextHook();
	}
	// Исправляем загрузочные экраны и меню
	if (menu_patch) {
		MenuHook();
	}
	// Исправляем пропорции видеороликов
	if (movie_patch) {
		MovieHook();
	}
	// Меняем дальность прорисовки
	if (draw_dist_patch) {
		DrawDistanceHook();
	}
	// Другие исправления
	OtherHooks();
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		ReadINI("");
		ClearLog();
		if (Init()){
			GetGameResolution();
		}
	}
	return TRUE;
}


#pragma region some asm
#define DW(d,c,b,a)			_asm _emit 0x ## a _asm _emit 0x ## b _asm _emit 0x ## c _asm _emit 0x ## d
#define MOV_DL_BYTE_PTR		_asm _emit 0x8A _asm _emit 0x15
#define MOV_EAX_DWORD_PTR	_asm _emit 0xA1
#define FLD_DWORD_PTR		_asm _emit 0xD9 _asm _emit 0x05
#define FSTP_DWORD_PTR		_asm _emit 0xD9 _asm _emit 0x1D
#define FILD_DWORD_PTR		_asm _emit 0xDB _asm _emit 0x05
#pragma endregion

void __declspec(naked)v10_Eng_Get_Game_Resolution() {

	_asm {
					PUSHAD;
					PUSHFD;
					FILD_DWORD_PTR DW(00,6F,95,2E);
					FSTP DWORD PTR[cur_game_width];
					FILD_DWORD_PTR DW(00,6F,95,32);
					FSTP DWORD PTR[cur_game_height];
					CALL SetHooks;
					POPFD;
					POPAD;

					MOV_EAX_DWORD_PTR DW(00,6F,95,2E);
					RETN;
	}
}

void __declspec(naked)v11_Eng_Get_Game_Resolution() {

	_asm {
					PUSHAD;
					PUSHFD;
					FILD_DWORD_PTR DW(00,64,6E,16);
					FSTP DWORD PTR[cur_game_width];
					FILD_DWORD_PTR DW(00,64,6E,1A);
					FSTP DWORD PTR[cur_game_height];
					CALL SetHooks;
					POPFD;
					POPAD;

					MOV_EAX_DWORD_PTR DW(00,64,6E,1A);
					RETN;
	}
}

void __declspec(naked)v12_Eng_Get_Game_Resolution() {

	_asm {
					PUSHAD;
					PUSHFD;
					FILD_DWORD_PTR DW(00,64,7E,E6);
					FSTP DWORD PTR[cur_game_width];
					FILD_DWORD_PTR DW(00,64,7E,EA);
					FSTP DWORD PTR[cur_game_height];
					CALL SetHooks;
					POPFD;
					POPAD;

					MOV_EAX_DWORD_PTR DW(00,64,7E,EA);
					RETN;
	}
}

void __declspec(naked)v10_Eng_FOV_Hook() {
	static int init = 0x1;

	static int sub_1009756C = 0x0009756C;

	_asm {
//					_emit 0xCC;
					CMP init, 0;
					JZ label_begin;
					MOV init, 0;
					CALL label_init;

	label_begin:
					ADD BYTE PTR[ESP], 0x1;
					//-------------------------------------------------------------------------
					PUSH EAX;									//
					MOV EAX, DWORD PTR[sub_1009756C];			// FMUL DWORD PTR[100985B0]
					FMUL DWORD PTR[EAX];						//
					POP EAX;									//
					//-------------------------------------------------------------------------
					FLD ST;
					FCOS;
					FXCH ST(1);
					FSIN;
					FDIVP ST(1), ST;
					FMUL DWORD PTR[ori_width_p_height];
					FLD DWORD PTR[cur_width_p_height];
					FXCH ST(1);
					FPATAN;
					RETN;

	label_init:
					PUSH EAX;

					MOV EAX, DWORD PTR[sub_1009756C];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_1009756C], EAX;

					POP EAX;
					RETN;
	}
}

void __declspec(naked)v11_Eng_FOV_Hook() {
	static int init = 0x1;

	static int sub_100985B0 = 0x000985B0;

	_asm {
//					_emit 0xCC;
					CMP init, 0;
					JZ label_begin;
					MOV init, 0;
					CALL label_init;

	label_begin:
					ADD BYTE PTR[ESP], 0x1;
					//-------------------------------------------------------------------------
					PUSH EAX;									//
					MOV EAX, DWORD PTR[sub_100985B0];			// FMUL DWORD PTR[100985B0]
					FMUL DWORD PTR[EAX];						//
					POP EAX;									//
					//-------------------------------------------------------------------------
					FLD ST;
					FCOS;
					FXCH ST(1);
					FSIN;
					FDIVP ST(1), ST;
					FMUL DWORD PTR[ori_width_p_height];
					FLD DWORD PTR[cur_width_p_height];
					FXCH ST(1);
					FPATAN;
					RETN;

	label_init:
					PUSH EAX;

					MOV EAX, DWORD PTR[sub_100985B0];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_100985B0], EAX;

					POP EAX;
					RETN;
	}
}

void __declspec(naked)v12_Eng_FOV_Hook() {
	static int init = 0x1;

	static int sub_1009B5B0 = 0x0009B5B0;

	_asm {
//					_emit 0xCC;
					CMP init, 0;
					JZ label_begin;
					MOV init, 0;
					CALL label_init;

	label_begin:
					ADD BYTE PTR[ESP], 0x1;
					//-------------------------------------------------------------------------
					PUSH EAX;									//
					MOV EAX, DWORD PTR[sub_1009B5B0];			// FMUL DWORD PTR[1009B5B0]
					FMUL DWORD PTR[EAX];						//
					POP EAX;									//
					//-------------------------------------------------------------------------
					FLD ST;
					FCOS;
					FXCH ST(1);
					FSIN;
					FDIVP ST(1), ST;
					FMUL DWORD PTR[ori_width_p_height];
					FLD DWORD PTR[cur_width_p_height];
					FXCH ST(1);
					FPATAN;
					RETN;

	label_init:
					PUSH EAX;

					MOV EAX, DWORD PTR[sub_1009B5B0];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_1009B5B0], EAX;

					POP EAX;
					RETN;
	}
}

void __declspec(naked)v10_Eng_Map_Hook_Player_Marker_x() {
	_asm {
					FMUL DWORD PTR[map_player_marker_x];
					RETN;
	}
}

void __declspec(naked)v11_Eng_Map_Hook_Player_Marker_x() {
	_asm {
					FMUL DWORD PTR[map_player_marker_x];
					FXCH ST(1);
					RETN;
	}
}

void __declspec(naked)v10_Eng_Menu_Hook_01() {
	static int sub_0057D850 = 0x0057D850;

	_asm {
					MOV EAX, DWORD PTR[ESP + 0x8];
					SUB ESP, 0x8;
					PUSH ESI;
					MOV ESI, ECX;
					MOV DWORD PTR[ESI + 0x34], EAX;
					MOV EAX, DWORD PTR[ESP + 0x10];
					MOV DWORD PTR[ESI], 0x0063DBDC;
					MOV ECX, DWORD PTR[EAX];
					MOV DWORD PTR[ESI + 0x4], ECX;
					XOR ECX, ECX;
					MOV DWORD PTR[ESI + 0x8], ECX;
					MOV EDX, DWORD PTR[EAX + 0x18];
					MOV DWORD PTR[ESI + 0x0C], EDX;
					MOV_DL_BYTE_PTR DW(00,67,26,28);
					CMP DL, CL;
					JE SHORT label_005704FB;
					MOV EDX, DWORD PTR[EAX + 0x8];
					MOV DWORD PTR[ESI + 0x10], EDX;
					MOV EDX, DWORD PTR[EAX + 0x0C];
					MOV DWORD PTR[ESI + 0x14], EDX;
					MOV EDX, DWORD PTR[EAX + 0x10];
					MOV DWORD PTR[ESI + 0x18], EDX;
					MOV EDX, DWORD PTR[EAX + 0x14];
					MOV DWORD PTR[ESI + 0x1C], EDX;
					JMP SHORT label_0057052B;
	label_005704FB:
					FLD_DWORD_PTR DW(00,67,A4,C0);
					FMUL DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					CALL label_filter;						// Correct value
					//-------------------------------------------------------------------------
					FSTP DWORD PTR[ESI + 0x10];
					FLD_DWORD_PTR DW(00,67,A4,C4);
					FMUL DWORD PTR[EAX + 0x0C];
					FSTP DWORD PTR[ESI + 0x14];
					FLD_DWORD_PTR DW(00,67,A4,C0);
					FMUL DWORD PTR[EAX + 0x10];
					FSTP DWORD PTR[ESI + 0x18];
					FLD_DWORD_PTR DW(00,67,A4,C4);
					FMUL DWORD PTR[EAX + 0x14];
					FSTP DWORD PTR[ESI + 0x1C];
	label_0057052B:
					MOV EDX, DWORD PTR[EAX + 0x1C];
					MOV DWORD PTR[ESP + 0x8], ECX;
					MOV DWORD PTR[ESI + 0x20], EDX;
					MOV EDX, DWORD PTR[EAX + 0x20];
					MOV DWORD PTR[ESI + 0x24], EDX;
					MOV EAX, DWORD PTR[EAX + 0x20];
					MOV DWORD PTR[ESP + 0x4], EAX;
					MOV EAX, DWORD PTR[ESI + 0x0C];
					FILD QWORD PTR[ESP + 0x4];
					CMP EAX, ECX;
					MOV DWORD PTR[ESI + 0x2C], ECX;
					FSTP DWORD PTR[ESI + 0x28];
					JE SHORT label_0057056A;
					PUSH EAX;
					MOV ECX, 0x0067A57C;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_0057D850];				// CALL 0057D850
					//-------------------------------------------------------------------------
					MOV DWORD PTR[ESI + 0x30], EAX;
					MOV EAX, ESI;
					POP ESI;
					ADD ESP, 0x8;
					RETN 0x8;
	label_0057056A:
					MOV DWORD PTR[ESI + 0x30], ECX;
					MOV EAX, ESI;
					POP ESI;
					ADD ESP, 0x8;
					RETN 0x8;

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
					FADD DWORD PTR[menu_menu_x];
					RETN;
	}
}

void __declspec(naked)v11_Eng_Menu_Hook_01() {
	static int sub_0060F370 = 0x0060F370;
	static int sub_005E9650 = 0x005E9650;

	_asm {
					PUSH - 0x1;
					PUSH 0x0062172B;
					MOV EAX, DWORD PTR FS : [0x0];
					PUSH EAX;
					MOV DWORD PTR FS : [0x0], ESP;
					PUSH ECX;
					MOV AL, BYTE PTR[ESP + 0x14];
					PUSH ESI;
					PUSH EDI;
					MOV ESI, ECX;
					XOR EDI, EDI;
					MOV DWORD PTR[ESP + 0x8], ESI;
					MOV BYTE PTR[ESI + 0x30], AL;
					MOV DWORD PTR[ESI + 0x34], EDI;
					MOV DWORD PTR[ESI + 0x38], EDI;
					MOV DWORD PTR[ESI + 0x3C], EDI;
					MOV EAX, DWORD PTR[ESP + 0x1C];
					MOV DWORD PTR[ESI], 0x00624FF8;
					MOV DWORD PTR[ESP + 0x14], EDI;
					MOV ECX, DWORD PTR[EAX];
					MOV DWORD PTR[ESI + 0x4], ECX;
					MOV EDX, DWORD PTR[EAX + 0x1C];
					MOV DWORD PTR[ESI + 0x8], EDX;
					FLD_DWORD_PTR DW(00,6B,C7,B8);
					FMUL DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					CALL label_filter;						// Correct value
					//-------------------------------------------------------------------------
					FST DWORD PTR[ESI + 0x24];
					FSTP DWORD PTR[ESI + 0x14];
					FLD_DWORD_PTR DW(00,6B,C7,BC);
					FMUL DWORD PTR[EAX + 0x0C];
					FST DWORD PTR[ESI + 0x28];
					FSTP DWORD PTR[ESI + 0x18];
					FLD_DWORD_PTR DW(00,6B,C7,B8);
					FMUL DWORD PTR[EAX + 0x10];
					FSTP DWORD PTR[ESI + 0x1C];
					FLD_DWORD_PTR DW(00,6B,C7,BC);
					FMUL DWORD PTR[EAX + 0x14];
					FSTP DWORD PTR[ESI + 0x20];
					MOV EAX, DWORD PTR[EAX + 0x18];
					CMP EAX, EDI;
					MOV DWORD PTR[ESI + 0x0C], EAX;
					JE SHORT label_005E9479;
					PUSH EAX;
					MOV ECX, 0x006D7644;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_0060F370];				// CALL 0060F370
					//-------------------------------------------------------------------------
					MOV DWORD PTR[ESI + 0x10], EAX;
					JMP SHORT label_005E947C;
	label_005E9479:
					MOV DWORD PTR[ESI + 0x10], EDI;
	label_005E947C:
					MOV ECX, DWORD PTR[ESP + 0x20];
					CMP ECX, EDI;
					MOV DWORD PTR[ESI + 0x2C], ECX;
					JE SHORT label_005E948D;
					PUSH ESI;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_005E9650];				// CALL 005E9650
					//-------------------------------------------------------------------------
	label_005E948D:
					MOV ECX, DWORD PTR[ESP + 0x0C];
					MOV EAX, ESI;
					POP EDI;
					POP ESI;
					MOV DWORD PTR FS : [0x0], ECX;
					ADD ESP, 0x10;
					RETN 0x8;

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
					FADD DWORD PTR[menu_menu_x];
					RETN;
	}
}

void __declspec(naked)v12_Eng_Menu_Hook_01() {
	static int sub_0060FB40 = 0x0060FB40;
	static int sub_005E9E50 = 0x005E9E50;

	_asm {
					PUSH - 0x1;
					PUSH 0x00621EFB;
					MOV EAX, DWORD PTR FS : [0x0];
					PUSH EAX;
					MOV DWORD PTR FS : [0x0], ESP;
					PUSH ECX;
					MOV AL, BYTE PTR[ESP + 0x14];
					PUSH ESI;
					PUSH EDI;
					MOV ESI, ECX;
					XOR EDI, EDI;
					MOV DWORD PTR[ESP + 0x8], ESI;
					MOV BYTE PTR[ESI + 0x30], AL;
					MOV DWORD PTR[ESI + 0x34], EDI;
					MOV DWORD PTR[ESI + 0x38], EDI;
					MOV DWORD PTR[ESI + 0x3C], EDI;
					MOV EAX, DWORD PTR[ESP + 0x1C];
					MOV DWORD PTR[ESI], 0x00625FF8;
					MOV DWORD PTR[ESP + 0x14], EDI;
					MOV ECX, DWORD PTR[EAX];
					MOV DWORD PTR[ESI + 0x4], ECX;
					MOV EDX, DWORD PTR[EAX + 0x1C];
					MOV DWORD PTR[ESI + 0x8], EDX;
					FLD_DWORD_PTR DW(00,6B,D8,88);
					FMUL DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					CALL label_filter;						// Correct value
					//-------------------------------------------------------------------------
					FST DWORD PTR[ESI + 0x24];
					FSTP DWORD PTR[ESI + 0x14];
					FLD_DWORD_PTR DW(00,6B,D8,8C);
					FMUL DWORD PTR[EAX + 0x0C];
					FST DWORD PTR[ESI + 0x28];
					FSTP DWORD PTR[ESI + 0x18];
					FLD_DWORD_PTR DW(00,6B,D8,88);
					FMUL DWORD PTR[EAX + 0x10];
					FSTP DWORD PTR[ESI + 0x1C];
					FLD_DWORD_PTR DW(00,6B,D8,8C);
					FMUL DWORD PTR[EAX + 0x14];
					FSTP DWORD PTR[ESI + 0x20];
					MOV EAX, DWORD PTR[EAX + 0x18];
					CMP EAX, EDI;
					MOV DWORD PTR[ESI + 0x0C], EAX;
					JE SHORT label_005E9C79;
					PUSH EAX;
					MOV ECX, 0x006D8714;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_0060FB40];				// CALL 0060FB40
					//-------------------------------------------------------------------------
					MOV DWORD PTR[ESI + 0x10], EAX;
					JMP SHORT label_005E9C7C;
	label_005E9C79:
					MOV DWORD PTR[ESI + 0x10], EDI;
	label_005E9C7C:
					MOV ECX, DWORD PTR[ESP + 0x20];
					CMP ECX, EDI;
					MOV DWORD PTR[ESI + 0x2C], ECX;
					JE SHORT label_005E9C8D;
					PUSH ESI;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_005E9E50];				// CALL 005E9E50
					//-------------------------------------------------------------------------
	label_005E9C8D:
					MOV ECX, DWORD PTR[ESP + 0x0C];
					MOV EAX, ESI;
					POP EDI;
					POP ESI;
					MOV DWORD PTR FS : [0x0], ECX;
					ADD ESP, 0x10;
					RETN 0x8;

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
					FADD DWORD PTR[menu_menu_x];
					RETN;
	}
}

void __declspec(naked)v10_Eng_Menu_Hook_02() {
	static int sub_0057D850 = 0x0057D850;

	_asm {
					MOV EAX, DWORD PTR[ESP + 0x8];
					SUB ESP, 0x8;
					PUSH ESI;
					MOV ESI, ECX;
					MOV DWORD PTR[ESI + 0x34], EAX;
					MOV EAX, DWORD PTR[ESP + 0x10];
					MOV DWORD PTR[ESI], 0x0063DBDC;
					MOV ECX, DWORD PTR[EAX];
					MOV DWORD PTR[ESI + 0x4], ECX;
					XOR ECX, ECX;
					MOV DWORD PTR[ESI + 0x8], ECX;
					MOV EDX, DWORD PTR[EAX + 0x18];
					MOV DWORD PTR[ESI + 0x0C], EDX;
					MOV_DL_BYTE_PTR DW(00,67,26,28);
					CMP DL, CL;
					JE SHORT label_005704FB;
					MOV EDX, DWORD PTR[EAX + 0x8];
					MOV DWORD PTR[ESI + 0x10], EDX;
					MOV EDX, DWORD PTR[EAX + 0x0C];
					MOV DWORD PTR[ESI + 0x14], EDX;
					MOV EDX, DWORD PTR[EAX + 0x10];
					MOV DWORD PTR[ESI + 0x18], EDX;
					MOV EDX, DWORD PTR[EAX + 0x14];
					MOV DWORD PTR[ESI + 0x1C], EDX;
					JMP SHORT label_0057052B;
	label_005704FB:
					FLD_DWORD_PTR DW(00,67,A4,C0);
					FMUL DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					CALL label_filter;						// Correct value
					//-------------------------------------------------------------------------
					FSTP DWORD PTR[ESI + 0x10];
					FLD_DWORD_PTR DW(00,67,A4,C4);
					FMUL DWORD PTR[EAX + 0x0C];
					FSTP DWORD PTR[ESI + 0x14];
					FLD_DWORD_PTR DW(00,67,A4,C0);
					FMUL DWORD PTR[EAX + 0x10];
					FSTP DWORD PTR[ESI + 0x18];
					FLD_DWORD_PTR DW(00,67,A4,C4);
					FMUL DWORD PTR[EAX + 0x14];
					FSTP DWORD PTR[ESI + 0x1C];
	label_0057052B:
					MOV EDX, DWORD PTR[EAX + 0x1C];
					MOV DWORD PTR[ESP + 0x8], ECX;
					MOV DWORD PTR[ESI + 0x20], EDX;
					MOV EDX, DWORD PTR[EAX + 0x20];
					MOV DWORD PTR[ESI + 0x24], EDX;
					MOV EAX, DWORD PTR[EAX + 0x20];
					MOV DWORD PTR[ESP + 0x4], EAX;
					MOV EAX, DWORD PTR[ESI + 0x0C];
					FILD QWORD PTR[ESP + 0x4];
					CMP EAX, ECX;
					MOV DWORD PTR[ESI + 0x2C], ECX;
					FSTP DWORD PTR[ESI + 0x28];
					JE SHORT label_0057056A;
					PUSH EAX;
					MOV ECX, 0x0067A57C;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_0057D850];				// CALL 0057D850
					//-------------------------------------------------------------------------
					MOV DWORD PTR[ESI + 0x30], EAX;
					MOV EAX, ESI;
					POP ESI;
					ADD ESP, 0x8;
					RETN 0x8;
	label_0057056A:
					MOV DWORD PTR[ESI + 0x30], ECX;
					MOV EAX, ESI;
					POP ESI;
					ADD ESP, 0x8;
					RETN 0x8;

	label_filter:
					CMP EAX, 0x00678DB0;		// Load Screens
					JE label_add;
					CMP EAX, 0x00674C94;		// Carcyclopedia
					JE label_add;
					RETN;
	label_add:
					FADD DWORD PTR[menu_menu_x];
					RETN;
	}
}

void __declspec(naked)v11_Eng_Menu_Hook_02() {
	static int sub_0060F370 = 0x0060F370;
	static int sub_005E9650 = 0x005E9650;

	_asm {
					PUSH - 0x1;
					PUSH 0x0062172B;
					MOV EAX, DWORD PTR FS : [0x0];
					PUSH EAX;
					MOV DWORD PTR FS : [0x0], ESP;
					PUSH ECX;
					MOV AL, BYTE PTR[ESP + 0x14];
					PUSH ESI;
					PUSH EDI;
					MOV ESI, ECX;
					XOR EDI, EDI;
					MOV DWORD PTR[ESP + 0x8], ESI;
					MOV BYTE PTR[ESI + 0x30], AL;
					MOV DWORD PTR[ESI + 0x34], EDI;
					MOV DWORD PTR[ESI + 0x38], EDI;
					MOV DWORD PTR[ESI + 0x3C], EDI;
					MOV EAX, DWORD PTR[ESP + 0x1C];
					MOV DWORD PTR[ESI], 0x00624FF8;
					MOV DWORD PTR[ESP + 0x14], EDI;
					MOV ECX, DWORD PTR[EAX];
					MOV DWORD PTR[ESI + 0x4], ECX;
					MOV EDX, DWORD PTR[EAX + 0x1C];
					MOV DWORD PTR[ESI + 0x8], EDX;
					FLD_DWORD_PTR DW(00,6B,C7,B8);
					FMUL DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					CALL label_filter;						// Correct value
					//-------------------------------------------------------------------------
					FST DWORD PTR[ESI + 0x24];
					FSTP DWORD PTR[ESI + 0x14];
					FLD_DWORD_PTR DW(00,6B,C7,BC);
					FMUL DWORD PTR[EAX + 0x0C];
					FST DWORD PTR[ESI + 0x28];
					FSTP DWORD PTR[ESI + 0x18];
					FLD_DWORD_PTR DW(00,6B,C7,B8);
					FMUL DWORD PTR[EAX + 0x10];
					FSTP DWORD PTR[ESI + 0x1C];
					FLD_DWORD_PTR DW(00,6B,C7,BC);
					FMUL DWORD PTR[EAX + 0x14];
					FSTP DWORD PTR[ESI + 0x20];
					MOV EAX, DWORD PTR[EAX + 0x18];
					CMP EAX, EDI;
					MOV DWORD PTR[ESI + 0x0C], EAX;
					JE SHORT label_005E9479;
					PUSH EAX;
					MOV ECX, 0x006D7644;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_0060F370];				// CALL 0060F370
					//-------------------------------------------------------------------------
					MOV DWORD PTR[ESI + 0x10], EAX;
					JMP SHORT label_005E947C;
	label_005E9479:
					MOV DWORD PTR[ESI + 0x10], EDI;
	label_005E947C:
					MOV ECX, DWORD PTR[ESP + 0x20];
					CMP ECX, EDI;
					MOV DWORD PTR[ESI + 0x2C], ECX;
					JE SHORT label_005E948D;
					PUSH ESI;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_005E9650];				// CALL 005E9650
					//-------------------------------------------------------------------------
	label_005E948D:
					MOV ECX, DWORD PTR[ESP + 0x0C];
					MOV EAX, ESI;
					POP EDI;
					POP ESI;
					MOV DWORD PTR FS : [0x0], ECX;
					ADD ESP, 0x10;
					RETN 0x8;

	label_filter:
					CMP ECX, 0x1;				// Load Screens, Carcyclopedia
					JE label_add;
					CMP ECX, 0x2;				// Carcyclopedia - Show Mode
					JE label_add;
					CMP ECX, 0x3;				// Load Game
					JE label_add;
					RETN;
	label_add:
					FADD DWORD PTR[menu_menu_x];
					RETN;
	}
}

void __declspec(naked)v12_Eng_Menu_Hook_02() {
	static int sub_0060FB40 = 0x0060FB40;
	static int sub_005E9E50 = 0x005E9E50;

	_asm {
					PUSH - 0x1;
					PUSH 0x00621EFB;
					MOV EAX, DWORD PTR FS : [0x0];
					PUSH EAX;
					MOV DWORD PTR FS : [0x0], ESP;
					PUSH ECX;
					MOV AL, BYTE PTR[ESP + 0x14];
					PUSH ESI;
					PUSH EDI;
					MOV ESI, ECX;
					XOR EDI, EDI;
					MOV DWORD PTR[ESP + 0x8], ESI;
					MOV BYTE PTR[ESI + 0x30], AL;
					MOV DWORD PTR[ESI + 0x34], EDI;
					MOV DWORD PTR[ESI + 0x38], EDI;
					MOV DWORD PTR[ESI + 0x3C], EDI;
					MOV EAX, DWORD PTR[ESP + 0x1C];
					MOV DWORD PTR[ESI], 0x00625FF8;
					MOV DWORD PTR[ESP + 0x14], EDI;
					MOV ECX, DWORD PTR[EAX];
					MOV DWORD PTR[ESI + 0x4], ECX;
					MOV EDX, DWORD PTR[EAX + 0x1C];
					MOV DWORD PTR[ESI + 0x8], EDX;
					FLD_DWORD_PTR DW(00,6B,D8,88);
					FMUL DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					CALL label_filter;						// Correct value
					//-------------------------------------------------------------------------
					FST DWORD PTR[ESI + 0x24];
					FSTP DWORD PTR[ESI + 0x14];
					FLD_DWORD_PTR DW(00,6B,D8,8C);
					FMUL DWORD PTR[EAX + 0x0C];
					FST DWORD PTR[ESI + 0x28];
					FSTP DWORD PTR[ESI + 0x18];
					FLD_DWORD_PTR DW(00,6B,D8,88);
					FMUL DWORD PTR[EAX + 0x10];
					FSTP DWORD PTR[ESI + 0x1C];
					FLD_DWORD_PTR DW(00,6B,D8,8C);
					FMUL DWORD PTR[EAX + 0x14];
					FSTP DWORD PTR[ESI + 0x20];
					MOV EAX, DWORD PTR[EAX + 0x18];
					CMP EAX, EDI;
					MOV DWORD PTR[ESI + 0x0C], EAX;
					JE SHORT label_005E9C79;
					PUSH EAX;
					MOV ECX, 0x006D8714;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_0060FB40];				// CALL 0060FB40
					//-------------------------------------------------------------------------
					MOV DWORD PTR[ESI + 0x10], EAX;
					JMP SHORT label_005E9C7C;
	label_005E9C79:
					MOV DWORD PTR[ESI + 0x10], EDI;
	label_005E9C7C:
					MOV ECX, DWORD PTR[ESP + 0x20];
					CMP ECX, EDI;
					MOV DWORD PTR[ESI + 0x2C], ECX;
					JE SHORT label_005E9C8D;
					PUSH ESI;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_005E9E50];				// CALL 005E9E50
					//-------------------------------------------------------------------------
	label_005E9C8D:
					MOV ECX, DWORD PTR[ESP + 0x0C];
					MOV EAX, ESI;
					POP EDI;
					POP ESI;
					MOV DWORD PTR FS : [0x0], ECX;
					ADD ESP, 0x10;
					RETN 0x8;

	label_filter:
					CMP ECX, 0x1;				// Load Screens, Carcyclopedia
					JE label_add;
					CMP ECX, 0x2;				// Carcyclopedia - Show Mode
					JE label_add;
					CMP ECX, 0x3;				// Load Game
					JE label_add;
					RETN;
	label_add:
					FADD DWORD PTR[menu_menu_x];
					RETN;
	}
}

void __declspec(naked)v10_Eng_Menu_Hook_03() {
	static int sub_0057D850 = 0x0057D850;

	_asm {
					MOV EAX, DWORD PTR[ESP + 0x8];
					SUB ESP, 0x8;
					PUSH ESI;
					MOV ESI, ECX;
					MOV DWORD PTR[ESI + 0x34], EAX;
					MOV EAX, DWORD PTR[ESP + 0x10];
					MOV DWORD PTR[ESI], 0x0063DBDC;
					MOV ECX, DWORD PTR[EAX];
					MOV DWORD PTR[ESI + 0x4], ECX;
					XOR ECX, ECX;
					MOV DWORD PTR[ESI + 0x8], ECX;
					MOV EDX, DWORD PTR[EAX + 0x18];
					MOV DWORD PTR[ESI + 0x0C], EDX;
					MOV_DL_BYTE_PTR DW(00,67,26,28);
					CMP DL, CL;
					JE SHORT label_005704FB;
					MOV EDX, DWORD PTR[EAX + 0x8];
					MOV DWORD PTR[ESI + 0x10], EDX;
					MOV EDX, DWORD PTR[EAX + 0x0C];
					MOV DWORD PTR[ESI + 0x14], EDX;
					MOV EDX, DWORD PTR[EAX + 0x10];
					MOV DWORD PTR[ESI + 0x18], EDX;
					MOV EDX, DWORD PTR[EAX + 0x14];
					MOV DWORD PTR[ESI + 0x1C], EDX;
					JMP SHORT label_0057052B;
	label_005704FB:
					FLD_DWORD_PTR DW(00,67,A4,C0);
					FMUL DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					CALL label_filter;						// Correct value
					//-------------------------------------------------------------------------
					FSTP DWORD PTR[ESI + 0x10];
					FLD_DWORD_PTR DW(00,67,A4,C4);
					FMUL DWORD PTR[EAX + 0x0C];
					FSTP DWORD PTR[ESI + 0x14];
					FLD_DWORD_PTR DW(00,67,A4,C0);
					FMUL DWORD PTR[EAX + 0x10];
					FSTP DWORD PTR[ESI + 0x18];
					FLD_DWORD_PTR DW(00,67,A4,C4);
					FMUL DWORD PTR[EAX + 0x14];
					FSTP DWORD PTR[ESI + 0x1C];
	label_0057052B:
					MOV EDX, DWORD PTR[EAX + 0x1C];
					MOV DWORD PTR[ESP + 0x8], ECX;
					MOV DWORD PTR[ESI + 0x20], EDX;
					MOV EDX, DWORD PTR[EAX + 0x20];
					MOV DWORD PTR[ESI + 0x24], EDX;
					MOV EAX, DWORD PTR[EAX + 0x20];
					MOV DWORD PTR[ESP + 0x4], EAX;
					MOV EAX, DWORD PTR[ESI + 0x0C];
					FILD QWORD PTR[ESP + 0x4];
					CMP EAX, ECX;
					MOV DWORD PTR[ESI + 0x2C], ECX;
					FSTP DWORD PTR[ESI + 0x28];
					JE SHORT label_0057056A;
					PUSH EAX;
					MOV ECX, 0x0067A57C;
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_0057D850];				// CALL 0057D850
					//-------------------------------------------------------------------------
					MOV DWORD PTR[ESI + 0x30], EAX;
					MOV EAX, ESI;
					POP ESI;
					ADD ESP, 0x8;
					RETN 0x8;
	label_0057056A:
					MOV DWORD PTR[ESI + 0x30], ECX;
					MOV EAX, ESI;
					POP ESI;
					ADD ESP, 0x8;
					RETN 0x8;

	label_filter:
					CMP EAX, 0x00677658;		// "New cars have been added to the garage"
					JE label_add;
					CMP EAX, 0x00676F98;		// Racing (in Race Mission)
					JE label_add;
					CMP EAX, 0x00674DB4;		// Carcyclopedia (Cars List)
					JE label_add;
					RETN;
	label_add:
					FADD DWORD PTR[menu_menu_x];
					RETN;
	}
}

void __declspec(naked)v10_Eng_Movie_Hook() {
	static int init = 0x1;

	static int sub_10070240 = 0x00070240;
	static int sub_10097198 = 0x00097198;
	static int sub_101C13A8 = 0x001C13A8;
	static int sub_101C13F4 = 0x001C13F4;
	static int sub_101C1588 = 0x001C1588;
	_asm {
//					_emit 0xCC;
					CMP init, 0;
					JZ label_begin;
					MOV init, 0;
					CALL label_init;

	label_begin:
					//-------------------------------------------------------------------------
					MOV EAX, DWORD PTR[sub_101C1588];			//
					MOV AL, BYTE PTR[EAX];						// MOV AL, BYTE PTR[101C1588]
					//-------------------------------------------------------------------------
					MOV ECX, DWORD PTR[sub_101C13A8];			//
					MOV ECX, DWORD PTR[ECX];					// MOV ECX, DWORD PTR[101C13A8]
					//-------------------------------------------------------------------------
					SUB ESP, 0x10;
					TEST AL, AL;
					JS label_10070448;
					//-------------------------------------------------------------------------
					MOV ECX, DWORD PTR[sub_101C13F4];			//
					MOV ECX, DWORD PTR[ECX];					// MOV ECX, DWORD PTR[101C13F4]
					//-------------------------------------------------------------------------

	label_10070448:
					MOV EAX, DWORD PTR[ESP + 0x18];
					TEST EAX, EAX;
					JNZ label_1007045E;
					LEA EAX, [ESP];
					PUSH EAX;
					PUSH ECX;
					//-------------------------------------------------------------------------
					PUSH EAX;									//
					MOV EAX, DWORD PTR[sub_10097198];			//
					MOV EAX, DWORD PTR[EAX];					// CALL DWORD PTR[10097198]
					MOV DWORD PTR[temp], EAX;					//
					POP EAX;									//
					CALL DWORD PTR[temp];						//
					//-------------------------------------------------------------------------
					JMP label_10070479;
	label_1007045E:
					MOV ECX, DWORD PTR[EAX];
					MOV EDX, DWORD PTR[EAX + 0x4];
					MOV DWORD PTR[ESP], ECX;
					MOV ECX, DWORD PTR[EAX + 0x8];
					MOV DWORD PTR[ESP + 0x4], EDX;
					MOV EDX, DWORD PTR[EAX + 0x0C];
					MOV DWORD PTR[ESP + 0x8], ECX;
					MOV DWORD PTR[ESP + 0x0C], EDX;
	label_10070479:
					MOV ECX, DWORD PTR[ESP + 0x14];
//					MOV EDX,DWORD PTR [ESP];
					LEA EAX, [ECX + 0x28];
//					MOV DWORD PTR [EAX],EDX;
//					MOV EDX,DWORD PTR [ESP+0x4];
//					MOV DWORD PTR [EAX+0x4],EDX;
//					MOV EDX,DWORD PTR [ESP+0x8];
//					MOV DWORD PTR [EAX+0x8],EDX;
//					MOV EDX,DWORD PTR [ESP+0x0C];
//					MOV DWORD PTR [EAX+0x0C],EDX;
					//-------------------------------------------------------------------------
					CMP movie_patch, 1;
					JA label_fix2;
					CALL label_movie_fix1;
					JMP label_end_fix;
	label_fix2:
					CALL label_movie_fix2;
	label_end_fix:
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_10070240];				// CALL 10070240
					//-------------------------------------------------------------------------
					XOR EAX, EAX;
					ADD ESP, 0x10;
					RETN 0x8;

	label_movie_fix1:
					//-------------------------------------------------------------------------
					FLD DWORD PTR[cur_game_width];				// Левая сторона видео
					FILD DWORD PTR[ESP + 0xC];
					FDIV DWORD PTR[movie_aspect];
					FSUBP ST(1), ST;
					FMUL DWORD PTR[half];
					FISTP DWORD PTR[EAX];
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0x8];				// Верхняя сторона видео
					MOV DWORD PTR[EAX + 0x4], EDX;
					//-------------------------------------------------------------------------
					FILD DWORD PTR[ESP + 0xC];					// Ширина видео
					FDIV DWORD PTR[movie_aspect];
					FISTP DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0x10];				// Высота видео
					MOV DWORD PTR[EAX + 0x0C], EDX;
					//-------------------------------------------------------------------------
					RETN;

	label_movie_fix2:
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0x4];				// Левая сторона видео
					MOV DWORD PTR[EAX], EDX;
					//-------------------------------------------------------------------------
					FLD DWORD PTR[cur_game_height];				// Верхняя сторона видео
					FILD DWORD PTR[ESP + 0x10];
					FMUL DWORD PTR[movie_aspect];
					FSUBP ST(1), ST;
					FMUL DWORD PTR[half];
					FISTP DWORD PTR[EAX + 0x4];
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0xC];				// Ширина видео
					MOV DWORD PTR[EAX + 0xC], EDX;
					//-------------------------------------------------------------------------
					FILD DWORD PTR[ESP + 0x10];					// Высота видео
					FMUL DWORD PTR[movie_aspect];
					FISTP DWORD PTR[EAX + 0x0C];
					//-------------------------------------------------------------------------
					RETN;

	label_init:
					PUSH EAX;

					MOV EAX, DWORD PTR[sub_10070240];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_10070240], EAX;
					MOV EAX, DWORD PTR[sub_10097198];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_10097198], EAX;
					MOV EAX, DWORD PTR[sub_101C13A8];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_101C13A8], EAX;
					MOV EAX, DWORD PTR[sub_101C13F4];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_101C13F4], EAX;
					MOV EAX, DWORD PTR[sub_101C1588];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_101C1588], EAX;

					POP EAX;
					RETN;
	}
}

void __declspec(naked)v11_Eng_Movie_Hook() {
	static int init = 0x1;

	static int sub_10071270 = 0x00071270;
	static int sub_100981B0 = 0x000981B0;
	static int sub_101C2508 = 0x001C2508;
	static int sub_101C2558 = 0x001C2558;
	static int sub_101C26E8 = 0x001C26E8;
	_asm {
//					_emit 0xCC;
					CMP init, 0;
					JZ label_begin;
					MOV init, 0;
					CALL label_init;

	label_begin:
					//-------------------------------------------------------------------------
					MOV EAX, DWORD PTR[sub_101C26E8];			//
					MOV AL, BYTE PTR[EAX];						// MOV AL,BYTE PTR [101C26E8]
					//-------------------------------------------------------------------------
					MOV ECX, DWORD PTR[sub_101C2508];			//
					MOV ECX, DWORD PTR[ECX];					// MOV ECX, DWORD PTR[101C2508]
					//-------------------------------------------------------------------------
					SUB ESP, 0x10;
					TEST AL, AL;
					JS label_100714A8;
					//-------------------------------------------------------------------------
					MOV ECX, DWORD PTR[sub_101C2558];			//
					MOV ECX, DWORD PTR[ECX];					// MOV ECX, DWORD PTR[101C2558]
					//-------------------------------------------------------------------------
	label_100714A8:
					MOV EAX, DWORD PTR[ESP + 0x18];
					XOR EDX, EDX;
					CMP EAX, EDX;
					MOV DWORD PTR[ESP], EDX;
					MOV DWORD PTR[ESP + 0x4], EDX;
					MOV DWORD PTR[ESP + 0x8], 0x280;
					MOV DWORD PTR[ESP + 0x0C], 0x1E0;
					JNE label_100714D8;
					LEA EAX, [ESP];
					PUSH EAX;
					PUSH ECX;
					//-------------------------------------------------------------------------
					PUSH EAX;									//
					MOV EAX, DWORD PTR[sub_100981B0];			//
					MOV EAX, DWORD PTR[EAX];					// CALL DWORD PTR[100981B0]
					MOV DWORD PTR[temp], EAX;					//
					POP EAX;									//
					CALL DWORD PTR[temp];						//
					//-------------------------------------------------------------------------
					JMP label_100714F3;
	label_100714D8:
					MOV ECX, DWORD PTR[EAX];
					MOV EDX, DWORD PTR[EAX + 0x4];
					MOV DWORD PTR[ESP], ECX;
					MOV ECX, DWORD PTR[EAX + 0x8];
					MOV DWORD PTR[ESP + 0x4], EDX;
					MOV EDX, DWORD PTR[EAX + 0x0C];
					MOV DWORD PTR[ESP + 0x8], ECX;
					MOV DWORD PTR[ESP + 0x0C], EDX;
	label_100714F3:
					MOV ECX, DWORD PTR[ESP + 0x14];
//					MOV EDX, DWORD PTR[ESP];
					LEA EAX, [ECX + 0x28];
//					MOV DWORD PTR[EAX], EDX;
//					MOV EDX, DWORD PTR[ESP + 0x4];
//					MOV DWORD PTR[EAX + 0x4], EDX;
//					MOV EDX, DWORD PTR[ESP + 0x8];
//					MOV DWORD PTR[EAX + 0x8], EDX;
//					MOV EDX, DWORD PTR[ESP + 0x0C];
//					MOV DWORD PTR[EAX + 0x0C], EDX;
					//-------------------------------------------------------------------------
					CMP movie_patch, 1;
					JA label_fix2;
					CALL label_movie_fix1;
					JMP label_end_fix;
	label_fix2:
					CALL label_movie_fix2;
	label_end_fix:
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_10071270];				// CALL 10071270
					//-------------------------------------------------------------------------
					XOR EAX, EAX;
					ADD ESP, 0x10;
					RETN 0x8;

	label_movie_fix1:
					//-------------------------------------------------------------------------
					FLD DWORD PTR[cur_game_width];				// Левая сторона видео
					FILD DWORD PTR[ESP + 0xC];
					FDIV DWORD PTR[movie_aspect];
					FSUBP ST(1), ST;
					FMUL DWORD PTR[half];
					FISTP DWORD PTR[EAX];
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0x8];				// Верхняя сторона видео
					MOV DWORD PTR[EAX + 0x4], EDX;
					//-------------------------------------------------------------------------
					FILD DWORD PTR[ESP + 0xC];					// Ширина видео
					FDIV DWORD PTR[movie_aspect];
					FISTP DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0x10];				// Высота видео
					MOV DWORD PTR[EAX + 0x0C], EDX;
					//-------------------------------------------------------------------------
					RETN;

	label_movie_fix2:
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0x4];				// Левая сторона видео
					MOV DWORD PTR[EAX], EDX;
					//-------------------------------------------------------------------------
					FLD DWORD PTR[cur_game_height];				// Верхняя сторона видео
					FILD DWORD PTR[ESP + 0x10];
					FMUL DWORD PTR[movie_aspect];
					FSUBP ST(1), ST;
					FMUL DWORD PTR[half];
					FISTP DWORD PTR[EAX + 0x4];
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0xC];				// Ширина видео
					MOV DWORD PTR[EAX + 0xC], EDX;
					//-------------------------------------------------------------------------
					FILD DWORD PTR[ESP + 0x10];					// Высота видео
					FMUL DWORD PTR[movie_aspect];
					FISTP DWORD PTR[EAX + 0x0C];
					//-------------------------------------------------------------------------
					RETN;

	label_init:
					PUSH EAX;

					MOV EAX, DWORD PTR[sub_10071270];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_10071270], EAX;
					MOV EAX, DWORD PTR[sub_100981B0];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_100981B0], EAX;
					MOV EAX, DWORD PTR[sub_101C2508];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_101C2508], EAX;
					MOV EAX, DWORD PTR[sub_101C2558];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_101C2558], EAX;
					MOV EAX, DWORD PTR[sub_101C26E8];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_101C26E8], EAX;

					POP EAX;
					RETN;
	}
}

void __declspec(naked)v12_Eng_Movie_Hook() {
	static int init = 0x1;

	static int sub_10074380 = 0x00074380;
	static int sub_1009B1B0 = 0x0009B1B0;
	static int sub_101C5408 = 0x001C5408;
	static int sub_101C5458 = 0x001C5458;
	static int sub_101C55E8 = 0x001C55E8;
	_asm {
//					_emit 0xCC;
					CMP init, 0;
					JZ label_begin;
					MOV init, 0;
					CALL label_init;

	label_begin:
					//-------------------------------------------------------------------------
					MOV EAX, DWORD PTR[sub_101C55E8];			//
					MOV AL, BYTE PTR[EAX];						// MOV AL, BYTE PTR[101C55E8]
					//-------------------------------------------------------------------------
					MOV ECX, DWORD PTR[sub_101C5408];			//
					MOV ECX, DWORD PTR[ECX];					// MOV ECX, DWORD PTR[101C5408]
					//-------------------------------------------------------------------------
					SUB ESP, 0x10;
					TEST AL, AL;
					JS label_100745B8;
					//-------------------------------------------------------------------------
					MOV ECX, DWORD PTR[sub_101C5458];			//
					MOV ECX, DWORD PTR[ECX];					// MOV ECX, DWORD PTR[101C5458]
					//-------------------------------------------------------------------------
	label_100745B8:
					MOV EAX, DWORD PTR[ESP + 0x18];
					XOR EDX, EDX;
					CMP EAX, EDX;
					MOV DWORD PTR[ESP], EDX;
					MOV DWORD PTR[ESP + 0x4], EDX;
					MOV DWORD PTR[ESP + 0x8], 0x280;
					MOV DWORD PTR[ESP + 0x0C], 0x1E0;
					JNE label_100745E8;
					LEA EAX, [ESP];
					PUSH EAX;
					PUSH ECX;
					//-------------------------------------------------------------------------
					PUSH EAX;									//
					MOV EAX, DWORD PTR[sub_1009B1B0];			//
					MOV EAX, DWORD PTR[EAX];					// CALL DWORD PTR[1009B1B0]
					MOV DWORD PTR[temp], EAX;					//
					POP EAX;									//
					CALL DWORD PTR[temp];						//
					//-------------------------------------------------------------------------
					JMP label_10074603;
	label_100745E8:
					MOV ECX, DWORD PTR[EAX];
					MOV EDX, DWORD PTR[EAX + 0x4];
					MOV DWORD PTR[ESP], ECX;
					MOV ECX, DWORD PTR[EAX + 0x8];
					MOV DWORD PTR[ESP + 0x4], EDX;
					MOV EDX, DWORD PTR[EAX + 0x0C];
					MOV DWORD PTR[ESP + 0x8], ECX;
					MOV DWORD PTR[ESP + 0x0C], EDX;
	label_10074603:
					MOV ECX, DWORD PTR[ESP + 0x14];
//					MOV EDX, DWORD PTR[ESP];
					LEA EAX, [ECX + 0x2C];
//					MOV DWORD PTR[EAX], EDX;
//					MOV EDX, DWORD PTR[ESP + 0x4];
//					MOV DWORD PTR[EAX + 0x4], EDX;
//					MOV EDX, DWORD PTR[ESP + 0x8];
//					MOV DWORD PTR[EAX + 0x8], EDX;
//					MOV EDX, DWORD PTR[ESP + 0x0C];
//					MOV DWORD PTR[EAX + 0x0C], EDX;
					//-------------------------------------------------------------------------
					CMP movie_patch, 1;
					JA label_fix2;
					CALL label_movie_fix1;
					JMP label_end_fix;
	label_fix2:
					CALL label_movie_fix2;
	label_end_fix:
					//-------------------------------------------------------------------------
					CALL DWORD PTR[sub_10074380];				// CALL 10074380
					//-------------------------------------------------------------------------
					XOR EAX, EAX;
					ADD ESP, 0x10;
					RETN 0x8;

	label_movie_fix1:
					//-------------------------------------------------------------------------
					FLD DWORD PTR[cur_game_width];				// Левая сторона видео
					FILD DWORD PTR[ESP + 0xC];
					FDIV DWORD PTR[movie_aspect];
					FSUBP ST(1), ST;
					FMUL DWORD PTR[half];
					FISTP DWORD PTR[EAX];
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0x8];				// Верхняя сторона видео
					MOV DWORD PTR[EAX + 0x4], EDX;
					//-------------------------------------------------------------------------
					FILD DWORD PTR[ESP + 0xC];					// Ширина видео
					FDIV DWORD PTR[movie_aspect];
					FISTP DWORD PTR[EAX + 0x8];
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0x10];				// Высота видео
					MOV DWORD PTR[EAX + 0x0C], EDX;
					//-------------------------------------------------------------------------
					RETN;

	label_movie_fix2:
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0x4];				// Левая сторона видео
					MOV DWORD PTR[EAX], EDX;
					//-------------------------------------------------------------------------
					FLD DWORD PTR[cur_game_height];				// Верхняя сторона видео
					FILD DWORD PTR[ESP + 0x10];
					FMUL DWORD PTR[movie_aspect];
					FSUBP ST(1), ST;
					FMUL DWORD PTR[half];
					FISTP DWORD PTR[EAX + 0x4];
					//-------------------------------------------------------------------------
					MOV EDX, DWORD PTR[ESP + 0xC];				// Ширина видео
					MOV DWORD PTR[EAX + 0xC], EDX;
					//-------------------------------------------------------------------------
					FILD DWORD PTR[ESP + 0x10];					// Высота видео
					FMUL DWORD PTR[movie_aspect];
					FISTP DWORD PTR[EAX + 0x0C];
					//-------------------------------------------------------------------------
					RETN;

	label_init:
					PUSH EAX;

					MOV EAX, DWORD PTR[sub_10074380];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_10074380], EAX;
					MOV EAX, DWORD PTR[sub_1009B1B0];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_1009B1B0], EAX;
					MOV EAX, DWORD PTR[sub_101C5408];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_101C5408], EAX;
					MOV EAX, DWORD PTR[sub_101C5458];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_101C5458], EAX;
					MOV EAX, DWORD PTR[sub_101C55E8];
					ADD EAX, DWORD PTR[base_LS3DF];
					MOV DWORD PTR[sub_101C55E8], EAX;

					POP EAX;
					RETN;
	}
}


float __cdecl Draw_Distance_Hook_01(float dist) {
	if ((dist > draw_dist_min) && (dist < draw_dist_max)) {
		dist = draw_dist_value;
	}
	return dist;
}

void __declspec(naked)v10_Eng_Draw_Distance_Hook() {

	_asm {
					ADD BYTE PTR[ESP], 0x2;
					PUSHAD;
					PUSHFD;

					PUSH DWORD PTR[ESP + 0x0DC];
					CALL Draw_Distance_Hook_01;
					ADD ESP, 0x4;
					FSTP DWORD PTR[ESP + 0x0DC];

					POPFD;
					POPAD;
					MOV ECX, DWORD PTR[ESP + 0x0B8];
					RETN;
	}
}


void __declspec(naked)v10_Eng_Ru_Credits_Hook() {
	static int sub_00560550 = 0x00560550;

	_asm {
					MOV ECX, DWORD PTR[ESP + 0x0C];
					MOV EDX, DWORD PTR[ESI + 0x8];
					PUSH 0x0;
					PUSH 0x1;
					PUSH 0x2;
					PUSH -0x1;
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
					CALL DWORD PTR[sub_00560550];
					POP ESI;
					POP ECX;
					RETN 0x4;
	}
}

void __declspec(naked)v11_Eng_Ru_Credits_Hook() {
	static int sub_0060E0A0 = 0x0060E0A0;

	_asm {
					MOV ECX, DWORD PTR[ESP + 0x0C];
					MOV EDX, DWORD PTR[ESI + 0x8];
					PUSH 0x0;
					PUSH 0x1;
					PUSH 0x2;
					PUSH -0x1;
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
					CALL DWORD PTR[sub_0060E0A0];
					POP ESI;
					POP ECX;
					RETN 0x4;
	}
}


void ReadINI(char* name) {
	CIniReader iniReader(name);
	fov_patch = iniReader.ReadInteger("WideScreen", "FOV Hor+", 1);
	hud_patch = iniReader.ReadInteger("WideScreen", "Fix HUD", 2);
	borders_patch = iniReader.ReadInteger("WideScreen", "Fix Borders", 0);
	map_patch = iniReader.ReadInteger("WideScreen", "Fix Map", 2);
	text_patch = iniReader.ReadInteger("WideScreen", "Fix Text", 1);
	menu_patch = iniReader.ReadInteger("WideScreen", "Fix Menu", 0);
	movie_patch = iniReader.ReadInteger("WideScreen", "Fix Movie", 2);

	draw_dist_patch = iniReader.ReadInteger("Draw Distance", "Change Distance", 1);
	draw_dist_value = iniReader.ReadFloat("Draw Distance", "New Distance", 600.0f);
	draw_dist_min = iniReader.ReadFloat("Draw Distance", "Skip Min Distance", 20.0f);
	draw_dist_max = iniReader.ReadFloat("Draw Distance", "Skip Max Distance", 450.0f);

	write_log = iniReader.ReadInteger("Other", "Write Log", 0);
	fps_limit = iniReader.ReadInteger("Other", "FPS Limit", 0);
	ru_credits = iniReader.ReadInteger("Other", "Ru Credits Fix", 0);
}

void ClearLog() {
	if (write_log) {
		fopen_s(&LogFile, "mafia_widescreen_fix.log", "w");
		fclose(LogFile);
	}
}

void WriteLog(const char* str, ...) {
	if (write_log) {
		va_list args;
		va_start(args, str);
		fopen_s(&LogFile, "mafia_widescreen_fix.log", "a");
		vfprintf(LogFile, str, args);
		fclose(LogFile);
	}
}
