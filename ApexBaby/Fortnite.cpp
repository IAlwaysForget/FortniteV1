#include <iostream>
#include <random>
#include <thread>

#include "context.h"
#include "Config/Config.h"
#include "Fuser/Fuser.h"
#include <DMALibrary/Memory/Memory.h>

#include "imgui_internal.h"
#include "Cheat/MainThread.h"
#include "Cheat/Classes/Camera.h"
#include "Cheat/Classes/Vector.h"
#include "Cheat/Globals.h"
#include "Cheat/Aimbot.h"
#include "Utils/Mouse/kmbox.h"
//#include "Vorpal/API/Vorpal.h"
//#include "Vorpal/HWID/HWID.h"

#include <unordered_map>

c_context ctx;
#define INIT_TIMER auto start2 = std::chrono::high_resolution_clock::now();
#define START_TIMER  start2 = std::chrono::high_resolution_clock::now();
#define STOP_TIMER(name)  std::cout << "RUNTIME of " << name << ": " << \
std::fixed << std::setprecision(2) << \
std::chrono::duration<double, std::chrono::milliseconds::period>( \
std::chrono::high_resolution_clock::now()-start2 \
).count() << " ms " << std::endl;

ImColor get_team_color(int teamNumber);

CEntity m_localplayer = NULL;

/*
 *If we want we can read from AActor weapon data like "IsReloadingWeapon, IsChargingWeapon, IsEquippingData
 *BOOLEANS:
 * 0x0348 - bIsEquippingWeapon
 * 0x0360 - bIsReloadingWeapon
 * 0x0361 - bIsChargingWeapon
 * 0x0364 - bIsAimingConsumable
 *
 *
 *INT32
 * 0x0DC8 - WeaponLevel
 * 0x0DCC - AmmoCount
 */

namespace RGBColours
{
	inline ULONGLONG LastRGBTime = 0;
	inline std::vector<ImColor> RGBColours = {
		ImColor(255, 0, 0), ImColor(255, 150, 0), ImColor(255, 255, 0), ImColor(0, 150, 0), ImColor(0, 255, 0), ImColor(0, 255, 150), ImColor(0, 255, 255), ImColor(0, 0, 150), ImColor(0, 0, 255), ImColor(255, 0, 150), ImColor(255, 0, 255), ImColor(255, 150, 255), ImColor(255, 255, 255), ImColor(255, 150, 150), ImColor(255, 150, 0),
		ImColor(255, 0, 0)
	};
	inline int RGBPhase = 0;
	inline ImColor RGBColour;
}

