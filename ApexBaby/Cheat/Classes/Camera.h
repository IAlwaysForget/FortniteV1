#pragma once

#include "Transform.h"
#include "Vector.h"

struct camera_data_t
{
	D3DMATRIXUWU temp_matrix { };
	vec3_t rotation { }, location { };
	float field_of_view;
};

extern camera_data_t camera_data;

namespace camera
{
	extern void Update();
	extern void Update(VMMDLL_SCATTER_HANDLE s);
	extern vec2_t WorldToScreen(vec3_t world);
	extern bool IsOnScreen(vec2_t screen);
	extern float distance_from_crosshair(const vec2_t& vec);
	extern float GetDistance(const vec3_t& position);
	extern float ComputeZRotation();
}
