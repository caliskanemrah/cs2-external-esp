#include "reader.hpp"
#include <thread>
#include <map>
#include <cmath>
#include <limits>
#define _USE_MATH_DEFINES
#include <math.h>
#include "../classes/auto_updater.hpp"
#include "../classes/config.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Aimbot sýnýfý
class CAimbot {
private:
    bool aggressiveMode = false;
    bool lastToggleState = false;

public:
    void SmoothAimToTarget() {
        if (!g_game.inGame || g_game.players.empty()) return;

        // DEBUG: Config deðerlerini doðrudan kontrol et
        static int debugCounter = 0;
        if (debugCounter++ % 100 == 0) {
            std::cout << "[AIMBOT DEBUG] Config Values - "
                << "Smoothness: " << config::aimbot_smoothness
                << ", Aggressive: " << config::aimbot_aggressive_smoothness
                << ", Toggle: " << config::aimbot_toggle_smooth
                << ", ToggleKey: " << config::aimbot_toggle_key << std::endl;
        }

       
        bool currentToggleState = (GetAsyncKeyState(config::aimbot_toggle_key) & 0x8000) != 0;
        if (currentToggleState && !lastToggleState) {
            aggressiveMode = !aggressiveMode;
            std::cout << "=== AIMBOT TOGGLE ===" << std::endl;
            std::cout << "[AIMBOT] Mode: " << (aggressiveMode ? "AGGRESSIVE" : "NORMAL") << std::endl;
        }
        lastToggleState = currentToggleState;

        // Sol fare tuþu kontrolü
        bool leftMousePressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (!leftMousePressed) {
            return;
        }

        float closestDistance = (std::numeric_limits<float>::max)();
        Vector3 bestHeadPos;
        bool foundTarget = false;

        // En yakýn düþmaný bul
        for (auto& player : g_game.players) {
            if (player.health <= 0 || player.team == g_game.localTeam) continue;

            float distance = (g_game.localOrigin - player.origin).length();
            if (distance < closestDistance) {
                closestDistance = distance;
                bestHeadPos = player.head;
                foundTarget = true;
            }
        }

        if (!foundTarget) return;

        Vector3 currentAngles = g_game.process->read<Vector3>(g_game.localpCSPlayerPawn + updater::offsets::m_angEyeAngles);
        Vector3 aimAngles = CalculateAimAngle(g_game.localOrigin, bestHeadPos);

        // Toggle durumuna göre smoothness seç
        float smoothness;
        if (aggressiveMode) {
            smoothness = config::aimbot_aggressive_smoothness;
        }
        else {
            smoothness = config::aimbot_smoothness;
        }

        std::cout << "[AIMBOT] Using smoothness: " << smoothness << std::endl;

        Vector3 smoothedAngles;
        smoothedAngles.x = currentAngles.x + (aimAngles.x - currentAngles.x) * smoothness;
        smoothedAngles.y = currentAngles.y + (aimAngles.y - currentAngles.y) * smoothness;
        smoothedAngles.z = 0;

        g_game.process->write<Vector3>(g_game.localpCSPlayerPawn + updater::offsets::m_angEyeAngles, smoothedAngles);
    }

private:
    Vector3 CalculateAimAngle(Vector3 localPos, Vector3 targetPos) {
        Vector3 delta = targetPos - localPos;
        Vector3 angles;

        // Yaw (yatay)
        angles.y = atan2(delta.y, delta.x) * (180.0 / M_PI);

        // Pitch (dikey)
        float hypotenuse = sqrt(delta.x * delta.x + delta.y * delta.y);
        angles.x = atan2(-delta.z, hypotenuse) * (180.0 / M_PI);

        return angles;
    }
};

static CAimbot aimbot;

std::map<std::string, int> boneMap = {
    {"head", 6},
    {"neck_0", 5},
    {"spine_1", 4},
    {"spine_2", 2},
    {"pelvis", 0},
    {"arm_upper_L", 8},
    {"arm_lower_L", 9},
    {"hand_L", 10},
    {"arm_upper_R", 13},
    {"arm_lower_R", 14},
    {"hand_R", 15},
    {"leg_upper_L", 22},
    {"leg_lower_L", 23},
    {"ankle_L", 24},
    {"leg_upper_R", 25},
    {"leg_lower_R", 26},
    {"ankle_R", 27}
};

// CC4
uintptr_t CC4::get_planted() {
    return g_game.process->read<uintptr_t>(g_game.process->read<uintptr_t>(g_game.base_client.base + updater::offsets::dwPlantedC4));
}

uintptr_t CC4::get_node() {
    return g_game.process->read<uintptr_t>(get_planted() + updater::offsets::m_pGameSceneNode);
}

Vector3 CC4::get_origin() {
    return g_game.process->read<Vector3>(get_node() + updater::offsets::m_vecAbsOrigin);
}

