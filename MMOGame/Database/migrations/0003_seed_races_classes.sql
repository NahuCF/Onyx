-- 0003_seed_races_classes.sql
-- Seed sample races, classes, and creature templates so the editor has
-- something to reference. IDs MUST stay stable — the editor's
-- RaceClassRegistry hardcodes these IDs.
--
-- Race IDs:    1=Human (from 0001), 2=Orc (from 0001), 3=Elf (added here)
-- Class IDs:   1=Warrior (from 0001), 2=Witch (from 0001),
--              3=Mage (added here), 4=Rogue (added here)
--
-- class_mask bits (low-to-high): bit 0 = Warrior, bit 1 = Witch,
--                                bit 2 = Mage,    bit 3 = Rogue.
--
-- Combos surfaced in the editor (9 total):
--   Human × {Warrior, Witch, Mage, Rogue}            (mask 15 = 0b1111)
--   Orc   × {Warrior, Rogue}                          (mask  9 = 0b1001)
--   Elf   × {Witch,   Mage, Rogue}                    (mask 14 = 0b1110)

-- ============================================================
-- New race: Elf
-- ============================================================

INSERT INTO race_template (id, name, faction, class_mask, bonus_strength, bonus_agility, bonus_stamina, bonus_intellect)
VALUES (3, 'Elf', 0, 14, -1, 2, 0, 1)
ON CONFLICT (id) DO NOTHING;

-- ============================================================
-- Update Human + Orc class_masks to expose new classes in valid combos
-- ============================================================

UPDATE race_template SET class_mask = 15 WHERE id = 1;  -- Human: all four
UPDATE race_template SET class_mask = 9  WHERE id = 2;  -- Orc:   Warrior + Rogue

-- ============================================================
-- New classes: Mage, Rogue
-- ============================================================

INSERT INTO class_template (id, name, base_health, base_mana, base_strength, base_agility, base_stamina, base_intellect)
VALUES (3, 'Mage', 90, 110, 4, 6, 5, 11)
ON CONFLICT (id) DO NOTHING;

INSERT INTO class_template (id, name, base_health, base_mana, base_strength, base_agility, base_stamina, base_intellect)
VALUES (4, 'Rogue', 110, 60, 6, 12, 6, 4)
ON CONFLICT (id) DO NOTHING;

-- ============================================================
-- Sample creature_template entries so the editor has IDs to reference.
-- These are minimal placeholders — full creature template authoring is Tier 2.
-- ============================================================

INSERT INTO creature_template (entry, name, level_min, level_max, faction, npcflag, model_id, ai_name, base_health, base_mana, armor)
VALUES
    (1,  'Werewolf',     3, 5,  10, 0, 0, 'AI_Aggressive', 80,  0, 5),
    (2,  'Forest Spider',2, 4,  10, 0, 0, 'AI_Aggressive', 50,  0, 3),
    (50, 'Shadow Lord',  10,10, 11, 0, 0, 'AI_Boss',       1500,0, 30)
ON CONFLICT (entry) DO NOTHING;
