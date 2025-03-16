#pragma once
#include "hash.h"
#include "fileIO.h"
#include <vector>
#include <string>

#include "Draw.h"
#include "imgui.h"
#include "xorstr.hpp"

struct string_setting_t
{
	char text[128];
};

class node_t
{
public:
	virtual ~node_t() = default;

	virtual void add(node_t* node_ptr) = 0;

	virtual void load_setting(std::string& config_str) = 0;
	virtual void save_setting(std::string& config_str) = 0;
	//virtual bool set_value(std::string& feature) = 0;
};

class holder_t : public node_t
{
public:
	holder_t(const char* name) : m_name(name)
	{
	}

	holder_t(holder_t* holder_ptr, const char* name) : m_name(name) { holder_ptr->add(this); }

	void load_setting(std::string& node) override
	{
	}

	void save_setting(std::string& node) override
	{
	}

	//bool set_value(std::string& feature) override { return false; }

	void add(node_t* node_ptr) override;

	void load(const char* path, const char* file);

	void load(std::string& config);

	void save(std::string& config);

	void save(const char* path, const char* file);

	const char* m_name;
	std::vector<node_t*> m_nodes;
	std::string tempConfig;
};

class ISetting : public node_t
{
	void add(node_t* node) override
	{
	}
};

template <typename T>
class setting_t : ISetting
{
public:
	__forceinline setting_t(holder_t* holder_ptr, uint64_t name) : m_name(name) { holder_ptr->add(this); }

	__forceinline setting_t(holder_t* holder_ptr, uint64_t name, const T& rhs) : m_old_value(rhs), m_value(rhs), m_default(rhs), m_name(name) { holder_ptr->add(this); }

	bool changed()
	{
		uint8_t* data1 = reinterpret_cast<uint8_t*>(&m_value);
		uint8_t* data2 = reinterpret_cast<uint8_t*>(&m_old_value);

		for (size_t i = 0; i < sizeof(m_value); i++)
		{
			if (data1[i] != data2[i])
			{
				m_old_value = m_value;
				return true; // Objects are not equal
			}
		}
		return false;
	}

	void load_setting(std::string& config_str) override
	{
		m_value = m_default;

		uint8_t* data = (uint8_t*)&m_value;

		auto pos = config_str.find(std::to_string(m_name));
		if (pos == std::string::npos)
			return;

		pos += std::to_string(m_name).length() + 1;
		const char* buffer = config_str.data() + pos;
		for (size_t i = 0; i < sizeof(m_value); i++)
		{
			unsigned temp;
			sscanf(&buffer[2 * i], "%02X", &temp);
			data[i] = temp; //Set Value
		}
	}

	void save_setting(std::string& config_str) override
	{
		config_str.append(std::to_string(m_name) + xorstr_(":"));

		uint8_t* data = (uint8_t*)&m_value;

		char* buffer = new char[sizeof(m_value) * 2 + 1];
		for (size_t i = 0; i < sizeof(m_value); i++)
			sprintf(&buffer[2 * i], "%02X", data[i]);

		config_str.append(std::string(buffer) + xorstr_(";"));
		delete[] buffer;
	}

	setting_t& operator=(const T& rhs)
	{
		return *this;
	}

	__forceinline operator T&()
	{
		return m_value;
	}

	__forceinline operator const T&() const
	{
		return m_value;
	}

	__forceinline T* operator &()
	{
		return &m_value;
	}

	__forceinline const T* operator &() const
	{
		return &m_value;
	}

	__forceinline T& operator ()()
	{
		return m_value;
	}

	__forceinline const T& operator ()() const
	{
		return m_value;
	}

	bool operator!=(const T& rhs) const
	{
		return m_value != rhs;
	}

private:
	T m_value;
	T m_old_value;
	T m_default;
	uint64_t m_name;
};

template <typename t>
class settingtype_t
{
public:
	settingtype_t(const t& value = { })
	{
		for (size_t i = 0; i < 1; ++i)
			m_data[i] = value;
	}

