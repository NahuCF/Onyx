#include "MapTemplates.h"

namespace MMO {
namespace MapTemplates {

// ============================================================
// MAP TEMPLATES DATA
// ============================================================

// Starting Zone (Map ID 1)
MapTemplate CreateStartingZone() {
    MapTemplate tmpl;
    tmpl.id = 1;
    tmpl.name = "Starting Zone";
    tmpl.width = 100.0f;
    tmpl.height = 100.0f;
    tmpl.spawnPoint = Vec2(10.0f, 10.0f);

    // Portal to Dark Forest (on right side)
    Portal portal1;
    portal1.id = 1;
    portal1.position = Vec2(45.0f, 10.0f);
    portal1.size = Vec2(5.0f, 5.0f);
    portal1.destMapId = 2;
    portal1.destPosition = Vec2(18.0f, 10.0f);  // Offset from Dark Forest portal
    tmpl.portals.push_back(portal1);

    // Mob spawns - close to player spawn
    // Starter zone has faster respawns (30s) to help new players
    MobSpawnPoint spawn1;
    spawn1.id = 1;
    spawn1.creatureTemplateId = 1;  // Werewolf
    spawn1.position = Vec2(18.0f, 15.0f);
    spawn1.respawnTimeOverride = 30.0f;  // Fast respawn for starter zone
    tmpl.mobSpawns.push_back(spawn1);

    MobSpawnPoint spawn2;
    spawn2.id = 2;
    spawn2.creatureTemplateId = 1;  // Werewolf
    spawn2.position = Vec2(25.0f, 8.0f);
    spawn2.respawnTimeOverride = 30.0f;  // Fast respawn for starter zone
    tmpl.mobSpawns.push_back(spawn2);

    return tmpl;
}

// Dark Forest (Map ID 2)
MapTemplate CreateDarkForest() {
    MapTemplate tmpl;
    tmpl.id = 2;
    tmpl.name = "Dark Forest";
    tmpl.width = 100.0f;
    tmpl.height = 100.0f;
    tmpl.spawnPoint = Vec2(20.0f, 10.0f);

    // Portal back to Starting Zone (on left side)
    Portal portal1;
    portal1.id = 1;
    portal1.position = Vec2(10.0f, 10.0f);
    portal1.size = Vec2(5.0f, 5.0f);
    portal1.destMapId = 1;
    portal1.destPosition = Vec2(38.0f, 10.0f);  // Offset from Starting Zone portal
    tmpl.portals.push_back(portal1);

    // Mob spawns - close to spawn point
    // Uses creature template defaults (no override)
    MobSpawnPoint spawn1;
    spawn1.id = 1;
    spawn1.creatureTemplateId = 2;  // Forest Spider
    spawn1.position = Vec2(28.0f, 12.0f);
    // No override - uses template/rank default (2 min for NORMAL)
    tmpl.mobSpawns.push_back(spawn1);

    MobSpawnPoint spawn2;
    spawn2.id = 2;
    spawn2.creatureTemplateId = 2;  // Forest Spider
    spawn2.position = Vec2(25.0f, 18.0f);
    tmpl.mobSpawns.push_back(spawn2);

    MobSpawnPoint spawn3;
    spawn3.id = 3;
    spawn3.creatureTemplateId = 1;  // Werewolf
    spawn3.position = Vec2(35.0f, 8.0f);
    tmpl.mobSpawns.push_back(spawn3);

    // Shadow Lord Boss
    MobSpawnPoint spawn4;
    spawn4.id = 4;
    spawn4.creatureTemplateId = 50;  // Shadow Lord (boss)
    spawn4.position = Vec2(50.0f, 25.0f);
    spawn4.respawnTimeOverride = 300.0f;  // 5 minute respawn
    tmpl.mobSpawns.push_back(spawn4);

    return tmpl;
}

} // namespace MapTemplates
} // namespace MMO
