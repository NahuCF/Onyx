#pragma once

#include "../../../Shared/Source/Types/Types.h"
#include <vector>
#include <algorithm>

namespace MMO {

// ============================================================
// SUMMON LIST - Tracks summoned creatures (AzerothCore-style)
// ============================================================

class SummonList {
public:
    SummonList() = default;

    void Add(EntityId summonId) {
        m_Summons.push_back(summonId);
    }

    void Remove(EntityId summonId) {
        auto it = std::find(m_Summons.begin(), m_Summons.end(), summonId);
        if (it != m_Summons.end()) {
            m_Summons.erase(it);
        }
    }

    void Clear() {
        m_Summons.clear();
    }

    bool IsEmpty() const { return m_Summons.empty(); }
    size_t Count() const { return m_Summons.size(); }

    const std::vector<EntityId>& GetAll() const { return m_Summons; }

    // Check if a specific summon is still alive (in the list)
    bool Contains(EntityId id) const {
        return std::find(m_Summons.begin(), m_Summons.end(), id) != m_Summons.end();
    }

private:
    std::vector<EntityId> m_Summons;
};

} // namespace MMO
