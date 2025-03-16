#include "Aimbot.h"

#include <iostream>
#include <random>
#include <unordered_map>

#include "context.h"
#include "Globals.h"
#include "MainThread.h"
#include "Offsets.h"
#include "Classes/Camera.h"
#include "Classes/Entity.h"
#include "Classes/Vector.h"
#include "DMALibrary/Memory/Memory.h"
#include "Utils/PerlinNoise.hpp"
#include "Utils/Mouse/kmbox.h"
#include "Utils/Mouse/kmboxNet.h"
//#include "Utils/Mouse/PicoW.h"

namespace Aim
{
	struct weapon_projectiles
	{
		float speed;
		float max_speed;
		float gravity_scale;
	};

	enum class AimbotType
	{
		Sniper,
		Launcher,
		Bow,
		Pistol,
		SMG,
		Shotgun,
		Rifle,
		Other
	};

	inline bool AimbotToggle = true;
	inline bool TriggerbotToggle = true;
	AimbotConfig CurrentAimbotConfig;
	AimbotType CurrentAimbotType;

	AimbotConfig GetAimbotConfig()
	{
		globals::local::weapon_name_mutex.lock();
		std::string tempweaponname = globals::local::weapon_name;
		globals::local::weapon_name_mutex.unlock();
		if (tempweaponname.find(xorstr_("Sniper Rifle")) != std::string::npos)
		{
			CurrentAimbotType = AimbotType::Sniper;
			return ctx.m_settings.SniperAimbot;
		}
		else if (tempweaponname == xorstr_("Hunting Rifle"))
		{
			CurrentAimbotType = AimbotType::Sniper;
			return ctx.m_settings.SniperAimbot;
		}
		else if (tempweaponname.find(xorstr_(" Launcher")) != std::string::npos)
		{
			CurrentAimbotType = AimbotType::Launcher;
			return ctx.m_settings.LauncherAimbot;
		}
		else if (tempweaponname.find(xorstr_(" Bow")) != std::string::npos)
		{
			CurrentAimbotType = AimbotType::Bow;
			return ctx.m_settings.BowAimbot;
		}
		else if (tempweaponname.find(xorstr_(" Boom Bolt")) != std::string::npos)
		{
			CurrentAimbotType = AimbotType::Bow;
			return ctx.m_settings.BowAimbot;
		}
		else if (tempweaponname.find(xorstr_(" Pistol")) != std::string::npos)
		{
			CurrentAimbotType = AimbotType::Pistol;
			return ctx.m_settings.PistolAimbot;
		}
		else if (tempweaponname == xorstr_("Hand Cannon"))
		{
			CurrentAimbotType = AimbotType::Pistol;
			return ctx.m_settings.PistolAimbot;
		}
		else if (tempweaponname == xorstr_("Submachine Gun"))
		{
			CurrentAimbotType = AimbotType::SMG;
			return ctx.m_settings.SMGAimbot;
		}
		else if (tempweaponname.find(xorstr_(" Drum Gun")) != std::string::npos)
		{
			// smg
			CurrentAimbotType = AimbotType::SMG;
			return ctx.m_settings.SMGAimbot;
		}
		else if (tempweaponname.find(xorstr_(" SMG")) != std::string::npos)
		{
			CurrentAimbotType = AimbotType::SMG;
			return ctx.m_settings.SMGAimbot;
		}
		else if (tempweaponname.find(xorstr_(" Shotgun")) != std::string::npos)
		{
			CurrentAimbotType = AimbotType::Shotgun;
			return ctx.m_settings.ShotgunAimbot;
		}
		else if (tempweaponname.find(xorstr_(" Assault Rifle")) != std::string::npos)
		{
			CurrentAimbotType = AimbotType::Rifle;
			return ctx.m_settings.RifleAimbot;
		}
		else if (tempweaponname.find(xorstr_(" AR")) != std::string::npos)
		{
			CurrentAimbotType = AimbotType::Rifle;
			return ctx.m_settings.RifleAimbot;
		}
		CurrentAimbotType = AimbotType::Other;
		return ctx.m_settings.OtherAimbot;
		//	std::string name = globals::local::weapon_name;
		//printf("Weapon Name -> %s\n", name.c_str());
	}

