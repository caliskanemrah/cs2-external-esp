#include "config.hpp"

namespace config {
    bool read() {
        std::string config_path = get_config_path();
        std::cout << "[CONFIG] Looking for config at: " << config_path << std::endl;

        std::ifstream f(config_path);
        if (!f.good()) {
            std::cout << "[CONFIG] Config file not found, creating default..." << std::endl;
            save();
            return false;
        }

        std::cout << "[CONFIG] Config file found, reading..." << std::endl;

        json data;
        try {
            data = json::parse(f);
            std::cout << "[CONFIG] JSON parsed successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "[CONFIG] JSON parse error: " << e.what() << std::endl;
            return false;
        }

        if (data.empty()) {
            std::cout << "[CONFIG] Config data is empty" << std::endl;
            return false;
        }

        // Tüm config deðerlerini debug et
        std::cout << "[CONFIG] Raw values from JSON:" << std::endl;
        std::cout << "  - aimbot_smoothness: " << data["aimbot_smoothness"] << std::endl;
        std::cout << "  - aimbot_aggressive_smoothness: " << data["aimbot_aggressive_smoothness"] << std::endl;
        std::cout << "  - aimbot_toggle_smooth: " << data["aimbot_toggle_smooth"] << std::endl;

        // ESP ayarlarýný oku
        if (data["show_box_esp"].is_boolean())
            show_box_esp = data["show_box_esp"];
        if (data["show_skeleton_esp"].is_boolean())
            show_skeleton_esp = data["show_skeleton_esp"];
        if (data["show_head_tracker"].is_boolean())
            show_head_tracker = data["show_head_tracker"];
        if (data["team_esp"].is_boolean())
            team_esp = data["team_esp"];
        if (data["automatic_update"].is_boolean())
            automatic_update = data["automatic_update"];
        if (data["render_distance"].is_number())
            render_distance = data["render_distance"];
        if (data["flag_render_distance"].is_number())
            flag_render_distance = data["flag_render_distance"];
        if (data["show_extra_flags"].is_boolean())
            show_extra_flags = data["show_extra_flags"];

        // Aimbot ayarlarýný oku
        if (data["aimbot_enabled"].is_boolean()) {
            aimbot_enabled = data["aimbot_enabled"];
            std::cout << "[CONFIG] aimbot_enabled: " << aimbot_enabled << std::endl;
        }
        if (data["aimbot_smoothness"].is_number()) {
            aimbot_smoothness = data["aimbot_smoothness"];
            std::cout << "[CONFIG] aimbot_smoothness: " << aimbot_smoothness << std::endl;
        }
        if (data["aimbot_max_distance"].is_number()) {
            aimbot_max_distance = data["aimbot_max_distance"];
            std::cout << "[CONFIG] aimbot_max_distance: " << aimbot_max_distance << std::endl;
        }
        if (data["aimbot_humanize"].is_boolean()) {
            aimbot_humanize = data["aimbot_humanize"];
            std::cout << "[CONFIG] aimbot_humanize: " << aimbot_humanize << std::endl;
        }
        if (data["aimbot_toggle_smooth"].is_boolean()) {
            aimbot_toggle_smooth = data["aimbot_toggle_smooth"];
            std::cout << "[CONFIG] aimbot_toggle_smooth: " << aimbot_toggle_smooth << std::endl;
        }
        if (data["aimbot_aggressive_smoothness"].is_number()) {
            aimbot_aggressive_smoothness = data["aimbot_aggressive_smoothness"];
            std::cout << "[CONFIG] aimbot_aggressive_smoothness: " << aimbot_aggressive_smoothness << std::endl;
        }
        if (data["aimbot_toggle_key"].is_number()) {
            aimbot_toggle_key = data["aimbot_toggle_key"];
            std::cout << "[CONFIG] aimbot_toggle_key: " << aimbot_toggle_key << std::endl;
        }

        // Renk ayarlarýný oku
        if (data.find("esp_box_color_team") != data.end()) {
            esp_box_color_team = {
                data["esp_box_color_team"][0].get<int>(),
                data["esp_box_color_team"][1].get<int>(),
                data["esp_box_color_team"][2].get<int>()
            };
        }

        if (data.find("esp_box_color_enemy") != data.end()) {
            esp_box_color_enemy = {
                data["esp_box_color_enemy"][0].get<int>(),
                data["esp_box_color_enemy"][1].get<int>(),
                data["esp_box_color_enemy"][2].get<int>()
            };
        }

        if (data.find("esp_skeleton_color_team") != data.end()) {
            esp_skeleton_color_team = {
                data["esp_skeleton_color_team"][0].get<int>(),
                data["esp_skeleton_color_team"][1].get<int>(),
                data["esp_skeleton_color_team"][2].get<int>()
            };
        }

        if (data.find("esp_skeleton_color_enemy") != data.end()) {
            esp_skeleton_color_enemy = {
                data["esp_skeleton_color_enemy"][0].get<int>(),
                data["esp_skeleton_color_enemy"][1].get<int>(),
                data["esp_skeleton_color_enemy"][2].get<int>()
            };
        }

        if (data.find("esp_name_color") != data.end()) {
            esp_name_color = {
                data["esp_name_color"][0].get<int>(),
                data["esp_name_color"][1].get<int>(),
                data["esp_name_color"][2].get<int>()
            };
        }

        if (data.find("esp_distance_color") != data.end()) {
            esp_distance_color = {
                data["esp_distance_color"][0].get<int>(),
                data["esp_distance_color"][1].get<int>(),
                data["esp_distance_color"][2].get<int>()
            };
        }

        std::cout << "[CONFIG] Config reading completed!" << std::endl;
        return true;
    }

