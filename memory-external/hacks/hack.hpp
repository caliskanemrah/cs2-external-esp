#include <thread>
#include <cmath>
#include "reader.hpp"
#include "../classes/render.hpp"
#include "../classes/config.hpp"
#include "../classes/globals.hpp"

namespace hack {
	std::vector<std::pair<std::string, std::string>> boneConnections = {
						{"neck_0", "spine_1"},
						{"spine_1", "spine_2"},
						{"spine_2", "pelvis"},
						{"spine_1", "arm_upper_L"},
						{"arm_upper_L", "arm_lower_L"},
						{"arm_lower_L", "hand_L"},
						{"spine_1", "arm_upper_R"},
						{"arm_upper_R", "arm_lower_R"},
						{"arm_lower_R", "hand_R"},
						{"pelvis", "leg_upper_L"},
						{"leg_upper_L", "leg_lower_L"},
						{"leg_lower_L", "ankle_L"},
						{"pelvis", "leg_upper_R"},
						{"leg_upper_R", "leg_lower_R"},
						{"leg_lower_R", "ankle_R"}
	};

	void loop() {

		std::lock_guard<std::mutex> lock(reader_mutex);

		if (g_game.isC4Planted)
		{
			Vector3 c4Origin = g_game.c4.get_origin();
			const Vector3 c4ScreenPos = g_game.world_to_screen(&c4Origin);

			if (c4ScreenPos.z >= 0.01f) {
				float c4Distance = g_game.localOrigin.calculate_distance(c4Origin);
				float c4RoundedDistance = std::round(c4Distance / 500.f);

				float height = 10 - c4RoundedDistance;
				float width = height * 1.4f;

				render::DrawFilledBox(
					g::hdcBuffer,
					c4ScreenPos.x - (width / 2),
					c4ScreenPos.y - (height / 2),
					width,
					height,
					config::esp_box_color_enemy
				);

				render::RenderText(
					g::hdcBuffer,
					c4ScreenPos.x + (width / 2 + 5),
					c4ScreenPos.y,
					"C4",
					config::esp_name_color,
					10
				);
			}
		}

		int playerIndex = 0;
		uintptr_t list_entry;

		/**
		* Loop through all the players in the entity list
		*
		* (This could have been done by getting a entity list count from the engine, but I'm too lazy to do that)
		**/
		for (auto player = g_game.players.begin(); player < g_game.players.end(); player++) {
			const Vector3 screenPos = g_game.world_to_screen(&player->origin);
			const Vector3 screenHead = g_game.world_to_screen(&player->head);

			if (
				screenPos.z < 0.01f || 
				!utils.is_in_bounds(screenPos, g_game.game_bounds.right, g_game.game_bounds.bottom)
				)
				continue;

			const float height = screenPos.y - screenHead.y;
			const float width = height / 2.4f;

			float distance = g_game.localOrigin.calculate_distance(player->origin);
			int roundedDistance = std::round(distance / 10.f);

			if (config::show_skeleton_esp) {
				// Önce iskelet çizgilerini çiz
				for (const auto& connection : boneConnections) {
					const std::string& boneFrom = connection.first;
					const std::string& boneTo = connection.second;

					COLORREF skeletonColor = g_game.localTeam == player->team
						? config::esp_skeleton_color_team
						: config::esp_skeleton_color_enemy;

					// Gölge efekti için önce siyah çizgi
					render::DrawLine(
						g::hdcBuffer,
						player->bones.bonePositions[boneFrom].x + 1,
						player->bones.bonePositions[boneFrom].y + 1,
						player->bones.bonePositions[boneTo].x + 1,
						player->bones.bonePositions[boneTo].y + 1,
						RGB(0, 0, 0)
					);

					// Asýl renkli çizgi
					render::DrawLine(
						g::hdcBuffer,
						player->bones.bonePositions[boneFrom].x,
						player->bones.bonePositions[boneFrom].y,
						player->bones.bonePositions[boneTo].x,
						player->bones.bonePositions[boneTo].y,
						skeletonColor
					);
				}
			}

			if (config::show_head_tracker) {
				COLORREF headColor = g_game.localTeam == player->team
					? config::esp_skeleton_color_team
					: config::esp_skeleton_color_enemy;

				// Gölge efekti için siyah daire
				render::DrawCircle(
					g::hdcBuffer,
					player->bones.bonePositions["head"].x + 1,
					player->bones.bonePositions["head"].y - width / 12 + 1,
					width / 5,
					RGB(0, 0, 0)
				);

				// Asýl renkli daire
				render::DrawCircle(
					g::hdcBuffer,
					player->bones.bonePositions["head"].x,
					player->bones.bonePositions["head"].y - width / 12,
					width / 5,
					headColor
				);

				// Ýçi dolu küçük nokta (hedef noktasý)
				render::DrawFilledBox(
					g::hdcBuffer,
					player->bones.bonePositions["head"].x - 2,
					player->bones.bonePositions["head"].y - width / 12 - 2,
					4,
					4,
					headColor
				);
			}

			if (config::show_box_esp)
			{
				render::DrawBorderBox(
					g::hdcBuffer,
					screenHead.x - width / 2,
					screenHead.y,
					width,
					height,
					(g_game.localTeam == player->team ? config::esp_box_color_team : config::esp_box_color_enemy)
				);
			}

			// Health bar - Dolu kýsým
			float healthWidth = width * player->health / 100.0f;
			render::DrawFilledBox(
				g::hdcBuffer,
				screenHead.x - width / 2,
				screenHead.y - 10,
				healthWidth,
				6,
				RGB((255 - player->health), (55 + player->health * 2), 75)
			);

			// Health bar - Eksik kýsým (çizgili)
			render::DrawBorderBox(
				g::hdcBuffer,
				screenHead.x - width / 2,
				screenHead.y - 10,
				width,  // Tam geniþlik
				6,
				RGB(255, 100, 100)  // Açýk kýrmýzý çerçeve
			);

			// Armor bar - Dolu kýsým
			float armorWidth = width * player->armor / 100.0f;
			render::DrawFilledBox(
				g::hdcBuffer,
				screenHead.x - width / 2,
				screenHead.y - 3,
				armorWidth,
				6,
				RGB(0, 185, 255)
			);

			// Armor bar - Eksik kýsým (çizgili)
			render::DrawBorderBox(
				g::hdcBuffer,
				screenHead.x - width / 2,
				screenHead.y - 3,
				width,  // Tam geniþlik
				6,
				RGB(100, 200, 255)  // Açýk mavi çerçeve
			);

			render::RenderText(
				g::hdcBuffer,
				screenHead.x + (width / 2 + 5),
				screenHead.y,
				(std::to_string(roundedDistance) + "m away").c_str(),
				config::esp_distance_color,
				17
			);

			render::RenderText(
				g::hdcBuffer,
				screenHead.x + (width / 2 + 5),
				screenHead.y + 20,
				player->name.c_str(),
				config::esp_name_color,
				15
			);

			/**
			* I know is not the best way but a simple way to not saturate the screen with a ton of information
			*/
			if (roundedDistance > config::flag_render_distance)
				continue;

			render::RenderText(
				g::hdcBuffer,
				screenHead.x + (width / 2 + 5),
				screenHead.y + 40,
				("Can :" + std::to_string(player->health) + "hp").c_str(),
				config::esp_hp_color,
				15
			);

			const char* zirhText = "Zirh: ";
			std::string displayText = std::string(zirhText) + std::to_string(player->armor);
			render::RenderText(
				g::hdcBuffer,
				screenHead.x + (width / 2 + 5),
				screenHead.y + 55,
				displayText.c_str(),
				config::esp_armor_color,
				15
			);

			if (config::show_extra_flags)
			{
				render::RenderText(
					g::hdcBuffer,
					screenHead.x + (width / 2 + 5),
					screenHead.y + 70,
					("Silah : " + player->weapon).c_str(),
					config::esp_weapon_color,
					15
				);

				

				render::RenderText(
					g::hdcBuffer,
					screenHead.x + (width / 2 + 5),
					screenHead.y + 85,
					("Para : $" + std::to_string(player->money)).c_str(),
					config::esp_money_color,
					15
				);

				if (player->flashAlpha > 100)
				{
					render::RenderText(
						g::hdcBuffer,
						screenHead.x + (width / 2 + 5),
						screenHead.y + 60,
						"Player is flashed",
						config::esp_distance_color,
						15
					);
				}

				if (player->is_defusing)
				{
					const std::string defuText = "Player is defusing";
					render::RenderText(
						g::hdcBuffer,
						screenHead.x + (width / 2 + 5),
						screenHead.y + 60,
						defuText.c_str(),
						config::esp_distance_color,
						10
					);
				}
			}
		}
		// std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