	void DrawAimbot()
	{
		if (ctx.m_settings.visuals_fov_circle)
		{
			if (CurrentAimbotConfig.Aimbot == false)
				return;
			auto drawList = ImGui::GetBackgroundDrawList();
			ImVec2 size = ctx.m_settings.menu_size;

			float fov = CurrentAimbotConfig.FOV;
			drawList->AddCircle(ImVec2((size.x / 2), (size.y / 2)), fov, ImColor(ctx.m_settings.visuals_fov_circle_color), 35);
		}
	}

	vec3_t get_closest_hitbox(CEntity entity)
	{
		std::vector<vec3_t> all_hitboxes = {entity.bone_positions.head, entity.bone_positions.chest, entity.bone_positions.pelvis};

		float best_distance = FLT_MAX;
		vec3_t best_hitbox { };

		for (auto& hitbox : all_hitboxes)
		{
			vec2_t screen_pos = camera::WorldToScreen(hitbox);
			if (!camera::IsOnScreen(screen_pos)) continue;

			float distance = camera::distance_from_crosshair(screen_pos);
			if (distance < best_distance)
			{
				best_distance = distance;
				best_hitbox = hitbox;
			}
		}

		return best_hitbox;
	}

	vec3_t hitbox_from_cfg(int hitbox_mode, CEntity entity)
	{
		vec3_t hitbox { };
		switch (hitbox_mode)
		{
		case 0:
			{
				hitbox = entity.bone_positions.head;
				break;
			}
		case 1:
			{
				hitbox = entity.bone_positions.chest;
				break;
			}
		case 2:
			{
				hitbox = entity.bone_positions.pelvis;
				break;
			}
		case 3:
			{
				hitbox = entity.bone_positions.spine1;
				break;
			}
		case 4:
			{
				hitbox = get_closest_hitbox(entity);
				break;
			}
		}

		return hitbox;
	}

	bool rules(CEntity target)
	{
		if (target.pawn_private == globals::local::pawn) return false;
		if (target.team_index == entity_list.m_localplayer.team_index && ctx.m_settings.aimbot_team) //optionally add a boolean for team check
			return false;
		if (ctx.m_settings.aimbot_vis_check && !target.IsVisible()) return false;

		return true;
	}

	struct best_tar
	{
		bool valid = false;
		CEntity entity;
		vec3_t hitbox;
		float best_fov;
	};

	best_tar GetBestTarget(std::vector<CEntity> target_list, bool triggerbot)
	{
		best_tar test;
		float best_fov = FLT_MAX;
		for (size_t i = 0; i < target_list.size(); i++)
		{
			auto entity = target_list[i];
			if (!ptr_valid(entity.pawn_private) || !ptr_valid(entity.mesh)) continue;
			if (!rules(entity)) continue;

			vec3_t hitbox = hitbox_from_cfg(CurrentAimbotConfig.HitBox1, entity);
			if (hitbox.x == 0 || hitbox.y == 0 || hitbox.z == 0)
				continue;
			vec2_t screen_pos = camera::WorldToScreen(hitbox);
			if (!camera::IsOnScreen(screen_pos)) continue;
			if (!triggerbot)
			{
				if (camera::GetDistance(entity.bone_positions.base) > CurrentAimbotConfig.MaxDistance)
					continue;
			}
			else
			{
				if (camera::GetDistance(entity.bone_positions.base) > CurrentAimbotConfig.TriggerMaxDistance)
					continue;
			}

			BYTE is_dbno = entity.isdbno >> 4 & 1;
			BYTE is_dying = entity.isdying >> 4 & 1;

			float distance = camera::distance_from_crosshair(screen_pos);
			if (distance < best_fov && !is_dying && !is_dbno)
			{
				best_fov = distance;
				test.valid = true;
				test.entity = entity;
				test.hitbox = hitbox;
				test.best_fov = best_fov;
				//	printf("Target Found\n");
			}
		}
		return test;
	}

