#pragma once

#include <corecrt_math_defines.h>
#include <cmath>
#include <math.h>
#include <algorithm>

inline float _sqrt(float fNumber)
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = fNumber * 0.5F;
	y = fNumber;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));
	return 1 / y;
}

struct vec2_t
{
	float x, y;

	vec2_t operator+(const vec2_t& v) const
	{
		return vec2_t {x + v.x, y + v.y};
	}

	vec2_t operator-(const vec2_t& v) const
	{
		return vec2_t {x - v.x, y - v.y};
	}

	vec2_t operator*(float s) const
	{
		return vec2_t {x * s, y * s};
	}

	vec2_t operator/(float s) const
	{
		return vec2_t {x / s, y / s};
	}

	vec2_t& operator*=(float s)
	{
		x *= s;
		y *= s;
		return *this;
	}

	vec2_t& operator+=(const vec2_t& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	vec2_t& operator-=(const vec2_t& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	float length() const
	{
		return _sqrt(x * x + y * y);
	}

	static float Distance(vec2_t x, vec2_t y)
	{
		return sqrtf((x.x - y.x) * (x.x - y.x) + (x.y - y.y) * (x.y - y.y));
	}

	void normalize()
	{
		float fLength = length();
		if (fLength != 0)
		{
			x /= fLength;
			y /= fLength;
		}
	}
};

struct vec3_t
{
	double x, y, z;

	float dot(vec3_t b)
	{
		return (x * b.x + y * b.y + z * b.z);
	}

	inline double Length()
	{
		return sqrt(x * x + y * y + z * z);
	}

	__forceinline vec3_t ClampAngles()
	{
		if (x < -180.0)
			x += 360.0;

		if (x > 180.0)
			x -= 360.0;

		if (y > 89.0)
			y -= 360.0;

		if (y < -89.0)
			y += 360.0;

		return vec3_t(x, y, 0);
	}

	float Distance(const vec3_t& v) const
	{
		return float(sqrtf(powf(v.x - x, 2.0f) + powf(v.y - y, 2.0f) + powf(v.z - z, 2.0f)));
	}

	vec3_t operator*(vec3_t v) const
	{
		return vec3_t {x * v.x, y * v.y, z * v.z};
	}

	vec3_t operator*(float s) const
	{
		return vec3_t {x * s, y * s, z * s};
	}

	vec3_t operator/(float s) const
	{
		return vec3_t {x / s, y / s, z / s};
	}

	vec3_t operator+(const vec3_t& v) const
	{
		return vec3_t {x + v.x, y + v.y, z + v.z};
	}

	vec3_t operator+=(const vec3_t& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	vec3_t operator/(const vec3_t& v) const
	{
		return vec3_t {x / v.x, y / v.y, z / v.z};
	}

	vec3_t operator-(const vec3_t& v) const
	{
		return vec3_t {x - v.x, y - v.y, z - v.z};
	}

	vec3_t operator/=(float s)
	{
		x /= s;
		y /= s;
		z /= s;
		return *this;
	}

	vec3_t operator/=(const vec3_t& v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	float magnitude() const
	{
		return sqrtf(x * x + y * y + z * z);
	}
};

struct quat_t
{
	double x;
	double y;
	double z;
	double w;
};
