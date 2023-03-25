#include "dijkstraMapGen.h"
#include "ecsTypes.h"
#include "dungeonUtils.h"

template<typename Callable>
static void query_dungeon_data(flecs::world &ecs, Callable c)
{
  static auto dungeonDataQuery = ecs.query<const DungeonData>();

  dungeonDataQuery.each(c);
}

template<typename Callable>
static void query_characters_positions(flecs::world &ecs, Callable c)
{
  static auto characterPositionQuery = ecs.query<const Position, const Team>();

  characterPositionQuery.each(c);
}

constexpr float invalid_tile_value = 1e5f;

static void init_tiles(std::vector<float> &map, const DungeonData &dd)
{
  map.resize(dd.width * dd.height);
  for (float &v : map)
    v = invalid_tile_value;
}

// scan version, could be implemented as Dijkstra version as well
static void process_dmap(std::vector<float> &map, const DungeonData &dd, const DmapFunc &funcArgs)
{
  bool done = false;
  auto getMapAt = [&](size_t x, size_t y, float def)
  {
    if (x < dd.width && y < dd.width && dd.tiles[y * dd.width + x] == dungeon::floor)
      return map[y * dd.width + x];
    return def;
  };
  auto getMinNei = [&](size_t x, size_t y)
  {
    float val = map[y * dd.width + x];
    val = std::min(val, getMapAt(x - 1, y + 0, val));
    val = std::min(val, getMapAt(x + 1, y + 0, val));
    val = std::min(val, getMapAt(x + 0, y - 1, val));
    val = std::min(val, getMapAt(x + 0, y + 1, val));
    return val;
  };
  
  float maxVal = 0;

  while (!done)
  {
    done = true;
    for (size_t y = 0; y < dd.height; ++y)
      for (size_t x = 0; x < dd.width; ++x)
      {
        const size_t i = y * dd.width + x;
        if (dd.tiles[i] != dungeon::floor)
          continue;
        const float myVal = getMapAt(x, y, invalid_tile_value);
        const float minVal = getMinNei(x, y);
        if (minVal < myVal - 1.f)
        {
          map[i] = minVal + 1.f;
          done = false;
          if (map[i]>maxVal)
            maxVal = map[i];
        }
      }
  }
  maxVal += 1;
  std::vector<float> cacheV;

  float prevValue = 0;
  if (funcArgs.type == DF_EXP)
    prevValue = 1;

  cacheV.push_back(prevValue);

  for (size_t i=1;i<=maxVal;i++)
  {
      
      switch (funcArgs.type)
        {
          case DF_LINEAR:
            prevValue = prevValue + funcArgs.a;
          break;
          case DF_EXP:
            prevValue = prevValue * funcArgs.a;
          break;
          case DF_POW:
            prevValue = pow(i, funcArgs.a);
          break;
        }
    cacheV.push_back(prevValue);
  }

    for (size_t y = 0; y < dd.height; ++y)
      for (size_t x = 0; x < dd.width; ++x)
      {
        const size_t i = y * dd.width + x;
         if (map[i] < invalid_tile_value)
            map[i] = cacheV[(int)map[i]];
      }
    
}

void dmaps::gen_player_approach_map(flecs::world &ecs, std::vector<float> &map, const DmapFunc &funcArgs)
{
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    query_characters_positions(ecs, [&](const Position &pos, const Team &t)
    {
      if (t.team == 0) // player team hardcode
        map[pos.y * dd.width + pos.x] = 0.f;
    });
    process_dmap(map, dd, funcArgs);
  });
}

void dmaps::gen_player_flee_map(flecs::world &ecs, std::vector<float> &map, const DmapFunc &funcArgs)
{
  gen_player_approach_map(ecs, map, funcArgs);
  for (float &v : map)
    if (v < invalid_tile_value)
      v *= -1.2f;
  /* ????
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    process_dmap(map, dd, funcArgs);
  });*/
}

void dmaps::gen_player_archer_map(flecs::world &ecs, std::vector<float> &map, const DmapFunc &funcArgs)
{

  int const shooting_range = 4;
  
  /*gen_player_approach_map(ecs, map);
  for (float &v : map)
    if (v < invalid_tile_value)
      v = abs(v - shooting_range);
        
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    process_dmap(map, dd);
  });*/



  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    query_characters_positions(ecs, [&](const Position &pos, const Team &t)
    {
      if (t.team == 0)
        for (int i =0;i<shooting_range;i++)
          for (int dx=-1;dx<2;dx+=2)
            for (int dy=-1;dy<2;dy+=2)
            {

                int x = pos.x + ((dy != dx ? shooting_range*dy : 0) + i*dx);
                int y = pos.y + ((dy == dx ? -shooting_range*dx : 0) + i*dy);


                if (x < dd.width && y < dd.width && x >= 0 && y >= 0)
                {

                  int range_coords = y * dd.width + x;

                  if (dd.tiles[range_coords] == dungeon::floor)
                  {
                      bool has_obstacle = false;

                      for (int j=0;j<shooting_range;j++)
                      {
                          int check_x = round((float)pos.x + (float)j/(float)shooting_range*((float)x-pos.x));
                          int check_y = round((float)pos.y + (float)j/(float)shooting_range*((float)y-pos.y));


                          int check_coords = check_y * dd.width + check_x; 
                          if (dd.tiles[check_coords] != dungeon::floor)
                          {
                             has_obstacle = true;
                             break;
                          }
                      }

                      if (!has_obstacle)
                        map[range_coords] = 0.f;
                  }
                      
                }



            }
    });
    process_dmap(map, dd, funcArgs);
  });



}

void dmaps::gen_hive_pack_map(flecs::world &ecs, std::vector<float> &map, const DmapFunc &funcArgs)
{
  static auto hiveQuery = ecs.query<const Position, const Hive>();
  query_dungeon_data(ecs, [&](const DungeonData &dd)
  {
    init_tiles(map, dd);
    hiveQuery.each([&](const Position &pos, const Hive &)
    {
      map[pos.y * dd.width + pos.x] = 0.f;
    });
    process_dmap(map, dd, funcArgs);
  });
}

