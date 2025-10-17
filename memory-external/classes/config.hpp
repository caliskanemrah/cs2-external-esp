#pragma once
#include <fstream>
#include <windows.h>
#include "utils.h"
#include "json.hpp"
#include "auto_updater.hpp"

using json = nlohmann::json;

struct RGB {
    int r;
    int g;
    int b;

    operator COLORREF() const {
        return RGB(r, g, b);
    }
};

namespace config {
    // Exe yolunu alan fonksiyon
    inline std::string get_exe_path() {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string::size_type pos = std::string(buffer).find_last_of("\\/");
        return std::string(buffer).substr(0, pos);
    }

    // Config dosya yolu - her çaðrýldýðýnda exe yolundan alýr
    inline std::string get_config_path() {
        return get_exe_path() + "\\config.json";
    }

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

    // Aimbot ayarlarý
    inline bool aimbot_enabled = true;
    inline float aimbot_smoothness = 0.1f;
    inline float aimbot_max_distance = 2000000.0f;
    inline bool aimbot_humanize = false;
    inline bool aimbot_toggle_smooth = true;
    inline float aimbot_aggressive_smoothness = 0.01f;
    inline int aimbot_toggle_key = 112;

    inline RGB esp_box_color_team = { 75, 175, 75 };
    inline RGB esp_box_color_enemy = { 225, 75, 75 };
    inline RGB esp_skeleton_color_team = { 75, 175, 75 };
    inline RGB esp_skeleton_color_enemy = { 225, 75, 75 };
    inline RGB esp_name_color = { 250, 250, 250 };
    inline RGB esp_distance_color = { 75, 75, 175 };
}