void DrawPlayers()
{
	auto tmp_list = entity_list.GetPlayers();
	entity_list.FastCache(tmp_list);
	//camera::Update();

	auto drawList = ImGui::GetBackgroundDrawList();
	Aim::Run(tmp_list, drawList);

	float heighest_distance = 0.f;
	for (size_t i = 0; i < tmp_list.size(); i++)
	{
		auto entity = tmp_list[i];
		if (!entity.mesh)
			continue;

		if (entity.pawn_private == globals::local::pawn)
		{
			m_localplayer = entity;
			continue;
		}
		float distance = m_localplayer.bone_positions.base.Distance(entity.bone_positions.base) / 100.f;
		if (heighest_distance < distance)
			heighest_distance = distance;
	}

	for (size_t i = 0; i < tmp_list.size(); i++)
	{
		auto entity = tmp_list[i];
		if (!entity.mesh)
			continue;

		if (entity.pawn_private == globals::local::pawn)
			continue;

		//TODO: make it nice, and rotate the radar based on view angle

		bool is_team = false;
		if (globals::local::team_index == entity.team_index) //skip teammates, maybe add option to color differently
			is_team = true;
		if (is_team && !ctx.m_settings.visuals_team_check)
			continue;

		auto base_pos = camera::WorldToScreen(entity.bone_positions.base);
		if (!camera::IsOnScreen(base_pos))
			continue;

		vec3_t world_top = entity.bone_positions.head;
		auto head_pos = camera::WorldToScreen(world_top);
		world_top.z += 12.0f;

		auto s_head_pos = camera::WorldToScreen(world_top);
		if (!camera::IsOnScreen(s_head_pos))
			continue;

		float distance = m_localplayer.bone_positions.base.Distance(entity.bone_positions.base) / 100.f;
		if (distance > ctx.m_settings.visuals_max_distance) //Distance check lol
			continue;

		auto distance_loc = entity.bone_positions.base;

		float height = abs(base_pos.y - s_head_pos.y);
		auto width = height / ctx.m_settings.visuals_esp_box_width;
		float thickness = ctx.m_settings.visuals_esp_box_thickness;
		bool is_visible = entity.IsVisible();
		bool is_bot = (strcmp(entity.decrypted_name.c_str(), "Bot") == 0);

		vec2_t name_pos = {base_pos.x, base_pos.y - height - 10};
		vec2_t weapon_pos = {base_pos.x, base_pos.y + 10};
		vec2_t distance_pos = {base_pos.x, base_pos.y + 10};
		if (ctx.m_settings.visuals_weapon() && !entity.weapon_name.empty())
			distance_pos.y += 13; // Can do text size ofc

		if (ctx.m_settings.visuals_filled_esp_box)
		{
			ImColor filled_color = !is_bot ? ImColor(ctx.m_settings.visuals_filled_esp_box_color) : ImColor(ctx.m_settings.visuals_filled_esp_box_color_bot);
			if (is_visible)
				filled_color = !is_bot ? ImColor(ctx.m_settings.visuals_filled_esp_box_color_visible) : ImColor(ctx.m_settings.visuals_filled_esp_box_color_visible_bot);

			drawList->AddRectFilled(ImVec2(base_pos.x - (width / 2), s_head_pos.y), ImVec2(base_pos.x + (width / 2), base_pos.y), filled_color);
		}

		if (ctx.m_settings.visuals_esp_box)
		{
			ImColor box_color = !is_bot ? is_team ? ImColor(ctx.m_settings.visuals_team_color) : ImColor(ctx.m_settings.visuals_esp_box_color) : ImColor(ctx.m_settings.visuals_esp_box_color_bot);
			if (is_visible)
				box_color = !is_bot ? is_team ? ImColor(ctx.m_settings.visuals_team_color_visible) : ImColor(ctx.m_settings.visuals_esp_box_color_visible) : ImColor(ctx.m_settings.visuals_esp_box_color_visible_bot);
			switch (ctx.m_settings.visuals_esp_box_type)
			{
			case box_type::NORMAL:
				drawList->AddRect(ImVec2(base_pos.x - (width / 2), s_head_pos.y), ImVec2(base_pos.x + (width / 2), base_pos.y), box_color, 0, 0, thickness);
				break;
			case box_type::CORNER:
				{
					//Draw corner box
					drawList->AddLine(ImVec2(base_pos.x - (width / 2), s_head_pos.y), ImVec2(base_pos.x - (width / 2) + (width / 4), s_head_pos.y), box_color, thickness);
					drawList->AddLine(ImVec2(base_pos.x - (width / 2), s_head_pos.y), ImVec2(base_pos.x - (width / 2), s_head_pos.y + (height / 4)), box_color, thickness);
					drawList->AddLine(ImVec2(base_pos.x + (width / 2), s_head_pos.y), ImVec2(base_pos.x + (width / 2) - (width / 4), s_head_pos.y), box_color, thickness);
					drawList->AddLine(ImVec2(base_pos.x + (width / 2), s_head_pos.y), ImVec2(base_pos.x + (width / 2), s_head_pos.y + (height / 4)), box_color, thickness);
					drawList->AddLine(ImVec2(base_pos.x - (width / 2), base_pos.y), ImVec2(base_pos.x - (width / 2) + (width / 4), base_pos.y), box_color, thickness);
					drawList->AddLine(ImVec2(base_pos.x - (width / 2), base_pos.y), ImVec2(base_pos.x - (width / 2), base_pos.y - (height / 4)), box_color, thickness);
					drawList->AddLine(ImVec2(base_pos.x + (width / 2), base_pos.y), ImVec2(base_pos.x + (width / 2) - (width / 4), base_pos.y), box_color, thickness);
					drawList->AddLine(ImVec2(base_pos.x + (width / 2), base_pos.y), ImVec2(base_pos.x + (width / 2), base_pos.y - (height / 4)), box_color, thickness);
				}
				break;
			}
		}

		if (ctx.m_settings.visuals_snapline > 0)
		{
			ImVec2 size = ctx.m_settings.menu_size;
			ImColor color = !is_bot ? ImColor(ctx.m_settings.visuals_snapline_color) : ImColor(ctx.m_settings.visuals_snapline_color_bot);

			if (ctx.m_settings.visuals_snapline == 1)
				ctx.m_draw->DrawLine({base_pos.x, base_pos.y}, {size.x / 2, (float)size.y}, color, 1.0f);
			else if (ctx.m_settings.visuals_snapline == 2)
				ctx.m_draw->DrawLine({base_pos.x, base_pos.y}, {size.x / 2, (float)size.y / 2}, color, 1.0f);
		}

		if (ctx.m_settings.visuals_name && camera::IsOnScreen(name_pos))
		{
			ImColor name_color = !is_bot ? ImColor(ctx.m_settings.visuals_name_color) : ImColor(ctx.m_settings.visuals_name_color_bot);

			if (ctx.m_settings.visuals_team_unique_colors)
			{
				name_color = get_team_color(entity.team_index);
			}

			if (is_visible)
				name_color = !is_bot ? ImColor(ctx.m_settings.visuals_name_color_visible) : ImColor(ctx.m_settings.visuals_name_color_visible_bot);

			//if (tmp_list.size() > 2) // This shit is beyond ugly removed it , on lower end fusers your colors will fade to black - ringo
			//name_color.Value.w = ImLerp<float>(1.f, 0.f, distance / (heighest_distance + 1.f));

			if (distance < ctx.m_settings.visuals_esp_name_render_distance)
			{
				ImVec2 name_size = ImGui::CalcTextSize(entity.decrypted_name.c_str());
				//center text
				ImVec2 center_name = ImVec2(name_pos.x - (name_size.x / 2), name_pos.y - (name_size.y / 2));
				ctx.m_draw->DrawString(entity.decrypted_name.c_str(), ImVec2(center_name.x, center_name.y), name_color);
			}
		}

		if (ctx.m_settings.visuals_distance)
		{
			ImColor distance_color = !is_bot ? ImColor(ctx.m_settings.visuals_distance_color) : ImColor(ctx.m_settings.visuals_distance_color_bot);

			if (ctx.m_settings.visuals_team_unique_colors)
			{
				distance_color = get_team_color(entity.team_index);
			}

			if (is_visible)
				distance_color = !is_bot ? ImColor(ctx.m_settings.visuals_distance_color_visible) : ImColor(ctx.m_settings.visuals_distance_color_visible_bot);
			if (distance < ctx.m_settings.visuals_esp_name_render_distance)
			{
				std::string distance_str = "[" + std::to_string((int)std::round(distance)) + "m]";
				ImVec2 text_size = ImGui::CalcTextSize(distance_str.c_str());
				//distance_color.Value.w = ImLerp<float>(1.f, 0.f, distance / (heighest_distance + 1.f)); //no no no no - ringo
				ImVec2 middle = ImVec2(distance_pos.x - (text_size.x / 2), distance_pos.y - (text_size.y / 2));
				ctx.m_draw->DrawString(distance_str.c_str(), middle, distance_color);
			}
		}

		if (ctx.m_settings.visuals_weapon)
		{
			ImColor weapon_color = !is_bot ? ImColor(ctx.m_settings.visuals_weapon_color) : ImColor(ctx.m_settings.visuals_weapon_color_bot);

			if (ctx.m_settings.visuals_team_unique_colors)
			{
				weapon_color = get_team_color(entity.team_index);
			}

			if (is_visible)
				weapon_color = !is_bot ? ImColor(ctx.m_settings.visuals_weapon_color_visible) : ImColor(ctx.m_settings.visuals_weapon_color_visible_bot);

			if (distance < ctx.m_settings.visuals_esp_name_render_distance)
			{
				std::string weapon_str = entity.weapon_name;
				ImVec2 text_size = ImGui::CalcTextSize(weapon_str.c_str());
				//weapon_color.Value.w = ImLerp<float>(1.f, 0.f, weapon / (heighest_weapon + 1.f)); //no no no no - ringo
				ImVec2 middle = ImVec2(weapon_pos.x - (text_size.x / 2), weapon_pos.y - (text_size.y / 2));
				ctx.m_draw->DrawString(weapon_str.c_str(), middle, weapon_color);
			}
		}

		if (ctx.m_settings.visuals_head_pos)
		{
			ImColor head_pos_color = !is_bot ? ImColor(ctx.m_settings.visuals_head_pos_color) : ImColor(ctx.m_settings.visuals_head_pos_color_bot);
			if (is_visible)
				head_pos_color = !is_bot ? ImColor(ctx.m_settings.visuals_head_pos_color_visible) : ImColor(ctx.m_settings.visuals_head_pos_color_visible_bot);
			drawList->AddCircleFilled(ImVec2(head_pos.x, head_pos.y), ctx.m_settings.visuals_head_pos_size, head_pos_color);
		}

		if (ctx.m_settings.visuals_skeleton)
		{
			//For whatever reason these bones never change, but they do change the position of where the bone is... it's so fucking goblins idk
			vec2_t screen_right_hand = camera::WorldToScreen(entity.bone_positions.right_hand);
			vec2_t screen_right_collar_bone = camera::WorldToScreen(entity.bone_positions.right_collar_bone);
			vec2_t screen_left_hand = camera::WorldToScreen(entity.bone_positions.left_hand);
			vec2_t screen_left_collar_bone = camera::WorldToScreen(entity.bone_positions.left_collar_bone);
			vec2_t screen_chest = camera::WorldToScreen(entity.bone_positions.chest);
			vec2_t screen_spine2 = camera::WorldToScreen(entity.bone_positions.spine2);
			vec2_t screen_spine1 = camera::WorldToScreen(entity.bone_positions.spine1);
			vec2_t screen_right_thigh = camera::WorldToScreen(entity.bone_positions.right_thigh);
			vec2_t screen_left_thigh = camera::WorldToScreen(entity.bone_positions.left_thigh);
			vec2_t screen_right_calf = camera::WorldToScreen(entity.bone_positions.right_calf);
			vec2_t screen_left_calf = camera::WorldToScreen(entity.bone_positions.left_calf);

			auto line = [](bool visible, bool is_bot, vec2_t a, vec2_t b)
			{
				ImColor color = !is_bot ? ImColor(ctx.m_settings.visuals_skeleton_color) : ImColor(ctx.m_settings.visuals_skeleton_color_bot);
				if (visible)
					color = !is_bot ? ImColor(ctx.m_settings.visuals_skeleton_color_visible) : ImColor(ctx.m_settings.visuals_skeleton_color_visible_bot);
				ctx.m_draw->DrawLine({a.x, a.y}, {b.x, b.y}, color, ctx.m_settings.visuals_skeleton_thickness);
			};

			line(is_visible, is_bot, screen_right_collar_bone, screen_right_hand);
			line(is_visible, is_bot, screen_left_collar_bone, screen_left_hand);
			line(is_visible, is_bot, screen_chest, screen_left_collar_bone);
			line(is_visible, is_bot, screen_chest, screen_right_collar_bone);
			line(is_visible, is_bot, head_pos, screen_chest);
			line(is_visible, is_bot, screen_spine2, screen_chest);
			line(is_visible, is_bot, screen_spine1, screen_spine2);
			line(is_visible, is_bot, screen_spine1, screen_right_thigh);
			line(is_visible, is_bot, screen_spine1, screen_left_thigh);
			line(is_visible, is_bot, screen_right_thigh, screen_right_calf);
			line(is_visible, is_bot, screen_left_thigh, screen_left_calf);
		}
	}

	if (ctx.m_settings.visuals_2d_radar)
	{
		if (ctx.m_settings.visuals_2d_radar_disco)
		{
			if (RGBColours::LastRGBTime < GetTickCount64())
			{
				RGBColours::RGBColour = RGBColours::RGBColours[RGBColours::RGBPhase];
				if (RGBColours::RGBPhase < RGBColours::RGBColours.size() - 1)
					RGBColours::RGBPhase++;
				else
					RGBColours::RGBPhase = 0;

				RGBColours::LastRGBTime = GetTickCount64() + 350;
			}
		}

		static ImGuiWindowFlags flag = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;
		ImVec2 size = ctx.m_settings.menu_size;
		ImVec2 radar_size = ImVec2(ctx.m_settings.visuals_2d_radar_size, ctx.m_settings.visuals_2d_radar_size);
		ImVec2 settings_pos = ctx.m_settings.visuals_2d_radar_pos;
		ImVec2 radar_pos = ImVec2(size.x - (radar_size.x + 50.f), 50);
		if (settings_pos.x != 0.1f && settings_pos.y != 0.1f)
			radar_pos = settings_pos;

		ImGui::SetNextWindowPos(radar_pos, ImGuiCond_Once);
		ImGui::SetNextWindowSize(radar_size, ImGuiCond_Always);
		ImGui::Begin(xorstr_("Radar"), nullptr, flag);
		auto pos = ImGui::GetCursorScreenPos();
		radar_size = ImGui::GetContentRegionAvail();
		if (ctx.m_settings.visuals_2d_radar_move)
		{
			if (settings_pos.x != pos.x || settings_pos.y != pos.y)
				ctx.m_settings.Set<ImVec2>(&ctx.m_settings.visuals_2d_radar_pos, pos);
			flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;
		}
		else
			flag = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;
		ImColor bordercol = ctx.m_settings.visuals_2d_radar_border_colour;
		ImColor backgroundcol = ctx.m_settings.visuals_2d_radar_background_colour;
		drawList->AddRect(pos, ImVec2(pos.x + radar_size.x, pos.y + radar_size.y), ctx.m_settings.visuals_2d_radar_disco ? RGBColours::RGBColour : bordercol);
		if (ctx.m_settings.visuals_2d_radar_background)
			drawList->AddRectFilled(pos, ImVec2(pos.x + radar_size.x, pos.y + radar_size.y), backgroundcol);
		for (size_t i = 0; i < tmp_list.size(); i++)
		{
			auto entity = tmp_list[i];
			if (!entity.mesh)
				continue;

			if (entity.pawn_private == globals::local::pawn)
				continue;
			ImVec2 enemyWorldPos = ImVec2((float)entity.bone_positions.base.x, (float)entity.bone_positions.base.y);

			//ImVec2 relativePos = ImVec2(enemyWorldPos.x - (float)m_localplayer.bone_positions.base.x, enemyWorldPos.y - (float)m_localplayer.bone_positions.base.y);
			// Adjust the scaling factor to display more players
			ImVec2 relativePos = ImVec2(
				(enemyWorldPos.x - (float)m_localplayer.bone_positions.base.x) * 0.05f,
				(enemyWorldPos.y - (float)m_localplayer.bone_positions.base.y) * 0.05f
			);
			float scale = ctx.m_settings.visuals_2d_radar_scaling;
			ImVec2 enemyRadarPos = ImVec2(
				pos.x + radar_size.x * 0.5f + relativePos.x * scale,
				pos.y + radar_size.y * 0.5f + relativePos.y * scale
			);

			//local player dot

			ImColor friendcol = ctx.m_settings.visuals_2d_radar_friend_colour;
			ImColor enemycol = ctx.m_settings.visuals_2d_radar_enemy_colour;

			// Check if the enemy is within the radar boundaries
			if (enemyRadarPos.x >= pos.x && enemyRadarPos.x <= pos.x + radar_size.x &&
				enemyRadarPos.y >= pos.y && enemyRadarPos.y <= pos.y + radar_size.y)
			{
				if (globals::local::team_index == entity.team_index)
				{
					if (ctx.m_settings.visuals_2d_radar_draw_friend)
					{
						drawList->AddCircleFilled(enemyRadarPos, 3.0f, friendcol); // Green dot for teammate
						std::string distance = std::to_string((int)camera::GetDistance(entity.bone_positions.base)) + xorstr_("m");
						ImVec2 distancesize = ImGui::CalcTextSize(distance.c_str());
						ImVec2 middledistance = ImVec2((enemyRadarPos.x - (distancesize.x / 2)) + 3, enemyRadarPos.y + (ctx.m_settings.visuals_2d_radar_font_size / 2));
						ImVec2 weaponsize = ImGui::CalcTextSize(entity.weapon_name.c_str());
						ImVec2 middleweapon = ImVec2((enemyRadarPos.x - (weaponsize.x / 2) + 3), enemyRadarPos.y + (ctx.m_settings.visuals_2d_radar_font_size * 1.5));
						if (ctx.m_settings.visuals_2d_radar_draw_distance())
						ctx.m_draw->DrawString(distance, ctx.m_settings.visuals_2d_radar_font_size, middledistance, enemycol);
						if (ctx.m_settings.visuals_2d_radar_draw_weapon())
						ctx.m_draw->DrawString(entity.weapon_name, ctx.m_settings.visuals_2d_radar_font_size, middleweapon, enemycol);
					}
				}
				else
				{
					if (ctx.m_settings.visuals_2d_radar_draw_enemy)
					{
						drawList->AddCircleFilled(enemyRadarPos, 3.0f, enemycol); // Red dot for enemy
						std::string distance = std::to_string((int)camera::GetDistance(entity.bone_positions.base)) + xorstr_("m");
						ImVec2 distancesize = ImGui::CalcTextSize(distance.c_str());
						ImVec2 middledistance = ImVec2((enemyRadarPos.x - (distancesize.x / 2)) + 3, enemyRadarPos.y + (ctx.m_settings.visuals_2d_radar_font_size / 2));
						ImVec2 weaponsize = ImGui::CalcTextSize(entity.weapon_name.c_str());
						ImVec2 middleweapon = ImVec2((enemyRadarPos.x - (weaponsize.x / 2) + 3), enemyRadarPos.y + (ctx.m_settings.visuals_2d_radar_font_size * 1.5));
						if (ctx.m_settings.visuals_2d_radar_draw_distance())
						ctx.m_draw->DrawString(distance, ctx.m_settings.visuals_2d_radar_font_size, middledistance, enemycol);
						if(ctx.m_settings.visuals_2d_radar_draw_weapon())
						ctx.m_draw->DrawString(entity.weapon_name, ctx.m_settings.visuals_2d_radar_font_size, middleweapon, ctx.m_settings.visuals_2d_radar_disco ? RGBColours::RGBColour : enemycol);
					}
				}
			}
			ImVec2 localPlayerRadarPos = ImVec2(
				pos.x + radar_size.x * 0.5f,
				pos.y + radar_size.y * 0.5f
			);
			ImColor localplayercol = ctx.m_settings.visuals_2d_radar_localplayer_colour;
			drawList->AddCircleFilled(localPlayerRadarPos, 3.0f, localplayercol); // dot for local player
			float line_length = localPlayerRadarPos.y * 0.08f;
			float angle_rad = camera::ComputeZRotation() * 3.14159f / 180.0f;
			angle_rad += 90;
			ImVec2 line_end(localPlayerRadarPos.x + std::cos(angle_rad) * line_length, localPlayerRadarPos.y + std::sin(angle_rad) * line_length);
			drawList->AddLine(localPlayerRadarPos, line_end, localplayercol, ctx.m_settings.visuals_2d_radar_localplayer_line_width);
		}
		ImGui::End();
	}

	Aim::DrawAimbot();

	ctx.m_draw->Run();
}

