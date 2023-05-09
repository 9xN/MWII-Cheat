/*
	idol#0001 | ©️LUNARWARE
	This shit is an utter mess anyways.
*/

#pragma region Includes&Variables
#include "Offsets.hpp"
#include "Functions.h"
#include "Overlay.h"
#include "driver.h"
#include <iostream>
#include <vector>
#include "globals.hh"
#include "xorstr.h"
#include <fstream>
#include <regex>
#include "auth.hpp"
#include <string>
#include "skStr.h"
#include "Theme.h"
#include "Font.h"
#include <string>
#include <direct.h>
#include <filesystem>
std::string tm_to_readable_time(tm ctx);
static std::time_t string_to_timet(std::string timestamp);
static std::tm timet_to_tm(time_t timestamp);
using namespace std;
using namespace KeyAuth;
#pragma comment(lib, "urlmon.lib")
LPCSTR TargetProcess = "cod.exe";
bool ShowMenu = true;
bool ImGui_Initialised = false;
bool showconsole = true;
const char* PresetArray[]{ "Legit", "Semi-Legit", "Rage" };
int selected_config;
ImColor crosshaircolor = ImColor(255.f / 255.f, 0.f, 255.f);
ImColor FovColor = ImColor(255.0f / 255, 255.f, 255.0f);
ImColor ESPColor = ImColor(255.0f / 255, 255.f, 255.0f);
ImU32 green = IM_COL32(0, 255, 0, 255);
ImU32 red = IM_COL32(255, 0, 0, 255);
RGBA colGreen = { 0,255,0,255 };
RGBA colRed = { 255,0,0,255 };
RGBA colWhite = { 255, 255, 255, 255 };
#pragma endregion

/*
	Works Fine Without Keyauth stuff, Just thought i'd add it to make it easier for ppl.
*/
std::string sslPin = "ssl pin key (optional)";
std::string name = "mw2esp";
std::string ownerid = "Q5JP68vffp";
std::string secret = "7af5de1082fff766d8d1804914c5bec60f5ade5c01e7c1c9b638f9cda4309a7f";
std::string version = "1.0";
std::string url = "https://keyauth.win/api/1.2/";
api KeyAuthApp(name, ownerid, secret, version, url, sslPin);


namespace Process {
	DWORD ID;
	HANDLE Handle;
	HWND Hwnd;
	WNDPROC WndProc;
	int WindowWidth;
	int WindowHeight;
	int WindowLeft;
	int WindowRight;
	int WindowTop;
	int WindowBottom;
	LPCSTR Title;
	LPCSTR ClassName;
	LPCSTR Path;
}
namespace OverlayWindow {
	WNDCLASSEX WindowClass;
	HWND Hwnd;
	LPCSTR Name;
}
namespace DirectX9Interface {
	IDirect3D9Ex* Direct3D9 = NULL;
	IDirect3DDevice9Ex* pDevice = NULL;
	D3DPRESENT_PARAMETERS pParams = { NULL };
	MARGINS Margin = { -1 };
	MSG Message = { NULL };
}
DWORD GetProcessId2(std::string ProcessName)
{
	HANDLE hsnap;
	PROCESSENTRY32 pt;
	DWORD PiD;
	hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pt.dwSize = sizeof(PROCESSENTRY32);
	do {
		if (!strcmp(pt.szExeFile, ProcessName.c_str())) {
			CloseHandle(hsnap);
			PiD = pt.th32ProcessID;
			return PiD;
			if (PiD != NULL) {
				return 0;
			}
		}
	} while (Process32Next(hsnap, &pt));
	return 1;
}
BOOL IsProcessRunning(DWORD pid)
{
	HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
	DWORD ret = WaitForSingleObject(process, 0);
	CloseHandle(process);
	return ret == WAIT_TIMEOUT;
}
float DrawOutlinedText(ImFont* pFont, const ImVec2& pos, float size, ImU32 color, bool center, const char* text, ...)
{
	va_list(args);
	va_start(args, text);

	CHAR wbuffer[256] = { };
	vsprintf_s(wbuffer, text, args);

	va_end(args);

	auto DrawList = ImGui::GetForegroundDrawList();

	std::stringstream stream(text);
	std::string line;

	float y = 0.0f;
	int i = 0;

	while (std::getline(stream, line))
	{
		ImVec2 textSize = pFont->CalcTextSizeA(size, FLT_MAX, 0.0f, wbuffer);

		if (center)
		{
			DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), wbuffer);
			DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), wbuffer);
			DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), wbuffer);
			DrawList->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), wbuffer);

			DrawList->AddText(pFont, size, ImVec2(pos.x - textSize.x / 2.0f, pos.y + textSize.y * i), ImGui::GetColorU32(color), wbuffer);
		}
		else
		{
			DrawList->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), wbuffer);
			DrawList->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), wbuffer);
			DrawList->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), wbuffer);
			DrawList->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), wbuffer);

			DrawList->AddText(pFont, size, ImVec2(pos.x, pos.y + textSize.y * i), ImGui::GetColorU32(color), wbuffer);
		}

		y = pos.y + textSize.y * (i + 1);
		i++;
	}
	return y;
}

