#include "Globals.h"
namespace globals
{
	uint64_t gworld = 0;
	uint64_t location_ptr = 0;
	uint64_t rotation_ptr = 0;
	uint64_t targeted_player = 0;

	namespace local
	{
		uint64_t player_controller = 0;
		uint64_t pawn = 0;
		uint64_t player_state = 0;
		int team_index = 0;

		uint64_t active_weapon = 0;
		uint64_t weapon_data = 0;
		uint64_t display_name_ptr = 0;
		std::mutex weapon_name_mutex;
		std::string weapon_name;
		weapon_zoom zoom;
	}
}
