#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace FileTypes {
  inline const std::unordered_map<uint32_t, std::string> type_name_map{
      {64, "def (spell)"},          {65, "def (spritedef)"},
      {66, "def (creature)"},       {67, "def (adventure object)"},
      {68, "def (adventure hero)"}, {69, "def (terrain)"},
      {70, "def (cursor)"},         {71, "def (interface)"},
      {72, "def (sprite frame)"},   {73, "def (combat hero)"},
  };

  inline const std::unordered_map<std::string, std::vector<std::string>> animation_groups{
      {"def (creature)",
       {"moving",         "mouse over", "standing",        "getting hit",         "defend",
        "death",          "???",        "turn left",       "turn right",          "turn left",
        "turn right",     "attack up",  "attack straight", "attack down",         "shoot up",
        "shoot straight", "shoot down", "hex attack up",   "hex attack straight", "hex attack down",
        "start moving",   "stop moving"}},

      {"def (adventure hero)",
       {"turn north", "turn northeast", "turn east", "turn southeast", "turn south", "move north",
        "move northeast", "move east", "move southeast", "move south"}},

      {"def (combat hero)", {"standing", "standing", "failure", "success", "spellcasting"}}};
} // namespace FileTypes