-- 0001_baseline.sql
-- Baseline schema for MMO. Contents identical to the legacy MMOGame/Database/schema.sql.
-- Subsequent migrations build on this.
-- PostgreSQL.

-- ============================================================
-- MIGRATION TRACKING (created here so the bootstrap is part of the SQL,
-- but MigrationRunner::ApplyAll also creates it defensively before reading)
-- ============================================================

CREATE TABLE IF NOT EXISTS schema_migrations (
    id          INTEGER PRIMARY KEY,
    applied_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- ============================================================
-- ACCOUNTS
-- ============================================================

CREATE TABLE IF NOT EXISTS accounts (
    id              BIGSERIAL PRIMARY KEY,
    username        VARCHAR(32) UNIQUE NOT NULL,
    email           VARCHAR(255) UNIQUE NOT NULL,
    password_hash   VARCHAR(255) NOT NULL,
    salt            VARCHAR(64) NOT NULL,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login      TIMESTAMP,
    is_banned       BOOLEAN DEFAULT FALSE,
    ban_reason      TEXT
);

CREATE INDEX IF NOT EXISTS idx_accounts_username ON accounts(username);
CREATE INDEX IF NOT EXISTS idx_accounts_email ON accounts(email);

-- ============================================================
-- CHARACTERS
-- ============================================================

CREATE TABLE IF NOT EXISTS characters (
    id              BIGSERIAL PRIMARY KEY,
    account_id      BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    name            VARCHAR(32) UNIQUE NOT NULL,
    race            SMALLINT NOT NULL DEFAULT 1,
    class           SMALLINT NOT NULL,
    level           INT DEFAULT 1,
    experience      BIGINT DEFAULT 0,
    money           INT DEFAULT 0,

    zone_id         VARCHAR(64) NOT NULL DEFAULT '1',
    position_x      REAL NOT NULL DEFAULT 0.0,
    position_y      REAL NOT NULL DEFAULT 0.0,
    position_z      REAL NOT NULL DEFAULT 0.0,
    orientation     REAL NOT NULL DEFAULT 0.0,

    max_health      INT NOT NULL,
    max_mana        INT NOT NULL,
    current_health  INT NOT NULL,
    current_mana    INT NOT NULL,

    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    played_time     BIGINT DEFAULT 0,
    last_played     TIMESTAMP,
    is_deleted      BOOLEAN DEFAULT FALSE
);

CREATE INDEX IF NOT EXISTS idx_characters_account ON characters(account_id);
CREATE INDEX IF NOT EXISTS idx_characters_name ON characters(name);

-- ============================================================
-- SESSIONS
-- ============================================================

CREATE TABLE IF NOT EXISTS sessions (
    token           VARCHAR(255) PRIMARY KEY,
    account_id      BIGINT NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    character_id    BIGINT REFERENCES characters(id) ON DELETE SET NULL,
    world_server    VARCHAR(64),
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at      TIMESTAMP NOT NULL,
    ip_address      VARCHAR(45)
);

CREATE INDEX IF NOT EXISTS idx_sessions_account ON sessions(account_id);
CREATE INDEX IF NOT EXISTS idx_sessions_expires ON sessions(expires_at);

-- ============================================================
-- CHARACTER COOLDOWNS
-- ============================================================

CREATE TABLE IF NOT EXISTS character_cooldowns (
    character_id    BIGINT NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    ability_id      INT NOT NULL,
    remaining       REAL NOT NULL,
    PRIMARY KEY (character_id, ability_id)
);

CREATE INDEX IF NOT EXISTS idx_cooldowns_character ON character_cooldowns(character_id);

-- ============================================================
-- ITEM INSTANCES
-- ============================================================

CREATE SEQUENCE IF NOT EXISTS item_instance_seq START WITH 1;

-- ============================================================
-- CHARACTER INVENTORY
-- ============================================================

CREATE TABLE IF NOT EXISTS character_inventory (
    instance_id     BIGINT PRIMARY KEY DEFAULT nextval('item_instance_seq'),
    character_id    BIGINT NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    slot            SMALLINT NOT NULL,
    template_id     INT NOT NULL,
    stack_count     INT DEFAULT 1,
    UNIQUE(character_id, slot)
);

CREATE INDEX IF NOT EXISTS idx_inventory_character ON character_inventory(character_id);

-- ============================================================
-- CHARACTER EQUIPMENT
-- ============================================================

CREATE TABLE IF NOT EXISTS character_equipment (
    instance_id     BIGINT PRIMARY KEY DEFAULT nextval('item_instance_seq'),
    character_id    BIGINT NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    slot            SMALLINT NOT NULL,
    template_id     INT NOT NULL,
    UNIQUE(character_id, slot)
);

CREATE INDEX IF NOT EXISTS idx_equipment_character ON character_equipment(character_id);

-- ============================================================
-- MAP TEMPLATE
-- ============================================================

CREATE TABLE IF NOT EXISTS map_template (
    id          SERIAL PRIMARY KEY,
    name        VARCHAR(64) NOT NULL,
    width       REAL NOT NULL DEFAULT 100.0,
    height      REAL NOT NULL DEFAULT 100.0,
    spawn_x     REAL NOT NULL DEFAULT 0.0,
    spawn_y     REAL NOT NULL DEFAULT 0.0,
    spawn_z     REAL NOT NULL DEFAULT 0.0
);

-- ============================================================
-- PORTALS BETWEEN MAPS
-- ============================================================

CREATE TABLE IF NOT EXISTS portal (
    id              SERIAL PRIMARY KEY,
    map_id          INT NOT NULL REFERENCES map_template(id) ON DELETE CASCADE,
    position_x      REAL NOT NULL,
    position_y      REAL NOT NULL,
    size_x          REAL NOT NULL DEFAULT 5.0,
    size_y          REAL NOT NULL DEFAULT 5.0,
    dest_map_id     INT NOT NULL,
    dest_x          REAL NOT NULL,
    dest_y          REAL NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_portal_map ON portal(map_id);

-- ============================================================
-- CREATURE SPAWN POINTS (legacy shape; replaced in 0002)
-- ============================================================

CREATE TABLE IF NOT EXISTS creature_spawn (
    id                  SERIAL PRIMARY KEY,
    map_id              INT NOT NULL REFERENCES map_template(id) ON DELETE CASCADE,
    creature_template_id INT NOT NULL,
    position_x          REAL NOT NULL,
    position_y          REAL NOT NULL,
    respawn_time        REAL NOT NULL DEFAULT 0.0,
    corpse_decay_time   REAL NOT NULL DEFAULT 0.0
);

CREATE INDEX IF NOT EXISTS idx_creature_spawn_map ON creature_spawn(map_id);

-- ============================================================
-- RACE DEFINITIONS
-- ============================================================

CREATE TABLE IF NOT EXISTS race_template (
    id              SMALLINT PRIMARY KEY,
    name            VARCHAR(32) NOT NULL,
    faction         SMALLINT NOT NULL DEFAULT 0,
    class_mask      INT NOT NULL DEFAULT 0,
    bonus_strength  SMALLINT NOT NULL DEFAULT 0,
    bonus_agility   SMALLINT NOT NULL DEFAULT 0,
    bonus_stamina   SMALLINT NOT NULL DEFAULT 0,
    bonus_intellect SMALLINT NOT NULL DEFAULT 0
);

-- ============================================================
-- CLASS DEFINITIONS
-- ============================================================

CREATE TABLE IF NOT EXISTS class_template (
    id              SMALLINT PRIMARY KEY,
    name            VARCHAR(32) NOT NULL,
    base_health     INT NOT NULL,
    base_mana       INT NOT NULL,
    base_strength   SMALLINT NOT NULL DEFAULT 0,
    base_agility    SMALLINT NOT NULL DEFAULT 0,
    base_stamina    SMALLINT NOT NULL DEFAULT 0,
    base_intellect  SMALLINT NOT NULL DEFAULT 0
);

-- ============================================================
-- PLAYER CREATE INFO
-- ============================================================

CREATE TABLE IF NOT EXISTS player_create_info (
    race        SMALLINT NOT NULL,
    class       SMALLINT NOT NULL,
    map_id      INT NOT NULL,
    position_x  REAL NOT NULL,
    position_y  REAL NOT NULL,
    position_z  REAL NOT NULL DEFAULT 0.0,
    orientation REAL NOT NULL DEFAULT 0.0,
    PRIMARY KEY (race, class)
);

-- ============================================================
-- HELPER FUNCTIONS
-- ============================================================

CREATE OR REPLACE FUNCTION cleanup_expired_sessions()
RETURNS void AS $$
BEGIN
    DELETE FROM sessions WHERE expires_at < CURRENT_TIMESTAMP;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION give_starter_items(char_id BIGINT, class_id SMALLINT)
RETURNS void AS $$
BEGIN
    CASE class_id
        WHEN 1 THEN
            INSERT INTO character_equipment (character_id, slot, template_id) VALUES (char_id, 12, 1001);
            INSERT INTO character_equipment (character_id, slot, template_id) VALUES (char_id, 13, 1020);
        WHEN 2 THEN
            INSERT INTO character_equipment (character_id, slot, template_id) VALUES (char_id, 12, 1010);
        ELSE
            INSERT INTO character_equipment (character_id, slot, template_id) VALUES (char_id, 12, 1001);
    END CASE;

    INSERT INTO character_inventory (character_id, slot, template_id)
    VALUES
        (char_id, 0, 2001),
        (char_id, 1, 2101),
        (char_id, 2, 2201),
        (char_id, 3, 2301);
END;
$$ LANGUAGE plpgsql;

-- ============================================================
-- BASELINE SEED DATA (kept identical to legacy schema.sql; 0003 extends it)
-- ============================================================

INSERT INTO race_template VALUES (1, 'Human', 0, 3, 0, 0, 0, 0)
    ON CONFLICT (id) DO NOTHING;
INSERT INTO race_template VALUES (2, 'Orc', 1, 3, 2, 0, 1, -1)
    ON CONFLICT (id) DO NOTHING;

INSERT INTO class_template VALUES (1, 'Warrior', 150, 50, 10, 5, 8, 3)
    ON CONFLICT (id) DO NOTHING;
INSERT INTO class_template VALUES (2, 'Witch', 80, 120, 3, 5, 4, 12)
    ON CONFLICT (id) DO NOTHING;

INSERT INTO map_template (id, name, width, height, spawn_x, spawn_y, spawn_z)
VALUES (1, 'Starting Zone', 100.0, 100.0, 10.0, 10.0, 0.0)
    ON CONFLICT (id) DO NOTHING;
INSERT INTO map_template (id, name, width, height, spawn_x, spawn_y, spawn_z)
VALUES (2, 'Dark Forest', 100.0, 100.0, 5.0, 5.0, 0.0)
    ON CONFLICT (id) DO NOTHING;

INSERT INTO player_create_info VALUES (1, 1, 1, 10.0, 10.0, 0.0, 0.0) ON CONFLICT (race, class) DO NOTHING;
INSERT INTO player_create_info VALUES (1, 2, 1, 10.0, 10.0, 0.0, 0.0) ON CONFLICT (race, class) DO NOTHING;
INSERT INTO player_create_info VALUES (2, 1, 1, 10.0, 10.0, 0.0, 0.0) ON CONFLICT (race, class) DO NOTHING;
INSERT INTO player_create_info VALUES (2, 2, 1, 10.0, 10.0, 0.0, 0.0) ON CONFLICT (race, class) DO NOTHING;
