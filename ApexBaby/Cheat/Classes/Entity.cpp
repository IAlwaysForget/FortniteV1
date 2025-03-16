#include <DMALibrary/Memory/Memory.h>

#include "Entity.h"

#include "context.h"
#include "../Offsets.h"

uint64_t CEntity::GetBoneArray()
{
	if (!bone_array)
		return bone_array_cache;

	return bone_array;
}

void CEntity::PreparePawnPrivate(VMMDLL_SCATTER_HANDLE handle)
{
	//please god make it so if the size is defaulted to 0 it just does sizeof tyty
	mem.AddScatterReadRequest(handle, player_state + offsets::PawnPrivate, &pawn_private, sizeof(pawn_private));
	mem.AddScatterReadRequest(handle, player_state + offsets::TeamIndex, &team_index, sizeof(team_index));
	mem.AddScatterReadRequest(handle, player_state + offsets::NameStructure, &name_structure_ptr, sizeof(name_structure_ptr));
}

void CEntity::PrepareA(VMMDLL_SCATTER_HANDLE handle)
{
	mem.AddScatterReadRequest(handle, pawn_private + offsets::RootComponent, &RootComponent, sizeof(RootComponent));
	mem.AddScatterReadRequest(handle, pawn_private + offsets::Mesh, &mesh, sizeof(mesh));
	mem.AddScatterReadRequest(handle, name_structure_ptr + 0x8, &name_encrypted_buffer_ptr, sizeof(name_encrypted_buffer_ptr));
	mem.AddScatterReadRequest(handle, name_structure_ptr + 0x10, &name_encrypted_buffer_length, sizeof(name_encrypted_buffer_length));
	mem.AddScatterReadRequest(handle, pawn_private + offsets::CurrentWeapon, &active_weapon, sizeof(uintptr_t));
}

void CEntity::PrepareB(VMMDLL_SCATTER_HANDLE handle)
{
	mem.AddScatterReadRequest(handle, mesh + offsets::BoneArray, &bone_array, sizeof(bone_array));
	mem.AddScatterReadRequest(handle, mesh + offsets::BoneArrayCache, &bone_array_cache, sizeof(bone_array_cache));
	mem.AddScatterReadRequest(handle, active_weapon + offsets::WeaponData, &weapon_data, sizeof(weapon_data));
	if (name_encrypted_buffer_length < 0 || name_encrypted_buffer_length > 0x1000)
		return;
	//Modify this to a smart pointer? Or atleast clean it...
	//if (name_buffer)
	//	delete name_buffer;
	name_buffer.resize(name_encrypted_buffer_length);
	//name_buffer = new wchar_t[name_encrypted_buffer_length];

	if (name_encrypted_buffer_ptr)
		mem.AddScatterReadRequest(handle, name_encrypted_buffer_ptr, name_buffer.data(), name_encrypted_buffer_length * sizeof(wchar_t));
}

void CEntity::PrepareC(VMMDLL_SCATTER_HANDLE handle)
{
	auto bone_array = GetBoneArray();
	if (!bone_array)
		return;

	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanBase * 0x60), &transforms.base, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanHead * 0x60), &transforms.head, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanPelvis * 0x60), &transforms.pelvis, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanChest * 0x60), &transforms.chest, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanSpine3 * 0x60), &transforms.spine1, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanSpine1 * 0x60), &transforms.spine2, sizeof(transform_t));

	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanLHand * 0x60), &transforms.left_hand, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanRHand * 0x60), &transforms.right_hand, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanLForearm1 * 0x60), &transforms.left_collar_bone, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanRForearm1 * 0x60), &transforms.right_collar_bone, sizeof(transform_t));

	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanRThigh2 * 0x60), &transforms.right_thigh, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanLThigh2 * 0x60), &transforms.left_thigh, sizeof(transform_t));

	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanLCalf * 0x60), &transforms.left_calf, sizeof(transform_t));
	mem.AddScatterReadRequest(handle, bone_array + ((int)e_bone_ids::HumanRCalf * 0x60), &transforms.right_calf, sizeof(transform_t));
}

vec3_t CEntity::ResolveMatrix(transform_t transform)
{
	D3DMATRIXUWU matrix = Multiply(transform.ToMatrixWithScale(), transforms.component_to_world.ToMatrixWithScale());

	return vec3_t {matrix._41, matrix._42, matrix._43};
}

void CEntity::TransformsToPositions()
{
	bone_positions.base = ResolveMatrix(transforms.base);
	bone_positions.head = ResolveMatrix(transforms.head);
	bone_positions.pelvis = ResolveMatrix(transforms.pelvis);
	bone_positions.chest = ResolveMatrix(transforms.chest);
	bone_positions.spine1 = ResolveMatrix(transforms.spine1);
	bone_positions.spine2 = ResolveMatrix(transforms.spine2);

	bone_positions.left_hand = ResolveMatrix(transforms.left_hand);
	bone_positions.right_hand = ResolveMatrix(transforms.right_hand);

	bone_positions.left_collar_bone = ResolveMatrix(transforms.left_collar_bone);
	bone_positions.right_collar_bone = ResolveMatrix(transforms.right_collar_bone);

	bone_positions.right_thigh = ResolveMatrix(transforms.right_thigh);
	bone_positions.left_thigh = ResolveMatrix(transforms.left_thigh);

	bone_positions.left_calf = ResolveMatrix(transforms.left_calf);
	bone_positions.right_calf = ResolveMatrix(transforms.right_calf);
}

void CEntity::ProcessNameDecryption()
{
	if (name_encrypted_buffer_length <= 0 || name_encrypted_buffer_length > 128)
		return;

	if (!name_structure_ptr || !name_encrypted_buffer_ptr)
		return;

	int v25 = name_encrypted_buffer_length - 1;
	int v26 = 0;
	uint16_t* name_buffer_ptr = (uint16_t*)name_buffer.data();

	int last_used_char = name_encrypted_buffer_length - 1;;
	for (int i = (v25) & 3; ; *name_buffer_ptr++ += i & 7) //tbh no idea what's going on here ask IDA
	{
		if (v26 >= last_used_char)
			break;

		i += 3;
		++v26;
	}

	std::wstring temp_wstring(name_buffer);
	decrypted_name = std::string(temp_wstring.begin(), temp_wstring.end());
	//delete name_buffer;
	if (decrypted_name.empty())
		decrypted_name = "probably a bot";
}

bool CEntity::IsVisible()
{
	return visible.LastRenderTimeOnScreen + 0.15f >= visible.LastSubmitTime;
}
