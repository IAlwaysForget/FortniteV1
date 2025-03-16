#pragma once

#include <stdint.h>

#include "Transform.h"
#include <DMALibrary/libs/vmmdll.h>

#include "Cheat/Offsets.h"

inline bool ptr_valid(uint64_t ptr)
{
	return ptr > 0x10000; //yes test
}

struct transforms_t
{
	transform_t component_to_world { };
	transform_t base { }, head { }, pelvis { }, chest { }, spine1 { }, spine2 { };

	transform_t left_hand { }, right_hand { }, left_collar_bone { }, right_collar_bone { }, left_thigh { }, right_thigh { }, left_calf { }, right_calf { };
};

struct bone_positions_t
{
	vec3_t head { }, base { }, pelvis { }, chest { }, spine1 { }, spine2 { };

	vec3_t left_hand { }, right_hand { }, left_collar_bone { }, right_collar_bone { }, left_thigh { }, right_thigh { }, left_calf { }, right_calf { };
};

class CEntity
{
private:
public:
	//will leave all data public for testing purposes

	uint64_t player_state = 0, pawn_private = 0, mesh = 0, bone_array = 0, bone_array_cache = 0, RootComponent = 0;

	//name shit
	uint64_t name_structure_ptr = 0, name_encrypted_buffer_ptr = 0;
	std::wstring name_buffer = L"";
	int name_encrypted_buffer_length = 0;
	std::string decrypted_name = "Bot";

	uint64_t active_weapon = 0;
	uint64_t weapon_data = 0;
	BYTE isdbno = false;
	BYTE isdying = false;
	uint64_t display_name_ptr = 0;
	std::wstring weapon_buffer = L"";
	int weapon_length = 0;
	uint64_t weapon_ptr = 0;
	std::string weapon_name = "None";
	char team_index = 0;
	int id = 0;

	offsets::visible_t visible;
	//float last_submit_time = 0.0f, last_render_time_on_screen = 0.0f;
	vec3_t velocity = { };
	//bone shit
	transforms_t transforms { };
	bone_positions_t bone_positions { };
	CEntity() = default;

	CEntity(uint64_t in_player_state) : player_state(in_player_state)
	{
	}

	void PreparePawnPrivate(VMMDLL_SCATTER_HANDLE handle);

	//amazing naming scheme
	void PrepareName(VMMDLL_SCATTER_HANDLE handle);
	void PrepareA(VMMDLL_SCATTER_HANDLE handle);
	void PrepareB(VMMDLL_SCATTER_HANDLE handle);
	void PrepareC(VMMDLL_SCATTER_HANDLE handle);

	uint64_t GetBoneArray();
	vec3_t ResolveMatrix(transform_t transform);
	void TransformsToPositions();
	void ProcessNameDecryption();

	bool IsVisible();
};
