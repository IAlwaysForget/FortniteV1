#include <DMALibrary/Memory/Memory.h>

#include "EntityList.h"

#include "Camera.h"
#include "Cheat/Globals.h"
#include "Cheat/MainThread.h"
#include "Cheat/Offsets.h"

void CEntityList::SlowCache()
{
	if (!array_start || array_size == 0 || array_size > 1000000 || array_size < 0) //https://media4.giphy.com/media/sEULHciNa7tUQ/giphy.gif
		return;

	auto scat_handle = mem.CreateScatterHandle();

	std::unique_ptr<uint64_t[]> entity_raw_ptrs = std::make_unique<uint64_t[]>(array_size);
	mem.Read(array_start, entity_raw_ptrs.get(), array_size * sizeof(uint64_t));

	std::vector<CEntity> tmp_entities { };
	for (int i = 0; i <= array_size; i++)
	{
		uint64_t raw_player_state = entity_raw_ptrs[i];
		if (ptr_valid(raw_player_state))
			tmp_entities.push_back(CEntity(entity_raw_ptrs[i]));
	}

	if (tmp_entities.empty())
	{
		mem.CloseScatterHandle(scat_handle);
		return;
	}
	for (size_t i = 0; i < tmp_entities.size(); i++)
		tmp_entities[i].PreparePawnPrivate(scat_handle);
	mem.ExecuteReadScatter(scat_handle);

	for (size_t i = 0; i < tmp_entities.size(); i++)
		tmp_entities[i].PrepareA(scat_handle);
	mem.AddScatterReadRequest(scat_handle, globals::local::pawn + offsets::CurrentWeapon, &globals::local::active_weapon, sizeof(uintptr_t));
	mem.ExecuteReadScatter(scat_handle);

	for (size_t i = 0; i < tmp_entities.size(); i++)
		tmp_entities[i].PrepareB(scat_handle);
	mem.ExecuteReadScatter(scat_handle);

	for (size_t i = 0; i < tmp_entities.size(); i++)
		tmp_entities[i].ProcessNameDecryption();

	//This part is fast cache basicly. we need this so the entities are already populated with some data when we send it over.
	//so we dont skip frames on ESP (:
	for (size_t i = 0; i < tmp_entities.size(); i++)
	{
		if (tmp_entities[i].player_state == globals::local::player_state)
			m_localplayer = tmp_entities[i];
		mem.AddScatterReadRequest(scat_handle, tmp_entities[i].mesh + offsets::ComponentToWorld, &tmp_entities[i].transforms.component_to_world, sizeof(tmp_entities[i].transforms.component_to_world));
		mem.AddScatterReadRequest(scat_handle, tmp_entities[i].mesh + offsets::LastSubmitTime, &tmp_entities[i].visible, sizeof(tmp_entities[i].visible));
	}
	mem.ExecuteReadScatter(scat_handle);

	for (size_t i = 0; i < tmp_entities.size(); i++)
		tmp_entities[i].PrepareC(scat_handle);
	mem.ExecuteReadScatter(scat_handle);

	for (size_t i = 0; i < tmp_entities.size(); i++)
		tmp_entities[i].TransformsToPositions();

	cache_entities = tmp_entities;
	mem.CloseScatterHandle(scat_handle);
}

void CEntityList::FastCache(std::vector<CEntity>& ent)
{
	if (ent.empty())
		return;
	int weapon_length = 0;
	uint64_t weapon_ptr = 0;
	auto scat_handle = mem.CreateScatterHandle();
	for (size_t i = 0; i < ent.size(); i++)
	{
		mem.AddScatterReadRequest(scat_handle, ent[i].RootComponent + offsets::ComponentVelocity, &ent[i].velocity, sizeof(ent[i].velocity));
		mem.AddScatterReadRequest(scat_handle, ent[i].mesh + offsets::ComponentToWorld, &ent[i].transforms.component_to_world, sizeof(ent[i].transforms.component_to_world));
		mem.AddScatterReadRequest(scat_handle, ent[i].mesh + offsets::LastSubmitTime, &ent[i].visible, sizeof(ent[i].visible));
		mem.AddScatterReadRequest(scat_handle, ent[i].pawn_private + offsets::bIsDBNO, &ent[i].isdbno, sizeof(ent[i].isdbno));
		mem.AddScatterReadRequest(scat_handle, ent[i].pawn_private + offsets::bIsDying, &ent[i].isdying, sizeof(ent[i].isdying));
		mem.AddScatterReadRequest(scat_handle, ent[i].weapon_data + offsets::DisplayName, &ent[i].display_name_ptr, sizeof(ent[i].display_name_ptr));
	}
	mem.AddScatterReadRequest(scat_handle, globals::gworld + 0x110, &globals::location_ptr, 8); //both are in padding
	mem.AddScatterReadRequest(scat_handle, globals::gworld + 0x120, &globals::rotation_ptr, 8);
	mem.AddScatterReadRequest(scat_handle, globals::local::player_controller + offsets::TargetedFortPawn, &globals::targeted_player, sizeof(uintptr_t));
	mem.AddScatterReadRequest(scat_handle, globals::local::display_name_ptr + 0x28, &weapon_ptr, sizeof(weapon_ptr));
	mem.AddScatterReadRequest(scat_handle, globals::local::display_name_ptr + 0x30, &weapon_length, sizeof(weapon_length));
	mem.ExecuteReadScatter(scat_handle);

	for (size_t i = 0; i < ent.size(); i++)
	{
		ent[i].PrepareC(scat_handle);
		mem.AddScatterReadRequest(scat_handle, ent[i].display_name_ptr + 0x28, &ent[i].weapon_ptr, sizeof(ent[i].weapon_ptr));
		mem.AddScatterReadRequest(scat_handle, ent[i].display_name_ptr + 0x30, &ent[i].weapon_length, sizeof(ent[i].weapon_length));
	}
	mem.ExecuteReadScatter(scat_handle);

	for (size_t i = 0; i < ent.size(); i++)
	{
		if (ent[i].weapon_length > 0 && ent[i].weapon_length < 50)
		{
			ent[i].weapon_buffer.resize(ent[i].weapon_length);
			if (ent[i].weapon_ptr)
				mem.AddScatterReadRequest(scat_handle, ent[i].weapon_ptr, ent[i].weapon_buffer.data(), ent[i].weapon_length * sizeof(wchar_t));
		}
	}
	mem.ExecuteReadScatter(scat_handle);

	mem.AddScatterReadRequest(scat_handle, globals::local::active_weapon + offsets::bIsTargeting, &globals::local::zoom, sizeof(globals::local::weapon_zoom));
	if (weapon_length > 0 && weapon_length < 50)
	{
		wchar_t* ftext_buf = new wchar_t[weapon_length];
		mem.AddScatterReadRequest(scat_handle, weapon_ptr, ftext_buf, weapon_length * sizeof(wchar_t));
		camera::Update(scat_handle); //Camera also executes the scatter
		std::wstring wstr_buf(ftext_buf);

		globals::local::weapon_name_mutex.lock();
		globals::local::weapon_name = std::string(wstr_buf.begin(), wstr_buf.end());
		globals::local::weapon_name_mutex.unlock();
		delete[] ftext_buf;
	}
	else
		camera::Update(scat_handle); //Camera also executes the scatter

	for (size_t i = 0; i < ent.size(); i++)
	{
		ent[i].TransformsToPositions();

		std::wstring wstr_buf(ent[i].weapon_buffer);
		ent[i].weapon_name = std::string(wstr_buf.begin(), wstr_buf.end());
	}
	mem.CloseScatterHandle(scat_handle);
}
