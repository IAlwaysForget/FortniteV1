#include "Draw.h"
#include "Font/newIcons.h"
#include "Font/textFont.h"
#include "Discord/discord.h"
#include "context.h"
#include "Utils/Mouse/kmbox.h"
#include "Utils/Mouse/kmboxNet.h"

//#include "Utils/Mouse/PicoW.h"

inline int WeaponConfigIndex = 0;
void c_draw::Init(Fuser& fuse)
{
	Font::initializeFonts();
	Discord::Init();

	this->fuser = fuse;
	//this->fuslie

}

void c_draw::Run()
{
	if (GetAsyncKeyState(ctx.m_settings.menu_menu_key_hotkey) & 0x1)
		ctx.m_settings.Toggle(&ctx.m_settings.menu_menu_key);

	if (!ctx.m_settings.menu_menu_key)
		return;

	this->Begin();
	this->DisplayBrand("Moe Fortnite V0.1");
	this->End();

	this->BeginMenu();
	this->MoveWindow();
	this->DrawBackground();

	//display tab 
	this->DisplayTab({
		TabButtons("Visuals", ICON_ESP, 9.5f),
		TabButtons("Radar", ICON_RADAR, 9.5f),
		TabButtons("Aimbot", ICON_AIMBOT, 9.5f),
			TabButtons("Aimbot Input", ICON_MOUSE, 9.5f),
		TabButtons("Miscellaneous", ICON_MISC, 9.5f)
	});

	this->DisplayConfigs();
	this->DisplayPadding();

	if (this->IsActive("Visuals"))
	{
		this->Visuals();
	}
	else if (this->IsActive("Radar"))
	{
		this->Radar();
	}
	else if (this->IsActive("Aimbot"))
	{
		this->Aimbot();
	}
	else if (this->IsActive("Aimbot Input"))
	{
		this->AimbotInput();
	}
	else if (this->IsActive("Miscellaneous"))
	{
		this->Miscellaneous();
	}

	this->DisplayModals();

	this->EndMenu();
}

