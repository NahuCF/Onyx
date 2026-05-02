# Race & Class System Design

## Design conversation from 2026-02-19

---

## Core Insight

Race and class are **independent axes** that combine multiplicatively:

- **Race** = what you *are* — appearance, faction, small stat bonuses, racial abilities
- **Class** = what you *do* — combat role, resource type, abilities, stat growth per level
- **Race + Class** = what you *start as* — spawn location, starting gear, action bar

Not every combination is valid (e.g., Tauren Rogue doesn't exist in WoW). So you need a way to express allowed combos.

---

## Current State (before implementation)

- No race system — only `CharacterClass` (Warrior/Witch)
- All characters hardcoded to spawn at `(10, 10)` on map `'1'`
- Single `spawnPoint` per `MapTemplate`
- 2D positions only
- `give_starter_items()` SQL function exists but is never called from C++
- Stats hardcoded in `Database::CreateCharacter()` (Warrior: 150hp/50mp, Witch: 80hp/120mp)

---

## Recommended Structure

### Enums

```cpp
enum class CharacterRace : uint8_t {
    NONE    = 0,
    HUMAN   = 1,
    ORC     = 2,
    ELF     = 3,
    UNDEAD  = 4
};
constexpr uint8_t MAX_RACES = 5;

enum class CharacterClass : uint8_t {
    NONE    = 0,
    WARRIOR = 1,
    WITCH   = 2
};
constexpr uint8_t MAX_CLASSES = 3;

enum class Faction : uint8_t {
    NEUTRAL  = 0,
    ALLIANCE = 1,
    HORDE    = 2
};
```

### Race Template — what you *are*

```cpp
struct RaceTemplate {
    CharacterRace id;
    std::string   name;
    Faction       faction;
    std::string   maleModelPath;
    std::string   femaleModelPath;

    // Small flat bonuses added ON TOP of class base stats
    int32_t bonusStrength  = 0;
    int32_t bonusAgility   = 0;
    int32_t bonusStamina   = 0;
    int32_t bonusIntellect = 0;

    // Which classes this race can pick (bitmask)
    // e.g. (1 << WARRIOR) | (1 << WITCH) = both allowed
    uint32_t classMask = 0;

    // Racial ability IDs (e.g. "Blood Fury", "Escape Artist")
    std::vector<uint32_t> racialAbilities;
};
```

```sql
CREATE TABLE race_template (
    id              SMALLINT PRIMARY KEY,
    name            VARCHAR(32) NOT NULL,
    faction         SMALLINT NOT NULL DEFAULT 0,
    male_model      VARCHAR(128),
    female_model    VARCHAR(128),
    bonus_strength  SMALLINT NOT NULL DEFAULT 0,
    bonus_agility   SMALLINT NOT NULL DEFAULT 0,
    bonus_stamina   SMALLINT NOT NULL DEFAULT 0,
    bonus_intellect SMALLINT NOT NULL DEFAULT 0,
    class_mask      INT NOT NULL DEFAULT 0
);
```

### Class Template — what you *do*

```cpp
struct ClassTemplate {
    CharacterClass id;
    std::string    name;

    // Base stats at level 1 (before race bonuses)
    int32_t baseHealth;
    int32_t baseMana;
    int32_t baseStrength;
    int32_t baseAgility;
    int32_t baseStamina;
    int32_t baseIntellect;

    // Growth per level
    float healthPerLevel;
    float manaPerLevel;
    float strengthPerLevel;
    float agilityPerLevel;
    float staminaPerLevel;
    float intellectPerLevel;

    // Abilities every character of this class starts with
    std::vector<uint32_t> startingAbilities;
};
```

```sql
CREATE TABLE class_template (
    id                  SMALLINT PRIMARY KEY,
    name                VARCHAR(32) NOT NULL,
    base_health         INT NOT NULL,
    base_mana           INT NOT NULL,
    base_strength       SMALLINT NOT NULL DEFAULT 0,
    base_agility        SMALLINT NOT NULL DEFAULT 0,
    base_stamina        SMALLINT NOT NULL DEFAULT 0,
    base_intellect      SMALLINT NOT NULL DEFAULT 0,
    health_per_level    REAL NOT NULL DEFAULT 10.0,
    mana_per_level      REAL NOT NULL DEFAULT 5.0,
    strength_per_level  REAL NOT NULL DEFAULT 1.0,
    agility_per_level   REAL NOT NULL DEFAULT 1.0,
    stamina_per_level   REAL NOT NULL DEFAULT 1.0,
    intellect_per_level REAL NOT NULL DEFAULT 1.0
);
```

### Player Create Info — what you *start as* (race + class combined)

```cpp
struct PlayerCreateInfo {
    uint32_t mapId;
    float    positionX, positionY, positionZ;
    float    orientation;
};
```

```sql
CREATE TABLE player_create_info (
    race        SMALLINT NOT NULL,
    class       SMALLINT NOT NULL,
    map_id      INT NOT NULL,
    position_x  REAL NOT NULL,
    position_y  REAL NOT NULL,
    position_z  REAL NOT NULL DEFAULT 0.0,
    orientation REAL NOT NULL DEFAULT 0.0,
    PRIMARY KEY (race, class)
);
```

---

## How Stats Combine at Creation

```
Final stat = ClassTemplate.base_X + RaceTemplate.bonus_X

Example: Orc Warrior at level 1
  Health    = 150 (Warrior base)
  Strength  = 10 (Warrior base) + 3 (Orc bonus) = 13
  Intellect = 3  (Warrior base) + 0 (Orc bonus)  = 3

Example: Orc Witch at level 1
  Health    = 80  (Witch base)
  Strength  = 4   (Witch base) + 3 (Orc bonus) = 7
  Intellect = 12  (Witch base) + 0 (Orc bonus)  = 12

At level N:
  Health = base_health + health_per_level * (level - 1)
  Str    = (base_strength + race_bonus) + strength_per_level * (level - 1)
```

Race gives identity/flavor (Orcs are stronger, Elves are more agile). Class gives the role and scaling curve. Neither needs to know about the other.

---

## Validity Check

```cpp
bool IsValidRaceClass(CharacterRace race, CharacterClass cls) {
    const RaceTemplate* raceInfo = GetRaceTemplate(race);
    if (!raceInfo) return false;
    return (raceInfo->classMask & (1 << static_cast<uint8_t>(cls))) != 0;
}
```

The bitmask on the race template is the simplest approach. Adding a new class to a race is flipping one bit.

---

## Storage and Loading

```cpp
// In a DataManager or ObjectMgr-style singleton
class GameDataStore {
public:
    void LoadAll();  // called once at server startup

    const RaceTemplate*      GetRaceTemplate(CharacterRace race) const;
    const ClassTemplate*     GetClassTemplate(CharacterClass cls) const;
    const PlayerCreateInfo*  GetPlayerCreateInfo(CharacterRace race, CharacterClass cls) const;
    bool IsValidRaceClass(CharacterRace race, CharacterClass cls) const;

private:
    std::unordered_map<uint8_t, RaceTemplate>  m_RaceTemplates;
    std::unordered_map<uint8_t, ClassTemplate> m_ClassTemplates;
    std::unordered_map<uint16_t, PlayerCreateInfo> m_CreateInfo; // key = (race << 8) | class
};
```

---

## Data Ownership Summary

| Question | Answered by |
|----------|-------------|
| What faction am I? | `RaceTemplate` |
| What do I look like? | `RaceTemplate` (model paths) |
| Can this race be this class? | `RaceTemplate.classMask` |
| How much HP do I have at level 1? | `ClassTemplate.baseHealth` |
| How do my stats grow? | `ClassTemplate` (per-level values) |
| What abilities do I start with? | `ClassTemplate.startingAbilities` + `RaceTemplate.racialAbilities` |
| Where do I spawn? | `PlayerCreateInfo` (race+class key) |
| What gear do I start with? | Future `player_create_items` table (race+class key, with wildcards) |

---

## Companion Tables (add when needed)

| Table | Purpose |
|-------|---------|
| `player_create_info` | Spawn position per race/class |
| `player_create_items` | Starting items (race=0 or class=0 as wildcard) |
| `player_create_spells` | Starting abilities |
| `player_create_actions` | Default action bar layout |

---

## Changes Needed to Current Codebase

1. Add `CharacterRace` enum to `Types.h`
2. Add `race` column to `characters` table
3. Add `race` field to `C_CreateCharacter` packet, `CharacterInfo`, `CharacterData`
4. Create `race_template`, `class_template`, `player_create_info` tables in `schema.sql`
5. Create `GameDataStore` class (or extend existing data loading) to load templates at startup
6. `Database::CreateCharacter()` looks up `player_create_info` + computes stats from templates instead of hardcoding
7. Client UI needs a race selection step before class selection

---

## AzerothCore Reference

AzerothCore uses the same pattern:
- `playercreateinfo` table with `(race, class)` primary key
- `PlayerInfo` struct in `Player.h` bundles spawn position + items + spells + skills
- `_playerInfo[MAX_RACES][MAX_CLASSES]` 2D array on `ObjectMgr`
- Loaded at startup by `ObjectMgr::LoadPlayerInfo()`
- Used once in `Player::Create()`, then position is saved to `characters` table
- Key difference from our design: AzerothCore bundles everything into one struct. We keep them separate (RaceTemplate, ClassTemplate, PlayerCreateInfo) for cleaner separation.

### AzerothCore spawn locations for reference

| Race | Map | Location |
|------|-----|----------|
| Human | 0 (Eastern Kingdoms) | Northshire Valley |
| Orc / Troll | 1 (Kalimdor) | Valley of Trials (shared) |
| Dwarf / Gnome | 0 | Coldridge Valley (shared) |
| Night Elf | 1 | Shadowglen |
| Undead | 0 | Deathknell |
| Tauren | 1 | Camp Narache |
| Blood Elf | 530 (Outland) | Sunstrider Isle |
| Draenei | 530 | Ammen Vale |
| All Death Knights | 609 | Acherus (class override) |

Races can share spawn locations (Orc+Troll, Dwarf+Gnome). A class can override the race spawn (all Death Knights go to Acherus regardless of race).