	best_tar UpdateTarget(std::vector<CEntity> target_list, best_tar curr_target)
	{
		for (size_t i = 0; i < target_list.size(); i++)
		{
			auto player = target_list[i];
			if (player.player_state == curr_target.entity.player_state)
			{
				curr_target.entity = player;

				return curr_target;
			}
		}
		return curr_target;
	}

	float generateNoise(float x, float y)
	{
		std::random_device rd;
		siv::PerlinNoise::seed_type seed = rd();
		const siv::PerlinNoise perlin {seed};
		return perlin.octave2D_01(x, y, 4);
	}

	double ThingFunc(double t, vec3_t a1, vec3_t v_t, vec3_t p_t, double s)
	{
		return std::pow(t, 4) * (a1.dot(a1) / 4.0)
			+ std::pow(t, 3) * (a1.dot(v_t))
			+ std::pow(t, 2) * (a1.dot(p_t) + v_t.dot(v_t) - s * s)
			+ t * (2.0 * v_t.dot(p_t))
			+ p_t.dot(p_t);
	}

	vec3_t calcPrediction(double s, vec3_t p_t, vec3_t v_t, vec3_t a_t, vec3_t a_p, double tol, int steps, float min_t, float max_t)
	{
		const auto a1 = a_t - a_p;

		auto signum = [&](double d)
		{
			if (d < 0) return -1;
			if (d > 0) return 1;
			return 0;
		};

		auto a = min_t;
		auto b = max_t;
		double fa = ThingFunc(a, a1, v_t, p_t, s);
		double fb = ThingFunc(b, a1, v_t, p_t, s);

		if (signum(fa) == signum(fb))
			return {0, 0, 0}; 

		for (int i = 0; i < steps; i++)
		{
			double c = (a + b) * 0.5;
			double fc = ThingFunc(c, a1, v_t, p_t, s);

			if ((b - a) * 0.5 < tol)
			{
				double t = c;
				vec3_t travel = p_t + v_t * t + (a_t / 2.0) * (t * t);
				vec3_t v_p = (travel / t) - (a_p / 2.0) * t;
				return v_p;
			}

			if (signum(fc) == signum(fa))
			{
				a = c;
				fa = fc;
			}
			else
			{
				b = c;
				fb = fc;
			}
		}

		return {0, 0, 0};
	}

	ImVec2 prev_aim { };

	vec3_t CalcAngle(vec3_t AimLocation)
	{
		vec3_t VectorPos = AimLocation - camera_data.location;
		vec3_t RetVector;
		float distance = VectorPos.Length();
		RetVector.x = -((acosf(VectorPos.z / distance) * (180.0f / M_PI)) - 90.f);
		RetVector.y = atan2f(VectorPos.y, VectorPos.x) * (180.0f / M_PI);
		return RetVector.ClampAngles();
	}

	vec3_t SmoothAngles(vec3_t TargetRotation, float smoothing)
	{
		vec3_t Dist = (TargetRotation - camera_data.rotation).ClampAngles();
		float Pitch = camera_data.rotation.x + (Dist.x / (1.34f + smoothing));
		float Yaw = camera_data.rotation.y + (Dist.y / (1.34f + smoothing));

		return vec3_t(Pitch, Yaw, 0).ClampAngles();
	}

	ULONGLONG LastAimbotToggleTime = 0;
	ULONGLONG LastTriggerbotToggleTime = 0;

