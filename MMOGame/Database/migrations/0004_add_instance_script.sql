-- Migration 0004: add instance_script column to map_template
-- Allows a MapInstance to bind a hand-coded InstanceScript singleton
-- (registered via ScriptRegistry<InstanceScript>) for per-run encounter state.

ALTER TABLE map_template
    ADD COLUMN IF NOT EXISTS instance_script VARCHAR(64) NOT NULL DEFAULT '';
