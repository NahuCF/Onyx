-- 0002_creature_npc_gameobject.sql
-- Locked creature/NPC/gameobject/trigger schema per docs/release-pipeline.md
-- "Decided: creature/NPC schema layout".
--
-- AzerothCore-style: a single creature_template holds all living things;
-- npcflag bitfield differentiates roles (vendor, quest-giver, trainer, etc).
-- gameobjects (interactables) are on a parallel track.
--
-- The legacy creature_spawn from 0001 (id SERIAL PK, map_id FK, position_x/y only)
-- is incompatible with the new shape (guid TEXT PK, entry FK to creature_template,
-- position_x/y/z + orientation). Pre-launch the table is empty, so DROP + recreate
-- is acceptable.

DROP TABLE IF EXISTS creature_spawn CASCADE;

-- ============================================================
-- CREATURE TEMPLATE (mobs + NPCs share this)
-- ============================================================

CREATE TABLE creature_template (
    entry           INTEGER PRIMARY KEY,
    name            VARCHAR(100) NOT NULL,
    level_min       SMALLINT NOT NULL DEFAULT 1,
    level_max       SMALLINT NOT NULL DEFAULT 1,
    faction         INTEGER NOT NULL DEFAULT 0,
    npcflag         BIGINT NOT NULL DEFAULT 0,
    model_id        INTEGER NOT NULL DEFAULT 0,
    ai_name         VARCHAR(64) NOT NULL DEFAULT '',
    base_health     INTEGER NOT NULL DEFAULT 1,
    base_mana       INTEGER NOT NULL DEFAULT 0,
    armor           INTEGER NOT NULL DEFAULT 0,
    speed_walk      REAL NOT NULL DEFAULT 1.0,
    speed_run       REAL NOT NULL DEFAULT 1.142857,
    loot_table_id   INTEGER NOT NULL DEFAULT 0,
    scripts         VARCHAR(64) NOT NULL DEFAULT ''
);

-- ============================================================
-- CREATURE SPAWN (per-placement; editor-stable guid)
-- ============================================================

CREATE TABLE creature_spawn (
    guid                TEXT PRIMARY KEY,
    map_id              INTEGER NOT NULL,
    entry               INTEGER NOT NULL REFERENCES creature_template(entry) ON DELETE CASCADE,
    position_x          REAL NOT NULL,
    position_y          REAL NOT NULL,
    position_z          REAL NOT NULL DEFAULT 0.0,
    orientation         REAL NOT NULL DEFAULT 0.0,
    respawn_secs        REAL NOT NULL DEFAULT 60.0,
    wander_radius       REAL NOT NULL DEFAULT 0.0,
    max_count           INTEGER NOT NULL DEFAULT 1,
    equipment_override  INTEGER,
    model_override      INTEGER
);

CREATE INDEX idx_creature_spawn_map ON creature_spawn(map_id);
CREATE INDEX idx_creature_spawn_entry ON creature_spawn(entry);

-- ============================================================
-- CREATURE ADDON (non-combat presentation)
-- ============================================================

CREATE TABLE creature_addon (
    entry           INTEGER PRIMARY KEY REFERENCES creature_template(entry) ON DELETE CASCADE,
    mount_id        INTEGER NOT NULL DEFAULT 0,
    idle_emote      INTEGER NOT NULL DEFAULT 0,
    weapon_main     INTEGER NOT NULL DEFAULT 0,
    weapon_off      INTEGER NOT NULL DEFAULT 0,
    weapon_ranged   INTEGER NOT NULL DEFAULT 0
);

-- ============================================================
-- CREATURE LOOT TEMPLATE
-- ============================================================

CREATE TABLE creature_loot_template (
    table_id        INTEGER NOT NULL,
    item_id         INTEGER NOT NULL,
    chance          REAL NOT NULL DEFAULT 1.0,
    min_count       INTEGER NOT NULL DEFAULT 1,
    max_count       INTEGER NOT NULL DEFAULT 1,
    PRIMARY KEY (table_id, item_id)
);

-- ============================================================
-- NPC VENDOR (joins creature_template by entry)
-- ============================================================

CREATE TABLE npc_vendor (
    entry           INTEGER NOT NULL REFERENCES creature_template(entry) ON DELETE CASCADE,
    item_id         INTEGER NOT NULL,
    max_count       INTEGER NOT NULL DEFAULT 0,        -- 0 = unlimited
    restock_secs    INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (entry, item_id)
);