	t m_data[1];

	t& operator( )()
	{
		return m_data[0];
	}

	t& operator[](const size_t& iter)
	{
		return m_data[iter];
	}
};

// fuck it!
struct settingstring_t
{
	char buffer[128];

	settingstring_t()
	{
		clear();
	}

	settingstring_t(const char* text)
	{
		set(text);
	}

	__forceinline void set(std::string& str)
	{
		set(str.c_str());
	}

	__forceinline void set(const char* text)
	{
		clear();
		strncpy_s(buffer, 128, text, 128);
	}

	__forceinline std::string get() const
	{
		return buffer;
	}

	__forceinline bool empty() const
	{
		return buffer[0] == 0;
	}

	__forceinline void clear()
	{
		memset(buffer, 0, 128);
	}

	__forceinline const char* c_str()
	{
		return buffer;
	}
};
struct AimbotConfig
{

	bool Triggerbot = true;
	int TriggerKey = VK_XBUTTON2;
	int TriggerKeyType = 0;
	int TriggerMaxDistance = 100;
	int TriggerDelay = 200;

	bool Aimbot = true;
	int HitBox1 = 0;
	int HitBox2 = 2;
	int AimKey = VK_XBUTTON1;
	int AimKey2 = 88;
	float Smoothing = 2;
	int FOV = 100;
	int ZoomFOV = 200;
	int AimKeyType = 0;
	int MaxDistance = 200;
	int SmoothType = 0;
	float AimingSmoothing = 2;
	bool AimingSmoothOverride = false;

};
class c_settings
{
public:
	bool tooltips = false;
	holder_t m_holder {""};

	setting_t<bool> menu_menu_key {&m_holder, fnvc("menu_menu_key"), true};
	setting_t<int> menu_menu_key_hotkey {&m_holder, fnvc("menu_menu_key_hotkey"), VK_INSERT};
	setting_t<ImVec2> menu_size {&m_holder, fnvc("menu_size"), {1920.f, 1080.f}};
	setting_t<int> menu_size_type {&m_holder, fnvc("menu_size_type"), 9};

	/* Hardware mouse type */
	setting_t<int> hardware_mouse_type {&m_holder, fnvc("hardware_mouse_type"), 0};
	setting_t<int> hardware_kmbox_baudrate {&m_holder, fnvc("hardware_kmbox_baudrate"), 115200};
	setting_t<int> hardware_kmbox_delay {&m_holder, fnvc("hardware_kmbox_delay"), 10};
	setting_t<std::string> hardware_kmbox_mouse_network_ip {&m_holder, fnvc("hardware_kmbox_mouse_network_ip"), "192.168.178.102"};
	setting_t<std::string> hardware_kmbox_mouse_network_port {&m_holder, fnvc("hardware_kmbox_mouse_network_port"), "512"};
	setting_t<std::string> hardware_kmbox_mouse_network_mac {&m_holder, fnvc("hardware_kmbox_mouse_network_mac"), "C362383B"};
	setting_t<std::string> hardware_mouse_network_ip {&m_holder, fnvc("hardware_mouse_network_ip"), "192.168.178.102"};
	setting_t<std::string> hardware_mouse_com_port {&m_holder, fnvc("hardware_mouse_com_port"), "COM0"};

	/*Visual Settings*/
	setting_t<bool> visuals_esp_box {&m_holder, fnvc("visuals_esp_box"), true};
	setting_t<bool> visuals_filled_esp_box {&m_holder, fnvc("visuals_filled_esp_box"), false};

