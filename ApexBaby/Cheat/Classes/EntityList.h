#pragma once

#include <vector>

#include "Entity.h"

class CEntityList
{
private:
	uint64_t array_start = 0;
	int array_size = 0;
	std::vector<CEntity> entities { };
	std::vector<CEntity> cache_entities { };

public:
	CEntity m_localplayer;

	CEntityList()
	{
	}

	void Size(uint64_t in_array_start, int in_array_size)
	{
		array_start = in_array_start;
		array_size = in_array_size;
	}

	void FastCache(std::vector<CEntity>& ent);
	void SlowCache();

	std::vector<CEntity> GetPlayers()
	{
		if (cache_entities.empty())
			return entities;

		entities = cache_entities;
		return entities;
	}
};

inline std::vector<CEntity> g_entities { };