-- ============================================================
-- NPC GOSSIP (right-click dialogue text)
-- ============================================================

CREATE TABLE npc_gossip (
    entry           INTEGER PRIMARY KEY REFERENCES creature_template(entry) ON DELETE CASCADE,
    text            TEXT NOT NULL DEFAULT ''
);

-- ============================================================
-- NPC TRAINER (spell list per trainer)
-- ============================================================

CREATE TABLE npc_trainer (
    entry           INTEGER NOT NULL REFERENCES creature_template(entry) ON DELETE CASCADE,
    spell_id        INTEGER NOT NULL,
    cost            INTEGER NOT NULL DEFAULT 0,
    required_level  SMALLINT NOT NULL DEFAULT 1,
    PRIMARY KEY (entry, spell_id)
);

-- ============================================================
-- GAMEOBJECT TEMPLATE (doors, chests, mining, mailbox, etc.)
-- ============================================================

CREATE TABLE gameobject_template (
    entry           INTEGER PRIMARY KEY,
    name            VARCHAR(100) NOT NULL,
    type            SMALLINT NOT NULL DEFAULT 0,       -- 0=door, 1=chest, 2=mining, 3=herb, 4=mailbox, 5=ah, 6=banker, 7=quest
    display_id      INTEGER NOT NULL DEFAULT 0,
    data0           INTEGER NOT NULL DEFAULT 0,
    data1           INTEGER NOT NULL DEFAULT 0,
    data2           INTEGER NOT NULL DEFAULT 0,
    data3           INTEGER NOT NULL DEFAULT 0,
    loot_table_id   INTEGER NOT NULL DEFAULT 0,
    scripts         VARCHAR(64) NOT NULL DEFAULT ''
);

-- ============================================================
-- GAMEOBJECT SPAWN
-- ============================================================

CREATE TABLE gameobject_spawn (
    guid            TEXT PRIMARY KEY,
    map_id          INTEGER NOT NULL,
    entry           INTEGER NOT NULL REFERENCES gameobject_template(entry) ON DELETE CASCADE,
    position_x      REAL NOT NULL,
    position_y      REAL NOT NULL,
    position_z      REAL NOT NULL DEFAULT 0.0,
    orientation     REAL NOT NULL DEFAULT 0.0,
    respawn_secs    REAL NOT NULL DEFAULT 60.0,
    state           SMALLINT NOT NULL DEFAULT 0
);

CREATE INDEX idx_gameobject_spawn_map ON gameobject_spawn(map_id);
CREATE INDEX idx_gameobject_spawn_entry ON gameobject_spawn(entry);

-- ============================================================
-- GAMEOBJECT LOOT TEMPLATE
-- ============================================================

CREATE TABLE gameobject_loot_template (
    table_id        INTEGER NOT NULL,
    item_id         INTEGER NOT NULL,
    chance          REAL NOT NULL DEFAULT 1.0,
    min_count       INTEGER NOT NULL DEFAULT 1,
    max_count       INTEGER NOT NULL DEFAULT 1,
    PRIMARY KEY (table_id, item_id)
);

-- ============================================================
-- TRIGGER VOLUME (event zones)
-- ============================================================

CREATE TABLE trigger_volume (
    guid                TEXT PRIMARY KEY,
    map_id              INTEGER NOT NULL,
    shape               SMALLINT NOT NULL DEFAULT 0,   -- 0=BOX, 1=SPHERE, 2=CAPSULE
    position_x          REAL NOT NULL,
    position_y          REAL NOT NULL,
    position_z          REAL NOT NULL DEFAULT 0.0,
    orientation         REAL NOT NULL DEFAULT 0.0,
    half_extent_x       REAL NOT NULL DEFAULT 1.0,
    half_extent_y       REAL NOT NULL DEFAULT 1.0,
    half_extent_z       REAL NOT NULL DEFAULT 1.0,
    radius              REAL NOT NULL DEFAULT 1.0,
    trigger_event       SMALLINT NOT NULL DEFAULT 0,   -- 0=ON_ENTER, 1=ON_EXIT, 2=ON_STAY
    trigger_once        BOOLEAN NOT NULL DEFAULT FALSE,
    trigger_players     BOOLEAN NOT NULL DEFAULT TRUE,
    trigger_creatures   BOOLEAN NOT NULL DEFAULT FALSE,
    script_name         VARCHAR(64) NOT NULL DEFAULT '',
    event_id            INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX idx_trigger_volume_map ON trigger_volume(map_id);