	setting_t<bool> visuals_head_pos {&m_holder, fnvc("visuals_head_pos"), false};
	setting_t<float> visuals_head_pos_size {&m_holder, fnvc("visuals_head_pos_size"), 2.0f};
	setting_t<bool> visuals_skeleton {&m_holder, fnvc("visuals_skeleton"), false};
	setting_t<float> visuals_skeleton_thickness {&m_holder, fnvc("visuals_skeleton_thickness"), 1.0f};
	setting_t<bool> visuals_name {&m_holder, fnvc("visuals_name"), true};
	setting_t<bool> visuals_distance {&m_holder, fnvc("visuals_distance"), false};
	setting_t<bool> visuals_fov_circle {&m_holder, fnvc("visuals_fov_circle"), false};	
	
	setting_t<int> visuals_snapline {&m_holder, fnvc("visuals_snapline"), 0};
	setting_t<bool> visuals_weapon {&m_holder, fnvc("visuals_weapon"), false};

	setting_t<bool> visuals_team_check {&m_holder, fnvc("visuals_team_check"), true};
	setting_t<bool> visuals_team_unique_colors {&m_holder, fnvc("visuals_team_unique_colors"), false};

	setting_t<bool> visuals_2d_radar{ &m_holder, fnvc("visuals_2d_radar"), false };
	setting_t<float> visuals_2d_radar_size {&m_holder, fnvc("visuals_2d_radar_size"), 250.f};
	setting_t<bool> visuals_2d_radar_move {&m_holder, fnvc("visuals_2d_radar_move"), false};
	setting_t<float> visuals_2d_radar_scaling {&m_holder, fnvc("visuals_2d_radar_scaling"), 0.3f};
	setting_t<float> visuals_max_distance {&m_holder, fnvc("visuals_max_distance"), 1000.f};
	setting_t<ImVec2> visuals_2d_radar_pos {&m_holder, fnvc("visuals_2d_radar_pos"), ImVec2(0.1f, 0.1f)};
	setting_t<int> visuals_2d_radar_localplayer_line_width{ &m_holder, fnvc("visuals_2d_radar_localplayer_line_width"), 2 };
	setting_t<ImColor> visuals_2d_radar_localplayer_colour{ &m_holder, fnvc("visuals_2d_radar_localplayer_colour"), ImColor(0, 233, 255, 255) };
	setting_t<ImColor> visuals_2d_radar_enemy_colour{ &m_holder, fnvc("visuals_2d_radar_enemy_colour"), ImColor(255, 0, 0, 255) };
	setting_t<ImColor> visuals_2d_radar_friend_colour{ &m_holder, fnvc("visuals_2d_radar_friend_colour"), ImColor(0, 255, 0, 255) };
	setting_t<bool> visuals_2d_radar_draw_friend{ &m_holder, fnvc("visuals_2d_radar_draw_friend"), false};
	setting_t<bool> visuals_2d_radar_draw_enemy{ &m_holder, fnvc("visuals_2d_radar_draw_enemy"), true };
	setting_t<bool> visuals_2d_radar_draw_distance{ &m_holder, fnvc("visuals_2d_radar_draw_distance"), true };
	setting_t<bool> visuals_2d_radar_draw_weapon{ &m_holder, fnvc("visuals_2d_radar_draw_weapon"), true };
	setting_t<int> visuals_2d_radar_font_size{ &m_holder, fnvc("visuals_2d_radar_font_size"), 11 };
	setting_t<bool> visuals_2d_radar_background{ &m_holder, fnvc("visuals_2d_radar_background"), false };
	setting_t<ImColor> visuals_2d_radar_background_colour{ &m_holder, fnvc("visuals_2d_radar_background_colour"), ImColor(0, 0, 0, 255) };
	setting_t<ImColor> visuals_2d_radar_border_colour{ &m_holder, fnvc("visuals_2d_radar_Border_colour"), ImColor(255, 255, 255, 255) };
	setting_t<bool> visuals_2d_radar_disco{ &m_holder, fnvc("visuals_2d_radar_disco"), false };

	setting_t<int> hotkey_test {&m_holder, fnvc("hotkey_test"), VK_F1};

