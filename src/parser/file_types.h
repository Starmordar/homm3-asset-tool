#pragma once

#include <unordered_set>

namespace FileType {
  inline const std::unordered_set<int> def_types{
      64, // spell
      65, // spritedef
      66, // creature
      67, // adventure object
      68, // adventure hero
      69, // terrain
      70, // cursor
      71, // interface button and town buildings
      72, // sprite frame
      73, // combat hero
  };

  inline const std::unordered_map<uint32_t, std::vector<std::string>> def_groups{
      {66,
       {"moving",         "mouse over", "standing",        "getting hit",         "defend",
        "death",          "???",        "turn left",       "turn right",          "turn left",
        "turn right",     "attack up",  "attack straight", "attack down",         "shoot up",
        "shoot straight", "shoot down", "hex attack up",   "hex attack straight", "hex attack down",
        "start moving",   "stop moving"}},
      {68,
       {"turn north", "turn northeast", "turn east", "turn southeast", "turn south", "move north",
        "move northeast", "move east", "move southeast", "move south"}},
      {73, {"standing", "standing", "failure", "success", "spellcasting"}}};
} // namespace FileType