void c_draw::Visuals()
{
	UII::Controls::beginSection("ESP");
	{
		UII::Controls::Toggle("Box", "Toggles Box ESP", &ctx.m_settings.visuals_esp_box);
		UII::Controls::Toggle("Filled Box", "Toggles Filled Box ESP", &ctx.m_settings.visuals_filled_esp_box);
		UII::Controls::Toggle("Head Dot", "Toggles Head ESP", &ctx.m_settings.visuals_head_pos);
		if (ctx.m_settings.visuals_head_pos)
			UII::Controls::Slider("Head Dot Size", &ctx.m_settings.visuals_head_pos_size, 1.0f, 8.0f);

		UII::Controls::Toggle("Skeleton", "Toggles Skeleton ESP", &ctx.m_settings.visuals_skeleton);
		if (ctx.m_settings.visuals_skeleton)
		{
			UII::Controls::Slider("Skeleton Thickness", &ctx.m_settings.visuals_skeleton_thickness, 1.0f, 8.0f);
		}
		UII::Controls::Toggle("Name", "Toggles Name ESP", &ctx.m_settings.visuals_name);
		UII::Controls::Toggle("Distance", "Toggles Distance ESP", &ctx.m_settings.visuals_distance);
		UII::Controls::Combo("Snapline", {"Disabled", "Origin", "Middle", "Target"}, ctx.m_settings.visuals_snapline);
		UII::Controls::Toggle("Fov", "Toggles Fov Circle ESP", &ctx.m_settings.visuals_fov_circle);
		UII::Controls::Toggle("Weapon", "Toggles Weapon ESP", &ctx.m_settings.visuals_weapon);
	}
	UII::Controls::endSection();

	UII::Controls::nextSection();

	UII::Controls::beginSection("ESP Settings");
	{
		UII::Controls::Combo("ESP Box Type", {"Normal", "Corner"}, ctx.m_settings.visuals_esp_box_type);
		UII::Controls::Combo("ESP Color", {"Box", "Filled Box", "Head", "Skeleton", "Name", "Distance", "Fov Circle", "Snapline", "Weapon", "Team", "Box Bot", "Filled Box Bot", "Head Bot", "Skeleton Bot", "Name Bot", "Distance Bot", "Snapline Bot", "Weapon Bot"}, ctx.m_settings.visuals_esp_color_type);
		switch (ctx.m_settings.visuals_esp_color_type)
		{
		case sp_color_type::BOX:
			UII::Controls::ColorPicker("ESP Box Color", "picks a color", &ctx.m_settings.visuals_esp_box_color, &ctx.m_settings.visuals_esp_box_color_visible);
			break;
		case sp_color_type::FILLED_BOX:
			UII::Controls::ColorPicker("ESP Filled Box Color", "picks a color", &ctx.m_settings.visuals_filled_esp_box_color, &ctx.m_settings.visuals_filled_esp_box_color_visible);
			break;
		case sp_color_type::HEAD:
			UII::Controls::ColorPicker("ESP Head Color", "picks a color", &ctx.m_settings.visuals_head_pos_color, &ctx.m_settings.visuals_head_pos_color_visible);
			break;
		case sp_color_type::SKELETON:
			UII::Controls::ColorPicker("ESP Skeleton Color", "picks a color", &ctx.m_settings.visuals_skeleton_color, &ctx.m_settings.visuals_skeleton_color_visible);
			break;
		case sp_color_type::NAME:
			UII::Controls::ColorPicker("ESP Name Color", "picks a color", &ctx.m_settings.visuals_name_color, &ctx.m_settings.visuals_name_color_visible);
			break;
		case sp_color_type::DISTANCE:
			UII::Controls::ColorPicker("ESP Distance Color", "picks a color", &ctx.m_settings.visuals_distance_color, &ctx.m_settings.visuals_distance_color_visible);
			break;
		case sp_color_type::FOV_CIRCLE:
			UII::Controls::ColorPicker("ESP Fov Circle Color", "picks a color", &ctx.m_settings.visuals_fov_circle_color);
			break;
		case sp_color_type::SNAPLINE:
			UII::Controls::ColorPicker("ESP Snapline Color", "picks a color", &ctx.m_settings.visuals_snapline_color);
			break;
		case sp_color_type::WEAPON:
			UII::Controls::ColorPicker("ESP Weapon Color", "picks a color", &ctx.m_settings.visuals_weapon_color, &ctx.m_settings.visuals_weapon_color_visible);
			break;
		case sp_color_type::TEAM:
			UII::Controls::ColorPicker("ESP Team Color", "picks a color", &ctx.m_settings.visuals_team_color, &ctx.m_settings.visuals_team_color_visible);
			break;
		case sp_color_type::BOX_BOT:
			UII::Controls::ColorPicker("ESP Box Color", "picks a color", &ctx.m_settings.visuals_esp_box_color_bot, &ctx.m_settings.visuals_esp_box_color_visible_bot);
			break;
		case sp_color_type::FILLED_BOX_BOT:
			UII::Controls::ColorPicker("ESP Filled Box Color", "picks a color", &ctx.m_settings.visuals_filled_esp_box_color_bot, &ctx.m_settings.visuals_filled_esp_box_color_visible_bot);
			break;
		case sp_color_type::HEAD_BOT:
			UII::Controls::ColorPicker("ESP Head Color", "picks a color", &ctx.m_settings.visuals_head_pos_color_bot, &ctx.m_settings.visuals_head_pos_color_visible_bot);
			break;
		case sp_color_type::SKELETON_BOT:
			UII::Controls::ColorPicker("ESP Skeleton Color", "picks a color", &ctx.m_settings.visuals_skeleton_color_bot, &ctx.m_settings.visuals_skeleton_color_visible_bot);
			break;
		case sp_color_type::NAME_BOT:
			UII::Controls::ColorPicker("ESP Name Color", "picks a color", &ctx.m_settings.visuals_name_color_bot, &ctx.m_settings.visuals_name_color_visible_bot);
			break;
		case sp_color_type::DISTANCE_BOT:
			UII::Controls::ColorPicker("ESP Distance Color", "picks a color", &ctx.m_settings.visuals_distance_color_bot, &ctx.m_settings.visuals_distance_color_visible_bot);
			break;
		case sp_color_type::SNAPLINE_BOT:
			UII::Controls::ColorPicker("ESP Snapline Color", "picks a color", &ctx.m_settings.visuals_snapline_color_bot);
			break;
		case sp_color_type::WEAPON_BOT:
			UII::Controls::ColorPicker("ESP Weapon Color", "picks a color", &ctx.m_settings.visuals_weapon_color_bot, &ctx.m_settings.visuals_weapon_color_visible_bot);
			break;
		default:
			break;
		}
		UII::Controls::Toggle("Team", "Displays Team On ESP", &ctx.m_settings.visuals_team_check);
		//Temp removed
		//UII::Controls::Toggle("Team Colors", "Use Unique Team Colors", &ctx.m_settings.visuals_team_unique_colors);
		UII::Controls::Slider("Distance Check", &ctx.m_settings.visuals_max_distance, 0.f, 10000.f);
		UII::Controls::Slider("Name Distance Check", &ctx.m_settings.visuals_esp_name_render_distance, 0, 10000);
		UII::Controls::Slider("ESP Width", &ctx.m_settings.visuals_esp_box_width, 1, 3);
		UII::Controls::Slider("ESP Thickness", &ctx.m_settings.visuals_esp_box_thickness, 1, 4);
	}
	UII::Controls::endSection();
}
void c_draw::AimbotInput()
{
	UII::Controls::beginSection("Misc");
	{
		static char network_ip[256] = "";
		static char network_port[256] = "";
		static char network_mac[256] = "";
		static bool one = false;
		static std::string prev = "";
		UII::Controls::Combo("Hardware", { "KMBox", "PicoW", "KMBox NET", "Memory" }, ctx.m_settings.hardware_mouse_type);
		if (ctx.m_settings.hardware_mouse_type == hardware_type::MEMORY)
		{
			UII::Controls::Combo("Triggerbot Hardware", { "KMBox", "PicoW", "KMBox NET", "No Triggerbot" }, ctx.m_settings.memory_trigger_input);
			if (ctx.m_settings.memory_trigger_input == hardware_type::KMBOX)
			{
				UII::Controls::Input("KMbox Comport", &ctx.m_settings.hardware_mouse_com_port, 256);
				UII::Controls::SliderInt("KMBox Aim Delay", &ctx.m_settings.hardware_kmbox_delay, 0, 100);
				UII::Controls::InputInt("KMbox Baudrate", &ctx.m_settings.hardware_kmbox_baudrate);
			}
			if (ctx.m_settings.memory_trigger_input == hardware_type::PICOW)
				UII::Controls::Input("PicoW Comport", &ctx.m_settings.hardware_mouse_network_ip, 256);
			if (ctx.m_settings.memory_trigger_input == hardware_type::KMBOXNET)
			{
				if (!one)
				{
					ZeroMemory(network_ip, 256);
					ZeroMemory(network_port, 256);
					ZeroMemory(network_mac, 256);
					std::string i = ctx.m_settings.hardware_kmbox_mouse_network_ip;
					strcpy(network_ip, i.c_str());
					std::string p = ctx.m_settings.hardware_kmbox_mouse_network_port;
					strcpy(network_port, p.c_str());
					std::string m = ctx.m_settings.hardware_kmbox_mouse_network_mac;
					strcpy(network_mac, m.c_str());
					one = true;
				}

				UII::Controls::Input("KMboxNET IP", network_ip, 256);
				UII::Controls::Input("KMboxNET Port", network_port, 256);
				UII::Controls::Input("KMboxNET MAC", network_mac, 256);
				UII::Controls::SliderInt("KMBoxNET Aim Delay", &ctx.m_settings.hardware_kmbox_delay, 0, 100);
			}
			if (ctx.m_settings.memory_trigger_input != hardware_type::MEMORY)
			{
				if (UII::Controls::Button("Connect", "Initializes Mouse Hardware"))
				{
					if (ctx.m_settings.memory_trigger_input == hardware_type::PICOW)
					{
						//DeviceLocalIP = ctx.m_settings.hardware_mouse_network_ip;
					}
					if (ctx.m_settings.memory_trigger_input == hardware_type::KMBOX)
					{
						std::string com = ctx.m_settings.hardware_mouse_com_port;
						kmbox::initialize(com.c_str());
					}
					if (ctx.m_settings.memory_trigger_input == hardware_type::KMBOXNET)
					{
						ctx.m_settings.Set(&ctx.m_settings.hardware_kmbox_mouse_network_ip, std::string(network_ip));
						ctx.m_settings.Set(&ctx.m_settings.hardware_kmbox_mouse_network_port, std::string(network_port));
						ctx.m_settings.Set(&ctx.m_settings.hardware_kmbox_mouse_network_mac, std::string(network_mac));

						KmBoxMgr.InitDevice(ctx.m_settings.hardware_kmbox_mouse_network_ip, std::stoi(ctx.m_settings.hardware_kmbox_mouse_network_port), ctx.m_settings.hardware_kmbox_mouse_network_mac);
					}
				}
			}
		}

		if (ctx.m_settings.hardware_mouse_type == hardware_type::KMBOX)
		{
			UII::Controls::Input("KMbox Comport", &ctx.m_settings.hardware_mouse_com_port, 256);
			UII::Controls::SliderInt("KMBox Aim Delay", &ctx.m_settings.hardware_kmbox_delay, 0, 100);
			UII::Controls::InputInt("KMbox Baudrate", &ctx.m_settings.hardware_kmbox_baudrate);
		}
		if (ctx.m_settings.hardware_mouse_type == hardware_type::PICOW)
			UII::Controls::Input("PicoW Comport", &ctx.m_settings.hardware_mouse_network_ip, 256);
		if (ctx.m_settings.hardware_mouse_type == hardware_type::KMBOXNET)
		{
			if (!one)
			{
				ZeroMemory(network_ip, 256);
				ZeroMemory(network_port, 256);
				ZeroMemory(network_mac, 256);
				std::string i = ctx.m_settings.hardware_kmbox_mouse_network_ip;
				strcpy(network_ip, i.c_str());
				std::string p = ctx.m_settings.hardware_kmbox_mouse_network_port;
				strcpy(network_port, p.c_str());
				std::string m = ctx.m_settings.hardware_kmbox_mouse_network_mac;
				strcpy(network_mac, m.c_str());
				one = true;
			}

			UII::Controls::Input("KMboxNET IP", network_ip, 256);
			UII::Controls::Input("KMboxNET Port", network_port, 256);
			UII::Controls::Input("KMboxNET MAC", network_mac, 256);
			UII::Controls::SliderInt("KMBoxNET Aim Delay", &ctx.m_settings.hardware_kmbox_delay, 0, 100);
		}
		if (ctx.m_settings.hardware_mouse_type != hardware_type::MEMORY)
		{
			if (UII::Controls::Button("Connect", "Initializes Mouse Hardware"))
			{
				if (ctx.m_settings.hardware_mouse_type == hardware_type::PICOW)
				{
					//DeviceLocalIP = ctx.m_settings.hardware_mouse_network_ip;
				}
				if (ctx.m_settings.hardware_mouse_type == hardware_type::KMBOX)
				{
					std::string com = ctx.m_settings.hardware_mouse_com_port;
					kmbox::initialize(com.c_str());
				}
				if (ctx.m_settings.hardware_mouse_type == hardware_type::KMBOXNET)
				{
					ctx.m_settings.Set(&ctx.m_settings.hardware_kmbox_mouse_network_ip, std::string(network_ip));
					ctx.m_settings.Set(&ctx.m_settings.hardware_kmbox_mouse_network_port, std::string(network_port));
					ctx.m_settings.Set(&ctx.m_settings.hardware_kmbox_mouse_network_mac, std::string(network_mac));

					KmBoxMgr.InitDevice(ctx.m_settings.hardware_kmbox_mouse_network_ip, std::stoi(ctx.m_settings.hardware_kmbox_mouse_network_port), ctx.m_settings.hardware_kmbox_mouse_network_mac);
				}
			}
		}

	}
	UII::Controls::endSection();
}