    void save() {
        std::string config_path = get_config_path();
        std::cout << "[CONFIG] Saving config to: " << config_path << std::endl;

        json data;

        // ESP ayarlarýný kaydet
        data["show_box_esp"] = show_box_esp;
        data["show_skeleton_esp"] = show_skeleton_esp;
        data["show_head_tracker"] = show_head_tracker;
        data["team_esp"] = team_esp;
        data["automatic_update"] = automatic_update;
        data["render_distance"] = render_distance;
        data["flag_render_distance"] = flag_render_distance;
        data["show_extra_flags"] = show_extra_flags;

        // Aimbot ayarlarýný kaydet
        data["aimbot_enabled"] = aimbot_enabled;
        data["aimbot_smoothness"] = aimbot_smoothness;
        data["aimbot_max_distance"] = aimbot_max_distance;
        data["aimbot_humanize"] = aimbot_humanize;
        data["aimbot_toggle_smooth"] = aimbot_toggle_smooth;
        data["aimbot_aggressive_smoothness"] = aimbot_aggressive_smoothness;
        data["aimbot_toggle_key"] = aimbot_toggle_key;

        // Renk ayarlarýný kaydet
        data["esp_box_color_team"] = { esp_box_color_team.r, esp_box_color_team.g, esp_box_color_team.b };
        data["esp_box_color_enemy"] = { esp_box_color_enemy.r, esp_box_color_enemy.g, esp_box_color_enemy.b };
        data["esp_skeleton_color_team"] = { esp_skeleton_color_team.r, esp_skeleton_color_team.g, esp_skeleton_color_team.b };
        data["esp_skeleton_color_enemy"] = { esp_skeleton_color_enemy.r, esp_skeleton_color_enemy.g, esp_skeleton_color_enemy.b };
        data["esp_name_color"] = { esp_name_color.r, esp_name_color.g, esp_name_color.b };
        data["esp_distance_color"] = { esp_distance_color.r, esp_distance_color.g, esp_distance_color.b };

        std::ofstream output(config_path);
        output << std::setw(4) << data << std::endl;
        output.close();

        std::cout << "[CONFIG] Config saved successfully to: " << config_path << std::endl;
        utils.update_console_title();
    }
}