/*
	My poor attempt at distance cache for aimbot, couldn't figure it out ngl.
*/
static vector<float> distances = { };
static float bestdist;


/* ***IF YOU'RE GOING TO USE THIS ON STEAM, REPLACE ALL OF THE BNET DECRYPTIONS TO THE STEAM DECRYPTIONS. SEE "sdk.cpp" TO UNDERSTAND WHAT I MEAN*** */
void Render() {
	if (GetAsyncKeyState(VK_INSERT) & 1) ShowMenu = !ShowMenu;
	ImVec4* colors = ImGui::GetStyle().Colors;
	Theme2();
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::PushFont(Custom_Font);
	if (ShowMenu == true) {
		//	ImGui::GetIO().MouseDrawCursor = ShowMenu;
		static int tabb = 0;
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowSize(ImVec2(465, 450));
		// ImGui::SetNextWindowPos(ImVec2(200, 200), ImGuiCond_Once);
		ImGui::Begin("RK SERVICES", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
		// Tabs
		if (ImGui::Button("Visual", ImVec2(135, 25)))
		{
			tabb = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button("AIM", ImVec2(135, 25)))
		{
			tabb = 1;
		}
		ImGui::SameLine();
		if (ImGui::Button("PRESETS", ImVec2(135, 25)))
		{
			tabb = 2;
		}
		// End Tabs
		ImGui::Separator();
		// Tab Logic
		if (tabb == 0) {
			ImGui::Checkbox(xorstr_("BONE ESP"), &vars::boneesp);
			ImGui::Checkbox(xorstr_("BOX ESP"), &vars::boxes);
			ImGui::Checkbox(xorstr_("TEXT ESP"), &vars::esptext);
			// LINES
			ImGui::Checkbox(xorstr_("Line ESP"), &vars::drawlines);
			ImGui::SliderInt(xorstr_("Line Style"), &vars::linetype, 1, 2);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(xorstr_("1 = Bottom Screen\n2 = Center Screen"));
			ImGui::SliderFloat(xorstr_("ESP DISTANCE"), &vars::maxespdist, 10, 1000.0f);
			ImGui::ColorEdit3("ESP Color", (float*)&ESPColor, ImGuiColorEditFlags_NoDragDrop);
		}
		else if (tabb == 1) {
			ImGui::Checkbox(xorstr_("No Recoil (buggy)"), &vars::norecoil);
			ImGui::Checkbox(xorstr_("Crosshair"), &vars::chrosshair);
			ImGui::Checkbox(xorstr_("Aimbot"), &vars::aim);
			ImGui::Checkbox(xorstr_("Aimbot FOV Circle"), &vars::DrawCircleFOV);
			ImGui::SliderFloat(xorstr_("AIM Smooth"), &vars::smooth, 1, 20.0f);
			ImGui::SliderFloat(xorstr_("AIM FOV"), &vars::fov, 1, 360.0f);
			ImGui::Combo("Aim Bone", &vars::boneselected, vars::bones, IM_ARRAYSIZE(vars::bones));
		}
		else if (tabb == 2) {
			if (ImGui::Combo("##list", &selected_config, PresetArray, IM_ARRAYSIZE(PresetArray), 3)) {
				switch (selected_config)
				{
					case 0:	// Legit
						vars::boneesp = true;
						vars::esptext = true;
						vars::norecoil = true;
						vars::aim = true;
						vars::DrawCircleFOV = true;
						vars::smooth = 2.5f;
						vars::fov = 45.0f;
						vars::boneselected = 1;
						vars::maxespdist = 150.0f;
						break;
					case 1: // Semi Legit
						vars::boneesp = true;
						vars::esptext = true;
						vars::norecoil = true;
						vars::aim = true;
						vars::DrawCircleFOV = true;
						vars::smooth = 1.75f;
						vars::fov = 60.0f;
						vars::boneselected = 0;
						vars::maxespdist = 200.0f;
						break;
					case 2: // Rage (kinda)
						vars::boneesp = true;
						vars::esptext = true;
						vars::norecoil = true;
						vars::aim = true;
						vars::DrawCircleFOV = true;
						vars::smooth = 1.25f;
						vars::fov = 70.0f;
						vars::boneselected = 0;
						vars::maxespdist = 240.0f;
						break;
				}
			}
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			sdk::player_t local(decryption::get_client_info_base() + (sdk::local_index() * offsets::player::size));
			auto local_pos = local.get_pos();
			auto local_team = local.team_id();
			auto local_health = local.GetNameEntry(0).health;
			ImGui::Text(xorstr_("[DEBUG] CLIENT INFO: 0x%X"), sdk::client_info);
			ImGui::Text(xorstr_("[DEBUG] CLIENT Base: 0x%X"), sdk::client_info_base);
			ImGui::Text(xorstr_("[DEBUG] Refdef: 0x%X"), sdk::ref_def);
			ImGui::Text(xorstr_("[DEBUG] Refdef Resolution: %ix%i"), sdk::ref_def.width, sdk::ref_def.height);
			ImGui::Text(xorstr_("[DEBUG] Local Player: 0x%X"), local);
			ImGui::Text(xorstr_("[DEBUG] Local Index: %d"), sdk::local_index());
			ImGui::Text(xorstr_("[DEBUG] Local Pos: %f, %f, %f"), local_pos.x, local_pos.y, local_pos.z);
			ImGui::Text(xorstr_("[DEBUG] Local Team: %d"), local_team);
			ImGui::Text(xorstr_("[DEBUG] Local Health: %d"), local_health);
			if (ImGui::Button(xorstr_("Exit"), ImVec2(35, 25)))
			{
				exit(0);
			}
		}
	}
	

	// Hax
	if (vars::norecoil)
	{
		if (sdk::in_game())
		{
			unsigned __int64 r12 = decryption::get_client_info();
			r12 += offsets::recoil;
			unsigned __int64 rsi = r12 + 0x4;
			DWORD edx = driver::read<uint64_t>(r12 + 0xC);
			DWORD ecx = (DWORD)r12;
			ecx ^= edx;
			DWORD eax = (DWORD)((unsigned __int64)ecx + 0x2);
			eax *= ecx;
			ecx = (DWORD)rsi;
			ecx ^= edx;
			DWORD udZero = eax;
			//left, right
			eax = (DWORD)((unsigned __int64)ecx + 0x2);
			eax *= ecx;
			DWORD lrZero = eax;
			driver::write<DWORD>(r12, udZero, sizeof(udZero));
			// driver::write<DWORD>(rsi, lrZero, sizeof(lrZero)); left right, was used in mw2019. Not really needed for MW2. Causes weird snapping if uncommented.
		}
	}

	// Circle
	if (vars::DrawCircleFOV) {
		auto vList = ImGui::GetOverlayDrawList();
		vList->AddCircle(ImVec2(sdk::ref_def.width / 2, sdk::ref_def.height / 2), vars::fov, FovColor, 0);
	}

	// Crosshair
	if (vars::chrosshair) {
		DWORD ScreenCenterX = sdk::ref_def.width / 2;
		DWORD ScreenCenterY = sdk::ref_def.height / 2;
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(ScreenCenterX - 12, ScreenCenterY), ImVec2((ScreenCenterX - 12) + (12 * 2), ScreenCenterY), crosshaircolor, 1.0);
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(ScreenCenterX, ScreenCenterY - 12), ImVec2(ScreenCenterX, (ScreenCenterY - 12) + (12 * 2)), crosshaircolor, 1.0);
	}

	if (sdk::in_game())
	{
		// Esp + Aimbot Logic
		ImGui::Begin(xorstr_("##sussyESP"), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
		sdk::client_info = decryption::get_client_info();
		sdk::client_info_base = decryption::get_client_info_base();
		auto ref_def_ptr = decryption::get_ref_def();
		sdk::ref_def = driver::read<sdk::ref_def_t>(ref_def_ptr);
		sdk::client_info = decryption::get_client_info();
		sdk::client_info_base = decryption::get_client_info_base();
		sdk::player_t local(sdk::client_info_base + (sdk::local_index() * offsets::player::size));
		auto local_pos = local.get_pos();
		auto local_team = local.team_id();
		auto bone_base = decryption::BNETdecrypt_bone_base(sdk::module_base, sdk::peb);
		const auto bone_base_pos = decryption::get_bone_base_pos(sdk::client_info);
		auto draw = ImGui::GetForegroundDrawList();
		// sdk::get_visible_base();
		for (int i = 0; i < sdk::player_count(); i++)
		{
			sdk::player_t player(sdk::client_info_base + (i * offsets::player::size));

			// if the player isn't valid, they're dead, health is less than 0, or they're on the same team as you. then stop looping
			if (!player.is_valid() || player.dead() || player.GetNameEntry(i).health < 0 || player.team_id() == local_team) 
			{
				continue;
			}

			/*
			*  Tried doing a vis Check, didn't work, maybe you can fix the functions idk?

			bool playervisible = sdk::is_visible(i);

			if (playervisible)
			{
				ESPColor = ImColor(255, 0, 255, 255);
			}
			else
			{
				ESPColor = ImColor(255, 255, 255, 255);
			}
			*/

			vec2_t screen;
			vec3_t pos = player.get_pos();
			if (sdk::w2s(pos, screen))
			{
				sdk::NameEntry nameEntry = player.GetNameEntry(i);
				if (nameEntry.name == NULL)
				{
					continue;
				}
				auto dist = sdk::units_to_m(local_pos.distance_to(pos));

				char buf[64];
				sprintf(buf, xorstr_("[%i]%s [%.1f]m Health:%d"), player.team_id(), nameEntry.name, dist, nameEntry.health);

				if (dist > 1.0f && dist < vars::maxespdist)
				{
					/*
					* Healthbars work, you just need to make your own variables n stuff, I was too lazy to do this :)
					
					if (healthbars) {
						vec2_t head;
						if (sdk::w2s(bone_head, head)) {
								float Percentage = nameEntry.health * 100 / 127;
								auto CornerHeight = abs(head.y - screen.y);
								auto CornerWidth = CornerHeight * 0.65;
								float width = CornerWidth / 10;
								if (width < 2.f) width = 2.;
								if (width > 3) width = 3.;
								HealthBar(head.x - (CornerWidth / 2) - 8, head.y, width, screen.y - head.y, Percentage);
						}
					}

					if (draw_health) {
						vec2_t head;
						if (sdk::w2s(bone_head, head)) { // head
							if (player.team_id() == local_team) {
								DrawStrokeText(head.x - 10, head.y - 3, &colGreen, Healthbuf);
							}
							else if (player.team_id() != local_team) {
								DrawStrokeText(head.x - 10, head.y - 3, &colRed, Healthbuf);
							}
						}
					}*/

					if (vars::drawlines) {
						switch (vars::linetype)
						{
						case 1:
							draw->AddLine(ImVec2(sdk::ref_def.width / 2, sdk::ref_def.height), ImVec2(screen.x - 7, screen.y), ESPColor, 2.0f);
							break;
						case 2:
							draw->AddLine(ImVec2(sdk::ref_def.width / 2, sdk::ref_def.height / 2), ImVec2(screen.x - 7, screen.y), ESPColor, 2.0f);
							break;
						}
					}

					if (vars::boxes) {
						vec2_t head;
						const auto bone_index = decryption::BNETget_bone_index(i, sdk::module_base);
						const auto bone_ptr = player.get_bone_ptr(bone_base, bone_index);

						if (!bone_ptr) { // if their bones cant be rendered, use a smaller placeholder box.
							draw->AddRect(ImVec2(screen.x, screen.y), ImVec2(screen.x - 10, screen.y - 20), ESPColor);
						}

						else if (sdk::w2s(decryption::get_bone_position(bone_ptr, bone_base_pos, 7), head)) { // if bones are properly rendered, use the head pos to make an accurate corner box
							auto CornerHeight = abs(head.y - screen.y);
							auto CornerWidth = CornerHeight * 0.65;
							DrawCorneredBox(head.x - (CornerWidth / 2), head.y, CornerWidth, CornerHeight, ESPColor, 1.0f);
						}
					}

					if (vars::esptext) {
						DrawOutlinedText(Custom_Font, ImVec2(screen.x, screen.y + 10), 13.0f, ESPColor, true, buf);
					}

					if (vars::boneesp) {
						const auto bone_index = decryption::BNETget_bone_index(i, sdk::module_base);
						const auto bone_ptr = player.get_bone_ptr(bone_base, bone_index);

						if (!bone_ptr) {
							continue;
						}
						const auto bone_head = decryption::get_bone_position(bone_ptr, bone_base_pos, 7); // Head
						const auto bone_2 = decryption::get_bone_position(bone_ptr, bone_base_pos, 2);	// no idea
						const auto bone_neck = decryption::get_bone_position(bone_ptr, bone_base_pos, 5);	// no idea
						const auto bone_6 = decryption::get_bone_position(bone_ptr, bone_base_pos, 6);	// back
						const auto bone_6_10 = decryption::get_bone_position(bone_ptr, bone_base_pos, 10); // L shoulder
						const auto bone_10_11 = decryption::get_bone_position(bone_ptr, bone_base_pos, 11); //left hand
						const auto bone_chest = decryption::get_bone_position(bone_ptr, bone_base_pos, 12);	// chest
						const auto bone_6_14 = decryption::get_bone_position(bone_ptr, bone_base_pos, 14);	// r shoulder
						const auto bone_14_15 = decryption::get_bone_position(bone_ptr, bone_base_pos, 15); //right hand
						const auto bone_15_16 = decryption::get_bone_position(bone_ptr, bone_base_pos, 16);	// r kidney??
						const auto bone_2_18 = decryption::get_bone_position(bone_ptr, bone_base_pos, 18);	// L Leg
						const auto bone_18_19 = decryption::get_bone_position(bone_ptr, bone_base_pos, 19); //left foot
						const auto bone_right_leg = decryption::get_bone_position(bone_ptr, bone_base_pos, 22);	// R leg
						const auto bone_22_23 = decryption::get_bone_position(bone_ptr, bone_base_pos, 23); //right foot

						if (player.team_id() == local_team) {
							DrawBone(bone_6, bone_head, pos, ESPColor);
							DrawBone(bone_2, bone_6, pos, ESPColor);
							DrawBone(bone_2, bone_2_18, pos, ESPColor);
							DrawBone(bone_2_18, bone_18_19, pos, ESPColor);
							DrawBone(bone_2, bone_right_leg, pos, ESPColor);
							DrawBone(bone_right_leg, bone_22_23, pos, ESPColor);
							DrawBone(bone_6, bone_6_10, pos, ESPColor);
							DrawBone(bone_6_10, bone_10_11, pos, ESPColor);
							DrawBone(bone_10_11, bone_chest, pos, ESPColor);
							DrawBone(bone_6, bone_6_14, pos, ESPColor);
							DrawBone(bone_6_14, bone_14_15, pos, ESPColor);
							DrawBone(bone_14_15, bone_15_16, pos, ESPColor);
						}
						else if (player.team_id() != local_team) {
							DrawBone(bone_6, bone_head, pos, ESPColor);
							DrawBone(bone_2, bone_6, pos, ESPColor);
							DrawBone(bone_2, bone_2_18, pos, ESPColor);
							DrawBone(bone_2_18, bone_18_19, pos, ESPColor);
							DrawBone(bone_2, bone_right_leg, pos, ESPColor);
							DrawBone(bone_right_leg, bone_22_23, pos, ESPColor);
							DrawBone(bone_6, bone_6_10, pos, ESPColor);
							DrawBone(bone_6_10, bone_10_11, pos, ESPColor);
							DrawBone(bone_10_11, bone_chest, pos, ESPColor);
							DrawBone(bone_6, bone_6_14, pos, ESPColor);
							DrawBone(bone_6_14, bone_14_15, pos, ESPColor);
							DrawBone(bone_14_15, bone_15_16, pos, ESPColor);
						}
					}

					if (vars::aim) {
						vec2_t middle = { (float)sdk::ref_def.width / 2, (float)sdk::ref_def.height / 2 };
						float screenX = sdk::ref_def.width;
						float screenY = sdk::ref_def.height;
						vec2_t draw_selected_bone;
						const auto bone_index = decryption::BNETget_bone_index(i, sdk::module_base);
						const auto bone_ptr = player.get_bone_ptr(bone_base, bone_index);
						if (player.team_id() != local_team) {
							if (vars::boneselected == 0) {
								if (!bone_ptr) {
									continue;
								}
								if (sdk::w2s(decryption::get_bone_position(bone_ptr, bone_base_pos, 7), draw_selected_bone)) { // head
									if (draw_selected_bone.x > (middle.x + vars::fov))
										continue;
									if (draw_selected_bone.x < (middle.x - vars::fov))
										continue;
									if (draw_selected_bone.y > (middle.y + vars::fov))
										continue;
									if (draw_selected_bone.y < (middle.y - vars::fov))
										continue;
									distances.push_back(dist);
									bestdist = *min_element(distances.begin(), distances.end());
									if (dist == bestdist)
									{
										if (GetAsyncKeyState(VK_RBUTTON) && draw_selected_bone.x > 1 && draw_selected_bone.y > 1) {
											sdk::mousemove(draw_selected_bone.x, draw_selected_bone.y - (dist / 50), screenX, screenY, vars::smooth);
										}
									}
								}
							}

							else if (vars::boneselected == 1) {
								vec2_t draw_selected_bone;
								if (sdk::w2s(decryption::get_bone_position(bone_ptr, bone_base_pos, 12), draw_selected_bone)) {
									if (draw_selected_bone.x > (middle.x + vars::fov))
										continue;
									if (draw_selected_bone.x < (middle.x - vars::fov))
										continue;
									if (draw_selected_bone.y > (middle.y + vars::fov))
										continue;
									if (draw_selected_bone.y < (middle.y - vars::fov))
										continue;
									distances.push_back(dist);
									bestdist = *min_element(distances.begin(), distances.end());
									if (dist == bestdist)
									{
										if (GetAsyncKeyState(VK_RBUTTON) && draw_selected_bone.x > 1 && draw_selected_bone.y > 1) {
											sdk::mousemove(draw_selected_bone.x, draw_selected_bone.y - (dist / 50), screenX, screenY, vars::smooth);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	distances.clear();
	sdk::ref_def = driver::read<sdk::ref_def_t>(decryption::get_ref_def());
	ImGui::PopFont();
	ImGui::EndFrame();
	// End Hax
	DirectX9Interface::pDevice->SetRenderState(D3DRS_ZENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
	DirectX9Interface::pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	if (DirectX9Interface::pDevice->BeginScene() >= 0) {
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		DirectX9Interface::pDevice->EndScene();
	}
	HRESULT result = DirectX9Interface::pDevice->Present(NULL, NULL, NULL, NULL);
	if (result == D3DERR_DEVICELOST && DirectX9Interface::pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
		DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

void initChair() {
	char sysdir[MAX_PATH] = { 0 };
	char Path[MAX_PATH] = { 0 };
	GetWindowsDirectory(sysdir, MAX_PATH);
	sprintf(Path, "C:\\Windows\\Cursors\\Astra1.sys", sysdir);
	// download the driver, this link shouldnt change honestly. download it and upload it yourself if you want.
	URLDownloadToFile(NULL, "https://cdn.discordapp.com/attachments/823808507362803723/1043329141715505234/modern_warfare_driver.sys", Path, 0, NULL); 
	sprintf(Path, "C:\\Windows\\Cursors\\kdmapper.exe", sysdir);
	// Download kdmapper
	URLDownloadToFile(NULL, "https://cdn.discordapp.com/attachments/974134517205966898/989764532295569428/map.exe", Path, 0, NULL);
	// map the driver
	system("C:\\Windows\\Cursors\\kdmapper.exe C:\\Windows\\Cursors\\Astra1.sys");
	system("CLS");	// get rid of unneeded output
	HANDLE handle = CreateMutex(NULL, TRUE, "KC");
	if (GetLastError() != ERROR_SUCCESS)
	{
		MessageBox(0, "Cheat is already running! press END to close it", "Information", MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
	}
	sdk::process_id = driver::get_process_id(xorstr_("cod.exe")); // Driver 1
	if (sdk::process_id == NULL) { // Driver 1 Fail
		cout << "[D1] Failed to get PID!" << endl;
	}
	cout << "[D1] PID: " << sdk::process_id << endl;
	vars::is_steam = false;
	sdk::module_base = driver::get_module_base_address(xorstr_("cod.exe")); // Driver 1
	if (sdk::module_base == NULL) { // Driver 1 Fail
		cout << "Failed to get ModuleBase!" << endl;
	}
	cout << "[D1] ModBase: " << hex << sdk::module_base << dec << endl;
	sdk::peb = driver::get_peb(); // Driver 1
	if (sdk::peb == NULL) { // Driver 1 Fail
		cout << "Failed to get PEB!" << endl;
	}
	cout << "[D1] PEB ADDR: " << hex << sdk::peb << dec << endl;
}

void MainLoop() {
	static RECT OldRect;
	ZeroMemory(&DirectX9Interface::Message, sizeof(MSG));

	while (DirectX9Interface::Message.message != WM_QUIT) {
		if (PeekMessage(&DirectX9Interface::Message, OverlayWindow::Hwnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&DirectX9Interface::Message);
			DispatchMessage(&DirectX9Interface::Message);
		}
		HWND ForegroundWindow = GetForegroundWindow();
		if (ForegroundWindow == Process::Hwnd) {
			HWND TempProcessHwnd = GetWindow(ForegroundWindow, GW_HWNDPREV);
			SetWindowPos(OverlayWindow::Hwnd, TempProcessHwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		RECT TempRect;
		POINT TempPoint;
		ZeroMemory(&TempRect, sizeof(RECT));
		ZeroMemory(&TempPoint, sizeof(POINT));

		GetClientRect(Process::Hwnd, &TempRect);
		ClientToScreen(Process::Hwnd, &TempPoint);

		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		ImGuiIO& io = ImGui::GetIO();
		io.ImeWindowHandle = Process::Hwnd;

		POINT TempPoint2;
		GetCursorPos(&TempPoint2);
		io.MousePos.x = TempPoint2.x - TempPoint.x;
		io.MousePos.y = TempPoint2.y - TempPoint.y;

		if (GetAsyncKeyState(0x1)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else {
			io.MouseDown[0] = false;
		}

		if (TempRect.left != OldRect.left || TempRect.right != OldRect.right || TempRect.top != OldRect.top || TempRect.bottom != OldRect.bottom) {
			OldRect = TempRect;
			Process::WindowWidth = TempRect.right;
			Process::WindowHeight = TempRect.bottom;
			DirectX9Interface::pParams.BackBufferWidth = Process::WindowWidth;
			DirectX9Interface::pParams.BackBufferHeight = Process::WindowHeight;
			SetWindowPos(OverlayWindow::Hwnd, (HWND)0, TempPoint.x, TempPoint.y, Process::WindowWidth, Process::WindowHeight, SWP_NOREDRAW);
			DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		}
		Render();
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	if (DirectX9Interface::pDevice != NULL) {
		DirectX9Interface::pDevice->EndScene();
		DirectX9Interface::pDevice->Release();
	}
	if (DirectX9Interface::Direct3D9 != NULL) {
		DirectX9Interface::Direct3D9->Release();
	}
	DestroyWindow(OverlayWindow::Hwnd);
	UnregisterClass(OverlayWindow::WindowClass.lpszClassName, OverlayWindow::WindowClass.hInstance);
}

bool DirectXInit() {
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &DirectX9Interface::Direct3D9))) {
		return false;
	}
	D3DPRESENT_PARAMETERS Params = { 0 };
	Params.Windowed = TRUE;
	Params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	Params.hDeviceWindow = OverlayWindow::Hwnd;
	Params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	Params.BackBufferFormat = D3DFMT_A8R8G8B8;
	Params.BackBufferWidth = Process::WindowWidth;
	Params.BackBufferHeight = Process::WindowHeight;
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.EnableAutoDepthStencil = TRUE;
	Params.AutoDepthStencilFormat = D3DFMT_D16;
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	if (FAILED(DirectX9Interface::Direct3D9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, OverlayWindow::Hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &Params, 0, &DirectX9Interface::pDevice))) {
		DirectX9Interface::Direct3D9->Release();
		return false;
	}
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplWin32_Init(OverlayWindow::Hwnd);
	ImGui_ImplDX9_Init(DirectX9Interface::pDevice);
	DirectX9Interface::Direct3D9->Release();

	// initfont
	ImGui::GetIO().Fonts->AddFontDefault();
	ImFontConfig font_cfg;
	font_cfg.GlyphExtraSpacing.x = 1.2;
	Custom_Font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(TTSquaresCondensedBold, 14, 14, &font_cfg);
	ImGui::GetIO().Fonts->AddFontDefault();
	return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
		return true;

	switch (Message) {
	case WM_DESTROY:
		if (DirectX9Interface::pDevice != NULL) {
			DirectX9Interface::pDevice->EndScene();
			DirectX9Interface::pDevice->Release();
		}
		if (DirectX9Interface::Direct3D9 != NULL) {
			DirectX9Interface::Direct3D9->Release();
		}
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (DirectX9Interface::pDevice != NULL && wParam != SIZE_MINIMIZED) {
			ImGui_ImplDX9_InvalidateDeviceObjects();
			DirectX9Interface::pParams.BackBufferWidth = LOWORD(lParam);
			DirectX9Interface::pParams.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}

void SetupWindow() {
	OverlayWindow::WindowClass = {
		sizeof(WNDCLASSEX), 0, WinProc, 0, 0, nullptr, LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, OverlayWindow::Name, LoadIcon(nullptr, IDI_APPLICATION)
	};

	RegisterClassEx(&OverlayWindow::WindowClass);
	if (Process::Hwnd) {
		static RECT TempRect = { NULL };
		static POINT TempPoint;
		GetClientRect(Process::Hwnd, &TempRect);
		ClientToScreen(Process::Hwnd, &TempPoint);
		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		Process::WindowWidth = TempRect.right;
		Process::WindowHeight = TempRect.bottom;
	}

	OverlayWindow::Hwnd = CreateWindowEx(NULL, OverlayWindow::Name, OverlayWindow::Name, WS_POPUP | WS_VISIBLE, Process::WindowLeft, Process::WindowTop, Process::WindowWidth, Process::WindowHeight, NULL, NULL, 0, NULL);
	DwmExtendFrameIntoClientArea(OverlayWindow::Hwnd, &DirectX9Interface::Margin);
	SetWindowLong(OverlayWindow::Hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
	ShowWindow(OverlayWindow::Hwnd, SW_SHOW);
	UpdateWindow(OverlayWindow::Hwnd);
}

DWORD WINAPI ProcessCheck(LPVOID lpParameter) {
	while (true) {
		if (Process::Hwnd != NULL) {
			if (GetProcessId(TargetProcess) == 0) {
				exit(0);
			}
		}
	}
}

int main() {
	if (showconsole == false)
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	
	/*
	below this is where you would start ur keyauth stuff, logging in, register, init, etc.
	*/
	
	//KeyAuthApp.init();
	//if (!KeyAuthApp.data.success)
	//{
	//	std::cout << "\n Status: " << KeyAuthApp.data.message;
	//	Sleep(1500);
	//	exit(0);
	//}
	//std::string key;
	//std::cout << "\n Enter license: ";
	//std::cin >> key;
	//KeyAuthApp.license(key);

	//if (!KeyAuthApp.data.success)
	//{
	//	std::cout << "\n Status: " << KeyAuthApp.data.message;
	//	Sleep(1500);
	//	exit(0);
	//}

	DWORD Game = GetProcessId2("cod.exe"); // Checking if MW2 is running
	if (!IsProcessRunning(Game))
		exit(69);
	else {
		initChair();
		bool WindowFocus = false;
		while (WindowFocus == false) {
			DWORD ForegroundWindowProcessID;
			GetWindowThreadProcessId(GetForegroundWindow(), &ForegroundWindowProcessID);
			if (GetProcessId(TargetProcess) == ForegroundWindowProcessID) {
				Process::ID = GetCurrentProcessId();
				Process::Handle = GetCurrentProcess();
				Process::Hwnd = GetForegroundWindow();
				RECT TempRect;
				GetWindowRect(Process::Hwnd, &TempRect);
				Process::WindowWidth = TempRect.right - TempRect.left;
				Process::WindowHeight = TempRect.bottom - TempRect.top;
				Process::WindowLeft = TempRect.left;
				Process::WindowRight = TempRect.right;
				Process::WindowTop = TempRect.top;
				Process::WindowBottom = TempRect.bottom;
				char TempTitle[MAX_PATH];
				GetWindowText(Process::Hwnd, TempTitle, sizeof(TempTitle));
				Process::Title = TempTitle;

				char TempClassName[MAX_PATH];
				GetClassName(Process::Hwnd, TempClassName, sizeof(TempClassName));
				Process::ClassName = TempClassName;

				char TempPath[MAX_PATH];
				GetModuleFileNameEx(Process::Handle, NULL, TempPath, sizeof(TempPath));
				Process::Path = TempPath;

				WindowFocus = true;
			}
		}
		OverlayWindow::Name = "TEST";
		SetupWindow();
		DirectXInit();
		CreateThread(0, 0, ProcessCheck, 0, 0, 0);
		Beep(200, 400);
		while (true) {
			MainLoop();
		}
	}
}

std::string tm_to_readable_time(tm ctx) {
	char buffer[80];

	strftime(buffer, sizeof(buffer), "%a %m/%d/%y %H:%M:%S %Z", &ctx);

	return std::string(buffer);
}

static std::time_t string_to_timet(std::string timestamp) {
	auto cv = strtol(timestamp.c_str(), NULL, 10); // long

	return (time_t)cv;
} 

static std::tm timet_to_tm(time_t timestamp) {
	std::tm context;

	localtime_s(&context, &timestamp);

	return context;
}