AimbotConfig GetWeaponConfig(const int& index)
{
	switch (index)
	{
	case 0:
		return ctx.m_settings.OtherAimbot;
	case 1:
		return ctx.m_settings.PistolAimbot;
	case 2:
		return ctx.m_settings.ShotgunAimbot;
	case 3:
		return ctx.m_settings.SniperAimbot;
	case 4:
		return ctx.m_settings.RifleAimbot;
	case 5:
		return ctx.m_settings.SMGAimbot;
	case 6:
		return ctx.m_settings.LauncherAimbot;
	case 7:
		return ctx.m_settings.BowAimbot;


	}
	return ctx.m_settings.OtherAimbot;
}

void c_draw::Aimbot()
{
	UII::Controls::beginSection("Aimbot");
	{
		UII::Controls::Toggle("Aimbot", "Aimbot Master Switch", &ctx.m_settings.aimbot);
		UII::Controls::Toggle("Prediction", "Prediction Check", &ctx.m_settings.aimbot_prediction);
		UII::Controls::Toggle("Visible Check", "Visible Check", &ctx.m_settings.aimbot_vis_check);
		UII::Controls::Toggle("Team Check", "Shoot on team", &ctx.m_settings.aimbot_team);
		UII::Controls::Combo("Aimbot Category", { "Other", "Pistol","Shotgun","Sniper","Rifle","SMG","Launcher","Bow"}, WeaponConfigIndex);

	}
	UII::Controls::endSection();

	UII::Controls::nextSection();

	UII::Controls::beginSection("Aimbot Settings");
	{
		AimbotConfig config = GetWeaponConfig(WeaponConfigIndex);
	


		UII::Controls::Toggle("Triggerbot", "Enables Triggerbot For This Category", &config.Triggerbot);
		if (config.Triggerbot)
		{
			if (config.TriggerKeyType != 2)
				UII::Controls::Hotkey("Trigger Key", &config.TriggerKey);
			UII::Controls::SliderInt("Triggerbot Delay", &config.TriggerDelay, 1, 1000);
			UII::Controls::SliderInt("Triggerbot Max Distance", &config.TriggerMaxDistance, 1, 1000);
			UII::Controls::Combo("Triggerbot Category", { "Hold","Toggle","Always" }, config.TriggerKeyType);
		}

		UII::Controls::Toggle("Aimbot", "Enables Aimbot For This Category", &config.Aimbot);
		if (config.Aimbot)
		{
			UII::Controls::Combo("Aimbot Category", { "Hold","Toggle","Always" }, config.AimKeyType);
			if (config.AimKeyType != 2)
			{
				UII::Controls::Hotkey("Aimbot Key 1", &config.AimKey);
			
			}
			UII::Controls::Combo("Hitbox", { "Head", "Chest", "Pelvis", "Spine", "Closest" }, config.HitBox1);
			if (config.AimKeyType == 0)
			{
				UII::Controls::Hotkey("Aimbot Key2", &config.AimKey2);
				UII::Controls::Combo("Hitbox 2", { "Head", "Chest", "Pelvis", "Spine", "Closest" }, config.HitBox2);

			}
			UII::Controls::SliderInt("Aimbot Max Distance", &config.MaxDistance, 1, 1000);
			UII::Controls::Slider("Aimbot Smoothing", &config.Smoothing, 1.f, 50.f);
			UII::Controls::Combo("Smoothing Type", { "Linear", "Non-Linear (Perlin)" }, config.SmoothType);
			UII::Controls::SliderInt("Aimbot FOV", &config.FOV, 1, 2000);
			UII::Controls::SliderInt("Aimbot Zoom FOV", &config.ZoomFOV, 1, 2000);
			UII::Controls::Toggle("Different Smoothing When Aiming", "Enables Aimbot For This Category", &config.AimingSmoothOverride);
			if(config.AimingSmoothOverride)
			UII::Controls::Slider("Aimbot Aim Smoothing", &config.AimingSmoothing, 1.f, 50.f);

		}
		switch (WeaponConfigIndex)
		{
		case 0:
			ctx.m_settings.Set(&ctx.m_settings.OtherAimbot, config);
			break;
		case 1:
			ctx.m_settings.Set(&ctx.m_settings.PistolAimbot, config);
			break;
		case 2:
			ctx.m_settings.Set(&ctx.m_settings.ShotgunAimbot, config);
			break;
		case 3:
			ctx.m_settings.Set(&ctx.m_settings.SniperAimbot, config);
			break;
		case 4:
			ctx.m_settings.Set(&ctx.m_settings.RifleAimbot, config);
			break;
		case 5:
			ctx.m_settings.Set(&ctx.m_settings.SMGAimbot, config);
			break;
		case 6:
			ctx.m_settings.Set(&ctx.m_settings.LauncherAimbot, config);
			break;
		case 7:
			ctx.m_settings.Set(&ctx.m_settings.BowAimbot, config);
			break;


		}
	}
	UII::Controls::endSection();


}
void c_draw::Radar()
{

	UII::Controls::beginSection("Radar");
	{
		UII::Controls::Toggle("Enable", "Draws Radar", &ctx.m_settings.visuals_2d_radar);
		UII::Controls::Toggle("Radar Move", "Allows You To Move The Radar", &ctx.m_settings.visuals_2d_radar_move);
		UII::Controls::Slider("Radar Zoom",&ctx.m_settings.visuals_2d_radar_scaling,0,1);
		UII::Controls::Slider("Radar Size", &ctx.m_settings.visuals_2d_radar_size, 20, 500);
		UII::Controls::ColorPicker("LocalPlayer Colour", "Colour Of LocalPlayer", &ctx.m_settings.visuals_2d_radar_localplayer_colour);
		UII::Controls::SliderInt("LocalPlayer Look Line Width", &ctx.m_settings.visuals_2d_radar_localplayer_line_width, 1, 6);
		UII::Controls::SliderInt("Radar Text Size", &ctx.m_settings.visuals_2d_radar_font_size, 1, 16);
		UII::Controls::Toggle("Draw Radar Background", "Draws A Background For Radar", &ctx.m_settings.visuals_2d_radar_background);
		if(ctx.m_settings.visuals_2d_radar_background)
			UII::Controls::ColorPicker("Background Colour", "Colour Of Background", &ctx.m_settings.visuals_2d_radar_background_colour);
		UII::Controls::ColorPicker("Border Colour", "Colour Of Radar Border", &ctx.m_settings.visuals_2d_radar_border_colour);
		UII::Controls::Toggle("Disco Mode", "BOOM BOOM BOOM BOOM", &ctx.m_settings.visuals_2d_radar_disco);

	}
	UII::Controls::endSection();
	UII::Controls::nextSection();
	UII::Controls::beginSection("Radar Entities");
	{
		UII::Controls::Toggle("Draw Enemies", "Draws Enemies", &ctx.m_settings.visuals_2d_radar_draw_enemy);
		UII::Controls::ColorPicker("Enemey Colour", "Colour Of Hostiles", &ctx.m_settings.visuals_2d_radar_enemy_colour);
		UII::Controls::Toggle("Draw Friends", "Draws Friends", &ctx.m_settings.visuals_2d_radar_draw_friend);
		UII::Controls::ColorPicker("Friend Colour", "Colour Of Friends", &ctx.m_settings.visuals_2d_radar_friend_colour);
		UII::Controls::Toggle("Draw Distance", "Draws Enemy Distance", &ctx.m_settings.visuals_2d_radar_draw_distance);
		UII::Controls::Toggle("Draw Weapon", "Draws Enemy Weapon", &ctx.m_settings.visuals_2d_radar_draw_weapon);


	}
	UII::Controls::endSection();
}

