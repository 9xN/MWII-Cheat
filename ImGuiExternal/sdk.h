#pragma once
#include <cstdint>
#include <windows.h>
#include "leVectors.h"
#include "ImGui/imgui.h"

namespace vars
{
	// bool
	static bool is_steam = false;
	static bool aim = false;
	static bool boxes = false;
	static bool boneesp = false;
	static bool DrawCircleFOV = false;
	static bool norecoil = false;
	static bool chrosshair = false;
	static bool drawlines = false;
	static bool fovcircle = false;
	static bool draw_health = false;
	static bool esptext = false;
	static bool healthbars = false;

	// Floats
	static float smooth = 1;
	static float fov = 90.0f;
	static float maxespdist = 200.0f;

	// ints
	static int linetype = 1;
	static int boneselected = 0;

	static const char* bones[]{ "Head", "Chest" };
	static ImColor crosshaircolor = ImColor(255.f / 255.f, 0.f, 255.f);
	static ImColor FovColor = ImColor(255.0f / 255, 255.f, 255.0f);
	static ImColor ESPColor = ImColor(255.0f / 255, 255.f, 255.0f);
}

namespace sdk {
	extern HANDLE	 process_id;
	extern uintptr_t module_base;
	extern uintptr_t peb;
	extern HWND      hwnd;

	extern uintptr_t client_info;
	extern uintptr_t client_info_base;
	extern uint64_t bone_base;
	extern uint64_t bone_index;
	extern uintptr_t currentvisoffset;
	extern uintptr_t last_visible_offset;

	struct ref_def_view {
		vec2_t tan_half_fov;
		char pad[0xC];
		vec3_t axis[3];
	};

	struct ref_def_t {
		int x;
		int y;
		int width;
		int height;
		ref_def_view view;
	};

	struct NameEntry
	{
		uint32_t index;
		char name[36];
		uint8_t pad[92];
		int32_t health;
		uint8_t pad2[70];
	};

	enum bones {
		bone_pos_helmet = 8,

		bone_pos_head = 7,
		bone_pos_neck = 6,
		bone_pos_chest = 5,
		bone_pos_mid = 4,
		bone_pos_tummy = 3,
		bone_pos_pelvis = 2,

		bone_pos_right_foot_1 = 21,
		bone_pos_right_foot_2 = 22,
		bone_pos_right_foot_3 = 23,
		bone_pos_right_foot_4 = 24,

		bone_pos_left_foot_1 = 17,
		bone_pos_left_foot_2 = 18,
		bone_pos_left_foot_3 = 19,
		bone_pos_left_foot_4 = 20,

		bone_pos_left_hand_1 = 13,
		bone_pos_left_hand_2 = 14,
		bone_pos_left_hand_3 = 15,
		bone_pos_left_hand_4 = 16,

		bone_pos_right_hand_1 = 9,
		bone_pos_right_hand_2 = 10,
		bone_pos_right_hand_3 = 11,
		bone_pos_right_hand_4 = 12
	};

	extern struct ref_def_t ref_def;
	void mousemove(float tarx, float tary, float X, float Y, int smooth);// absoulte doesnt work at all for warzone

	void set_game_hwnd();

	int player_count();

	int local_index();

	uint64_t get_visible_base();

	bool is_visible(int index);

	bool in_game();

	class player_t {
	public:
		player_t(uintptr_t address) {
			this->address = address;
		}

		uintptr_t address{};

		int id{};

		bool is_valid();

		bool dead();


		int team_id();

		vec3_t get_pos();
		uint32_t getIndex();
		NameEntry GetNameEntry(uint32_t index);
		uintptr_t get_bone_ptr(uint64_t bone_base, uint64_t bone_index);
	};

	bool w2s(vec3_t world_position, vec2_t& screen_position);

	float units_to_m(float units);
	uint64_t GetNameList();
}
float Distance3D(vec3_t point1, vec3_t point2);
void DrawBone(vec3_t from, vec3_t to, vec3_t m_location, ImU32 col);
namespace decryption {
	uintptr_t get_client_info();

	uintptr_t get_client_info_base();

	uint64_t get_bone_decrypted_base();

	uint64_t platform_get_bone_index(int index);

	uintptr_t get_ref_def();

	extern "C" auto BNETdecrypt_client_info(uint64_t imageBase, uint64_t peb)->uint64_t;

	extern "C" auto BNETdecrypt_client_base(uint64_t clientInfo, uint64_t imageBase, uint64_t peb)->uint64_t;

	extern "C" auto BNETdecrypt_bone_base(uint64_t imageBase, uint64_t peb)->uint64_t;

	extern "C" auto BNETget_bone_index(uint32_t index, uint64_t imageBase)->uint64_t;

	extern "C" auto STEAMdecrypt_client_info(uint64_t imageBase, uint64_t peb)->uint64_t;

	extern "C" auto STEAMdecrypt_client_base(uint64_t clientInfo, uint64_t imageBase, uint64_t peb)->uint64_t;

	extern "C" auto STEAMdecrypt_bone_base(uint64_t imageBase, uint64_t peb)->uint64_t;

	extern "C" auto STEAMget_bone_index(uint32_t index, uint64_t imageBase)->uint64_t;

	vec3_t get_bone_position(const uintptr_t bone_ptr, const vec3_t& base_pos, const int bone);

	vec3_t get_bone_base_pos(const uintptr_t client_info);

}
