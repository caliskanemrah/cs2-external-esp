#pragma once
#include <fstream>
#include "utils.h"
#include "json.hpp"
#include "auto_updater.hpp"

using json = nlohmann::json;

struct RGB {
	int r;
	int g;
	int b;

	// Conversion function from RGB to COLORREF
	operator COLORREF() const {
		return RGB(r, g, b);
	}
};

namespace config {
	const std::string file_path = "config.json";

	extern bool read();
	extern void save();

	inline bool automatic_update = false;
	inline bool team_esp = false;
	inline float render_distance = -1.f;
	inline int flag_render_distance = 200;
	inline bool show_box_esp = true;
	inline bool show_skeleton_esp = false;
	inline bool show_head_tracker = false;
	inline bool show_extra_flags = false;

	inline RGB esp_box_color_team = { 75, 175, 75 };
	inline RGB esp_box_color_enemy = { 225, 75, 75 };
	inline RGB esp_skeleton_color_team = { 75, 175, 75 };
	inline RGB esp_skeleton_color_enemy = { 225, 75, 75 };

	inline RGB esp_name_color = { 255, 220, 100 };
	inline RGB esp_distance_color = { 252, 3, 132 };
	inline RGB esp_hp_color = { 100, 255, 150 };
	inline RGB esp_armor_color = { 100, 200, 255 };
	inline RGB esp_weapon_color = { 252, 3, 65 };
	inline RGB esp_money_color = { 3, 119, 252 };
}