std::vector<std::string> menu_size =
{
	"640 x 480",
	"800 x 600",
	"1024 x 768",
	"1280 x 720",
	"1280 x 800",
	"1366 x 768",
	"1440 x 900",
	"1600 x 900",
	"1680 x 1050",
	"1720 x 1080",
	"1728 x 1080",
	"1920 x 1080",
	"1920 x 1200",
	"2560 x 1440",
	"2880 x 1800",
	"3440 x 1440",
	"3840 x 2160"
};

enum menu_types
{
	SIZE_640_480,
	SIZE_800_600,
	SIZE_1024_768,
	SIZE_1280_720,
	SIZE_1280_800,
	SIZE_1366_768,
	SIZE_1440_900,
	SIZE_1600_900,
	SIZE_1680_1050,
	SIZE_1720_1080,
	SIZE_1728_1080,
	SIZE_1920_1080,
	SIZE_1920_1200,
	SIZE_2560_1440,
	SIZE_2880_1800,
	SIZE_3440_1440,
	SIZE_3840_2160
};

void c_draw::Miscellaneous()
{
	UII::Controls::beginSection("Main");
	{
		static int first_dx = ctx.m_settings.directx_support;
		UII::Controls::Toggle("Allow Memory Writes", "Required For Some Features", &ctx.m_settings.allow_memory_writes);
		UII::Controls::Toggle("VSync", "Enabled VSync for Overlay", &ctx.m_settings.vsync);
		UII::Controls::Hotkey("Menu Key", &ctx.m_settings.menu_menu_key_hotkey);
		UII::Controls::Combo("Fuser Aspect Ratio", menu_size, ctx.m_settings.menu_size_type);
		switch (ctx.m_settings.menu_size_type)
		{
		case menu_types::SIZE_640_480:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(640, 480));
			break;
		case menu_types::SIZE_800_600:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(800, 600));
			break;
		case menu_types::SIZE_1024_768:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1024, 768));
			break;
		case menu_types::SIZE_1280_720:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1280, 720));
			break;
		case menu_types::SIZE_1280_800:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1280, 800));
			break;
		case menu_types::SIZE_1366_768:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1366, 768));
			break;
		case menu_types::SIZE_1440_900:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1440, 900));
			break;
		case menu_types::SIZE_1600_900:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1600, 900));
			break;
		case menu_types::SIZE_1680_1050:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1680, 1050));
			break;
		case menu_types::SIZE_1720_1080:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1720, 1080));
			break;
		case menu_types::SIZE_1728_1080:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1728, 1080));
			break;
		case menu_types::SIZE_1920_1080:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1920, 1080));
			break;
		case menu_types::SIZE_1920_1200:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(1920, 1200));
			break;
		case menu_types::SIZE_2560_1440:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(2560, 1440));
			break;
		case menu_types::SIZE_2880_1800:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(2880, 1800));
			break;
		case menu_types::SIZE_3440_1440:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(3440, 1440));
			break;
		case menu_types::SIZE_3840_2160:
			ctx.m_settings.Set(&ctx.m_settings.menu_size, ImVec2(3840, 2160));
			break;
		default:
			break;
		}
		if (ctx.m_settings.allow_memory_writes)
		{
			UII::Controls::Toggle("Remove Recoil", "Removes Weapon Recoil", &ctx.m_settings.remove_recoil);
			UII::Controls::Toggle("Freeze Players", "Freeze Players", &ctx.m_settings.freeze);
			if (ctx.m_settings.freeze)
			{
				UII::Controls::Hotkey("Freeze Key", &ctx.m_settings.freeze_key);
			}
		}
	}
	UII::Controls::endSection();

	UII::Controls::nextSection();

	UII::Controls::beginSection("Movement", ImVec2(250, 223 - 5));
	{
	}
	UII::Controls::endSection();

	UII::Controls::sameSection();

	UII::Controls::beginSection("World", ImVec2(250, 223 - 5));
	{
	}
	UII::Controls::endSection();
}

