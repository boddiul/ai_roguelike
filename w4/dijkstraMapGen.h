#pragma once
#include <vector>
#include <flecs.h>
#include "ecsTypes.h"

namespace dmaps
{
  void gen_player_approach_map(flecs::world &ecs, std::vector<float> &map,const DmapFunc &funcArgs);
  void gen_player_flee_map(flecs::world &ecs, std::vector<float> &map,const DmapFunc &funcArgs);
  void gen_player_archer_map(flecs::world &ecs, std::vector<float> &map,const DmapFunc &funcArgs);
  void gen_hive_pack_map(flecs::world &ecs, std::vector<float> &map,const DmapFunc &funcArgs);
};

