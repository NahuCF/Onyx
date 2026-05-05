#include "RaceClassRegistry.h"

namespace MMO {

namespace {

// Bit position for a class in race_template.class_mask: (classId - 1).
constexpr uint32_t ClassBit(CharacterClass cls) {
    return 1u << (static_cast<uint32_t>(cls) - 1u);
}

const std::vector<RaceDef>& BuildRaces() {
    static const std::vector<RaceDef> kRaces = {
        // Human: all four classes (Warrior + Witch + Mage + Rogue)
        { CharacterRace::HUMAN, "Human",
          ClassBit(CharacterClass::WARRIOR) | ClassBit(CharacterClass::WITCH) |
          ClassBit(CharacterClass::MAGE)    | ClassBit(CharacterClass::ROGUE) },
        // Orc: Warrior + Rogue (physical races)
        { CharacterRace::ORC, "Orc",
          ClassBit(CharacterClass::WARRIOR) | ClassBit(CharacterClass::ROGUE) },
        // Elf: Witch + Mage + Rogue (magical races, no Warrior)
        { CharacterRace::ELF, "Elf",
          ClassBit(CharacterClass::WITCH) | ClassBit(CharacterClass::MAGE) |
          ClassBit(CharacterClass::ROGUE) },
    };
    return kRaces;
}

const std::vector<ClassDef>& BuildClasses() {
    static const std::vector<ClassDef> kClasses = {
        { CharacterClass::WARRIOR, "Warrior" },
        { CharacterClass::WITCH,   "Witch" },
        { CharacterClass::MAGE,    "Mage" },
        { CharacterClass::ROGUE,   "Rogue" },
    };
    return kClasses;
}

const std::vector<RaceClassCombo>& BuildCombos() {
    static const std::vector<RaceClassCombo> kCombos = []() {
        std::vector<RaceClassCombo> out;
        for (const auto& race : BuildRaces()) {
            for (const auto& cls : BuildClasses()) {
                if (race.classMask & ClassBit(cls.id)) {
                    out.push_back({race.id, cls.id});
                }
            }
        }
        return out;
    }();
    return kCombos;
}

} // namespace

std::string RaceClassCombo::DisplayName() const {
    const RaceDef*  race = RaceClassRegistry::FindRace(raceId);
    const ClassDef* cls  = RaceClassRegistry::FindClass(classId);
    if (!race || !cls) return "(invalid)";
    return race->name + " " + cls->name;
}

const std::vector<RaceDef>&        RaceClassRegistry::Races()   { return BuildRaces(); }
const std::vector<ClassDef>&       RaceClassRegistry::Classes() { return BuildClasses(); }
const std::vector<RaceClassCombo>& RaceClassRegistry::Combos()  { return BuildCombos(); }

const RaceDef* RaceClassRegistry::FindRace(CharacterRace id) {
    for (const auto& r : Races()) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

const ClassDef* RaceClassRegistry::FindClass(CharacterClass id) {
    for (const auto& c : Classes()) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

} // namespace MMO