	setting_t<int> visuals_esp_box_type {&m_holder, fnvc("visuals_esp_box_type"), 0};
	setting_t<int> visuals_esp_color_type {&m_holder, fnvc("visuals_esp_color_type"), 0};
	setting_t<float> visuals_esp_box_width {&m_holder, fnvc("visuals_esp_box_width"), 2.15f};
	setting_t<float> visuals_esp_box_thickness {&m_holder, fnvc("visuals_esp_box_thickness"), 1.f};
	setting_t<float> visuals_esp_name_render_distance {&m_holder, fnvc("esp_name_render_distance"), 10000.f};
	//Visual Colors
	setting_t<ImColor> visuals_esp_box_color {&m_holder, fnvc("visuals_esp_box_color"), ImColor(201, 18, 46, 255)};
	setting_t<ImColor> visuals_esp_box_color_visible {&m_holder, fnvc("visuals_esp_box_color_visible"), ImColor(18, 196, 30, 255)};

	setting_t<ImColor> visuals_filled_esp_box_color {&m_holder, fnvc("visuals_filled_esp_box_color"), ImColor(33, 33, 33, 80)};
	setting_t<ImColor> visuals_filled_esp_box_color_visible {&m_holder, fnvc("visuals_filled_esp_box_color_visible"), ImColor(33, 33, 33, 80)};

	setting_t<ImColor> visuals_head_pos_color {&m_holder, fnvc("visuals_head_pos_color"), ImColor(201, 18, 46, 255)};
	setting_t<ImColor> visuals_head_pos_color_visible {&m_holder, fnvc("visuals_head_pos_color_visible"), ImColor(18, 196, 30, 255)};

	setting_t<ImColor> visuals_skeleton_color {&m_holder, fnvc("visuals_skeleton_color"), ImColor(201, 18, 46, 255)};
	setting_t<ImColor> visuals_skeleton_color_visible {&m_holder, fnvc("visuals_skeleton_color_visible"), ImColor(18, 196, 30, 255)};

	setting_t<ImColor> visuals_name_color {&m_holder, fnvc("visuals_name_color"), ImColor(255, 255, 255, 255)};
	setting_t<ImColor> visuals_name_color_visible {&m_holder, fnvc("visuals_name_color_visible"), ImColor(255, 255, 255, 255)};

	setting_t<ImColor> visuals_distance_color {&m_holder, fnvc("visuals_distance_color"), ImColor(255, 255, 255, 255)};
	setting_t<ImColor> visuals_distance_color_visible {&m_holder, fnvc("visuals_distance_color_visible"), ImColor(255, 255, 255, 255)};

	setting_t<ImColor> visuals_snapline_color {&m_holder, fnvc("visuals_snapline_color"), ImColor(255, 255, 255, 255)};

	setting_t<ImColor> visuals_weapon_color {&m_holder, fnvc("visuals_weapon_color"), ImColor(255, 255, 255, 255)};
	setting_t<ImColor> visuals_weapon_color_visible {&m_holder, fnvc("visuals_weapon_color_visible"), ImColor(255, 255, 255, 255)};

	//BOT COLORS
	setting_t<ImColor> visuals_esp_box_color_bot {&m_holder, fnvc("visuals_esp_box_color_bot"), ImColor(138, 32, 48, 255)};
	setting_t<ImColor> visuals_esp_box_color_visible_bot {&m_holder, fnvc("visuals_esp_box_color_visible_bot"), ImColor(51, 138, 57, 255)};

	setting_t<ImColor> visuals_filled_esp_box_color_bot {&m_holder, fnvc("visuals_filled_esp_box_color_bot"), ImColor(33, 33, 33, 80)};
	setting_t<ImColor> visuals_filled_esp_box_color_visible_bot {&m_holder, fnvc("visuals_filled_esp_box_color_visible_bot"), ImColor(33, 33, 33, 80)};