	void Run(std::vector<CEntity> entities, ImDrawList* drawList)
	{
		static bool isKeyPressed = false;
		static bool wasKeyPressed = false;
		static std::chrono::time_point<std::chrono::system_clock> triggerbot_start = std::chrono::system_clock::now();
		CurrentAimbotConfig = GetAimbotConfig();
		if (CurrentAimbotConfig.Triggerbot)
		{
			bool currentKeyPressed = mem.GetKeyboard()->IsKeyDown(CurrentAimbotConfig.TriggerKey) || GetAsyncKeyState(CurrentAimbotConfig.TriggerKey) || kmbox::IsDown(CurrentAimbotConfig.TriggerKey);

			if (CurrentAimbotConfig.TriggerKeyType == 0)
			{
				isKeyPressed = currentKeyPressed;
			}
			else if (CurrentAimbotConfig.TriggerKeyType == 1) // toggle
			{
				// only main keybind for toggle users
				if ((currentKeyPressed) && LastTriggerbotToggleTime < GetTickCount64())
				{
					TriggerbotToggle = !TriggerbotToggle;
					LastTriggerbotToggleTime = GetTickCount64() + 250;
					if (TriggerbotToggle)
						printf(xorstr_("Aimbot Toggled\n"));
					else
						printf(xorstr_("Aimbot Untoggled\n"));
				}
				isKeyPressed = TriggerbotToggle;
			}
			else if (CurrentAimbotConfig.TriggerKeyType == 2) // always on
			{
				isKeyPressed = true;
			}

			if (isKeyPressed)
			{
				ctx.m_draw->DrawString(xorstr_("Triggerbot: ON"), {10, 50}, ImColor(0, 255, 0));
				int delay = CurrentAimbotConfig.TriggerDelay;
				std::chrono::milliseconds time = std::chrono::milliseconds(delay);
				auto _target = GetBestTarget(entities, true);
				if (_target.valid)
				{
					if (globals::targeted_player == _target.entity.pawn_private)
					{
						if ((triggerbot_start + time) < std::chrono::system_clock::now())
						{
							if (ctx.m_settings.hardware_mouse_type == MEMORY)
							{
								if (ctx.m_settings.memory_trigger_input == PICOW)
								{
									//MoveMouseWifi(0, 0, true);
								}
								else if (ctx.m_settings.memory_trigger_input == KMBOX)
								{
									kmbox::left_click();
									kmbox::left_click_release();
								}
								else if (ctx.m_settings.memory_trigger_input == KMBOXNET)
								{
									KmBoxMgr.Mouse.Left(1);
									KmBoxMgr.Mouse.Left(0);
								}
							}
							else if (ctx.m_settings.hardware_mouse_type == PICOW)
							{
								//MoveMouseWifi(0, 0, true);
							}
							else if (ctx.m_settings.hardware_mouse_type == KMBOX)
							{
								kmbox::left_click();
								kmbox::left_click_release();
							}
							else if (ctx.m_settings.hardware_mouse_type == KMBOXNET)
							{
								KmBoxMgr.Mouse.Left(1);
								KmBoxMgr.Mouse.Left(0);
							}
							triggerbot_start = std::chrono::system_clock::now();
						}
					}
				}
			}
		}

		static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
		if (!CurrentAimbotConfig.Aimbot) return;

		bool key_down1 = mem.GetKeyboard()->IsKeyDown(CurrentAimbotConfig.AimKey) || GetAsyncKeyState(CurrentAimbotConfig.AimKey) || kmbox::IsDown(CurrentAimbotConfig.AimKey);
		bool key_down2 = mem.GetKeyboard()->IsKeyDown(CurrentAimbotConfig.AimKey2) || GetAsyncKeyState(CurrentAimbotConfig.AimKey2) || kmbox::IsDown(CurrentAimbotConfig.AimKey2);
		bool key_down3 = mem.GetKeyboard()->IsKeyDown(ctx.m_settings.freeze_key) || GetAsyncKeyState(ctx.m_settings.freeze_key) || kmbox::IsDown(ctx.m_settings.freeze_key);
		bool doaim = false;
		bool toggledaimbot = AimbotToggle;
		if (CurrentAimbotConfig.AimKeyType == 0)
		{
			doaim = key_down1 || key_down2;
		}
		else if (CurrentAimbotConfig.AimKeyType == 1) // toggle
		{
			// only main keybind for toggle users
			if ((key_down1) && LastAimbotToggleTime < GetTickCount64())
			{
				AimbotToggle = !AimbotToggle;
				LastAimbotToggleTime = GetTickCount64() + 250;
				if (AimbotToggle)
					printf(xorstr_("Aimbot Toggled\n"));
				else
					printf(xorstr_("Aimbot Untoggled\n"));
				prev_aim = ImVec2(0, 0);
			}
			doaim = AimbotToggle;
		}
		else if (CurrentAimbotConfig.AimKeyType == 2) // always on
		{
			doaim = true;
		}
		if (CurrentAimbotConfig.AimKeyType == 1)
		{
			if (AimbotToggle)
				ctx.m_draw->DrawString(xorstr_("Aimbot: ON"), {10, 65}, ImColor(0, 255, 0));
			else
				ctx.m_draw->DrawString(xorstr_("Aimbot: OFF"), {10, 65}, ImColor(255, 0, 0));
		}
		static best_tar _target;
		if (!_target.valid && (doaim || key_down3)) { _target = GetBestTarget(entities, false); }
		if (_target.valid)
		{
			_target = UpdateTarget(entities, _target);
			auto target = _target.entity;
			if (ctx.m_settings.freeze)
			{
				if (!target.pawn_private) return;
				if (key_down3) mem.Write<float>(target.pawn_private + offsets::CustomTimeDilation, 0.001f);
				else mem.Write<float>(target.pawn_private + offsets::CustomTimeDilation, 1.f);
			}
			BYTE is_dbno = _target.entity.isdbno >> 4 & 1;
			BYTE is_dying = _target.entity.isdying >> 4 & 1;
			if (is_dbno || is_dying)
			{
				prev_aim = ImVec2(0, 0);
				_target.valid = false;
				return;
			}
			if ((!key_down1 && !key_down2) && CurrentAimbotConfig.AimKeyType == 0)
			{
				prev_aim = ImVec2(0, 0);
				_target.valid = false;
				return;
			}

			float max_fov = CurrentAimbotConfig.FOV;
			float max_fov_zoom = CurrentAimbotConfig.ZoomFOV;
			int aimbot_smoothing_type = CurrentAimbotConfig.SmoothType;
			float smoothing = CurrentAimbotConfig.Smoothing;
			float aimedsmoothing = CurrentAimbotConfig.AimingSmoothing;
			bool overridesmoothing = CurrentAimbotConfig.AimingSmoothOverride;
			vec3_t hitbox = hitbox_from_cfg(CurrentAimbotConfig.HitBox1, target);

			if (!key_down1 && !key_down2 && CurrentAimbotConfig.AimKeyType == 0)
			{
				prev_aim = ImVec2(0, 0);
				_target.valid = false;
			}

			if (key_down2 && !key_down1 && CurrentAimbotConfig.AimKeyType == 0)
			{
				if (CurrentAimbotConfig.AimKeyType == 0)
					hitbox = hitbox_from_cfg(CurrentAimbotConfig.HitBox2, target);
			}

			ImVec2 res = ctx.m_settings.menu_size;
			int center_x = res.x / 2;
			int center_y = res.y / 2;

			vec2_t screen_pos = camera::WorldToScreen(hitbox);
			if (!camera::IsOnScreen(screen_pos)) return;
			if (_target.entity.bone_positions.head.x == 0 || _target.entity.bone_positions.head.y == 0 || _target.entity.bone_positions.head.z == 0) // this helps to reduce it's ability to target dead players, despite the screen check it needs this check
			{
				prev_aim = ImVec2(0, 0);
				_target.valid = false;
				return;
			}
			if (CurrentAimbotConfig.AimKeyType == 1 && !AimbotToggle)
			{
				prev_aim = ImVec2(0, 0);
				_target.valid = false;
				return;
			}
			auto handle = mem.CreateScatterHandle();
			float speed, gravity;
			vec3_t Velocity;
			mem.AddScatterReadRequest(handle, globals::local::active_weapon + 0x1AA0, &speed, sizeof(speed));
			mem.AddScatterReadRequest(handle, globals::local::active_weapon + 0x1D0C, &gravity, sizeof(gravity));
			mem.AddScatterReadRequest(handle, target.RootComponent + 0x168, &Velocity, sizeof(Velocity));
			mem.ExecuteReadScatter(handle);
			mem.CloseScatterHandle(handle);

			//printf("%f %f\n", speed, gravity);
			if (speed != 0.f && gravity != 0.f)
			{
				vec3_t targetPt = calcPrediction(
					speed, //s
					hitbox - camera_data.location, // p_t
					Velocity, //v_t
					{0, 0, 0}, //a_t
					{0, 0, gravity * -1000.f}, //a_p
					0.0001, //tol
					1000, //steps
					0.01f, //min_t (avoid zero to prevent division issues)
					2.0f); //max_t
				hitbox += targetPt;
			}

			float fov = camera::distance_from_crosshair(screen_pos);
			if (globals::local::zoom.bIsTargeting) { if (fov > max_fov_zoom) return; }
			else if (fov > max_fov) return;
			if (globals::local::zoom.bIsTargeting)
			{
				if (overridesmoothing)
				{
					smoothing = aimedsmoothing;
				}
			}
			if (ctx.m_settings.visuals_snapline == 3)
			{
				ImVec2 size = ctx.m_settings.menu_size;
				ctx.m_draw->DrawLine({screen_pos.x, screen_pos.y}, {size.x / 2, (float)size.y / 2}, ImColor(ctx.m_settings.visuals_snapline_color), 1.0f);
			}

			auto TargetLocation = camera::WorldToScreen(hitbox);

			ImVec2 mouse_aim_pos = ImVec2((TargetLocation.x - center_x) / smoothing, (TargetLocation.y - center_y) / smoothing);
			mouse_aim_pos.x += prev_aim.x;
			mouse_aim_pos.y += prev_aim.y;
			const int mdx = static_cast<int>(mouse_aim_pos.x);
			const int mdy = static_cast<int>(mouse_aim_pos.y);
			prev_aim.x = mouse_aim_pos.x - static_cast<float>(mdx);
			prev_aim.y = mouse_aim_pos.y - static_cast<float>(mdy);

			if (mdx != 0 || mdy != 0)
			{
				if (ctx.m_settings.hardware_mouse_type == PICOW)
				{
					if (start + std::chrono::milliseconds(ctx.m_settings.hardware_kmbox_delay) < std::chrono::system_clock::now())
					{
						//MoveMouseWifi(mdx, mdy, false);
						start = std::chrono::system_clock::now();
					}
				}
				else if (ctx.m_settings.hardware_mouse_type == KMBOX)
				{
					if (start + std::chrono::milliseconds(ctx.m_settings.hardware_kmbox_delay) < std::chrono::system_clock::now())
					{
						kmbox::move(mdx, mdy);
						start = std::chrono::system_clock::now();
					}
				}
				else if (ctx.m_settings.hardware_mouse_type == KMBOXNET)
				{
					if (start + std::chrono::milliseconds(ctx.m_settings.hardware_kmbox_delay) < std::chrono::system_clock::now())
					{
						KmBoxMgr.Mouse.Move(mdx, mdy);
						start = std::chrono::system_clock::now();
					}
				}
				else if (ctx.m_settings.hardware_mouse_type == MEMORY)
				{
					vec3_t Angle = (SmoothAngles(CalcAngle(hitbox), smoothing) - camera_data.rotation).ClampAngles();
					//GOTTO WRITE BOTH!!! WE DONT WANT BAN DURING COMP/TOURNAMENT!!!
					mem.Write<vec3_t>(globals::local::player_controller + 0x510, Angle); //APlayerController->NetConnection + 0x8
					mem.Write<vec3_t>(globals::local::player_controller + 0x930, Angle); //APlayerController->NetConnection + 0x8 
				}
			}
		}
	}
}
