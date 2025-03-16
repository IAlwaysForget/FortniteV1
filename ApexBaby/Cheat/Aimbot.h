#pragma once
#include <vector>

#include "imgui.h"

class CEntity;

namespace Aim
{
	extern void Run(std::vector<CEntity> entities, ImDrawList* drawList);
	extern void DrawAimbot();
}