void c_draw::DrawString(std::string text, ImVec2 pos, ImColor color)
{
	auto draw_list = ImGui::GetBackgroundDrawList();

	draw_list->AddText(pos, color, text.c_str());
}
void c_draw::DrawString(std::string text, const int& fontsize,ImVec2 pos, ImColor color)
{
	auto draw_list = ImGui::GetBackgroundDrawList();

	draw_list->AddText(pos,fontsize, color, text.c_str());
}
void c_draw::DrawRect(ImVec2 min, ImVec2 max, float thickness, ImColor color)
{
	auto draw_list = ImGui::GetBackgroundDrawList();
	draw_list->AddRect(min, max, color, 0.0f, 0, thickness);
}

void c_draw::DrawFilledRect(ImVec2 min, ImVec2 max, ImColor color)
{
	auto draw_list = ImGui::GetBackgroundDrawList();
	draw_list->AddRectFilled(min, max, color, 0.0f, 0);
}

void c_draw::DrawLine(ImVec2 p1, ImVec2 p2, ImColor color, float thickness)
{
	auto draw_list = ImGui::GetBackgroundDrawList();
	draw_list->AddLine(p1, p2, color, thickness);
}

void c_draw::DrawCircle(ImVec2 center, float raduis, ImColor color, int segments, float thickness)
{
	auto draw_list = ImGui::GetBackgroundDrawList();
	draw_list->AddCircle(center, raduis, color, segments, thickness);
}

void c_draw::DrawFilledCircle(ImVec2 center, float raduis, ImColor color, int segments)
{
	auto draw_list = ImGui::GetBackgroundDrawList();
	draw_list->AddCircleFilled(center, raduis, color, segments);
}
