#pragma once

#include "CreatureTemplate.h"
#include <unordered_map>

namespace MMO {
namespace CreatureTemplates {

// ============================================================
// CREATURE TEMPLATES (Data) - Simple mobs
// ============================================================

// Factory functions
CreatureTemplate CreateWerewolf();
CreatureTemplate CreateForestSpider();
CreatureTemplate CreateShadowAdd();
CreatureTemplate CreateShadowLord();

// Template registry
std::unordered_map<uint32_t, CreatureTemplate>& GetTemplateRegistry();
const CreatureTemplate* GetTemplate(uint32_t id);

} // namespace CreatureTemplates
} // namespace MMO
