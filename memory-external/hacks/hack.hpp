#include <thread>
#include <cmath>
#include <algorithm>
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

	void render_off_screen_players(const std::vector<CPlayer>& offScreenPlayers) {
		if (offScreenPlayers.empty()) return;

		// Ekranýn sað üst köþesinde listeleyelim
		int screenWidth = g_game.game_bounds.right;
		int startX = screenWidth - 220; // Sað taraftan 220 pixel içeride
		int startY = 100; // Üstten 100 pixel aþaðýda
		int lineHeight = 18;

		// Uzaklýða göre sýrala (yakýndan uzaða)
		std::vector<CPlayer> sortedPlayers = offScreenPlayers;
		std::sort(sortedPlayers.begin(), sortedPlayers.end(), [](const CPlayer& a, const CPlayer& b) {
			return g_game.localOrigin.calculate_distance(a.origin) < g_game.localOrigin.calculate_distance(b.origin);
			});

		// Baþlýk
		render::RenderText(
			g::hdcBuffer,
			startX,
			startY - 25,
			"GORUNMEYEN DUSMANLAR",
			RGB(255, 50, 50),
			14
		);

		// Sadece ilk 8 oyuncuyu göster (ekraný doldurmasýn)
		int count = (8 < sortedPlayers.size()) ? 8 : sortedPlayers.size();

		for (int i = 0; i < count; i++) {
			const auto& player = sortedPlayers[i];
			float distance = g_game.localOrigin.calculate_distance(player.origin);
			int roundedDistance = std::round(distance / 10.f);

			int currentY = startY + (i * lineHeight);

			// Ýsim (kýsaltýlmýþ)
			std::string name = player.name;
			if (name.length() > 12) {
				name = name.substr(0, 12) + "...";
			}

			// Uzaklýk
			std::string distanceText = std::to_string(roundedDistance) + "m";

			// HP
			std::string hpText = std::to_string(player.health) + "HP";

			// HP rengi
			COLORREF hpColor;
			if (player.health > 75) {
				hpColor = RGB(0, 255, 0); // Yeþil
			}
			else if (player.health > 25) {
				hpColor = RGB(255, 255, 0); // Sarý
			}
			else {
				hpColor = RGB(255, 0, 0); // Kýrmýzý
			}

			// Ýsim
			render::RenderText(
				g::hdcBuffer,
				startX,
				currentY,
				name.c_str(),
				config::esp_name_color,
				12
			);

			// Uzaklýk
			render::RenderText(
				g::hdcBuffer,
				startX + 80,
				currentY,
				distanceText.c_str(),
				config::esp_distance_color,
				12
			);

			// HP
			render::RenderText(
				g::hdcBuffer,
				startX + 130,
				currentY,
				hpText.c_str(),
				hpColor,
				12
			);

			// Zýrh (eðer varsa)
			if (player.armor > 0) {
				std::string armorText = "Z:" + std::to_string(player.armor);
				render::RenderText(
					g::hdcBuffer,
					startX + 170,
					currentY,
					armorText.c_str(),
					RGB(100, 200, 255),
					10
				);
			}
		}
	}

	void loop() {
		std::lock_guard<std::mutex> lock(reader_mutex);

		if (g_game.isC4Planted) {
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

		std::vector<CPlayer> offScreenPlayers; // Ekran dýþý oyuncular

		/**
		* Loop through all the players in the entity list
		*/
		for (auto player = g_game.players.begin(); player < g_game.players.end(); player++) {
			const Vector3 screenPos = g_game.world_to_screen(&player->origin);
			const Vector3 screenHead = g_game.world_to_screen(&player->head);

			// Sadece düþmanlarý kontrol et
			if (config::team_esp && player->team == g_game.localTeam) {
				continue;
			}

			// Eðer oyuncu ekranda görünmüyorsa, offScreenPlayers listesine ekle
			bool isOnScreen = (screenPos.z >= 0.01f &&
				utils.is_in_bounds(screenPos, g_game.game_bounds.right, g_game.game_bounds.bottom));

			if (!isOnScreen) {
				offScreenPlayers.push_back(*player);
				continue; // Ekranda görünmeyen oyuncuyu normal ESP çizimine dahil etme
			}

			// Ekranda görünen oyuncular için normal ESP çizimi
			const float height = screenPos.y - screenHead.y;
			const float width = height / 2.4f;

			float distance = g_game.localOrigin.calculate_distance(player->origin);
			int roundedDistance = std::round(distance / 10.f);

			if (config::show_skeleton_esp) {
				// Önce iskelet çizgilerini çiz
				for (const auto& connection : boneConnections) {
					const std::string& boneFrom = connection.first;
					const std::string& boneTo = connection.second;

					COLORREF skeletonColor = config::esp_skeleton_color_enemy;

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
				COLORREF headColor = config::esp_skeleton_color_enemy;

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

			if (config::show_box_esp) {
				render::DrawBorderBox(
					g::hdcBuffer,
					screenHead.x - width / 2,
					screenHead.y,
					width,
					height,
					config::esp_box_color_enemy
				);
			}

			// Health bar - Dolu kýsým
			float healthWidth = width * player->health / 100.0f;
			render::DrawFilledBox(
				g::hdcBuffer,
				screenHead.x - width / 2,
				screenHead.y - 30,
				healthWidth,
				5,
				RGB((255 - player->health), (55 + player->health * 2), 75)
			);

			// Health bar - Eksik kýsým (çizgili)
			render::DrawBorderBox(
				g::hdcBuffer,
				screenHead.x - width / 2,
				screenHead.y - 30,
				width,  // Tam geniþlik
				5,
				RGB(255, 100, 100)  // Açýk kýrmýzý çerçeve
			);

			// Armor bar - Dolu kýsým
			float armorWidth = width * player->armor / 100.0f;
			render::DrawFilledBox(
				g::hdcBuffer,
				screenHead.x - width / 2,
				screenHead.y - 20,
				armorWidth,
				5,
				RGB(0, 185, 255)
			);

			// Armor bar - Eksik kýsým (çizgili)
			render::DrawBorderBox(
				g::hdcBuffer,
				screenHead.x - width / 2,
				screenHead.y - 20,
				width,  // Tam geniþlik
				5,
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

			if (config::show_extra_flags) {
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

				if (player->flashAlpha > 100) {
					render::RenderText(
						g::hdcBuffer,
						screenHead.x + (width / 2 + 5),
						screenHead.y + 60,
						"Player is flashed",
						config::esp_distance_color,
						15
					);
				}

				if (player->is_defusing) {
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

		// Ekran dýþý oyuncularý render et
		render_off_screen_players(offScreenPlayers);
	}
}