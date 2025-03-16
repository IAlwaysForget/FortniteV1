#pragma once

#include <stdint.h>
#include <string>
#include <mutex>
namespace globals
{
	extern uint64_t gworld;
	extern uint64_t location_ptr;
	extern uint64_t rotation_ptr;
	extern uint64_t targeted_player;

	namespace local
	{
		extern uint64_t player_controller;
		extern uint64_t pawn;
		extern uint64_t player_state;
		extern int team_index;

		struct weapon_zoom
		{
			bool bIsPlayingFireFX : 1; // 0x0D30:0 (0x0001)  
			bool bFireFXTriggered : 1; // 0x0D30:1 (0x0001)  
			bool bWeaponFireCustomAudioTriggered : 1; // 0x0D30:2 (0x0001)  
			bool bAllowTargeting : 1; // 0x0D30:3 (0x0001)  
			bool bExitTargetingWhenPrimaryIsFired : 1; // 0x0D30:4 (0x0001)  
			bool bIsTargeting : 1; // 0x0D30:5 (0x0001)  //TARGET ZOOM IS WHEN ZOOMING IN it will become true, test the rest later 
			bool bForceCameraTargetSource : 1; // 0x0D30:6 (0x0001)  
			bool bTraceThroughPawns : 1; // 0x0D30:7 (0x0001)  
		};

		extern uint64_t active_weapon;
		extern uint64_t weapon_data;
		extern uint64_t display_name_ptr;
		extern std::mutex weapon_name_mutex;
		extern std::string weapon_name;
		extern weapon_zoom zoom;
	}
}