// CGame
void CGame::init() {
    std::cout << "[cs2] Waiting for cs2.exe..." << std::endl;

    process = std::make_shared<pProcess>();
    while (!process->AttachProcess("cs2.exe"))
        std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "[cs2] Attached to cs2.exe\n" << std::endl;

    do {
        base_client = process->GetModule("client.dll");
        base_engine = process->GetModule("engine2.dll");
        if (base_client.base == 0 || base_engine.base == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "[cs2] Failed to find module client.dll/engine2.dll, waiting for the game to load it..." << std::endl;
        }
    } while (base_client.base == 0 || base_engine.base == 0);

    GetClientRect(process->hwnd_, &game_bounds);

    buildNumber = process->read<uintptr_t>(base_engine.base + updater::offsets::dwBuildNumber);
}

void CGame::close() {
    std::cout << "[cs2] Deatachig from process" << std::endl;
    process->Close();
}

void CGame::loop() {
    std::lock_guard<std::mutex> lock(reader_mutex);

    if (updater::offsets::dwLocalPlayerController == 0x0)
        throw std::runtime_error("Offsets have not been correctly set, cannot proceed.");

    inGame = false;
    isC4Planted = false;

    localPlayer = process->read<uintptr_t>(base_client.base + updater::offsets::dwLocalPlayerController);
    if (!localPlayer) return;

    localPlayerPawn = process->read<std::uint32_t>(localPlayer + updater::offsets::m_hPlayerPawn);
    if (!localPlayerPawn) return;

    entity_list = process->read<uintptr_t>(base_client.base + updater::offsets::dwEntityList);

    // Güncellenmiþ kýsým - 120 yerine 112 kullanýlýyor
    localList_entry2 = process->read<uintptr_t>(entity_list + 0x8 * ((localPlayerPawn & 0x7FFF) >> 9) + 0x10);
    localpCSPlayerPawn = process->read<uintptr_t>(localList_entry2 + 112 * (localPlayerPawn & 0x1FF));
    if (!localpCSPlayerPawn) return;

    view_matrix = process->read<view_matrix_t>(base_client.base + updater::offsets::dwViewMatrix);

    localTeam = process->read<int>(localPlayer + updater::offsets::m_iTeamNum);
    localOrigin = process->read<Vector3>(localpCSPlayerPawn + updater::offsets::m_vOldOrigin);
    isC4Planted = process->read<bool>(base_client.base + updater::offsets::dwPlantedC4 - 0x8);

    inGame = true;
    int playerIndex = 0;
    std::vector<CPlayer> list;
    CPlayer player;
    uintptr_t list_entry, list_entry2, playerPawn, playerMoneyServices, clippingWeapon, weaponData;

    while (true) {
        playerIndex++;
        // Güncellenmiþ kýsým - +16 yerine +0x10 ve 120 yerine 112
        list_entry = process->read<uintptr_t>(entity_list + (8 * (playerIndex & 0x7FFF) >> 9) + 0x10);
        if (!list_entry) break;

        player.entity = process->read<uintptr_t>(list_entry + 112 * (playerIndex & 0x1FF));
        if (!player.entity) continue;

        player.team = process->read<int>(player.entity + updater::offsets::m_iTeamNum);
        if (config::team_esp && (player.team == localTeam)) continue;

        playerPawn = process->read<std::uint32_t>(player.entity + updater::offsets::m_hPlayerPawn);

        // Güncellenmiþ kýsým - +16 yerine +0x10
        list_entry2 = process->read<uintptr_t>(entity_list + 0x8 * ((playerPawn & 0x7FFF) >> 9) + 0x10);
        if (!list_entry2) continue;

        // Güncellenmiþ kýsým - 120 yerine 112
        player.pCSPlayerPawn = process->read<uintptr_t>(list_entry2 + 112 * (playerPawn & 0x1FF));
        if (!player.pCSPlayerPawn) continue;

        player.health = process->read<int>(player.pCSPlayerPawn + updater::offsets::m_iHealth);
        player.armor = process->read<int>(player.pCSPlayerPawn + updater::offsets::m_ArmorValue);
        if (player.health <= 0 || player.health > 100) continue;

        if (config::team_esp && (player.pCSPlayerPawn == localPlayer)) continue;

        // Read entity controller from the player pawn
        uintptr_t handle = process->read<std::uintptr_t>(player.pCSPlayerPawn + updater::offsets::m_hController);
        int index = handle & 0x7FFF;
        int segment = index >> 9;
        int entry = index & 0x1FF;

        uintptr_t controllerListSegment = process->read<uintptr_t>(entity_list + 0x8 * segment + 0x10);
        uintptr_t controller = process->read<uintptr_t>(controllerListSegment + 112 * entry);

        if (!controller)
            continue;

        // Read player name from the controller
        char buffer[256] = {};
        process->read_raw(controller + updater::offsets::m_iszPlayerName, buffer, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        player.name = buffer;

        player.gameSceneNode = process->read<uintptr_t>(player.pCSPlayerPawn + updater::offsets::m_pGameSceneNode);
        player.origin = process->read<Vector3>(player.pCSPlayerPawn + updater::offsets::m_vOldOrigin);

        // Kafa pozisyonunu hassas hesapla
        bool needsBones = config::aimbot_enabled || config::show_skeleton_esp || config::show_head_tracker;
        if (needsBones) {
            player.gameSceneNode = process->read<uintptr_t>(player.pCSPlayerPawn + updater::offsets::m_pGameSceneNode);
            player.boneArray = process->read<uintptr_t>(player.gameSceneNode + 0x210);

            // Kafa kemiðinden gerçek kafa pozisyonunu al
            uintptr_t headBoneAddress = player.boneArray + 6 * 32;
            Vector3 headBonePos = process->read<Vector3>(headBoneAddress);
            player.head = headBonePos;
        }
        else {
            player.head = { player.origin.x, player.origin.y, player.origin.z + 75.f };
        }

        if (player.origin.x == localOrigin.x && player.origin.y == localOrigin.y && player.origin.z == localOrigin.z)
            continue;

        if (config::render_distance != -1 && (localOrigin - player.origin).length2d() > config::render_distance) continue;
        if (player.origin.x == 0 && player.origin.y == 0) continue;

        // Bone array
        if (config::show_skeleton_esp) {
            player.ReadBones();
        }

        if (config::show_head_tracker && !config::show_skeleton_esp) {
            player.ReadHead();
        }

        if (config::show_extra_flags) {
            player.is_defusing = process->read<bool>(player.pCSPlayerPawn + updater::offsets::m_bIsDefusing);

            playerMoneyServices = process->read<uintptr_t>(player.entity + updater::offsets::m_pInGameMoneyServices);
            player.money = process->read<int32_t>(playerMoneyServices + updater::offsets::m_iAccount);

            player.flashAlpha = process->read<float>(player.pCSPlayerPawn + updater::offsets::m_flFlashOverlayAlpha);

            clippingWeapon = process->read<std::uint64_t>(player.pCSPlayerPawn + updater::offsets::m_pClippingWeapon);
            std::uint64_t firstLevel = process->read<std::uint64_t>(clippingWeapon + 0x10);
            weaponData = process->read<std::uint64_t>(firstLevel + 0x20);
            char buffer[MAX_PATH];
            process->read_raw(weaponData, buffer, sizeof(buffer));
            std::string weaponName = std::string(buffer);
            if (weaponName.compare(0, 7, "weapon_") == 0)
                player.weapon = weaponName.substr(7, weaponName.length());
            else
                player.weapon = "Invalid Weapon Name";
        }

        list.push_back(player);
    }

    players.clear();
    players.assign(list.begin(), list.end());

    // Aimbot - Sol fare tuþu basýlý tutulduðunda
    if (config::aimbot_enabled && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        aimbot.SmoothAimToTarget();
    }
}

uintptr_t boneAddress;
Vector3 bonePosition;
int boneIndex;
void CPlayer::ReadHead() {
    boneAddress = boneArray + 6 * 32;
    bonePosition = g_game.process->read<Vector3>(boneAddress);
    bones.bonePositions["head"] = g_game.world_to_screen(&bonePosition);
}

void CPlayer::ReadBones() {
    for (const auto& entry : boneMap) {
        const std::string& boneName = entry.first;
        boneIndex = entry.second;
        boneAddress = boneArray + boneIndex * 32;
        bonePosition = g_game.process->read<Vector3>(boneAddress);
        bones.bonePositions[boneName] = g_game.world_to_screen(&bonePosition);
    }
}

Vector3 CGame::world_to_screen(Vector3* v) {
    float _x = view_matrix[0][0] * v->x + view_matrix[0][1] * v->y + view_matrix[0][2] * v->z + view_matrix[0][3];
    float _y = view_matrix[1][0] * v->x + view_matrix[1][1] * v->y + view_matrix[1][2] * v->z + view_matrix[1][3];

    float w = view_matrix[3][0] * v->x + view_matrix[3][1] * v->y + view_matrix[3][2] * v->z + view_matrix[3][3];

    float inv_w = 1.f / w;
    _x *= inv_w;
    _y *= inv_w;

    float x = game_bounds.right * .5f;
    float y = game_bounds.bottom * .5f;

    x += 0.5f * _x * game_bounds.right + 0.5f;
    y -= 0.5f * _y * game_bounds.bottom + 0.5f;

    return { x, y, w };
}