	setting_t<ImColor> visuals_head_pos_color_bot {&m_holder, fnvc("visuals_head_pos_color_bot"), ImColor(138, 32, 48, 255)};
	setting_t<ImColor> visuals_head_pos_color_visible_bot {&m_holder, fnvc("visuals_head_pos_color_visible_bot"), ImColor(51, 138, 57, 255)};

	setting_t<ImColor> visuals_skeleton_color_bot {&m_holder, fnvc("visuals_skeleton_color_bot"), ImColor(138, 32, 48, 255)};
	setting_t<ImColor> visuals_skeleton_color_visible_bot {&m_holder, fnvc("visuals_skeleton_color_visible_bot"), ImColor(51, 138, 57, 255)};

	setting_t<ImColor> visuals_name_color_bot {&m_holder, fnvc("visuals_name_color_bot"), ImColor(255, 255, 255, 255)};
	setting_t<ImColor> visuals_name_color_visible_bot {&m_holder, fnvc("visuals_name_color_visible_bot"), ImColor(255, 255, 255, 255)};

	setting_t<ImColor> visuals_distance_color_bot {&m_holder, fnvc("visuals_distance_color_bot"), ImColor(255, 255, 255, 255)};
	setting_t<ImColor> visuals_distance_color_visible_bot {&m_holder, fnvc("visuals_distance_color_visible_bot"), ImColor(255, 255, 255, 255)};

	setting_t<ImColor> visuals_snapline_color_bot {&m_holder, fnvc("visuals_snapline_color_bot"), ImColor(255, 255, 255, 255)};

	setting_t<ImColor> visuals_weapon_color_bot {&m_holder, fnvc("visuals_weapon_color_bot"), ImColor(255, 255, 255, 255)};
	setting_t<ImColor> visuals_weapon_color_visible_bot {&m_holder, fnvc("visuals_weapon_color_visible_bot"), ImColor(255, 255, 255, 255)};

	//Normal colors for esp
	setting_t<ImColor> visuals_fov_circle_color {&m_holder, fnvc("visuals_fov_circle_color_bot"), ImColor(255, 255, 255, 255)};

	setting_t<ImColor> visuals_team_color {&m_holder, fnvc("visuals_team_color_bot"), ImColor(19, 93, 212, 255)};
	setting_t<ImColor> visuals_team_color_visible {&m_holder, fnvc("visuals_team_color_visible_bot"), ImColor(110, 165, 255, 255)};

	/*Aimbot Settings*/
	setting_t<bool> aimbot {&m_holder, fnvc("aimbot"), false};
	setting_t<bool> aimbot_vis_check {&m_holder, fnvc("aimbot_vis_check"), true};
	setting_t<bool> aimbot_team {&m_holder, fnvc("aimbot_team"), true};
	setting_t<bool> aimbot_prediction {&m_holder, fnvc("aimbot_prediction"), true};

	setting_t<AimbotConfig> RifleAimbot{ &m_holder, fnvc("RifleAimbot"), AimbotConfig() };
	setting_t<AimbotConfig> ShotgunAimbot{ &m_holder, fnvc("ShotgunAimbot"), AimbotConfig() };
	setting_t<AimbotConfig> SMGAimbot{ &m_holder, fnvc("SMGAimbot"), AimbotConfig() };
	setting_t<AimbotConfig> PistolAimbot{ &m_holder, fnvc("PistolAimbot"), AimbotConfig() };
	setting_t<AimbotConfig> SniperAimbot{ &m_holder, fnvc("SniperAimbot"), AimbotConfig() };
	setting_t<AimbotConfig> LauncherAimbot{ &m_holder, fnvc("LauncherAimbot"), AimbotConfig() };
	setting_t<AimbotConfig> BowAimbot{ &m_holder, fnvc("BowAimbot"), AimbotConfig() };
	setting_t<AimbotConfig> OtherAimbot{ &m_holder, fnvc("OtherAimbot"), AimbotConfig() };

