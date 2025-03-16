#include <DMALibrary/Memory/Memory.h>

#include "Camera.h"

#include "context.h"
#include "imgui.h"
#include "../Globals.h"

#define M_DPI 3.14159265358979323846264338327950288//maybe the PI is not SPECIFIC ENOUGH

static D3DMATRIXUWU to_matrix(vec3_t rot, vec3_t origin = vec3_t(0, 0, 0))
{
	float radPitch = (rot.x * float(M_DPI) / 180.f);
	float radYaw = (rot.y * float(M_DPI) / 180.f);
	float radRoll = (rot.z * float(M_DPI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIXUWU matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}

camera_data_t camera_data { };

void camera::Update()
{
	if (!globals::local::player_controller)
		return;

	camera_data_t new_camera_data { };

	VMMDLL_SCATTER_HANDLE s = mem.CreateScatterHandle(); //do not care about the name just needs to be small to save my fingers

	struct fn_rot_t
	{
		double a; //0x0000
		char pad_0008[24]; //0x0008
		double b; //0x0020
		char pad_0028[424]; //0x0028
		double c; //0x01D0
	} fn_rot;

	float raw_field_of_view = 0;

	mem.AddScatterReadRequest(s, globals::rotation_ptr, &fn_rot.a, 8);
	mem.AddScatterReadRequest(s, globals::rotation_ptr + 0x20, &fn_rot.b, 8);
	mem.AddScatterReadRequest(s, globals::rotation_ptr + 0x1d0, &fn_rot.c, 8);
	mem.AddScatterReadRequest(s, globals::location_ptr, &new_camera_data.location, sizeof(vec3_t));
	mem.AddScatterReadRequest(s, globals::local::player_controller + 0x394, &raw_field_of_view, 4);

	mem.ExecuteReadScatter(s);
	mem.CloseScatterHandle(s);
	new_camera_data.rotation.x = asin(fn_rot.c) * (180.0 / M_DPI);
	new_camera_data.rotation.y = ((atan2(fn_rot.a * -1, fn_rot.b) * (180.0 / M_DPI)) * -1) * -1;
	new_camera_data.field_of_view = raw_field_of_view * 90.f;

	//logger::info("location -> %f %f %f\n", new_camera_data.location.x, new_camera_data.location.y, new_camera_data.location.z);
	//logger::info("rotation -> %f %f %f\n", new_camera_data.rotation.x, new_camera_data.rotation.y, new_camera_data.rotation.z);
	//logger::info("field of view -> %f\n\n", new_camera_data.field_of_view);

	camera_data = new_camera_data;

	camera_data.temp_matrix = to_matrix(camera_data.rotation);
}
float camera::ComputeZRotation()
{
	float roll = atan2(camera_data.temp_matrix.m[1][0], camera_data.temp_matrix.m[0][0]);

	roll = roll * (-180.0 / M_PI);

	roll -= 90.0f;

	if (roll < 0.0f)
		roll += 360.0f;
	else if (roll >= 360.0f)
		roll -= 360.0f;

	return roll;
}

void camera::Update(VMMDLL_SCATTER_HANDLE s)
{
	if (!globals::local::player_controller)
		return;

	camera_data_t new_camera_data { };

	struct fn_rot_t
	{
		double a; //0x0000
		char pad_0008[24]; //0x0008
		double b; //0x0020
		char pad_0028[424]; //0x0028
		double c; //0x01D0
	} fn_rot;

	float raw_field_of_view = 0;

	mem.AddScatterReadRequest(s, globals::rotation_ptr, &fn_rot.a, 8);
	mem.AddScatterReadRequest(s, globals::rotation_ptr + 0x20, &fn_rot.b, 8);
	mem.AddScatterReadRequest(s, globals::rotation_ptr + 0x1d0, &fn_rot.c, 8);
	mem.AddScatterReadRequest(s, globals::location_ptr, &new_camera_data.location, sizeof(vec3_t));
	mem.AddScatterReadRequest(s, globals::local::player_controller + 0x394, &raw_field_of_view, 4);

	mem.ExecuteReadScatter(s);
	new_camera_data.rotation.x = asin(fn_rot.c) * (180.0 / M_DPI);
	new_camera_data.rotation.y = ((atan2(fn_rot.a * -1, fn_rot.b) * (180.0 / M_DPI)) * -1) * -1;
	new_camera_data.field_of_view = raw_field_of_view * 90.f;

	//logger::info("location -> %f %f %f\n", new_camera_data.location.x, new_camera_data.location.y, new_camera_data.location.z);
	//logger::info("rotation -> %f %f %f\n", new_camera_data.rotation.x, new_camera_data.rotation.y, new_camera_data.rotation.z);
	//logger::info("field of view -> %f\n\n", new_camera_data.field_of_view);

	camera_data = new_camera_data;

	camera_data.temp_matrix = to_matrix(camera_data.rotation);
}

vec2_t camera::WorldToScreen(vec3_t world)
{
	vec3_t vAxisX = vec3_t(camera_data.temp_matrix.m[0][0], camera_data.temp_matrix.m[0][1], camera_data.temp_matrix.m[0][2]);
	vec3_t vAxisY = vec3_t(camera_data.temp_matrix.m[1][0], camera_data.temp_matrix.m[1][1], camera_data.temp_matrix.m[1][2]);
	vec3_t vAxisZ = vec3_t(camera_data.temp_matrix.m[2][0], camera_data.temp_matrix.m[2][1], camera_data.temp_matrix.m[2][2]);

	vec3_t vDelta = {world.x - camera_data.location.x, world.y - camera_data.location.y, world.z - camera_data.location.z};
	vec3_t vTransformed = vec3_t(vDelta.dot(vAxisY), vDelta.dot(vAxisZ), vDelta.dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	//for now resolution is hardcoded...
	ImVec2 size = ctx.m_settings.menu_size;
	float x = (size.x / 2.0f) + vTransformed.x * (((size.x / 2.0f) / tanf(camera_data.field_of_view * (float)M_DPI / 360.f))) / vTransformed.z;
	float y = (size.y / 2.0f) - vTransformed.y * (((size.x / 2.0f) / tanf(camera_data.field_of_view * (float)M_DPI / 360.f))) / vTransformed.z;

	return vec2_t(x, y);
}

bool camera::IsOnScreen(vec2_t screen)
{
	ImVec2 size = ctx.m_settings.menu_size;
	return !(screen.x < 1 || screen.y < 1 || screen.x > size.x || screen.y > size.y);
}

float camera::distance_from_crosshair(const vec2_t& vec)
{
	ImVec2 size = ctx.m_settings.menu_size;
	const float center_x = size.x / 2.0f;
	const float center_y = size.y / 2.0f;

	const float dx = vec.x - center_x;
	const float dy = vec.y - center_y;

	return sqrt(dx * dx + dy * dy);
}

float camera::GetDistance(const vec3_t& position)
{
	const auto difference = vec3_t(
		camera_data.location.x - position.x,
		camera_data.location.y - position.y,
		camera_data.location.z - position.z);

	return sqrt(
		powf(difference.x, 2.0f) +
		powf(difference.y, 2.0f) +
		powf(difference.z, 2.0f)
	)/100.f;
}