//#define DO_AUTH
#ifdef DO_AUTH
#endif

int main()
{
	srand(time(NULL));
#ifdef DO_AUTH
	char username[50];
	char password[50];
	auto currentPath = std::filesystem::temp_directory_path();
	if (std::filesystem::exists(currentPath.string() + xorstr_("\\login.txt")))
	{
		std::ifstream infile(currentPath.string() + xorstr_("\\login.txt"));

		std::string line;
		std::getline(infile, line);
		if (line.find(xorstr_(":")) != std::string::npos)
		{
			std::string User = line.substr(0, line.find(":"));
			std::string Pass = line.substr(line.find(":") + 1);
			User.copy(username, User.size() + 1);
			Pass.copy(password, Pass.size() + 1);

			// null-terminate the char arrays
			username[User.size()] = '\0';
			password[Pass.size()] = '\0';
		}

		infile.close();
	}
	ctx.m_logger->Info(xorstr_("Username: %s\n"), username);
	ctx.m_logger->Info(xorstr_("Logging in...\n"));
	ctx.m_logger->Info(xorstr_("Logged in...\n"));
	ctx.m_logger->Info(xorstr_("Finding application...\n"));
	ctx.m_logger->Info(xorstr_("login but second time...\n"));
	ctx.m_logger->Info(xorstr_("logged in but second time...\n"));
	ctx.m_logger->Info(xorstr_("hashed id matches\n"));
										
#endif
	ctx.m_logger->Succes(xorstr_("We in baby\n"));
	while (!mem.Init(xorstr_("FortniteClient-Win64-Shipping.exe")))
	{
		ctx.m_logger->Error(xorstr_("Failed to find \"\FortniteClient-Win64-Shipping.exe\"\ retrying in a second\n"));

		Sleep(1000);
	}

	//mem.DumpMemory(mem.GetBaseDaddy("FortniteClient-Win64-Shipping.exe"), "D:\\dumpy_2.exe");
	if (!mem.GetKeyboard()->InitKeyboard())
		ctx.m_logger->Error(xorstr_("Failed to find keyboard\n"));
	else
		ctx.m_logger->Succes(xorstr_("Initialized keyboard\n"));
	ctx.m_logger->Info(xorstr_("Initializing Cheat...\n"));
	std::thread mt(cheeto::main_thread);

	ctx.m_logger->Info(xorstr_("Loading Default Config\n"));
	if (!ctx.m_settings.exists(xorstr_("Default.conf")))
		ctx.m_settings.save(xorstr_("Default.conf"));
	ctx.m_settings.load(xorstr_("Default.conf"));
	ctx.m_logger->Succes(xorstr_("Loaded Default Config\n"));
	auto comport = kmbox::find_port(xorstr_("USB-SERIAL CH340"));
	ctx.m_logger->Info(xorstr_("Maybe This is your Comport: %s\n"), comport.c_str());

	if (!comport.empty())
	{
		ctx.m_settings.Set<std::string>(&ctx.m_settings.hardware_mouse_com_port, std::string(comport));
		kmbox::initialize(comport.c_str());
	}
	fuser.Initialize(ctx.m_settings.menu_size);
	ctx.m_draw->Init(fuser); //Initialize font ect after we have initialized fuser.
	fuser.Run(&DrawPlayers);

	mt.join();
#ifdef DO_AUTH
	MessageBoxA(NULL, m_vorpal.LoginInfo.Error.c_str(), xorstr_("ERROR"), MB_OK);
#endif
	return 0;
}

std::unordered_map<int, ImColor> teamColorMap;

ImColor get_team_color(int teamNumber)
{
	std::seed_seq seed {teamNumber};
	std::mt19937 generator(seed);
	std::uniform_int_distribution<int> distribution(150, 206);
	int r = distribution(generator);
	generator.seed(teamNumber * r);
	int g = distribution(generator);
	generator.seed(teamNumber * g);
	int b = distribution(generator);
	return ImColor(r, g, b, 255);
}