	setting_t<int> memory_trigger_input {&m_holder, fnvc("memory_trigger_input"), 0};
	/*Miscellaneous settings*/
	setting_t<bool> allow_memory_writes {&m_holder, fnvc("allow_memory_writes"), false};
	setting_t<bool> vsync {&m_holder, fnvc("vsync"), false};
	setting_t<int> directx_support {&m_holder, fnvc("directx_support"), 1};
	setting_t<bool> remove_recoil {&m_holder, fnvc("remove_recoil"), false};
	setting_t<bool> freeze {&m_holder, fnvc("freeze"), false};
	setting_t<int> freeze_key {&m_holder, fnvc("freeze_key"), VK_F5};
	//Misc
	setting_t<bool> aimbot_crosshair {&m_holder, fnvc("aimbot_crosshair"), false};

	void save(const char* name)
	{
		std::string path = std::getenv("USERPROFILE");
		path += "\\Documents";
		if (!DoesPathExist(path))
			CreateFolder(path);
		path += "\\Sentry";
		if (!DoesPathExist(path))
			CreateFolder(path);
		path += "\\Config";
		if (!DoesPathExist(path))
			CreateFolder(path);
		path += "\\Fortnite";
		if (!DoesPathExist(path))
			CreateFolder(path);
		m_holder.save(path.c_str(), name);
	}

	bool exists(const char* name)
	{
		std::string path = std::getenv("USERPROFILE");
		path += "\\Documents";
		if (!DoesPathExist(path))
			return false;
		path += "\\Sentry";
		if (!DoesPathExist(path))
			return false;
		path += "\\Config";
		if (!DoesPathExist(path))
			return false;
		path += "\\Fortnite";
		if (!DoesPathExist(path))
			return false;
		path += "\\" + std::string(name);
		if (std::filesystem::exists(path))
			return true;
		return false;
	}

	void load(const char* name)
	{
		std::string path = std::getenv("USERPROFILE");
		path += "\\Documents";
		if (!DoesPathExist(path))
			CreateFolder(path);
		path += "\\Sentry";
		if (!DoesPathExist(path))
			CreateFolder(path);
		path += "\\Config";
		if (!DoesPathExist(path))
			CreateFolder(path);
		path += "\\Fortnite";
		if (!DoesPathExist(path))
			CreateFolder(path);
		m_holder.load(path.c_str(), name);
	}

	void _delete(const char* name)
	{
		std::string path = std::getenv("USERPROFILE");
		path += "\\Documents";
		if (!DoesPathExist(path))
			return;
		path += "\\Sentry";
		if (!DoesPathExist(path))
			return;
		path += "\\Config";
		if (!DoesPathExist(path))
			return;
		path += "\\Fortnite";
		if (!DoesPathExist(path))
			CreateFolder(path);
		path += "\\" + std::string(name);
		if (std::filesystem::exists(path))
			std::filesystem::remove(path);
	}

	std::vector<std::string> get_all_configs()
	{
		std::vector<std::string> configs;
		std::string path = std::getenv("USERPROFILE");
		path += "\\Documents";
		if (!DoesPathExist(path))
			return configs;
		path += "\\Sentry";
		if (!DoesPathExist(path))
			return configs;
		path += "\\Config";
		if (!DoesPathExist(path))
			return configs;
		path += "\\Fortnite";
		if (!DoesPathExist(path))
			return configs;
		//get all files in folder
		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(path))
				if (std::filesystem::is_regular_file(entry.path()))
					configs.push_back(entry.path().filename().string());
		}
		catch (const std::exception& e)
		{
		}
		return configs;
	}

	void save_conf(std::string& config) { m_holder.save(config); }
	void load_conf(std::string& config) { m_holder.load(config); }

	template <typename T>
	void Set(T* setting, T value)
	{
		*setting = value;
	}

	template <typename T>
	void Toggle(T* setting)
	{
		*setting = !*setting;
	}
};

extern c_settings settings;
