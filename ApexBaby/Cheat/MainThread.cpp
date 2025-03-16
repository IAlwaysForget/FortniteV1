#include <thread>

#include <DMALibrary/Memory/Memory.h>

#include "MainThread.h"

#include "Offsets.h"
#include "Globals.h"

//CEntityList entity_list;
uint64_t cheeto::local_player;

int cheeto::main_thread()
{
	if (!mem.vHandle)
	{
		ctx.m_logger->Error("Invalid vHandle on main thread\n");
		return 0;
	}

	/*if (!mem.FixCr3())
		ctx.m_logger->Warning("Failed to fix cr3, continuing anyway...\n");*/

	size_t base = mem.GetBaseDaddy("FortniteClient-Win64-Shipping.exe");
	size_t size = mem.GetBaseSize("FortniteClient-Win64-Shipping.exe");

	//mem.DumpMemory(base, "E:\\trashfortnite.exe");

	ctx.m_logger->Info("Base -> 0x%llx, size -> 0x%lx\n", base, size);

	/*
		Other Possible sigs
		48 8B 15 ? ? ? ? 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B F8
		48 8B 1D ? ? ? ? 48 8B F1 48 85 DB 74 31
		48 8B 05 ? ? ? ? 8B 48 18
		48 8B 05 ? ? ? ? F3 0F 10 9B 84 00 00 00
		48 8B 05 ? ? ? ? 0F 57 C0 48 8B CE
	*/
	uint64_t gworld_sig = mem.FindSignature("48 8B 0D ? ? ? ? 48 85 C0 48 0F 45 C8 48 8B 81 D8 01 00 00", base, base + size);
	int relative = mem.Read<int>(gworld_sig + 3);
	while (!gworld_sig)
	{
		gworld_sig = mem.FindSignature("48 8B 0D ? ? ? ? 48 85 C0 48 0F 45 C8 48 8B 81 D8 01 00 00", base, base + size);
		relative = mem.Read<int>(gworld_sig + 3);
	}
	ctx.m_logger->Info("GWorld sig -> 0x%llx\n", gworld_sig);

	/*uint64_t PersistentLevel = mem.Read<uint64_t>(gworld_sig + offsets::PersistentLevel);
	uint64_t WorldSettingsClass = mem.Read<uint64_t>(PersistentLevel + offsets::WorldSettingsClass);
	float WorldGravityZ = mem.Read<float>(WorldSettingsClass + offsets::WorldGravityZ);
	printf("WorldGravityZ -> %f\n", WorldGravityZ);*/
	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> end = start + std::chrono::seconds(5);

	int previous_player_count = 0;
	uint64_t game_state = 0, game_instance = 0, local_players = 0;
	while (true)
	{
		if (!globals::gworld || !game_state || std::chrono::system_clock::now() > end)
		{
			globals::gworld = mem.Read<uint64_t>(gworld_sig + 7 + relative);
			if (!globals::gworld)
				continue;
			game_state = mem.Read<uint64_t>(globals::gworld + offsets::GameState);
			if (!game_state)
				continue;
			game_instance = mem.Read<uint64_t>(globals::gworld + offsets::OwningGameInstance);
			if (!game_instance)
				continue;
			local_players = mem.Read<uint64_t>(game_instance + offsets::LocalPlayers);
			if (!local_players)
				continue;
			local_player = mem.Read<uint64_t>(local_players);
			if (!local_player)
				continue;
			globals::local::player_controller = mem.Read<uint64_t>(local_player + offsets::PlayerController);
			if (!globals::local::player_controller)
				continue;
			//only update end if everything is found
			end = std::chrono::system_clock::now() + std::chrono::seconds(5);
		}

		auto write_handle = mem.CreateScatterHandle();

		static float old_value = 0.f;
		static bool reset = false;
		if (ctx.m_settings.allow_memory_writes)
		{
			globals::local::player_controller = mem.Read<uint64_t>(local_player + offsets::PlayerController);
			if (ctx.m_settings.remove_recoil && globals::local::player_controller)
			{
				if (old_value == 0.f || old_value == -1.0f)
					old_value = mem.Read<float>(globals::local::player_controller + offsets::CustomTimeDilation);
				float new_value = -1.0f;
				mem.AddScatterWriteRequest(write_handle, globals::local::player_controller + offsets::CustomTimeDilation, &new_value, sizeof(new_value));
				mem.ExecuteWriteScatter(write_handle);
				reset = true;
			}
		}
		if (reset && !ctx.m_settings.remove_recoil)
		{
			float new_value = old_value;
			mem.AddScatterWriteRequest(write_handle, globals::local::player_controller + offsets::CustomTimeDilation, &new_value, sizeof(new_value));
			mem.ExecuteWriteScatter(write_handle);
			reset = false;
		}

		uint64_t player_array = 0;
		int player_count = 0;

		auto scat_handle = mem.CreateScatterHandle();

		mem.AddScatterReadRequest(scat_handle, game_state + offsets::PlayerArray, &player_array, 8);
		mem.AddScatterReadRequest(scat_handle, game_state + offsets::PlayerArraySize, &player_count, 4);
		mem.AddScatterReadRequest(scat_handle, globals::local::active_weapon + offsets::WeaponData, &globals::local::weapon_data, sizeof(globals::local::weapon_data));
		mem.ExecuteReadScatter(scat_handle);
		if (player_count)
		{
			uintptr_t tmp_pawn;
			mem.AddScatterReadRequest(scat_handle, globals::local::player_controller + offsets::AcknowledgedPawn, &tmp_pawn, 8);
			mem.AddScatterReadRequest(scat_handle, globals::local::weapon_data + offsets::DisplayName, &globals::local::display_name_ptr, sizeof(globals::local::display_name_ptr));
			mem.ExecuteReadScatter(scat_handle);
			globals::local::pawn = tmp_pawn;
			if (previous_player_count != player_count)
			{
				previous_player_count = player_count;
				if (globals::local::pawn) //should just sneak all of this shit into scatters
				{
					globals::local::player_state = mem.Read<uint64_t>(globals::local::pawn + offsets::PlayerState);
					if (globals::local::player_state)
						globals::local::team_index = mem.Read<int>(globals::local::player_state + offsets::TeamIndex);
				}
				entity_list.Size(player_array, player_count);
				entity_list.SlowCache(); //Cache the pointers we need that should not update..
				mem.CloseScatterHandle(scat_handle);
				mem.CloseScatterHandle(write_handle);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}
			entity_list.SlowCache(); //Cache the pointers we need that should not update..
		}
		mem.CloseScatterHandle(scat_handle);
		mem.CloseScatterHandle(write_handle);

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return 0;
}
